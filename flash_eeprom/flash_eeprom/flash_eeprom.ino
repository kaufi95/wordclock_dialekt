#include <EEPROM.h>

const int eeC = 0;                    // eeprom address for colorstate
const int eeL = 1;                    // eeprom address for lang

void setup() {
  Serial.begin(9600);
  
  // write color
  EEPROM.put(eeC, 0);

  // write lang   (0: dialekt, 1: deutsch)
  EEPROM.put(eeL, 1);
}

void loop() {
  Serial.println("EEPROM:");
  for (int i = 0; i < 2; i++) {
    Serial.print("Byte ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(EEPROM.read(i));
  }
  delay(1000);
}
