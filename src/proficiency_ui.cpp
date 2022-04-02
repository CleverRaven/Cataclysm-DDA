#include "character.h"
#include "output.h"
#include "proficiency.h"
#include "ui_manager.h"

// Basic layout:
//
// +---------------------------Proficiencies---------------------------------+
// |  Category: < All >                                [/] filter  [?] help  |
// |  (category description)                                                 |
// |                                                                         |
// +--------------------------------+----------------------------------------+
// | >Proficiency 1                 |  Time to learn: 8 hours                |
// |  Proficiency 2                 |  Progress: 12%                         |
// |  Proficiency 3                 |  Category: Miscellaneous Crafts        |
// |  Proficiency 4                 |  Pre-requisite:                        |
// |  Proficiency 5                 |  Proficiency 2, Proficiency 4, ...     |
// |  Proficiency 6                 |  Description:                          |
// |  Proficiency 7                 |  Lorem ipsum etc........               |
// |  Proficiency 8                 |                                        |
// ...

struct _prof_window {
    input_context ctxt;
    weak_ptr_fast<ui_adaptor> ui;
    catacurses::window w_border;
    catacurses::window w_header;
    catacurses::window w_profs;
    catacurses::window w_details;

    const Character *u = nullptr;
    std::vector<const proficiency_category *> cats;
    std::vector<display_proficiency> all_profs;
    std::map<int, std::vector<display_proficiency *>> profs_by_cat;
    int max_rows = 0;
    int column_width = 0;
    int sel_prof = 0;
    int top_prof = 0;
    int current_cat = 0;

    _prof_window() = delete;
    _prof_window( _prof_window & ) = delete;
    _prof_window( _prof_window && ) = delete;
    explicit _prof_window( const Character *u ) : u( u ) {}
    shared_ptr_fast<ui_adaptor> create_or_get_ui_adaptor();
    void populate_categories();
    void init_ui_windows();
    void draw_borders();
    void draw_header();
    void draw_profs();
    void draw_details();
    void run();
};

void _prof_window::populate_categories()
{
    all_profs = u->display_proficiencies();
    cats.push_back( nullptr );
    for( const proficiency_category &pc : proficiency_category::get_all() ) {
        cats.push_back( &pc );
    }

    for( int i = 0; i < static_cast<int>( cats.size() ); i++ ) {
        for( display_proficiency &dp : all_profs ) {
            if( i == 0 || dp.id->prof_category() == cats[i]->id ) {
                profs_by_cat[i].push_back( &dp );
            }
        }
    }
}

void _prof_window::draw_details()
{
    werase( w_details );
    if( !profs_by_cat[current_cat].empty() ) {
        display_proficiency *p = profs_by_cat[current_cat][sel_prof];
        //NOLINTNEXTLINE(cata-use-named-point-constants)
        trim_and_print( w_details, point( 1, 0 ), column_width - 2, c_white,
                        string_format( "%s: %s", colorize( _( "Time to learn" ), c_magenta ),
                                       to_string( p->id->time_to_learn() ) ) );
        //NOLINTNEXTLINE(cata-use-named-point-constants)
        trim_and_print( w_details, point( 1, 1 ), column_width - 2, c_white,
                        string_format( "%s: %.2f%%", colorize( _( "Progress" ), c_magenta ), p->practice * 100.0f ) );
        trim_and_print( w_details, point( 1, 2 ), column_width - 2, c_white,
                        string_format( "%s: %s", colorize( _( "Category" ), c_magenta ),
                                       p->id->prof_category()->_name.translated() ) );
        trim_and_print( w_details, point( 1, 3 ), column_width - 2, c_white,
                        string_format( "%s:", colorize( _( "Pre-requisites" ), c_magenta ) ) );
        std::vector<std::string> reqs = foldstring( enumerate_as_string( p->id->required_proficiencies(),
        []( const proficiency_id & pid ) {
            return colorize( pid->name(), c_yellow );
        }, enumeration_conjunction::and_ ), column_width - 2 );
        for( int i = 0; i < static_cast<int>( reqs.size() ); i++ ) {
            trim_and_print( w_details, point( 1, i + 4 ), column_width - 1, c_white, reqs[i] );
        }
        fold_and_print( w_details, point( 1, 4 + reqs.size() ), column_width - 1, c_light_gray,
                        string_format( "%s: %s", colorize( _( "Description" ), c_magenta ), p->id->description() ) );
    }
    wnoutrefresh( w_details );
}

void _prof_window::draw_profs()
{
    werase( w_profs );
    for( int i = 0; i < max_rows &&
         i + top_prof < static_cast<int>( profs_by_cat[current_cat].size() ); i++ ) {
        nc_color colr = profs_by_cat[current_cat][i + top_prof]->color;
        if( sel_prof == i + top_prof ) {
            colr = hilite( colr );
            mvwputch( w_profs, point( 1, i ), c_yellow, '>' );
        }
        mvwprintz( w_profs, point( 2, i ), colr,
                   trim_by_length( profs_by_cat[current_cat][i + top_prof]->id->name(), column_width - 2 ) );
    }
    wnoutrefresh( w_profs );
}

void _prof_window::draw_header()
{
    const int w = catacurses::getmaxx( w_header );
    werase( w_header );
    std::string cat_title = ( current_cat == 0 ) ? _( "ALL" ) : cats[current_cat]->_name.translated();
    //NOLINTNEXTLINE(cata-use-named-point-constants)
    trim_and_print( w_header, point( 1, 0 ), w - 2, c_white, "%s:  %s %s %s", _( "Category" ),
                    colorize( "<", c_yellow ), colorize( cat_title, c_light_green ), colorize( ">", c_yellow ) );
    if( current_cat != 0 ) {
        //NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_header, point( 1, 1 ), w - 2, c_light_gray,
                        cats[current_cat]->_description.translated() );
    }
    std::string hint_txt = string_format( "[%s] %s   [%s] %s",
                                          colorize( ctxt.get_desc( "FILTER", 1 ), c_yellow ), _( "filter" ),
                                          colorize( ctxt.get_desc( "HELP_KEYBINDINGS", 1 ), c_yellow ), _( "help" ) );
    right_print( w_header, 0, 1, c_white, hint_txt );
    wnoutrefresh( w_header );
}

void _prof_window::draw_borders()
{
    const int w = catacurses::getmaxx( w_border );
    const int h = catacurses::getmaxy( w_border );
    draw_border( w_border, c_white, _( "Proficiencies" ), c_yellow );
    // horizontal header separator
    for( int i = 1; i < w - 1; i++ ) {
        mvwputch( w_border, point( i, 4 ), c_white, LINE_OXOX );
    }
    mvwputch( w_border, point( 0, 4 ), c_white, LINE_XXXO );
    mvwputch( w_border, point( w - 1, 4 ), c_white, LINE_XOXX );
    // vertical column separator
    for( int i = 5; i < h - 1; i++ ) {
        mvwputch( w_border, point( column_width + 1, i ), c_white, LINE_XOXO );
    }
    mvwputch( w_border, point( column_width + 1, 4 ), c_white, LINE_OXXX );
    mvwputch( w_border, point( column_width + 1, h - 1 ), c_white, LINE_XXOX );

    scrollbar()
    .border_color( c_white )
    .offset_x( 0 )
    .offset_y( 5 )
    .content_size( profs_by_cat[current_cat].size() )
    .viewport_pos( top_prof )
    .viewport_size( max_rows )
    .apply( w_border );

    wnoutrefresh( w_border );
}

void _prof_window::init_ui_windows()
{
    const int h = clamp( 20, 16, std::min( 32, TERMY ) );
    const int w = clamp( 80, 64, std::min( 80, TERMX ) );
    const point origin( TERMX / 2 - w / 2, TERMY / 2 - h / 2 );
    const int center = w / 2;
    max_rows = h - 6;
    column_width = center - 2;

    w_border  = catacurses::newwin( h, w, origin );
    //NOLINTNEXTLINE(cata-use-named-point-constants)
    w_header  = catacurses::newwin( 3, w - 2, origin + point( 1, 1 ) );
    w_profs   = catacurses::newwin( max_rows, column_width, origin + point( 1, 5 ) );
    w_details = catacurses::newwin( max_rows, column_width, origin + point( center + 1, 5 ) );
}

shared_ptr_fast<ui_adaptor> _prof_window::create_or_get_ui_adaptor()
{
    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( !current_ui ) {
        ui = current_ui = make_shared_fast<ui_adaptor>();
        current_ui->on_screen_resize( [this]( ui_adaptor & cui ) {
            init_ui_windows();
            cui.position_from_window( catacurses::stdscr );
        } );
        current_ui->mark_resize();
        current_ui->on_redraw( [this]( const ui_adaptor & ) {
            draw_borders();
            draw_header();
            draw_profs();
            draw_details();
        } );
    }
    return current_ui;
}

void _prof_window::run()
{
    if( !u ) {
        return;
    }

    ctxt = input_context( "PROFICIENCY_WINDOW" );
    ctxt.register_updown();
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "PREV_CATEGORY" );
    ctxt.register_action( "NEXT_CATEGORY" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "QUIT" );

    populate_categories();
    shared_ptr_fast<ui_adaptor> current_ui = create_or_get_ui_adaptor();
    init_ui_windows();

    bool done = false;
    while( !done ) {
        ui_manager::redraw();
        std::string action = ctxt.handle_input();
        if( action == "UP" ) {
            sel_prof--;
            if( sel_prof < 0 ) {
                sel_prof = std::max( static_cast<int>( profs_by_cat[current_cat].size() ) - 1, 0 );
            }
        } else if( action == "DOWN" ) {
            sel_prof++;
            if( sel_prof >= static_cast<int>( profs_by_cat[current_cat].size() ) ) {
                sel_prof = 0;
            }
        } else if( action == "PREV_CATEGORY" ) {
            current_cat--;
            if( current_cat < 0 ) {
                current_cat = std::max( static_cast<int>( cats.size() ) - 1, 0 );
            }
            sel_prof = 0;
            top_prof = 0;
        } else if( action == "NEXT_CATEGORY" ) {
            current_cat++;
            if( current_cat >= static_cast<int>( cats.size() ) ) {
                current_cat = 0;
            }
            sel_prof = 0;
            top_prof = 0;
        } else if( action == "FILTER" ) {
            // TODO: filter
        } else if( action == "QUIT" ) {
            done = true;
        }

        if( sel_prof < top_prof ) {
            top_prof = sel_prof;
        }
        if( sel_prof > top_prof + max_rows - 1 ) {
            top_prof = sel_prof - ( max_rows - 1 );
        }
        if( top_prof < 0 ) {
            top_prof = 0;
        }
    }
}

void show_proficiencies_window( const Character &u )
{
    _prof_window w( &u );
    w.run();
}