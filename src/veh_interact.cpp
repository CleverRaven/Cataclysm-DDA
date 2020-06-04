#include "veh_interact.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <numeric>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include "activity_handlers.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character_id.h"
#include "debug.h"
#include "enums.h"
#include "faction.h"
#include "fault.h"
#include "flat_set.h"
#include "game.h"
#include "game_constants.h"
#include "handle_liquid.h"
#include "item.h"
#include "item_contents.h"
#include "itype.h"
#include "map.h"
#include "map_selector.h"
#include "math_defines.h"
#include "memory_fast.h"
#include "messages.h"
#include "monster.h"
#include "npc.h"
#include "optional.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player.h"
#include "point.h"
#include "requirements.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_id.h"
#include "string_input_popup.h"
#include "tileray.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "veh_utils.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const itype_id fuel_type_battery( "battery" );

static const itype_id itype_battery( "battery" );
static const itype_id itype_plut_cell( "plut_cell" );

static const skill_id skill_mechanics( "mechanics" );

static const quality_id qual_JACK( "JACK" );
static const quality_id qual_LIFT( "LIFT" );
static const quality_id qual_HOSE( "HOSE" );
static const quality_id qual_SELF_JACK( "SELF_JACK" );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );

static const activity_id ACT_VEHICLE( "ACT_VEHICLE" );

static inline std::string status_color( bool status )
{
    return status ? "<color_green>" : "<color_red>";
}
static inline std::string health_color( bool status )
{
    return status ? "<color_light_green>" : "<color_light_red>";
}

// cap JACK requirements to support arbitrarily large vehicles
static double jack_quality( const vehicle &veh )
{
    const units::quantity<double, units::mass::unit_type> mass = std::min( veh.total_mass(),
            JACK_LIMIT );
    return std::ceil( mass / TOOL_LIFT_FACTOR );
}

/** Can part currently be reloaded with anything? */
static auto can_refill = []( const vehicle_part &pt )
{
    return pt.can_reload();
};

void act_vehicle_siphon( vehicle *veh );
void act_vehicle_unload_fuel( vehicle *veh );

player_activity veh_interact::serialize_activity()
{
    const auto *pt = sel_vehicle_part;
    const auto *vp = sel_vpart_info;

    if( sel_cmd == 'q' || sel_cmd == ' ' || !vp ) {
        return player_activity();
    }

    int time = 1000;
    switch( sel_cmd ) {
        case 'i':
            time = vp->install_time( g->u );
            break;
        case 'r':
            if( pt != nullptr ) {
                if( pt->is_broken() ) {
                    time = vp->install_time( g->u );
                } else if( pt->base.max_damage() > 0 ) {
                    time = vp->repair_time( g->u ) * pt->base.damage() / pt->base.max_damage();
                }
            }
            break;
        case 'o':
            time = vp->removal_time( g->u );
            break;
        default:
            break;
    }
    if( g->u.has_trait( trait_DEBUG_HS ) ) {
        time = 1;
    }
    player_activity res( ACT_VEHICLE, time, static_cast<int>( sel_cmd ) );

    // if we're working on an existing part, use that part as the reference point
    // otherwise (e.g. installing a new frame), just use part 0
    const point q = veh->coord_translate( pt ? pt->mount : veh->parts[0].mount );
    const vehicle_part *vpt = pt ? pt : &veh->parts[0];
    for( const tripoint &p : veh->get_points( true ) ) {
        res.coord_set.insert( g->m.getabs( p ) );
    }
    res.values.push_back( g->m.getabs( veh->global_pos3() ).x + q.x );    // values[0]
    res.values.push_back( g->m.getabs( veh->global_pos3() ).y + q.y );    // values[1]
    res.values.push_back( dd.x );   // values[2]
    res.values.push_back( dd.y );   // values[3]
    res.values.push_back( -dd.x );   // values[4]
    res.values.push_back( -dd.y );   // values[5]
    res.values.push_back( veh->index_of_part( vpt ) ); // values[6]
    res.str_values.push_back( vp->get_id().str() );
    res.targets.emplace_back( std::move( target ) );

    return res;
}

player_activity veh_interact::run( vehicle &veh, const point &p )
{
    veh_interact vehint( veh, p );
    vehint.do_main_loop();
    return vehint.serialize_activity();
}

vehicle_part &veh_interact::select_part( const vehicle &veh, const part_selector &sel,
        const std::string &title )
{
    static vehicle_part null_part;
    vehicle_part *res = &null_part;

    auto act = [&]( const vehicle_part & pt ) {
        res = const_cast<vehicle_part *>( &pt );
    };

    int opts = std::count_if( veh.parts.cbegin(), veh.parts.cend(), sel );

    if( opts == 1 ) {
        act( *std::find_if( veh.parts.cbegin(), veh.parts.cend(), sel ) );

    } else if( opts != 0 ) {
        veh_interact vehint( const_cast<vehicle &>( veh ) );
        vehint.title = title.empty() ? _( "Select part" ) : title;
        vehint.overview( sel, act );
    }

    return *res;
}

/**
 * Creates a blank veh_interact window.
 */
veh_interact::veh_interact( vehicle &veh, const point &p )
    : dd( p ), veh( &veh ), main_context( "VEH_INTERACT" )
{
    // Only build the shapes map and the wheel list once
    for( const auto &e : vpart_info::all() ) {
        const vpart_info &vp = e.second;
        vpart_shapes[ vp.name() + vp.item.str() ].push_back( &vp );
        if( vp.has_flag( "WHEEL" ) ) {
            wheel_types.push_back( &vp );
        }
    }

    main_context.register_directions();
    main_context.register_action( "QUIT" );
    main_context.register_action( "INSTALL" );
    main_context.register_action( "REPAIR" );
    main_context.register_action( "MEND" );
    main_context.register_action( "REFILL" );
    main_context.register_action( "REMOVE" );
    main_context.register_action( "RENAME" );
    main_context.register_action( "SIPHON" );
    main_context.register_action( "UNLOAD" );
    main_context.register_action( "ASSIGN_CREW" );
    main_context.register_action( "RELABEL" );
    main_context.register_action( "PREV_TAB" );
    main_context.register_action( "NEXT_TAB" );
    main_context.register_action( "OVERVIEW_DOWN" );
    main_context.register_action( "OVERVIEW_UP" );
    main_context.register_action( "FUEL_LIST_DOWN" );
    main_context.register_action( "FUEL_LIST_UP" );
    main_context.register_action( "DESC_LIST_DOWN" );
    main_context.register_action( "DESC_LIST_UP" );
    main_context.register_action( "CONFIRM" );
    main_context.register_action( "HELP_KEYBINDINGS" );
    main_context.register_action( "FILTER" );
    main_context.register_action( "ANY_INPUT" );

    count_durability();
    cache_tool_availability();
    // Initialize info of selected parts
    move_cursor( point_zero );
}

veh_interact::~veh_interact() = default;

void veh_interact::allocate_windows()
{
    // grid window
    const int grid_x = 1;
    const int grid_y = 1;
    const int grid_w = TERMX - 2; // exterior borders take 2
    const int grid_h = TERMY - 2; // exterior borders take 2

    const int mode_h  = 1;
    const int name_h  = 1;

    page_size = grid_h - ( mode_h + stats_h + name_h ) - 2;

    const int pane_y = grid_y + mode_h + 1;

    const int pane_w = ( grid_w / 3 ) - 1;

    const int disp_w = grid_w - ( pane_w * 2 ) - 2;
    const int disp_h = page_size * 0.45;
    const int parts_h = page_size - disp_h;
    const int parts_y = pane_y + disp_h;

    const int name_y = pane_y + page_size + 1;
    const int stats_y = name_y + name_h;

    const int list_x = grid_x + disp_w + 1;
    const int msg_x  = list_x + pane_w + 1;

    // covers right part of w_name and w_stats in vertical/hybrid
    const int details_y = name_y;
    const int details_x = list_x;

    const int details_h = 7;
    const int details_w = grid_x + grid_w - details_x;

    // make the windows
    w_border = catacurses::newwin( TERMY, TERMX, point_zero );
    w_mode  = catacurses::newwin( mode_h,    grid_w, point( grid_x, grid_y ) );
    w_msg   = catacurses::newwin( page_size, pane_w, point( msg_x, pane_y ) );
    w_disp  = catacurses::newwin( disp_h,    disp_w, point( grid_x, pane_y ) );
    w_parts = catacurses::newwin( parts_h,   disp_w, point( grid_x, parts_y ) );
    w_list  = catacurses::newwin( page_size, pane_w, point( list_x, pane_y ) );
    w_stats = catacurses::newwin( stats_h,   grid_w, point( grid_x, stats_y ) );
    w_name  = catacurses::newwin( name_h,    grid_w, point( grid_x, name_y ) );
    w_details = catacurses::newwin( details_h, details_w, point( details_x, details_y ) );
}

bool veh_interact::format_reqs( std::string &msg, const requirement_data &reqs,
                                const std::map<skill_id, int> &skills, int moves ) const
{
    const inventory &inv = g->u.crafting_inventory();
    bool ok = reqs.can_make_with_inventory( inv, is_crafting_component );

    msg += _( "<color_white>Time required:</color>\n" );
    // TODO: better have a from_moves function
    msg += "> " + to_string_approx( time_duration::from_turns( moves / 100 ) ) + "\n";

    msg += _( "<color_white>Skills required:</color>\n" );
    for( const auto &e : skills ) {
        bool hasSkill = g->u.get_skill_level( e.first ) >= e.second;
        if( !hasSkill ) {
            ok = false;
        }
        //~ %1$s represents the internal color name which shouldn't be translated, %2$s is skill name, and %3$i is skill level
        msg += string_format( _( "> %1$s%2$s %3$i</color>\n" ), status_color( hasSkill ),
                              e.first.obj().name(), e.second );
    }
    if( skills.empty() ) {
        //~ %1$s represents the internal color name which shouldn't be translated, %2$s is the word "NONE"
        msg += string_format( "> %1$s%2$s</color>", status_color( true ), _( "NONE" ) ) + "\n";
    }

    auto comps = reqs.get_folded_components_list( getmaxx( w_msg ) - 2, c_white, inv,
                 is_crafting_component );
    for( const std::string &line : comps ) {
        msg += line + "\n";
    }
    auto tools = reqs.get_folded_tools_list( getmaxx( w_msg ) - 2, c_white, inv );
    for( const std::string &line : tools ) {
        msg += line + "\n";
    }

    return ok;
}

struct veh_interact::install_info_t {
    int pos;
    size_t tab;
    std::vector<const vpart_info *> tab_vparts;
    std::array<std::string, 8> tab_list;
    std::array<std::string, 8> tab_list_short;
};

shared_ptr_fast<ui_adaptor> veh_interact::create_or_get_ui_adaptor()
{
    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( !current_ui ) {
        ui = current_ui = make_shared_fast<ui_adaptor>();
        current_ui->on_screen_resize( [this]( ui_adaptor & current_ui ) {
            allocate_windows();
            current_ui.position_from_window( catacurses::stdscr );
        } );
        current_ui->mark_resize();
        current_ui->on_redraw( [this]( const ui_adaptor & ) {
            display_grid();
            display_name();
            display_stats();
            display_veh();

            werase( w_parts );
            veh->print_part_list( w_parts, 0, getmaxy( w_parts ) - 1, getmaxx( w_parts ), cpart, highlight_part,
                                  true );
            wrefresh( w_parts );

            werase( w_msg );
            if( !msg.has_value() ) {
                veh->print_vparts_descs( w_msg, getmaxy( w_msg ), getmaxx( w_msg ), cpart, start_at, start_limit );
            } else {
                // NOLINTNEXTLINE(cata-use-named-point-constants)
                fold_and_print( w_msg, point( 1, 0 ), getmaxx( w_msg ) - 2, c_light_red, msg.value() );
            }
            wrefresh( w_msg );

            if( install_info ) {
                display_list( install_info->pos, install_info->tab_vparts, 2 );
                display_details( sel_vpart_info );
            } else {
                display_overview();
            }
            display_mode();
        } );
    }
    return current_ui;
}

void veh_interact::do_main_loop()
{
    bool finish = false;
    const bool owned_by_player = veh->handle_potential_theft( dynamic_cast<player &>( g->u ), true );
    faction *owner_fac;
    if( veh->has_owner() ) {
        owner_fac = g->faction_manager_ptr->get( veh->get_owner() );
    } else {
        owner_fac = g->faction_manager_ptr->get( faction_id( "no_faction" ) );
    }

    shared_ptr_fast<ui_adaptor> current_ui = create_or_get_ui_adaptor();

    while( !finish ) {
        calc_overview();
        ui_manager::redraw();
        const std::string action = main_context.handle_input();
        msg.reset();
        if( const cata::optional<tripoint> vec = main_context.get_direction( action ) ) {
            move_cursor( vec->xy() );
        } else if( action == "QUIT" ) {
            finish = true;
        } else if( action == "INSTALL" ) {
            if( veh->handle_potential_theft( dynamic_cast<player &>( g->u ) ) ) {
                do_install();
            }
        } else if( action == "REPAIR" ) {
            if( veh->handle_potential_theft( dynamic_cast<player &>( g->u ) ) ) {
                do_repair();
            }
        } else if( action == "MEND" ) {
            if( veh->handle_potential_theft( dynamic_cast<player &>( g->u ) ) ) {
                do_mend();
            }
        } else if( action == "REFILL" ) {
            if( veh->handle_potential_theft( dynamic_cast<player &>( g->u ) ) ) {
                do_refill();
            }
        } else if( action == "REMOVE" ) {
            if( veh->handle_potential_theft( dynamic_cast<player &>( g->u ) ) ) {
                do_remove();
            }
        } else if( action == "RENAME" ) {
            if( owned_by_player ) {
                do_rename();
            } else {
                if( owner_fac ) {
                    popup( _( "You cannot rename this vehicle as it is owned by: %s." ), _( owner_fac->name ) );
                }
            }
        } else if( action == "SIPHON" ) {
            if( veh->handle_potential_theft( dynamic_cast<player &>( g->u ) ) ) {
                do_siphon();
                // Siphoning may have started a player activity. If so, we should close the
                // vehicle dialog and continue with the activity.
                finish = !g->u.activity.is_null();
                if( !finish ) {
                    // it's possible we just invalidated our crafting inventory
                    cache_tool_availability();
                }
            }
        } else if( action == "UNLOAD" ) {
            if( veh->handle_potential_theft( dynamic_cast<player &>( g->u ) ) ) {
                finish = do_unload();
            }
        } else if( action == "ASSIGN_CREW" ) {
            if( owned_by_player ) {
                do_assign_crew();
            } else {
                if( owner_fac ) {
                    popup( _( "You cannot assign crew on this vehicle as it is owned by: %s." ),
                           _( owner_fac->name ) );
                }
            }
        } else if( action == "RELABEL" ) {
            if( owned_by_player ) {
                do_relabel();
            } else {
                if( owner_fac ) {
                    popup( _( "You cannot relabel this vehicle as it is owned by: %s." ), _( owner_fac->name ) );
                }
            }
        } else if( action == "FUEL_LIST_DOWN" ) {
            move_fuel_cursor( 1 );
        } else if( action == "FUEL_LIST_UP" ) {
            move_fuel_cursor( -1 );
        } else if( action == "OVERVIEW_DOWN" ) {
            move_overview_line( 1 );
        } else if( action == "OVERVIEW_UP" ) {
            move_overview_line( -1 );
        } else if( action == "DESC_LIST_DOWN" ) {
            move_cursor( point_zero, 1 );
        } else if( action == "DESC_LIST_UP" ) {
            move_cursor( point_zero, -1 );
        }
        if( sel_cmd != ' ' ) {
            finish = true;
        }
    }
}

void veh_interact::cache_tool_availability()
{
    crafting_inv = g->u.crafting_inventory();

    cache_tool_availability_update_lifting( g->u.pos() );
    int mech_jack = 0;
    if( g->u.is_mounted() ) {
        mech_jack = g->u.mounted_creature->mech_str_addition() + 10;
    }
    max_jack = std::max( { g->u.max_quality( qual_JACK ), mech_jack,
                           map_selector( g->u.pos(), PICKUP_RANGE ).max_quality( qual_JACK ),
                           vehicle_selector( g->u.pos(), 2, true, *veh ).max_quality( qual_JACK )
                         } );
}

void veh_interact::cache_tool_availability_update_lifting( const tripoint &world_cursor_pos )
{
    max_lift = g->u.best_nearby_lifting_assist( world_cursor_pos );
}

/**
 * Checks if the player is able to perform some command, and returns a nonzero
 * error code if they are unable to perform it. The return from this function
 * should be passed into the various do_whatever functions further down.
 * @param mode The command the player is trying to perform (i.e. 'r' for repair).
 * @return CAN_DO if the player has everything they need,
 *         INVALID_TARGET if the command can't target that square,
 *         LACK_TOOLS if the player lacks tools,
 *         NOT_FREE if something else obstructs the action,
 *         LACK_SKILL if the player's skill isn't high enough,
 *         LOW_MORALE if the player's morale is too low while trying to perform
 *             an action requiring a minimum morale,
 *         UNKNOWN_TASK if the requested operation is unrecognized.
 */
task_reason veh_interact::cant_do( char mode )
{
    bool enough_morale = true;
    bool valid_target = false;
    bool has_tools = false;
    bool part_free = true;
    bool has_skill = true;
    bool enough_light = true;

    switch( mode ) {
        case 'i':
            // install mode
            enough_morale = g->u.has_morale_to_craft();
            valid_target = !can_mount.empty() && 0 == veh->tags.count( "convertible" );
            //tool checks processed later
            enough_light = g->u.fine_detail_vision_mod() <= 4;
            has_tools = true;
            break;

        case 'r':
            // repair mode
            enough_morale = g->u.has_morale_to_craft();
            valid_target = !need_repair.empty() && cpart >= 0;
            // checked later
            has_tools = true;
            enough_light = g->u.fine_detail_vision_mod() <= 4;
            break;

        case 'm': {
            // mend mode
            enough_morale = g->u.has_morale_to_craft();
            const bool toggling = g->u.has_trait( trait_DEBUG_HS );
            valid_target = std::any_of( veh->parts.begin(),
            veh->parts.end(), [toggling]( const vehicle_part & pt ) {
                if( toggling ) {
                    return pt.is_available() && !pt.faults_potential().empty();
                } else {
                    return pt.is_available() && !pt.faults().empty();
                }
            } );
            enough_light = g->u.fine_detail_vision_mod() <= 4;
            // checked later
            has_tools = true;
        }
        break;

        case 'f':
            valid_target = std::any_of( veh->parts.begin(), veh->parts.end(), can_refill );
            has_tools = true;
            break;

        case 'o':
            // remove mode
            enough_morale = g->u.has_morale_to_craft();
            valid_target = cpart >= 0 && 0 == veh->tags.count( "convertible" );
            part_free = parts_here.size() > 1 || ( cpart >= 0 && veh->can_unmount( cpart ) );
            //tool and skill checks processed later
            has_tools = true;
            has_skill = true;
            enough_light = g->u.fine_detail_vision_mod() <= 4;
            break;

        case 's':
            // siphon mode
            valid_target = false;
            for( const vpart_reference &vp : veh->get_any_parts( VPFLAG_FLUIDTANK ) ) {
                if( vp.part().base.has_item_with( []( const item & it ) {
                return it.made_of( phase_id::LIQUID );
                } ) ) {
                    valid_target = true;
                    break;
                }
            }
            has_tools = g->u.has_quality( qual_HOSE );
            break;

        case 'd':
            // unload mode
            valid_target = false;
            has_tools = true;
            for( auto &e : veh->fuels_left() ) {
                if( e.first != fuel_type_battery && item::find_type( e.first )->phase == phase_id::SOLID ) {
                    valid_target = true;
                    break;
                }
            }
            break;

        case 'w':
            // assign crew
            if( g->allies().empty() ) {
                return INVALID_TARGET;
            }
            return std::any_of( veh->parts.begin(), veh->parts.end(), []( const vehicle_part & e ) {
                return e.is_seat();
            } ) ? CAN_DO : INVALID_TARGET;

        case 'a':
            // relabel
            valid_target = cpart >= 0;
            has_tools = true;
            break;
        default:
            return UNKNOWN_TASK;
    }

    if( std::abs( veh->velocity ) > 100 || g->u.controlling_vehicle ) {
        return MOVING_VEHICLE;
    }
    if( !valid_target ) {
        return INVALID_TARGET;
    }
    if( !enough_morale ) {
        return LOW_MORALE;
    }
    if( !enough_light ) {
        return LOW_LIGHT;
    }
    if( !has_tools ) {
        return LACK_TOOLS;
    }
    if( !part_free ) {
        return NOT_FREE;
    }
    // TODO: that is always false!
    if( !has_skill ) {
        return LACK_SKILL;
    }
    return CAN_DO;
}

bool veh_interact::is_drive_conflict()
{
    std::string conflict_type;
    bool has_conflict = veh->has_engine_conflict( sel_vpart_info, conflict_type );

    if( has_conflict ) {
        //~ %1$s is fuel_type
        msg = string_format( _( "Only one %1$s powered engine can be installed." ),
                             conflict_type );
    }
    return has_conflict;
}

bool veh_interact::can_self_jack()
{
    int lvl = jack_quality( *veh );

    for( const vpart_reference &vp : veh->get_avail_parts( "SELF_JACK" ) ) {
        if( vp.part().base.has_quality( qual_SELF_JACK, lvl ) ) {
            return true;
        }
    }
    return false;
}

bool veh_interact::can_install_part()
{
    if( sel_vpart_info == nullptr ) {
        return false;
    }

    if( is_drive_conflict() ) {
        return false;
    }
    if( veh->has_part( "NO_MODIFY_VEHICLE" ) && !sel_vpart_info->has_flag( "SIMPLE_PART" ) ) {
        msg = _( "This vehicle cannot be modified in this way.\n" );
        return false;
    } else if( sel_vpart_info->has_flag( "NO_INSTALL_PLAYER" ) ) {
        msg = _( "This part cannot be installed.\n" );
        return false;
    }

    if( sel_vpart_info->has_flag( "FUNNEL" ) ) {
        if( std::none_of( parts_here.begin(), parts_here.end(), [&]( const int e ) {
        return veh->parts[e].is_tank();
        } ) ) {
            msg = _( "Funnels need to be installed over a tank." );
            return false;
        }
    }

    if( sel_vpart_info->has_flag( "TURRET" ) ) {
        if( std::any_of( parts_here.begin(), parts_here.end(), [&]( const int e ) {
        return veh->parts[e].is_turret();
        } ) ) {
            msg = _( "Can't install turret on another turret." );
            return false;
        }
    }

    bool is_engine = sel_vpart_info->has_flag( "ENGINE" );
    //count current engines, some engines don't require higher skill
    int engines = 0;
    int dif_eng = 0;
    if( is_engine && sel_vpart_info->has_flag( "E_HIGHER_SKILL" ) ) {
        for( const vpart_reference &vp : veh->get_avail_parts( "ENGINE" ) ) {
            if( vp.has_feature( "E_HIGHER_SKILL" ) ) {
                engines++;
                dif_eng = dif_eng / 2 + 8;
            }
        }
    }

    int dif_steering = 0;
    if( sel_vpart_info->has_flag( "STEERABLE" ) ) {
        std::set<int> axles;
        for( auto &p : veh->steering ) {
            if( !veh->part_flag( p, "TRACKED" ) ) {
                // tracked parts don't contribute to axle complexity
                axles.insert( veh->parts[p].mount.x );
            }
        }

        if( !axles.empty() && axles.count( -dd.x ) == 0 ) {
            // Installing more than one steerable axle is hard
            // (but adding a wheel to an existing axle isn't)
            dif_steering = axles.size() + 5;
        }
    }

    const auto reqs = sel_vpart_info->install_requirements();

    std::string nmsg;
    bool ok = format_reqs( nmsg, reqs, sel_vpart_info->install_skills,
                           sel_vpart_info->install_time( g->u ) );

    nmsg += _( "<color_white>Additional requirements:</color>\n" );

    if( dif_eng > 0 ) {
        if( g->u.get_skill_level( skill_mechanics ) < dif_eng ) {
            ok = false;
        }
        //~ %1$s represents the internal color name which shouldn't be translated, %2$s is skill name, and %3$i is skill level
        nmsg += string_format( _( "> %1$s%2$s %3$i</color> for extra engines." ),
                               status_color( g->u.get_skill_level( skill_mechanics ) >= dif_eng ),
                               skill_mechanics.obj().name(), dif_eng ) + "\n";
    }

    if( dif_steering > 0 ) {
        if( g->u.get_skill_level( skill_mechanics ) < dif_steering ) {
            ok = false;
        }
        //~ %1$s represents the internal color name which shouldn't be translated, %2$s is skill name, and %3$i is skill level
        nmsg += string_format( _( "> %1$s%2$s %3$i</color> for extra steering axles." ),
                               status_color( g->u.get_skill_level( skill_mechanics ) >= dif_steering ),
                               skill_mechanics.obj().name(), dif_steering ) + "\n";
    }

    int lvl = 0;
    int str = 0;
    quality_id qual;
    bool use_aid = false;
    bool use_str = false;
    if( sel_vpart_info->has_flag( "NEEDS_JACKING" ) ) {
        qual = qual_JACK;
        lvl = jack_quality( *veh );
        str = veh->lift_strength();
        use_aid = ( max_jack >= lvl ) || can_self_jack();
        use_str = g->u.can_lift( *veh );
    } else {
        item base( sel_vpart_info->item );
        qual = qual_LIFT;
        lvl = std::ceil( units::quantity<double, units::mass::unit_type>( base.weight() ) /
                         TOOL_LIFT_FACTOR );
        str = base.lift_strength();
        use_aid = max_lift >= lvl;
        use_str = g->u.can_lift( base );
    }

    if( !( use_aid || use_str ) ) {
        ok = false;
    }

    nc_color aid_color = use_aid ? c_green : ( use_str ? c_dark_gray : c_red );
    nc_color str_color = use_str ? c_green : ( use_aid ? c_dark_gray : c_red );

    const auto helpers = g->u.get_crafting_helpers();
    std::string str_string;
    if( !helpers.empty() ) {
        str_string = string_format( _( "strength ( assisted ) %d" ), str );
    } else {
        str_string = string_format( _( "strength %d" ), str );
    }
    //~ %1$s is quality name, %2$d is quality level
    std::string aid_string = string_format( _( "1 tool with %1$s %2$d" ),
                                            qual.obj().name, lvl );
    nmsg += string_format( _( "> %1$s <color_white>OR</color> %2$s" ),
                           colorize( aid_string, aid_color ),
                           colorize( str_string, str_color ) ) + "\n";

    sel_vpart_info->format_description( nmsg, c_light_gray, getmaxx( w_msg ) - 4 );

    msg = colorize( nmsg, c_light_gray );
    return ok || g->u.has_trait( trait_DEBUG_HS );
}

/**
 * Moves list of fuels up or down.
 * @param delta -1 if moving up,
 *              1 if moving down
 */
void veh_interact::move_fuel_cursor( int delta )
{
    int max_fuel_indicators = static_cast<int>( veh->get_printable_fuel_types().size() );
    int height = 5;
    fuel_index += delta;

    if( fuel_index < 0 ) {
        fuel_index = 0;
    } else if( fuel_index > max_fuel_indicators - height ) {
        fuel_index = std::max( max_fuel_indicators - height, 0 );
    }
}

static void sort_uilist_entries_by_line_drawing( std::vector<uilist_entry> &shape_ui_entries )
{
    // An ordering of the line drawing symbols that does not result in
    // connecting when placed adjacent to each other vertically.
    const static std::map<int, int> symbol_order = {
        { LINE_XOXO, 0 }, { LINE_OXOX, 1 },
        { LINE_XOOX, 2 }, { LINE_XXOO, 3 },
        { LINE_XXXX, 4 }, { LINE_OXXO, 5 },
        { LINE_OOXX, 6 }
    };

    std::sort( shape_ui_entries.begin(), shape_ui_entries.end(),
    []( const uilist_entry & a, const uilist_entry & b ) {
        auto a_iter = symbol_order.find( a.extratxt.sym );
        auto b_iter = symbol_order.find( b.extratxt.sym );
        if( a_iter != symbol_order.end() ) {
            if( b_iter != symbol_order.end() ) {
                return a_iter->second < b_iter->second;
            } else {
                return true;
            }
        } else if( b_iter != symbol_order.end() ) {
            return false;
        } else {
            return a.extratxt.sym < b.extratxt.sym;
        }
    } );
}

void veh_interact::do_install()
{
    task_reason reason = cant_do( 'i' );

    if( reason == INVALID_TARGET ) {
        msg = _( "Cannot install any part here." );
        return;
    }

    restore_on_out_of_scope<cata::optional<std::string>> prev_title( title );
    title = _( "Choose new part to install here:" );

    restore_on_out_of_scope<std::unique_ptr<install_info_t>> prev_install_info( std::move(
                install_info ) );
    install_info = std::make_unique<install_info_t>();

    std::array<std::string, 8> &tab_list = install_info->tab_list = { {
            pgettext( "Vehicle Parts|", "All" ),
            pgettext( "Vehicle Parts|", "Cargo" ),
            pgettext( "Vehicle Parts|", "Light" ),
            pgettext( "Vehicle Parts|", "Util" ),
            pgettext( "Vehicle Parts|", "Hull" ),
            pgettext( "Vehicle Parts|", "Internal" ),
            pgettext( "Vehicle Parts|", "Other" ),
            pgettext( "Vehicle Parts|", "Filter" )
        }
    };

    install_info->tab_list_short = { {
            pgettext( "Vehicle Parts|", "A" ),
            pgettext( "Vehicle Parts|", "C" ),
            pgettext( "Vehicle Parts|", "L" ),
            pgettext( "Vehicle Parts|", "U" ),
            pgettext( "Vehicle Parts|", "H" ),
            pgettext( "Vehicle Parts|", "I" ),
            pgettext( "Vehicle Parts|", "O" ),
            pgettext( "Vehicle Parts|", "F" )
        }
    };

    std::array <std::function<bool( const vpart_info * )>, 8>
    tab_filters; // filter for each tab, last one
    tab_filters[0] = [&]( const vpart_info * ) {
        return true;
    }; // All
    tab_filters[1] = [&]( const vpart_info * p ) {
        auto &part = *p;
        return part.has_flag( VPFLAG_CARGO ) && // Cargo
               !part.has_flag( "TURRET" );
    };
    tab_filters[2] = [&]( const vpart_info * p ) {
        auto &part = *p;
        return part.has_flag( VPFLAG_LIGHT ) || // Light
               part.has_flag( VPFLAG_CONE_LIGHT ) ||
               part.has_flag( VPFLAG_WIDE_CONE_LIGHT ) ||
               part.has_flag( VPFLAG_CIRCLE_LIGHT ) ||
               part.has_flag( VPFLAG_DOME_LIGHT ) ||
               part.has_flag( VPFLAG_AISLE_LIGHT ) ||
               part.has_flag( VPFLAG_ATOMIC_LIGHT );
    };
    tab_filters[3] = [&]( const vpart_info * p ) {
        auto &part = *p;
        return part.has_flag( "TRACK" ) || //Util
               part.has_flag( VPFLAG_FRIDGE ) ||
               part.has_flag( VPFLAG_FREEZER ) ||
               part.has_flag( "KITCHEN" ) ||
               part.has_flag( "WELDRIG" ) ||
               part.has_flag( "CRAFTRIG" ) ||
               part.has_flag( "CHEMLAB" ) ||
               part.has_flag( "FORGE" ) ||
               part.has_flag( "HORN" ) ||
               part.has_flag( "BEEPER" ) ||
               part.has_flag( "AUTOPILOT" ) ||
               part.has_flag( "WATCH" ) ||
               part.has_flag( "ALARMCLOCK" ) ||
               part.has_flag( VPFLAG_RECHARGE ) ||
               part.has_flag( "VISION" ) ||
               part.has_flag( "POWER_TRANSFER" ) ||
               part.has_flag( "FAUCET" ) ||
               part.has_flag( "STEREO" ) ||
               part.has_flag( "CHIMES" ) ||
               part.has_flag( "MUFFLER" ) ||
               part.has_flag( "REMOTE_CONTROLS" ) ||
               part.has_flag( "CURTAIN" ) ||
               part.has_flag( "SEATBELT" ) ||
               part.has_flag( "SECURITY" ) ||
               part.has_flag( "SEAT" ) ||
               part.has_flag( "BED" ) ||
               part.has_flag( "SPACE_HEATER" ) ||
               part.has_flag( "COOLER" ) ||
               part.has_flag( "DOOR_MOTOR" ) ||
               part.has_flag( "WATER_PURIFIER" ) ||
               part.has_flag( "WORKBENCH" );
    };
    tab_filters[4] = [&]( const vpart_info * p ) {
        auto &part = *p;
        return( part.has_flag( VPFLAG_OBSTACLE ) || // Hull
                part.has_flag( "ROOF" ) ||
                part.has_flag( VPFLAG_ARMOR ) ) &&
              !part.has_flag( "WHEEL" ) &&
              !tab_filters[3]( p );
    };
    tab_filters[5] = [&]( const vpart_info * p ) {
        auto &part = *p;
        return part.has_flag( VPFLAG_ENGINE ) || // Internals
               part.has_flag( VPFLAG_ALTERNATOR ) ||
               part.has_flag( VPFLAG_CONTROLS ) ||
               part.location == "fuel_source" ||
               part.location == "on_battery_mount" ||
               ( part.location.empty() && part.has_flag( "FUEL_TANK" ) );
    };

    // Other: everything that's not in the other filters
    tab_filters[tab_filters.size() - 2] = [&]( const vpart_info * part ) {
        for( size_t i = 1; i < tab_filters.size() - 2; i++ ) {
            if( tab_filters[i]( part ) ) {
                return false;
            }
        }
        return true;
    };

    std::string filter; // The user specified filter
    tab_filters[7] = [&]( const vpart_info * p ) {
        return lcmatch( p->name(), filter );
    };

    // full list of mountable parts, to be filtered according to tab
    std::vector<const vpart_info *> &tab_vparts = install_info->tab_vparts = can_mount;

    shared_ptr_fast<ui_adaptor> current_ui = create_or_get_ui_adaptor();

    int &pos = install_info->pos = 0;
    size_t &tab = install_info->tab = 0;

    while( true ) {
        sel_vpart_info = tab_vparts.empty() ? nullptr : tab_vparts[pos]; // filtered list can be empty

        bool can_install = can_install_part();

        ui_manager::redraw();

        const std::string action = main_context.handle_input();
        msg.reset();
        if( action == "FILTER" ) {
            string_input_popup()
            .title( _( "Search for part" ) )
            .width( 50 )
            .description( _( "Filter" ) )
            .max_length( 100 )
            .edit( filter );
            tab = 7; // Move to the user filter tab.
        }
        if( action == "REPAIR" ) {
            filter.clear();
            tab = 0;
        }
        if( action == "INSTALL" || action == "CONFIRM" ) {
            if( can_install ) {
                switch( reason ) {
                    case LOW_MORALE:
                        msg = _( "Your morale is too low to construct…" );
                        return;
                    case LOW_LIGHT:
                        msg = _( "It's too dark to see what you are doing…" );
                        return;
                    case MOVING_VEHICLE:
                        msg = _( "You can't install parts while driving." );
                        return;
                    default:
                        break;
                }
                // Modifying a vehicle with rotors will make in not flightworthy (until we've got a better model)
                // It can only be the player doing this - an npc won't work well with query_yn
                if( veh->would_prevent_flyable( *sel_vpart_info ) ) {
                    if( query_yn(
                            _( "Installing this part will mean that this vehicle is no longer flightworthy.  Continue?" ) ) ) {
                        veh->set_flyable( false );
                    } else {
                        return;
                    }
                }
                if( veh->is_foldable() && !sel_vpart_info->has_flag( "FOLDABLE" ) &&
                    !query_yn( _( "Installing this part will make the vehicle unfoldable.  Continue?" ) ) ) {
                    return;
                }
                const auto &shapes =
                    vpart_shapes[ sel_vpart_info->name() + sel_vpart_info->item.str() ];
                int selected_shape = -1;
                if( shapes.size() > 1 ) {  // more than one shape available, display selection
                    std::vector<uilist_entry> shape_ui_entries;
                    for( size_t i = 0; i < shapes.size(); i++ ) {
                        uilist_entry entry( i, true, 0, shapes[i]->name() );
                        entry.extratxt.left = 1;
                        entry.extratxt.sym = special_symbol( shapes[i]->sym );
                        entry.extratxt.color = shapes[i]->color;
                        shape_ui_entries.push_back( entry );
                    }
                    sort_uilist_entries_by_line_drawing( shape_ui_entries );
                    uilist smenu;
                    smenu.settext( _( "Choose shape:" ) );
                    smenu.entries = shape_ui_entries;
                    smenu.w_width_setup = [this]() {
                        return getmaxx( w_list );
                    };
                    smenu.w_x_setup = [this]( const int ) {
                        return getbegx( w_list );
                    };
                    smenu.w_y_setup = [this]( const int ) {
                        return getbegy( w_list );
                    };
                    smenu.query();
                    selected_shape = smenu.ret;
                } else { // only one shape available, default to first one
                    selected_shape = 0;
                }
                if( selected_shape >= 0 && static_cast<size_t>( selected_shape ) < shapes.size() ) {
                    sel_vpart_info = shapes[selected_shape];
                    sel_cmd = 'i';
                    return;
                }
            }
        } else if( action == "QUIT" ) {
            sel_vpart_info = nullptr;
            break;
        } else if( action == "PREV_TAB" || action == "NEXT_TAB" || action == "FILTER" ||
                   action == "REPAIR" ) {
            tab_vparts.clear();
            pos = 0;

            if( action == "PREV_TAB" ) {
                tab = ( tab < 1 ) ? tab_list.size() - 1 : tab - 1;
            } else if( action == "NEXT_TAB" ) {
                tab = ( tab < tab_list.size() - 1 ) ? tab + 1 : 0;
            }

            copy_if( can_mount.begin(), can_mount.end(), back_inserter( tab_vparts ), tab_filters[tab] );
        } else {
            move_in_list( pos, action, tab_vparts.size(), 2 );
        }
    }
}

bool veh_interact::move_in_list( int &pos, const std::string &action, const int size,
                                 const int header ) const
{
    int lines_per_page = page_size - header;
    if( action == "PREV_TAB" || action == "LEFT" ) {
        pos -= lines_per_page;
    } else if( action == "NEXT_TAB" || action == "RIGHT" ) {
        pos += lines_per_page;
    } else if( action == "UP" ) {
        pos--;
    } else if( action == "DOWN" ) {
        pos++;
    } else {
        // Anything else -> no movement
        return false;
    }
    if( pos < 0 ) {
        pos = size - 1;
    } else if( pos >= size ) {
        pos = 0;
    }
    return true;
}

void veh_interact::do_repair()
{
    task_reason reason = cant_do( 'r' );

    if( reason == INVALID_TARGET ) {
        vehicle_part *most_repairable = get_most_repariable_part();
        if( most_repairable ) {
            move_cursor( ( most_repairable->mount + dd ).rotate( 3 ) );
            return;
        }
    }

    auto can_repair = [this, &reason]() {
        switch( reason ) {
            case LOW_MORALE:
                msg = _( "Your morale is too low to repair…" );
                return false;
            case LOW_LIGHT:
                msg = _( "It's too dark to see what you are doing…" );
                return false;
            case MOVING_VEHICLE:
                msg = _( "You can't repair stuff while driving." );
                return false;
            case INVALID_TARGET:
                msg = _( "There are no damaged parts on this vehicle." );
                return false;
            default:
                break;
        }
        return true;
    };

    if( !can_repair() ) {
        return;
    }

    restore_on_out_of_scope<cata::optional<std::string>> prev_title( title );
    title = _( "Choose a part here to repair:" );

    shared_ptr_fast<ui_adaptor> current_ui = create_or_get_ui_adaptor();

    int pos = 0;

    restore_on_out_of_scope<int> prev_hilight_part( highlight_part );

    while( true ) {
        vehicle_part &pt = veh->parts[parts_here[need_repair[pos]]];
        const vpart_info &vp = pt.info();

        std::string nmsg;

        // this will always be set, but the gcc thinks that sometimes it won't be
        bool ok = true;
        if( pt.is_broken() ) {
            ok = format_reqs( nmsg, vp.install_requirements(), vp.install_skills, vp.install_time( g->u ) );
        } else {
            if( vp.has_flag( "NO_REPAIR" ) || vp.repair_requirements().is_empty() ||
                pt.base.max_damage() <= 0 ) {
                nmsg += colorize( _( "This part cannot be repaired.\n" ), c_light_red );
                ok = false;
            } else if( veh->has_part( "NO_MODIFY_VEHICLE" ) && !vp.has_flag( "SIMPLE_PART" ) ) {
                nmsg += colorize( _( "This vehicle cannot be repaired.\n" ), c_light_red );
                ok = false;
            } else if( veh->would_prevent_flyable( vp ) ) {
                // Modifying a vehicle with rotors will make in not flightworthy (until we've got a better model)
                // It can only be the player doing this - an npc won't work well with query_yn
                if( query_yn(
                        _( "Repairing this part will mean that this vehicle is no longer flightworthy.  Continue?" ) ) ) {
                    veh->set_flyable( false );
                } else {
                    nmsg += colorize( _( "You chose not to install this part to keep the vehicle flyable.\n" ),
                                      c_light_red );
                    ok = false;
                }
            } else {
                ok = format_reqs( nmsg, vp.repair_requirements() * pt.base.damage_level( 4 ), vp.repair_skills,
                                  vp.repair_time( g->u ) * pt.base.damage() / pt.base.max_damage() );
            }
        }

        const nc_color desc_color = pt.is_broken() ? c_dark_gray : c_light_gray;
        vp.format_description( nmsg, desc_color, getmaxx( w_msg ) - 4 );

        msg = colorize( nmsg, c_light_gray );

        highlight_part = need_repair[pos];

        ui_manager::redraw();

        const std::string action = main_context.handle_input();
        msg.reset();
        if( ( action == "REPAIR" || action == "CONFIRM" ) && ok ) {
            reason = cant_do( 'r' );
            if( !can_repair() ) {
                return;
            }
            sel_vehicle_part = &pt;
            sel_vpart_info = &vp;
            const std::vector<npc *> helpers = g->u.get_crafting_helpers();
            for( const npc *np : helpers ) {
                add_msg( m_info, _( "%s helps with this task…" ), np->name );
            }
            sel_cmd = 'r';
            break;

        } else if( action == "QUIT" ) {
            break;

        } else {
            move_in_list( pos, action, need_repair.size() );
        }
    }
}

void veh_interact::do_mend()
{
    switch( cant_do( 'm' ) ) {
        case LOW_MORALE:
            msg = _( "Your morale is too low to mend…" );
            return;
        case LOW_LIGHT:
            msg = _( "It's too dark to see what you are doing…" );
            return;
        case INVALID_TARGET:
            msg = _( "No faulty parts require mending." );
            return;
        case MOVING_VEHICLE:
            msg = _( "You can't mend stuff while driving." );
            return;
        default:
            break;
    }

    restore_on_out_of_scope<cata::optional<std::string>> prev_title( title );
    title = _( "Choose a part here to mend:" );

    const bool toggling = g->u.has_trait( trait_DEBUG_HS );
    auto sel = [toggling]( const vehicle_part & pt ) {
        if( toggling ) {
            return !pt.faults_potential().empty();
        } else {
            return !pt.faults().empty();
        }
    };

    auto act = [&]( const vehicle_part & pt ) {
        g->u.mend_item( veh->part_base( veh->index_of_part( &pt ) ) );
        sel_cmd = 'q';
    };

    overview( sel, act );
}

void veh_interact::do_refill()
{
    switch( cant_do( 'f' ) ) {
        case MOVING_VEHICLE:
            msg = _( "You can't refill a moving vehicle." );
            return;

        case INVALID_TARGET:
            msg = _( "No parts can currently be refilled." );
            return;

        default:
            break;
    }

    restore_on_out_of_scope<cata::optional<std::string>> prev_title( title );
    title = _( "Select part to refill:" );

    auto act = [&]( const vehicle_part & pt ) {
        auto validate = [&]( const item & obj ) {
            if( pt.is_tank() ) {
                if( obj.is_container() && !obj.contents.empty() ) {
                    // we are assuming only one pocket here, and it's a liquid so only one item
                    return pt.can_reload( obj.contents.only_item() );
                }
            } else if( pt.is_fuel_store() ) {
                bool can_reload = pt.can_reload( obj );
                if( obj.typeId() == fuel_type_battery && can_reload ) {
                    msg = _( "You cannot recharge a vehicle battery with handheld batteries" );
                    return false;
                }
                return can_reload;
            }
            return false;
        };

        target = g->inv_map_splice( validate, string_format( _( "Refill %s" ), pt.name() ), 1 );
        if( target ) {
            sel_vehicle_part = &pt;
            sel_vpart_info = &pt.info();
            sel_cmd = 'f';
        }
    };

    overview( can_refill, act );
}

void veh_interact::calc_overview()
{
    const auto next_hotkey = [&]( const vehicle_part & pt, char &hotkey ) {
        if( overview_action && overview_enable && overview_enable( pt ) ) {
            const char ret = hotkey;
            // Calculate next hotkey
            ++hotkey;
            bool finish = false;
            while( !finish ) {
                switch( hotkey ) {
                    default:
                        finish = true;
                        break;
                    case '{':
                        hotkey = 'A';
                        break;
                    case 'c':
                    case 'g':
                    case 'j':
                    case 'k':
                    case 'l':
                    case 'p':
                    case 'q':
                    case 't':
                    case 'v':
                    case 'x':
                    case 'z':
                        ++hotkey;
                        break;
                }
            }
            return ret;
        } else {
            return '\0';
        }
    };

    overview_opts.clear();
    overview_headers.clear();

    int epower_w = veh->net_battery_charge_rate_w();
    overview_headers["ENGINE"] = [this]( const catacurses::window & w, int y ) {
        trim_and_print( w, point( 1, y ), getmaxx( w ) - 2, c_light_gray,
                        string_format( _( "Engines: %sSafe %4d kW</color> %sMax %4d kW</color>" ),
                                       health_color( true ), veh->total_power_w( true, true ) / 1000,
                                       health_color( false ), veh->total_power_w() / 1000 ) );
        right_print( w, y, 1, c_light_gray, _( "Fuel     Use" ) );
    };
    overview_headers["TANK"] = []( const catacurses::window & w, int y ) {
        trim_and_print( w, point( 1, y ), getmaxx( w ) - 2, c_light_gray, _( "Tanks" ) );
        right_print( w, y, 1, c_light_gray, _( "Contents     Qty" ) );
    };
    overview_headers["BATTERY"] = [epower_w]( const catacurses::window & w, int y ) {
        std::string batt;
        if( std::abs( epower_w ) < 10000 ) {
            batt = string_format( _( "Batteries: %s%+4d W</color>" ),
                                  health_color( epower_w >= 0 ), epower_w );
        } else {
            batt = string_format( _( "Batteries: %s%+4.1f kW</color>" ),
                                  health_color( epower_w >= 0 ), epower_w / 1000.0 );
        }
        trim_and_print( w, point( 1, y ), getmaxx( w ) - 2, c_light_gray, batt );
        right_print( w, y, 1, c_light_gray, _( "Capacity  Status" ) );
    };
    overview_headers["REACTOR"] = [this, epower_w]( const catacurses::window & w, int y ) {
        int reactor_epower_w = veh->max_reactor_epower_w();
        if( reactor_epower_w > 0 && epower_w < 0 ) {
            reactor_epower_w += epower_w;
        }
        std::string reactor;
        if( reactor_epower_w == 0 ) {
            reactor = _( "Reactors" );
        } else if( reactor_epower_w < 10000 ) {
            reactor = string_format( _( "Reactors: Up to %s%+4d W</color>" ),
                                     health_color( reactor_epower_w ), reactor_epower_w );
        } else {
            reactor = string_format( _( "Reactors: Up to %s%+4.1f kW</color>" ),
                                     health_color( reactor_epower_w ), reactor_epower_w / 1000.0 );
        }
        trim_and_print( w, point( 1, y ), getmaxx( w ) - 2, c_light_gray, reactor );
        right_print( w, y, 1, c_light_gray, _( "Contents     Qty" ) );
    };
    overview_headers["TURRET"] = []( const catacurses::window & w, int y ) {
        trim_and_print( w, point( 1, y ), getmaxx( w ) - 2, c_light_gray, _( "Turrets" ) );
        right_print( w, y, 1, c_light_gray, _( "Ammo     Qty" ) );
    };
    overview_headers["SEAT"] = []( const catacurses::window & w, int y ) {
        trim_and_print( w, point( 1, y ), getmaxx( w ) - 2, c_light_gray, _( "Seats" ) );
        right_print( w, y, 1, c_light_gray, _( "Who" ) );
    };

    char hotkey = 'a';

    for( auto &pt : veh->parts ) {
        if( pt.is_engine() && pt.is_available() ) {
            // if tank contains something then display the contents in milliliters
            auto details = []( const vehicle_part & pt, const catacurses::window & w, int y ) {
                right_print(
                    w, y, 1, item::find_type( pt.ammo_current() )->color,
                    string_format(
                        "%s     <color_light_gray>%s</color>",
                        !pt.fuel_current().is_null() ? item::nname( pt.fuel_current() ) : "",
                        //~ translation should not exceed 3 console cells
                        right_justify( pt.enabled ? _( "Yes" ) : _( "No" ), 3 ) ) );
            };

            // display engine faults (if any)
            auto msg_cb = [&]( const vehicle_part & pt ) {
                msg = std::string();
                for( const auto &e : pt.faults() ) {
                    msg = msg.value() + string_format( "%s\n  %s\n\n", colorize( e->name(), c_red ),
                                                       colorize( e->description(), c_light_gray ) );
                }
            };
            overview_opts.emplace_back( "ENGINE", &pt, next_hotkey( pt, hotkey ), details, msg_cb );
        }
    }

    for( vehicle_part &pt : veh->parts ) {
        if( pt.is_tank() && pt.is_available() ) {
            auto details = []( const vehicle_part & pt, const catacurses::window & w, int y ) {
                if( !pt.ammo_current().is_null() ) {
                    std::string specials;
                    // vehicle parts can only have one pocket, and we are showing a liquid, which can only be one.
                    const item &it = pt.base.contents.legacy_front();
                    // a space isn't actually needed in front of the tags here,
                    // but item::display_name tags use a space so this prevents
                    // needing *second* translation for the same thing with a
                    // space in front of it
                    if( it.item_tags.count( "FROZEN" ) ) {
                        specials += _( " (frozen)" );
                    } else if( it.rotten() ) {
                        specials += _( " (rotten)" );
                    }
                    const itype *pt_ammo_cur = item::find_type( pt.ammo_current() );
                    auto stack = units::legacy_volume_factor / pt_ammo_cur->stack_size;
                    int offset = 1;
                    std::string fmtstring = "%s %s  %5.1fL";
                    if( pt.is_leaking() ) {
                        fmtstring = "%s %s " + leak_marker + "%5.1fL" + leak_marker;
                        offset = 0;
                    }
                    right_print( w, y, offset, pt_ammo_cur->color,
                                 string_format( fmtstring, specials, pt_ammo_cur->nname( 1 ),
                                                round_up( to_liter( pt.ammo_remaining() * stack ), 1 ) ) );
                } else {
                    if( pt.is_leaking() ) {
                        std::string outputstr = leak_marker + "      " + leak_marker;
                        right_print( w, y, 0, c_light_gray, outputstr );
                    }
                }
            };
            overview_opts.emplace_back( "TANK", &pt, next_hotkey( pt, hotkey ), details );
        } else if( pt.is_fuel_store() && !( pt.is_battery() || pt.is_reactor() ) && !pt.is_broken() ) {
            auto details = []( const vehicle_part & pt, const catacurses::window & w, int y ) {
                if( !pt.ammo_current().is_null() ) {
                    const itype *pt_ammo_cur = item::find_type( pt.ammo_current() );
                    auto stack = units::legacy_volume_factor / pt_ammo_cur->stack_size;
                    int offset = 1;
                    std::string fmtstring = "%s  %5.1fL";
                    if( pt.is_leaking() ) {
                        fmtstring = "%s  " + leak_marker + "%5.1fL" + leak_marker;
                        offset = 0;
                    }
                    right_print( w, y, offset, pt_ammo_cur->color,
                                 string_format( fmtstring, item::nname( pt.ammo_current() ),
                                                round_up( to_liter( pt.ammo_remaining() * stack ), 1 ) ) );
                }
            };
            overview_opts.emplace_back( "TANK", &pt, next_hotkey( pt, hotkey ), details );
        }
    }

    for( auto &pt : veh->parts ) {
        if( pt.is_battery() && pt.is_available() ) {
            // always display total battery capacity and percentage charge
            auto details = []( const vehicle_part & pt, const catacurses::window & w, int y ) {
                int pct = ( static_cast<double>( pt.ammo_remaining() ) / pt.ammo_capacity(
                                ammotype( "battery" ) ) ) * 100;
                int offset = 1;
                std::string fmtstring = "%i    %3i%%";
                if( pt.is_leaking() ) {
                    fmtstring = "%i   " + leak_marker + "%3i%%" + leak_marker;
                    offset = 0;
                }
                right_print( w, y, offset, item::find_type( pt.ammo_current() )->color,
                             string_format( fmtstring, pt.ammo_capacity( ammotype( "battery" ) ), pct ) );
            };
            overview_opts.emplace_back( "BATTERY", &pt, next_hotkey( pt, hotkey ), details );
        }
    }

    auto details_ammo = []( const vehicle_part & pt, const catacurses::window & w, int y ) {
        if( pt.ammo_remaining() ) {
            int offset = 1;
            std::string fmtstring = "%s   %5i";
            if( pt.is_leaking() ) {
                fmtstring = "%s  " + leak_marker + "%5i" + leak_marker;
                offset = 0;
            }
            right_print( w, y, offset, item::find_type( pt.ammo_current() )->color,
                         string_format( fmtstring, item::nname( pt.ammo_current() ), pt.ammo_remaining() ) );
        }
    };

    for( auto &pt : veh->parts ) {
        if( pt.is_reactor() && pt.is_available() ) {
            overview_opts.emplace_back( "REACTOR", &pt, next_hotkey( pt, hotkey ), details_ammo );
        }
    }

    for( auto &pt : veh->parts ) {
        if( pt.is_turret() && pt.is_available() ) {
            overview_opts.emplace_back( "TURRET", &pt, next_hotkey( pt, hotkey ), details_ammo );
        }
    }

    for( auto &pt : veh->parts ) {
        auto details = []( const vehicle_part & pt, const catacurses::window & w, int y ) {
            const npc *who = pt.crew();
            if( who ) {
                right_print( w, y, 1, pt.passenger_id == who->getID() ? c_green : c_light_gray, who->name );
            }
        };
        if( pt.is_seat() && pt.is_available() ) {
            overview_opts.emplace_back( "SEAT", &pt, next_hotkey( pt, hotkey ), details );
        }
    }
}

void veh_interact::display_overview()
{
    werase( w_list );
    std::string last;
    int y = 0;
    if( overview_offset ) {
        trim_and_print( w_list, point( 1, y ), getmaxx( w_list ) - 1,
                        c_yellow, _( "'{' to scroll up" ) );
        y++;
    }
    for( int idx = overview_offset; idx != static_cast<int>( overview_opts.size() ); ++idx ) {
        const auto &pt = *overview_opts[idx].part;

        // if this is a new section print a header row
        if( last != overview_opts[idx].key ) {
            y += last.empty() ? 0 : 1;
            overview_headers[overview_opts[idx].key]( w_list, y );
            y += 2;
            last = overview_opts[idx].key;
        }

        bool highlighted = false;
        // No action means no selecting, just highlight relevant ones
        if( overview_pos < 0 && overview_enable && !overview_action ) {
            highlighted = overview_enable( pt );
        } else if( overview_pos == idx ) {
            highlighted = true;
        }

        // print part name
        nc_color col = overview_opts[idx].hotkey ? c_white : c_dark_gray;
        trim_and_print( w_list, point( 1, y ), getmaxx( w_list ) - 1,
                        highlighted ? hilite( col ) : col,
                        "<color_dark_gray>%c </color>%s",
                        overview_opts[idx].hotkey ? overview_opts[idx].hotkey : ' ', pt.name() );

        // print extra columns (if any)
        overview_opts[idx].details( pt, w_list, y );
        y++;
        if( y < ( getmaxy( w_list ) - 1 ) ) {
            overview_limit = overview_offset;
        } else {
            overview_limit = idx;
            trim_and_print( w_list, point( 1, y ), getmaxx( w_list ) - 1,
                            c_yellow, _( "'}' to scroll down" ) );
            break;
        }
    }

    wrefresh( w_list );
}

void veh_interact::overview( const overview_enable_t &enable,
                             const overview_action_t &action )
{
    restore_on_out_of_scope<overview_enable_t> prev_overview_enable( overview_enable );
    restore_on_out_of_scope<overview_action_t> prev_overview_action( overview_action );
    overview_enable = enable;
    overview_action = action;

    restore_on_out_of_scope<int> prev_overview_pos( overview_pos );

    shared_ptr_fast<ui_adaptor> current_ui = create_or_get_ui_adaptor();

    while( true ) {
        calc_overview();

        if( overview_pos < 0 || static_cast<size_t>( overview_pos ) >= overview_opts.size() ) {
            overview_pos = -1;
            do {
                if( ++overview_pos >= static_cast<int>( overview_opts.size() ) ) {
                    overview_pos = -1;
                    break; // nothing could be selected
                }
            } while( !overview_opts[overview_pos].hotkey );
        }

        const bool has_any_hotkey = std::any_of( overview_opts.begin(), overview_opts.end(),
        []( const part_option & e ) {
            return e.hotkey;
        } );
        if( !has_any_hotkey ) {
            return; // nothing is selectable
        }

        if( overview_pos >= 0 && static_cast<size_t>( overview_pos ) < overview_opts.size() ) {
            move_cursor( ( overview_opts[overview_pos].part->mount + dd ).rotate( 3 ) );
        }

        if( overview_pos >= 0 && static_cast<size_t>( overview_pos ) < overview_opts.size() &&
            overview_opts[overview_pos].message ) {
            overview_opts[overview_pos].message( *overview_opts[overview_pos].part );
        } else {
            msg.reset();
        }

        ui_manager::redraw();

        const std::string input = main_context.handle_input();
        msg.reset();
        if( input == "CONFIRM" && overview_opts[overview_pos].hotkey && overview_action ) {
            overview_action( *overview_opts[overview_pos].part );
            break;

        } else if( input == "QUIT" ) {
            break;

        } else if( input == "UP" ) {
            do {
                move_overview_line( -1 );
                if( --overview_pos < 0 ) {
                    overview_pos = overview_opts.size() - 1;
                }
            } while( !overview_opts[overview_pos].hotkey );
        } else if( input == "DOWN" ) {
            do {
                move_overview_line( 1 );
                if( ++overview_pos >= static_cast<int>( overview_opts.size() ) ) {
                    overview_pos = 0;
                }
            } while( !overview_opts[overview_pos].hotkey );
        } else {
            // did we try and activate a hotkey option?
            char hotkey = main_context.get_raw_input().get_first_input();
            if( hotkey && overview_action ) {
                auto iter = std::find_if( overview_opts.begin(),
                overview_opts.end(), [&hotkey]( const part_option & e ) {
                    return e.hotkey == hotkey;
                } );
                if( iter != overview_opts.end() ) {
                    overview_action( *iter->part );
                    break;
                }
            }
        }
    }
}

void veh_interact::move_overview_line( int amount )
{
    overview_offset += amount;
    overview_offset = std::max( 0, overview_offset );
    overview_offset = std::min( overview_limit, overview_offset );
}

vehicle_part *veh_interact::get_most_damaged_part() const
{
    auto part_damage_comparison = []( const vehicle_part & a, const vehicle_part & b ) {
        return !b.removed && b.base.damage() > a.base.damage();
    };

    auto high_damage_iterator = std::max_element( veh->parts.begin(),
                                veh->parts.end(),
                                part_damage_comparison );
    if( high_damage_iterator == veh->parts.end() ||
        high_damage_iterator->removed ) {
        return nullptr;
    }

    return &( *high_damage_iterator );
}

vehicle_part *veh_interact::get_most_repariable_part() const
{
    auto &part = veh_utils::most_repairable_part( *veh, g->u );
    return part ? &part : nullptr;
}

bool veh_interact::can_remove_part( int idx, const player &p )
{
    sel_vehicle_part = &veh->parts[idx];
    sel_vpart_info = &sel_vehicle_part->info();
    std::string nmsg;

    if( veh->has_part( "NO_MODIFY_VEHICLE" ) && !sel_vpart_info->has_flag( "SIMPLE_PART" ) ) {
        msg = _( "This vehicle cannot be modified in this way.\n" );
        return false;
    } else if( sel_vpart_info->has_flag( "NO_UNINSTALL" ) ) {
        msg = _( "This part cannot be uninstalled.\n" );
        return false;
    }

    if( sel_vehicle_part->is_broken() ) {
        nmsg += string_format(
                    _( "<color_white>Removing the broken %1$s may yield some fragments.</color>\n" ),
                    sel_vehicle_part->name() );
    } else {
        item result_of_removal = sel_vehicle_part->properties_to_item();
        nmsg += string_format(
                    _( "<color_white>Removing the %1$s will yield:</color>\n> %2$s\n" ),
                    sel_vehicle_part->name(), result_of_removal.display_name() );
    }

    const auto reqs = sel_vpart_info->removal_requirements();
    bool ok = format_reqs( nmsg, reqs, sel_vpart_info->removal_skills,
                           sel_vpart_info->removal_time( p ) );

    nmsg += _( "<color_white>Additional requirements:</color>\n" );

    int lvl = 0;
    int str = 0;
    quality_id qual;
    bool use_aid = false;
    bool use_str = false;
    if( sel_vpart_info->has_flag( "NEEDS_JACKING" ) ) {
        qual = qual_JACK;
        lvl = jack_quality( *veh );
        str = veh->lift_strength();
        use_aid = ( max_jack >= lvl ) || can_self_jack();
        use_str = g->u.can_lift( *veh );
    } else {
        item base( sel_vpart_info->item );
        qual = qual_LIFT;
        lvl = std::ceil( units::quantity<double, units::mass::unit_type>( base.weight() ) /
                         TOOL_LIFT_FACTOR );
        str = base.lift_strength();
        use_aid = max_lift >= lvl;
        use_str = g->u.can_lift( base );
    }

    if( !( use_aid || use_str ) ) {
        ok = false;
    }
    nc_color aid_color = use_aid ? c_green : ( use_str ? c_dark_gray : c_red );
    nc_color str_color = use_str ? c_green : ( use_aid ? c_dark_gray : c_red );
    const auto helpers = g->u.get_crafting_helpers();
    //~ %1$s is quality name, %2$d is quality level
    std::string aid_string = string_format( _( "1 tool with %1$s %2$d" ),
                                            qual.obj().name, lvl );

    std::string str_string;
    if( !helpers.empty() ) {
        str_string = string_format( _( "strength ( assisted ) %d" ), str );
    } else {
        str_string = string_format( _( "strength %d" ), str );
    }

    nmsg += string_format( _( "> %1$s <color_white>OR</color> %2$s" ),
                           colorize( aid_string, aid_color ),
                           colorize( str_string, str_color ) ) + "\n";
    std::string reason;
    if( !veh->can_unmount( idx, reason ) ) {
        //~ %1$s represents the internal color name which shouldn't be translated, %2$s is pre-translated reason
        nmsg += string_format( _( "> %1$s%2$s</color>" ), status_color( false ), reason ) + "\n";
        ok = false;
    }
    const nc_color desc_color = sel_vehicle_part->is_broken() ? c_dark_gray : c_light_gray;
    sel_vehicle_part->info().format_description( nmsg, desc_color, getmaxx( w_msg ) - 4 );

    msg = colorize( nmsg, c_light_gray );
    return ok || g->u.has_trait( trait_DEBUG_HS );
}

void veh_interact::do_remove()
{
    task_reason reason = cant_do( 'o' );

    if( reason == INVALID_TARGET ) {
        msg = _( "No parts here." );
        return;
    }

    restore_on_out_of_scope<cata::optional<std::string>> prev_title( title );
    title = _( "Choose a part here to remove:" );

    int pos = 0;
    for( size_t i = 0; i < parts_here.size(); i++ ) {
        if( can_remove_part( parts_here[ i ], g->u ) ) {
            pos = i;
            break;
        }
    }
    msg.reset();

    shared_ptr_fast<ui_adaptor> current_ui = create_or_get_ui_adaptor();

    restore_on_out_of_scope<overview_enable_t> prev_overview_enable( overview_enable );

    restore_on_out_of_scope<int> prev_hilight_part( highlight_part );

    while( true ) {
        int part = parts_here[ pos ];

        bool can_remove = can_remove_part( part, g->u );

        overview_enable = [this, part]( const vehicle_part & pt ) {
            return &pt == &veh->parts[part];
        };

        highlight_part = pos;

        calc_overview();
        ui_manager::redraw();

        //read input
        const std::string action = main_context.handle_input();
        msg.reset();
        if( can_remove && ( action == "REMOVE" || action == "CONFIRM" ) ) {
            switch( reason ) {
                case LOW_MORALE:
                    msg = _( "Your morale is too low to construct…" );
                    return;
                case LOW_LIGHT:
                    msg = _( "It's too dark to see what you are doing…" );
                    return;
                case NOT_FREE:
                    msg = _( "You cannot remove that part while something is attached to it." );
                    return;
                case MOVING_VEHICLE:
                    msg = _( "Better not remove something while driving." );
                    return;
                default:
                    break;
            }

            // Modifying a vehicle with rotors will make in not flightworthy (until we've got a better model)
            // It can only be the player doing this - an npc won't work well with query_yn
            if( veh->would_prevent_flyable( veh->parts[part].info() ) ) {
                if( query_yn(
                        _( "Removing this part will mean that this vehicle is no longer flightworthy.  Continue?" ) ) ) {
                    veh->set_flyable( false );
                } else {
                    return;
                }
            }
            const std::vector<npc *> helpers = g->u.get_crafting_helpers();
            for( const npc *np : helpers ) {
                add_msg( m_info, _( "%s helps with this task…" ), np->name );
            }
            sel_cmd = 'o';
            break;
        } else if( action == "QUIT" ) {
            break;
        } else {
            move_in_list( pos, action, parts_here.size() );
        }
    }
}

void veh_interact::do_siphon()
{
    switch( cant_do( 's' ) ) {
        case INVALID_TARGET:
            msg = _( "The vehicle has no liquid fuel left to siphon." );
            return;

        case LACK_TOOLS:
            msg = _( "You need a <color_red>hose</color> to siphon liquid fuel." );
            return;

        case MOVING_VEHICLE:
            msg = _( "You can't siphon from a moving vehicle." );
            return;

        default:
            break;
    }

    restore_on_out_of_scope<cata::optional<std::string>> prev_title( title );
    title = _( "Select part to siphon:" );

    auto sel = [&]( const vehicle_part & pt ) {
        return( pt.is_tank() && !pt.base.contents.empty() &&
                pt.base.contents.only_item().made_of( phase_id::LIQUID ) );
    };

    auto act = [&]( const vehicle_part & pt ) {
        const item &base = pt.get_base();
        const int idx = veh->find_part( base );
        item liquid( base.contents.legacy_front() );
        const int liq_charges = liquid.charges;
        if( liquid_handler::handle_liquid( liquid, nullptr, 1, nullptr, veh, idx ) ) {
            veh->drain( idx, liq_charges - liquid.charges );
        }
    };

    overview( sel, act );
}

bool veh_interact::do_unload()
{
    switch( cant_do( 'd' ) ) {
        case INVALID_TARGET:
            msg = _( "The vehicle has no solid fuel left to remove." );
            return false;

        case MOVING_VEHICLE:
            msg = _( "You can't unload from a moving vehicle." );
            return false;

        default:
            break;
    }

    act_vehicle_unload_fuel( veh );
    return true;
}

void veh_interact::do_assign_crew()
{
    if( cant_do( 'w' ) != CAN_DO ) {
        msg = _( "Need at least one seat and an ally to assign crew members." );
        return;
    }

    restore_on_out_of_scope<cata::optional<std::string>> prev_title( title );
    title = _( "Assign crew positions:" );

    auto sel = []( const vehicle_part & pt ) {
        return pt.is_seat();
    };

    auto act = [&]( vehicle_part & pt ) {
        uilist menu;
        menu.text = _( "Select crew member" );

        if( pt.crew() ) {
            menu.addentry( 0, true, 'c', _( "Clear assignment" ) );
        }

        for( const npc *e : g->allies() ) {
            menu.addentry( e->getID().get_value(), true, -1, e->name );
        }

        menu.query();
        if( menu.ret == 0 ) {
            pt.unset_crew();
        } else if( menu.ret > 0 ) {
            const auto &who = *g->critter_by_id<npc>( character_id( menu.ret ) );
            veh->assign_seat( pt, who );
        }
    };

    overview( sel, act );
}

void veh_interact::do_rename()
{
    std::string name = string_input_popup()
                       .title( _( "Enter new vehicle name:" ) )
                       .width( 20 )
                       .query_string();
    if( !name.empty() ) {
        veh->name = name;
        if( veh->tracking_on ) {
            overmap_buffer.remove_vehicle( veh );
            // Add the vehicle again, this time with the new name
            overmap_buffer.add_vehicle( veh );
        }
    }
}

void veh_interact::do_relabel()
{
    if( cant_do( 'a' ) == INVALID_TARGET ) {
        msg = _( "There are no parts here to label." );
        return;
    }

    const vpart_position vp( *veh, cpart );
    std::string text = string_input_popup()
                       .title( _( "New label:" ) )
                       .width( 20 )
                       .text( vp.get_label().value_or( "" ) )
                       .query_string();
    // empty input removes the label
    vp.set_label( text );
}

/**
 * Returns the first part on the vehicle at the given position.
 * @param d The coordinates, relative to the viewport's 0-point (?)
 * @return The first vehicle part at the specified coordinates.
 */
int veh_interact::part_at( const point &d )
{
    const point vd = -dd + d.rotate( 1 );
    return veh->part_displayed_at( vd );
}

/**
 * Checks to see if you can potentially install this part at current position.
 * Affects coloring in display_list() and is also used to
 * sort can_mount so potentially installable parts come first.
 */
bool veh_interact::can_potentially_install( const vpart_info &vpart )
{
    return g->u.has_trait( trait_DEBUG_HS ) ||
           vpart.install_requirements().can_make_with_inventory( crafting_inv, is_crafting_component );
}

/**
 * Moves the cursor on the vehicle editing window.
 * @param d How far to move the cursor.
 * @param dstart_at How far to change the start position for vehicle part descriptions
 */
void veh_interact::move_cursor( const point &d, int dstart_at )
{
    dd += d.rotate( 3 );
    if( d != point_zero ) {
        start_limit = 0;
    } else {
        start_at += dstart_at;
    }

    // Update the current active component index to the new position.
    cpart = part_at( point_zero );
    const point vd = -dd;
    const point q = veh->coord_translate( vd );
    const tripoint vehp = veh->global_pos3() + q;
    const bool has_critter = g->critter_at( vehp );
    bool obstruct = g->m.impassable_ter_furn( vehp );
    const optional_vpart_position ovp = g->m.veh_at( vehp );
    if( ovp && &ovp->vehicle() != veh ) {
        obstruct = true;
    }

    can_mount.clear();
    if( !obstruct ) {
        int divider_index = 0;
        for( const auto &e : vpart_info::all() ) {
            const vpart_info &vp = e.second;
            if( has_critter && vp.has_flag( VPFLAG_OBSTACLE ) ) {
                continue;
            }
            if( veh->can_mount( vd, vp.get_id() ) ) {
                if( vp.get_id() != vpart_shapes[ vp.name() + vp.item.str() ][ 0 ]->get_id() ) {
                    // only add first shape to install list
                    continue;
                }
                if( can_potentially_install( vp ) ) {
                    can_mount.insert( can_mount.begin() + divider_index++, &vp );
                } else {
                    can_mount.push_back( &vp );
                }
            }
        }
    }

    need_repair.clear();
    parts_here.clear();
    wheel = nullptr;
    if( cpart >= 0 ) {
        parts_here = veh->parts_at_relative( veh->parts[cpart].mount, true );
        for( size_t i = 0; i < parts_here.size(); i++ ) {
            auto &pt = veh->parts[parts_here[i]];

            if( pt.base.damage() > 0 && pt.info().is_repairable() ) {
                need_repair.push_back( i );
            }
            if( pt.info().has_flag( "WHEEL" ) ) {
                wheel = &pt;
            }
        }
    }

    /* Update the lifting quality to be the that is available for this newly selected tile */
    cache_tool_availability_update_lifting( vehp );
}

void veh_interact::display_grid()
{
    // border window
    draw_border( w_border );

    // match grid lines
    const int y_mode = getmaxy( w_mode ) + 1;
    // |-
    mvwputch( w_border, point( 0, y_mode ), BORDER_COLOR, LINE_XXXO );
    // -|
    mvwputch( w_border, point( TERMX - 1, y_mode ), BORDER_COLOR, LINE_XOXX );
    const int y_list = getbegy( w_list ) + getmaxy( w_list );
    // |-
    mvwputch( w_border, point( 0, y_list ), BORDER_COLOR, LINE_XXXO );
    // -|
    mvwputch( w_border, point( TERMX - 1, y_list ), BORDER_COLOR, LINE_XOXX );

    const int grid_w = getmaxx( w_border ) - 2;

    // Two lines dividing the three middle sections.
    for( int i = 1 + getmaxy( w_mode ); i < ( 1 + getmaxy( w_mode ) + page_size ); ++i ) {
        // |
        mvwputch( w_border, point( getmaxx( w_disp ) + 1, i + 1 ), BORDER_COLOR, LINE_XOXO );
        // |
        mvwputch( w_border, point( getmaxx( w_disp ) + 2 + getmaxx( w_list ), i + 1 ), BORDER_COLOR,
                  LINE_XOXO );
    }
    // Two lines dividing the vertical menu sections.
    for( int i = 0; i < grid_w; ++i ) {
        // -
        mvwputch( w_border, point( i + 1, getmaxy( w_mode ) + 1 ), BORDER_COLOR, LINE_OXOX );
        // -
        mvwputch( w_border, point( i + 1, getmaxy( w_mode ) + 2 + page_size ), BORDER_COLOR, LINE_OXOX );
    }
    // Fix up the line intersections.
    mvwputch( w_border, point( getmaxx( w_disp ) + 1, getmaxy( w_mode ) + 1 ), BORDER_COLOR,
              LINE_OXXX );
    // _|_
    mvwputch( w_border, point( getmaxx( w_disp ) + 1, getmaxy( w_mode ) + 2 + page_size ), BORDER_COLOR,
              LINE_XXOX );
    mvwputch( w_border, point( getmaxx( w_disp ) + 2 + getmaxx( w_list ), getmaxy( w_mode ) + 1 ),
              BORDER_COLOR, LINE_OXXX );
    // _|_
    mvwputch( w_border, point( getmaxx( w_disp ) + 2 + getmaxx( w_list ),
                               getmaxy( w_mode ) + 2 + page_size ),
              BORDER_COLOR, LINE_XXOX );

    wrefresh( w_border );
}

/**
 * Draws the viewport with the vehicle.
 */
void veh_interact::display_veh()
{
    werase( w_disp );
    const point h_size = point( getmaxx( w_disp ), getmaxy( w_disp ) ) / 2;

    if( debug_mode ) {
        // show CoM, pivot in debug mode

        const point &pivot = veh->pivot_point();
        const point &com = veh->local_center_of_mass();

        mvwprintz( w_disp, point_zero, c_green, "CoM   %d,%d", com.x, com.y );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( w_disp, point( 0, 1 ), c_red,   "Pivot %d,%d", pivot.x, pivot.y );

        const point com_s = ( com + dd ).rotate( 3 ) + h_size;
        const point pivot_s = ( pivot + dd ).rotate( 3 ) + h_size;

        for( int x = 0; x < getmaxx( w_disp ); ++x ) {
            if( x <= com_s.x ) {
                mvwputch( w_disp, point( x, com_s.y ), c_green, LINE_OXOX );
            }

            if( x >= pivot_s.x ) {
                mvwputch( w_disp, point( x, pivot_s.y ), c_red, LINE_OXOX );
            }
        }

        for( int y = 0; y < getmaxy( w_disp ); ++y ) {
            if( y <= com_s.y ) {
                mvwputch( w_disp, point( com_s.x, y ), c_green, LINE_XOXO );
            }

            if( y >= pivot_s.y ) {
                mvwputch( w_disp, point( pivot_s.x, y ), c_red, LINE_XOXO );
            }
        }
    }

    // Draw guidelines to make current selection point more visible.
    for( int y = 0; y < getmaxy( w_disp ); ++y ) {
        mvwputch( w_disp, point( h_size.x, y ), c_dark_gray, LINE_XOXO );
    }

    for( int x = 0; x < getmaxx( w_disp ); ++x ) {
        mvwputch( w_disp, point( x, h_size.y ), c_dark_gray, LINE_OXOX );
    }

    //Iterate over structural parts so we only hit each square once
    std::vector<int> structural_parts = veh->all_parts_at_location( "structure" );
    for( auto &structural_part : structural_parts ) {
        const int p = structural_part;
        int sym = veh->part_sym( p );
        nc_color col = veh->part_color( p );

        const point q = ( veh->parts[p].mount + dd ).rotate( 3 );

        if( q == point_zero ) {
            col = hilite( col );
            cpart = p;
        }
        mvwputch( w_disp, h_size + q, col, special_symbol( sym ) );
    }

    const int hw = getmaxx( w_disp ) / 2;
    const int hh = getmaxy( w_disp ) / 2;
    const point vd = -dd;
    const point q = veh->coord_translate( vd );
    const tripoint vehp = veh->global_pos3() + q;
    bool obstruct = g->m.impassable_ter_furn( vehp );
    const optional_vpart_position ovp = g->m.veh_at( vehp );
    if( ovp && &ovp->vehicle() != veh ) {
        obstruct = true;
    }
    nc_color col = cpart >= 0 ? veh->part_color( cpart ) : c_black;
    int sym = cpart >= 0 ? veh->part_sym( cpart ) : ' ';
    mvwputch( w_disp, point( hw, hh ), obstruct ? red_background( col ) : hilite( col ),
              special_symbol( sym ) );
    wrefresh( w_disp );
}

static std::string wheel_state_description( const vehicle &veh )
{
    bool is_boat = !veh.floating.empty();
    bool is_land = !veh.wheelcache.empty() || !is_boat;

    bool suf_land = veh.sufficient_wheel_config();
    bool bal_land = veh.balanced_wheel_config();

    bool suf_boat = veh.can_float();

    float steer = veh.steering_effectiveness();

    std::string wheel_status;
    if( !suf_land && is_boat ) {
        wheel_status = _( "<color_light_red>disabled</color>" );
    } else if( !suf_land ) {
        wheel_status = _( "<color_light_red>lack</color>" );
    } else if( !bal_land ) {
        wheel_status = _( "<color_light_red>unbalanced</color>" );
    } else if( steer < 0 ) {
        wheel_status = _( "<color_light_red>no steering</color>" );
    } else if( steer < 0.033 ) {
        wheel_status = _( "<color_light_red>broken steering</color>" );
    } else if( steer < 0.5 ) {
        wheel_status = _( "<color_light_red>poor steering</color>" );
    } else {
        wheel_status = _( "<color_light_green>enough</color>" );
    }

    std::string boat_status;
    if( !suf_boat ) {
        boat_status = _( "<color_light_red>sinks</color>" );
    } else {
        boat_status = _( "<color_light_blue>floats</color>" );
    }

    if( is_boat && is_land ) {
        return string_format( _( "Wheels/boat: %s/%s" ), wheel_status, boat_status );
    }

    if( is_boat ) {
        return string_format( _( "Boat: %s" ), boat_status );
    }

    return string_format( _( "Wheels: %s" ), wheel_status );
}

/**
 * Displays the vehicle's stats at the bottom of the window.
 */
void veh_interact::display_stats() const
{
    werase( w_stats );

    // see exec()
    const int extraw = ( ( TERMX - FULL_SCREEN_WIDTH ) / 4 ) * 2;
    // 3 * stats_h
    const int slots = 24;
    int x[slots], y[slots], w[slots];

    units::volume total_cargo = 0_ml;
    units::volume free_cargo = 0_ml;
    for( const vpart_reference &vp : veh->get_any_parts( "CARGO" ) ) {
        const size_t p = vp.part_index();
        total_cargo += veh->max_volume( p );
        free_cargo += veh->free_volume( p );
    }

    const int second_column = 33 + ( extraw / 4 );
    const int third_column = 65 + ( extraw / 2 );
    for( int i = 0; i < slots; i++ ) {
        if( i < stats_h ) {
            // First column
            x[i] = 1;
            y[i] = i;
            w[i] = second_column - 2;
        } else if( i < ( 2 * stats_h ) ) {
            // Second column
            x[i] = second_column;
            y[i] = i - stats_h;
            w[i] = third_column - second_column - 1;
        } else {
            // Third column
            x[i] = third_column;
            y[i] = i - 2 * stats_h;
            w[i] = extraw - third_column - 2;
        }
    }

    bool is_boat = !veh->floating.empty();
    bool is_ground = !veh->wheelcache.empty() || !is_boat;
    bool is_aircraft = veh->is_rotorcraft() && veh->is_flying_in_air();

    const auto vel_to_int = []( const double vel ) {
        return static_cast<int>( convert_velocity( vel, VU_VEHICLE ) );
    };

    int i = 0;
    if( is_aircraft ) {
        fold_and_print( w_stats, point( x[i], y[i] ), w[i], c_light_gray,
                        _( "Air Safe/Top Speed: <color_light_green>%3d</color>/<color_light_red>%3d</color> %s" ),
                        vel_to_int( veh->safe_rotor_velocity( false ) ),
                        vel_to_int( veh->max_rotor_velocity( false ) ),
                        velocity_units( VU_VEHICLE ) );
        i += 1;
        fold_and_print( w_stats, point( x[i], y[i] ), w[i], c_light_gray,
                        _( "Air Acceleration: <color_light_blue>%3d</color> %s/s" ),
                        vel_to_int( veh->rotor_acceleration( false ) ),
                        velocity_units( VU_VEHICLE ) );
        i += 1;
    } else {
        if( is_ground ) {
            fold_and_print( w_stats, point( x[i], y[i] ), w[i], c_light_gray,
                            _( "Safe/Top Speed: <color_light_green>%3d</color>/<color_light_red>%3d</color> %s" ),
                            vel_to_int( veh->safe_ground_velocity( false ) ),
                            vel_to_int( veh->max_ground_velocity( false ) ),
                            velocity_units( VU_VEHICLE ) );
            i += 1;
            // TODO: extract accelerations units to its own function
            fold_and_print( w_stats, point( x[i], y[i] ), w[i], c_light_gray,
                            //~ /t means per turn
                            _( "Acceleration: <color_light_blue>%3d</color> %s/s" ),
                            vel_to_int( veh->ground_acceleration( false ) ),
                            velocity_units( VU_VEHICLE ) );
            i += 1;
        } else {
            i += 2;
        }
        if( is_boat ) {
            fold_and_print( w_stats, point( x[i], y[i] ), w[i], c_light_gray,
                            _( "Water Safe/Top Speed: <color_light_green>%3d</color>/<color_light_red>%3d</color> %s" ),
                            vel_to_int( veh->safe_water_velocity( false ) ),
                            vel_to_int( veh->max_water_velocity( false ) ),
                            velocity_units( VU_VEHICLE ) );
            i += 1;
            // TODO: extract accelerations units to its own function
            fold_and_print( w_stats, point( x[i], y[i] ), w[i], c_light_gray,
                            //~ /t means per turn
                            _( "Water Acceleration: <color_light_blue>%3d</color> %s/s" ),
                            vel_to_int( veh->water_acceleration( false ) ),
                            velocity_units( VU_VEHICLE ) );
            i += 1;
        } else {
            i += 2;
        }
    }
    fold_and_print( w_stats, point( x[i], y[i] ), w[i], c_light_gray,
                    _( "Mass: <color_light_blue>%5.0f</color> %s" ),
                    convert_weight( veh->total_mass() ), weight_units() );
    i += 1;
    fold_and_print( w_stats, point( x[i], y[i] ), w[i], c_light_gray,
                    _( "Cargo Volume: <color_light_blue>%s</color> / <color_light_blue>%s</color> %s" ),
                    format_volume( total_cargo - free_cargo ),
                    format_volume( total_cargo ), volume_units_abbr() );
    i += 1;
    // Write the overall damage
    mvwprintz( w_stats, point( x[i], y[i] ), c_light_gray, _( "Status:" ) );
    x[i] += utf8_width( _( "Status:" ) ) + 1;
    fold_and_print( w_stats, point( x[i], y[i] ), w[i], total_durability_color, total_durability_text );
    i += 1;

    fold_and_print( w_stats, point( x[i], y[i] ), w[i], c_light_gray,
                    wheel_state_description( *veh ) );
    i += 1;

    //This lambda handles printing parts in the "Most damaged" and "Needs repair" cases
    //for the veh_interact ui
    const auto print_part = [&]( const std::string & str, int slot, vehicle_part * pt ) {
        mvwprintz( w_stats, point( x[slot], y[slot] ), c_light_gray, str );
        int iw = utf8_width( str ) + 1;
        return fold_and_print( w_stats, point( x[slot] + iw, y[slot] ), w[slot], c_light_gray, pt->name() );
    };

    vehicle_part *mostDamagedPart = get_most_damaged_part();
    vehicle_part *most_repairable = get_most_repariable_part();

    // Write the most damaged part
    if( mostDamagedPart ) {
        const std::string damaged_header = mostDamagedPart == most_repairable ?
                                           _( "Most damaged:" ) :
                                           _( "Most damaged (can't repair):" );
        i += print_part( damaged_header, i, mostDamagedPart );
    } else {
        i += 1;
    }

    // Write the part that needs repair the most.
    if( most_repairable && most_repairable != mostDamagedPart ) {
        const std::string needsRepair = _( "Needs repair:" );
        i += print_part( needsRepair, i, most_repairable );
    } else {
        i += 1;
    }

    fold_and_print( w_stats, point( x[i], y[i] ), w[i], c_light_gray,
                    _( "Air drag:       <color_light_blue>%5.2f</color>" ),
                    veh->coeff_air_drag() );
    i += 1;

    if( is_boat ) {
        fold_and_print( w_stats, point( x[i], y[i] ), w[i], c_light_gray,
                        _( "Water drag:     <color_light_blue>%5.2f</color>" ),
                        veh->coeff_water_drag() );
    }
    i += 1;

    if( is_ground ) {
        fold_and_print( w_stats, point( x[i], y[i] ), w[i], c_light_gray,
                        _( "Rolling drag:   <color_light_blue>%5.2f</color>" ),
                        veh->coeff_rolling_drag() );
    }
    i += 1;

    fold_and_print( w_stats, point( x[i], y[i] ), w[i], c_light_gray,
                    _( "Static drag:    <color_light_blue>%5d</color>" ),
                    veh->static_drag( false ) );
    i += 1;

    fold_and_print( w_stats, point( x[i], y[i] ), w[i], c_light_gray,
                    _( "Offroad:        <color_light_blue>%4d</color>%%" ),
                    static_cast<int>( veh->k_traction( veh->wheel_area() *
                                      veh->average_or_rating() ) * 100 ) );
    i += 1;

    if( is_boat ) {

        const double water_clearance = veh->water_hull_height() - veh->water_draft();
        std::string draft_string = water_clearance > 0 ?
                                   _( "Draft/Clearance:<color_light_blue>%4.2f</color>m/<color_light_blue>%4.2f</color>m" ) :
                                   _( "Draft/Clearance:<color_light_blue>%4.2f</color>m/<color_light_red>%4.2f</color>m" ) ;

        fold_and_print( w_stats, point( x[i], y[i] ), w[i], c_light_gray,
                        draft_string.c_str(),
                        veh->water_draft(), water_clearance );
        i += 1;
    }

    i = std::max( i, 2 * stats_h );

    // Print fuel percentage & type name only if it fits in the window, 10 is width of "E...F 100%"
    veh->print_fuel_indicators( w_stats, point( x[i], y[i] ), fuel_index, true,
                                ( x[ i ] + 10 < getmaxx( w_stats ) ),
                                ( x[ i ] + 10 < getmaxx( w_stats ) ) );

    if( install_info ) {
        const int details_w = getmaxx( w_details );
        // clear rightmost blocks of w_stats to avoid overlap
        int stats_col_2 = 33;
        int stats_col_3 = 65 + ( ( TERMX - FULL_SCREEN_WIDTH ) / 4 );
        int clear_x = getmaxx( w_stats ) - details_w + 1 >= stats_col_3 ? stats_col_3 : stats_col_2;
        for( int i = 0; i < getmaxy( w_stats ); i++ ) {
            mvwhline( w_stats, point( clear_x, i ), ' ', getmaxx( w_stats ) - clear_x );
        }
    }

    wrefresh( w_stats );
}

void veh_interact::display_name()
{
    werase( w_name );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w_name, point( 1, 0 ), c_light_gray, _( "Name: " ) );

    mvwprintz( w_name, point( 1 + utf8_width( _( "Name: " ) ), 0 ),
               !veh->is_owned_by( g->u, true ) ? c_light_red : c_light_green,
               string_format( _( "%s (%s)" ), veh->name, veh->get_owner_name() ) );
    wrefresh( w_name );
}

/**
 * Prints the list of usable commands, and highlights the hotkeys used to activate them.
 */
void veh_interact::display_mode()
{
    werase( w_mode );

    if( title.has_value() ) {
        nc_color title_col = c_light_gray;
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        print_colored_text( w_mode, point( 1, 0 ), title_col, title_col, title.value() );
    } else {
        size_t esc_pos = display_esc( w_mode );

        // broken indentation preserved to avoid breaking git history for large number of lines
        const std::array<std::string, 10> actions = { {
                { _( "<i>nstall" ) },
                { _( "<r>epair" ) },
                { _( "<m>end" ) },
                { _( "re<f>ill" ) },
                { _( "rem<o>ve" ) },
                { _( "<s>iphon" ) },
                { _( "unloa<d>" ) },
                { _( "cre<w>" ) },
                { _( "r<e>name" ) },
                { _( "l<a>bel" ) },
            }
        };

        const std::array<bool, std::tuple_size<decltype( actions )>::value> enabled = { {
                !cant_do( 'i' ),
                !cant_do( 'r' ),
                !cant_do( 'm' ),
                !cant_do( 'f' ),
                !cant_do( 'o' ),
                !cant_do( 's' ),
                !cant_do( 'd' ),
                !cant_do( 'w' ),
                true,          // 'rename' is always available
                !cant_do( 'a' ),
            }
        };

        int pos[std::tuple_size<decltype( actions )>::value + 1];
        pos[0] = 1;
        for( size_t i = 0; i < actions.size(); i++ ) {
            pos[i + 1] = pos[i] + utf8_width( actions[i] ) - 2;
        }
        int spacing = static_cast<int>( ( esc_pos - 1 - pos[actions.size()] ) / actions.size() );
        int shift = static_cast<int>( ( esc_pos - pos[actions.size()] - spacing *
                                        ( actions.size() - 1 ) ) / 2 ) - 1;
        for( size_t i = 0; i < actions.size(); i++ ) {
            shortcut_print( w_mode, point( pos[i] + spacing * i + shift, 0 ),
                            enabled[i] ? c_light_gray : c_dark_gray, enabled[i] ? c_light_green : c_green,
                            actions[i] );
        }
    }
    wrefresh( w_mode );
}

size_t veh_interact::display_esc( const catacurses::window &win )
{
    std::string backstr = _( "<ESC>-back" );
    // right text align
    size_t pos = getmaxx( win ) - utf8_width( backstr ) + 2;
    shortcut_print( win, point( pos, 0 ), c_light_gray, c_light_green, backstr );
    return pos;
}

/**
 * Draws the list of parts that can be mounted in the selected square. Used
 * when installing new parts.
 * @param pos The current cursor position in the list.
 * @param list The list to display parts from.
 * @param header Number of lines occupied by the list header
 */
void veh_interact::display_list( size_t pos, const std::vector<const vpart_info *> &list,
                                 const int header )
{
    werase( w_list );
    int lines_per_page = page_size - header;
    size_t page = pos / lines_per_page;
    for( size_t i = page * lines_per_page; i < ( page + 1 ) * lines_per_page && i < list.size(); i++ ) {
        const vpart_info &info = *list[i];
        int y = i - page * lines_per_page + header;
        mvwputch( w_list, point( 1, y ), info.color, special_symbol( info.sym ) );
        nc_color col = can_potentially_install( info ) ? c_white : c_dark_gray;
        trim_and_print( w_list, point( 3, y ), getmaxx( w_list ) - 3, pos == i ? hilite( col ) : col,
                        info.name() );
    }

    if( install_info ) {
        auto &tab_list = install_info->tab_list;
        auto &tab_list_short = install_info->tab_list_short;
        auto &tab = install_info->tab;
        // draw tab menu
        int tab_x = 0;
        for( size_t i = 0; i < tab_list.size(); i++ ) {
            std::string tab_name = ( tab == i ) ? tab_list[i] : tab_list_short[i]; // full name for selected tab
            tab_x += ( tab == i ); // add a space before selected tab
            draw_subtab( w_list, tab_x, tab_name, tab == i, false );
            tab_x += ( 1 + utf8_width( tab_name ) + ( tab ==
                       i ) ); // one space padding and add a space after selected tab
        }
    }
    wrefresh( w_list );
}

/**
 * Used when installing parts.
 * Opens up w_details containing info for part currently selected in w_list.
 */
void veh_interact::display_details( const vpart_info *part )
{
    const int details_w = getmaxx( w_details );

    werase( w_details );

    wborder( w_details, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO,
             LINE_XOOX );

    if( part == nullptr ) {
        wrefresh( w_details );
        return;
    }
    // displays data in two columns
    int column_width = details_w / 2;
    int col_1 = 2;
    int col_2 = col_1 + column_width;
    int line = 0;
    bool small_mode = column_width < 20;

    // line 0: part name
    fold_and_print( w_details, point( col_1, line ), details_w, c_light_green, part->name() );

    // line 1: (column 1) durability   (column 2) damage mod
    fold_and_print( w_details, point( col_1, line + 1 ), column_width, c_white,
                    "%s: <color_light_gray>%d</color>",
                    small_mode ? _( "Dur" ) : _( "Durability" ),
                    part->durability );
    fold_and_print( w_details, point( col_2, line + 1 ), column_width, c_white,
                    "%s: <color_light_gray>%d%%</color>",
                    small_mode ? _( "Dmg" ) : _( "Damage" ),
                    part->dmg_mod );

    // line 2: (column 1) weight   (column 2) folded volume (if applicable)
    fold_and_print( w_details, point( col_1, line + 2 ), column_width, c_white,
                    "%s: <color_light_gray>%.1f%s</color>",
                    small_mode ? _( "Wgt" ) : _( "Weight" ),
                    convert_weight( item::find_type( part->item )->weight ),
                    weight_units() );
    if( part->folded_volume != 0_ml ) {
        fold_and_print( w_details, point( col_2, line + 2 ), column_width, c_white,
                        "%s: <color_light_gray>%s %s</color>",
                        small_mode ? _( "FoldVol" ) : _( "Folded Volume" ),
                        format_volume( part->folded_volume ),
                        volume_units_abbr() );
    }

    // line 3: (column 1) size, bonus, wheel diameter (if applicable)    (column 2) epower, wheel width (if applicable)
    if( part->size > 0_ml && part->has_flag( VPFLAG_CARGO ) ) {
        fold_and_print( w_details, point( col_1, line + 3 ), column_width, c_white,
                        "%s: <color_light_gray>%s %s</color>",
                        small_mode ? _( "Cap" ) : _( "Capacity" ),
                        format_volume( part->size ), volume_units_abbr() );
    }

    if( part->bonus > 0 ) {
        std::string label;
        if( part->has_flag( VPFLAG_SEATBELT ) ) {
            label = small_mode ? _( "Str" ) : _( "Strength" );
        } else if( part->has_flag( "HORN" ) ) {
            label = _( "Noise" );
        } else if( part->has_flag( "MUFFLER" ) ) {
            label = small_mode ? _( "NoisRed" ) : _( "Noise Reduction" );
        } else if( part->has_flag( VPFLAG_EXTENDS_VISION ) ) {
            label = _( "Range" );
        } else if( part->has_flag( VPFLAG_LIGHT ) || part->has_flag( VPFLAG_CONE_LIGHT ) ||
                   part->has_flag( VPFLAG_WIDE_CONE_LIGHT ) ||
                   part->has_flag( VPFLAG_CIRCLE_LIGHT ) || part->has_flag( VPFLAG_DOME_LIGHT ) ||
                   part->has_flag( VPFLAG_AISLE_LIGHT ) || part->has_flag( VPFLAG_EVENTURN ) ||
                   part->has_flag( VPFLAG_ODDTURN ) || part->has_flag( VPFLAG_ATOMIC_LIGHT ) ) {
            label = _( "Light" );
        }

        if( !label.empty() ) {
            fold_and_print( w_details, point( col_1, line + 3 ), column_width, c_white,
                            "%s: <color_light_gray>%d</color>", label,
                            part->bonus );
        }
    }

    if( part->has_flag( VPFLAG_WHEEL ) ) {
        // Note: there is no guarantee that whl is non-empty!
        const cata::value_ptr<islot_wheel> &whl = item::find_type( part->item )->wheel;
        fold_and_print( w_details, point( col_1, line + 3 ), column_width, c_white,
                        "%s: <color_light_gray>%d\"</color>",
                        small_mode ? _( "Dia" ) : _( "Wheel Diameter" ),
                        whl ? whl->diameter : 0 );
        fold_and_print( w_details, point( col_2, line + 3 ), column_width, c_white,
                        "%s: <color_light_gray>%d\"</color>",
                        small_mode ? _( "Wdt" ) : _( "Wheel Width" ),
                        whl ? whl->width : 0 );
    }

    if( part->epower != 0 ) {
        fold_and_print( w_details, point( col_2, line + 3 ), column_width, c_white,
                        "%s: <color_light_gray>%+4d</color>",
                        small_mode ? _( "Epwr" ) : _( "Electric Power" ),
                        part->epower );
    }

    // line 4 [horizontal]: fuel_type (if applicable)
    // line 4 [vertical/hybrid]: (column 1) fuel_type (if applicable)    (column 2) power (if applicable)
    // line 5 [horizontal]: power (if applicable)
    if( !part->fuel_type.is_null() ) {
        fold_and_print( w_details, point( col_1, line + 4 ), column_width,
                        c_white, _( "Charge: <color_light_gray>%s</color>" ),
                        item::nname( part->fuel_type ) );
    }
    int part_consumption = part->energy_consumption;
    if( part_consumption != 0 ) {
        fold_and_print( w_details, point( col_2, line + 4 ), column_width, c_white,
                        _( "Drain: <color_light_gray>%+8d</color>" ), -part_consumption );
    }

    // line 5 [vertical/hybrid] flags
    std::vector<std::string> flags = { { "OPAQUE", "OPENABLE", "BOARDABLE" } };
    std::vector<std::string> flag_labels = { { _( "opaque" ), _( "openable" ), _( "boardable" ) } };
    std::string label;
    for( size_t i = 0; i < flags.size(); i++ ) {
        if( part->has_flag( flags[i] ) ) {
            label += ( label.empty() ? "" : " " ) + flag_labels[i];
        }
    }
    // 6 [horizontal]: (column 1) flags    (column 2) battery capacity (if applicable)
    fold_and_print( w_details, point( col_1, line + 5 ), details_w, c_yellow, label );

    if( part->fuel_type == itype_battery && !part->has_flag( VPFLAG_ENGINE ) &&
        !part->has_flag( VPFLAG_ALTERNATOR ) ) {
        const cata::value_ptr<islot_magazine> &battery = item::find_type( part->item )->magazine;
        fold_and_print( w_details, point( col_2, line + 5 ), column_width, c_white,
                        "%s: <color_light_gray>%8d</color>",
                        small_mode ? _( "BatCap" ) : _( "Battery Capacity" ),
                        battery->capacity );
    } else {
        int part_power = part->power;
        if( part_power == 0 ) {
            part_power = item( part->item ).engine_displacement();
        }
        if( part_power != 0 ) {
            fold_and_print( w_details, point( col_2, line + 5 ), column_width, c_white,
                            _( "Power: <color_light_gray>%+8d</color>" ), part_power );
        }
    }

    wrefresh( w_details );
}

void veh_interact::count_durability()
{
    int qty = std::accumulate( veh->parts.begin(), veh->parts.end(), 0,
    []( int lhs, const vehicle_part & rhs ) {
        return lhs + std::max( rhs.base.damage(), 0 );
    } );

    int total = std::accumulate( veh->parts.begin(), veh->parts.end(), 0,
    []( int lhs, const vehicle_part & rhs ) {
        return lhs + rhs.base.max_damage();
    } );

    int pct = total ? 100 * qty / total : 0;

    if( pct < 5 ) {
        total_durability_text = _( "like new" );
        total_durability_color = c_light_green;

    } else if( pct < 33 ) {
        total_durability_text = _( "dented" );
        total_durability_color = c_yellow;

    } else if( pct < 66 ) {
        total_durability_text = _( "battered" );
        total_durability_color = c_magenta;

    } else if( pct < 100 ) {
        total_durability_text = _( "wrecked" );
        total_durability_color = c_red;

    } else {
        total_durability_text = _( "destroyed" );
        total_durability_color = c_dark_gray;
    }
}

void act_vehicle_siphon( vehicle *veh )
{
    std::vector<itype_id> fuels;
    bool has_liquid = false;
    for( const vpart_reference &vp : veh->get_any_parts( VPFLAG_FLUIDTANK ) ) {
        if( vp.part().get_base().contents.legacy_front().made_of( phase_id::LIQUID ) ) {
            has_liquid = true;
            break;
        }
    }
    if( !has_liquid ) {
        add_msg( m_info, _( "The vehicle has no liquid fuel left to siphon." ) );
        return;
    }

    std::string title = _( "Select tank to siphon:" );
    auto sel = []( const vehicle_part & pt ) {
        return pt.is_tank() && pt.get_base().contents.legacy_front().made_of( phase_id::LIQUID );
    };
    vehicle_part &tank = veh_interact::select_part( *veh, sel, title );
    if( tank ) {
        const item &base = tank.get_base();
        const int idx = veh->find_part( base );
        item liquid( base.contents.legacy_front() );
        const int liq_charges = liquid.charges;
        if( liquid_handler::handle_liquid( liquid, nullptr, 1, nullptr, veh, idx ) ) {
            veh->drain( idx, liq_charges - liquid.charges );
            veh->invalidate_mass();
        }
    }
}

void act_vehicle_unload_fuel( vehicle *veh )
{
    std::vector<itype_id> fuels;
    for( auto &e : veh->fuels_left() ) {
        const itype *type = item::find_type( e.first );

        if( e.first == fuel_type_battery || type->phase != phase_id::SOLID ) {
            // This skips battery and plutonium cells
            continue;
        }
        fuels.push_back( e.first );
    }
    if( fuels.empty() ) {
        add_msg( m_info, _( "The vehicle has no solid fuel left to remove." ) );
        return;
    }
    itype_id fuel;
    if( fuels.size() > 1 ) {
        uilist smenu;
        smenu.text = _( "Remove what?" );
        for( auto &fuel : fuels ) {
            if( fuel == itype_plut_cell && veh->fuel_left( fuel ) < PLUTONIUM_CHARGES ) {
                continue;
            }
            smenu.addentry( item::nname( fuel ) );
        }
        smenu.query();
        if( smenu.ret < 0 || static_cast<size_t>( smenu.ret ) >= fuels.size() ) {
            add_msg( m_info, _( "Never mind." ) );
            return;
        }
        fuel = fuels[smenu.ret];
    } else {
        fuel = fuels.front();
    }

    int qty = veh->fuel_left( fuel );
    if( fuel == itype_plut_cell ) {
        if( qty / PLUTONIUM_CHARGES == 0 ) {
            add_msg( m_info, _( "The vehicle has no charged plutonium cells." ) );
            return;
        }
        item plutonium( fuel, calendar::turn, qty / PLUTONIUM_CHARGES );
        g->u.i_add( plutonium );
        veh->drain( fuel, qty - ( qty % PLUTONIUM_CHARGES ) );
    } else {
        item solid_fuel( fuel, calendar::turn, qty );
        g->u.i_add( solid_fuel );
        veh->drain( fuel, qty );
    }

}

/**
 * Called when the activity timer for installing parts, repairing, etc times
 * out and the action is complete.
 */
void veh_interact::complete_vehicle( player &p )
{
    if( p.activity.values.size() < 7 ) {
        debugmsg( "Invalid activity ACT_VEHICLE values:%d", p.activity.values.size() );
        return;
    }
    optional_vpart_position vp = g->m.veh_at( g->m.getlocal( tripoint( p.activity.values[0],
                                 p.activity.values[1], p.posz() ) ) );
    if( !vp ) {
        // so the vehicle could have lost some of its parts from other NPCS works during this player/NPCs activity.
        // check the vehicle points that were stored at beginning of activity.
        if( !p.activity.coord_set.empty() ) {
            for( const auto pt : p.activity.coord_set ) {
                vp = g->m.veh_at( g->m.getlocal( pt ) );
                if( vp ) {
                    break;
                }
            }
        }
        // check again, to see if it really is a case of vehicle gone missing.
        if( !vp ) {
            debugmsg( "Activity ACT_VEHICLE: vehicle not found" );
            return;
        }
    }
    vehicle *const veh = &vp->vehicle();

    int dx = p.activity.values[4];
    int dy = p.activity.values[5];
    int vehicle_part = p.activity.values[6];
    const vpart_id part_id( p.activity.str_values[0] );

    const vpart_info &vpinfo = part_id.obj();

    // cmd = Install Repair reFill remOve Siphon Unload reName relAbel
    switch( static_cast<char>( p.activity.index ) ) {
        case 'i': {
            const inventory &inv = p.crafting_inventory();

            const auto reqs = vpinfo.install_requirements();
            if( !reqs.can_make_with_inventory( inv, is_crafting_component ) ) {
                add_msg( m_info, _( "You don't meet the requirements to install the %s." ), vpinfo.name() );
                break;
            }

            // consume items extracting a match for the parts base item
            item base;
            for( const auto &e : reqs.get_components() ) {
                for( auto &obj : p.consume_items( e, 1, is_crafting_component ) ) {
                    if( obj.typeId() == vpinfo.item ) {
                        base = obj;
                    }
                }
            }
            if( base.is_null() ) {
                if( !p.has_trait( trait_DEBUG_HS ) ) {
                    add_msg( m_info, _( "Could not find base part in requirements for %s." ), vpinfo.name() );
                    break;
                } else {
                    base = item( vpinfo.item );
                }
            }

            for( const auto &e : reqs.get_tools() ) {
                p.consume_tools( e );
            }

            p.invalidate_crafting_inventory();

            int partnum = !base.is_null() ? veh->install_part( point( dx, dy ), part_id,
                          std::move( base ) ) : -1;
            if( partnum < 0 ) {
                debugmsg( "complete_vehicle install part fails dx=%d dy=%d id=%s", dx, dy, part_id.c_str() );
                break;
            }

            // Need map-relative coordinates to compare to output of look_around.
            // Need to call coord_translate() directly since it's a new part.
            const point q = veh->coord_translate( point( dx, dy ) );

            if( vpinfo.has_flag( VPFLAG_CONE_LIGHT ) ||
                vpinfo.has_flag( VPFLAG_WIDE_CONE_LIGHT ) ||
                vpinfo.has_flag( VPFLAG_HALF_CIRCLE_LIGHT ) ) {
                // Stash offset and set it to the location of the part so look_around will start there.
                const tripoint old_view_offset = p.view_offset;
                const tripoint offset = veh->global_pos3() + q;
                p.view_offset = offset - p.pos();

                point delta;
                do {
                    popup( _( "Press space, choose a facing direction for the new %s and confirm with enter." ),
                           vpinfo.name() );
                    const cata::optional<tripoint> chosen = g->look_around();
                    if( !chosen ) {
                        continue;
                    }
                    delta = ( *chosen - offset ).xy();
                    // atan2 only gives reasonable values when delta is not all zero
                } while( delta == point_zero );

                // Restore previous view offsets.
                p.view_offset = old_view_offset;

                int dir = static_cast<int>( atan2( static_cast<float>( delta.y ),
                                                   static_cast<float>( delta.x ) ) * 180.0 / M_PI );
                dir -= veh->face.dir();
                while( dir < 0 ) {
                    dir += 360;
                }
                while( dir > 360 ) {
                    dir -= 360;
                }

                veh->parts[partnum].direction = dir;
            }

            const tripoint vehp = veh->global_pos3() + tripoint( q, 0 );
            // TODO: allow boarding for non-players as well.
            player *const pl = g->critter_at<player>( vehp );
            if( vpinfo.has_flag( VPFLAG_BOARDABLE ) && pl ) {
                g->m.board_vehicle( vehp, pl );
            }

            p.add_msg_if_player( m_good, _( "You install a %1$s into the %2$s." ), veh->parts[ partnum ].name(),
                                 veh->name );

            for( const auto &sk : vpinfo.install_skills ) {
                p.practice( sk.first, veh_utils::calc_xp_gain( vpinfo, sk.first ) );
            }

            break;
        }

        case 'r': {
            veh_utils::repair_part( *veh, veh->parts[ vehicle_part ], p );
            break;
        }

        case 'f': {
            if( p.activity.targets.empty() || !p.activity.targets.front() ) {
                debugmsg( "Activity ACT_VEHICLE: missing refill source" );
                break;
            }

            item_location &src = p.activity.targets.front();
            struct vehicle_part &pt = veh->parts[ vehicle_part ];
            if( pt.is_tank() && src->is_container() && !src->contents.empty() ) {
                item &contained = src->contents.legacy_front();
                contained.charges -= pt.base.fill_with( *contained.type, contained.charges );
                src->on_contents_changed();

                if( pt.remaining_ammo_capacity() ) {
                    //~ 1$s vehicle name, 2$s tank name
                    p.add_msg_if_player( m_good, _( "You refill the %1$s's %2$s." ), veh->name, pt.name() );
                } else {
                    //~ 1$s vehicle name, 2$s tank name
                    p.add_msg_if_player( m_good, _( "You completely refill the %1$s's %2$s." ), veh->name, pt.name() );
                }

                if( src->contents.legacy_front().charges == 0 ) {
                    src->remove_item( src->contents.legacy_front() );
                } else {
                    p.add_msg_if_player( m_good, _( "There's some left over!" ) );
                }

            } else if( pt.is_fuel_store() ) {
                auto qty = src->charges;
                pt.base.reload( p, std::move( src ), qty );

                //~ 1$s vehicle name, 2$s reactor name
                p.add_msg_if_player( m_good, _( "You refuel the %1$s's %2$s." ), veh->name, pt.name() );

            } else {
                debugmsg( "vehicle part is not reloadable" );
                break;
            }

            veh->invalidate_mass();
            break;
        }

        case 'o': {
            const inventory &inv = p.crafting_inventory();
            if( vehicle_part >= static_cast<int>( veh->parts.size() ) ) {
                vehicle_part = veh->get_next_shifted_index( vehicle_part, p );
                if( vehicle_part == -1 ) {
                    p.add_msg_if_player( m_info, _( "The %s has already been removed by someone else." ),
                                         vpinfo.name() );
                    return;
                }
            }
            const auto reqs = vpinfo.removal_requirements();
            if( !reqs.can_make_with_inventory( inv, is_crafting_component ) ) {
                add_msg( m_info, _( "You don't meet the requirements to remove the %s." ), vpinfo.name() );
                break;
            }

            for( const auto &e : reqs.get_components() ) {
                p.consume_items( e, 1, is_crafting_component );
            }
            for( const auto &e : reqs.get_tools() ) {
                p.consume_tools( e );
            }

            p.invalidate_crafting_inventory();

            // This will be a list of all the items which arise from this removal.
            std::list<item> resulting_items;

            // First we get all the contents of the part
            vehicle_stack contents = veh->get_items( vehicle_part );
            resulting_items.insert( resulting_items.end(), contents.begin(), contents.end() );
            contents.clear();

            // Power cables must remove parts from the target vehicle, too.
            if( veh->part_flag( vehicle_part, "POWER_TRANSFER" ) ) {
                veh->remove_remote_part( vehicle_part );
            }
            if( veh->is_towing() || veh->is_towed() ) {
                std::cout << "vehicle is towing/towed" << std::endl;
                vehicle *other_veh = veh->is_towing() ? veh->tow_data.get_towed() : veh->tow_data.get_towed_by();
                if( other_veh ) {
                    std::cout << "other veh exists" << std::endl;
                    other_veh->remove_part( other_veh->part_with_feature( other_veh->get_tow_part(), "TOW_CABLE",
                                            true ) );
                    other_veh->tow_data.clear_towing();
                }
                veh->tow_data.clear_towing();
            }
            bool broken = veh->parts[ vehicle_part ].is_broken();

            if( broken ) {
                p.add_msg_if_player( _( "You remove the broken %1$s from the %2$s." ),
                                     veh->parts[ vehicle_part ].name(), veh->name );
            } else {
                p.add_msg_if_player( _( "You remove the %1$s from the %2$s." ),
                                     veh->parts[ vehicle_part ].name(), veh->name );
            }

            if( !broken ) {
                resulting_items.push_back( veh->parts[vehicle_part].properties_to_item() );
                for( const auto &sk : vpinfo.install_skills ) {
                    // removal is half as educational as installation
                    p.practice( sk.first, veh_utils::calc_xp_gain( vpinfo, sk.first ) / 2 );
                }

            } else {
                auto pieces = veh->parts[vehicle_part].pieces_for_broken_part();
                resulting_items.insert( resulting_items.end(), pieces.begin(), pieces.end() );
            }

            if( veh->parts.size() < 2 ) {
                p.add_msg_if_player( _( "You completely dismantle the %s." ), veh->name );
                p.activity.set_to_null();
                g->m.destroy_vehicle( veh );
            } else {
                veh->remove_part( vehicle_part );
                veh->part_removal_cleanup();
            }
            // This will be part of an NPC "job" where they need to clean up the acitivty items afterwards
            if( p.is_npc() ) {
                for( item &it : resulting_items ) {
                    it.set_var( "activity_var", p.name );
                }
            }
            // Finally, put all the reults somewhere (we wanted to wait until this
            // point because we don't want to put them back into the vehicle part
            // that just got removed).
            put_into_vehicle_or_drop( p, item_drop_reason::deliberate, resulting_items );
            break;
        }
    }
    p.invalidate_crafting_inventory();
}
