#include <EEPROM.h>

byte eeC = 0;
byte eeL = 1;

void setup() {
  EEPROM.put(eeC, 0);
  EEPROM.put(EEL, 0);
}

void loop() {

}
