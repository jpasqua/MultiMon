//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
//                                  WebThing Includes
#include <WebThingApp/gui/Screen.h>
//                                  Local Includes
//--------------- End:    Includes ---------------------------------------------


class SplashScreen : public Screen {
public:
  SplashScreen();
  void display(bool activating = false);
  virtual void processPeriodicActivity();
};