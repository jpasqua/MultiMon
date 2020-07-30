/*
 * PluginMgr
 *    Finds, instantiates, and manages all Plugins
 *
 * NOTES:
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
#include "PluginMgr.h"
#include "BlynkPlugin.h"
#include "GenericPlugin.h"
//--------------- End:    Includes ---------------------------------------------


/*------------------------------------------------------------------------------
 *
 * Internal Utility Functions
 *
 *----------------------------------------------------------------------------*/

// SPIFFS is being deprecated which causes warnings at compile time. I have a task to move off
// of SPIFFS to LittleFS, but in the mean time, I don't want to keep seeing the warnings so
// I wrapped the SPIFFS calls with pragma's to avoid them
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  inline File FS_open(const char* path, const char* mode) { return SPIFFS.open(path, mode); }
  inline File FS_open(const String& path, const char* mode) { return SPIFFS.open(path, mode); }

  inline bool FS_exists(const char* path) { return SPIFFS.exists(path); }
  inline bool FS_exists(const String& path) { return SPIFFS.exists(path); }

  inline Dir FS_openDir(const char* path) { return SPIFFS.openDir(path); }
  inline Dir FS_openDir(const String& path) { return SPIFFS.openDir(path); }
#pragma GCC diagnostic pop

DynamicJsonDocument* PluginMgr::getDoc(String filePathString, uint16_t maxFileSize) {
  const char *filePath = filePathString.c_str();
  File file = FS_open(filePath, "r");
  if (!file) {
    Log.warning(F("Error opening: %s"), filePath);
    return NULL;
  }

  DynamicJsonDocument *doc = new DynamicJsonDocument(maxFileSize);
  auto error = deserializeJson(*doc, file);
  file.close();
  if (error) {
    delete doc;
    Log.warning(F("Failed to parse %s: %s"), filePath, error.c_str());
    return NULL;
  }

  return doc;
}


/*------------------------------------------------------------------------------
 *
 * Private Functions
 *
 *----------------------------------------------------------------------------*/

void PluginMgr::newPlugin(String pluginPath) {
Log.verbose("Trying to create plugin for %s", pluginPath.c_str());
  if (_nPlugins == PluginMgr::MaxPlugins) {
    Log.warning(F("Maximum number of plugins exceeded"));
  }

  DynamicJsonDocument* doc = getDoc(pluginPath + "/plugin.json", 2048);
  if (doc == NULL) return;
  String type = (*doc)[F("type")].as<String>();
  String name = (*doc)[F("name")].as<String>();
  String piNamespace = (*doc)[F("namespace")].as<String>();
  delete doc;

  Plugin *p = NULL;
  if (type.equalsIgnoreCase("generic")) {
    p = new GenericPlugin();
  } else if (type.equalsIgnoreCase("blynk")) {
    p = new BlynkPlugin();
  }
  if (p == NULL) return;

Log.verbose("Initializing plugin %s", name.c_str());
  if (!p->init(name, piNamespace, pluginPath)) {
    delete p;
    return;
  }
Log.verbose("completed setting up %s", name.c_str());

  _plugins[_nPlugins++] = p;
} 

bool PluginMgr::validatePluginFiles(String pluginPath) {
  // Make sure all the required files exist, or don't bother going any further
  if (FS_exists(pluginPath + "/plugin.json") &&
      FS_exists(pluginPath + "/form.json") &&
      FS_exists(pluginPath + "/settings.json") &&
      FS_exists(pluginPath + "/screen.json")) return true;

  Log.warning(F("For plugin %s, not all json files are present"), pluginPath.c_str());
  return false;
}

/*------------------------------------------------------------------------------
 *
 * Public Functions
 *
 *----------------------------------------------------------------------------*/

void PluginMgr::loadAll(String pluginRoot) {
  _nPlugins = 0;
  String pluginDirNames[MaxPlugins];
  uint8_t nPluginsFound = 0;

  uint8_t lengthOfPIPath = pluginRoot.length();
  if (pluginRoot[lengthOfPIPath-1] != '/') { pluginRoot += '/'; lengthOfPIPath++; }
  // Because of the way SPIFFS works (or actually because of the way it doesn't work)
  // We need to take a circuitous path to finding all the well-formed plugins. We can't
  // really enumerate directories (they are a convenient fiction in SPIFFS) nor can we
  // depend on the order of enumeration being breadth-first OR depth-first.
  // Instead, we just enumerate all the files under pluginRoot and keep track of
  // the unique subdirectories

  Dir nextFile = FS_openDir(pluginRoot);

  while (nPluginsFound < MaxPlugins && nextFile.next()) {
    String path = nextFile.fileName();
    int firstSlash = path.indexOf('/', lengthOfPIPath);
    if (firstSlash == -1) continue; // Not in a subdirectory;
    String pluginDirName = path.substring(lengthOfPIPath, firstSlash);
    int i;
    for (i = 0; i < nPluginsFound; i++) {
      if (pluginDirNames[i] == pluginDirName) break;
    }
    if (i == nPluginsFound && validatePluginFiles(pluginRoot+pluginDirName)) {
      pluginDirNames[nPluginsFound++] = pluginDirName;
    }
  }

  // Sort the plugins by name
  for (size_t i = 1; i < nPluginsFound; i++) {
    for (size_t j = i; j > 0 && (pluginDirNames[j-1] >  pluginDirNames[j]); j--) {
      String tmp = pluginDirNames[j-1];
      pluginDirNames[j-1] = pluginDirNames[j];
      pluginDirNames[j] = tmp;
    }
  }

  // Now that we have a list of unique plugin subdirectories, enumerate and load them
  for (int i = 0; i < nPluginsFound; i++) newPlugin(pluginRoot + pluginDirNames[i]);
}

void PluginMgr::refreshAll(bool force) {
  for (int i = 0; i < _nPlugins; i++) {
    _plugins[i]->refresh(force);
  }
}

uint8_t PluginMgr::getPluginCount() { return _nPlugins; }

Plugin* PluginMgr::getPlugin(uint8_t index) {
  if (index > _nPlugins) return NULL;
  return _plugins[index];
}

void PluginMgr::map(const String& key, String& value) {
  int indexOfSeparator = key.indexOf('.');
  String piNamespace = key.substring(0, indexOfSeparator);
  for (int i = 0; i < _nPlugins; i++) {
    Plugin *p = _plugins[i];
    if (p->getNamespace() == piNamespace) {
      if (p->enabled()) { p->typeSpecificMapper(key.substring(indexOfSeparator+1), value); }
      return;
    }
  }
}




