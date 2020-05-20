/*
 * TimeScreen:
 *    This is the "home" screen. It displays the time, online line of wether,
 *    and a status overview for each printer. 
 *                    
 * TO DO:
 *
 * COMPLETE:
 * o Optimize the display function to avoid flashing the screen while also
 *   providing more frequent updates for active pritners. For example,
 *   use sprites, don't update unless things change, etc.
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <TimeLib.h>
//                                  Local Includes
#include "TimeScreen.h"
#include "../Basics.h"
#include "fonts/DSEG7_Classic_Bold_100.h"
//--------------- End:    Includes ---------------------------------------------

using GUI::tft;
using GUI::sprite;

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

static const auto WeatherFont = &FreeSansBold9pt7b;
static const uint16_t WeatherFontHeight = WeatherFont->yAdvance;
static const uint16_t WeatherXOrigin = 0;
static const uint16_t WeatherYOrigin = 0;
static const uint16_t WeatherHeight = WeatherFontHeight;
static const uint16_t WeatherWidth = Screen::Width;

// NC is short for Next Completion
static const auto NCFont = &FreeSansBold9pt7b;
static const uint16_t NCFontHeight = NCFont->yAdvance;
static const uint16_t NCXOrigin = 0;
static const uint16_t NCYOrigin = WeatherYOrigin + WeatherHeight + 2;
static const uint16_t NCHeight = NCFontHeight;
static const uint16_t NCWidth = Screen::Width;

// NOTE: The rightmost frame of ProgressBar[i] overlaps the leftmost frame
//       of ProgressBar[i+1]
static const uint16_t PB_FrameSize = 2;                             // Size of the Frame
static const uint16_t PB_Width = 81;                                // Includes Frame
static const uint16_t PB_Height = 42;                               // Includes Frame
static const uint16_t PB_BarWidth = PB_Width - (PB_FrameSize*2);    // Just the bar, no frame
static const uint16_t PB_BarHeight = PB_Height - (PB_FrameSize*2);  // Just the bar, no frame
static const uint16_t PB_XOrigin = 1;                               // X of origin of 1st progress bar
static const uint16_t PB_YOrigin = Screen::Height - PB_Height;      // Y Origin of all progress bars

static const uint16_t ClockXOrigin = 0;                       // Starts at left edge of screen
static const uint16_t ClockYOrigin = NCYOrigin + NCHeight;    // Starts below the NextCompletion area
static const uint16_t ClockWidth = Screen::Width;             // Full width of the screen
static const uint16_t ClockHeight = PB_YOrigin-ClockYOrigin;  // The space between the other 2 areas
static const auto ClockFont = &DSEG7_Classic_Bold_100;
static const uint16_t ClockFontHeight = ClockFont->yAdvance;

static const int WeatherAreaIndex = 4;
static const int ClockAreaIndex = 5;

/*------------------------------------------------------------------------------
 *
 * Constructors and Public methods
 *
 *----------------------------------------------------------------------------*/


TimeScreen::TimeScreen() {
  auto buttonHandler =[&](int id, PressType type) -> void {
    Log.verbose("In TimeScreen Button Handler, id = %d", id);
    if (id < MultiMon::MaxServers) {
      if (MultiMon::settings.printer[id].isActive &&
          MultiMon::printer[id]->getState() > PrintClient::State::Operational)
        GUI::displayDetailScreen(id);
      return;
    }
    if (type > NormalPress) { GUI::displayInfoScreen(); return; }
    if (id == ClockAreaIndex) { GUI::displayStatusScreen(); return; }
    if (id == WeatherAreaIndex) { GUI::displayWeatherScreen(); return; }
  };

  nButtons = MultiMon::MaxServers + 2;  // The weather area, the clock face, and the printer status areas
                                        // are each a button.
  buttons = new Button[nButtons];
  uint16_t x = PB_XOrigin;
  for (int i = 0; i < MultiMon::MaxServers; i++) {
    buttons[i].init(x, PB_YOrigin, PB_Width, PB_Height, buttonHandler, i);
    x += PB_Width - PB_FrameSize;
  }

  // Because the weather area is slim and at the top of the screen, we make a bigger button
  // right in the middle to make it easier to touch. It overlaps the Clock button area
  // but has priority as it is earlier in the button list
  buttons[WeatherAreaIndex].init(
    0, WeatherYOrigin, Screen::Width, /*WeatherHeight*/50,
    buttonHandler, WeatherAreaIndex);
  buttons[ClockAreaIndex].init(
    0, ClockYOrigin, Screen::Width, ClockHeight,
    buttonHandler, ClockAreaIndex);
}

void TimeScreen::display(bool activating) {
  if (activating) { tft.fillScreen(GUI::Color_Background); }

  drawClock(activating);
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

  if (!MultiMon::settings.use24Hour) {
    if (hr >= 12) { hr -= 12; }
    if (hr == 0) { hr = 12; }
  }

  timeString[0] = (hr < 10) ? ' ' : '1';
  timeString[1] = '0' + (hr % 10);
  timeString[2] = ':';
  timeString[3] = '0' + (min / 10);
  timeString[4] = '0' + (min % 10);
  timeString[5] = '\0';

  sprite->setColorDepth(1);
  sprite->createSprite(ClockWidth, ClockFontHeight);
  sprite->fillSprite(GUI::Mono_Background);

  sprite->setFreeFont(ClockFont);
  sprite->setTextColor(GUI::Mono_Foreground);
  // With this large font some manual "kerning" is required to make it fit
  uint16_t baseline = ClockFontHeight-1;
  sprite->setCursor(   0, baseline); sprite->print(timeString[0]);
  sprite->setCursor(  70, baseline); sprite->print(timeString[1]);
  sprite->setCursor( 145, baseline); sprite->print(timeString[2]);
  sprite->setCursor( 160, baseline); sprite->print(timeString[3]);
  sprite->setCursor( 230, baseline); sprite->print(timeString[4]);

  sprite->setBitmapColor(GUI::Color_AlertGood, GUI::Color_Background);
  uint16_t yPlacement = ClockYOrigin+((ClockHeight-ClockFontHeight)/2);
  yPlacement -= 5;  // Having it perfectly centered doesn't look as good,
                    // especially when no "next completion time" is displayed
  sprite->pushSprite(ClockXOrigin, yPlacement);
  sprite->deleteSprite();
}

void TimeScreen::drawProgressBar(int i, uint16_t barColor, uint16_t txtColor, float pct, String txt) {
  static const uint8_t BackgroundIndex = 0;
  static const uint8_t BarIndex = 1;
  static const uint8_t TextIndex = 2;
  static const uint8_t FrameIndex = 3;
  uint16_t cmap[16];
  cmap[BackgroundIndex] = GUI::Color_Background;
  cmap[BarIndex] = barColor;
  cmap[TextIndex] = txtColor;
  cmap[FrameIndex] = GUI::Color_Border;

  sprite->setColorDepth(4);
  sprite->createSprite(PB_Width, PB_Height);
  sprite->createPalette(cmap);
  sprite->fillSprite(BackgroundIndex);

  // Draw the frame
  for (int i = 0; i < PB_FrameSize; i++) {
    sprite->drawRect(0+i, 0+i, PB_Width-(2*i), PB_Height-(2*i), FrameIndex);
  }

  // Draw the bar
  sprite->fillRect(PB_FrameSize, PB_FrameSize, pct*PB_BarWidth, PB_BarHeight, BarIndex);

  // Draw the overlay text
  sprite->setFreeFont(&FreeSansBold9pt7b);
  sprite->setTextColor(TextIndex);
  sprite->setTextDatum(MC_DATUM);
  String note = (txt.isEmpty()) ? String((int)(pct*100)) + "%" : txt;
  sprite->drawString(note, PB_Width/2, PB_Height/2);

  // Push to the display and cleanup
  sprite->pushSprite(buttons[i]._x, buttons[i]._y);
  sprite->deleteSprite();
}

void TimeScreen::drawWeather(bool force) {
  if (!MultiMon::owmClient) { Log.verbose("owmClient = NULL"); return; }
  if (!MultiMon::settings.owm.enabled) return;
  String readout;

  sprite->setColorDepth(1);
  sprite->createSprite(WeatherWidth, WeatherHeight);
  sprite->fillSprite(GUI::Mono_Background);

  uint32_t textColor = GUI::Color_NormalText;
  if (MultiMon::owmClient->weather.dt == 0) {
    textColor = GUI::Color_AlertError;
    readout = "No Weather Data";
    return;
  } else {
    readout = MultiMon::owmClient->weather.location.city;
    readout += ": ";
    readout += String((int)(MultiMon::owmClient->weather.readings.temp+0.5));
    readout += (MultiMon::settings.useMetric) ? "C, " : "F, ";
    readout += MultiMon::owmClient->weather.description.longer;
  }
  sprite->setFreeFont(WeatherFont);
  sprite->setTextColor(GUI::Mono_Foreground);
  sprite->setTextDatum(MC_DATUM);
  sprite->drawString(readout, WeatherWidth/2, WeatherHeight/2);

  sprite->setBitmapColor(textColor, GUI::Color_Background);
  sprite->pushSprite(WeatherXOrigin, WeatherYOrigin);
  sprite->deleteSprite();
}

void TimeScreen::drawNextComplete(bool force) {
  sprite->setColorDepth(1);
  sprite->createSprite(NCWidth, NCHeight);
  sprite->fillSprite(GUI::Mono_Background);

  uint32_t minCompletion = UINT32_MAX;
  int printerWithNextCompletion;
  for (int i = 0; i < MultiMon::MaxServers; i++) {
    if (!MultiMon::settings.printer[i].isActive) continue;
    if (MultiMon::printer[i]->getState() == PrintClient::State::Printing) {
      uint32_t thisCompletion = MultiMon::printer[i]->getPrintTimeLeft();
      if (thisCompletion < minCompletion) {
        minCompletion = thisCompletion;
        printerWithNextCompletion = i;
      }
    }
  }

  if (minCompletion != UINT32_MAX) {
    PrintClient *printer = MultiMon::printer[printerWithNextCompletion];
    PrinterSettings *ps = &MultiMon::settings.printer[printerWithNextCompletion];
    time_t theTime = now() + printer->getPrintTimeLeft();

    String readout =  (ps->nickname.isEmpty()) ? readout = ps->server : ps->nickname;
    readout += ": ";
    readout += String(dayShortStr(weekday(theTime)));
    readout += " ";
    readout += (MultiMon::settings.use24Hour) ? hour(theTime) : hourFormat12(theTime);
    readout += ":";
    int theMinute =  minute(theTime);
    if (theMinute < 10) readout += '0';
    readout += theMinute;
    if (!MultiMon::settings.use24Hour) readout += isAM(theTime) ? "AM" : "PM";
    sprite->setFreeFont(NCFont);
    sprite->setTextColor(GUI::Mono_Foreground);
    sprite->setTextDatum(TC_DATUM);
    sprite->drawString(readout, NCWidth/2, 0);
  }

  sprite->setBitmapColor(GUI::Color_AlertGood, GUI::Color_Background);
  sprite->pushSprite(NCXOrigin, NCYOrigin);
  sprite->deleteSprite();

}

void TimeScreen::drawStatus(bool force) {
  for (int i = 0; i < MultiMon::MaxServers; i++) {
    PrintClient *printer = MultiMon::printer[i];

    if (!MultiMon::settings.printer[i].isActive) {
      drawProgressBar(i, GUI::Color_Inactive, GUI::Color_NormalText, 1.0, "Unused");
    } else {
      switch (MultiMon::printer[i]->getState()) {
        case PrintClient::State::Offline:
          drawProgressBar(i, GUI::Color_Offline, GUI::Color_NormalText, 1.0, "Offline");
          break;
        case PrintClient::State::Operational:
          drawProgressBar(i, GUI::Color_Online, GUI::Color_Background, 1.0, "Online");
          break;
        case PrintClient::State::Complete:
        case PrintClient::State::Printing:
          drawProgressBar(i, GUI::Color_Progress, GUI::Color_NormalText, printer->getPctComplete()/100.0);
          break;
      }
    }
  }
}
