#ifndef COLOR_H
#define COLOR_H

#include "cursesdef.h"
#include "json.h"
#include <string>
#include <list>
#include <unordered_map>

#define all_colors get_all_colors()

#define c_black all_colors.get(def_c_black)
#define c_white all_colors.get(def_c_white)
#define c_ltgray all_colors.get(def_c_ltgray)
#define c_dkgray all_colors.get(def_c_dkgray)
#define c_red all_colors.get(def_c_red)
#define c_green all_colors.get(def_c_green)
#define c_blue all_colors.get(def_c_blue)
#define c_cyan all_colors.get(def_c_cyan)
#define c_magenta all_colors.get(def_c_magenta)
#define c_brown all_colors.get(def_c_brown)
#define c_ltred all_colors.get(def_c_ltred)
#define c_ltgreen all_colors.get(def_c_ltgreen)
#define c_ltblue all_colors.get(def_c_ltblue)
#define c_ltcyan all_colors.get(def_c_ltcyan)
#define c_pink all_colors.get(def_c_pink)
#define c_yellow all_colors.get(def_c_yellow)

#define h_black all_colors.get(def_h_black)
#define h_white all_colors.get(def_h_white)
#define h_ltgray all_colors.get(def_h_ltgray)
#define h_dkgray all_colors.get(def_h_dkgray)
#define h_red all_colors.get(def_h_red)
#define h_green all_colors.get(def_h_green)
#define h_blue all_colors.get(def_h_blue)
#define h_cyan all_colors.get(def_h_cyan)
#define h_magenta all_colors.get(def_h_magenta)
#define h_brown all_colors.get(def_h_brown)
#define h_ltred all_colors.get(def_h_ltred)
#define h_ltgreen all_colors.get(def_h_ltgreen)
#define h_ltblue all_colors.get(def_h_ltblue)
#define h_ltcyan all_colors.get(def_h_ltcyan)
#define h_pink all_colors.get(def_h_pink)
#define h_yellow all_colors.get(def_h_yellow)

#define i_black all_colors.get(def_i_black)
#define i_white all_colors.get(def_i_white)
#define i_ltgray all_colors.get(def_i_ltgray)
#define i_dkgray all_colors.get(def_i_dkgray)
#define i_red all_colors.get(def_i_red)
#define i_green all_colors.get(def_i_green)
#define i_blue all_colors.get(def_i_blue)
#define i_cyan all_colors.get(def_i_cyan)
#define i_magenta all_colors.get(def_i_magenta)
#define i_brown all_colors.get(def_i_brown)
#define i_ltred all_colors.get(def_i_ltred)
#define i_ltgreen all_colors.get(def_i_ltgreen)
#define i_ltblue all_colors.get(def_i_ltblue)
#define i_ltcyan all_colors.get(def_i_ltcyan)
#define i_pink all_colors.get(def_i_pink)
#define i_yellow all_colors.get(def_i_yellow)

#define c_unset all_colors.get(def_c_unset)

#define c_white_red all_colors.get(def_c_white_red)
#define c_ltgray_red all_colors.get(def_c_ltgray_red)
#define c_dkgray_red all_colors.get(def_c_dkgray_red)
#define c_red_red all_colors.get(def_c_red_red)
#define c_green_red all_colors.get(def_c_green_red)
#define c_blue_red all_colors.get(def_c_blue_red)
#define c_cyan_red all_colors.get(def_c_cyan_red)
#define c_magenta_red all_colors.get(def_c_magenta_red)
#define c_brown_red all_colors.get(def_c_brown_red)
#define c_ltred_red all_colors.get(def_c_ltred_red)
#define c_ltgreen_red all_colors.get(def_c_ltgreen_red)
#define c_ltblue_red all_colors.get(def_c_ltblue_red)
#define c_ltcyan_red all_colors.get(def_c_ltcyan_red)
#define c_pink_red all_colors.get(def_c_pink_red)
#define c_yellow_red all_colors.get(def_c_yellow_red)

#define c_black_white all_colors.get(def_c_black_white)
#define c_dkgray_white all_colors.get(def_c_dkgray_white)
#define c_ltgray_white all_colors.get(def_c_ltgray_white)
#define c_white_white all_colors.get(def_c_white_white)
#define c_red_white all_colors.get(def_c_red_white)
#define c_ltred_white all_colors.get(def_c_ltred_white)
#define c_green_white all_colors.get(def_c_green_white)
#define c_ltgreen_white all_colors.get(def_c_ltgreen_white)
#define c_brown_white all_colors.get(def_c_brown_white)
#define c_yellow_white all_colors.get(def_c_yellow_white)
#define c_blue_white all_colors.get(def_c_blue_white)
#define c_ltblue_white all_colors.get(def_c_ltblue_white)
#define c_magenta_white all_colors.get(def_c_magenta_white)
#define c_pink_white all_colors.get(def_c_pink_white)
#define c_cyan_white all_colors.get(def_c_cyan_white)
#define c_ltcyan_white all_colors.get(def_c_ltcyan_white)

#define c_black_green all_colors.get(def_c_black_green)
#define c_dkgray_green all_colors.get(def_c_dkgray_green)
#define c_ltgray_green all_colors.get(def_c_ltgray_green)
#define c_white_green all_colors.get(def_c_white_green)
#define c_red_green all_colors.get(def_c_red_green)
#define c_ltred_green all_colors.get(def_c_ltred_green)
#define c_green_green all_colors.get(def_c_green_green)
#define c_ltgreen_green all_colors.get(def_c_ltgreen_green)
#define c_brown_green all_colors.get(def_c_brown_green)
#define c_yellow_green all_colors.get(def_c_yellow_green)
#define c_blue_green all_colors.get(def_c_blue_green)
#define c_ltblue_green all_colors.get(def_c_ltblue_green)
#define c_magenta_green all_colors.get(def_c_magenta_green)
#define c_pink_green all_colors.get(def_c_pink_green)
#define c_cyan_green all_colors.get(def_c_cyan_green)
#define c_ltcyan_green all_colors.get(def_c_ltcyan_green)

#define c_black_yellow all_colors.get(def_c_black_yellow)
#define c_dkgray_yellow all_colors.get(def_c_dkgray_yellow)
#define c_ltgray_yellow all_colors.get(def_c_ltgray_yellow)
#define c_white_yellow all_colors.get(def_c_white_yellow)
#define c_red_yellow all_colors.get(def_c_red_yellow)
#define c_ltred_yellow all_colors.get(def_c_ltred_yellow)
#define c_green_yellow all_colors.get(def_c_green_yellow)
#define c_ltgreen_yellow all_colors.get(def_c_ltgreen_yellow)
#define c_brown_yellow all_colors.get(def_c_brown_yellow)
#define c_yellow_yellow all_colors.get(def_c_yellow_yellow)
#define c_blue_yellow all_colors.get(def_c_blue_yellow)
#define c_ltblue_yellow all_colors.get(def_c_ltblue_yellow)
#define c_magenta_yellow all_colors.get(def_c_magenta_yellow)
#define c_pink_yellow all_colors.get(def_c_pink_yellow)
#define c_cyan_yellow all_colors.get(def_c_cyan_yellow)
#define c_ltcyan_yellow all_colors.get(def_c_ltcyan_yellow)

#define c_black_magenta all_colors.get(def_c_black_magenta)
#define c_dkgray_magenta all_colors.get(def_c_dkgray_magenta)
#define c_ltgray_magenta all_colors.get(def_c_ltgray_magenta)
#define c_white_magenta all_colors.get(def_c_white_magenta)
#define c_red_magenta all_colors.get(def_c_red_magenta)
#define c_ltred_magenta all_colors.get(def_c_ltred_magenta)
#define c_green_magenta all_colors.get(def_c_green_magenta)
#define c_ltgreen_magenta all_colors.get(def_c_ltgreen_magenta)
#define c_brown_magenta all_colors.get(def_c_brown_magenta)
#define c_yellow_magenta all_colors.get(def_c_yellow_magenta)
#define c_blue_magenta all_colors.get(def_c_blue_magenta)
#define c_ltblue_magenta all_colors.get(def_c_ltblue_magenta)
#define c_magenta_magenta all_colors.get(def_c_magenta_magenta)
#define c_pink_magenta all_colors.get(def_c_pink_magenta)
#define c_cyan_magenta all_colors.get(def_c_cyan_magenta)
#define c_ltcyan_magenta all_colors.get(def_c_ltcyan_magenta)

#define c_black_cyan all_colors.get(def_c_black_cyan)
#define c_dkgray_cyan all_colors.get(def_c_dkgray_cyan)
#define c_ltgray_cyan all_colors.get(def_c_ltgray_cyan)
#define c_white_cyan all_colors.get(def_c_white_cyan)
#define c_red_cyan all_colors.get(def_c_red_cyan)
#define c_ltred_cyan all_colors.get(def_c_ltred_cyan)
#define c_green_cyan all_colors.get(def_c_green_cyan)
#define c_ltgreen_cyan all_colors.get(def_c_ltgreen_cyan)
#define c_brown_cyan all_colors.get(def_c_brown_cyan)
#define c_yellow_cyan all_colors.get(def_c_yellow_cyan)
#define c_blue_cyan all_colors.get(def_c_blue_cyan)
#define c_ltblue_cyan all_colors.get(def_c_ltblue_cyan)
#define c_magenta_cyan all_colors.get(def_c_magenta_cyan)
#define c_pink_cyan all_colors.get(def_c_pink_cyan)
#define c_cyan_cyan all_colors.get(def_c_cyan_cyan)
#define c_ltcyan_cyan all_colors.get(def_c_ltcyan_cyan)

// def_x is a color that maps to x with default settings
enum color_id {
    def_c_black = 0,
    def_c_white,
    def_c_ltgray,
    def_c_dkgray,
    def_c_red,
    def_c_green,
    def_c_blue,
    def_c_cyan,
    def_c_magenta,
    def_c_brown,
    def_c_ltred,
    def_c_ltgreen,
    def_c_ltblue,
    def_c_ltcyan,
    def_c_pink,
    def_c_yellow,

    def_h_black,
    def_h_white,
    def_h_ltgray,
    def_h_dkgray,
    def_h_red,
    def_h_green,
    def_h_blue,
    def_h_cyan,
    def_h_magenta,
    def_h_brown,
    def_h_ltred,
    def_h_ltgreen,
    def_h_ltblue,
    def_h_ltcyan,
    def_h_pink,
    def_h_yellow,

    def_i_black,
    def_i_white,
    def_i_ltgray,
    def_i_dkgray,
    def_i_red,
    def_i_green,
    def_i_blue,
    def_i_cyan,
    def_i_magenta,
    def_i_brown,
    def_i_ltred,
    def_i_ltgreen,
    def_i_ltblue,
    def_i_ltcyan,
    def_i_pink,
    def_i_yellow,

    def_c_unset,

    def_c_white_red,
    def_c_ltgray_red,
    def_c_dkgray_red,
    def_c_red_red,
    def_c_green_red,
    def_c_blue_red,
    def_c_cyan_red,
    def_c_magenta_red,
    def_c_brown_red,
    def_c_ltred_red,
    def_c_ltgreen_red,
    def_c_ltblue_red,
    def_c_ltcyan_red,
    def_c_pink_red,
    def_c_yellow_red,

    def_c_black_white,
    def_c_dkgray_white,
    def_c_ltgray_white,
    def_c_white_white,
    def_c_red_white,
    def_c_ltred_white,
    def_c_green_white,
    def_c_ltgreen_white,
    def_c_brown_white,
    def_c_yellow_white,
    def_c_blue_white,
    def_c_ltblue_white,
    def_c_magenta_white,
    def_c_pink_white,
    def_c_cyan_white,
    def_c_ltcyan_white,

    def_c_black_green,
    def_c_dkgray_green,
    def_c_ltgray_green,
    def_c_white_green,
    def_c_red_green,
    def_c_ltred_green,
    def_c_green_green,
    def_c_ltgreen_green,
    def_c_brown_green,
    def_c_yellow_green,
    def_c_blue_green,
    def_c_ltblue_green,
    def_c_magenta_green,
    def_c_pink_green,
    def_c_cyan_green,
    def_c_ltcyan_green,

    def_c_black_yellow,
    def_c_dkgray_yellow,
    def_c_ltgray_yellow,
    def_c_white_yellow,
    def_c_red_yellow,
    def_c_ltred_yellow,
    def_c_green_yellow,
    def_c_ltgreen_yellow,
    def_c_brown_yellow,
    def_c_yellow_yellow,
    def_c_blue_yellow,
    def_c_ltblue_yellow,
    def_c_magenta_yellow,
    def_c_pink_yellow,
    def_c_cyan_yellow,
    def_c_ltcyan_yellow,

    def_c_black_magenta,
    def_c_dkgray_magenta,
    def_c_ltgray_magenta,
    def_c_white_magenta,
    def_c_red_magenta,
    def_c_ltred_magenta,
    def_c_green_magenta,
    def_c_ltgreen_magenta,
    def_c_brown_magenta,
    def_c_yellow_magenta,
    def_c_blue_magenta,
    def_c_ltblue_magenta,
    def_c_magenta_magenta,
    def_c_pink_magenta,
    def_c_cyan_magenta,
    def_c_ltcyan_magenta,

    def_c_black_cyan,
    def_c_dkgray_cyan,
    def_c_ltgray_cyan,
    def_c_white_cyan,
    def_c_red_cyan,
    def_c_ltred_cyan,
    def_c_green_cyan,
    def_c_ltgreen_cyan,
    def_c_brown_cyan,
    def_c_yellow_cyan,
    def_c_blue_cyan,
    def_c_ltblue_cyan,
    def_c_magenta_cyan,
    def_c_pink_cyan,
    def_c_cyan_cyan,
    def_c_ltcyan_cyan,

    num_colors
};

class JsonObject;

void init_colors();

enum col_attribute {
WA_NULL = 0,
HI = 1,
INV = 2
};

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

typedef int nc_color;

class color_manager : public JsonSerializer, public JsonDeserializer {
    private:
        void add_color( const color_id col, const std::string &name,
                        const nc_color color_pair, const color_id inv_enum );
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
            // String names for custom colors
            std::string name_custom;
            std::string name_invert_custom;
        };

        std::array<color_struct, num_colors> color_array;
        std::unordered_map<nc_color, color_id> inverted_map;
        std::unordered_map<std::string, color_id> name_map;

        bool save_custom();

    public:
        color_manager() {};

        nc_color get( const color_id id ) const;

        nc_color get_invert( const nc_color color ) const;
        nc_color get_highlight( const nc_color color, const hl_enum bg ) const;
        nc_color get_random() const;

        color_id color_to_id( const nc_color color ) const;
        color_id name_to_id( const std::string &name ) const;

        std::string get_name( const nc_color color ) const;
        std::string id_to_name( const color_id id ) const;

        nc_color name_to_color( const std::string &name ) const;

        nc_color highlight_from_names( const std::string &name, const std::string &bg_name ) const;

        void load_default();
        void load_custom( const std::string &sPath = "" );

        void show_gui();

        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const override;
        void deserialize(JsonIn &jsin) override;
};

color_manager &get_all_colors();

struct note_color {
    nc_color color;
    std::string name;
};

extern std::unordered_map<std::string, note_color> color_by_string_map;
extern std::unordered_map<std::string, note_color> color_shortcuts;

nc_color hilite(nc_color c);
nc_color invert_color(nc_color c);
nc_color red_background(nc_color c);
nc_color white_background(nc_color c);
nc_color green_background(nc_color c);
nc_color yellow_background(nc_color c);
nc_color magenta_background(nc_color c);
nc_color cyan_background(nc_color c);

nc_color color_from_string(const std::string &color);
std::string string_from_color(const nc_color color);
nc_color bgcolor_from_string(std::string color);
nc_color get_color_from_tag(const std::string &s, const nc_color base_color);

void setattr(nc_color &col, col_attribute attr);
void load_colors(JsonObject &jo);

nc_color get_note_color(std::string const &note_id);
std::list<std::pair<std::string, std::string>> get_note_color_names();

#endif
