/*
 * ForecastScreen:
 *    Display the 5-day forecast. Since we are arranging the readings in two columns
 *    with the 3 forecast cells each, we've got an extra spot. Make the first spot
 *    the current readings and the next 5 spots the 5-day forecast.
 *                    
 * TO DO:
 *
 * COMPLETE
 * o Respect settings.use24Hour
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
static const uint16_t ReadingsFontHeight = ReadingsFont->yAdvance;

static const uint16_t TileWidth = Screen::Width/2;
static const uint16_t TileHeight = Screen::Height/3;
static const uint16_t TextVPad = (TileHeight - (2*ReadingsFontHeight))/2;

static int16_t TinyFont = 2;  // A small 5x7 font

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
  (void)activating; // We don't use this parameter - avoid a warning...

  // We should not get here if owm is disabled or we were unable to contact the
  // OWM service, but just double check...
  if (!MultiMon::settings.owm.enabled || !MultiMon::owmClient) return;

  tft.fillScreen(GUI::Color_WeatherBkg);
  uint16_t x = 0, y = 0;

  // The first element of the forecast display is really the current temperature
  Forecast current;
  current.dt = MultiMon::owmClient->weather.dt + WebThing::getGMTOffset();
  current.hiTemp = MultiMon::owmClient->weather.readings.temp;
  current.loTemp = Forecast::NoReading;
  current.icon = MultiMon::owmClient->weather.description.icon;
  displaySingleForecast(&current, x, y);
  y += TileHeight;

  Forecast *f = MultiMon::owmClient->getForecast();
  for (int i = 0; i < OWMClient::ForecastElements; i++) {
    // It's possible that the current temperature is higher than what was
    // forecast. If so, update the forecast with the known higher temp
    if (i == 0 && day(f[i].dt) == day(current.dt)) {
      if (f[i].hiTemp < current.hiTemp) {
        f[i].dt = current.dt;
        f[i].hiTemp = current.hiTemp;
        f[i].icon = current.icon;
      }
    }
    displaySingleForecast(&f[i], x, y);
    if (i == 1) x += TileWidth;
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
  if (!MultiMon::settings.use24Hour) {
    if (pm) h -= 12;
    if (h == 0) h = 12;
    reading += " " + String(h);
  } else {
    reading += " ";
    if (h < 10) reading += "0";
    reading += String(h);
  }

  x += tft.drawString(reading, x, y) + 1;
  if (MultiMon::settings.use24Hour) {
    reading = ":00";
  } else {
    reading = (pm ? "PM" : "AM");
  }
  tft.drawString(reading, x+1, y+1, TinyFont);
}
