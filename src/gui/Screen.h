#ifndef Screen_h
#define Screen_h

#include "GUI.h"

typedef enum {NormalPress, LongPress, VeryLongPress} PressType;
typedef std::function<void(int, PressType)> ButtonCallback;

static const uint32_t LongPressInterval = 500;
static const uint32_t VeryLongPressInterval = 1000;

class Button {
public:
  uint16_t _x;
  uint16_t _y;
  uint16_t _w;
  uint16_t _h;
  ButtonCallback _callback;
  uint8_t _id;

  Button() { }
  Button(uint16_t x, uint16_t y, uint16_t w, uint16_t h, ButtonCallback callback, uint8_t id) {
    init(x, y, w, h, callback, id);
  }

  void init(uint16_t x, uint16_t y, uint16_t w, uint16_t h, ButtonCallback callback, uint8_t id) {
    _x = x; _y = y; _w = w; _h = h; _callback = callback; _id = id;
  }

  bool processTouch(uint16_t tx, uint16_t ty, PressType type) {
    if ((tx >= _x) && (tx < _x+_w) && (ty >= _y) && (ty < _y+_h)) { _callback(_id, type); return true; }
    return false;
  }
};

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
        PressType pt;
        uint32_t pressDuration = (millis() - startOfPress);
        if (pressDuration >= VeryLongPressInterval) pt = VeryLongPress;
        else if (pressDuration >= LongPressInterval) pt = LongPress;
        else pt = NormalPress;

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