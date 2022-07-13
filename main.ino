#include <EEPROM.h>
#include <Key.h>
#include <Keypad.h>
#include <LiquidCrystal.h>

//Keypad setup using Keypad library
const byte rows = 4; //four rows
const byte cols = 3; //three columns
char keys[rows][cols] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'#','0','*'}
};
byte rowPins[rows] = {8, 9, 10, 11}; //connect to the row pinouts of the keypad
byte colPins[cols] = {14, 13, 12}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );

//LCD setup
const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//Buttons
const int8_t up=15, down=16, enter=17, back=18, greenLED=1, redLED=19;

//Stores entered password digit by digit (Default password is 0000)
byte pin[4] = {};

//This array stores the addresses of passwords in EEPROM starting from address 16
//max=24 passwords, values entered from EEPROM, 0=free >0=used
int16_t passArray[24];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  lcd.begin(16, 2);
  
  //Buttons (pulldown resistor)
  pinMode(up, INPUT);
  pinMode(down, INPUT);
  pinMode(enter, INPUT);
  pinMode(back, INPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);

  //Initial Screen
  EnterPin();
  
  //Compare pin with EEPROM pin
  while(1){   //FIX UNNECCESSARY LOOP
    if(!checkPin()){
      lcd.clear();
      lcd.print("Wrong pin");
      delay(1000);
      EnterPin();
    }
    else if(checkPin()){
      lcd.clear();
      lcd.setCursor(4,0);
      lcd.print("Welcome");
      delay(2000);
      updatePassArray(); //Initially loads values from EEPROM into the array passArray
      Menu(); //Enters the Menu
    }
   }
}
 

void loop() {
  //Not used
}

String menu[4] = {"Change pin", "Store Pass", "View Pass", "Delete Pass"};
int selected = 0;

void Menu(){
  //Initial screen
  updateMenuLCD();

  //Scrolling functions up/down
  while(1){
    if(digitalRead(down)){
      selected++;
      if(selected==4){selected=0;}
      updateMenuLCD();
    }
    else if(digitalRead(up)){
      selected--;
      if(selected==-1){selected=3;}
      updateMenuLCD();
    }


    //Entering options
    if(digitalRead(enter)){
       if(selected==0){changePin();}  //Change pin
       else if(selected==1){storePass();}  //Store Password
       else if(selected==2){viewPass();} //View Password
       else if(selected==3){deletePass();} //Delete Password
     }
  }
}

//Prints menu to array while perserving last state of menu
void updateMenuLCD(){
  lcd.clear();
  lcd.print(menu[selected]);
  lcd.setCursor(12,0);
  lcd.print(" <--");
  lcd.setCursor(0,1);
  if(selected==3){
    lcd.print(menu[0]); 
  }else{
    lcd.print(menu[selected+1]);  
  }
  delay(300);
  return;
}

//Asks user to enter pin then stores it
void EnterPin(){
  lcd.clear();
  lcd.print("Enter Pin(****):");
  lcd.setCursor(0,1);
  int i=0;
  while(!digitalRead(enter)){
     byte temp = (keypad.getKey() - '0');
     if(digitalRead(back)){ //Allow user to delete entered digits (backspace)
        delay(200);
        lcd.clear();
        lcd.print("Enter Pin(****):");
        lcd.setCursor(0,1);
        int j = 0;
        while(j<i-1){lcd.print("*"); j++;}
        if(i>0){i--;}
      }
     if(temp<10 && i<4){ //Allow user to only enter 4 numbers
      pin[i]=temp;
      delay(100);
      lcd.print("*");
      i++;
     }
   }
    //once 4 digits are entered wait for user to press enter
}

//Checks if the correct pin has been entered, returns true if correct, false if wrong
bool checkPin(){
  for(int i=0; i<4; i++){
    if(pin[i]!=EEPROM.read(i)){
      for(int i=0; i<2; i++){ //blink red led if wrong pin entered
        digitalWrite(redLED, HIGH);
        delay(300);
        digitalWrite(redLED, LOW);
        delay(300);
      }
      return false;
    }
    else if(i==3){
      for(int i=0; i<2; i++){ //blink green led if wrong pin entered
        digitalWrite(greenLED, HIGH);
        delay(300);
        digitalWrite(greenLED, LOW);
        delay(300);
      }
      return true;
    }
   }  
}

//Changes the pin stored in the EEPROM, asks to enter current pin first
void changePin(){
  while(1){
    delay(300);
    EnterPin();

    if(checkPin()){
        //Enter new pin
        lcd.clear();
        lcd.print("New pin(****):");
        lcd.setCursor(0,1);
        int i=0;
        while(i<4){
          pin[i] = (keypad.waitForKey() - '0');
          if(pin[i]<10){
            lcd.print("*");
           i++;
          }
        }
        while(!digitalRead(enter));
       
        //Store new pin in EEPROM
        for(int i=0; i<4; i++){
          EEPROM.write(i, pin[i]);
        }
        delay(300);
        Menu();
    }
    else if(!checkPin()){
      lcd.clear();
      lcd.print("Wrong pin!");
      delay(500);
      while(1){
        if(digitalRead(enter)){break;}
        if(digitalRead(back)){updateMenuLCD(); return;}
      }
    } 
  }
}

//Displays stored passwords from EEPROM
void viewPass(){
  
  //Array that stores indexes where(address) a password is present in passArray
  //All elements set to -1 by default
  int8_t indexes[24];
  memset(indexes, -1, 24 * sizeof(indexes[0])); //Set all elements to -1
  
  int8_t j = 0; //Counter for inserting into the indexes array 
  for(int i=0; i<24; i++){
    if(passArray[i]>0){
      indexes[j] = passArray[i];
      j++;
    }
  }
  
  //If no passwords are stored display message
  if(indexes[0] == -1){
    lcd.clear();
    lcd.setCursor(2,0);
    lcd.print("No passwords");
    lcd.setCursor(5,1);
    lcd.print("stored");
    while(!digitalRead(back)){} //Wait for user to press back then return to Menu
    updateMenuLCD();
    return;
  }

  int8_t add = 0;
  int16_t address = indexes[add];
  while(1){  //Display passwords till exit by pressing back
    lcd.clear();
    //Display pass name
    for(int i=0; i<16; i++){
      lcd.print((char)EEPROM.read(address));
      address++;
    }
    lcd.setCursor(0,1);
    //Display password
    for(int i=0; i<16; i++){
      lcd.print((char)EEPROM.read(address));
      address++;
    }

    while(1){
      if(digitalRead(back)){updateMenuLCD(); return;}
      if(digitalRead(down)){
        add++;
        if((indexes[add]==-1) || (add>23)){
          add=0;
        }
        address = indexes[add];
        delay(300);
        break;
      }
      if(digitalRead(up)){
        add--;
        if(indexes[add]==-1){add++;}
        if(add<0){
          add=23;
          while(1){
            if(indexes[add]==-1){add--;}
            else{break;}
          }
        }
        address = indexes[add];
        delay(300);
        break;
        }
      }
    }
}


//Stores new password in EEPROM
void storePass(){
  delay(300);
  int index = -1;   //flag
  for(int i=0; i<24; i++){
    if(passArray[i]==0){
      index=i;
      break;
    }
  }
  
  if(index==-1){
    lcd.clear();
    lcd.setCursor(2,0); 
    lcd.print("Memory full");
    lcd.setCursor(0,1); 
    lcd.print("Delete a pass");
    while(!digitalRead(enter));
    delay(300);
    updateMenuLCD();
    return;
  }
  else{
    //address to write to
    int16_t address = (16+(32*index));
    
    //Enter name of new pass
    String nameInput;
    lcd.clear();
    lcd.print("Pass Name:");
    while(!digitalRead(enter)){
        if(digitalRead(back)){updateMenuLCD(); return;} 
    }
    delay(300);
    nameInput = Serial.readString();
    //Store name in EEPROM
    for(int i=0; i<16; i++){
      if(!nameInput.charAt(i)==0){    //Only store string in EEPROM if it is valid (0 is NULL cannot be displayed on LCD)(0 replaced with 32 which is space in ASCII)
        EEPROM.update(address, nameInput.charAt(i));
      }else{
        EEPROM.update(address, 32);
      }
      address++;
    }

    //Enter pass
    String passInput;
    lcd.clear();
    lcd.print("Enter pass:");
    while(!digitalRead(enter)){
        if(digitalRead(back)){updateMenuLCD(); return;} 
    }
    passInput = Serial.readString();
    //Store pass in EEPROM
    for(int i=0; i<16; i++){
      if(!passInput.charAt(i)==0){    //Only store string in EEPROM if it is valid (0 is NULL cannot be displayed on LCD)(0 replaced with 32 which is space in ASCII)
        EEPROM.update(address, passInput.charAt(i));
      }else{
        EEPROM.update(address, 32);
      }
      address++;
    }
              
    address = (16+(32*index));    //return address to first address written to
    //Change address to value to be written to EEPROM
    //Lookup table
    byte temp;
    if(address==16 || address==272 || address==528){temp=0b10000000;} //0
    else if(address==48 || address==304 || address==560){temp=0b01000000;}  //1
    else if(address==80 || address==336 || address==592){temp=0b00100000;}  //2
    else if(address==112 || address==368 || address==624){temp=0b00010000;}  //3
    else if(address==144 || address==400 || address==656){temp=0b00001000;} //4
    else if(address==176 || address==432 || address==688){temp=0b00000100;} //5
    else if(address==208 || address==464 || address==720){temp=0b00000010;} //6
    else if(address==240 || address==496 || address==752){temp=0b00000001;} //7

    
    //Update EEPROM array
    if(index<=7){ //Address 4
      byte x = EEPROM.read(4);
      EEPROM.update(4, (x | temp)); //Only overwrites bits that needs to be changed
    }else if(index <=15){ //Address 5
      byte x = EEPROM.read(5);
      EEPROM.update(5, (x | temp)); //Only overwrites bits that needs to be changed
    }else if(index<=23){ //Address 6
      byte x = EEPROM.read(6);
      EEPROM.update(6, (x | temp)); //Only overwrites bits that needs to be changed
    }
    updatePassArray(); //Update the array after insertion of new password is complete
    updateMenuLCD();
    return;
  }
}

//Allows the user to delete a password from the EEPROM
void deletePass(){

  //Array that stores indexes where(address) a password is present in passArray
  //All elements set to -1 by default
  int8_t indexes[24];
  memset(indexes, -1, 24 * sizeof(indexes[0])); //Set all elements to -1
  
  int8_t j = 0; //Counter for inserting into the indexes array 
  for(int i=0; i<24; i++){
    if(passArray[i]>0){
      indexes[j] = passArray[i];
      j++;
    }
  }
  
  //If no passwords are stored display message
  if(indexes[0] == -1){
    lcd.clear();
    lcd.setCursor(2,0);
    lcd.print("No passwords");
    lcd.setCursor(5,1);
    lcd.print("stored");
    while(!digitalRead(back)){} //Wait for user to press back then return to Menu
    updateMenuLCD();
    return;
  }

  int8_t add = 0;
  int16_t address = indexes[add];
  while(1){  //Display passwords till exit by pressing back
    lcd.clear();
    //Display pass name
    for(int i=0; i<16; i++){
      lcd.print((char)EEPROM.read(address));
      address++;
    }
    lcd.setCursor(0,1);
    //Display password
    for(int i=0; i<16; i++){
      lcd.print((char)EEPROM.read(address));
      address++;
    }
    delay(300);
    while(1){
      if(digitalRead(back)){updateMenuLCD(); return;}
      if(digitalRead(down)){
        add++;
        if((indexes[add]==-1) || (add>23)){
          add=0;
        }
        address = indexes[add];
        delay(300);
        break;
      }
      if(digitalRead(up)){
        add--;
        if(indexes[add]==-1){add++;}
        if(add<0){
          add=23;
          while(1){
            if(indexes[add]==-1){add--;}
            else{break;}
          }
        }
        address = indexes[add];
        delay(300);
        break;
      }
      if(digitalRead(enter)){
        lcd.clear();
        lcd.print("Are you sure?");
        
        //Wait for enter or back
        delay(300);
        bool flag = false;
        while(!digitalRead(enter)){
          if(digitalRead(back)){flag=true; break;}
        }
        delay(200);
        address=indexes[add];
        if(flag==true){break;}

        //Enter is pressed, overwrite pass with 0's, then remove record
        //write 0's
        for(int i=0; i<32; i++){
          EEPROM.update(address, (int)0);
          address++;
        }
        //Remove record from EEPROM
        address = indexes[add];
        byte temp;
        if(address==16 || address==272 || address==528){temp=0b01111111;} //0
        else if(address==48 || address==304 || address==560){temp=0b10111111;}  //1
        else if(address==80 || address==336 || address==592){temp=0b11011111;}  //2
        else if(address==112 || address==368 || address==624){temp=0b11101111;}  //3
        else if(address==144 || address==400 || address==656){temp=0b11110111;} //4
        else if(address==176 || address==432 || address==688){temp=0b11111011;} //5
        else if(address==208 || address==464 || address==720){temp=0b11111101;} //6
        else if(address==240 || address==496 || address==752){temp=0b11111110;} //7        
        
        if(address<=240){
          byte x = EEPROM.read(4);
          EEPROM.update(4, (x & temp));
        }else if(address<=496){
          byte x = EEPROM.read(5);
          EEPROM.update(5, (x & temp));
        }else if(address<=752){
          byte x = EEPROM.read(6);
          EEPROM.update(6, (x & temp));
        }
        updatePassArray();
        //exit
        updateMenuLCD();
        return;
      }
    } 
  }
}

//Loads values from EEPROM into the array passArray
//addresses 4,5, and 6 in the EEPROM are used to determine locations of the passwords in the EEPROM
void updatePassArray(){
  byte data=0; //temp storage
  data=EEPROM.read(4);
  for(int i=7; i>=0; i--){
    passArray[i] = (16+(32*i))*(data%2);
    data = data/2;
  }
  data=EEPROM.read(5);
  for(int i=15; i>=8; i--){
    passArray[i] = (16+(32*i))*(data%2);
    data = data/2;    
  }
  data=EEPROM.read(6);
  for(int i=23; i>=16; i--){
    passArray[i] = (16+(32*i))*(data%2);
    data = data/2;    
  }
}
