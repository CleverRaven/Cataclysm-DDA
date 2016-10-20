#include "construction.h"

#include "coordinate_conversions.h"
#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "debug.h"
#include "input.h"
#include "output.h"
#include "player.h"
#include "inventory.h"
#include "mapdata.h"
#include "skill.h"
#include "action.h"
#include "translations.h"
#include "messages.h"
#include "rng.h"
#include "requirements.h"
#include "trap.h"
#include "overmapbuffer.h"
#include "options.h"
#include "npc.h"
#include "iuse.h"
#include "veh_type.h"
#include "vehicle.h"
#include "item_group.h"
#include "cata_utility.h"
#include "uistate.h"

#include <algorithm>
#include <map>
#include <sstream>

static const skill_id skill_electronics( "electronics" );
static const skill_id skill_carpentry( "carpentry" );
static const skill_id skill_unarmed( "unarmed" );
static const skill_id skill_throw( "throw" );

// Construction functions.
namespace construct
{
// Checks for whether terrain mod can proceed
bool check_nothing( const tripoint & )
{
    return true;
}
bool check_empty( const tripoint & ); // tile is empty
bool check_support( const tripoint & ); // at least two orthogonal supports
bool check_deconstruct( const tripoint & ); // either terrain or furniture must be deconstructable
bool check_up_OK( const tripoint & ); // tile is empty and you're not on the surface
bool check_down_OK( const tripoint & ); // tile is empty and you're not on z-10 already

// Special actions to be run post-terrain-mod
void done_nothing( const tripoint & ) {}
void done_tree( const tripoint & );
void done_trunk_log( const tripoint & );
void done_trunk_plank( const tripoint & );
void done_vehicle( const tripoint & );
void done_deconstruct( const tripoint & );
void done_digormine_stair( const tripoint &, bool );
void done_dig_stair( const tripoint & );
void done_mine_downstair( const tripoint & );
void done_mine_upstair( const tripoint & );
void done_window_curtains( const tripoint & );
};

// Helper functions, nobody but us needs to call these.
static bool can_construct( const std::string &desc );
static bool can_construct( const construction &con );
static bool player_can_build( player &p, const inventory &inv, const construction &con );
static bool player_can_build( player &p, const inventory &pinv, const std::string &desc );
static void place_construction( const std::string &desc );

std::vector<construction> constructions;

void standardize_construction_times( int const time )
{
    for( auto &c : constructions ) {
        c.time = time;
    }
}

std::vector<construction *> constructions_by_desc( const std::string &description )
{
    std::vector<construction *> result;
    for( auto &constructions_a : constructions ) {
        if( constructions_a.description == description ) {
            result.push_back( &constructions_a );
        }
    }
    return result;
}

void load_available_constructions( std::vector<std::string> &available,
                                   std::map<std::string, std::vector<std::string>> &cat_available,
                                   bool hide_unconstructable )
{
    cat_available.clear();
    available.clear();
    for( auto &it : constructions ) {
        if( !hide_unconstructable || can_construct( it ) ) {
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

void draw_grid( WINDOW *w, const int list_width )
{
    draw_border( w );
    mvwprintz( w, 0, 2, c_ltred, _( " Construction " ) );
    // draw internal lines
    mvwvline( w, 1, list_width, LINE_XOXO, getmaxy( w ) - 2 );
    mvwhline( w, 2, 1, LINE_OXOX, list_width );
    // draw intersections
    mvwputch( w, 0, list_width, c_ltgray, LINE_OXXX );
    mvwputch( w, getmaxy( w ) - 1, list_width, c_ltgray, LINE_XXOX );
    mvwputch( w, 2, 0, c_ltgray, LINE_XXXO );
    mvwputch( w, 2, list_width, c_ltgray, LINE_XOXX );

    wrefresh( w );
}

nc_color construction_color( std::string &con_name, bool highlight )
{
    nc_color col = c_dkgray;
    if( g->u.has_trait( "DEBUG_HS" ) ) {
        col = c_white;
    } else if( can_construct( con_name ) ) {
        construction *con_first = nullptr;
        std::vector<construction *> cons = constructions_by_desc( con_name );
        const inventory &total_inv = g->u.crafting_inventory();
        for( auto &con : cons ) {
            if( con->requirements->can_make_with_inventory( total_inv ) ) {
                con_first = con;
                break;
            }
        }
        if( con_first != nullptr ) {
            int pskill = g->u.get_skill_level( con_first->skill );
            int diff = con_first->difficulty;
            if( pskill < diff ) {
                col = c_red;
            } else if( pskill == diff ) {
                col = c_ltblue;
            } else {
                col = c_white;
            }
        }
    }
    return highlight ? hilite( col ) : col;
}

void construction_menu()
{
    static bool hide_unconstructable = false;
    // only display constructions the player can theoretically perform
    std::vector<std::string> available;
    std::map<std::string, std::vector<std::string>> cat_available;
    load_available_constructions( available, cat_available, hide_unconstructable );

    if( available.empty() ) {
        popup( _( "You can not construct anything here." ) );
        return;
    }

    int w_height = TERMY;
    if( ( int )available.size() + 2 < w_height ) {
        w_height = available.size() + 2;
    }
    if( w_height < FULL_SCREEN_HEIGHT ) {
        w_height = FULL_SCREEN_HEIGHT;
    }

    const int w_width = std::max( FULL_SCREEN_WIDTH, TERMX * 2 / 3);
    const int w_y0 = ( TERMY > w_height ) ? ( TERMY - w_height ) / 2 : 0;
    const int w_x0 = ( TERMX > w_width ) ? ( TERMX - w_width ) / 2 : 0;
    WINDOW_PTR w_con_ptr {newwin( w_height, w_width, w_y0, w_x0 )};
    WINDOW *const w_con = w_con_ptr.get();

    const int w_list_width = int( .375 * w_width );
    const int w_list_height = w_height - 4;
    const int w_list_x0 = 1;
    WINDOW_PTR w_list_ptr {newwin( w_list_height, w_list_width, w_y0 + 3, w_x0 + w_list_x0 )};
    WINDOW *const w_list = w_list_ptr.get();

    draw_grid( w_con, w_list_width + w_list_x0 );

    //tabcount needs to be increased to add more categories
    int tabcount = 10;
    std::string construct_cat[] = {_( "All" ), _( "Constructions" ), _( "Furniture" ),
                                   _( "Digging and Mining" ), _( "Repairing" ),
                                   _( "Reinforcing" ), _( "Decorative" ),
                                   _( "Farming and Woodcutting" ), _( "Others" ),
                                   _( "Filter" )
                                  };

    bool update_info = true;
    bool update_cat = true;
    bool isnew = true;
    int tabindex = 0;
    int select = 0;
    int offset = 0;
    bool exit = false;
    std::string category_name = "";
    std::vector<std::string> constructs;
    //storage for the color text so it can be scrolled
    std::vector< std::vector < std::string > > construct_buffers;
    std::vector<std::string> full_construct_buffer;
    std::vector<int> construct_buffer_breakpoints;
    int total_project_breakpoints = 0;
    int current_construct_breakpoint = 0;
    bool previous_hide_unconstructable = false;
    //track the cursor to determine when to refresh the list of construction recipes
    int previous_tabindex = -1;
    int previous_select = -1;

    const inventory &total_inv = g->u.crafting_inventory();

    input_context ctxt( "CONSTRUCTION" );
    ctxt.register_action( "UP", _( "Move cursor up" ) );
    ctxt.register_action( "DOWN", _( "Move cursor down" ) );
    ctxt.register_action( "RIGHT", _( "Move tab right" ) );
    ctxt.register_action( "LEFT", _( "Move tab left" ) );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "TOGGLE_UNAVAILABLE_CONSTRUCTIONS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "FILTER" );
    
    std::string filter;
    int previous_index = 0;
    do {
        if( update_cat ) {
            update_cat = false;
            switch( tabindex ) {
                case 0:
                    category_name = "ALL";
                    break;
                case 1:
                    category_name = "CONSTRUCT";
                    break;
                case 2:
                    category_name = "FURN";
                    break;
                case 3:
                    category_name = "DIG";
                    break;
                case 4:
                    category_name = "REPAIR";
                    break;
                case 5:
                    category_name = "REINFORCE";
                    break;
                case 6:
                    category_name = "DECORATE";
                    break;
                case 7:
                    category_name = "FARM_WOOD";
                    break;
                case 8:
                    category_name = "OTHER";
                    break;
                case 9:
                    category_name = "FILTER";
                    break;
            }
            
            if( category_name == "ALL" ) {
                constructs = available;
                previous_index = tabindex;
            } else if( category_name == "FILTER" ) {
                constructs.clear();
                std::copy_if( available.begin(), available.end(),
                    std::back_inserter( constructs ),
                    [&](const std::string &a){
                        return lcmatch(a, filter);
                    } );
            } else {
                constructs = cat_available[category_name];
                previous_index = tabindex;
            }
            if( isnew ){
                if( !uistate.last_construction.empty() ){
                    select = std::distance(constructs.begin(),
                                            std::find( constructs.begin(),
                                                        constructs.end(),
                                                        uistate.last_construction ));
                }
                filter = uistate.construction_filter;
            }
        }
        // Erase existing tab selection & list of constructions
        mvwhline( w_con, 1, 1, ' ', w_list_width );
        werase( w_list );
        // Print new tab listing
        mvwprintz( w_con, 1, 1, c_yellow, "<< %s >>", construct_cat[tabindex].c_str() );
        // Determine where in the master list to start printing
        calcStartPos( offset, select, w_list_height, constructs.size() );
        // Print the constructions between offset and max (or how many will fit)
        for( size_t i = 0; ( int )i < w_list_height && ( i + offset ) < constructs.size(); i++ ) {
            int current = i + offset;
            std::string con_name = constructs[current];
            bool highlight = ( current == select );

            trim_and_print( w_list, i, 0, w_list_width,
                            construction_color( con_name, highlight ), "%s",
                            con_name.c_str() );
        }

        if( update_info ) {
            update_info = false;
            // Clear out lines for tools & materials
            const int pos_x = ( w_list_width + w_list_x0 + 2 );
            for( int i = 1; i < w_height - 1; i++ ) {
                mvwhline( w_con, i, pos_x, ' ', w_width - pos_x - 1 );
            }

            //leave room for top and bottom UI text
            int available_buffer_height = w_height - 7 - 3;
            int available_window_width = w_width - ( w_list_width + w_list_x0 + 2 ) - 1;
            nc_color color_stage = c_white;

            if( !constructs.empty() ) {
                std::string current_desc = constructs[select];
                trim_and_print( w_con, w_height - 5, ( w_list_width + w_list_x0 + 2 ),
                           available_window_width, c_white,
                           _("Press %s or %s to tab."), 
                           ctxt.get_desc("LEFT").c_str(),
                           ctxt.get_desc("RIGHT").c_str() );
                mvwprintz( w_con, w_height - 4, ( w_list_width + w_list_x0 + 2 ), c_white,
                           _( "Press %s to search." ), ctxt.get_desc( "FILTER" ).c_str() );
                // Print instructions for toggling recipe hiding.
                mvwprintz( w_con, w_height - 3, ( w_list_width + w_list_x0 + 2 ), c_white,
                           _( "Press %s to toggle unavailable constructions." ),
                           ctxt.get_desc( "TOGGLE_UNAVAILABLE_CONSTRUCTIONS" ).c_str() );
                mvwprintz( w_con, w_height - 2, ( w_list_width + w_list_x0 + 2 ), c_white,
                           _( "Press %s to view and edit key-bindings." ),
                           ctxt.get_desc( "HELP_KEYBINDINGS" ).c_str() );
                // Print construction name
                mvwprintz( w_con, 1, ( w_list_width + w_list_x0 + 2 ), c_white, "%s", current_desc.c_str() );

                //only reconstruct the project list when moving away from the current item, or when changing the display mode
                if( previous_select != select || previous_tabindex != tabindex ||
                    previous_hide_unconstructable != hide_unconstructable ) {
                    previous_select = select;
                    previous_tabindex = tabindex;
                    previous_hide_unconstructable = hide_unconstructable;

                    //construct the project list buffer

                    // Print stages and their requirement.
                    std::vector<construction *> options = constructions_by_desc( current_desc );

                    construct_buffers.clear();
                    total_project_breakpoints = 0;
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
                        current_con->requirements->can_make_with_inventory( total_inv );

                        std::vector<std::string> current_buffer;
                        std::ostringstream current_line;

                        // display result only if more than one step.
                        // Assume single stage constructions should be clear
                        // in their description what their result is.
                        if( current_con->post_terrain != "" && options.size() > 1 ) {
                            //also print out stage number when multiple stages are available
                            current_line << _( "Stage #" ) << stage_counter;
                            current_buffer.push_back( current_line.str() );
                            current_line.str( "" );

                            std::string result_string;
                            if( current_con->post_is_furniture ) {
                                result_string = furn_str_id( current_con->post_terrain ).obj().name;
                            } else {
                                result_string = ter_str_id( current_con->post_terrain ).obj().name;
                            }
                            current_line << "<color_" << string_from_color( color_stage ) << ">" << string_format(
                                             _( "Result: %s" ), result_string.c_str() ) << "</color>";
                            std::vector<std::string> folded_result_string = foldstring( current_line.str(),
                                    available_window_width );
                            current_buffer.insert( current_buffer.end(), folded_result_string.begin(),
                                                   folded_result_string.end() );
                        }

                        current_line.str( "" );
                        // display required skill and difficulty
                        int pskill = g->u.get_skill_level( current_con->skill );
                        int diff = ( current_con->difficulty > 0 ) ? current_con->difficulty : 0;

                        current_line << "<color_" << string_from_color( ( pskill >= diff ? c_white : c_red ) ) << ">" <<
                                     string_format( _( "Skill Req: %d (%s)" ), diff,
                                                    current_con->skill.obj().name().c_str() ) << "</color>";
                        current_buffer.push_back( current_line.str() );
                        // TODO: Textify pre_flags to provide a bit more information.
                        // Example: First step of dig pit could say something about
                        // requiring diggable ground.
                        current_line.str( "" );
                        if( current_con->pre_terrain != "" ) {
                            std::string require_string;
                            if( current_con->pre_is_furniture ) {
                                require_string = furn_str_id( current_con->pre_terrain ).obj().name;
                            } else {
                                require_string = ter_str_id( current_con->pre_terrain ).obj().name;
                            }
                            current_line << "<color_" << string_from_color( color_stage ) << ">" << string_format(
                                             _( "Requires: %s" ), require_string.c_str() ) << "</color>";
                            std::vector<std::string> folded_result_string = foldstring( current_line.str(),
                                    available_window_width );
                            current_buffer.insert( current_buffer.end(), folded_result_string.begin(),
                                                   folded_result_string.end() );
                        }
                        // get pre-folded versions of the rest of the construction project to be displayed later

                        // get time needed
                        std::vector<std::string> folded_time = current_con->get_folded_time_string(
                                available_window_width );
                        current_buffer.insert( current_buffer.end(), folded_time.begin(), folded_time.end() );

                        std::vector<std::string> folded_tools = current_con->requirements->get_folded_tools_list(
                                available_window_width, color_stage, total_inv );
                        current_buffer.insert( current_buffer.end(), folded_tools.begin(), folded_tools.end() );

                        std::vector<std::string> folded_components = current_con->requirements->get_folded_components_list(
                                    available_window_width, color_stage, total_inv );
                        current_buffer.insert( current_buffer.end(), folded_components.begin(), folded_components.end() );

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
                            full_construct_buffer.push_back( std::string( "" ) );
                            current_buffer_location++;
                        }
                    }
                    total_project_breakpoints = static_cast<int>( construct_buffer_breakpoints.size() );
                }
                if( current_construct_breakpoint > 0 ) {
                    // Print previous stage indicator if breakpoint is past the beginning
                    mvwprintz( w_con, 2, ( w_list_width + w_list_x0 + 2 ), c_white, _( "^ [P]revious stage(s)" ) );
                }
                if( static_cast<size_t>( construct_buffer_breakpoints[current_construct_breakpoint] +
                                         available_buffer_height ) < full_construct_buffer.size() ) {
                    // Print next stage indicator if more breakpoints are remaining after screen height
                    mvwprintz( w_con, w_height - 6, ( w_list_width + w_list_x0 + 2 ), c_white, _( "v [N]ext stage(s)" ) );
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
                    print_colored_text( w_con, ypos++, ( w_list_width + w_list_x0 + 2 ), stored_color, color_stage, full_construct_buffer[i] );
                }
            }
        } // Finished updating

        draw_scrollbar( w_con, select, w_list_height, constructs.size(), 3 );
        wrefresh( w_con );
        wrefresh( w_list );

        const std::string action = ctxt.handle_input();
        if( action == "FILTER" ){
            filter = string_input_popup( _( "Search" ), 50, filter, "", _( "Filter" ), 100, false );
            if( !filter.empty() ){
                update_info = true;
                update_cat = true;
                tabindex = 9;
                select = 0;
            }else if( previous_index !=9 ){
                tabindex = previous_index;
                update_info = true;
                update_cat = true;
                select = 0;
            }
            uistate.construction_filter = filter;
        } else if( action == "DOWN" ) {
            update_info = true;
            if( select < ( int )constructs.size() - 1 ) {
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
            select = 0;
            tabindex--;
            if( tabindex < 0 ) {
                tabindex = tabcount - 1;
            }
        } else if( action == "RIGHT" ) {
            update_info = true;
            update_cat = true;
            select = 0;
            tabindex = ( tabindex + 1 ) % tabcount;
        } else if( action == "PAGE_UP" ) {
            update_info = true;
            if( current_construct_breakpoint > 0 ) {
                current_construct_breakpoint--;
            }
            if( current_construct_breakpoint < 0 ) {
                current_construct_breakpoint = 0;
            }
        } else if( action == "PAGE_DOWN" ) {
            update_info = true;
            if( current_construct_breakpoint < total_project_breakpoints - 1 ) {
                current_construct_breakpoint++;
            }
            if( current_construct_breakpoint >= total_project_breakpoints ) {
                current_construct_breakpoint = total_project_breakpoints - 1;
            }
        } else if( action == "QUIT" ) {
            exit = true;
        } else if( action == "HELP_KEYBINDINGS" ) {
            draw_grid( w_con, w_list_width + w_list_x0 );
        } else if( action == "TOGGLE_UNAVAILABLE_CONSTRUCTIONS" ) {
            update_info = true;
            update_cat = true;
            hide_unconstructable = !hide_unconstructable;
            select = 0;
            offset = 0;
            load_available_constructions( available, cat_available, hide_unconstructable );
        } else if( action == "CONFIRM" ) {
            if( player_can_build( g->u, total_inv, constructs[select] ) ) {
                place_construction( constructs[select] );
                uistate.last_construction = constructs[select];
                exit = true;
            } else {
                popup( _( "You can't build that!" ) );
                draw_grid( w_con, w_list_width + w_list_x0 );
                update_info = true;
            }
        }
    } while( !exit );

    w_list_ptr.reset();
    w_con_ptr.reset();
    g->refresh_all();
}

bool player_can_build( player &p, const inventory &pinv, const std::string &desc )
{
    // check all with the same desc to see if player can build any
    std::vector<construction *> cons = constructions_by_desc( desc );
    for( auto &con : cons ) {
        if( player_can_build( p, pinv, *con ) ) {
            return true;
        }
    }
    return false;
}

bool player_can_build( player &p, const inventory &pinv, const construction &con )
{
    if( p.has_trait( "DEBUG_HS" ) ) {
        return true;
    }

    if( p.get_skill_level( con.skill ) < con.difficulty ) {
        return false;
    }
    return con.requirements->can_make_with_inventory( pinv );
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
    if( !con.pre_terrain.empty() ) {
        if( con.pre_is_furniture ) {
            furn_id f = furn_id( con.pre_terrain );
            place_okay &= g->m.furn( p ) == f;
        } else {
            ter_id t = ter_id( con.pre_terrain );
            place_okay &= g->m.ter( p ) == t;
        }
    }
    // see if the flags check out
    place_okay &= std::all_of( con.pre_flags.begin(), con.pre_flags.end(),
        [&p] ( const std::string &flag ) {
        return g->m.has_flag( flag, p );
    });

    // make sure the construction would actually do something
    if( !con.post_terrain.empty() ) {
        if( con.post_is_furniture ) {
            furn_id f = furn_id( con.post_terrain );
            place_okay &= g->m.furn( p ) != f;
        } else {
            ter_id t = ter_id( con.post_terrain );
            place_okay &= g->m.ter( p ) != t;
        }
    }
    return place_okay;
}

bool can_construct( const construction &con )
{
    for( const tripoint &p : g->m.points_in_radius( g->u.pos(), 1 ) ) {
        if( p != g->u.pos() && can_construct( con, p ) ) {
            return true;
        }
    }
    return false;
}

void place_construction( const std::string &desc )
{
    g->refresh_all();
    const inventory &total_inv = g->u.crafting_inventory();

    std::vector<construction *> cons = constructions_by_desc( desc );
    std::map<tripoint, const construction *> valid;
    for( const tripoint &p : g->m.points_in_radius( g->u.pos(), 1 ) ) {
        for( const auto *con : cons ) {
            if( p != g->u.pos() && can_construct( *con, p ) && player_can_build( g->u, total_inv, *con ) ) {
                valid[ p ] = con;
            }
        }
    }

    for( auto &elem : valid ) {
        g->m.drawsq( g->w_terrain, g->u, elem.first, true, false,
                     g->u.pos() + g->u.view_offset );
    }
    wrefresh( g->w_terrain );

    tripoint dirp;
    if( !choose_adjacent( _( "Construct where?" ), dirp ) ) {
        return;
    }

    if( valid.find( dirp ) == valid.end() ) {
        add_msg( m_info, _( "You cannot build there!" ) );
        return;
    }

    const construction &con = *valid.find( dirp )->second;
    g->u.assign_activity( ACT_BUILD, con.adjusted_time(), con.id );
    g->u.activity.placement = dirp;
}

void complete_construction()
{
    player &u = g->u;
    const construction &built = constructions[u.activity.index];

    u.practice( built.skill, ( int )( ( 10 + 15 * built.difficulty ) * ( 1 + built.time / 30000.0 ) ),
                ( int )( built.difficulty * 1.25 ) );


    // Friendly NPCs gain exp from assisting or watching...
    for( auto &elem : g->u.get_crafting_helpers() ) {
        //If the NPC can understand what you are doing, they gain more exp
        if (elem->get_skill_level(built.skill) >= built.difficulty){
            elem->practice( built.skill, (int)( (10 + 15*built.difficulty) * (1 + built.time/30000.0) ),
                                (int)(built.difficulty * 1.25) );
            add_msg(m_info, _("%s assists you with the work..."), elem->name.c_str());
        //NPC near you isn't skilled enough to help
        } else {
            elem->practice( built.skill, (int)( (10 + 15*built.difficulty) * (1 + built.time/30000.0) ),
                                (int)(built.difficulty * 1.25) );
            add_msg(m_info, _("%s watches you work..."), elem->name.c_str());
        }
    }

    for( const auto &it : built.requirements->get_components() ) {
        u.consume_items( it );
    }
    for( const auto &it : built.requirements->get_tools() ) {
        u.consume_tools( it );
    }

    // Make the terrain change
    const tripoint terp = u.activity.placement;
    if( !built.post_terrain.empty() ) {
        if( built.post_is_furniture ) {
            g->m.furn_set( terp, furn_str_id( built.post_terrain ) );
        } else {
            g->m.ter_set( terp, ter_str_id( built.post_terrain ) );
        }
    }

    // clear the activity
    u.activity.type = ACT_NULL;

    // This comes after clearing the activity, in case the function interrupts
    // activities
    built.post_special( terp );
}

bool construct::check_empty( const tripoint &p )
{
    return ( g->m.has_flag( "FLAT", p ) && !g->m.has_furn( p ) &&
             g->is_empty( p ) && g->m.tr_at( p ).is_null() &&
             g->m.i_at( p ).empty() && g->m.veh_at( p ) == NULL );
}

bool construct::check_support( const tripoint &p )
{
    // need two or more orthogonally adjacent supports
    int num_supports = 0;
    if( g->m.impassable( p ) ) {
        return false;
    }
    if( g->m.has_flag( "SUPPORTS_ROOF", tripoint( p.x, p.y - 1, p.z ) ) ) {
        ++num_supports;
    }
    if( g->m.has_flag( "SUPPORTS_ROOF", tripoint( p.x, p.y + 1, p.z ) ) ) {
        ++num_supports;
    }
    if( g->m.has_flag( "SUPPORTS_ROOF", tripoint( p.x - 1, p.y, p.z ) ) ) {
        ++num_supports;
    }
    if( g->m.has_flag( "SUPPORTS_ROOF", tripoint( p.x + 1, p.y, p.z ) ) ) {
        ++num_supports;
    }
    return num_supports >= 2;
}

bool construct::check_deconstruct( const tripoint &p )
{
    if( g->m.has_furn( p.x, p.y ) ) {
        return g->m.furn( p.x, p.y ).obj().deconstruct.can_do;
    }
    // terrain can only be deconstructed when there is no furniture in the way
    return g->m.ter( p.x, p.y ).obj().deconstruct.can_do;
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

void construct::done_tree( const tripoint &p )
{
    tripoint dirp;
    while( !choose_direction( _( "Press a direction for the tree to fall in:" ), dirp ) ) {
        // try again
    }

    tripoint to = p + point( 3 * dirp.x + rng( -1, 1 ), 3 * dirp.y + rng( -1, 1 ) );
    std::vector<tripoint> tree = line_to( p, to, rng( 1, 8 ) );
    for( auto &elem : tree ) {
        g->m.destroy( elem );
        g->m.ter_set( elem, t_trunk );
    }
}

void construct::done_trunk_log( const tripoint &p )
{
    g->m.spawn_item( p.x, p.y, "log", rng( 5, 15 ), 0, calendar::turn );
}

void construct::done_trunk_plank( const tripoint &p )
{
    ( void )p; //unused
    int num_logs = rng( 5, 15 );
    for( int i = 0; i < num_logs; ++i ) {
        iuse::cut_log_into_planks( &( g->u ) );
    }
}

const vpart_str_id &vpart_from_item( const std::string &item_id )
{
    for( auto vp : vpart_info::get_all() ) {
        if( vp->item == item_id && vp->has_flag( "INITIAL_PART" ) ) {
            return vp->id;
        }
    }
    // The INITIAL_PART flag is optional, if no part (based on the given item) has it, just use the
    // first part that is based in the given item (this is fine for example if there is only one
    // such type anyway).
    for( auto vp : vpart_info::get_all() ) {
        if( vp->item == item_id ) {
            return vp->id;
        }
    }
    debugmsg( "item %s used by construction is not base item of any vehicle part!", item_id.c_str() );
    static const vpart_str_id frame_id( "frame_vertical_2" );
    return frame_id;
}

void construct::done_vehicle( const tripoint &p )
{
    std::string name = string_input_popup( _( "Enter new vehicle name:" ), 20 );
    if( name.empty() ) {
        name = _( "Car" );
    }

    vehicle *veh = g->m.add_vehicle( vproto_id( "none" ), p, 270, 0, 0 );

    if( !veh ) {
        debugmsg( "error constructing vehicle" );
        return;
    }
    veh->name = name;
    veh->install_part( 0, 0, vpart_from_item( g->u.lastconsumed ) );

    // Update the vehicle cache immediately,
    // or the vehicle will be invisible for the first couple of turns.
    g->m.add_vehicle_to_cache( veh );
}

void construct::done_deconstruct( const tripoint &p )
{
    // TODO: Make this the argument
    if( g->m.has_furn( p ) ) {
        const furn_t &f = g->m.furn( p ).obj();
        if( !f.deconstruct.can_do ) {
            add_msg( m_info, _( "That %s can not be disassembled!" ), f.name.c_str() );
            return;
        }
        if( f.deconstruct.furn_set.str().empty() ) {
            g->m.furn_set( p, f_null );
        } else {
            g->m.furn_set( p, f.deconstruct.furn_set );
        }
        add_msg( _( "You disassemble the %s." ), f.name.c_str() );
        g->m.spawn_items( p, item_group::items_from( f.deconstruct.drop_group, calendar::turn ) );
        // Hack alert.
        // Signs have cosmetics associated with them on the submap since
        // furniture can't store dynamic data to disk. To prevent writing
        // mysteriously appearing for a sign later built here, remove the
        // writing from the submap.
        g->m.delete_signage( p );
    } else {
        const ter_t &t = g->m.ter( p ).obj();
        if( !t.deconstruct.can_do ) {
            add_msg( _( "That %s can not be disassembled!" ), t.name.c_str() );
            return;
        }
        if( t.id == "t_console_broken" )  {
            if( g->u.get_skill_level( skill_electronics ) >= 1 ) {
                g->u.practice( skill_electronics, 20, 4 );
            }
        }
        if( t.id == "t_console" )  {
            if( g->u.get_skill_level( skill_electronics ) >= 1 ) {
                g->u.practice( skill_electronics, 40, 8 );
            }
        }
        g->m.ter_set( p, t.deconstruct.ter_set );
        add_msg( _( "You disassemble the %s." ), t.name.c_str() );
        g->m.spawn_items( p, item_group::items_from( t.deconstruct.drop_group, calendar::turn ) );
    }
}

void unroll_digging( int const numer_of_2x4s )
{
    // refund components!
    item rope( "rope_30" );
    g->m.add_item_or_charges( g->u.pos(), rope );
    // presuming 2x4 to conserve lumber.
    g->m.spawn_item( g->u.pos(), "2x4", numer_of_2x4s );
}

void construct::done_digormine_stair( const tripoint &p, bool dig )
{
    tripoint const abs_pos = p;
    tripoint const pos_sm = ms_to_sm_copy( abs_pos );
    tinymap tmpmap;
    tmpmap.load( pos_sm.x, pos_sm.y, pos_sm.z - 1, false );
    tripoint const local_tmp = tmpmap.getlocal( abs_pos );

    bool dig_muts = g->u.has_trait( "PAINRESIST_TROGLO" ) || g->u.has_trait( "STOCKY_TROGLO" );

    int no_mut_penalty = dig_muts ? 10 : 0;
    int mine_penalty = dig ? 0 : 10;
    g->u.mod_hunger( 5 + mine_penalty + no_mut_penalty );
    g->u.mod_thirst( 5 + mine_penalty + no_mut_penalty );
    g->u.mod_fatigue( 10 + mine_penalty + no_mut_penalty );

    if( tmpmap.ter( local_tmp ) == t_lava ) {
        if( !( query_yn( _( "The rock feels much warmer than normal. Proceed?" ) ) ) ) {
            g->m.ter_set( p, t_pit ); // You dug down a bit before detecting the problem
            unroll_digging( dig ? 8 : 12 );
        } else {
            add_msg( m_warning, _( "You just tunneled into lava!" ) );
            g->u.add_memorial_log( pgettext( "memorial_male", "Dug a shaft into lava." ),
                                   pgettext( "memorial_female", "Dug a shaft into lava." ) );
            g->m.ter_set( p, t_hole );
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
    g->m.ter_set( p, t_stairs_down ); // There's the top half
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
    tripoint const abs_pos = p;
    tripoint const pos_sm = ms_to_sm_copy( abs_pos );
    tinymap tmpmap;
    tmpmap.load( pos_sm.x, pos_sm.y, pos_sm.z + 1, false );
    const tripoint local_tmp = tmpmap.getlocal( abs_pos );

    if( tmpmap.ter( local_tmp ) == t_lava ) {
        g->m.ter_set( p.x, p.y, t_rock_floor ); // You dug a bit before discovering the problem
        add_msg(m_warning, _("The rock overhead feels hot.  You decide *not* to mine magma."));
        unroll_digging( 12 );
        return;
    }

    static const std::set<ter_id> liquids = {{
         t_water_sh, t_sewage,t_water_dp, t_water_pool
    }};

    if ( liquids.count( tmpmap.ter( local_tmp ) ) > 0 ) {
        g->m.ter_set( p.x, p.y, t_rock_floor ); // You dug a bit before discovering the problem
        add_msg(m_warning, _("The rock above is rather damp.  You decide *not* to mine water."));
        unroll_digging( 12 );
        return;
    }

    bool dig_muts = g->u.has_trait( "PAINRESIST_TROGLO" ) || g->u.has_trait( "STOCKY_TROGLO" );

    int no_mut_penalty = dig_muts ? 15 : 0;
    g->u.mod_hunger( 20 + no_mut_penalty );
    g->u.mod_thirst( 20 + no_mut_penalty );
    g->u.mod_fatigue( 25 + no_mut_penalty );

    add_msg( _("You drill out a passage, heading for the surface.") );
    g->m.ter_set( p.x, p.y, t_stairs_up ); // There's the bottom half
    // We need to write to submap-local coordinates.
    tmpmap.ter_set( local_tmp, t_stairs_down ); // and there's the top half.
    tmpmap.save();
}

void construct::done_window_curtains( const tripoint & )
{
    // copied from iexamine::curtains
    g->m.spawn_item( g->u.pos(), "nail", 1, 4 );
    g->m.spawn_item( g->u.pos(), "sheet", 2 );
    g->m.spawn_item( g->u.pos(), "stick" );
    g->m.spawn_item( g->u.pos(), "string_36" );
    g->u.add_msg_if_player( _("After boarding up the window the curtains and curtain rod are left.") );
}

void load_construction(JsonObject &jo)
{
    construction con;
    con.id = constructions.size();

    con.description = _(jo.get_string("description").c_str());
    con.skill = skill_id( jo.get_string( "skill", skill_carpentry.str() ) );
    con.difficulty = jo.get_int("difficulty");
    con.category = jo.get_string("category", "OTHER");
    // constructions use different time units in json, this makes it compatible
    // with recipes/requirements, TODO: should be changed in json
    con.time = jo.get_int("time") * 1000;

    if( jo.has_string( "using" ) ) {
        con.requirements = requirement_id( jo.get_string( "using" ) );
    } else {
        // Warning: the IDs may change!
        std::string req_id = string_format( "inline_construction_%i", con.id );
        requirement_data::load_requirement( jo, req_id );
        con.requirements = requirement_id( req_id );
    }

    con.pre_terrain = jo.get_string("pre_terrain", "");
    if (con.pre_terrain.size() > 1
        && con.pre_terrain[0] == 'f'
        && con.pre_terrain[1] == '_') {
        con.pre_is_furniture = true;
    } else {
        con.pre_is_furniture = false;
    }

    con.post_terrain = jo.get_string("post_terrain", "");
    if (con.post_terrain.size() > 1
        && con.post_terrain[0] == 'f'
        && con.post_terrain[1] == '_') {
        con.post_is_furniture = true;
    } else {
        con.post_is_furniture = false;
    }

    con.pre_flags = jo.get_tags("pre_flags");

    std::string prefunc = jo.get_string("pre_special", "");
    if (prefunc == "check_empty") {
        con.pre_special = &construct::check_empty;
    } else if (prefunc == "check_support") {
        con.pre_special = &construct::check_support;
    } else if (prefunc == "check_deconstruct") {
        con.pre_special = &construct::check_deconstruct;
    } else if (prefunc == "check_up_OK") {
        con.pre_special = &construct::check_up_OK;
    } else if (prefunc == "check_down_OK") {
        con.pre_special = &construct::check_down_OK;
    } else {
        if (prefunc != "") {
            debugmsg("Unknown pre_special function: %s", prefunc.c_str());
        }
        con.pre_special = &construct::check_nothing;
    }

    std::string postfunc = jo.get_string("post_special", "");
    if (postfunc == "done_tree") {
        con.post_special = &construct::done_tree;
    } else if (postfunc == "done_trunk_log") {
        con.post_special = &construct::done_trunk_log;
    } else if (postfunc == "done_trunk_plank") {
        con.post_special = &construct::done_trunk_plank;
    } else if (postfunc == "done_vehicle") {
        con.post_special = &construct::done_vehicle;
    } else if (postfunc == "done_deconstruct") {
        con.post_special = &construct::done_deconstruct;
    } else if (postfunc == "done_dig_stair") {
        con.post_special = &construct::done_dig_stair;
    } else if (postfunc == "done_mine_downstair") {
        con.post_special = &construct::done_mine_downstair;
    } else if (postfunc == "done_mine_upstair") {
        con.post_special = &construct::done_mine_upstair;
    } else if (postfunc == "done_window_curtains") {
        con.post_special = &construct::done_window_curtains;
    } else {
        if (postfunc != "") {
            debugmsg("Unknown post_special function: %s", postfunc.c_str());
        }
        con.post_special = &construct::done_nothing;
    }

    constructions.push_back(con);
}

void reset_constructions()
{
    constructions.clear();
}

void check_constructions()
{
    for( size_t i = 0; i < constructions.size(); i++ ) {
        const construction *c = &constructions[ i ];
        const std::string display_name = std::string("construction ") + c->description;
        // Note: print the description as the id is just a generated number,
        // the description can be searched for in the json files.
        if( !c->skill.is_valid() ) {
            debugmsg("Unknown skill %s in %s", c->skill.c_str(), display_name.c_str());
        }

        if( !c->requirements.is_valid() ) {
            debugmsg( "construction %s has missing requirement data %s",
                      display_name.c_str(), c->requirements.c_str() );
        }

        if( !c->pre_terrain.empty() ) {
            if( c->pre_is_furniture ) {
                if( !furn_str_id( c->pre_terrain ).is_valid() ) {
                    debugmsg("Unknown pre_terrain (furniture) %s in %s", c->pre_terrain.c_str(), display_name.c_str() );
                }
            } else if( !ter_str_id( c->pre_terrain ).is_valid() ) {
                debugmsg("Unknown pre_terrain (terrain) %s in %s", c->pre_terrain.c_str(), display_name.c_str());
            }
        }
        if( !c->post_terrain.empty() ) {
            if( c->post_is_furniture ) {
                if( !furn_str_id( c->post_terrain ).is_valid() ) {
                    debugmsg("Unknown post_terrain (furniture) %s in %s", c->post_terrain.c_str(), display_name.c_str());
                }
            } else if( !ter_str_id( c->post_terrain ).is_valid() ) {
                debugmsg("Unknown post_terrain (terrain) %s in %s", c->post_terrain.c_str(), display_name.c_str());
            }
        }
        if( c->id != i ) {
            debugmsg( "Construction \"%s\" has id %d, but should have %d",
                      c->description.c_str(), c->id, i );
        }
    }
}

int construction::print_time( WINDOW *w, int ypos, int xpos, int width,
                              nc_color col ) const
{
    std::string text = get_time_string();
    return fold_and_print( w, ypos, xpos, width, col, text );
}

float construction::time_scale() const
{
    //incorporate construction time scaling
    if( get_world_option<int>( "CONSTRUCTION_SCALING" ) == 0 ) {
        return calendar::season_ratio();
    } else {
        return 100.0 / get_world_option<int>( "CONSTRUCTION_SCALING" );
    }
}

int construction::adjusted_time() const
{
    int final_time = time;
    int assistants = 0;

    for( auto &elem : g->u.get_crafting_helpers() ) {
        if( elem->get_skill_level( skill ) >= difficulty ) {
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
    const int turns = adjusted_time() / 100;
    std::string text;
    if( turns < MINUTES( 1 ) ) {
        const int seconds = std::max( 1, turns * 6 );
        text = string_format( ngettext( "%d second", "%d seconds", seconds ), seconds );
    } else {
        const int minutes = ( turns % HOURS( 1 ) ) / MINUTES( 1 );
        const int hours = turns / HOURS( 1 );
        if( hours == 0 ) {
            text = string_format( ngettext( "%d minute", "%d minutes", minutes ), minutes );
        } else if( minutes == 0 ) {
            text = string_format( ngettext( "%d hour", "%d hours", hours ), hours );
        } else {
            const std::string h = string_format( ngettext( "%d hour", "%d hours", hours ), hours );
            const std::string m = string_format( ngettext( "%d minute", "%d minutes", minutes ), minutes );
            //~ A time duration: first is hours, second is minutes, e.g. "4 hours" "6 minutes"
            text = string_format( _( "%1$s and %2$s" ), h.c_str(), m.c_str() );
        }
    }
    text = string_format( _( "Time to complete: %s" ), text.c_str() );
    return text;
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
    for( auto &vp : vpart_info::get_all() ) {
        if( !vp->has_flag( "INITIAL_PART" ) ) {
            continue;
        }
        frame_items.push_back( item_comp( vp->item, 1 ) );
    }

    if( frame_items.empty() ) {
        debugmsg( "No valid frames detected for vehicle construction" );
    }

    for( construction &con : constructions ) {
        if( con.post_special == &construct::done_vehicle ) {
            const_cast<requirement_data &>( con.requirements.obj() ).get_components().push_back( frame_items );
        }
    }

    constructions.erase( std::remove_if( constructions.begin(), constructions.end(),
        [&]( const construction &c ) {
            return c.requirements->is_blacklisted();
    } ), constructions.end() );

    for( size_t i = 0; i < constructions.size(); i++ ) {
        constructions[ i ].id = i;
    }
}
