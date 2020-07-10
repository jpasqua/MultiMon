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
  JsonArrayConst osArray = doc["printerSettings"];
  int i = 0;
  for (JsonObjectConst os : osArray) {
    printer[i++].fromJSON(os);
    if (i == MaxServers) break;
  }
  printerRefreshInterval = doc["printerRefreshInterval"];

  use24Hour = doc["use24Hour"];
  useMetric = doc["useMetric"];

  blynk.enabled = doc["blynk"]["enabled"];
  blynk.id1 = doc["blynk"]["id1"].as<String>();
  blynk.id2 = doc["blynk"]["id2"].as<String>();
  blynk.nickname1 = doc["blynk"]["nickname1"].as<String>();
  blynk.nickname2 = doc["blynk"]["nickname2"].as<String>();

  showDevMenu = doc["showDevMenu"];

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
  doc["printerRefreshInterval"] = printerRefreshInterval;

  doc["use24Hour"] = use24Hour;
  doc["useMetric"] = useMetric;

  doc["blynk"]["enabled"] = blynk.enabled;
  doc["blynk"]["id1"] = blynk.id1;
  doc["blynk"]["id2"] = blynk.id2;
  doc["blynk"]["nickname1"] = blynk.nickname1;
  doc["blynk"]["nickname2"] = blynk.nickname2;


  doc["showDevMenu"] = showDevMenu;

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
    Log.verbose(F("Printer Settings %d"), i);
    printer[i].logSettings();
  }
  Log.verbose(F("Printer refresh interval: %d"), printerRefreshInterval);
  Log.verbose(F("Display Settings"));
  Log.verbose(F("  use24Hour: %T"), use24Hour);
  Log.verbose(F("  useMetric: %T"), useMetric);
  Log.verbose(F("  blynk enabled: %T"), blynk.enabled);
  Log.verbose(F("    blynk ID1: %s"), blynk.id1.c_str());
  Log.verbose(F("    blynk ID2: %s"), blynk.id2.c_str());
  Log.verbose(F("    blynk Nickname 1: %s"), blynk.nickname1.c_str());
  Log.verbose(F("    blynk Nickname 2: %s"), blynk.nickname2.c_str());
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

