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

class avatar;
class Character;
class Creature;
template <typename T> struct enum_traits;
class item;
class map;
struct map_data_common_t;

enum class surroundings_menu_tab_enum : int {
    items = 0,
    monsters,
    terfurn,
    num_tabs
};

template<>
struct enum_traits<surroundings_menu_tab_enum> {
    static constexpr surroundings_menu_tab_enum last = surroundings_menu_tab_enum::num_tabs;
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
        // each group will start at a new line
        // currently roughly grouped by actions on an entry and actions on the list as a whole
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
        virtual void set_filter_active( bool active ) const = 0;
        virtual bool is_monster_tab() const {
            return false;
        }
        virtual bool can_fire_at_selected( avatar &, int ) const {
            return false;
        }
        virtual bool fire_at_selected( avatar &, int, const tripoint_rel_ms & ) const {
            return false;
        }

        void filter_popup();
        bool has_filter() const;
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
        explicit item_tab_data( const std::string &title = _( "Items" ) );
        void init( const Character &you, map &m );
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

        void reset_filter() override;
        void apply_filter() override;
        int get_max_entity_width() override;
        std::optional<tripoint_rel_ms> get_selected_pos() override;
        void select_prev_internal() override;
        void select_next_internal() override;
        void move_selection( int amount ) override;
        void priority_popup( bool high ) override;
        void set_filter_active( bool active ) const override;

    private:
        std::map<std::string, map_entity_stack<item>> items;
        void find_nearby_items( const Character &you, map &m );
        void add_item_recursive( std::vector<std::string> &item_order, const item *it,
                                 const tripoint_rel_ms &relative_pos );
};

class monster_tab_data : public tab_data
{
    public:
        explicit monster_tab_data( const std::string &title = _( "Monsters" ) );
        void init( const Character &you );
        std::vector<map_entity_stack<Creature>*> monster_list;
        std::vector<map_entity_stack<Creature>*> filtered_list;
        map_entity_stack<Creature> *selected_entry = nullptr;

        size_t get_filtered_list_size() override {
            return filtered_list.size();
        }

        void reset_filter() override;
        void apply_filter() override;
        int get_max_entity_width() override;
        std::optional<tripoint_rel_ms> get_selected_pos() override;
        void select_prev_internal() override;
        void select_next_internal() override;
        void move_selection( int amount ) override;
        void set_filter_active( bool active ) const override;
        bool is_monster_tab() const override {
            return true;
        }
        bool can_fire_at_selected( avatar &you, int max_gun_range ) const override;
        bool fire_at_selected( avatar &you, int max_gun_range,
                               const tripoint_rel_ms &stored_view_offset ) const override;

    private:
        // could be turned into a map to allow monster grouping
        std::vector<map_entity_stack<Creature>> monsters;
        void find_nearby_monsters( const Character &you );
};

class terfurn_tab_data : public tab_data
{
    public:
        explicit terfurn_tab_data( const std::string &title = _( "Terrain/Furniture" ) );
        void init( const Character &you, map &m );
        std::vector<map_entity_stack<map_data_common_t>*> terfurn_list;
        std::vector<map_entity_stack<map_data_common_t>*> filtered_list;
        map_entity_stack<map_data_common_t> *selected_entry = nullptr;

        size_t get_filtered_list_size() override {
            return filtered_list.size();
        }

        void reset_filter() override;
        void apply_filter() override;
        int get_max_entity_width() override;
        std::optional<tripoint_rel_ms> get_selected_pos() override;
        void select_prev_internal() override;
        void select_next_internal() override;
        void move_selection( int amount ) override;
        void set_filter_active( bool active ) const override;

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
        void draw_hotkey_buttons( const tab_data *tab );
        float get_hotkey_buttons_height( const tab_data *tab );

        void update_path_end();
        void handle_list_input( const std::string &action );
        std::optional<tripoint_rel_ms> get_selected_pos();

        tab_data *get_tab_data( surroundings_menu_tab_enum tab );
        tab_data *get_selected_data();
        void select_prev();
        void select_next();

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

        int max_gun_range = 0;
        bool highlight_new = false;
        const std::string item_new_str = pgettext( "surroundings menu", "NEW!" );
        const std::string item_maybe_new_str = pgettext( "surroundings menu", "hides contents" );
        const nc_color item_new_col = c_light_green;
        const nc_color item_maybe_new_col = c_light_gray;
        const int item_new_str_width = str_width_to_pixels( utf8_width( item_new_str ) );
        const int item_maybe_new_str_width = str_width_to_pixels( utf8_width( item_maybe_new_str ) );

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
