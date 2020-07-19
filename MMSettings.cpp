/*
 * MMSettings
 *    Handle reading and writing settings information to the file system
 *    in JSON format.
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
#include <ArduinoLog.h>
#include <ArduinoJson.h>
//                                  Local Includes
#include "MMSettings.h"
//--------------- End:    Includes ---------------------------------------------

const uint32_t  MMSettings::CurrentVersion = 0x0003;


/*------------------------------------------------------------------------------
 *
 * PrinterSettings Implementation
 *
 *----------------------------------------------------------------------------*/

PrinterSettings::PrinterSettings() {
  init();
}

void PrinterSettings::init() {
  apiKey = "";
  server = "octopi.local";
  port = 80;
  user = "";
  pass = "";
  String nickname = "";
  isActive = false;
  mock = false;
}

void PrinterSettings::fromJSON(JsonObjectConst settings) {
  type = settings[F("type")].as<String>();
  apiKey = settings[F("apiKey")].as<String>();
  server = settings[F("server")].as<String>();
  port = settings[F("port")];
  user = settings[F("user")].as<String>();
  pass = settings[F("pass")].as<String>();
  nickname = settings[F("nickname")].as<String>();
  isActive = settings[F("isActive")];
  mock = settings[F("mock")];
}

void PrinterSettings::toJSON(JsonObject settings) const {
  settings[F("type")] = type;
  settings[F("apiKey")] = apiKey;
  settings[F("server")] = server;
  settings[F("port")] = port;
  settings[F("user")] = user;
  settings[F("pass")] = pass;
  settings[F("nickname")] = nickname;
  settings[F("isActive")] = isActive;
  settings[F("mock")] = mock;
}

void PrinterSettings::logSettings() {
  Log.verbose(F("  ----- %s: %s"), nickname.c_str(), type.c_str());
  Log.verbose(F("  isActive: %T"), isActive);
  Log.verbose(F("  server: %s"), server.c_str());
  Log.verbose(F("  port: %d"), port);
  Log.verbose(F("  apiKey: %s"), apiKey.c_str());
  Log.verbose(F("  user: %s"), user.c_str());
  Log.verbose(F("  pass: %s"), pass.c_str());
  Log.verbose(F("  mock: %T"), mock);
}


/*------------------------------------------------------------------------------
 *
 * MMSettings Implementation
 *
 *----------------------------------------------------------------------------*/

MMSettings::MMSettings() {
  version = MMSettings::CurrentVersion;
  maxFileSize = 2048;
  for (int i = 0; i < MaxServers; i++) { printer[i].init(); }
  memset(&calibrationData[0], 0, sizeof(calibrationData));
  scheduleActive = true;
  morning.hr =  8; morning.min = 0; morning.brightness = 100;
  evening.hr = 20; evening.min = 0; evening.brightness = 20;
}

void MMSettings::fromJSON(JsonDocument &doc) {
  JsonArrayConst osArray = doc[F("printerSettings")];
  int i = 0;
  for (JsonObjectConst os : osArray) {
    printer[i++].fromJSON(os);
    if (i == MaxServers) break;
  }
  printerRefreshInterval = doc[F("printerRefreshInterval")];

  use24Hour = doc[F("use24Hour")];
  useMetric = doc[F("useMetric")];

  showDevMenu = doc[F("showDevMenu")];

  invertDisplay = doc[F("invertDisplay")];
  for (int i = 0; i < nCalReadings; i++) { calibrationData[i] = doc[F("calibrationData")][i]; }

  owm.enabled = doc[F("owm")][F("enabled")];
  owm.key = doc[F("owm")][F("key")].as<String>();
  owm.cityID = doc[F("owm")][F("cityID")];
  owm.language = doc[F("owm")][F("language")].as<String>();

  scheduleActive = doc[F("scheduleActive")];
  morning.hr = doc[F("morning")][F("hr")];
  morning.min = doc[F("morning")][F("min")];
  morning.brightness = doc[F("morning")][F("brightness")];

  evening.hr = doc[F("evening")][F("hr")];
  evening.min = doc[F("evening")][F("min")];
  evening.brightness = doc[F("evening")][F("brightness")];
}

void MMSettings::toJSON(JsonDocument &doc) {
  JsonArray printerSettings = doc.createNestedArray(F("printerSettings"));
  for (int i = 0; i < MaxServers; i++) {
    printer[i].toJSON(printerSettings.createNestedObject());
  }
  doc[F("printerRefreshInterval")] = printerRefreshInterval;

  doc[F("use24Hour")] = use24Hour;
  doc[F("useMetric")] = useMetric;

  doc[F("showDevMenu")] = showDevMenu;

  doc[F("invertDisplay")] = invertDisplay;
  JsonArray cd = doc.createNestedArray(F("calibrationData"));
  for (int i = 0; i < nCalReadings; i++) { cd.add(calibrationData[i]); }

  doc[F("owm")][F("enabled")] = owm.enabled;
  doc[F("owm")][F("key")] = owm.key;
  doc[F("owm")][F("cityID")] = owm.cityID;
  doc[F("owm")][F("language")] = owm.language;

  doc[F("scheduleActive")] = scheduleActive;

  doc[F("morning")][F("hr")] = morning.hr;
  doc[F("morning")][F("min")] = morning.min;
  doc[F("morning")][F("brightness")] = morning.brightness;

  doc[F("evening")][F("hr")] = evening.hr;
  doc[F("evening")][F("min")] = evening.min;
  doc[F("evening")][F("brightness")] = evening.brightness;

  // serializeJsonPretty(doc, Serial); Serial.println();
}

void MMSettings::logSettings() {
  for (int i = 0; i < MaxServers; i++) {
    Log.verbose(F("Printer Settings %d"), i);
    printer[i].logSettings();
  }
  Log.verbose(F("Printer refresh interval: %d"), printerRefreshInterval);
  Log.verbose(F("Display Settings"));
  Log.verbose(F("  use24Hour: %T"), use24Hour);
  Log.verbose(F("  useMetric: %T"), useMetric);
  Log.verbose(F("  show dev menu: %T"), showDevMenu);
  Log.verbose(F("HW Settings"));
  Log.verbose(F("  invertDisplay: %T"), invertDisplay);
  Log.verbose(F("  CalibrationData: ["));
  for (int i = 0; i < nCalReadings; i++) { Log.verbose(F("    %d,"), calibrationData[i]); }
  Log.verbose(F("  ]"));
  Log.verbose(F("OpenWeatherMap Settings"));
  Log.verbose(F("  enabled: %T"), owm.enabled);
  Log.verbose(F("  API Key: %s"), owm.key.c_str());
  Log.verbose(F("  City ID: %d"), owm.cityID);
  Log.verbose(F("  Language: %s"), owm.language.c_str());
  Log.verbose(F("Schedules (active: %T)"), scheduleActive);
  Log.verbose(F("  Morning: [time: %d:%d, %d]"), morning.hr, morning.min, morning.brightness);
  Log.verbose(F("  Evening: [time: %d:%d, %d]"), evening.hr, evening.min, evening.brightness);
}

