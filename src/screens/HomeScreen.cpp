/*
 * HomeScreen:
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
#include <BPABasics.h>
#include <gui/Display.h>
#include <gui/Theme.h>
#include <gui/ScreenMgr.h>
//                                  Local Includes
#include "../../MMDataSupplier.h"
#include "../../MultiMonApp.h"
#include "HomeScreen.h"
//--------------- End:    Includes ---------------------------------------------


// ----- Coordinates of the various graphical elements
// The weather area is on top, the labels are on the bottom,
// and the clock area is defined to be the space in between

/*
  ASCII art HomeScreen layout:

      +--------------------------------------------+
      |    City    Temp   Weather Description      |
      |  [Next print completion time | forecast]   |
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

static constexpr auto WeatherFont = Display.FontID::SB9;
static constexpr uint16_t WeatherFontHeight = 22;   // WeatherFont->yAdvance;
static constexpr uint16_t WeatherXOrigin = 0;
static constexpr uint16_t WeatherYOrigin = 0;
static constexpr uint16_t WeatherHeight = WeatherFontHeight;
static constexpr uint16_t WeatherWidth = Display.Width;

static int16_t PrinterNameFont = 2; // A small 5x7 font

// NC is short for Next Completion
static constexpr auto NCFont = Display.FontID::SB9;
static constexpr uint16_t NCFontHeight = 22;      // NCFont->yAdvance;
static constexpr uint16_t NCXOrigin = 0;
static constexpr uint16_t NCYOrigin = WeatherYOrigin + WeatherHeight + 2;
static constexpr uint16_t NCHeight = NCFontHeight;
static constexpr uint16_t NCWidth = Display.Width;

// NOTE: The rightmost frame of ProgressBar[i] overlaps the leftmost frame
//       of ProgressBar[i+1]
static constexpr uint16_t PB_FrameSize = 2;                             // Size of the Frame
static constexpr uint16_t PB_Width = 81;                                // Includes Frame
static constexpr uint16_t PB_Height = 42;                               // Includes Frame
static constexpr uint16_t PB_BarWidth = PB_Width - (PB_FrameSize*2);    // Just the bar, no frame
static constexpr uint16_t PB_BarHeight = PB_Height - (PB_FrameSize*2);  // Just the bar, no frame
static constexpr uint16_t PB_XOrigin = 1;                               // X of origin of 1st progress bar
static constexpr uint16_t PB_YOrigin = Display.Height - PB_Height;      // Y Origin of all progress bars
static constexpr uint16_t PBLabelsYOrigin = PB_YOrigin-10;              // Space for teeny label + pad
static constexpr auto     PB_Font = Display.FontID::SB9;                // Font for the Progress Bar

static constexpr uint16_t ClockXOrigin = 0;                             // Starts at left edge of screen
static constexpr uint16_t ClockYOrigin = NCYOrigin + NCHeight;          // Starts below the NextCompletion area
static constexpr uint16_t ClockWidth = Display.Width;                   // Full width of the screen
static constexpr uint16_t ClockHeight = PBLabelsYOrigin-ClockYOrigin;   // The space between the other 2 areas
static constexpr auto     ClockFont = Display.FontID::D100;
static constexpr uint16_t ClockFontHeight = 109;    // ClockFont->yAdvance;

static constexpr uint8_t FirstProgressBar = 0;
static constexpr uint8_t N_ProgressBars   = 4;
static constexpr uint8_t WeatherAreaLabel = FirstProgressBar + N_ProgressBars;
static constexpr uint8_t ClockAreaLabel   = WeatherAreaLabel + 1;
static constexpr uint8_t N_Labels         = ClockAreaLabel + 1;

/*------------------------------------------------------------------------------
 *
 * Constructors and Public methods
 *
 *----------------------------------------------------------------------------*/


HomeScreen::HomeScreen() {

  buttonHandler = [this](uint8_t id, PressType type) -> void {
    Log.verbose(F("In HomeScreen Button Handler, id = %d"), id);
    if (id < mmApp->MaxPrinters &&
        mmSettings->printer[id].isActive &&
        mmApp->printer[id]->getState() > PrintClient::State::Operational)
    {
      mmApp->detailScreen->setIndex(id);
      ScreenMgr.display(mmApp->detailScreen);
      return;
    }
    if (type > PressType::Normal) {
      String subheading = "Heap: Free/Frag = ";
      String subcontent = String(ESP.getFreeHeap()) + ", " + String(GenericESP::getHeapFragmentation()) + "%"; 
      wtAppImpl->screens.utilityScreen->setSub(subheading, subcontent);
      ScreenMgr.display(wtAppImpl->screens.utilityScreen);
      return;
    }
    if (id == ClockAreaLabel) { wtAppImpl->pluginMgr.displayPlugin(0); return; }
    if (id == WeatherAreaLabel) {
      if (wtApp->owmClient && wtApp->settings->owmOptions.enabled) {
        ScreenMgr.display(mmApp->screens.weatherScreen);
      }
      return;
    }
  };

  labels = new Label[(nLabels = N_Labels)];

  Region r {PB_XOrigin, PB_YOrigin, PB_Width, PB_Height};
  for (int i = 0; i < mmApp->MaxPrinters; i++) {
    labels[i].init(r, i);
    r.x += PB_Width - PB_FrameSize;
  }

  // Because the weather area is slim and at the top of the screen, we make a bigger button
  // right in the middle to make it easier to touch. It overlaps the Clock button area
  // but has priority as it is earlier in the button list
  labels[WeatherAreaLabel].init(0, WeatherYOrigin, Display.Width, 50, WeatherAreaLabel);
  labels[ClockAreaLabel].init(0, ClockYOrigin, Display.Width, ClockHeight, ClockAreaLabel);
}

void HomeScreen::display(bool activating) {
  if (activating) { Display.tft.fillScreen(Theme::Color_Background); }

  drawClock(activating);
  drawPrinterNames(activating);
  drawStatus(activating);
  drawWeather(activating);
  drawSecondLine(activating);
  nextUpdateTime = millis() + 10 * 1000L;
}

void HomeScreen::processPeriodicActivity() {
  if (millis() >= nextUpdateTime) display();
}

/*------------------------------------------------------------------------------
 *
 * Private methods
 *
 *----------------------------------------------------------------------------*/

void HomeScreen::drawClock(bool force) {
  static int lastTimeDisplayed = -1;
  time_t  t = now();
  int     hr = hour(t);
  int     min = minute(t);
  auto&   sprite = Display.sprite;

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

  Display.setSpriteFont(ClockFont);
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

void HomeScreen::drawProgressBar(
    int i, uint16_t barColor, uint16_t txtColor,
    float pct, String txt, bool showPct) {
  labels[i].drawProgress(
        pct, txt, PB_Font, PB_FrameSize,
        txtColor, Theme::Color_Border, barColor, Theme::Color_Background,
        showPct, true);
}

void HomeScreen::drawWeather(bool) {
  String readout("No weather data available");
  auto& sprite = Display.sprite;

  sprite->setColorDepth(1);
  sprite->createSprite(WeatherWidth, WeatherHeight);
  sprite->fillSprite(Theme::Mono_Background);

  uint32_t textColor = Theme::Color_NormalText;
  if (wtApp->owmClient && wtApp->settings->owmOptions.enabled) {
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
  }

  Display.setSpriteFont(WeatherFont);
  sprite->setTextColor(Theme::Mono_Foreground);
  sprite->setTextDatum(MC_DATUM);
  sprite->drawString(readout, WeatherWidth/2, WeatherHeight/2);

  sprite->setBitmapColor(textColor, Theme::Color_Background);
  sprite->pushSprite(WeatherXOrigin, WeatherYOrigin);
  sprite->deleteSprite();
}

void HomeScreen::drawSecondLine(bool) {
  auto& sprite = Display.sprite;

  sprite->setColorDepth(1);
  sprite->createSprite(NCWidth, NCHeight);
  sprite->fillSprite(Theme::Mono_Background);
  sprite->setTextColor(Theme::Mono_Foreground);
  sprite->setTextDatum(TC_DATUM);
  Display.setSpriteFont(NCFont);

  uint16_t textColor = Theme::Color_NormalText;

  String printerName, formattedTime;
  uint32_t delta;
  MMDataSupplier::Printing::nextCompletion(printerName, formattedTime, delta);
  String text;
  if (printerName.isEmpty()) {
    // Nothing to display, so show the forecast if available
    if (wtApp->owmClient && wtApp->settings->owmOptions.enabled) {
      Forecast *f = wtApp->owmClient->getForecast();
      text.reserve(20);
      text = "High: ";
      text += String(f[0].hiTemp, 0);
      text += ", Low: ";
      text += String(f[0].loTemp, 0);
    }
  } else {
    text.reserve(60);
    text = printerName;
    text += ": ";
    delta /= 60;
    if (delta <= 15) {
      textColor = Theme::Color_AlertGood;
      text += ((delta == 0) ? 1 : delta);
      text += " minute";
      if (delta != 1) text += 's';
    } else {
      text += formattedTime;
    }
  }

  sprite->drawString(text, NCWidth/2, 0);
  sprite->setBitmapColor(textColor, Theme::Color_Background);
  sprite->pushSprite(NCXOrigin, NCYOrigin);
  sprite->deleteSprite();

}

void HomeScreen::drawPrinterNames(bool) {
  auto& tft = Display.tft;
  uint16_t yPos = PB_YOrigin;
  uint16_t xDelta = Display.Width/mmApp->MaxPrinters;
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

void HomeScreen::drawStatus(bool force) {
  (void)force;  // We don't use this parameter. Avoid a warning...
  for (int i = 0; i < mmApp->MaxPrinters; i++) {
    PrintClient *printer = mmApp->printer[i];

    if (!mmSettings->printer[i].isActive) {
      drawProgressBar(i, Theme::Color_Inactive, Theme::Color_NormalText, 1.0, "Unused", false);
    } else {
      switch (mmApp->printer[i]->getState()) {
        case PrintClient::State::Offline:
          drawProgressBar(i, Theme::Color_Offline, Theme::Color_NormalText, 1.0, "Offline", false);
          break;
        case PrintClient::State::Operational:
          drawProgressBar(i, Theme::Color_Online, Theme::Color_Background, 1.0, "Online", false);
          break;
        case PrintClient::State::Complete:
        case PrintClient::State::Printing:
          drawProgressBar(
              i, Theme::Color_Progress, Theme::Color_NormalText,
              printer->getPctComplete()/100.0, "", true);
          break;
      }
    }
  }
}
