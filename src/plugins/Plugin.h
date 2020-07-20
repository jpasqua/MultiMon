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
    // Return a serialized version of the JSON doc representing the settings
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
  Basics::StringMapper _mapper;
    // Implemented by the concrete subclass. Maps names in the UI description
    // to values provided by one or more data sources

private:
  static const uint32_t MaxScreenDescriptorSize = 6*1024;
};

#endif // Plugin_h
