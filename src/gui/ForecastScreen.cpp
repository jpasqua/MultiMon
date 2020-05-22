/*
 * ForecastScreen:
 *    Display forecast info for the next 9 hours and the next
 *    three days.
 *                    
 * TO DO:
 * o Respect settings.use24Hour
 *
 * COMPLETE
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
//                                  Local Includes
#include "ForecastScreen.h"
#include "../Basics.h"
#include "../clients/OWMClient.h"
#include "images/Icons.h"
//--------------- End:    Includes ---------------------------------------------


using GUI::tft;

/*------------------------------------------------------------------------------
 *
 * CONSTANTS
 *
 *----------------------------------------------------------------------------*/

static const auto ReadingsFont = &FreeSansBold9pt7b;
static const uint16_t ReadingsFontHeight = 22;

static const uint16_t TileWidth = Screen::Width/2;
static const uint16_t TileHeight = Screen::Height/3;
static const uint16_t TextVPad = (TileHeight - (2*ReadingsFontHeight))/2;


/*------------------------------------------------------------------------------
 *
 * Constructors and Public methods
 *
 *----------------------------------------------------------------------------*/

ForecastScreen::ForecastScreen() {
  auto buttonHandler =[&](int id, Button::PressType type) -> void {
    Log.verbose("In ForecastScreen Button Handler, id = %d, type = %d", id, type);
    if (type > Button::PressType::NormalPress) GUI::displayWeatherScreen();
    GUI::displayHomeScreen();
  };

  buttons = new Button[(nButtons = 1)];
  buttons[0].init(0, 0, Screen::Width, Screen::Height, buttonHandler, 0);
}


void ForecastScreen::display(bool activating) {
  // We should not get here if owm is disabled or we were unable to contact the
  // OWM service, but just double check...
  if (!MultiMon::settings.owm.enabled || !MultiMon::owmClient) return;

  tft.fillScreen(GUI::Color_WeatherBkg);
  uint16_t x = 0, y = 0;
  Forecast *f = MultiMon::owmClient->getForecast();
  for (int i = 0; i < OWMClient::ForecastElements; i++) {
    displaySingleForecast(&f[i], x, y);
    if (i == 2) x += TileWidth;
    y = (y + TileHeight) % Screen::Height;
  }
}

void ForecastScreen::processPeriodicActivity() {
  // Nothing to do here...
}

void ForecastScreen::displaySingleForecast(Forecast *f, uint16_t x, uint16_t y) {
  tft.pushImage(
      x, y+((TileHeight-WI_Height)/2),
      WI_Width, WI_Height,
      getWeatherIcon(f->icon),
      WI_Transparent);
  x += WI_Width;

  tft.setTextColor(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);

  String reading = "";
  String units = MultiMon::settings.useMetric ? "C" : "F";

  if (f->loTemp != Forecast::NoReading) reading = String((int)(f->loTemp+0.5)) + " / ";
  reading += String((int)(f->hiTemp+0.5)) + units;

  y += TextVPad;
  tft.setTextColor(TFT_BLACK);
  tft.setFreeFont(ReadingsFont);
  tft.drawString(reading, x, y);

  y += ReadingsFontHeight;
  reading = dayShortStr(weekday(f->dt));
  int h = hour(f->dt);
  bool pm = isPM(f->dt);
  if (pm) h -= 12;
  else if (h == 0) h = 12;
  reading += " " + String(h) + (pm ? "P" : "A");

  tft.drawString(reading, x, y);
}
