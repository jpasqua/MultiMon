/*
 * MMDataSupplier:
 *    Supplies app-specific data to the WebThing DataBroker
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoLog.h>
#include <TimeLib.h>
#include <WebThing.h>
//                                  Local Includes
#include "MultiMonApp.h"
#include "MMDataSupplier.h"
//--------------- End:    Includes ---------------------------------------------


namespace MMDataSupplier {
  
  namespace Printing {
    void completionTime(String &formattedTime, uint32_t timeLeft) {
      time_t theTime = now() + timeLeft;
      formattedTime = String(dayShortStr(weekday(theTime)));
      formattedTime += " ";
      formattedTime += (wtApp->settings->uiOptions.use24Hour) ? hour(theTime) : hourFormat12(theTime);
      formattedTime += ":";
      int theMinute =  minute(theTime);
      if (theMinute < 10) formattedTime += '0';
      formattedTime += theMinute;
      if (!wtApp->settings->uiOptions.use24Hour) formattedTime += isAM(theTime) ? "AM" : "PM";
    }

    void nextCompletion(String &printer, String &formattedTime, uint32_t &delta) {
      uint32_t minCompletion = UINT32_MAX;
      int printerWithNextCompletion;
      for (int i = 0; i < MultiMonApp::MaxPrinters; i++) {
        if (!mmSettings->printer[i].isActive) continue;
        if (mmApp->printer[i]->getState() == PrintClient::State::Printing) {
          uint32_t thisCompletion = mmApp->printer[i]->getPrintTimeLeft();
          if (thisCompletion < minCompletion) {
            minCompletion = thisCompletion;
            printerWithNextCompletion = i;
          }
        }
      }

      if (minCompletion != UINT32_MAX) {
        PrinterSettings *ps = &mmSettings->printer[printerWithNextCompletion];
        printer =  (ps->nickname.isEmpty()) ? ps->server : ps->nickname;
        delta = minCompletion;
        completionTime(formattedTime, delta);
      } else {
        printer = "";
        formattedTime = "";
        delta = 0;
      }
    }

    void mapPrinterSpecific(const String& key, String& value, int printerIndex) {
      if (printerIndex > MultiMonApp::MaxPrinters) return;
      PrintClient *p = mmApp->printer[printerIndex];
      PrinterSettings *ps = &mmSettings->printer[printerIndex];
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
  }
    void printerDataSupplier(const String& key, String& value) {
      // Map printer related keys
      if (key.equalsIgnoreCase("next")) {
        uint32_t delta;
        String printer, formattedTime;
        Printing::nextCompletion(printer, formattedTime, delta);
        if (printer.isEmpty()) value += F("No print in progress");
        value += printer; value += ": "; value += formattedTime;
        return;
      }

      // Check for printer-specific keys
      if (isDigit(key[0]) && key[1] == '.') {
        int index = (key[0] - '0') - 1;
        String subkey = key.substring(2);
        Printing::mapPrinterSpecific(subkey, value, index);
        return;
      }
    }
};