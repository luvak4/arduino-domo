// -*-c++-*-
/////////////////////////
// CALDAIA
/////////////////////////

// leggelo stato di 4 LED della caldaia, ne tiene conto dei minuti 
// in cui sono rimasti accesi singolarmente. Ogni 24 ore vengono
// resettati i contatori. Le informazioni sullo stato vengono rese
// disponibili via radio
/*--------------------------------
* configurazioni
*/
#include <VirtualWire.h>
#include <EEPROM.h>
// indirizzo di memoria EEPROM che
// contiene l'ultimo indirizzo usato
// all'inizio del secondo giorno
// contiene il valore 4 (1=fiamma, 2=termo,
// 3=acqua, 4=interruttore);
// all'inizio del terzo giorno contiene 8
// (5=fiamma, 6=termo, 7=acqua, 8=interruttore)
// e cos� via.
// i dati del giorno 'n' si recuperano agli
// indirizzi:
// n*4-3 = fiamma
// n*4-2 = termo
// n*4-1 = acqua
// n*4   = interruttore
#define eepromLASTADDRESSused 0 
                        

/*--------------------------------
** pins
*/
#define pin_photoA    A0
#define pin_photoB    A1
#define pin_photoC    A2
#define pin_photoD    A3
#define pin_rx    11
#define pin_tx    12
/*--------------------------------
** domande (IN)
*/
#define MASTCa 150 // leggi tempo led A/B/C
#define MASTCb 151 // leggi tempo led D
#define MASTCc 152 // get stato leds
#define MASTCd 153 // tempo led ABC ieri
#define MASTCz 190 // set giorno 0
/*--------------------------------
** risposte (OUT)
*/
#define CALDAa   1010 // get tempo led A/B/C
#define CALDAb   1011 // get tempo led D
#define CALDAc   1012 // get stato leds
#define CALDAd   1013 // get tempo led ABC ieri
#define CALDAz   1020 // ok: set giorno 0
/*--------------------------------
** radio tx rx
*/
byte CIFR[]={223,205,228,240,43,146,241,\
	     87,213,48,235,131,6,81,26,\
	     70,34,74,224,27,111,150,22,\
	     138,239,200,179,222,231,212};
#define mask 0x00FF
#define VELOCITAstd   500   // velocita standard
#define MESSnum         0   // posizione in BYTEradio
#define DATOa           1   //  "
#define DATOb           2   //  "
#define DATOc           3   //  "
#define BYTEStoTX       8   // numbero of bytes to tx
#define AGCdelay 2000       // delay for AGC
int     INTERIlocali[4]={0,0,0,0}; // N.Mess,Da,Db,Dc
byte    BYTEradio[BYTEStoTX];
uint8_t buflen = BYTEStoTX; //for rx

/*--------------------------------
** varie
*/
const int SOGLIA=   700;
//

byte secondi;
byte SecPhotoA=0;
byte SecPhotoB=0;
byte SecPhotoC=0;
byte SecPhotoD=0;
//
unsigned int minuti;
unsigned int MinPhotoA=0;
unsigned int MinPhotoB=0;
unsigned int MinPhotoC=0;
unsigned int MinPhotoD=0;
//
unsigned long tempo;
//
/*////////////////////////////////
* setup()
*/
void setup() {
  vw_set_tx_pin(pin_tx);
  vw_set_rx_pin(pin_rx);
  vw_setup(VELOCITAstd);
  vw_rx_start();
  tempo=millis();
  //Serial.begin(9600); // debug
}
//
/*////////////////////////////////
* loop()
*/
void loop(){
  int eepADDR;
  byte n=0;
/*--------------------------------
** tieni il tempo
*/
  if ((abs(millis()-tempo))>1000){
    tempo=millis();
    secondi+=1;
    if (secondi > 59){
      secondi=0;
      minuti+=1;
      if (minuti>1440){
	//--------------------------------
	// ogni 24 h azzera i dati
	// ma prima li salva su EEPROM
	//--------------------------------
	eepADDR=EEPROM.read(eepromLASTADDRESSused);
	if (eepADDR>240){
	  // memorizza i dati di 240 giorni
	  eepADDR=0;
	}
	eepADDR++;
	EEPROM.write(eepADDR,MinPhotoA);
	eepADDR++;
	EEPROM.write(eepADDR,MinPhotoB);
	eepADDR++;
	EEPROM.write(eepADDR,MinPhotoC);	
	eepADDR++;
	EEPROM.write(eepADDR,MinPhotoD);
	// ultimo indirizzo usato
	EEPROM.write(eepromLASTADDRESSused,eepADDR);
	// cancellazione
	minuti=0;
	MinPhotoA=0; // fiamma
	MinPhotoB=0; // termo
	MinPhotoC=0; // acqua
	MinPhotoD=0; // interruttore
      }
    }
    if (analogRead(pin_photoA)<SOGLIA){
      SecPhotoA+=1;
      if (SecPhotoA>59){
	SecPhotoA=0;
	MinPhotoA+=1;
      }
    }
    if (analogRead(pin_photoB)<SOGLIA){
      SecPhotoB+=1;
      if (SecPhotoB>59){
	SecPhotoB=0;
	MinPhotoB+=1;
      }
    }
    if (analogRead(pin_photoC)<SOGLIA){
      SecPhotoC+=1;
      if (SecPhotoC>59){
	SecPhotoC=0;
	MinPhotoC+=1;
      }
    }
    if (analogRead(pin_photoD)<SOGLIA){
      SecPhotoD+=1;
      if (SecPhotoD>59){
	SecPhotoD=0;
	MinPhotoD+=1;
      }
    }    
  }
/*--------------------------------
** radio rx
*/
  if (vw_get_message(BYTEradio, &buflen)){
    vw_rx_stop();
    decodeMessage();
    //Serial.println(INTERIlocali[MESSnum]);
    switch (INTERIlocali[MESSnum]){
    case MASTCa:
      // imposta l'indirizzo
      INTERIlocali[MESSnum]=CALDAa;
      // valori in memoria
      INTERIlocali[DATOa]=MinPhotoA; // fiamma
      INTERIlocali[DATOb]=MinPhotoB; // termo
      INTERIlocali[DATOc]=MinPhotoC; // acqua
      //
      tx();
      break;
    case MASTCb:
      // imposta l'indirizzo
      INTERIlocali[MESSnum]=CALDAb;
      // valori in memoria
      INTERIlocali[DATOa]=MinPhotoD; // caldaia accesa
      INTERIlocali[DATOb]=0;
      INTERIlocali[DATOc]=0;
      //
      tx();      
      break;
    case MASTCc:
      // imposta l'indirizzo
      INTERIlocali[MESSnum]=CALDAc;
      // valori in memoria
      if (analogRead(pin_photoD)<SOGLIA){
        n=1;
      } 
      n=n<<1;
      if (analogRead(pin_photoC)<SOGLIA){
        n=n | 1;
      }    
     n=n<<1;
      if (analogRead(pin_photoB)<SOGLIA){
        n=n | 1;
      }  
     n=n<<1;
      if (analogRead(pin_photoA)<SOGLIA){
        n=n | 1;
      }
      INTERIlocali[DATOa]=n;
      INTERIlocali[DATOb]=0;
      INTERIlocali[DATOc]=0;
      //
      tx();         
      break;
    case MASTCd:
      eepADDR=EEPROM.read(eepromLASTADDRESSused);
      // imposta l'indirizzo
      INTERIlocali[MESSnum]=CALDAd;
      // valori in memoria
      INTERIlocali[DATOa]=EEPROM.read(eepADDR-3); // fiamma
      INTERIlocali[DATOb]=EEPROM.read(eepADDR-2); // termo
      INTERIlocali[DATOc]=EEPROM.read(eepADDR-1); // acqua
      //
      tx();      
      break;      
    case MASTCz:
      EEPROM.write(eepromLASTADDRESSused,0);
      INTERIlocali[MESSnum]=CALDAz;
      tx();         
      break;
    }
    vw_rx_stop();
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
  for (byte n=0; n<4;n++){
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
/*--------------------------------
* tx()
*/
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
