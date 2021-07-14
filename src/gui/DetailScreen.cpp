/*
 * DetailsScreen:
 *    Display info about the current print. Things like file name, time
 *    remaining, etc.
 *                    
 * TO DO:
 *
 * COMPLETE
 * o Optimize repaints
 *   + Use sprites, don't redraw static content, etc.
 * o Show the expected time that the print will complete.
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
//                                  Local Includes
#include "DetailScreen.h"
//--------------- End:    Includes ---------------------------------------------

using GUI::tft;
using GUI::sprite;


/*------------------------------------------------------------------------------
 *
 * CONSTANTS
 *
 *----------------------------------------------------------------------------*/

static const uint16_t TitleAreaYOrigin = 0;
static const auto TitleFont = GUI::Font::FontID::SB18;
static const auto TitleFontHeight = 42;     // TitleFont->yAdvance;
static const auto TitleAreaHeight = TitleFontHeight;

static const uint16_t FileNameYOrigin = TitleAreaYOrigin + TitleAreaHeight;
static const auto FileNameFont = GUI::Font::FontID::SB9;
static const auto FileNameFontHeight = 22;  // FileNameFont->yAdvance;

// The button that initiates scrolling of the file name covers both the title
// area and the file name area. The file name area by itself is too small.
static const uint16_t FileNameButtonX = 0;
static const uint16_t FileNameButtonY = 0;
static const uint16_t FileNameButtonWidth = Screen::Width;
static const uint16_t FileNameButtonHeight = 64; // TitleAreaHeight+FileNameFontHeight;

static const auto ProgressFont = GUI::Font::FontID::SB18;
static const uint16_t ProgressXInset = 4;
static const uint16_t ProgressXOrigin = ProgressXInset;
static const uint16_t ProgressYOrigin = 100;
static const uint16_t ProgressHeight = 42;      // ProgressFont->yAdvance;
static const uint16_t ProgressWidth = Screen::Width - (2 * ProgressXInset);


static const auto TimeFont = GUI::Font::FontID::D20;
static const uint16_t TimeYOrigin = ProgressYOrigin + ProgressHeight + 15;
static const uint16_t TimeWidth = 100;
static const uint16_t TimeHeight = 22;

static const auto DetailFont = GUI::Font::FontID::SB9;
static const uint16_t DetailXInset = 10;
static const uint16_t DetailYBottomMargin = 4;
static const uint16_t DetailFontHeight = 22;    // DetailFont->yAdvance;
static const uint16_t DetailWidth = Screen::Width;
static const uint16_t DetailHeight = 2 * DetailFontHeight;
static const uint16_t DetailXOrigin = 0;
static const uint16_t DetailYOrigin = Screen::Height - DetailHeight - DetailYBottomMargin;

static const uint8_t FileNameButtonID = 0;
static const uint8_t FullScreenButtonID = 1;

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

    PrintClient *p = MultiMon::printer[index];
    if (type > Button::PressType::NormalPress && p->getState() == PrintClient::State::Complete) {
      p->acknowledgeCompletion();
    }
    GUI::displayHomeScreen();
  };

  buttons = new Button[(nButtons = 2)];
  buttons[0].init(
      FileNameButtonX, FileNameButtonY, FileNameButtonWidth, FileNameButtonHeight,
      buttonHandler, FileNameButtonID);
  buttons[1].init(0, 0, Screen::Width, Screen::Height, buttonHandler, FullScreenButtonID);
}

void DetailScreen::setIndex(int i) { index = i; }

void DetailScreen::display(bool activating) {
  PrintClient *printer = MultiMon::printer[index];

  if (activating) {
    scrollIndex = -1; // We're doing an inital display, so we aren't scrolling
    tft.fillScreen(GUI::Color_Background);
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

inline void appendDate(time_t theTime, String &target) {
  target += String(dayShortStr(weekday(theTime)));
  target += " ";
  target += (MultiMon::settings.use24Hour) ? hour(theTime) : hourFormat12(theTime);
  target += ":";
  int theMinute =  minute(theTime);
  if (theMinute < 10) target += '0';
  target += theMinute;
  if (!MultiMon::settings.use24Hour) target += isAM(theTime) ? "AM" : "PM";
}

void DetailScreen::drawStaticContent(PrintClient *printer, bool force) {
  (void)force;  // We don't use this parameter. Avoid a warning...

  // ----- Display the nickname
  tft.setTextDatum(TC_DATUM);
  GUI::Font::setUsingID(TitleFont, tft);
  tft.setTextColor(GUI::Color_Nickname);
  tft.drawString(MultiMon::settings.printer[index].nickname, Screen::XCenter, 5);

  String name = printer->getFilename();
  GUI::Font::setUsingID(DetailFont, tft); // Set font BEFORE measuring width
  nameWidth = tft.textWidth(name);        // Remember width in case we need to scroll
  tft.setTextColor(GUI::Color_DimText);
  if (nameWidth < Screen::Width)  {
    tft.setTextDatum(TC_DATUM);
    tft.drawString(name, Screen::XCenter, FileNameYOrigin);
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

  if (!MultiMon::settings.use24Hour) {
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
  sprite->fillSprite(GUI::Mono_Background);

  GUI::Font::setUsingID(TimeFont, sprite);
  sprite->setTextColor(GUI::Mono_Foreground);
  sprite->setTextDatum(TC_DATUM);
  sprite->drawString(timeString, TimeWidth/2, 0);

  sprite->setBitmapColor(GUI::Color_Nickname, GUI::Color_Background);
  uint16_t yPlacement = TimeYOrigin;
  sprite->pushSprite((Screen::Width-TimeWidth)/2, TimeYOrigin);
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

  static const uint8_t BackgroundIndex = 0;
  static const uint8_t BarIndex = 1;
  static const uint8_t TextIndex = 2;
  static const uint8_t BorderIndex = 3;
  uint16_t cmap[16];
  cmap[BackgroundIndex] = GUI::Color_Background;
  cmap[BarIndex] = GUI::Color_Progress;
  cmap[TextIndex] = GUI::Color_NormalText;
  cmap[BorderIndex] = GUI::Color_Border;

  sprite->setColorDepth(4);
  sprite->createSprite(w, h);
  sprite->createPalette(cmap);
  sprite->fillSprite(BackgroundIndex);

  sprite->drawRect(0, 0, w, h, BorderIndex);
  int bw = (pct/100)*(w-2);
  sprite->fillRect(1, 1, bw, (h-2), BarIndex);

  static const uint16_t PctXInset = 10;
  static const uint16_t TxtXInset = 5;
  GUI::Font::setUsingID(ProgressFont, sprite);
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
  sprite->createSprite(Screen::Width, DetailHeight);
  sprite->fillSprite(GUI::Mono_Background);
  GUI::Font::setUsingID(DetailFont, sprite);
  sprite->setTextColor(GUI::Mono_Foreground);
  sprite->setTextDatum(TL_DATUM);

  // ----- Display the temps
  float target, actual;
  printer->getBedTemps(actual, target);
  String temp = "Bed: " + String(actual, 1) + " / " + String(target, 1);
  sprite->drawString(temp, DetailXInset, 0);

  printer->getToolTemps(actual, target);
  temp = "E0: " + String(actual, 1) + " / " + String(target, 1);
  sprite->drawString(temp, Screen::XCenter+DetailXInset, 0);

  // ----- Display the elapsed Time
  GUI::Font::setUsingID(DetailFont, sprite);
  sprite->setTextDatum(TL_DATUM);
  sprite->setTextColor(GUI::Color_NormalText);
  String elapsed = "Done: " + WebThing::formattedInterval(printer->getElapsedTime());
  sprite->drawString(elapsed, DetailXInset, DetailFontHeight);

  // ----- Display expected completion time
  String est = "Complete";
  if (printer->getState() == PrintClient::State::Printing) {
    est = "Est: ";
    appendDate(now() + printer->getPrintTimeLeft(), est);
  }
  sprite->drawString(est, Screen::XCenter+DetailXInset, DetailFontHeight);

  sprite->setBitmapColor(GUI::Color_NormalText, GUI::Color_Background);
  sprite->pushSprite(DetailXOrigin, DetailYOrigin);
  sprite->deleteSprite();
}

void DetailScreen::scrollFileName() {
  sprite->setColorDepth(1);
  sprite->createSprite(Screen::Width, DetailHeight);
  sprite->fillSprite(GUI::Mono_Background);
  GUI::Font::setUsingID(DetailFont, sprite);
  sprite->setTextColor(GUI::Mono_Foreground);
  sprite->setTextDatum(TL_DATUM);

  uint32_t extraDelay = 0;
  String name = MultiMon::printer[index]->getFilename();
  if (scrollIndex == nameWidth - Screen::Width) { delta = -delta; extraDelay = 500; }
  sprite->drawString(name, -scrollIndex, 0);
  sprite->setBitmapColor(GUI::Color_DimText, GUI::Color_Background);
  sprite->pushSprite(0, FileNameYOrigin);
  sprite->deleteSprite();

  nextScrollTime = millis() + 10 + extraDelay;
  scrollIndex = scrollIndex + delta;
}

void DetailScreen::revealFullFileName() {
  if (nameWidth <= Screen::Width) return; // It's already revealed
  if (scrollIndex != -1) {  // We're already scrolling, finish
    scrollIndex = 0;
    delta = -1;
  } else {
    scrollIndex = 1;
    delta = 1;    
  }
}





