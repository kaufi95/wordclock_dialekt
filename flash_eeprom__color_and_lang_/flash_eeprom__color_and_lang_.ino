#include <EEPROM.h>

void setup() {
  // write color
  EEPROM.put(0, 3);
  // write lang
  EEPROM.put(1, 0);
}

void loop() {

}
