#pragma once
#ifndef CATA_SRC_SURROUNDINGS_MENU_H
#define CATA_SRC_SURROUNDINGS_MENU_H

#include <climits>
#include <map>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>
#include <imgui/imgui.h>
#include <stddef.h>

#include "cata_imgui.h"
#include "color.h"
#include "coordinates.h"
#include "input_context.h"
#include "map_entity_stack.h"
#include "translations.h"
#include "uistate.h"

class avatar;
class Character;
class Creature;
class item;
class map;
struct map_data_common_t;

enum class surroundings_menu_ret : int {
    quit = 0,
    fire,
};

enum class surroundings_menu_tab_enum : int {
    items = 0,
    monsters,
    terfurn,
    num_tabs
};

class tab_data
{
    protected:
        explicit tab_data( const std::string &title );
    public:
        virtual ~tab_data() = default;
        input_context ctxt;
        std::string title;

        bool draw_categories = false;
        std::vector<std::unordered_set<std::string>> hotkey_groups;

        virtual size_t get_filtered_list_size() = 0;
        virtual void reset_filter() = 0;
        virtual void apply_filter() = 0;
        virtual int get_max_entity_width() = 0;
        virtual std::optional<tripoint_rel_ms> get_selected_pos() = 0;
        virtual void select_prev_internal() = 0;
        virtual void select_next_internal() = 0;
        virtual void move_selection( int amount ) = 0;
        virtual void priority_popup( bool ) {}

        void filter_popup();
        bool has_filter();
    protected:
        const int radius = 60;

        std::string filter;
        std::string filter_title;
        std::string filter_description;
        std::string filter_label = _( "Filter: " );
        std::string filter_identifier;
};

class item_tab_data : public tab_data
{
    public:
        item_tab_data( const Character &you, map &m, const std::string &title = _( "Items" ) );
        std::vector<map_entity_stack<item>*> item_list;
        std::vector<map_entity_stack<item>*> filtered_list;
        map_entity_stack<item> *selected_entry = nullptr;
        int priority_plus_end = -1;
        int priority_minus_start = INT_MAX;
        std::string filter_priority_plus;
        std::string filter_priority_minus;

        size_t get_filtered_list_size() override {
            return filtered_list.size();
        }

        void reset_filter() override {
            filter = std::string();
            uistate.list_item_filter = std::string();
            filtered_list = item_list;
        }

        //void priority_popup( bool high ) override;
        void apply_filter() override;
        int get_max_entity_width() override;
        std::optional<tripoint_rel_ms> get_selected_pos() override;
        void select_prev_internal() override;
        void select_next_internal() override;
        void move_selection( int amount ) override;
        void priority_popup( bool high ) override;

    private:
        std::map<std::string, map_entity_stack<item>> items;
        void find_nearby_items( const Character &you, map &m );
        void add_item_recursive( std::vector<std::string> &item_order, const item *it,
                                 const tripoint_rel_ms &relative_pos );
};

class monster_tab_data : public tab_data
{
    public:
        explicit monster_tab_data( const Character &you, const std::string &title = _( "Monsters" ) );
        std::vector<map_entity_stack<Creature>*> monster_list;
        std::vector<map_entity_stack<Creature>*> filtered_list;
        map_entity_stack<Creature> *selected_entry = nullptr;

        size_t get_filtered_list_size() override {
            return filtered_list.size();
        }

        void reset_filter() override {
            filter = std::string();
            uistate.monster_filter = std::string();
            filtered_list = monster_list;
        }

        void apply_filter() override;
        int get_max_entity_width() override;
        std::optional<tripoint_rel_ms> get_selected_pos() override;
        void select_prev_internal() override;
        void select_next_internal() override;
        void move_selection( int amount ) override;

    private:
        // could be turned into a map to allow monster grouping
        std::vector<map_entity_stack<Creature>> monsters;
        void find_nearby_monsters( const Character &you );
};

class terfurn_tab_data : public tab_data
{
    public:
        terfurn_tab_data( const Character &you, map &m,
                          const std::string &title = _( "Terrain/Furniture" ) );
        std::vector<map_entity_stack<map_data_common_t>*> terfurn_list;
        std::vector<map_entity_stack<map_data_common_t>*> filtered_list;
        map_entity_stack<map_data_common_t> *selected_entry = nullptr;

        size_t get_filtered_list_size() override {
            return filtered_list.size();
        }

        void reset_filter() override {
            filter = std::string();
            uistate.terfurn_filter = std::string();
            filtered_list = terfurn_list;
        }

        void apply_filter() override;
        int get_max_entity_width() override;
        std::optional<tripoint_rel_ms> get_selected_pos() override;
        void select_prev_internal() override;
        void select_next_internal() override;
        void move_selection( int amount ) override;

    private:
        std::map<std::string, map_entity_stack<map_data_common_t>> terfurns;
        void add_terfurn( std::vector<std::string> &item_order, const map_data_common_t *terfurn,
                          const tripoint_rel_ms &relative_pos );
        void find_nearby_terfurn( const Character &you, map &m );
};

class surroundings_menu : public cataimgui::window
{
    public:
        surroundings_menu( avatar &you, map &m, std::optional<tripoint_bub_ms> &path_end, int width );
        void execute();

    protected:
        void draw_controls() override;
        cataimgui::bounds get_bounds() override;

    private:
        void draw_item_tab();
        void draw_monster_tab();
        void draw_terfurn_tab();
        void draw_examine_info();
        void draw_category_separator( const std::string &category, std::string &last_category,
                                      int target_col );
        void draw_hotkey_buttons();
        float get_hotkey_buttons_height();

        void toggle_safemode_entry( const map_entity_stack<Creature> *mstack );
        void update_path_end();
        void handle_list_input( const std::string &action );
        std::optional<tripoint_rel_ms> get_selected_pos();
        /*template<typename T>
        const T *get_selected_element() const;*/

        tab_data *get_selected_data();
        void select_prev();
        void select_next();

        template<typename T>
        void select_page_up_generic( T &data );
        template<typename T>
        void select_page_down_generic( T &data );
        void select_page_up();
        void select_page_down();

        void select_prev_internal();
        void select_next_internal();

        void change_selected_tab_sorting();
        void get_filter();
        void reset_filter();
        void get_filter_priority( bool high );

        surroundings_menu_tab_enum selected_tab = surroundings_menu_tab_enum::items;
        surroundings_menu_tab_enum switch_tab = surroundings_menu_tab_enum::num_tabs;
        // auto scrolling when navigating by keyboard
        bool auto_scroll = false;
        bool hide_ui = false;

        bool highlight_new = false;
        const std::string item_new_str = pgettext( "surroundings menu", "NEW!" );
        const std::string item_maybe_new_str = pgettext( "surroundings menu", "hides contents" );
        const nc_color item_new_col = c_light_green;
        const nc_color item_maybe_new_col = c_light_gray;

        avatar &you;
        std::optional<tripoint_bub_ms> &path_end;
        const tripoint_rel_ms stored_view_offset;

        cataimgui::scroll info_scroll = cataimgui::scroll::none;

        int width;
        int info_height;
        float magic_number_other_elements_height = ImGui::GetTextLineHeight() * 3 +
                ImGui::GetStyle().FramePadding.y * 2;

        item_tab_data item_data;
        monster_tab_data monster_data;
        terfurn_tab_data terfurn_data;

        const int dist_width = 7;
        const int page_scroll = 10;
};

#endif // CATA_SRC_SURROUNDINGS_MENU_H
