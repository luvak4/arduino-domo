////////////////////////////////
// SERVO PULSANTE
////////////////////////////////

// è una interfaccia che spegne e accende dispositivi
// elettrici che hanno solo un pulsante per lo
// spegnimento. Rende disponibile anche lo stato dei dispositivi
// usando le spie già in essere.

/*
              +------------+
              |            |
premi A	 ---->|            |-------> Servo A
              |            |
premi B	 ---->|            |-------> Servo B
              |            |
              |            |
luce A	<-----|            |<------- Sensore luce A
              |            |
luce B	<-----|            |<------- Sensore luce B
              |            |
              +------------+
                atmega328
*/
#include <Servo.h>

#define pin_photoA    A0
#define pin_photoB    A1
#define pin_servoA    9
#define pin_servoB    10

#define pin_luceA      2
#define pin_luceB      3
#define pin_premiA     4
#define pin_premiB     5
#define FALSE 0
#define TRUE 1

Servo servoA;
Servo servoB;

bool servoAon=FALSE;
bool servoBon=FALSE;

#define SOGLIA   400

unsigned long tempoA;
unsigned long tempoB;
unsigned long tempoC;
unsigned long tempoD;
void setup() {
  pinMode(pin_luceA,OUTPUT);
  pinMode(pin_luceB,OUTPUT);
  pinMode(pin_premiA,INPUT);
  pinMode(pin_premiB,INPUT);  
  Serial.begin(9600); // debug
}

void loop(){
  // ogni secondo
  if ((abs(millis()-tempoA))>1000){
    tempoA=millis();
    if (analogRead(pin_photoA)<SOGLIA){
      digitalWrite(pin_luceA,LOW);
    }
  }
  if ((abs(millis()-tempoB))>1000){
    tempoB=millis();
    if (analogRead(pin_photoB)<SOGLIA){
      digitalWrite(pin_luceB,LOW);
    }
  }
  if ((abs(millis()-tempoC))>1000){
    tempoC=millis();
    if (servoAon){
      servoAon=FALSE;
      servoA.write(0);
    }
  }
  if ((abs(millis()-tempoD))>1000){
    tempoD=millis();
    if (servoBon){
      servoBon=FALSE;
      servoB.write(0);
    }
  }    
  //continuamente
  if (analogRead(pin_photoA)>SOGLIA){
    digitalWrite(pin_luceA,HIGH);
    tempoA=millis();      
  }
  if (analogRead(pin_photoB)>SOGLIA){
    digitalWrite(pin_luceB,HIGH);
    tempoB=millis();
  }
  if (digitalRead(pin_premiA)){
    servoAon=TRUE;
    servoA.write(90);
    tempoC=millis();
  }
  if (digitalRead(pin_premiB)){
    servoBon=TRUE;
    servoB.write(90);
    tempoD=millis();
  }  
}
