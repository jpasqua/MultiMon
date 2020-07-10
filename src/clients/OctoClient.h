
#ifndef OctoClient_h
#define OctoClient_h

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <JSONService.h>
//                                  Local Includes
#include "PrintClient.h"
//--------------- End:    Includes ---------------------------------------------


class JobState {
public:
  JobState() { reset(); }
  bool valid;                       // Is the data valid; i.e did we successfully read it from OctoPrint
  String state;                     // State of the job
  struct {
    String    name;                 // Name of the file being printed
    uint32_t  size;                 // Size of the file being printed
  } file;
  uint32_t averagePrintTime;
  uint32_t estimatedPrintTime;
  uint32_t lastPrintTime;
  uint32_t filamentLength;
  struct {
    uint32_t  filepos;              // Byte location in the file being processed
    uint32_t  printTime;            // Elapsed print time (seconds)
    uint32_t  printTimeLeft;        // Remaining print time (seconds)
    float     completion;           // 0.0-100.0, not 0.0-1.0
  } progress;

  void reset() {
    valid = false;
    state = "";
    file.name = "";
    file.size = 0;
    averagePrintTime = 0;
    estimatedPrintTime = 0;
    lastPrintTime = 0;
    filamentLength = 0;
    progress.filepos = 0;
    progress.printTime = 0;
    progress.printTimeLeft = 0;
    progress.completion = 0.0;
  }

  void dumpToLog() {
    if (!valid) Log.verbose(F("----- Job State: Values have not been set"));
    else {
      Log.verbose(F("----- Job State: %s -----"), state.c_str());
      if (state.startsWith("Offline")) return;
      Log.verbose(F("File"));
      Log.verbose(F("  fileName: %s"), file.name.c_str());
      Log.verbose(F("  fileSize: %d"), file.size);
      Log.verbose(F("Time"));
      Log.verbose(F("  averagePrintTime: %d (sec)"), averagePrintTime);
      Log.verbose(F("  estimatedPrintTime: %d (sec)"), estimatedPrintTime);
      Log.verbose(F("  lastPrintTime: %d (sec)"), lastPrintTime);
      Log.verbose(F("filamentLength: %d (mm)"), filamentLength);
      Log.verbose(F("Progress:"));
      Log.verbose(F("  Filepos: %d"), progress.filepos);
      Log.verbose(F("  PrintTime: %d (sec)"), progress.printTime);
      Log.verbose(F("  PrintTimeLeft: %d (sec)"), progress.printTimeLeft);
      Log.verbose(F("  Completion: %F"), progress.completion);
      Log.verbose(F("----------"));
    }
  }
};

class PrinterState {
public:
  PrinterState() { reset(); }

  bool valid;
  bool isPrinting;
  struct {
    float actual;
    float target;
  } toolTemp;
  struct {
    float actual;
    float target;
  } bedTemp;

  void reset() {
    valid = false;
    isPrinting = false;

    toolTemp.actual = 0.0;
    toolTemp.target = 0.0;
    bedTemp.actual = 0.0;
    bedTemp.target = 0.0;
  }

  void dumpToLog() {
    if (!valid) Log.verbose(F("Printer State: Values have not been set"));
    else {
      Log.verbose(F("----- Printer State: printing = %T"), isPrinting);
      Log.verbose(F("  Tool Temp: %F (C)"), toolTemp.actual);
      Log.verbose(F("  Tool Target Temp: %F (C)"), toolTemp.target);
      Log.verbose(F("  Tool Temp: %F (C)"), bedTemp.actual);
      Log.verbose(F("  Tool Target Temp: %F (C)"), bedTemp.target);
      Log.verbose(F("----------"));
    }
  }
};


class OctoClient : public PrintClient {
public:
  // ----- Constructors and initialization
  void init(String apiKey, String server, int port, String user, String pass);

  // ----- Interrogate the OctoPrint Server
  void updateState();

  // ----- Utility Functions
  void acknowledgeCompletion();
  inline void dumpToLog() { jobState.dumpToLog();  printerState.dumpToLog(); }

  // ----- Getters
  inline bool isPrinting() { return printerState.valid && printerState.isPrinting; }
  State getState();
  float getPctComplete() { return jobState.progress.completion; }
  uint32_t getPrintTimeLeft() { return jobState.progress.printTimeLeft; }
  uint32_t getElapsedTime() { return jobState.progress.printTime; }
  String getFilename() { return jobState.file.name; }
  void getBedTemps(float &actual, float &target) { actual = printerState.bedTemp.actual; target = printerState.bedTemp.target; }
  void getToolTemps(float &actual, float &target) { actual = printerState.toolTemp.actual; target = printerState.toolTemp.target; }

private:
  ServiceDetails  details;
  JSONService     *service = NULL;
  JobState      jobState;
  PrinterState  printerState;
  bool          completionAcknowledged = false;
  
  void getJobState();
  void getPrinterState();
};

#endif // OctoClient_h
