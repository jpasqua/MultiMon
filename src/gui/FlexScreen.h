#ifndef FlexScreen_h
#define FlexScreen_h


//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoJson.h>
//                                  Local Includes
#include "../Basics.h"
#include "MMScreen.h"
//--------------- End:    Includes ---------------------------------------------


class FlexItem {
public:
  typedef enum {INT, FLOAT, STRING, BOOL} Type;

  void fromJSON(JsonObjectConst& item);

  uint16_t _x, _y;    // Location of the field
  uint16_t _w, _h;    // Size of the field
  uint16_t _xOff;     // x offset of text within field
  uint16_t _yOff;     // y offset of text within field
  GFXfont* _gfxFont;  // The GFXFont to use. If NULL, use a built-in font
  uint8_t  _font;     // Font to use if no GFXFont was given
  uint16_t _color;    // Color to use
  String   _format;   // Format string to use when displaying the value
  uint8_t  _datum;    // Justification of the output

  uint8_t _strokeWidth; 

  String _key;        // The key that will be used to get the value
  bool _isLiteral;    // If true, the key is really a literal value
  Type _dataType;

  void display(uint16_t bkg, Basics::StringMapper vc);
};


class FlexScreen : public Screen {
public:
  // ----- Functions that are specific to FlexScreen
  virtual ~FlexScreen();

  bool init(
      JsonObjectConst& screen,
      uint32_t refreshInterval,
      const Basics::StringMapper &vc);
  String getName() { return _name; }

  // ----- Functions defined in Screen class
  void display(bool activating = false);
  virtual void processPeriodicActivity();


private:
  FlexItem* _items;   // An array of items on the screen
  uint8_t   _nItems;  // How many items there are
  uint16_t  _bkg;     // Background color of the screen
  String    _name;    // Name of the screen
  uint32_t _refreshInterval;  // How often to refresh the display
  Basics::StringMapper _vc;   // A function to call to get a value for an item
  uint32_t  lastDisplayTime;  // Last time the display() function ran

  bool fromJSON(JsonObjectConst& screen);
};

#endif  // FlexScreen_h