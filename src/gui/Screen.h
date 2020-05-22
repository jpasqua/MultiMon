#ifndef Screen_h
#define Screen_h

#include "GUI.h"
#include "Button.h"

class Screen {
public:
  // ----- Constants
  static const uint16_t Width = 320;
  static const uint16_t Height = 240;
  static const uint16_t XCenter = Width/2;
  static const uint16_t YCenter = Height/2;

  // ----- State
  Button *buttons;
  uint8_t nButtons;

  // ----- Functions that must be implemented by subclasses
  virtual void display(bool force = false) = 0;
  virtual void processPeriodicActivity() = 0;

  // ----- Functions that may be overriden by subclasses
  virtual void activate() {
    wasPressed = false;
    startOfPress = 0;
    display(true);
  }

  void processInput() {
    uint16_t tx = 0, ty = 0;
    bool pressed = GUI::getTouch(&tx, &ty);
    if (pressed) {
      if (!wasPressed) {
        wasPressed = true;
        startOfPress = millis();
      }
      lastX = tx; lastY = ty;
    } else {
      if (wasPressed) {
        // Ok, we got a press/release, see which button (if any) is associated
        uint32_t endOfPress = millis();
        Button::PressType pt;
        uint32_t pressDuration = (millis() - startOfPress);
        if (pressDuration >= Button::VeryLongPressInterval) pt = Button::PressType::VeryLongPress;
        else if (pressDuration >= Button::LongPressInterval) pt = Button::PressType::LongPress;
        else pt = Button::PressType::NormalPress;

        for (int i = 0; i < nButtons; i++) {
          if (buttons[i].processTouch(lastX, lastY, pt)) break;
        }
      }
      wasPressed = false;
    }
  }

private:
  bool wasPressed;
  uint32_t startOfPress;
  uint16_t lastX, lastY;
};

#endif // Screen_h