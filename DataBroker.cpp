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
#include <GenericESP.h>
//                                  Local Includes
#include "MultiMon.h"
#include "DataBroker.h"
//--------------- End:    Includes ---------------------------------------------

namespace MM = MultiMon;

namespace DataBroker {

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

    void mapPrinterSpecific(const String& key, String& value, int printerIndex) {
      if (printerIndex > MM::MaxPrinters) return;
      PrintClient *p = MM::printer[printerIndex];
      PrinterSettings *ps = &MM::settings.printer[printerIndex];
      bool active = ps->isActive;

      if (key.equalsIgnoreCase("name")) {
        if (!ps->nickname.isEmpty()) { value += ps->nickname; }
        else if (!ps->server.isEmpty()) { value += ps->server; }
        else value += "Inactive";
        return;
      }

      if (key.equalsIgnoreCase("pct")) {
        if (active && p->getState() >= PrintClient::State::Complete) { value += (int)(p->getPctComplete()); }
        return;
      }

      if (key.equalsIgnoreCase("state")) {
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

      if (key.equalsIgnoreCase("status")) {
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

      if (key.equalsIgnoreCase("next")) {
        if (!active) return;
        if (p->isPrinting()) Printing::completionTime(value, p->getPrintTimeLeft());
        return;
      }

      if (key.equalsIgnoreCase("remaining")) {
        if (active && p->getState() == PrintClient::State::Printing) {
          value = WebThing::formattedInterval(p->getPrintTimeLeft(), true, true);
        }
        return;
      }
    }

    void map(const String& key, String& value) {
      // Map printer related keys
      if (key.equalsIgnoreCase("next")) {
        uint32_t delta;
        String printer, formattedTime;
        nextCompletion(printer, formattedTime, delta);
        if (printer.isEmpty()) value += F("No print in progress");
        value += printer; value += ": "; value += formattedTime;
        return;
      }

      // Check for printer-specific keys
      if (isDigit(key[0]) && key[1] == '.') {
        int index = (key[0] - '0') - 1;
        String subkey = key.substring(2);
        mapPrinterSpecific(subkey, value, index);
        return;
      }
    }
  } // ----- END: Databroker::Printing namespace

  namespace System {
    void map(const String& key, String& value) {
      if (key.equalsIgnoreCase("time")) {
        char buf[9];
        time_t theTime = now();
        sprintf(buf, "%2d|%2d|%2d", hourFormat12(theTime), minute(theTime), second(theTime));
        value += buf;
      }
      else if (key.equalsIgnoreCase("author")) value += F("Joe Pasqua");
      else if (key.equalsIgnoreCase("heap")) {
        value += F("Heap: Free=");
        value += GenericESP::getFreeHeap();
        value += ", Frag=";
        value += GenericESP::getHeapFragmentation();
        value += '%';
      }
    }
  } // ----- END: Databroker::System namespace


  namespace Weather {
    void map(const String& key, String& value) {
      if (MM::owmClient == NULL) return;
      if (key.equalsIgnoreCase("temp")) value += MM::owmClient->weather.readings.temp;
      else if (key.equalsIgnoreCase("city")) value += MM::owmClient->weather.location.city;
      else if (key.equalsIgnoreCase("desc")) value += MM::owmClient->weather.description.basic;
      else if (key.equalsIgnoreCase("ldesc")) value += MM::owmClient->weather.description.longer;
    }
  } // ----- END: Databroker::Weather namespace


  // Upon entering this function, value is an empty String
  void map(const String& key, String& value) {
    // Keys are of the form: $P.subkey, where P is a prefix character indicating the namespace
    if (key.length() < 4 || key[0] != '$' || key[2] != '.') return;
    char prefix = key[1];

    String subkey = key.substring(3);

    switch (prefix) {
      case 'S': System::map(subkey, value); break;
      case 'P': Printing::map(subkey, value); break;
      case 'W': Weather::map(subkey, value); break;
      case 'E': MM::pluginMgr.map(subkey, value); break;
    }
  }
};