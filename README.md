# wordclock dialekt version

This is a wordclock project based on different other versions of wordclocks. The main difference is that this version can display the time in the regular german language as well as an specific dialect of the german language (Vorarlbergerisch / Austria). The corresponding sheet metal letter matrix needs to be used for the specific language.

## Description

I designed the wordmartix myself and used a laser cutter to cut the letters out of a 2mm sheet metal. The letters are roughly 2cm high. The matrix is 10 rows high, each containing 11 letters + an additional row of 4 pixels for the specific minutes.

The first version of the wordclock is controlled by an Arduino Nano Every. The time is provided by a GPS module (NEO-6M) and a RTC module (DS3231). The GPS module is used to set the time and the RTC module is used to keep the time when the GPS module is not available.

The second version of the wordclock is controlled by an ESP32. The ESP32 spans a WiFi network and provides a web interface to set the color and the language. The time is provided by the wifi clients webbrowser and is synced to the RTC module (DS3231).

The wordclock is powered by a 5V 1A power supply. The power supply is connected to the Arduino/ESP32 and the LED strip. The LED strip is connected to the Arduino Nano Every via pin 6.

The backplate of the clock is made of 10mm plywood. Groves are milled into the plywood to fit the LED strip and the other electronical components. The frontplate is mounted to the backplate with 4 screws. Another layer of 10mm plywood is between the frontplate and the backplate. This layer is used to limit the light-spreading of each LED.

### Hardware

- Ardunio Nano Every or ESP32 (Xiao ESP32S3)
- WS2812B LED Strip (114 LEDs)
- 5V 1A Power Supply
- GPS Module (NEO-6M) (not needed for the ESP32 version)
- RTC Module (DS3231)
- Micro USB or USB-C cable

### other Materials

- 2mm sheet metal for Frontplate
- 10mm plywood for Backplate
- 10mm plywood for inner frame
- 4 screws

TODO:

- Move languages to specific files
- Add pictures
- Add wiring diagram
- Add a 3d model of the all plates
