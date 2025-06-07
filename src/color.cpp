#include "color.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "cata_path.h"
#include "cata_utility.h"
#include "cursesdef.h"
#include "debug.h"
#include "filesystem.h"
#include "flexbuffer_json-inl.h"
#include "flexbuffer_json.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "json.h"
#include "output.h"
#include "path_info.h"
#include "point.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "uilist.h"
#include "ui_manager.h"
#include "cata_imgui.h"

nc_color::operator ImVec4()
{
    return cataimgui::imvec4_from_color( *this );
}

void nc_color::serialize( JsonOut &jsout ) const
{
    jsout.write( attribute_value );
}

void nc_color::deserialize( int value )
{
    attribute_value = value;
}

color_manager &get_all_colors()
{
    static color_manager single_instance;
    return single_instance;
}

static std::unordered_map<std::string, note_color> color_by_string_map;

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

    for( color_manager::color_struct &entry : color_array ) {
        entry.invert = get( entry.invert_id );

        if( !entry.name_custom.empty() ) {
            // Not using name_to_color because we want default color of this name
            const color_id id = name_to_id( entry.name_custom );
            auto &other = color_array[id];
            entry.custom = other.color;
        }

        if( !entry.name_invert_custom.empty() ) {
            const color_id id = name_to_id( entry.name_invert_custom );
            auto &other = color_array[id];
            entry.invert_custom = other.color;
        }

        inverted_map[entry.color] = entry.col_id;
        inverted_map[entry.invert] = entry.invert_id;
    }

    // Highlights in a next run, to make sure custom colors are set
    for( color_manager::color_struct &entry : color_array ) {
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

nc_color color_manager::name_to_color( const std::string &name,
                                       const report_color_error color_error ) const
{
    const color_id id = name_to_id( name, color_error );
    const color_struct &entry = color_array[id];

    return entry.custom.to_int() > 0 ? entry.custom : entry.color;
}

color_id color_manager::name_to_id( const std::string &name,
                                    const report_color_error color_error ) const
{
    auto iter = name_map.find( name );
    if( iter == name_map.end() ) {
        if( color_error == report_color_error::yes ) {
            debugmsg( "couldn't parse color: %s", name );
        }
        return def_c_unset;
    }

    return iter->second;
}

std::string color_manager::id_to_name( const color_id id ) const
{
    return color_array[id].name;
}

color_id color_manager::color_to_id( const nc_color &color ) const
{
    auto iter = inverted_map.find( color );
    if( iter != inverted_map.end() ) {
        return iter->second;
    }

    // Optimally this shouldn't happen, but allow for now
    for( const color_manager::color_struct &entry : color_array ) {
        if( entry.color == color ) {
            debugmsg( "Couldn't find color %d", color.operator int() );
            return entry.col_id;
        }
    }

    debugmsg( "Couldn't find color %d", color.operator int() );
    return def_c_unset;
}

nc_color color_manager::get( const color_id id ) const
{
    if( id >= num_colors ) {
        debugmsg( "Invalid color index: %d. Color array size: %zd", id, color_array.size() );
        return nc_color();
    }

    const auto &entry = color_array[id];

    return entry.custom.to_int() > 0 ? entry.custom : entry.color;
}

std::string color_manager::get_name( const nc_color &color ) const
{
    color_id id = color_to_id( color );
    return id_to_name( id );
}

nc_color color_manager::get_invert( const nc_color &color ) const
{
    const color_id id = color_to_id( color );
    const color_struct &entry = color_array[id];

    return entry.invert_custom.to_int() > 0 ? entry.invert_custom : entry.invert;
}

nc_color color_manager::get_random() const
{
    return random_entry( color_array ).color;
}

void color_manager::add_color( const color_id col, const std::string &name,
                               const nc_color &color_pair, const color_id inv_id )
{
    color_struct st = {color_pair, nc_color(), nc_color(), nc_color(), {{nc_color(), nc_color(), nc_color(), nc_color(), nc_color(), nc_color(), nc_color()}}, col, inv_id, name, "", "" };
    color_array[col] = st;
    inverted_map[color_pair] = col;
    name_map[name] = col;
}

nc_color color_manager::get_highlight( const nc_color &color, const hl_enum bg ) const
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
    //            Base Name      Highlight      Red BG              White BG            Green BG            Yellow BG
    add_highlight("c_black",     "h_black",     "",                 "c_black_white",    "c_black_green",    "c_black_yellow",   "c_black_magenta",      "c_black_cyan");
    add_highlight("c_white",     "h_white",     "c_white_red",      "c_white_white",    "c_white_green",    "c_white_yellow",   "c_white_magenta",      "c_white_cyan");
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

    // Allow real dark gray for terminals that support it
    if( catacurses::supports_256_colors() ) {
        add_color( def_c_dark_gray, "c_dark_gray", color_pair( 72 ), def_i_dark_gray );
        add_color( def_h_dark_gray, "h_dark_gray", color_pair( 75 ), def_c_light_blue );
        add_color( def_i_dark_gray, "i_dark_gray", color_pair( 79 ).blink(), def_c_dark_gray );
        add_color( def_c_dark_gray_red, "c_dark_gray_red", color_pair( 73 ), def_c_dark_gray_red );
        add_color( def_c_dark_gray_white, "c_dark_gray_white", color_pair( 79 ), def_c_white );
        add_color( def_c_dark_gray_green, "c_dark_gray_green", color_pair( 74 ), def_c_light_green );
        add_color( def_c_dark_gray_yellow, "c_dark_gray_yellow", color_pair( 78 ), def_c_yellow );
        add_color( def_c_dark_gray_magenta, "c_dark_gray_magenta", color_pair( 77 ), def_c_pink );
        add_color( def_c_dark_gray_cyan, "c_dark_gray_cyan", color_pair( 76 ), def_c_light_cyan );
#if !(defined(TILES) || defined(WIN32))
        imclient->set_alloced_pair_count( 79 );
#endif
    } else {
        add_color( def_c_dark_gray, "c_dark_gray", color_pair( 30 ).bold(), def_i_dark_gray );
        add_color( def_h_dark_gray, "h_dark_gray", color_pair( 20 ).bold(), def_c_light_blue );
        add_color( def_i_dark_gray, "i_dark_gray", color_pair( 32 ).blink(), def_c_dark_gray );
        add_color( def_c_dark_gray_red, "c_dark_gray_red", color_pair( 9 ).bold(), def_c_dark_gray_red );
        add_color( def_c_dark_gray_white, "c_dark_gray_white", color_pair( 32 ).bold(), def_c_white );
        add_color( def_c_dark_gray_green, "c_dark_gray_green", color_pair( 40 ).bold(), def_c_light_green );
        add_color( def_c_dark_gray_yellow, "c_dark_gray_yellow", color_pair( 48 ).bold(), def_c_yellow );
        add_color( def_c_dark_gray_magenta, "c_dark_gray_magenta", color_pair( 56 ).bold(), def_c_pink );
        add_color( def_c_dark_gray_cyan, "c_dark_gray_cyan", color_pair( 64 ).bold(), def_c_light_cyan );
#if !(defined(TILES) || defined(WIN32))
        imclient->set_alloced_pair_count( 71 );
#endif
    }
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

    init_pair( 72, dark_gray,  black );
    init_pair( 73, dark_gray,  red );
    init_pair( 74, dark_gray,  green );
    init_pair( 75, dark_gray,  blue );
    init_pair( 76, dark_gray,  cyan );
    init_pair( 77, dark_gray,  magenta );
    init_pair( 78, dark_gray,  yellow );
    init_pair( 79, dark_gray,  white );
    init_pair( 80, black,      dark_gray );
    init_pair( 81, red,        dark_gray );
    init_pair( 82, green,      dark_gray );
    init_pair( 83, blue,       dark_gray );
    init_pair( 84, cyan,       dark_gray );
    init_pair( 85, magenta,    dark_gray );
    init_pair( 86, yellow,     dark_gray );
    init_pair( 87, white,      dark_gray );

    all_colors.load_default();
    all_colors.load_custom( {} );

    // The short color codes (e.g. "br") are intentionally untranslatable.
    color_by_string_map = {
        {"br", {c_brown, to_translation( "brown" )}}, {"lg", {c_light_gray, to_translation( "light gray" )}},
        {"dg", {c_dark_gray, to_translation( "dark gray" )}}, {"r", {c_light_red, to_translation( "light red" )}},
        {"R", {c_red, to_translation( "red" )}}, {"g", {c_light_green, to_translation( "light green" )}},
        {"G", {c_green, to_translation( "green" )}}, {"b", {c_light_blue, to_translation( "light blue" )}},
        {"B", {c_blue, to_translation( "blue" )}}, {"W", {c_white, to_translation( "white" )}},
        {"C", {c_cyan, to_translation( "cyan" )}}, {"c", {c_light_cyan, to_translation( "light cyan" )}},
        {"P", {c_pink, to_translation( "pink" )}}, {"m", {c_magenta, to_translation( "magenta" )}}
    };
}

nc_color invert_color( const nc_color &c )
{
    const nc_color color = all_colors.get_invert( c );
    return static_cast<int>( color ) > 0 ? color : c_pink;
}

nc_color hilite( const nc_color &c )
{
    const nc_color color = all_colors.get_highlight( c, HL_BLUE );
    return static_cast<int>( color ) > 0 ? color : h_white;
}

nc_color red_background( const nc_color &c )
{
    const nc_color color = all_colors.get_highlight( c, HL_RED );
    return static_cast<int>( color ) > 0 ? color : c_white_red;
}

nc_color white_background( const nc_color &c )
{
    const nc_color color = all_colors.get_highlight( c, HL_WHITE );
    return static_cast<int>( color ) > 0 ? color : c_black_white;
}

nc_color green_background( const nc_color &c )
{
    const nc_color color = all_colors.get_highlight( c, HL_GREEN );
    return static_cast<int>( color ) > 0 ? color : c_black_green;
}

nc_color yellow_background( const nc_color &c )
{
    const nc_color color = all_colors.get_highlight( c, HL_YELLOW );
    return static_cast<int>( color ) > 0 ? color : c_black_yellow;
}

nc_color magenta_background( const nc_color &c )
{
    const nc_color color = all_colors.get_highlight( c, HL_MAGENTA );
    return static_cast<int>( color ) > 0 ? color : c_black_magenta;
}

nc_color cyan_background( const nc_color &c )
{
    const nc_color color = all_colors.get_highlight( c, HL_CYAN );
    return static_cast<int>( color ) > 0 ? color : c_black_cyan;
}

std::string hilite_string( const std::string &text )
{
    std::string highlighted = text;
    size_t pos = 0;
    size_t tag_length = 0;
    int color_tag_count = 0;
    while( ( pos = highlighted.find( "<color_", pos ) ) != std::string::npos ) {
        tag_length = highlighted.find( '>', pos ) - pos + 1;
        if( tag_length <= 0 ) {
            debugmsg( "Tag length calculated incorrectly.  Unable to highlight text %s", text );
            return text;
        }
        std::string tag = highlighted.substr( pos, tag_length );
        color_tag_parse_result old_color = get_color_from_tag( tag );
        if( old_color.type != color_tag_parse_result::open_color_tag ) {
            debugmsg( "Unable to highlight text %s, parsing color tag %s failed", text, tag );
            return text;
        }
        nc_color new_color = hilite( old_color.color );
        std::string new_tag = get_tag_from_color( new_color );
        highlighted.replace( pos, tag_length, new_tag );
        pos += new_tag.length();
        ++color_tag_count;
    }
    if( color_tag_count < 1 ) {
        highlighted = colorize( highlighted, h_white );
    }
    return highlighted;
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
nc_color color_from_string( std::string_view color,
                            const report_color_error color_error )
{
    if( color.empty() ) {
        return c_unset;
    }
    std::string new_color( color );
    if( new_color.substr( 1, 1 ) != "_" ) { //c_  //i_  //h_
        new_color = "c_" + new_color;
    }

    const std::array<std::pair<std::string, std::string>, 2> pSearch = { {
            { "light_", "lt" }, { "dark_", "dk" }
        }
    };
    for( const auto &i : pSearch ) {
        size_t pos = 0;
        while( ( pos = new_color.find( i.second, pos ) ) != std::string::npos ) {
            new_color.replace( pos, i.second.length(), i.first );
            pos += i.first.length();
            if( color_error == report_color_error::yes ) {
                debugmsg( "Deprecated foreground color suffix was used: (%s) in (%s).  Please update mod that uses that.",
                          i.second, color );
            }
        }
    }

    const nc_color col = all_colors.name_to_color( new_color, color_error );
    if( col.to_int() > 0 ) {
        return col;
    }

    return c_unset;
}

/**
 * The reverse of color_from_string.
 */
std::string string_from_color( const nc_color &color )
{
    std::string sColor = all_colors.get_name( color );

    if( sColor != "c_unset" ) {
        return sColor;
    }

    return "c_white";
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

    const std::array<std::pair<std::string, std::string>, 2> pSearch = { {
            { "light_", "lt" }, { "dark_", "dk" }
        }
    };
    for( const auto &i : pSearch ) {
        size_t pos = 0;
        while( ( pos = new_color.find( i.second, pos ) ) != std::string::npos ) {
            new_color.replace( pos, i.second.length(), i.first );
            pos += i.first.length();
            debugmsg( "Deprecated background color suffix was used: (%s) in (%s).  Please update mod that uses that.",
                      i.second, color );
        }
    }

    const nc_color col = all_colors.name_to_color( new_color );
    if( col.to_int() > 0 ) {
        return col;
    }

    return i_white;
}

color_tag_parse_result get_color_from_tag( std::string_view s,
        const report_color_error color_error )
{
    if( s.empty() || s[0] != '<' ) {
        return { color_tag_parse_result::non_color_tag, {} };
    }
    if( s.substr( 0, 8 ) == "</color>" ) {
        return { color_tag_parse_result::close_color_tag, {} };
    }
    if( s.substr( 0, 7 ) != "<color_" ) {
        return { color_tag_parse_result::non_color_tag, {} };
    }
    size_t tag_close = s.find( '>' );
    if( tag_close == std::string::npos ) {
        return { color_tag_parse_result::non_color_tag, {} };
    }
    std::string_view color_name = s.substr( 7, tag_close - 7 );
    const nc_color color = color_from_string( color_name, color_error );
    if( color != c_unset ) {
        return { color_tag_parse_result::open_color_tag, color };
    } else {
        return { color_tag_parse_result::non_color_tag, color };
    }
}

std::string get_tag_from_color( const nc_color &color )
{
    return "<color_" + string_from_color( color ) + ">";
}

std::string colorize( const std::string &text, const nc_color &color )
{
    const std::string tag = get_tag_from_color( color );
    size_t strpos = text.find( '\n' );
    if( strpos == std::string::npos ) {
        return tag + text + "</color>";
    }

    size_t prevpos = 0;
    std::string ret;
    while( ( strpos = text.find( '\n', prevpos ) ) != std::string::npos ) {
        ret += tag + text.substr( prevpos, strpos - prevpos ) + "</color>\n";
        prevpos = strpos + 1;
    }
    ret += tag + text.substr( prevpos, text.size() - prevpos ) + "</color>";
    return ret;
}

std::string colorize( const translation &text, const nc_color &color )
{
    return colorize( text.translated(), color );
}

std::string get_note_string_from_color( const nc_color &color )
{
    for( const std::pair<const std::string, note_color> &i : color_by_string_map ) {
        if( i.second.color == color ) {
            return i.first;
        }
    }
    // The default note string.
    return "Y";
}

nc_color get_note_color( std::string_view note_id )
{
    // TODO in C++20 we can pass a string_view in directly rather than
    // constructing a string to use as the find argument
    const auto candidate_color = color_by_string_map.find( std::string( note_id ) );
    if( candidate_color != std::end( color_by_string_map ) ) {
        return candidate_color->second.color;
    }
    // The default note color.
    return c_yellow;
}

const std::unordered_map<std::string, note_color> &get_note_color_names()
{
    return color_by_string_map;
}

void color_manager::clear()
{
    name_map.clear();
    inverted_map.clear();
    for( color_manager::color_struct &entry : color_array ) {
        entry.name.clear();
        entry.name_custom.clear();
        entry.name_invert_custom.clear();
    }
}

static void draw_header( const catacurses::window &w )
{
    int tmpx = 0;
    tmpx += shortcut_print( w, point( tmpx, 0 ), c_white, c_light_green,
                            _( "<R>emove custom color" ) ) + 2;
    tmpx += shortcut_print( w, point( tmpx, 0 ), c_white, c_light_green,
                            _( "<Arrow Keys> To navigate" ) ) + 2;
    shortcut_print( w, point( tmpx, 0 ), c_white, c_light_green, _( "<Enter>-Edit" ) );
    tmpx = 0;
    tmpx += shortcut_print( w, point( tmpx, 1 ), c_white, c_light_green,
                            _( "Load a <C>olor theme" ) ) + 2;
    shortcut_print( w, point( tmpx, 1 ), c_white, c_light_green, _( "Load <T>emplate" ) );

    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 0, 2 ), c_white, _( "Some color changes may require a restart." ) );

    wattron( w, BORDER_COLOR );
    mvwhline( w, point( 0, 3 ), LINE_OXOX, getmaxx( w ) ); // Draw line under header
    mvwaddch( w, point( 48, 3 ), LINE_OXXX ); //^|^
    wattroff( w, BORDER_COLOR );

    mvwprintz( w, point( 3, 4 ), c_white, _( "Colorname" ) );
    mvwprintz( w, point( 21, 4 ), c_white, _( "Normal" ) );
    mvwprintz( w, point( 52, 4 ), c_white, _( "Invert" ) );

    wnoutrefresh( w );
}

void color_manager::show_gui()
{
    const int iHeaderHeight = 4;
    int iContentHeight = 0;

    point iOffset;

    std::vector<int> vLines;
    vLines.reserve( 2 );
    vLines.push_back( -1 );
    vLines.push_back( 48 );

    const int iTotalCols = vLines.size();

    catacurses::window w_colors_border;
    catacurses::window w_colors_header;
    catacurses::window w_colors;

    const auto calc_offset_y = []() -> int {
        return TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;
    };

    ui_adaptor ui;
    const auto init_windows = [&]( ui_adaptor & ui ) {
        iContentHeight = FULL_SCREEN_HEIGHT - 2 - iHeaderHeight;

        iOffset.x = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;
        iOffset.y = calc_offset_y();

        w_colors_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                              iOffset );
        w_colors_header = catacurses::newwin( iHeaderHeight, FULL_SCREEN_WIDTH - 2,
                                              iOffset + point::south_east );
        w_colors = catacurses::newwin( iContentHeight, FULL_SCREEN_WIDTH - 2,
                                       iOffset + point( 1, iHeaderHeight + 1 ) );

        ui.position_from_window( w_colors_border );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    int iCurrentLine = 0;
    int iCurrentCol = 1;
    int iStartPos = 0;
    const int iMaxColors = color_array.size();
    bool bStuffChanged = false;
    input_context ctxt( "COLORS" );
    ctxt.register_navigate_ui_list();
    ctxt.register_leftright();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "REMOVE_CUSTOM" );
    ctxt.register_action( "LOAD_TEMPLATE" );
    ctxt.register_action( "LOAD_BASE_COLORS" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    std::map<std::string, color_struct> name_color_map;

    for( const auto &pr : name_map ) {
        name_color_map[pr.first] = color_array[pr.second];
    }

    ui.on_redraw( [&]( const ui_adaptor & ) {
        draw_border( w_colors_border, BORDER_COLOR, _( "Color manager" ) );
        wattron( w_colors_border, BORDER_COLOR );
        mvwaddch( w_colors_border, point( 0, 4 ), LINE_XXXO ); // |-
        mvwaddch( w_colors_border, point( getmaxx( w_colors_border ) - 1, 4 ), LINE_XOXX ); // -|

        for( const int &iCol : vLines ) {
            if( iCol > -1 ) {
                mvwaddch( w_colors_border, point( iCol + 1, FULL_SCREEN_HEIGHT - 1 ), LINE_XXOX ); // _|_
                mvwaddch( w_colors_header, point( iCol, 4 ), LINE_XOXO );
            }
        }
        wattroff( w_colors_border, BORDER_COLOR );
        wnoutrefresh( w_colors_border );

        draw_header( w_colors_header );

        // Clear all lines
        mvwrectf( w_colors, point::zero, c_black, ' ', 79, iContentHeight );
        for( int &iCol : vLines ) {
            mvwvline( w_colors, point( iCol, 0 ), BORDER_COLOR, LINE_XOXO, iContentHeight );
        }

        calcStartPos( iStartPos, iCurrentLine, iContentHeight, iMaxColors );

        draw_scrollbar( w_colors_border, iCurrentLine, iContentHeight, iMaxColors, point( 0, 5 ) );
        wnoutrefresh( w_colors_border );

        auto iter = name_color_map.begin();
        std::advance( iter, iStartPos );

        // display color manager
        for( int i = iStartPos; iter != name_color_map.end(); ++iter, ++i ) {
            if( i >= iStartPos &&
                i < iStartPos + std::min( iContentHeight, iMaxColors ) ) {
                color_manager::color_struct &entry = iter->second;

                if( iCurrentLine == i ) {
                    mvwprintz( w_colors, point( vLines[iCurrentCol - 1] + 2, i - iStartPos ), c_yellow, ">" );
                }

                mvwprintz( w_colors, point( 3, i - iStartPos ), c_white, iter->first ); //color name
                mvwprintz( w_colors, point( 21, i - iStartPos ), entry.color, _( "default" ) ); //default color

                if( !entry.name_custom.empty() ) {
                    mvwprintz( w_colors, point( 30, i - iStartPos ), name_color_map[entry.name_custom].color,
                               entry.name_custom ); //custom color
                }

                mvwprintz( w_colors, point( 52, i - iStartPos ), entry.invert,
                           _( "default" ) ); //invert default color

                if( !entry.name_invert_custom.empty() ) {
                    mvwprintz( w_colors, point( 61, i - iStartPos ), name_color_map[entry.name_invert_custom].color,
                               entry.name_invert_custom ); //invert custom color
                }
            }
        }

        wnoutrefresh( w_colors );
    } );

    while( true ) {
        ui_manager::redraw();

        const std::string action = ctxt.handle_input();
        const int recmax = iMaxColors;
        const int scroll_rate = recmax > 20 ? 10 : 3;
        if( action == "QUIT" ) {
            break;
        } else if( navigate_ui_list( action, iCurrentLine, scroll_rate, recmax, true ) ) {
        } else if( action == "LEFT" || action == "RIGHT" ) {
            iCurrentCol = inc_clamp_wrap( iCurrentCol, 1, iTotalCols );
        } else if( action == "REMOVE_CUSTOM" ) {
            color_manager::color_struct &entry = std::next( name_color_map.begin(), iCurrentLine )->second;

            if( iCurrentCol == 1 && !entry.name_custom.empty() ) {
                bStuffChanged = true;
                entry.name_custom.clear();

            } else if( iCurrentCol == 2 && !entry.name_invert_custom.empty() ) {
                bStuffChanged = true;
                entry.name_invert_custom.clear();

            }

            finalize(); // Need to recalculate caches

        } else if( action == "LOAD_TEMPLATE" ) {
            auto vFiles = get_files_from_path( ".json", PATH_INFO::color_templates(), false, true );

            if( !vFiles.empty() ) {
                uilist ui_templates;
                ui_templates.text = _( "Color templates:" );

                for( const cata_path &file : vFiles ) {
                    ui_templates.addentry( file.get_relative_path().filename().generic_u8string() );
                }

                ui_templates.query();

                if( ui_templates.ret >= 0 && static_cast<size_t>( ui_templates.ret ) < vFiles.size() ) {
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

        } else if( action == "LOAD_BASE_COLORS" ) {
            auto vFiles = get_files_from_path( ".json", PATH_INFO::color_themes(), false, true );

            if( !vFiles.empty() ) {
                uilist ui_templates;
                ui_templates.text = _( "Color themes:" );

                for( const cata_path &filename : vFiles ) {
                    ui_templates.addentry( filename.get_relative_path().filename().generic_u8string() );
                }

                ui_templates.query();

                if( ui_templates.ret >= 0 && static_cast<size_t>( ui_templates.ret ) < vFiles.size() ) {
                    copy_file( vFiles[ui_templates.ret], PATH_INFO::base_colors() );
                }
            }
        } else if( action == "CONFIRM" ) {
            uilist ui_colors;

            const color_manager::color_struct &entry = std::next( name_color_map.begin(),
                    iCurrentLine )->second;

            std::string sColorType = _( "Normal" );
            std::string sSelected = entry.name_custom;

            if( iCurrentCol == 2 ) {
                sColorType = _( "Invert" );
                sSelected = entry.name_invert_custom;

            }

            ui_colors.text = string_format( _( "Custom %s color:" ), sColorType );

            int i = 0;
            for( auto &iter : name_color_map ) {
                std::string sColor = iter.first;
                std::string sType = _( "default" );

                std::string name_custom;

                if( sSelected == sColor ) {
                    ui_colors.selected = i;
                }

                if( !iter.second.name_custom.empty() ) {
                    name_custom = " <color_" + iter.second.name_custom + ">" + iter.second.name_custom + "</color>";
                }

                ui_colors.addentry( string_format( "%-17s <color_%s>%s</color>%s", iter.first,
                                                   sColor, sType, name_custom ) );

                i++;
            }

            ui_colors.query();

            if( ui_colors.ret >= 0 && static_cast<size_t>( ui_colors.ret ) < name_color_map.size() ) {
                bStuffChanged = true;

                auto iter = name_color_map.begin();
                std::advance( iter, ui_colors.ret );

                color_manager::color_struct &entry = std::next( name_color_map.begin(), iCurrentLine )->second;

                if( iCurrentCol == 1 ) {
                    entry.name_custom = iter->first;

                } else if( iCurrentCol == 2 ) {
                    entry.name_invert_custom = iter->first;

                }
            }

            finalize(); // Need to recalculate caches
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
        load_custom( {} );
    }
}

bool color_manager::save_custom() const
{
    const cata_path savefile = PATH_INFO::custom_colors();

    return write_to_file( savefile.generic_u8string(), [&](
    std::ostream & fout ) {
        JsonOut jsout( fout );
        serialize( jsout );
    }, _( "custom colors" ) );
}

void color_manager::load_custom( const cata_path &sPath )
{
    const cata_path file = sPath.empty() ? PATH_INFO::custom_colors() : sPath;

    read_from_file_optional_json( file, [this]( const JsonArray & jsonin ) {
        deserialize( jsonin );
    } );
    finalize(); // Need to finalize regardless of success
}

void color_manager::serialize( JsonOut &json ) const
{
    json.start_array();
    for( const color_struct &entry : color_array ) {
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

void color_manager::deserialize( const JsonArray &ja )
{
    for( JsonObject joColors : ja ) {
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
