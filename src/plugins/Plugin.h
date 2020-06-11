#ifndef Plugin_h
#define Plugin_h

/*
 * Plugin.h
 *    A method for adding enw functionality to MultiMon.
 *
 * NOTES:
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
#include <ArduinoJson.h>
//                                  Local Includes
#include "../Basics.h"
//--------------- End:    Includes ---------------------------------------------


class Plugin {
public:
  static void loadAll(String filePath);
  static void refreshAll(bool force = false);

  virtual ~Plugin() { }
	virtual bool init(JsonObjectConst &obj) = 0;
	virtual void refresh(bool force = false) = 0;

protected:
  uint32_t _refreshInterval;
  Basics::StringMapper _mapper;

private:
  static const uint8_t MaxPlugins = 5;
  static uint8_t _nPlugins;
  static Plugin* _plugins[MaxPlugins];

  static void newPlugin(DynamicJsonDocument &doc);
};

#endif // Plugin_h
