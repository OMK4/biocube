/* Nunz Aquarium Feeder
  */

#include <AccelStepper.h>

#define FULLSTEP 4
#define HALFSTEP 8
                        
#define MotorPin1  10
#define MotorPin2  11
#define MotorPin3  12
#define MotorPin4  13

#define Port_Endstop 12
#define Port_Starter 7
#define Port_Led13 13

unsigned long FeedingActive = false;
unsigned long AktTime = 0;
unsigned long MeasureTime = 0;
int PositioningMode = 0;

AccelStepper Stepper1(FULLSTEP, MotorPin1, MotorPin3, MotorPin2, MotorPin4);

void setup() 
{
  Serial.begin(9600);

  pinMode(Port_Endstop, INPUT_PULLUP);
  pinMode(Port_Starter, INPUT);
  pinMode(Port_Led13, OUTPUT);
  digitalWrite(Port_Led13, LOW);
  
  Stepper1.setMaxSpeed(70.0);
  Stepper1.setAcceleration(1000.0);
  Stepper1.setSpeed(70);

  Stepper1.disableOutputs();

  GotoStartPosition();
  DoFeed();
} 

void loop() 
{
  AktTime = millis();
  
// The starter signal must be at HIGH for at least a 1/2 second before you go
  if(digitalRead(Port_Starter) == HIGH)
  {
    if(MeasureTime == 0)
      MeasureTime = AktTime;
      
    if(MeasureTime > 0 && AktTime - MeasureTime > 1000 && FeedingActive == false)
    {
      FeedingActive = true;
      digitalWrite(Port_Led13, HIGH);
      // Ready to go
      digitalWrite(Port_Led13, HIGH);
      Serial.println("Start");
      Stepper1.enableOutputs();
      if(GotoStartPosition() == true)
      {
        DoFeed();
        DoFeed();
      }
      digitalWrite(Port_Led13, LOW);
    }
  }
  else if(FeedingActive == true)  // Only a LOW at the starter puts everything back
  {
    FeedingActive = false;
    MeasureTime = 0;
    digitalWrite(Port_Led13, LOW);
    Serial.println("Reset");
  }
}

// The carriage is positioned using the end stop so that the
// Switch just stopped switching, so the slide is at the very beginning
bool GotoStartPosition()
{
  unsigned long TimeOutTime = AktTime;

  Stepper1.setMaxSpeed(70.0);
  Stepper1.setAcceleration(1000.0);
  Stepper1.setSpeed(70);

  do
  {
    AktTime = millis();
    if(PositioningMode == 0)
    { 
      Stepper1.enableOutputs();
      Stepper1.moveTo(10000);
      PositioningMode = 1;
    }
  
    if(PositioningMode == 1)
    { 
      if(digitalRead(Port_Endstop) == LOW)
      { 
        PositioningMode = 2;
        Stepper1.stop();
        Serial.println("Stop1");
      }
    }
    if(PositioningMode == 2)
    { 
      Stepper1.moveTo(-10000);
      PositioningMode = 3;
    }
    if(PositioningMode == 3)
    { 
      if(digitalRead(Port_Endstop) == HIGH)
      { 
        unsigned WaitTime = millis();
        do
        {
          Stepper1.run();  
        } while(millis() - WaitTime < 400);
        PositioningMode = 4;
        Stepper1.stop();
        Serial.println("Stop2");
        break;
      }
    }
    if(PositioningMode > 0)
      Stepper1.run();  
  } while(PositioningMode < 4 && AktTime - TimeOutTime < 5000);  // if it jams and the button can not be reached

  if(PositioningMode < 4 && AktTime - TimeOutTime >= 5000)
  {
    Serial.println(PositioningMode);
    PositioningMode = 0;
    Serial.println("Timeout Positioning");
    return false;
  }

  Stepper1.setCurrentPosition(0);
  PositioningMode = 0;
  
  Serial.println("Positioned");
  return true;
}

// initiate a feeding process
// It is assumed that the slide is already sitting correctly at the end
void DoFeed() 
{
  Stepper1.enableOutputs();
  Stepper1.setMaxSpeed(500.0);
  Stepper1.setAcceleration(2000.0);
  Stepper1.setSpeed(300);

  int extPath = -720;
  
  // Drive forward quickly to eject feed
  Serial.println("extension path");
  int CurrPos = Stepper1.currentPosition();
  Stepper1.moveTo(extPath);
//  Serial.println(CurrPos);

  while(Stepper1.distanceToGo() != 0)
  {
//    Serial.println(Stepper1.currentPosition());
    Stepper1.run();
  }

// Wiggle a little back and forth so that the food falls as well
  Stepper1.setAcceleration(20000.0);
  Serial.println("Wiggle");
  for(int t = 0; t< 3; t++)
  {
    Stepper1.moveTo(extPath+50);
    while(Stepper1.distanceToGo() != 0)
        Stepper1.run();
    Stepper1.moveTo(extPath);
    while(Stepper1.distanceToGo() != 0)
        Stepper1.run();
  }

// enter again to reload
  Serial.println(Stepper1.currentPosition());
  Stepper1.setAcceleration(2000.0);
  Serial.println("retract");
  Stepper1.moveTo(0);
  while(Stepper1.distanceToGo() != 0)
  {
//    Serial.println(Stepper1.currentPosition());
      Stepper1.run();
  }
  Stepper1.disableOutputs();
//  Serial.println(Stepper1.currentPosition());
  Serial.println("Finished");
}

