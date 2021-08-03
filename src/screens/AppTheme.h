/*
 * AppTheme
 *     Define theme elements that are specific to this app
 *
 * NOTES:
 * o ScreenMgr::Theme defines many common colors and fonts that are used by
 *   the implementation of Screen objects.
 * o An app-specific screen may want to use thematic elements that are
 *   not generic. Those can be defined here in the AppTheme namespace.
 *
 */

#ifndef AppTheme_h
#define AppTheme_h

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <TFT_eSPI.h>
//                                  Local Includes
//--------------- End:    Includes ---------------------------------------------

namespace AppTheme {
  static const uint32_t Color_Nickname = 0xE51D;      // Light Purple
  static const uint32_t Color_UpdatingPrinter = TFT_DARKGREEN;
  static const uint32_t Color_SplashOcto = TFT_GREEN;
  static const uint32_t Color_SplashDuet = TFT_BLUE;
  static const uint32_t Color_SplashRR   = 0x0488;
    // RR Logo color is #029346, 565 version of that is 0x0488
  static const uint32_t Color_SplashD3   = 0x02DF;
  //static const uint32_t Color_SplashD3   = 0x000E;
    // Duet3D Logo color is #040073, 565 version of that is 0x000E
};

#endif	// AppTheme_h