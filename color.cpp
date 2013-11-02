#include "color.h"
#include "cursesdef.h"
#include "options.h"
#include "rng.h"

#define HILIGHT COLOR_BLUE
void init_colors()
{
 start_color();

 init_pair( 1, COLOR_WHITE,   COLOR_BLACK  );
 init_pair( 2, COLOR_RED,     COLOR_BLACK  );
 init_pair( 3, COLOR_GREEN,   COLOR_BLACK  );
 init_pair( 4, COLOR_BLUE,    COLOR_BLACK  );
 init_pair( 5, COLOR_CYAN,    COLOR_BLACK  );
 init_pair( 6, COLOR_MAGENTA, COLOR_BLACK  );
 init_pair( 7, COLOR_YELLOW,  COLOR_BLACK  );

 // Inverted Colors
 init_pair( 8, COLOR_BLACK,   COLOR_WHITE  );
 init_pair( 9, COLOR_BLACK,   COLOR_RED    );
 init_pair(10, COLOR_BLACK,   COLOR_GREEN  );
 init_pair(11, COLOR_BLACK,   COLOR_BLUE   );
 init_pair(12, COLOR_BLACK,   COLOR_CYAN   );
 init_pair(13, COLOR_BLACK,   COLOR_MAGENTA);
 init_pair(14, COLOR_BLACK,   COLOR_YELLOW );

 // Highlighted - blue background
 init_pair(15, COLOR_WHITE,   HILIGHT);
 init_pair(16, COLOR_RED,     HILIGHT);
 init_pair(17, COLOR_GREEN,   HILIGHT);
 init_pair(18, COLOR_BLUE,    HILIGHT);
 init_pair(19, COLOR_CYAN,    HILIGHT);
 init_pair(20, COLOR_BLACK,   HILIGHT);
 init_pair(21, COLOR_MAGENTA, HILIGHT);
 init_pair(22, COLOR_YELLOW,  HILIGHT);

 // Red background - for monsters on fire
 init_pair(23, COLOR_WHITE,   COLOR_RED);
 init_pair(24, COLOR_RED,     COLOR_RED);
 init_pair(25, COLOR_GREEN,   COLOR_RED);
 init_pair(26, COLOR_BLUE,    COLOR_RED);
 init_pair(27, COLOR_CYAN,    COLOR_RED);
 init_pair(28, COLOR_MAGENTA, COLOR_RED);
 init_pair(29, COLOR_YELLOW,  COLOR_RED);

 init_pair(30, COLOR_BLACK,   COLOR_BLACK);
 init_pair(31, COLOR_WHITE,   COLOR_BLACK);

 init_pair(32, COLOR_BLACK, COLOR_WHITE);
 init_pair(33, COLOR_WHITE, COLOR_WHITE);
 init_pair(34, COLOR_RED, COLOR_WHITE);
 init_pair(35, COLOR_GREEN, COLOR_WHITE);
 init_pair(36, COLOR_YELLOW, COLOR_WHITE);
 init_pair(37, COLOR_BLUE, COLOR_WHITE);
 init_pair(38, COLOR_MAGENTA, COLOR_WHITE);
 init_pair(39, COLOR_CYAN, COLOR_WHITE);

 init_pair(40, COLOR_BLACK, COLOR_GREEN);
 init_pair(41, COLOR_WHITE, COLOR_GREEN);
 init_pair(42, COLOR_RED, COLOR_GREEN);
 init_pair(43, COLOR_GREEN, COLOR_GREEN);
 init_pair(44, COLOR_YELLOW, COLOR_GREEN);
 init_pair(45, COLOR_BLUE, COLOR_GREEN);
 init_pair(46, COLOR_MAGENTA, COLOR_GREEN);
 init_pair(47, COLOR_CYAN, COLOR_GREEN);

 init_pair(48, COLOR_BLACK, COLOR_YELLOW);
 init_pair(49, COLOR_WHITE, COLOR_YELLOW);
 init_pair(50, COLOR_RED, COLOR_YELLOW);
 init_pair(51, COLOR_GREEN, COLOR_YELLOW);
 init_pair(52, COLOR_YELLOW, COLOR_YELLOW);
 init_pair(53, COLOR_BLUE, COLOR_YELLOW);
 init_pair(54, COLOR_MAGENTA, COLOR_YELLOW);
 init_pair(55, COLOR_CYAN, COLOR_YELLOW);

 init_pair(56, COLOR_BLACK, COLOR_MAGENTA);
 init_pair(57, COLOR_WHITE, COLOR_MAGENTA);
 init_pair(58, COLOR_RED, COLOR_MAGENTA);
 init_pair(59, COLOR_GREEN, COLOR_MAGENTA);
 init_pair(60, COLOR_YELLOW, COLOR_MAGENTA);
 init_pair(61, COLOR_BLUE, COLOR_MAGENTA);
 init_pair(62, COLOR_MAGENTA, COLOR_MAGENTA);
 init_pair(63, COLOR_CYAN, COLOR_MAGENTA);

 init_pair(64, COLOR_BLACK, COLOR_CYAN);
 init_pair(65, COLOR_WHITE, COLOR_CYAN);
 init_pair(66, COLOR_RED, COLOR_CYAN);
 init_pair(67, COLOR_GREEN, COLOR_CYAN);
 init_pair(68, COLOR_YELLOW, COLOR_CYAN);
 init_pair(69, COLOR_BLUE, COLOR_CYAN);
 init_pair(70, COLOR_MAGENTA, COLOR_CYAN);
 init_pair(71, COLOR_CYAN, COLOR_CYAN);

}

int color_to_int(nc_color col)
{
 switch (col) {
  case c_black  : return  0;
  case c_white  : return  1;
  case c_ltgray : return  2;
  case c_dkgray : return  3;
  case c_red    : return  4;
  case c_green  : return  5;
  case c_blue   : return  6;
  case c_cyan   : return  7;
  case c_magenta: return  8;
  case c_brown  : return  9;
  case c_ltred  : return 10;
  case c_ltgreen: return 11;
  case c_ltblue : return 12;
  case c_ltcyan : return 13;
  case c_pink   : return 14;
  case c_yellow : return 15;

  case h_black  : return 16;
  case h_white  : return 17;
  case h_ltgray : return 18;
  case h_dkgray : return 19;
  case h_red    : return 20;
  case h_green  : return 21;
  case h_cyan   : return 23;
  case h_magenta: return 24;
  case h_brown  : return 25;
  case h_ltred  : return 26;
  case h_ltgreen: return 27;
  case h_ltblue : return 28;
  case h_ltcyan : return 29;
  case h_pink   : return 30;
  case h_yellow : return 31;

  case i_white  : return 33;
  case i_ltgray : return 34;
  case i_dkgray : return 35;
  case i_red    : return 36;
  case i_green  : return 37;
  case i_blue   : return 38;
  case i_cyan   : return 39;
  case i_magenta: return 40;
  case i_brown  : return 41;
  case i_ltred  : return 42;
  case i_ltgreen: return 43;
  case i_ltblue : return 44;
  case i_ltcyan : return 45;
  case i_pink   : return 46;
  case i_yellow : return 47;

  case c_white_red  : return 48;
  case c_ltgray_red : return 49;
  case c_green_red  : return 52;
  case c_blue_red   : return 53;
  case c_cyan_red   : return 54;
  case c_magenta_red: return 55;
  case c_brown_red  : return 56;
  case c_ltred_red  : return 57;
  case c_ltgreen_red: return 58;
  case c_ltblue_red : return 59;
  case c_ltcyan_red : return 60;
  case c_pink_red   : return 61;
  case c_yellow_red : return 62;

  case c_unset      : return 63;

   case c_black_white  : return 64;
   case c_dkgray_white  : return 65;
   case c_ltgray_white  : return 66;
   case c_white_white  : return 67;
   case c_red_white  : return 68;
   case c_ltred_white  : return 69;
   case c_green_white  : return 70;
   case c_ltgreen_white  : return 71;
   case c_brown_white  : return 72;
   case c_yellow_white  : return 73;
   case c_blue_white  : return 74;
   case c_ltblue_white  : return 75;
   case c_magenta_white  : return 76;
   case c_pink_white  : return 77;
   case c_cyan_white  : return 78;
   case c_ltcyan_white  : return 79;

   case c_black_green  : return 80;
   case c_dkgray_green  : return 81;
   case c_ltgray_green  : return 82;
   case c_white_green  : return 83;
   case c_red_green  : return 84;
   case c_ltred_green  : return 85;
   case c_green_green  : return 86;
   case c_ltgreen_green  : return 87;
   case c_brown_green  : return 88;
   case c_yellow_green  : return 89;
   case c_blue_green  : return 90;
   case c_ltblue_green  : return 91;
   case c_magenta_green  : return 92;
   case c_pink_green  : return 93;
   case c_cyan_green  : return 94;
   case c_ltcyan_green  : return 95;

   case c_black_yellow  : return 96;
   case c_dkgray_yellow  : return 97;
   case c_ltgray_yellow  : return 98;
   case c_white_yellow  : return 99;
   case c_red_yellow  : return 100;
   case c_ltred_yellow  : return 101;
   case c_green_yellow  : return 102;
   case c_ltgreen_yellow  : return 103;
   case c_brown_yellow  : return 104;
   case c_yellow_yellow  : return 105;
   case c_blue_yellow  : return 106;
   case c_ltblue_yellow  : return 107;
   case c_magenta_yellow  : return 108;
   case c_pink_yellow  : return 109;
   case c_cyan_yellow  : return 110;
   case c_ltcyan_yellow  : return 111;

   case c_black_magenta  : return 112;
   case c_dkgray_magenta  : return 113;
   case c_ltgray_magenta  : return 114;
   case c_white_magenta  : return 115;
   case c_red_magenta  : return 116;
   case c_ltred_magenta  : return 117;
   case c_green_magenta  : return 118;
   case c_ltgreen_magenta  : return 119;
   case c_brown_magenta  : return 120;
   case c_yellow_magenta  : return 121;
   case c_blue_magenta  : return 122;
   case c_ltblue_magenta  : return 123;
   case c_magenta_magenta  : return 124;
   case c_pink_magenta  : return 125;
   case c_cyan_magenta  : return 126;
   case c_ltcyan_magenta  : return 127;

   case c_black_cyan  : return 128;
   case c_dkgray_cyan  : return 129;
   case c_ltgray_cyan  : return 130;
   case c_white_cyan  : return 131;
   case c_red_cyan  : return 132;
   case c_ltred_cyan  : return 133;
   case c_green_cyan  : return 134;
   case c_ltgreen_cyan  : return 135;
   case c_brown_cyan  : return 136;
   case c_yellow_cyan  : return 137;
   case c_blue_cyan  : return 138;
   case c_ltblue_cyan  : return 139;
   case c_magenta_cyan  : return 140;
   case c_pink_cyan  : return 141;
   case c_cyan_cyan  : return 142;
   case c_ltcyan_cyan  : return 143;

 }
 return 0;
}

nc_color int_to_color(int key)
{
 switch (key) {
  case  0: return c_black  ;
  case  1: return c_white  ;
  case  2: return c_ltgray ;
  case  3: return c_dkgray ;
  case  4: return c_red    ;
  case  5: return c_green  ;
  case  6: return c_blue   ;
  case  7: return c_cyan   ;
  case  8: return c_magenta;
  case  9: return c_brown  ;
  case 10: return c_ltred  ;
  case 11: return c_ltgreen;
  case 12: return c_ltblue ;
  case 13: return c_ltcyan ;
  case 14: return c_pink   ;
  case 15: return c_yellow ;

  case 16: return h_black  ;
  case 17: return h_white  ;
  case 18: return h_ltgray ;
  case 19: return h_dkgray ;
  case 20: return h_red    ;
  case 21: return h_green  ;
  case 22: return h_blue   ;
  case 23: return h_cyan   ;
  case 24: return h_magenta;
  case 25: return h_brown  ;
  case 26: return h_ltred  ;
  case 27: return h_ltgreen;
  case 28: return h_ltblue ;
  case 29: return h_ltcyan ;
  case 30: return h_pink   ;
  case 31: return h_yellow ;

  case 32: return i_black  ;
  case 33: return i_white  ;
  case 34: return i_ltgray ;
  case 35: return i_dkgray ;
  case 36: return i_red    ;
  case 37: return i_green  ;
  case 38: return i_blue   ;
  case 39: return i_cyan   ;
  case 40: return i_magenta;
  case 41: return i_brown  ;
  case 42: return i_ltred  ;
  case 43: return i_ltgreen;
  case 44: return i_ltblue ;
  case 45: return i_ltcyan ;
  case 46: return i_pink   ;
  case 47: return i_yellow ;

  case 48: return c_white_red  ;
  case 49: return c_ltgray_red ;
  case 50: return c_dkgray_red ;
  case 51: return c_red_red    ;
  case 52: return c_green_red  ;
  case 53: return c_blue_red   ;
  case 54: return c_cyan_red   ;
  case 55: return c_magenta_red;
  case 56: return c_brown_red  ;
  case 57: return c_ltred_red  ;
  case 58: return c_ltgreen_red;
  case 59: return c_ltblue_red ;
  case 60: return c_ltcyan_red ;
  case 61: return c_pink_red   ;
  case 62: return c_yellow_red ;

  case 63: return c_unset;

   case 64: return c_black_white;
   case 65: return c_dkgray_white;
   case 66: return c_ltgray_white;
   case 67: return c_white_white;
   case 68: return c_red_white;
   case 69: return c_ltred_white;
   case 70: return c_green_white;
   case 71: return c_ltgreen_white;
   case 72: return c_brown_white;
   case 73: return c_yellow_white;
   case 74: return c_blue_white;
   case 75: return c_ltblue_white;
   case 76: return c_magenta_white;
   case 77: return c_pink_white;
   case 78: return c_cyan_white;
   case 79: return c_ltcyan_white;
 
   case 80: return c_black_green;
   case 81: return c_dkgray_green;
   case 82: return c_ltgray_green;
   case 83: return c_white_green;
   case 84: return c_red_green;
   case 85: return c_ltred_green;
   case 86: return c_green_green;
   case 87: return c_ltgreen_green;
   case 88: return c_brown_green;
   case 89: return c_yellow_green;
   case 90: return c_blue_green;
   case 91: return c_ltblue_green;
   case 92: return c_magenta_green;
   case 93: return c_pink_green;
   case 94: return c_cyan_green;
   case 95: return c_ltcyan_green;
 
   case 96: return c_black_yellow;
   case 97: return c_dkgray_yellow;
   case 98: return c_ltgray_yellow;
   case 99: return c_white_yellow;
   case 100: return c_red_yellow;
   case 101: return c_ltred_yellow;
   case 102: return c_green_yellow;
   case 103: return c_ltgreen_yellow;
   case 104: return c_brown_yellow;
   case 105: return c_yellow_yellow;
   case 106: return c_blue_yellow;
   case 107: return c_ltblue_yellow;
   case 108: return c_magenta_yellow;
   case 109: return c_pink_yellow;
   case 110: return c_cyan_yellow;
   case 111: return c_ltcyan_yellow;
 
   case 112: return c_black_magenta;
   case 113: return c_dkgray_magenta;
   case 114: return c_ltgray_magenta;
   case 115: return c_white_magenta;
   case 116: return c_red_magenta;
   case 117: return c_ltred_magenta;
   case 118: return c_green_magenta;
   case 119: return c_ltgreen_magenta;
   case 120: return c_brown_magenta;
   case 121: return c_yellow_magenta;
   case 122: return c_blue_magenta;
   case 123: return c_ltblue_magenta;
   case 124: return c_magenta_magenta;
   case 125: return c_pink_magenta;
   case 126: return c_cyan_magenta;
   case 127: return c_ltcyan_magenta;
 
   case 128: return c_black_cyan;
   case 129: return c_dkgray_cyan;
   case 130: return c_ltgray_cyan;
   case 131: return c_white_cyan;
   case 132: return c_red_cyan;
   case 133: return c_ltred_cyan;
   case 134: return c_green_cyan;
   case 135: return c_ltgreen_cyan;
   case 136: return c_brown_cyan;
   case 137: return c_yellow_cyan;
   case 138: return c_blue_cyan;
   case 139: return c_ltblue_cyan;
   case 140: return c_magenta_cyan;
   case 141: return c_pink_cyan;
   case 142: return c_cyan_cyan;
   case 143: return c_ltcyan_cyan;

 }
 return c_black;
}

nc_color hilite(nc_color c)
{
    switch (c) {
    case c_white:   return h_white;
    case c_ltgray:  return h_ltgray;
    case c_dkgray:  return h_dkgray;
    case c_red:     return h_red;
    case c_green:   return h_green;
    case c_blue:    return h_blue;
    case c_cyan:    return h_cyan;
    case c_magenta: return h_magenta;
    case c_brown:   return h_brown;
    case c_ltred:   return h_ltred;
    case c_ltgreen: return h_ltgreen;
    case c_ltblue:  return h_ltblue;
    case c_ltcyan:  return h_ltcyan;
    case c_pink:    return h_pink;
    case c_yellow:  return h_yellow;
    }
    return h_white;
}

nc_color invert_color(nc_color c)
{
    if (OPTIONS["NO_BRIGHT_BACKGROUNDS"]) {
        switch (c) {
        case c_white:
        case c_ltgray:
        case c_dkgray:  return i_ltgray;
        case c_red:
        case c_ltred:   return i_red;
        case c_green:
        case c_ltgreen: return i_green;
        case c_blue:
        case c_ltblue:  return i_blue;
        case c_cyan:
        case c_ltcyan:  return i_cyan;
        case c_magenta:
        case c_pink:    return i_magenta;
        case c_brown:
        case c_yellow:  return i_brown;
        }
    }

    switch (c) {
    case c_white:   return i_white;
    case c_ltgray:  return i_ltgray;
    case c_dkgray:  return i_dkgray;
    case c_red:     return i_red;
    case c_green:   return i_green;
    case c_blue:    return i_blue;
    case c_cyan:    return i_cyan;
    case c_magenta: return i_magenta;
    case c_brown:   return i_brown;
    case c_yellow:  return i_yellow;
    case c_ltred:   return i_ltred;
    case c_ltgreen: return i_ltgreen;
    case c_ltblue:  return i_ltblue;
    case c_ltcyan:  return i_ltcyan;
    case c_pink:    return i_pink;
    }

    return c_pink;
}

nc_color red_background(nc_color c)
{
    switch (c) {
    case c_white:   return c_white_red;
    case c_ltgray:  return c_ltgray_red;
    case c_dkgray:  return c_dkgray_red;
    case c_red:     return c_red_red;
    case c_green:   return c_green_red;
    case c_blue:    return c_blue_red;
    case c_cyan:    return c_cyan_red;
    case c_magenta: return c_magenta_red;
    case c_brown:   return c_brown_red;
    case c_ltred:   return c_ltred_red;
    case c_ltgreen: return c_ltgreen_red;
    case c_ltblue:  return c_ltblue_red;
    case c_ltcyan:  return c_ltcyan_red;
    case c_pink:    return c_pink_red;
    case c_yellow:  return c_yellow_red;
    }
    return c_white_red;
}

/// colors need to be totally redone, really..
nc_color white_background(nc_color c)
{
    switch (c) {
    case c_black:   return c_black_white;
    case c_dkgray:  return c_dkgray_white;
    case c_ltgray:  return c_ltgray_white;
    case c_white:   return c_white_white;
    case c_red:     return c_red_white;
    case c_ltred:   return c_ltred_white;
    case c_green:   return c_green_white;
    case c_ltgreen: return c_ltgreen_white;
    case c_brown:   return c_brown_white;
    case c_yellow:  return c_yellow_white;
    case c_blue:    return c_blue_white;
    case c_ltblue:  return c_ltblue_white;
    case c_magenta: return c_magenta_white;
    case c_pink:    return c_pink_white;
    case c_cyan:    return c_cyan_white;
    case c_ltcyan:  return c_ltcyan_white;
    default: return c_black_white;
    }
}

nc_color green_background(nc_color c)
{
    switch (c) {
    case c_black:   return c_black_green;
    case c_dkgray:  return c_dkgray_green;
    case c_ltgray:  return c_ltgray_green;
    case c_white:   return c_white_green;
    case c_red:     return c_red_green;
    case c_ltred:   return c_ltred_green;
    case c_green:   return c_green_green;
    case c_ltgreen: return c_ltgreen_green;
    case c_brown:   return c_brown_green;
    case c_yellow:  return c_yellow_green;
    case c_blue:    return c_blue_green;
    case c_ltblue:  return c_ltblue_green;
    case c_magenta: return c_magenta_green;
    case c_pink:    return c_pink_green;
    case c_cyan:    return c_cyan_green;
    case c_ltcyan:  return c_ltcyan_green;
    default: return c_black_green;
    }
}

nc_color yellow_background(nc_color c)
{
    switch (c) {
    case c_black:   return c_black_yellow;
    case c_dkgray:  return c_dkgray_yellow;
    case c_ltgray:  return c_ltgray_yellow;
    case c_white:   return c_white_yellow;
    case c_red:     return c_red_yellow;
    case c_ltred:   return c_ltred_yellow;
    case c_green:   return c_green_yellow;
    case c_ltgreen: return c_ltgreen_yellow;
    case c_brown:   return c_brown_yellow;
    case c_yellow:  return c_yellow_yellow;
    case c_blue:    return c_blue_yellow;
    case c_ltblue:  return c_ltblue_yellow;
    case c_magenta: return c_magenta_yellow;
    case c_pink:    return c_pink_yellow;
    case c_cyan:    return c_cyan_yellow;
    case c_ltcyan:  return c_ltcyan_yellow;
    default: return c_black_yellow;
    }
}

nc_color magenta_background(nc_color c)
{
    switch (c) {
    case c_black:   return c_black_magenta;
    case c_dkgray:  return c_dkgray_magenta;
    case c_ltgray:  return c_ltgray_magenta;
    case c_white:   return c_white_magenta;
    case c_red:     return c_red_magenta;
    case c_ltred:   return c_ltred_magenta;
    case c_green:   return c_green_magenta;
    case c_ltgreen: return c_ltgreen_magenta;
    case c_brown:   return c_brown_magenta;
    case c_yellow:  return c_yellow_magenta;
    case c_blue:    return c_blue_magenta;
    case c_ltblue:  return c_ltblue_magenta;
    case c_magenta: return c_magenta_magenta;
    case c_pink:    return c_pink_magenta;
    case c_cyan:    return c_cyan_magenta;
    case c_ltcyan:  return c_ltcyan_magenta;
    default: return c_black_magenta;
    }
}

nc_color cyan_background(nc_color c)
{
    switch (c) {
    case c_black:   return c_black_cyan;
    case c_dkgray:  return c_dkgray_cyan;
    case c_ltgray:  return c_ltgray_cyan;
    case c_white:   return c_white_cyan;
    case c_red:     return c_red_cyan;
    case c_ltred:   return c_ltred_cyan;
    case c_green:   return c_green_cyan;
    case c_ltgreen: return c_ltgreen_cyan;
    case c_brown:   return c_brown_cyan;
    case c_yellow:  return c_yellow_cyan;
    case c_blue:    return c_blue_cyan;
    case c_ltblue:  return c_ltblue_cyan;
    case c_magenta: return c_magenta_cyan;
    case c_pink:    return c_pink_cyan;
    case c_cyan:    return c_cyan_cyan;
    case c_ltcyan:  return c_ltcyan_cyan;
    default: return c_black_cyan;
    }
}

nc_color rand_color()
{
    switch (rng(0, 9)) {
    case 0: return c_white;
    case 1: return c_ltgray;
    case 2: return c_green;
    case 3: return c_red;
    case 4: return c_yellow;
    case 5: return c_blue;
    case 6: return c_ltblue;
    case 7: return c_pink;
    case 8: return c_magenta;
    case 9: return c_brown;
    }
    return c_dkgray;
}

std::map<std::string,nc_color> colormap;

void init_colormap()
{
    colormap["black"] = c_black;
    colormap["white"] = c_white;
    colormap["ltgray"] = c_ltgray;
    colormap["light_gray"] = c_ltgray;
    colormap["dkgray"] = c_dkgray;
    colormap["dark_gray"] = c_dkgray;
    colormap["red"] = c_red;
    colormap["green"] = c_green;
    colormap["blue"] = c_blue;
    colormap["cyan"] = c_cyan;
    colormap["magenta"] = c_magenta;
    colormap["brown"] = c_brown;
    colormap["ltred"] = c_ltred;
    colormap["light_red"] = c_ltred;
    colormap["ltgreen"] = c_ltgreen;
    colormap["light_green"] = c_ltgreen;
    colormap["ltblue"] = c_ltblue;
    colormap["light_blue"] = c_ltblue;
    colormap["ltcyan"] = c_ltcyan;
    colormap["light_cyan"] = c_ltcyan;
    colormap["pink"] = c_pink;
    colormap["yellow"] = c_yellow;

    colormap["h_black"] = h_black;
    colormap["h_white"] = h_white;
    colormap["h_ltgray"] = h_ltgray;
    colormap["h_dkgray"] = h_dkgray;
    colormap["h_red"] = h_red;
    colormap["h_green"] = h_green;
    colormap["h_blue"] = h_blue;
    colormap["h_cyan"] = h_cyan;
    colormap["h_magenta"] = h_magenta;
    colormap["h_brown"] = h_brown;
    colormap["h_ltred"] = h_ltred;
    colormap["h_ltgreen"] = h_ltgreen;
    colormap["h_ltblue"] = h_ltblue;
    colormap["h_ltcyan"] = h_ltcyan;
    colormap["h_pink"] = h_pink;
    colormap["h_yellow"] = h_yellow;

    colormap["i_black"] = i_black;
    colormap["i_white"] = i_white;
    colormap["i_ltgray"] = i_ltgray;
    colormap["i_dkgray"] = i_dkgray;
    colormap["i_red"] = i_red;
    colormap["i_green"] = i_green;
    colormap["i_blue"] = i_blue;
    colormap["i_cyan"] = i_cyan;
    colormap["i_magenta"] = i_magenta;
    colormap["i_brown"] = i_brown;
    colormap["i_ltred"] = i_ltred;
    colormap["i_ltgreen"] = i_ltgreen;
    colormap["i_ltblue"] = i_ltblue;
    colormap["i_ltcyan"] = i_ltcyan;
    colormap["i_pink"] = i_pink;
    colormap["i_yellow"] = i_yellow;

    colormap["white_red"] = c_white_red;
    colormap["ltgray_red"] = c_ltgray_red;
    colormap["light_gray_red"] = c_ltgray_red;
    colormap["dkgray_red"] = c_dkgray_red;
    colormap["dark_gray_red"] = c_dkgray_red;
    colormap["red_red"] = c_red_red;
    colormap["green_red"] = c_green_red;
    colormap["blue_red"] = c_blue_red;
    colormap["cyan_red"] = c_cyan_red;
    colormap["magenta_red"] = c_magenta_red;
    colormap["brown_red"] = c_brown_red;
    colormap["ltred_red"] = c_ltred_red;
    colormap["light_red_red"] = c_ltred_red;
    colormap["ltgreen_red"] = c_ltgreen_red;
    colormap["light_green_red"] = c_ltgreen_red;
    colormap["ltblue_red"] = c_ltblue_red;
    colormap["light_blue_red"] = c_ltblue_red;
    colormap["ltcyan_red"] = c_ltcyan_red;
    colormap["light_cyan_red"] = c_ltcyan_red;
    colormap["pink_red"] = c_pink_red;
    colormap["yellow_red"] = c_yellow_red;

    colormap["unset"] = c_unset;

    colormap["black_white"] = c_black_white;
    colormap["dkgray_white"] = c_dkgray_white;
    colormap["ltgray_white"] = c_ltgray_white;
    colormap["white_white"] = c_white_white;
    colormap["red_white"] = c_red_white;
    colormap["ltred_white"] = c_ltred_white;
    colormap["green_white"] = c_green_white;
    colormap["ltgreen_white"] = c_ltgreen_white;
    colormap["brown_white"] = c_brown_white;
    colormap["yellow_white"] = c_yellow_white;
    colormap["blue_white"] = c_blue_white;
    colormap["ltblue_white"] = c_ltblue_white;
    colormap["magenta_white"] = c_magenta_white;
    colormap["pink_white"] = c_pink_white;
    colormap["cyan_white"] = c_cyan_white;
    colormap["ltcyan_white"] = c_ltcyan_white;

    colormap["black_green"] = c_black_green;
    colormap["dkgray_green"] = c_dkgray_green;
    colormap["ltgray_green"] = c_ltgray_green;
    colormap["white_green"] = c_white_green;
    colormap["red_green"] = c_red_green;
    colormap["ltred_green"] = c_ltred_green;
    colormap["green_green"] = c_green_green;
    colormap["ltgreen_green"] = c_ltgreen_green;
    colormap["brown_green"] = c_brown_green;
    colormap["yellow_green"] = c_yellow_green;
    colormap["blue_green"] = c_blue_green;
    colormap["ltblue_green"] = c_ltblue_green;
    colormap["magenta_green"] = c_magenta_green;
    colormap["pink_green"] = c_pink_green;
    colormap["cyan_green"] = c_cyan_green;
    colormap["ltcyan_green"] = c_ltcyan_green;
    colormap["black_yellow"] = c_black_yellow;
    colormap["dkgray_yellow"] = c_dkgray_yellow;
    colormap["ltgray_yellow"] = c_ltgray_yellow;
    colormap["white_yellow"] = c_white_yellow;
    colormap["red_yellow"] = c_red_yellow;
    colormap["ltred_yellow"] = c_ltred_yellow;
    colormap["green_yellow"] = c_green_yellow;
    colormap["ltgreen_yellow"] = c_ltgreen_yellow;
    colormap["brown_yellow"] = c_brown_yellow;
    colormap["yellow_yellow"] = c_yellow_yellow;
    colormap["blue_yellow"] = c_blue_yellow;
    colormap["ltblue_yellow"] = c_ltblue_yellow;
    colormap["magenta_yellow"] = c_magenta_yellow;
    colormap["pink_yellow"] = c_pink_yellow;
    colormap["cyan_yellow"] = c_cyan_yellow;
    colormap["ltcyan_yellow"] = c_ltcyan_yellow;
    colormap["black_magenta"] = c_black_magenta;
    colormap["dkgray_magenta"] = c_dkgray_magenta;
    colormap["ltgray_magenta"] = c_ltgray_magenta;
    colormap["white_magenta"] = c_white_magenta;
    colormap["red_magenta"] = c_red_magenta;
    colormap["ltred_magenta"] = c_ltred_magenta;
    colormap["green_magenta"] = c_green_magenta;
    colormap["ltgreen_magenta"] = c_ltgreen_magenta;
    colormap["brown_magenta"] = c_brown_magenta;
    colormap["yellow_magenta"] = c_yellow_magenta;
    colormap["blue_magenta"] = c_blue_magenta;
    colormap["ltblue_magenta"] = c_ltblue_magenta;
    colormap["magenta_magenta"] = c_magenta_magenta;
    colormap["pink_magenta"] = c_pink_magenta;
    colormap["cyan_magenta"] = c_cyan_magenta;
    colormap["ltcyan_magenta"] = c_ltcyan_magenta;
    colormap["black_cyan"] = c_black_cyan;
    colormap["dkgray_cyan"] = c_dkgray_cyan;
    colormap["ltgray_cyan"] = c_ltgray_cyan;
    colormap["white_cyan"] = c_white_cyan;
    colormap["red_cyan"] = c_red_cyan;
    colormap["ltred_cyan"] = c_ltred_cyan;
    colormap["green_cyan"] = c_green_cyan;
    colormap["ltgreen_cyan"] = c_ltgreen_cyan;
    colormap["brown_cyan"] = c_brown_cyan;
    colormap["yellow_cyan"] = c_yellow_cyan;
    colormap["blue_cyan"] = c_blue_cyan;
    colormap["ltblue_cyan"] = c_ltblue_cyan;
    colormap["magenta_cyan"] = c_magenta_cyan;
    colormap["pink_cyan"] = c_pink_cyan;
    colormap["cyan_cyan"] = c_cyan_cyan;
    colormap["ltcyan_cyan"] = c_ltcyan_cyan;
}

/**
 * Given the name of a color, returns the nc_color constant that matches. If
 * no match is found, c_white is returned.
 * @param new_color The color to get, as a std::string.
 * @return The nc_color constant that matches the input.
 */
nc_color color_from_string(std::string new_color){
    std::map<std::string,nc_color>::iterator iter = colormap.find(new_color);
    if (iter == colormap.end()) {
        return c_unset;
    } else {
        return iter->second;
    }
}

/**
 * Given the name of a background color (that is, one of the i_xxxxx colors),
 * returns the nc_color constant that matches. If no match is found, i_white is
 * returned.
 * @param new_color The color to get, as a std::string.
 * @return The nc_color constant that matches the input.
 */
nc_color bgcolor_from_string(std::string new_color) {
  if("black" == new_color) {
    return i_black;
  } else if("white" == new_color) {
    return i_white;
  } else if("light_gray" == new_color || "ltgray" == new_color) {
    return i_ltgray;
  } else if("dark_gray" == new_color || "dkgray" == new_color) {
    return i_dkgray;
  } else if("red" == new_color) {
    return i_red;
  } else if("green" == new_color) {
    return i_green;
  } else if("blue" == new_color) {
    return i_blue;
  } else if("cyan" == new_color) {
    return i_cyan;
  } else if("magenta" == new_color) {
    return i_magenta;
  } else if("brown" == new_color) {
    return i_brown;
  } else if("light_red" == new_color || "ltred" == new_color) {
    return i_ltred;
  } else if("light_blue" == new_color || "ltblue" == new_color) {
    return i_ltblue;
  } else if("light_cyan" == new_color || "ltcyan" == new_color) {
    return i_ltcyan;
  } else if("pink" == new_color) {
    return i_pink;
  } else if("yellow" == new_color) {
    return i_yellow;
  } else {
    return i_white;
  }
}

nc_color get_color_from_tag(const std::string &s, const nc_color base_color)
{
    if (s.empty() || s[0] != '<' || s.substr(0,8) == "</color>") {
        return base_color;
    }
    if (s.substr(0,7) != "<color_") {
        return base_color;
    }
    size_t tag_close = s.find('>');
    if (tag_close == std::string::npos) {
        return base_color;
    }
    std::string color_name = s.substr(7,tag_close-7);
    return color_from_string(color_name);
}

