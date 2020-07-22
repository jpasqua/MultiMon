/*
 * Plugin
 *    A method for adding new functionality to MultiMon.
 *
 * NOTES:
 * o A plugin consists of multiple parts:
 *   + A subclass of Plugin that can is specific to some data or activity
 *   + A screen definition which can be materialized by FlexScreen. This takes
 *     the form of a JSON file which describes the screen layout and look
 *   + A source for data to be provided to the plugin (e.g. a news source,
 *     a feed of weather info, data from some other IoT device)
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
//--------------- End:    Includes ---------------------------------------------


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  inline File FS_open(const char* path, const char* mode) { return SPIFFS.open(path, mode); }
#pragma GCC diagnostic pop

bool Plugin::init(String& name, String& pluginDir) {
  _name = name;
  _pluginDir = pluginDir;
  if (!typeSpecificInit()) return false;
  
  // Create the FlexScreen UI
  DynamicJsonDocument* doc = PluginMgr::getDoc(_pluginDir + "/screen.json", MaxScreenDescriptorSize);
  if (doc == NULL) return false;
  _screenName = GUI::createFlexScreen(*doc, getUIRefreshInterval(), _mapper);
  delete doc;
  return !_screenName.isEmpty();
}

void Plugin::getForm(String& form) {
  String fullPath = _pluginDir+"/form.json";
Log.verbose("PluginMgr::getForm() path = %s", fullPath.c_str());
  File f = FS_open(fullPath.c_str(), "r");
  if (!f) { form = "{ }"; }

  size_t size = f.size();
  form.reserve(size);

  while (f.available()) {
    String line = f.readString();
    if (!line.isEmpty()) form += line;
  }
  f.close();
}