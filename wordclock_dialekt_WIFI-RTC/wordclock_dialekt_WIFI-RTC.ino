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

#include "src/dialekt.h"
#include "src/deutsch.h"

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
  displayTime();
  refreshMatrix(false);
  delay(15000);
}

void displayTime() {
  time_t timeRTC = generateTimeByRTC();
  displayTimeInfo(timeRTC, "RTC");

  time_t timeAT = AT.toLocal(timeRTC);
  displayTimeInfo(timeAT, "AT");
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

  time_t time = mapTime(String(doc["datetime"]).c_str());
  rtc.adjust(time);
  if (doc["color"]) config.color = (uint16_t)String(doc["color"]).toInt();
  if (doc["brightness"]) config.brightness = (uint8_t)String(doc["brightness"]).toInt();
  if (doc["language"]) config.language = String(doc["language"]);

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

// clears, generates and fills pixels
void refreshMatrix(bool settingsChanged) {
  time_t time = AT.toLocal(generateTimeByRTC());
  if (lastMin != minute(time) || settingsChanged) {
    matrix.fillScreen(0);
    fillMatrix(time);
    matrix.show();
    lastMin = minute(time);
  }
}

// converts time into matrix
void fillMatrix(time_t time) {
  if (config.language == "dialekt") {
    auto pixels = dialekt::timeToMatrix(time);
    turnPixelsOn(pixels);
    printMatrix(pixels);
  }
  if (config.language == "deutsch") {
    auto pixels = deutsch::timeToMatrix(time);
    turnPixelsOn(pixels);
    printMatrix(pixels);
  }
}

// turns the pixels from startIndex to endIndex of startIndex row on
void turnPixelsOn(std::array<std::array<bool, 11>, 11> &pixels) {
  for (uint8_t i = 0; i < 11; i++) {
    for (uint8_t j = 0; j < 11; j++) {
      if (pixels[i][j]) {
        matrix.drawPixel(j, i, config.color);
      }
    }
  }
}

void printMatrix(std::array<std::array<bool, 11>, 11> &pixels) {
  for (uint8_t i = 0; i < 11; i++) {
    for (uint8_t j = 0; j < 11; j++) {
      Serial.print(pixels[i][j] ? "1" : "0");
    }
    Serial.println();
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
