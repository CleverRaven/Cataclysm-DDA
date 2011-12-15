#include "color.h"

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

 init_pair(30, COLOR_BLACK,   COLOR_BLACK  );
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
 }
 return c_black;
}
