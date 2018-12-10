// Automated BioCube 
// by Oscar Gutierrez
// (Started Jan 2017) - (UPDATED Aug 2018)
// RTC (Real Time Clock) using DS3231.h
// RTC controls lighting RGB Strip & NeoPixel Ring 
// RTC controls feeder(servo)
// BioCube outer switches to override RTC Lighting
// 
// **                     Work in Progress                       **
// ** Reading thermosistor from inside/outside BioCube           **
// ** Displays external/internal water temperature on LCD display**

// DS3231_Serial_Hard
// Copyright (C)2015 Rinky-Dink Electronics, Henning Karlsen. All right reserved
// web: http://www.RinkyDinkElectronics.com/
// R G B   F A D E 
// April 2007, Clay Shirky <clay.shirky@nyu.edu> 

#ifdef   __AVR__
#include <avr/power.h>
#endif
#include <DS3231.h>  
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#define  DS3231_I2C_ADDRESS 0x68

/// / / / / / / / / / / / / / / / / / / / / / // C O N S T A N T S 
#define  RING_PIN 6
#define  SERVO_PIN 10
#define  SWITCH1_PIN 11
#define  SWITCH2_PIN 12
#define  THERMO_PIN A0                                                               
#define  NUMPIXELS 52
                  
/// / / / / / / / / / / / / / / / / / / / / / // I N S T A N T I A T I O N  O F  C O M P O N E N T S              
DS3231              Clock;                    // 
LiquidCrystal_I2C   lcd(0x3f, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
Servo               servo;
Adafruit_NeoPixel   ring = Adafruit_NeoPixel(NUMPIXELS, RING_PIN, NEO_GRB + NEO_KHZ800);

int numFeed       =       2;           //  S E R V O
int pos           =       0;           // 0 degrees
int endPos        =      20;           // 180 degrees

float hoodTemp      =       0;         // init val
                                
int switch1State ;                     //  S W I T C H E S
int switch2State ;                     // set to 0 and 1 to 
int switch1Prev  ;                     // allow switchState()
int switch2Prev  ;                     // to work

boolean autoLightCycle = true;         //  L I G H T   C Y C L E   P R O G R A M 
int prevCommand;
                                                                                       
int wait        =   500;               // R G B   R I N G   V A L U E S 
                
int black[3]    = {   0,   0,   0 };   // Color arrays
int white[3]    = { 245, 245, 255 };
int red[3]      = { 255,   0,   0 };
int green[3]    = {   0, 255,   0 };
int blue[3]     = {   0,   0, 255 };
int purple[3]   = { 200,   0, 255 };
int dimPurp[3]  = {  15,   0,  15 };
int dimBlue[3]  = {   0,   0,  25 };
int dimWhite[3] = {  25,  25,  25 };                                          
                                              
bool Century=false;                        //  C L O C K
bool h12;
bool PM;
byte ADay, AHour, AMinute, ASecond, ABits;
bool ADy, A12h, Apm;
byte year, month, date, DoW, hour, minute, second;
int  cSecond, cMinute, cHour, cDayOfWeek, cDayOfMonth, cMonth, cYear;
                                                 
bool timeToFeed      = false;             //  F E E D E R 
bool hasBeenFed      = false;
int  lastTimeFed;
int  feedCount; 
                                     
/// / / / / / / / / / / / / / / / / / / / / / // S E T U P
void setup() {                                //
  Wire.begin();                               //
  Serial.begin(57600);                        // Setup Serial connection
  pinMode(SWITCH1_PIN, INPUT);                // Switch 1
  pinMode(SWITCH2_PIN, INPUT);                // Switch 2
  lcd.begin(16, 2);                           // LCD Begin
  lcd.clear();
  lcd.backlight();
  ring.begin();
  ring.show();                                // Initialize all pixels to 'off'

  // feedCount   = 0;                         // Reset Feed Count After Refilling Feed Tank
                                              // The following lines can be uncommented to set the date and time
  //setDS3231time(0,47,23,2,13,3,18);           // DS3231 seconds, minutes, hours, day, date, month, year
}                                             // E N D   S E T U P


// / / / / / / / / / / / / / / / / / / / / / / / L O O P
void loop() {                                 
  switchState();                              // C H E C K   S W I T C H   S T A T E
  updateDisplay();                            // U P D A T E   L C D   D I S P L A Y
  //isItTimeToFeed();                           // Is It Time To Feed? Check.
  //Serial.print(cHour);Serial.print(":");Serial.print(cMinute);// Print Debug
  Serial.print(cHour);Serial.print(":");Serial.print(cMinute);Serial.println();
}                                             // E N D   L O O P

// / / / / / / / / / / / / / / / / / / / / / / / 
void updateDisplay() {                        //  U P D A T E   D I S P L A Y
                                              //  lcd.print("Hood Temp: ");
                                              //  hoodTemp = analogRead(THERMO_PIN);
                                              //  hoodTemp =(hoodTemp/1024)*1.8+32;
                                              //  lcd.print(hoodTemp);  
  updateTime();      
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Time: ");
  lcd.print(cHour);  
  lcd.print(":");
  if (cMinute<10){
    lcd.print("0");
  }
  lcd.print(cMinute);
  delay(2000);
  lcd.clear();
  lcd.print("Times Fed: ");
  lcd.print(feedCount);
  lcd.setCursor(0,1);
  lcd.print("Next Feeding: ");
  lcd.print("null");
  delay(2000);
  lcd.clear();
  lcd.clear();
  lcd.print("AutoLighting");
  lcd.setCursor(0,1);
  if (autoLightCycle){
    lcd.print("ON");
  }else{
    lcd.print("OFF");
  }
  delay(2000);
  lcd.clear();
  lcd.backlight();
}
/// / / / / / / / / / / / / / / / / / / / / / // L E D  S W I T C H   T O G G L E   < < < < < < < < < < < BUG
void switchState(){
  switch1State = digitalRead(SWITCH2_PIN);    // OutSide Switch Check flipped 
  switch2State = digitalRead(SWITCH1_PIN);    
  if (switch1State == HIGH) {                 // Switch States
    Serial.println("SWITCH1 ON HIGH");        // Print Debug   
    if (switch2State == HIGH) {               // 1 1 - AUTO LIGHT CYCLE 
      Serial.println("SWITCH2 ON HIGH");      // Print Debug
      autoLightCycle = true;
      switchesToggle(6);                      
    } else {                                  // 1 0 - PARTY MODE
      Serial.println("SWITCH1 ON LOW");
      autoLightCycle = false;
      switchesToggle(3);                      
    }
  } else{
    autoLightCycle = false;                      
    if (switch2State == HIGH) {               // 0 1 - BRIGHT MODE
      Serial.println("SWITCH2 ON HIGH");      // Print Debug
      switchesToggle(4);                      
    } else {                                  // 0 0 - DIM MODE
      Serial.println("SWITCH1 ON LOW");
      Serial.println("SWITCH2 ON LOW");
      switchesToggle(2);                       
    }
  }
}
                                              // L I G H T  S W I T C H  T O G G L E R 
void switchesToggle(int switchCommand) {      
  if (prevCommand != switchCommand || switchCommand == 6 ||  switchCommand == 3) {
    lcd.clear();
    switch (switchCommand) {
      case 1:                                 // L I G H T S   O F F  ( 2  L E D S   O N )
        lcd.print("Night-Lighting");
        nightLighting();
        break;
      case 2:                                 // D I M   B L U E  /  P U R P 
        lcd.print("Dim Blue/Purple");
        twoColorDim(dimBlue,dimPurp);
        break;
      case 3:                                 // P A R T Y   M O D E
        lcd.print("Party!!!!!!!! ");
        theaterChaseRainbow(wait); 
        break;
      case 4:                                 // B R I G H T   W H I T E / P U R P L E
        lcd.print("Bright Mode");
        changeBrightColor();
        break;          
      case 5:                                 // D I M   W H I T E  /  P U R P L E                                  
        lcd.print("Dim White / Purp");
        twoColorDim(dimWhite, dimPurp);
        break;
      case 6:                                 // S E T  A U T O L I G H T 
        lcd.print("Auto Light Cycle");             
        autoLightCycler();
        break;
    }                                         // T O D O   C A L L   L I G H T I N G   E F F E C T   F U N C T I O N 
  }                                           
  lcd.clear();
  prevCommand = switchCommand;
}                                             
void autoLightCycler(){                       // A U T O - L I G H T I N G   F U N C T I O N 
  updateTime();
  if(cHour >= 0  && cHour < 5){               // Turn off Lights   |  12a - 5a  |  5 hrs of off
    switchesToggle(1);  
  }else if(cHour >=   5 && cHour < 8){        // Turn on Dim |  5a - 8a   |  3 hrs of dim
    switchesToggle(5); 
  }else if(cHour >=   8 && cHour < 22){       // Turn on Bright    |  8a - 10p  | 14 hrs of bright
    switchesToggle(4); 
  }else if(cHour >=  22 && cHour < 24){       // Turn on Dim | 10p - 12a  |  2 hrs of dim
    switchesToggle(5);  
  }                                           // 5 hrs off  | 5 hrs dim  | 14 hrs on
}


/// / / / / / / / / / / / / / / / / / / / / / // S E R V O   A C T I V A T I O N                                               
void servoFeed(){                             // F E E D   F U N C T I O N 
  servo.attach(SERVO_PIN);                              
  delay(15);                                  
  for(int i = 0; i<=numFeed;i++){
    for (pos = 0; pos <= 180; pos += 1) {       // goes from 0 degrees to 180 degrees in steps of 1 degree
      servo.write(pos);                         // tell servo to go to position in variable 'pos'
      delay(15);                                // waits 15ms for the servo to reach the position
    }
    for (pos = 180; pos >= 0; pos -= 1) {       // goes from 180 degrees to 0 degrees
      servo.write(pos);                         // tell servo to go to position in variable 'pos'
      delay(15);                                // waits 15ms for the servo to reach the position
    }
  } 
  servo.detach();
  void theaterChaseRainbow();
}                                             


/// / / / / / / / / / / / / / / / / / / / / / // D S 3 2 3 1    C L O C K
void updateTime(){                            // G E T   T I M E 
  cSecond     =Clock.getSecond();
  cMinute     =Clock.getMinute();
  cHour       =Clock.getHour(h12, PM);
  cDayOfWeek  =Clock.getDate();
  cMonth      =Clock.getMonth(Century);
  cYear       =Clock.getYear();
}

/// / / / / / / / / / / / / / / / / / / / / / // S E T  C L O C K  T I M E 
void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek,byte dayOfMonth, byte month, byte year){
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0);                              // set next input to start at the seconds register
  Wire.write(decToBcd(second));               // set seconds
  Wire.write(decToBcd(minute));               // set minutes
  Wire.write(decToBcd(hour));                 // set hours
  Wire.write(decToBcd(dayOfWeek));            // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth));           // set date (1 to 31)
  Wire.write(decToBcd(month));                // set month
  Wire.write(decToBcd(year));                 // set year (0 to 99)
  Wire.endTransmission();
}
byte decToBcd(byte val){                                                  
  return( (val/10*16) + (val%10) );           // Convert normal decimal numbers to binary coded decimal
}
byte bcdToDec(byte val){                      // Convert binary coded decimal to normal decimal numbers
  return( (val/16*10) + (val%16) );
}
void readDS3231time(byte *second,byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year){
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0);                              // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);    // request seven bytes of data from DS3231 starting from register 00h
  *second     = bcdToDec(Wire.read() & 0x7f);
  *minute     = bcdToDec(Wire.read());
  *hour       = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());                                                       
}                                             


// / / / / / / / / / / / / / / / / / / / / / / 
//    R G B  R I N G  F U N C T I O N S    / /
// / / / / / / / / / / / / / / / / / / / / / / 


void twoColorDim(int color[3], int color2[3]) {
  // 1ST, 2ND, 3RD LEDS ARE WHITE
  // 4TH, 5TH      LEDS ARE COLOR 
  ring.begin();
  ring.show(); 
  for(int i=0;i<NUMPIXELS;i+=5){                          
    ring.setPixelColor(i, ring.Color(15,15,15));          // DIM WHITE
    delay(wait);
    ring.show();
    ring.setPixelColor(i+1, ring.Color(15,15,15));        // DIM WHITE
    delay(wait);
    ring.show();
    ring.setPixelColor(i+2, ring.Color(15,15,15));        // DIM WHITE
    delay(wait);
    ring.show();
    ring.setPixelColor(i+3, ring.Color(15,15,15));        // DIM WHITE
    delay(wait);
    ring.show();
    ring.setPixelColor(i+4, ring.Color(color[0],color[1], color[2]));   // COLOR 1
    delay(wait);
    ring.show();
    ring.setPixelColor(i+5, ring.Color(color2[0], color2[1], color2[2] ) );   // COLOR 2
    delay(wait);
    ring.show();
  }
}

// TWO PIXELS ON LOW OVERNIGHT
void nightLighting() {
  //one pixel mode
  ring.begin();
  ring.show(); 
  for(int i=0;i<NUMPIXELS;i++){
      ring.setPixelColor(i, ring.Color(0,0,0));   // TURN ALL PIXELS OFF ONE BY ONE W/ DELAY
      delay(wait);
      ring.show();
  }
  ring.setPixelColor(10, ring.Color(15,15,15));
  delay(wait);
  ring.setPixelColor(50, ring.Color(15,15,15));        
  ring.show();                                  // This sends the updated pixel color to the hardware.
}

// TURN ON BRIGHT MODE ONE PIXEL AT A TIME W/ DELAY
void changeBrightColor() {
  ring.begin();
  ring.show(); 
  for(int i=0;i<NUMPIXELS;i+=5){               // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    ring.setPixelColor(i, ring.Color(175,222,175));     // red & blue
    delay(wait);
    ring.show();
    ring.setPixelColor(i+1, ring.Color(175,255,175));     // red & blue
    delay(wait);
    ring.show();
    ring.setPixelColor(i+2, ring.Color(75,0,255));      // blue
    delay(wait);
    ring.show();
    ring.setPixelColor(i+3, ring.Color(225,175,225));   // white
    delay(wait);
    ring.show();
    ring.setPixelColor(i+4, ring.Color(225,175,225));   // white
    delay(wait);
    ring.show();
  }
}

// P A R T Y  M O D E 
void theaterChaseRainbow(uint8_t wait) {      // Theatre-style crawling lights with rainbow effect
  for (int j = 0; j < 256; j++) {             // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (uint16_t i = 0; i < ring.numPixels(); i = i + 2) {
        ring.setPixelColor(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
      }
      ring.show();
//      for (uint16_t i = 0; i < ring.numPixels(); i = i + 2) {
//        ring.setPixelColor(i + q, 0);         // turn every third pixel off
//      }
      
      delay(wait);
    }
  }
}  
// P A R T Y  M O D E  H E L P E R 
uint32_t Wheel(byte WheelPos) {               // Input a value 0 to 255 to get a color value.
  WheelPos = 255 - WheelPos;                  // The colours are a transition r - g - b - back to r.
  if (WheelPos < 85) {
    return ring.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return ring.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return ring.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}                                              // P A R T Y  M O D E  E N D 
