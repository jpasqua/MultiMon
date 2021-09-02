/*
 * DetailsScreen:
 *    Display info about the current print. Things like file name, time
 *    remaining, etc.
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoLog.h>
#include <TimeLib.h>
//                                  WebThing Includes
#include <WebThing.h>
#include <gui/Display.h>
#include <gui/Theme.h>
#include <gui/ScreenMgr.h>
//                                  Local Includes
#include "DetailScreen.h"
#include "../clients/PrintClient.h"
#include "../../MultiMonApp.h"
#include "AppTheme.h"
//--------------- End:    Includes ---------------------------------------------


using Display::tft;
using Display::sprite;

/*------------------------------------------------------------------------------
 *
 * CONSTANTS
 *
 *----------------------------------------------------------------------------*/

static constexpr uint16_t TitleAreaYOrigin = 0;
static constexpr auto TitleFont = Display::Font::FontID::SB18;
static constexpr auto TitleFontHeight = 42;     // TitleFont->yAdvance;
static constexpr auto TitleAreaHeight = TitleFontHeight;

static constexpr uint16_t FileNameYOrigin = TitleAreaYOrigin + TitleAreaHeight;
static constexpr auto FileNameFont = Display::Font::FontID::SB9;
static constexpr auto FileNameFontHeight = 22;  // FileNameFont->yAdvance;

// The button that initiates scrolling of the file name covers both the title
// area and the file name area. The file name area by itself is too small.
static constexpr uint16_t FileNameButtonX = 0;
static constexpr uint16_t FileNameButtonY = 0;
static constexpr uint16_t FileNameButtonWidth = Display::Width;
static constexpr uint16_t FileNameButtonHeight = 64; // TitleAreaHeight+FileNameFontHeight;

static constexpr auto ProgressFont = Display::Font::FontID::SB18;
static constexpr uint16_t ProgressXInset = 4;
static constexpr uint16_t ProgressXOrigin = ProgressXInset;
static constexpr uint16_t ProgressYOrigin = 100;
static constexpr uint16_t ProgressHeight = 42;      // ProgressFont->yAdvance;
static constexpr uint16_t ProgressWidth = Display::Width - (2 * ProgressXInset);


static constexpr auto TimeFont = Display::Font::FontID::D20;
static constexpr uint16_t TimeYOrigin = ProgressYOrigin + ProgressHeight + 15;
static constexpr uint16_t TimeWidth = 100;
static constexpr uint16_t TimeHeight = 22;

static constexpr auto DetailFont = Display::Font::FontID::SB9;
static constexpr uint16_t DetailXInset = 10;
static constexpr uint16_t DetailYBottomMargin = 4;
static constexpr uint16_t DetailFontHeight = 22;    // DetailFont->yAdvance;
static constexpr uint16_t DetailWidth = Display::Width;
static constexpr uint16_t DetailHeight = 2 * DetailFontHeight;
static constexpr uint16_t DetailXOrigin = 0;
static constexpr uint16_t DetailYOrigin = Display::Height - DetailHeight - DetailYBottomMargin;

static constexpr uint8_t FileNameButtonID = 0;
static constexpr uint8_t FullScreenButtonID = 1;

/*------------------------------------------------------------------------------
 *
 * Constructors and Public methods
 *
 *----------------------------------------------------------------------------*/

DetailScreen::DetailScreen() {
  auto buttonHandler =[&](int id, Button::PressType type) -> void {
    Log.verbose(F("In DetailScreen ButtonHandler, id = %d"), id);
    if (id == FileNameButtonID) {  // The file name was tapped
      revealFullFileName();
      return;
    }

    PrintClient *p = mmApp->printer[index];
    if (type > Button::PressType::NormalPress && p->getState() == PrintClient::State::Complete) {
      p->acknowledgeCompletion();
    }
    ScreenMgr::displayHomeScreen();
  };

  buttons = new Button[(nButtons = 2)];
  buttons[0].init(
      FileNameButtonX, FileNameButtonY, FileNameButtonWidth, FileNameButtonHeight,
      buttonHandler, FileNameButtonID);
  buttons[1].init(0, 0, Display::Width, Display::Height, buttonHandler, FullScreenButtonID);
}

void DetailScreen::setIndex(int i) { index = i; }

void DetailScreen::display(bool activating) {
  PrintClient *printer = mmApp->printer[index];

  if (activating) {
    scrollIndex = -1; // We're doing an inital display, so we aren't scrolling
    tft.fillScreen(Theme::Color_Background);
    drawStaticContent(printer, activating);
  }

  drawProgressBar(ProgressXOrigin, ProgressYOrigin, ProgressWidth, ProgressHeight,
    printer->getPctComplete(),
    WebThing::formattedInterval(printer->getPrintTimeLeft()),
    activating);
  drawDetailInfo(printer, activating);
  drawTime(activating);

  nextUpdateTime = millis() + (10 * 1000L);
}

void DetailScreen::processPeriodicActivity() {
  if (scrollIndex != -1 && (millis() >= nextScrollTime)) {
    scrollFileName();
  }
  if (millis() >= nextUpdateTime) { display();  }
  else { drawTime(false); }
}


/*------------------------------------------------------------------------------
 *
 * Private methods
 *
 *----------------------------------------------------------------------------*/

inline void DetailScreen::appendDate(time_t theTime, String &target) {
  target += String(dayShortStr(weekday(theTime)));
  target += " ";
  target += (wtApp->settings->uiOptions.use24Hour) ? hour(theTime) : hourFormat12(theTime);
  target += ":";
  int theMinute =  minute(theTime);
  if (theMinute < 10) target += '0';
  target += theMinute;
  if (!wtApp->settings->uiOptions.use24Hour) target += isAM(theTime) ? "AM" : "PM";
}

void DetailScreen::drawStaticContent(PrintClient *printer, bool force) {
  (void)force;  // We don't use this parameter. Avoid a warning...

  // ----- Display the nickname
  tft.setTextDatum(TC_DATUM);
  Display::Font::setUsingID(TitleFont, tft);
  tft.setTextColor(AppTheme::Color_Nickname);
  tft.drawString(mmSettings->printer[index].nickname, Display::XCenter, 5);

  String name = printer->getFilename();
  Display::Font::setUsingID(DetailFont, tft); // Set font BEFORE measuring width
  nameWidth = tft.textWidth(name);        // Remember width in case we need to scroll
  tft.setTextColor(Theme::Color_DimText);
  if (nameWidth < Display::Width)  {
    tft.setTextDatum(TC_DATUM);
    tft.drawString(name, Display::XCenter, FileNameYOrigin);
  } else {
    tft.setTextDatum(TL_DATUM);
    tft.drawString(name, 0, FileNameYOrigin);
  }
}

void DetailScreen::drawTime(bool force) {
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
  sprite->createSprite(TimeWidth, TimeHeight);
  sprite->fillSprite(Theme::Mono_Background);

  Display::Font::setUsingID(TimeFont, sprite);
  sprite->setTextColor(Theme::Mono_Foreground);
  sprite->setTextDatum(TC_DATUM);
  sprite->drawString(timeString, TimeWidth/2, 0);

  sprite->setBitmapColor(AppTheme::Color_Nickname, Theme::Color_Background);
  sprite->pushSprite((Display::Width-TimeWidth)/2, TimeYOrigin);
  sprite->deleteSprite();
}

void DetailScreen::drawProgressBar(
    uint16_t x, uint16_t y, uint16_t w, uint16_t h, float pct, String txt, bool force) {
  static float  lastPct = -1;
  static String lastTxt = "";

  if (pct == 100.0f && lastPct != 100.0f) force = true; // Special case, we want 100% at the end
  if ((pct - lastPct < 1) && (txt == lastTxt) && !force) return;
  lastPct = pct;
  lastTxt = txt;

  constexpr uint8_t BackgroundIndex = 0;
  constexpr uint8_t BarIndex = 1;
  constexpr uint8_t TextIndex = 2;
  constexpr uint8_t BorderIndex = 3;
  uint16_t cmap[16];
  cmap[BackgroundIndex] = Theme::Color_Background;
  cmap[BarIndex] = Theme::Color_Progress;
  cmap[TextIndex] = Theme::Color_NormalText;
  cmap[BorderIndex] = Theme::Color_Border;

  sprite->setColorDepth(4);
  sprite->createSprite(w, h);
  sprite->createPalette(cmap);
  sprite->fillSprite(BackgroundIndex);

  sprite->drawRect(0, 0, w, h, BorderIndex);
  int bw = (pct/100)*(w-2);
  sprite->fillRect(1, 1, bw, (h-2), BarIndex);

  constexpr uint16_t PctXInset = 10;
  constexpr uint16_t TxtXInset = 5;
  Display::Font::setUsingID(ProgressFont, sprite);
  sprite->setTextColor(TextIndex);
  sprite->setTextDatum(ML_DATUM);
  sprite->drawString(String((int)(pct)) + "%", PctXInset, (h/2));
  sprite->setTextDatum(MR_DATUM);
  sprite->drawString(txt, w-TxtXInset, (h/2));
  sprite->pushSprite(x, y);

  sprite->deleteSprite();
}

void DetailScreen::drawDetailInfo(PrintClient *printer, bool force) {
  (void)force;  // We don't use this parameter. Avoid a warning...

  sprite->setColorDepth(1);
  sprite->createSprite(Display::Width, DetailHeight);
  sprite->fillSprite(Theme::Mono_Background);
  Display::Font::setUsingID(DetailFont, sprite);
  sprite->setTextColor(Theme::Mono_Foreground);
  sprite->setTextDatum(TL_DATUM);

  // ----- Display the temps
  float target, actual;
  printer->getBedTemps(actual, target);
  String temp = "Bed: " + String(actual, 1) + " / " + String(target, 1);
  sprite->drawString(temp, DetailXInset, 0);

  printer->getToolTemps(actual, target);
  temp = "E0: " + String(actual, 1) + " / " + String(target, 1);
  sprite->drawString(temp, Display::XCenter+DetailXInset, 0);

  // ----- Display the elapsed Time
  Display::Font::setUsingID(DetailFont, sprite);
  sprite->setTextDatum(TL_DATUM);
  sprite->setTextColor(Theme::Color_NormalText);
  String elapsed = "Done: " + WebThing::formattedInterval(printer->getElapsedTime());
  sprite->drawString(elapsed, DetailXInset, DetailFontHeight);

  // ----- Display expected completion time
  String est = "Complete";
  if (printer->getState() == PrintClient::State::Printing) {
    est = "Est: ";
    appendDate(now() + printer->getPrintTimeLeft(), est);
  }
  sprite->drawString(est, Display::XCenter+DetailXInset, DetailFontHeight);

  sprite->setBitmapColor(Theme::Color_NormalText, Theme::Color_Background);
  sprite->pushSprite(DetailXOrigin, DetailYOrigin);
  sprite->deleteSprite();
}

void DetailScreen::scrollFileName() {
  sprite->setColorDepth(1);
  sprite->createSprite(Display::Width, DetailHeight);
  sprite->fillSprite(Theme::Mono_Background);
  Display::Font::setUsingID(DetailFont, sprite);
  sprite->setTextColor(Theme::Mono_Foreground);
  sprite->setTextDatum(TL_DATUM);

  uint32_t extraDelay = 0;
  String name = mmApp->printer[index]->getFilename();
  if (scrollIndex == nameWidth - Display::Width) { delta = -delta; extraDelay = 500; }
  sprite->drawString(name, -scrollIndex, 0);
  sprite->setBitmapColor(Theme::Color_DimText, Theme::Color_Background);
  sprite->pushSprite(0, FileNameYOrigin);
  sprite->deleteSprite();

  nextScrollTime = millis() + 10 + extraDelay;
  scrollIndex = scrollIndex + delta;
}

void DetailScreen::revealFullFileName() {
  if (nameWidth <= Display::Width) return; // It's already revealed
  if (scrollIndex != -1) {  // We're already scrolling, finish
    scrollIndex = 0;
    delta = -1;
  } else {
    scrollIndex = 1;
    delta = 1;    
  }
}





