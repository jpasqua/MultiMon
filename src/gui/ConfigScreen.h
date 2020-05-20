/*
 * ConfigScreen:
 *    Displays a screen with instructions when the device is going through
 *    an initil WiFi setup.
 *                    
 * TO DO:
 *
 * COMPLETE:
 *
 */


//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
#include <TimeLib.h>
//                                  Local Includes
#include "MMScreen.h"
#include "images/Gears160x240.h"
//--------------- End:    Includes ---------------------------------------------

using GUI::tft;

class ConfigScreen : public Screen {
public:
  ConfigScreen() {
    nButtons = 0;
    buttons = NULL;
  }

  void display(bool activating = false) {
    tft.fillScreen(GUI::Color_Background);
    tft.drawBitmap(0, 0, Gears160x240, 160, 240, GUI::Color_AlertGood);

    tft.setFreeFont(&FreeSans12pt7b);
    tft.setTextSize(1);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(GUI::Color_NormalText);
    tft.drawString("Initial",    160+(160/2), 50);
    tft.drawString("Setup:",     160+(160/2), 72);
    tft.setTextColor(GUI::Color_AlertGood);
    tft.drawString("Connect to", 160+(160/2), 100);
    tft.drawString("WiFi named", 160+(160/2), 122);
    tft.setTextColor(GUI::Color_NormalText);
    tft.drawString(_ssid,        160+(160/2), 156);
    tft.setTextDatum(TL_DATUM);
  }

  virtual void processPeriodicActivity() {
    // Nothing to do here, we're displaying a static screen
  }

  void setSSID(String &ssid) { _ssid = ssid; }

private:
  String _ssid;
};