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
#include <ArduinoJSON.h>
#include <BaseSettings.h>
//                                  Local Includes
//--------------- End:    Includes ---------------------------------------------


static const String Type_Octo = "OctoPrint";
static const String Type_Duet = "Duet3D";

class PrinterSettings {
public:
  PrinterSettings();
  void init();

  void fromJSON(JsonObjectConst settings);
  void toJSON(JsonObject settings) const;
  void logSettings();

  String type;    // Must be either "OctoPrint" or "Duet3D"
  String apiKey;
  String server;
  int port;
  String user;
  String pass;
  String nickname;
  bool isActive;
  bool mock;
};

class MMSettings: public BaseSettings {
public:
  // ----- Constructors and methods
  MMSettings();
  void fromJSON(JsonDocument &doc) override;
  void toJSON(JsonDocument &doc);
  void logSettings();

  // ----- Settings
  static const uint8_t MaxPrinters = 4;
  PrinterSettings printer[MaxPrinters];
  uint32_t printerRefreshInterval = 10;

  // Display Options
  bool use24Hour = false;             // 23:00 military 24 hour clock
  bool useMetric = false;
  bool showDevMenu = false;

  // HW Settings
  bool     invertDisplay = false;      // true = pins at top | false = pins at the bottom
  static const int nCalReadings = 5;
  uint16_t calibrationData[nCalReadings];

  struct {
    bool    enabled = true;           // Get weather data from OpenWeatherMap?
    String  key = "";                 // Your API Key from http://openweathermap.org/
    int     cityID =  5372223;        // OpenWeatherMap City ID
    String  language = "en";          // See OpenWeatherMap for list of available languages
  } owm;
  
  bool scheduleActive;                // Is the scheduling system active?
  struct {
    uint8_t hr;                       // Hour that the period starts (24 hour time)
    uint8_t min;                      // Minute that the period starts
    uint8_t brightness;               // The brightness level during this period
  } morning, evening;

private:
  // ----- Constants -----
  static const uint32_t CurrentVersion;
};
#endif // MMSettings_h