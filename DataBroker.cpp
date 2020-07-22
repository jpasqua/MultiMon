/*
 * DataBroker:
 *    A centralized point to get data from all available sources
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoLog.h>
#include <TimeLib.h>
//                                  Local Includes
#include "MultiMon.h"
#include "DataBroker.h"
//--------------- End:    Includes ---------------------------------------------

namespace MM = MultiMon;

namespace DataBroker {
  String EmptyString = "";

  namespace Printing {
    void nextCompletion(String &printer, String &formattedTime, uint32_t &delta) {
      uint32_t minCompletion = UINT32_MAX;
      int printerWithNextCompletion;
      for (int i = 0; i < MM::MaxServers; i++) {
        if (!MM::settings.printer[i].isActive) continue;
        if (MM::printer[i]->getState() == PrintClient::State::Printing) {
          uint32_t thisCompletion = MM::printer[i]->getPrintTimeLeft();
          if (thisCompletion < minCompletion) {
            minCompletion = thisCompletion;
            printerWithNextCompletion = i;
          }
        }
      }

      if (minCompletion != UINT32_MAX) {
        PrinterSettings *ps = &MM::settings.printer[printerWithNextCompletion];
        time_t theTime = now() + minCompletion;

        delta = minCompletion;
        printer =  (ps->nickname.isEmpty()) ? formattedTime = ps->server : ps->nickname;
        formattedTime = String(dayShortStr(weekday(theTime)));
        formattedTime += " ";
        formattedTime += (MM::settings.use24Hour) ? hour(theTime) : hourFormat12(theTime);
        formattedTime += ":";
        int theMinute =  minute(theTime);
        if (theMinute < 10) formattedTime += '0';
        formattedTime += theMinute;
        if (!MM::settings.use24Hour) formattedTime += isAM(theTime) ? "AM" : "PM";
      } else {
        printer = "";
        formattedTime = "";
        delta = 0;
      }
    }
  };

  String map(String& key) {
Log.verbose("DataBroker::map(%s)", key.c_str());
    int length = key.length();
    if (length < 3) return EmptyString;
    if (key[0] != '$') return EmptyString;
    int index = key.indexOf('.');
    if (index < 1 || index == length - 1) return EmptyString;

    String prefix = key.substring(1, index);
    String name = key.substring(index+1);

    if (prefix.equalsIgnoreCase("S")) {
      // Map system-related keys
      if (name.equalsIgnoreCase("author")) return String(F("Joe Pasqua"));
      if (name.equalsIgnoreCase("heap"))
        return ("Heap: Free="+String(ESP.getFreeHeap())+", Frag="+String(ESP.getHeapFragmentation())+"%");
      // ...
    } else if (prefix.equalsIgnoreCase("W")) {
      // Map weather-related keys
      if (name.equalsIgnoreCase("temp")) return String(MM::owmClient->weather.readings.temp);
      if (name.equalsIgnoreCase("city")) return MM::owmClient->weather.location.city;
      if (name.equalsIgnoreCase("desc")) return MM::owmClient->weather.description.basic;
      if (name.equalsIgnoreCase("ldesc")) {
        return MM::owmClient->weather.description.longer;
      }
    } else if (prefix.equalsIgnoreCase("P")) {
      // Map printer related keys
      if (name.equalsIgnoreCase("next")) {
        uint32_t delta;
        String printer, formattedTime;
        Printing::nextCompletion(printer, formattedTime, delta);
        if (printer.isEmpty()) return "No print in progress";
        return printer + ": " + formattedTime;
      }
    }
    return EmptyString;
  }
};