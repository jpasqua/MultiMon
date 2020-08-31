/*
 * StatusScreen:
 *    Provides an overview of all printers. Each printer status area acts
 *    as a button that moves to a DetailScreen for that printer. 
 *                    
 * TO DO:
 * o Clean up how dimensions are defined / used
 *
 * DONE:
 * o Avoid screen flashing on updates
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
//                                  Local Includes
#include "StatusScreen.h"
//--------------- End:    Includes ---------------------------------------------

using GUI::tft;
using GUI::sprite;


/*------------------------------------------------------------------------------
 *
 * CONSTANTS
 *
 *----------------------------------------------------------------------------*/

static const int TileWidth = Screen::XCenter-2;
static const int TileHeight = Screen::YCenter-2;
static const int XCenterOffset = (TileWidth)/2;
static const int YCenterOffset = (TileHeight)/2;
static const int BorderSize = 2;
static const auto StatusFont = GUI::Font::FontID::SB12;
static const uint16_t StatusFontHeight = 29; // StatusFont->yAdvance
static const uint16_t StatusHeight = StatusFontHeight*2;

/*------------------------------------------------------------------------------
 *
 * Constructors and Public methods
 *
 *----------------------------------------------------------------------------*/

StatusScreen::StatusScreen() {
  auto buttonHandler =[&](int id, Button::PressType type) -> void {
    Log.verbose(F("In StatusScreen ButtonHandler, id = %d, type = %d"), id, type);
    if (type > Button::PressType::NormalPress) { GUI::displayHomeScreen(); return; }
    if (MultiMon::settings.printer[id].isActive &&
        MultiMon::printer[id]->getState() > PrintClient::State::Operational) { GUI::displayDetailScreen(id); return; }
    GUI::displayHomeScreen();
  };

  buttons = new Button[(nButtons = MultiMon::MaxPrinters)];
  for (int i = 0; i < MultiMon::MaxPrinters; i++) {
    buttons[i].init(i%2*160, (i>1)*120, 160, 120, buttonHandler, i);
  }
}

void StatusScreen::display(bool activating) {
  if (activating) {
    tft.fillScreen(GUI::Color_Background);

    // Draw all the static elements
    for (int i = 0; i < MultiMon::MaxPrinters; i++) {
      displayStaticElements(i, activating);
    }

    // Draw a 2-pixel wide grid dividing the screen into four tiles
    tft.drawRect(  0,   0, Screen::Width, 2, GUI::Color_Border);
    tft.drawRect(  0, 119, Screen::Width, 2, GUI::Color_Border);
    tft.drawRect(  0, 238, Screen::Width, 2, GUI::Color_Border);

    tft.drawRect(  0,   0, 2, Screen::Height, GUI::Color_Border);
    tft.drawRect(159,   0, 2, Screen::Height, GUI::Color_Border);
    tft.drawRect(318,   0, 2, Screen::Height, GUI::Color_Border);
  }
  for (int i = 0; i < MultiMon::MaxPrinters; i++) {
    displaySingleStatus(i, activating);
  }
  nextUpdateTime = millis() + 10 * 1000L;
}

void StatusScreen::processPeriodicActivity() {
  if (millis() >= nextUpdateTime) {
    display();
  }
}


/*------------------------------------------------------------------------------
 *
 * Private methods
 *
 *----------------------------------------------------------------------------*/

void StatusScreen::displayStaticElements(int index, bool force) {
  (void)force; // We don't use this parameter - avoid a warning...

  int x = (index%2) * (TileWidth+1);
  int y = (index>1) * (TileHeight+1);

  // Display the nickname
  tft.setTextDatum(MC_DATUM);
  GUI::Font::setUsingID(GUI::Font::FontID::SB12, tft);
  tft.setTextColor(GUI::Color_Nickname);
  tft.drawString(MultiMon::settings.printer[index].nickname, x+XCenterOffset, y+18);
}

void StatusScreen::displaySingleStatus(int index, bool force) {
  (void)force; // We don't use this parameter - avoid a warning...

  PrintClient *printer = MultiMon::printer[index];
  int x = (index%2) * (TileWidth+1);
  int y = (index>1) * (TileHeight+1);
  uint16_t statusColor = GUI::Color_AlertError;
  String statusMessage;
  String extraMessage = "";

  if (!MultiMon::settings.printer[index].isActive) {
    statusColor = GUI::Color_Inactive;
    statusMessage = "Unused";
  } else {
    switch (MultiMon::printer[index]->getState()) {
      case PrintClient::State::Offline:
        statusColor = GUI::Color_Offline;
        statusMessage  = "Offline";
        break;
      case PrintClient::State::Operational:
        statusColor = GUI::Color_Online;
        statusMessage = "Online";
        break;
      case PrintClient::State::Complete:
      case PrintClient::State::Printing:
        statusColor = GUI::Color_Online;
        statusMessage = String((int)(printer->getPctComplete())) + "%";
        extraMessage = WebThing::formattedInterval(printer->getPrintTimeLeft());
        break;
    }
  }

  sprite->setColorDepth(1);
  sprite->createSprite(TileWidth-BorderSize*2, StatusHeight);
  sprite->fillSprite(GUI::Mono_Background);
  GUI::Font::setUsingID(StatusFont, sprite);
  sprite->setTextColor(GUI::Mono_Foreground);
  sprite->setTextDatum(TC_DATUM);

  // Display the messages
  if (extraMessage.isEmpty()) {
    sprite->drawString(statusMessage, XCenterOffset, (StatusHeight - StatusFontHeight)/2);
  } else {
    sprite->drawString(statusMessage, XCenterOffset, 0);
    sprite->drawString(extraMessage, XCenterOffset, StatusFontHeight);
  }

  sprite->setBitmapColor(statusColor, GUI::Color_Background);
  sprite->pushSprite(x+BorderSize, y+YCenterOffset-StatusFontHeight/2);
  sprite->deleteSprite();
}




