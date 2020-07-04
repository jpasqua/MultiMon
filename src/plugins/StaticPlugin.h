#ifndef StaticPlugin_h
#define StaticPlugin_h

/*
 * StaticPlugin.h
 *    A plugin that just displays static content. Not super useful. 
 *
 * NOTES:
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

class StaticPlugin : public Plugin {
  ~StaticPlugin() {
    // Nothing to do here...
  }

  bool init(JsonObject &obj) {
    Log.trace("StaticPlugin::init()");
    // There is no type specific data for a StaticPlugin...
    (void)obj;  // Avoid "unused" compiler warning
    
    _refreshInterval = UINT32_MAX;

    _mapper = [&](String& key) -> String {
        Log.verbose("StaticPlugin::mapper %s", key.c_str());
        return "";
      };

    return true;
  }

  void refresh(bool force = false) {
    (void)force;  // Avoid compiler warning for unused parameter
  }

};

#endif  // StaticPlugin_h
