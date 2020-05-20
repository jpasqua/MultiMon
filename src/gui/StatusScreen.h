//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
#include <ArduinoLog.h>
//                                  Local Includes
#include "MMScreen.h"
//--------------- End:    Includes ---------------------------------------------

using GUI::tft;

class StatusScreen : public Screen {
public:
  StatusScreen();
  void display(bool force = false);
  virtual void processPeriodicActivity();

private:
  uint32_t nextUpdateTime = UINT32_MAX;
  void displaySingleStatus(int index, bool force = false);
  void displayStaticElements(int index, bool force = false);
};
