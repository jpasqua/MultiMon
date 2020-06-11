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
  if (colorSpecifier.startsWith("0x")) index = 2;
  else if (colorSpecifier.startsWith("#")) index = 1;
  uint32_t hexVal = strtol(colorSpecifier.substring(index).c_str(), NULL, 16);
  return tft.color24to16(hexVal);
}

FlexItem::Type mapType(String t) {
  if (t.equalsIgnoreCase("INT")) return FlexItem::Type::INT;
  if (t.equalsIgnoreCase("FLOAT")) return FlexItem::Type::FLOAT;
  if (t.equalsIgnoreCase("STRING")) return FlexItem::Type::STRING;
  if (t.equalsIgnoreCase("BOOL")) return FlexItem::Type::BOOL;
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
  if (justify.equalsIgnoreCase("TL")) { return TL_DATUM;}
  if (justify.equalsIgnoreCase("TC")) { return TC_DATUM;}
  if (justify.equalsIgnoreCase("TR")) { return TR_DATUM;}
  if (justify.equalsIgnoreCase("ML")) { return ML_DATUM;}
  if (justify.equalsIgnoreCase("MC")) { return MC_DATUM;}
  if (justify.equalsIgnoreCase("MR")) { return MR_DATUM;}
  if (justify.equalsIgnoreCase("BL")) { return BL_DATUM;}
  if (justify.equalsIgnoreCase("BC")) { return BC_DATUM;}
  if (justify.equalsIgnoreCase("BR")) { return BR_DATUM;}

  return TL_DATUM;
}

static const struct  {
  const char *name;
  const GFXfont *font;
} GFXFonts[] = {
  {"M9",    &FreeMono9pt7b},
  {"MB9",   &FreeMonoBold9pt7b},
  {"MO9",   &FreeMonoOblique9pt7b},
  {"MBO9",  &FreeMonoBoldOblique9pt7b},

  {"S9",    &FreeSans9pt7b},
  {"SB9",   &FreeSansBold9pt7b},
  {"SO9",   &FreeSansOblique9pt7b},
  {"SBO9",  &FreeSansBoldOblique9pt7b},

  {"S12",   &FreeSans12pt7b},
  {"SB12",  &FreeSansBold12pt7b},
  {"SO12",  &FreeSansOblique12pt7b},
  {"SBO12", &FreeSansBoldOblique12pt7b},

  {"S18",   &FreeSans18pt7b},
  {"SB18",  &FreeSansBold18pt7b},
  {"SO18",  &FreeSansOblique18pt7b},
  {"SBO18", &FreeSansBoldOblique18pt7b},

  {"S24",   &FreeSans24pt7b},
  {"SB24",  &FreeSansBold24pt7b},
  {"SO24",  &FreeSansOblique24pt7b},
  {"SBO24", &FreeSansBoldOblique24pt7b}
};
static const uint8_t nGFXFonts = ARRAY_SIZE(GFXFonts);

void mapFont(String fontName, GFXfont* &gfxFont, uint8_t &font) {
  // Use a default if no matching font is found
  font = 2;
  gfxFont = NULL;

  if (fontName.length() == 1 && isDigit(fontName[0])) {
    font = fontName[0] - '0';
    return;
  } 

  for (int i = 0; i < nGFXFonts; i++) {
    if (fontName == GFXFonts[i].name) {
      gfxFont = const_cast<GFXfont*>(GFXFonts[i].font);
      return;
    }
  }
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
    Log.verbose("In Flex Screen Button Handler, id = %d, type = %d", id, type);
    GUI::displayHomeScreen();
  };

  _vc = vc;
  _refreshInterval = refreshInterval;

  buttons = new Button[(nButtons = 1)];
  buttons[0].init(0, 0, Screen::Width, Screen::Height, buttonHandler, 0);

  return fromJSON(screen);
}

void FlexScreen::display(bool activating) {
  auto mapper = [&](String& key) -> String {
    if (key.isEmpty()) return "";
    if (key.equals("SCREEN_NAME")) return _name;
    else return _vc(key);
  };

  if (activating) { tft.fillScreen(_bkg); }
  for (int i = 0; i < _nItems; i++) {
    if (activating || !_items[i]._isLiteral) {
      _items[i].display(_bkg, mapper);
    }
  }
  lastDisplayTime = millis();
}

void FlexScreen:: processPeriodicActivity() {
  if (millis() - lastDisplayTime > _refreshInterval) display(false);
}

// ----- Private functions

bool FlexScreen::fromJSON(JsonObjectConst& screen) {
  // TO DO: If we are overwriting an existing screen
  // we need to clean up all the old data first

  JsonArrayConst itemArray = screen["items"];
  _nItems = itemArray.size();  
  _items = new FlexItem[_nItems];

  int i = 0;
  for (JsonObjectConst item : itemArray) {
    _items[i++].fromJSON(item);
  }

  _bkg = mapColor(screen["bkg"].as<String>());
  _name = screen["name"].as<String>();

  return true;
}


/*------------------------------------------------------------------------------
 *
 * FlexItem Implementation
 *
 *----------------------------------------------------------------------------*/

void FlexItem::fromJSON(JsonObjectConst& item) {
  // What it is...
  _dataType = mapType(item["type"].as<String>());
  mapKey(item["key"].as<String>(), _key, _isLiteral);

  // Where it goes...
  _x = item["x"]; _y = item["y"];
  _w = item["w"]; _h = item["h"];
  _xOff = item["xOff"]; _yOff = item["yOff"];

  // How it is displayed...
  mapFont(item["font"].as<String>(), _gfxFont, _font);
  _color = mapColor(item["color"].as<String>());
  _format = String(item["format"]|"");
  _datum = mapDatum(item["justify"].as<String>());

  _strokeWidth = item["strokeWidth"];
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
        char c = value[0];
        bool bv = (c == 't' || c == 'T' || c == '1') ;
        sprintf(buf, fmt, bv ? "True" : "False");
        break;
    }
    if (_gfxFont != NULL) { sprite->setFreeFont(_gfxFont);}
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

