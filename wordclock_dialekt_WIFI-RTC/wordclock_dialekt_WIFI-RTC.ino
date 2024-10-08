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

#define VERSION "3.2"

// define WiFi params
#define AP_SSID "WordClock"
#define AP_TIMEOUT 600000  // milliseconds
#define DNSName "wordclock"
#define HTTP_PORT 80

// define matrix params
#define WIDTH 11        // width of LED matirx
#define HEIGHT 11       // height of LED matrix + additional row for minute leds
#define NEOPIXEL_PIN 4  // define pin for Neopixels

// fs
#define SETTINGS_FILE "/settings.json"
#define FORMAT_LITTLEFS_IF_FAILED true

struct Config {
  uint16_t color;      // color
  uint8_t brightness;  // brightness
  String language;     // language
  bool termination;    // terminate WiFi after 10 min
};

// create config object and set default values
Config config = { 65535, 128, "dialekt", true };

uint8_t lastMin;
bool APRunning;
bool update = false;

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
  Serial.println("v" + VERSION);
  Serial.println("by kaufi95");

  startFS();

  startWiFi();
  startServer();
  startMDNS();

  startRTC();
  startMatrix();
}

void loop() {
  updateSettings();
  displayTime();
  refreshMatrix(false);
  updateWiFi();
  delay(15000);
}

void updateSettings() {
  if (!update) return;
  matrix.setBrightness(config.brightness);
  refreshMatrix(true);
  storeSettings(SETTINGS_FILE);
  update = false;
}

void displayTime() {
  time_t timeRTC = generateTimeByRTC();
  displayTimeInfo(timeRTC, "RTC");

  time_t timeAT = AT.toLocal(timeRTC);
  displayTimeInfo(timeAT, "AT");
}

void printSettings() {
  Serial.println("Color:\t\t" + String(config.color));
  Serial.println("Brightness:\t" + String(config.brightness));
  Serial.println("Language:\t" + config.language);
  Serial.println("Termination:\t" + String(config.termination));
}

// ------------------------------------------------------------
// matrix

void startMatrix() {
  // init LED matrix
  Serial.println("initiating matrix");
  matrix.begin();
  matrix.setBrightness(config.brightness);
  matrix.fillScreen(0);
  matrix.show();
}

// ------------------------------------------------------------
// rtc

void startRTC() {
  // initializing rtc
  if (!rtc.begin()) {
    Serial.println("Error setting up RTC");
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(2024, 1, 1, 0, 0, 0));
  }

  Serial.println("RTC initialized");
}

// ------------------------------------------------------------
// wifi

void updateWiFi() {
  if (!APRunning) return;
  if (!config.termination) return;
  if (millis() < AP_TIMEOUT) return;
  terminateWiFi();
}

void startWiFi() {
  // setup WiFI
  while (!WiFi.softAP(AP_SSID)) {
    Serial.println("WiFi AP not started yet...");
    delay(1000);
  }
  APRunning = true;
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void terminateWiFi() {
  if (!APRunning) {
    Serial.println("WiFi already terminated...");
    return;
  }
  if (WiFi.softAPgetStationNum() == 0) {
    Serial.println("Terminating WiFi.");
    terminateServer();
    terminateMDNS();
    WiFi.mode(WIFI_OFF);
    APRunning = false;
  } else {
    Serial.println("WiFi termination failed; Client(s) still connected");
  }
}

// ------------------------------------------------------------
// mdns

void startMDNS() {
  while (!MDNS.begin(DNSName)) {
    Serial.println("mDNS responder not started yet...");
    delay(1000);
  }
  MDNS.addService("http", "tcp", 80);
  Serial.println("mDNS responder started");
}

void terminateMDNS() {
  Serial.println("Terminating mDNS.");
  MDNS.end();
}

// ------------------------------------------------------------
// webserver

void startServer() {
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
}

void terminateServer() {
  Serial.println("Terminating WebServer");
  server.end();
}

void handleNotFound(AsyncWebServerRequest *request) {
  request->redirect("/index.html");
}

void handleConnect(AsyncWebServerRequest *request) {
  request->redirect("/index.html");
}

void handleStatus(AsyncWebServerRequest *request) {
  JsonDocument doc;

  doc["color"] = config.color;
  doc["brightness"] = config.brightness;
  doc["language"] = config.language;
  doc["termination"] = config.termination;

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

  rtc.adjust((time_t)doc["datetime"]);
  config.color = (uint16_t)doc["color"];
  config.brightness = (uint8_t)doc["brightness"];
  config.language = String(doc["language"]);
  config.termination = (bool)doc["termination"];

  update = true;

  request->send(200, "text/plain", "ok");
}

// ------------------------------------------------------------
// storage

void startFS() {
  // initialize LittleFS
  while (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("File system mount failed...");
    delay(1000);
  }
  Serial.println("File system mounted");

  // load stored values
  Serial.println("reading settings from file");
  loadSettings(SETTINGS_FILE);
}

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

  config.color = (uint16_t)doc["color"];
  config.brightness = (uint8_t)doc["brightness"];
  config.language = String(doc["language"]);
  config.termination = (bool)doc["termination"];

  file.close();

  printSettings();
}

void storeSettings(const char *filename) {
  LittleFS.remove(filename);

  File file = LittleFS.open(filename, "w");
  if (!file) {
    Serial.println("Failed to create file");
    return;
  }

  JsonDocument doc;

  doc["color"] = config.color;
  doc["brightness"] = config.brightness;
  doc["language"] = config.language;
  doc["termination"] = config.termination;

  if (!serializeJson(doc, file)) {
    Serial.println("Failed to write to file");
  }

  // Close the file
  file.close();

  printSettings();
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
    getPixels(time);
    matrix.show();
    lastMin = minute(time);
  }
}

// converts time into pixels
void getPixels(time_t time) {
  std::array<std::array<bool, 11>, 11> pixels = { false };
  if (config.language == "dialekt") {
    dialekt::timeToPixels(time, pixels);
  }
  if (config.language == "deutsch") {
    deutsch::timeToPixels(time, pixels);
  }
  drawPixelsOnMatrix(pixels);
}

// turns the pixels from startIndex to endIndex of startIndex row on
void drawPixelsOnMatrix(std::array<std::array<bool, 11>, 11> &pixels) {
  for (uint8_t i = 0; i < 11; i++) {
    for (uint8_t j = 0; j < 11; j++) {
      if (pixels[i][j]) {
        matrix.drawPixel(j, i, config.color);
      }
    }
  }
  // printPixelsToSerial(pixels);
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
