/*
 * InfoScreen:
 *    Display info about the MultiMon including things like the server name,
 *    wifi address, heap stats, etc. Also allow brightness adjustment 
 *                    
 * TO DO:
 *
 * DONE
 * o Add WiFi strength meter (use WebThing::wifiQualityAsPct)
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <FS.h>
#include <ESP8266WiFi.h>
//                                  Third Party Libraries
#include <TimeLib.h>
//                                  Local Includes
#include "MMScreen.h"
//--------------- End:    Includes ---------------------------------------------

using GUI::tft;

static const auto NormalFont = &FreeSansBold9pt7b;
static const uint16_t NormalFontHeight = NormalFont->yAdvance;
static const auto HeaderFont = &FreeSansBold12pt7b;
static const uint16_t HeaderFontHeight = HeaderFont->yAdvance;
static const auto ButtonFont = NormalFont;
static const uint16_t ButtonFontHeight = ButtonFont->yAdvance;

static const uint16_t ButtonFrameSize = 2;
static const uint16_t ButtonHeight = ButtonFontHeight + 2 * ButtonFrameSize+10;

static const uint16_t RefreshButtonWidth = Screen::Width/2;
static const uint16_t RefreshButtonX = (Screen::Width - RefreshButtonWidth)/2;
static const uint16_t RefreshButtonY = 65;

static const uint16_t ButtonInsetFromEdge = 5;
static const uint16_t ButtonWidth = 100;
static const uint16_t ButtonSpacing = (Screen::Width - 3*ButtonWidth - 2*ButtonInsetFromEdge)/2;
static const uint16_t ButtonYOrigin = 120;
static const char *label[3] = {"Dim", "Medium", "Bright"};

static const uint16_t WifiBarsWidth = 13;
static const uint16_t WifiBarsHeight = 16;

static const uint8_t DimButtonIndex = 0;
static const uint8_t MediumButtonIndex = 1;
static const uint8_t BrightButtonIndex = 2;
static const uint8_t RefreshButtonIndex = 3;
static const uint8_t ReturnButtonIndex = 4;

class InfoScreen : public Screen {
public:
  InfoScreen() {
    auto buttonHandler =[&](int id, Button::PressType type) -> void {
      Log.verbose("In InfoScreenButtonHandler, id = %d", id);
      if (id <= BrightButtonIndex) {
        GUI::setBrightness(id == DimButtonIndex ? 20 : (id == MediumButtonIndex ? 50 : 90));
        displayCurrentBrightness(true);
      } else if (id == RefreshButtonIndex) {
        MultiMon::Protected::updatePrinterData();
      } else {
        if (type > Button::PressType::NormalPress) GUI::displayCalibrationScreen();
        else GUI::displayHomeScreen();
      }
    };

    buttons = new Button[(nButtons = 5)];

    buttons[DimButtonIndex].init(
        ButtonInsetFromEdge, ButtonYOrigin, ButtonWidth, ButtonHeight,
        buttonHandler, DimButtonIndex);
    buttons[MediumButtonIndex].init(
        ButtonInsetFromEdge+ButtonWidth+ButtonSpacing, ButtonYOrigin, ButtonWidth, ButtonHeight,
        buttonHandler, MediumButtonIndex);
    buttons[BrightButtonIndex].init(
        Screen::Width-ButtonWidth-ButtonInsetFromEdge, ButtonYOrigin, ButtonWidth, ButtonHeight,
        buttonHandler, BrightButtonIndex);
    buttons[RefreshButtonIndex].init(
        RefreshButtonX, RefreshButtonY, RefreshButtonWidth, ButtonHeight,
        buttonHandler, RefreshButtonIndex);
    buttons[ReturnButtonIndex].init(  // The rest of the screen is a button
        0, 0, Screen::Width, Screen::Height,
        buttonHandler, ReturnButtonIndex); 
  }

  void display(bool activating = false) {
    (void)activating; // We don't use this parameter - avoid a warning...
    uint16_t y = 0;

    tft.fillScreen(GUI::Color_Background);

    tft.setTextColor(GUI::Color_AlertGood);
    tft.setFreeFont(HeaderFont);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("MultiMon v" + MultiMon::VersionString, Screen::XCenter, y);
    y += HeaderFontHeight;

    tft.setFreeFont(NormalFont);
    tft.setTextColor(GUI::Color_NormalText);
    tft.setTextDatum(TC_DATUM);
    String address = WebThing::settings.hostname + " (" + WiFi.localIP().toString() + ")";
    tft.drawString(address, Screen::XCenter, y);
    drawWifiStrength(Screen::Width-WifiBarsWidth-3, y+NormalFontHeight-8, GUI::Color_NormalText); // Put a little padding around the Bars
    y += NormalFontHeight;

    for (int i = DimButtonIndex; i <= BrightButtonIndex; i++) { drawButton(label[i], i); }
    drawButton("Refresh", RefreshButtonIndex);

    y = displayCurrentBrightness(false);

    tft.setFreeFont(NormalFont);
    tft.setTextColor(GUI::Color_NormalText);
    tft.setTextDatum(TC_DATUM);
    String heapStats = "Heap: Free="+String(ESP.getFreeHeap())+", Frag="+String(ESP.getHeapFragmentation())+"%";
    y += 1; // A little padding
    tft.drawString(heapStats, Screen::XCenter, y);
    y += NormalFontHeight;

    tft.setTextColor(GUI::Color_Inactive);
    tft.setFreeFont(NormalFont);
    tft.drawString("Written by Joe Pasqua", Screen::XCenter, Screen::Height - NormalFontHeight);
  }

  virtual void processPeriodicActivity() {  }

private:
  uint16_t displayCurrentBrightness(bool clear) {
    if (clear) tft.fillRect(0, ButtonYOrigin+ButtonHeight+5, Screen::Width, ButtonFontHeight, GUI::Color_Background);
    tft.setFreeFont(NormalFont);
    tft.setTextColor(GUI::Color_NormalText);
    tft.setTextDatum(TC_DATUM);
    String b = "(Current brightness: " + String(GUI::getBrightness()) + "%)";
    uint16_t yLoc = ButtonYOrigin+ButtonHeight+6; // A little padding
    tft.drawString(b, Screen::XCenter, yLoc); 
    return yLoc+NormalFontHeight;
  }

  void drawButton(String label, int i, bool clear = false) {
    if (clear) buttons[i].clear(GUI::Color_Background);
    buttons[i].drawSimple(
        label, ButtonFont, ButtonFrameSize,
        GUI::Color_AlertGood, GUI::Color_Border, GUI::Color_Background);
  }

  // (x, y) represents the bottom left corner of the wifi strength bars
  void drawWifiStrength(uint16_t x, uint16_t y, uint32_t color) {
    int8_t quality = WebThing::wifiQualityAsPct();
    for (int i = 0; i < 4; i++) {
      int h = (quality > (25 * i)) ? 4*(i+1) : 1;
      tft.drawRect(x+(i*4), y-h+1, 1, h, color);
    }
  }

};












