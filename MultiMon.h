#ifndef MultiMon_h
#define MultiMon_h

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <WebThing.h>
//                                  Local Includes
#include "MMSettings.h"
#include "src/Basics.h"
#include "src/clients/OctoClient.h"
#include "src/clients/OWMClient.h"
#include "src/plugins/PluginMgr.h"
//--------------- End:    Includes ---------------------------------------------



namespace MultiMon {
  // ----- Constants
  static const String VersionString = "0.3.0";
  static const int MaxPrinters = 4;

  // ----- State
  extern MMSettings settings;
  extern PrintClient *printer[MultiMon::MaxPrinters];
  extern OWMClient *owmClient;
  extern PluginMgr pluginMgr;
  
  // ----- Exported Functions
  namespace Protected {
    void askToReboot();
    void configMayHaveChanged();
    void weatherConfigMayHaveChanged();
    void updateAllData();
    void updatePrinterData();
    void updateWeatherData();
    void saveSettings();
    void printerWasActivated(int index);
  }
}

#endif  // MultiMon_h