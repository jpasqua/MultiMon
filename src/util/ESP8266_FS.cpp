/*
 * ESP8266_FS
 *    Implementation of the ESP_FS interface for ESP8266
 *
 * NOTES:
 * o Because of the way SPIFFS works (or actually because of the way it doesn't work)
 *   We need to take a circuitous path to finding all the well-formed plugins. We can't
 *   really enumerate directories (they are a convenient fiction in SPIFFS) nor can we
 *   depend on the order of enumeration being breadth-first OR depth-first.
 *   Instead, we just enumerate all the files under pluginRoot and keep track of
 *   the unique subdirectories
 *
 */

#if defined(ESP8266)

#include <FS.h>
#include "ESP_FS.h"

namespace ESP_FS {
  // SPIFFS is being deprecated on ESP8266which causes warnings at compile time.
  // I have a task to move off of SPIFFS to LittleFS, but in the mean time, I
  // don't want to keep seeing the warnings so I wrapped the SPIFFS calls with
  // pragma's to silence the warnings
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    File open(const char* path, const char* mode) { return SPIFFS.open(path, mode); }
    File open(const String& path, const char* mode) { return SPIFFS.open(path, mode); }

    bool exists(const char* path) { return SPIFFS.exists(path); }
    bool exists(const String& path) { return SPIFFS.exists(path); }

    Dir openDir(const char* path) { return SPIFFS.openDir(path); }
    Dir openDir(const String& path) { return SPIFFS.openDir(path); }
  #pragma GCC diagnostic pop

  static Dir enumRoot;
  bool beginFileList(String& path) {
    enumRoot = FS_openDir(path);
    return (enumRoot != NULL);
  }
  bool getNextFileName(String &name) {
    if (!enumRoot.next()) return false;
    name = enumRoot.fileName();
    return true;
  }
};

#endif