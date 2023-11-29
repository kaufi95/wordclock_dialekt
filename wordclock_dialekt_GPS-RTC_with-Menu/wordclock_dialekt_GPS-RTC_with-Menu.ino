// TODO: find and fix error: clock works only when connected to pc

#include <EEPROM.h>
#include <RTClib.h>
#include <TimeLib.h>
#include <Button2.h>
#include <Timezone.h>
#include <TinyGPS++.h>
#include <Adafruit_NeoMatrix.h>

// define pins
#define BUTTON_PIN 3   // define pin for color switching
#define NEOPIXEL_PIN 6 // define pin for Neopixels

// define global variables
bool menuSwitch = false;      // menu active
bool initialSync = false;     // sync on first valid gps signal
const int eeC = 0;            // eeprom address for color
const int eeB = 1;            // eeprom address for brightness
const int eeL = 2;            // eeprom address for language (0: DIA, 1: HD)
byte currentMenu = 0;         // current menu (0: color, 1: brightness, 2: language)
byte settings[3] = {0, 0, 0}; // settings array (color, brightness, language)
int longPressTime = 100;      // longpresstime for button

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
Button2 button;

// define time change rules and timezone
TimeChangeRule atST = {"ST", Last, Sun, Mar, 2, 120}; // UTC + 2 hours
TimeChangeRule atRT = {"RT", Last, Sun, Oct, 3, 60};  // UTC + 1 hour
Timezone AT(atST, atRT);

void setup()
{
  // enable serial output
  Serial.begin(9600);
  Serial.println("WordClock");
  Serial.println("version 2");
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
    // this will adjust to the date and time of compilation if rtc lost power
    rtc.adjust(DateTime(2021, 1, 1, 0, 0, 0));
  }

  // init LED matrix
  Serial.println("initiating matrix");
  matrix.begin();
  matrix.fillScreen(0);
  matrix.show();

  // define color button as input
  Serial.println("changing pinMode");
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // sync on first valid gps signal
  initialSync = true;

  if (digitalRead(BUTTON_PIN) == 0)
  {
    Serial.println("button pressed @ setup");
    Serial.println("entering menu...");
    menuSwitch = true;

    // Initialize the button.
    button.begin(BUTTON_PIN);
    button.setDebounceTime(100);
    button.setLongClickTime(1500);
    button.setClickHandler(handler);
    button.setLongClickHandler(handler);
  }

  // read color, brightness and language
  Serial.println("reading eeprom @ setup");
  settings[0] = EEPROM.read(eeC); // read color from eeprom
  settings[1] = EEPROM.read(eeB); // read brightness from eeprom
  settings[2] = EEPROM.read(eeL); // read language from eeprom

  printEEPROM();
}

void loop()
{
  if (menuSwitch)
  {
    button.loop();
    showMenuOnMatrix();
    return;
  }

  refreshMatrix();
  smartDelay(1000);
  displayTimeInfo(generateTimeByRTC(), "RTC");
  displayTimeInfo(generateTimeByGPS(), "GPS");
  displayTimeInfo(AT.toLocal(generateTimeByRTC()), "AT ");
  syncTime();
}

// ----------------------------------------------------------------------------------------------------

// handles button events
void handler(Button2 &btn)
{
  switch (btn.getType())
  {
  case long_click:
    currentMenu = (currentMenu + 1) % 4;
  case single_click:
    switch (currentMenu)
    {
    case 0:
      settings[currentMenu] = (settings[currentMenu] + 1) % 7;
      EEPROM.write(eeC, settings[0]);
      break;
    case 1:
      settings[currentMenu] = (settings[currentMenu] + 1) % 4;
      EEPROM.write(eeB, settings[1]);
      break;
    case 2:
      settings[currentMenu] = (settings[currentMenu] + 1) % 2;
      EEPROM.write(eeL, settings[2]);
      break;
    case 3:
      Serial.println("exiting menu...");
      menuSwitch = false;
      //   writeValuesToEEPROM();
      break;
    }
  }
  printMenu();
}

// ----------------------------------------------------------------------------------------------------

// syncronizes time with gps module
void syncTime()
{
  // get time from GPS module
  if (gps.time.isValid() && gps.date.isValid() && gps.date.year() >= 2021)
  {
    if (initialSync || gps.time.minute() != rtc.now().minute() || gps.time.second() != rtc.now().second())
    {
      Serial.println("setting rtctime...");
      rtc.adjust(generateTimeByGPS());
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
    while (Serial1.available())
    {
      gps.encode(Serial1.read());
    }
  } while (millis() - start < ms);
}

// generates time object
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

// turns the pixels from startIndex to endIndex of a specific row on
void turnPixelsOn(uint16_t startIndex, uint16_t endIndex, uint16_t row)
{
  for (int i = startIndex; i <= endIndex; i++)
  {
    matrix.drawPixel(i, row, colors[settings[0]]);
  }
}

// clears matrix, calculates matrix and fills it
void refreshMatrix()
{
  matrix.fillScreen(0);
  byte brightness = 64 + settings[1] * 16;
  matrix.setBrightness(brightness);
  time_t convertedTime = AT.toLocal(generateTimeByRTC());
  timeToMatrix(convertedTime);
  matrix.show();
}

// // writes values to eeprom
// void writeValuesToEEPROM()
// {
//   Serial.println("writing values to eeprom");
//   for (int i = 0; i < 3; i++)
//   {
//     EEPROM.write(i, settings[i]);
//   }
// }

// ----------------------------------------------------------------------------------------------------

// print the eeeprom values
void printEEPROM()
{
  Serial.println("reading eeprom");
  for (int i = 0; i < 3; i++)
  {
    Serial.print("Byte ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(EEPROM.read(i));
  }
}

// outputs settings on matrix
void showMenuOnMatrix()
{
  matrix.fillScreen(0);
  turnPixelsOn(0, settings[currentMenu], 0);
  turnPixelsOn(0, currentMenu, 10);
  matrix.show();
}

// prints the current menu
void printMenu()
{
  Serial.println("current menu: ");
  switch (currentMenu)
  {
  case 0:
    Serial.println("color");
    Serial.print("current value:");
    switch (settings[currentMenu])
    {
    case 0:
      Serial.println("white");
      break;
    case 1:
      Serial.println("red");
      break;
    case 2:
      Serial.println("green");
      break;
    case 3:
      Serial.println("blue");
      break;
    case 4:
      Serial.println("cyan");
      break;
    case 5:
      Serial.println("magenta");
      break;
    case 6:
      Serial.println("yellow");
      break;
    }
    break;
  case 1:
    Serial.println("brightness");
    Serial.print("current value:");
    switch (settings[currentMenu])
    {
    case 0:
      Serial.println("low");
      break;
    case 1:
      Serial.println("medium");
      break;
    case 2:
      Serial.println("high");
      break;
    case 3:
      Serial.println("very high");
      break;
    }
    break;
  case 2:
    Serial.println("language");
    Serial.print("current value:");
    switch (settings[currentMenu])
    {
    case 0:
      Serial.println("DIA");
      break;
    case 1:
      Serial.println("HD");
      break;
    }
    break;
  case 3:
    Serial.println("exit...");
    break;
  }
}

// display details of gps signal
void displayTimeInfo(time_t t, String component)
{

  Serial.print(component + ": ");

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

// ----------------------------------------------------------------------------------------------------

// converts time into matrix
void timeToMatrix(time_t time)
{
  uint8_t hours = hour(time);
  uint8_t minutes = minute(time);

  Serial.println("timeToMatrix");
  // Es isch/ist
  switch (settings[2])
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
    matrix.drawPixel(i - 1, 10, colors[settings[0]]);
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
  switch (settings[2])
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
  switch (settings[2])
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
  switch (settings[2])
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
  switch (settings[2])
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
    switch (settings[2])
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
    switch (settings[2])
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
  switch (settings[2])
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
  switch (settings[2])
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
  switch (settings[2])
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
  switch (settings[2])
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
    switch (settings[2])
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
    switch (settings[2])
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
  switch (settings[2])
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
  switch (settings[2])
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
  switch (settings[2])
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
  switch (settings[2])
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
  switch (settings[2])
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
  switch (settings[2])
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
  switch (settings[2])
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
  switch (settings[2])
  {
  case 0:
    break;
  case 1:
    Serial.print(" uhr");
    turnPixelsOn(8, 10, 9);
    break;
  }
}