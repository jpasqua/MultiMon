/*
 * TimeScreen:
 *    This is the "home" screen. It displays the time, one line line of
 *    weather data, and a status overview for each printer. 
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <TimeLib.h>
//                                  WebThing Includes
#include <GenericESP.h>
#include <WTBasics.h>
#include <gui/Display.h>
#include <gui/Theme.h>
#include <gui/ScreenMgr.h>
//                                  Local Includes
#include "../../MMDataSupplier.h"
#include "../../MultiMonApp.h"
#include "TimeScreen.h"
//--------------- End:    Includes ---------------------------------------------

using Display::tft;
using Display::sprite;

// ----- Coordinates of the various graphical elements
// The weather area is on top, the buttons are on the bottom,
// and the clock area is defined to be the space in between

/*
  ASCII art TimeScreen layout:

      +--------------------------------------------+
      |    City    Temp   Weather Description      |
      |   [Next Print completion time if avail]    |
      |              11      2222   33333          |
      |              11 ::  2   2        3         |
      |              11        2       333         |
      |              11 ::   2           3         |
      |              11    222222   33333          |
      | ------------------------------------------ |
      | ------------------------------------------ |
      | || Button || Button || Button || Button || |
      | ------------------------------------------ |
      | ------------------------------------------ |
      +--------------------------------------------+
*/

/*------------------------------------------------------------------------------
 *
 * CONSTANTS
 *
 *----------------------------------------------------------------------------*/

static constexpr auto WeatherFont = Display::Font::FontID::SB9;
static constexpr uint16_t WeatherFontHeight = 22;   // WeatherFont->yAdvance;
static constexpr uint16_t WeatherXOrigin = 0;
static constexpr uint16_t WeatherYOrigin = 0;
static constexpr uint16_t WeatherHeight = WeatherFontHeight;
static constexpr uint16_t WeatherWidth = Display::Width;

static int16_t PrinterNameFont = 2; // A small 5x7 font

// NC is short for Next Completion
static constexpr auto NCFont = Display::Font::FontID::SB9;
static constexpr uint16_t NCFontHeight = 22;      // NCFont->yAdvance;
static constexpr uint16_t NCXOrigin = 0;
static constexpr uint16_t NCYOrigin = WeatherYOrigin + WeatherHeight + 2;
static constexpr uint16_t NCHeight = NCFontHeight;
static constexpr uint16_t NCWidth = Display::Width;

// NOTE: The rightmost frame of ProgressBar[i] overlaps the leftmost frame
//       of ProgressBar[i+1]
static constexpr uint16_t PB_FrameSize = 2;                             // Size of the Frame
static constexpr uint16_t PB_Width = 81;                                // Includes Frame
static constexpr uint16_t PB_Height = 42;                               // Includes Frame
static constexpr uint16_t PB_BarWidth = PB_Width - (PB_FrameSize*2);    // Just the bar, no frame
static constexpr uint16_t PB_BarHeight = PB_Height - (PB_FrameSize*2);  // Just the bar, no frame
static constexpr uint16_t PB_XOrigin = 1;                               // X of origin of 1st progress bar
static constexpr uint16_t PB_YOrigin = Display::Height - PB_Height;      // Y Origin of all progress bars
static constexpr uint16_t PBLabelsYOrigin = PB_YOrigin-10;              // Space for teeny label + pad
static constexpr auto PB_Font = Display::Font::FontID::SB9;                 // Font for the Progress Bar

static constexpr uint16_t ClockXOrigin = 0;                             // Starts at left edge of screen
static constexpr uint16_t ClockYOrigin = NCYOrigin + NCHeight;          // Starts below the NextCompletion area
static constexpr uint16_t ClockWidth = Display::Width;                   // Full width of the screen
static constexpr uint16_t ClockHeight = PBLabelsYOrigin-ClockYOrigin;   // The space between the other 2 areas
static constexpr auto ClockFont = Display::Font::FontID::D100;
static constexpr uint16_t ClockFontHeight = 109;    // ClockFont->yAdvance;

static constexpr int WeatherAreaIndex = 4;
static constexpr int ClockAreaIndex = 5;

/*------------------------------------------------------------------------------
 *
 * Constructors and Public methods
 *
 *----------------------------------------------------------------------------*/


TimeScreen::TimeScreen() {

  auto buttonHandler =[this](int id, Button::PressType type) -> void {
    Log.verbose(F("In TimeScreen Button Handler, id = %d"), id);
    if (id < mmApp->MaxPrinters &&
        mmSettings->printer[id].isActive &&
        mmApp->printer[id]->getState() > PrintClient::State::Operational)
    {
      mmApp->detailScreen->setIndex(id);
      ScreenMgr::display(mmApp->detailScreen);
      return;
    }
    if (type > Button::PressType::NormalPress) {
      String subheading = "Heap: Free/Frag = ";
      String subcontent = String(ESP.getFreeHeap()) + ", " + String(GenericESP::getHeapFragmentation()) + "%"; 
      wtAppImpl->utilityScreen->setSub(subheading, subcontent);
      ScreenMgr::display(mmApp->utilityScreen);
      return;
    }
    if (id == ClockAreaIndex) { wtAppImpl->pluginMgr.displayPlugin(0); return; }
    if (id == WeatherAreaIndex) { ScreenMgr::display(mmApp->weatherScreen); return; }
  };

  nButtons = mmApp->MaxPrinters + 2;  // The weather area, the clock face, and the printer status areas
                                        // are each a button.
  buttons = new Button[nButtons];
  uint16_t x = PB_XOrigin;
  for (int i = 0; i < mmApp->MaxPrinters; i++) {
    buttons[i].init(x, PB_YOrigin, PB_Width, PB_Height, buttonHandler, i);
    x += PB_Width - PB_FrameSize;
  }

  // Because the weather area is slim and at the top of the screen, we make a bigger button
  // right in the middle to make it easier to touch. It overlaps the Clock button area
  // but has priority as it is earlier in the button list
  buttons[WeatherAreaIndex].init(
    0, WeatherYOrigin, Display::Width, /*WeatherHeight*/50,
    buttonHandler, WeatherAreaIndex);
  buttons[ClockAreaIndex].init(
    0, ClockYOrigin, Display::Width, ClockHeight,
    buttonHandler, ClockAreaIndex);
}

void TimeScreen::display(bool activating) {
  if (activating) { tft.fillScreen(Theme::Color_Background); }

  drawClock(activating);
  drawPrinterNames(activating);
  drawStatus(activating);
  drawWeather(activating);
  drawNextComplete(activating);
  nextUpdateTime = millis() + 10 * 1000L;
}

void TimeScreen::processPeriodicActivity() {
  if (millis() >= nextUpdateTime) display();
}

/*------------------------------------------------------------------------------
 *
 * Private methods
 *
 *----------------------------------------------------------------------------*/

void TimeScreen::drawClock(bool force) {
  static int lastTimeDisplayed = -1;
  time_t  t = now();
  int     hr = hour(t);
  int     min = minute(t);

  int compositeTime = hr * 100 + min;
  if (!force && (compositeTime == lastTimeDisplayed)) return;
  else lastTimeDisplayed = compositeTime;

  char timeString[6]; // HH:MM<NULL>

  if (!wtApp->settings->uiOptions.use24Hour) {
    if (hr >= 12) { hr -= 12; }
    if (hr == 0) { hr = 12; }
  }

  timeString[0] = (hr < 10) ? ' ' : (hr/10 + '0');
  timeString[1] = '0' + (hr % 10);
  timeString[2] = ':';
  timeString[3] = '0' + (min / 10);
  timeString[4] = '0' + (min % 10);
  timeString[5] = '\0';

  sprite->setColorDepth(1);
  sprite->createSprite(ClockWidth, ClockFontHeight);
  sprite->fillSprite(Theme::Mono_Background);

  Display::Font::setUsingID(ClockFont, sprite);
  sprite->setTextColor(Theme::Mono_Foreground);
  // With this large font some manual "kerning" is required to make it fit
  uint16_t baseline = ClockFontHeight-1;
  sprite->setCursor(   0, baseline); sprite->print(timeString[0]);
  sprite->setCursor(  70, baseline); sprite->print(timeString[1]);
  sprite->setCursor( 145, baseline); sprite->print(timeString[2]);
  sprite->setCursor( 160, baseline); sprite->print(timeString[3]);
  sprite->setCursor( 230, baseline); sprite->print(timeString[4]);

  sprite->setBitmapColor(Theme::Color_AlertGood, Theme::Color_Background);
  uint16_t yPlacement = ClockYOrigin+((ClockHeight-ClockFontHeight)/2);
  yPlacement -= 10; // Having it perfectly centered doesn't look as good,
                    // especially when no "next completion time" is displayed
  sprite->pushSprite(ClockXOrigin, yPlacement);
  sprite->deleteSprite();
}

void TimeScreen::drawProgressBar(int i, uint16_t barColor, uint16_t txtColor, float pct, String txt) {
  buttons[i].drawProgress(
        pct, txt, PB_Font, PB_FrameSize,
        txtColor, Theme::Color_Border, barColor, Theme::Color_Background,
        WTBasics::EmptyString, true);
}

void TimeScreen::drawWeather(bool force) {
  (void)force;  // We don't use this parameter. Avoid a warning...
  if (!wtApp->owmClient) { Log.verbose(F("owmClient = NULL")); return; }
  if (!wtApp->settings->owmOptions.enabled) return;
  String readout;

  sprite->setColorDepth(1);
  sprite->createSprite(WeatherWidth, WeatherHeight);
  sprite->fillSprite(Theme::Mono_Background);

  uint32_t textColor = Theme::Color_NormalText;
  if (wtApp->owmClient->weather.dt == 0) {
    textColor = Theme::Color_AlertError;
    readout = "No Weather Data";
    return;
  } else {
    if (wtApp->settings->owmOptions.nickname.isEmpty())
      readout = wtApp->owmClient->weather.location.city;
    else
      readout = wtApp->settings->owmOptions.nickname;
    readout += ": ";
    readout += String((int)(wtApp->owmClient->weather.readings.temp+0.5));
    readout += (wtApp->settings->uiOptions.useMetric) ? "C, " : "F, ";
    readout += wtApp->owmClient->weather.description.longer;
  }
  Display::Font::setUsingID(WeatherFont, sprite);
  sprite->setTextColor(Theme::Mono_Foreground);
  sprite->setTextDatum(MC_DATUM);
  sprite->drawString(readout, WeatherWidth/2, WeatherHeight/2);

  sprite->setBitmapColor(textColor, Theme::Color_Background);
  sprite->pushSprite(WeatherXOrigin, WeatherYOrigin);
  sprite->deleteSprite();
}

void TimeScreen::drawNextComplete(bool force) {
  (void)force;  // We don't use this parameter. Avoid a warning...

  sprite->setColorDepth(1);
  sprite->createSprite(NCWidth, NCHeight);
  sprite->fillSprite(Theme::Mono_Background);

  String printerName, formattedTime;
  uint32_t delta;
  MMDataSupplier::Printing::nextCompletion(printerName, formattedTime, delta);
  if (!printerName.isEmpty()) {
    Display::Font::setUsingID(NCFont, sprite);
    sprite->setTextColor(Theme::Mono_Foreground);
    sprite->setTextDatum(TC_DATUM);
    sprite->drawString(printerName +": " + formattedTime, NCWidth/2, 0);
  }

  uint16_t color = delta < (15*60) ? Theme::Color_AlertGood : Theme::Color_NormalText;
  sprite->setBitmapColor(color, Theme::Color_Background);
  sprite->pushSprite(NCXOrigin, NCYOrigin);
  sprite->deleteSprite();
}

void TimeScreen::drawPrinterNames(bool force) {
  (void)force;  // We don't use this parameter. Avoid a warning...
  uint16_t yPos = PB_YOrigin;
  uint16_t xDelta = Display::Width/mmApp->MaxPrinters;
  uint16_t xPos = 0 + xDelta/2;
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(Theme::Color_NormalText);
  for (int i = 0; i < mmApp->MaxPrinters; i++) {
    String name;
    if (!mmSettings->printer[i].isActive) name = "";
    else name = mmSettings->printer[i].nickname;
    tft.drawString(name, xPos, yPos, PrinterNameFont);
    xPos += xDelta;
  }
}

void TimeScreen::drawStatus(bool force) {
  (void)force;  // We don't use this parameter. Avoid a warning...
  for (int i = 0; i < mmApp->MaxPrinters; i++) {
    PrintClient *printer = mmApp->printer[i];

    if (!mmSettings->printer[i].isActive) {
      drawProgressBar(i, Theme::Color_Inactive, Theme::Color_NormalText, 1.0, "Unused");
    } else {
      switch (mmApp->printer[i]->getState()) {
        case PrintClient::State::Offline:
          drawProgressBar(i, Theme::Color_Offline, Theme::Color_NormalText, 1.0, "Offline");
          break;
        case PrintClient::State::Operational:
          drawProgressBar(i, Theme::Color_Online, Theme::Color_Background, 1.0, "Online");
          break;
        case PrintClient::State::Complete:
        case PrintClient::State::Printing:
          drawProgressBar(i, Theme::Color_Progress, Theme::Color_NormalText, printer->getPctComplete()/100.0);
          break;
      }
    }
  }
}
