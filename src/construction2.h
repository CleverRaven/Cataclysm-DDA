#include "output.h"
#include "game.h"
#include <string>

class construct_ui {
    

    public:
    enum construct_filter { ALL = 0, AVAILABLE, UNAVAILABLE };

    void display( player *p );

    construct_ui() :
        head_height( 3 ),
        min_w_height( 10 ),
        max_w_height( FULL_SCREEN_HEIGHT ),
        min_w_width( FULL_SCREEN_WIDTH ),
        max_w_width( 90 ),
        min_l_width( 30 ),
        max_l_width( 40 ),
        redraw_head( true ),
        redraw_list( true ),
        redraw_description( true ),
        filter( ALL ),
        exit(false){}

    private:
    void draw_header(WINDOW*);
    void draw_list(WINDOW*);
    void draw_description(WINDOW*);
    void init(player *p);
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
    bool redraw_head, redraw_description, redraw_list;

    construct_filter filter;

    player *u;
};