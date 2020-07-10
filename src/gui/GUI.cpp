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
#include "FlexScreen.h"
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
#include "fonts/DSEG7_Classic_Bold_22.h"
#include "fonts/DSEG7_Classic_Bold_72.h"
#include "fonts/DSEG7_Classic_Bold_100.h"
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
  FlexScreen blynkScreen;
  Screen *curScreen = NULL;

  static const uint8_t MaxFlexScreens = 5;
  FlexScreen* flexScreens[MaxFlexScreens];
  uint8_t nFlexScreens = 0;

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
      // tft.setFreeFont(&FreeSerifBoldItalic9pt7b);
      tft.setFreeFont(&FreeSansBold9pt7b);
      tft.setTextColor(textColor);
      tft.drawString("i", centerX, centerY);
    }

    void initInfoIcon() {
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

  namespace Font {
    const struct  {
      const char *name;
      const GFXfont *font;
    } GFXFonts[] = {
      // ORDER MUST MATCH GUI::Font::FontID enum
      {"M9",    &FreeMono9pt7b},
      {"MB9",   &FreeMonoBold9pt7b},
      {"MO9",   &FreeMonoOblique9pt7b},
      {"MBO9",  &FreeMonoBoldOblique9pt7b},

      {"S9",    &FreeSans9pt7b},
      {"SB9",   &FreeSansBold9pt7b},
      {"SO9",   &FreeSansOblique9pt7b},
      {"SBO9",  &FreeSansBoldOblique9pt7b},

      {"S12",   &FreeSans12pt7b},
      {"SB12",  &FreeSansBold12pt7b},
      {"SO12",  &FreeSansOblique12pt7b},
      {"SBO12", &FreeSansBoldOblique12pt7b},

      {"S18",   &FreeSans18pt7b},
      {"SB18",  &FreeSansBold18pt7b},
      {"SO18",  &FreeSansOblique18pt7b},
      {"SBO18", &FreeSansBoldOblique18pt7b},

      {"S24",   &FreeSans24pt7b},
      {"SB24",  &FreeSansBold24pt7b},
      {"SO24",  &FreeSansOblique24pt7b},
      {"SBO24", &FreeSansBoldOblique24pt7b},

      {"D20",   &DSEG7_Classic_Bold_20},
      {"D72",   &DSEG7_Classic_Bold_72},
      {"D100",  &DSEG7_Classic_Bold_100}
    };
    const uint8_t nGFXFonts = ARRAY_SIZE(GFXFonts);

    void setUsingID(uint8_t fontID, TFT_eSPI& t) { t.setFreeFont(GFXFonts[fontID].font); }
    void setUsingID(uint8_t fontID, TFT_eSprite *s) { s->setFreeFont(GFXFonts[fontID].font); }

    int8_t idFromName(String fontName) {
      for (int i = 0; i < nGFXFonts; i++) {
        if (fontName == GFXFonts[i].name) return i;
      }
      return -1;
    }

    uint8_t getHeight(uint8_t fontID) { return GFXFonts[fontID].font->yAdvance; }
  }  // ----- END: GUI::Font namespace


  void setBrightness(uint8_t b) {
    if (TFT_LED == -1) return;
    if (b == brightness) return;
    brightness = b;
    int analogValue = map(brightness, 0, 100, 0, PWMRANGE);
    analogWrite(TFT_LED, analogValue);
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
      uint32_t today = day();
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
    if (TFT_LED != -1) pinMode(TFT_LED, OUTPUT);
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
      Log.trace(F("GUI::init: Applying calibration data from settings"));
      tft.setTouch(MultiMon::settings.calibrationData);
    } else {
      Log.trace(F("GUI::init: No valid calibration data in settings"));
    }

    Internal::initInfoIcon();
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

  void displayFlexScreen(String name)  {
    for (int i = 0; i < nFlexScreens; i++) {
      if (name.equalsIgnoreCase(flexScreens[i]->getName())) {
        curScreen = flexScreens[i];
        curScreen->activate();
        return;
      }
    }
    Log.error(F("Requesting a non-existent screen: %s"), name.c_str());
  }

  bool createFlexScreen(
      JsonObjectConst &descriptor,
      uint32_t refreshInterval,
      const Basics::StringMapper &vc) {
    if (nFlexScreens == MaxFlexScreens) {
      Log.warning(F("Maximum number of FlexScreens exceeded"));
      return false;
    }

    FlexScreen *flexScreen = new FlexScreen();
    if (!flexScreen->init(descriptor, refreshInterval, vc)) {
      delete flexScreen;
      return false;
    }

    flexScreens[nFlexScreens++] = flexScreen;
    return true;
  }

  void showUpdatingIcon(uint16_t accentColor) { Internal::showUpdating(accentColor); }
  void hideUpdatingIcon() { Internal::hideUpdating(); }


  uint32_t getSizeOfScreenShotAsBMP() {
    return (2ul * tft.width() * tft.height() + 54); // pix data + 54 byte hdr
  }

  void streamScreenShotAsBMP(Stream &s) {
    // Adapted form https://forum.arduino.cc/index.php?topic=406416.0
    byte hiByte, loByte;
    uint16_t i, j = 0;

    uint8_t bmFlHdr[14] = {
      'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0
    };
    // 54 = std total "old" Windows BMP file header size = 14 + 40
    
    uint8_t bmInHdr[40] = {
      40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 16, 0
    };   
    // 40 = info header size
    //  1 = num of color planes
    // 16 = bits per pixel
    // all other header info = 0, including RI_RGB (no compr), DPI resolution

    uint32_t w = tft.width();
    uint32_t h = tft.height();
    unsigned long bmpSize = 2ul * h * w + 54; // pix data + 54 byte hdr
    
    bmFlHdr[ 2] = (uint8_t)(bmpSize      ); // all ints stored little-endian
    bmFlHdr[ 3] = (uint8_t)(bmpSize >>  8); // i.e., LSB first
    bmFlHdr[ 4] = (uint8_t)(bmpSize >> 16);
    bmFlHdr[ 5] = (uint8_t)(bmpSize >> 24);

    bmInHdr[ 4] = (uint8_t)(w      );
    bmInHdr[ 5] = (uint8_t)(w >>  8);
    bmInHdr[ 6] = (uint8_t)(w >> 16);
    bmInHdr[ 7] = (uint8_t)(w >> 24);
    bmInHdr[ 8] = (uint8_t)(h      );
    bmInHdr[ 9] = (uint8_t)(h >>  8);
    bmInHdr[10] = (uint8_t)(h >> 16);
    bmInHdr[11] = (uint8_t)(h >> 24);

    s.write(bmFlHdr, sizeof(bmFlHdr));
    s.write(bmInHdr, sizeof(bmInHdr));

    for (i = h; i > 0; i--) {
      byte buf[w*2];
      byte *ptr = &buf[0];
      for (j = 0; j < w; j++) {
        uint16_t rgb = tft.readPixel(j,i);  // Get pixel in rgb565 format
        
        hiByte = (rgb & 0xFF00) >> 8;   // High Byte
        loByte = (rgb & 0x00FF);        // Low Byte
        
        // RGB565 to RGB555 conversion... 555 is default for uncompressed BMP
        loByte = (hiByte << 7) | ((loByte & 0xC0) >> 1) | (loByte & 0x1f);
        hiByte = (hiByte >> 1);
        
        *ptr++ = loByte;
        *ptr++ = hiByte;
      }
      s.write(buf, w*2);
    }
  }
} // ----- END: GUI namespace