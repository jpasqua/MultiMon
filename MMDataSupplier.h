/*
 * MMDataSupplier:
 *    Supplies app-specific data to the WebThing DataBroker
 *
 */

#ifndef MMDataSupplier_h
#define MMDataSupplier_h

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
//                                  WebThing Includes
//                                  Local Includes
//--------------- End:    Includes ---------------------------------------------


namespace MMDataSupplier {
  constexpr char PrinterPrefix = 'P';

  namespace Printing {
    extern void nextCompletion(String &printer, String &formattedTime, uint32_t &delta);
  }

  extern void printerDataSupplier(const String& key, String& value);
}

#endif  // MMDataSupplier_h