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
//--------------- End:    Includes ---------------------------------------------



namespace MultiMon {
  // ----- Constants
  static const String VersionString = "0.5";
  static const int MaxServers = 4;
  static const int8_t Pin_LEDBrightnessControl = D4;  // -1 for no pin

  // ----- State
  extern MMSettings settings;
  extern PrintClient *printer[MultiMon::MaxServers];
  extern OWMClient *owmClient;

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