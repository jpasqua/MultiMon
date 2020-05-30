/*
 * DuetClient:
 *    A simple client to get information from (not control) Duet3D controllers 
 *                    
 * TO DO:
 *
 * COMPLETE:
 *
 */


//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoLog.h>
//                                  Local Includes
#include "DuetClient.h"
//--------------- End:    Includes ---------------------------------------------


/*------------------------------------------------------------------------------
 *
 * Public Methods
 *
 *----------------------------------------------------------------------------*/

// ----- Constructors and initialization

void DuetClient::init(String server, int port, String pass) {
  details.server = server;
  details.port = port;
  details.pass = pass;
  details.apiKey = "";
  details.apiKeyName = "";
  if (service) delete service;
  service = new JSONService(details);
  rrState.reset();
  fileInfo.reset();
}

// ----- Interrogate the Printer

void DuetClient::updateState() {
  if (connect()) {
    String oldStatus = rrState.status;  // Let's see if this changes...
    getRRState();                       // Refresh the RepRap State
    // Only get the file info if we need it...
    if (fileInfo.err || oldStatus != rrState.status) getFileInfo();
    updateDerivedValues();
    disconnect();
  } else {
    printerState = PrintClient::State::Offline;
  }
}

// ----- Getters

DuetClient::State DuetClient::getState() { return printerState; }

bool DuetClient::isPrinting() { return printerState == PrintClient::State::Printing; }

float DuetClient::getPctComplete() {
  if (printerState == Offline || printerState == Operational) return 0.0f;
  if (printerState == Complete) return 100.0f;
  // Assert(printerState == Printing)
  if (printTimeEstimate == 0) return 0.0f;
  return (elapsed*100.0f)/((float)printTimeEstimate);
}

uint32_t DuetClient::getPrintTimeLeft() {
  if (printerState == Printing) return printTimeEstimate - elapsed;
  // Assert(printerState == Offline | Operational | Complete)
  return 0;
}

uint32_t DuetClient::getElapsedTime() {
  if (printerState == Offline || printerState == Operational) return 0;
  // Assert(printerState == Complete | Printing)
  return elapsed;
}

String DuetClient::getFilename() {
  if (printerState == Offline || printerState == Operational) return "No File";
  // Assert(printerState == Complete | Printing)
  return fileInfo.name;
}

void DuetClient::getBedTemps(float &actual, float &target) {
  if (printerState == Offline) { actual = target = 0.0f; return; }
  actual = rrState.bedTemp.actual;
  target = rrState.bedTemp.target;
}

void DuetClient::getToolTemps(float &actual, float &target) {
  if (printerState == Offline) { actual = target = 0.0f; return; }
  actual = rrState.toolTemp.actual;
  target = rrState.toolTemp.target;
}

// ----- Public Utility Methods

void DuetClient::acknowledgeCompletion() {
  if (printerState == Complete) {
    printerState = Operational;
  }
}

static const char *_PrintStateNames[] = {"Offline", "Operational", "Complete", "Printing"};

void DuetClient::dumpToLog() {
  Log.verbose("----- Derived Values -----");
  Log.verbose("  printerState: %s", _PrintStateNames[printerState]);
  Log.verbose("  printTimeEstimate: %d", printTimeEstimate);
  Log.verbose("  elapsed: %F", elapsed);
  fileInfo.dumpToLog();
  rrState.dumpToLog();
}

/*------------------------------------------------------------------------------
 *
 * Private methods
 *
 *----------------------------------------------------------------------------*/

bool DuetClient::connect() {
  static const uint32_t RRConnectJSONSize = 128;
  String RRConnectEndpoint = "/rr_connect?password=";

  RRConnectEndpoint += (details.pass.isEmpty()) ? "reprap" : details.pass;
  DynamicJsonDocument *root = service->issueGET(RRConnectEndpoint, RRConnectJSONSize);
  if (!root) {
    Log.warning("issueGET failed for RRConnect (%s)", RRConnectEndpoint.c_str());
    return false;
  }
  // serializeJsonPretty(*root, Serial); Serial.println();

  int err = (*root)["err"];
  delete root;

  if (err) { Log.warning("rr_connect error: %d", err); return false; }
  return true;
}

bool DuetClient::disconnect() {
  static const uint32_t RRDisonnectJSONSize = 128;
  static const String RRDisconnectEndpoint = "/rr_disconnect";

  DynamicJsonDocument *root = service->issueGET(RRDisconnectEndpoint, RRDisonnectJSONSize);
  if (!root) {
    Log.warning("issueGET failed for RRDisconnect");
    return false;
  }
  // serializeJsonPretty(*root, Serial); Serial.println();

  int err = (*root)["err"];
  delete root;

  if (err) { Log.warning("rr_disconnect error: %d", err); return false; }
  return true;
}

void DuetClient::getRRState() {
  static const String RRStateEndpoint = "/rr_status?type=3";
  static const uint32_t RRStateJSONSize = 2048;

  DynamicJsonDocument *root = service->issueGET(RRStateEndpoint, RRStateJSONSize);
  if (!root) {
    Log.warning("issueGET failed for RRState");
    rrState.reset();
    return;
  }
  //serializeJsonPretty(*root, Serial); Serial.println();

  rrState.status = (*root)["status"].as<String>();

  rrState.warmupDuration = (*root)["warmUpDuration"];
  rrState.printDuration = (*root)["printDuration"];
  rrState.remaining[0] = (*root)["timesLeft"]["file"];
  rrState.remaining[1] = (*root)["timesLeft"]["filament"];
  rrState.remaining[2] = (*root)["timesLeft"]["layer"];

  rrState.toolTemp.actual = (*root)["temps"]["current"][1];
  rrState.toolTemp.target = (*root)["temps"]["tools"]["active"][0][0];
  rrState.bedTemp.actual  = (*root)["temps"]["bed"]["current"];
  rrState.bedTemp.target  = (*root)["temps"]["bed"]["active"];

  timeOfLastUpdate = millis();
  delete root;
}

void DuetClient::getFileInfo() {
  static const String FileInfoEndpoint = "/rr_fileinfo";
  static const uint32_t FileInfoJSONSize = 1024;

  DynamicJsonDocument *root = service->issueGET(FileInfoEndpoint, FileInfoJSONSize);
  if (!root) {
    Log.warning("issueGET failed for FileInfo");
    fileInfo.reset();
    return;
  }
  // serializeJsonPretty(*root, Serial); Serial.println();

  fileInfo.err = (*root)["err"];
  if (fileInfo.err) {
    // We may have an error because the previous job has completed and there
    // is no new file. In that case, leave the data intact so it can continue
    // to be used. If it is a "normal" error, clear out the fileInfo and return;
    int savedErr = fileInfo.err;
    if (printerState != PrintClient::State::Complete) { fileInfo.reset(); fileInfo.err = savedErr; }
    timeOfLastUpdate = millis();
    delete root;
    return;
  }

  fileInfo.name = (*root)["fileName"].as<String>();
  int index = fileInfo.name.lastIndexOf('/');
  if (index != -1) { fileInfo.name.remove(0, index+1); }

  fileInfo.size = (*root)["size"];
  fileInfo.generatedBy = (*root)["generatedBy"].as<String>();
  fileInfo.lastModified = (*root)["lastModified"].as<String>();
  fileInfo.height = (*root)["height"];
  fileInfo.printTime = (*root)["printTime"];

  fileInfo.filament = 0;
  JsonArray filaments = (*root)["filament"];
  for (JsonVariant value : filaments) {
    uint32_t thisFilament = value;
    fileInfo.filament += thisFilament;
  }

  fileInfo.firstLayerHeight = (*root)["firstLayerHeight"];
  fileInfo.layerHeight = (*root)["layerHeight"];

  timeOfLastUpdate = millis();
  delete root;
}

void DuetClient::updateDerivedValues() {
  // Update printerState
  if (rrState.status.isEmpty()) { printerState = Offline; }
  else {
    char s = rrState.status[0];
    switch (s) {
      case 'D':
      case 'S':
      case 'R':
      case 'P':
      case 'M':
        printerState = Printing;
        break;
      default:
        if (printerState == Printing) {
          // We've transitioned from printing to not-printing
          // In this case we call the print complete
          printerState = Complete;
          printTimeEstimate = elapsed; // Force it to be 100% complete.
        } else if (printerState != Complete) { printerState = Operational; }
        break;
    }
  }

  if (printerState == Printing) {
    // Update derived values that are relevant to active prints
    elapsed = rrState.printDuration - rrState.warmupDuration;
    // Ensure that printTimeEstimate >= elapsed
    if (elapsed < fileInfo.printTime) { printTimeEstimate = fileInfo.printTime; }
    else {
      // We've already been printing longer than the estimated time based on the slicer
      // so start looking at the other estimates and use one of those. Use the smallest
      // non-zero value of the {file,filament,layer} estimates.
      float newPTE = 60;  // Add a minute if there are no non-zero values
      for (int i = 0; i < 3; i++) {
        float timeLeft = rrState.remaining[i];
        if ((timeLeft > 0.5) && (timeLeft < newPTE)) newPTE = timeLeft;
      }
      printTimeEstimate = elapsed + newPTE;
    }
  }
}
