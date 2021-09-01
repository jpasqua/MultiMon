/*
 * MultiMonApp:
 *    This is the core of the MultiMon functionality. It is invoked
 *    by the primary sketch ".ino" file.
 *
 * NOTES:
 * o MultiMonApp is derived from WTAppImpl, which is itself derived
 *   from WTApp.
 * o This is a singleton and is made available to the whole app via
 *   an extern declaration in WTApp.h:
 *       WTApp* wtApp;
 * o This must be cast back to MultiMonApp* in order to get to app-specific
 *   data and functions. Technically this should be a dynamic_cast, but for
 *   efficiency, we perform a "C-style" cast.
 * o Macros are provided to easily get the app and settings in their
 *   specialized forms.
 *
 * Customization:
 * o To add a new screen to the app, declare it here and instantiate it
 *   in the associated .cpp file.
 */


#ifndef MultiMonApp_h
#define MultiMonApp_h

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
//                                  WebThing Includes
#include <WTAppImpl.h>
//                                  Local Includes
#include "MMSettings.h"
#include "src/screens/DetailScreen.h"
#include "src/screens/SplashScreen.h"
#include "src/screens/TimeScreen.h"
//--------------- End:    Includes ---------------------------------------------


// A couple of convenience macros:
// mmApp simply yields the wtApp singleton cast as a MultiMonApp*
// mmSettings yields the settings object from wtApp, cast as a MMSettings*
#define mmApp ((MultiMonApp*)wtApp)
#define mmSettings ((MMSettings*)mmApp->settings)

class MultiMonApp : public WTAppImpl {
public:
  static const int MaxPrinters = 4;

  static void create();

  DetailScreen*	  detailScreen;
  SplashScreen*   splashScreen;
  TimeScreen*     timeScreen;
  PrintClient*    printer[MaxPrinters];
  
  // ----- Functions that *must* be provided by subclasses
  virtual void app_registerDataSuppliers() override;
  virtual Screen* app_registerScreens() override;
  virtual void app_initWebUI() override;
  virtual void app_initClients() override;
  virtual void app_conditionalUpdate(bool force = false) override;
  virtual void app_loop() override;

  // ----- Public functions
  MultiMonApp(MMSettings* settings);
  void printerWasActivated(int index);

 private:
  String printerIPs[MaxPrinters];

  void cachePrinterIP(int i);
  void refreshPrinterData(bool force = false);
  void activatePrinter(int i);
};

#endif	// MultiMonApp_h