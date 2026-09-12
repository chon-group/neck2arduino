#include <NECK.hpp>   /* https://github.com/chon-group/neck2arduino */

#define LED_PIN 13

/*Apparatus Description*/
Apparatus(arduinoBoard) {
    Element(led);
}

bool blinking = false;
bool ledStatus = false;
unsigned long previousBlink = 0;
void setup() {pinMode(LED_PIN, OUTPUT);}
void loop() {arduinoBoard.embody();}

/* Apparatus configuration*/
Percept(led, ledStatus, PROPRIOCEPTION) {
  if(digitalRead(LED_PIN)) return true;
  return false;
}

Act(led, blinkOn) {
  if (blinking) return ALREADY;

  blinking = true;
  return EXECUTED;
}

Act(led, blinkOff) {
  if (!blinking) return ALREADY;

  blinking = false;
  digitalWrite(LED_PIN, LOW);

  return EXECUTED;
}

Behaving(led) {
  if (blinking && millis() - previousBlink >= 250) {
      previousBlink = millis();
      ledStatus = !ledStatus;
      digitalWrite(LED_PIN, ledStatus);
  }
}