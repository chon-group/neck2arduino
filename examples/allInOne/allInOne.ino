#include <NECK.hpp>   /*  https://github.com/chon-group/neckArduino */
Apparatus(myApparatus); 

TacitKnowledge(myApparatus, nomequalquer1, "gasdfdsafsadfsadfsadfsdafdsafdsafasd");
TacitKnowledge(myApparatus, segundoNome, "contextoXXXX", "qwerwqerewqrwqerwqerwerweqrwe");

boolean blinkStatus = false;
int ledPin = 13;
void setup() {pinMode(ledPin, OUTPUT); pinMode(12, OUTPUT);}

void loop() {
  myApparatus.embody();
  if(blinkStatus){
    digitalWrite(ledPin,!digitalRead(ledPin));
    delay(250);
  }
}  

Element(myApparatus,led2);
Element(myApparatus,led);     
  
Action (led2,blinkOperation){
  if(!ActionArgs.isBool(0)) return INVALID; 
  if(blinkStatus == ActionArgs.asBool(0)) return ALREADY;

  NECKArgs out;
  out.add("aaa");
  out.add(0.877665);
  led2.trieb("dentro_da_blinkOperation",out,0.99);
  blinkStatus = ActionArgs.asBool(0);
  return EXECUTED;  
}

Action (led,toggleLED){
  led.trieb("dentrodatoggleLED",0.88);
  digitalWrite(ledPin,!digitalRead(ledPin));
  return EXECUTED;  
}

Perception (led2,ledStatus, PROPRIOCEPTION){
  if(digitalRead(ledPin)) return true;
  else return false;
}

Perception (led,ledID, PROPRIOCEPTION){
  led.trieb("funga",0.7);
  //if(testeQualquer) return UNAVAILABLE;
  //else 
  return ledPin;
}

Perception (led,outraCoisa, PROPRIOCEPTION){
  NECKArgs out;
  if(algumaFuncao) out.add("ok123");
  out.add(funcaRetornaTrueouFalse);

  led.trieb("foiSerah",out,0.3);
  return out;
}

bool algumaFuncao(){return true;}
bool funcaRetornaTrueouFalse(){return false;}
bool testeQualquer(){return false;}