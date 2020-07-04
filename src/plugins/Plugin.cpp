/*
 * Plugin
 *    A method for adding new functionality to MultiMon.
 *
 * NOTES:
 * o A plugin consists of multiple parts:
 *   + A subclass of Plugin that can is specific to some data or activity
 *   + A screen definition which can be materialized by FlexScreen. This takes
 *     the form of a JSON file which describes the screen layout and look
 *   + A source for data to be provided to the plugin (e.g. a news source,
 *     a feed of weather info, data from some other IoT device)
 *
 */


//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
#include <FS.h>
//                                  Third Party Libraries
#include <ArduinoJson.h>
//                                  Local Includes
#include "../../MultiMon.h"
#include "../gui/GUI.h"
#include "Plugin.h"
#include "BlynkPlugin.h"
#include "StaticPlugin.h"
//--------------- End:    Includes ---------------------------------------------


static uint32_t MaxFileSize = 6 * 1024;

uint8_t Plugin::_nPlugins = 0;
Plugin* Plugin::_plugins[MaxPlugins];

void Plugin::newPlugin(DynamicJsonDocument &doc) {
  if (_nPlugins == Plugin::MaxPlugins) {
    Log.warning("Maximum number of plugins exceeded");
  }

  String type = doc["type"].as<String>();
  JsonObject typeSpecific = doc["typeSpecific"];
  JsonObjectConst screen = doc["screen"];

  Plugin *p = NULL;
  if (type.equalsIgnoreCase("static")) {
    p = new StaticPlugin();
  } else if (type.equalsIgnoreCase("blynk")) {
    if (MultiMon::settings.blynk.enabled) {
      p = new BlynkPlugin();
      // Override plugin values from general settings if available
      if (!MultiMon::settings.blynk.id1.isEmpty()) typeSpecific["blynkIDs"][0] = MultiMon::settings.blynk.id1;
      if (!MultiMon::settings.blynk.nickname1.isEmpty()) typeSpecific["nicknames"][0] = MultiMon::settings.blynk.nickname1;
      if (!MultiMon::settings.blynk.id2.isEmpty()) typeSpecific["blynkIDs"][1] = MultiMon::settings.blynk.id2;
      if (!MultiMon::settings.blynk.nickname2.isEmpty()) typeSpecific["nicknames"][1] = MultiMon::settings.blynk.nickname2;
    }
  } 

  if (p == NULL) return;
  if (!p->init(typeSpecific)) {
    delete p;
    return;
  }

  if (!GUI::createFlexScreen(screen, p->_refreshInterval, p->_mapper)) {
    delete p;
    return;
  }

  _plugins[_nPlugins++] = p;
} 

void Plugin::loadAll(String filePath) {
  _nPlugins = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  Dir dir = SPIFFS.openDir(filePath);
#pragma GCC diagnostic pop

  while (dir.next()) {
    String name = dir.fileName();
    if (!name.endsWith(".json")) continue;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    File file = SPIFFS.open(name.c_str(), "r");
#pragma GCC diagnostic pop
    if (!file) {
      Log.warning("Can't read Plugin descriptor even though it exists: %s", name.c_str());
      continue;
    }

    size_t size = file.size();
    if (size > MaxFileSize) {
      Log.error("Plugin descriptor (%s) is too large (%d)", name.c_str(), size);
      continue;
    }


    DynamicJsonDocument doc(MaxFileSize);
    auto error = deserializeJson(doc, file);
    file.close();
    if (error) {
      Log.warning("Failed to parse plugin descriptor (%s): %s", name.c_str(), error.c_str());
      continue;
    }
    // serializeJsonPretty(doc, Serial); Serial.println();

    newPlugin(doc);
  }
}

void Plugin::refreshAll(bool force) {
  for (int i = 0; i < _nPlugins; i++) {
    _plugins[i]->refresh(force);
  }
}
