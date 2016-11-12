// -*-c++-*-
////////////////////////////////
// lettura potenza assorbita
// attraverso lampeggio del led
// del contatore. Funzionamento a batteria
/*
 +------------+
 |            |
 |          2 |<------- Sensore luce A
 |            |
 |         12 |-------> radio tx
 +------------+
  ARDUINO UNO
*/


#include <VirtualWire.h>
#include "LowPower.h"
/*--------------------------------
** pins
*/
#define pin_luceA  2
#define pin_tx    12

/*--------------------------------
** innesco (OUT)
*/
#define POWERa 1100   // send when "N" pulses reaches
/*--------------------------------
** radio tx
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
int     INTERIlocali[4]={0,0,0,0}; // N.Mess,DatoA,DatoB,DatoC
byte    BYTEradio[BYTEStoTX];  // buffer per la trasmissione
/*--------------------------------
** varie
*/
#define pulsesToTx 50

unsigned long contaImpulsiLuce;
unsigned int diff;


void sveglia()
{
    // Just a handler for the pin interrupt.
  contaImpulsiLuce++;
  diff=contaImpulsiLuce-contaImpulsiLucePrec;
  if (diff>=pulsesToTx){
    contaImpulsiLucePrec=contaImpulsiLuce;
    // imposta l'indirizzo
    INTERIlocali[MESSnum]=POWERa;
    //
  tx();    
  }
}

void setup()
{
    pinMode(pin_luceA, INPUT);
  // radio
  vw_set_tx_pin(pin_tx);
  vw_setup(VELOCITAstd);   
}

void loop() 
{
    // Allow wake up pin to trigger interrupt on low.
    attachInterrupt(0, sveglia, LOW);    
    // Enter power down state with ADC and BOD module disabled.
    // Wake up when wake up pin is low.
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
    
    // Disable external pin interrupt on wake up pin.
    //detachInterrupt(0); 
    
    // Do something here
    // Example: Read sensor, data logging, data transmission.
}

void tx(){
  // codifica in bytes
  encodeMessage();
  //******************************
  // prima di trasmettere attende
  // che l'AGC di rx-MAESTRO
  // abbia recuperato
  //******************************
  //delay(AGCdelay); // IMPORTANTE
  //******************************
  vw_rx_stop();
  vw_send((uint8_t *)BYTEradio,BYTEStoTX);
  vw_wait_tx();
  vw_rx_start();  
}
/*--------------------------------
* encodeMessage()
*/
// locale -> RADIO
//
void encodeMessage(){
  byte m=0;
  for (byte n=0; n<4;n++){
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
