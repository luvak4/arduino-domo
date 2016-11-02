// -*-c++-*-
////////////////////////////////
// SERVO PULSANTE
////////////////////////////////

// E' una interfaccia che spegne e accende dispositivi
// elettrici che hanno solo un pulsante per lo
// spegnimento. Rende disponibile anche lo stato dei dispositivi
// usando le spie gia in essere.

/*
              +------------+
              |            |
              |            |-------> Servo A
              |            |-------> Servo B
              |            |
              |            |<------- Sensore luce A
              |            |<------- Sensore luce B
              |            |
              |            |-------> rele A
              |            |-------> rele B
              |            |
spegni pfs--->|            |-------> com RS232 (pfsense)
              |            |
radio rx ---->|            |-------> radio tx
              +------------+
                atmega328
*/
/*--------------------------------
* configurazioni
*/
#include <VirtualWire.h>
#include <ServoTimer2.h> 
/*--------------------------------
** pins
*/
#define pin_luceA      2
#define pin_luceB      3
#define pin_premiA     4
#define pin_premiB     5
#define pin_rx    11
#define pin_tx    12
/*--------------------------------
** domande (IN)
*/
#define MASTSa 200 // move servoA (push button)
#define MASTSb 201 // move servoB (push button)
#define MASTSa 202 // get stato leds servoA and servoB
/*--------------------------------
** risposte (OUT)
*/
#define SERVOa   1013 // ok pushbutton A
#define SERVOb   1014 // ok pushbutton B
#define SERVOc   1015 // get stato leds servoA and servoB
/*--------------------------------
** radio tx rx
*/
byte CIFR[]={223,205,228,240,43,146,241,\
	     87,213,48,235,131,6,81,26,\
	     70,34,74,224,27,111,150,22,\
	     138,239,200,179,222,231,212};

#define mask 0x00FF
int     INTERIlocali[4]={0,0,0,0}; // N.Mess,Da,Db,Dc
byte    BYTEradio[BYTEStoTX];
uint8_t buflen = BYTEStoTX; //for rx
#define VELOCITAstd   500   // velocita standard
#define MESSnum         0   // posizione in BYTEradio
#define DATOa           1   //  "
#define DATOb           2   //  "
#define DATOc           3   //  "
#define BYTEStoTX       8   // numbero of bytes to tx
#define AGCdelay 1000       // delay for AGC
/*--------------------------------
** varie
*/
ServoTimer2 servoA;
ServoTimer2 servoB;
//
bool servoAon=false;
bool servoBon=false;
//
#define SOGLIA   400
//
unsigned long tempoA;
unsigned long tempoB;
unsigned long tempoC;
unsigned long tempoD;
//
/*////////////////////////////////
* setup()
*/
void setup() {
  pinMode(pin_luceA,OUTPUT);
  pinMode(pin_luceB,OUTPUT);
  pinMode(pin_premiA,INPUT);
  pinMode(pin_premiB,INPUT);  
  servoA.attach(pin_servoA);
  servoB.attach(pin_servoB);
  Serial.begin(9600); // debug
  // radio
  vw_set_tx_pin(pin_tx);
  vw_set_rx_pin(pin_rx);
  vw_setup(VELOCITAstd);
  vw_rx_start();    
  //servoA.write(1000);
}
//
/*////////////////////////////////
* loop()
*/
void loop(){
/*--------------------------------
** tieni il tempo
*/
  // ogni secondo
  // viene spento lo stato del pin
  // se led spento
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
      servoAon=false;
      servoA.write(0);
    }
  }
  if ((abs(millis()-tempoD))>1000){
    tempoD=millis();
    if (servoBon){
      servoBon=false;
      servoB.write(0);
    }
  }    
  // istantaneamente
  // accende lo stato del pin se led acceso
  if (analogRead(pin_photoA)>SOGLIA){
    digitalWrite(pin_luceA,HIGH);
    tempoA=millis();      
  }
  if (analogRead(pin_photoB)>SOGLIA){
    digitalWrite(pin_luceB,HIGH);
    tempoB=millis();
  }
  if (digitalRead(pin_premiA)){
    servoAon=true;
    servoA.write(90);
    tempoC=millis();
  }
  if (digitalRead(pin_premiB)){
    servoBon=true;
    servoB.write(90);
    tempoD=millis();
  }
/*--------------------------------
** radio rx
*/
  if (vw_get_message(BYTEradio, &buflen)){
    vw_rx_stop();
    decodeMessage();
    switch (INTERIlocali[MESSnum]){
    case MASTSa:
      servoA.write(1000);
      break;
    case MASTSb:
      servoA.write(1900);
      break;
    case MASTSc:
      break;
    }    
  }
}
/*--------------------------------
* decodeMessage()
*/
// RADIO -> locale
//
void decodeMessage(){
  byte m=0;
  cipher();
  for (int n=0; n<4;n++){
    INTERIlocali[n]=BYTEradio[m+1];
    INTERIlocali[n]=INTERIlocali[n] << 8;
    INTERIlocali[n]=INTERIlocali[n]+BYTEradio[m];
    m+=2;
  }
}
/*--------------------------------
* encodeMessage()
*/
// locale -> RADIO
//
void encodeMessage(){
  byte m=0;
  for (int n=0; n<4;n++){
    BYTEradio[m]=INTERIlocali[n] & mask;
    INTERIlocali[n]=INTERIlocali[n] >> 8;
    BYTEradio[m+1]=INTERIlocali[n] & mask;
    m+=2;
  }
  cipher();
}
/*--------------------------------
* cipher()
*/
// cifratura XOR del messaggio
//
void cipher(){
  for (byte n=0;n<8;n++){
    BYTEradio[n]=BYTEradio[n]^CIFR[n];
  }
}
/*--------------------------------
* tx()
*/
void tx(){
  encodeMessage();
  vw_rx_stop();
  vw_send((uint8_t *)BYTEradio,BYTEStoTX);
  vw_wait_tx();
  vw_rx_start(); 
}
