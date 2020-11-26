#ifndef PluginMgr_h
#define PluginMgr_h

/*
 * PluginMgr.h
 *    Finds, instantiates, and manages all Plugins
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
#include "Plugin.h"
//--------------- End:    Includes ---------------------------------------------


class PluginMgr {
public:
  static DynamicJsonDocument* getDoc(String filePathString, uint16_t maxFileSize);
  void    loadAll(String pluginRoot);
  void    refreshAll(bool force = false);
  uint8_t getPluginCount();
  Plugin* getPlugin(uint8_t index);
  void    map(const String& key, String& value);

private:
  static const uint8_t MaxPlugins = 4;
  uint8_t _nPlugins;
  Plugin* _plugins[MaxPlugins];

  bool validatePluginFiles(String pluginPath);
  void newPlugin(String pluginPath);
  uint8_t enumPlugins(String& pluginRoot, String* pluginDirNames);
};

#endif // PluginMgr_h
