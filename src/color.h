#ifndef COLOR_H
#define COLOR_H

#include "cursesdef.h"
#include "json.h"
#include <string>
#include <list>
#include <unordered_map>

#define all_colors get_all_colors()

#define c_black all_colors.get("c_black")
#define c_white all_colors.get("c_white")
#define c_ltgray all_colors.get("c_ltgray")
#define c_dkgray all_colors.get("c_dkgray")
#define c_red all_colors.get("c_red")
#define c_green all_colors.get("c_green")
#define c_blue all_colors.get("c_blue")
#define c_cyan all_colors.get("c_cyan")
#define c_magenta all_colors.get("c_magenta")
#define c_brown all_colors.get("c_brown")
#define c_ltred all_colors.get("c_ltred")
#define c_ltgreen all_colors.get("c_ltgreen")
#define c_ltblue all_colors.get("c_ltblue")
#define c_ltcyan all_colors.get("c_ltcyan")
#define c_pink all_colors.get("c_pink")
#define c_yellow all_colors.get("c_yellow")

#define h_black all_colors.get("h_black")
#define h_white all_colors.get("h_white")
#define h_ltgray all_colors.get("h_ltgray")
#define h_dkgray all_colors.get("h_dkgray")
#define h_red all_colors.get("h_red")
#define h_green all_colors.get("h_green")
#define h_blue all_colors.get("h_blue")
#define h_cyan all_colors.get("h_cyan")
#define h_magenta all_colors.get("h_magenta")
#define h_brown all_colors.get("h_brown")
#define h_ltred all_colors.get("h_ltred")
#define h_ltgreen all_colors.get("h_ltgreen")
#define h_ltblue all_colors.get("h_ltblue")
#define h_ltcyan all_colors.get("h_ltcyan")
#define h_pink all_colors.get("h_pink")
#define h_yellow all_colors.get("h_yellow")

#define i_black all_colors.get("i_black")
#define i_white all_colors.get("i_white")
#define i_ltgray all_colors.get("i_ltgray")
#define i_dkgray all_colors.get("i_dkgray")
#define i_red all_colors.get("i_red")
#define i_green all_colors.get("i_green")
#define i_blue all_colors.get("i_blue")
#define i_cyan all_colors.get("i_cyan")
#define i_magenta all_colors.get("i_magenta")
#define i_brown all_colors.get("i_brown")
#define i_ltred all_colors.get("i_ltred")
#define i_ltgreen all_colors.get("i_ltgreen")
#define i_ltblue all_colors.get("i_ltblue")
#define i_ltcyan all_colors.get("i_ltcyan")
#define i_pink all_colors.get("i_pink")
#define i_yellow all_colors.get("i_yellow")

#define c_unset all_colors.get("c_unset")

#define c_white_red all_colors.get("c_white_red")
#define c_ltgray_red all_colors.get("c_ltgray_red")
#define c_dkgray_red all_colors.get("c_dkgray_red")
#define c_red_red all_colors.get("c_red_red")
#define c_green_red all_colors.get("c_green_red")
#define c_blue_red all_colors.get("c_blue_red")
#define c_cyan_red all_colors.get("c_cyan_red")
#define c_magenta_red all_colors.get("c_magenta_red")
#define c_brown_red all_colors.get("c_brown_red")
#define c_ltred_red all_colors.get("c_ltred_red")
#define c_ltgreen_red all_colors.get("c_ltgreen_red")
#define c_ltblue_red all_colors.get("c_ltblue_red")
#define c_ltcyan_red all_colors.get("c_ltcyan_red")
#define c_pink_red all_colors.get("c_pink_red")
#define c_yellow_red all_colors.get("c_yellow_red")

#define c_black_white all_colors.get("c_black_white")
#define c_dkgray_white all_colors.get("c_dkgray_white")
#define c_ltgray_white all_colors.get("c_ltgray_white")
#define c_white_white all_colors.get("c_white_white")
#define c_red_white all_colors.get("c_red_white")
#define c_ltred_white all_colors.get("c_ltred_white")
#define c_green_white all_colors.get("c_green_white")
#define c_ltgreen_white all_colors.get("c_ltgreen_white")
#define c_brown_white all_colors.get("c_brown_white")
#define c_yellow_white all_colors.get("c_yellow_white")
#define c_blue_white all_colors.get("c_blue_white")
#define c_ltblue_white all_colors.get("c_ltblue_white")
#define c_magenta_white all_colors.get("c_magenta_white")
#define c_pink_white all_colors.get("c_pink_white")
#define c_cyan_white all_colors.get("c_cyan_white")
#define c_ltcyan_white all_colors.get("c_ltcyan_white")

#define c_black_green all_colors.get("c_black_green")
#define c_dkgray_green all_colors.get("c_dkgray_green")
#define c_ltgray_green all_colors.get("c_ltgray_green")
#define c_white_green all_colors.get("c_white_green")
#define c_red_green all_colors.get("c_red_green")
#define c_ltred_green all_colors.get("c_ltred_green")
#define c_green_green all_colors.get("c_green_green")
#define c_ltgreen_green all_colors.get("c_ltgreen_green")
#define c_brown_green all_colors.get("c_brown_green")
#define c_yellow_green all_colors.get("c_yellow_green")
#define c_blue_green all_colors.get("c_blue_green")
#define c_ltblue_green all_colors.get("c_ltblue_green")
#define c_magenta_green all_colors.get("c_magenta_green")
#define c_pink_green all_colors.get("c_pink_green")
#define c_cyan_green all_colors.get("c_cyan_green")
#define c_ltcyan_green all_colors.get("c_ltcyan_green")

#define c_black_yellow all_colors.get("c_black_yellow")
#define c_dkgray_yellow all_colors.get("c_dkgray_yellow")
#define c_ltgray_yellow all_colors.get("c_ltgray_yellow")
#define c_white_yellow all_colors.get("c_white_yellow")
#define c_red_yellow all_colors.get("c_red_yellow")
#define c_ltred_yellow all_colors.get("c_ltred_yellow")
#define c_green_yellow all_colors.get("c_green_yellow")
#define c_ltgreen_yellow all_colors.get("c_ltgreen_yellow")
#define c_brown_yellow all_colors.get("c_brown_yellow")
#define c_yellow_yellow all_colors.get("c_yellow_yellow")
#define c_blue_yellow all_colors.get("c_blue_yellow")
#define c_ltblue_yellow all_colors.get("c_ltblue_yellow")
#define c_magenta_yellow all_colors.get("c_magenta_yellow")
#define c_pink_yellow all_colors.get("c_pink_yellow")
#define c_cyan_yellow all_colors.get("c_cyan_yellow")
#define c_ltcyan_yellow all_colors.get("c_ltcyan_yellow")

#define c_black_magenta all_colors.get("c_black_magenta")
#define c_dkgray_magenta all_colors.get("c_dkgray_magenta")
#define c_ltgray_magenta all_colors.get("c_ltgray_magenta")
#define c_white_magenta all_colors.get("c_white_magenta")
#define c_red_magenta all_colors.get("c_red_magenta")
#define c_ltred_magenta all_colors.get("c_ltred_magenta")
#define c_green_magenta all_colors.get("c_green_magenta")
#define c_ltgreen_magenta all_colors.get("c_ltgreen_magenta")
#define c_brown_magenta all_colors.get("c_brown_magenta")
#define c_yellow_magenta all_colors.get("c_yellow_magenta")
#define c_blue_magenta all_colors.get("c_blue_magenta")
#define c_ltblue_magenta all_colors.get("c_ltblue_magenta")
#define c_magenta_magenta all_colors.get("c_magenta_magenta")
#define c_pink_magenta all_colors.get("c_pink_magenta")
#define c_cyan_magenta all_colors.get("c_cyan_magenta")
#define c_ltcyan_magenta all_colors.get("c_ltcyan_magenta")

#define c_black_cyan all_colors.get("c_black_cyan")
#define c_dkgray_cyan all_colors.get("c_dkgray_cyan")
#define c_ltgray_cyan all_colors.get("c_ltgray_cyan")
#define c_white_cyan all_colors.get("c_white_cyan")
#define c_red_cyan all_colors.get("c_red_cyan")
#define c_ltred_cyan all_colors.get("c_ltred_cyan")
#define c_green_cyan all_colors.get("c_green_cyan")
#define c_ltgreen_cyan all_colors.get("c_ltgreen_cyan")
#define c_brown_cyan all_colors.get("c_brown_cyan")
#define c_yellow_cyan all_colors.get("c_yellow_cyan")
#define c_blue_cyan all_colors.get("c_blue_cyan")
#define c_ltblue_cyan all_colors.get("c_ltblue_cyan")
#define c_magenta_cyan all_colors.get("c_magenta_cyan")
#define c_pink_cyan all_colors.get("c_pink_cyan")
#define c_cyan_cyan all_colors.get("c_cyan_cyan")
#define c_ltcyan_cyan all_colors.get("c_ltcyan_cyan")

class JsonObject;

void init_colors();

enum col_attribute {
WA_NULL = 0,
HI = 1,
INV = 2
};

typedef int nc_color;

class clColors : public JsonSerializer, public JsonDeserializer {
    private:
        void add_color(const std::string &sName, const nc_color iColorPair, const std::string &sInvert);

        struct stColors {
            nc_color color; // Default color
            std::string sCustom; // Custom color name
            std::string sInvert; // Invert
            std::string sInvertCustom; // Invert Custom
        };

        std::unordered_map<std::string, stColors> mapColors;

        bool save_custom();

    public:
        clColors() {};

        nc_color get(const std::string &sName);
        std::string get(const nc_color color);
        nc_color get_invert(const nc_color color);
        nc_color get_default(const std::string &sName);
        nc_color get_highlight(const nc_color color, const std::string &bgColor = "");
        nc_color get_random();

        void load_default();
        void load_custom(const std::string &sPath = "");

        void show_gui();

        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const override;
        void deserialize(JsonIn &jsin) override;
};

clColors &get_all_colors();

struct note_color {
    nc_color color;
    std::string name;
};

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
