#include "color.h"
#include "cursesdef.h"
#include "options.h"
#include "rng.h"
#include "output.h"
#include "debug.h"
#include "input.h"
#include "worldfactory.h"
#include "path_info.h"
#include "mapsharing.h"
#include "filesystem.h"
#include <iostream>
#include <fstream>

#define HILIGHT COLOR_BLUE

clColors &get_all_colors()
{
    static clColors single_instance;
    return single_instance;
}

std::unordered_map<std::string, note_color> color_shortcuts;

nc_color clColors::get(const std::string &sName)
{
    auto const iter = mapColors.find( sName );

    if( iter == mapColors.end() ) {
        return 0;
    }

    auto &entry = iter->second;

    return ( !entry.sCustom.empty() ) ? mapColors[entry.sCustom].color : entry.color;
}

std::string clColors::get(const nc_color color)
{
    for ( const auto& iter : mapColors ) {
        if ( iter.second.color == color ) {
            return iter.first;
            break;
        }
    }

    return "c_unset";
}

nc_color clColors::get_invert(const nc_color color)
{
    const std::string sName = all_colors.get(color);

    auto const iter = mapColors.find( sName );
    if( iter == mapColors.end() ) {
        return 0;
    }

    auto &entry = iter->second;

    if ( !entry.sInvertCustom.empty() ) {
        return get(entry.sInvertCustom);
    }

    return get(entry.sInvert);
}

nc_color clColors::get_default(const std::string &sName)
{
    auto const iter = mapColors.find( sName );
    if( iter == mapColors.end() ) {
        return 0;
    }

    return iter->second.color;
}

nc_color clColors::get_random()
{
    auto item = mapColors.begin();
    std::advance( item, rand() % mapColors.size() );

    return item->second.color;
}

void clColors::add_color(const std::string &sName, const nc_color color, const std::string &sInvert)
{
    mapColors[sName] = {color, "", sInvert, ""};
}

nc_color clColors::get_highlight(const nc_color color, const std::string &bgColor)
{
    /*
    //             Base Name      Highlight      Red BG              White BG            Green BG            Yellow BG
    add_hightlight("c_black",     "h_black",     "",                 "c_black_white",    "c_black_green",    "c_black_yellow",   "c_black_magenta",      "c_black_cyan");
    add_hightlight("c_white",     "h_white",     "c_white_red",      "c_white_white",    "c_white_green",    "c_white_yellow",   "c_white_magenta",      "c_white_cyan");
    etc.
    */

    std::string sName = "";

    for ( const auto& iter : mapColors ) {
        if ( iter.second.color == color ) { //c_black -> c_black_<bgColor>
            sName = iter.first + (( !bgColor.empty() ) ? "_" + bgColor : (std::string)"");
            break;
        }
    }

    if ( sName.empty() ) {
        return 0;
    }

    if ( bgColor.empty() ) {  //c_black -> h_black
        sName = "h_" + sName.substr(2, sName.length() - 2);
    }

    if ( mapColors[sName].color == 0 ) {
        return 0;
    }

    return get(sName);
}

void clColors::load_default()
{
    //        Color Name             Color Pair                  Invert Name
    add_color("c_black",             COLOR_PAIR(30),             "i_black");
    add_color("c_white",             COLOR_PAIR(1) | A_BOLD,     "i_white");
    add_color("c_ltgray",            COLOR_PAIR(1),              "i_ltgray");
    add_color("c_dkgray",            COLOR_PAIR(30) | A_BOLD,    "i_dkgray");
    add_color("c_red",               COLOR_PAIR(2),              "i_red");
    add_color("c_green",             COLOR_PAIR(3),              "i_green");
    add_color("c_blue",              COLOR_PAIR(4),              "i_blue");
    add_color("c_cyan",              COLOR_PAIR(5),              "i_cyan");
    add_color("c_magenta",           COLOR_PAIR(6),              "i_magenta");
    add_color("c_brown",             COLOR_PAIR(7),              "i_brown");
    add_color("c_ltred",             COLOR_PAIR(2) | A_BOLD,     "i_ltred");
    add_color("c_ltgreen",           COLOR_PAIR(3) | A_BOLD,     "i_ltgreen");
    add_color("c_ltblue",            COLOR_PAIR(4) | A_BOLD,     "i_ltblue");
    add_color("c_ltcyan",            COLOR_PAIR(5) | A_BOLD,     "i_ltcyan");
    add_color("c_pink",              COLOR_PAIR(6) | A_BOLD,     "i_pink");
    add_color("c_yellow",            COLOR_PAIR(7) | A_BOLD,     "i_yellow");

    add_color("h_black",             COLOR_PAIR(20),             "c_blue");
    add_color("h_white",             COLOR_PAIR(15) | A_BOLD,    "c_ltblue_white");
    add_color("h_ltgray",            COLOR_PAIR(15),             "c_blue_white");
    add_color("h_dkgray",            COLOR_PAIR(20) | A_BOLD,    "c_ltblue");
    add_color("h_red",               COLOR_PAIR(16),             "c_blue_red");
    add_color("h_green",             COLOR_PAIR(17),             "c_blue_green");
    add_color("h_blue",              COLOR_PAIR(20),             "h_blue");
    add_color("h_cyan",              COLOR_PAIR(19),             "c_blue_cyan");
    add_color("h_magenta",           COLOR_PAIR(21),             "c_blue_magenta");
    add_color("h_brown",             COLOR_PAIR(22),             "c_blue_yellow");
    add_color("h_ltred",             COLOR_PAIR(16) | A_BOLD,    "c_ltblue_red");
    add_color("h_ltgreen",           COLOR_PAIR(17) | A_BOLD,    "c_ltblue_green");
    add_color("h_ltblue",            COLOR_PAIR(18) | A_BOLD,    "h_ltblue");
    add_color("h_ltcyan",            COLOR_PAIR(19) | A_BOLD,    "c_ltblue_cyan");
    add_color("h_pink",              COLOR_PAIR(21) | A_BOLD,    "c_ltblue_magenta");
    add_color("h_yellow",            COLOR_PAIR(22) | A_BOLD,    "c_ltblue_yellow");

    add_color("i_black",             COLOR_PAIR(32),             "c_black");
    add_color("i_white",             COLOR_PAIR(8) | A_BLINK,    "c_white");
    add_color("i_ltgray",            COLOR_PAIR(8),              "c_ltgray");
    add_color("i_dkgray",            COLOR_PAIR(32) | A_BLINK,   "c_dkgray");
    add_color("i_red",               COLOR_PAIR(9),              "c_red");
    add_color("i_green",             COLOR_PAIR(10),             "c_green");
    add_color("i_blue",              COLOR_PAIR(11),             "c_blue");
    add_color("i_cyan",              COLOR_PAIR(12),             "c_cyan");
    add_color("i_magenta",           COLOR_PAIR(13),             "c_magenta");
    add_color("i_brown",             COLOR_PAIR(14),             "c_brown");
    add_color("i_ltred",             COLOR_PAIR(9) | A_BLINK,    "c_ltred");
    add_color("i_ltgreen",           COLOR_PAIR(10) | A_BLINK,   "c_ltgreen");
    add_color("i_ltblue",            COLOR_PAIR(11) | A_BLINK,   "c_ltblue");
    add_color("i_ltcyan",            COLOR_PAIR(12) | A_BLINK,   "c_ltcyan");
    add_color("i_pink",              COLOR_PAIR(13) | A_BLINK,   "c_pink");
    add_color("i_yellow",            COLOR_PAIR(14) | A_BLINK,   "c_yellow");

    add_color("c_white_red",         COLOR_PAIR(23) | A_BOLD,    "c_red_white");
    add_color("c_ltgray_red",        COLOR_PAIR(23),             "c_ltred_white");
    add_color("c_dkgray_red",        COLOR_PAIR(9),              "c_dkgray_red");
    add_color("c_red_red",           COLOR_PAIR(9),              "c_red_red");
    add_color("c_green_red",         COLOR_PAIR(25),             "c_red_green");
    add_color("c_blue_red",          COLOR_PAIR(26),             "h_red");
    add_color("c_cyan_red",          COLOR_PAIR(27),             "c_red_cyan");
    add_color("c_magenta_red",       COLOR_PAIR(28),             "c_red_magenta");
    add_color("c_brown_red",         COLOR_PAIR(29),             "c_red_yellow");
    add_color("c_ltred_red",         COLOR_PAIR(24) | A_BOLD,    "c_red_red");
    add_color("c_ltgreen_red",       COLOR_PAIR(25) | A_BOLD,    "c_ltred_green");
    add_color("c_ltblue_red",        COLOR_PAIR(26) | A_BOLD,    "h_ltred");
    add_color("c_ltcyan_red",        COLOR_PAIR(27) | A_BOLD,    "c_ltred_cyan");
    add_color("c_pink_red",          COLOR_PAIR(28) | A_BOLD,    "c_ltred_magenta");
    add_color("c_yellow_red",        COLOR_PAIR(29) | A_BOLD,    "c_red_yellow");

    add_color("c_unset",             COLOR_PAIR(31),             "c_unset");

    add_color("c_black_white",       COLOR_PAIR(32),             "c_ltgray");
    add_color("c_dkgray_white",      COLOR_PAIR(32) | A_BOLD,    "c_white");
    add_color("c_ltgray_white",      COLOR_PAIR(33),             "c_ltgray_white");
    add_color("c_white_white",       COLOR_PAIR(33) | A_BOLD,    "c_white_white");
    add_color("c_red_white",         COLOR_PAIR(34),             "c_white_red");
    add_color("c_ltred_white",       COLOR_PAIR(34) | A_BOLD,    "c_ltgray_red");
    add_color("c_green_white",       COLOR_PAIR(35),             "c_ltgray_green");
    add_color("c_ltgreen_white",     COLOR_PAIR(35) | A_BOLD,    "c_white_green");
    add_color("c_brown_white",       COLOR_PAIR(36),             "c_ltgray_yellow");
    add_color("c_yellow_white",      COLOR_PAIR(36) | A_BOLD,    "c_white_yellow");
    add_color("c_blue_white",        COLOR_PAIR(37),             "h_ltgray");
    add_color("c_ltblue_white",      COLOR_PAIR(37) | A_BOLD,    "h_white");
    add_color("c_magenta_white",     COLOR_PAIR(38),             "c_ltgray_magenta");
    add_color("c_pink_white",        COLOR_PAIR(38) | A_BOLD,    "c_white_magenta");
    add_color("c_cyan_white",        COLOR_PAIR(39),             "c_ltgray_cyan");
    add_color("c_ltcyan_white",      COLOR_PAIR(39) | A_BOLD,    "c_white_cyan");

    add_color("c_black_green",       COLOR_PAIR(40),             "c_green");
    add_color("c_dkgray_green",      COLOR_PAIR(40) | A_BOLD,    "c_ltgreen");
    add_color("c_ltgray_green",      COLOR_PAIR(41),             "c_green_white");
    add_color("c_white_green",       COLOR_PAIR(41) | A_BOLD,    "c_ltgreen_white");
    add_color("c_red_green",         COLOR_PAIR(42),             "c_green_red");
    add_color("c_ltred_green",       COLOR_PAIR(42) | A_BOLD,    "c_ltgreen_red");
    add_color("c_green_green",       COLOR_PAIR(43),             "c_green_green");
    add_color("c_ltgreen_green",     COLOR_PAIR(43) | A_BOLD,    "c_ltgreen_green");
    add_color("c_brown_green",       COLOR_PAIR(44),             "c_green_yellow");
    add_color("c_yellow_green",      COLOR_PAIR(44) | A_BOLD,    "c_ltgreen_yellow");
    add_color("c_blue_green",        COLOR_PAIR(45),             "h_green");
    add_color("c_ltblue_green",      COLOR_PAIR(45) | A_BOLD,    "h_ltgreen");
    add_color("c_magenta_green",     COLOR_PAIR(46),             "c_green_magenta");
    add_color("c_pink_green",        COLOR_PAIR(46) | A_BOLD,    "c_ltgreen_magenta");
    add_color("c_cyan_green",        COLOR_PAIR(47),             "c_green_cyan");
    add_color("c_ltcyan_green",      COLOR_PAIR(47) | A_BOLD,    "c_ltgreen_cyan");

    add_color("c_black_yellow",      COLOR_PAIR(48),             "c_brown");
    add_color("c_dkgray_yellow",     COLOR_PAIR(48) | A_BOLD,    "c_yellow");
    add_color("c_ltgray_yellow",     COLOR_PAIR(49),             "c_brown_white");
    add_color("c_white_yellow",      COLOR_PAIR(49) | A_BOLD,    "c_yellow_white");
    add_color("c_red_yellow",        COLOR_PAIR(50),             "c_yellow_red");
    add_color("c_ltred_yellow",      COLOR_PAIR(50) | A_BOLD,    "c_yellow_red");
    add_color("c_green_yellow",      COLOR_PAIR(51),             "c_brown_green");
    add_color("c_ltgreen_yellow",    COLOR_PAIR(51) | A_BOLD,    "c_yellow_green");
    add_color("c_brown_yellow",      COLOR_PAIR(52),             "c_brown_yellow");
    add_color("c_yellow_yellow",     COLOR_PAIR(52) | A_BOLD,    "c_yellow_yellow");
    add_color("c_blue_yellow",       COLOR_PAIR(53),             "h_brown");
    add_color("c_ltblue_yellow",     COLOR_PAIR(53) | A_BOLD,    "h_yellow");
    add_color("c_magenta_yellow",    COLOR_PAIR(54),             "c_brown_magenta");
    add_color("c_pink_yellow",       COLOR_PAIR(54) | A_BOLD,    "c_yellow_magenta");
    add_color("c_cyan_yellow",       COLOR_PAIR(55),             "c_brown_cyan");
    add_color("c_ltcyan_yellow",     COLOR_PAIR(55) | A_BOLD,    "c_yellow_cyan");

    add_color("c_black_magenta",     COLOR_PAIR(56),             "c_magenta");
    add_color("c_dkgray_magenta",    COLOR_PAIR(56) | A_BOLD,    "c_pink");
    add_color("c_ltgray_magenta",    COLOR_PAIR(57),             "c_magenta_white");
    add_color("c_white_magenta",     COLOR_PAIR(57) | A_BOLD,    "c_pink_white");
    add_color("c_red_magenta",       COLOR_PAIR(58),             "c_magenta_red");
    add_color("c_ltred_magenta",     COLOR_PAIR(58) | A_BOLD,    "c_pink_red");
    add_color("c_green_magenta",     COLOR_PAIR(59),             "c_magenta_green");
    add_color("c_ltgreen_magenta",   COLOR_PAIR(59) | A_BOLD,    "c_pink_green");
    add_color("c_brown_magenta",     COLOR_PAIR(60),             "c_magenta_yellow");
    add_color("c_yellow_magenta",    COLOR_PAIR(60) | A_BOLD,    "c_pink_yellow");
    add_color("c_blue_magenta",      COLOR_PAIR(61),             "h_magenta");
    add_color("c_ltblue_magenta",    COLOR_PAIR(61) | A_BOLD,    "h_pink");
    add_color("c_magenta_magenta",   COLOR_PAIR(62),             "c_magenta_magenta");
    add_color("c_pink_magenta",      COLOR_PAIR(62) | A_BOLD,    "c_pink_magenta");
    add_color("c_cyan_magenta",      COLOR_PAIR(63),             "c_magenta_cyan");
    add_color("c_ltcyan_magenta",    COLOR_PAIR(63) | A_BOLD,    "c_pink_cyan");

    add_color("c_black_cyan",        COLOR_PAIR(64),             "c_cyan");
    add_color("c_dkgray_cyan",       COLOR_PAIR(64) | A_BOLD,    "c_ltcyan");
    add_color("c_ltgray_cyan",       COLOR_PAIR(65),             "c_cyan_white");
    add_color("c_white_cyan",        COLOR_PAIR(65) | A_BOLD,    "c_ltcyan_white");
    add_color("c_red_cyan",          COLOR_PAIR(66),             "c_cyan_red");
    add_color("c_ltred_cyan",        COLOR_PAIR(66) | A_BOLD,    "c_ltcyan_red");
    add_color("c_green_cyan",        COLOR_PAIR(67),             "c_cyan_green");
    add_color("c_ltgreen_cyan",      COLOR_PAIR(67) | A_BOLD,    "c_ltcyan_green");
    add_color("c_brown_cyan",        COLOR_PAIR(68),             "c_cyan_yellow");
    add_color("c_yellow_cyan",       COLOR_PAIR(68) | A_BOLD,    "c_ltcyan_yellow");
    add_color("c_blue_cyan",         COLOR_PAIR(69),             "h_cyan");
    add_color("c_ltblue_cyan",       COLOR_PAIR(69) | A_BOLD,    "h_ltcyan");
    add_color("c_magenta_cyan",      COLOR_PAIR(70),             "c_cyan_magenta");
    add_color("c_pink_cyan",         COLOR_PAIR(70) | A_BOLD,    "c_ltcyan_magenta");
    add_color("c_cyan_cyan",         COLOR_PAIR(71),             "c_cyan_cyan");
    add_color("c_ltcyan_cyan",       COLOR_PAIR(71) | A_BOLD,    "c_ltcyan_cyan");
}

void init_colors()
{
    start_color();

    init_pair( 1, COLOR_WHITE,      COLOR_BLACK  );
    init_pair( 2, COLOR_RED,        COLOR_BLACK  );
    init_pair( 3, COLOR_GREEN,      COLOR_BLACK  );
    init_pair( 4, COLOR_BLUE,       COLOR_BLACK  );
    init_pair( 5, COLOR_CYAN,       COLOR_BLACK  );
    init_pair( 6, COLOR_MAGENTA,    COLOR_BLACK  );
    init_pair( 7, COLOR_YELLOW,     COLOR_BLACK  );

    // Inverted Colors
    init_pair( 8, COLOR_BLACK,      COLOR_WHITE  );
    init_pair( 9, COLOR_BLACK,      COLOR_RED    );
    init_pair(10, COLOR_BLACK,      COLOR_GREEN  );
    init_pair(11, COLOR_BLACK,      COLOR_BLUE   );
    init_pair(12, COLOR_BLACK,      COLOR_CYAN   );
    init_pair(13, COLOR_BLACK,      COLOR_MAGENTA);
    init_pair(14, COLOR_BLACK,      COLOR_YELLOW );

    // Highlighted - blue background
    init_pair(15, COLOR_WHITE,      HILIGHT);
    init_pair(16, COLOR_RED,        HILIGHT);
    init_pair(17, COLOR_GREEN,      HILIGHT);
    init_pair(18, COLOR_BLUE,       HILIGHT);
    init_pair(19, COLOR_CYAN,       HILIGHT);
    init_pair(20, COLOR_BLACK,      HILIGHT);
    init_pair(21, COLOR_MAGENTA,    HILIGHT);
    init_pair(22, COLOR_YELLOW,     HILIGHT);

    // Red background - for monsters on fire
    init_pair(23, COLOR_WHITE,      COLOR_RED);
    init_pair(24, COLOR_RED,        COLOR_RED);
    init_pair(25, COLOR_GREEN,      COLOR_RED);
    init_pair(26, COLOR_BLUE,       COLOR_RED);
    init_pair(27, COLOR_CYAN,       COLOR_RED);
    init_pair(28, COLOR_MAGENTA,    COLOR_RED);
    init_pair(29, COLOR_YELLOW,     COLOR_RED);

    init_pair(30, COLOR_BLACK,      COLOR_BLACK);
    init_pair(31, COLOR_WHITE,      COLOR_BLACK);

    init_pair(32, COLOR_BLACK,      COLOR_WHITE);
    init_pair(33, COLOR_WHITE,      COLOR_WHITE);
    init_pair(34, COLOR_RED,        COLOR_WHITE);
    init_pair(35, COLOR_GREEN,      COLOR_WHITE);
    init_pair(36, COLOR_YELLOW,     COLOR_WHITE);
    init_pair(37, COLOR_BLUE,       COLOR_WHITE);
    init_pair(38, COLOR_MAGENTA,    COLOR_WHITE);
    init_pair(39, COLOR_CYAN,       COLOR_WHITE);

    init_pair(40, COLOR_BLACK,      COLOR_GREEN);
    init_pair(41, COLOR_WHITE,      COLOR_GREEN);
    init_pair(42, COLOR_RED,        COLOR_GREEN);
    init_pair(43, COLOR_GREEN,      COLOR_GREEN);
    init_pair(44, COLOR_YELLOW,     COLOR_GREEN);
    init_pair(45, COLOR_BLUE,       COLOR_GREEN);
    init_pair(46, COLOR_MAGENTA,    COLOR_GREEN);
    init_pair(47, COLOR_CYAN,       COLOR_GREEN);

    init_pair(48, COLOR_BLACK,      COLOR_YELLOW);
    init_pair(49, COLOR_WHITE,      COLOR_YELLOW);
    init_pair(50, COLOR_RED,        COLOR_YELLOW);
    init_pair(51, COLOR_GREEN,      COLOR_YELLOW);
    init_pair(52, COLOR_YELLOW,     COLOR_YELLOW);
    init_pair(53, COLOR_BLUE,       COLOR_YELLOW);
    init_pair(54, COLOR_MAGENTA,    COLOR_YELLOW);
    init_pair(55, COLOR_CYAN,       COLOR_YELLOW);

    init_pair(56, COLOR_BLACK,      COLOR_MAGENTA);
    init_pair(57, COLOR_WHITE,      COLOR_MAGENTA);
    init_pair(58, COLOR_RED,        COLOR_MAGENTA);
    init_pair(59, COLOR_GREEN,      COLOR_MAGENTA);
    init_pair(60, COLOR_YELLOW,     COLOR_MAGENTA);
    init_pair(61, COLOR_BLUE,       COLOR_MAGENTA);
    init_pair(62, COLOR_MAGENTA,    COLOR_MAGENTA);
    init_pair(63, COLOR_CYAN,       COLOR_MAGENTA);

    init_pair(64, COLOR_BLACK,      COLOR_CYAN);
    init_pair(65, COLOR_WHITE,      COLOR_CYAN);
    init_pair(66, COLOR_RED,        COLOR_CYAN);
    init_pair(67, COLOR_GREEN,      COLOR_CYAN);
    init_pair(68, COLOR_YELLOW,     COLOR_CYAN);
    init_pair(69, COLOR_BLUE,       COLOR_CYAN);
    init_pair(70, COLOR_MAGENTA,    COLOR_CYAN);
    init_pair(71, COLOR_CYAN,       COLOR_CYAN);

    all_colors.load_default();
    all_colors.load_custom();

    // The color codes are intentionally untranslatable.
    color_shortcuts = {
        {"br", {c_brown, _("brown")}}, {"lg", {c_ltgray, _("lt gray")}},
        {"dg", {c_dkgray, _("dk gray")}}, {"r", {c_ltred, _("lt red")}},
        {"R", {c_red, _("red")}}, {"g", {c_ltgreen, _("lt green")}},
        {"G", {c_green, _("green")}}, {"b", {c_ltblue, _("lt blue")}},
        {"B", {c_blue, _("blue")}}, {"W", {c_white, _("white")}},
        {"C", {c_cyan, _("cyan")}}, {"c", {c_ltcyan, _("lt cyan")}},
        {"P", {c_pink, _("pink")}}, {"m", {c_magenta, _("magenta")}}
    };
}

nc_color hilite(nc_color c)
{
    const nc_color color = all_colors.get_highlight(c);
    return ((int)color > 0) ? color : h_white;
}

nc_color invert_color(nc_color c)
{
    const nc_color color = all_colors.get_invert(c);
    return ((int)color > 0) ? color : c_pink;
}

nc_color red_background(nc_color c)
{
    const nc_color color = all_colors.get_highlight(c, "red");
    return ((int)color > 0) ? color : c_white_red;
}

nc_color white_background(nc_color c)
{
    const nc_color color = all_colors.get_highlight(c, "white");
    return ((int)color > 0) ? color : c_black_white;
}

nc_color green_background(nc_color c)
{
    const nc_color color = all_colors.get_highlight(c, "green");
    return ((int)color > 0) ? color : c_black_green;
}

nc_color yellow_background(nc_color c)
{
    const nc_color color = all_colors.get_highlight(c, "yellow");
    return ((int)color > 0) ? color : c_black_yellow;
}

nc_color magenta_background(nc_color c)
{
    const nc_color color = all_colors.get_highlight(c, "magenta");
    return ((int)color > 0) ? color : c_black_magenta;
}

nc_color cyan_background(nc_color c)
{
    const nc_color color = all_colors.get_highlight(c, "cyan");
    return ((int)color > 0) ? color : c_black_cyan;
}

/**
 * Given the name of a color, returns the nc_color value that matches. If
 * no match is found, c_unset is returned.
 * Special cases:
 * {"black"           , c_black}, // missing default prefix c_
 * {"<c|h|i>_black"   , h_black}, // has prefix c_ or h_ or i_
 * {"dark_gray_red"   , c_dkgray_red}, // dark_ instead of dk
 * {"light_blue_red"  , c_ltblue_red}, // light_ instead of lt
 * @param new_color The color to get, as a std::string.
 * @return The nc_color constant that matches the input.
 */
nc_color color_from_string(const std::string &color)
{
    std::string new_color = color;
    if ( new_color.substr(1, 1) != "_" ) {  //c_  //i_  //h_
        new_color = "c_" + new_color;
    }

    const std::pair<std::string, std::string> pSearch[2] = {{"light_", "lt"}, {"dark_", "dk"}};
    for (int i=0; i < 2; ++i) {
        size_t pos = 0;
        while ((pos = new_color.find(pSearch[i].first, pos)) != std::string::npos) {
            new_color.replace(pos, pSearch[i].first.length(), pSearch[i].second);
            pos += pSearch[i].second.length();
        }
    }

    const nc_color col = all_colors.get(new_color);
    if ( col > 0 ) {
        return col;
    }

    debugmsg("color_from_string: couldn't parse color: %s", color.c_str());
    return c_unset;
}

/**
 * The reverse of color_from_string.
 */
std::string string_from_color(const nc_color color)
{
    std::string sColor = all_colors.get(color);
    sColor = sColor.substr(2, sColor.length()-2);

    if ( sColor != "unset" ) {
        return sColor;
    }

    return "white";
}

/**
 * Given the name of a background color (that is, one of the i_xxxxx colors),
 * returns the nc_color constant that matches. If no match is found, i_white is
 * returned.
 * @param new_color The color to get, as a std::string.
 * @return The nc_color constant that matches the input.
 */
nc_color bgcolor_from_string(std::string color)
{
    color = "i_" + color;

    const std::pair<std::string, std::string> pSearch[2] = {{"light_", "lt"}, {"dark_", "dk"}};
    for (int i=0; i < 2; ++i) {
        size_t pos = 0;
        while ((pos = color.find(pSearch[i].first, pos)) != std::string::npos) {
            color.replace(pos, pSearch[i].first.length(), pSearch[i].second);
            pos += pSearch[i].second.length();
        }
    }

    const nc_color col = all_colors.get(color);
    if ( col > 0 ) {
        return col;
    }

    return i_white;
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

nc_color get_note_color(std::string const &note_id)
{
    auto const candidate_color = color_shortcuts.find( note_id );
    if( candidate_color != std::end( color_shortcuts ) ) {
        return candidate_color->second.color;
    }
    // The default note color.
    return c_yellow;
}

std::list<std::pair<std::string, std::string>> get_note_color_names()
{
    std::list<std::pair<std::string, std::string>> color_list;
    for( auto const &color_pair : color_shortcuts ) {
        color_list.emplace_back( color_pair.first, color_pair.second.name );
    }
    return color_list;
}


void clColors::show_gui()
{
    const int iHeaderHeight = 4;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 2 - iHeaderHeight;

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0;

    std::vector<int> vLines;
    vLines.push_back(-1);
    vLines.push_back(48);

    const int iTotalCols = vLines.size();

    WINDOW *w_colors_help = newwin((FULL_SCREEN_HEIGHT / 2) - 2, FULL_SCREEN_WIDTH * 3 / 4,
                                        7 + iOffsetY + (FULL_SCREEN_HEIGHT / 2) / 2, iOffsetX + 19 / 2);
    WINDOW_PTR w_colors_helpptr( w_colors_help );

    WINDOW *w_colors_border = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);
    WINDOW_PTR w_colors_borderptr( w_colors_border );
    WINDOW *w_colors_header = newwin(iHeaderHeight, FULL_SCREEN_WIDTH - 2, 1 + iOffsetY,
                                          1 + iOffsetX);
    WINDOW_PTR w_colors_headerptr( w_colors_header );
    WINDOW *w_colors = newwin(iContentHeight, FULL_SCREEN_WIDTH - 2, iHeaderHeight + 1 + iOffsetY,
                                   1 + iOffsetX);
    WINDOW_PTR w_colorsptr( w_colors );

    draw_border(w_colors_border);
    mvwputch(w_colors_border, 3,  0, c_ltgray, LINE_XXXO); // |-
    mvwputch(w_colors_border, 3, 79, c_ltgray, LINE_XOXX); // -|

    for (int i = 0; i < 78; i++) {
        mvwputch(w_colors_header, 2, i, c_ltgray, LINE_OXOX); // Draw line under header
    }

    for( auto &iCol : vLines ) {
        if ( iCol > -1 ) {
            mvwputch(w_colors_border, FULL_SCREEN_HEIGHT - 1, iCol + 1, c_ltgray, LINE_XXOX); // _|_
            mvwputch(w_colors_header, 2, iCol, c_ltgray, LINE_OXXX);
            mvwputch(w_colors_header, 3, iCol, c_ltgray, LINE_XOXO);
        }
    }

    mvwprintz(w_colors_border, 0, 32, c_ltred, _(" COLOR MANAGER "));
    wrefresh(w_colors_border);

    int tmpx = 0;
    tmpx += shortcut_print(w_colors_header, 0, tmpx, c_white, c_ltgreen, _("<R>emove custom color")) + 2;
    tmpx += shortcut_print(w_colors_header, 0, tmpx, c_white, c_ltgreen, _("<Arrow Keys> To navigate")) + 2;
    tmpx += shortcut_print(w_colors_header, 0, tmpx, c_white, c_ltgreen, _("<Enter>-Edit")) + 2;
    shortcut_print(w_colors_header, 0, tmpx, c_white, c_ltgreen, _("Load <T>emplate"));

    mvwprintz(w_colors_header, 1, 0, c_white, _("Some color changes may require a restart."));

    mvwprintz(w_colors_header, 3, 3, c_white, _("Colorname"));
    mvwprintz(w_colors_header, 3, 21, c_white, _("Normal"));
    mvwprintz(w_colors_header, 3, 52, c_white, _("Invert"));

    wrefresh(w_colors_header);

    int iCurrentLine = 0;
    int iCurrentCol = 1;
    int iStartPos = 0;
    const int iMaxColors = mapColors.size();
    bool bStuffChanged = false;
    input_context ctxt("COLORS");
    ctxt.register_cardinal();
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("REMOVE_CUSTOM");
    ctxt.register_action("LOAD_TEMPLATE");
    ctxt.register_action("HELP_KEYBINDINGS");

    std::map<std::string, stColors> mapColorsOrdered(mapColors.begin(), mapColors.end());

    while(true) {
        // Clear all lines
        for (int i = 0; i < iContentHeight; i++) {
            for (int j = 0; j < 79; j++) {
                mvwputch(w_colors, i, j, c_black, ' ');

                for( auto &iCol : vLines ) {
                    if ( iCol == j ) {
                        mvwputch(w_colors, i, j, c_ltgray, LINE_XOXO);
                    }
                }
            }
        }

        calcStartPos(iStartPos, iCurrentLine, iContentHeight, iMaxColors);

        //Draw Scrollbar
        draw_scrollbar(w_colors_border, iCurrentLine, iContentHeight, iMaxColors, 5);

        auto iter = mapColorsOrdered.begin();
        std::advance( iter, iStartPos );

        std::string sActive = "";

        // display colormanager
        for (int i=iStartPos; iter != mapColorsOrdered.end(); ++iter, ++i) {
            if (i >= iStartPos && i < iStartPos + ((iContentHeight > iMaxColors) ? iMaxColors : iContentHeight)) {
                auto &entry = iter->second;

                if (iCurrentLine == i) {
                    sActive = iter->first;
                    mvwprintz(w_colors, i - iStartPos, vLines[iCurrentCol-1] + 2, c_yellow, ">");
                }

                mvwprintz(w_colors, i - iStartPos, 3, c_white, iter->first.c_str()); //colorname
                mvwprintz(w_colors, i - iStartPos, 21, entry.color, _("default")); //default color

                if ( !entry.sCustom.empty() ) {
                    mvwprintz(w_colors, i - iStartPos, 30, mapColorsOrdered[entry.sCustom].color, entry.sCustom.c_str()); //custom color
                }

                mvwprintz(w_colors, i - iStartPos, 52, mapColorsOrdered[entry.sInvert].color, _("default")); //invert default color

                if ( !entry.sInvertCustom.empty() ) {
                    mvwprintz(w_colors, i - iStartPos, 61, mapColorsOrdered[entry.sInvertCustom].color, entry.sInvertCustom.c_str()); //invert custom color
                }
            }
        }

        wrefresh(w_colors);

        const std::string action = ctxt.handle_input();

        if (action == "QUIT") {
            break;
        } else if (action == "UP") {
            iCurrentLine--;
            if (iCurrentLine < 0) {
                iCurrentLine = iMaxColors - 1;
            }
        } else if (action == "DOWN") {
            iCurrentLine++;
            if (iCurrentLine >= (int)iMaxColors) {
                iCurrentLine = 0;
            }
        } else if (action == "LEFT") {
            iCurrentCol--;
            if (iCurrentCol < 1) {
                iCurrentCol = iTotalCols;
            }
        } else if (action == "RIGHT") {
            iCurrentCol++;
            if (iCurrentCol > iTotalCols) {
                iCurrentCol = 1;
            }
        } else if (action == "REMOVE_CUSTOM") {
            auto &entry = mapColorsOrdered[sActive];

            if ( iCurrentCol == 1 && !entry.sCustom.empty() ) {
                bStuffChanged = true;
                entry.sCustom = "";

            } else if ( iCurrentCol == 2 && !entry.sInvertCustom.empty() ) {
                bStuffChanged = true;
                entry.sInvertCustom = "";

            }

        } else if (action == "LOAD_TEMPLATE") {
            auto vFiles = get_files_from_path(".json", FILENAMES["color_templates"], false, true);

            if ( vFiles.size() > 0 ) {
                uimenu ui_templates;
                ui_templates.w_y = iHeaderHeight + 1 + iOffsetY;
                ui_templates.w_height = 18;
                ui_templates.return_invalid = true;

                ui_templates.text = _("Color templates:");

                for ( const auto& filename : vFiles ) {
                    ui_templates.addentry( filename.substr(filename.find_last_of("/") + 1) );
                }

                ui_templates.addentry(std::string(_("Cancel")));
                ui_templates.query();

                if ( (size_t)ui_templates.ret < vFiles.size() ) {
                    bStuffChanged = true;

                    auto mapColorsTemp = mapColors;

                    mapColors.clear();
                    load_default();

                    load_custom(vFiles[ui_templates.ret]);

                    mapColorsOrdered.clear();
                    std::map<std::string, stColors> mapColorsOrderedTemp(mapColors.begin(), mapColors.end());

                    mapColorsOrdered = mapColorsOrderedTemp;
                }
            }

        } else if (action == "CONFIRM") {
            uimenu ui_colors;
            ui_colors.w_y = iHeaderHeight + 1 + iOffsetY;
            ui_colors.w_height = 18;
            ui_colors.return_invalid = true;

            std::string sColorType = _("Normal");
            std::string sSelected = mapColorsOrdered[sActive].sCustom;

            if ( iCurrentCol == 2 ) {
                sColorType = _("Invert");
                sSelected = mapColorsOrdered[sActive].sInvertCustom;

            }

            ui_colors.text = string_format( _("Custom %s color:"), sColorType.c_str() );

            int i = 0;
            for ( auto &iter : mapColorsOrdered ) {
                std::string sColor = iter.first;
                std::string sType = _("default");

                std::string sCustom = "";

                if ( sSelected == sColor ) {
                    ui_colors.selected = i;
                }

                if ( !iter.second.sCustom.empty() ) {
                    sCustom = " <color_" + iter.second.sCustom + ">" + iter.second.sCustom + "</color>";
                }

                ui_colors.addentry(string_format( "%-17s <color_%s>%s</color>%s", iter.first.c_str(), sColor.c_str(), sType.c_str(), sCustom.c_str() ) );

                i++;
            }

            ui_colors.addentry(std::string(_("Cancel")));
            ui_colors.query();

            if ( (size_t)ui_colors.ret < mapColorsOrdered.size() ) {
                bStuffChanged = true;

                iter = mapColorsOrdered.begin();
                std::advance( iter, ui_colors.ret );

                auto &entry = mapColorsOrdered[sActive];

                if ( iCurrentCol == 1 ) {
                    entry.sCustom = iter->first;

                } else if ( iCurrentCol == 2 ) {
                    entry.sInvertCustom = iter->first;

                }
            }
        }
    }

    if (bStuffChanged) {
        if(query_yn(_("Save changes?"))) {
            std::unordered_map<std::string, stColors> mapColorsUnordered(mapColorsOrdered.begin(), mapColorsOrdered.end());
            mapColors.clear();
            mapColors = mapColorsUnordered;

            save_custom();
        }
    }
}

bool clColors::save_custom()
{
    const auto savefile = FILENAMES["custom_colors"];

    try {
        std::ofstream fout;
        fout.exceptions(std::ios::badbit | std::ios::failbit);

        fopen_exclusive(fout, savefile.c_str());
        if(!fout.is_open()) {
            return true; //trick game into thinking it was saved
        }

        fout << serialize();
        fclose_exclusive(fout, savefile.c_str());
        return true;

    } catch(std::ios::failure &) {
        popup(_("Failed to save custom colors to %s"), savefile.c_str());
        return false;
    }

    return false;
}

void clColors::load_custom(const std::string &sPath)
{
    const auto file = ( sPath.empty() ) ? FILENAMES["custom_colors"] : sPath;

    std::ifstream fin;
    fin.open(file.c_str(), std::ifstream::in | std::ifstream::binary);
    if(!fin.good()) {
        fin.close();
        return;
    }

    try {
        JsonIn jsin(fin);
        deserialize(jsin);
    } catch (std::string e) {
        DebugLog(D_ERROR, DC_ALL) << "load_custom: " << e;
    }

    fin.close();
}

void clColors::serialize(JsonOut &json) const
{
    json.start_array();
    for( auto &iter : mapColors ) {
        auto &entry = iter.second;

        if ( !entry.sCustom.empty() || !entry.sInvertCustom.empty()) {
            json.start_object();

            json.member( "name", iter.first );
            json.member( "custom", entry.sCustom );
            json.member( "invertcustom", entry.sInvertCustom );

            json.end_object();
        }
    }

    json.end_array();
}

void clColors::deserialize(JsonIn &jsin)
{
    jsin.start_array();
    while (!jsin.end_array()) {
        JsonObject joColors = jsin.get_object();

        const std::string sName = joColors.get_string("name");
        const std::string sCustom = joColors.get_string("custom");
        const std::string sInvertCustom = joColors.get_string("invertcustom");

        auto iter = mapColors.find( sName );

        if( iter == mapColors.end() ) {
            continue;
        }

        auto &entry = iter->second;

        if ( !sCustom.empty() ) {
            entry.sCustom = sCustom;
        }

        if ( !sInvertCustom.empty() ) {
            entry.sInvertCustom = sInvertCustom;
        }
    }
}
