//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
#include <ArduinoLog.h>
//                                  Local Includes
#include "MMScreen.h"
//--------------- End:    Includes ---------------------------------------------

using GUI::tft;

class ForecastScreen : public Screen {
public:
  ForecastScreen();
  void display(bool force = false);
  virtual void processPeriodicActivity();

private:
  void displaySingleForecast(Forecast *f, uint16_t x, uint16_t y);
  void showTime(bool clear = false);
};
