/*
 * CalibrationScreen:
 *    Provides a way to calibrate the touch sensor on the screen
 *                    
 * TO DO:
 *
 * COMPLETE:
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
#include <TimeLib.h>
//                                  Local Includes
#include "MMScreen.h"
//--------------- End:    Includes ---------------------------------------------

using GUI::tft;


class CalibrationScreen : public Screen {
public:
  CalibrationScreen() {
    init();
  }

  void display(bool activating = false) {
    if (activating) state = pre;

    tft.fillScreen(GUI::Color_Background);
    if (state == pre) {
      GUI::Font::setUsingID(GUI::Font::FontID::SB9, tft);
      tft.setTextColor(GUI::Color_AlertGood);
      tft.setTextDatum(MC_DATUM);
      tft.drawString("Touch to begin calibration", Screen::XCenter, Screen::YCenter);
    } else if (state == post) {
      GUI::Font::setUsingID(GUI::Font::FontID::SB9, tft);
      tft.setTextColor(GUI::Color_AlertGood);
      tft.setTextDatum(MC_DATUM);
      tft.drawString("Done! Touch to continue", Screen::XCenter, Screen::YCenter);
      state = complete;
    }
  }

  virtual void processPeriodicActivity() {
    if (state == post) { display(); }
  }

private:
  enum {pre, post, complete} state;

  void init() {
    auto buttonHandler =[&](int id, Button::PressType type) -> void {
      (void)type; // We don't use this parameter - avoid a warning...
      Log.verbose("In CalibrationScreenButtonHandler, id = %d", id);
      switch (state) {
        case pre:
          tft.fillScreen(GUI::Color_Background);
          GUI::Font::setUsingID(GUI::Font::FontID::SB9, tft);
          tft.setTextColor(GUI::Color_AlertGood);
          tft.setTextDatum(MC_DATUM);
          tft.drawString("Touch each corner arrow", Screen::XCenter, Screen::YCenter);
          tft.calibrateTouch(MultiMon::settings.calibrationData, TFT_WHITE, TFT_BLACK, 15);
          state = post;
          break;
        case complete:
          Log.verbose("Finished calibration, saving settings : [");
          for (int i = 0; i < MMSettings::nCalReadings; i++) { Log.verbose("  %d,", MultiMon::settings.calibrationData[i]); }
          Log.verbose("]");
          MultiMon::Protected::saveSettings();
          GUI::displayHomeScreen();
          break;
        case post:
          // Assert: should never get here
          break;
      }
    };

    buttons = new Button[(nButtons = 1)];
    buttons[0].init(0, 0, Screen::Width, Screen::Height, buttonHandler, 0);
    state = pre;
  }

};