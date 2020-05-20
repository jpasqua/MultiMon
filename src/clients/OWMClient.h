#ifndef OWMClient_h
#define OWMClient_h

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoLog.h>
#include <TimeLib.h>
//                                  Local Includes
//--------------- End:    Includes ---------------------------------------------


class Weather {
public:
  struct {
    float   lat;          // Data is for this location: latitude
    float   lon;          // Data is for this location: longitude
    String   country;     // Name of containing country
    String   city;        // Name of containing country
    uint32_t cityID;      // ID of City
  } location;
  struct {
    int16_t  tz;          // Timezone offset in seconds
    uint32_t sunrise;     // Time of sunrise
    uint32_t sunset;      // Time of sunset
  } time;
  struct {
    String   basic;       // One word description
    String   longer;      // A little more detail
    String   icon;        // Specifier for the associated weather icon
    uint16_t code;        // See: https://openweathermap.org/weather-conditions#Weather-Condition-Codes-2
  } description;
  struct {
    float    temp;        // degrees (metric or imperial)
    float    feelsLike;   // degrees (metric or imperial)
    float    minTemp;     // degrees (metric or imperial)
    float    maxTemp;     // degrees (metric or imperial)
    uint16_t pressure;    // hPa (always hPa regardless of units setting)
    uint8_t  humidity;    // Percent humidity
    float    windSpeed;   // Meters/sec
    uint16_t visibility;  // meters (always meters regardless of units setting)
    uint8_t  cloudiness;  // How cloudy is it 0-100%
  } readings;
  uint32_t dt;            // Time that the data was calculated
  bool cached;            // Is this cached data? (ie a subsequent read failed)
  String error;           // Error reported by OWM

  void dumpToLog() {
    Log.verbose("----- Location -----");
    Log.verbose("  Coords = (%F, %F)", location.lat, location.lon);
    Log.verbose("  %s, %s (%d)", location.country.c_str(), location.city.c_str(), location.cityID);
    Log.verbose("----- Time Related -----");
    Log.verbose("  TZOffset: %d, sunrise: %d, sunset: %d", time.tz, time.sunrise, time.sunset);
    Log.verbose("----- Description -----");
    Log.verbose("  %s (%s)", description.basic.c_str(), description.longer.c_str());
    Log.verbose("  Icon: %s, Code: %d", description.icon.c_str(), description.code);
    Log.verbose("----- Readings -----");
    Log.verbose("  temp: %F, feelsLike: %F", readings.temp, readings.feelsLike);
    Log.verbose("  MinTemp: %F, MaxTemp: %F", readings.minTemp, readings.maxTemp);
    Log.verbose("  Pressure: %d, Humidity: %d", readings.pressure, readings.humidity);
    Log.verbose("  Wind Speed: %F, Visibility: %d, Cloudiness: %d", readings.windSpeed, readings.visibility, readings.cloudiness);
    Log.verbose("----- Response data -----");
    Log.verbose("  Time of readings: %d", dt);
    Log.verbose("  Cached: %T", cached);
    Log.verbose("  Error: %s", error.c_str());
  }
};

class Forecast {
public:
  static constexpr float NoReading = -9999.0;
  uint32_t dt;            // Time that the data was calculated
  float    hiTemp;        // The highest temp of the forecast period (degrees metric or imperial)
  float    loTemp;        // The lowest temp of the forecast period (degrees metric or imperial)
  String   icon;          // Specifier for the associated weather icon

  void dumpToLog() {
    Log.verbose("----- Forecast Time: %d-%d %d:%d (%d)", month(dt), day(dt), hour(dt), minute(dt), dt);
    Log.verbose("  low: %F, hi: %F", loTemp, hiTemp);
    Log.verbose("  Icon: %s", icon.c_str());
  }

};

class OWMClient {
public:
  static const uint8_t ForecastElements = 6;

  Weather weather;

  OWMClient(String key, int cityID, bool useMetric, String language);
  void update();
  void updateForecast(int32_t gmtOffset);
  inline Forecast *getForecast() { return &forecast[0]; }

  // Convenience functions - getters and setters
  void setKey(String key) { _key = key; }
  void setLanguage(String language) { _lang = language; }
  void setUnits(bool useMetric) { _useMetric = useMetric; }

  String getCityName() { return weather.location.city; }
  String getCountryName() { return weather.location.country; }
  String getError() { return weather.error; }
  String getOneWordDescription() { return weather.description.basic; }
  String getLongerDescription() { return weather.description.longer; }
  String getIcon() {  return weather.description.icon; }

private:
  String _cityID = "";
  String _key = "";
  bool   _useMetric = false;
  String _lang = "";
  Forecast forecast[ForecastElements];

  void dumpForecast();
};

#endif  // OWMClient_h