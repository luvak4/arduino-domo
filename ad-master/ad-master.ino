// -*-c++-*-
// verificare che agc delay serva oppure no
// *quando non si ottiene risposta entro determinato tempo
// *segnalare inviando byte conformato in un certo modo
// *impostare la lettura automatica ogni minuto
// *impostare la disabilitazione dell'invio a display
// *salvare i parametri in EEPROM
////////////////////////////////
// MASTER (ex 'tastiera')
////////////////////////////////
// E' l'unita radio che fa domande alle periferiche
// attende le risposte, le ripete per i
// dispositivi nelle vicinanze e accetta 
// domande (che ripetera') da dispositivi nelle vicinanze
//
//              +-----------+
//              |           |
//              |           |         
//       IR --->| 2         |
//              |           |
// radio rx --->| 11     12 |---> radio tx
//              |           |
//              +-----------+
//               ARDUINO
//               ATMEGA 328
//
/*--------------------------------
* configurazioni
*/
#include <IRremote.h> 
#include <VirtualWire.h>
#include <EEPROM.h>
/*--------------------------------
** pins
*/
const int pin_rx  = 11;
const int pin_tx  = 12;
const int pin_ir  =  2; // ir pin
/*--------------------------------
** messaggi (OUT)
*/
#define MASTRdisplay  100 // info to display
/*
**** risposte ritrasmesse
*/
#define kRITRASMISSIONE 10000 // valore sommato alle risposte
#define MARITR_CANTIa   11000 // get value luce/temp/rele
#define MARITR_CANTIb   11001 // get soglie luce/temp
#define MARITR_CANTIc   11002 // get AGC 
#define MARITR_CANTId   11003 // get temp/luce STATO/tempo
#define MARITR_CANTIokA 11004 // get ok salva eprom
#define MARITR_CANTIokB 11005 // get ok carica eprom
#define MARITR_CANTIokC 11006 // get ok carica default
/*--------------------------------
** domande (OUT)
*/
// cantina
#define MASTRa 101 // get luce/temp/rele               (CANTIa)
#define MASTRb 102 // !- set temp (soglia) up   +10    (CANTIb)
#define MASTRc 103 // !- set temp (soglia) down -10    (CANTIb)
#define MASTRd 104 // !- set luce (soglia a) up +5     (CANTIb)
#define MASTRe 105 // !- set luce (soglia a) dn -5     (CANTIb)
#define MASTRf 106 // !- set luce (soglia b) up +50    (CANTIb)
#define MASTRg 107 // !- set luce (soglia b) dn -50    (CANTIb)
#define MASTRh 108 // !---> get soglie                 (CANTIb)
#define MASTRi 109 // !- set AGC delay up +300         (CANTIc)
#define MASTRj 110 // !- set AGC delay dn -300         (CANTIc)
#define MASTRk 111 // !---> get AGC delay              (CANTIc)
#define MASTRl 112 // >>> salva  EEPROM                (CANTIokA)
#define MASTRm 113 // >>> carica EEPROM                (CANTIokB)
#define MASTRn 114 // >>> carica DEFAULT               (CANTIokC)
#define MASTRo 115 // get temp/luce STATO/tempo        (CANTId)
#define MASTRp 116 // rele A ON                        (CANTIa)
#define MASTRq 117 // rele A OFF                       (CANTIa)
#define MASTRr 118 // rele A toggle                    (CANTIa)
#define MASTRpp 119 // rele B ON                       (CANTIa)
#define MASTRqq 120 // rele B OFF                      (CANTIa)
#define MASTRrr 121 // rele B toggle                   (CANTIa)
// interno
#define MASTRaa 122 // ogni minuto MASTRa
#define MASTRab 123 // disable ogni minuto MASTRa
#define MASTRoo 124 // ogni minuto MASTRo
#define MASTRop 125 // disable ogni minuto MASTRo
#define MASTRdon 126 // enable invio a display
#define MASTRdof 127 // disable invio a display
#define MASTRclear 130 // clear display
// caldaia
#define MASTCa 150 // leggi tempo led A/B/C
#define MASTCb 151 // leggi tempo led D
#define MASTCc 152 // get stato leds
// pfsense
#define MASTSa 200 // move servoA (push button)
#define MASTSb 201 // move servoB (push button)
#define MASTSa 202 // get stato leds servoA and servoB
#define MASTPa 250 // get stato leds pfSense
#define MASTPb 251 // shutdown pfSense
#define MASTPc 252 // reboot pfSense
/*--------------------------------
** risposte (IN)
*/
#define CANTIa   1000 // get value luce/temp/rele
#define CANTIb   1001 // get soglie luce/temp
#define CANTIc   1002 // get AGC 
#define CANTId   1003 // get temp/luce STATO/tempo
#define CANTIokA 1004 // get ok salva eprom
#define CANTIokB 1005 // get ok carica eprom
#define CANTIokC 1006 // get ok carica default
#define CALDAa   1010 // get tempo led A/B/C
#define CALDAb   1011 // get tempo led D
#define CALDAc   1012 // get stato leds
#define SERVOa   1013 // ok pushbutton A
#define SERVOb   1014 // ok pushbutton B
#define SERVOc   1015 // get stato leds servoA and servoB
#define PFSENa   1016 // get status of leds
#define PFSENb   1017 // ok shutdown
#define PFSENc   1018 // ok reboot
#define PFSENc   1019 // operation not possible
/*--------------------------------
** radio tx rx
*/
byte CIFR[]={223,205,228,240,43,146,241,//
	     87,213,48,235,131,6,81,26,//
	     70,34,74,224,27,111,150,22,//
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
** LCM
*/
byte    BYTEradioindirDISPLAY[VW_MAX_MESSAGE_LEN];
String  CARATTERI;
// trasmissione radio 
#define DISPLAYindirizzoLSB    0
#define DISPLAYindirizzoMSB    1
#define DISPLAYcolonna         2
#define DISPLAYriga            3
#define DISPLAYnCaratteri      4
#define DISPLAYinizioTesto     5
#define VELOCITAhi          2000
#define delayAGCdisplay      300
// CARATTERI personalizzati
#define SIMBluce  1 //0 -> incrementato x problemi con String
#define SIMBtermo 2 //1
#define SIMBlivB  3 //2
#define SIMBlivC  4 //3
#define SIMBlivD  5 //4
#define SIMBlivE  6 //5
#define SIMBlivF  7 //6
#define SIMBgiu   8 //7
// CARATTERI interni
#define SIMBsu    B01011110
#define SIMBlivA  B01011111
#define SIMBon    255 //* cambia secondo il display
#define SIMBoff   252 //* cambia secondo il display
/*--------------------------------
** stati
*/
#define SALITA              1
#define DISCESA             0
#define LUCEpoca            0
#define LUCEmedia           1
#define LUCEtanta           2
#define RELEon              1
#define RELEoff             0
/*--------------------------------
** rx infrarossi IR
*/
#define KEY_1     -983386969
#define KEY_2    -1519975698
#define KEY_3     1007641363
#define KEY_4       11835754
#define KEY_5     -374420087
#define KEY_6     1316801890
#define KEY_7    -1829136225
#define KEY_8    -1395652082
#define KEY_9      497848941
#define KEY_0    -1975719008
#define KEY_UP    2113210209
#define KEY_DN     712987970
#define KEY_OK   -1812574087
#define KEY_CLEAR -477592334
//
IRrecv irrecv(pin_ir); // ir initialize library
decode_results irX;    // ir variable
//
int NUMcomp=0;  // numero che viene composto con IR
/*--------------------------------
** EEPROM
*/
#define eepMASTRa 0
#define eepDISPLAY 1
#define eepMASTRo 2
bool autoMASTRa=false;  // tx automatico MASTRa (valori)
bool autoMASTRo=false;  // tx automatico MASTRo (stato)
bool DISPLAYenable=true;
#define secAA 1            // al secondo 1 di ogni min, se abilitato, tx MASTRa
#define secOO 10           // al secondo 10 di ogni min, se abilitato, tx MASTRo
/*--------------------------------
** tempo
*/
unsigned long tempo;
byte decimi;
byte secondi;
byte minuti;
#define maxWAITresponse 3000 // massimo tempo di attesa per una risposta (ms)
//
/*--------------------------------
* setup
*/
void setup()
{
  vw_set_tx_pin(pin_tx); 
  vw_set_rx_pin(pin_rx);  
  vw_setup(VELOCITAstd);       
  vw_rx_start();               
  irrecv.enableIRIn();
  DISPLAYenable=EEPROM.read(eepDISPLAY);
  autoMASTRa=EEPROM.read(eepMASTRa);
  autoMASTRo=EEPROM.read(eepMASTRo);
}
//
/*--------------------------------
* loop()
*/
void loop(){
  // tieni il tempo
  if ((abs(millis()-tempo))>100){
    tempo=millis();
    decimi++;
    //BEGIN ogni decimo
    //END   ogni decimo
    if (decimi>9){
      //BEGIN ogni secondo
      switch(secondi){
      case secAA:
	if (autoMASTRa){INTERIlocali[MESSnum]=MASTRa;  tx();}	break;
      case secOO:
	if (autoMASTRo){INTERIlocali[MESSnum]=MASTRo;  tx();}	break;	  	
      }
      //END ogni secondo
      decimi=0;
      secondi++;
      if (secondi>59){
	//BEGIN ogni minuto
	//END   ogni minuto
	secondi=0;
	minuti++;
	if (minuti>250){
	  minuti=0;
	}
      }
    }
    // IR
    chechForIR();
  }
}
//
/*--------------------------------
* ritrasmette()
*/
void ritrasmette(){
  // 
  // ritrasmette i messaggi ricevuti 
  // cambiando id ma mantenendo i dati ricevuti
  // 
  int ss =INTERIlocali[MESSnum];
  int da =INTERIlocali[DATOa];
  int db =INTERIlocali[DATOb];
  int dc =INTERIlocali[DATOc];
  int mm = ss+10000;
  INTERIlocali[MESSnum]=mm;
  tx();
  INTERIlocali[MESSnum]=ss;
  INTERIlocali[DATOa]=da;
  INTERIlocali[DATOb]=db;
  INTERIlocali[DATOc]=dc;
  delay(100);
}
//
/*--------------------------------
* ir_decode()
*/
// decodifica ir
//
long ir_decode(decode_results *irX){
  long keyLongNumber = irX->value;
  //Serial.println(keyLongNumber);
  return keyLongNumber;
}
//
/*--------------------------------
* chechForIR()
*/
// controlla se segnale IR
//
void chechForIR(){
  ///////start check for IR///////
  if (irrecv.decode(&irX)) {
    long key=ir_decode(&irX);
    ////////start switch////////////
    switch (key){
    case KEY_OK:
      switch (NUMcomp){
	// comandi interni
      case MASTRclear: clearDISPLAY(); NUMcomp=0; break;
      case MASTRaa:autoMASTRa=true;EEPROMsaveRipeti(); break;
      case MASTRab:autoMASTRa=false;EEPROMsaveRipeti();	break;
      case MASTRoo:autoMASTRo=true;EEPROMsaveRipeti();	break;
      case MASTRop:autoMASTRo=false;EEPROMsaveRipeti();	break;
      case MASTRdon:DISPLAYenable=true;EEPROM.write(eepDISPLAY,DISPLAYenable);break;
      case MASTRdof:DISPLAYenable=false;EEPROM.write(eepDISPLAY,DISPLAYenable);break;
	// comandi radio
      default:	stampaNc();INTERIlocali[MESSnum]=NUMcomp;tx();break;
      }
      break;
    case KEY_1: scorriNumero(1);break;
    case KEY_2: scorriNumero(2);break;
    case KEY_3: scorriNumero(3);break;
    case KEY_4: scorriNumero(4);break;
    case KEY_5: scorriNumero(5);break;
    case KEY_6: scorriNumero(6);break;
    case KEY_7: scorriNumero(7);break;
    case KEY_8: scorriNumero(8);break;
    case KEY_9: scorriNumero(9);break;
    case KEY_0: scorriNumero(0);break;
    case KEY_CLEAR: NUMcomp=0; stampaNc(); break;
    case KEY_UP:NUMcomp++; stampaNc(); break;
    case KEY_DN:NUMcomp--; stampaNc(); break;      
    }
    ////////end switch////////////////    
    delay(100);
    irrecv.resume();
  }
  /////end check for IR///////////
}
//
/*--------------------------------
* txDISPLAY()
*/
// trasmette al display via radio
// occorre riempire anche il testo
// "CARATTERI"
//
void txDISPLAY(byte colonna, byte riga){
  // pulisce bytes-radio
  for (byte n=0;n<20;n++){
    BYTEradioindirDISPLAY[n]=0;
  }
  // indirizzo display
  int g=MASTRdisplay;
  // caricamento int su bytes
  BYTEradioindirDISPLAY[0]=g & mask;
  g=g >> 8;
  BYTEradioindirDISPLAY[1]=g & mask;
  // caricamento riga e colonna su bytes
  BYTEradioindirDISPLAY[2]=colonna;
  BYTEradioindirDISPLAY[3]=riga;
  // caricamento lunghezza testo su bytes
  BYTEradioindirDISPLAY[4]=CARATTERI.length();
  // la lunghezza deve essere non superiore a 20
  if (BYTEradioindirDISPLAY[4]>20){
    BYTEradioindirDISPLAY[4]=20;
  }
  // caricamento messaggio su bytes
  for (byte n=5;n<BYTEradioindirDISPLAY[4]+5;n++){
    BYTEradioindirDISPLAY[n]=CARATTERI[n-5];
  }
  // cifratura
  for (byte n=0; n<27;n++){
    BYTEradioindirDISPLAY[n]=BYTEradioindirDISPLAY[n] ^ CIFR[n];
  }
  // tx via radio
  vw_rx_stop();
  // cambio di velocita
  vw_setup(VELOCITAhi);
  vw_send((uint8_t *)BYTEradioindirDISPLAY,VW_MAX_PAYLOAD);
  vw_wait_tx();
  // ritardo per AGC
  delay(delayAGCdisplay);
  // ripristino velocita
  vw_setup(VELOCITAstd);
  vw_rx_start();
}
//
/*--------------------------------
* stampaNc()
*/
// trasmette al display il numero
// ricevuto via IR
//
void stampaNc(){
  char buf[5];
  CARATTERI="     ";
  if (NUMcomp!=0){
    sprintf(buf, "%5d",NUMcomp);
    CARATTERI=buf;
  }
  txDISPLAY(10,0);//---->
}
/*--------------------------------
* scorriNumero()
*/
// viene composto il numero digitato
// via IR
//
void scorriNumero(byte aggiungi){
  NUMcomp=NUMcomp*10+aggiungi;
  if (NUMcomp<0){
    NUMcomp=0;
  }  
  stampaNc();
}
/*--------------------------------
* INTtoBYTE()
*/
// conversione da intero a due bytes
//
void INTtoBYTE(int x, byte& lsb, byte& msb){
  lsb =x & 0x00FF;
  x = x >> 8;
  msb = x & 0x00FF;
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
  // attende al massimo 3 secondi per una risposta
  if (vw_wait_rx_max(maxWAITresponse)){
    /*--------------------------------
    ** radio rx
    */
    if (vw_get_message(BYTEradio, &buflen)){
      char buf[20];
      //
      vw_rx_stop();
      //
      decodeMessage();
      ritrasmette();
      if (DISPLAYenable){
	switch (INTERIlocali[MESSnum]){
	  case CALDAa:
	    clearDISPLAY();
	    CARATTERI = "STATO CALDAIA (" + String(CALDAa) + ")";
	    txDISPLAY(0,0);
	    //            12345678901234567890
	    sprintf(buf,"minuti TERMO : %4d",INTERIlocali[DATOa]);
	    CARATTERI=String(buf);
	    txDISPLAY(0,1);
	    //            12345678901234567890
	    sprintf(buf,"minuti ACQUA : %4d",INTERIlocali[DATOb]);
	    CARATTERI=String(buf);
	    txDISPLAY(0,2);
	    //            12345678901234567890
	    sprintf(buf,"minuti FIAMMA: %4d",INTERIlocali[DATOc]);
	    CARATTERI=String(buf);
	    txDISPLAY(0,3);	    	    
	    break;
	  case CALDAb:
	    clearDISPLAY();
	    CARATTERI = "STATO CALDAIA (" + String(CALDAb) + ")";
	    txDISPLAY(0,0);
	    sprintf(buf,"minuti ON: %4d",INTERIlocali[DATOa]);
	    CARATTERI=String(buf);
	    txDISPLAY(0,1);
	    break;
	  case CALDAc:
	    clearDISPLAY();
	    CARATTERI = "STATO CALDAIA (" + String(CALDAc) + ")";
	    txDISPLAY(0,0);
	    CARATTERI = "TERMO : " +  getONorFF(INTERILOCALI[DATOa]&1)
	    txDISPLAY(0,1);
	    CARATTERI = "ACQUA : " +  getONorFF(INTERILOCALI[DATOa]&2)
	    txDISPLAY(0,2);	    
	    CARATTERI = "FIAMMA: " +  getONorFF(INTERILOCALI[DATOa]&4)
	    txDISPLAY(0,3);	    
	    CARATTERI = "caldaia: " +  getONorFF(INTERILOCALI[DATOa]&8)
	    txDISPLAY(9,3);	    
	    break;
	  case CANTIokA:
	    clearDISPLAY();
	    CARATTERI =  "DIGITAL-A (" + String(CANTIokA) + ")";
	    txDISPLAY(0,0);	    
	    CARATTERI = "OK salva su EEPROM";
	    txDISPLAY(0,1);	    
	    break;
	  case CANTIokB:
	    clearDISPLAY();
	    CARATTERI = "DIGITAL-A (" + String(CANTIokB) + ")";
	    txDISPLAY(0,0);	    
	    CARATTERI = "OK carica da EEPROM";
	    txDISPLAY(0,1);	    	    
	    break;
	  case CANTIokC:
	    clearDISPLAY();
	    CARATTERI = "DIGITAL-A (" + String(CANTIokC) + ")";
	    txDISPLAY(0,0);	    
	    CARATTERI = "OK carica default";
	    txDISPLAY(0,1);	    	    	    
	    break;    
	  case CANTIa:
	    clearDISPLAY();
	    CARATTERI =  "DIGITAL-A (" + String(CANTIa) + ")";
	    txDISPLAY(0,0);
	    CARATTERI = "TEMP: " + String(INTERIlocali[DATOb]);
	    txDISPLAY(0,1);
	    CARATTERI = "LUCE: " + String(INTERIlocali[DATOa]);
	    txDISPLAY(9,1);
	    CARATTERI = "rele A: " +  getONorFF(INTERILOCALI[DATOc]&1)
	    txDISPLAY(0,2);
	    CARATTERI = "rele B: " +  getONorFF(INTERILOCALI[DATOc]&2)
	    txDISPLAY(9,2);
	    break;
	  case CANTIb:
	    clearDISPLAY();
	    CARATTERI =  "DIGITAL-A (" + String(CANTIb) + ")";
	    txDISPLAY(0,0);
	    CARATTERI = "soglia TEMP: " + String(INTERIlocali[DATOa]);
	    txDISPLAY(0,1);
	    CARATTERI = "soglia 'A' LUCE: " + String(INTERIlocali[DATOb]);
	    txDISPLAY(0,2);
	    CARATTERI = "soglia 'B' LUCE: " + String(INTERIlocali[DATOc]);
	    txDISPLAY(0,3);
	    break;
	  case CANTIc:
	    clearDISPLAY();
	    CARATTERI =  "DIGITAL-A (" + String(CANTIc) + ")";
	    txDISPLAY(0,0);
	    CARATTERI = "delay AGC: " + String(INTERIlocali[DATOa]);
	    txDISPLAY(0,1);
	    break;
	  case CANTId:
	    clearDISPLAY();
	    CARATTERI =  "DIGITAL-A (" + String(CANTId) + ")";
	    txDISPLAY(0,0);   
	    byte stLuce=INTERIlocali[DATOa] & 0x00FF;
	    CARATTERI="stato LUCE: ";
	    switch (stLuce){
	    case LUCEpoca:
	      CARATTERI+="poca";
	      break;
	    case LUCEmedia:
	      CARATTERI+="media";
	      break;
	    case LUCEtanta:
	      CARATTERI+="tanta";
	      break;	      	      
	    }
	    txDISPLAY(0,1);	    
	    //
	    INTERIlocali[DATOa]=INTERIlocali[DATOa] >> 8;	    
	    byte stTemp=INTERIlocali[DATOa] & 0x00FF;	    
	    CARATTERI="stato TEMP: ";
	    switch (stTemp){
	    case SALITA:
	      CARATTERI+="salita";
	      break;
	    case DISCESA:
	      CARATTERI+="discesa";
	      break;	      
	    }
	    txDISPLAY(0,2);
	    //
	    CARATTERI="min T: " +string(INTERIlocali[DATOb]);
	    CARATTERI+=" L: " +string(INTERIlocali[DATOC]);
	    txDISPLAY(0,3);
	    break;
	  }
      }
      //
      vw_rx_start();
      //
    } 
    //CARATTERI = "  ";
    //txDISPLAY(0,0); 
  } else {
    clearDISPLAY();
    CARATTERI = "<nessuna risposta>";
    txDISPLAY(0,0); 
  }
}
/*--------------------------------
** clearDISPLAY()
*/
void clearDISPLAY(){
CARATTERI = "<clear>";
txDISPLAY(0,0);     
//txDISPLAY(0,1);     
//txDISPLAY(0,2);   
//txDISPLAY(0,3);  
}

String getONorFF(byte value){
  if (value){
    return "ON";
  } else {
    return "off";
  }
}
