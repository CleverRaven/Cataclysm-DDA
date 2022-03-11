#pragma once
#ifndef CATA_SRC_LIST_VIEW_H
#define CATA_SRC_LIST_VIEW_H

#include <vector>

#include "input.h"
#include "ui_element.h"

namespace catacurses
{
class window;
}// namespace catacurses

template<typename T = ui_element>
class list_view : public ui_element
{
    public:
        list_view() : allow_selecting( false ) {};
        explicit list_view( input_context &ctxt );
        ~list_view() override;

        T *get_selected_element();
        int get_selected_index();
        int get_selected_position();
        int get_count();
        int get_count_with_categories();
        void select_next();
        void select_prev();
        void select_page_up();
        void select_page_down();
        void set_elements( std::vector<T *> new_list );
        void draw( const catacurses::window &w, const point &p, const int width, const int height,
                   const bool selected = false ) override;
        bool handle_input( const std::string &action );
        bool toggle_draw_categories();
        void calc_startpos_with_categories( int &start_pos, int &max_size, const int height );
        void calculate_categories();
        std::string get_element_category( const int index );
        bool is_drawing_categories();

    private:
        std::vector<T *> list_elements;
        int selected_index = 0;
        int page_scroll = 10;
        bool draw_categories = false;
        bool allow_selecting = false;
        std::map<int, std::string> categories;
};

#endif // CATA_SRC_LIST_VIEW_H
