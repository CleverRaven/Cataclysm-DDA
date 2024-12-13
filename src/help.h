#pragma once
#ifndef CATA_SRC_HELP_H
#define CATA_SRC_HELP_H

#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "cata_imgui.h"
#include "input_context.h"
#include "imgui/imgui.h"
#include "output.h"

class JsonArray;
class JsonObject;
class translation;
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

// Make pointer?
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
        //void draw_tabless();
        //void draw_tabbed();
        //void draw_tab_contents( size_t tab );
        static std::string get_note_colors();
        static std::string get_dir_grid();
        input_context ctxt;
        help &data = get_help();
        std::map<int, input_event> hotkeys;
        cataimgui::bounds bounds{ 0.0f, 0.0f, static_cast<float>( str_width_to_pixels( TERMX ) ), static_cast<float>( str_height_to_pixels( TERMY ) ) };
        short mouse_selected_option;
        short keyboard_selected_option;
        short last_keyboard_selected_option;
        void draw_category( translation &category_name, std::vector<translation> &paragraphs );
};

std::string get_hint();

#endif // CATA_SRC_HELP_H
