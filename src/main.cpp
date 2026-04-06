#include <Arduino.h>

void setup() {
  // Initialisiere die LED-Pins als Ausgänge
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  // Alle LEDs zuerst ausschalten (HIGH = AUS)
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
}

void loop() {
  // Rot an
  digitalWrite(LED_RED, LOW);
  delay(500);
  digitalWrite(LED_RED, HIGH);

  // Grün an
  digitalWrite(LED_GREEN, LOW);
  delay(500);
  digitalWrite(LED_GREEN, HIGH);

  // Blau an
  digitalWrite(LED_BLUE, LOW);
  delay(500);
  digitalWrite(LED_BLUE, HIGH);
}

