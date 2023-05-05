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
#include <BPA_PrintClient.h>
#include <BPA_DuetClient.h>
#include <BPA_MockPrintClient.h>
#include <BPA_OctoClient.h>
//                                  WebThing Includes
#include <WebUI.h>
#include <DataBroker.h>
#include <gui/ScreenMgr.h>
#include <plugins/PluginMgr.h>
#include <plugins/common/GenericPlugin.h>
#include <plugins/common/AIOPlugin.h>
#include <plugins/common/CryptoPlugin.h>
//                                  Local Includes
#include "MultiMonApp.h"
#include "MMSettings.h"
#include "MMWebUI.h"
#include "src/screens/AppTheme.h"
//--------------- End:    Includes ---------------------------------------------


static constexpr const char* VersionString = "0.5.1";
static constexpr const char* AppName = "MultiMon";
static constexpr const char* AppPrefix = "MM-";

/*------------------------------------------------------------------------------
 *
 * Private Utility Functions
 *
 *----------------------------------------------------------------------------*/

Plugin* pluginFactory(const String& type) {
  Plugin *p = NULL;
  if      (type.equalsIgnoreCase("generic")) { p = new GenericPlugin(); }
  else if (type.equalsIgnoreCase("aio")) { p = new AIOPlugin(); }
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
  printerGroup->activatePrinter(index);
}


/*------------------------------------------------------------------------------
 *
 * Mandatory WTAppImpl virtual functions
 *
 *----------------------------------------------------------------------------*/

void MultiMonApp::app_loop() {
  // Add app-specific code here...
}

void MultiMonApp::app_registerDataSuppliers() {
  DataBroker::registerMapper(
      [this](const String& key,String& val) { this->printerGroup->dataSupplier(key, val); },
      PrinterGroup::DataProviderPrefix);
}

void MultiMonApp::app_initWebUI() {
  MMWebUI::init();
}

void MultiMonApp::app_initClients() {
  printerGroup = new PrinterGroup(
    MaxPrinters, mmSettings->printer,
    mmSettings->printerRefreshInterval,
    // std::bind(&MultiMonApp::showPrinterActivity, this, std::placeholders::_1));
    [this](bool busy){this->showPrinterActivity(busy);});
  for (int i = 0; i < MaxPrinters; i++) {
    printerGroup->activatePrinter(i);
  }
}

void MultiMonApp::app_conditionalUpdate(bool force) {
  printerGroup->refreshPrinterData(force);
}

Screen* MultiMonApp::app_registerScreens() {
  detailScreen = new DetailScreen();
  splashScreen = new SplashScreen();
  homeScreen = new HomeScreen();

  ScreenMgr.registerScreen("Detail", detailScreen);
  ScreenMgr.registerScreen("Splash", splashScreen);
  ScreenMgr.registerScreen("Time", homeScreen);

  ScreenMgr.setAsHomeScreen(homeScreen);

  return splashScreen;
}


/*------------------------------------------------------------------------------
 *
 * MultiMonApp Private Functions
 *
 *----------------------------------------------------------------------------*/

void MultiMonApp::showPrinterActivity(bool busy) {
 if (busy) ScreenMgr.showActivityIcon(AppTheme::Color_UpdatingPrinter);
 else ScreenMgr.hideActivityIcon();
}

