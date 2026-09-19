#include <NECK.hpp>   /* https://github.com/chon-group/neck2arduino */

#define LED_PIN 13
bool blinking = true;
bool ledStatus = false;
unsigned long previousBlink = 0;

Apparatus(arduinoBoard) {
    Element(led);
}

Preparation{
  pinMode(LED_PIN, OUTPUT);
}


Sensing(led){
  ledStatus = digitalRead(LED_PIN);
}

Percept(led, ledStatus, PROPRIOCEPTION) {
  return ledStatus;
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