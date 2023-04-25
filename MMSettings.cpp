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

