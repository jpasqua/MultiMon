/*
 * MMWebUI:
 *    Implements the WebUI for MultiMon
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <ESP8266WiFi.h>
//                                  Third Party Libraries
#include <ArduinoLog.h>
//                                  Local Includes
#include "MultiMon.h"
#include "MMWebUI.h"
#include "src/gui/GUI.h"
//--------------- End:    Includes ---------------------------------------------

namespace MM = MultiMon;

// ----- BEGIN: WebUI namespace
namespace MMWebUI {
  static const String   checkedOrNot[2] = {"", "checked='checked'"};
  static const String   EmptyString = "";
  ESPTemplateProcessor  *templateHandler;

  // ----- BEGIN: MMWebUI::Internal
  namespace Internal {
    String MM_ACTIONS =
      "<a class='w3-bar-item w3-button' href='/updateStatus'>"
      "<i class='fa fa-recycle'></i> Update Status</a>"
      "<a class='w3-bar-item w3-button' href='/presentPrinterConfig'>"
      "<i class='fa fa-cog'></i> Configure Printers</a>"
      "<a class='w3-bar-item w3-button' href='/presentDisplayConfig'>"
      "<i class='fa fa-desktop'></i> Configure Display</a>"
      "<a class='w3-bar-item w3-button' href='/presentWeatherConfig'>"
      "<i class='fa fa-thermometer-three-quarters'></i> Configure Weather</a>";

    String DEV_ACTION =
      "<a class='w3-bar-item w3-button' href='/dev'>"
      "<i class='fa fa-gears'></i> Dev Settings</a>";
  }

  void updateSinglePrinter(int i) {
    String prefix = "_p" + String(i) + "_";
    MM::settings.printer[i].isActive = WebUI::hasArg(prefix + "enabled");
    MM::settings.printer[i].apiKey = WebUI::arg(prefix + "key");
    MM::settings.printer[i].server = WebUI::arg(prefix + "server");
    MM::settings.printer[i].port = WebUI::arg(prefix + "port").toInt();
    MM::settings.printer[i].user =  WebUI::arg(prefix + "user");
    MM::settings.printer[i].pass =  WebUI::arg(prefix + "pass");
    MM::settings.printer[i].nickname =  WebUI::arg(prefix + "nick");
    MM::settings.printer[i].type =  WebUI::arg(prefix + "type");
  }
  // ----- END: MMWebUI::Internal


  // ----- BEGIN: MMWebUI::Endpoints
  namespace Endpoints {
    void setBrightness() {
      if (!WebUI::authenticationOK()) { return; }
      Log.trace("Handling /setBrightness");

      uint8_t b = WebUI::arg("brightness").toInt();
      if (b <= 0 || b > 100) {  // NOTE: 0 is not an allowed value!
        Log.warning("/setBrightness: %d is an unallowed brightness setting", b);
        WebUI::closeConnection(400, "Invalid Brightness: " + WebUI::arg("brightness"));
      } else {
        GUI::setBrightness(b);
        WebUI::closeConnection(200, "Brightness Set");
      }
    }

    void updateStatus() {
      if (!WebUI::authenticationOK()) { return; }
      MM::Protected::updateAllData();
      WebUI::redirectHome();
    }

    void updateWeatherConfig() {
      if (!WebUI::authenticationOK()) { return; }
      MM::settings.owm.enabled = WebUI::hasArg("useOWM");
      MM::settings.useMetric = WebUI::hasArg("metric");
      MM::settings.owm.key = WebUI::arg("openWeatherMapApiKey");
      MM::settings.owm.cityID = WebUI::arg("cityID").toInt();
      MM::settings.owm.language = WebUI::arg("language");

      MM::settings.write();
      // MM::settings.logSettings();
      
      MM::Protected::weatherConfigMayHaveChanged();
      WebUI::redirectHome();
    }

    void updatePrinterConfig() {
      if (!WebUI::authenticationOK()) { return; }

      for (int i = 0; i < 4; i++) {
        bool wasActive = MM::settings.printer[i].isActive;
        updateSinglePrinter(i);
        if (!wasActive && MM::settings.printer[i].isActive) MultiMon::Protected::printerWasActivated(i);
      }
      MM::settings.printerRefreshInterval = WebUI::arg("refreshInterval").toInt();
      MM::settings.write();
      // MM::settings.logSettings();

      // Act on changed settings...
      MM::Protected::configMayHaveChanged();
      WebUI::redirectHome();
    }

    void updateDisplayConfig() {
      if (!WebUI::authenticationOK()) { return; }

      MM::settings.scheduleActive = WebUI::hasArg("scheduleEnabled");
      String t = WebUI::arg("morning");
      int separator = t.indexOf(":");
      MM::settings.morning.hr = t.substring(0, separator).toInt();
      MM::settings.morning.min = t.substring(separator+1).toInt();
      MM::settings.morning.brightness = WebUI::arg("mBright").toInt();

      t = WebUI::arg("evening");
      separator = t.indexOf(":");
      MM::settings.evening.hr = t.substring(0, separator).toInt();
      MM::settings.evening.min = t.substring(separator+1).toInt();
      MM::settings.evening.brightness = WebUI::arg("eBright").toInt();

      MM::settings.use24Hour = WebUI::hasArg("is24hour");
      MM::settings.invertDisplay = WebUI::hasArg("invDisp");

      MM::settings.blynk.enabled = WebUI::hasArg("blynkEnabled");
      MM::settings.blynk.id1 = WebUI::arg("blynkID1");
      MM::settings.blynk.id2 = WebUI::arg("blynkID2");
      MM::settings.blynk.nickname1 = WebUI::arg("blynkNN1");
      MM::settings.blynk.nickname2 = WebUI::arg("blynkNN2");

      MM::settings.write();
      //MM::settings.logSettings();

      // Act on changed settings...
      GUI::setOrientation(MM::settings.invertDisplay);

      WebUI::redirectHome();
    }
  }   // ----- END: MMWebUI::Endpoints

  // ----- BEGIN: MMWebUI::Dev
  namespace Dev {
    void presentDevConfig() {
      Log.trace("Web Request: Handle Dev Configure");
      if (!WebUI::authenticationOK()) { return; }

      auto mapper =[](String &key) -> String {
        if (key.startsWith("_P")) {
          int i = (key.charAt(2) - '0');
          key.remove(0, 4); // Get rid of the prefix; e.g. _P1_
          if (key == "NICK") return MM::settings.printer[i].nickname;
          if (key == "MOCK") {
            return checkedOrNot[MM::settings.printer[i].mock];
          }
          if (key == "SERVER") return MM::settings.printer[i].server;
        }
        return EmptyString;
      };

      WebUI::startPage();
      templateHandler->send("/ConfigDev.html", mapper);
      WebUI::finishPage();
    }

    void updateDevData() {
      if (!WebUI::authenticationOK()) { return; }
      MM::settings.printer[0].mock = WebUI::hasArg("_p0_mock");
      MM::settings.printer[1].mock = WebUI::hasArg("_p1_mock");
      MM::settings.printer[2].mock = WebUI::hasArg("_p2_mock");
      MM::settings.printer[3].mock = WebUI::hasArg("_p3_mock");

      // Save the config, but don't change which printers are mocked until reboot
      MM::settings.write();
      WebUI::redirectHome();
    }

    void reboot() {
      if (!WebUI::authenticationOK()) { return; }
      MultiMon::Protected::askToReboot();
      WebUI::redirectHome();
    }

    void forceScreen() {
      Log.trace("Web Request: /dev/forceScreen");
      if (!WebUI::authenticationOK()) { return; }
      String screen = WebUI::arg("screen");
      if (screen.isEmpty()) return;
      else if (screen == "splash") GUI::displaySplashScreen();
      else if (screen == "wifi") GUI::displayWiFiScreen();
      else if (screen == "config") {
        String ssid = "MM-nnnnnn";
        GUI::displayConfigScreen(ssid);
      }
      else GUI::displayFlexScreen(screen);
      WebUI::redirectHome();
    }

    void yieldSettings() {
      Log.trace("Web Request: /dev/settings");
      if (!WebUI::authenticationOK()) { return; }

      DynamicJsonDocument *doc = MultiMon::settings.asJSON();
      WebUI::sendJSONContent(doc);
      doc->clear();
      delete doc;
    }

    void yieldScreenShot() {
      Log.trace("Web Request: /dev/screenShot");
      if (!WebUI::authenticationOK()) { return; }

      WebUI::sendArbitraryContent("image/bmp", GUI::getSizeOfScreenShotAsBMP(), GUI::streamScreenShotAsBMP);
    }
  }   // ----- END: MMWebUI::Dev


  namespace Pages {
    void presentWeatherConfig() {
      Log.trace("Web Request: Handle Weather Configure");
      if (!WebUI::authenticationOK()) { return; }

      String langTarget = "SL" + MM::settings.owm.language;

      auto mapper =[langTarget](String &key) -> String {
        if (key == "WEATHER_KEY") return MM::settings.owm.key;
        if (key == "CITY_NAME" && MM::settings.owm.enabled) return MM::owmClient->weather.location.city;
        if (key == "CITY") return String(MM::settings.owm.cityID);
        if (key == "USE_METRIC") return checkedOrNot[MM::settings.useMetric];
        if (key == langTarget) return "selected";
        if (key == "USE_OWM")  return checkedOrNot[MM::settings.owm.enabled];
        return EmptyString;
      };

      WebUI::startPage();
      templateHandler->send("/ConfigWeather.html", mapper);
      WebUI::finishPage();
    }

    void presentHomePage() {
      Log.trace("Web Request: Display Home Page");
      if (!WebUI::authenticationOK()) { return; }

      auto mapper =[](String &key) -> String {
        if (key.startsWith("_P")) {
          int i = (key.charAt(2) - '0');
          key.remove(0, 4); // Get rid of the prefix; e.g. _P1_
          if (MM::settings.printer[i].isActive) {
            if (key == "VIS") return "block";
            if (key == "HOST") return  MM::settings.printer[i].server;
            if (key == "PORT") return String(MM::settings.printer[i].port);
            if (key == "AUTH") {
              if (!MM::settings.printer[i].user.isEmpty()) return MM::settings.printer[i].user + ':' + MM::settings.printer[i].pass + '@';
              else return EmptyString;
            }
            if (key == "NICK") return  MM::settings.printer[i].nickname;
          } else {
            if (key == "VIS") return "none";
          }
        }
        if (key == "CITYID") {
          if (MM::settings.owm.enabled) return String(MM::settings.owm.cityID);
          else return String("5380748");  // Palo Alto, CA, USA
        }
        if (key == "WEATHER_KEY") return MM::settings.owm.key;
        if (key == "UNITS") return MM::settings.useMetric ? "metric" : "imperial";
        if (key == "BRIGHT") return String(GUI::getBrightness());
        return EmptyString;
      };

      WebUI::startPage();
      templateHandler->send("/HomePage.html", mapper);
      WebUI::finishPage();
    }

    void presentPrinterConfig() {
      Log.trace("Web Request: Handle Printer Configure");
      if (!WebUI::authenticationOK()) { return; }

      auto mapper =[](String &key) -> String {
        if (key.startsWith("_P")) {
          int i = (key.charAt(2) - '0');
          key.remove(0, 4); // Get rid of the prefix; e.g. _P1_
          String type = "T_" + MM::settings.printer[i].type;
          if (key == "ENABLED") return checkedOrNot[MM::settings.printer[i].isActive];
          if (key == "KEY") return MM::settings.printer[i].apiKey;
          if (key == "HOST") return  MM::settings.printer[i].server;
          if (key == "PORT") return String(MM::settings.printer[i].port);
          if (key == "USER") return  MM::settings.printer[i].user;
          if (key == "PASS") return  MM::settings.printer[i].pass;
          if (key == "NICK") return  MM::settings.printer[i].nickname;
          if (key == type) return "selected";
        }
        if (key == "RFRSH") return String(MM::settings.printerRefreshInterval);
        return EmptyString;
      };

      WebUI::startPage();
      templateHandler->send("/ConfigPrinters.html", mapper);
      WebUI::finishPage();
    }

    void presentDisplayConfig() {
      Log.trace("Web Request: Handle Display Configure");
      if (!WebUI::authenticationOK()) { return; }

      auto mapper =[](String &key) -> String {
        if (key == "SCHED_ENABLED") return checkedOrNot[MM::settings.scheduleActive];
        if (key == "MORN") return WebThing::formattedInterval(MM::settings.morning.hr, MM::settings.morning.min, 0, true, false);
        if (key == "EVE") return WebThing::formattedInterval(MM::settings.evening.hr, MM::settings.evening.min, 0, true, false);
        if (key == "M_BRIGHT") return String(MM::settings.morning.brightness);
        if (key == "E_BRIGHT") return String(MM::settings.evening.brightness);

        if (key == "USE_24HOUR") return checkedOrNot[MM::settings.use24Hour];
        if (key == "INVERT_DISPLAY") return checkedOrNot[MM::settings.invertDisplay];
        if (key == "BLYNK_ENABLED") return checkedOrNot[MM::settings.blynk.enabled];
        if (key == "BLYNK_ID1") return MM::settings.blynk.id1;
        if (key == "BLYNK_ID2") return MM::settings.blynk.id2;
        if (key == "BLYNK_NN1") return MM::settings.blynk.nickname1;
        if (key == "BLYNK_NN2") return MM::settings.blynk.nickname2;
        return EmptyString;
      };

      WebUI::startPage();
      templateHandler->send("/ConfigDisplay.html", mapper);
      WebUI::finishPage();
    }
  }   // ----- END: MMWebUI::Pages


  void init() {
    String actions = Internal::MM_ACTIONS;
    if (MM::settings.showDevMenu) {
      actions += Internal::DEV_ACTION;
    }
    WebUI::addMenuItems(actions);

    WebUI::registerHandler("/",                       Pages::presentHomePage);
    WebUI::registerHandler("/presentWeatherConfig",   Pages::presentWeatherConfig);
    WebUI::registerHandler("/presentPrinterConfig",   Pages::presentPrinterConfig);
    WebUI::registerHandler("/presentDisplayConfig",   Pages::presentDisplayConfig);

    WebUI::registerHandler("/updateStatus",           Endpoints::updateStatus);
    WebUI::registerHandler("/updateWeatherConfig",    Endpoints::updateWeatherConfig);
    WebUI::registerHandler("/updatePrinterConfig",    Endpoints::updatePrinterConfig);
    WebUI::registerHandler("/updateDisplayConfig",    Endpoints::updateDisplayConfig);
    WebUI::registerHandler("/setBrightness",          Endpoints::setBrightness);

    WebUI::registerHandler("/dev",                    Dev::presentDevConfig);
    WebUI::registerHandler("/dev/updateDevData",      Dev::updateDevData);
    WebUI::registerHandler("/dev/reboot",             Dev::reboot);
    WebUI::registerHandler("/dev/settings",           Dev::yieldSettings);
    WebUI::registerHandler("/dev/screenShot",         Dev::yieldScreenShot);
    WebUI::registerHandler("/dev/forceScreen",        Dev::forceScreen);

    templateHandler = WebUI::getTemplateHandler();
  }

}
// ----- END: MMWebUI