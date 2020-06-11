#include <ESP8266WiFi.h>
#include <ArduinoLog.h>
#include <JSONService.h>
#include "BlynkClient.h"

static const String FailedRead = "";
static const String BlynkServer = "blynk-cloud.com";
static const uint16_t BlynkPort = 80;

static ServiceDetails blynkDetails(BlynkServer, BlynkPort);
static JSONService blynkService(blynkDetails);

String BlynkClient::readPin(String blynkAppID, String pin) {
    String endpoint = "/" + blynkAppID + "/get/" + pin;

    DynamicJsonDocument *root = blynkService.issueGET(endpoint, 256);
    if (!root) {
      Log.warning("BlynkClient::readPin(): issueGet failed");
      return FailedRead;
    }
    //serializeJsonPretty(*root, Serial); Serial.println();

    String val = (*root)[0].as<String>();
    delete root;

    return val;
}