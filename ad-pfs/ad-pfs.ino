#include <VirtualWire.h>
const int led_pin = 13;
const int led_rosso_pin = 2;
const int led_giallo_pin = 3
const int led_verde_pin = 4;
const int led_blu_pin = 5;
const int receive_pin = 11;
const int transmit_pin = 12;
// max lenght of my message
const int MSG_LEN = 7;
//
#define ST_FIRSTBOOT    1
#define ST_BOOTING      2
#define ST_DOOFF_A      3
#define ST_DOOFF_B      4
#define ST_DOOFF_C      5
#define ST_DOREBOOT_A   6
#define ST_DOREBOOT_B   7
#define ST_DOREBOOT_C   8
#define ST_ON           9
#define ST_OFF         10
#define ST_SYNC        11
#define ST_PING        12
#define ST_XX          16
////////////////////////////////
// setup
////////////////////////////////
void setup() {
  Serial.begin(9600); // connection to pfSense serial port
  Serial.flush();     // flush
  pinMode(led_pin, OUTPUT);        // led
  pinMode(led_rosso_pin, OUTPUT);  // led red
  pinMode(led_giallo_pin, OUTPUT); // led yellow
  pinMode(led_verde_pin, OUTPUT);  // led green
  pinMode(led_blu_pin, OUTPUT);    // led blue
  vw_set_tx_pin(transmit_pin);     // radio set tx pin
  vw_set_rx_pin(receive_pin);      // radio set rx pin
  vw_setup(2000);                  // radio speed
  vw_rx_start();                   // radio rx ON
}

////////////////////////////////
// loop
////////////////////////////////
void loop() {
  uint8_t buf[MSG_LEN]={0,0,0,0,0,0,0}; // empty buffer
  uint8_t buflen = MSG_LEN;             // lenght of buffer
  if (vw_get_message(buf, &buflen)){    // received a message?
    if (buf[0]==0xAA){
      switch (buf[1]){
      case 0x01:if(intlStep==ST_ON){intlStep=ST_DOOFF_A;}   break;
      case 0x02:if(intlStep==ST_ON){intlStep=ST_DOREBOOT_A;} break;
      case 0x03:if(intlStep==ST_ON){intlStep=ST_SYNC;}
	if(intlStep==ST_FIRSTBOOT){intlStep=ST_SYNC;}  break;
      case 0x04:if(intlStep==ST_ON){intlStep=ST_PING;} break;
      case 0x05:if(intlStep==ST_ON){intlStep=ST_XX;} break;
      }
    }
  }
}

bool ckSerial(String text){
  if (Serial.available() > 0) {
    if (Serial.find(text)){
      return true;
    } else {
      return false;
    }
  }
}

void ParteSeriale(){
  switch (intlStep){
  case ST_FIRSTBOOT: if(ckSerial("PC Engines ALIX")){intlStep=ST_BOOTING;txStat(ST_FIRSTBOOT);} break;
  case ST_BOOTING:if(ckSerial("Enter an option")){intlStep=ST_ON;txStat(ST_ON);} break;
  case ST_DOOFF_A:Serial.write("6\n");intlStep=ST_DOOFF_B; break;
  case ST_DOOFF_B:if(ckSerial("proceed")){intlStep=ST_DOOFF_C;txStat(ST_DOOFF_C);} break;
  case ST_DOOFF_C:if(ckSerial("has halted")){intlStep=ST_FIRSTBOOT;txStat(ST_OFF);} break;
  case 6:
    //--------------------------------
    // reboot
    //--------------------------------
    Serial.write("5\n");
    intlStep=7;
    break;	  	  
  case 7:
    if (Serial.available()) {
      if (Serial.find("proceed")){
	intlStep=8;
	Serial.write("y\n");
	// pfSense in shutdown
	//msgStatoServer = prefissoStatoServer + "3";
	txStat('3');
      }
    }
    break;	  
  case 8:
    if (Serial.available()) {
      if (Serial.find("Rebooting")){
	intlStep=1;
	// pfSense OFF
	txStat('4');
      }
    }
    break; 
  case 11:
    //--------------------------------
    // sync Arduino -> pfSense
    //--------------------------------
    Serial.write("exit\n");
    delay(2000); 
    Serial.write("\n");
    delay(5000);
    intlStep=2;
    // attempt sync
    txStat('0');
    break;        
  case 12:
    //--------------------------------
    // ping on google
    //--------------------------------
    Serial.write("7\n");
    intlStep=13;
    break;   
  case 13:
    if (Serial.available()) {
      if (Serial.find("or IP address")){
	intlStep=14;     
      }
    }
    break;
  case 14:
    Serial.write("www.google.it\n");
    intlStep=15;
    break;   
  case 15:  
    if (Serial.available()) {
      if (Serial.find("0.0% packet")){
	// internet OK
	txStat('5');
      } else {
	// internet KO
	txStat('6');
      }
      intlStep=16;
    } 
    break;
  case 16:
    // "push a key..."
    Serial.write("\n");
    intlStep=9;
    break;
  }
}
//================================
// send "command receive" to "display"
//================================
void txRicevutoComando(){
  digitalWrite(pinLED, HIGH);
  vw_rx_stop(); // disable rx section
  vw_send((uint8_t *)msgTxComandoRicevuto,MSG_LEN);
  vw_wait_tx(); // Wait until the whole message is gone
  vw_rx_start(); // enable rx section
  digitalWrite(pinLED, LOW);
}
void txStat(char charState){
  switch (charState){
  case '0':
    // attempt sync
    turnOFFleds();
    break;
  case '4':
    // pfSense is OFF
    digitalWrite(pinLEDrosso,HIGH);
    digitalWrite(pinLEDgiallo,LOW);
    digitalWrite(pinLEDverde,LOW);
    digitalWrite(pinLEDblu,LOW);
    break;  
  case '1'://
    // pfSense is in ignition state
    digitalWrite(pinLEDrosso,LOW);
    digitalWrite(pinLEDgiallo,HIGH);
    digitalWrite(pinLEDverde,LOW);
    digitalWrite(pinLEDblu,LOW); 
    break;
  case '3'://
    // pfSense in shutdown
    digitalWrite(pinLEDrosso,LOW);
    digitalWrite(pinLEDgiallo,HIGH);
    digitalWrite(pinLEDverde,LOW);
    digitalWrite(pinLEDblu,LOW);  
    break;     
  case '2'://
    // pfSense ON
    digitalWrite(pinLEDrosso,LOW);
    digitalWrite(pinLEDgiallo,LOW);
    digitalWrite(pinLEDverde,HIGH);
    digitalWrite(pinLEDblu,LOW);
    break; 
  case '6'://
    // internet KO
    turnOFFleds();
    delay(1000);    
    //
    digitalWrite(pinLEDrosso,HIGH);
    digitalWrite(pinLEDgiallo,LOW);
    digitalWrite(pinLEDverde,HIGH);
    digitalWrite(pinLEDblu,LOW); 
    break;      
  case '5':
    // internet OK
    turnOFFleds();
    delay(1000);    
    //
    digitalWrite(pinLEDrosso,LOW);
    digitalWrite(pinLEDgiallo,LOW);
    digitalWrite(pinLEDverde,HIGH);
    digitalWrite(pinLEDblu,HIGH);  
    break;                          
  }
  //
  msgTxStatoServer[POSIZIONE_CARATT]=charState;
  vw_rx_stop(); // disable rx section
  vw_send((uint8_t *)msgTxStatoServer,MSG_LEN);
  vw_wait_tx(); // Wait until the whole message is gone
  vw_rx_start(); // enable rx section
  msgTxStatoServer[POSIZIONE_CARATT]='0';
}
//================================
// OFF leds
//================================
void turnOFFleds(){
  digitalWrite(pinLEDrosso,LOW);
  digitalWrite(pinLEDgiallo,LOW);
  digitalWrite(pinLEDverde,LOW);
  digitalWrite(pinLEDblu,LOW);    
}
