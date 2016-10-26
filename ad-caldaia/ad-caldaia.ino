/////////////////////////
// CALDAIA
/////////////////////////

#include <VirtualWire.h>

#define pin_photoA    A0
#define pin_photoB    A1
#define pin_photoC    A2
#define pin_photoD    A3
#define pin_rx    11
#define pin_tx    12

// domande (IN)
#define MASTCa 150 // leggi tempo led A/B/C
#define MASTCb 151 // leggi tempo led D
// risposte (OUT)
#define CALDAa   1010 // get tempo led A/B/C
#define CALDAb   1011 // get tempo led D

#define AGCdelay 1000
#define SOGLIA   400

byte secondi;
byte SecPhotoA=0;
byte SecPhotoB=0;
byte SecPhotoC=0;
byte SecPhotoD=0;

unsigned int minuti;
unsigned int MinPhotoA=0;
unsigned int MinPhotoB=0;
unsigned int MinPhotoC=0;
unsigned int MinPhotoD=0;

unsigned long tempo;

/*--------------------------------
** comunicazione radio principale
*/
#define VELOCITAstd   500
#define MESSnum         0
#define DATOa           1
#define DATOb           2
#define DATOc           3
#define BYTEStoTX       8
int     INTERIlocali[4]={0,0,0,0};
byte    BYTEradio[BYTEStoTX];
uint8_t buflen = BYTEStoTX; //for rx
/*--------------------------------
** varie
*/
byte CIFR[]={223,205,228,240,43,146,241,//
	     87,213,48,235,131,6,81,26,//
	     70,34,74,224,27,111,150,22,//
	     138,239,200,179,222,231,212};
#define mask 0x00FF

void setup() {
  vw_set_tx_pin(pin_tx);
  vw_set_rx_pin(pin_rx);
  vw_setup(VELOCITAstd);
  vw_rx_start();
  tempo=millis();
  Serial.begin(9600); // debug
}

void loop(){
  // tieni il tempo
  if ((abs(millis()-tempo))>1000){
    tempo=millis();
    secondi+=1;
    if (secondi > 59){
      secondi=0;
      minuti+=1;
      if (minuti>1440){
	minuti=0;
	MinPhotoA=0;
	MinPhotoB=0;
	MinPhotoC=0;
	MinPhotoD=0;
      }
    }
    if (analogRead(pin_photoA)>SOGLIA){
      SecPhotoA+=1;
      if (SecPhotoA>59){
	SecPhotoA=0;
	MinPhotoA+=1;
      }
    }
    if (analogRead(pin_photoB)>SOGLIA){
      SecPhotoB+=1;
      if (SecPhotoB>59){
	SecPhotoB=0;
	MinPhotoB+=1;
      }
    }
    if (analogRead(pin_photoC)>SOGLIA){
      SecPhotoC+=1;
      if (SecPhotoC>59){
	SecPhotoC=0;
	MinPhotoC+=1;
      }
    }
    if (analogRead(pin_photoD)>SOGLIA){
      SecPhotoD+=1;
      if (SecPhotoD>59){
	SecPhotoD=0;
	MinPhotoD+=1;
      }
    }    
  }
  // radio rx
  if (vw_get_message(BYTEradio, &buflen)){
    vw_rx_stop();
    decodeMessage();
    switch (INTERIlocali[MESSnum]){
    case MASTCa:
      // imposta l'indirizzo
      INTERIlocali[MESSnum]=CALDAa;
      // valori in memoria
      INTERIlocali[DATOa]=MinPhotoA;
      INTERIlocali[DATOb]=MinPhotoB;
      INTERIlocali[DATOc]=MinPhotoC;
      //
      tx();
      break;
    case MASTCb:
      // imposta l'indirizzo
      INTERIlocali[MESSnum]=CALDAb;
      // valori in memoria
      INTERIlocali[DATOa]=MinPhotoD;
      INTERIlocali[DATOb]=0;
      INTERIlocali[DATOc]=0;
      //
      tx();      
      break;
    }
  }  
}

void decodeMessage(){
  byte m=0;
  cipher();
  for (byte n=0; n<4;n++){
    INTERIlocali[n]=BYTEradio[m+1];
    INTERIlocali[n]=INTERIlocali[n] << 8;
    INTERIlocali[n]=INTERIlocali[n]+BYTEradio[m];
    m+=2;
  }
}

void cipher(){
  for (byte n=0;n<8;n++){
    BYTEradio[n]=BYTEradio[n]^CIFR[n];
  }
}

void tx(){
  // codifica in bytes
  encodeMessage();
  //******************************
  // prima di trasmettere attende
  // che l'AGC di rx-MAESTRO
  // abbia recuperato
  //******************************
  delay(AGCdelay); // IMPORTANTE
  //******************************
  vw_rx_stop();
  vw_send((uint8_t *)BYTEradio,BYTEStoTX);
  vw_wait_tx();
  vw_rx_start();
}

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
