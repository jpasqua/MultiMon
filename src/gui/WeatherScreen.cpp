/*
 * WeatherScreen:
 *    Display weather info for the selected city and a running clock. 
 *                    
 * TO DO:
 *
 * COMPLETE:
 * o Optimize the clock display so it doesn't flash during updates.
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
//                                  Local Includes
#include "WeatherScreen.h"
#include "images/Icons.h"
#include "../Basics.h"
//--------------- End:    Includes ---------------------------------------------

using GUI::tft;
using GUI::sprite;

/*------------------------------------------------------------------------------
 *
 * CONSTANTS
 *
 *----------------------------------------------------------------------------*/

static const auto TimeFont = GUI::Font::FontID::D20;
static const uint16_t TimeFontWidth = 17;
static const uint16_t TimeFontHeight = 22;
static const uint16_t TimeFontColonWidth = 9;

static const auto TempUnitsFont = TimeFont;
static const uint16_t TempUnitsFontWidth = TimeFontWidth;
static const uint16_t TempUnitsFontHeight = TimeFontHeight;

static const auto TempFont = GUI::Font::FontID::D72;
static const uint16_t TempFontWidth = 60;
static const uint16_t TempFontHeight = 72;

static const auto ReadingsFont = GUI::Font::FontID::S12;
static const uint16_t ReadingsFontHeight = 29;


static const uint16_t YCentralArea = 50;
static const uint16_t ImageInset = 2;
static const uint16_t YTopArea = 3;
static const uint16_t XTopAreaInset = 3;
static const uint16_t YTopAreaHeight = 22;
static const uint16_t YReadingsArea = Screen::Height - 2*ReadingsFontHeight;

static const uint16_t TimeDisplayWidth = 6*TimeFontWidth + 2*TimeFontColonWidth;


/*------------------------------------------------------------------------------
 *
 * Constructors and Public methods
 *
 *----------------------------------------------------------------------------*/

WeatherScreen::WeatherScreen() {
  auto buttonHandler =[&](int id, Button::PressType type) -> void {
    Log.verbose(F("In WeatherScreen Button Handler, id = %d, type = %d"), id, type);
    if (type > Button::PressType::NormalPress) GUI::displayHomeScreen();
    else GUI::displayForecastScreen();
  };

  nButtons = 1;
  buttons = new Button[nButtons];
  buttons[0].init(0, 0, Screen::Width, Screen::Height, buttonHandler, 0);
}


void WeatherScreen::display(bool activating) {
  (void)activating; // We don't use this parameter. Avoid a warning...
  
  // We should not get here if owm is disabled or we were unable to contact the
  // OWM service, but just double check...
  if (!MultiMon::settings.owm.enabled || !MultiMon::owmClient) return;

  bool useMetric = MultiMon::settings.useMetric;

  tft.fillScreen(GUI::Color_WeatherBkg);

  if (MultiMon::owmClient->weather.dt == 0) {
    GUI::Font::setUsingID(GUI::Font::FontID::SB18, tft);
    tft.setTextColor(GUI::Color_AlertError);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("No Weather Data", Screen::XCenter, Screen::YCenter);
    return;
  }

  // ----- Draw Summary line at the top of the screen
  GUI::Font::setUsingID(GUI::Font::FontID::SB12, tft);
  tft.setTextColor(GUI::Color_WeatherTxt);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(MultiMon::owmClient->weather.location.city, XTopAreaInset, YTopArea);
  showTime();

  // ----- Draw the central display area
  float temp = MultiMon::owmClient->weather.readings.temp;

  tft.pushImage(ImageInset, YCentralArea, WI_Width, WI_Height,
      getWeatherIcon(MultiMon::owmClient->weather.description.icon), WI_Transparent);
  tft.pushImage(Screen::Width-WI_Width-ImageInset, YCentralArea, WindIcon_Width, WindIcon_Height,
      getWindIcon(MultiMon::owmClient->weather.readings.windSpeed), WI_Transparent);

  int textOffset = (WindIcon_Height-TempFontHeight)/2;
  GUI::Font::setUsingID(TempFont, tft);
  tft.setTextColor(GUI::Color_Progress);
  tft.setTextDatum(TL_DATUM);
  int nDigits = temp < 10 ? 1 : (temp < 100 ? 2 : 3);
  int xLoc = Screen::XCenter - ((nDigits*TempFontWidth+TempUnitsFontWidth)/2);
  tft.drawString(String((int)temp), xLoc, YCentralArea-textOffset);
  GUI::Font::setUsingID(TimeFont, tft);
  tft.drawString(
      (useMetric ? "C" : "F"),
      xLoc+(nDigits*TempFontWidth),
      YCentralArea-textOffset+TempFontHeight-TempUnitsFontHeight);

  tft.setTextDatum(TC_DATUM);
  char firstChar = MultiMon::owmClient->weather.description.longer[0];
  if (isLowerCase(firstChar)) {
    MultiMon::owmClient->weather.description.longer[0] = toUpperCase(firstChar);
  }
  tft.setTextColor(GUI::Color_WeatherTxt);
  GUI::Font::setUsingID(ReadingsFont, tft);
  tft.drawString(
      MultiMon::owmClient->weather.description.longer,
      Screen::XCenter,YCentralArea-textOffset+TempFontHeight + 5); // A little spacing in Y

  // Readings Area
  tft.setTextColor(GUI::Color_WeatherTxt);
  GUI::Font::setUsingID(ReadingsFont, tft);
  tft.setTextDatum(TL_DATUM);
  String reading = "Humidty: " + String(MultiMon::owmClient->weather.readings.humidity) + "%";
  tft.drawString(reading, XTopAreaInset, YReadingsArea);
  tft.setTextDatum(TR_DATUM);
  float pressure = MultiMon::owmClient->weather.readings.pressure;
  if (!useMetric) pressure = Basics::hpa_to_inhg(pressure);
  reading = String(pressure) +  (useMetric ? "hPa" : "inHG"),
  tft.drawString(reading, Screen::Width - XTopAreaInset, YReadingsArea);

  // NOTE: For some reason visibility seems to ignore the units setting and always return meters!!
  uint16_t visibility = (MultiMon::owmClient->weather.readings.visibility);
  String units = "km";
  if (useMetric) { visibility /= 1000;  }
  else {
    units = "mi";
    visibility = (uint16_t)Basics::km_to_m( ((float)(visibility+500)) / 1000.0);
  }
  reading = "Visibility: " + String(visibility) +  units;
  tft.setTextDatum(TL_DATUM);
  tft.drawString(reading, XTopAreaInset, YReadingsArea+ReadingsFontHeight);

  tft.setTextDatum(TR_DATUM);
  float feelsLike = MultiMon::owmClient->weather.readings.feelsLike;

  units = useMetric ? "C" : "F";
  reading = "Feels " + String((int)(feelsLike+0.5)) +  units;
  tft.setTextColor(GUI::Color_Progress);
  GUI::Font::setUsingID(GUI::Font::FontID::SB12, tft);
  tft.drawString(reading, Screen::Width - XTopAreaInset, YReadingsArea+ReadingsFontHeight);

  lastDT = MultiMon::owmClient->weather.dt;
}

void WeatherScreen::processPeriodicActivity() {
  if (lastDT != MultiMon::owmClient->weather.dt) { display(); return; }
  if (millis() - lastClockUpdate >= 1000L) { showTime(); return; }
}


/*------------------------------------------------------------------------------
 *
 * Private methods
 *
 *----------------------------------------------------------------------------*/

void WeatherScreen::showTime() {
  sprite->setColorDepth(1);
  sprite->createSprite(TimeDisplayWidth, TimeFontHeight);
  sprite->fillSprite(GUI::Mono_Background);

  lastClockUpdate = millis();

  GUI::Font::setUsingID(TimeFont, sprite);
  sprite->setTextColor(GUI::Mono_Foreground);
  sprite->setTextDatum(TR_DATUM);
  sprite->drawString(
      WebThing::formattedTime(MultiMon::settings.use24Hour, false),
      TimeDisplayWidth-1-XTopAreaInset, 0);

  sprite->setBitmapColor(GUI::Color_WeatherTxt, GUI::Color_WeatherBkg);
  sprite->pushSprite(Screen::Width - TimeDisplayWidth, YTopArea);
  sprite->deleteSprite();
}








