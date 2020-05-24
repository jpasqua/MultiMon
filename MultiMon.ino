/*
 * MultiMon:
 *    Use a color touch screen display to monitor for up to 4 OctoPrint servers
 *    Also displays time and weather information 
 *                    
 * TO DO:
 * o Web UI / GUI
 *   - Perhaps at some point allow "themes" to be set for colors used on the display.
 *     Probably not fonts since the layout is not flexible
 *   - Figure out the whole w3-row, w3-container model and apply across all pages
 * o Change the way mocking works. Take it out of the individual print client impls
 *   and create a MockPrintClient. Instantiate one of those instead of a real client
 *
 * o Consider:
 *   - Local temperature sensor support
 *   - BlynkClient support
 *   - Pop up a screen while refreshing printer status which can take a while?
 *
 * ISSUES TO RESOLVE:
 * 
 * COMPLETE:
 * o Add support for Duet3D in addition to OctoPrint
 *   - Will require an abstract PrintClient that exposes the common denominator data.
 *     OctoClient and DuetClient would be subclasses
 *   - The WebUI will need a "type" for each printer to be specified
 *   - The type will need to be stored in the settings. 
 * o Web UI
 *   - Configuration
 *     + Allow configuration of the 4 octoprint servers
 *     + Configure the weather server settings
 *   - Home screen
 *     + Display links to the Octoprint UI for each configured printer
 *     + display a weather widget from OWM for the configured city
 *   - TRIED & REJECTED: Consider adding a swipe gesture. The ButtonHandler
 *     would see new PressType values such as SwipeLeft and SwiptRight
 *   - WiFi screen: Show while searching for / connecting to WiFi
 * o Brightness Control
 *   - Wire the TFT_LED pin to a free pin like D4
 *   - Add a GUI::brightness method that PWMs the pin. Lower duty cycle ->
 *     lower brightness.
 *   - Schedule morning/evening brightness levels
 *   - Set brightness from home page.
 * o Resolved Issues
 *   - Intermittent 404's on octoClient /api/job requests. This appears to be
 *     a failure in connect() given a host.local server name. For some reason
 *     it seems to resolve to localhost.
 *   - Intermittent reboots (memory leak?)
 */


//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <FS.h>
#include <ESP8266WiFi.h>
//                                  Third Party Libraries
#include <ArduinoLog.h>
#include <WebUI.h>
#include <WebThingSettings.h>
//                                  Local Includes
#include "MultiMon.h"
#include "MMSettings.h"
#include "MMWebUI.h"
#include "src/gui/GUI.h"
#include "src/clients/PrintClient.h"
#include "src/clients/DuetClient.h"
#include "src/clients/MockPrintClient.h"
#include "src/clients/OctoClient.h"
#include "src/clients/OWMClient.h"
//--------------- End:    Includes ---------------------------------------------

namespace MultiMon {
  // ---- State
  MMSettings settings;
  String printerIPs[MaxServers];
  PrintClient *printer[MaxServers];
  OWMClient *owmClient;

  namespace Internal {
    static const String SettingsFileName = "/settings.json";

    void cachePrinterIP(int i) {
      IPAddress printerIP;
      int result = WiFi.hostByName(settings.printer[i].server.c_str(), printerIP) ;
      if (result == 1) {
        printerIPs[i] = printerIP.toString().c_str();
      } else {
        printerIPs[i] = "";
      }
    }

    void refreshWeatherData(bool force = false) {
      static const uint32_t ForecastInterval = 8 * 60 * 60 * 1000L; // 8 Hours

      if (!settings.owm.enabled) return;

      static uint32_t lastWeatherUpdate = 0;
      static uint32_t lastForecastUpdate = 0;
      uint32_t curMillis = millis();
      uint32_t threshold = (WebThing::settings.processingInterval * 60 * 1000L);

      if (force || ((curMillis - lastWeatherUpdate) > threshold)) {
        GUI::showUpdatingIcon(GUI::Color_UpdatingWeather);
        owmClient->update();
        lastWeatherUpdate = curMillis;
      }
      if (force || ((curMillis - lastForecastUpdate) > ForecastInterval)) {
        GUI::showUpdatingIcon(GUI::Color_UpdatingWeather);
        owmClient->updateForecast(WebThing::getGMTOffset());
        lastForecastUpdate = curMillis;
      }
      GUI::hideUpdatingIcon();
    }

    void refreshPrinterData(bool force = false) {
      static uint32_t lastUpdateTime[MultiMon::MaxServers] = {0, 0, 0, 0};

      for (int i = 0; i < MultiMon::MaxServers; i++) {
        if (settings.printer[i].isActive) {
          uint32_t threshold;
          // Randomize the refresh times a little so we aren't do all the updates
          // at once which can cause the UI to become unresponsive
          switch (printer[i]->getState()) {
            case PrintClient::State::Offline:
              threshold = (random(5*60, 10*60) * 1000L);  // 5 to 10 minutes
              break;
            case PrintClient::State::Operational: 
            case PrintClient::State::Complete:
              threshold = (random(1*60, 3*60) * 1000L);   // 1 to 3 minutes
              break;
            case PrintClient::State::Printing:
              threshold = (10 * 1000L);                   // 10 seconds
              break;
          }
          if (force || ((millis() -  lastUpdateTime[i])) > threshold) {
            GUI::showUpdatingIcon(GUI::Color_UpdatingPrinter);
            printer[i]->updateState();
            lastUpdateTime[i] = millis();
            printer[i]->dumpToLog();
          }
        }
      }
      GUI::hideUpdatingIcon();
    }

    void cmCallback(String &ssid, String &ip) {
      // GUI::displayConfigMode(ssid, ip);
    }
  
    void setTitle() {
      String hostname = WebThing::settings.hostname;
      if (hostname.isEmpty() || hostname.startsWith("MM-"))
        WebUI::setTitle("MultiMon (" + hostname + ")");
      else
        WebUI::setTitle(hostname);
    }

    void baseConfigChange() { setTitle(); }

    void configModeCallback(String &ssid, String &ip) {
      GUI::displayConfigScreen(ssid);
    }

    void prepWeather() {
      if (settings.owm.enabled) {
        if (settings.owm.key.isEmpty()) { settings.owm.enabled = false; }
        else {
          if (owmClient != NULL) { /* TO DO: Do any necessary cleanup */ }
          owmClient = new OWMClient(
            settings.owm.key, settings.owm.cityID, settings.useMetric, settings.owm.language);
        }
      } else {
        owmClient = NULL;
      }
    }

    void prepWebThing() {
      // Some of the MultiMonitor hardware configuration is fixed, so the corresponding settings
      // should have specific values. Rather than make the user configure them, set them to the
      // right values here.
      WebThing::settings.hasVoltageSensing = false;
      WebThing::settings.sleepOverridePin = WebThingSettings::NoPinAssigned;
      WebThing::displayPowerOptions(false);               // Don't let the user fool with this.
      WebThing::settings.indicatorLEDPin = WebThingSettings::NoPinAssigned;
        // Don't use the IndicatorLED. We may want that pin for something else.

      if (WebThing::settings.hostname.isEmpty()) WebThing::settings.hostname = ("MM-" + String(ESP.getChipId(), HEX));
      WebThing::notifyOnConfigMode(configModeCallback);
      WebThing::notifyConfigChange(baseConfigChange);
      WebThing::setup();
      WebThing::setDisplayedVersion("MM " + VersionString);
    }

    void prepWebUI() {
      setTitle();
      MMWebUI::init();
    }

    void activatePrinter(int i) {
      PrinterSettings *ps = &settings.printer[i];
      if (!ps->isActive) return;

      cachePrinterIP(i);
      if (printerIPs[i].isEmpty()) {
        Log.warning("Unable to resolve server address for %s", ps->server.c_str());
        ps->isActive = false;
        return;
      }

      if (printer[i] != NULL) {
        Log.warning("Trying to activate a printer this is already active: %s", ps->server.c_str());
        return;
      }

      if (ps->mock) {
        Log.verbose(
            "Setting up a MockPrintClient of type %s for %s",
            ps->type.c_str(), ps->server.c_str());
        MockPrintClient *mc = new MockPrintClient();
        printer[i] = mc;
      } else if (ps->type == Type_Octo) {
        Log.verbose("Setting up an OctoClient for %s: ", ps->server.c_str());
        OctoClient *oc = new OctoClient();
        oc->init(ps->apiKey, printerIPs[i], ps->port, ps->user, ps->pass);
        printer[i] = oc;
      } else if (ps->type == Type_Duet) {
        Log.verbose("Setting up an DuetClient for %s: ", ps->server.c_str());
        DuetClient *dc = new DuetClient();
        dc->init(printerIPs[i], ps->port, ps->pass);
        printer[i] = dc;
      } else {
        Log.warning("Bad printer type: %s", ps->type.c_str());
        ps->isActive = false;
      }
    }

    void prepPrintClients() {
      for (int i = 0; i < MultiMon::MaxServers; i++) { activatePrinter(i); }
    }

  } // ----- END: MultiMon::Internal namespace


  namespace Protected {
    void askToReboot() { GUI::displayRebootScreen(); }

    void saveSettings() {
      settings.write();
    }

    void configMayHaveChanged() {
      // TO DO: Implement me!!
    }

    void weatherConfigMayHaveChanged() {
      // TO DO: Implement me!!
    }

    void updateAllData() {
      Internal::refreshPrinterData(true);
      Internal::refreshWeatherData(true);
    }

    void updateWeatherData() { Internal::refreshWeatherData(true); }

    void updatePrinterData() { Internal::refreshPrinterData(true); }

    void printerWasActivated(int index) {
      // TO DO: May need to cache the IP!!!
      Internal::activatePrinter(index);
    }
  } // ----- END: MultiMon::Protected namespace

} // ----- END: MultiMon namespace


/*------------------------------------------------------------------------------
 *
 * GLOBAL SCOPE: setup() and loop()
 *
 * The setup() and loop() functions need to be in the global scope, but are logically
 * part of the PrintMonitor namespace. Use a "using" directive for JUST THESE 2 functions
 * to treat them as if they were in that namespace.
 *
 *----------------------------------------------------------------------------*/

using namespace MultiMon;

void setup() {
  WebThing::preSetup();                 // Must be first

  settings.init(Internal::SettingsFileName);
  settings.read();
  settings.logSettings();

  GUI::init(settings.invertDisplay);
  GUI::displayWiFiScreen();

  Internal::prepWebThing();
  Internal::prepWebUI();

  GUI::displaySplashScreen();
  Internal::prepPrintClients();
  Internal::prepWeather();
  Internal::refreshPrinterData(true);   // Updates printer status periodically
  Internal::refreshWeatherData(true);

  GUI::displayHomeScreen();

  WebThing::postSetup();                // Must be last
}

void loop() { 
  WebThing::loop();

  Internal::refreshWeatherData();       // Updates weather data periodically
  Internal::refreshPrinterData();       // Updates printer status periodically

  GUI::loop();

  static uint32_t nextStats = 0;

  if (millis() > nextStats) {
    Log.verbose("Heap: free=%d, frag=%d%%", ESP.getFreeHeap(), ESP.getHeapFragmentation());
    nextStats = millis() + 60 * 1000L;
  }
}











