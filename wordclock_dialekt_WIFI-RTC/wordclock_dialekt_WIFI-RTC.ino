#include <EEPROM.h>
#include <RTClib.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <Adafruit_NeoMatrix.h>

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// define constantcs for eeprom
#define EEPROM_SIZE 3
#define EEC 0
#define EEL 1
#define EEB 2

// define WiFi settings
#define SSID "WordClock"

// define ports
#define HTTP_PORT 80

// define pins
#define NEOPIXEL_PIN 4  // define pin for Neopixels

// define parameters
#define WIDTH 11   // width of LED matirx
#define HEIGHT 11  // height of LED matrix + additional row for minute leds

// define global variables
byte color = 0;               // color
byte language = 0;            // language (0: dialekt; 1: deutsch)
byte brightness = 128;        // brightness
byte lastMin = 0;             // last minute

// create Adafruit_NeoMatrix object
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(WIDTH, HEIGHT, NEOPIXEL_PIN,
                                               NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
                                               NEO_GRB + NEO_KHZ800);

// define color modes
const uint16_t colors[] = {
  matrix.Color(255, 255, 255),  // white
  matrix.Color(255, 0, 0),      // red
  matrix.Color(0, 255, 0),      // green
  matrix.Color(0, 0, 255),      // blue
  matrix.Color(0, 255, 255),    // cyan
  matrix.Color(255, 0, 200),    // magenta
  matrix.Color(255, 255, 0),    // yellow
};

// create RTC object
RTC_DS3231 rtc;

// create webserver
WebServer server(HTTP_PORT);

// define time change rules and timezone
TimeChangeRule atST = { "ST", Last, Sun, Mar, 2, 120 };  // UTC + 2 hours
TimeChangeRule atRT = { "RT", Last, Sun, Oct, 3, 60 };   // UTC + 1 hour
Timezone AT(atST, atRT);

void setup() {

  // enable serial output
  Serial.begin(9600);
  Serial.println("WordClock");
  Serial.println("version 3.0");
  Serial.println("by kaufi95");

  // initialize access point and webserver
  Serial.println("Configuring access point...");
  if (!WiFi.softAP(SSID)) {
    Serial.println("Soft AP creation failed!");
    while (1)
      delay(10);
  }
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // start server
  server.begin();
  Serial.println("HTTP server started");

  // ser server handlers
  server.onNotFound(handleNotFound);
  server.on("/", HTTP_GET, handleConnect);
  server.on("/update", HTTP_POST, handleUpdate);

  // serve static files
  server.serveStatic("/index.html", SPIFFS, "/index.html");
  server.serveStatic("/app.js", SPIFFS, "/app.js");

  // initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    while (1)
      delay(10);
  }
  Serial.println("File system mounted");

  // initializing rtc
  if (!rtc.begin()) {
    Serial.println("could not find rtc");
    Serial.flush();
    while (1)
      delay(10);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(2024, 1, 1, 0, 0, 0));
  }

  // initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // read stored values from eeprom
  Serial.println("reading eeprom @ setup");
  color = EEPROM.read(EEC);         // stored color
  language = EEPROM.read(EEL);      // stored language
  brightness = EEPROM.read(EEB);    // stored brightness

  // init LED matrix
  Serial.println("initiating matrix");
  matrix.begin();
  matrix.setBrightness(brightness);
  matrix.fillScreen(0);
  matrix.show();

  printEEPROM();
}

void loop() {
  time_t time = generateTimeByRTC();
  displayTimeInfo(time, "RTC");

  time_t localTime = AT.toLocal(time);
  displayTimeInfo(localTime, "AT");
  refreshMatrix(false);

  server.handleClient();
  delay(1000);
}

// ------------------------------------------------------------
// webserver handlers

void handleNotFound() {
  // server.send(404, "text/plain", "File Not Found");
  server.sendHeader("Location", "/index.html", true);
  server.send(302, "text/plain", "");
}

void handleConnect() {
  server.sendHeader("Location", "/index.html", true);
  server.send(302, "text/plain", "");
}

void handleUpdate() {
  String body = server.arg("plain");

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }

  time_t t = mapTime(doc["datetime"]);
  Serial.print("datetime: ");
  Serial.println(t);
  rtc.adjust(t);

  byte c = mapper(doc["color"]);
  Serial.print("color: ");
  Serial.println(c);
  EEPROM.write(EEC, c);
  color = c;

  byte l = mapper(doc["language"]);
  Serial.print("language: ");
  Serial.println(l);
  EEPROM.write(EEL, l);
  language = l;

  byte b = mapper(doc["brightness"]);
  Serial.print("brightness: ");
  Serial.println(b);
  EEPROM.write(EEB, b);
  brightness = b;
  matrix.setBrightness(brightness);

  EEPROM.commit();

  refreshMatrix(true);
  printEEPROM();

  server.send(200, "text/plain", "");
}

static byte mapper(const char* input) {
  String key = String(input);
  if (key == "dialekt") return 0;
  if (key == "deutsch") return 1;
  if (key == "white") return 0;
  if (key == "red") return 1;
  if (key == "green") return 2;
  if (key == "blue") return 3;
  if (key == "cyan") return 4;
  if (key == "magenta") return 5;
  if (key == "yellow") return 6;
  if (key == "1") return 64;
  if (key == "2") return 96;
  if (key == "3") return 128;
  if (key == "4") return 160;
  return 0;
}

static time_t mapTime(const char* timestamp) {
  unsigned long time = strtoul(timestamp, NULL, 10);
  return (time_t)time;
}

// ------------------------------------------------------------
// wordclock logic

// generate time
static time_t generateTimeByRTC() {
  DateTime dt = rtc.now();
  tmElements_t tm;
  tm.Second = dt.second();
  tm.Minute = dt.minute();
  tm.Hour = dt.hour();
  tm.Day = dt.day();
  tm.Month = dt.month();
  tm.Year = dt.year() - 1970;
  time_t time = makeTime(tm);
  return time;
}

// turns the pixels from startIndex to endIndex of startIndex row on
void turnPixelsOn(uint16_t startIndex, uint16_t endIndex, uint16_t row) {
  for (int i = startIndex; i <= endIndex; i++) {
    matrix.drawPixel(i, row, colors[color]);
  }
}

// clears matrix, generates matrix and fills matrix
void refreshMatrix(bool settingsChanged) {
  time_t time = AT.toLocal(generateTimeByRTC());
  if (lastMin != minute(time) || settingsChanged) {
    Serial.println("refreshing matrix");
    matrix.fillScreen(0);
    timeToMatrix(time);
    matrix.show();
    lastMin = minute(time);
  }
}

void printEEPROM() {
  Serial.println("reading eeprom");
  for (int i = 0; i < 3; i++) {
    Serial.print("Byte ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(EEPROM.read(i));
  }
}

// ------------------------------------------------------------
// serial output

// display details of gps signal
void displayTimeInfo(time_t t, String component) {

  Serial.print(component + "\t: ");

  // print date
  if (day(t) < 10)
    Serial.print("0");
  Serial.print(day(t));
  Serial.print(F("/"));
  if (month(t) < 10)
    Serial.print("0");
  Serial.print(month(t));
  Serial.print("/");
  Serial.print(year(t));

  Serial.print(" / ");

  // print time
  if (hour(t) < 10)
    Serial.print("0");
  Serial.print(hour(t));
  Serial.print(":");
  if (minute(t) < 10)
    Serial.print("0");
  Serial.print(minute(t));
  Serial.print(":");
  if (second(t) < 10)
    Serial.print("0");
  Serial.print(second(t));
  Serial.println();
}

// ------------------------------------------------------------

// determine if "es isch/es ist" is shown
bool showEsIst(uint8_t minutes) {
  // show "Es ist" or "Es isch" randomized
  bool randomized = random() % 2 == 0;
  Serial.println("randomized 'es isch': " + String(randomized));

  bool showEsIst = randomized || minutes % 30 < 5;
  Serial.println("show 'es isch': " + String(showEsIst));

  return showEsIst;
}

// converts time into matrix
void timeToMatrix(time_t time) {
  uint8_t hours = hour(time);
  uint8_t minutes = minute(time);

  Serial.println("timeToMatrix");

  // show "Es ist" or "Es isch" randomized
  if (showEsIst(minutes)) {
    // Es isch/ist
    switch (language) {
      case 0:
        Serial.print("Es isch");
        turnPixelsOn(1, 2, 0);
        turnPixelsOn(5, 8, 0);
        break;
      case 1:
        Serial.print("Es ist");
        turnPixelsOn(1, 2, 0);
        turnPixelsOn(5, 7, 0);
        break;
    }
    Serial.print(" ");
  }

  // show minutes
  if (minutes >= 0 && minutes < 5) {
    uhr();
  } else if (minutes >= 5 && minutes < 10) {
    five(true, false);
    Serial.print(" ");
    after();
  } else if (minutes >= 10 && minutes < 15) {
    ten(true, false);
    Serial.print(" ");
    after();
  } else if (minutes >= 15 && minutes < 20) {
    ;
    quarter();
    Serial.print(" ");
    after();
  } else if (minutes >= 20 && minutes < 25) {
    twenty();
    Serial.print(" ");
    after();
  } else if (minutes >= 25 && minutes < 30) {
    five(true, false);
    Serial.print(" ");
    to();
    Serial.print(" ");
    half();
  } else if (minutes >= 30 && minutes < 35) {
    half();
  } else if (minutes >= 35 && minutes < 40) {
    five(true, false);
    Serial.print(" ");
    after();
    Serial.print(" ");
    half();
  } else if (minutes >= 40 && minutes < 45) {
    twenty();
    Serial.print(" ");
    to();
  } else if (minutes >= 45 && minutes < 50) {
    quarter();
    Serial.print(" ");
    to();
  } else if (minutes >= 50 && minutes < 55) {
    ten(true, false);
    Serial.print(" ");
    to();
  } else if (minutes >= 55 && minutes < 60) {
    five(true, false);
    Serial.print(" ");
    to();
  }

  Serial.print(" ");

  // convert hours to 12h format
  if (hours >= 12) {
    hours -= 12;
  }

  if (minutes >= 25) {
    hours++;
  }

  if (hours == 12) {
    hours = 0;
  }

  // show hours
  switch (hours) {
    case 0:
      // Zwölfe
      twelve(true);
      break;
    case 1:
      // Oans
      if (minutes >= 0 && minutes < 5) {
        one(false);
      } else {
        one(true);
      }
      break;
    case 2:
      // Zwoa
      two();
      break;
    case 3:
      // Drei
      three();
      break;
    case 4:
      // Viere
      four(true);
      break;
    case 5:
      // Fünfe
      five(false, true);
      break;
    case 6:
      // Sechse
      six(true);
      break;
    case 7:
      // Siebne
      seven(true, true);
      break;
    case 8:
      // Achte
      eight(true);
      break;
    case 9:
      // Nüne
      nine(true);
      break;
    case 10:
      // Zehne
      ten(false, true);
      break;
    case 11:
      // Elfe
      eleven(true);
      break;
  }

  // pixels for minutes in additional row
  for (byte i = 1; i <= minutes % 5; i++) {
    matrix.drawPixel(i - 1, 10, colors[color]);
  }

  Serial.print(" + ");
  Serial.print(minutes % 5);
  Serial.print(" min");
  Serial.println();
}

// ------------------------------------------------------------
// numbers as labels

void one(bool s) {
  // oans/eins
  switch (language) {
    case 0:
      Serial.print("oans");
      turnPixelsOn(7, 10, 4);
      break;
    case 1:
      if (s) {
        Serial.print("eins");
        turnPixelsOn(7, 10, 4);
      } else {
        Serial.print("ein");
        turnPixelsOn(7, 9, 4);
      }
      break;
  }
}

void two() {
  // zwoa/zwei
  switch (language) {
    case 0:
      Serial.print("zwoa");
      turnPixelsOn(5, 8, 4);
      break;
    case 1:
      Serial.print("zwei");
      turnPixelsOn(5, 8, 4);
      break;
  }
}

void three() {
  // drei
  switch (language) {
    case 0:
      Serial.print("drei");
      turnPixelsOn(0, 3, 5);
      break;
    case 1:
      Serial.print("drei");
      turnPixelsOn(0, 3, 5);
      break;
  }
}

void four(bool e) {
  // vier/e/vier
  switch (language) {
    case 0:
      Serial.print("vier");
      turnPixelsOn(0, 3, 8);
      if (e) {
        Serial.print("e");
        turnPixelsOn(4, 4, 8);
      }
      break;
    case 1:
      Serial.print("vier");
      turnPixelsOn(0, 3, 8);
      break;
  }
}

void five(bool min, bool e) {
  // fünf/e/fünf
  if (min) {
    switch (language) {
      case 0:
        Serial.print("fünf");
        turnPixelsOn(0, 3, 1);
        break;
      case 1:
        Serial.print("fünf");
        turnPixelsOn(0, 3, 1);
        break;
    }
  } else {
    switch (language) {
      case 0:
        Serial.print("fünf");
        turnPixelsOn(0, 3, 7);
        if (e) {
          Serial.print("e");
          turnPixelsOn(4, 4, 7);
        }
        break;
      case 1:
        Serial.print("fünf");
        turnPixelsOn(0, 3, 7);
        break;
    }
  }
}

void six(bool e) {
  // sechs/e/sechs
  switch (language) {
    case 0:
      Serial.print("sechs");
      turnPixelsOn(5, 9, 5);
      if (e) {
        Serial.print("e");
        turnPixelsOn(10, 10, 5);
      }
      break;
    case 1:
      Serial.print("sechs");
      turnPixelsOn(5, 9, 5);
      break;
  }
}

void seven(bool n, bool e) {
  // siebn/e/sieben
  switch (language) {
    case 0:
      Serial.print("sieb");
      turnPixelsOn(0, 3, 6);
      if (n) {
        Serial.print("n");
        turnPixelsOn(4, 4, 6);
      }
      if (e) {
        Serial.print("e");
        turnPixelsOn(5, 5, 6);
      }
      break;
    case 1:
      Serial.print("sieben");
      turnPixelsOn(0, 5, 6);
      break;
  }
}

void eight(bool e) {
  // acht/e/acht
  switch (language) {
    case 0:
      Serial.print("acht");
      turnPixelsOn(6, 9, 7);
      if (e) {
        Serial.print("e");
        turnPixelsOn(10, 10, 7);
      }
      break;
    case 1:
      Serial.print("acht");
      turnPixelsOn(6, 9, 7);
      break;
  }
}

void nine(bool e) {
  // nün/e/neun
  switch (language) {
    case 0:
      Serial.print("nün");
      turnPixelsOn(7, 9, 6);
      if (e) {
        Serial.print("e");
        turnPixelsOn(10, 10, 6);
      }
      break;
    case 1:
      Serial.print("neun");
      turnPixelsOn(7, 10, 6);
      break;
  }
}

void ten(bool min, bool e) {
  // zehn/e/zehn
  if (min) {
    switch (language) {
      case 0:
        Serial.print("zehn");
        turnPixelsOn(7, 10, 2);
        break;
      case 1:
        Serial.print("zehn");
        turnPixelsOn(7, 10, 2);
        break;
    }
  } else {
    switch (language) {
      case 0:
        Serial.print("zehn");
        turnPixelsOn(6, 9, 8);
        if (e) {
          Serial.print("e");
          turnPixelsOn(10, 10, 8);
        }
        break;
      case 1:
        Serial.print("zehn");
        turnPixelsOn(3, 6, 9);
        break;
    }
  }
}

void eleven(bool e) {
  // elf/e
  switch (language) {
    case 0:
      Serial.print("elf");
      turnPixelsOn(0, 2, 9);
      if (e) {
        Serial.print("e");
        turnPixelsOn(3, 3, 9);
      }
      break;
    case 1:
      Serial.print("elf");
      turnPixelsOn(0, 2, 9);
      break;
  }
}

void twelve(bool e) {
  // zwölf/e
  Serial.print("zwölf");
  switch (language) {
    case 0:
      turnPixelsOn(5, 9, 9);
      if (e) {
        Serial.print("e");
        turnPixelsOn(10, 10, 9);
      }
      break;
    case 1:
      turnPixelsOn(5, 9, 8);
      break;
  }
}

void quarter() {
  // viertel
  Serial.print("viertel");
  switch (language) {
    case 0:
      turnPixelsOn(0, 6, 2);
      break;
    case 1:
      turnPixelsOn(0, 6, 2);
      break;
  }
}

void twenty() {
  // zwanzig
  Serial.print("zwanzig");
  switch (language) {
    case 0:
      turnPixelsOn(4, 10, 1);
      break;
    case 1:
      turnPixelsOn(4, 10, 1);
      break;
  }
}

// ------------------------------------------------------------

void to() {
  // vor/vor
  turnPixelsOn(1, 3, 3);
  switch (language) {
    case 0:
      Serial.print("vor");
      turnPixelsOn(1, 3, 3);
      break;
    case 1:
      Serial.print("vor");
      turnPixelsOn(1, 3, 3);
      break;
  }
}

void after() {
  // noch/nach
  switch (language) {
    case 0:
      Serial.print("noch");
      turnPixelsOn(5, 8, 3);
      break;
    case 1:
      Serial.print("nach");
      turnPixelsOn(5, 8, 3);
      break;
  }
}

void half() {
  // halb
  switch (language) {
    case 0:
      Serial.print("halb");
      turnPixelsOn(0, 3, 4);
      break;
    case 1:
      Serial.print("halb");
      turnPixelsOn(0, 3, 4);
      break;
  }
}

void uhr() {
  // uhr
  switch (language) {
    case 0:
      break;
    case 1:
      Serial.print(" uhr");
      turnPixelsOn(8, 10, 9);
      break;
  }
}
