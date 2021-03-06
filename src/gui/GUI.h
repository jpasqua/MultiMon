#ifndef GUI_h
#define GUI_h

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
//                                  Local Includes
#include "../Basics.h"
#include "FlexScreen.h"
//--------------- End:    Includes ---------------------------------------------


namespace GUI {
  static const uint32_t Color_Online = TFT_GREEN;
  static const uint32_t Color_Offline = TFT_RED;
  static const uint32_t Color_Progress = TFT_DARKGREEN;
  static const uint32_t Color_Inactive = TFT_DARKGREY;

  static const uint32_t Color_AlertError = TFT_RED;
  static const uint32_t Color_AlertGood = TFT_GREEN;

  static const uint32_t Color_Nickname = 0xE51D;      // Light Purple
  static const uint32_t Color_NormalText = TFT_WHITE;
  static const uint32_t Color_DimText = 0xAD75;       // Darker than TFT_LIGHTGREY

  static const uint32_t Color_Border = TFT_WHITE;
  static const uint32_t Color_Background = TFT_BLACK;

  static const uint32_t Color_SplashBkg  = TFT_BLACK;  // Can swap white and black
  static const uint32_t Color_SplashText = TFT_WHITE; // Can swap white and black
  static const uint32_t Color_SplashOcto = TFT_GREEN;
  static const uint32_t Color_SplashDuet = TFT_BLUE;
  static const uint32_t Color_SplashRR   = 0x0488;
    // RR Logo color is #029346, 565 version of that is 0x0488
  static const uint32_t Color_SplashD3   = 0x02DF;
  //static const uint32_t Color_SplashD3   = 0x000E;
    // Duet3D Logo color is #040073, 565 version of that is 0x000E

  static const uint32_t Color_WeatherTxt = TFT_PURPLE;
  static const uint32_t Color_WeatherBkg = TFT_WHITE;

  static const uint32_t Color_UpdatingWeather = TFT_ORANGE;
  static const uint32_t Color_UpdatingPrinter = TFT_DARKGREEN;
  static const uint32_t Color_UpdatingPlugins = TFT_SKYBLUE;
  static const uint32_t Color_UpdatingText = TFT_WHITE;
  static const uint32_t Color_UpdatingFill = TFT_BLACK;

  static const uint32_t Color_WiFiBlue = 0x7D1A;

  static const uint32_t Mono_Background = 0x0000;     // Background for 1bpp sprites
  static const uint32_t Mono_Foreground = 0x0001;     // Foreground for 1bpp sprites

  void init(bool flipDisplay = false);
  void setOrientation(bool flip);

  void setBrightness(uint8_t b);
  uint8_t getBrightness();
  
  void loop();

  FlexScreen* createFlexScreen(
      JsonDocument &doc,
      uint32_t refreshInterval,
      const Basics::ReferenceMapper &mapper);

  void displayHomeScreen();
  void displayCalibrationScreen();
  void displayConfigScreen(String &ssid);
  void displayDetailScreen(int index);
  void displayForecastScreen();
  void displayRebootScreen();
  void displaySplashScreen();
  void displayTimeScreen();
  void displayWeatherScreen();
  void displayWiFiScreen();
  void displayUtilityScreen();
  void displayFlexScreen(FlexScreen* fs);
  void displayFlexScreenByID(String id);
  void displayNextPlugin();

  /**
   * Overlay the current screen with an icon to indicate that a potentially
   * long-running update is in progress. this lets the user know that the UI
   * will be unresponsive in this period. Calling showUpdatingIcon()
   * when the icon is already displayed is safe and does nothing.
   * @param   accentColor   An accent color to indicate what's happening
   */
  void showUpdatingIcon(uint16_t accentColor);

  /**
   * Remove the "updating icon" from the current screen and restore the original
   * screen content. Calling hideUpdatingIcon() when no icon is displayed
   * is safe and does nothing.
   */
  void hideUpdatingIcon();

  /**
   * Dump the pixel values from the display to the specified stream in the form
   * of a BMP image. This can be used to take "screen shots".
   * @param s     The Stream to which the screen shot in BMP format should be written
   */
  void streamScreenShotAsBMP(Stream &s);
  uint32_t getSizeOfScreenShotAsBMP();

  extern TFT_eSPI tft;
  extern TFT_eSprite *sprite;

  namespace Font {
    enum FontID {
      M9,  MB9,  MO9,  MBO9,
      S9,  SB9,  SO9,  SBO9,
      S12, SB12, SO12, SBO12,
      S18, SB18, SO18, SBO18,
      S24, SB24, SO24, SBO24,
      D20, D72,  D100
    };


    extern void setUsingID(uint8_t fontID, TFT_eSPI& t);
    extern void setUsingID(uint8_t fontID, TFT_eSprite *s);
    extern int8_t idFromName(String fontName);
    extern uint8_t getHeight(uint8_t fontID);
  }
}

#endif  // GUI_h