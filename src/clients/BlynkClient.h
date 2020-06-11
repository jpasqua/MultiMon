#ifndef BlynkClient_h
#define BlynkClient_h

#include <Arduino.h>

class BlynkClient {
public:
  static String readPin(String blynkAppID, String pin);
};

#endif  // BlynkClient_h