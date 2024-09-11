#include <RTClib.h>
#include <TimeLib.h>
#include <Timezone.h>

#include <LittleFS.h>
#include <ArduinoJson.h>

#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// #define ENABLE_MATRIX
#ifdef ENABLE_MATRIX
#include <Adafruit_NeoMatrix.h>

#include "src/dialekt.h"
#include "src/deutsch.h"
#endif

#include <NTPClient.h>

#define VERSION "3.2"

// define time params
#define TIME_SYNC_DIFF 5  // seconds

// define WiFi params
#define AP_SSID "WordClock"
#define AP_TIMEOUT 600000  // milliseconds
#define DNS_NAME "wordclock"
#define HTTP_PORT 80

#ifdef ENABLE_MATRIX
// define matrix params
#define MATRIX_WIDTH 11   // width of LED matirx
#define MATRIX_HEIGHT 11  // height of LED matrix + additional row for minute leds
#define MATRIX_PIN 4      // define pin for Neopixels
#define PRINT_MATRIX_TO_SERIAL
#endif

// define fs params
#define SETTINGS_FILE "/settings.json"
#define FORMAT_LITTLEFS_IF_FAILED true

struct Config {
  uint16_t color;      // color
  uint8_t brightness;  // brightness
  String language;     // language
  bool terminateAP;    // terminate WiFi?
  bool ntp;            // ntp
  String ssid;         // ssid
  String pw;           // password
};

String WiFiStates[] = { "NO SHIELD",
                        "STOPPED",
                        "IDLE STATUS",
                        "NO SSID AVAILABLE",
                        "SCAN COMPLETED",
                        "CONNECTED",
                        "CONNECT FAILED",
                        "CONNECTION LOST",
                        "DISCONNECTED" };

// create config object and set default values
Config config = { 65535, 128, "dialekt", true, false, "", "" };

uint8_t lastMin;
bool AP = false;
bool STA = false;
bool timeSynced = false;
bool reconnect = false;
bool update = false;

unsigned long nextTimeSync = 0;
const int wifiConnectOffset = 60000;  // 1 minute before the hour to start Wi-Fi connection

// create RTC object
RTC_DS3231 rtc;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// create webserver
AsyncWebServer server(HTTP_PORT);

// create eventsource
AsyncEventSource events("/events");

#ifdef ENABLE_MATRIX
// create Adafruit_NeoMatrix object
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_PIN,
                                               NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
                                               NEO_GRB + NEO_KHZ800);
#endif

// define time change rules and timezone
TimeChangeRule atST = { "ST", Last, Sun, Mar, 2, 120 };  // UTC + 2 hours
TimeChangeRule atRT = { "RT", Last, Sun, Oct, 3, 60 };   // UTC + 1 hour
Timezone AT(atST, atRT);

void setup() {

  // enable serial output
  Serial.begin(115200);
  Serial.println("WordClock");
  Serial.println("v" + String(VERSION));
  Serial.println("by kaufi95");

  startFS();

  startWiFi();
  startServer();
  startMDNS();

  startRTC();
#ifdef ENABLE_MATRIX
  startMatrix();
#endif
}

// ------------------------------------------------------------
// main

void loop() {
  updateSettings();
  updateWiFi();
  updateTime();

  displayTime();

#ifdef ENABLE_MATRIX
  refreshMatrix(false);
#endif

  delay(15000);
}

void updateSettings() {
  if (!update) return;

#ifdef ENABLE_MATRIX
  matrix.setBrightness(config.brightness);
  refreshMatrix(true);
#endif

  storeSettings(SETTINGS_FILE);
  update = false;
}

void displayTime() {
  time_t timeRTC = generateTimeByRTC();
  displayTimeInfo(timeRTC, "RTC");

  time_t timeAT = AT.toLocal(timeRTC);
  displayTimeInfo(timeAT, "AT");
}

void updateTime() {
  if (!config.ntp) {
    return;
  }

  if (millis() >= nextTimeSync) {
    timeSynced = false;
  }

  if (timeSynced) {
    return;
  }

  if (!STA) {
    runSTA();
    return;
  }

  timeClient.update();
  if (!timeClient.isTimeSet()) {
    Serial.println("TimeClient not ready yet...");
    return;
  }

  time_t time_ntp = timeClient.getEpochTime();
  time_t time_rtc = generateTimeByRTC();
  displayTimeInfo(time_ntp, "NTP");
  if (abs(time_ntp - time_rtc) > TIME_SYNC_DIFF) {
    rtc.adjust(time_ntp);
    Serial.println("Time synced over NTP.");
  } else {
    Serial.println("Time not synced over NTP, since difference is less than " + String(TIME_SYNC_DIFF) + "sec.");
  }
  timeSynced = true;
  scheduleNextTimeSync();
  timeClient.end();
  terminateSTA();
}

// Function to calculate the time for the next connection
void scheduleNextTimeSync() {
  DateTime now = rtc.now();
  int currentMinute = now.minute();
  int currentSecond = now.second();

  // Calculate time until the next hour, considering a minute before the hour
  int minutesUntilNextHour = (60 - currentMinute) - (currentSecond > 0 ? 1 : 0); // Adjust for seconds
  unsigned long millisecondsUntilNextHour = minutesUntilNextHour * 60000UL;

  nextTimeSync = millis() + millisecondsUntilNextHour - wifiConnectOffset;
}

void printSettings() {
  Serial.println("Color:\t\t" + String(config.color));
  Serial.println("Brightness:\t" + String(config.brightness));
  Serial.println("Language:\t" + config.language);
  Serial.println("TerminateAP:\t" + String(config.terminateAP));
  Serial.println("NTP:\t\t" + String(config.ntp));
  Serial.println("SSID:\t\t" + config.ssid);
  Serial.println("PW:\t\t" + config.pw);
}

// ------------------------------------------------------------
// wifi

void updateWiFi() {
  if (AP) {
    events.send(WiFiStates[WiFi.status()].c_str(), "status", millis());
    if (config.terminateAP) terminateAP();
  }
  if (reconnect) configureWiFi();
}

void configureWiFi() {
  if (WiFi.status() == WL_CONNECTED && !reconnect) {
    Serial.println("WiFi already connected");
    return;
  }

  if (config.ntp) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi network");
    } else {
      Serial.println("Failed to connect to WiFi network");
    }
  } else {
    terminateSTA();
  }
  reconnect = false;
}

void startWiFi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(DNS_NAME);
  WiFi.onEvent(STAConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  runAP();
}

void runAP() {
  Serial.println("WiFi AP starting...");
  while (!WiFi.softAP(AP_SSID)) {
    Serial.println("WiFi AP not started yet...");
    delay(1000);
  }
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  AP = true;
}

void terminateAP() {
  if (!AP) {
    Serial.println("AP already terminated...");
    return;
  }

  if (millis() < AP_TIMEOUT) {
    return;
  }

  if (WiFi.softAPgetStationNum() == 0) {
    Serial.println("Terminating AP.");
    terminateServer();
    terminateMDNS();
    AP = false;
  } else {
    Serial.println("AP termination failed; Client(s) still connected");
  }
}

void STAConnected(WiFiEvent_t event) {
  STA = true;
}

void runSTA() {
  Serial.println("Connecting to WiFi network");

  if (config.ssid == "" || config.pw == "") {
    Serial.println("No WiFi credentials stored.");
    return;
  }

  Serial.println("SSID: \t" + config.ssid);
  Serial.println("PW: \t" + config.pw);

  WiFi.begin(config.ssid.c_str(), config.pw.c_str());
}

void terminateSTA() {
  Serial.println("Terminating STA.");
  WiFi.disconnect();
  STA = false;
}

// ------------------------------------------------------------
// mdns

void startMDNS() {
  while (!MDNS.begin(DNS_NAME)) {
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

#ifdef ENABLE_MATRIX
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
#endif

// ------------------------------------------------------------
// rtc

void startRTC() {
  // initializing rtc
  while (!rtc.begin()) {
    Serial.println("RTC not started yet...");
    delay(1000);
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(2024, 1, 1, 0, 0, 0));
  }
  rtc.disable32K();

  Serial.println("RTC initialized");
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

  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient *client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
  });
  server.addHandler(&events);

  Serial.println("Handlers set and files served");

  timeClient.begin();
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
  doc["terminateAP"] = config.terminateAP;

  doc["ntp"] = config.ntp;
  doc["ssid"] = String(config.ssid);
  doc["status"] = WiFiStates[WiFi.status()];

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
  config.terminateAP = (bool)doc["terminateAP"];
  
  bool old_ntp = config.ntp;
  config.ntp = (bool)doc["ntp"];
  if (old_ntp != config.ntp) reconnect = true;

  update = true;

  String old_ssid = config.ssid;
  config.ssid = String(doc["ssid"]);
  config.pw = String(doc["pw"]);
  if (old_ssid != config.ssid) reconnect = true;

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
  config.terminateAP = (bool)doc["terminateAP"];
  config.ntp = (bool)doc["ntp"];
  config.ssid = String(doc["ssid"]);
  config.pw = String(doc["pw"]);

  delay(1000);

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
  doc["terminateAP"] = config.terminateAP;
  doc["ntp"] = config.ntp;
  doc["ssid"] = config.ssid;
  doc["pw"] = config.pw;

  if (!serializeJson(doc, file)) {
    Serial.println("Failed to write to file");
  }

  // Close the file
  file.close();

  delay(1000);

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

#ifdef ENABLE_MATRIX
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
#ifdef PRINT_MATRIX_TO_SERIAL
// printPixelsToSerial(pixels);
#endif
}
#endif
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
