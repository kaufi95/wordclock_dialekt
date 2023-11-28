#include <EEPROM.h>
#include <RTClib.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <ezButton.h>
#include <TinyGPS++.h>
#include <Adafruit_NeoMatrix.h>

// define setup
bool testMode = false;
const byte color = 0;
const byte language = 0;

// define pins
#define COLOR_BUTTON_PIN 3 // define pin for color switching
#define NEOPIXEL_PIN 6     // define pin for Neopixels

// define global variables
bool initialSync;
const int eeC = 0;                 // eeprom address for colorstate
const int eeL = 1;                 // eeprom address for lang
byte activeColorID;                // current active color mode
byte lang;                         // active language
unsigned long debounceDelay = 250; // the debounce time for button

// define parameters
const int width = 11;  // width of LED matirx
const int height = 11; // height of LED matrix + additional row for minute leds

// create Adafruit_NeoMatrix object
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(width, height, NEOPIXEL_PIN,
                                               NEO_MATRIX_TOP + NEO_MATRIX_LEFT +
                                                   NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
                                               NEO_GRB + NEO_KHZ800);

// define color modes
const uint16_t colors[] = {
    matrix.Color(255, 255, 255), // white
    matrix.Color(255, 0, 0),     // red
    matrix.Color(0, 255, 0),     // green
    matrix.Color(0, 0, 255),     // blue
    matrix.Color(0, 255, 255),   // cyan
    matrix.Color(255, 0, 200),   // magenta
    matrix.Color(255, 255, 0),   // yellow
};

// create GPS object
TinyGPSPlus gps;

// create RTC object
RTC_DS3231 rtc;

// create button object
ezButton button(COLOR_BUTTON_PIN); // create ezButton object that attach to led color button pin;

// define time change rules and timezone
TimeChangeRule atST = {"ST", Last, Sun, Mar, 2, 120}; // UTC + 2 hours
TimeChangeRule atRT = {"RT", Last, Sun, Oct, 3, 60};  // UTC + 1 hour
Timezone AT(atST, atRT);

void setup()
{

  // enable serial output
  Serial.begin(9600);
  Serial.println("WordClock");
  Serial.println("version 1.4");
  Serial.println("by kaufi");

  // initialize hardware serial at 9600 baud
  Serial.println("initiating hardwareserial for gps module");
  Serial1.begin(9600);

  // initializing rtc
  if (!rtc.begin())
  {
    Serial.println("could not find rtc");
    Serial.flush();
    while (1)
      delay(10);
  }

  if (rtc.lostPower())
  {
    // this will adjust to the date and time at compilation
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rtc.adjust(DateTime(2021, 1, 1, 0, 0, 0));
  }

  // init LED matrix
  Serial.println("initiating matrix");
  matrix.begin();
  matrix.setBrightness(100);
  matrix.fillScreen(0);
  matrix.show();

  // define color button as input
  Serial.println("changing pinMode");
  pinMode(COLOR_BUTTON_PIN, INPUT_PULLUP);
  button.setDebounceTime(debounceDelay);

  // sync on first valid gps signal
  initialSync = true;

  if (digitalRead(COLOR_BUTTON_PIN) == 0)
  {
    Serial.println("writing eeprom");

    // write color
    EEPROM.write(eeC, color);

    // write lang (0 dialekt; 1 deutsch)
    EEPROM.write(eeL, language);
  }

  // read color and lang
  Serial.println("reading eeprom @ setup");
  activeColorID = EEPROM.read(eeC); // current active color mode
  lang = EEPROM.read(eeL);          // switch languages (0: dialekt, 1: deutsch, 2: ...)

  printEEPROM();
}

void loop()
{
  // checkColorButton();
  refreshMatrix();
  smartDelay(1000);
  displayinfoRTC();
  displayinfoGPS();
  displayinfoSYS();
  syncTime();
}

void syncTime()
{

  rtc.adjust(AT.toLocal(generateTimeByRTC()));

  // get time from GPS module
  if (gps.time.isValid() && gps.date.isValid() && gps.date.year() >= 2021)
  {
    if (initialSync || rtc.now().hour() == 2 && gps.time.second() != rtc.now().second())
    {
      Serial.println("setting rtctime...");
      rtc.adjust(AT.toLocal(generateTimeByGPS()));
      if (initialSync)
      {
        Serial.println("initialSync");
        initialSync = false;
      }
    }
  }
  else
  {
    Serial.println("gps signal invalid! (time or date)");
  }
}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    checkColorButton();
    while (Serial1.available())
    {
      gps.encode(Serial1.read());
    }
  } while (millis() - start < ms);
}

// generate time
static time_t generateTimeByRTC()
{
  // returns time_t from rtc date and time
  tmElements_t tm;
  tm.Second = rtc.now().second();
  tm.Minute = rtc.now().minute();
  tm.Hour = rtc.now().hour();
  tm.Day = rtc.now().day();
  tm.Month = rtc.now().month();
  tm.Year = rtc.now().year() - 1970;
  time_t time = makeTime(tm);
  return time;
}

static time_t generateTimeByGPS()
{
  // returns time_t from gps date and time
  tmElements_t tm;
  tm.Second = gps.time.second();
  tm.Minute = gps.time.minute();
  tm.Hour = gps.time.hour();
  tm.Day = gps.date.day();
  tm.Month = gps.date.month();
  tm.Year = gps.date.year() - 1970;
  time_t time = makeTime(tm);
  return time;
}

// turns the pixels from a to b of a row on
void turnPixelsOn(uint16_t a, uint16_t b, uint16_t c)
{
  for (int i = a; i <= b; i++)
  {
    matrix.drawPixel(i, c, colors[activeColorID]);
  }
}

// clears matrix, generates matrix and fills matrix
void refreshMatrix()
{
  bool newYear = testMode || (gps.date.day() == 31 && gps.date.month() == 12 && gps.time.hour() == 23 && gps.time.minute() == 59 && gps.time.second() >= 30);

  matrix.fillScreen(0);

  if (newYear)
  {
    newYearSpecial(rtc.now().second());
    matrix.show();
    return;
  }

  timeToMatrix(rtc.now().hour(), rtc.now().minute());
  matrix.show();
}

// checks if colorbutton is pressed and write new value to eeprom
void checkColorButton()
{
  button.loop(); // MUST call the loop() function first
  if (button.isPressed())
  {
    Serial.println("changing color");
    activeColorID = (activeColorID + 1) % (sizeof(colors) / sizeof(colors[0]));
    Serial.println("writing color to eeprom");
    EEPROM.write(eeC, activeColorID);
    printEEPROM();
  }
}

void printEEPROM()
{
  Serial.println("reading eeprom");
  for (int i = 0; i < 2; i++)
  {
    Serial.print("Byte ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(EEPROM.read(i));
  }
}

// ----------------------------------------------------------------------------------------------------

// display details of rtc
void displayinfoRTC()
{

  Serial.print("RTC: ");

  // print date
  if (rtc.now().day() < 10)
    Serial.print("0");
  Serial.print(rtc.now().day());
  Serial.print(F("/"));
  if (rtc.now().month() < 10)
    Serial.print("0");
  Serial.print(rtc.now().month());
  Serial.print("/");
  Serial.print(rtc.now().year());

  Serial.print(" / ");

  // print time
  if (rtc.now().hour() < 10)
    Serial.print("0");
  Serial.print(rtc.now().hour());
  Serial.print(":");
  if (rtc.now().minute() < 10)
    Serial.print("0");
  Serial.print(rtc.now().minute());
  Serial.print(":");
  if (rtc.now().second() < 10)
    Serial.print("0");
  Serial.print(rtc.now().second());
  Serial.println();
}

// display details of gps signal
void displayinfoGPS()
{

  Serial.print("GPS: ");

  // print date
  if (gps.date.day() < 10)
    Serial.print("0");
  Serial.print(gps.date.day());
  Serial.print(F("/"));
  if (gps.date.month() < 10)
    Serial.print("0");
  Serial.print(gps.date.month());
  Serial.print("/");
  Serial.print(gps.date.year());

  Serial.print(" / ");

  // print time
  if (gps.time.hour() < 10)
    Serial.print("0");
  Serial.print(gps.time.hour());
  Serial.print(":");
  if (gps.time.minute() < 10)
    Serial.print("0");
  Serial.print(gps.time.minute());
  Serial.print(":");
  if (gps.time.second() < 10)
    Serial.print("0");
  Serial.print(gps.time.second());
  Serial.println();
}

// display details of gps signal
void displayinfoSYS()
{

  Serial.print("SYS: ");

  // print date
  if (day() < 10)
    Serial.print("0");
  Serial.print(day());
  Serial.print(F("/"));
  if (month() < 10)
    Serial.print("0");
  Serial.print(month());
  Serial.print("/");
  Serial.print(year());

  Serial.print(" / ");

  // print time
  if (hour() < 10)
    Serial.print("0");
  Serial.print(hour());
  Serial.print(":");
  if (minute() < 10)
    Serial.print("0");
  Serial.print(minute());
  Serial.print(":");
  if (second() < 10)
    Serial.print("0");
  Serial.print(second());
  Serial.println();
}

// ----------------------------------------------------------------------------------------------------

// converts time into matrix
void timeToMatrix(uint8_t hours, uint8_t minutes)
{

  Serial.println("timeToMatrix");
  // Es isch/ist
  switch (lang)
  {
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

  // show minutes
  if (minutes >= 0 && minutes < 5)
  {
    uhr();
  }
  else if (minutes >= 5 && minutes < 10)
  {
    five(true, false);
    Serial.print(" ");
    after();
  }
  else if (minutes >= 10 && minutes < 15)
  {
    ten(true, false);
    Serial.print(" ");
    after();
  }
  else if (minutes >= 15 && minutes < 20)
  {
    ;
    quarter();
    Serial.print(" ");
    after();
  }
  else if (minutes >= 20 && minutes < 25)
  {
    twenty();
    Serial.print(" ");
    after();
  }
  else if (minutes >= 25 && minutes < 30)
  {
    five(true, false);
    Serial.print(" ");
    to();
    Serial.print(" ");
    half();
  }
  else if (minutes >= 30 && minutes < 35)
  {
    half();
  }
  else if (minutes >= 35 && minutes < 40)
  {
    five(true, false);
    Serial.print(" ");
    after();
    Serial.print(" ");
    half();
  }
  else if (minutes >= 40 && minutes < 45)
  {
    twenty();
    Serial.print(" ");
    to();
  }
  else if (minutes >= 45 && minutes < 50)
  {
    quarter();
    Serial.print(" ");
    to();
  }
  else if (minutes >= 50 && minutes < 55)
  {
    ten(true, false);
    Serial.print(" ");
    to();
  }
  else if (minutes >= 55 && minutes < 60)
  {
    five(true, false);
    Serial.print(" ");
    to();
  }

  Serial.print(" ");

  // convert hours to 12h format
  if (hours >= 12)
  {
    hours -= 12;
  }

  if (minutes >= 25)
  {
    hours++;
  }

  if (hours == 12)
  {
    hours = 0;
  }

  // show hours
  switch (hours)
  {
  case 0:
    // Zwölfe
    twelve(true);
    break;
  case 1:
    // Oans
    if (minutes >= 0 && minutes < 5)
    {
      one(false);
    }
    else
    {
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
  for (byte i = 1; i <= minutes % 5; i++)
  {
    matrix.drawPixel(i - 1, 10, colors[activeColorID]);
  }

  Serial.print(" + ");
  Serial.print(minutes % 5);
  Serial.print(" min");
  Serial.println();
}

// ----------------------------------------------------------------------------------------------------

// numbers as labels

void one(bool s)
{
  // oans/eins
  switch (lang)
  {
  case 0:
    Serial.print("oans");
    turnPixelsOn(7, 10, 4);
    break;
  case 1:
    if (s)
    {
      Serial.print("eins");
      turnPixelsOn(7, 10, 4);
    }
    else
    {
      Serial.print("ein");
      turnPixelsOn(7, 9, 4);
    }
    break;
  }
}

void two()
{
  // zwoa/zwei
  switch (lang)
  {
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

void three()
{
  // drei
  switch (lang)
  {
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

void four(bool e)
{
  // vier/e/vier
  switch (lang)
  {
  case 0:
    Serial.print("vier");
    turnPixelsOn(0, 3, 8);
    if (e)
    {
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

void five(bool min, bool e)
{
  // fünf/e/fünf
  if (min)
  {
    switch (lang)
    {
    case 0:
      Serial.print("fünf");
      turnPixelsOn(0, 3, 1);
      break;
    case 1:
      Serial.print("fünf");
      turnPixelsOn(0, 3, 1);
      break;
    }
  }
  else
  {
    switch (lang)
    {
    case 0:
      Serial.print("fünf");
      turnPixelsOn(0, 3, 7);
      if (e)
      {
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

void six(bool e)
{
  // sechs/e/sechs
  switch (lang)
  {
  case 0:
    Serial.print("sechs");
    turnPixelsOn(5, 9, 5);
    if (e)
    {
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

void seven(bool n, bool e)
{
  // siebn/e/sieben
  switch (lang)
  {
  case 0:
    Serial.print("sieb");
    turnPixelsOn(0, 3, 6);
    if (n)
    {
      Serial.print("n");
      turnPixelsOn(4, 4, 6);
    }
    if (e)
    {
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

void eight(bool e)
{
  // acht/e/acht
  switch (lang)
  {
  case 0:
    Serial.print("acht");
    turnPixelsOn(6, 9, 7);
    if (e)
    {
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

void nine(bool e)
{
  // nün/e/neun
  switch (lang)
  {
  case 0:
    Serial.print("nün");
    turnPixelsOn(7, 9, 6);
    if (e)
    {
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

void ten(bool min, bool e)
{
  // zehn/e/zehn
  if (min)
  {
    switch (lang)
    {
    case 0:
      Serial.print("zehn");
      turnPixelsOn(7, 10, 2);
      break;
    case 1:
      Serial.print("zehn");
      turnPixelsOn(7, 10, 2);
      break;
    }
  }
  else
  {
    switch (lang)
    {
    case 0:
      Serial.print("zehn");
      turnPixelsOn(6, 9, 8);
      if (e)
      {
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

void eleven(bool e)
{
  // elf/e
  switch (lang)
  {
  case 0:
    Serial.print("elf");
    turnPixelsOn(0, 2, 9);
    if (e)
    {
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

void twelve(bool e)
{
  // zwölf/e
  Serial.print("zwölf");
  switch (lang)
  {
  case 0:
    turnPixelsOn(5, 9, 9);
    if (e)
    {
      Serial.print("e");
      turnPixelsOn(10, 10, 9);
    }
    break;
  case 1:
    turnPixelsOn(5, 9, 8);
    break;
  }
}

void quarter()
{
  // viertel
  Serial.print("viertel");
  switch (lang)
  {
  case 0:
    turnPixelsOn(0, 6, 2);
    break;
  case 1:
    turnPixelsOn(0, 6, 2);
    break;
  }
}

void twenty()
{
  // zwanzig
  Serial.print("zwanzig");
  switch (lang)
  {
  case 0:
    turnPixelsOn(4, 10, 1);
    break;
  case 1:
    turnPixelsOn(4, 10, 1);
    break;
  }
}

// ----------------------------------------------------------------------------------------------------

void to()
{
  // vor/vor
  turnPixelsOn(1, 3, 3);
  switch (lang)
  {
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

void after()
{
  // noch/nach
  switch (lang)
  {
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

void half()
{
  // halb
  switch (lang)
  {
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

void uhr()
{
  // uhr
  switch (lang)
  {
  case 0:
    break;
  case 1:
    Serial.print(" uhr");
    turnPixelsOn(8, 10, 9);
    break;
  }
}

// ----------------------------------------------------------------------------------------------------

// newYearSpecial
void newYearSpecial(uint8_t seconds)
{
  switch (seconds)
  {
  case 30:
    matrix.fillScreen(0);
    break;
  case 34:
    matrix.fillScreen(colors[activeColorID]);
    break;
  case 35:
    matrix.fillScreen(0);
    break;
  case 36:
    matrix.fillScreen(colors[activeColorID]);
    break;
  case 37:
    matrix.fillScreen(0);
    break;
  case 38:
    matrix.fillScreen(colors[activeColorID]);
    break;
  case 39:
    matrix.fillScreen(0);
    break;
  case 40:
    matrix.fillScreen(0);
    twenty();
    break;
  case 41:
    matrix.fillScreen(0);
    nine(false);
    ten(false, false);
    break;
  case 42:
    matrix.fillScreen(0);
    eight(false);
    ten(false, false);
    break;
  case 43:
    matrix.fillScreen(0);
    seven(false, false);
    ten(false, false);
    break;
  case 44:
    matrix.fillScreen(0);
    six(false);
    ten(false, false);
    break;
  case 45:
    matrix.fillScreen(0);
    five(false, false);
    ten(false, false);
    break;
  case 46:
    matrix.fillScreen(0);
    four(false);
    ten(false, false);
    break;
  case 47:
    matrix.fillScreen(0);
    three();
    ten(false, false);
    break;
  case 48:
    matrix.fillScreen(0);
    twelve(false);
    break;
  case 49:
    matrix.fillScreen(0);
    eleven(false);
    break;
  case 50:
    matrix.fillScreen(0);
    ten(false, false);
    break;
  case 51:
    matrix.fillScreen(0);
    nine(false);
    break;
  case 52:
    matrix.fillScreen(0);
    eight(false);
    break;
  case 53:
    matrix.fillScreen(0);
    seven(true, false);
    break;
  case 54:
    matrix.fillScreen(0);
    six(false);
    break;
  case 55:
    matrix.fillScreen(0);
    five(false, false);
    break;
  case 56:
    matrix.fillScreen(0);
    four(false);
    break;
  case 57:
    matrix.fillScreen(0);
    three();
    break;
  case 58:
    matrix.fillScreen(0);
    two();
    break;
  case 59:
    matrix.fillScreen(0);
    one(true);
    break;
  default:
    break;
  }
}