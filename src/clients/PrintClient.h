#ifndef PrintClient_h
#define PrintClient_h

class PrintClient {
public:
  // ----- Types
  enum State {Offline, Operational, Complete, Printing};
    // This enum is ordered by "activeness". Offline is obviously the least active,
    // Operational means OctoPrint is connected to the printer - that is more active.
    // Complete means that although we aren't printing now, we did finish a print.
    // And Printing is the most active since activity is occuring now.

  // ----- State
  uint32_t      timeOfLastUpdate = 0;

  // ----- Interrogate the Printer
  virtual void updateState() = 0;

  // ----- Utility Functions
  virtual void acknowledgeCompletion() = 0;
  virtual void dumpToLog() = 0;

  // ----- Getters
  virtual bool isPrinting() = 0;
  virtual State getState() = 0;
  virtual float getPctComplete() = 0;
  virtual uint32_t getPrintTimeLeft() = 0;
  virtual uint32_t getElapsedTime() = 0;
  virtual String getFilename() = 0;
  virtual void getBedTemps(float &actual, float &target) = 0;
  virtual void getToolTemps(float &actual, float &target) = 0;
};

#endif  // PrintClient_h
