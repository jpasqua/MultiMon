//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
//                                  Local Includes
#include "MMScreen.h"
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
  void drawNextComplete(bool force = false);
  void drawPrinterNames(bool force = false);
};