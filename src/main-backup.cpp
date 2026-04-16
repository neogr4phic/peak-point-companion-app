#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Neue Werte für dein 0.91" Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
// Der Reset-Pin ist bei I2C-Displays meist nicht vorhanden, daher -1
#define OLED_RESET -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pins wie besprochen
const int PIN_CLK = D0; // CLK am Encoder
const int PIN_DT  = D1; // DT am Encoder
const int PIN_SW  = D2; // SW am Encoder

int counter = 0;
int lastStateCLK;
unsigned long lastButtonPress = 0;

void setup() {
  pinMode(PIN_CLK, INPUT_PULLUP);
  pinMode(PIN_DT,  INPUT_PULLUP);
  pinMode(PIN_SW,  INPUT_PULLUP);

  // Initialisierung mit der I2C-Adresse 0x3C
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); // Fehler: Display nicht gefunden
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  lastStateCLK = digitalRead(PIN_CLK);
}

void loop() {
  // 1. Encoder auslesen
  int currentStateCLK = digitalRead(PIN_CLK);
  if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {
    if (digitalRead(PIN_DT) != currentStateCLK) {
      counter--;
    } else {
      counter++;
    }
  }
  lastStateCLK = currentStateCLK;

  // 2. Taster auslesen
  if (digitalRead(PIN_SW) == LOW) {
    if (millis() - lastButtonPress > 200) {
      counter = 0;
      lastButtonPress = millis();
    }
  }

  // 3. Anzeige (alle 50ms)
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 50) {
    display.clearDisplay();
    
    // Kopfzeile
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("XIAO nRF52840 Test");
    
    // Trennlinie
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
    
    // Wert groß anzeigen
    display.setTextSize(2); 
    display.setCursor(0, 15);
    display.print("Val: ");
    display.print(counter);
    
    // Status-Indikator für Button
    if (digitalRead(PIN_SW) == LOW) {
      display.fillCircle(115, 22, 5, SSD1306_WHITE);
    }
    
    display.display();
    lastUpdate = millis();
  }
}