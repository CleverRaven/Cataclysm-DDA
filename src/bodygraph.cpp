#include "avatar.h"
#include "bodygraph.h"
#include "bodypart.h"
#include "cursesdef.h"
#include "damage.h"
#include "generic_factory.h"
#include "input.h"
#include "ui_manager.h"

#define BPGRAPH_MAXROWS 20
#define BPGRAPH_MAXCOLS 40

static const bodygraph_id bodygraph_full_body( "full_body" );

namespace
{

generic_factory<bodygraph> bodygraph_factory( "bodygraph" );

} // namespace

/** @relates string_id */
template<>
const bodygraph &string_id<bodygraph>::obj() const
{
    return bodygraph_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<bodygraph>::is_valid() const
{
    return bodygraph_factory.is_valid( *this );
}

void bodygraph::load_bodygraphs( const JsonObject &jo, const std::string &src )
{
    bodygraph_factory.load( jo, src );
}

void bodygraph::reset()
{
    bodygraph_factory.reset();
}

const std::vector<bodygraph> &bodygraph::get_all()
{
    return bodygraph_factory.get_all();
}

void bodygraph::finalize_all()
{
    bodygraph_factory.finalize();
}

void bodygraph::check_all()
{
    bodygraph_factory.check();
}

void bodygraph::load( const JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "parent_bodypart", parent_bp );
    optional( jo, was_loaded, "fill_sym", fill_sym );
    if( jo.has_string( "fill_color" ) ) {
        fill_color = color_from_string( jo.get_string( "fill_color" ) );
    }

    if( !was_loaded || jo.has_array( "rows" ) ) {
        rows.clear();
        for( const JsonValue jval : jo.get_array( "rows" ) ) {
            if( !jval.test_string() ) {
                jval.throw_error( "\"rows\" array must contain string values." );
            } else {
                std::wstring wrow = utf8_to_wstr( jval.get_string() );
                std::vector<std::string> row;
                for( const wchar_t wc : wrow ) {
                    row.emplace_back( wstr_to_utf8( std::wstring( 1, wc ) ) );
                }
                rows.emplace_back( row );
            }
        }
    }

    if( !was_loaded || jo.has_object( "parts" ) ) {
        parts.clear();
        for( const JsonMember memb : jo.get_object( "parts" ) ) {
            const std::string sym = memb.name();
            JsonObject mobj = memb.get_object();
            bodygraph_part bpg;
            optional( mobj, false, "body_parts", bpg.bodyparts );
            optional( mobj, false, "sub_body_parts", bpg.sub_bodyparts );
            optional( mobj, false, "sym", bpg.sym, fill_sym );
            if( mobj.has_string( "select_color" ) ) {
                bpg.sel_color = color_from_string( mobj.get_string( "select_color" ) );
            } else {
                bpg.sel_color = fill_color;
            }
            optional( mobj, false, "nested_graph", bpg.nested_graph );
            parts.emplace( sym, bpg );
        }
    }
}

void bodygraph::finalize()
{
    if( rows.size() > BPGRAPH_MAXROWS ) {
        debugmsg( "body_graph \"%s\" defines more rows than the maximum (%d).", id.c_str(),
                  BPGRAPH_MAXROWS );
    }

    int width = -1;
    bool w_warned = false;
    for( std::vector<std::string> &r : rows ) {
        int w = r.size();
        if( width == -1 ) {
            width = w;
        } else if( !w_warned && width != w ) {
            debugmsg( "body_graph \"%s\" defines rows with different widths.", id.c_str() );
            w_warned = true;
        } else if( !w_warned && w > BPGRAPH_MAXCOLS ) {
            debugmsg( "body_graph \"%s\" defines rows with more columns than the maximum (%d).", id.c_str(),
                      BPGRAPH_MAXCOLS );
            w_warned = true;
        }
        for( int i = ( BPGRAPH_MAXCOLS - w ) / 2; i > 0; i-- ) {
            r.insert( r.begin(), " " );
        }
        for( int i = r.size(); i <= BPGRAPH_MAXCOLS; i++ ) {
            r.emplace_back( " " );
        }
    }

    for( int i = ( BPGRAPH_MAXROWS - rows.size() ) / 2; i > 0; i-- ) {
        std::vector<std::string> r;
        for( int j = 0; j < BPGRAPH_MAXCOLS; j++ ) {
            r.emplace_back( " " );
        }
        rows.insert( rows.begin(), r );
    }

    for( int i = rows.size(); i <= BPGRAPH_MAXROWS; i++ ) {
        std::vector<std::string> r;
        for( int j = 0; j < BPGRAPH_MAXCOLS; j++ ) {
            r.emplace_back( " " );
        }
        rows.emplace_back( r );
    }
}

void bodygraph::check() const
{
    if( parts.empty() ) {
        debugmsg( "body_graph \"%s\" defined without parts.", id.c_str() );
    }
    for( const auto &bgp : parts ) {
        if( utf8_width( bgp.first ) > 1 ) {
            debugmsg( "part \"%s\" in body_graph \"%s\" is more than 1 character.", bgp.first, id.c_str() );
        }
        if( bgp.second.bodyparts.empty() && bgp.second.sub_bodyparts.empty() ) {
            debugmsg( "part \"%s\" in body_graph \"%s\" contains no body_parts or sub_body_parts definitions.",
                      bgp.first, id.c_str() );
        }
    }
}

/***************************** Display routines *****************************/

/* Basic layout. Not accurate or final, but illustrates the basic idea.

     parts list                      body graph                  part info
<- variable width -><------------ fixed width (40) --------><- variable width ->

+------------------+----------------Body Status-------------+------------------+
| >Head            |                                        | Worn:            |
|  Torso           |                     O                  |  Baseball cap,   |
|  L. Arm          |                   #####                |  Eyeglasses      |
|  R. Arm          |                  #######               | Coverage: ~60%   |
|  L. Hand         |                 ## ### ##              | Encumbrance: 8   |
|  R. Hand         |                 #  ###  #              | Protection:      |
...

*/

#define BPGRAPH_HEIGHT 24

bodygraph_info::bodygraph_info()
{
    std::vector<std::string> worn_names = {};
    avg_coverage = 0;
    total_encumbrance = 0;
    worst_case = resistances();
    median_case = resistances();
    best_case = resistances();
}

struct bodygraph_display {
    bodygraph_id id;
    input_context ctxt;
    weak_ptr_fast<ui_adaptor> ui;
    catacurses::window w_border;
    catacurses::window w_partlist;
    catacurses::window w_graph;
    catacurses::window w_info;
    std::vector<std::tuple<bodypart_id, const sub_body_part_type *, const bodygraph_part *>> partlist;
    bodygraph_info info;
    int partlist_width = 0;
    int info_width = 0;
    int sel_part = 0;
    int top_part = 0;

    bodygraph_display() = delete;
    bodygraph_display( const bodygraph_display & ) = delete;
    bodygraph_display( const bodygraph_display && ) = delete;
    explicit bodygraph_display( const bodygraph_id &id );

    shared_ptr_fast<ui_adaptor> create_or_get_ui_adaptor();
    void prepare_partlist();
    void prepare_infolist();
    void init_ui_windows();
    void draw_borders();
    void draw_partlist();
    void draw_graph();
    void draw_info();
    void display();
};

bodygraph_display::bodygraph_display( const bodygraph_id &id ) : id( id ), ctxt( "BODYGRAPH" )
{
    if( id.is_null() ) {
        this->id = bodygraph_full_body;
    }

    ctxt.register_directions();
    ctxt.register_action( "SCROLL_INFOBOX_UP" );
    ctxt.register_action( "SCROLL_INFOBOX_DOWN" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
}

void bodygraph_display::init_ui_windows()
{
    partlist_width = 18;
    info_width = 18;
    int avail_w = clamp( TERMX - ( BPGRAPH_MAXCOLS + 40 ), 0, 20 );
    for( int i = avail_w; i > 0; i-- ) {
        if( i % 4 == 0 ) {
            partlist_width++;
        } else {
            info_width++;
        }
    }
    int total_w = partlist_width + info_width + BPGRAPH_MAXCOLS + 4;
    point top_left( TERMX / 2 - total_w / 2, TERMY / 2 - BPGRAPH_HEIGHT / 2 );

    w_border = catacurses::newwin( BPGRAPH_HEIGHT, total_w, top_left );
    //NOLINTNEXTLINE(cata-use-named-point-constants)
    w_partlist = catacurses::newwin( BPGRAPH_HEIGHT - 2, partlist_width, top_left + point( 1, 1 ) );
    w_graph = catacurses::newwin( BPGRAPH_MAXROWS, BPGRAPH_MAXCOLS,
                                  top_left + point( 2 + partlist_width, 2 ) );
    w_info = catacurses::newwin( BPGRAPH_HEIGHT - 2, info_width,
                                 top_left + point( 3 + partlist_width + BPGRAPH_MAXCOLS, 1 ) );
}

void bodygraph_display::draw_borders()
{
    draw_border( w_border, c_white );
    // vertical header separators
    for( int i = 1; i < BPGRAPH_HEIGHT - 1; i++ ) {
        mvwputch( w_border, point( 1 + partlist_width, i ), c_white, LINE_XOXO );
        mvwputch( w_border, point( 2 + partlist_width + BPGRAPH_MAXCOLS, i ), c_white, LINE_XOXO );
    }
    mvwputch( w_border, point( 1 + partlist_width, 0 ), c_white, LINE_OXXX );
    mvwputch( w_border, point( 2 + partlist_width + BPGRAPH_MAXCOLS, 0 ), c_white, LINE_OXXX );
    mvwputch( w_border, point( 1 + partlist_width, BPGRAPH_HEIGHT - 1 ), c_white, LINE_XXOX );
    mvwputch( w_border, point( 2 + partlist_width + BPGRAPH_MAXCOLS, BPGRAPH_HEIGHT - 1 ), c_white,
              LINE_XXOX );

    const int first_win_width = partlist_width;
    auto center_txt_start = [&first_win_width]( const std::string & txt ) {
        return 2 + first_win_width + ( BPGRAPH_MAXCOLS / 2 - utf8_width( txt, true ) / 2 );
    };

    // window title
    const std::string title_txt = string_format( "< %s >", colorize( _( "Body status" ), c_yellow ) );
    trim_and_print( w_border, point( center_txt_start( title_txt ), 0 ),
                    BPGRAPH_MAXCOLS, c_white, title_txt );

    // body part subtitle
    if( id->parent_bp.has_value() ) {
        const std::string bpname = string_format( "\\_ %s _/",
                                   colorize( to_upper_case( id->parent_bp.value()->name.translated() ), c_yellow ) );
        trim_and_print( w_border, point( center_txt_start( bpname ), 1 ),
                        BPGRAPH_MAXCOLS, c_white, bpname );
    }

    scrollbar()
    .border_color( c_white )
    .offset_x( 0 )
    .offset_y( 1 )
    .content_size( partlist.size() )
    .viewport_pos( sel_part )
    .viewport_size( BPGRAPH_HEIGHT - 2 )
    .apply( w_border );

    wnoutrefresh( w_border );
}

void bodygraph_display::draw_partlist()
{
    werase( w_partlist );
    int y = 0;
    for( int i = top_part; y < BPGRAPH_HEIGHT - 2 && i < static_cast<int>( partlist.size() ); i++ ) {
        const auto bgt = partlist[i];
        std::string txt = !std::get<1>( bgt ) ?
                          std::get<0>( bgt )->name.translated() :
                          std::get<1>( bgt )->name.translated();
        txt = trim_by_length( uppercase_first_letter( txt ), partlist_width - 2 );
        txt = left_justify( txt, partlist_width - 2, true );
        txt.insert( 0, colorize( i == sel_part ? ">" : " ", c_yellow ) );
        txt.append( !std::get<2>( bgt )->nested_graph.is_null() ?
                    colorize( ">", i == sel_part ? hilite( c_light_green ) : c_light_green ) : " " );
        trim_and_print( w_partlist, point( 0, y++ ), partlist_width,
                        i == sel_part ? hilite( c_white ) : c_white, txt );
    }
    wnoutrefresh( w_partlist );
}

void bodygraph_display::draw_graph()
{
    werase( w_graph );
    const bodygraph_part *selected_graph = std::get<2>( partlist[sel_part] );
    std::string selected_sym;
    if( !!selected_graph ) {
        for( const auto &bgp : id->parts ) {
            if( selected_graph == &bgp.second ) {
                selected_sym = bgp.first;
            }
        }
    }
    for( unsigned i = 0; i < id->rows.size() && i < BPGRAPH_MAXROWS; i++ ) {
        for( unsigned j = 0; j < id->rows[i].size() && j < BPGRAPH_MAXCOLS; j++ ) {
            std::string sym = id->fill_sym;
            nc_color col = id->fill_color;
            auto iter = id->parts.find( id->rows[i][j] );
            if( iter != id->parts.end() ) {
                sym = iter->second.sym;
            }
            if( id->rows[i][j] == " " ) {
                col = c_unset;
                sym = " ";
            } else if( id->rows[i][j] == selected_sym ) {
                col = id->parts.at( selected_sym ).sel_color;
            }
            mvwputch( w_graph, point( j, i ), col, sym );
        }
    }
    wnoutrefresh( w_graph );
}

void bodygraph_display::draw_info()
{
    //TODO
}

void bodygraph_display::prepare_partlist()
{
    partlist.clear();
    for( const auto &bgp : id->parts ) {
        for( const bodypart_id &bid : bgp.second.bodyparts ) {
            partlist.emplace_back( std::make_tuple( bid, static_cast<const sub_body_part_type *>( nullptr ),
                                                    &bgp.second ) );
        }
        for( const sub_bodypart_id &sid : bgp.second.sub_bodyparts ) {
            partlist.emplace_back( std::make_tuple( sid->parent.id(), &*sid, &bgp.second ) );
        }
    }
    std::sort( partlist.begin(), partlist.end(),
               []( const std::tuple<bodypart_id, const sub_body_part_type *, const bodygraph_part *> &a,
    const std::tuple<bodypart_id, const sub_body_part_type *, const bodygraph_part *> &b ) {
        if( !!std::get<1>( a ) && !!std::get<1>( b ) ) {
            return std::get<1>( a )->name.translated_lt( std::get<1>( b )->name );
        }
        if( std::get<0>( a )->name.translated_ne( std::get<0>( b )->name ) ) {
            return std::get<0>( a )->name.translated_lt( std::get<0>( b )->name );
        }
        if( !std::get<1>( a ) != !std::get<1>( b ) ) {
            return !std::get<1>( a );
        }
        return true;
    } );
}

void bodygraph_display::prepare_infolist()
{
    // reset info for a new read
    info = bodygraph_info();

    const bodypart_id &bp = std::get<0>( partlist[sel_part] );

    // sbps will either be a group for a body part and we'll need to do averages OR a single sub part
    std::vector<sub_bodypart_id> sub_parts;

    // should maybe be not just the avater I'm not sure how you are getting character passed in
    const avatar &p = get_avatar();

    // this might be null need to test for nullbp as this all continues
    if( std::get<1>( partlist[sel_part] ) != nullptr ) {
        sub_parts.push_back( std::get<1>( partlist[sel_part] )->id );
    } else {
        for( const sub_bodypart_id &sbp : bp->sub_parts ) {
            // don't worry about secondary sub parts would just make things confusing
            if( !sbp->secondary ) {
                sub_parts.push_back( sbp );
            }
        }
    }

    p.worn.prepare_bodymap_info( info, bp, sub_parts, p );
}

shared_ptr_fast<ui_adaptor> bodygraph_display::create_or_get_ui_adaptor()
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
            draw_partlist();
            draw_graph();
            draw_info();
        } );
    }
    return current_ui;
}

void bodygraph_display::display()
{
    prepare_partlist();
    prepare_infolist();
    shared_ptr_fast<ui_adaptor> current_ui = create_or_get_ui_adaptor();

    bool done = false;
    while( !done ) {
        ui_manager::redraw();
        std::string action = ctxt.handle_input();
        if( action == "QUIT" || ( action == "LEFT" && id != bodygraph_full_body ) ) {
            done = true;
        } else if( action == "CONFIRM" || action == "RIGHT" ) {
            if( !!std::get<2>( partlist[sel_part] ) ) {
                bodygraph_id nextgraph = std::get<2>( partlist[sel_part] )->nested_graph;
                if( !nextgraph.is_null() ) {
                    display_bodygraph( nextgraph );
                    prepare_infolist();
                }
            }
        } else if( action == "UP" ) {
            sel_part--;
            if( sel_part < 0 ) {
                sel_part = partlist.size() - 1;
            }
            prepare_infolist();
        } else if( action == "DOWN" ) {
            sel_part++;
            if( sel_part >= static_cast<int>( partlist.size() ) ) {
                sel_part = 0;
            }
            prepare_infolist();
        }
        if( sel_part < top_part ) {
            top_part = sel_part;
        } else if( sel_part >= top_part + ( BPGRAPH_HEIGHT - 2 ) ) {
            top_part = sel_part - ( BPGRAPH_HEIGHT - 3 );
        }
    }
}

void display_bodygraph( const bodygraph_id &id )
{
    bodygraph_display bgd( id );
    bgd.display();
}
