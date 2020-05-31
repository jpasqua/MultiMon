//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
#include <TimeLib.h>
//                                  Local Includes
#include "MMScreen.h"
#include "images/RepRapMono.h"
#include "images/OctoMono.h"
//--------------- End:    Includes ---------------------------------------------

using GUI::tft;

class SplashScreen : public Screen {
public:
  SplashScreen() {
    nButtons = 0;
    buttons = NULL;
  }

  void display(bool activating = false) {
    (void)activating; // We don't use this parameter - avoid a warning...

    tft.fillScreen(GUI::Color_SplashBkg);
    tft.drawBitmap(
        6, 0, OctoMono, OctoMono_Width, OctoMono_Height,
        GUI::Color_SplashBkg, GUI::Color_SplashOcto);
    tft.drawBitmap(
        Screen::XCenter+6, 0, RepRapMono, RepRapMono_Width, RepRapMono_Height,
        GUI::Color_SplashBkg, GUI::Color_SplashRR);

    tft.setFreeFont(&FreeSansBoldOblique24pt7b);
    tft.setTextColor(GUI::Color_SplashText);
    tft.setTextDatum(BC_DATUM);
    tft.drawString("MultiMon", Screen::XCenter, Screen::Height-1);
  }

  virtual void processPeriodicActivity() {
    // Nothing to do here, we're displaying a static screen
  }

private:
};