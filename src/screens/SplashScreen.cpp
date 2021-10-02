//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
#include <TimeLib.h>
//                                  WebThing Includes
#include <WTApp.h>
#include <gui/Display.h>
#include <gui/Theme.h>
//                                  Local Includes
#include "SplashScreen.h"
#include "AppTheme.h"
#include "images/Duet3DMono.h"
#include "images/OctoMono.h"
//--------------- End:    Includes ---------------------------------------------


SplashScreen::SplashScreen() {
  nLabels = 0;
  labels = NULL;
}

void SplashScreen::display(bool) {
  auto& tft = Display.tft;

  tft.fillScreen(Theme::Color_SplashBkg);
  tft.drawBitmap(
      6, 10, OctoMono, OctoMono_Width, OctoMono_Height,
      Theme::Color_SplashBkg, AppTheme::Color_SplashOcto);
  tft.drawBitmap(
      Display.XCenter+6, 10, Duet3DMono, Duet3DMono_Width, Duet3DMono_Height,
      Theme::Color_SplashBkg, AppTheme::Color_SplashD3);

  Display.fonts.setUsingID(Display.fonts.FontID::SBO24, tft);
  tft.setTextColor(Theme::Color_SplashText);
  tft.setTextDatum(BC_DATUM);
  tft.drawString(wtApp->appName, Display.XCenter, Display.Height-1);
}

void SplashScreen::processPeriodicActivity() {
  // Nothing to do here, we're displaying a static screen
}
