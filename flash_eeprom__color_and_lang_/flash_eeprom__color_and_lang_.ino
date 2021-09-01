#include <EEPROM.h>

byte eeC = 0;
byte eeL = 1;

void setup() {
  EEPROM.put(eeC, 3);
  EEPROM.put(eeL, 0);
}

void loop() {

}
