/*
 * RebootScreen:
 *    Confirmation screen to trigger a reboot 
 *                    
 * TO DO:
 *
 * DONE
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <FS.h>
//#include <ESP8266WiFi.h>
//                                  Third Party Libraries
#include <TimeLib.h>
//                                  Local Includes
#include "MMScreen.h"
#include "images/CancelBitmap.h"
#include "images/RebootBitmap.h"
//--------------- End:    Includes ---------------------------------------------

using GUI::tft;

static uint16_t IconInset = 10;

static const uint8_t RebootButtonID = 0;
static const uint8_t CancelButtonID = 1;


class RebootScreen : public Screen {
public:
  RebootScreen() {
    auto buttonHandler =[&](int id, Button::PressType type) -> void {
      Log.verbose(F("In RebootScreen ButtonHandler, id = %d"), id);
      if (id == RebootButtonID && type > Button::PressType::NormalPress) { ESP.restart(); }
      if (id == CancelButtonID) GUI::displayHomeScreen();
    };

    buttons = new Button[(nButtons = 2)];
    buttons[0].init(0, 0,                Screen::Width, Screen::Height/2, buttonHandler, RebootButtonID);
    buttons[1].init(0, Screen::Height/2, Screen::Width, Screen::Height/2, buttonHandler, CancelButtonID);
  }

  void display(bool activating = false) {
    (void)activating; // We don't use this parameter - avoid a warning...
    tft.fillScreen(GUI::Color_Background);

    GUI::Font::setUsingID(GUI::Font::FontID::SB12, tft);
    tft.setTextDatum(MC_DATUM);

    uint16_t xc = (Screen::Width + (IconInset+RebootIcon_Width))/2;
    uint16_t yc = (Screen::Height)/4;
    tft.drawRect(
        buttons[RebootButtonID]._x, buttons[RebootButtonID]._y,
        buttons[RebootButtonID]._w, buttons[RebootButtonID]._h, GUI::Color_AlertError);
    tft.drawBitmap(
        IconInset, (Screen::Height/2-RebootIcon_Height)/2, RebootIcon,
        RebootIcon_Width, RebootIcon_Height,
        GUI::Color_AlertError);
    tft.setTextColor(GUI::Color_AlertError);
    tft.drawString(F("Reboot"), xc, yc);

    xc = (Screen::Width + (IconInset+CancelIcon_Width))/2;
    yc = (Screen::Height*3)/4;
    tft.drawRect(
        buttons[CancelButtonID]._x, buttons[CancelButtonID]._y,
        buttons[CancelButtonID]._w, buttons[CancelButtonID]._h, GUI::Color_AlertGood);
    tft.drawBitmap(
        IconInset, yc-(CancelIcon_Height/2), CancelIcon,
        CancelIcon_Width, CancelIcon_Height,
        GUI::Color_AlertGood);
    tft.setTextColor(GUI::Color_AlertGood);
    tft.drawString(F("Cancel"), xc, yc);
    autoCancelTime = millis() + 60 * 1000L; // If nothing has happened in a minute, cancel
  }

  void processPeriodicActivity() {
    if (millis() >= autoCancelTime) GUI::displayHomeScreen();
  }

private:
  uint32_t autoCancelTime = UINT32_MAX;
};












