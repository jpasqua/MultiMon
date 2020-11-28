/*
 * ESP32_FS
 *    Implementation of the ESP_FS interface for ESP32
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

#if defined(ESP32)

#include <FS.h>
#include <SPIFFS.h>

namespace ESP_FS {
  File open(const char* path, const char* mode) { return SPIFFS.open(path, mode); }
  File open(const String& path, const char* mode) { return SPIFFS.open(path, mode); }

  bool exists(const char* path) { return SPIFFS.exists(path); }
  bool exists(const String& path) { return SPIFFS.exists(path); }

  static File enumRoot;

  bool beginFileList(String& path) {
    enumRoot = SPIFFS.open(path);
    return (enumRoot.isDirectory());
  }

  bool getNextFileName(String& name) {
    File f;
    do { f = enumRoot.openNextFile(); } while (f && f.isDirectory());
    if (!f) return false;
    name = f.name();
    return true;
  }};

#endif