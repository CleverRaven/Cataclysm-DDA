#include "construction.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <numeric>
#include <utility>

#include "action.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_utility.h"
#include "colony.h"
#include "color.h"
#include "construction_category.h"
#include "coordinate_conversions.h"
#include "cursesdef.h"
#include "debug.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "game.h"
#include "game_constants.h"
#include "input.h"
#include "int_id.h"
#include "item.h"
#include "item_group.h"
#include "item_stack.h"
#include "iuse.h"
#include "json.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "morale_types.h"
#include "mtype.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "player.h"
#include "player_activity.h"
#include "point.h"
#include "requirements.h"
#include "rng.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_id.h"
#include "string_input_popup.h"
#include "translations.h"
#include "trap.h"
#include "ui_manager.h"
#include "uistate.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"

static const activity_id ACT_BUILD( "ACT_BUILD" );
static const activity_id ACT_MULTIPLE_CONSTRUCTION( "ACT_MULTIPLE_CONSTRUCTION" );

static const construction_category_id construction_category_ALL( "ALL" );
static const construction_category_id construction_category_FILTER( "FILTER" );
static const construction_category_id construction_category_REPAIR( "REPAIR" );

static const itype_id itype_2x4( "2x4" );
static const itype_id itype_nail( "nail" );
static const itype_id itype_sheet( "sheet" );
static const itype_id itype_stick( "stick" );
static const itype_id itype_string_36( "string_36" );

static const trap_str_id tr_firewood_source( "tr_firewood_source" );
static const trap_str_id tr_practice_target( "tr_practice_target" );
static const trap_str_id tr_unfinished_construction( "tr_unfinished_construction" );

static const skill_id skill_electronics( "electronics" );
static const skill_id skill_fabrication( "fabrication" );

static const quality_id qual_CUT( "CUT" );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );
static const trait_id trait_PAINRESIST_TROGLO( "PAINRESIST_TROGLO" );
static const trait_id trait_SPIRITUAL( "SPIRITUAL" );
static const trait_id trait_STOCKY_TROGLO( "STOCKY_TROGLO" );

static const std::string flag_FLAT( "FLAT" );
static const std::string flag_INITIAL_PART( "INITIAL_PART" );
static const std::string flag_SUPPORTS_ROOF( "SUPPORTS_ROOF" );

class inventory;

static bool finalized = false;

// Construction functions.
namespace construct
{
// Checks for whether terrain mod can proceed
static bool check_nothing( const tripoint & )
{
    return true;
}
bool check_empty( const tripoint & ); // tile is empty
bool check_support( const tripoint & ); // at least two orthogonal supports
bool check_deconstruct( const tripoint & ); // either terrain or furniture must be deconstructible
bool check_empty_up_OK( const tripoint & ); // tile is empty and below OVERMAP_HEIGHT
bool check_up_OK( const tripoint & ); // tile is below OVERMAP_HEIGHT
bool check_down_OK( const tripoint & ); // tile is above OVERMAP_DEPTH
bool check_no_trap( const tripoint & );

// Special actions to be run post-terrain-mod
static void done_nothing( const tripoint & ) {}
void done_trunk_plank( const tripoint & );
void done_grave( const tripoint & );
void done_vehicle( const tripoint & );
void done_deconstruct( const tripoint & );
void done_digormine_stair( const tripoint &, bool );
void done_dig_stair( const tripoint & );
void done_mine_downstair( const tripoint & );
void done_mine_upstair( const tripoint & );
void done_wood_stairs( const tripoint & );
void done_window_curtains( const tripoint & );
void done_extract_maybe_revert_to_dirt( const tripoint & );
void done_mark_firewood( const tripoint & );
void done_mark_practice_target( const tripoint & );

void failure_standard( const tripoint & );
void failure_deconstruct( const tripoint & );
} // namespace construct

static std::vector<construction> constructions;
static std::map<construction_str_id, construction_id> construction_id_map;

// Helper functions, nobody but us needs to call these.
static bool can_construct( const std::string &desc );
static bool can_construct( const construction &con );
static bool player_can_build( player &p, const inventory &inv, const std::string &desc );
static void place_construction( const std::string &desc );

// Color standardization for string streams
static const deferred_color color_title = def_c_light_red; //color for titles
static const deferred_color color_data = def_c_cyan; //color for data parts

static bool has_pre_terrain( const construction &con, const tripoint &p )
{
    if( con.pre_terrain.empty() ) {
        return true;
    }

    map &here = get_map();
    if( con.pre_is_furniture ) {
        furn_id f = furn_id( con.pre_terrain );
        return here.furn( p ) == f;
    } else {
        ter_id t = ter_id( con.pre_terrain );
        return here.ter( p ) == t;
    }
}

static bool has_pre_terrain( const construction &con )
{
    for( const tripoint &p : get_map().points_in_radius( g->u.pos(), 1 ) ) {
        if( p != g->u.pos() && has_pre_terrain( con, p ) ) {
            return true;
        }
    }
    return false;
}

void standardize_construction_times( const int time )
{
    if( !finalized ) {
        debugmsg( "standardize_construction_times called before finalization" );
        return;
    }
    for( auto &c : constructions ) {
        c.time = time;
    }
}

static std::vector<construction *> constructions_by_desc( const std::string &description )
{
    if( !finalized ) {
        debugmsg( "constructions_by_desc called before finalization" );
        return {};
    }
    std::vector<construction *> result;
    for( auto &constructions_a : constructions ) {
        if( constructions_a.description == description ) {
            result.push_back( &constructions_a );
        }
    }
    return result;
}

static void load_available_constructions( std::vector<std::string> &available,
        std::map<construction_category_id, std::vector<std::string>> &cat_available,
        bool hide_unconstructable )
{
    cat_available.clear();
    available.clear();
    if( !finalized ) {
        debugmsg( "load_available_constructions called before finalization" );
        return;
    }
    for( auto &it : constructions ) {
        if( it.on_display && ( !hide_unconstructable ||
                               ( can_construct( it ) && player_can_build( g->u, g->u.crafting_inventory(), it ) ) ) ) {
            bool already_have_it = false;
            for( auto &avail_it : available ) {
                if( avail_it == it.description ) {
                    already_have_it = true;
                    break;
                }
            }
            if( !already_have_it ) {
                available.push_back( it.description );
                cat_available[it.category].push_back( it.description );
            }
        }
    }
}

static void draw_grid( const catacurses::window &w, const int list_width )
{
    draw_border( w );
    mvwprintz( w, point( 2, 0 ), c_light_red, _( " Construction " ) );
    // draw internal lines
    mvwvline( w, point( list_width, 1 ), LINE_XOXO, getmaxy( w ) - 2 );
    mvwhline( w, point( 1, 2 ), LINE_OXOX, list_width );
    // draw intersections
    mvwputch( w, point( list_width, 0 ), c_light_gray, LINE_OXXX );
    mvwputch( w, point( list_width, getmaxy( w ) - 1 ), c_light_gray, LINE_XXOX );
    mvwputch( w, point( 0, 2 ), c_light_gray, LINE_XXXO );
    mvwputch( w, point( list_width, 2 ), c_light_gray, LINE_XOXX );

    wnoutrefresh( w );
}

static nc_color construction_color( const std::string &con_name, bool highlight )
{
    nc_color col = c_dark_gray;
    if( g->u.has_trait( trait_DEBUG_HS ) ) {
        col = c_white;
    } else if( can_construct( con_name ) ) {
        construction *con_first = nullptr;
        std::vector<construction *> cons = constructions_by_desc( con_name );
        const inventory &total_inv = g->u.crafting_inventory();
        for( auto &con : cons ) {
            if( con->requirements->can_make_with_inventory( total_inv, is_crafting_component ) ) {
                con_first = con;
                break;
            }
        }
        if( con_first != nullptr ) {
            col = c_white;
            for( const auto &pr : con_first->required_skills ) {
                int s_lvl = g->u.get_skill_level( pr.first );
                if( s_lvl < pr.second ) {
                    col = c_red;
                } else if( s_lvl < pr.second * 1.25 ) {
                    col = c_light_blue;
                }
            }
        }
    }
    return highlight ? hilite( col ) : col;
}

const std::vector<construction> &get_constructions()
{
    if( !finalized ) {
        debugmsg( "get_constructions called before finalization" );
        static std::vector<construction> fake_constructions;
        return fake_constructions;
    }
    return constructions;
}

construction_id construction_menu( const bool blueprint )
{
    if( !finalized ) {
        debugmsg( "construction_menu called before finalization" );
        return construction_id( -1 );
    }
    static bool hide_unconstructable = false;
    // only display constructions the player can theoretically perform
    std::vector<std::string> available;
    std::map<construction_category_id, std::vector<std::string>> cat_available;
    load_available_constructions( available, cat_available, hide_unconstructable );

    if( available.empty() ) {
        popup( _( "You can not construct anything here." ) );
        return construction_id( -1 );
    }

    int w_height = 0;
    int w_width = 0;
    catacurses::window w_con;

    int w_list_width = 0;
    int w_list_height = 0;
    const int w_list_x0 = 1;
    catacurses::window w_list;

    std::vector<std::string> notes;
    int pos_x = 0;
    int available_window_width = 0;
    int available_buffer_height = 0;

    construction_id ret( -1 );

    bool update_info = true;
    bool update_cat = true;
    bool isnew = true;
    int tabindex = 0;
    int select = 0;
    int offset = 0;
    bool exit = false;
    construction_category_id category_id;
    std::vector<std::string> constructs;
    //storage for the color text so it can be scrolled
    std::vector< std::vector < std::string > > construct_buffers;
    std::vector<std::string> full_construct_buffer;
    std::vector<int> construct_buffer_breakpoints;
    int total_project_breakpoints = 0;
    int current_construct_breakpoint = 0;
    const inventory &total_inv = g->u.crafting_inventory();

    input_context ctxt( "CONSTRUCTION" );
    ctxt.register_action( "UP", to_translation( "Move cursor up" ) );
    ctxt.register_action( "DOWN", to_translation( "Move cursor down" ) );
    ctxt.register_action( "RIGHT", to_translation( "Move tab right" ) );
    ctxt.register_action( "LEFT", to_translation( "Move tab left" ) );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "TOGGLE_UNAVAILABLE_CONSTRUCTIONS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );

    const std::vector<construction_category> &construct_cat = construction_categories::get_all();
    const int tabcount = static_cast<int>( construction_category::count() );

    std::string filter;

    const nc_color color_stage = c_white;
    ui_adaptor ui;

    const auto recalc_buffer = [&]() {
        //leave room for top and bottom UI text
        available_buffer_height = w_height - 3 - 3 - static_cast<int>( notes.size() );

        if( !constructs.empty() ) {
            if( select >= static_cast<int>( constructs.size() ) ) {
                select = 0;
            }
            std::string current_desc = constructs[select];

            //construct the project list buffer

            // Print stages and their requirement.
            std::vector<construction *> options = constructions_by_desc( current_desc );

            construct_buffers.clear();
            current_construct_breakpoint = 0;
            construct_buffer_breakpoints.clear();
            full_construct_buffer.clear();
            int stage_counter = 0;
            for( std::vector<construction *>::iterator it = options.begin();
                 it != options.end(); ++it ) {
                stage_counter++;
                construction *current_con = *it;
                if( hide_unconstructable && !can_construct( *current_con ) ) {
                    continue;
                }
                // Update the cached availability of components and tools in the requirement object
                current_con->requirements->can_make_with_inventory( total_inv, is_crafting_component );

                std::vector<std::string> current_buffer;

                const auto add_folded = [&]( const std::vector<std::string> &folded ) {
                    current_buffer.insert( current_buffer.end(), folded.begin(), folded.end() );
                };
                const auto add_line = [&]( const std::string & line ) {
                    add_folded( foldstring( line, available_window_width ) );
                };

                // display final product name only if more than one step.
                // Assume single stage constructions should be clear
                // in their title what their result is.
                if( !current_con->post_terrain.empty() && options.size() > 1 ) {
                    //also print out stage number when multiple stages are available
                    std::string current_line = string_format( _( "Stage/Variant #%d: " ), stage_counter );

                    // print name of the result of each stage
                    std::string result_string;
                    if( current_con->post_is_furniture ) {
                        result_string = furn_str_id( current_con->post_terrain ).obj().name();
                    } else {
                        result_string = ter_str_id( current_con->post_terrain ).obj().name();
                    }
                    current_line += colorize( result_string, color_title );
                    add_line( current_line );

                    // display description of the result for multi-stages
                    current_line = _( "Result: " );
                    if( current_con->post_is_furniture ) {
                        current_line += colorize(
                                            furn_str_id( current_con->post_terrain ).obj().description,
                                            color_data
                                        );
                    } else {
                        current_line += colorize(
                                            ter_str_id( current_con->post_terrain ).obj().description,
                                            color_data
                                        );
                    }
                    add_line( current_line );

                    // display description of the result for single stages
                } else if( !current_con->post_terrain.empty() ) {
                    std::string current_line = _( "Result: " );
                    if( current_con->post_is_furniture ) {
                        current_line += colorize(
                                            furn_str_id( current_con->post_terrain ).obj().description,
                                            color_data
                                        );
                    } else {
                        current_line += colorize(
                                            ter_str_id( current_con->post_terrain ).obj().description,
                                            color_data
                                        );
                    }
                    add_line( current_line );
                }

                // display required skill and difficulty
                if( current_con->required_skills.empty() ) {
                    add_line( _( "N/A" ) );
                } else {
                    std::string current_line = _( "Required skills: " ) + enumerate_as_string(
                                                   current_con->required_skills.begin(), current_con->required_skills.end(),
                    []( const std::pair<skill_id, int> &skill ) {
                        nc_color col;
                        int s_lvl = g->u.get_skill_level( skill.first );
                        if( s_lvl < skill.second ) {
                            col = c_red;
                        } else if( s_lvl < skill.second * 1.25 ) {
                            col = c_light_blue;
                        } else {
                            col = c_green;
                        }

                        return colorize( string_format( "%s (%d)", skill.first.obj().name(), skill.second ), col );
                    }, enumeration_conjunction::none );
                    add_line( current_line );
                }

                // TODO: Textify pre_flags to provide a bit more information.
                // Example: First step of dig pit could say something about
                // requiring diggable ground.
                if( !current_con->pre_terrain.empty() ) {
                    std::string require_string;
                    if( current_con->pre_is_furniture ) {
                        require_string = furn_str_id( current_con->pre_terrain )->name();
                    } else {
                        require_string = ter_str_id( current_con->pre_terrain )->name();
                    }
                    nc_color pre_color = has_pre_terrain( *current_con ) ? c_green : c_red;
                    add_line( _( "Requires: " ) + colorize( require_string, pre_color ) );
                }
                if( !current_con->pre_note.empty() ) {
                    add_line( _( "Annotation: " ) + colorize( _( current_con->pre_note ), color_data ) );
                }
                // get pre-folded versions of the rest of the construction project to be displayed later

                // get time needed
                add_folded( current_con->get_folded_time_string( available_window_width ) );

                add_folded( current_con->requirements->get_folded_tools_list( available_window_width, color_stage,
                            total_inv ) );

                add_folded( current_con->requirements->get_folded_components_list( available_window_width,
                            color_stage, total_inv, is_crafting_component ) );

                construct_buffers.push_back( current_buffer );
            }

            //determine where the printing starts for each project, so it can be scrolled to those points
            size_t current_buffer_location = 0;
            for( size_t i = 0; i < construct_buffers.size(); i++ ) {
                construct_buffer_breakpoints.push_back( static_cast<int>( current_buffer_location ) );
                full_construct_buffer.insert( full_construct_buffer.end(), construct_buffers[i].begin(),
                                              construct_buffers[i].end() );

                //handle text too large for one screen
                if( construct_buffers[i].size() > static_cast<size_t>( available_buffer_height ) ) {
                    construct_buffer_breakpoints.push_back( static_cast<int>( current_buffer_location +
                                                            static_cast<size_t>( available_buffer_height ) ) );
                }
                current_buffer_location += construct_buffers[i].size();
                if( i < construct_buffers.size() - 1 ) {
                    full_construct_buffer.push_back( std::string() );
                    current_buffer_location++;
                }
            }
            total_project_breakpoints = static_cast<int>( construct_buffer_breakpoints.size() );
        }
    };

    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        w_height = TERMY;
        if( static_cast<int>( available.size() ) + 2 < w_height ) {
            w_height = available.size() + 2;
        }
        if( w_height < FULL_SCREEN_HEIGHT ) {
            w_height = FULL_SCREEN_HEIGHT;
        }

        w_width = std::max( FULL_SCREEN_WIDTH, TERMX * 2 / 3 );
        const int w_y0 = ( TERMY > w_height ) ? ( TERMY - w_height ) / 2 : 0;
        const int w_x0 = ( TERMX > w_width ) ? ( TERMX - w_width ) / 2 : 0;
        w_con = catacurses::newwin( w_height, w_width, point( w_x0, w_y0 ) );

        w_list_width = static_cast<int>( .375 * w_width );
        w_list_height = w_height - 4;
        w_list = catacurses::newwin( w_list_height, w_list_width,
                                     point( w_x0 + w_list_x0, w_y0 + 3 ) );

        pos_x = w_list_width + w_list_x0 + 2;
        available_window_width = w_width - pos_x - 1;

        recalc_buffer();

        ui.position_from_window( w_con );
    } );
    ui.mark_resize();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        draw_grid( w_con, w_list_width + w_list_x0 );

        // Erase existing tab selection & list of constructions
        mvwhline( w_con, point_south_east, ' ', w_list_width );
        werase( w_list );
        // Print new tab listing
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( w_con, point( 1, 1 ), c_yellow, "<< %s >>", construct_cat[tabindex].name() );
        // Determine where in the master list to start printing
        calcStartPos( offset, select, w_list_height, constructs.size() );
        // Print the constructions between offset and max (or how many will fit)
        cata::optional<point> cursor_pos;
        for( size_t i = 0; static_cast<int>( i ) < w_list_height &&
             ( i + offset ) < constructs.size(); i++ ) {
            int current = i + offset;
            std::string con_name = constructs[current];
            bool highlight = ( current == select );
            const point print_from( 0, i );
            if( highlight ) {
                cursor_pos = print_from;
            }
            trim_and_print( w_list, print_from, w_list_width,
                            construction_color( con_name, highlight ), _( con_name ) );
        }

        // Clear out lines for tools & materials
        for( int i = 1; i < w_height - 1; i++ ) {
            mvwhline( w_con, point( pos_x, i ), ' ', available_window_width );
        }

        // print the hotkeys regardless of if there are constructions
        for( size_t i = 0; i < notes.size(); ++i ) {
            trim_and_print( w_con, point( pos_x,
                                          w_height - 1 - static_cast<int>( notes.size() ) + static_cast<int>( i ) ),
                            available_window_width, c_white, notes[i] );
        }

        if( !constructs.empty() ) {
            if( select >= static_cast<int>( constructs.size() ) ) {
                select = 0;
            }
            std::string current_desc = constructs[select];
            // Print construction name
            trim_and_print( w_con, point( pos_x, 1 ), available_window_width, c_white, _( current_desc ) );

            if( current_construct_breakpoint > 0 ) {
                // Print previous stage indicator if breakpoint is past the beginning
                trim_and_print( w_con, point( pos_x, 2 ), available_window_width, c_white,
                                _( "Press %s to show previous stage(s)." ),
                                ctxt.get_desc( "PAGE_UP" ) );
            }
            if( static_cast<size_t>( construct_buffer_breakpoints[current_construct_breakpoint] +
                                     available_buffer_height ) < full_construct_buffer.size() ) {
                // Print next stage indicator if more breakpoints are remaining after screen height
                trim_and_print( w_con, point( pos_x, w_height - 2 - static_cast<int>( notes.size() ) ),
                                available_window_width,
                                c_white, _( "Press %s to show next stage(s)." ),
                                ctxt.get_desc( "PAGE_DOWN" ) );
            }
            // Leave room for above/below indicators
            int ypos = 3;
            nc_color stored_color = color_stage;
            for( size_t i = static_cast<size_t>( construct_buffer_breakpoints[current_construct_breakpoint] );
                 i < full_construct_buffer.size(); i++ ) {
                //the value of 3 is from leaving room at the top of window
                if( ypos > available_buffer_height + 3 ) {
                    break;
                }
                print_colored_text( w_con, point( w_list_width + w_list_x0 + 2, ypos++ ), stored_color, color_stage,
                                    full_construct_buffer[i] );
            }
        }

        draw_scrollbar( w_con, select, w_list_height, constructs.size(), point( 0, 3 ) );
        wnoutrefresh( w_con );

        // place the cursor at the selected construction name as expected by screen readers
        if( cursor_pos ) {
            wmove( w_list, cursor_pos.value() );
        }
        wnoutrefresh( w_list );
    } );

    do {
        if( update_cat ) {
            update_cat = false;
            cata::optional<std::string> last_construction;
            if( isnew ) {
                filter = uistate.construction_filter;
                tabindex = uistate.construction_tab.is_valid()
                           ? uistate.construction_tab.id().to_i() : 0;
                last_construction = uistate.last_construction;
            } else if( select >= 0 && static_cast<size_t>( select ) < constructs.size() ) {
                last_construction = constructs[select];
            }
            category_id = construct_cat[tabindex].id;
            if( category_id == construction_category_ALL ) {
                constructs = available;
            } else if( category_id == construction_category_FILTER ) {
                constructs.clear();
                std::copy_if( available.begin(), available.end(),
                              std::back_inserter( constructs ),
                [&]( const std::string & a ) {
                    return lcmatch( _( a ), filter );
                } );
            } else {
                constructs = cat_available[category_id];
            }
            select = 0;
            if( last_construction ) {
                const auto it = std::find( constructs.begin(), constructs.end(),
                                           *last_construction );
                if( it != constructs.end() ) {
                    select = std::distance( constructs.begin(), it );
                }
            }
        }
        isnew = false;

        if( update_info ) {
            update_info = false;

            notes.clear();
            if( tabindex == tabcount - 1 && !filter.empty() ) {
                notes.push_back( string_format( _( "Press [<color_red>%s</color>] to clear filter." ),
                                                ctxt.get_desc( "RESET_FILTER" ) ) );
            }
            notes.push_back( string_format( _( "Press [<color_yellow>%s or %s</color>] to tab." ),
                                            ctxt.get_desc( "LEFT" ),
                                            ctxt.get_desc( "RIGHT" ) ) );
            notes.push_back( string_format( _( "Press [<color_yellow>%s</color>] to search." ),
                                            ctxt.get_desc( "FILTER" ) ) );
            if( !hide_unconstructable ) {
                notes.push_back( string_format(
                                     _( "Press [<color_yellow>%s</color>] to hide unavailable constructions." ),
                                     ctxt.get_desc( "TOGGLE_UNAVAILABLE_CONSTRUCTIONS" ) ) );
            } else {
                notes.push_back( string_format(
                                     _( "Press [<color_red>%s</color>] to show unavailable constructions." ),
                                     ctxt.get_desc( "TOGGLE_UNAVAILABLE_CONSTRUCTIONS" ) ) );
            }
            notes.push_back( string_format(
                                 _( "Press [<color_yellow>%s</color>] to view and edit keybindings." ),
                                 ctxt.get_desc( "HELP_KEYBINDINGS" ) ) );

            recalc_buffer();
        } // Finished updating

        ui_manager::redraw();

        const std::string action = ctxt.handle_input();
        if( action == "FILTER" ) {
            string_input_popup popup;
            popup
            .title( _( "Search" ) )
            .width( 50 )
            .description( _( "Filter" ) )
            .max_length( 100 )
            .text( tabindex == tabcount - 1 ? filter : std::string() )
            .query();
            if( popup.confirmed() ) {
                filter = popup.text();
                uistate.construction_filter = filter;
                update_info = true;
                update_cat = true;
                tabindex = tabcount - 1;
            }
        } else if( action == "RESET_FILTER" ) {
            if( tabindex == tabcount - 1 && !filter.empty() ) {
                filter.clear();
                uistate.construction_filter.clear();
                update_info = true;
                update_cat = true;
            }
        } else if( action == "DOWN" ) {
            update_info = true;
            if( select < static_cast<int>( constructs.size() ) - 1 ) {
                select++;
            } else {
                select = 0;
            }
        } else if( action == "UP" ) {
            update_info = true;
            if( select > 0 ) {
                select--;
            } else {
                select = constructs.size() - 1;
            }
        } else if( action == "LEFT" ) {
            update_info = true;
            update_cat = true;
            tabindex--;
            if( tabindex < 0 ) {
                tabindex = tabcount - 1;
            }
        } else if( action == "RIGHT" ) {
            update_info = true;
            update_cat = true;
            tabindex = ( tabindex + 1 ) % tabcount;
        } else if( action == "PAGE_UP" ) {
            if( current_construct_breakpoint > 0 ) {
                current_construct_breakpoint--;
            }
            if( current_construct_breakpoint < 0 ) {
                current_construct_breakpoint = 0;
            }
        } else if( action == "PAGE_DOWN" ) {
            if( current_construct_breakpoint < total_project_breakpoints - 1 ) {
                current_construct_breakpoint++;
            }
            if( current_construct_breakpoint >= total_project_breakpoints ) {
                current_construct_breakpoint = total_project_breakpoints - 1;
            }
        } else if( action == "QUIT" ) {
            exit = true;
        } else if( action == "TOGGLE_UNAVAILABLE_CONSTRUCTIONS" ) {
            update_info = true;
            update_cat = true;
            hide_unconstructable = !hide_unconstructable;
            offset = 0;
            load_available_constructions( available, cat_available, hide_unconstructable );
        } else if( action == "CONFIRM" ) {
            if( constructs.empty() || select >= static_cast<int>( constructs.size() ) ) {
                // Nothing to be done here
                continue;
            }
            if( !blueprint ) {
                if( player_can_build( g->u, total_inv, constructs[select] ) ) {
                    if( !player_can_see_to_build( g->u, constructs[select] ) ) {
                        add_msg( m_info, _( "It is too dark to construct right now." ) );
                    } else {
                        ui.reset();
                        place_construction( constructs[select] );
                        uistate.last_construction = constructs[select];
                    }
                    exit = true;
                } else {
                    popup( _( "You can't build that!" ) );
                    update_info = true;
                }
            } else {
                // get the index of the overall constructions list from current_desc
                const std::vector<construction> &list_constructions = get_constructions();
                for( int i = 0; i < static_cast<int>( list_constructions.size() ); ++i ) {
                    if( constructs[select] == list_constructions[i].description ) {
                        ret = construction_id( i );
                        break;
                    }
                }
                exit = true;
            }
        }
    } while( !exit );

    uistate.construction_tab = int_id<construction_category>( tabindex ).id();

    return ret;
}

bool player_can_build( player &p, const inventory &inv, const std::string &desc )
{
    // check all with the same desc to see if player can build any
    std::vector<construction *> cons = constructions_by_desc( desc );
    for( auto &con : cons ) {
        if( player_can_build( p, inv, *con ) ) {
            return true;
        }
    }
    return false;
}

bool player_can_build( player &p, const inventory &inv, const construction &con )
{
    if( p.has_trait( trait_DEBUG_HS ) ) {
        return true;
    }

    if( !p.meets_skill_requirements( con ) ) {
        return false;
    }

    return con.requirements->can_make_with_inventory( inv, is_crafting_component );
}

bool player_can_see_to_build( player &p, const std::string &desc )
{
    if( p.fine_detail_vision_mod() < 4 || p.has_trait( trait_DEBUG_HS ) ) {
        return true;
    }
    std::vector<construction *> cons = constructions_by_desc( desc );
    for( construction *&con : cons ) {
        if( con->dark_craftable ) {
            return true;
        }
    }
    return false;
}

bool can_construct( const std::string &desc )
{
    // check all with the same desc to see if player can build any
    std::vector<construction *> cons = constructions_by_desc( desc );
    for( auto &con : cons ) {
        if( can_construct( *con ) ) {
            return true;
        }
    }
    return false;
}

bool can_construct( const construction &con, const tripoint &p )
{
    // see if the special pre-function checks out
    bool place_okay = con.pre_special( p );
    // see if the terrain type checks out
    place_okay &= has_pre_terrain( con, p );
    // see if the flags check out
    place_okay &= std::all_of( con.pre_flags.begin(), con.pre_flags.end(),
    [&p]( const std::string & flag ) {
        return get_map().has_flag( flag, p );
    } );
    // make sure the construction would actually do something
    if( !con.post_terrain.empty() ) {
        map &here = get_map();
        if( con.post_is_furniture ) {
            furn_id f = furn_id( con.post_terrain );
            place_okay &= here.furn( p ) != f;
        } else {
            ter_id t = ter_id( con.post_terrain );
            place_okay &= here.ter( p ) != t;
        }
    }
    return place_okay;
}

bool can_construct( const construction &con )
{
    for( const tripoint &p : get_map().points_in_radius( g->u.pos(), 1 ) ) {
        if( p != g->u.pos() && can_construct( con, p ) ) {
            return true;
        }
    }
    return false;
}

void place_construction( const std::string &desc )
{
    const inventory &total_inv = g->u.crafting_inventory();

    std::vector<construction *> cons = constructions_by_desc( desc );
    std::map<tripoint, const construction *> valid;
    map &here = get_map();
    for( const tripoint &p : here.points_in_radius( g->u.pos(), 1 ) ) {
        for( const auto *con : cons ) {
            if( p != g->u.pos() && can_construct( *con, p ) && player_can_build( g->u, total_inv, *con ) ) {
                valid[ p ] = con;
            }
        }
    }

    shared_ptr_fast<game::draw_callback_t> draw_valid = make_shared_fast<game::draw_callback_t>( [&]() {
        map &here = get_map();
        for( auto &elem : valid ) {
            here.drawsq( g->w_terrain, g->u, elem.first, true, false,
                         g->u.pos() + g->u.view_offset );
        }
    } );
    g->add_draw_callback( draw_valid );

    const cata::optional<tripoint> pnt_ = choose_adjacent( _( "Construct where?" ) );
    if( !pnt_ ) {
        return;
    }
    const tripoint pnt = *pnt_;

    if( valid.find( pnt ) == valid.end() ) {
        cons.front()->explain_failure( pnt );
        return;
    }
    // Maybe there is already a partial_con on an existing trap, that isn't caught by the usual trap-checking.
    // because the pre-requisite construction is already a trap anyway.
    // This shouldn't normally happen, unless it's a spike pit being built on a pit for example.
    partial_con *pre_c = here.partial_con_at( pnt );
    if( pre_c ) {
        add_msg( m_info,
                 _( "There is already an unfinished construction there, examine it to continue working on it" ) );
        return;
    }
    std::list<item> used;
    const construction &con = *valid.find( pnt )->second;
    // create the partial construction struct
    partial_con pc;
    pc.id = con.id;
    // Set the trap that has the examine function
    // Special handling for constructions that take place on existing traps.
    // Basically just don't add the unfinished construction trap.
    // TODO: handle this cleaner, instead of adding a special case to pit iexamine.
    if( here.tr_at( pnt ).loadid == tr_null ) {
        here.trap_set( pnt, tr_unfinished_construction );
    }
    // Use up the components
    for( const auto &it : con.requirements->get_components() ) {
        std::list<item> tmp = g->u.consume_items( it, 1, is_crafting_component );
        used.splice( used.end(), tmp );
    }
    pc.components = used;
    here.partial_con_set( pnt, pc );
    for( const auto &it : con.requirements->get_tools() ) {
        g->u.consume_tools( it );
    }
    g->u.assign_activity( ACT_BUILD );
    g->u.activity.placement = here.getabs( pnt );
}

void complete_construction( player *p )
{
    if( !finalized ) {
        debugmsg( "complete_construction called before finalization" );
        return;
    }
    map &here = get_map();
    const tripoint terp = here.getlocal( p->activity.placement );
    partial_con *pc = here.partial_con_at( terp );
    if( !pc ) {
        debugmsg( "No partial construction found at activity placement in complete_construction()" );
        if( here.tr_at( terp ).loadid == tr_unfinished_construction ) {
            here.remove_trap( terp );
        }
        if( p->is_npc() ) {
            npc *guy = dynamic_cast<npc *>( p );
            guy->current_activity_id = activity_id::NULL_ID();
            guy->revert_after_activity();
            guy->set_moves( 0 );
        }
        return;
    }
    const construction &built = pc->id.obj();
    const auto award_xp = [&]( player & c ) {
        for( const auto &pr : built.required_skills ) {
            c.practice( pr.first, static_cast<int>( ( 10 + 15 * pr.second ) * ( 1 + built.time / 180000.0 ) ),
                        static_cast<int>( pr.second * 1.25 ) );
        }
    };

    award_xp( *p );
    // Friendly NPCs gain exp from assisting or watching...
    // TODO: NPCs watching other NPCs do stuff and learning from it
    if( p->is_player() ) {
        for( auto &elem : g->u.get_crafting_helpers() ) {
            if( elem->meets_skill_requirements( built ) ) {
                add_msg( m_info, _( "%s assists you with the work…" ), elem->name );
            } else {
                //NPC near you isn't skilled enough to help
                add_msg( m_info, _( "%s watches you work…" ), elem->name );
            }

            award_xp( *elem );
        }
    }
    if( here.tr_at( terp ).loadid == tr_unfinished_construction ) {
        here.remove_trap( terp );
    }
    here.partial_con_remove( terp );
    // Some constructions are allowed to have items left on the tile.
    if( built.post_flags.count( "keep_items" ) == 0 ) {
        // Move any items that have found their way onto the construction site.
        std::vector<tripoint> dump_spots;
        for( const tripoint &pt : here.points_in_radius( terp, 1 ) ) {
            if( here.can_put_items( pt ) && pt != terp ) {
                dump_spots.push_back( pt );
            }
        }
        if( !dump_spots.empty() ) {
            tripoint dump_spot = random_entry( dump_spots );
            map_stack items = here.i_at( terp );
            for( map_stack::iterator it = items.begin(); it != items.end(); ) {
                here.add_item_or_charges( dump_spot, *it );
                it = items.erase( it );
            }
        } else {
            debugmsg( "No space to displace items from construction finishing" );
        }
    }
    // Make the terrain change
    if( !built.post_terrain.empty() ) {
        if( built.post_is_furniture ) {
            here.furn_set( terp, furn_str_id( built.post_terrain ) );
        } else {
            here.ter_set( terp, ter_str_id( built.post_terrain ) );
        }
    }

    // Spawn byproducts
    if( built.byproduct_item_group ) {
        here.spawn_items( p->pos(), item_group::items_from( *built.byproduct_item_group, calendar::turn ) );
    }

    add_msg( m_info, _( "%s finished construction: %s." ), p->disp_name(), _( built.description ) );
    // clear the activity
    p->activity.set_to_null();

    // This comes after clearing the activity, in case the function interrupts
    // activities
    built.post_special( terp );
    // npcs will automatically resume backlog, players wont.
    if( p->is_player() && !p->backlog.empty() &&
        p->backlog.front().id() == ACT_MULTIPLE_CONSTRUCTION ) {
        p->backlog.clear();
        p->assign_activity( ACT_MULTIPLE_CONSTRUCTION );
    }
}

bool construct::check_empty( const tripoint &p )
{
    map &here = get_map();
    return ( here.has_flag( flag_FLAT, p ) && !here.has_furn( p ) &&
             g->is_empty( p ) && here.tr_at( p ).is_null() &&
             here.i_at( p ).empty() && !here.veh_at( p ) );
}

inline std::array<tripoint, 4> get_orthogonal_neighbors( const tripoint &p )
{
    return {{
            p + point_north,
            p + point_south,
            p + point_west,
            p + point_east
        }};
}

bool construct::check_support( const tripoint &p )
{
    map &here = get_map();
    // need two or more orthogonally adjacent supports
    if( here.impassable( p ) ) {
        return false;
    }
    int num_supports = 0;
    for( const tripoint &nb : get_orthogonal_neighbors( p ) ) {
        if( here.has_flag( flag_SUPPORTS_ROOF, nb ) ) {
            num_supports++;
        }
    }
    return num_supports >= 2;
}

bool construct::check_deconstruct( const tripoint &p )
{
    map &here = get_map();
    if( here.has_furn( p.xy() ) ) {
        return here.furn( p.xy() ).obj().deconstruct.can_do;
    }
    // terrain can only be deconstructed when there is no furniture in the way
    return here.ter( p.xy() ).obj().deconstruct.can_do;
}

bool construct::check_empty_up_OK( const tripoint &p )
{
    return check_empty( p ) && check_up_OK( p );
}

bool construct::check_up_OK( const tripoint & )
{
    // You're not going above +OVERMAP_HEIGHT.
    return ( g->get_levz() < OVERMAP_HEIGHT );
}

bool construct::check_down_OK( const tripoint & )
{
    // You're not going below -OVERMAP_DEPTH.
    return ( g->get_levz() > -OVERMAP_DEPTH );
}

bool construct::check_no_trap( const tripoint &p )
{
    return get_map().tr_at( p ).is_null();
}

void construct::done_trunk_plank( const tripoint &/*p*/ )
{
    int num_logs = rng( 2, 3 );
    for( int i = 0; i < num_logs; ++i ) {
        iuse::cut_log_into_planks( g->u );
    }
}

void construct::done_grave( const tripoint &p )
{
    map &here = get_map();
    map_stack its = here.i_at( p );
    for( item it : its ) {
        if( it.is_corpse() ) {
            if( it.get_corpse_name().empty() ) {
                if( it.get_mtype()->has_flag( MF_HUMAN ) ) {
                    if( g->u.has_trait( trait_SPIRITUAL ) ) {
                        g->u.add_morale( MORALE_FUNERAL, 50, 75, 1_days, 1_hours );
                        add_msg( m_good,
                                 _( "You feel relieved after providing last rites for this human being, whose name is lost in the Cataclysm." ) );
                    } else {
                        add_msg( m_neutral, _( "You bury remains of a human, whose name is lost in the Cataclysm." ) );
                    }
                }
            } else {
                if( g->u.has_trait( trait_SPIRITUAL ) ) {
                    g->u.add_morale( MORALE_FUNERAL, 50, 75, 1_days, 1_hours );
                    add_msg( m_good,
                             _( "You feel sadness, but also relief after providing last rites for %s, whose name you will keep in your memory." ),
                             it.get_corpse_name() );
                } else {
                    add_msg( m_neutral,
                             _( "You bury remains of %s, who joined uncounted masses perished in the Cataclysm." ),
                             it.get_corpse_name() );
                }
            }
            g->events().send<event_type::buries_corpse>(
                g->u.getID(), it.get_mtype()->id, it.get_corpse_name() );
        }
    }
    if( g->u.has_quality( qual_CUT ) ) {
        iuse::handle_ground_graffiti( g->u, nullptr, _( "Inscribe something on the grave?" ), p );
    } else {
        add_msg( m_neutral,
                 _( "Unfortunately you don't have anything sharp to place an inscription on the grave." ) );
    }

    here.destroy_furn( p, true );
}

static vpart_id vpart_from_item( const itype_id &item_id )
{
    for( const auto &e : vpart_info::all() ) {
        const vpart_info &vp = e.second;
        if( vp.item == item_id && vp.has_flag( flag_INITIAL_PART ) ) {
            return vp.get_id();
        }
    }
    // The INITIAL_PART flag is optional, if no part (based on the given item) has it, just use the
    // first part that is based in the given item (this is fine for example if there is only one
    // such type anyway).
    for( const auto &e : vpart_info::all() ) {
        const vpart_info &vp = e.second;
        if( vp.item == item_id ) {
            return vp.get_id();
        }
    }
    debugmsg( "item %s used by construction is not base item of any vehicle part!", item_id.c_str() );
    static const vpart_id frame_id( "frame_vertical_2" );
    return frame_id;
}

void construct::done_vehicle( const tripoint &p )
{
    std::string name = string_input_popup()
                       .title( _( "Enter new vehicle name:" ) )
                       .width( 20 )
                       .query_string();
    if( name.empty() ) {
        name = _( "Car" );
    }

    map &here = get_map();
    vehicle *veh = here.add_vehicle( vproto_id( "none" ), p, 270, 0, 0 );

    if( !veh ) {
        debugmsg( "error constructing vehicle" );
        return;
    }
    veh->name = name;
    veh->install_part( point_zero, vpart_from_item( g->u.lastconsumed ) );

    // Update the vehicle cache immediately,
    // or the vehicle will be invisible for the first couple of turns.
    here.add_vehicle_to_cache( veh );
}

void construct::done_deconstruct( const tripoint &p )
{
    map &here = get_map();
    // TODO: Make this the argument
    if( here.has_furn( p ) ) {
        const furn_t &f = here.furn( p ).obj();
        if( !f.deconstruct.can_do ) {
            add_msg( m_info, _( "That %s can not be disassembled!" ), f.name() );
            return;
        }
        if( f.deconstruct.furn_set.str().empty() ) {
            here.furn_set( p, f_null );
        } else {
            here.furn_set( p, f.deconstruct.furn_set );
        }
        add_msg( _( "The %s is disassembled." ), f.name() );
        here.spawn_items( p, item_group::items_from( f.deconstruct.drop_group, calendar::turn ) );
        // HACK: Hack alert.
        // Signs have cosmetics associated with them on the submap since
        // furniture can't store dynamic data to disk. To prevent writing
        // mysteriously appearing for a sign later built here, remove the
        // writing from the submap.
        here.delete_signage( p );
    } else {
        const ter_t &t = here.ter( p ).obj();
        if( !t.deconstruct.can_do ) {
            add_msg( _( "That %s can not be disassembled!" ), t.name() );
            return;
        }
        if( t.deconstruct.deconstruct_above ) {
            const tripoint top = p + tripoint_above;
            if( here.has_furn( top ) ) {
                add_msg( _( "That %s can not be disassembled, since there is furniture above it." ), t.name() );
                return;
            }
            done_deconstruct( top );
        }
        if( t.id.id() == t_console_broken )  {
            if( g->u.get_skill_level( skill_electronics ) >= 1 ) {
                g->u.practice( skill_electronics, 20, 4 );
            }
        }
        if( t.id.id() == t_console )  {
            if( g->u.get_skill_level( skill_electronics ) >= 1 ) {
                g->u.practice( skill_electronics, 40, 8 );
            }
        }
        here.ter_set( p, t.deconstruct.ter_set );
        add_msg( _( "The %s is disassembled." ), t.name() );
        here.spawn_items( p, item_group::items_from( t.deconstruct.drop_group, calendar::turn ) );
    }
}

static void unroll_digging( const int numer_of_2x4s )
{
    // refund components!
    item rope( "rope_30" );
    map &here = get_map();
    here.add_item_or_charges( g->u.pos(), rope );
    // presuming 2x4 to conserve lumber.
    here.spawn_item( g->u.pos(), itype_2x4, numer_of_2x4s );
}

void construct::done_digormine_stair( const tripoint &p, bool dig )
{
    map &here = get_map();
    const tripoint abs_pos = here.getabs( p );
    const tripoint pos_sm = ms_to_sm_copy( abs_pos );
    tinymap tmpmap;
    tmpmap.load( tripoint( pos_sm.xy(), pos_sm.z - 1 ), false );
    const tripoint local_tmp = tmpmap.getlocal( abs_pos );

    bool dig_muts = g->u.has_trait( trait_PAINRESIST_TROGLO ) || g->u.has_trait( trait_STOCKY_TROGLO );

    int no_mut_penalty = dig_muts ? 10 : 0;
    int mine_penalty = dig ? 0 : 10;
    g->u.mod_stored_nutr( 5 + mine_penalty + no_mut_penalty );
    g->u.mod_thirst( 5 + mine_penalty + no_mut_penalty );
    g->u.mod_fatigue( 10 + mine_penalty + no_mut_penalty );

    if( tmpmap.ter( local_tmp ) == t_lava ) {
        if( !( query_yn( _( "The rock feels much warmer than normal.  Proceed?" ) ) ) ) {
            here.ter_set( p, t_pit ); // You dug down a bit before detecting the problem
            unroll_digging( dig ? 8 : 12 );
        } else {
            add_msg( m_warning, _( "You just tunneled into lava!" ) );
            g->events().send<event_type::digs_into_lava>();
            here.ter_set( p, t_hole );
        }

        return;
    }

    bool impassable = tmpmap.impassable( local_tmp );
    if( !impassable ) {
        add_msg( _( "You dig into a preexisting space, and improvise a ladder." ) );
    } else if( dig ) {
        add_msg( _( "You dig a stairway, adding sturdy timbers and a rope for safety." ) );
    } else {
        add_msg( _( "You drill out a passage, heading deeper underground." ) );
    }
    here.ter_set( p, t_stairs_down ); // There's the top half
    // Again, need to use submap-local coordinates.
    tmpmap.ter_set( local_tmp, impassable ? t_stairs_up : t_ladder_up ); // and there's the bottom half.
    // And save to the center coordinate of the current active map.
    tmpmap.save();
}

void construct::done_dig_stair( const tripoint &p )
{
    done_digormine_stair( p, true );
}

void construct::done_mine_downstair( const tripoint &p )
{
    done_digormine_stair( p, false );
}

void construct::done_mine_upstair( const tripoint &p )
{
    map &here = get_map();
    const tripoint abs_pos = here.getabs( p );
    const tripoint pos_sm = ms_to_sm_copy( abs_pos );
    tinymap tmpmap;
    tmpmap.load( tripoint( pos_sm.xy(), pos_sm.z + 1 ), false );
    const tripoint local_tmp = tmpmap.getlocal( abs_pos );

    if( tmpmap.ter( local_tmp ) == t_lava ) {
        here.ter_set( p.xy(), t_rock_floor ); // You dug a bit before discovering the problem
        add_msg( m_warning, _( "The rock overhead feels hot.  You decide *not* to mine magma." ) );
        unroll_digging( 12 );
        return;
    }

    if( tmpmap.has_flag_ter( TFLAG_SHALLOW_WATER, local_tmp ) ||
        tmpmap.has_flag_ter( TFLAG_DEEP_WATER, local_tmp ) ) {
        here.ter_set( p.xy(), t_rock_floor ); // You dug a bit before discovering the problem
        add_msg( m_warning, _( "The rock above is rather damp.  You decide *not* to mine water." ) );
        unroll_digging( 12 );
        return;
    }

    bool dig_muts = g->u.has_trait( trait_PAINRESIST_TROGLO ) || g->u.has_trait( trait_STOCKY_TROGLO );

    int no_mut_penalty = dig_muts ? 15 : 0;
    g->u.mod_stored_nutr( 20 + no_mut_penalty );
    g->u.mod_thirst( 20 + no_mut_penalty );
    g->u.mod_fatigue( 25 + no_mut_penalty );

    add_msg( _( "You drill out a passage, heading for the surface." ) );
    here.ter_set( p.xy(), t_stairs_up ); // There's the bottom half
    // We need to write to submap-local coordinates.
    tmpmap.ter_set( local_tmp, t_stairs_down ); // and there's the top half.
    tmpmap.save();
}

void construct::done_wood_stairs( const tripoint &p )
{
    const tripoint top = p + tripoint_above;
    get_map().ter_set( top, ter_id( "t_wood_stairs_down" ) );
}

void construct::done_window_curtains( const tripoint & )
{
    map &here = get_map();
    // copied from iexamine::curtains
    here.spawn_item( g->u.pos(), itype_nail, 1, 4 );
    here.spawn_item( g->u.pos(), itype_sheet, 2 );
    here.spawn_item( g->u.pos(), itype_stick );
    here.spawn_item( g->u.pos(), itype_string_36 );
    g->u.add_msg_if_player(
        _( "After boarding up the window the curtains and curtain rod are left." ) );
}

void construct::done_extract_maybe_revert_to_dirt( const tripoint &p )
{
    map &here = get_map();
    if( one_in( 10 ) ) {
        here.ter_set( p, t_dirt );
    }

    if( here.ter( p ) == t_clay ) {
        add_msg( _( "You gather some clay." ) );
    } else if( here.ter( p ) == t_sand ) {
        add_msg( _( "You gather some sand." ) );
    } else {
        // Fall through to an undefined material.
        add_msg( _( "You gather some materials." ) );
    }
}

void construct::done_mark_firewood( const tripoint &p )
{
    get_map().trap_set( p, tr_firewood_source );
}

void construct::done_mark_practice_target( const tripoint &p )
{
    get_map().trap_set( p, tr_practice_target );
}

void construct::failure_standard( const tripoint & )
{
    add_msg( m_info, _( "You cannot build there!" ) );
}

void construct::failure_deconstruct( const tripoint & )
{
    add_msg( m_info, _( "You cannot deconstruct this!" ) );
}

template <typename T>
void assign_or_debugmsg( T &dest, const std::string &fun_id,
                         const std::map<std::string, T> &possible )
{
    const auto iter = possible.find( fun_id );
    if( iter != possible.end() ) {
        dest = iter->second;
    } else {
        dest = possible.find( "" )->second;
        const std::string list_available = enumerate_as_string( possible.begin(), possible.end(),
        []( const std::pair<std::string, T> &pr ) {
            return pr.first;
        } );
        debugmsg( "Unknown function: %s, available values are %s", fun_id.c_str(), list_available );
    }
}

void load_construction( const JsonObject &jo )
{
    construction con;
    // These ids are only temporary. The actual ids are determined in finalize_construction,
    // after removing blacklisted constructions.
    con.id = construction_id( -1 );
    con.str_id = construction_str_id( jo.get_string( "id" ) );
    if( con.str_id.is_null() ) {
        jo.throw_error( "Null construction id specified", "id" );
    } else if( construction_id_map.find( con.str_id ) != construction_id_map.end() ) {
        jo.throw_error( "Duplicate construction id", "id" );
    }

    con.description = jo.get_string( "description" );
    if( jo.has_member( "required_skills" ) ) {
        for( JsonArray arr : jo.get_array( "required_skills" ) ) {
            con.required_skills[skill_id( arr.get_string( 0 ) )] = arr.get_int( 1 );
        }
    } else {
        skill_id legacy_skill( jo.get_string( "skill", skill_fabrication.str() ) );
        int legacy_diff = jo.get_int( "difficulty" );
        con.required_skills[ legacy_skill ] = legacy_diff;
    }

    con.category = construction_category_id( jo.get_string( "category", "OTHER" ) );
    if( jo.has_int( "time" ) ) {
        con.time = to_moves<int>( time_duration::from_minutes( jo.get_int( "time" ) ) );
    } else if( jo.has_string( "time" ) ) {
        con.time = to_moves<int>( read_from_json_string<time_duration>( *jo.get_raw( "time" ),
                                  time_duration::units ) );
    }

    const requirement_id req_id( "inline_construction_" + con.str_id.str() );
    requirement_data::load_requirement( jo, req_id );
    con.requirements = req_id;

    if( jo.has_string( "using" ) ) {
        con.reqs_using = { { requirement_id( jo.get_string( "using" ) ), 1} };
    } else if( jo.has_array( "using" ) ) {
        for( JsonArray cur : jo.get_array( "using" ) ) {
            con.reqs_using.emplace_back( requirement_id( cur.get_string( 0 ) ), cur.get_int( 1 ) );
        }
    }

    con.pre_note = jo.get_string( "pre_note", "" );
    con.pre_terrain = jo.get_string( "pre_terrain", "" );
    if( con.pre_terrain.size() > 1
        && con.pre_terrain[0] == 'f'
        && con.pre_terrain[1] == '_' ) {
        con.pre_is_furniture = true;
    }

    con.post_terrain = jo.get_string( "post_terrain", "" );
    if( con.post_terrain.size() > 1
        && con.post_terrain[0] == 'f'
        && con.post_terrain[1] == '_' ) {
        con.post_is_furniture = true;
    }

    con.pre_flags = jo.get_tags( "pre_flags" );

    con.post_flags = jo.get_tags( "post_flags" );

    if( jo.has_member( "byproducts" ) ) {
        con.byproduct_item_group = item_group::load_item_group( jo.get_member( "byproducts" ),
                                   "collection" );
    }

    static const std::map<std::string, std::function<bool( const tripoint & )>> pre_special_map = {{
            { "", construct::check_nothing },
            { "check_empty", construct::check_empty },
            { "check_support", construct::check_support },
            { "check_deconstruct", construct::check_deconstruct },
            { "check_empty_up_OK", construct::check_empty_up_OK },
            { "check_up_OK", construct::check_up_OK },
            { "check_down_OK", construct::check_down_OK },
            { "check_no_trap", construct::check_no_trap }
        }
    };
    static const std::map<std::string, std::function<void( const tripoint & )>> post_special_map = {{
            { "", construct::done_nothing },
            { "done_trunk_plank", construct::done_trunk_plank },
            { "done_grave", construct::done_grave },
            { "done_vehicle", construct::done_vehicle },
            { "done_deconstruct", construct::done_deconstruct },
            { "done_dig_stair", construct::done_dig_stair },
            { "done_mine_downstair", construct::done_mine_downstair },
            { "done_mine_upstair", construct::done_mine_upstair },
            { "done_wood_stairs", construct::done_wood_stairs },
            { "done_window_curtains", construct::done_window_curtains },
            { "done_extract_maybe_revert_to_dirt", construct::done_extract_maybe_revert_to_dirt },
            { "done_mark_firewood", construct::done_mark_firewood },
            { "done_mark_practice_target", construct::done_mark_practice_target }
        }
    };
    std::map<std::string, std::function<void( const tripoint & )>> explain_fail_map;
    if( jo.has_string( "pre_special" ) &&
        jo.get_string( "pre_special" )  == std::string( "check_deconstruct" ) ) {
        explain_fail_map[""] = construct::failure_deconstruct;
    } else {
        explain_fail_map[""] = construct::failure_standard;
    }

    assign_or_debugmsg( con.pre_special, jo.get_string( "pre_special", "" ), pre_special_map );
    assign_or_debugmsg( con.post_special, jo.get_string( "post_special", "" ), post_special_map );
    assign_or_debugmsg( con.explain_failure, jo.get_string( "explain_failure", "" ), explain_fail_map );
    con.vehicle_start = jo.get_bool( "vehicle_start", false );

    con.on_display = jo.get_bool( "on_display", true );
    con.dark_craftable = jo.get_bool( "dark_craftable", false );

    constructions.push_back( con );
    construction_id_map.emplace( con.str_id, con.id );
}

void reset_constructions()
{
    constructions.clear();
    construction_id_map.clear();
    finalized = false;
}

void check_constructions()
{
    for( size_t i = 0; i < constructions.size(); i++ ) {
        const construction &c = constructions[ i ];
        const std::string display_name = std::string( "construction " ) + c.description;
        // Note: print the description as the id is just a generated number,
        // the description can be searched for in the json files.
        for( const auto &pr : c.required_skills ) {
            if( !pr.first.is_valid() ) {
                debugmsg( "Unknown skill %s in %s", pr.first.c_str(), display_name );
            }
        }

        if( !c.requirements.is_valid() ) {
            debugmsg( "construction %s has missing requirement data %s",
                      display_name, c.requirements.c_str() );
        }

        if( !c.pre_terrain.empty() ) {
            if( c.pre_is_furniture ) {
                if( !furn_str_id( c.pre_terrain ).is_valid() ) {
                    debugmsg( "Unknown pre_terrain (furniture) %s in %s", c.pre_terrain, display_name );
                }
            } else if( !ter_str_id( c.pre_terrain ).is_valid() ) {
                debugmsg( "Unknown pre_terrain (terrain) %s in %s", c.pre_terrain, display_name );
            }
        }
        if( !c.post_terrain.empty() ) {
            if( c.post_is_furniture ) {
                if( !furn_str_id( c.post_terrain ).is_valid() ) {
                    debugmsg( "Unknown post_terrain (furniture) %s in %s", c.post_terrain, display_name );
                }
            } else if( !ter_str_id( c.post_terrain ).is_valid() ) {
                debugmsg( "Unknown post_terrain (terrain) %s in %s", c.post_terrain, display_name );
            }
        }
        if( c.id != construction_id( i ) ) {
            debugmsg( "Construction \"%s\" has id %u, but should have %u",
                      c.description, c.id.to_i(), i );
        }
        if( construction_id_map.find( c.str_id ) == construction_id_map.end() ) {
            debugmsg( "Construction \"%s\" has invalid string id %s",
                      c.description, c.str_id.str() );
        } else if( construction_id_map[c.str_id] != construction_id( i ) ) {
            debugmsg( "Construction \"%s\" has string id \"%s\" that points to int id %u, but should point to %u",
                      c.description, c.str_id.str(), construction_id_map[c.str_id].to_i(), i );
        }
    }
}

int construction::print_time( const catacurses::window &w, const point &p, int width,
                              nc_color col ) const
{
    std::string text = get_time_string();
    return fold_and_print( w, p, width, col, text );
}

float construction::time_scale() const
{
    //incorporate construction time scaling
    if( get_option<int>( "CONSTRUCTION_SCALING" ) == 0 ) {
        return calendar::season_ratio();
    } else {
        return get_option<int>( "CONSTRUCTION_SCALING" ) / 100.0;
    }
}

int construction::adjusted_time() const
{
    int final_time = time;
    int assistants = 0;

    for( auto &elem : g->u.get_crafting_helpers() ) {
        if( elem->meets_skill_requirements( *this ) ) {
            assistants++;
        }
    }

    if( assistants >= 2 ) {
        final_time *= 0.4f;
    } else if( assistants == 1 ) {
        final_time *= 0.75f;
    }

    final_time *= time_scale();

    return final_time;
}

std::string construction::get_time_string() const
{
    const time_duration turns = time_duration::from_turns( adjusted_time() / 100 );
    return _( "Time to complete: " ) + colorize( to_string( turns ), color_data );
}

std::vector<std::string> construction::get_folded_time_string( int width ) const
{
    std::string time_text = get_time_string();
    std::vector<std::string> folded_time = foldstring( time_text, width );
    return folded_time;
}

void finalize_constructions()
{
    std::vector<item_comp> frame_items;
    for( const auto &e : vpart_info::all() ) {
        const vpart_info &vp = e.second;
        if( !vp.has_flag( flag_INITIAL_PART ) ) {
            continue;
        }
        frame_items.push_back( item_comp( vp.item, 1 ) );
    }

    if( frame_items.empty() ) {
        debugmsg( "No valid frames detected for vehicle construction" );
    }

    for( construction &con : constructions ) {
        if( con.vehicle_start ) {
            const_cast<requirement_data &>( con.requirements.obj() ).get_components().push_back( frame_items );
        }
        bool is_valid_construction_category = false;
        for( const construction_category &cc : construction_categories::get_all() ) {
            if( con.category == cc.id ) {
                is_valid_construction_category = true;
                break;
            }
        }
        if( !is_valid_construction_category ) {
            debugmsg( "Invalid construction category (%s) defined for construction (%s)", con.category.str(),
                      con.description );
        }
        requirement_data requirements_ = std::accumulate( con.reqs_using.begin(), con.reqs_using.end(),
                                         *con.requirements,
        []( const requirement_data & lhs, const std::pair<requirement_id, int> &rhs ) {
            return lhs + ( *rhs.first * rhs.second );
        } );

        requirement_data::save_requirement( requirements_, con.requirements );
        con.reqs_using.clear();
    }

    constructions.erase( std::remove_if( constructions.begin(), constructions.end(),
    [&]( const construction & c ) {
        return c.requirements->is_blacklisted();
    } ), constructions.end() );

    construction_id_map.clear();
    for( size_t i = 0; i < constructions.size(); i++ ) {
        constructions[ i ].id = construction_id( i );
        construction_id_map.emplace( constructions[i].str_id, constructions[i].id );
    }

    finalized = true;
}

void get_build_reqs_for_furn_ter_ids( const std::pair<std::map<ter_id, int>,
                                      std::map<furn_id, int>> &changed_ids,
                                      build_reqs &total_reqs )
{
    if( !finalized ) {
        debugmsg( "get_build_reqs_for_furn_ter_ids called before finalization" );
        return;
    }
    std::map<construction_id, int> total_builds;

    // iteratively recurse through the pre-terrains until the pre-terrain is empty, adding
    // the constructions to the total_builds map
    const auto add_builds = [&total_builds]( const construction & build, int count ) {
        if( total_builds.find( build.id ) == total_builds.end() ) {
            total_builds[build.id] = 0;
        }
        total_builds[build.id] += count;
        std::string build_pre_ter = build.pre_terrain;
        while( !build_pre_ter.empty() ) {
            for( const construction &pre_build : constructions ) {
                if( pre_build.category == construction_category_REPAIR ) {
                    continue;
                }
                if( pre_build.post_terrain == build_pre_ter ) {
                    if( total_builds.find( pre_build.id ) == total_builds.end() ) {
                        total_builds[pre_build.id] = 0;
                    }
                    total_builds[pre_build.id] += count;
                    build_pre_ter = pre_build.pre_terrain;
                    break;
                }
            }
            break;
        }
    };

    // go through the list of terrains and add their constructions and any pre-constructions
    // to the map of total builds
    for( const auto &ter_data : changed_ids.first ) {
        for( const construction &build : constructions ) {
            if( build.post_terrain.empty() || build.post_is_furniture ||
                build.category == construction_category_REPAIR ) {
                continue;
            }
            if( ter_id( build.post_terrain ) == ter_data.first ) {
                add_builds( build, ter_data.second );
                break;
            }
        }
    }
    // same, but for furniture
    for( const auto &furn_data : changed_ids.second ) {
        for( const construction &build : constructions ) {
            if( build.post_terrain.empty() || !build.post_is_furniture ||
                build.category == construction_category_REPAIR ) {
                continue;
            }
            if( furn_id( build.post_terrain ) == furn_data.first ) {
                add_builds( build, furn_data.second );
                break;
            }
        }
    }

    for( const auto &build_data : total_builds ) {
        const construction &build = build_data.first.obj();
        const int count = build_data.second;
        total_reqs.time += build.time * count;
        if( total_reqs.reqs.find( build.requirements ) == total_reqs.reqs.end() ) {
            total_reqs.reqs[build.requirements] = 0;
        }
        total_reqs.reqs[build.requirements] += count;
        for( const auto &req_skill : build.required_skills ) {
            if( total_reqs.skills.find( req_skill.first ) == total_reqs.skills.end() ) {
                total_reqs.skills[req_skill.first] = req_skill.second;
            } else if( total_reqs.skills[req_skill.first] < req_skill.second ) {
                total_reqs.skills[req_skill.first] = req_skill.second;
            }
        }
    }
}

static const construction null_construction {};

template <>
const construction_str_id &construction_id::id() const
{
    if( !finalized ) {
        debugmsg( "construction_id::id called before finalization" );
        return construction_str_id::NULL_ID();
    } else if( is_valid() ) {
        return constructions[to_i()].str_id;
    } else {
        if( to_i() != -1 ) {
            debugmsg( "Invalid construction id %d", to_i() );
        }
        return construction_str_id::NULL_ID();
    }
}

template <>
const construction &construction_id::obj() const
{
    if( !finalized ) {
        debugmsg( "construction_id::obj called before finalization" );
        return null_construction;
    } else if( is_valid() ) {
        return constructions[to_i()];
    } else {
        debugmsg( "Invalid construction id %d", to_i() );
        return null_construction;
    }
}

template <>
bool construction_id::is_valid() const
{
    if( !finalized ) {
        debugmsg( "construction_id::is_valid called before finalization" );
        return false;
    }
    return to_i() >= 0 && static_cast<size_t>( to_i() ) < constructions.size();
}

template <>
construction_id construction_str_id::id() const
{
    if( !finalized ) {
        debugmsg( "construction_str_id::id called before finalization" );
        return construction_id( -1 );
    }
    auto it = construction_id_map.find( *this );
    if( it != construction_id_map.end() ) {
        return it->second;
    } else {
        if( !is_null() ) {
            debugmsg( "Invalid construction str id %s", str() );
        }
        return construction_id( -1 );
    }
}

template <>
const construction &construction_str_id::obj() const
{
    if( !finalized ) {
        debugmsg( "construction_str_id::obj called before finalization" );
        return null_construction;
    }
    auto it = construction_id_map.find( *this );
    if( it != construction_id_map.end() ) {
        return it->second.obj();
    } else {
        debugmsg( "Invalid construction str id %s", str() );
        return null_construction;
    }
}

template <>
bool construction_str_id::is_valid() const
{
    if( !finalized ) {
        debugmsg( "construction_str_id::is_valid called before finalization" );
        return false;
    }
    return construction_id_map.find( *this ) != construction_id_map.end();
}
