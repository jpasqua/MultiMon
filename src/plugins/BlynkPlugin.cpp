/*
 * BlynkPlugin
 *    A plugin that gets and displays content from Blynk
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
#include <ArduinoLog.h>
//                                  Third Party Libraries
//                                  App Libraries and Includes
#include "BlynkPlugin.h"
#include "../gui/GUI.h"
#include "../clients/BlynkClient.h"
//--------------- End:    Includes ---------------------------------------------

BlynkPlugin::~BlynkPlugin() {
  if (_pins) delete _pins;
  if (_pinVals) delete _pinVals;
  if (_blynkIDs) delete _blynkIDs;
}

bool BlynkPlugin::init(JsonObject &obj) {
  // Log.trace(F("BlynkPlugin::init()"));

  // Get type-specific data
  _refreshInterval = obj["refreshInterval"];

  JsonArrayConst ids = obj["blynkIDs"];
  JsonArrayConst names = obj["nicknames"];
  _nBlynkIDs = ids.size();  
  _blynkIDs = new String[_nBlynkIDs];
  _nicknames = new String[_nBlynkIDs];

  for (int i = 0; i < _nBlynkIDs; i++) {
    _blynkIDs[i] = ids[i].as<String>();
    _nicknames[i] = names[i].as<String>();
  }

  JsonArrayConst pinArray = obj["pins"];
  _nPins = pinArray.size();  
  _pins = new String[_nPins];
  _pinVals = new String[_nPins];

  for (int i = 0; i < _nPins; i++) {
    _pins[i] = pinArray[i].as<String>();
  }

  _mapper = [&](String& key) -> String {
      for (int i = 0; i < _nPins; i++) {
        if (key == _pins[i]) return _pinVals[i];
      }
      if (key == "NN1" && _nBlynkIDs >= 1) return _nicknames[0];
      if (key == "NN2" && _nBlynkIDs >= 2) return _nicknames[1];
      return "";
    };

  return true;
}

void BlynkPlugin::refresh(bool force) {
  if (!force && (_nextRefresh > millis())) return;
  GUI::showUpdatingIcon(GUI::Color_UpdatingPlugins);
  for (int i = 0; i < _nPins; i++) {
    int index = _pins[i].indexOf('/');
    int blynkIndex = _pins[i].substring(0, index).toInt();
    String rawPin = _pins[i].substring(index+1);
    _pinVals[i] = BlynkClient::readPin(_blynkIDs[blynkIndex], rawPin);
  }
  _nextRefresh = millis() + _refreshInterval;
  GUI::hideUpdatingIcon();
}
