#include "panels.h"

#include <algorithm>
#include <cstddef>
#include <iosfwd>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include "avatar.h"
#include "cached_options.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "cursesdef.h"
#include "display.h"
#include "debug.h"
#include "game.h"
#include "game_constants.h"
#include "game_ui.h"
#include "input_context.h"
#include "json.h"
#include "map.h"
#include "messages.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "path_info.h"
#include "point.h"
#include "string_formatter.h"
#include "type_id.h"
#include "ui_manager.h"
#include "widget.h"

//TODO: Use these variables again later to enhance the limb health widget.
//static const efftype_id effect_got_checked( "got_checked" );
//static const efftype_id effect_mending( "mending" );
//static const flag_id json_flag_SPLINT( "SPLINT" );
//static const flag_id json_flag_THERMOMETER( "THERMOMETER" );

//static const string_id<behavior::node_t> behavior_node_t_npc_needs( "npc_needs" );

//static const trait_id trait_NOPAIN( "NOPAIN" );

// constructor
window_panel::window_panel(
    const std::function<void( const draw_args & )> &draw_func,
    const std::string &id, const translation &nm, const int ht, const int wd,
    const bool default_toggle_, const std::function<bool()> &render_func,
    const bool force_draw )
    : draw( [draw_func]( const draw_args & d ) -> int { draw_func( d ); return 0; } ),
      render( render_func ), toggle( default_toggle_ ), always_draw( force_draw ),
      height( ht ), width( wd ), id( id ), name( nm )
{
    wgt = widget_id::NULL_ID();
}

window_panel::window_panel(
    const std::string &id, const translation &nm, const int ht, const int wd,
    const bool default_toggle_, const std::function<bool()> &render_func,
    const bool force_draw )
    : draw( []( const draw_args & ) -> int { return 0; } ),
      render( render_func ), toggle( default_toggle_ ), always_draw( force_draw ),
      height( ht ), width( wd ), id( id ), name( nm )
{
    wgt = widget_id::NULL_ID();
}

// ====================================
// panels prettify and helper functions
// ====================================

static std::string trunc_ellipse( const std::string &input, unsigned int trunc )
{
    if( utf8_width( input ) > static_cast<int>( trunc ) ) {
        return utf8_truncate( input, trunc - 1 ) + "…";
    }
    return input;
}

static int get_wgt_height( const widget_id &wgt )
{
    if( wgt->_widgets.empty() || wgt->_arrange == "columns" || wgt->_arrange == "minimum_columns" ) {
        return wgt->_height > 0 ? wgt->_height : 1;
    }
    int h = 0;
    for( const widget_id &w : wgt->_widgets ) {
        h += get_wgt_height( w );
    }
    return h;
}

int window_panel::get_height() const
{
    if( height == -1 ) {
        if( pixel_minimap_option ) {
            return  get_option<int>( "PIXEL_MINIMAP_HEIGHT" ) > 0 ?
                    get_option<int>( "PIXEL_MINIMAP_HEIGHT" ) :
                    width / 2;
        } else {
            return 0;
        }
    }
    if( wgt.is_valid() ) {
        return get_wgt_height( wgt );
    }
    return height;
}

int window_panel::get_width() const
{
    return width;
}

const std::string &window_panel::get_id() const
{
    return id;
}

std::string window_panel::get_name() const
{
    return name.translated();
}

void window_panel::set_widget( const widget_id &w )
{
    wgt = w;
}

const widget_id &window_panel::get_widget() const
{
    return wgt;
}

void window_panel::set_draw_func( const std::function<int( const draw_args & )> &draw_func )
{
    draw = draw_func;
}

panel_layout::panel_layout( const translation &_name, const std::vector<window_panel> &_panels )
    : _name( _name ), _panels( _panels )
{
}

std::string panel_layout::name() const
{
    return _name.translated();
}

const std::vector<window_panel> &panel_layout::panels() const
{
    return _panels;
}

std::vector<window_panel> &panel_layout::panels()
{
    return _panels;
}

void overmap_ui::draw_overmap_chunk( const catacurses::window &w_minimap, const avatar &you,
                                     const tripoint_abs_omt &global_omt, const point &start_input,
                                     const int width, const int height )
{
    // Map is centered on curs - typically player's global_omt_location
    const point_abs_omt curs = global_omt.xy();
    const tripoint_abs_omt targ = you.get_active_mission_target();
    bool drew_mission = targ == overmap::invalid_tripoint;
    const int start_y = start_input.y;
    const int start_x = start_input.x;
    const point mid( width / 2, height / 2 );
    map &here = get_map();
    const int sight_points = you.overmap_sight_range( g->light_level( you.posz() ) );

    // i scans across width, with 0 in the middle(ish)
    //     -(w/2) ... w-(w/2)-1
    // w:9   -4 ... 4
    // w:10  -5 ... 4
    // w:11  -5 ... 5
    // w:12  -6 ... 5
    // w:13  -6 ... 6
    for( int i = -( width / 2 ); i <= width - ( width / 2 ) - 1; i++ ) {
        // j scans across height, with 0 in the middle(ish)
        // (same algorithm)
        for( int j = -( height / 2 ); j <= height - ( height / 2 ) - 1; j++ ) {
            // omp is the current overmap point, at the current z-level
            const tripoint_abs_omt omp( curs + point( i, j ), here.get_abs_sub().z() );
            // Terrain color and symbol to use for this point
            nc_color ter_color;
            std::string ter_sym;
            const bool seen = overmap_buffer.seen( omp );
            if( overmap_buffer.has_note( omp ) ) {
                const std::string &note_text = overmap_buffer.note( omp );
                std::pair<std::string, nc_color> sym_color = display::overmap_note_symbol_color( note_text );
                ter_sym = sym_color.first;
                ter_color = sym_color.second;
            } else if( !seen ) {
                // Always gray # for unseen
                ter_sym = "#";
                ter_color = c_dark_gray;
            } else if( overmap_buffer.has_vehicle( omp ) ) {
                ter_color = c_cyan;
                ter_sym = overmap_buffer.get_vehicle_ter_sym( omp );
            } else {
                // Otherwise, get symbol and color appropriate for the terrain
                const oter_id &cur_ter = overmap_buffer.ter( omp );
                ter_sym = cur_ter->get_symbol();
                if( overmap_buffer.is_explored( omp ) ) {
                    ter_color = c_dark_gray;
                } else {
                    ter_color = cur_ter->get_color();
                }
            }
            if( !drew_mission && targ.xy() == omp.xy() ) {
                // If there is a mission target, and it's not on the same
                // overmap terrain as the player character, mark it.
                // TODO: Inform player if the mission is above or below
                drew_mission = true;
                if( i != 0 || j != 0 ) {
                    ter_color = red_background( ter_color );
                }
            }
            // TODO: Build colorized string instead of writing directly to window
            if( i == 0 && j == 0 ) {
                // Highlight player character position in center of minimap
                mvwputch_hi( w_minimap, mid + point( start_x, start_y ), ter_color, ter_sym );
            } else {
                mvwputch( w_minimap, mid + point( i + start_x, j + start_y ), ter_color,
                          ter_sym );
            }

            if( i < -1 || i > 1 || j < -1 || j > 1 ) {
                // Show hordes on minimap, leaving a one-tile space around the player
                int horde_size = overmap_buffer.get_horde_size( omp );
                if( horde_size >= HORDE_VISIBILITY_SIZE &&
                    overmap_buffer.seen( omp ) && you.overmap_los( omp, sight_points ) ) {
                    mvwputch( w_minimap, mid + point( i + start_x, j + start_y ), c_green,
                              horde_size > HORDE_VISIBILITY_SIZE * 2 ? 'Z' : 'z' );
                }
            }
        }
    }

    // When the mission marker is not visible within the current overmap extents,
    // draw an arrow at the edge of the map pointing in the general mission direction.
    // TODO: Replace `drew_mission` with a function like `is_mission_on_map`
    if( !drew_mission ) {
        char glyph = '*';
        if( targ.z() > you.posz() ) {
            glyph = '^';
        } else if( targ.z() < you.posz() ) {
            glyph = 'v';
        }
        const point arrow = display::mission_arrow_offset( you, width, height );
        mvwputch( w_minimap, arrow + point( start_x, start_y ), c_red, glyph );
    }
}

static void decorate_panel( const std::string_view name, const catacurses::window &w )
{
    werase( w );
    draw_border( w );

    static const char *title_prefix = " ";
    const std::string_view title = name;
    static const char *title_suffix = " ";
    static const std::string full_title = string_format( "%s%s%s",
                                          title_prefix, title, title_suffix );
    const int start_pos = center_text_pos( full_title, 0, getmaxx( w ) - 1 );
    mvwprintz( w, point( start_pos, 0 ), c_white, title_prefix );
    wprintz( w, c_light_red, title );
    wprintz( w, c_white, title_suffix );
}

static void draw_messages( const draw_args &args )
{
    const catacurses::window &w = args._win;

    werase( w );
    int line = getmaxy( w ) - 1;
    int maxlength = getmaxx( w );
    Messages::display_messages( w, 1, 0 /*topline*/, maxlength - 1, line );
    wnoutrefresh( w );
}

#if defined(TILES)
static void draw_mminimap( const draw_args &args )
{
    const catacurses::window &w = args._win;

    werase( w );
    g->draw_pixel_minimap( w );
    wnoutrefresh( w );
}
#endif

// ============
// INITIALIZERS
// ============

bool default_render()
{
    return true;
}

// Initialize custom panels from a given "sidebar" style widget
static std::vector<window_panel> initialize_default_custom_panels( const widget &wgt )
{
    std::vector<window_panel> ret;

    // Use defined width, or at least 16
    const int width = std::max( wgt._width, 16 );

    // Add window panel for each child widget
    for( const widget_id &row_wid : wgt._widgets ) {
        widget row_widget = row_wid.obj();
        ret.emplace_back( row_widget.get_window_panel( width ) );
    }

    // Add compass, message log, and map to fill remaining space
    // TODO: Make these into proper widgets
    ret.emplace_back( draw_messages, "Log", to_translation( "Log" ),
                      -2, width, true );
#if defined(TILES)
    ret.emplace_back( draw_mminimap, "Map", to_translation( "Map" ),
                      -1, width, true, default_render, true );
#endif // TILES

    return ret;
}

static std::map<std::string, panel_layout> initialize_default_panel_layouts()
{
    std::map<std::string, panel_layout> ret;
    // Add panel layout for each "sidebar" widget
    for( const widget &wgt : widget::get_all() ) {
        if( wgt._style == "sidebar" ) {
            ret.emplace( wgt.getId().str(),
                         panel_layout( wgt._label, initialize_default_custom_panels( wgt ) ) );
        }
    }

    return ret;
}

panel_manager::panel_manager()
{
    current_layout_id = "legacy_labels_sidebar";
    // Set empty layouts; these will be populated by load()
    layouts = std::map<std::string, panel_layout>();
}

panel_layout &panel_manager::get_current_layout()
{
    auto kv = layouts.find( current_layout_id );
    if( kv != layouts.end() ) {
        return kv->second;
    }
    debugmsg( "Invalid current panel layout, defaulting to classic" );
    current_layout_id = "legacy_classic_sidebar";
    return get_current_layout();
}

widget *panel_manager::get_current_sidebar()
{
    panel_layout layout = get_current_layout();
    const std::string name = layout.name();
    return get_sidebar( name );
}

widget *panel_manager::get_sidebar( const std::string &name )
{
    widget *w = nullptr;
    for( const widget &wgt : widget::get_all() ) {
        if( wgt._style == "sidebar" && wgt._label.translated() == name ) {
            w = const_cast<widget *>( &wgt );
            break;
        }
    }
    return w;
}

std::string panel_manager::get_current_layout_id() const
{
    return current_layout_id;
}

int panel_manager::get_width_right() const
{
    if( get_option<std::string>( "SIDEBAR_POSITION" ) == "left" ) {
        return width_left;
    }
    return width_right;
}

int panel_manager::get_width_left() const
{
    if( get_option<std::string>( "SIDEBAR_POSITION" ) == "left" ) {
        return width_right;
    }
    return width_left;
}

void panel_manager::init()
{
    layouts = initialize_default_panel_layouts();
    load();
    update_offsets( get_current_layout().panels().begin()->get_width() );
    if( get_current_sidebar() != nullptr ) {
        widget::finalize_inherited_fields_recursive( get_current_sidebar()->getId(),
                get_current_sidebar()->_separator, get_current_sidebar()->_padding );
    }
}

void panel_manager::update_offsets( int x )
{
    width_right = x;
    width_left = 0;
}

bool panel_manager::save()
{
    return write_to_file( PATH_INFO::panel_options(), [&]( std::ostream & fout ) {
        JsonOut jout( fout, true );
        serialize( jout );
    }, _( "panel options" ) );
}

bool panel_manager::load()
{
    return read_from_file_optional_json( PATH_INFO::panel_options(), [&]( const JsonArray & jsin ) {
        deserialize( jsin );
    } );
}

void panel_manager::serialize( JsonOut &json )
{
    json.start_array();
    json.start_object();

    json.member( "current_layout_id", current_layout_id );
    json.member( "layouts" );

    json.start_array();

    for( const auto &kv : layouts ) {
        json.start_object();

        json.member( "layout_id", kv.first );
        json.member( "panels" );

        json.start_array();

        for( const window_panel &panel : kv.second.panels() ) {
            json.start_object();

            json.member( "name", panel.get_id() );
            json.member( "toggle", panel.toggle );

            json.end_object();
        }

        json.end_array();
        json.end_object();
    }

    json.end_array();

    json.end_object();
    json.end_array();
}

void panel_manager::deserialize( const JsonArray &ja )
{
    JsonObject joLayouts = ja.get_object( 0 );

    current_layout_id = joLayouts.get_string( "current_layout_id" );
    if( layouts.find( current_layout_id ) == layouts.end() ) {
        // Layout id updated between loads.
        // Shouldn't happen unless custom sidebar id's were modified or removed.
        joLayouts.allow_omitted_members();
        current_layout_id = "labels";
        return;
    }

    for( JsonObject joLayout : joLayouts.get_array( "layouts" ) ) {
        std::string layout_id = joLayout.get_string( "layout_id" );
        const auto &cur_layout = layouts.find( layout_id );
        if( cur_layout == layouts.end() ) {
            joLayout.allow_omitted_members();
            continue;
        }

        auto &layout = cur_layout->second.panels();
        auto it = layout.begin();

        for( JsonObject joPanel : joLayout.get_array( "panels" ) ) {
            std::string id = joPanel.get_string( "name" );
            bool toggle = joPanel.get_bool( "toggle" );

            for( auto it2 = layout.begin() + std::distance( layout.begin(), it ); it2 != layout.end(); ++it2 ) {
                if( it2->get_id() == id ) {
                    if( it->get_id() != id ) {
                        window_panel panel = *it2;
                        layout.erase( it2 );
                        it = layout.insert( it, panel );
                    }
                    it->toggle = toggle;
                    ++it;
                    break;
                }
            }
        }
    }
    if( ja.size() > 1 ) {
        ja.throw_error( "panel_manager expects one object" );
    }
}

// Dummy render pass to recalculate layout height
static void dummy_wgt_render( const widget_id &wid, int width )
{
    if( wid.is_null() || !wid.is_valid() ) {
        return;
    }
    if( wid->_style == "sidebar" ) {
        for( const widget_id &subw : wid->_widgets ) {
            dummy_wgt_render( subw, width );
        }
    }
    widget w = wid.obj();
    w.layout( get_avatar(), width, 0, w.has_flag( "W_NO_PADDING" ) );
}

static void draw_border_win( catacurses::window &w, const std::vector<int> &column_widths,
                             int popup_height )
{
    werase( w );
    decorate_panel( _( "Sidebar options" ), w );
    // Draw vertical separators
    mvwvline( w, point( column_widths[0] + 1, 1 ), 0, popup_height - 2 );
    mvwvline( w, point( column_widths[0] + column_widths[1] + 2, 1 ), 0, popup_height - 2 );
    wnoutrefresh( w );
}

static void draw_left_win( catacurses::window &w, const std::map<size_t, size_t> &row_indices,
                           const std::vector<window_panel> &panels, size_t source_index, size_t current_row,
                           size_t source_row, bool cur_col, bool swapping, int width, int height, int start )
{
    werase( w );
    for( std::pair<size_t, size_t> row_indx : row_indices ) {
        if( row_indx.first < static_cast<size_t>( start ) ) {
            continue;
        }
        const size_t row = row_indx.first - start;

        const std::string name = panels[row_indx.second].get_name();
        if( swapping && source_index == row_indx.second ) {
            mvwprintz( w, point( 4, current_row - start ), c_yellow, name );
        } else {
            int offset = 0;
            if( !swapping ) {
                offset = 0;
            } else if( current_row > source_row && row_indx.first > source_row &&
                       row_indx.first <= current_row ) {
                offset = -1;
            } else if( current_row < source_row && row_indx.first < source_row &&
                       row_indx.first >= current_row ) {
                offset = 1;
            }
            const nc_color toggle_color = panels[row_indx.second].toggle ? c_white : c_dark_gray;
            mvwprintz( w, point( 3, row + offset ), toggle_color, name );
        }
    }
    if( cur_col ) {
        mvwprintz( w, point( 0, current_row - start ), c_yellow, ">>" );
    }

    scrollbar()
    .offset_x( width - 1 )
    .offset_y( 0 )
    .content_size( row_indices.size() )
    .viewport_pos( start )
    .viewport_size( height )
    .apply( w );

    wnoutrefresh( w );
}

static void draw_right_win( catacurses::window &w,
                            const std::map<std::string, panel_layout> &layouts,
                            const std::string &current_layout_id,
                            size_t current_row, bool cur_col )
{
    werase( w );
    size_t i = 0;
    for( const auto &layout : layouts ) {
        mvwprintz( w, point( 3, i ), current_layout_id == layout.first ? c_light_blue : c_white,
                   layout.second.name() );
        i++;
    }
    if( cur_col ) {
        mvwprintz( w, point( 0, current_row ), c_yellow, ">>" );
    }
    wnoutrefresh( w );
}

static void draw_center_win( catacurses::window &w, int col_width, const input_context &ctxt,
                             const widget &sidebar, const std::map<size_t, size_t> &row_indices,
                             const std::vector<window_panel> &panels, size_t current_row, bool left_panel )
{
    werase( w );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_green, trunc_ellipse( ctxt.get_desc( "TOGGLE_PANEL" ),
               col_width - 1 ) + ":" );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_white, _( "Toggle panels on/off" ) );
    mvwprintz( w, point( 1, 2 ), c_light_green, trunc_ellipse( ctxt.get_desc( "MOVE_PANEL" ),
               col_width - 1 ) + ":" );
    mvwprintz( w, point( 1, 3 ), c_white, _( "Change display order" ) );
    mvwprintz( w, point( 1, 4 ), c_light_green, trunc_ellipse( ctxt.get_desc( "QUIT" ),
               col_width - 1 ) + ":" );
    mvwprintz( w, point( 1, 5 ), c_white, _( "Exit" ) );

    if( left_panel ) {
        const widget_id current_widget = panels[row_indices.at( current_row )].get_widget();
        for( const widget &wgt : widget::get_all() ) {
            if( wgt.getId() == current_widget ) {
                fold_and_print( w, point( 1, 7 ), col_width - 2, c_white, _( wgt._description ) );
                break;
            }
        }
    } else {
        fold_and_print( w, point( 1, 7 ), col_width - 2, c_white, _( sidebar._description ) );
    }

    wnoutrefresh( w );
}

void panel_manager::show_adm()
{
    input_context ctxt( "PANEL_MGMT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "LEFT" );
    ctxt.register_action( "RIGHT" );
    ctxt.register_action( "MOVE_PANEL" );
    ctxt.register_action( "TOGGLE_PANEL" );

    const std::vector<int> column_widths = { 25, 37, 17 };

    size_t current_col = 0;
    size_t current_row = 0;
    bool swapping = false;
    size_t source_row = 0;
    size_t source_index = 0;
    size_t start = 0;

    bool recalc = true;
    bool exit = false;
    // map of row the panel is on vs index
    // panels not renderable due to game configuration will not be in this map
    std::map<size_t, size_t> row_indices;

    g->show_panel_adm = true;
    g->invalidate_main_ui_adaptor();

    catacurses::window w_border;
    catacurses::window w_left;
    catacurses::window w_center;
    catacurses::window w_right;

    const int popup_height = 24;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        const point uipos( ( TERMX / 2 ) - 38, ( TERMY / 2 ) - 10 );
        w_border = catacurses::newwin( popup_height, 83, uipos );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        w_left = catacurses::newwin( popup_height - 2, column_widths[0], uipos + point( 1, 1 ) );
        w_center = catacurses::newwin( popup_height - 2, column_widths[1],
                                       uipos + point( 2 + column_widths[0], 1 ) );
        w_right = catacurses::newwin( popup_height - 2, column_widths[2],
                                      uipos + point( 3 + column_widths[0] + column_widths[1], 1 ) );

        ui.position_from_window( w_border );
    } );
    ui.mark_resize();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        draw_border_win( w_border, column_widths, popup_height );
        draw_right_win( w_right, layouts, current_layout_id, current_row, current_col == 2 );
        auto &panels = get_current_layout().panels();
        draw_left_win( w_left, row_indices, panels, source_index, current_row, source_row, current_col == 0,
                       swapping, column_widths[0], popup_height - 2, start );

        widget *sidebar = nullptr;
        if( current_col == 0 ) {
            sidebar = get_current_sidebar();
        } else {
            auto it = layouts.begin();
            std::advance( it, current_row );
            panel_layout layout = it->second;
            sidebar = get_sidebar( layout.name() );
        }
        draw_center_win( w_center, column_widths[1], ctxt, *sidebar, row_indices, panels, current_row,
                         current_col == 0 );
    } );

    while( !exit ) {
        auto &panels = get_current_layout().panels();

        if( recalc ) {
            recalc = false;
            row_indices.clear();
            if( get_current_sidebar() != nullptr ) {
                widget::finalize_inherited_fields_recursive( get_current_sidebar()->getId(),
                        get_current_sidebar()->_separator, get_current_sidebar()->_padding );
            }
            for( size_t i = 0, row = 0; i < panels.size(); i++ ) {
                if( panels[i].render() ) {
                    row_indices.emplace( row, i );
                    row++;
                }
            }
        }

        const size_t num_rows = current_col == 0 ? row_indices.size() : layouts.size();
        current_row = clamp<size_t>( current_row, 0, num_rows - 1 );
        if( current_row < start ) {
            start = current_row > popup_height - 3 ? current_row - ( popup_height - 3 ) : 0;
        }

        ui_manager::redraw();

        const std::string action = ctxt.handle_input();
        if( action == "UP" ) {
            if( current_row > 0 ) {
                current_row -= 1;
                if( current_row < start ) {
                    start = current_row;
                }
            } else {
                current_row = num_rows - 1;
                if( current_row > popup_height - 3 ) {
                    start = current_row - ( popup_height - 3 );
                }
            }
        } else if( action == "DOWN" ) {
            if( current_row + 1 < num_rows ) {
                current_row += 1;
                if( current_row > start + popup_height - 3 ) {
                    start = current_row - ( popup_height - 3 );
                }
            } else {
                current_row = 0;
                start = 0;
            }
        } else if( action == "MOVE_PANEL" && current_col == 0 ) {
            swapping = !swapping;
            if( swapping ) {
                // source window from the swap
                // saving win1 index
                source_row = current_row;
                source_index = row_indices[current_row];
            } else {
                // dest window for the swap
                // saving win2 index
                const size_t target_index = row_indices[current_row];

                int distance = target_index - source_index;
                size_t step_dir = distance > 0 ? 1 : -1;
                for( size_t i = source_index; i != target_index; i += step_dir ) {
                    std::swap( panels[i], panels[i + step_dir] );
                }
                g->invalidate_main_ui_adaptor();
                recalc = true;
            }
        } else if( !swapping && action == "MOVE_PANEL" && current_col == 2 ) {
            auto iter = std::next( layouts.begin(), current_row );
            current_layout_id = iter->first;
            int width = panel_manager::get_manager().get_current_layout().panels().begin()->get_width();
            update_offsets( width );
            int h; // to_map_font_dimension needs a second input
            to_map_font_dimension( width, h );
            // tell the game that the main screen might have a different size now.
            g->mark_main_ui_adaptor_resize();
            // run a dummy render to set the initial height of each widget
            dummy_wgt_render( widget_id( current_layout_id ), width_right );
            recalc = true;
        } else if( !swapping && ( action == "RIGHT" || action == "LEFT" ) ) {
            // there are only two columns
            if( current_col == 0 ) {
                current_col = 2;
            } else {
                current_col = 0;
            }
        } else if( !swapping && action == "TOGGLE_PANEL" && current_col == 0 ) {
            panels[row_indices[current_row]].toggle = !panels[row_indices[current_row]].toggle;
            g->invalidate_main_ui_adaptor();
        } else if( action == "QUIT" ) {
            exit = true;
            save();
        }
    }

    g->show_panel_adm = false;
    g->invalidate_main_ui_adaptor();
}
