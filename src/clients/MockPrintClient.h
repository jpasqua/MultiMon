#include "PrintClient.h"

static const uint8_t N_Names = 6;
static const char *names[N_Names] = {
  "covid19_headband_all_rc3_0.2mm_PLA.gcode",
  "Box (Bottom Facing USB)_0.2mm_PLA_UXL_2h57m.gcode",
  "D1 2.4 TFT Shield Box Alt3 v4_0.2mm_PLA.gcode",
  "Slanted Box Bottom_0.2mm_PLA_MK3_38m.gcode",
  "smallv3_router_table_insert_0.2mm_PLA_MK3_53m.gcode",
  "Reset shaft_0.2mm_PLA.gcode",
};
static constexpr float RoomTemp = 21.1f;

class MockState {
public:
  void init() {
    randomSeed(millis() + (ESP.getFreeHeap() * ESP.getHeapFragmentation()));

    if (random(0, 20) == 5) {   // A 1-in-20 chance the printer is idle
      setIdle();
      return;
    }

    state = PrintClient::State::Printing;
    fileName = names[random(0, N_Names)];

    uint16_t minutes;
    uint16_t timeRegion = random(0, 100);
    if (timeRegion < 40) minutes = random(10, 60);        // 40% of jobs are 10-59 minutes
    else if (timeRegion < 70) minutes = random(60, 120);  // 30% of jobs are 1-2   hours
    else if (timeRegion < 90) minutes = random(120, 300); // 20% of jobs are 2-5   hours
    else minutes = random(300, 12*60);                    // 10% of jobs are 5-12  hours
    totalPrintTime = minutes * 60;
    // totalPrintTime = random(5, 10) * 60; // Short times for quick tests

    elapsed = random(0, totalPrintTime/2);
    bedTarget = 210;
    bedActual = bedTarget + ((float)random(-100, 101))/100;
    toolTarget = 60;
    toolActual = toolTarget + ((float)random(-100, 101))/100;
  }

  void setIdle() {
    state = PrintClient::State::Operational;
    fileName = "";
    totalPrintTime = 0;
    elapsed = 0;
    bedTarget = toolTarget = 0.0;
    bedActual = toolActual = RoomTemp;
  }

  PrintClient::State state;
  String fileName;
  uint32_t totalPrintTime;
  uint32_t elapsed;
  float bedTarget, bedActual;
  float toolTarget, toolActual;
};

class MockPrintClient : public PrintClient {
public:
  MockPrintClient() {
    ms.init();
    timeOfLastUpdate = millis();
  }

  // ----- Interrogate the Printer
  void updateState() {
    uint32_t curTime = millis();
    uint32_t secsSinceUpdate = (curTime - timeOfLastUpdate)/1000L;
    timeOfLastUpdate = curTime;

    if (ms.state == PrintClient::State::Printing) {
      ms.elapsed += secsSinceUpdate;
      if (ms.elapsed >= ms.totalPrintTime) {
        ms.elapsed = ms.totalPrintTime;
        ms.state = PrintClient::State::Complete;
      } else {
        ms.bedActual = ms.bedTarget + ((float)random(-100, 101))/100;
        ms.toolActual = ms.toolTarget + ((float)random(-100, 101))/100;
      }
    } else if (ms.state == PrintClient::State::Complete) {
      // Drop the actual temps
      ms.bedActual -= 0.4f;  if (ms.bedActual  < RoomTemp) ms.bedActual  = RoomTemp;
      ms.toolActual -= 2.0f; if (ms.toolActual < RoomTemp) ms.toolActual = RoomTemp;
    }
  }

  // ----- Utility Functions
  void acknowledgeCompletion() { ms.setIdle();  }

  void dumpToLog() { }

  // ----- Getters
  bool isPrinting() { return (ms.state == PrintClient::State::Printing); }
  State getState() { return ms.state; }
  float getPctComplete() { 
    if (ms.state == Offline || ms.state == Operational) return 0.0f;
    if (ms.state == Complete) return 100.0f;
    // Assert(printerState == Printing)
    return (ms.elapsed*100.0f)/((float)ms.totalPrintTime);
  }
  uint32_t getPrintTimeLeft() {
    if (ms.state == Offline || ms.state == Operational) return 0.0f;
    if (ms.state == Complete) return 0;
    // Assert(printerState == Printing)
    return ms.totalPrintTime - ms.elapsed;
  }
  uint32_t getElapsedTime() { return ms.elapsed; }
  String getFilename() { return ms.fileName; }
  void getBedTemps(float &actual, float &target) {
    actual = ms.bedActual;
    target = ms.bedTarget;
  }
  void getToolTemps(float &actual, float &target) {
    actual = ms.toolActual;
    target = ms.toolTarget;
  }

private:
  MockState ms;
};
