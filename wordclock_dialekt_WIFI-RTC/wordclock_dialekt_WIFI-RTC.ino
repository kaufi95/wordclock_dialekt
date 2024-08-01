#include <RTClib.h>
#include <TimeLib.h>
#include <Timezone.h>

#include <LittleFS.h>
#include <ArduinoJson.h>

#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <Adafruit_NeoMatrix.h>

// define WiFi settings
#define SSID "WordClock"
#define DNSName "wordclock"

// define ports
#define HTTP_PORT 80

// define pins
#define NEOPIXEL_PIN 4  // define pin for Neopixels

// define parameters
#define WIDTH 11   // width of LED matirx
#define HEIGHT 11  // height of LED matrix + additional row for minute leds

#define SETTINGS_FILE "/settings.json"

#define FORMAT_LITTLEFS_IF_FAILED true

struct Config {
  uint16_t color;      // color
  uint8_t brightness;  // brightness
  String language;     // language
};

// create config object and set default values
Config config = { 65535, 128, "dialekt" };

uint8_t lastMin;

// create RTC object
RTC_DS3231 rtc;

// create webserver
AsyncWebServer server(HTTP_PORT);

// create Adafruit_NeoMatrix object
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(WIDTH, HEIGHT, NEOPIXEL_PIN,
                                               NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
                                               NEO_GRB + NEO_KHZ800);

// define time change rules and timezone
TimeChangeRule atST = { "ST", Last, Sun, Mar, 2, 120 };  // UTC + 2 hours
TimeChangeRule atRT = { "RT", Last, Sun, Oct, 3, 60 };   // UTC + 1 hour
Timezone AT(atST, atRT);

void setup() {

  // enable serial output
  Serial.begin(115200);
  Serial.println("WordClock");
  Serial.println("version 3.0");
  Serial.println("by kaufi95");

  // initialize LittleFS
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("File system mount failed...");
  }
  Serial.println("File system mounted");

  delay(1000);

  Serial.println("default settings");
  printSettings();

  // load stored values
  Serial.println("reading settings from file");
  loadSettings(SETTINGS_FILE);

  printSettings();

  // setup WiFI
  while (!WiFi.softAP(SSID)) {
    Serial.println("WiFi AP not started yet...");
    delay(1000);
  }
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // start server
  server.begin();
  Serial.println("AsyncWebServer started");

  // set server handlers
  server.onNotFound(handleNotFound);
  server.on("/", HTTP_GET, handleConnect);
  server.on("/status", HTTP_GET, handleStatus);

  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (request->url() == "/update") {
      handleUpdate(request, data);
    }
  });

  // serve static files
  server.serveStatic("/index.html", LittleFS, "/index.html");
  server.serveStatic("/app.js", LittleFS, "/app.js");
  server.serveStatic("/styles.css", LittleFS, "/styles.css");

  Serial.println("Handlers set and files served");

  if (!MDNS.begin(DNSName)) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");

  MDNS.addService("http", "tcp", 80);

  // initializing rtc
  if (!rtc.begin()) {
    Serial.println("Error setting up RTC");
  }
  Serial.println("RTC initialized");

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(2024, 1, 1, 0, 0, 0));
  }

  // init LED matrix
  Serial.println("initiating matrix");
  matrix.begin();
  matrix.setBrightness(config.brightness);
  matrix.fillScreen(0);
  matrix.show();
}

void loop() {
  time_t time = generateTimeByRTC();
  displayTimeInfo(time, "RTC");

  time_t localTime = AT.toLocal(time);
  displayTimeInfo(localTime, "AT");

  refreshMatrix(false);

  delay(15000);
}

void printSettings() {
  Serial.println("Color:\t" + String(config.color));
  Serial.println("Bright:\t" + String(config.brightness));
  Serial.println("Lang:\t" + config.language);
}

// ------------------------------------------------------------
// webserver

void handleNotFound(AsyncWebServerRequest *request) {
  request->redirect("/index.html");
}

void handleConnect(AsyncWebServerRequest *request) {
  request->redirect("/index.html");
}

void handleStatus(AsyncWebServerRequest *request) {
  JsonDocument doc;

  doc["color"] = String(config.color);
  doc["brightness"] = String(config.brightness);
  doc["language"] = config.language;

  String response;
  if (!serializeJson(doc, response)) {
    Serial.println("Failed to create response!");
  }

  request->send(200, "application/json", response);
}

void handleUpdate(AsyncWebServerRequest *request, uint8_t *data) {

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, data);

  if (error) {
    Serial.println("Failed to deserialize json from update-request.");
    Serial.println(error.c_str());
    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  config.color = (uint16_t)String(doc["color"]).toInt();
  config.brightness = (uint8_t)String(doc["brightness"]).toInt();
  config.language = String(doc["language"]);
  time_t time = mapTime(String(doc["datetime"]).c_str());
  rtc.adjust(time);

  matrix.setBrightness(config.brightness);
  refreshMatrix(true);

  storeSettings(SETTINGS_FILE);
  printSettings();

  request->send(200, "text/plain", "ok");
}

time_t mapTime(const char *timestamp) {
  return (time_t)strtoul(timestamp, NULL, 10);
}

// ------------------------------------------------------------
// storage

void loadSettings(const char *filename) {
  File file = LittleFS.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open file");
    return;
  }

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println("Failed to read file");
    file.close();
    return;
  }

  config.color = (uint16_t)String(doc["color"]).toInt();
  config.brightness = (uint8_t)String(doc["brightness"]).toInt();
  config.language = String(doc["language"]);

  file.close();
}

void storeSettings(const char *filename) {
  LittleFS.remove(filename);

  File file = LittleFS.open(filename, "w");
  if (!file) {
    Serial.println("Failed to create file");
    return;
  }

  JsonDocument doc;

  doc["color"] = String(config.color);
  doc["brightness"] = String(config.brightness);
  doc["language"] = config.language;

  if (!serializeJson(doc, file)) {
    Serial.println("Failed to write to file");
  }

  // Close the file
  file.close();
}

// ------------------------------------------------------------
// wordclock logic

// generate time
time_t generateTimeByRTC() {
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
    matrix.drawPixel(i, row, config.color);
  }
}

// clears, generates and fills pixels
void refreshMatrix(bool settingsChanged) {
  time_t time = AT.toLocal(generateTimeByRTC());
  if (lastMin != minute(time) || settingsChanged) {
    matrix.fillScreen(0);
    timeToMatrix(time);
    matrix.show();
    lastMin = minute(time);
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
  bool randomized = random() % 2 == 0;
  bool showEsIst = randomized || minutes % 30 < 5;
  return showEsIst;
}

// converts time into matrix
void timeToMatrix(time_t time) {
  uint8_t hours = hour(time);
  uint8_t minutes = minute(time);

  // show "Es ist" or "Es isch" randomized
  if (showEsIst(minutes)) {
    // Es isch/ist
    if (config.language == "dialekt") {
      Serial.print("Es isch ");
      turnPixelsOn(1, 2, 0);
      turnPixelsOn(5, 8, 0);
    }
    if (config.language == "deutsch") {
      Serial.print("Es ist ");
      turnPixelsOn(1, 2, 0);
      turnPixelsOn(5, 7, 0);
    }
  }

  // show minutes
  if (minutes >= 5 && minutes < 10) {
    min_five();
    Serial.print(" ");
    after();
  } else if (minutes >= 10 && minutes < 15) {
    min_ten();
    Serial.print(" ");
    after();
  } else if (minutes >= 15 && minutes < 20) {
    quarter();
    Serial.print(" ");
    after();
  } else if (minutes >= 20 && minutes < 25) {
    twenty();
    Serial.print(" ");
    after();
  } else if (minutes >= 25 && minutes < 30) {
    min_five();
    Serial.print(" ");
    to();
    Serial.print(" ");
    half();
  } else if (minutes >= 30 && minutes < 35) {
    half();
  } else if (minutes >= 35 && minutes < 40) {
    min_five();
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
    min_ten();
    Serial.print(" ");
    to();
  } else if (minutes >= 55 && minutes < 60) {
    min_five();
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
      hour_twelve();
      break;
    case 1:
      // Oans
      if (minutes >= 0 && minutes < 5) {
        hour_one(false);
      } else {
        hour_one(true);
      }
      break;
    case 2:
      // Zwoa
      hour_two();
      break;
    case 3:
      // Drei
      hour_three();
      break;
    case 4:
      // Viere
      hour_four();
      break;
    case 5:
      // Fünfe
      hour_five();
      break;
    case 6:
      // Sechse
      hour_six();
      break;
    case 7:
      // Siebne
      hour_seven();
      break;
    case 8:
      // Achte
      hour_eight();
      break;
    case 9:
      // Nüne
      hour_nine();
      break;
    case 10:
      // Zehne
      hour_ten();
      break;
    case 11:
      // Elfe
      hour_eleven();
      break;
  }

  if (minutes >= 0 && minutes < 5) {
    uhr();
  }

  // pixels for minutes in additional row
  turnPixelsOn(0, (minutes % 5) - 1, 10);
  // for (uint8_t i = 1; i <= minutes % 5; i++) {
  //   matrix.drawPixel(i - 1, 10, config.color);
  // }

  Serial.print(" + ");
  Serial.print(minutes % 5);
  Serial.print(" min");
  Serial.println();
}

// ------------------------------------------------------------
// numbers as labels

void hour_one(bool s) {
  // oans/eins
  if (config.language == "dialekt") {
    Serial.print("oans");
    turnPixelsOn(7, 10, 4);
  }
  if (config.language == "deutsch") {
    if (s) {
      Serial.print("eins");
      turnPixelsOn(7, 10, 4);
    } else {
      Serial.print("ein");
      turnPixelsOn(7, 9, 4);
    }
  }
}

void hour_two() {
  // zwoa/zwei
  if (config.language == "dialekt") {
    Serial.print("zwoa");
    turnPixelsOn(5, 8, 4);
  }
  if (config.language == "deutsch") {
    Serial.print("zwei");
    turnPixelsOn(5, 8, 4);
  }
}

void hour_three() {
  // drei
  Serial.print("drei");
  if (config.language == "dialekt") {
    turnPixelsOn(0, 3, 5);
  }
  if (config.language == "deutsch") {
    turnPixelsOn(0, 3, 5);
  }
}

void hour_four() {
  // vier/e/vier
  if (config.language == "dialekt") {
    Serial.print("viere");
    turnPixelsOn(0, 4, 8);
  }
  if (config.language == "deutsch") {
    Serial.print("vier");
    turnPixelsOn(0, 3, 8);
  }
}

void hour_five() {
  // fünfe/fünf
  if (config.language == "dialekt") {
    Serial.print("fünfe");
    turnPixelsOn(0, 4, 7);
  }
  if (config.language == "deutsch") {
    Serial.print("fünf");
    turnPixelsOn(0, 3, 7);
  }
}

void min_five() {
  // fünf/fünf
  Serial.print("fünf");
  if (config.language == "dialekt") {
    turnPixelsOn(0, 3, 1);
  }
  if (config.language == "deutsch") {
    turnPixelsOn(0, 3, 1);
  }
}

void hour_six() {
  // sechse/sechs
  if (config.language == "dialekt") {
    Serial.print("sechse");
    turnPixelsOn(5, 10, 5);
  }
  if (config.language == "deutsch") {
    Serial.print("sechs");
    turnPixelsOn(5, 9, 5);
  }
}

void hour_seven() {
  // siebne/sieben
  if (config.language == "dialekt") {
    Serial.print("siebne");
    turnPixelsOn(0, 5, 6);
  }
  if (config.language == "deutsch") {
    Serial.print("sieben");
    turnPixelsOn(0, 5, 6);
  }
}

void hour_eight() {
  // achte/acht
  if (config.language == "dialekt") {
    Serial.print("achte");
    turnPixelsOn(6, 10, 7);
  }
  if (config.language == "deutsch") {
    Serial.print("acht");
    turnPixelsOn(6, 9, 7);
  }
}

void hour_nine() {
  // nüne/neun
  if (config.language == "dialekt") {
    Serial.print("nüne");
    turnPixelsOn(7, 10, 6);
  }
  if (config.language == "deutsch") {
    Serial.print("neun");
    turnPixelsOn(7, 10, 6);
  }
}

void hour_ten() {
  // zehne/zehn
  if (config.language == "dialekt") {
    Serial.print("zehne");
    turnPixelsOn(6, 10, 8);
  }
  if (config.language == "deutsch") {
    Serial.print("zehn");
    turnPixelsOn(3, 6, 9);
  }
}

void min_ten() {
  // zehn/zehn
  Serial.print("zehn");
  if (config.language == "dialekt") {
    turnPixelsOn(7, 10, 2);
  }
  if (config.language == "deutsch") {
    turnPixelsOn(7, 10, 2);
  }
}

void hour_eleven() {
  // elfe/elf
  if (config.language == "dialekt") {
    Serial.print("elfe");
    turnPixelsOn(0, 3, 9);
  }
  if (config.language == "deutsch") {
    Serial.print("elf");
    turnPixelsOn(0, 2, 9);
  }
}

void hour_twelve() {
  // zwölfe/zwölf
  if (config.language == "dialekt") {
    Serial.print("zwölfe");
    turnPixelsOn(5, 10, 9);
  }
  if (config.language == "deutsch") {
    Serial.print("zwölf");
    turnPixelsOn(5, 9, 8);
  }
}

void quarter() {
  // viertel
  Serial.print("viertel");
  if (config.language == "dialekt") {
    turnPixelsOn(0, 6, 2);
  }
  if (config.language == "deutsch") {
    turnPixelsOn(0, 6, 2);
  }
}

void twenty() {
  // zwanzig
  Serial.print("zwanzig");
  if (config.language == "dialekt") {
    turnPixelsOn(4, 10, 1);
  }
  if (config.language == "deutsch") {
    turnPixelsOn(4, 10, 1);
  }
}

// ------------------------------------------------------------

void to() {
  // vor/vor
  Serial.print("vor");
  if (config.language == "dialekt") {
    turnPixelsOn(1, 3, 3);
  }
  if (config.language == "deutsch") {
    turnPixelsOn(1, 3, 3);
  }
}

void after() {
  // noch/nach
  if (config.language == "dialekt") {
    Serial.print("noch");
    turnPixelsOn(5, 8, 3);
  }
  if (config.language == "deutsch") {
    Serial.print("nach");
    turnPixelsOn(5, 8, 3);
  }
}

void half() {
  // halb
  Serial.print("halb");
  if (config.language == "dialekt") {
    turnPixelsOn(0, 3, 4);
  }
  if (config.language == "deutsch") {
    turnPixelsOn(0, 3, 4);
  }
}

void uhr() {
  // uhr
  if (config.language == "deutsch") {
    Serial.print(" uhr");
    turnPixelsOn(8, 10, 9);
  }
}
