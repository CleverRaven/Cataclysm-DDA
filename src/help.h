#pragma once
#ifndef CATA_SRC_HELP_H
#define CATA_SRC_HELP_H

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "cata_imgui.h"
#include "input_context.h"
#include "imgui/imgui.h"
#include "options.h"
#include "output.h"

class JsonObject;
struct input_event;

namespace catacurses
{
class window;
}  // namespace catacurses

struct help_category {
    translation name;
    std::vector<translation> paragraphs;
};

class help
{
    public:
        static void load( const JsonObject &jo, const std::string &src );
        static void reset();
        // TODO: Shouldn't be public
        std::map<const int, const help_category> help_categories;
        // Only persists per load (eg resets on menu -> game, game -> menu)
        std::unordered_set<int> read_categories;
    private:
        void load_object( const JsonObject &jo, const std::string &src );
        void reset_instance();
        // Modifier for each mods order
        int current_order_start = 0;
        std::string current_src;
};

help &get_help();

class help_window : public cataimgui::window
{
    public:
        explicit help_window();
        void show();
    protected:
        void draw_controls() override;
        cataimgui::bounds get_bounds() override;
    private:
        // Fullscreen
        float window_width = static_cast<float>( str_width_to_pixels( TERMX ) );
        float window_height = static_cast<float>( str_width_to_pixels( TERMY ) );
        float wrap_width = window_width * 0.95f;
        cataimgui::bounds bounds{ 0.f, 0.f, static_cast<float>( str_width_to_pixels( TERMX ) ), static_cast<float>( str_height_to_pixels( TERMY ) ) };

        // 66 is optimal characters per line for reading?
        //float window_width = static_cast<float>( wrap_width ) * 2.025f;
        //float window_height = static_cast<float>( str_width_to_pixels( TERMY ) ) * 0.8f;
        //float wrap_width = std::min( window_width * 0.95f, static_cast<float>( str_width_to_pixels( 66 ) ) );
        //cataimgui::bounds bounds{ 0.f, 0.f, window_width, window_height };

        help &data = get_help();
        input_context ctxt;
        std::map<int, input_event> hotkeys;

        void draw_category_selection();
        void format_title( const std::string translated_category_name );
        std::string seperator( int length, char c );

        void draw_category_option( const int &option, const help_category &category );
        int selected_option;
        bool has_selected_category = false;
        int loaded_option;

        void draw_category();
        void parse_tags_help_window( std::string &translated_line );
        static std::string get_note_colors();
        static std::string get_dir_grid();
};

std::string get_hint();

#endif // CATA_SRC_HELP_H
