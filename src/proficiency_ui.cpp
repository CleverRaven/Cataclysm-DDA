#include "character.h"
#include "input_context.h"
#include "output.h"
#include "proficiency.h"
#include "string_input_popup.h"
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
// |  Proficiency 6                 |  Leads to:                             |
// |  Proficiency 7                 |  Proficiency 6, Proficiency 7, ...     |
// |  Proficiency 8                 |  Description:                          |
// |  Proficiency 9                 |  Lorem ipsum etc........               |
// |  Proficiency 10                |                                        |
// ...

using display_prof_deps = std::pair<display_proficiency, std::vector<proficiency_id>>;

struct prof_window {
    input_context ctxt;
    weak_ptr_fast<ui_adaptor> ui;
    catacurses::window w_border;
    catacurses::window w_header;
    catacurses::window w_profs;
    catacurses::window w_details;

    const Character *u = nullptr;
    std::vector<const proficiency_category *> cats;
    std::vector<display_prof_deps> all_profs;
    std::vector<display_prof_deps *> filtered_profs;
    std::map<int, std::vector<display_prof_deps *>> profs_by_cat;
    std::string filter_str;
    int max_rows = 0;
    int column_width = 0;
    int sel_prof = 0;
    int top_prof = 0;
    int current_cat = 0;

    prof_window() = delete;
    prof_window( prof_window & ) = delete;
    prof_window( prof_window && ) = delete;
    explicit prof_window( const Character *u ) : u( u ) {}
    shared_ptr_fast<ui_adaptor> create_or_get_ui_adaptor();
    void filter();
    std::vector<display_prof_deps *> &get_current_set();
    void populate_categories();
    void init_ui_windows();
    void draw_borders();
    void draw_header();
    void draw_profs();
    void draw_details();
    void run( std::optional<proficiency_id> default_selection = std::nullopt );
};

std::vector<display_prof_deps *> &prof_window::get_current_set()
{
    return filter_str.empty() ? profs_by_cat[current_cat] : filtered_profs;
}

void prof_window::filter()
{
    enum class _prof_filter_prefix {
        LEARNED,
        UNLEARNED,
        AS_PROGRESS,
        NONE
    };
    std::string qry = trim( filter_str );
    filtered_profs.clear();
    if( qry.empty() ) {
        return;
    }
    _prof_filter_prefix prefix = _prof_filter_prefix::NONE;
    if( qry.size() > 2 && qry[1] == ':' ) {
        switch( qry[0] ) {
            case 'l':
                prefix = _prof_filter_prefix::LEARNED;
                qry = qry.substr( 2 );
                break;
            case 'i':
                prefix = _prof_filter_prefix::UNLEARNED;
                qry = qry.substr( 2 );
                break;
            case 'p':
                prefix = _prof_filter_prefix::AS_PROGRESS;
                qry = qry.substr( 2 );
                break;
            default:
                break;
        }
    }
    float prog_val = 0.0f;
    if( prefix == _prof_filter_prefix::AS_PROGRESS ) {
        try {
            prog_val = std::stof( qry );
        } catch( const std::invalid_argument &e ) {
            // User mistyped query. Not severe enough for debugmsg.
            DebugLog( DebugLevel::D_WARNING, DebugClass::D_GAME ) <<
                    "Malformed proficiency query \"" << filter_str << "\": " << e.what();
            return;
        }
    }
    for( display_prof_deps &dp : all_profs ) {
        if( prefix == _prof_filter_prefix::AS_PROGRESS ) {
            if( prog_val < dp.first.practice * 100.0f ) {
                filtered_profs.push_back( &dp );
            }
            continue;
        }
        if( ( prefix == _prof_filter_prefix::LEARNED && !dp.first.known ) ||
            ( prefix == _prof_filter_prefix::UNLEARNED && dp.first.known ) ) {
            continue;
        }
        if( qry == "*" || lcmatch( dp.first.id->name(), qry ) ) {
            filtered_profs.push_back( &dp );
        }
    }
}

void prof_window::populate_categories()
{
    for( display_proficiency &p : u->display_proficiencies() ) {
        all_profs.push_back( { p, {} } );
    }
    for( const proficiency &p : proficiency::get_all() ) {
        for( const proficiency_id &pid : p.required_proficiencies() ) {
            auto iter = std::find_if( all_profs.begin(), all_profs.end(),
            [&pid]( const display_prof_deps & dpd ) {
                return dpd.first.id == pid;
            } );
            if( iter != all_profs.end() ) {
                iter->second.push_back( p.prof_id() );
            }
        }
    }
    cats.push_back( nullptr );
    for( const proficiency_category &pc : proficiency_category::get_all() ) {
        cats.push_back( &pc );
    }

    for( int i = 0; i < static_cast<int>( cats.size() ); i++ ) {
        for( display_prof_deps &dp : all_profs ) {
            if( i == 0 || dp.first.id->prof_category() == cats[i]->id ) {
                profs_by_cat[i].push_back( &dp );
            }
        }
    }
}

void prof_window::draw_details()
{
    werase( w_details );
    std::vector<display_prof_deps *> &cur_set = get_current_set();
    if( !cur_set.empty() ) {
        int y = 0;
        display_prof_deps *p = cur_set[sel_prof];
        // Time to learn
        std::string ttl = !p->first.id->can_learn() ? colorize( _( "Can't learn" ), c_dark_gray ) :
                          to_string( p->first.id->time_to_learn() );
        trim_and_print( w_details, point( 1, y++ ), column_width - 2, c_white,
                        string_format( "%s: %s", colorize( _( "Time to learn" ), c_magenta ), ttl ) );
        // Progress
        trim_and_print( w_details, point( 1, y++ ), column_width - 2, c_white,
                        string_format( "%s: %.2f%%", colorize( _( "Progress" ), c_magenta ), p->first.practice * 100.0f ) );
        // Category
        trim_and_print( w_details, point( 1, y++ ), column_width - 2, c_white,
                        string_format( "%s: %s", colorize( _( "Category" ), c_magenta ),
                                       p->first.id->prof_category()->_name.translated() ) );
        // Pre-requisites
        trim_and_print( w_details, point( 1, y++ ), column_width - 2, c_white,
                        string_format( "%s:", colorize( _( "Pre-requisites" ), c_magenta ) ) );
        std::vector<std::string> reqs =
            foldstring( enumerate_as_string( p->first.id->required_proficiencies(),
        []( const proficiency_id & pid ) {
            return colorize( pid->name(), c_yellow );
        }, enumeration_conjunction::and_ ), column_width - 2 );
        for( auto &req : reqs ) {
            trim_and_print( w_details, point( 1, y++ ), column_width - 1, c_white, req );
        }
        // Leads to
        trim_and_print( w_details, point( 1, y++ ), column_width - 2, c_white,
                        string_format( "%s:", colorize( _( "Leads to" ), c_magenta ) ) );
        std::vector<std::string> leads = foldstring( enumerate_as_string( p->second,
        []( const proficiency_id & pid ) {
            return colorize( pid->name(), c_yellow );
        }, enumeration_conjunction::and_ ), column_width - 2 );
        for( auto &lead : leads ) {
            trim_and_print( w_details, point( 1, y++ ), column_width - 1, c_white, lead );
        }
        // Description
        fold_and_print( w_details, point( 1, y ), column_width - 1, c_light_gray,
                        string_format( "%s: %s", colorize( _( "Description" ), c_magenta ), p->first.id->description() ) );
    }
    wnoutrefresh( w_details );
}

void prof_window::draw_profs()
{
    werase( w_profs );
    std::vector<display_prof_deps *> &cur_set = get_current_set();
    for( int i = 0; i < max_rows && i + top_prof < static_cast<int>( cur_set.size() ); i++ ) {
        nc_color colr = cur_set[i + top_prof]->first.color;
        if( sel_prof == i + top_prof ) {
            colr = hilite( colr );
            mvwputch( w_profs, point( 1, i ), c_yellow, '>' );
        }
        mvwprintz( w_profs, point( 2, i ), colr,
                   trim_by_length( cur_set[i + top_prof]->first.id->name(), column_width - 2 ) );
    }
    wnoutrefresh( w_profs );
}

void prof_window::draw_header()
{
    const int w = catacurses::getmaxx( w_header );
    werase( w_header );
    std::string cat_title = !filter_str.empty() ? _( "FILTERED" ) :
                            ( current_cat == 0 ) ? _( "ALL" ) : cats[current_cat]->_name.translated();
    //NOLINTNEXTLINE(cata-use-named-point-constants)
    trim_and_print( w_header, point( 1, 0 ), w - 2, c_white, "%s:  %s %s %s", _( "Category" ),
                    colorize( "<", c_yellow ), colorize( cat_title, c_light_green ), colorize( ">", c_yellow ) );
    if( current_cat != 0 && filter_str.empty() ) {
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

void prof_window::draw_borders()
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
    .content_size( get_current_set().size() )
    .viewport_pos( top_prof )
    .viewport_size( max_rows )
    .apply( w_border );

    wnoutrefresh( w_border );
}

void prof_window::init_ui_windows()
{
    const int h = std::min( 28, TERMY );
    const int w = std::min( 120, TERMX );
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

shared_ptr_fast<ui_adaptor> prof_window::create_or_get_ui_adaptor()
{
    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( !current_ui ) {
        ui = current_ui = make_shared_fast<ui_adaptor>();
        current_ui->on_screen_resize( [this]( ui_adaptor & cui ) {
            init_ui_windows();
            cui.position_from_window( w_border );
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

void prof_window::run( std::optional<proficiency_id> default_selection )
{
    if( !u ) {
        return;
    }

    ctxt = input_context( "PROFICIENCY_WINDOW" );
    ctxt.register_navigate_ui_list();
    ctxt.register_leftright();
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "PREV_CATEGORY" );
    ctxt.register_action( "NEXT_CATEGORY" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "QUIT" );

    populate_categories();
    shared_ptr_fast<ui_adaptor> current_ui = create_or_get_ui_adaptor();

    if( default_selection ) {
        std::vector<display_prof_deps *> &cur_set = get_current_set();
        for( int i = 0; i < static_cast<int>( cur_set.size() ); i++ ) {
            if( cur_set[i]->first.id == default_selection.value() ) {
                sel_prof = i;
                break;
            }
        }
    }

    bool done = false;
    while( !done ) {
        ui_manager::redraw();
        std::string action = ctxt.handle_input();
        std::vector<display_prof_deps *> &cur_set = get_current_set();
        if( navigate_ui_list( action, sel_prof, 10, cur_set.size(), true ) ) {
        } else if( filter_str.empty() && ( action == "LEFT" || action == "PREV_CATEGORY" ||
                                           action == "PREV_TAB" ||
                                           action == "RIGHT" || action == "NEXT_CATEGORY" || action == "NEXT_TAB" ) ) {
            current_cat = inc_clamp_wrap( current_cat,
                                          action == "RIGHT" || action == "NEXT_CATEGORY" || action == "NEXT_TAB", cats.size() );
            sel_prof = 0;
            top_prof = 0;
        } else if( action == "FILTER" ) {
            //~ Refers to single-character search prefixes, like p: or s:
            std::string desc( _( "Available prefixes:" ) );
            desc += "\n  ";
            desc += _( "<color_c_light_cyan>l:</color>  Learned proficiencies" );
            desc += "\n  ";
            desc += _( "<color_c_light_cyan>i:</color>  In-progress proficiencies" );
            desc += "\n  ";
            desc += _( "<color_c_light_cyan>p:</color>  Returns proficiencies above the given progress.  "
                       "ex: <color_c_yellow>p:95.00</color>" );
            desc += "\n\n";
            desc += _( "By default, the filter applies to proficiencies containing the given string.  "
                       "Using just a glob <color_c_yellow>*</color> matches anything.\n"
                       "ex: <color_c_yellow>l:*</color> returns all learned proficiencies." );
            desc += "\n\n";

            string_input_popup popup;
            popup
            .title( _( "Search:" ) )
            .width( 78 )
            .description( desc )
            .desc_color( c_light_gray )
            .identifier( "proficiency_filter" )
            .hist_use_uilist( false )
            .edit( filter_str );

            if( popup.confirmed() ) {
                sel_prof = 0;
                top_prof = 0;
                current_cat = 0;
                filter();
            }
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

void show_proficiencies_window( const Character &u,
                                std::optional<proficiency_id> default_selection )
{
    prof_window w( &u );
    w.run( default_selection );
}
