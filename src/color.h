#ifndef _COLOR_H_
#define _COLOR_H_

#ifndef _COLOR_LIST_
#define _COLOR_LIST_

#include "json.h"
#include "cursesdef.h"
#include <string>

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
c_yellow_red  = COLOR_PAIR(29) | A_BOLD,

c_unset       = COLOR_PAIR(31),

 c_black_white  = COLOR_PAIR(32),
 c_dkgray_white  = COLOR_PAIR(32) | A_BOLD,
 c_ltgray_white  = COLOR_PAIR(33),
 c_white_white  = COLOR_PAIR(33) | A_BOLD,
 c_red_white  = COLOR_PAIR(34),
 c_ltred_white  = COLOR_PAIR(34) | A_BOLD,
 c_green_white  = COLOR_PAIR(35),
 c_ltgreen_white  = COLOR_PAIR(35) | A_BOLD,
 c_brown_white  = COLOR_PAIR(36),
 c_yellow_white  = COLOR_PAIR(36) | A_BOLD,
 c_blue_white  = COLOR_PAIR(37),
 c_ltblue_white  = COLOR_PAIR(37) | A_BOLD,
 c_magenta_white  = COLOR_PAIR(38),
 c_pink_white  = COLOR_PAIR(38) | A_BOLD,
 c_cyan_white  = COLOR_PAIR(39),
 c_ltcyan_white  = COLOR_PAIR(39) | A_BOLD,
 c_black_green  = COLOR_PAIR(40),
 c_dkgray_green  = COLOR_PAIR(40) | A_BOLD,
 c_ltgray_green  = COLOR_PAIR(41),
 c_white_green  = COLOR_PAIR(41) | A_BOLD,
 c_red_green  = COLOR_PAIR(42),
 c_ltred_green  = COLOR_PAIR(42) | A_BOLD,
 c_green_green  = COLOR_PAIR(43),
 c_ltgreen_green  = COLOR_PAIR(43) | A_BOLD,
 c_brown_green  = COLOR_PAIR(44),
 c_yellow_green  = COLOR_PAIR(44) | A_BOLD,
 c_blue_green  = COLOR_PAIR(45),
 c_ltblue_green  = COLOR_PAIR(45) | A_BOLD,
 c_magenta_green  = COLOR_PAIR(46),
 c_pink_green  = COLOR_PAIR(46) | A_BOLD,
 c_cyan_green  = COLOR_PAIR(47),
 c_ltcyan_green  = COLOR_PAIR(47) | A_BOLD,
 c_black_yellow  = COLOR_PAIR(48),
 c_dkgray_yellow  = COLOR_PAIR(48) | A_BOLD,
 c_ltgray_yellow  = COLOR_PAIR(49),
 c_white_yellow  = COLOR_PAIR(49) | A_BOLD,
 c_red_yellow  = COLOR_PAIR(50),
 c_ltred_yellow  = COLOR_PAIR(50) | A_BOLD,
 c_green_yellow  = COLOR_PAIR(51),
 c_ltgreen_yellow  = COLOR_PAIR(51) | A_BOLD,
 c_brown_yellow  = COLOR_PAIR(52),
 c_yellow_yellow  = COLOR_PAIR(52) | A_BOLD,
 c_blue_yellow  = COLOR_PAIR(53),
 c_ltblue_yellow  = COLOR_PAIR(53) | A_BOLD,
 c_magenta_yellow  = COLOR_PAIR(54),
 c_pink_yellow  = COLOR_PAIR(54) | A_BOLD,
 c_cyan_yellow  = COLOR_PAIR(55),
 c_ltcyan_yellow  = COLOR_PAIR(55) | A_BOLD,
 c_black_magenta  = COLOR_PAIR(56),
 c_dkgray_magenta  = COLOR_PAIR(56) | A_BOLD,
 c_ltgray_magenta  = COLOR_PAIR(57),
 c_white_magenta  = COLOR_PAIR(57) | A_BOLD,
 c_red_magenta  = COLOR_PAIR(58),
 c_ltred_magenta  = COLOR_PAIR(58) | A_BOLD,
 c_green_magenta  = COLOR_PAIR(59),
 c_ltgreen_magenta  = COLOR_PAIR(59) | A_BOLD,
 c_brown_magenta  = COLOR_PAIR(60),
 c_yellow_magenta  = COLOR_PAIR(60) | A_BOLD,
 c_blue_magenta  = COLOR_PAIR(61),
 c_ltblue_magenta  = COLOR_PAIR(61) | A_BOLD,
 c_magenta_magenta  = COLOR_PAIR(62),
 c_pink_magenta  = COLOR_PAIR(62) | A_BOLD,
 c_cyan_magenta  = COLOR_PAIR(63),
 c_ltcyan_magenta  = COLOR_PAIR(63) | A_BOLD,
 c_black_cyan  = COLOR_PAIR(64),
 c_dkgray_cyan  = COLOR_PAIR(64) | A_BOLD,
 c_ltgray_cyan  = COLOR_PAIR(65),
 c_white_cyan  = COLOR_PAIR(65) | A_BOLD,
 c_red_cyan  = COLOR_PAIR(66),
 c_ltred_cyan  = COLOR_PAIR(66) | A_BOLD,
 c_green_cyan  = COLOR_PAIR(67),
 c_ltgreen_cyan  = COLOR_PAIR(67) | A_BOLD,
 c_brown_cyan  = COLOR_PAIR(68),
 c_yellow_cyan  = COLOR_PAIR(68) | A_BOLD,
 c_blue_cyan  = COLOR_PAIR(69),
 c_ltblue_cyan  = COLOR_PAIR(69) | A_BOLD,
 c_magenta_cyan  = COLOR_PAIR(70),
 c_pink_cyan  = COLOR_PAIR(70) | A_BOLD,
 c_cyan_cyan  = COLOR_PAIR(71),
 c_ltcyan_cyan  = COLOR_PAIR(71) | A_BOLD

};

int color_to_int(nc_color col);
nc_color int_to_color(int key);

nc_color hilite(nc_color c);
nc_color invert_color(nc_color c);
nc_color red_background(nc_color c);
nc_color white_background(nc_color c);
nc_color green_background(nc_color c);
nc_color yellow_background(nc_color c);
nc_color magenta_background(nc_color c);
nc_color cyan_background(nc_color c);
nc_color rand_color();

nc_color color_from_string(std::string color);
nc_color bgcolor_from_string(std::string color);
nc_color get_color_from_tag(const std::string &s, const nc_color base_color);

void setattr(nc_color &col, col_attribute attr);
void load_colors(JsonObject &jo);
void init_colormap();

#endif // _COLOR_LIST_

#endif // _COLOR_H_
