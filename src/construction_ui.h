#ifndef _CONSTRUCTION_UI_H_
#define _CONSTRUCTION_UI_H_

#include "output.h"
#include "game.h"
#include <string>

class construct_ui {
    public:
    enum construct_filter { ALL = 0, AVAILABLE, UNAVAILABLE };
    enum construct_windows { HEAD = 0x01, LIST = 0x02, DESCRIPTION = 0x04, ALL_WIN = HEAD | LIST | DESCRIPTION, BODY = LIST | DESCRIPTION };

    void display( player *p );

    construct_ui() :
        head_height( 4 ),
        min_w_height( 10 ),
        max_w_height( FULL_SCREEN_HEIGHT ),
        min_w_width( FULL_SCREEN_WIDTH ),
        max_w_width( 90 ),
        min_l_width( 30 ),
        max_l_width( 40 ),
        exit( false ),
        redraw_flags( ALL_WIN ),
        filter( ALL ),
        selected_item(NULL) {}

    private:
    class construct_ui_item {
        public:
        bool available;

        construct_ui_item( std::string desc) :
            available( false ),
            cached( false ),
             name( desc )
        {};

        void add( construction* );
        std::string display_name( int ) const;
        std::string full_name() const;
        bool can_construct( player&, inventory, bool = true);
        std::vector<construction*>::const_iterator begin() const;
        std::vector<construction*>::const_iterator end() const;

        private:
        std::vector<construction*> constructions;
        int display_width;
        bool cached;

        const std::string name;
    };

    struct construction_sort_functor {
        bool operator()( construct_ui_item const &a, construct_ui_item const &b ) {
            if( a.available > b.available ) { return true; }
            if( a.available < b.available ) { return false; }
            return a.full_name() < b.full_name();
        }
    };

    typedef std::vector<construction*>::const_iterator con_iter;
    typedef std::map<std::string, construct_ui_item>::iterator cui_iter;
    typedef std::set<construct_ui_item, construction_sort_functor>::const_iterator sort_it;

    void draw_header( WINDOW* );
    void draw_list( WINDOW* );
    void draw_description( WINDOW* );
    int print_construction( WINDOW*, int, construction* );
    void init( player *p );
    void handle_key( int );
    std::string get_filter_text();
    std::set<construct_ui_item, construction_sort_functor> get_filtered_items();

    const int head_height;
    const int min_w_height;
    const int max_w_height;
    const int min_w_width;
    const int max_w_width;
    const int min_l_width;
    const int max_l_width;

    int w_width, w_height, l_width;
    int headstart, colstart;
    int selected_index;
    int items_size;
    bool exit;
    construct_windows redraw_flags;
    construct_filter filter;
    player *u;

    const construct_ui_item *selected_item;

    std::map<std::string, construct_ui_item> construct_ui_items;
    bool loaded;

    void load_constructions( con_iter, con_iter );
};

inline construct_ui::construct_windows operator| ( construct_ui::construct_windows a, construct_ui::construct_windows b ) {
    return static_cast<construct_ui::construct_windows>( static_cast<int>( a ) | static_cast<int>( b ) );
}

inline construct_ui::construct_windows& operator|= ( construct_ui::construct_windows &a, construct_ui::construct_windows b ) {
    return a = a | b;
}

inline construct_ui::construct_windows operator& ( construct_ui::construct_windows a, construct_ui::construct_windows b ) {
    return static_cast<construct_ui::construct_windows>( static_cast<int>(a)& static_cast<int>( b ) );
}

inline construct_ui::construct_windows& operator&= ( construct_ui::construct_windows &a, construct_ui::construct_windows b ) {
    return a = a & b;
}

inline construct_ui::construct_windows operator^ ( construct_ui::construct_windows a, construct_ui::construct_windows b ) {
    return static_cast<construct_ui::construct_windows>( static_cast<int>( a ) ^ static_cast<int>( b ) );
}

inline construct_ui::construct_windows& operator^= ( construct_ui::construct_windows &a, construct_ui::construct_windows b ) {
    return a = a ^ b;
}

inline construct_ui::construct_filter& operator++ ( construct_ui::construct_filter &a ) {
    switch( a ) {
        case construct_ui::ALL:
            return a = construct_ui::AVAILABLE;
        case construct_ui::AVAILABLE:
            return a = construct_ui::UNAVAILABLE;
        case construct_ui::UNAVAILABLE:
        default :
            return a = construct_ui::ALL;
    }
}

inline construct_ui::construct_filter operator++ ( construct_ui::construct_filter &a, int) {
    construct_ui::construct_filter result = a;
    ++a;
    return result;
}
#endif // _CONSTRUCTION_UI_H_
