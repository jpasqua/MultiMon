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


/*------------------------------------------------------------------------------
 *
 * MMSettings Implementations
 *
 *----------------------------------------------------------------------------*/

MMSettings::MMSettings() {
  version = MMSettings::CurrentVersion;
  maxFileSize = 2048;
  for (int i = 0; i < MaxPrinters; i++) { printer[i].init(); }
}

void MMSettings::fromJSON(const JsonDocument &doc) {
  JsonArrayConst osArray = doc[F("printerSettings")];
  int i = 0;
  for (JsonObjectConst os : osArray) {
    printer[i++].fromJSON(os);
    if (i == MaxPrinters) break;
  }
  printerRefreshInterval = doc[F("printerRefreshInterval")];
  if (printerRefreshInterval == 0) printerRefreshInterval = 30; // Sanity check

  WTAppSettings::fromJSON(doc);
  logSettings();
}

void MMSettings::toJSON(JsonDocument &doc) {
  JsonArray printerSettings = doc.createNestedArray(F("printerSettings"));
  for (int i = 0; i < MaxPrinters; i++) {
    printer[i].toJSON(printerSettings.createNestedObject());
  }
  doc[F("printerRefreshInterval")] = printerRefreshInterval;

  WTAppSettings::toJSON(doc);
}

void MMSettings::logSettings() {
  for (int i = 0; i < MaxPrinters; i++) {
    Log.verbose(F("Printer Settings %d"), i);
    printer[i].logSettings();
  }
  Log.verbose(F("Printer refresh interval: %d"), printerRefreshInterval);
  WTAppSettings::logSettings();
}



/*------------------------------------------------------------------------------
 *
 * PrinterSettings Implementations
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