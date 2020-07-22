/*
 * FlexScreen:
 *    Display values driven by a screen layout definition 
 *                    
 * TO DO:
 *
 * DONE
 *
 *
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Arduino.h>
#include <FS.h>
//                                  Third Party Libraries
//                                  Local Includes
#include "../Basics.h"
#include "../../DataBroker.h"
#include "MMScreen.h"
#include "FlexScreen.h"
//--------------- End:    Includes ---------------------------------------------

using GUI::tft;
using GUI::sprite;


/*------------------------------------------------------------------------------
 *
 * Local Utility Functions
 *
 *----------------------------------------------------------------------------*/

inline uint16_t mapColor(String colorSpecifier) {
  int index = 0;
  if (colorSpecifier.startsWith(F("0x"))) index = 2;
  else if (colorSpecifier.startsWith(F("#"))) index = 1;
  uint32_t hexVal = strtol(colorSpecifier.substring(index).c_str(), NULL, 16);
  return tft.color24to16(hexVal);
}

FlexItem::Type mapType(String t) {
  if (t.equalsIgnoreCase(F("INT"))) return FlexItem::Type::INT;
  if (t.equalsIgnoreCase(F("FLOAT"))) return FlexItem::Type::FLOAT;
  if (t.equalsIgnoreCase(F("STRING"))) return FlexItem::Type::STRING;
  if (t.equalsIgnoreCase(F("BOOL"))) return FlexItem::Type::BOOL;
  if (t.equalsIgnoreCase(F("CLOCK"))) return FlexItem::Type::CLOCK;
  return FlexItem::Type::STRING;
}

void mapKey(String input, String &key, bool &isLiteral) {
  if (input.isEmpty()) {
    isLiteral = true;
    key = input;    
  } else if (input[0] == '#') {
    isLiteral = true;
    key = input.substring(1);
  } else {
    isLiteral = false;
    key = input;
  }
}

uint8_t mapDatum(String justify) {
  if (justify.equalsIgnoreCase(F("TL"))) { return TL_DATUM;}
  if (justify.equalsIgnoreCase(F("TC"))) { return TC_DATUM;}
  if (justify.equalsIgnoreCase(F("TR"))) { return TR_DATUM;}
  if (justify.equalsIgnoreCase(F("ML"))) { return ML_DATUM;}
  if (justify.equalsIgnoreCase(F("MC"))) { return MC_DATUM;}
  if (justify.equalsIgnoreCase(F("MR"))) { return MR_DATUM;}
  if (justify.equalsIgnoreCase(F("BL"))) { return BL_DATUM;}
  if (justify.equalsIgnoreCase(F("BC"))) { return BC_DATUM;}
  if (justify.equalsIgnoreCase(F("BR"))) { return BR_DATUM;}

  return TL_DATUM;
}

void mapFont(String fontName, int8_t& gfxFont, uint8_t& font) {
  // Use a default if no matching font is found
  font = 2;
  gfxFont = -1;

  if (fontName.length() == 1 && isDigit(fontName[0])) {
    font = fontName[0] - '0';
    return;
  } 

  gfxFont = GUI::Font::idFromName(fontName);
} 

/*------------------------------------------------------------------------------
 *
 * FlexScreen Implementation
 *
 *----------------------------------------------------------------------------*/

FlexScreen::~FlexScreen() {
  // TO DO: Cleanup!
}

bool FlexScreen::init(
    JsonObjectConst& screen,
    uint32_t refreshInterval,
    const Basics::StringMapper &vc) {

  auto buttonHandler =[&](int id, Button::PressType type) -> void {
    Log.verbose(F("In Flex Screen Button Handler, id = %d, type = %d"), id, type);
    GUI::displayHomeScreen();
  };

  _vc = vc;
  _refreshInterval = refreshInterval;

  buttons = new Button[(nButtons = 1)];
  buttons[0].init(0, 0, Screen::Width, Screen::Height, buttonHandler, 0);

  _clock = NULL;
  return fromJSON(screen);
}

void FlexScreen::display(bool activating) {
  auto mapper = [&](String& key) -> String {
    if (key.isEmpty()) return "";
    if (key.equals(F("SCREEN_NAME"))) return _name;
    if (key[0] == '$') {
Log.verbose("FlexScreen::display, key = %s", key.c_str());
      return DataBroker::map(key);
    }
    else return _vc(key);
  };

  if (activating) { tft.fillScreen(_bkg); }
  for (int i = 0; i < _nItems; i++) {
    if (activating || !_items[i]._isLiteral) {
      _items[i].display(_bkg, mapper);
    }
  }
  lastDisplayTime = lastClockTime = millis();
}

String nullMapper(String& k) {
  (void)k;  // Avoid warning for unused parameter
  return "";
}

void FlexScreen:: processPeriodicActivity() {
  uint32_t curMillis = millis();
  if (curMillis - lastDisplayTime > _refreshInterval) display(false);
  else if (_clock != NULL  && (curMillis - lastClockTime > 1000L)) {
    _clock->display(_bkg, nullMapper);
    lastClockTime = curMillis;
  }
}

// ----- Private functions

bool FlexScreen::fromJSON(JsonObjectConst& screen) {
  // TO DO: If we are overwriting an existing screen
  // we need to clean up all the old data first

  JsonArrayConst itemArray = screen[F("items")];
  _nItems = itemArray.size();  
  _items = new FlexItem[_nItems];

  int i = 0;
  for (JsonObjectConst item : itemArray) {
    _items[i].fromJSON(item);
    if (_items[i]._dataType == FlexItem::Type::CLOCK) {
      _clock = &_items[i];
    }
    i++;
  }

  _bkg = mapColor(screen[F("bkg")].as<String>());
  _name = screen[F("name")].as<String>();

  return true;
}


/*------------------------------------------------------------------------------
 *
 * FlexItem Implementation
 *
 *----------------------------------------------------------------------------*/

void FlexItem::fromJSON(JsonObjectConst& item) {
  // What it is...
  _dataType = mapType(item[F("type")].as<String>());
  mapKey(String(item[F("key")]|""), _key, _isLiteral);

  // Where it goes...
  _x = item[F("x")]; _y = item[F("y")];
  _w = item[F("w")]; _h = item[F("h")];
  _xOff = item[F("xOff")]; _yOff = item[F("yOff")];

  // How it is displayed...
  mapFont(item[F("font")].as<String>(), _gfxFont, _font);
  _color = mapColor(item[F("color")].as<String>());
  _format = String(item[F("format")]|"");
  _datum = mapDatum(item[F("justify")].as<String>());

  _strokeWidth = item[F("strokeWidth")];
}

void FlexItem::display(uint16_t bkg, Basics::StringMapper vc) {
  sprite->setColorDepth(1);
  sprite->createSprite(_w, _h);
  sprite->fillSprite(GUI::Mono_Background);

  const char *fmt = _format.c_str();
  if (fmt[0] != 0) {
    int bufSize = Screen::Width/6 + 1; // Assume 6 pixel spacing is smallest font
    char buf[bufSize];

    String value = _isLiteral ? _key : vc(_key);
    switch (_dataType) {
      case FlexItem::Type::INT:
        sprintf(buf, fmt, value.toInt());
        break;
      case FlexItem::Type::FLOAT:
        sprintf(buf, fmt, value.toFloat());
        break;
      case FlexItem::Type::STRING:
        sprintf(buf, fmt, value.c_str());
        break;
      case FlexItem::Type::BOOL:
        {
          char c = value[0];
          bool bv = (c == 't' || c == 'T' || c == '1') ;
          sprintf(buf, fmt, bv ? F("True") : F("False"));
          break;
        }
      case FlexItem::Type::CLOCK:
        sprintf(buf, fmt, hourFormat12(), minute(), second());
        break;
    }
    if (_gfxFont >= 0) { GUI::Font::setUsingID(_gfxFont, sprite); }
    else { sprite->setTextFont(_font);}
    sprite->setTextColor(GUI::Mono_Foreground);
    sprite->setTextDatum(_datum);
    sprite->drawString(buf, _xOff, _yOff);
  }

  for (int i = 0; i < _strokeWidth; i++) {
    sprite->drawRect(i, i, _w-2*i, _h-2*i, GUI::Mono_Foreground);
  }

  sprite->setBitmapColor(_color, bkg);
  sprite->pushSprite(_x, _y);
  sprite->deleteSprite();
}

