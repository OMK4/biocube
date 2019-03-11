// Automated BioCube 
// by Oscar Gutierrez
// (Started Jan 2017)  
// (Updates Mar 2019)
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
#define  NUMPIXELS 40
                  
/// / / / / / / / / / / / / / / / / / / / / / // I N S T A N T I A T I O N  O F  C O M P O N E N T S              
DS3231              Clock;                    // 
LiquidCrystal_I2C   lcd(0x3f, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
Servo               servo;
Adafruit_NeoPixel   ring = Adafruit_NeoPixel(NUMPIXELS, RING_PIN, NEO_GRB + NEO_KHZ800);

int switch1State ;                     // S W I T C H E S
int switch2State ;                     // 
int prevMode;                                                                                  
int wait               = 250;
boolean autoLightCycle = true;         // L I G H T   C Y C L E   P R O G R A M 
                                          
bool Century           =false;         // C L O C K
bool h12;
bool PM;
byte ADay, AHour, AMinute, ASecond, ABits;
bool ADy, A12h, Apm;
byte year, month, date, DoW, hour, minute, second;
int  cSecond, cMinute, cHour, cDayOfWeek, cDayOfMonth, cMonth, cYear;
                                                 
int rgbIntensity = 1;
int prevMinute;

uint32_t black    = ring.Color(0, 0, 0);  // R G B   R I N G   V A L U E S in ms                               
uint32_t white    = ring.Color(200, 200, 200);
uint32_t white2   = ring.Color(200, 200, 75);
uint32_t red      = ring.Color(122, 0, 0);
uint32_t green    = ring.Color(0, 122, 0);
uint32_t blue     = ring.Color(0, 0, 122);
uint32_t dimPurp  = ring.Color(12,0,12);
uint32_t dimBlue  = ring.Color(0, 0, 25);
uint32_t dimWhite = ring.Color(5, 5, 5);

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
                                              // The following lines can be uncommented to set the date and time
  //setDS3231time(0,47,23,2,13,3,18);         // DS3231 seconds, minutes, hours, day, date, month, year
}    

// / / / / / / / / / / / / / / / / / / / / / / / L O O P
void loop() {                                 
  switchState();                              // C H E C K   S W I T C H   S T A T E
  updateDisplay();                            // U P D A T E   L C D   D I S P L A Y
  // Serial.print(cHour);Serial.print(":");Serial.print(cMinute);// Print Debug
  Serial.print(cHour);Serial.print(":");Serial.print(cMinute);Serial.println();
} 

// / / / / / / / / / / / / / / / / / / / / / /  U P D A T E   D I S P L A Y 
void updateDisplay() {                        
  updateTime();      
  lcd.clear(); lcd.setCursor(0, 1);
  lcd.print("Time: "); lcd.print(cHour); lcd.print(":");
  if (cMinute<10){
    lcd.print("0");
  }
  lcd.print(cMinute); delay(2000); lcd.clear();
  lcd.clear(); lcd.print("AutoLighting"); lcd.setCursor(0,1);
  if (autoLightCycle){
    lcd.print("ON");
  }else{
    lcd.print("OFF");
  }
  delay(2000); lcd.clear(); lcd.backlight();
}

/// / / / / / / / / / / / / / / / / / / / / / // L E D  S W I T C H   T O G G L E   
void switchState(){
  switch1State = digitalRead(SWITCH2_PIN);    // OutSide Switch Check flipped 
  switch2State = digitalRead(SWITCH1_PIN);    
  if (switch1State == HIGH) {                 // Switch States
    Serial.println("SWITCH1 ON HIGH");        // Print Debug   
    if (switch2State == HIGH) {               // 1 1 - AUTO LIGHT CYCLE 
      Serial.println("SWITCH2 ON HIGH");      // Print Debug
      autoLightCycle = true;
      lightMode(5);                      
    } else {                                  // 1 0 - PARTY MODE
      Serial.println("SWITCH2 ON LOW");
      autoLightCycle = false;
      lightMode(3);                      
    }
  } else{
    autoLightCycle = false; 
    if (switch2State == HIGH) {               // 0 1 - BRIGHT MODE
      Serial.println("SWITCH2 ON HIGH");      // Print Debug
      lightMode(4);                      
    } else { 
      Serial.println("SWITCH1 ON LOW");       // 0 0 - DIM MODE                   
      Serial.println("SWITCH2 ON LOW");
      lightMode(2);                       
    }
  }
}

// / / / / / / / / / / / / / / / / / / / / / / A U T O   L I G H T   C Y C L E R
void autoLightCycler(){
  if(cHour >= 0  && cHour < 7){                // Turn off Lights       7hrs OFF
    lightMode(1);
  }else if( cHour>=7 && cHour<13 ){            // Turn on Dim           6hrs FADE UP
    lightMode(2);                                                
  }else if( cHour>=13 && cHour<17 ){           // Keep Full Brightness  4hrs BRIGHT
    lightMode(4);                                                             
  }else if( cHour>=17 && cHour<24 ){           // Turn on Bright        7hrs FADE OUT
    lightMode(2); 
  } 
}  
                                              // L I G H T  S W I T C H  T O G G L E R 
void lightMode(int mode) {      
  if (prevMode != mode || mode == 5 ||  mode == 3) {
    lcd.clear();
    switch (mode) {
      case 1:                                 // L I G H T S   O F F  ( 2  L E D S   O N )
        lcd.print("Night-Lighting");
        nightLighting();
        break;
      case 2:                                 // D I M   B L U E  /  P U R P 
        lcd.print("Dim Blue/Purple");
        if(cHour<12)
          lightFade();
        else
          lightFade();
        break;
      case 3:                                 // P A R T Y   M O D E
        lcd.print("Party Mode ON!");
        theaterChaseRainbow(wait); 
        break;
      case 4:                                 // B R I G H T   W H I T E / P U R P L E
        lcd.print("Bright Mode");
        changeBrightColor();
        break;          
      case 5:                                 // S E T  A U T O L I G H T 
        lcd.print("Auto Light Cycle");             
        autoLightCycler();
        break;
    }                                         // T O D O   C A L L   L I G H T I N G   E F F E C T   F U N C T I O N 
  }                                           
  lcd.clear();
  prevMode = mode;
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

// C H A N G E   L E D   I N T E N S I T Y   B A S E D   O N   T I M E   O F   D A Y 
void lightFade() {
  if      (cHour == 11 || cHour == 19){
    rgbIntensity = 255 * .5;
  }else if(cHour == 10 || cHour == 20){
    rgbIntensity = 255 * .25;
  }else if(cHour == 9  || cHour == 21){
    rgbIntensity = 255 * .125;
  }else if(cHour == 8  || cHour == 22){
    rgbIntensity = 255 * .0625;
  }else if(cHour == 7  || cHour == 23){
    rgbIntensity = 255 * .03125;
  }
  ring.begin(); ring.show(); 
  for(int i=0;i<NUMPIXELS;i+=4){               // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    ring.setPixelColor(i  , ring.Color(rgbIntensity, rgbIntensity, rgbIntensity));// White
    delay(wait); ring.show();
    ring.setPixelColor(i+1, ring.Color(0, 0, rgbIntensity));     // R
    delay(wait); ring.show();
    ring.setPixelColor(i+2, ring.Color(0, rgbIntensity, 0));     // G
    delay(wait); ring.show();
    ring.setPixelColor(i+3, ring.Color(rgbIntensity, 0, 0));     // B
    delay(wait); ring.show();
  }
}

// T W O   P I X E L S   O N   D I M   O V E R N I G H T
void nightLighting() {
  //one pixel mode
  ring.begin(); ring.show(); 
  for(int i=0;i<NUMPIXELS;i++){
    ring.setPixelColor(i, black);           // TURN ALL PIXELS OFF ONE BY ONE W/ DELAY
    delay(wait); ring.show();
  }
  ring.setPixelColor(10, dimWhite);
  ring.setPixelColor(50, dimWhite);        
  delay(wait); ring.show();                 // This sends the updated pixel color to the hardware.
}

// T U R N   O N   B R I G H T   M O D E   O N E   P I X E L   A T   A   T I M E   W /   D E L A Y
void changeBrightColor() {
  ring.begin(); ring.show(); 
  for(int i=0;i<NUMPIXELS;i+=5){               // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    ring.setPixelColor(i, red);        // R
    delay(wait); ring.show();
    ring.setPixelColor(i+1, green);    // G
    delay(wait); ring.show();
    ring.setPixelColor(i+2, blue);     // B
    delay(wait); ring.show();
    ring.setPixelColor(i+3, white2);   // W
    delay(wait); ring.show();
    ring.setPixelColor(i+4, white);    // W
    delay(wait); ring.show();
  }
}

// P A R T Y  M O D E 
void theaterChaseRainbow(uint8_t wait) {      // Theatre-style crawling lights with rainbow effect
  for (int j = 0; j < 256; j++) {             // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (uint16_t i = 0; i < ring.numPixels(); i = i + 2) {
        ring.setPixelColor(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
      }
      delay(wait*.25); ring.show();
      if((switch1State != HIGH ) && (switch2State !=LOW))
        break;break;
} } }  
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
