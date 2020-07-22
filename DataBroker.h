/*
 * DataBroker:
 *    A centralized point to get data from all available sources
 *
 */

#ifndef DataBroker_h
#define DataBroker_h


//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
//                                  Third Party Libraries
//                                  Local Includes
//--------------- End:    Includes ---------------------------------------------

namespace DataBroker {
  namespace Printing {
    void nextCompletion(String &printer, String &formattedTime, uint32_t &delta);
  };

  String map(String& key);
};

#endif  // DataBroker_h