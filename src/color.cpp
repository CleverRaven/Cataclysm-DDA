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
#include "ui.h"
#include "translations.h"
#include <iostream>
#include <fstream>

color_manager &get_all_colors()
{
    static color_manager single_instance;
    return single_instance;
}

std::unordered_map<std::string, note_color> color_by_string_map;

void color_manager::finalize()
{
    static const std::array<std::string, NUM_HL> hilights = {{
        "",
        "red",
        "white",
        "green",
        "yellow",
        "magenta",
        "cyan"
    }};

    for( size_t i = 0; i < color_array.size(); i++ ) {
        color_struct &entry = color_array[i];

        entry.invert = get( entry.invert_id );

        if( !entry.name_custom.empty() ) {
            // Not using name_to_color because we want default color of this name
            auto const id = name_to_id( entry.name_custom );
            auto &other = color_array[id];
            entry.custom = other.color;
        }

        if( !entry.name_invert_custom.empty() ) {
            auto const id = name_to_id( entry.name_invert_custom );
            auto &other = color_array[id];
            entry.custom = other.color;
        }

        inverted_map[entry.color] = entry.col_id;
        inverted_map[entry.invert] = entry.invert_id;
    }

    // Highlights in a next run, to make sure custom colors are set
    for( size_t i = 0; i < color_array.size(); i++ ) {
        color_struct &entry = color_array[i];
        const std::string my_name = get_name( entry.color );
        for( size_t j = 0; j < NUM_HL; j++ ) {
            entry.highlight[j] = highlight_from_names( my_name, hilights[j] );
        }
    }
}

nc_color color_manager::name_to_color( const std::string &name ) const
{
    auto const id = name_to_id( name );
    auto &entry = color_array[id];

    return entry.custom > 0 ? entry.custom : entry.color;
}

color_id color_manager::name_to_id( const std::string &name ) const
{
    auto iter = name_map.find( name );
    if( iter == name_map.end() ) {
        DebugLog( D_ERROR, DC_ALL) << "couldn't parse color: " << name ;
        return def_c_unset;
    }

    return iter->second;
}

std::string color_manager::id_to_name( const color_id id ) const
{
    for( const auto &pr : name_map ) {
        if( pr.second == id ) {
            return pr.first;
        }
    }

    return "c_unset";
}

color_id color_manager::color_to_id( const nc_color color ) const
{
    auto iter = inverted_map.find( color );
    if( iter != inverted_map.end() ) {
        return iter->second;
    }

    // Optimally this shouldn't happen, but allow for now
    for( size_t i = 0; i < color_array.size(); i++ ) {
        if( color_array[i].color == color ) {
debugmsg( "Couldn't find color %d, but got id: %s", color, get_name( color ).c_str() );
            return color_array[i].col_id;
        }
    }

    debugmsg( "Couldn't find color %d", color );
    return def_c_unset;
}

nc_color color_manager::get( const color_id col ) const
{
    if( col >= num_colors ) {
        debugmsg( "Invalid color index: %d. Color array size: %d", col, color_array.size() );
        return 0;
    }

    auto &entry = color_array[col];

    return entry.custom > 0 ? entry.custom : entry.color;
}

std::string color_manager::get_name(const nc_color color) const
{
    color_id id = color_to_id( color );
    for ( const auto& iter : name_map ) {
        if ( iter.second == id ) {
            return iter.first;
        }
    }

    return "c_unset";
}

nc_color color_manager::get_invert( const nc_color col ) const
{
    const color_id id = color_to_id( col );
    auto &entry = color_array[id];

    return entry.invert_custom > 0 ? entry.invert_custom : entry.invert;
}

nc_color color_manager::get_random() const
{
    auto item = color_array.begin();
    std::advance( item, rand() % num_colors );

    return item->color;
}

void color_manager::add_color( const color_id col, const std::string &name,
                               const nc_color color_pair, const color_id inv_id )
{
    color_struct st = {color_pair, 0, 0, 0, {{0,0,0,0,0,0,0}}, col, inv_id, "", "" };
    color_array[col] = st;
    inverted_map[color_pair] = col;
    name_map[name] = col;
}

nc_color color_manager::get_highlight( const nc_color color, const hl_enum bg ) const
{
    const color_id id = color_to_id( color );
    const color_struct &st = color_array[id];
    const auto &hl = st.highlight;
    return hl[bg];
}

nc_color color_manager::highlight_from_names( const std::string &name, const std::string &bg_name ) const
{
    /*
    //             Base Name      Highlight      Red BG              White BG            Green BG            Yellow BG
    add_hightlight("c_black",     "h_black",     "",                 "c_black_white",    "c_black_green",    "c_black_yellow",   "c_black_magenta",      "c_black_cyan");
    add_hightlight("c_white",     "h_white",     "c_white_red",      "c_white_white",    "c_white_green",    "c_white_yellow",   "c_white_magenta",      "c_white_cyan");
    etc.
    */

    std::string hi_name;

    if( bg_name.empty() ) {  //c_black -> h_black
        hi_name = "h_" + name.substr(2, name.length() - 2);
    } else {
        hi_name = name + "_" + bg_name;
    }

    color_id id = name_to_id( hi_name );
    return color_array[id].color;
}

void color_manager::load_default()
{
    //        Color         Name      Color Pair      Invert
    add_color(def_c_black, "c_black", COLOR_PAIR(30), def_i_black );
    add_color(def_c_white, "c_white", COLOR_PAIR(1) | A_BOLD, def_i_white );
    add_color(def_c_ltgray, "c_ltgray", COLOR_PAIR(1), def_i_ltgray );
    add_color(def_c_dkgray, "c_dkgray", COLOR_PAIR(30) | A_BOLD, def_i_dkgray );
    add_color(def_c_red, "c_red", COLOR_PAIR(2), def_i_red );
    add_color(def_c_green, "c_green", COLOR_PAIR(3), def_i_green );
    add_color(def_c_blue, "c_blue", COLOR_PAIR(4), def_i_blue );
    add_color(def_c_cyan, "c_cyan", COLOR_PAIR(5), def_i_cyan );
    add_color(def_c_magenta, "c_magenta", COLOR_PAIR(6), def_i_magenta );
    add_color(def_c_brown, "c_brown", COLOR_PAIR(7), def_i_brown );
    add_color(def_c_ltred, "c_ltred", COLOR_PAIR(2) | A_BOLD, def_i_ltred );
    add_color(def_c_ltgreen, "c_ltgreen", COLOR_PAIR(3) | A_BOLD, def_i_ltgreen );
    add_color(def_c_ltblue, "c_ltblue", COLOR_PAIR(4) | A_BOLD, def_i_ltblue );
    add_color(def_c_ltcyan, "c_ltcyan", COLOR_PAIR(5) | A_BOLD, def_i_ltcyan );
    add_color(def_c_pink, "c_pink", COLOR_PAIR(6) | A_BOLD, def_i_pink );
    add_color(def_c_yellow, "c_yellow", COLOR_PAIR(7) | A_BOLD, def_i_yellow );

    add_color(def_h_black, "h_black", COLOR_PAIR(20), def_c_blue );
    add_color(def_h_white, "h_white", COLOR_PAIR(15) | A_BOLD, def_c_ltblue_white );
    add_color(def_h_ltgray, "h_ltgray", COLOR_PAIR(15), def_c_blue_white );
    add_color(def_h_dkgray, "h_dkgray", COLOR_PAIR(20) | A_BOLD, def_c_ltblue );
    add_color(def_h_red, "h_red", COLOR_PAIR(16), def_c_blue_red );
    add_color(def_h_green, "h_green", COLOR_PAIR(17), def_c_blue_green );
    add_color(def_h_blue, "h_blue", COLOR_PAIR(20), def_h_blue );
    add_color(def_h_cyan, "h_cyan", COLOR_PAIR(19), def_c_blue_cyan );
    add_color(def_h_magenta, "h_magenta", COLOR_PAIR(21), def_c_blue_magenta );
    add_color(def_h_brown, "h_brown", COLOR_PAIR(22), def_c_blue_yellow );
    add_color(def_h_ltred, "h_ltred", COLOR_PAIR(16) | A_BOLD, def_c_ltblue_red );
    add_color(def_h_ltgreen, "h_ltgreen", COLOR_PAIR(17) | A_BOLD, def_c_ltblue_green );
    add_color(def_h_ltblue, "h_ltblue", COLOR_PAIR(18) | A_BOLD, def_h_ltblue );
    add_color(def_h_ltcyan, "h_ltcyan", COLOR_PAIR(19) | A_BOLD, def_c_ltblue_cyan );
    add_color(def_h_pink, "h_pink", COLOR_PAIR(21) | A_BOLD, def_c_ltblue_magenta );
    add_color(def_h_yellow, "h_yellow", COLOR_PAIR(22) | A_BOLD, def_c_ltblue_yellow );

    add_color(def_i_black, "i_black", COLOR_PAIR(32), def_c_black );
    add_color(def_i_white, "i_white", COLOR_PAIR(8) | A_BLINK, def_c_white );
    add_color(def_i_ltgray, "i_ltgray", COLOR_PAIR(8), def_c_ltgray );
    add_color(def_i_dkgray, "i_dkgray", COLOR_PAIR(32) | A_BLINK, def_c_dkgray );
    add_color(def_i_red, "i_red", COLOR_PAIR(9), def_c_red );
    add_color(def_i_green, "i_green", COLOR_PAIR(10), def_c_green );
    add_color(def_i_blue, "i_blue", COLOR_PAIR(11), def_c_blue );
    add_color(def_i_cyan, "i_cyan", COLOR_PAIR(12), def_c_cyan );
    add_color(def_i_magenta, "i_magenta", COLOR_PAIR(13), def_c_magenta );
    add_color(def_i_brown, "i_brown", COLOR_PAIR(14), def_c_brown );
    add_color(def_i_ltred, "i_ltred", COLOR_PAIR(9) | A_BLINK, def_c_ltred );
    add_color(def_i_ltgreen, "i_ltgreen", COLOR_PAIR(10) | A_BLINK, def_c_ltgreen );
    add_color(def_i_ltblue, "i_ltblue", COLOR_PAIR(11) | A_BLINK, def_c_ltblue );
    add_color(def_i_ltcyan, "i_ltcyan", COLOR_PAIR(12) | A_BLINK, def_c_ltcyan );
    add_color(def_i_pink, "i_pink", COLOR_PAIR(13) | A_BLINK, def_c_pink );
    add_color(def_i_yellow, "i_yellow", COLOR_PAIR(14) | A_BLINK, def_c_yellow );

    add_color(def_c_white_red, "c_white_red", COLOR_PAIR(23) | A_BOLD, def_c_red_white );
    add_color(def_c_ltgray_red, "c_ltgray_red", COLOR_PAIR(23), def_c_ltred_white );
    add_color(def_c_dkgray_red, "c_dkgray_red", COLOR_PAIR(9), def_c_dkgray_red );
    add_color(def_c_red_red, "c_red_red", COLOR_PAIR(9), def_c_red_red );
    add_color(def_c_green_red, "c_green_red", COLOR_PAIR(25), def_c_red_green );
    add_color(def_c_blue_red, "c_blue_red", COLOR_PAIR(26), def_h_red );
    add_color(def_c_cyan_red, "c_cyan_red", COLOR_PAIR(27), def_c_red_cyan );
    add_color(def_c_magenta_red, "c_magenta_red", COLOR_PAIR(28), def_c_red_magenta );
    add_color(def_c_brown_red, "c_brown_red", COLOR_PAIR(29), def_c_red_yellow );
    add_color(def_c_ltred_red, "c_ltred_red", COLOR_PAIR(24) | A_BOLD, def_c_red_red );
    add_color(def_c_ltgreen_red, "c_ltgreen_red", COLOR_PAIR(25) | A_BOLD, def_c_ltred_green );
    add_color(def_c_ltblue_red, "c_ltblue_red", COLOR_PAIR(26) | A_BOLD, def_h_ltred );
    add_color(def_c_ltcyan_red, "c_ltcyan_red", COLOR_PAIR(27) | A_BOLD, def_c_ltred_cyan );
    add_color(def_c_pink_red, "c_pink_red", COLOR_PAIR(28) | A_BOLD, def_c_ltred_magenta );
    add_color(def_c_yellow_red, "c_yellow_red", COLOR_PAIR(29) | A_BOLD, def_c_red_yellow );

    add_color(def_c_unset, "c_unset", COLOR_PAIR(31), def_c_unset );

    add_color(def_c_black_white, "c_black_white", COLOR_PAIR(32), def_c_ltgray );
    add_color(def_c_dkgray_white, "c_dkgray_white", COLOR_PAIR(32) | A_BOLD, def_c_white );
    add_color(def_c_ltgray_white, "c_ltgray_white", COLOR_PAIR(33), def_c_ltgray_white );
    add_color(def_c_white_white, "c_white_white", COLOR_PAIR(33) | A_BOLD, def_c_white_white );
    add_color(def_c_red_white, "c_red_white", COLOR_PAIR(34), def_c_white_red );
    add_color(def_c_ltred_white, "c_ltred_white", COLOR_PAIR(34) | A_BOLD, def_c_ltgray_red );
    add_color(def_c_green_white, "c_green_white", COLOR_PAIR(35), def_c_ltgray_green );
    add_color(def_c_ltgreen_white, "c_ltgreen_white", COLOR_PAIR(35) | A_BOLD, def_c_white_green );
    add_color(def_c_brown_white, "c_brown_white", COLOR_PAIR(36), def_c_ltgray_yellow );
    add_color(def_c_yellow_white, "c_yellow_white", COLOR_PAIR(36) | A_BOLD, def_c_white_yellow );
    add_color(def_c_blue_white, "c_blue_white", COLOR_PAIR(37), def_h_ltgray );
    add_color(def_c_ltblue_white, "c_ltblue_white", COLOR_PAIR(37) | A_BOLD, def_h_white );
    add_color(def_c_magenta_white, "c_magenta_white", COLOR_PAIR(38), def_c_ltgray_magenta );
    add_color(def_c_pink_white, "c_pink_white", COLOR_PAIR(38) | A_BOLD, def_c_white_magenta );
    add_color(def_c_cyan_white, "c_cyan_white", COLOR_PAIR(39), def_c_ltgray_cyan );
    add_color(def_c_ltcyan_white, "c_ltcyan_white", COLOR_PAIR(39) | A_BOLD, def_c_white_cyan );

    add_color(def_c_black_green, "c_black_green", COLOR_PAIR(40), def_c_green );
    add_color(def_c_dkgray_green, "c_dkgray_green", COLOR_PAIR(40) | A_BOLD, def_c_ltgreen );
    add_color(def_c_ltgray_green, "c_ltgray_green", COLOR_PAIR(41), def_c_green_white );
    add_color(def_c_white_green, "c_white_green", COLOR_PAIR(41) | A_BOLD, def_c_ltgreen_white );
    add_color(def_c_red_green, "c_red_green", COLOR_PAIR(42), def_c_green_red );
    add_color(def_c_ltred_green, "c_ltred_green", COLOR_PAIR(42) | A_BOLD, def_c_ltgreen_red );
    add_color(def_c_green_green, "c_green_green", COLOR_PAIR(43), def_c_green_green );
    add_color(def_c_ltgreen_green, "c_ltgreen_green", COLOR_PAIR(43) | A_BOLD, def_c_ltgreen_green );
    add_color(def_c_brown_green, "c_brown_green", COLOR_PAIR(44), def_c_green_yellow );
    add_color(def_c_yellow_green, "c_yellow_green", COLOR_PAIR(44) | A_BOLD, def_c_ltgreen_yellow );
    add_color(def_c_blue_green, "c_blue_green", COLOR_PAIR(45), def_h_green );
    add_color(def_c_ltblue_green, "c_ltblue_green", COLOR_PAIR(45) | A_BOLD, def_h_ltgreen );
    add_color(def_c_magenta_green, "c_magenta_green", COLOR_PAIR(46), def_c_green_magenta );
    add_color(def_c_pink_green, "c_pink_green", COLOR_PAIR(46) | A_BOLD, def_c_ltgreen_magenta );
    add_color(def_c_cyan_green, "c_cyan_green", COLOR_PAIR(47), def_c_green_cyan );
    add_color(def_c_ltcyan_green, "c_ltcyan_green", COLOR_PAIR(47) | A_BOLD, def_c_ltgreen_cyan );

    add_color(def_c_black_yellow, "c_black_yellow", COLOR_PAIR(48), def_c_brown );
    add_color(def_c_dkgray_yellow, "c_dkgray_yellow", COLOR_PAIR(48) | A_BOLD, def_c_yellow );
    add_color(def_c_ltgray_yellow, "c_ltgray_yellow", COLOR_PAIR(49), def_c_brown_white );
    add_color(def_c_white_yellow, "c_white_yellow", COLOR_PAIR(49) | A_BOLD, def_c_yellow_white );
    add_color(def_c_red_yellow, "c_red_yellow", COLOR_PAIR(50), def_c_yellow_red );
    add_color(def_c_ltred_yellow, "c_ltred_yellow", COLOR_PAIR(50) | A_BOLD, def_c_yellow_red );
    add_color(def_c_green_yellow, "c_green_yellow", COLOR_PAIR(51), def_c_brown_green );
    add_color(def_c_ltgreen_yellow, "c_ltgreen_yellow", COLOR_PAIR(51) | A_BOLD, def_c_yellow_green );
    add_color(def_c_brown_yellow, "c_brown_yellow", COLOR_PAIR(52), def_c_brown_yellow );
    add_color(def_c_yellow_yellow, "c_yellow_yellow", COLOR_PAIR(52) | A_BOLD, def_c_yellow_yellow );
    add_color(def_c_blue_yellow, "c_blue_yellow", COLOR_PAIR(53), def_h_brown );
    add_color(def_c_ltblue_yellow, "c_ltblue_yellow", COLOR_PAIR(53) | A_BOLD, def_h_yellow );
    add_color(def_c_magenta_yellow, "c_magenta_yellow", COLOR_PAIR(54), def_c_brown_magenta );
    add_color(def_c_pink_yellow, "c_pink_yellow", COLOR_PAIR(54) | A_BOLD, def_c_yellow_magenta );
    add_color(def_c_cyan_yellow, "c_cyan_yellow", COLOR_PAIR(55), def_c_brown_cyan );
    add_color(def_c_ltcyan_yellow, "c_ltcyan_yellow", COLOR_PAIR(55) | A_BOLD, def_c_yellow_cyan );

    add_color(def_c_black_magenta, "c_black_magenta", COLOR_PAIR(56), def_c_magenta );
    add_color(def_c_dkgray_magenta, "c_dkgray_magenta", COLOR_PAIR(56) | A_BOLD, def_c_pink );
    add_color(def_c_ltgray_magenta, "c_ltgray_magenta", COLOR_PAIR(57), def_c_magenta_white );
    add_color(def_c_white_magenta, "c_white_magenta", COLOR_PAIR(57) | A_BOLD, def_c_pink_white );
    add_color(def_c_red_magenta, "c_red_magenta", COLOR_PAIR(58), def_c_magenta_red );
    add_color(def_c_ltred_magenta, "c_ltred_magenta", COLOR_PAIR(58) | A_BOLD, def_c_pink_red );
    add_color(def_c_green_magenta, "c_green_magenta", COLOR_PAIR(59), def_c_magenta_green );
    add_color(def_c_ltgreen_magenta, "c_ltgreen_magenta", COLOR_PAIR(59) | A_BOLD, def_c_pink_green );
    add_color(def_c_brown_magenta, "c_brown_magenta", COLOR_PAIR(60), def_c_magenta_yellow );
    add_color(def_c_yellow_magenta, "c_yellow_magenta", COLOR_PAIR(60) | A_BOLD, def_c_pink_yellow );
    add_color(def_c_blue_magenta, "c_blue_magenta", COLOR_PAIR(61), def_h_magenta );
    add_color(def_c_ltblue_magenta, "c_ltblue_magenta", COLOR_PAIR(61) | A_BOLD, def_h_pink );
    add_color(def_c_magenta_magenta, "c_magenta_magenta", COLOR_PAIR(62), def_c_magenta_magenta );
    add_color(def_c_pink_magenta, "c_pink_magenta", COLOR_PAIR(62) | A_BOLD, def_c_pink_magenta );
    add_color(def_c_cyan_magenta, "c_cyan_magenta", COLOR_PAIR(63), def_c_magenta_cyan );
    add_color(def_c_ltcyan_magenta, "c_ltcyan_magenta", COLOR_PAIR(63) | A_BOLD, def_c_pink_cyan );

    add_color(def_c_black_cyan, "c_black_cyan", COLOR_PAIR(64), def_c_cyan );
    add_color(def_c_dkgray_cyan, "c_dkgray_cyan", COLOR_PAIR(64) | A_BOLD, def_c_ltcyan );
    add_color(def_c_ltgray_cyan, "c_ltgray_cyan", COLOR_PAIR(65), def_c_cyan_white );
    add_color(def_c_white_cyan, "c_white_cyan", COLOR_PAIR(65) | A_BOLD, def_c_ltcyan_white );
    add_color(def_c_red_cyan, "c_red_cyan", COLOR_PAIR(66), def_c_cyan_red );
    add_color(def_c_ltred_cyan, "c_ltred_cyan", COLOR_PAIR(66) | A_BOLD, def_c_ltcyan_red );
    add_color(def_c_green_cyan, "c_green_cyan", COLOR_PAIR(67), def_c_cyan_green );
    add_color(def_c_ltgreen_cyan, "c_ltgreen_cyan", COLOR_PAIR(67) | A_BOLD, def_c_ltcyan_green );
    add_color(def_c_brown_cyan, "c_brown_cyan", COLOR_PAIR(68), def_c_cyan_yellow );
    add_color(def_c_yellow_cyan, "c_yellow_cyan", COLOR_PAIR(68) | A_BOLD, def_c_ltcyan_yellow );
    add_color(def_c_blue_cyan, "c_blue_cyan", COLOR_PAIR(69), def_h_cyan );
    add_color(def_c_ltblue_cyan, "c_ltblue_cyan", COLOR_PAIR(69) | A_BOLD, def_h_ltcyan );
    add_color(def_c_magenta_cyan, "c_magenta_cyan", COLOR_PAIR(70), def_c_cyan_magenta );
    add_color(def_c_pink_cyan, "c_pink_cyan", COLOR_PAIR(70) | A_BOLD, def_c_ltcyan_magenta );
    add_color(def_c_cyan_cyan, "c_cyan_cyan", COLOR_PAIR(71), def_c_cyan_cyan );
    add_color(def_c_ltcyan_cyan, "c_ltcyan_cyan", COLOR_PAIR(71) | A_BOLD, def_c_ltcyan_cyan );
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
    init_pair(15, COLOR_WHITE,      COLOR_BLUE);
    init_pair(16, COLOR_RED,        COLOR_BLUE);
    init_pair(17, COLOR_GREEN,      COLOR_BLUE);
    init_pair(18, COLOR_BLUE,       COLOR_BLUE);
    init_pair(19, COLOR_CYAN,       COLOR_BLUE);
    init_pair(20, COLOR_BLACK,      COLOR_BLUE);
    init_pair(21, COLOR_MAGENTA,    COLOR_BLUE);
    init_pair(22, COLOR_YELLOW,     COLOR_BLUE);

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
    color_by_string_map = {
        {"br", {c_brown, _("brown")}}, {"lg", {c_ltgray, _("lt gray")}},
        {"dg", {c_dkgray, _("dk gray")}}, {"r", {c_ltred, _("lt red")}},
        {"R", {c_red, _("red")}}, {"g", {c_ltgreen, _("lt green")}},
        {"G", {c_green, _("green")}}, {"b", {c_ltblue, _("lt blue")}},
        {"B", {c_blue, _("blue")}}, {"W", {c_white, _("white")}},
        {"C", {c_cyan, _("cyan")}}, {"c", {c_ltcyan, _("lt cyan")}},
        {"P", {c_pink, _("pink")}}, {"m", {c_magenta, _("magenta")}}
    };
}

nc_color invert_color(nc_color c)
{
    const nc_color color = all_colors.get_invert( c );
    return ((int)color > 0) ? color : c_pink;
}

nc_color hilite(nc_color c)
{
    const nc_color color = all_colors.get_highlight(c, HL_BLUE);
    return ((int)color > 0) ? color : h_white;
}

nc_color red_background(nc_color c)
{
    const nc_color color = all_colors.get_highlight(c, HL_RED);
    return ((int)color > 0) ? color : c_white_red;
}

nc_color white_background(nc_color c)
{
    const nc_color color = all_colors.get_highlight(c, HL_WHITE);
    return ((int)color > 0) ? color : c_black_white;
}

nc_color green_background(nc_color c)
{
    const nc_color color = all_colors.get_highlight(c, HL_GREEN);
    return ((int)color > 0) ? color : c_black_green;
}

nc_color yellow_background(nc_color c)
{
    const nc_color color = all_colors.get_highlight(c, HL_YELLOW);
    return ((int)color > 0) ? color : c_black_yellow;
}

nc_color magenta_background(nc_color c)
{
    const nc_color color = all_colors.get_highlight(c, HL_MAGENTA);
    return ((int)color > 0) ? color : c_black_magenta;
}

nc_color cyan_background(nc_color c)
{
    const nc_color color = all_colors.get_highlight(c, HL_CYAN);
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

    const nc_color col = all_colors.name_to_color(new_color);
    if ( col > 0 ) {
        return col;
    }

    return c_unset;
}

/**
 * The reverse of color_from_string.
 */
std::string string_from_color(const nc_color color)
{
    std::string sColor = all_colors.get_name(color);
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

    const nc_color col = all_colors.name_to_color(color);
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
    auto const candidate_color = color_by_string_map.find( note_id );
    if( candidate_color != std::end( color_by_string_map ) ) {
        return candidate_color->second.color;
    }
    // The default note color.
    return c_yellow;
}

std::list<std::pair<std::string, std::string>> get_note_color_names()
{
    std::list<std::pair<std::string, std::string>> color_list;
    for( auto const &color_pair : color_by_string_map ) {
        color_list.emplace_back( color_pair.first, color_pair.second.name );
    }
    return color_list;
}


void color_manager::clear()
{
    name_map.clear();
    inverted_map.clear();
    for( auto &entry : color_array ) {
        entry.name_custom = "";
        entry.name_invert_custom = "";
    }
}

void color_manager::show_gui()
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
    const int iMaxColors = color_array.size();
    bool bStuffChanged = false;
    input_context ctxt("COLORS");
    ctxt.register_cardinal();
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("REMOVE_CUSTOM");
    ctxt.register_action("LOAD_TEMPLATE");
    ctxt.register_action("HELP_KEYBINDINGS");

    std::map<std::string, color_struct> name_color_map;

    for( const auto &pr : name_map ) {
        name_color_map[pr.first] = color_array[pr.second];
    }

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

        draw_scrollbar(w_colors_border, iCurrentLine, iContentHeight, iMaxColors, 5);
        wrefresh(w_colors_border);

        auto iter = name_color_map.begin();
        std::advance( iter, iStartPos );

        std::string sActive = "";

        // display colormanager
        for (int i=iStartPos; iter != name_color_map.end(); ++iter, ++i) {
            if (i >= iStartPos && i < iStartPos + ((iContentHeight > iMaxColors) ? iMaxColors : iContentHeight)) {
                auto &entry = iter->second;

                if (iCurrentLine == i) {
                    sActive = iter->first;
                    mvwprintz(w_colors, i - iStartPos, vLines[iCurrentCol-1] + 2, c_yellow, ">");
                }

                mvwprintz(w_colors, i - iStartPos, 3, c_white, iter->first.c_str()); //colorname
                mvwprintz(w_colors, i - iStartPos, 21, entry.color, _("default")); //default color

                if ( !entry.name_custom.empty() ) {
                    mvwprintz(w_colors, i - iStartPos, 30, name_color_map[entry.name_custom].color, entry.name_custom.c_str()); //custom color
                }

                mvwprintz(w_colors, i - iStartPos, 52, entry.invert, _("default")); //invert default color

                if ( !entry.name_invert_custom.empty() ) {
                    mvwprintz(w_colors, i - iStartPos, 61, name_color_map[entry.name_invert_custom].color, entry.name_invert_custom.c_str()); //invert custom color
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
            auto &entry = name_color_map[sActive];

            if ( iCurrentCol == 1 && !entry.name_custom.empty() ) {
                bStuffChanged = true;
                entry.name_custom = "";

            } else if ( iCurrentCol == 2 && !entry.name_invert_custom.empty() ) {
                bStuffChanged = true;
                entry.name_invert_custom = "";

            }

            finalize(); // Need to recalculate caches

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

                    clear();

                    load_default();
                    load_custom(vFiles[ui_templates.ret]);

                    name_color_map.clear();
                    for( const auto &pr : name_map ) {
                        name_color_map[pr.first] = color_array[pr.second];
                    }
                }
            }

            finalize(); // Need to recalculate caches

        } else if (action == "CONFIRM") {
            uimenu ui_colors;
            ui_colors.w_y = iHeaderHeight + 1 + iOffsetY;
            ui_colors.w_height = 18;
            ui_colors.return_invalid = true;

            std::string sColorType = _("Normal");
            std::string sSelected = name_color_map[sActive].name_custom;

            if ( iCurrentCol == 2 ) {
                sColorType = _("Invert");
                sSelected = name_color_map[sActive].name_invert_custom;

            }

            ui_colors.text = string_format( _("Custom %s color:"), sColorType.c_str() );

            int i = 0;
            for ( auto &iter : name_color_map ) {
                std::string sColor = iter.first;
                std::string sType = _("default");

                std::string name_custom = "";

                if ( sSelected == sColor ) {
                    ui_colors.selected = i;
                }

                if ( !iter.second.name_custom.empty() ) {
                    name_custom = " <color_" + iter.second.name_custom + ">" + iter.second.name_custom + "</color>";
                }

                ui_colors.addentry(string_format( "%-17s <color_%s>%s</color>%s", iter.first.c_str(), sColor.c_str(), sType.c_str(), name_custom.c_str() ) );

                i++;
            }

            ui_colors.addentry(std::string(_("Cancel")));
            ui_colors.query();

            if ( (size_t)ui_colors.ret < name_color_map.size() ) {
                bStuffChanged = true;

                iter = name_color_map.begin();
                std::advance( iter, ui_colors.ret );

                auto &entry = name_color_map[sActive];

                if ( iCurrentCol == 1 ) {
                    entry.name_custom = iter->first;

                } else if ( iCurrentCol == 2 ) {
                    entry.name_invert_custom = iter->first;

                }
            }

            finalize(); // Need to recalculate caches
        }
    }

    if( bStuffChanged && query_yn(_("Save changes?") ) ) {
        for( const auto &pr : name_color_map ) {
            color_id id = name_to_id( pr.first );
            color_array[id].name_custom = pr.second.name_custom;
            color_array[id].name_invert_custom = pr.second.name_invert_custom;
        }

        finalize();
        save_custom();

        clear();
        load_default();
        load_custom();
    }
}

bool color_manager::save_custom()
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

void color_manager::load_custom(const std::string &sPath)
{
    const auto file = ( sPath.empty() ) ? FILENAMES["custom_colors"] : sPath;

    std::ifstream fin;
    fin.open(file.c_str(), std::ifstream::in | std::ifstream::binary);
    if( !fin.good() ) {
        fin.close();
        finalize(); // Need to finalize regardless of success
        return;
    }

    try {
        JsonIn jsin(fin);
        deserialize(jsin);
    } catch( const JsonError &e ) {
        DebugLog(D_ERROR, DC_ALL) << "load_custom: " << e;
    }

    fin.close();
    finalize();
}

void color_manager::serialize(JsonOut &json) const
{
    json.start_array();
    for( auto &entry : color_array ) {
        if ( !entry.name_custom.empty() || !entry.name_invert_custom.empty()) {
            json.start_object();

            json.member( "name", id_to_name( entry.col_id ) );
            json.member( "custom", entry.name_custom );
            json.member( "invertcustom", entry.name_invert_custom );

            json.end_object();
        }
    }

    json.end_array();
}

void color_manager::deserialize(JsonIn &jsin)
{
    jsin.start_array();
    while (!jsin.end_array()) {
        JsonObject joColors = jsin.get_object();

        const std::string name = joColors.get_string("name");
        const std::string name_custom = joColors.get_string("custom");
        const std::string name_invert_custom = joColors.get_string("invertcustom");

        color_id id = name_to_id( name );
        auto &entry = color_array[id];

        if ( !name_custom.empty() ) {
            entry.name_custom = name_custom;
        }

        if ( !name_invert_custom.empty() ) {
            entry.name_invert_custom = name_invert_custom;
        }
    }
}
