//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
#include <TimeLib.h>
//                                  Local Includes
#include "MMScreen.h"
#include "images/WiFiLogo.h"
//--------------- End:    Includes ---------------------------------------------

using GUI::tft;

class WiFiScreen : public Screen {
public:
  WiFiScreen() {
    nButtons = 0;
    buttons = NULL;
  }

  void display(bool activating = false) {
    (void)activating; // We don't use this parameter - avoid a warning...
    tft.fillScreen(GUI::Color_Background);
    uint16_t x = (Screen::Width-WiFiLogo_Width)/2;
    uint16_t y = 30;  // A little space from the top of the screen
    tft.pushImage(x, y, WiFiLogo_Width, WiFiLogo_Height, WiFiLogo, WiFiLogo_Transparent);
    y += WiFiLogo_Height;

    tft.setFreeFont(&FreeMonoBoldOblique12pt7b);
    tft.setTextColor(GUI::Color_WiFiBlue);
    tft.setTextDatum(MC_DATUM);
    x = Screen::XCenter;
    y += (Screen::Height - y)/2;
    tft.drawString("Connecting...", x, y);
  }

  virtual void processPeriodicActivity() {
    // Nothing to do here, we're displaying a static screen
  }

};