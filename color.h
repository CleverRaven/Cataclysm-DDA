#ifndef _COLOR_H_
#define _COLOR_H

#ifndef _COLOR_LIST_
#define _COLOR_LIST_

#include "cursesdef.h"

void init_colors();

enum col_attribute {
WA_NULL = 0,
HI = 1,
INV = 2
};

enum nc_color {
c_black   = COLOR_PAIR(30),
c_white   = COLOR_PAIR(1)  | A_BOLD,
c_ltgray  = COLOR_PAIR(1),
c_dkgray  = COLOR_PAIR(30) | A_BOLD,
c_red     = COLOR_PAIR(2),
c_green   = COLOR_PAIR(3),
c_blue    = COLOR_PAIR(4),
c_cyan    = COLOR_PAIR(5),
c_magenta = COLOR_PAIR(6),
c_brown   = COLOR_PAIR(7),
c_ltred   = COLOR_PAIR(2)  | A_BOLD,
c_ltgreen = COLOR_PAIR(3)  | A_BOLD,
c_ltblue  = COLOR_PAIR(4)  | A_BOLD,
c_ltcyan  = COLOR_PAIR(5)  | A_BOLD,
c_pink    = COLOR_PAIR(6)  | A_BOLD,
c_yellow  = COLOR_PAIR(7)  | A_BOLD,

h_black   = COLOR_PAIR(20),
h_white   = COLOR_PAIR(15) | A_BOLD,
h_ltgray  = COLOR_PAIR(15),
h_dkgray  = COLOR_PAIR(20) | A_BOLD,
h_red     = COLOR_PAIR(16),
h_green   = COLOR_PAIR(17),
h_blue    = COLOR_PAIR(20),
h_cyan    = COLOR_PAIR(19),
h_magenta = COLOR_PAIR(21),
h_brown   = COLOR_PAIR(22),
h_ltred   = COLOR_PAIR(16) | A_BOLD,
h_ltgreen = COLOR_PAIR(17) | A_BOLD,
h_ltblue  = COLOR_PAIR(18) | A_BOLD,
h_ltcyan  = COLOR_PAIR(19) | A_BOLD,
h_pink    = COLOR_PAIR(21) | A_BOLD,
h_yellow  = COLOR_PAIR(22) | A_BOLD,

i_black   = COLOR_PAIR(30),
i_white   = COLOR_PAIR(8)  | A_BLINK,
i_ltgray  = COLOR_PAIR(8),
i_dkgray  = COLOR_PAIR(30) | A_BLINK,
i_red     = COLOR_PAIR(9),
i_green   = COLOR_PAIR(10),
i_blue    = COLOR_PAIR(11),
i_cyan    = COLOR_PAIR(12),
i_magenta = COLOR_PAIR(13),
i_brown   = COLOR_PAIR(14),
i_ltred   = COLOR_PAIR(9)  | A_BLINK,
i_ltgreen = COLOR_PAIR(10) | A_BLINK,
i_ltblue  = COLOR_PAIR(11) | A_BLINK,
i_ltcyan  = COLOR_PAIR(12) | A_BLINK,
i_pink    = COLOR_PAIR(13) | A_BLINK,
i_yellow  = COLOR_PAIR(14) | A_BLINK,

c_white_red   = COLOR_PAIR(23) | A_BOLD,
c_ltgray_red  = COLOR_PAIR(23),
c_dkgray_red  = COLOR_PAIR(9),
c_red_red     = COLOR_PAIR(9),
c_green_red   = COLOR_PAIR(25),
c_blue_red    = COLOR_PAIR(26),
c_cyan_red    = COLOR_PAIR(27),
c_magenta_red = COLOR_PAIR(28),
c_brown_red   = COLOR_PAIR(29),
c_ltred_red   = COLOR_PAIR(24) | A_BOLD,
c_ltgreen_red = COLOR_PAIR(25) | A_BOLD,
c_ltblue_red  = COLOR_PAIR(26) | A_BOLD,
c_ltcyan_red  = COLOR_PAIR(27) | A_BOLD,
c_pink_red    = COLOR_PAIR(28) | A_BOLD,
c_yellow_red  = COLOR_PAIR(29) | A_BOLD

};

int color_to_int(nc_color col);
nc_color int_to_color(int key);

void setattr(nc_color &col, col_attribute attr);

#endif

#endif
