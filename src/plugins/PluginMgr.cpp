/*
 * PluginMgr
 *    Finds, instantiates, and manages all Plugins
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
#include "../util/ESP_FS.h"
//--------------- End:    Includes ---------------------------------------------


/*------------------------------------------------------------------------------
 *
 * Internal Utility Functions
 *
 *----------------------------------------------------------------------------*/

DynamicJsonDocument* PluginMgr::getDoc(String filePathString, uint16_t maxFileSize) {
  const char *filePath = filePathString.c_str();
  File file = ESP_FS::open(filePath, "r");
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
  Log.verbose("Validating %s", pluginPath.c_str());
  // Make sure all the required files exist, or don't bother going any further
  if (ESP_FS::exists(pluginPath + "/plugin.json") &&
      ESP_FS::exists(pluginPath + "/form.json") &&
      ESP_FS::exists(pluginPath + "/settings.json") &&
      ESP_FS::exists(pluginPath + "/screen.json")) return true;

  Log.warning(F("For plugin %s, not all json files are present"), pluginPath.c_str());
  return false;
}

uint8_t PluginMgr::enumPlugins(String& pluginRoot, String* pluginDirNames) {
  uint8_t nPluginsFound = 0;
  uint8_t lengthOfPIPath = pluginRoot.length();

  if (!ESP_FS::beginFileList(pluginRoot)) {
    Log.warning("The specified plugin path (%s) is not a directory", pluginRoot);
    return 0;
  }

  String path;
  while (nPluginsFound < MaxPlugins && ESP_FS::getNextFileName(path)) {
    int firstSlash = path.indexOf('/', lengthOfPIPath+2);
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
  return nPluginsFound;
}


/*------------------------------------------------------------------------------
 *
 * Public Functions
 *
 *----------------------------------------------------------------------------*/


void PluginMgr::loadAll(String pluginRoot) {
  _nPlugins = 0;
  String  pluginDirNames[MaxPlugins];
  uint8_t nPluginsFound = enumPlugins(pluginRoot, &pluginDirNames[0]);

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




