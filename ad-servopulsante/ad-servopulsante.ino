// -*-c++-*-
////////////////////////////////
// SERVO PULSANTE (nuova versione)
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
              |            |-------> rele generale
              |            |-------> rele bridge A
              |            |-------> rele modem
              |            |
              |            |-------> com RS232 (pfsense)
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
#define pin_luceA       2
#define pin_luceB       3
#define pin_servoA      4
#define pin_servoB      5
#define pin_releGen     6
#define pin_releBridgeA 7
#define pin_releModem   8
#define pin_rx    11
#define pin_tx    12
/*--------------------------------
** domande (IN)
*/
#define MASTSa 200     // move servoA (push button)    (SERVOa)
#define MASTSb 201     // move servoB (push button)    (SERVOb)
#define MASTSc 202     // get stato leds servo/pfsense (SERVOc)
#define MASTSd 203     // get stato generale           (SERVOd)
#define MASTSe 204     // turn off UPS                 (SERVOe)
#define MASTSf 205     // turn off NAS                 (SERVOf)
#define MASTSg 206     // turn off generale            (SERVOg)
#define MASTSh 207     // turn on UPS                  (SERVOh)
#define MASTSi 208     // turn on NAS                  (SERVOi)
#define MASTSj 209     // turn on generale             (SERVOj)
#define MASTSk 210     // shutdown pfSense             (SERVOk)
#define MASTSl 211     // reboot pfSense               (SERVOl)
/*--------------------------------
** risposte (OUT)
*/
#define SERVOa 1030   // move servoA (push button)
#define SERVOb 1031   // move servoB (push button)
#define SERVOc 1032   // get stato leds servo/pfsense
#define SERVOd 1033   // get stato generale
#define SERVOe 1034   // turn off UPS      
#define SERVOf 1035   // turn off NAS      
#define SERVOg 1036   // turn off generale 
#define SERVOh 1037   // turn on UPS       
#define SERVOi 1038   // turn on NAS       
#define SERVOj 1039   // turn on generale  
#define SERVOk 1040   // shutdown pfSense  
#define SERVOl 1041   // reboot pfSense    
/*--------------------------------
** radio tx rx
*/
byte CIFR[]={223,205,228,240,43,146,241,\
       87,213,48,235,131,6,81,26,\
       70,34,74,224,27,111,150,22,\
       138,239,200,179,222,231,212};
#define mask       0x00FF
#define VELOCITAstd   500   // velocita standard
#define MESSnum         0   // posizione in BYTEradio
#define DATOa           1   //  "
#define DATOb           2   //  "
#define DATOc           3   //  "
#define BYTEStoTX       8   // numbero of bytes to tx
#define AGCdelay 1000       // delay for AGC
int     INTERIlocali[4]={0,0,0,0}; // N.Mess,DatoA,DatoB,DatoC
byte    BYTEradio[BYTEStoTX];  // buffer per la trasmissione
uint8_t buflen = BYTEStoTX; //for rx
/*--------------------------------
** varie
*/
#define ANGOLOservo 1200 // angolo (uS) per premere il pulsante
ServoTimer2 servoA;
ServoTimer2 servoB;
//
bool servoAon=false;
bool servoBon=false;
//
#define SOGLIA   400
//
/*////////////////////////////////
* setup()
*/
void setup() {
  pinMode(pin_luceA,OUTPUT);
  pinMode(pin_luceB,OUTPUT);
  servoA.attach(pin_servoA);
  servoB.attach(pin_servoB);
  Serial.begin(9600); // debug
  // radio
  vw_set_tx_pin(pin_tx);
  vw_set_rx_pin(pin_rx);
  vw_setup(VELOCITAstd);
  vw_rx_start();    
  servoA.write(0);
  servoB.write(0); 
}
//
/*////////////////////////////////
* loop()
*/
void loop(){
/*--------------------------------
** radio rx
*/

  // 1200 dovrebbe essere piu o meno l'angolo giusto
 

  if (vw_get_message(BYTEradio, &buflen)){
    vw_rx_stop();
    decodeMessage();
    switch (INTERIlocali[MESSnum]){
    case MASTSa:
      servoA.write(ANGOLOservo);
      delay(2000);
      servoA.write(0);      
      break;
    case MASTSb:
      servoB.write(ANGOLOservo);
      delay(2000);
      servoB.write(0);     
      break;
    case MASTSc:
      break;
    }
    vw_rx_start(); 
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
