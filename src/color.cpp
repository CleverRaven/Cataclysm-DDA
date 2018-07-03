#include "color.h"
#include "output.h"
#include "debug.h"
#include "input.h"
#include "path_info.h"
#include "cata_utility.h"
#include "filesystem.h"
#include "string_formatter.h"
#include "ui.h"
#include "translations.h"
#include "json.h"

#include <algorithm> // for std::count
#include <iostream>

void nc_color::serialize( JsonOut &jsout ) const
{
    jsout.write( attribute_value );
}

void nc_color::deserialize( JsonIn &jsin )
{
    attribute_value = jsin.get_int();
}

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
        }
    };

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
        const std::string root = my_name.substr( 2, my_name.length() - 2 );
        const size_t underscore_num = std::count( root.begin(), root.end(), '_' ) -
                                      ( root.find( "light_" ) != std::string::npos ) -
                                      ( root.find( "dark_" ) != std::string::npos );
        // do not try to highlight color pairs, highlighted, background, and invalid colors
        if( my_name.substr( 0, 2 ) == "c_" && root != "unset" && underscore_num < 1 ) {
            for( size_t j = 0; j < NUM_HL; j++ ) {
                entry.highlight[j] = highlight_from_names( my_name, hilights[j] );
            }
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
        DebugLog( D_ERROR, DC_ALL ) << "couldn't parse color: " << name ;
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
            debugmsg( "Couldn't find color %d", color.operator int() );
            return color_array[i].col_id;
        }
    }

    debugmsg( "Couldn't find color %d", color.operator int() );
    return def_c_unset;
}

nc_color color_manager::get( const color_id col ) const
{
    if( col >= num_colors ) {
        debugmsg( "Invalid color index: %d. Color array size: %ld", col,
                  static_cast<unsigned long>( color_array.size() ) );
        return nc_color();
    }

    auto &entry = color_array[col];

    return entry.custom > 0 ? entry.custom : entry.color;
}

std::string color_manager::get_name( const nc_color color ) const
{
    color_id id = color_to_id( color );
    for( const auto &iter : name_map ) {
        if( iter.second == id ) {
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
    color_struct st = {color_pair, nc_color(), nc_color(), nc_color(), {{nc_color(), nc_color(), nc_color(), nc_color(), nc_color(), nc_color(), nc_color()}}, col, inv_id, "", "" };
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

nc_color color_manager::highlight_from_names( const std::string &name,
        const std::string &bg_name ) const
{
    /*
    //             Base Name      Highlight      Red BG              White BG            Green BG            Yellow BG
    add_hightlight("c_black",     "h_black",     "",                 "c_black_white",    "c_black_green",    "c_black_yellow",   "c_black_magenta",      "c_black_cyan");
    add_hightlight("c_white",     "h_white",     "c_white_red",      "c_white_white",    "c_white_green",    "c_white_yellow",   "c_white_magenta",      "c_white_cyan");
    etc.
    */

    std::string hi_name;

    if( bg_name.empty() ) {  //c_black -> h_black
        hi_name = "h_" + name.substr( 2, name.length() - 2 );
    } else {
        hi_name = name + "_" + bg_name;
    }

    color_id id = name_to_id( hi_name );
    return color_array[id].color;
}

void color_manager::load_default()
{
    static const auto color_pair = []( const int n ) {
        return nc_color::from_color_pair_index( n );
    };
    //        Color         Name      Color Pair      Invert
    add_color( def_c_black, "c_black", color_pair( 30 ), def_i_black );
    add_color( def_c_white, "c_white", color_pair( 1 ).bold(), def_i_white );
    add_color( def_c_light_gray, "c_light_gray", color_pair( 1 ), def_i_light_gray );
    add_color( def_c_dark_gray, "c_dark_gray", color_pair( 30 ).bold(), def_i_dark_gray );
    add_color( def_c_red, "c_red", color_pair( 2 ), def_i_red );
    add_color( def_c_green, "c_green", color_pair( 3 ), def_i_green );
    add_color( def_c_blue, "c_blue", color_pair( 4 ), def_i_blue );
    add_color( def_c_cyan, "c_cyan", color_pair( 5 ), def_i_cyan );
    add_color( def_c_magenta, "c_magenta", color_pair( 6 ), def_i_magenta );
    add_color( def_c_brown, "c_brown", color_pair( 7 ), def_i_brown );
    add_color( def_c_light_red, "c_light_red", color_pair( 2 ).bold(), def_i_light_red );
    add_color( def_c_light_green, "c_light_green", color_pair( 3 ).bold(), def_i_light_green );
    add_color( def_c_light_blue, "c_light_blue", color_pair( 4 ).bold(), def_i_light_blue );
    add_color( def_c_light_cyan, "c_light_cyan", color_pair( 5 ).bold(), def_i_light_cyan );
    add_color( def_c_pink, "c_pink", color_pair( 6 ).bold(), def_i_pink );
    add_color( def_c_yellow, "c_yellow", color_pair( 7 ).bold(), def_i_yellow );

    add_color( def_h_black, "h_black", color_pair( 20 ), def_c_blue );
    add_color( def_h_white, "h_white", color_pair( 15 ).bold(), def_c_light_blue_white );
    add_color( def_h_light_gray, "h_light_gray", color_pair( 15 ), def_c_blue_white );
    add_color( def_h_dark_gray, "h_dark_gray", color_pair( 20 ).bold(), def_c_light_blue );
    add_color( def_h_red, "h_red", color_pair( 16 ), def_c_blue_red );
    add_color( def_h_green, "h_green", color_pair( 17 ), def_c_blue_green );
    add_color( def_h_blue, "h_blue", color_pair( 20 ), def_h_blue );
    add_color( def_h_cyan, "h_cyan", color_pair( 19 ), def_c_blue_cyan );
    add_color( def_h_magenta, "h_magenta", color_pair( 21 ), def_c_blue_magenta );
    add_color( def_h_brown, "h_brown", color_pair( 22 ), def_c_blue_yellow );
    add_color( def_h_light_red, "h_light_red", color_pair( 16 ).bold(), def_c_light_blue_red );
    add_color( def_h_light_green, "h_light_green", color_pair( 17 ).bold(), def_c_light_blue_green );
    add_color( def_h_light_blue, "h_light_blue", color_pair( 18 ).bold(), def_h_light_blue );
    add_color( def_h_light_cyan, "h_light_cyan", color_pair( 19 ).bold(), def_c_light_blue_cyan );
    add_color( def_h_pink, "h_pink", color_pair( 21 ).bold(), def_c_light_blue_magenta );
    add_color( def_h_yellow, "h_yellow", color_pair( 22 ).bold(), def_c_light_blue_yellow );

    add_color( def_i_black, "i_black", color_pair( 32 ), def_c_black );
    add_color( def_i_white, "i_white", color_pair( 8 ).blink(), def_c_white );
    add_color( def_i_light_gray, "i_light_gray", color_pair( 8 ), def_c_light_gray );
    add_color( def_i_dark_gray, "i_dark_gray", color_pair( 32 ).blink(), def_c_dark_gray );
    add_color( def_i_red, "i_red", color_pair( 9 ), def_c_red );
    add_color( def_i_green, "i_green", color_pair( 10 ), def_c_green );
    add_color( def_i_blue, "i_blue", color_pair( 11 ), def_c_blue );
    add_color( def_i_cyan, "i_cyan", color_pair( 12 ), def_c_cyan );
    add_color( def_i_magenta, "i_magenta", color_pair( 13 ), def_c_magenta );
    add_color( def_i_brown, "i_brown", color_pair( 14 ), def_c_brown );
    add_color( def_i_light_red, "i_light_red", color_pair( 9 ).blink(), def_c_light_red );
    add_color( def_i_light_green, "i_light_green", color_pair( 10 ).blink(), def_c_light_green );
    add_color( def_i_light_blue, "i_light_blue", color_pair( 11 ).blink(), def_c_light_blue );
    add_color( def_i_light_cyan, "i_light_cyan", color_pair( 12 ).blink(), def_c_light_cyan );
    add_color( def_i_pink, "i_pink", color_pair( 13 ).blink(), def_c_pink );
    add_color( def_i_yellow, "i_yellow", color_pair( 14 ).blink(), def_c_yellow );

    add_color( def_c_black_red, "c_black_red", color_pair( 9 ).bold(), def_c_red );
    add_color( def_c_white_red, "c_white_red", color_pair( 23 ).bold(), def_c_red_white );
    add_color( def_c_light_gray_red, "c_light_gray_red", color_pair( 23 ), def_c_light_red_white );
    add_color( def_c_dark_gray_red, "c_dark_gray_red", color_pair( 9 ), def_c_dark_gray_red );
    add_color( def_c_red_red, "c_red_red", color_pair( 9 ), def_c_red_red );
    add_color( def_c_green_red, "c_green_red", color_pair( 25 ), def_c_red_green );
    add_color( def_c_blue_red, "c_blue_red", color_pair( 26 ), def_h_red );
    add_color( def_c_cyan_red, "c_cyan_red", color_pair( 27 ), def_c_red_cyan );
    add_color( def_c_magenta_red, "c_magenta_red", color_pair( 28 ), def_c_red_magenta );
    add_color( def_c_brown_red, "c_brown_red", color_pair( 29 ), def_c_red_yellow );
    add_color( def_c_light_red_red, "c_light_red_red", color_pair( 24 ).bold(), def_c_red_red );
    add_color( def_c_light_green_red, "c_light_green_red", color_pair( 25 ).bold(),
               def_c_light_red_green );
    add_color( def_c_light_blue_red, "c_light_blue_red", color_pair( 26 ).bold(), def_h_light_red );
    add_color( def_c_light_cyan_red, "c_light_cyan_red", color_pair( 27 ).bold(),
               def_c_light_red_cyan );
    add_color( def_c_pink_red, "c_pink_red", color_pair( 28 ).bold(), def_c_light_red_magenta );
    add_color( def_c_yellow_red, "c_yellow_red", color_pair( 29 ).bold(), def_c_red_yellow );

    add_color( def_c_unset, "c_unset", color_pair( 31 ), def_c_unset );

    add_color( def_c_black_white, "c_black_white", color_pair( 32 ), def_c_light_gray );
    add_color( def_c_dark_gray_white, "c_dark_gray_white", color_pair( 32 ).bold(), def_c_white );
    add_color( def_c_light_gray_white, "c_light_gray_white", color_pair( 33 ), def_c_light_gray_white );
    add_color( def_c_white_white, "c_white_white", color_pair( 33 ).bold(), def_c_white_white );
    add_color( def_c_red_white, "c_red_white", color_pair( 34 ), def_c_white_red );
    add_color( def_c_light_red_white, "c_light_red_white", color_pair( 34 ).bold(),
               def_c_light_gray_red );
    add_color( def_c_green_white, "c_green_white", color_pair( 35 ), def_c_light_gray_green );
    add_color( def_c_light_green_white, "c_light_green_white", color_pair( 35 ).bold(),
               def_c_white_green );
    add_color( def_c_brown_white, "c_brown_white", color_pair( 36 ), def_c_light_gray_yellow );
    add_color( def_c_yellow_white, "c_yellow_white", color_pair( 36 ).bold(), def_c_white_yellow );
    add_color( def_c_blue_white, "c_blue_white", color_pair( 37 ), def_h_light_gray );
    add_color( def_c_light_blue_white, "c_light_blue_white", color_pair( 37 ).bold(), def_h_white );
    add_color( def_c_magenta_white, "c_magenta_white", color_pair( 38 ), def_c_light_gray_magenta );
    add_color( def_c_pink_white, "c_pink_white", color_pair( 38 ).bold(), def_c_white_magenta );
    add_color( def_c_cyan_white, "c_cyan_white", color_pair( 39 ), def_c_light_gray_cyan );
    add_color( def_c_light_cyan_white, "c_light_cyan_white", color_pair( 39 ).bold(),
               def_c_white_cyan );

    add_color( def_c_black_green, "c_black_green", color_pair( 40 ), def_c_green );
    add_color( def_c_dark_gray_green, "c_dark_gray_green", color_pair( 40 ).bold(), def_c_light_green );
    add_color( def_c_light_gray_green, "c_light_gray_green", color_pair( 41 ), def_c_green_white );
    add_color( def_c_white_green, "c_white_green", color_pair( 41 ).bold(), def_c_light_green_white );
    add_color( def_c_red_green, "c_red_green", color_pair( 42 ), def_c_green_red );
    add_color( def_c_light_red_green, "c_light_red_green", color_pair( 42 ).bold(),
               def_c_light_green_red );
    add_color( def_c_green_green, "c_green_green", color_pair( 43 ), def_c_green_green );
    add_color( def_c_light_green_green, "c_light_green_green", color_pair( 43 ).bold(),
               def_c_light_green_green );
    add_color( def_c_brown_green, "c_brown_green", color_pair( 44 ), def_c_green_yellow );
    add_color( def_c_yellow_green, "c_yellow_green", color_pair( 44 ).bold(),
               def_c_light_green_yellow );
    add_color( def_c_blue_green, "c_blue_green", color_pair( 45 ), def_h_green );
    add_color( def_c_light_blue_green, "c_light_blue_green", color_pair( 45 ).bold(),
               def_h_light_green );
    add_color( def_c_magenta_green, "c_magenta_green", color_pair( 46 ), def_c_green_magenta );
    add_color( def_c_pink_green, "c_pink_green", color_pair( 46 ).bold(), def_c_light_green_magenta );
    add_color( def_c_cyan_green, "c_cyan_green", color_pair( 47 ), def_c_green_cyan );
    add_color( def_c_light_cyan_green, "c_light_cyan_green", color_pair( 47 ).bold(),
               def_c_light_green_cyan );

    add_color( def_c_black_yellow, "c_black_yellow", color_pair( 48 ), def_c_brown );
    add_color( def_c_dark_gray_yellow, "c_dark_gray_yellow", color_pair( 48 ).bold(), def_c_yellow );
    add_color( def_c_light_gray_yellow, "c_light_gray_yellow", color_pair( 49 ), def_c_brown_white );
    add_color( def_c_white_yellow, "c_white_yellow", color_pair( 49 ).bold(), def_c_yellow_white );
    add_color( def_c_red_yellow, "c_red_yellow", color_pair( 50 ), def_c_yellow_red );
    add_color( def_c_light_red_yellow, "c_light_red_yellow", color_pair( 50 ).bold(),
               def_c_yellow_red );
    add_color( def_c_green_yellow, "c_green_yellow", color_pair( 51 ), def_c_brown_green );
    add_color( def_c_light_green_yellow, "c_light_green_yellow", color_pair( 51 ).bold(),
               def_c_yellow_green );
    add_color( def_c_brown_yellow, "c_brown_yellow", color_pair( 52 ), def_c_brown_yellow );
    add_color( def_c_yellow_yellow, "c_yellow_yellow", color_pair( 52 ).bold(), def_c_yellow_yellow );
    add_color( def_c_blue_yellow, "c_blue_yellow", color_pair( 53 ), def_h_brown );
    add_color( def_c_light_blue_yellow, "c_light_blue_yellow", color_pair( 53 ).bold(), def_h_yellow );
    add_color( def_c_magenta_yellow, "c_magenta_yellow", color_pair( 54 ), def_c_brown_magenta );
    add_color( def_c_pink_yellow, "c_pink_yellow", color_pair( 54 ).bold(), def_c_yellow_magenta );
    add_color( def_c_cyan_yellow, "c_cyan_yellow", color_pair( 55 ), def_c_brown_cyan );
    add_color( def_c_light_cyan_yellow, "c_light_cyan_yellow", color_pair( 55 ).bold(),
               def_c_yellow_cyan );

    add_color( def_c_black_magenta, "c_black_magenta", color_pair( 56 ), def_c_magenta );
    add_color( def_c_dark_gray_magenta, "c_dark_gray_magenta", color_pair( 56 ).bold(), def_c_pink );
    add_color( def_c_light_gray_magenta, "c_light_gray_magenta", color_pair( 57 ),
               def_c_magenta_white );
    add_color( def_c_white_magenta, "c_white_magenta", color_pair( 57 ).bold(), def_c_pink_white );
    add_color( def_c_red_magenta, "c_red_magenta", color_pair( 58 ), def_c_magenta_red );
    add_color( def_c_light_red_magenta, "c_light_red_magenta", color_pair( 58 ).bold(),
               def_c_pink_red );
    add_color( def_c_green_magenta, "c_green_magenta", color_pair( 59 ), def_c_magenta_green );
    add_color( def_c_light_green_magenta, "c_light_green_magenta", color_pair( 59 ).bold(),
               def_c_pink_green );
    add_color( def_c_brown_magenta, "c_brown_magenta", color_pair( 60 ), def_c_magenta_yellow );
    add_color( def_c_yellow_magenta, "c_yellow_magenta", color_pair( 60 ).bold(), def_c_pink_yellow );
    add_color( def_c_blue_magenta, "c_blue_magenta", color_pair( 61 ), def_h_magenta );
    add_color( def_c_light_blue_magenta, "c_light_blue_magenta", color_pair( 61 ).bold(), def_h_pink );
    add_color( def_c_magenta_magenta, "c_magenta_magenta", color_pair( 62 ), def_c_magenta_magenta );
    add_color( def_c_pink_magenta, "c_pink_magenta", color_pair( 62 ).bold(), def_c_pink_magenta );
    add_color( def_c_cyan_magenta, "c_cyan_magenta", color_pair( 63 ), def_c_magenta_cyan );
    add_color( def_c_light_cyan_magenta, "c_light_cyan_magenta", color_pair( 63 ).bold(),
               def_c_pink_cyan );

    add_color( def_c_black_cyan, "c_black_cyan", color_pair( 64 ), def_c_cyan );
    add_color( def_c_dark_gray_cyan, "c_dark_gray_cyan", color_pair( 64 ).bold(), def_c_light_cyan );
    add_color( def_c_light_gray_cyan, "c_light_gray_cyan", color_pair( 65 ), def_c_cyan_white );
    add_color( def_c_white_cyan, "c_white_cyan", color_pair( 65 ).bold(), def_c_light_cyan_white );
    add_color( def_c_red_cyan, "c_red_cyan", color_pair( 66 ), def_c_cyan_red );
    add_color( def_c_light_red_cyan, "c_light_red_cyan", color_pair( 66 ).bold(),
               def_c_light_cyan_red );
    add_color( def_c_green_cyan, "c_green_cyan", color_pair( 67 ), def_c_cyan_green );
    add_color( def_c_light_green_cyan, "c_light_green_cyan", color_pair( 67 ).bold(),
               def_c_light_cyan_green );
    add_color( def_c_brown_cyan, "c_brown_cyan", color_pair( 68 ), def_c_cyan_yellow );
    add_color( def_c_yellow_cyan, "c_yellow_cyan", color_pair( 68 ).bold(), def_c_light_cyan_yellow );
    add_color( def_c_blue_cyan, "c_blue_cyan", color_pair( 69 ), def_h_cyan );
    add_color( def_c_light_blue_cyan, "c_light_blue_cyan", color_pair( 69 ).bold(), def_h_light_cyan );
    add_color( def_c_magenta_cyan, "c_magenta_cyan", color_pair( 70 ), def_c_cyan_magenta );
    add_color( def_c_pink_cyan, "c_pink_cyan", color_pair( 70 ).bold(), def_c_light_cyan_magenta );
    add_color( def_c_cyan_cyan, "c_cyan_cyan", color_pair( 71 ), def_c_cyan_cyan );
    add_color( def_c_light_cyan_cyan, "c_light_cyan_cyan", color_pair( 71 ).bold(),
               def_c_light_cyan_cyan );
}

void init_colors()
{
    using namespace catacurses; // to get the base_color enumeration
    init_pair( 1, white,      black );
    init_pair( 2, red,        black );
    init_pair( 3, green,      black );
    init_pair( 4, blue,       black );
    init_pair( 5, cyan,       black );
    init_pair( 6, magenta,    black );
    init_pair( 7, yellow,     black );

    // Inverted Colors
    init_pair( 8, black,      white );
    init_pair( 9, black,      red );
    init_pair( 10, black,      green );
    init_pair( 11, black,      blue );
    init_pair( 12, black,      cyan );
    init_pair( 13, black,      magenta );
    init_pair( 14, black,      yellow );

    // Highlighted - blue background
    init_pair( 15, white,      blue );
    init_pair( 16, red,        blue );
    init_pair( 17, green,      blue );
    init_pair( 18, blue,       blue );
    init_pair( 19, cyan,       blue );
    init_pair( 20, black,      blue );
    init_pair( 21, magenta,    blue );
    init_pair( 22, yellow,     blue );

    // Red background - for monsters on fire
    init_pair( 23, white,      red );
    init_pair( 24, red,        red );
    init_pair( 25, green,      red );
    init_pair( 26, blue,       red );
    init_pair( 27, cyan,       red );
    init_pair( 28, magenta,    red );
    init_pair( 29, yellow,     red );

    init_pair( 30, black,      black );
    init_pair( 31, white,      black );

    init_pair( 32, black,      white );
    init_pair( 33, white,      white );
    init_pair( 34, red,        white );
    init_pair( 35, green,      white );
    init_pair( 36, yellow,     white );
    init_pair( 37, blue,       white );
    init_pair( 38, magenta,    white );
    init_pair( 39, cyan,       white );

    init_pair( 40, black,      green );
    init_pair( 41, white,      green );
    init_pair( 42, red,        green );
    init_pair( 43, green,      green );
    init_pair( 44, yellow,     green );
    init_pair( 45, blue,       green );
    init_pair( 46, magenta,    green );
    init_pair( 47, cyan,       green );

    init_pair( 48, black,      yellow );
    init_pair( 49, white,      yellow );
    init_pair( 50, red,        yellow );
    init_pair( 51, green,      yellow );
    init_pair( 52, yellow,     yellow );
    init_pair( 53, blue,       yellow );
    init_pair( 54, magenta,    yellow );
    init_pair( 55, cyan,       yellow );

    init_pair( 56, black,      magenta );
    init_pair( 57, white,      magenta );
    init_pair( 58, red,        magenta );
    init_pair( 59, green,      magenta );
    init_pair( 60, yellow,     magenta );
    init_pair( 61, blue,       magenta );
    init_pair( 62, magenta,    magenta );
    init_pair( 63, cyan,       magenta );

    init_pair( 64, black,      cyan );
    init_pair( 65, white,      cyan );
    init_pair( 66, red,        cyan );
    init_pair( 67, green,      cyan );
    init_pair( 68, yellow,     cyan );
    init_pair( 69, blue,       cyan );
    init_pair( 70, magenta,    cyan );
    init_pair( 71, cyan,       cyan );

    all_colors.load_default();
    all_colors.load_custom();

    // The short color codes (e.g. "br") are intentionally untranslatable.
    color_by_string_map = {
        {"br", {c_brown, _( "brown" )}}, {"lg", {c_light_gray, _( "light gray" )}},
        {"dg", {c_dark_gray, _( "dark gray" )}}, {"r", {c_light_red, _( "light red" )}},
        {"R", {c_red, _( "red" )}}, {"g", {c_light_green, _( "light green" )}},
        {"G", {c_green, _( "green" )}}, {"b", {c_light_blue, _( "light blue" )}},
        {"B", {c_blue, _( "blue" )}}, {"W", {c_white, _( "white" )}},
        {"C", {c_cyan, _( "cyan" )}}, {"c", {c_light_cyan, _( "light cyan" )}},
        {"P", {c_pink, _( "pink" )}}, {"m", {c_magenta, _( "magenta" )}}
    };
}

nc_color invert_color( nc_color c )
{
    const nc_color color = all_colors.get_invert( c );
    return ( ( int )color > 0 ) ? color : c_pink;
}

nc_color hilite( nc_color c )
{
    const nc_color color = all_colors.get_highlight( c, HL_BLUE );
    return ( ( int )color > 0 ) ? color : h_white;
}

nc_color red_background( nc_color c )
{
    const nc_color color = all_colors.get_highlight( c, HL_RED );
    return ( ( int )color > 0 ) ? color : c_white_red;
}

nc_color white_background( nc_color c )
{
    const nc_color color = all_colors.get_highlight( c, HL_WHITE );
    return ( ( int )color > 0 ) ? color : c_black_white;
}

nc_color green_background( nc_color c )
{
    const nc_color color = all_colors.get_highlight( c, HL_GREEN );
    return ( ( int )color > 0 ) ? color : c_black_green;
}

nc_color yellow_background( nc_color c )
{
    const nc_color color = all_colors.get_highlight( c, HL_YELLOW );
    return ( ( int )color > 0 ) ? color : c_black_yellow;
}

nc_color magenta_background( nc_color c )
{
    const nc_color color = all_colors.get_highlight( c, HL_MAGENTA );
    return ( ( int )color > 0 ) ? color : c_black_magenta;
}

nc_color cyan_background( nc_color c )
{
    const nc_color color = all_colors.get_highlight( c, HL_CYAN );
    return ( ( int )color > 0 ) ? color : c_black_cyan;
}

/**
 * Given the name of a foreground color, returns the nc_color value that matches. If
 * no match is found, c_unset is returned.
 * Special cases:
 * {"black"           , c_black}, // missing default prefix c_
 * {"<c|h|i>_black"   , h_black}, // has prefix c_ or h_ or i_
 * {"dark_gray_red"   , c_dark_gray_red}, // use dark_ instead of dk as the latter is being deprecated
 * {"light_blue_red"  , c_light_blue_red}, // use light_ instead of lt as the latter is being deprecated
 * @param color The color to get, as a std::string.
 * @return The nc_color constant that matches the input.
 */
nc_color color_from_string( const std::string &color )
{

    std::string new_color = color;
    if( new_color.substr( 1, 1 ) != "_" ) { //c_  //i_  //h_
        new_color = "c_" + new_color;
    }

    const std::pair<std::string, std::string> pSearch[2] = { { "light_", "lt" }, { "dark_", "dk" } };
    for( int i = 0; i < 2; ++i ) {
        size_t pos = 0;
        while( ( pos = new_color.find( pSearch[i].second, pos ) ) != std::string::npos ) {
            new_color.replace( pos, pSearch[i].second.length(), pSearch[i].first );
            pos += pSearch[i].first.length();
            DebugLog( D_WARNING, DC_ALL ) << "Deprecated foreground color suffix was used: (" <<
                                          pSearch[i].second << ") in (" << color << ").  Please update mod that uses that.";
        }
    }

    const nc_color col = all_colors.name_to_color( new_color );
    if( col > 0 ) {
        return col;
    }

    return c_unset;
}

/**
 * The reverse of color_from_string.
 */
std::string string_from_color( const nc_color color )
{
    std::string sColor = all_colors.get_name( color );
    sColor = sColor.substr( 2, sColor.length() - 2 );

    if( sColor != "unset" ) {
        return sColor;
    }

    return "white";
}

/**
 * Given the name of a background color (that is, one of the i_xxxxx colors),
 * returns the nc_color constant that matches. If no match is found, i_white is
 * returned.
 * @param color The color to get, as a std::string.
 * @return The nc_color constant that matches the input.
 */
nc_color bgcolor_from_string( const std::string &color )
{

    std::string new_color = "i_" + color;

    const std::pair<std::string, std::string> pSearch[2] = { { "light_", "lt" }, { "dark_", "dk" } };
    for( int i = 0; i < 2; ++i ) {
        size_t pos = 0;
        while( ( pos = new_color.find( pSearch[i].second, pos ) ) != std::string::npos ) {
            new_color.replace( pos, pSearch[i].second.length(), pSearch[i].first );
            pos += pSearch[i].first.length();
            DebugLog( D_WARNING, DC_ALL ) << "Deprecated background color suffix was used: (" <<
                                          pSearch[i].second << ") in (" << color << ").  Please update mod that uses that.";
        }
    }

    const nc_color col = all_colors.name_to_color( new_color );
    if( col > 0 ) {
        return col;
    }

    return i_white;
}

nc_color get_color_from_tag( const std::string &s, const nc_color base_color )
{
    if( s.empty() || s[0] != '<' || s.substr( 0, 8 ) == "</color>" ) {
        return base_color;
    }
    if( s.substr( 0, 7 ) != "<color_" ) {
        return base_color;
    }
    size_t tag_close = s.find( '>' );
    if( tag_close == std::string::npos ) {
        return base_color;
    }
    std::string color_name = s.substr( 7, tag_close - 7 );
    return color_from_string( color_name );
}

std::string get_tag_from_color( const nc_color color )
{
    return "<color_" + string_from_color( color ) + ">";
}

nc_color get_note_color( std::string const &note_id )
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
        entry.name_custom.clear();
        entry.name_invert_custom.clear();
    }
}

void draw_header( const catacurses::window &w )
{
    int tmpx = 0;
    tmpx += shortcut_print( w, 0, tmpx, c_white, c_light_green,
                            _( "<R>emove custom color" ) ) + 2;
    tmpx += shortcut_print( w, 0, tmpx, c_white, c_light_green,
                            _( "<Arrow Keys> To navigate" ) ) + 2;
    tmpx += shortcut_print( w, 0, tmpx, c_white, c_light_green, _( "<Enter>-Edit" ) ) + 2;
    shortcut_print( w, 0, tmpx, c_white, c_light_green, _( "Load <T>emplate" ) );

    mvwprintz( w, 1, 0, c_white, _( "Some color changes may require a restart." ) );

    mvwhline( w, 2, 0, LINE_OXOX, getmaxx( w ) ); // Draw line under header
    mvwputch( w, 2, 48, BORDER_COLOR, LINE_OXXX ); //^|^

    mvwprintz( w, 3, 3, c_white, _( "Colorname" ) );
    mvwprintz( w, 3, 21, c_white, _( "Normal" ) );
    mvwprintz( w, 3, 52, c_white, _( "Invert" ) );

    wrefresh( w );
}

void color_manager::show_gui()
{
    const int iHeaderHeight = 4;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 2 - iHeaderHeight;

    const int iOffsetX = ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;
    const int iOffsetY = ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;

    std::vector<int> vLines;
    vLines.push_back( -1 );
    vLines.push_back( 48 );

    const int iTotalCols = vLines.size();

    catacurses::window w_colors_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                         iOffsetY, iOffsetX );
    catacurses::window w_colors_header = catacurses::newwin( iHeaderHeight, FULL_SCREEN_WIDTH - 2,
                                         1 + iOffsetY, 1 + iOffsetX );
    catacurses::window w_colors = catacurses::newwin( iContentHeight, FULL_SCREEN_WIDTH - 2,
                                  iHeaderHeight + 1 + iOffsetY, 1 + iOffsetX );

    draw_border( w_colors_border, BORDER_COLOR, _( " COLOR MANAGER " ) );
    mvwputch( w_colors_border, 3,  0, BORDER_COLOR, LINE_XXXO ); // |-
    mvwputch( w_colors_border, 3, getmaxx( w_colors_border ) - 1, BORDER_COLOR, LINE_XOXX ); // -|

    for( auto &iCol : vLines ) {
        if( iCol > -1 ) {
            mvwputch( w_colors_border, FULL_SCREEN_HEIGHT - 1, iCol + 1, BORDER_COLOR, LINE_XXOX ); // _|_
            mvwputch( w_colors_header, 3, iCol, BORDER_COLOR, LINE_XOXO );
        }
    }
    wrefresh( w_colors_border );

    draw_header( w_colors_header );

    int iCurrentLine = 0;
    int iCurrentCol = 1;
    int iStartPos = 0;
    const int iMaxColors = color_array.size();
    bool bStuffChanged = false;
    input_context ctxt( "COLORS" );
    ctxt.register_cardinal();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "REMOVE_CUSTOM" );
    ctxt.register_action( "LOAD_TEMPLATE" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    std::map<std::string, color_struct> name_color_map;

    for( const auto &pr : name_map ) {
        name_color_map[pr.first] = color_array[pr.second];
    }

    while( true ) {
        // Clear all lines
        for( int i = 0; i < iContentHeight; i++ ) {
            for( int j = 0; j < 79; j++ ) {
                mvwputch( w_colors, i, j, c_black, ' ' );

                for( auto &iCol : vLines ) {
                    if( iCol == j ) {
                        mvwputch( w_colors, i, j, BORDER_COLOR, LINE_XOXO );
                    }
                }
            }
        }

        calcStartPos( iStartPos, iCurrentLine, iContentHeight, iMaxColors );

        draw_scrollbar( w_colors_border, iCurrentLine, iContentHeight, iMaxColors, 5 );
        wrefresh( w_colors_border );

        auto iter = name_color_map.begin();
        std::advance( iter, iStartPos );

        std::string sActive = "";

        // display color manager
        for( int i = iStartPos; iter != name_color_map.end(); ++iter, ++i ) {
            if( i >= iStartPos &&
                i < iStartPos + ( ( iContentHeight > iMaxColors ) ? iMaxColors : iContentHeight ) ) {
                auto &entry = iter->second;

                if( iCurrentLine == i ) {
                    sActive = iter->first;
                    mvwprintz( w_colors, i - iStartPos, vLines[iCurrentCol - 1] + 2, c_yellow, ">" );
                }

                mvwprintz( w_colors, i - iStartPos, 3, c_white, iter->first.c_str() ); //color name
                mvwprintz( w_colors, i - iStartPos, 21, entry.color, _( "default" ) ); //default color

                if( !entry.name_custom.empty() ) {
                    mvwprintz( w_colors, i - iStartPos, 30, name_color_map[entry.name_custom].color,
                               entry.name_custom.c_str() ); //custom color
                }

                mvwprintz( w_colors, i - iStartPos, 52, entry.invert, _( "default" ) ); //invert default color

                if( !entry.name_invert_custom.empty() ) {
                    mvwprintz( w_colors, i - iStartPos, 61, name_color_map[entry.name_invert_custom].color,
                               entry.name_invert_custom.c_str() ); //invert custom color
                }
            }
        }

        wrefresh( w_colors );

        const std::string action = ctxt.handle_input();

        if( action == "QUIT" ) {
            break;
        } else if( action == "UP" ) {
            iCurrentLine--;
            if( iCurrentLine < 0 ) {
                iCurrentLine = iMaxColors - 1;
            }
        } else if( action == "DOWN" ) {
            iCurrentLine++;
            if( iCurrentLine >= ( int )iMaxColors ) {
                iCurrentLine = 0;
            }
        } else if( action == "LEFT" ) {
            iCurrentCol--;
            if( iCurrentCol < 1 ) {
                iCurrentCol = iTotalCols;
            }
        } else if( action == "RIGHT" ) {
            iCurrentCol++;
            if( iCurrentCol > iTotalCols ) {
                iCurrentCol = 1;
            }
        } else if( action == "REMOVE_CUSTOM" ) {
            auto &entry = name_color_map[sActive];

            if( iCurrentCol == 1 && !entry.name_custom.empty() ) {
                bStuffChanged = true;
                entry.name_custom.clear();

            } else if( iCurrentCol == 2 && !entry.name_invert_custom.empty() ) {
                bStuffChanged = true;
                entry.name_invert_custom.clear();

            }

            finalize(); // Need to recalculate caches

        } else if( action == "LOAD_TEMPLATE" ) {
            auto vFiles = get_files_from_path( ".json", FILENAMES["color_templates"], false, true );

            if( vFiles.size() > 0 ) {
                uimenu ui_templates;
                ui_templates.w_y = iHeaderHeight + 1 + iOffsetY;
                ui_templates.w_height = 18;
                ui_templates.return_invalid = true;

                ui_templates.text = _( "Color templates:" );

                for( const auto &filename : vFiles ) {
                    ui_templates.addentry( filename.substr( filename.find_last_of( "/" ) + 1 ) );
                }

                ui_templates.addentry( std::string( _( "Cancel" ) ) );
                ui_templates.query();

                if( ( size_t )ui_templates.ret < vFiles.size() ) {
                    bStuffChanged = true;

                    clear();

                    load_default();
                    load_custom( vFiles[ui_templates.ret] );

                    name_color_map.clear();
                    for( const auto &pr : name_map ) {
                        name_color_map[pr.first] = color_array[pr.second];
                    }
                }
            }

            finalize(); // Need to recalculate caches

        } else if( action == "CONFIRM" ) {
            uimenu ui_colors;
            ui_colors.w_y = iHeaderHeight + 1 + iOffsetY;
            ui_colors.w_height = 18;
            ui_colors.return_invalid = true;

            std::string sColorType = _( "Normal" );
            std::string sSelected = name_color_map[sActive].name_custom;

            if( iCurrentCol == 2 ) {
                sColorType = _( "Invert" );
                sSelected = name_color_map[sActive].name_invert_custom;

            }

            ui_colors.text = string_format( _( "Custom %s color:" ), sColorType.c_str() );

            int i = 0;
            for( auto &iter : name_color_map ) {
                std::string sColor = iter.first;
                std::string sType = _( "default" );

                std::string name_custom = "";

                if( sSelected == sColor ) {
                    ui_colors.selected = i;
                }

                if( !iter.second.name_custom.empty() ) {
                    name_custom = " <color_" + iter.second.name_custom + ">" + iter.second.name_custom + "</color>";
                }

                ui_colors.addentry( string_format( "%-17s <color_%s>%s</color>%s", iter.first.c_str(),
                                                   sColor.c_str(), sType.c_str(), name_custom.c_str() ) );

                i++;
            }

            ui_colors.addentry( std::string( _( "Cancel" ) ) );
            ui_colors.query();

            if( ( size_t )ui_colors.ret < name_color_map.size() ) {
                bStuffChanged = true;

                iter = name_color_map.begin();
                std::advance( iter, ui_colors.ret );

                auto &entry = name_color_map[sActive];

                if( iCurrentCol == 1 ) {
                    entry.name_custom = iter->first;

                } else if( iCurrentCol == 2 ) {
                    entry.name_invert_custom = iter->first;

                }
            }

            finalize(); // Need to recalculate caches
        } else if( action == "HELP_KEYBINDINGS" ) {
            draw_header( w_colors_header );
        }
    }

    if( bStuffChanged && query_yn( _( "Save changes?" ) ) ) {
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

    return write_to_file_exclusive( savefile, [&]( std::ostream & fout ) {
        JsonOut jsout( fout );
        serialize( jsout );
    }, _( "custom colors" ) );
}

void color_manager::load_custom( const std::string &sPath )
{
    const auto file = ( sPath.empty() ) ? FILENAMES["custom_colors"] : sPath;

    read_from_file_optional_json( file, [this]( JsonIn & jsonin ) {
        deserialize( jsonin );
    } );
    finalize(); // Need to finalize regardless of success
}

void color_manager::serialize( JsonOut &json ) const
{
    json.start_array();
    for( auto &entry : color_array ) {
        if( !entry.name_custom.empty() || !entry.name_invert_custom.empty() ) {
            json.start_object();

            json.member( "name", id_to_name( entry.col_id ) );
            json.member( "custom", entry.name_custom );
            json.member( "invertcustom", entry.name_invert_custom );

            json.end_object();
        }
    }

    json.end_array();
}

void color_manager::deserialize( JsonIn &jsin )
{
    jsin.start_array();
    while( !jsin.end_array() ) {
        JsonObject joColors = jsin.get_object();

        const std::string name = joColors.get_string( "name" );
        const std::string name_custom = joColors.get_string( "custom" );
        const std::string name_invert_custom = joColors.get_string( "invertcustom" );

        color_id id = name_to_id( name );
        auto &entry = color_array[id];

        if( !name_custom.empty() ) {
            entry.name_custom = name_custom;
        }

        if( !name_invert_custom.empty() ) {
            entry.name_invert_custom = name_invert_custom;
        }
    }
}
