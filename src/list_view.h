#pragma once
#ifndef CATA_SRC_LIST_VIEW_H
#define CATA_SRC_LIST_VIEW_H

#include <map>

#include "ui_element.h"

class input_context;
namespace catacurses
{
class window;
}// namespace catacurses

template<typename T>
class list_view : public ui_element
{
    public:
        list_view( point pos, int width, int height ) :
            ui_element( pos, width, height ),
            allow_selecting( false ) {};
        explicit list_view( input_context &ctxt, const point pos, const int width, const int height );
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
        void set_template( const std::function<shared_ptr_fast<ui_element>( const T *, const int, bool )>
                           &template_func );
        void draw( const catacurses::window &w, const point offset ) const override;
        bool handle_input( const std::string &action );
        bool toggle_draw_categories();
        void calc_startpos_with_categories( int &start_pos, int &max_size ) const;
        void calculate_categories();
        std::string get_element_category( const int index ) const;
        bool is_drawing_categories();

    private:
        std::vector<T *> list_elements;
        int selected_index = 0;
        int page_scroll = 10;
        bool draw_categories = false;
        bool allow_selecting = false;
        std::map<int, std::string> categories;
        std::function<shared_ptr_fast<ui_element>( const T *, const int, bool )> template_func;
};

#endif // CATA_SRC_LIST_VIEW_H
