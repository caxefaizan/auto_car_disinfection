/*
Uber Decontamination 
*/

#include <EEPROM.h>
#include <Countimer.h>

// CONSTANTS

int       dR          =  1;                   // Decontamination Required
int       pD          =  0;                   // Process Done
int       buttonPress =  0;
int       prevState   = LOW;
int       calibState      ;
int       stateAddr   =  0;
int       presetAddr  =  16;
int       preset          ;
const int button      =  4;                   // Start Button
const int redLed      =  5;                   // Not decontaminated 
const int greenLed    =  6;                   // Decontaminated 
const int echoPin     =  2;
const int trigPin     =  3;
const int cButton      = 7; 
const int relayPin    = 13;                   // Relay 

// GLOBAL: create  object 

Countimer timer;

void setup() {
  Serial.begin(9600);
  pinMode(relayPin,   OUTPUT);     
  pinMode(redLed,     OUTPUT); 
  pinMode(greenLed,   OUTPUT);
  pinMode(trigPin,    OUTPUT);  
  pinMode(echoPin,    INPUT);   
  pinMode(button,     INPUT);
  pinMode(cButton,    INPUT);
 
  digitalWrite(redLed,    HIGH);
  digitalWrite(greenLed,  LOW);  
  digitalWrite(relayPin,  HIGH);
  
  timer.setCounter(0, 0, 10, timer.COUNT_DOWN, onComplete);
  timer.setInterval(refreshClock, 1000);
  
  int calib = digitalRead(cButton);
  if(calib == HIGH ){ // RESETTING CALIBATION , MAY ADD MULTIPLE BUTTONS'S STATE TO BE HIGH
    Serial.println("Force Recalibration");
    calibrate();
    
  }
  
  Serial.println("Finding calibration values");
  EEPROM.get(stateAddr,calibState);
  if(calibState == 1){
    Serial.println("Found calibration values");
    EEPROM.get(presetAddr,preset);
    Serial.print("The calibration value is: ");
    Serial.println(preset);
  }
  else{
    Serial.println("Could not find calibration values");
    int calib = digitalRead(cButton);
    while(calib != HIGH){
      Serial.println(" Waiting for Calibration Button Press!!");
      int calib = digitalRead(cButton);
    }
    calibrate();
  }
  
  
}

void calibrate(){
    preset = riderDetection();
    Serial.println("Saving calibration values through recalibration");
    EEPROM.put(presetAddr,preset);
    calibState = 1; 
    EEPROM.put(stateAddr,calibState);
}

void refreshClock() {
  Serial.print("Time left: ");
  Serial.println(timer.getCurrentTime());
}
 
void loop(){
  if((pD == 0) && (dR == 1)){
    Serial.println("Decontamination Required, No rider should be present");
    decontaminate();
  }
  
  if((pD == 1) && (dR == 0)){
    Serial.println("Safe, Detecting Rider");
    while(1){
      int val = riderDetection();
      if(val<(preset-20)){
        Serial.println("Rider Detected");
        Serial.println("Waiting against false detection of trip start!");
        delay(10000);
        val = riderDetection();
        if(val<(preset-20)){
          dR=1;
          pD=0;
          while(val<(preset-20)){
            val = riderDetection();
            Serial.println("Waiting for rider to end trip");
            delay(1000);
          }
          Serial.println("Waiting for false detection of end trip!");
          delay(10000);
          val = riderDetection();
          if(val>=(preset-20)){ //false detection safeguard
            break;
          }
        }
       }
     }
  }
  if((dR == 1) && (pD == 0)){
    digitalWrite(greenLed, LOW);
    digitalWrite(redLed, HIGH);
  }
  else{
    digitalWrite(greenLed, HIGH);
    digitalWrite(redLed, LOW);
  }
}

void onComplete() {
  digitalWrite(relayPin, HIGH);                                      // turns relay OFF
  Serial.println("Decontamination Done!!!");
  digitalWrite(greenLed, HIGH);
  digitalWrite(redLed, LOW);
  dR=0;
  pD=1;
}

int riderDetection() {
  digitalWrite(trigPin, LOW); // Clears the trigPin condition
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH); // Reads the echoPin, returns the sound wave travel time in microseconds
  int distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
  Serial.print(distance);
  Serial.println(" cm");
  delay(1000);
  return distance;
}

void decontaminate(){
  int val= riderDetection();
  while((pD==0)&&(dR==1) && (val>=20)){
    timer.run();
    int currState = digitalRead(button);
    
    if(currState!=prevState){
      ++buttonPress;  
      delay(1000);
    }
    
    if ((currState != LOW)  && (buttonPress==1)) {          // if no motion detected
      timer.start();
      //delay(5000);                                                          // driver gets out
      digitalWrite(relayPin, LOW);                                         // turns relay ON
      Serial.println("Decontamination Starts!!!");
    } 
   
    if ((currState != LOW)  && (buttonPress==2)) {               // pause
      digitalWrite(relayPin, HIGH);
      timer.pause();
      Serial.println("Decontamination Paused!!!");
    } 
    
    if((currState != LOW)  && (buttonPress==3)) {               // resume
      digitalWrite(relayPin, LOW);
      Serial.println("Decontamination Resumed!!!");
      timer.start();
      buttonPress=0;
    }
  }
}
