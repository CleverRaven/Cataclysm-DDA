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
#include "output.h"

class JsonObject;
struct input_event;

namespace catacurses
{
class window;
}  // namespace catacurses

class help
{
    public:
        static void load( const JsonObject &jo, const std::string &src );
        static void reset();
        // TODO: Shouldn't be public
        std::map<int, std::pair<translation, std::vector<translation>>> help_texts;
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
        cataimgui::bounds bounds{ 0.0f, 0.0f, static_cast<float>( str_width_to_pixels( TERMX ) ), static_cast<float>( str_height_to_pixels( TERMY ) ) };
        // 66 is optimal characters per line for reading?
        float window_width = static_cast<float>( str_width_to_pixels( TERMX ) );
        float window_height = static_cast<float>( str_width_to_pixels( TERMY ) );
        float wrap_width = std::min( window_width * 0.95f,
                                     static_cast<float>( str_width_to_pixels( 66 ) ) );
        static std::string get_note_colors();
        static std::string get_dir_grid();
        std::string seperator( int length, char c );
        input_context ctxt;
        help &data = get_help();
        std::map<int, input_event> hotkeys;
        int mouse_selected_option;
        int keyboard_selected_option;
        int last_keyboard_selected_option;
        bool selected_category = false;
        // TODO: Needs to be a pointer or iterator or something, currently assigning to it is overwriting the first help_text ;D
        std::pair<translation, std::vector<translation>> &category = data.help_texts.begin()->second;
        std::string parse_tags_help_window( std::string translated_line );
        void draw_category_selection();
        void draw_category( translation &category_name, std::vector<translation> &paragraphs );
        void format_title( const std::string translated_category_name );
};

std::string get_hint();

#endif // CATA_SRC_HELP_H
