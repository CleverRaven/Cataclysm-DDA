#pragma once
#ifndef CATA_SRC_SURROUNDINGS_MENU_H
#define CATA_SRC_SURROUNDINGS_MENU_H

#include "avatar.h"
#include "cata_imgui.h"
#include "map.h"
#include "map_entity_stack.h"

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

        virtual size_t get_filtered_list_size() = 0;
        virtual void reset_filter() = 0;
        virtual void apply_filter( std::string filter ) = 0;

    protected:
        const int radius = 60;
        std::string filter;
};

class item_tab_data : public tab_data
{
    public:
        item_tab_data( const Character &you, map &m, const std::string &title = "Items" );
        std::vector<map_entity_stack<item>*> item_list;
        std::vector<map_entity_stack<item>*> filtered_list;
        map_entity_stack<item> *selected_entry;

        size_t get_filtered_list_size() override {
            return filtered_list.size();
        }

        void reset_filter() override {
            filter = std::string();
            filtered_list = item_list;
        }

        void apply_filter( std::string filter ) override;

    private:
        std::map<std::string, map_entity_stack<item>> items;
        void find_nearby_items( const Character &you, map &m );
        void add_item_recursive( std::vector<std::string> &item_order, const item *it,
                                 const tripoint_rel_ms &relative_pos );
};

class monster_tab_data : public tab_data
{
    public:
        explicit monster_tab_data( const Character &you, const std::string &title = "Monsters" );
        std::vector<map_entity_stack<Creature>*> monster_list;
        std::vector<map_entity_stack<Creature>*> filtered_list;
        map_entity_stack<Creature> *selected_entry;

        size_t get_filtered_list_size() override {
            return filtered_list.size();
        }

        void reset_filter() override {
            filter = std::string();
            filtered_list = monster_list;
        }

        void apply_filter( std::string filter ) override;

    private:
        // could be turned into a map to allow monster grouping
        std::vector<map_entity_stack<Creature>> monsters;
        void find_nearby_monsters( const Character &you );
};

class terfurn_tab_data : public tab_data
{
    public:
        terfurn_tab_data( const Character &you, map &m, const std::string &title = "Terrain/Furniture" );
        std::vector<map_entity_stack<map_data_common_t>*> terfurn_list;
        std::vector<map_entity_stack<map_data_common_t>*> filtered_list;
        map_entity_stack<map_data_common_t> *selected_entry;

        size_t get_filtered_list_size() override {
            return filtered_list.size();
        }

        void reset_filter() override {
            filter = std::string();
            filtered_list = terfurn_list;
        }

        void apply_filter( std::string filter ) override;

    private:
        std::map<std::string, map_entity_stack<map_data_common_t>> terfurns;
        void add_terfurn( std::vector<std::string> &item_order, const map_data_common_t *terfurn,
                          const tripoint_rel_ms &relative_pos );
        void find_nearby_terfurn( const Character &you, map &m );
};

//struct list_layout_elements {
//    shared_ptr_fast<list_view> list;
//    shared_ptr_fast<text_block> title;
//    shared_ptr_fast<index_indicator> paging;
//    shared_ptr_fast<text_block> sort;
//};

class surroundings_menu : public cataimgui::window
{
    public:
        surroundings_menu( avatar &you, map &m, std::optional<tripoint> &path_end, int width );
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

        surroundings_menu_tab_enum selected_tab = surroundings_menu_tab_enum::items;
        surroundings_menu_tab_enum switch_tab = surroundings_menu_tab_enum::num_tabs;
        // auto scrolling when navigating by keyboard
        bool scroll = false;

        avatar &you;
        map &m;
        std::optional<tripoint> &path_end;
        const tripoint stored_view_offset;

        int info_scroll_pos;
        bool hide_ui;

        int width;
        int info_height;

        item_tab_data item_data;
        monster_tab_data monster_data;
        terfurn_tab_data terfurn_data;

        const int dist_width = 7;
        const int page_scroll = 10;
};

#endif // CATA_SRC_SURROUNDINGS_MENU_H
