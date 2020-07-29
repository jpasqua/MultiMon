#ifndef Plugin_h
#define Plugin_h

/*
 * Plugin.h
 *    A method for adding enw functionality to MultiMon.
 *
 * NOTES:
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
#include <ArduinoJson.h>
//                                  Local Includes
#include "../Basics.h"
//--------------- End:    Includes ---------------------------------------------


class Plugin {
public:
  // Implemented by subclasses
  virtual ~Plugin() { }
  virtual bool typeSpecificInit() = 0;
  virtual String typeSpecificMapper(String& key) = 0;
	virtual void refresh(bool force = false) = 0;
  virtual void getSettings(String &settings) = 0;
  virtual void newSettings(String &settings) = 0;
  virtual uint32_t getUIRefreshInterval() = 0;

  // Implemented by Plugin class
  bool init(String& name, String& pluginDir);
    // Called immediately after the Plugin's constructor. Will invoke
    // typeSpecificInit() after _name and _pluginDir are set
  void getForm(String& form);
    // Return a serialized version of the JSON doc representing the screen
  String getName() { return _name; }
    // Return the UI name of the plugin
  String getScreenID() { return _screenID; }
    // Return the name of the associated screen
  bool enabled() { return _enabled; }
    // Is this plugin enabled?

protected:
  String   _name;
    // Read from the plugin.json file and set in the init() function
  String _pluginDir;
    // The directory from which the plugin was loaded. Set in the init() function
  bool _enabled;
    // Is this plugin enabled. This may always be true, may be a setting, or may be
    // set based on some operational parameters
  String _screenID;

private:
  static const uint32_t MaxScreenDescriptorSize = 6*1024;
  Basics::StringMapper _mapper;
};

#endif // Plugin_h
