//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
#include <ArduinoLog.h>
//                                  Local Includes
#include "MMScreen.h"
//--------------- End:    Includes ---------------------------------------------

using GUI::tft;

class WeatherScreen : public Screen {
public:
  WeatherScreen();
  void display(bool force = false);
  virtual void processPeriodicActivity();

private:
  uint32_t lastDT = UINT32_MAX;
  uint32_t lastClockUpdate = 0;
  
  void displaySingleWeather(int index);
  void showTime();
};
