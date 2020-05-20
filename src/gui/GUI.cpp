/*
 * GUI:
 *    Provide the framework for the local on-screen UI (as opposed to the WebUI) 
 *                    
 * TO DO:
 *
 * COMPLETE
 * o Update to latest version of TFT_eSPI and get rid of pushSprite workaround
 * o Implement an overlay icon to show that we're performing a long update and
 *   therefore the UI is unresponsive. Read pixels from the spot where the
 *   icon is displayed so that when the icon is removed, the display can be
 *   updated without redrawing.
 * o Explore more dynamic options for allocating the savedPixels area. The
 *   method in use (malloc savedPixels once at the start) seems to provide the
 *   best results in terms of fragmentation and stability.
 * o Update to latest TFT_eSPI which has a bug fix that avoids need for setSwpaBytes()
 *
 */


//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
#include <FS.h>
#include <ESP8266WiFi.h>
          // Arg! The convoluted Arduino compilation process requires these to be
          // included here even though they are used in compilation units that
          // that include them explicitly!!
//                                  Third Party Libraries
#include <TFT_eSPI.h>
//                                  Local Includes
#include "../../MultiMon.h"
#include "GUI.h"
#include "CalibrationScreen.h"
#include "ConfigScreen.h"
#include "DetailScreen.h"
#include "ForecastScreen.h"
#include "InfoScreen.h"
#include "RebootScreen.h"
#include "SplashScreen.h"
#include "StatusScreen.h"
#include "TimeScreen.h"
#include "WeatherScreen.h"
#include "WiFiScreen.h"
//--------------- End:    Includes ---------------------------------------------


namespace GUI {
  TFT_eSPI tft = TFT_eSPI();
  TFT_eSprite *sprite = new TFT_eSprite(&tft);
  bool flipped;
  uint8_t brightness = 0;     // 0-100, Will be set to initial value in init()
  StatusScreen statusScreen;
  DetailScreen detailScreen;
  TimeScreen   timeScreen;
  ConfigScreen configScreen;
  RebootScreen rebootScreen;
  SplashScreen splashScreen;
  WeatherScreen weatherScreen;
  ForecastScreen forecastScreen;
  CalibrationScreen calibrationScreen;
  InfoScreen infoScreen;
  WiFiScreen wiFiScreen;
  Screen *curScreen = NULL;

  namespace Internal {
    static const uint16_t InfoIconSize = 32;
    static const uint16_t InfoIconBorderSize = 5;
    static const uint16_t InfoIconX = (Screen::Width - InfoIconSize);
    static const uint16_t InfoIconY = 0;
    uint16_t *savedPixels = NULL;
    bool infoIconIsDisplayed = false;

    void drawInfoIcon(uint16_t x, uint16_t y, uint16_t borderColor, uint16_t fillColor, uint16_t textColor) {
      uint16_t centerX = x+(InfoIconSize/2);
      uint16_t centerY = y+(InfoIconSize/2);
      tft.fillCircle(centerX, centerY, InfoIconSize/2-1, borderColor);
      tft.fillCircle(centerX, centerY, (InfoIconSize/2-1)-InfoIconBorderSize, fillColor);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSerifBoldItalic9pt7b);
      tft.setTextColor(textColor);
      tft.drawString("i", centerX, centerY);
    }

    void initUpdatingMessage() {
      // In theory it would be better to allocate/deallocate this as needed, but it causes
      // a lot more fragmentation and potentially a crash.
      savedPixels = (uint16_t *)malloc(InfoIconSize*InfoIconSize*sizeof(uint16_t));  // This is BIG!
    }

    void showUpdating(uint16_t accentColor) {
      if (infoIconIsDisplayed) return;
      tft.readRect(InfoIconX, InfoIconY, InfoIconSize, InfoIconSize, savedPixels);
      drawInfoIcon(InfoIconX, InfoIconY, accentColor, Color_UpdatingFill, Color_UpdatingText);
      infoIconIsDisplayed = true;
    }

    void hideUpdating() {
      if (!infoIconIsDisplayed) return;
      tft.pushRect(InfoIconX, InfoIconY, InfoIconSize, InfoIconSize, savedPixels);
      infoIconIsDisplayed = false;
    }
  }  // ----- END: GUI::Internal


  void setBrightness(uint8_t b) {
    if (b == brightness) return;
    brightness = b;
    int analogValue = map(brightness, 0, 100, 0, PWMRANGE);
    analogWrite(MultiMon::Pin_LEDBrightnessControl, analogValue);
  }

  uint8_t getBrightness() { return brightness; }

  void processSchedules() {
    static uint32_t eveningExecutedOnDay = UINT32_MAX;
    static uint32_t morningExecutedOnDay = UINT32_MAX;
    static uint32_t nextScheduleCheck = 0;

    if (MultiMon::settings.scheduleActive) {
      uint16_t morning = MultiMon::settings.morning.hr * 100 + MultiMon::settings.morning.min;
      uint16_t evening = MultiMon::settings.evening.hr * 100 + MultiMon::settings.evening.min;
      uint32_t curMillis = millis();
      int today = day();
      if (curMillis > nextScheduleCheck) {
        nextScheduleCheck = curMillis + (60-second()) * 1000L;
        uint16_t curTime = hour() * 100 + minute();
        if (curTime >= evening || curTime < morning) {
          if (eveningExecutedOnDay != today) {
            setBrightness(MultiMon::settings.evening.brightness);
            eveningExecutedOnDay = today;
          }
        } else if (morningExecutedOnDay != today) {
          setBrightness(MultiMon::settings.morning.brightness);
          morningExecutedOnDay = today;
        }
      }
    }
  }

  void init(bool flipDisplay) {
    flipped = flipDisplay;
    if (MultiMon::Pin_LEDBrightnessControl != -1) pinMode(MultiMon::Pin_LEDBrightnessControl, OUTPUT);
    setBrightness(100);
    tft.init();
    tft.setSwapBytes(true);
    tft.setRotation(flipDisplay ? 3 : 1);
    tft.fillScreen(Color_Background);

    uint16_t sum = 0;
    for (int i = 0; i < MMSettings::nCalReadings; i++) { sum += MultiMon::settings.calibrationData[i]; }
    if (sum) {
      // All zeroes in the calibration data means that it isn't set. 
      // We've got some values, so pass it in to the tft
      Log.trace("GUI::init: Applying calibration data from settings");
      tft.setTouch(MultiMon::settings.calibrationData);
    } else {
      Log.trace("GUI::init: No valid calibration data in settings");
    }

    Internal::initUpdatingMessage();
  }

  uint8_t getTouch(uint16_t *x, uint16_t *y) { return tft.getTouch(x, y); }

  void setOrientation(bool flip) {
    if (flip == flipped) return;
    flipped = flip;
    tft.setRotation(flip ? 3 : 1);
    curScreen->display(true);
  }

  void loop() {
    if (curScreen == NULL) return;
    curScreen->processPeriodicActivity();
    curScreen->processInput();
    processSchedules();
  }


  void inline display(Screen &s) { curScreen = &s; s.activate();  }

  void displayHomeScreen() { display(timeScreen); }

  void displayWiFiScreen() { display(wiFiScreen); }

  void displayStatusScreen() { display(statusScreen); }

  void displayDetailScreen(int index) { detailScreen.setIndex(index); display(detailScreen); }

  void displayTimeScreen() { display(timeScreen); }

  void displayConfigScreen(String &ssid) { configScreen.setSSID(ssid); display(configScreen); }

  void displaySplashScreen() { display(splashScreen); }

  void displayRebootScreen() { if (curScreen != &rebootScreen) display(rebootScreen); }

  void displayWeatherScreen() { display(weatherScreen); }

  void displayCalibrationScreen() { display(calibrationScreen); }

  void displayInfoScreen() { display(infoScreen); }

  void displayForecastScreen() { display(forecastScreen); }

  void showUpdatingIcon(uint16_t accentColor) { Internal::showUpdating(accentColor); }
  void hideUpdatingIcon() { Internal::hideUpdating(); }


} // ----- END: GUI namespace