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
    void completionTime(String &formattedTime, uint32_t timeLeft) {
      time_t theTime = now() + timeLeft;
      formattedTime = String(dayShortStr(weekday(theTime)));
      formattedTime += " ";
      formattedTime += (MM::settings.use24Hour) ? hour(theTime) : hourFormat12(theTime);
      formattedTime += ":";
      int theMinute =  minute(theTime);
      if (theMinute < 10) formattedTime += '0';
      formattedTime += theMinute;
      if (!MM::settings.use24Hour) formattedTime += isAM(theTime) ? "AM" : "PM";
    }

    void nextCompletion(String &printer, String &formattedTime, uint32_t &delta) {
      uint32_t minCompletion = UINT32_MAX;
      int printerWithNextCompletion;
      for (int i = 0; i < MM::MaxPrinters; i++) {
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

        printer =  (ps->nickname.isEmpty()) ? formattedTime = ps->server : ps->nickname;
        delta = minCompletion;
        completionTime(formattedTime, delta);
      } else {
        printer = "";
        formattedTime = "";
        delta = 0;
      }
    }
  };

  // Upon entering this function, value is an empty String
  void map(const String& key, String& value) {
    int length = key.length();
    if (length < 3 || key[0] != '$') return;
    int index = key.indexOf('.');
    if (index < 1 || index == length - 1) return;

    String prefix = key.substring(1, index);
    String name = key.substring(index+1);

    if (prefix.equalsIgnoreCase("S")) {
      // Map system-related keys
      if (name.equalsIgnoreCase("time")) {
        char timeValBuf[9];
        time_t theTime = now();
        sprintf(timeValBuf, "%2d|%2d|%2d", hourFormat12(theTime), minute(theTime), second(theTime));
        value += timeValBuf;
        return;
      }
      if (name.equalsIgnoreCase("author")) value = F("Joe Pasqua");
      if (name.equalsIgnoreCase("heap")) {
        value += F("Heap: Free=");
        value += ESP.getFreeHeap();
        value += ", Frag=";
        value += ESP.getHeapFragmentation();
        value += '%';
        return;
      }
    } else if (prefix.equalsIgnoreCase("W")) {
      // Map weather-related keys
      if (name.equalsIgnoreCase("temp")) value += MM::owmClient->weather.readings.temp; return;
      if (name.equalsIgnoreCase("city")) value += MM::owmClient->weather.location.city; return;
      if (name.equalsIgnoreCase("desc")) value += MM::owmClient->weather.description.basic; return;
      if (name.equalsIgnoreCase("ldesc")) value += MM::owmClient->weather.description.longer; return;
    } else if (prefix.equalsIgnoreCase("P")) {
      // Map printer related keys
      if (name.equalsIgnoreCase("next")) {
        uint32_t delta;
        String printer, formattedTime;
        Printing::nextCompletion(printer, formattedTime, delta);
        if (printer.isEmpty()) value += F("No print in progress");
        value += printer; value += ": "; value += formattedTime;
        return;
      }
      if (isDigit(name[0]) && name[1] == '.') {
        int index = (name[0] - '0') - 1;
        if (index > MM::MaxPrinters) return;
        PrintClient *p = MM::printer[index];
        PrinterSettings *ps = &MM::settings.printer[index];
        name.remove(0, 2);
        bool active = ps->isActive;

        if (name.equalsIgnoreCase("name")) {
          if (!ps->nickname.isEmpty()) { value += ps->nickname; }
          else if (!ps->server.isEmpty()) { value += ps->server; }
          else value += "Inactive";
          return;
        }

        if (name.equalsIgnoreCase("pct")) {
          if (active && p->getState() >= PrintClient::State::Complete) { value += (int)(p->getPctComplete()); }
          return;
        }

        if (name.equalsIgnoreCase("state")) {
          if (active) {
            switch (p->getState()) {
              case PrintClient::State::Offline: value += F("Offline"); break;
              case PrintClient::State::Operational: value += F("Online"); break;
              case PrintClient::State::Complete: value += F("Complete"); break;
              case PrintClient::State::Printing: value += F("Printing"); break;
            }
          } else value += F("Unused");
         return;
        }

        if (name.equalsIgnoreCase("status")) {
          int pct = 100;
          if (active) {
            switch (p->getState()) {
              case PrintClient::State::Offline: value += F("Offline"); break;
              case PrintClient::State::Operational: value += F("Online"); break;
              case PrintClient::State::Complete: value += F("Complete"); break;
              case PrintClient::State::Printing:
                value += "Printing";
                pct = (int)p->getPctComplete();
                break;
            }
          } else value += F("Unused");
          value += '|'; value += pct;
          return;
        }

        if (name.equalsIgnoreCase("next")) {
          if (!active) return;
          if (p->isPrinting()) Printing::completionTime(value, p->getPrintTimeLeft());
          return;
        }
      }
    }
    return;
  }
};