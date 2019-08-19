#pragma once
#ifndef COLOR_H
#define COLOR_H

#include <array>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>
#include <iosfwd>

class nc_color;

#define all_colors get_all_colors()

#define c_black all_colors.get(def_c_black)
#define c_white all_colors.get(def_c_white)
#define c_light_gray all_colors.get(def_c_light_gray)
#define c_dark_gray all_colors.get(def_c_dark_gray)
#define c_red all_colors.get(def_c_red)
#define c_green all_colors.get(def_c_green)
#define c_blue all_colors.get(def_c_blue)
#define c_cyan all_colors.get(def_c_cyan)
#define c_magenta all_colors.get(def_c_magenta)
#define c_brown all_colors.get(def_c_brown)
#define c_light_red all_colors.get(def_c_light_red)
#define c_light_green all_colors.get(def_c_light_green)
#define c_light_blue all_colors.get(def_c_light_blue)
#define c_light_cyan all_colors.get(def_c_light_cyan)
#define c_pink all_colors.get(def_c_pink)
#define c_yellow all_colors.get(def_c_yellow)

#define h_black all_colors.get(def_h_black)
#define h_white all_colors.get(def_h_white)
#define h_light_gray all_colors.get(def_h_light_gray)
#define h_dark_gray all_colors.get(def_h_dark_gray)
#define h_red all_colors.get(def_h_red)
#define h_green all_colors.get(def_h_green)
#define h_blue all_colors.get(def_h_blue)
#define h_cyan all_colors.get(def_h_cyan)
#define h_magenta all_colors.get(def_h_magenta)
#define h_brown all_colors.get(def_h_brown)
#define h_light_red all_colors.get(def_h_light_red)
#define h_light_green all_colors.get(def_h_light_green)
#define h_light_blue all_colors.get(def_h_light_blue)
#define h_light_cyan all_colors.get(def_h_light_cyan)
#define h_pink all_colors.get(def_h_pink)
#define h_yellow all_colors.get(def_h_yellow)

#define i_black all_colors.get(def_i_black)
#define i_white all_colors.get(def_i_white)
#define i_light_gray all_colors.get(def_i_light_gray)
#define i_dark_gray all_colors.get(def_i_dark_gray)
#define i_red all_colors.get(def_i_red)
#define i_green all_colors.get(def_i_green)
#define i_blue all_colors.get(def_i_blue)
#define i_cyan all_colors.get(def_i_cyan)
#define i_magenta all_colors.get(def_i_magenta)
#define i_brown all_colors.get(def_i_brown)
#define i_light_red all_colors.get(def_i_light_red)
#define i_light_green all_colors.get(def_i_light_green)
#define i_light_blue all_colors.get(def_i_light_blue)
#define i_light_cyan all_colors.get(def_i_light_cyan)
#define i_pink all_colors.get(def_i_pink)
#define i_yellow all_colors.get(def_i_yellow)

#define c_unset all_colors.get(def_c_unset)

#define c_white_red all_colors.get(def_c_white_red)
#define c_light_gray_red all_colors.get(def_c_light_gray_red)
#define c_dark_gray_red all_colors.get(def_c_dark_gray_red)
#define c_red_red all_colors.get(def_c_red_red)
#define c_green_red all_colors.get(def_c_green_red)
#define c_blue_red all_colors.get(def_c_blue_red)
#define c_cyan_red all_colors.get(def_c_cyan_red)
#define c_magenta_red all_colors.get(def_c_magenta_red)
#define c_brown_red all_colors.get(def_c_brown_red)
#define c_light_red_red all_colors.get(def_c_light_red_red)
#define c_light_green_red all_colors.get(def_c_light_green_red)
#define c_light_blue_red all_colors.get(def_c_light_blue_red)
#define c_light_cyan_red all_colors.get(def_c_light_cyan_red)
#define c_pink_red all_colors.get(def_c_pink_red)
#define c_yellow_red all_colors.get(def_c_yellow_red)

#define c_black_white all_colors.get(def_c_black_white)
#define c_dark_gray_white all_colors.get(def_c_dark_gray_white)
#define c_light_gray_white all_colors.get(def_c_light_gray_white)
#define c_white_white all_colors.get(def_c_white_white)
#define c_red_white all_colors.get(def_c_red_white)
#define c_light_red_white all_colors.get(def_c_light_red_white)
#define c_green_white all_colors.get(def_c_green_white)
#define c_light_green_white all_colors.get(def_c_light_green_white)
#define c_brown_white all_colors.get(def_c_brown_white)
#define c_yellow_white all_colors.get(def_c_yellow_white)
#define c_blue_white all_colors.get(def_c_blue_white)
#define c_light_blue_white all_colors.get(def_c_light_blue_white)
#define c_magenta_white all_colors.get(def_c_magenta_white)
#define c_pink_white all_colors.get(def_c_pink_white)
#define c_cyan_white all_colors.get(def_c_cyan_white)
#define c_light_cyan_white all_colors.get(def_c_light_cyan_white)

#define c_black_green all_colors.get(def_c_black_green)
#define c_dark_gray_green all_colors.get(def_c_dark_gray_green)
#define c_light_gray_green all_colors.get(def_c_light_gray_green)
#define c_white_green all_colors.get(def_c_white_green)
#define c_red_green all_colors.get(def_c_red_green)
#define c_light_red_green all_colors.get(def_c_light_red_green)
#define c_green_green all_colors.get(def_c_green_green)
#define c_light_green_green all_colors.get(def_c_light_green_green)
#define c_brown_green all_colors.get(def_c_brown_green)
#define c_yellow_green all_colors.get(def_c_yellow_green)
#define c_blue_green all_colors.get(def_c_blue_green)
#define c_light_blue_green all_colors.get(def_c_light_blue_green)
#define c_magenta_green all_colors.get(def_c_magenta_green)
#define c_pink_green all_colors.get(def_c_pink_green)
#define c_cyan_green all_colors.get(def_c_cyan_green)
#define c_light_cyan_green all_colors.get(def_c_light_cyan_green)

#define c_black_yellow all_colors.get(def_c_black_yellow)
#define c_dark_gray_yellow all_colors.get(def_c_dark_gray_yellow)
#define c_light_gray_yellow all_colors.get(def_c_light_gray_yellow)
#define c_white_yellow all_colors.get(def_c_white_yellow)
#define c_red_yellow all_colors.get(def_c_red_yellow)
#define c_light_red_yellow all_colors.get(def_c_light_red_yellow)
#define c_green_yellow all_colors.get(def_c_green_yellow)
#define c_light_green_yellow all_colors.get(def_c_light_green_yellow)
#define c_brown_yellow all_colors.get(def_c_brown_yellow)
#define c_yellow_yellow all_colors.get(def_c_yellow_yellow)
#define c_blue_yellow all_colors.get(def_c_blue_yellow)
#define c_light_blue_yellow all_colors.get(def_c_light_blue_yellow)
#define c_magenta_yellow all_colors.get(def_c_magenta_yellow)
#define c_pink_yellow all_colors.get(def_c_pink_yellow)
#define c_cyan_yellow all_colors.get(def_c_cyan_yellow)
#define c_light_cyan_yellow all_colors.get(def_c_light_cyan_yellow)

#define c_black_magenta all_colors.get(def_c_black_magenta)
#define c_dark_gray_magenta all_colors.get(def_c_dark_gray_magenta)
#define c_light_gray_magenta all_colors.get(def_c_light_gray_magenta)
#define c_white_magenta all_colors.get(def_c_white_magenta)
#define c_red_magenta all_colors.get(def_c_red_magenta)
#define c_light_red_magenta all_colors.get(def_c_light_red_magenta)
#define c_green_magenta all_colors.get(def_c_green_magenta)
#define c_light_green_magenta all_colors.get(def_c_light_green_magenta)
#define c_brown_magenta all_colors.get(def_c_brown_magenta)
#define c_yellow_magenta all_colors.get(def_c_yellow_magenta)
#define c_blue_magenta all_colors.get(def_c_blue_magenta)
#define c_light_blue_magenta all_colors.get(def_c_light_blue_magenta)
#define c_magenta_magenta all_colors.get(def_c_magenta_magenta)
#define c_pink_magenta all_colors.get(def_c_pink_magenta)
#define c_cyan_magenta all_colors.get(def_c_cyan_magenta)
#define c_light_cyan_magenta all_colors.get(def_c_light_cyan_magenta)

#define c_black_cyan all_colors.get(def_c_black_cyan)
#define c_dark_gray_cyan all_colors.get(def_c_dark_gray_cyan)
#define c_light_gray_cyan all_colors.get(def_c_light_gray_cyan)
#define c_white_cyan all_colors.get(def_c_white_cyan)
#define c_red_cyan all_colors.get(def_c_red_cyan)
#define c_light_red_cyan all_colors.get(def_c_light_red_cyan)
#define c_green_cyan all_colors.get(def_c_green_cyan)
#define c_light_green_cyan all_colors.get(def_c_light_green_cyan)
#define c_brown_cyan all_colors.get(def_c_brown_cyan)
#define c_yellow_cyan all_colors.get(def_c_yellow_cyan)
#define c_blue_cyan all_colors.get(def_c_blue_cyan)
#define c_light_blue_cyan all_colors.get(def_c_light_blue_cyan)
#define c_magenta_cyan all_colors.get(def_c_magenta_cyan)
#define c_pink_cyan all_colors.get(def_c_pink_cyan)
#define c_cyan_cyan all_colors.get(def_c_cyan_cyan)
#define c_light_cyan_cyan all_colors.get(def_c_light_cyan_cyan)

// def_x is a color that maps to x with default settings
enum color_id {
    def_c_black = 0,
    def_c_white,
    def_c_light_gray,
    def_c_dark_gray,
    def_c_red,
    def_c_green,
    def_c_blue,
    def_c_cyan,
    def_c_magenta,
    def_c_brown,
    def_c_light_red,
    def_c_light_green,
    def_c_light_blue,
    def_c_light_cyan,
    def_c_pink,
    def_c_yellow,

    def_h_black,
    def_h_white,
    def_h_light_gray,
    def_h_dark_gray,
    def_h_red,
    def_h_green,
    def_h_blue,
    def_h_cyan,
    def_h_magenta,
    def_h_brown,
    def_h_light_red,
    def_h_light_green,
    def_h_light_blue,
    def_h_light_cyan,
    def_h_pink,
    def_h_yellow,

    def_i_black,
    def_i_white,
    def_i_light_gray,
    def_i_dark_gray,
    def_i_red,
    def_i_green,
    def_i_blue,
    def_i_cyan,
    def_i_magenta,
    def_i_brown,
    def_i_light_red,
    def_i_light_green,
    def_i_light_blue,
    def_i_light_cyan,
    def_i_pink,
    def_i_yellow,

    def_c_unset,

    def_c_black_red,
    def_c_white_red,
    def_c_light_gray_red,
    def_c_dark_gray_red,
    def_c_red_red,
    def_c_green_red,
    def_c_blue_red,
    def_c_cyan_red,
    def_c_magenta_red,
    def_c_brown_red,
    def_c_light_red_red,
    def_c_light_green_red,
    def_c_light_blue_red,
    def_c_light_cyan_red,
    def_c_pink_red,
    def_c_yellow_red,

    def_c_black_white,
    def_c_dark_gray_white,
    def_c_light_gray_white,
    def_c_white_white,
    def_c_red_white,
    def_c_light_red_white,
    def_c_green_white,
    def_c_light_green_white,
    def_c_brown_white,
    def_c_yellow_white,
    def_c_blue_white,
    def_c_light_blue_white,
    def_c_magenta_white,
    def_c_pink_white,
    def_c_cyan_white,
    def_c_light_cyan_white,

    def_c_black_green,
    def_c_dark_gray_green,
    def_c_light_gray_green,
    def_c_white_green,
    def_c_red_green,
    def_c_light_red_green,
    def_c_green_green,
    def_c_light_green_green,
    def_c_brown_green,
    def_c_yellow_green,
    def_c_blue_green,
    def_c_light_blue_green,
    def_c_magenta_green,
    def_c_pink_green,
    def_c_cyan_green,
    def_c_light_cyan_green,

    def_c_black_yellow,
    def_c_dark_gray_yellow,
    def_c_light_gray_yellow,
    def_c_white_yellow,
    def_c_red_yellow,
    def_c_light_red_yellow,
    def_c_green_yellow,
    def_c_light_green_yellow,
    def_c_brown_yellow,
    def_c_yellow_yellow,
    def_c_blue_yellow,
    def_c_light_blue_yellow,
    def_c_magenta_yellow,
    def_c_pink_yellow,
    def_c_cyan_yellow,
    def_c_light_cyan_yellow,

    def_c_black_magenta,
    def_c_dark_gray_magenta,
    def_c_light_gray_magenta,
    def_c_white_magenta,
    def_c_red_magenta,
    def_c_light_red_magenta,
    def_c_green_magenta,
    def_c_light_green_magenta,
    def_c_brown_magenta,
    def_c_yellow_magenta,
    def_c_blue_magenta,
    def_c_light_blue_magenta,
    def_c_magenta_magenta,
    def_c_pink_magenta,
    def_c_cyan_magenta,
    def_c_light_cyan_magenta,

    def_c_black_cyan,
    def_c_dark_gray_cyan,
    def_c_light_gray_cyan,
    def_c_white_cyan,
    def_c_red_cyan,
    def_c_light_red_cyan,
    def_c_green_cyan,
    def_c_light_green_cyan,
    def_c_brown_cyan,
    def_c_yellow_cyan,
    def_c_blue_cyan,
    def_c_light_blue_cyan,
    def_c_magenta_cyan,
    def_c_pink_cyan,
    def_c_cyan_cyan,
    def_c_light_cyan_cyan,

    num_colors
};

class JsonOut;
class JsonIn;

void init_colors();

// Index for highlight cache
enum hl_enum {
    HL_BLUE = 0,
    HL_RED,
    HL_WHITE,
    HL_GREEN,
    HL_YELLOW,
    HL_MAGENTA,
    HL_CYAN,
    NUM_HL
};

class nc_color
{
    private:
        // color is actually an ncurses attribute.
        int attribute_value;

        nc_color( const int a ) : attribute_value( a ) { }

    public:
        nc_color() : attribute_value( 0 ) { }

        // Most of the functions here are implemented in ncurses_def.cpp
        // (for ncurses builds) *and* in cursesport.cpp (for other builds).

        static nc_color from_color_pair_index( int index );
        int to_color_pair_index() const;

        operator int() const {
            return attribute_value;
        }

        // Returns this attribute plus A_BOLD.
        nc_color bold() const;
        bool is_bold() const;
        // Returns this attribute plus A_BLINK.
        nc_color blink() const;
        bool is_blink() const;

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );
};

// Support hashing of nc_color by forwarding the hash of the contained int.
namespace std
{
template<>
struct hash<nc_color> {
    std::size_t operator()( const nc_color &v ) const {
        return hash<int>()( v.operator int() );
    }
};
} // namespace std

class color_manager
{
    private:
        void add_color( color_id col, const std::string &name,
                        const nc_color &color_pair, color_id inv_id );
        void clear();
        void finalize(); // Caches colors properly

        struct color_struct {
            nc_color color; // Default color
            nc_color invert; // Inverted color (not set until finalization)
            nc_color custom; // Custom color if > 0 (not set until finalization)
            nc_color invert_custom; // Custom inverted color if > 0 (not set until finalization)
            std::array<nc_color, NUM_HL> highlight; // Cached highlights (not set until finalization)

            color_id col_id; // Index of this color
            color_id invert_id; // Index of inversion of this color
            std::string name;
            // String names for custom colors
            std::string name_custom;
            std::string name_invert_custom;
        };

        std::array<color_struct, num_colors> color_array;
        std::unordered_map<nc_color, color_id> inverted_map;
        std::unordered_map<std::string, color_id> name_map;

        bool save_custom();

    public:
        color_manager() = default;

        nc_color get( color_id id ) const;

        nc_color get_invert( const nc_color &color ) const;
        nc_color get_highlight( const nc_color &color, hl_enum bg ) const;
        nc_color get_random() const;

        color_id color_to_id( const nc_color &color ) const;
        color_id name_to_id( const std::string &name ) const;

        std::string get_name( const nc_color &color ) const;
        std::string id_to_name( color_id id ) const;

        nc_color name_to_color( const std::string &name ) const;

        nc_color highlight_from_names( const std::string &name, const std::string &bg_name ) const;

        void load_default();
        void load_custom( const std::string &sPath = "" );

        void show_gui();

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
};

color_manager &get_all_colors();

/**
 * For color values that are created *before* the color definitions are loaded
 * from JSON. One can't use the macros (e.g. c_white) directly as they query
 * the color_manager, which may not be initialized. Instead one has to use
 * the color_id (e.g. def_c_white) and translate the id to an actual color
 * later. This is done by this class: it stores the id and translates it
 * when needed to the color value.
 */
class deferred_color
{
    private:
        color_id id;
    public:
        deferred_color( const color_id id ) : id( id ) { }
        operator nc_color() const {
            return all_colors.get( id );
        }
};

struct note_color {
    nc_color color;
    std::string name;
};

struct color_tag_parse_result {
    enum tag_type {
        open_color_tag,
        close_color_tag,
        non_color_tag,
    };
    tag_type type;
    nc_color color;
};

extern std::unordered_map<std::string, note_color> color_by_string_map;

nc_color hilite( const nc_color &c );
nc_color invert_color( const nc_color &c );
nc_color red_background( const nc_color &c );
nc_color white_background( const nc_color &c );
nc_color green_background( const nc_color &c );
nc_color yellow_background( const nc_color &c );
nc_color magenta_background( const nc_color &c );
nc_color cyan_background( const nc_color &c );

nc_color color_from_string( const std::string &color );
std::string string_from_color( const nc_color &color );
nc_color bgcolor_from_string( const std::string &color );
color_tag_parse_result get_color_from_tag( const std::string &s );
std::string get_tag_from_color( const nc_color &color );
std::string colorize( const std::string &text, const nc_color &color );

std::string get_note_string_from_color( const nc_color &color );
nc_color get_note_color( const std::string &note_id );
std::list<std::pair<std::string, std::string>> get_note_color_names();

#endif
