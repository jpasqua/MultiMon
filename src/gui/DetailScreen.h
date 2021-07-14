//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
//                                  Local Includes
#include "MMScreen.h"
//--------------- End:    Includes ---------------------------------------------

class DetailScreen : public Screen {
public:
  DetailScreen();
  void setIndex(int i);
  void display(bool activating = false);
  void processPeriodicActivity();

private:
  int index;
  uint32_t nextUpdateTime = UINT32_MAX;
  int scrollIndex = -1;
  int nameWidth;
  int delta;
  int bound;
  uint32_t nextScrollTime = 0;

  void drawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, float pct, String txt, bool force = false);
  void drawStaticContent(PrintClient *printer, bool force = false);
  void drawDetailInfo(PrintClient *printer, bool force = false);
  void drawTime(bool force = false);
  void scrollFileName();
  void revealFullFileName();
};
