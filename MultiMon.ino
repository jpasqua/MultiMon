/*
 * MultiMon:
 *    Use a color touch screen display to monitor for up to 4 OctoPrint/Duet3D servers
 *    Also displays time and weather information 
 *
 * NOTES:
 * o This is just boilerplate that initializes the app from the setup() function
 *   and gives the app cycles from the loop() function.
 * o The only thing that changes here from one WebThingApp to another is the
 *   definition of MainClassName.
 * o WebThingBasics is included just to force the IDE to pull in the WebThing
 *   library at build time. It isn't used by this file.
 *
 */

#define MainClassName MultiMonApp

#define str(s) #s
#define HEADER(c)  str(c.h)

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
//                                  WebThing Includes
#include <WTBasics.h>
//                                  Local Includes
#include HEADER(MainClassName)
//--------------- End:    Includes ---------------------------------------------


void setup() {
  MainClassName::create();  // Creates and starts the WTApp singleton
}

void loop() { 
  ((MainClassName*) wtApp)->loop();  // Passes control to the app's loop function
}
