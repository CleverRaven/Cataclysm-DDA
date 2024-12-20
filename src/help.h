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
#include "options.h"
#include "output.h"

class JsonArray;
class JsonObject;
class translation;
namespace catacurses
{
class window;
}  // namespace catacurses

struct help_category {
    translation name;
    std::vector<std::pair<translation, int>> paragraphs;
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
        const bool screen_reader = get_option<bool>( "SCREEN_READER_MODE" );
        help &data = get_help();
        input_context ctxt;
        std::map<int, input_event> hotkeys;

        void draw_category_selection();
        void format_title( const std::string translated_category_name );

        void draw_category_option( const int &option, const help_category &category );
        int selected_option;
        bool has_selected_category = false;
        int loaded_option;

        void swap_translated_paragraphs();
        std::vector<std::pair<std::string, int>> translated_paragraphs;
        void parse_keybind_tags();

        void draw_category();
        cataimgui::scroll s;
};

std::string get_hint();

#endif // CATA_SRC_HELP_H
