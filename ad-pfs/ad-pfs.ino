// -*-c++-*-
////////////////////////////////
// pf-Sense spegnitore
////////////////////////////////

// nasce per spegnere via radio
// un server PfSense, inviando comandi
// di shutdown/reboot attraverso
// la seriale. Legge lo stato interpretando
// quando PfSense invia a sua volta - in risposta - 
// sulla seriale
/*--------------------------------
* configurazioni
*/
#include <VirtualWire.h>
/*--------------------------------
** pins
*/
const int pin_rosso = 2;
const int pin_giallo = 3;
const int pin_verde = 4;
const int pin_rx = 11;
const int pin_tx = 12;
const int pin_pushbuttonOFF = 5;
/*--------------------------------
** stati
*/
#define ST_FIRSTBOOT   1
#define ST_BOOTING     2
#define ST_OFF_A       3
#define ST_OFF_B       4
#define ST_OFF_C       5
#define ST_OFF_D       6
#define ST_DOREBOOT_A  7
#define ST_DOREBOOT_B  8
#define ST_DOREBOOT_C  9
#define ST_DOREBOOT_D 10
#define ST_ON         11
#define ST_OFF        12
//
#define LUCEGIALLA     1
#define LUCEVERDE      2
#define LUCEROSSA      3
//
/*--------------------------------
** domande (IN)
*/
#define MASTPa 250 // get stato leds pfSense
#define MASTPb 251 // shutdown pfSense
#define MASTPc 252 // reboot pfSense
/*--------------------------------
** risposte (OUT)
*/
#define PFSENa 1016 // get status of leds
#define PFSENb 1017 // ok shutdown
#define PFSENc 1018 // ok reboot
#define PFSENc 1019 // operation not possible
/*--------------------------------
** radio tx rx
*/
byte CIFR[]={223,205,228,240,43,146,241,\
	     87,213,48,235,131,6,81,26,\
	     70,34,74,224,27,111,150,22,\
	     138,239,200,179,222,231,212};
#define mask        0x00FF
#define VELOCITAstd   500   // velocita standard
#define MESSnum         0   // posizione in BYTEradio
#define DATOa           1   //  "
#define DATOb           2   //  "
#define DATOc           3   //  "
#define BYTEStoTX       8   // numbero of bytes to tx
//#define AGCdelay      1000  // delay for AGC
int     INTERIlocali[4]={0,0,0,0}; // N.Mess,Da,Db,Dc
byte    BYTEradio[BYTEStoTX];
uint8_t buflen = BYTEStoTX; //for rx
/*--------------------------------
** varie
*/
byte intlStep=ST_FIRSTBOOT;
unsigned long tempo;
//
/*////////////////////////////////
* setup()
*/
void setup() {
  Serial.begin(9600); // connection to pfSense serial port
  Serial.flush();     // flush
  //
  pinMode(pin_rosso, OUTPUT);  // led red
  pinMode(pin_giallo, OUTPUT); // led yellow
  pinMode(pin_verde, OUTPUT);  // led green
  pinMode(pin_pushbuttonOFF, INPUT); // pulsante spegnimento
  //
  vw_set_tx_pin(pin_tx); 
  vw_set_rx_pin(pin_rx);  
  vw_setup(VELOCITAstd);       
  vw_rx_start();
}
//
/*////////////////////////////////
* loop()
*/
void loop() {
/*--------------------------------
** radio rx
*/
  if (vw_get_message(BYTEradio, &buflen)){
    vw_rx_stop();
    decodeMessage();
    switch (INTERIlocali[MESSnum]){
    case MASTPa:
      break;
    case MASTPb:
      if(intlStep==ST_ON){
	intlStep=ST_OFF_A;
      }
      break;
    case MASTPc:
      if(intlStep==ST_ON){
	intlStep=ST_DOREBOOT_A;
      }      
      break;
    }    
  }
/*--------------------------------
** internal steps
*/
  //ogni secondo
  if (abs(millis()-tempo>1000)){
    tempo=millis();
    switch (intlStep){
    case ST_FIRSTBOOT:
      if (Serial.available()) {
	if (Serial.find("PC Engines ALIX")){
	  intlStep=ST_BOOTING;
	  txStat(LUCEGIALLA);
	}
      }
      break;
    case ST_BOOTING:
      if (Serial.available()) {
	if (Serial.find("Enter an option")){
	  intlStep=ST_ON;
	  txStat(LUCEVERDE);
	}
      }
      break;
    case ST_ON:
      // *** sistema acceso ***
      //
      // spegne il sistema manualmente
      // attraverso il pulsante
      if (digitalRead(pin_pushbuttonOFF)){
	intlStep=ST_OFF_A;
      }
      // rilevazione di sistema in
      // spegnimento eseguita probabilmente via web
      if (Serial.available()) {
	if (Serial.find("has halted")){
	  intlStep=ST_FIRSTBOOT;
	  txStat(LUCEROSSA);
	}
      }
      // rilevazione di sistema in
      // riavvio eseguita probabilmente via web      
      if (Serial.available()) {
	if (Serial.find("rebooting")){
	  intlStep=ST_FIRSTBOOT;
	  txStat(LUCEROSSA);
	}
      }      
      break;
    case ST_OFF_A:
      // procedura di spegnimento del sistema
      Serial.write("6\n");
      intlStep=ST_OFF_B;
      break;
    case ST_OFF_B:
      if (Serial.available()) {
	if (Serial.find("proceed")){
	  intlStep=ST_OFF_C;
	  txStat(LUCEGIALLA);
	}
      }
      break;
    case ST_OFF_C:
      Serial.write("y\n");
      intlStep=ST_OFF_D;
      break;    
    case ST_OFF_D:
      // il sistema e' spento 
      if (Serial.available()) {
	if (Serial.find("has halted")){
	  intlStep=ST_FIRSTBOOT;
	  txStat(LUCEROSSA);
	}
      }
      break;
      // inizio procedura di riavvio
    case ST_DOREBOOT_A:
      Serial.write("5\n");
      intlStep=ST_DOREBOOT_B;
      break;
    case ST_DOREBOOT_B:
      if (Serial.available()) {
	if (Serial.find("proceed")){
	  intlStep=ST_DOREBOOT_C;
	  txStat(LUCEGIALLA);
	}
      }
      break;
    case ST_DOREBOOT_C:
      Serial.write("y\n");
      intlStep=ST_DOREBOOT_D;
      break;
    case ST_DOREBOOT_D:
      // il sistema si sta riavviando
      if (Serial.available()) {
	if (Serial.find("rebooting")){
	  intlStep=ST_FIRSTBOOT;
	  txStat(LUCEROSSA);
	}
      }
      break; 
    }
  }
}
/*--------------------------------
* txStat()
*/
//
void txStat(byte stato){
  digitalWrite(pin_rosso,LOW);
  digitalWrite(pin_giallo,LOW);
  digitalWrite(pin_verde,LOW);  
  switch (stato){
  case LUCEROSSA:
    digitalWrite(pin_rosso,HIGH);    
    break;
  case LUCEGIALLA:
    digitalWrite(pin_giallo,HIGH);
    break;
  case LUCEVERDE:
    digitalWrite(pin_verde,HIGH);
    break;
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
