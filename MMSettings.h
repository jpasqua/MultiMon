/*
 * MMSettings.h
 *    Defines the values that can be set through the web UI and sets their initial values
 *
 */

#ifndef MMSettings_h
#define MMSettings_h

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
#include <BPA_PrinterSettings.h>
//                                  WebThing
#include <WTAppSettings.h>
//                                  Local Includes
//--------------- End:    Includes ---------------------------------------------


class MMSettings: public WTAppSettings {
public:
  // ----- Constructors and methods
  MMSettings();
  void fromJSON(const JsonDocument &doc) override;
  void toJSON(JsonDocument &doc);
  void logSettings();

  static constexpr uint8_t MaxPrinters = 4;
  PrinterSettings printer[MaxPrinters];
  uint32_t printerRefreshInterval = 10;

private:
  // ----- Constants -----
  static constexpr uint32_t CurrentVersion = 0x0003;

};
#endif // MMSettings_h