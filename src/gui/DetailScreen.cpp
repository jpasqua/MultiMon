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
static const auto TitleFont = &FreeSansBold18pt7b;
static const auto TitleFontHeight = TitleFont->yAdvance;
static const auto TitleAreaHeight = TitleFontHeight;

static const uint16_t FileNameYOrigin = TitleAreaYOrigin + TitleAreaHeight;
static const auto FileNameFont = &FreeSansBold9pt7b;
static const auto FileNameFontHeight = FileNameFont->yAdvance;

// The button that initiates scrolling of the file name covers both the title
// area and the file name area. The file name area by itself is too small.
static const uint16_t FileNameButtonX = 0;
static const uint16_t FileNameButtonY = 0;
static const uint16_t FileNameButtonWidth = Screen::Width;
static const uint16_t FileNameButtonHeight = 64; // TitleAreaHeight+FileNameFontHeight;

static const auto ProgressFont = &FreeSansBold18pt7b;
static const uint16_t ProgressXInset = 4;
static const uint16_t ProgressXOrigin = ProgressXInset;
static const uint16_t ProgressYOrigin = 100;
static const uint16_t ProgressHeight = ProgressFont->yAdvance;
static const uint16_t ProgressWidth = Screen::Width - (2 * ProgressXInset);

static const auto DetailFont = &FreeSansBold9pt7b;
static const uint16_t DetailXInset = 10;
static const uint16_t DetailYBottomMargin = 4;
static const uint16_t DetailFontHeight = DetailFont->yAdvance;
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
    Log.verbose("In DetailScreen ButtonHandler, id = %d", id);
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

  nextUpdateTime = millis() + (10 * 1000L);
}

void DetailScreen::processPeriodicActivity() {
  if (scrollIndex != -1 && (millis() >= nextScrollTime)) {
    scrollFileName();
  }
  if (millis() >= nextUpdateTime) { display();  }
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
  // ----- Display the nickname
  tft.setTextDatum(TC_DATUM);
  tft.setFreeFont(TitleFont);
  tft.setTextColor(GUI::Color_Nickname);
  tft.drawString(MultiMon::settings.printer[index].nickname, Screen::XCenter, 5);

  String name = printer->getFilename();
  tft.setFreeFont(DetailFont);            // Set font BEFORE measuring width
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

void DetailScreen::drawProgressBar(
    uint16_t x, uint16_t y, uint16_t w, uint16_t h, float pct, String txt, bool force) {
  static float  lastPct = -1;
  static String lastTxt = "";

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
  sprite->setFreeFont(ProgressFont);
  sprite->setTextColor(TextIndex);
  sprite->setTextDatum(ML_DATUM);
  sprite->drawString(String((int)(pct)) + "%", PctXInset, (h/2));
  sprite->setTextDatum(MR_DATUM);
  sprite->drawString(txt, w-TxtXInset, (h/2));
  sprite->pushSprite(x, y);

  sprite->deleteSprite();
}

void DetailScreen::drawDetailInfo(PrintClient *printer, bool force) {
  sprite->setColorDepth(1);
  sprite->createSprite(Screen::Width, DetailHeight);
  sprite->fillSprite(GUI::Mono_Background);
  sprite->setFreeFont(DetailFont);
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
  sprite->setFreeFont(DetailFont);
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
  sprite->setFreeFont(DetailFont);
  sprite->setTextColor(GUI::Mono_Foreground);
  sprite->setTextDatum(TL_DATUM);

  String name = MultiMon::printer[index]->getFilename();
  if (scrollIndex == nameWidth - Screen::Width) { delta = -delta; }
  sprite->drawString(name, -scrollIndex, 0);
  sprite->setBitmapColor(GUI::Color_DimText, GUI::Color_Background);
  sprite->pushSprite(0, FileNameYOrigin);
  sprite->deleteSprite();

  nextScrollTime = millis() + 10;
  scrollIndex = scrollIndex + delta;
}

void DetailScreen::revealFullFileName() {
  if (nameWidth <= Screen::Width) return; // It's already revealed
  scrollIndex = 1;
  delta = 1;
}





