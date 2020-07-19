/*
 * GenericPlugin
 *    A plugin that gets and displays static or global content
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
#include <ArduinoLog.h>
//                                  Third Party Libraries
//                                  App Libraries and Includes
#include "GenericPlugin.h"
#include "../gui/GUI.h"
//--------------- End:    Includes ---------------------------------------------



/*------------------------------------------------------------------------------
 *
 * GenericSettings Implementation
 *
 *----------------------------------------------------------------------------*/

GenericSettings::GenericSettings() {
  version = 1;
  maxFileSize = 256;

  enabled = true;
  refreshInterval = 10 * 60 * 1000L;  // 10 Minutes
}

void GenericSettings::fromJSON(JsonDocument& doc) {
  enabled = doc[F("enabled")];
  refreshInterval = doc[F("refreshInterval")];
}

void GenericSettings::fromJSON(String& settings) {
  DynamicJsonDocument doc(maxFileSize);
  auto error = deserializeJson(doc, settings);
  if (error) {
    Log.warning(F("GenericSettings::fromJSON, failed to parse new settings: %s"), error.c_str());
    return;
  }
  fromJSON(doc);
}

void GenericSettings::toJSON(JsonDocument& doc) {
  doc[F("refreshInterval")] = refreshInterval;
  doc[F("enabled")] = enabled;
}

void GenericSettings::toJSON(String& serialized) {
  DynamicJsonDocument doc(maxFileSize);
  toJSON(doc);
  serializeJson(doc, serialized);
}

void GenericSettings::logSettings() {
  Log.verbose(F("----- GenericSettings"));
  Log.verbose(F("  enabled: %T"), enabled);
  Log.verbose(F("  refreshInterval: %d"), refreshInterval);
}


/*------------------------------------------------------------------------------
 *
 * GenericPlugin Implementation
 *
 *----------------------------------------------------------------------------*/

GenericPlugin::~GenericPlugin() {
  // TO DO: free the settings object
}

void GenericPlugin::getSettings(String& serializedSettings) {
  settings.toJSON(serializedSettings);
}

void GenericPlugin::newSettings(String& serializedSettings) {
  settings.fromJSON(serializedSettings);
  settings.write();
}

bool GenericPlugin::typeSpecificInit() {
  settings.init(_pluginDir + "/settings.json");
  settings.read();
  settings.logSettings();

  _refreshInterval = settings.refreshInterval;
  _enabled = settings.enabled;

  _mapper = [&](String& key) -> String {
    // TO DO: map to global content
    (void)key;  // Avoid compiler warning
    return "";    
  };

  return true;
}

void GenericPlugin::refresh(bool force) {
  // Nothing to update - we're relying on global content
  (void)force; // Avoid compiler warning
}
