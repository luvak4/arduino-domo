/////////////////////////////
// timer per ventilatore a pale a soffitto
///////////////////////////

// Attraverso un pulsante "a peretta" è possibile impostare il tempo
// di accensione di un ventilatore (per due/quattro/sei/otto ore). Il tempo
// impostato e lo stato di avanzamento del tempo
// è visualizzato da 4 led

//bool DEBUG =true;
const int inPulsante = 7;
const int LED1 = 2;
const int LED2 = 3;
const int LED3 = 4;
const int LED4 = 5;
const int RELE = 6;
bool pStatoPrec = LOW;
unsigned long int contaPremuto = 0;
unsigned long int contaRilasciato = 0;
//
//  0===|=======|============|=====================|====......
//  |   |       |            |                    >|<- pausa tra parole
//  |   |       |           >|<----------------------- linea
//  |   |      >|<------------------------------------ punto
//  |  >|<-------------------------------------------- disturbo
// >|<------------------------------------------------ 0
//
int tempoDisturbo = 2; // quarti di decimi di secondo ("disturbo")
int tempoPunto = 10;   // quarti di decimi di secondo ("punto")
int tempoLinea = 40;   // quarti di decimi di secondo ("linea")
int tempoLungo = 160;  // quarti di decimi di secondo ("pausa tra parole")
int giaPassato;
int stepOraA = 7200;   //secondi in 2 ore
int stepOraB = 14400;  //secondi in 4 ore
int stepOraC = 21600;  //secondi in 6 ore
int stepOraD = 28800;  //secondi in 8 ore
int tempoAspegne=0;
bool conteggioAttivo = false;
int dutyCycle = 0;
unsigned long int Pa;
unsigned long int Pb;
int oreProgrammate;
int segno;
//================================
// setup
//================================
void setup() {
  pinMode(inPulsante,INPUT);
  //
  pinMode(LED1,OUTPUT);
  digitalWrite(LED1,LOW);
  //
  pinMode(LED2,OUTPUT);
  digitalWrite(LED2,LOW);
  //
  pinMode(LED3,OUTPUT);
  digitalWrite(LED3,LOW);
  //
  pinMode(LED4,OUTPUT);
  digitalWrite(LED4,LOW);
  //
  pinMode(RELE,OUTPUT);
  digitalWrite(RELE,LOW);
  // solo per test
  // Serial.begin(115200);
  // 
  // LAMPEGGIO DEL PIN 13
  // poi lo spengo 
  //
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
  delay(500);
  digitalWrite(13,HIGH);
  delay(500);
  digitalWrite(13,LOW);  
}
//================================
// loop
//================================
void loop() {
  unsigned long int Qa;
  unsigned long int Qb;
  int DIFFa;
  int DIFFb;
  int Xa;
  int Xb;
  //
  dutyCycle += 1;
  if (dutyCycle > 9){
    dutyCycle = 0;
  }
  if (dutyCycle > 0){
    Qa=millis();
    if (Qa >= Pa){
      DIFFa=Qa-Pa;
      Xa = DIFFa - 25;
      if (Xa >= 0){
        Pa = Qa;
        //--------------------------------
        // da qui passa ogni 0.025 Sec
	      // quarto di decimo di secondo
        //--------------------------------
        if (digitalRead(inPulsante)){
          chkPremuto();
        } else {
          chkRilasciato();
        }        
      }
    } else {
      Pa = Qa - Xa;
    }
  } else {  
    Qb=millis();
    if (Qb >= Pb){
      DIFFb=Qb-Pb;
      Xb = DIFFb - 1000;
      if (Xb >= 0){
        Pb = Qb - Xb;
        //--------------------------------
        // da qui passa ogni secondo
        //--------------------------------
        chkTempo();
        setLEDs();
      }
    } else {
      Pb = Qb;
    }      
  }
}
void chkTempo(){
  //================================
  // se il conteggio e attivo
  // imposta le ore programmate
  // e lo stato del rele
  // sulla base del tempoAlloSpegnimento
  //================================
  if (conteggioAttivo){
    tempoAspegne -= 1;
    if (tempoAspegne == 0){
      oreProgrammate = 0;
      digitalWrite(RELE,LOW);
      //--------------------------------
      // spegne il conteggio del
      // tempoAlloSpegnimento
      //--------------------------------
      conteggioAttivo = false;
    } else {
      if (tempoAspegne > stepOraD){
        // non dovrebbe mai verificarsi
	tempoAspegne = stepOraD - 1;
      } else {
        if (tempoAspegne > stepOraC){
          oreProgrammate = 4;
          digitalWrite(RELE,HIGH);
        } else {
          if (tempoAspegne > stepOraB){
            oreProgrammate = 3;
            digitalWrite(RELE,HIGH);
          } else {
            if (tempoAspegne > stepOraA){
              oreProgrammate = 2;
              digitalWrite(RELE,HIGH);
            } else { 
              oreProgrammate = 1;
              digitalWrite(RELE,HIGH);
            }
          }
        }
      }
    }
  }
}
//================================
// impostazioni led in base alle
// ore programmate
//================================
void setLEDs(){
  if (oreProgrammate == 0){
    digitalWrite(LED1,LOW);
    digitalWrite(LED2,LOW);
    digitalWrite(LED3,LOW);
    digitalWrite(LED4,LOW);
  } else {
    if (oreProgrammate == 1){
      digitalWrite(LED1,HIGH);
      digitalWrite(LED2,LOW);
      digitalWrite(LED3,LOW);
      digitalWrite(LED4,LOW);
    } else {
      if (oreProgrammate == 2){
        digitalWrite(LED1,HIGH);
        digitalWrite(LED2,HIGH);
        digitalWrite(LED3,LOW);
        digitalWrite(LED4,LOW);
      } else {
        if (oreProgrammate == 3){
            digitalWrite(LED1,HIGH);
            digitalWrite(LED2,HIGH);
            digitalWrite(LED3,HIGH);
            digitalWrite(LED4,LOW);
        } else {
	    digitalWrite(LED1,HIGH);
            digitalWrite(LED2,HIGH);
            digitalWrite(LED3,HIGH);
            digitalWrite(LED4,HIGH);
        }        
      }      
    }
  }
}
//================================
// pulsante rilasciato
//================================
void chkRilasciato(){
  if (pStatoPrec == HIGH){
    pStatoPrec = LOW;
    contaRilasciato =0;
  } else {
    contaRilasciato +=1;
    if (contaRilasciato > tempoLungo){
      if (giaPassato != 4){
        giaPassato = 4;
        // funzione D
       }
    } else {
      if (contaRilasciato > tempoLinea){
        if (giaPassato != 3){
	  giaPassato = 3;	
          // funzione C
	  //--------------------------------
	  // attiva il conteggio del
	  // tempoAlloSpegnimento
	  //--------------------------------
	  if (oreProgrammate > 0){
	    conteggioAttivo = true;
	  }
        }
      } else {
        if (contaRilasciato > tempoPunto){
          if (giaPassato != 2){
	    giaPassato = 2;	
            // funzione B
          }
        } else {
          if (contaRilasciato > tempoDisturbo){
            if (giaPassato != 1){
	      giaPassato = 1;	
              // funzione A
              programmaLeOre();
              impostaTempo();
            }
          } else {
	    if (giaPassato != 0){
	      giaPassato = 0;
	    }
          }          
        }        
      }      
    }
  }
}
//================================
// pulsante premuto
//================================
void chkPremuto(){
  if (pStatoPrec == LOW){
    pStatoPrec = HIGH;
    contaPremuto =0;
  } else {
    contaPremuto +=1;
    if (contaPremuto > tempoLungo){
      if (segno != 4){
        segno = 4;
       }
    } else {
      if (contaPremuto > tempoLinea){
        if (segno != 3){
	  segno = 3;	
        }
      } else {
        if (contaPremuto > tempoPunto){
          if (segno != 2){
	    segno = 2;	
          }
        } else {
          if (contaPremuto > tempoDisturbo){
            if (segno != 1){
	      segno = 1;	
            }
          } else {
	    if (segno != 0){
	      segno = 0;
	    }
          }          
        }        
      }      
    }
  }
}
//================================
// programma le ore
//================================
void programmaLeOre(){
  if (segno == 1){
    //--------------------------------
    // tempo corto (punto)
    //--------------------------------
    oreProgrammate += 1;
    if (oreProgrammate > 4){
      oreProgrammate = 0;
    }
  } else {
    if (segno > 1 ){
      //--------------------------------
      // tempi piu lunghi (linea o tempo lungo)
      //--------------------------------
      if (oreProgrammate > 0){
	oreProgrammate = 0;  // spegne se acceso
      } else {
	oreProgrammate = 3; // accende se spento
      }
    }
  }
}
//================================
// impostazioni tempo
//================================
void impostaTempo(){
  switch (oreProgrammate){
    case 0:
      tempoAspegne = 1; // fine
      break;
    case 1:
      tempoAspegne = stepOraA;
      break;
    case 2:
      tempoAspegne = stepOraB;
      break;
    case 3:
      tempoAspegne = stepOraC;
      break;
    case 4:
      tempoAspegne = stepOraD;
      break;
  }
}
