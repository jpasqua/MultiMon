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
      "<i class='fa fa-thermometer-three-quarters'></i> Configure Weather</a>"
      "<a class='w3-bar-item w3-button' href='/presentPluginConfig'>"
      "<i class='fa fa-plug'></i> Configure Plugins</a>";

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
      Log.trace(F("Handling /setBrightness"));

      uint8_t b = WebUI::arg(F("brightness")).toInt();
      if (b <= 0 || b > 100) {  // NOTE: 0 is not an allowed value!
        Log.warning(F("/setBrightness: %d is an unallowed brightness setting"), b);
        WebUI::closeConnection(400, "Invalid Brightness: " + WebUI::arg(F("brightness")));
      } else {
        GUI::setBrightness(b);
        WebUI::closeConnection(200, F("Brightness Set"));
      }
    }

    void updateStatus() {
      if (!WebUI::authenticationOK()) { return; }
      MM::Protected::updateAllData();
      WebUI::redirectHome();
    }

    void updateWeatherConfig() {
      if (!WebUI::authenticationOK()) { return; }
      MM::settings.owm.enabled = WebUI::hasArg(F("useOWM"));
      MM::settings.useMetric = WebUI::hasArg(F("metric"));
      MM::settings.owm.key = WebUI::arg(F("openWeatherMapApiKey"));
      MM::settings.owm.cityID = WebUI::arg(F("cityID")).toInt();
      MM::settings.owm.language = WebUI::arg(F("language"));

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
      MM::settings.printerRefreshInterval = WebUI::arg(F("refreshInterval")).toInt();
      MM::settings.write();
      // MM::settings.logSettings();

      // Act on changed settings...
      MM::Protected::configMayHaveChanged();
      WebUI::redirectHome();
    }

    void updatePluginConfig() {
      if (!WebUI::authenticationOK()) { return; }

      String responseType = "text/plain";

      if (!WebUI::hasArg(F("pID"))) {
        Log.warning(F("No plugin ID was supplied"));
        WebUI::sendStringContent(responseType, F("ERR: No plugin ID"));
        return;
      }

      uint8_t pID = WebUI::arg(F("pID")).toInt()-1;
      Plugin *p = MM::pluginMgr.getPlugin(pID);
      if (p == NULL) {
        Log.warning(F("Invalid plugin ID: %d"), pID);
        WebUI::sendStringContent(responseType, F("ERR: Invalid plugin ID"));
        return;
      }

      if (!WebUI::hasArg(F("plain"))) {
        Log.warning(F("No payload supplied"));
        WebUI::sendStringContent(responseType, F("ERR: No payload"));
        return;
      }

      String settings = WebUI::arg("plain");
Log.verbose("New settings: %s", settings.c_str());
      p->newSettings(settings);        

      WebUI::sendStringContent(responseType, F("OK: settings updated"));
    }

    void updateDisplayConfig() {
      if (!WebUI::authenticationOK()) { return; }

      MM::settings.scheduleActive = WebUI::hasArg(F("scheduleEnabled"));
      String t = WebUI::arg(F("morning"));
      int separator = t.indexOf(":");
      MM::settings.morning.hr = t.substring(0, separator).toInt();
      MM::settings.morning.min = t.substring(separator+1).toInt();
      MM::settings.morning.brightness = WebUI::arg(F("mBright")).toInt();

      t = WebUI::arg(F("evening"));
      separator = t.indexOf(":");
      MM::settings.evening.hr = t.substring(0, separator).toInt();
      MM::settings.evening.min = t.substring(separator+1).toInt();
      MM::settings.evening.brightness = WebUI::arg(F("eBright")).toInt();

      MM::settings.use24Hour = WebUI::hasArg(F("is24hour"));
      MM::settings.invertDisplay = WebUI::hasArg(F("invDisp"));

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
      Log.trace(F("Web Request: Handle Dev Configure"));
      if (!WebUI::authenticationOK()) { return; }

      auto mapper =[](String &key) -> String {
        if (key.startsWith("_P")) {
          int i = (key.charAt(2) - '0');
          key.remove(0, 4); // Get rid of the prefix; e.g. _P1_
          if (key.equals(F("NICK"))) return MM::settings.printer[i].nickname;
          if (key.equals(F("MOCK"))) {
            return checkedOrNot[MM::settings.printer[i].mock];
          }
          if (key.equals(F("SERVER"))) return MM::settings.printer[i].server;
        }
        return EmptyString;
      };

      WebUI::startPage();
      templateHandler->send("/ConfigDev.html", mapper);
      WebUI::finishPage();
    }

    void updateDevData() {
      if (!WebUI::authenticationOK()) { return; }
      MM::settings.printer[0].mock = WebUI::hasArg(F("_p0_mock"));
      MM::settings.printer[1].mock = WebUI::hasArg(F("_p1_mock"));
      MM::settings.printer[2].mock = WebUI::hasArg(F("_p2_mock"));
      MM::settings.printer[3].mock = WebUI::hasArg(F("_p3_mock"));

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
      Log.trace(F("Web Request: /dev/forceScreen"));
      if (!WebUI::authenticationOK()) { return; }
      String screen = WebUI::arg(F("screen"));
      if (screen.isEmpty()) return;
      else if (screen == F("splash")) GUI::displaySplashScreen();
      else if (screen == F("wifi")) GUI::displayWiFiScreen();
      else if (screen == F("config")) {
        String ssid = "MM-nnnnnn";
        GUI::displayConfigScreen(ssid);
      }
      else GUI::displayFlexScreen(screen);
      WebUI::redirectHome();
    }

    void yieldSettings() {
      Log.trace(F("Web Request: /dev/settings"));
      if (!WebUI::authenticationOK()) { return; }

      DynamicJsonDocument *doc = (WebUI::hasArg("wt")) ? WebThing::settings.asJSON() :
                                                         MultiMon::settings.asJSON();
      WebUI::sendJSONContent(doc);
      doc->clear();
      delete doc;
    }

    void yieldScreenShot() {
      Log.trace(F("Web Request: /dev/screenShot"));
      if (!WebUI::authenticationOK()) { return; }

      WebUI::sendArbitraryContent("image/bmp", GUI::getSizeOfScreenShotAsBMP(), GUI::streamScreenShotAsBMP);
    }

    void enableDevMenu() {
      if (!WebUI::authenticationOK()) { return; }
      Log.trace("Web Request: /dev/enableDevMenu");

      MM::settings.showDevMenu = WebUI::hasArg("showDevMenu");
      MM::settings.write();

      WebUI::redirectHome();
    }
  }   // ----- END: MMWebUI::Dev


  namespace Pages {
    void presentWeatherConfig() {
      Log.trace(F("Web Request: Handle Weather Configure"));
      if (!WebUI::authenticationOK()) { return; }

      String langTarget = "SL" + MM::settings.owm.language;

      auto mapper =[langTarget](String &key) -> String {
        if (key.equals(F("WEATHER_KEY"))) return MM::settings.owm.key;
        if (key.equals(F("CITY_NAME")) && MM::settings.owm.enabled) {
          return MM::owmClient == NULL ? EmptyString : MM::owmClient->weather.location.city;
        }
        if (key.equals(F("CITY"))) return String(MM::settings.owm.cityID);
        if (key.equals(F("USE_METRIC"))) return checkedOrNot[MM::settings.useMetric];
        if (key == langTarget) return "selected";
        if (key.equals(F("USE_OWM")))  return checkedOrNot[MM::settings.owm.enabled];
        return EmptyString;
      };

      WebUI::startPage();
      templateHandler->send("/ConfigWeather.html", mapper);
      WebUI::finishPage();
    }

    void presentHomePage() {
      Log.trace(F("Web Request: Display Home Page"));
      if (!WebUI::authenticationOK()) { return; }

      auto mapper =[](String &key) -> String {
        if (key.startsWith("_P")) {
          int i = (key.charAt(2) - '0');
          key.remove(0, 4); // Get rid of the prefix; e.g. _P1_
          if (MM::settings.printer[i].isActive) {
            if (key.equals(F("VIS"))) return "block";
            if (key.equals(F("HOST"))) return  MM::settings.printer[i].server;
            if (key.equals(F("PORT"))) return String(MM::settings.printer[i].port);
            if (key.equals(F("AUTH"))) {
              if (!MM::settings.printer[i].user.isEmpty()) return MM::settings.printer[i].user + ':' + MM::settings.printer[i].pass + '@';
              else return EmptyString;
            }
            if (key.equals(F("NICK"))) return  MM::settings.printer[i].nickname;
          } else {
            if (key.equals(F("VIS"))) return "none";
          }
        }
        if (key.equals(F("CITYID"))) {
          if (MM::settings.owm.enabled) return String(MM::settings.owm.cityID);
          else return String("5380748");  // Palo Alto, CA, USA
        }
        if (key.equals(F("WEATHER_KEY"))) return MM::settings.owm.key;
        if (key.equals(F("UNITS"))) return MM::settings.useMetric ? "metric" : "imperial";
        if (key.equals(F("BRIGHT"))) return String(GUI::getBrightness());
        return EmptyString;
      };

      WebUI::startPage();
      templateHandler->send("/HomePage.html", mapper);
      WebUI::finishPage();
    }

    void presentPrinterConfig() {
      Log.trace(F("Web Request: Handle Printer Configure"));
      if (!WebUI::authenticationOK()) { return; }

      auto mapper =[](String &key) -> String {
        if (key.startsWith("_P")) {
          int i = (key.charAt(2) - '0');
          key.remove(0, 4); // Get rid of the prefix; e.g. _P1_
          String type = "T_" + MM::settings.printer[i].type;
          if (key.equals(F("ENABLED"))) return checkedOrNot[MM::settings.printer[i].isActive];
          if (key.equals(F("KEY"))) return MM::settings.printer[i].apiKey;
          if (key.equals(F("HOST"))) return  MM::settings.printer[i].server;
          if (key.equals(F("PORT"))) return String(MM::settings.printer[i].port);
          if (key.equals(F("USER"))) return  MM::settings.printer[i].user;
          if (key.equals(F("PASS"))) return  MM::settings.printer[i].pass;
          if (key.equals(F("NICK"))) return  MM::settings.printer[i].nickname;
          if (key == type) return "selected";
        }
        if (key.equals(F("RFRSH"))) return String(MM::settings.printerRefreshInterval);
        return EmptyString;
      };

      WebUI::startPage();
      templateHandler->send("/ConfigPrinters.html", mapper);
      WebUI::finishPage();
    }

    void presentPluginConfig() {
      Log.trace(F("Web Request: Handle Plugin Configure"));
      if (!WebUI::authenticationOK()) { return; }

      auto mapper =[](String &key) -> String {
        if (key.startsWith("_P")) {
          int i = (key.charAt(2) - '0');
          key.remove(0, 4); // Get rid of the prefix; e.g. _P1_
          Plugin *p = MM::pluginMgr.getPlugin(i-1);
          if (p == NULL) return EmptyString;
          if (key.equals(F("IDX"))) return String(i);
          if (key.equals(F("NAME"))) { return p->getName(); }
          if (key.equals(F("FORM"))) { String v; p->getForm(v); return v; }
          if (key.equals(F("VALS"))) { String v; p->getSettings(v); return v; }
        }
        return EmptyString;
      };

      WebUI::startPage();
      templateHandler->send("/ConfigPlugins.html", mapper);
      WebUI::finishPage();
    }

    void presentDisplayConfig() {
      Log.trace(F("Web Request: Handle Display Configure"));
      if (!WebUI::authenticationOK()) { return; }

      auto mapper =[](String &key) -> String {
        if (key.equals(F("SCHED_ENABLED"))) return checkedOrNot[MM::settings.scheduleActive];
        if (key.equals(F("MORN"))) return WebThing::formattedInterval(MM::settings.morning.hr, MM::settings.morning.min, 0, true, false);
        if (key.equals(F("EVE"))) return WebThing::formattedInterval(MM::settings.evening.hr, MM::settings.evening.min, 0, true, false);
        if (key.equals(F("M_BRIGHT"))) return String(MM::settings.morning.brightness);
        if (key.equals(F("E_BRIGHT"))) return String(MM::settings.evening.brightness);

        if (key.equals(F("USE_24HOUR"))) return checkedOrNot[MM::settings.use24Hour];
        if (key.equals(F("INVERT_DISPLAY"))) return checkedOrNot[MM::settings.invertDisplay];
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
    WebUI::registerHandler("/presentPluginConfig",    Pages::presentPluginConfig);

    WebUI::registerHandler("/updateStatus",           Endpoints::updateStatus);
    WebUI::registerHandler("/updateWeatherConfig",    Endpoints::updateWeatherConfig);
    WebUI::registerHandler("/updatePrinterConfig",    Endpoints::updatePrinterConfig);
    WebUI::registerHandler("/updateDisplayConfig",    Endpoints::updateDisplayConfig);
    WebUI::registerHandler("/updatePluginConfig",     Endpoints::updatePluginConfig);
    WebUI::registerHandler("/setBrightness",          Endpoints::setBrightness);

    WebUI::registerHandler("/dev",                    Dev::presentDevConfig);
    WebUI::registerHandler("/dev/updateDevData",      Dev::updateDevData);
    WebUI::registerHandler("/dev/reboot",             Dev::reboot);
    WebUI::registerHandler("/dev/settings",           Dev::yieldSettings);
    WebUI::registerHandler("/dev/screenShot",         Dev::yieldScreenShot);
    WebUI::registerHandler("/dev/forceScreen",        Dev::forceScreen);
    WebUI::registerHandler("/dev/enableDevMenu",      Dev::enableDevMenu);

    templateHandler = WebUI::getTemplateHandler();
  }

}
// ----- END: MMWebUI