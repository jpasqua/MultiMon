#ifndef BlynkPlugin_h
#define BlynkPlugin_h

/*
 * BlynkPlugin
 *    A plugin that gets and displays content from Blynk
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
#include <ArduinoLog.h>
//                                  Third Party Libraries
//                                  App Libraries and Includes
#include "Plugin.h"
//--------------- End:    Includes ---------------------------------------------

class BlynkPlugin : public Plugin {
public:
  ~BlynkPlugin();
  bool init(JsonObject &obj);
  void refresh(bool force = false);

protected:
  String getValue(String key);

private:
  uint8_t   _nBlynkIDs;
  String*   _blynkIDs;
  String*   _nicknames;
  uint8_t   _nPins;
  String*   _pins;
  String*   _pinVals;
  uint32_t  _nextRefresh = 0;
};

#endif  // BlynkPlugin_h
