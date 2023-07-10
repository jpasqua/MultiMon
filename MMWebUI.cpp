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
#include <WebThing.h>
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
    const __FlashStringHelper* APP_MENU_ITEMS = FPSTR(
      "<a class='w3-bar-item w3-button' href='/presentPrinterConfig'>"
      "<i class='fa fa-glass'></i> Configure Printers</a>");

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
    void ackPrinterDone() {
      auto action = [&]() {
        if (!mmApp->printerGroup) {
          Log.warning("ackPrinterDone: printerGroup is null");
          WebUI::sendStringContent("text/plain", "printer index out of range", "400 Bad Request");
          return;
        }

        int printerIndex = WebUI::arg("pi").toInt();
        if (printerIndex < 0 || printerIndex >= mmApp->printerGroup->numberOfPrinters()) {
          Log.warning("ackPrinterDone: index out of range: %d vs %d",
            printerIndex, mmApp->printerGroup->numberOfPrinters());
          WebUI::sendStringContent("text/plain", "printer index out of range", "400 Bad Request");
          return;
        }

        auto printer = mmApp->printerGroup->getPrinter(printerIndex);
        if (printer) {
          printer->acknowledgeCompletion();
          WebUI::sendStringContent("text/plain", "Printer Completion Acknowledged");
        } else {
          Log.warning("ackPrinterDone: no printer for index %d", printerIndex);
          WebUI::sendStringContent("text/plain", "printer index out of range", "400 Bad Request");
        }
      };

      WebUI::wrapWebAction("/ackPrinterDone", action);
    }
    
    void updatePrinterConfig() {
      auto action = []() {
        for (int i = 0; i < 4; i++) {
          PrinterSettings* printer = &(mmSettings->printer[i]);
          bool wasActive = printer->isActive;
          Internal::updateSinglePrinter(i);
          if (!wasActive && printer->isActive) mmApp->printerWasActivated(i);
        }
        mmSettings->printerRefreshInterval = WebUI::arg(F("refreshInterval")).toInt();

        if (WebThing::settings.showDevMenu) {
          PrinterSettings* printer = &(mmSettings->printer[0]);
          printer[0].mock = WebUI::hasArg(F("_p0_mock"));
          printer[1].mock = WebUI::hasArg(F("_p1_mock"));
          printer[2].mock = WebUI::hasArg(F("_p2_mock"));
          printer[3].mock = WebUI::hasArg(F("_p3_mock"));
        }

        // Act on changed settings...
        wtAppImpl->configMayHaveChanged();
        wtApp->settings->write();
        WebUI::redirectHome();
      };
  
      WebUI::wrapWebAction("/updatePrinterConfig", action);
    }

  }   // ----- END: MMWebUI::Endpoints


  namespace Pages {
    void presentHomePage() {
      auto mapper =[](const String& key, String& val) -> void {
        // ----- Printer-related items
        if (key.equals(F("PRINTER_INFO"))) {
          mmApp->printerGroup->printerInfo(val);
          return;
        }

        // ----- Weather-related items
        if (key.equals(F("CITYID"))) {
          if (wtApp->settings->owmOptions.enabled) val = wtApp->settings->owmOptions.cityID;
          else val.concat("5380748");  // Palo Alto, CA, USA
        }
        else if (key.equals(F("WEATHER_KEY"))) val = wtApp->settings->owmOptions.key;
        else if (key.equals(F("UNITS"))) val = wtApp->settings->uiOptions.useMetric ? "metric" : "imperial";
      };

      WebUI::wrapWebPage("/", "/HomePage.html", mapper);
    }

    void presentPrinterConfig() {
      auto mapper =[](const String& key, String& val) -> void {
        if (key.startsWith("_P")) {
          int i = (key.charAt(2) - '0');
          PrinterSettings* printer = &(mmSettings->printer[i]);
          const char* subkey = &(key.c_str()[4]); // Get rid of the prefix; e.g. _P1_
          String type = "T_" + printer->type;
          if (strcmp(subkey, "ENABLED") == 0) val = WebUIHelper::checkedOrNot[printer->isActive];
          else if (strcmp(subkey, "KEY") == 0) val = printer->apiKey;
          else if (strcmp(subkey, "HOST") == 0) val =  printer->server;
          else if (strcmp(subkey, "PORT") == 0) val.concat(printer->port);
          else if (strcmp(subkey, "USER") == 0) val = printer->user;
          else if (strcmp(subkey, "PASS") == 0) val = printer->pass;
          else if (strcmp(subkey, "NICK") == 0) val = printer->nickname;
          else if (strcmp(subkey, "MOCK") == 0)  val = WebUIHelper::checkedOrNot[printer->mock];
          else if (type.equals(subkey)) { val = "selected"; } 
        }
        else if (key.equals("SHOW_DEV")) val = WebThing::settings.showDevMenu ? "true" : "false";
        else if (key.equals(F("RFRSH"))) val.concat(mmSettings->printerRefreshInterval);
      };

      WebUI::wrapWebPage("/presentPrinterConfig", "/ConfigPrinters.html", mapper);
    }
  }   // ----- END: MMWebUI::Pages


  void init() {
    WebUIHelper::init(Internal::APP_MENU_ITEMS);

    WebUI::registerHandler("/",                       Pages::presentHomePage);
    WebUI::registerHandler("/presentPrinterConfig",   Pages::presentPrinterConfig);
    WebUI::registerHandler("/updatePrinterConfig",    Endpoints::updatePrinterConfig);
    WebUI::registerHandler("/ackPrinterDone",         Endpoints::ackPrinterDone);
  }

}
// ----- END: MMWebUI