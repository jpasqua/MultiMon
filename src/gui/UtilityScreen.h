/*
 * UtilityScreen:
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

static const char *pis_label[6] = {"Dim", "Medium", "Bright", "Refresh", "Calibrate", "Home"};
static const uint16_t pis_colors[6] = {
  GUI::Color_Inactive,   GUI::Color_DimText,    GUI::Color_NormalText,
  GUI::Color_NormalText, GUI::Color_NormalText, GUI::Color_AlertGood};

class UtilityScreen : public Screen {
public:
  UtilityScreen() {
    auto buttonHandler =[&](int id, Button::PressType type) -> void {
      Log.verbose(F("In UtilityScreen Button Handler, id = %d, type = %d"), id, type);

      if (id < MaxPlugins) {
        Plugin *p = MultiMon::pluginMgr.getPlugin(id);
        if (p && p->enabled()) { GUI::displayFlexScreen(p->getScreenID()); }
        return;
      }

      if (id <= BrightButtonIndex) {
        GUI::setBrightness(id == DimButtonIndex ? 20 : (id == MediumButtonIndex ? 50 : 90));
        return;
      }

      if (id == RefreshButtonIndex) { MultiMon::Protected::updateAllData(); return;}
      if (id == CalButtonIndex) { GUI::displayCalibrationScreen(); return; }
      if (id == HomeButtonIndex) { GUI::displayHomeScreen(); return; }
    };

    buttons = new Button[(nButtons = TotalButtons)];
    for (int i = 0; i < MaxPlugins; i++) {
      buttons[i].init(
        ButtonInset + ((i%2) * HalfWidth), PI_YOrigin + (i/2) * ButtonHeight, HalfWidth, ButtonHeight, buttonHandler, i);        
    }

    int x = ButtonInset;
    buttons[DimButtonIndex].init(
        x, 160, ThirdWidth, ButtonHeight, buttonHandler, DimButtonIndex);     x+= ThirdWidth;
    buttons[MediumButtonIndex].init(
        x, 160, ThirdWidth, ButtonHeight, buttonHandler, MediumButtonIndex);  x+= ThirdWidth;
    buttons[BrightButtonIndex].init(
        x, 160, ThirdWidth, ButtonHeight, buttonHandler, BrightButtonIndex);
    x = ButtonInset;
    buttons[RefreshButtonIndex].init(
        x, 200, ThirdWidth, ButtonHeight, buttonHandler, RefreshButtonIndex); x+= ThirdWidth;
    buttons[CalButtonIndex].init(
        x, 200, ThirdWidth, ButtonHeight, buttonHandler, CalButtonIndex);     x+= ThirdWidth;
    buttons[HomeButtonIndex].init(
        x, 200, ThirdWidth, ButtonHeight, buttonHandler, HomeButtonIndex);
  }

  void display(bool activating = false) {
    if (activating) tft.fillScreen(GUI::Color_Background);

    int y = 0;
    tft.setTextColor(GUI::Color_AlertGood);
    GUI::Font::setUsingID(HeaderFont, tft);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("MultiMon v" + MultiMon::VersionString, Screen::XCenter, 0);
    drawWifiStrength(Screen::Width-WifiBarsWidth-3, ButtonHeight-12, GUI::Color_NormalText);
    y += HeaderFontHeight;

    GUI::Font::setUsingID(ButtonFont, tft);
    tft.setTextColor(GUI::Color_NormalText);
    tft.setTextDatum(TC_DATUM);
    String address = WebThing::settings.hostname + " (" + WiFi.localIP().toString() + ")";
    tft.drawString(address, Screen::XCenter, y);

    String name;
    uint16_t textColor = GUI::Color_NormalText;
    uint8_t nPlugins = min(MultiMon::pluginMgr.getPluginCount(), MaxPlugins);

    for (int i = 0; i < TotalButtons; i++) {
      if (i < nPlugins) {
        Plugin *p = MultiMon::pluginMgr.getPlugin(i);
        if (!p->enabled()) textColor = GUI::Color_Inactive;
        name = p->getName();
        textColor = GUI::Color_WiFiBlue;
      } else if (i < MaxPlugins) {
        name = "Unused";
        textColor = GUI::Color_Inactive;
      } else {
        name = pis_label[i-MaxPlugins];
        textColor = pis_colors[i-MaxPlugins];
      }
      drawButton(name, i, textColor, activating);
    }
  }

  virtual void processPeriodicActivity() {  }


private:
  static const auto HeaderFont = GUI::Font::FontID::SB12;
  static const uint16_t HeaderFontHeight = 29;

  static const auto ButtonFont = GUI::Font::FontID::SB9;
  static const uint16_t ButtonFontHeight = 22;

  static const uint16_t ButtonFrameSize = 2;
  static const uint16_t ButtonHeight = 40;
  static const uint16_t ButtonInset = 1;
  static const uint16_t HalfWidth = (Screen::Width-(2*ButtonInset))/2;
  static const uint16_t ThirdWidth = (Screen::Width-(2*ButtonInset))/3;

  static const uint16_t PI_YOrigin = 60;

  static const uint8_t FirstPluginIndex = 0;
  static const uint8_t MaxPlugins = 4;
  static const uint8_t DimButtonIndex = 4;
  static const uint8_t MediumButtonIndex = 5;
  static const uint8_t BrightButtonIndex = 6;
  static const uint8_t RefreshButtonIndex = 7;
  static const uint8_t CalButtonIndex = 8;
  static const uint8_t HomeButtonIndex = 9;
  static const uint8_t TotalButtons = 10;

  static const uint16_t WifiBarsWidth = 13;
  static const uint16_t WifiBarsHeight = 16;

  void drawButton(String label, int i, uint16_t textColor, bool clear = false) {
    if (clear) buttons[i].clear(GUI::Color_Background);
    buttons[i].drawSimple(
        label, ButtonFont, ButtonFrameSize,
        textColor, GUI::Color_Border, GUI::Color_Background);
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












