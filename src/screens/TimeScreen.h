#ifndef TimeScreen_h
#define TimeScreen_h

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
//                                  WebThing Includes
#include <gui/Screen.h>
//                                  Local Includes
#include "DetailScreen.h"
//--------------- End:    Includes ---------------------------------------------

class TimeScreen : public Screen {
public:
  TimeScreen();

  void display(bool activating = false);

  virtual void processPeriodicActivity();

private:
  uint32_t nextUpdateTime = UINT32_MAX;

  void drawProgressBar(int i, uint16_t barColor, uint16_t txtColor, float pct, String txt = "");
  void drawClock(bool force = false);
  void drawStatus(bool force = false);
  void drawWeather(bool force = false);
  void drawSecondLine(bool force = false);
  void drawPrinterNames(bool force = false);
};

#endif  // TimeScreen_h