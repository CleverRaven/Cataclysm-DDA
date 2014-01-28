#include "output.h"
#include "game.h"
#include <string>

class construct_ui {
    public:
    enum construct_filter { ALL = 0, AVAILABLE, UNAVAILABLE };
    enum construct_windows { HEAD = 1, LIST = 2, DESCRIPTION = 4, ALL_WIN = HEAD | LIST | DESCRIPTION };

    void display( player *p );

    construct_ui() :
        head_height( 4 ),
        min_w_height( 10 ),
        max_w_height( FULL_SCREEN_HEIGHT ),
        min_w_width( FULL_SCREEN_WIDTH ),
        max_w_width( 90 ),
        min_l_width( 30 ),
        max_l_width( 40 ),
        redraw_flags( ALL_WIN ),
        filter( ALL ),
        exit( false ) {}

    private:
    class construct_ui_item;

    void draw_header( WINDOW* );
    void draw_list( WINDOW* );
    void draw_description( WINDOW* );
    void init( player *p );
    void handle_key( int );
    std::string get_filter_text();

    const int head_height;
    const int min_w_height;
    const int max_w_height;
    const int min_w_width;
    const int max_w_width;
    const int min_l_width;
    const int max_l_width;

    int w_width, w_height, l_width;
    int headstart, colstart;
    bool exit;
    construct_windows redraw_flags;
    int filter;
    player *u;

    static std::set<construct_ui_item> construct_ui_items;
    static bool loaded;

    static void load_constructions(construction_iterator start) {
        if( loaded ) { return; }

        

        loaded = true;
    }
};

class construct_ui::construct_ui_item {

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
        case construct_ui::construct_filter::ALL:
            return a = construct_ui::construct_filter::AVAILABLE;
        case construct_ui::construct_filter::AVAILABLE:
            return a = construct_ui::construct_filter::UNAVAILABLE;
        case construct_ui::construct_filter::UNAVAILABLE:
            return a = construct_ui::construct_filter::ALL;
    }
}