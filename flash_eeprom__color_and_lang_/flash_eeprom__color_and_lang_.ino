#include <EEPROM.h>

// define global variables
byte eeC = 0;                                 // eeprom address for colorstate
byte eeL = 1;                                 // eeprom address for lang

void setup() {
  // write color
  EEPROM.write(eeC, 0);
  // write lang (0 dialekt; 1 deutsch)
  EEPROM.write(eeL, 0);
}

void loop() {

}
