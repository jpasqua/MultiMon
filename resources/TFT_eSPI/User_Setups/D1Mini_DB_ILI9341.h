// Pin assignments for D1 Mini Daughter Board which connects to
// 2.4 / 2.8 / 3.2 ILI9341 TFT Displays
// See SetupX_Template.h for all options available

#define ILI9341_DRIVER

#undef  TFT_CS
#undef  TFT_DC
#undef  TFT_RST
#undef  TFT_LED
#undef  TOUCH_CS

#define TFT_CS  PIN_D8
#define TFT_DC  PIN_D4
#define TFT_RST -1
#define TFT_LED PIN_D3    // LED backlight brightness. Requires an added wire.
                          // Use -1 for no connection (full brightness)

#define TOUCH_CS  PIN_D2


#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT

#undef  SPI_FREQUENCY
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
//#define SUPPORT_TRANSACTIONS