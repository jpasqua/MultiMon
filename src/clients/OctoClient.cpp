/*
 * OctoClient:
 *    A simple client to get information from (not control) OctoPrint servers 
 *                    
 * TO DO:
 * o If getJobState() can't connect, consider setting state to "Offline"
 *   so we don't bother trying to get file  info.
 *
 * COMPLETE:
 *
 */


//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoLog.h>
//                                  Local Includes
#include "OctoClient.h"
//--------------- End:    Includes ---------------------------------------------


/*------------------------------------------------------------------------------
 *
 * Constructors and Public methods
 *
 *----------------------------------------------------------------------------*/

void OctoClient::init(
  String apiKey, String server, int port, String user, String pass) {
  details.server = server;
  details.port = port;
  details.user = user;
  details.pass = pass;
  details.apiKey = apiKey;
  details.apiKeyName = "X-Api-Key";
  if (service) delete service;
  service = new JSONService(details);
  jobState.reset();
  printerState.reset();
}

void OctoClient::updateState() {
  getJobState();
  // If we are offline, don't bother trying to get printerState
  if (!jobState.state.startsWith("Offline")) getPrinterState();
}

OctoClient::State OctoClient::getState() {
  if (printerState.isPrinting) return State::Printing;
  if (jobState.state == "Operational") {
    if (completionAcknowledged || jobState.progress.completion <= 99) return State::Operational;
    return State::Complete;
  } 
  return State::Offline;
}

void OctoClient::acknowledgeCompletion() {
  completionAcknowledged = true;
}

/*------------------------------------------------------------------------------
 *
 * Private methods
 *
 *----------------------------------------------------------------------------*/

void OctoClient::getJobState() {
  constexpr const char* JobStateEndpoint = "/api/job";
  constexpr uint32_t JobStateJSONSize =
      JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) +
      2*JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + 710;

  DynamicJsonDocument *root = service->issueGET(JobStateEndpoint, JobStateJSONSize);
  if (!root) {
    Log.warning(F("issueGET failed, giving up"));
    jobState.reset();
    return;
  }
  //serializeJsonPretty(*root, Serial); Serial.println();

  jobState.valid = true;
  jobState.state = (*root)["state"].as<String>();
  jobState.file.name = (*root)["job"]["file"]["name"].as<String>();
  jobState.file.size = (*root)["job"]["file"]["size"];

  jobState.averagePrintTime = (*root)["job"]["averagePrintTime"];
  jobState.estimatedPrintTime = (*root)["job"]["estimatedPrintTime"];
  jobState.lastPrintTime = (*root)["job"]["lastPrintTime"];
  jobState.filamentLength = (*root)["job"]["filament"]["tool0"]["length"];

  jobState.progress.filepos = (*root)["progress"]["filepos"];
  jobState.progress.printTime = (*root)["progress"]["printTime"];
  jobState.progress.printTimeLeft = (*root)["progress"]["printTimeLeft"];
  if (completionAcknowledged && jobState.state != "Operational") completionAcknowledged = false;
  jobState.progress.completion = (*root)["progress"]["completion"];

  timeOfLastUpdate = millis();
  delete root;
}

void OctoClient::getPrinterState() {
  constexpr const char* PrinterStateEndpoint = "/api/printer?exclude=sd,history";
  constexpr uint32_t PrinterStateJSONSize =
      3*JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(9) + 300;

  DynamicJsonDocument *root = service->issueGET(PrinterStateEndpoint, PrinterStateJSONSize);
  if (!root) {
    Log.warning(F("issueGET failed, giving up"));
    printerState.reset();
    return;
  }
  // serializeJsonPretty(root, Serial); Serial.println();

  printerState.valid = true;
  printerState.isPrinting = (*root)["state"]["flags"]["printing"];
  printerState.toolTemp.actual = (*root)["temperature"]["tool0"]["actual"];
  printerState.toolTemp.target =(*root)["temperature"]["tool0"]["target"];
  printerState.bedTemp.actual = (*root)["temperature"]["bed"]["actual"];
  printerState.bedTemp.target = (*root)["temperature"]["bed"]["target"];

  timeOfLastUpdate = millis();
  delete root;
}



