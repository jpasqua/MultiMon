/*
 * MultiMonApp:
 *    This is the core of the MultiMon functionality. It is invoked
 *    by the primary sketch ".ino" file.
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoLog.h>
//                                  WebThing Includes
#include <WebUI.h>
#include <DataBroker.h>
#include <gui/ScreenMgr.h>
#include <plugins/PluginMgr.h>
#include <plugins/common/GenericPlugin.h>
#include <plugins/common/BlynkPlugin.h>
#include <plugins/common/CryptoPlugin.h>
//                                  Local Includes
#include "MultiMonApp.h"
#include "MMSettings.h"
#include "MMWebUI.h"
#include "MMDataSupplier.h"
#include "src/screens/AppTheme.h"
#include "src/clients/PrintClient.h"
#include "src/clients/DuetClient.h"
#include "src/clients/MockPrintClient.h"
#include "src/clients/OctoClient.h"
//--------------- End:    Includes ---------------------------------------------


static const String VersionString = "0.5.0";
static const String AppName = "MultiMon";
static const String AppPrefix = "MM-";

/*------------------------------------------------------------------------------
 *
 * Private Utility Functions
 *
 *----------------------------------------------------------------------------*/

Plugin* pluginFactory(String& type) {
  Plugin *p = NULL;
  if      (type.equalsIgnoreCase("generic")) { p = new GenericPlugin(); }
  else if (type.equalsIgnoreCase("blynk"))   { p = new BlynkPlugin();   }
  // else if (type.equalsIgnoreCase("crypto"))  { p = new CryptoPlugin();  }
  
  if (p == NULL) {
    Log.warning("Unrecognized plugin type: %s", type.c_str());
  }
  return p;
}

/*------------------------------------------------------------------------------
 *
 * Class function to create and start the MultiMonApp singleton
 *
 *----------------------------------------------------------------------------*/

static MMSettings theSettings;  // Allocate storage for the app settings

void MultiMonApp::create() {
  PluginMgr::setFactory(pluginFactory);
  MultiMonApp* app = new MultiMonApp(&theSettings);
  app->begin();
}


/*------------------------------------------------------------------------------
 *
 * MultiMonApp Public Functions
 *
 *----------------------------------------------------------------------------*/

MultiMonApp::MultiMonApp(MMSettings* settings) :
    WTAppImpl(AppName, AppPrefix, VersionString, settings)
{
    // Nothing to do here...
}

void MultiMonApp::printerWasActivated(int index) {
  // TO DO: May need to cache the IP!!!
  activatePrinter(index);
}


/*------------------------------------------------------------------------------
 *
 * Optional WTAppImpl virtual functions
 *
 *----------------------------------------------------------------------------*/

void MultiMonApp::begin() {
  // Add app-specific code here...
  WTAppImpl::begin();

}

void MultiMonApp::loop() {
  // Add app-specific code here...
  WTAppImpl::loop();
}

/*------------------------------------------------------------------------------
 *
 * Mandatory WTAppImpl virtual functions
 *
 *----------------------------------------------------------------------------*/

void MultiMonApp::app_registerDataSuppliers() {
  DataBroker::registerMapper(MMDataSupplier::printerDataSupplier, MMDataSupplier::PrinterPrefix);
}

void MultiMonApp::app_initWebUI() {
  MMWebUI::init();
}

void MultiMonApp::app_initClients() {
  for (int i = 0; i < MaxPrinters; i++) {
    activatePrinter(i);
  }
}

void MultiMonApp::app_conditionalUpdate(bool force) {
  refreshPrinterData(force);
}

Screen* MultiMonApp::app_registerScreens() {
  detailScreen = new DetailScreen();
  splashScreen = new SplashScreen();
  timeScreen = new TimeScreen();

  ScreenMgr::registerScreen("Detail", detailScreen);
  ScreenMgr::registerScreen("Splash", splashScreen);
  ScreenMgr::registerScreen("Time", timeScreen);

  ScreenMgr::setAsHomeScreen(timeScreen);

  return splashScreen;
}

/*------------------------------------------------------------------------------
 *
 * MultiMonApp Private Functions
 *
 *----------------------------------------------------------------------------*/

void MultiMonApp::cachePrinterIP(int i) {
  IPAddress printerIP;
  int result = WiFi.hostByName(mmSettings->printer[i].server.c_str(), printerIP) ;
  if (result == 1) {
    printerIPs[i] = printerIP.toString().c_str();
  } else {
    printerIPs[i] = "";
  }
}

void MultiMonApp::refreshPrinterData(bool force) {
  static uint32_t lastUpdateTime[MaxPrinters] = {0, 0, 0, 0};

  for (int i = 0; i < MaxPrinters; i++) {
    if (mmSettings->printer[i].isActive) {
      uint32_t threshold = UINT32_MAX;
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
          threshold = (mmSettings->printerRefreshInterval * 1000L);  // O(10 seconds)
          break;
      }
      if (force || ((millis() -  lastUpdateTime[i])) > threshold) {
        ScreenMgr::showUpdatingIcon(AppTheme::Color_UpdatingPrinter);
        printer[i]->updateState();
        lastUpdateTime[i] = millis();
        printer[i]->dumpToLog();
      }
    }
  }
  ScreenMgr::hideUpdatingIcon();
}

void MultiMonApp::activatePrinter(int i) {
  PrinterSettings *ps = &mmSettings->printer[i];
  if (!ps->isActive) return;

  cachePrinterIP(i);
  if (printerIPs[i].isEmpty()) {
    Log.warning(F("Unable to resolve server address for %s"), ps->server.c_str());
    ps->isActive = false;
    return;
  }

  if (printer[i] != NULL) {
    Log.warning(F("Trying to activate a printer this is already active: %s"), ps->server.c_str());
    return;
  }

  if (ps->mock) {
    Log.verbose(
        "Setting up a MockPrintClient of type %s for %s",
        ps->type.c_str(), ps->server.c_str());
    MockPrintClient *mc = new MockPrintClient();
    printer[i] = mc;
  } else if (ps->type == Type_Octo) {
    Log.verbose(F("Setting up an OctoClient for %s: "), ps->server.c_str());
    OctoClient *oc = new OctoClient();
    oc->init(ps->apiKey, printerIPs[i], ps->port, ps->user, ps->pass);
    printer[i] = oc;
  } else if (ps->type == Type_Duet) {
    Log.verbose(F("Setting up an DuetClient for %s: "), ps->server.c_str());
    DuetClient *dc = new DuetClient();
    dc->init(printerIPs[i], ps->port, ps->pass);
    printer[i] = dc;
  } else {
    Log.warning(F("Bad printer type: %s"), ps->type.c_str());
    ps->isActive = false;
  }
}
