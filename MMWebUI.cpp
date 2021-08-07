/*
 * MMWebUI:
 *    Implements the WebUI for MultiMon
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoLog.h>
//                                  WebThing Includes
#include <WebThingBasics.h>
#include <WebUI.h>
#include <WebUIHelper.h>
#include <gui/Display.h>
//                                  Local Includes
#include "MultiMonApp.h"
#include "MMWebUI.h"
//--------------- End:    Includes ---------------------------------------------


// ----- BEGIN: WebUI namespace
namespace MMWebUI {

  // ----- BEGIN: MMWebUI::Internal
  namespace Internal {
    String CUSTOM_ACTIONS =
      "<a class='w3-bar-item w3-button' href='/presentPrinterConfig'>"
      "<i class='fa fa-cog'></i> Configure Printers</a>";

    void updateSinglePrinter(int i) {
      PrinterSettings* printer = &(mmSettings->printer[i]);
      String prefix = "_p" + String(i) + "_";
      printer->isActive = WebUI::hasArg(prefix + "enabled");
      printer->apiKey = WebUI::arg(prefix + "key");
      printer->server = WebUI::arg(prefix + "server");
      printer->port = WebUI::arg(prefix + "port").toInt();
      printer->user =  WebUI::arg(prefix + "user");
      printer->pass =  WebUI::arg(prefix + "pass");
      printer->nickname =  WebUI::arg(prefix + "nick");
      printer->type =  WebUI::arg(prefix + "type");
    }
  } // ----- END: MMWebUI::Internal


  // ----- BEGIN: MMWebUI::Endpoints
  namespace Endpoints {
    void updatePrinterConfig() {
      if (!WebUI::authenticationOK()) { return; }

      for (int i = 0; i < 4; i++) {
        PrinterSettings* printer = &(mmSettings->printer[i]);
        bool wasActive = printer->isActive;
        Internal::updateSinglePrinter(i);
        if (!wasActive && printer->isActive) mmApp->printerWasActivated(i);
      }
      mmSettings->printerRefreshInterval = WebUI::arg(F("refreshInterval")).toInt();
      wtApp->settings->write();
      // wtApp->settings->logSettings();

      // Act on changed settings...
      wtAppImpl->configMayHaveChanged();
      WebUI::redirectHome();
    }
  }   // ----- END: MMWebUI::Endpoints

  // ----- BEGIN: MMWebUI::Dev
  namespace Dev {
    void presentDevConfig() {
      Log.trace(F("Web Request: Handle Dev Configure"));
      if (!WebUI::authenticationOK()) { return; }

      auto mapper =[](String &key) -> String {
        if (key.startsWith("_P")) {
          int i = (key.charAt(2) - '0');
          PrinterSettings* printer = &(mmSettings->printer[i]);
          key.remove(0, 4); // Get rid of the prefix; e.g. _P1_
          if (key.equals(F("NICK"))) return printer->nickname;
          if (key.equals(F("MOCK"))) {
            return WebUIHelper::checkedOrNot[printer->mock];
          }
          if (key.equals(F("SERVER"))) return printer->server;
        }
        return WTBasics::EmptyString;
      };

      WebUI::startPage();
      WebUIHelper::templateHandler->send("/ConfigDev.html", mapper);
      WebUI::finishPage();
    }

    void updateDevConfig() {
      if (!WebUI::authenticationOK()) { return; }
      PrinterSettings* printer = &(mmSettings->printer[0]);
      printer[0].mock = WebUI::hasArg(F("_p0_mock"));
      printer[1].mock = WebUI::hasArg(F("_p1_mock"));
      printer[2].mock = WebUI::hasArg(F("_p2_mock"));
      printer[3].mock = WebUI::hasArg(F("_p3_mock"));

      // Save the config, but don't change which printers are mocked until reboot
      wtApp->settings->write();
      WebUI::redirectHome();
    }
  }   // ----- END: MMWebUI::Dev


  namespace Pages {
    void presentHomePage() {
      Log.trace(F("Web Request: Display Home Page"));
      if (!WebUI::authenticationOK()) { return; }

      auto mapper =[](String &key) -> String {
        if (key.startsWith("_P")) {
          int i = (key.charAt(2) - '0');
          PrinterSettings* printer = &(mmSettings->printer[i]);
          key.remove(0, 4); // Get rid of the prefix; e.g. _P1_
          if (printer->isActive) {
            if (key.equals(F("VIS"))) return "block";
            if (key.equals(F("HOST"))) return printer->server;
            if (key.equals(F("PORT"))) return String(printer->port);
            if (key.equals(F("AUTH"))) {
              if (!printer->user.isEmpty()) return printer->user + ':' + printer->pass + '@';
              else return WTBasics::EmptyString;
            }
            if (key.equals(F("NICK"))) return  printer->nickname;
          } else {
            if (key.equals(F("VIS"))) return "none";
          }
        }
        if (key.equals(F("CITYID"))) {
          if (wtApp->settings->owmOptions.enabled) return String(wtApp->settings->owmOptions.cityID);
          else return String("5380748");  // Palo Alto, CA, USA
        }
        if (key.equals(F("WEATHER_KEY"))) return wtApp->settings->owmOptions.key;
        if (key.equals(F("UNITS"))) return wtApp->settings->uiOptions.useMetric ? "metric" : "imperial";
        if (key.equals(F("BRIGHT"))) return String(Display::getBrightness());
        return WTBasics::EmptyString;
      };

      WebUI::startPage();
      WebUIHelper::templateHandler->send("/HomePage.html", mapper);
      WebUI::finishPage();
    }

    void presentPrinterConfig() {
      Log.trace(F("Web Request: Handle Printer Configure"));
      if (!WebUI::authenticationOK()) { return; }

      auto mapper =[](String &key) -> String {
        if (key.startsWith("_P")) {
          int i = (key.charAt(2) - '0');
          PrinterSettings* printer = &(mmSettings->printer[i]);
          key.remove(0, 4); // Get rid of the prefix; e.g. _P1_
          String type = "T_" + printer->type;
          if (key.equals(F("ENABLED"))) return WebUIHelper::checkedOrNot[printer->isActive];
          if (key.equals(F("KEY"))) return printer->apiKey;
          if (key.equals(F("HOST"))) return  printer->server;
          if (key.equals(F("PORT"))) return String(printer->port);
          if (key.equals(F("USER"))) return  printer->user;
          if (key.equals(F("PASS"))) return  printer->pass;
          if (key.equals(F("NICK"))) return  printer->nickname;
          if (key == type) return "selected";
        }
        if (key.equals(F("RFRSH"))) return String(mmSettings->printerRefreshInterval);
        return WTBasics::EmptyString;
      };

      WebUI::startPage();
      WebUIHelper::templateHandler->send("/ConfigPrinters.html", mapper);
      WebUI::finishPage();
    }
  }   // ----- END: MMWebUI::Pages


  void init() {
    WebUIHelper::init(Internal::CUSTOM_ACTIONS);

    WebUI::registerHandler("/",                       Pages::presentHomePage);
    WebUI::registerHandler("/presentPrinterConfig",   Pages::presentPrinterConfig);
    WebUI::registerHandler("/updatePrinterConfig",    Endpoints::updatePrinterConfig);
    WebUI::registerHandler("/dev",                    Dev::presentDevConfig);
    WebUI::registerHandler("/dev/updateDevData",      Dev::updateDevConfig);
  }

}
// ----- END: MMWebUI