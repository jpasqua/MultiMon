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
  type = settings["type"].as<String>();
  apiKey = settings["apiKey"].as<String>();
  server = settings["server"].as<String>();
  port = settings["port"];
  user = settings["user"].as<String>();
  pass = settings["pass"].as<String>();
  nickname = settings["nickname"].as<String>();
  isActive = settings["isActive"];
  mock = settings["mock"];
}

void PrinterSettings::toJSON(JsonObject settings) const {
  settings["type"] = type;
  settings["apiKey"] = apiKey;
  settings["server"] = server;
  settings["port"] = port;
  settings["user"] = user;
  settings["pass"] = pass;
  settings["nickname"] = nickname;
  settings["isActive"] = isActive;
  settings["mock"] = mock;
}

void PrinterSettings::logSettings() {
  Log.verbose("  ----- %s: %s", nickname.c_str(), type.c_str());
  Log.verbose("  isActive: %T", isActive);
  Log.verbose("  server: %s", server.c_str());
  Log.verbose("  port: %d", port);
  Log.verbose("  apiKey: %s", apiKey.c_str());
  Log.verbose("  user: %s", user.c_str());
  Log.verbose("  pass: %s", pass.c_str());
  Log.verbose("  mock: %T", mock);
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
  JsonArrayConst osArray = doc["printerSettings"];
  int i = 0;
  for (JsonObjectConst os : osArray) {
    printer[i++].fromJSON(os);
    if (i == MaxServers) break;
  }

  use24Hour = doc["use24Hour"];
  useMetric = doc["useMetric"];

  invertDisplay = doc["invertDisplay"];
  for (int i = 0; i < nCalReadings; i++) { calibrationData[i] = doc["calibrationData"][i]; }

  owm.enabled = doc["owm"]["enabled"];
  owm.key = doc["owm"]["key"].as<String>();
  owm.cityID = doc["owm"]["cityID"];
  owm.language = doc["owm"]["language"].as<String>();

  scheduleActive = doc["scheduleActive"];
  morning.hr = doc["morning"]["hr"];
  morning.min = doc["morning"]["min"];
  morning.brightness = doc["morning"]["brightness"];

  evening.hr = doc["evening"]["hr"];
  evening.min = doc["evening"]["min"];
  evening.brightness = doc["evening"]["brightness"];
}

void MMSettings::toJSON(JsonDocument &doc) {
  JsonArray printerSettings = doc.createNestedArray("printerSettings");
  for (int i = 0; i < MaxServers; i++) {
    printer[i].toJSON(printerSettings.createNestedObject());
  }

  doc["use24Hour"] = use24Hour;
  doc["useMetric"] = useMetric;

  doc["invertDisplay"] = invertDisplay;
  JsonArray cd = doc.createNestedArray("calibrationData");
  for (int i = 0; i < nCalReadings; i++) { cd.add(calibrationData[i]); }

  doc["owm"]["enabled"] = owm.enabled;
  doc["owm"]["key"] = owm.key;
  doc["owm"]["cityID"] = owm.cityID;
  doc["owm"]["language"] = owm.language;

  doc["scheduleActive"] = scheduleActive;

  doc["morning"]["hr"] = morning.hr;
  doc["morning"]["min"] = morning.min;
  doc["morning"]["brightness"] = morning.brightness;

  doc["evening"]["hr"] = evening.hr;
  doc["evening"]["min"] = evening.min;
  doc["evening"]["brightness"] = evening.brightness;

  // serializeJsonPretty(doc, Serial); Serial.println();
}

void MMSettings::logSettings() {
  for (int i = 0; i < MaxServers; i++) {
    Log.verbose("Octoprint Settings %d", i);
    printer[i].logSettings();
  }
  Log.verbose("Display Settings");
  Log.verbose("  use24Hour: %T", use24Hour);
  Log.verbose("  useMetric: %T", useMetric);
  Log.verbose("HW Settings");
  Log.verbose("  invertDisplay: %T", invertDisplay);
  Log.verbose("  CalibrationData: [");
  for (int i = 0; i < nCalReadings; i++) { Log.verbose("    %d,", calibrationData[i]); }
  Log.verbose("  ]");
  Log.verbose("OpenWeatherMap Settings");
  Log.verbose("  enabled: %T", owm.enabled);
  Log.verbose("  API Key: %s", owm.key.c_str());
  Log.verbose("  City ID: %d", owm.cityID);
  Log.verbose("  Language: %s", owm.language.c_str());
  Log.verbose("Schedules (active: %T)", scheduleActive);
  Log.verbose("  Morning: [time: %d:%d, %d]", morning.hr, morning.min, morning.brightness);
  Log.verbose("  Evening: [time: %d:%d, %d]", evening.hr, evening.min, evening.brightness);
}

