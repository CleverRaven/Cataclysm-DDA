#include "veh_interact.h"
#include <string>
#include "vehicle.h"
#include "overmapbuffer.h"
#include "game.h"
#include "player.h"
#include "action.h"
#include "map.h"
#include "output.h"
#include "catacharset.h"
#include "string_formatter.h"
#include "map_selector.h"
#include "crafting.h"
#include "options.h"
#include "debug.h"
#include "skill.h"
#include "messages.h"
#include "translations.h"
#include "veh_type.h"
#include "vpart_position.h"
#include "ui.h"
#include "itype.h"
#include "cata_utility.h"
#include "vehicle_selector.h"
#include "fault.h"
#include "npc.h"
#include "string_input_popup.h"
#include "veh_utils.h"

#include <cmath>
#include <list>
#include <functional>
#include <iterator>
#include <algorithm>
#include <numeric>
#include <cassert>

static inline const char *status_color( bool status )
{
    static const char *good = "green";
    static const char *bad = "red";
    return status ? good : bad;
}

// cap JACK requirements to support arbitrarily large vehicles
static double jack_qality( const vehicle &veh )
{
    const units::quantity<double, units::mass::unit_type> mass = std::min( veh.total_mass(),
            JACK_LIMIT );
    return ceil( mass / TOOL_LIFT_FACTOR );
}

/** Can part currently be reloaded with anything? */
static auto can_refill = []( const vehicle_part &pt )
{
    return pt.can_reload();
};

namespace
{
const std::string repair_hotkeys( "r1234567890" );
const quality_id LIFT( "LIFT" );
const quality_id JACK( "JACK" );
const skill_id skill_mechanics( "mechanics" );
} // namespace

void act_vehicle_siphon( vehicle *veh );

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
            if( pt->is_broken() ) {
                time = vp->install_time( g->u );
            } else {
                // why repairing part that cannot be damaged?
                assert( pt->base.max_damage() > 0 );
                time = vp->repair_time( g->u ) * double( pt->base.damage() ) / pt->base.max_damage();
            }
            break;
        case 'o':
            time = vp->removal_time( g->u );
            break;
        case 'c':
            time = vp->removal_time( g->u ) + vp->install_time( g->u );
            break;
    }
    if( g->u.has_trait( trait_id( "DEBUG_HS" ) ) ) {
        time = 1;
    }
    player_activity res( activity_id( "ACT_VEHICLE" ), time, ( int ) sel_cmd );

    // if we're working on an existing part, use that part as the reference point
    // otherwise (e.g. installing a new frame), just use part 0
    point q = veh->coord_translate( pt ? pt->mount : veh->parts[0].mount );
    res.values.push_back( veh->global_x() + q.x );    // values[0]
    res.values.push_back( veh->global_y() + q.y );    // values[1]
    res.values.push_back( ddx );   // values[2]
    res.values.push_back( ddy );   // values[3]
    res.values.push_back( -ddx );   // values[4]
    res.values.push_back( -ddy );   // values[5]
    res.values.push_back( veh->index_of_part( pt ) ); // values[6]
    res.str_values.push_back( vp->get_id().str() );
    res.targets.emplace_back( std::move( target ) );

    return res;
}

player_activity veh_interact::run( vehicle &veh, int x, int y )
{
    veh_interact vehint( veh, x, y );
    vehint.do_main_loop();
    g->refresh_all();
    return vehint.serialize_activity();
}

vehicle_part &veh_interact::select_part( const vehicle &veh, const part_selector &sel,
        const std::string &title )
{
    static vehicle_part null_part;
    vehicle_part *res = &null_part;

    auto act = [&]( const vehicle_part & pt ) {
        res = const_cast<vehicle_part *>( &pt );
        return false; // avoid redraw
    };

    int opts = std::count_if( veh.parts.cbegin(), veh.parts.cend(), sel );

    if( opts == 1 ) {
        act( *std::find_if( veh.parts.cbegin(), veh.parts.cend(), sel ) );

    } else if( opts != 0 ) {
        veh_interact vehint( const_cast<vehicle &>( veh ) );
        vehint.set_title( title.empty() ? _( "Select part" ) : title );
        vehint.overview( sel, act );
        g->refresh_all();
    }

    return *res;
}

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );

/**
 * Creates a blank veh_interact window.
 */
veh_interact::veh_interact( vehicle &veh, int x, int y )
    : ddx( x ), ddy( y ), veh( &veh ), main_context( "VEH_INTERACT" )
{
    // Only build the shapes map and the wheel list once
    for( const auto &e : vpart_info::all() ) {
        const vpart_info &vp = e.second;
        vpart_shapes[ vp.name() + vp.item ].push_back( &vp );
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
    main_context.register_action( "TIRE_CHANGE" );
    main_context.register_action( "ASSIGN_CREW" );
    main_context.register_action( "RELABEL" );
    main_context.register_action( "PREV_TAB" );
    main_context.register_action( "NEXT_TAB" );
    main_context.register_action( "FUEL_LIST_DOWN" );
    main_context.register_action( "FUEL_LIST_UP" );
    main_context.register_action( "DESC_LIST_DOWN" );
    main_context.register_action( "DESC_LIST_UP" );
    main_context.register_action( "CONFIRM" );
    main_context.register_action( "HELP_KEYBINDINGS" );
    main_context.register_action( "FILTER" );

    countDurability();
    cache_tool_availability();
    allocate_windows();
}

veh_interact::~veh_interact() = default;

void veh_interact::allocate_windows()
{
    // grid window
    const int grid_w = TERMX - 2; // exterior borders take 2
    const int grid_h = TERMY - 2; // exterior borders take 2
    w_grid = catacurses::newwin( grid_h, grid_w, 1, 1 );

    int mode_h  = 1;
    int name_h  = 1;
    int stats_h = 6;

    page_size = grid_h - ( mode_h + stats_h + name_h ) - 2;

    int pane_y = 1 + mode_h + 1;

    int pane_w = ( grid_w / 3 ) - 1;

    int disp_w = grid_w - ( pane_w * 2 ) - 2;
    int disp_h = page_size * 0.45;
    int parts_h = page_size - disp_h;
    int parts_y = pane_y + disp_h;

    int name_y = pane_y + page_size + 1;
    int stats_y = name_y + name_h;

    int list_x = 1 + disp_w + 1;
    int msg_x  = list_x + pane_w + 1;

    // make the windows
    w_mode  = catacurses::newwin( mode_h,    grid_w, 1,       1 );
    w_msg   = catacurses::newwin( page_size, pane_w, pane_y,  msg_x  );
    w_disp  = catacurses::newwin( disp_h,    disp_w, pane_y,  1 );
    w_parts = catacurses::newwin( parts_h,   disp_w, parts_y, 1 );
    w_list  = catacurses::newwin( page_size, pane_w, pane_y,  list_x );
    w_stats = catacurses::newwin( stats_h,   grid_w, stats_y, 1 );
    w_name  = catacurses::newwin( name_h,    grid_w, name_y,  1 );

    display_grid();
    display_name();
    display_stats();
    display_veh();
    move_cursor(0, 0); // display w_disp & w_parts
}

void veh_interact::set_title( const std::string &msg ) const
{
    werase( w_mode );
    nc_color col = c_light_gray;
    print_colored_text( w_mode, 0, 1, col, col, msg );
    wrefresh( w_mode );
}

bool veh_interact::format_reqs( std::ostringstream& msg, const requirement_data &reqs,
                                const std::map<skill_id, int> &skills, int moves ) const {

    const auto inv = g->u.crafting_inventory();
    bool ok = reqs.can_make_with_inventory( inv );

    msg << _( "<color_white>Time required:</color>\n" );
    //@todo: better have a from_moves function
    msg << "> " << to_string_approx( time_duration::from_turns( moves / 100 ) ) << "\n";

    msg << _( "<color_white>Skills required:</color>\n" );
    for( const auto& e : skills ) {
        bool hasSkill = g->u.get_skill_level( e.first ) >= e.second;
        if( !hasSkill ) {
            ok = false;
        }
        //~ %1$s represents the internal color name which shouldn't be translated, %2$s is skill name, and %3$i is skill level
        msg << string_format( _( "> <color_%1$s>%2$s %3$i</color>\n" ), status_color( hasSkill ),
                              e.first.obj().name().c_str(), e.second );
    }
    if( skills.empty() ) {
        msg << string_format( "> <color_%1$s>%2$s</color>", status_color( true ), _( "NONE" ) ) << "\n";
    }

    auto comps = reqs.get_folded_components_list( getmaxx( w_msg ) - 2, c_white, inv );
    std::copy( comps.begin(), comps.end(), std::ostream_iterator<std::string>( msg, "\n" ) );

    auto tools = reqs.get_folded_tools_list( getmaxx( w_msg ) - 2, c_white, inv );
    std::copy( tools.begin(), tools.end(), std::ostream_iterator<std::string>( msg, "\n" ) );

    return ok;
}

void veh_interact::do_main_loop()
{
    bool finish = false;
    while( !finish ) {
        overview();
        display_mode();
        const std::string action = main_context.handle_input();
        werase( w_msg );
        wrefresh( w_msg );
        std::string msg;
        bool redraw = false;
        int dx = 0;
        int dy = 0;
        if( main_context.get_direction( dx, dy, action ) ) {
            move_cursor( dx, dy );
        } else if( action == "QUIT" ) {
            finish = true;
        } else if( action == "INSTALL" ) {
            redraw = do_install( msg );
        } else if( action == "REPAIR" ) {
            redraw = do_repair( msg );
        } else if( action == "MEND" ) {
            redraw = do_mend( msg );
        } else if( action == "REFILL" ) {
            redraw = do_refill( msg );
        } else if( action == "REMOVE" ) {
            redraw = do_remove( msg );
        } else if( action == "RENAME" ) {
            redraw = do_rename( msg );
        } else if( action == "SIPHON" ) {
            redraw = do_siphon( msg );
            // Siphoning may have started a player activity. If so, we should close the
            // vehicle dialog and continue with the activity.
            finish = !g->u.activity.is_null();
        } else if( action == "TIRE_CHANGE" ) {
            redraw = do_tirechange( msg );
        } else if( action == "ASSIGN_CREW" ) {
            redraw = do_assign_crew( msg );
        } else if( action == "RELABEL" ) {
            redraw = do_relabel( msg );
        } else if( action == "FUEL_LIST_DOWN" ) {
            move_fuel_cursor( 1 );
        } else if( action == "FUEL_LIST_UP" ) {
            move_fuel_cursor( -1 );
        } else if( action == "DESC_LIST_DOWN" ) {
            move_cursor( 0, 0, 1 );
        } else if( action == "DESC_LIST_UP" ) {
            move_cursor( 0, 0, -1 );
        }
        if( sel_cmd != ' ' ) {
            finish = true;
        }

        if( !finish && redraw ) {
            display_grid();
            display_name();
            display_stats();
            display_veh();
            // Horrible hack warning:
            // Part display doesn't have a dedicated display function
            // Siphon menu obscures it, so it has to be redrawn
            move_cursor( 0, 0 );
        }

        if( !msg.empty() ) {
            werase( w_msg );
            fold_and_print( w_msg, 0, 1, getmaxx( w_msg ) - 2, c_light_red, msg );
            wrefresh( w_msg );
        } else {
            move_cursor( 0, 0 );
        }
    }
}

void veh_interact::cache_tool_availability()
{
    crafting_inv = g->u.crafting_inventory();

    has_wrench = crafting_inv.has_quality( quality_id( "WRENCH" ) );

    has_wheel = crafting_inv.has_components( "wheel", 1 ) ||
                crafting_inv.has_components( "wheel_wide", 1 ) ||
                crafting_inv.has_components( "wheel_armor", 1 ) ||
                crafting_inv.has_components( "wheel_bicycle", 1 ) ||
                crafting_inv.has_components( "wheel_motorbike", 1 ) ||
                crafting_inv.has_components( "wheel_small", 1 );

    max_lift = std::max( { g->u.max_quality( LIFT ),
                           map_selector( g->u.pos(), PICKUP_RANGE ).max_quality( LIFT ),
                           vehicle_selector(g->u.pos(), 2, true, *veh ).max_quality( LIFT ) } );

    max_jack = std::max( { g->u.max_quality( JACK ),
                           map_selector( g->u.pos(), PICKUP_RANGE ).max_quality( JACK ),
                           vehicle_selector(g->u.pos(), 2, true, *veh ).max_quality( JACK ) } );

    const double qual = jack_qality( *veh );

    has_jack = g->u.has_quality( JACK, qual ) ||
               map_selector( g->u.pos(), PICKUP_RANGE ).has_quality( JACK, qual ) ||
               vehicle_selector( g->u.pos(), 2, true, *veh ).has_quality( JACK,  qual );
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
task_reason veh_interact::cant_do (char mode)
{
    bool enough_morale = true;
    bool valid_target = false;
    bool has_tools = false;
    bool part_free = true;
    bool has_skill = true;

    switch (mode) {
    case 'i': // install mode
        enough_morale = g->u.has_morale_to_craft();
        valid_target = !can_mount.empty() && 0 == veh->tags.count("convertible");
        //tool checks processed later
        has_tools = true;
        break;
    case 'r': // repair mode
        enough_morale = g->u.has_morale_to_craft();
        valid_target = !need_repair.empty() && cpart >= 0;
        has_tools = true; // checked later
        break;

    case 'm': // mend mode
        enough_morale = g->u.has_morale_to_craft();
        valid_target = std::any_of( veh->parts.begin(), veh->parts.end(), []( const vehicle_part &pt ) {
            return !pt.faults().empty();
        } );
        has_tools = true; // checked later
        break;

    case 'f':
        return std::any_of( veh->parts.begin(), veh->parts.end(), can_refill ) ? CAN_DO : INVALID_TARGET;

    case 'o': // remove mode
        enough_morale = g->u.has_morale_to_craft();
        valid_target = cpart >= 0 && 0 == veh->tags.count("convertible");
        part_free = parts_here.size() > 1 || (cpart >= 0 && veh->can_unmount(cpart));
        //tool and skill checks processed later
        has_tools = true;
        has_skill = true;
        break;
    case 's': // siphon mode
        valid_target = false;
        for( auto & e : veh->fuels_left() ) {
            if( item::find_type( e.first )->phase == LIQUID ) {
                valid_target = true;
                break;
            }
        }
        has_tools = crafting_inv.has_tools( "hose", 1 );
        break;
    case 'c': // change tire
        valid_target = wheel != NULL;
        ///\EFFECT_STR allows changing tires on heavier vehicles without a jack
        has_tools = has_wrench && has_wheel && ( g->u.can_lift( *veh ) || has_jack );
        break;

    case 'w': // assign crew
        if( g->allies().empty() ) {
            return INVALID_TARGET;
        }
        return std::any_of( veh->parts.begin(), veh->parts.end(), []( const vehicle_part &e ) {
            return e.is_seat();
        } ) ? CAN_DO : INVALID_TARGET;

    case 'a': // relabel
        valid_target = cpart >= 0;
        has_tools = true;
        break;
    default:
        return UNKNOWN_TASK;
    }

    if( abs( veh->velocity ) > 100 || g->u.controlling_vehicle ) {
        return MOVING_VEHICLE;
    }
    if( !enough_morale ) {
        return LOW_MORALE;
    }
    if( !valid_target ) {
        return INVALID_TARGET;
    }
    if( !has_tools ) {
        return LACK_TOOLS;
    }
    if( !part_free ) {
        return NOT_FREE;
    }
    if( !has_skill ) { // @todo: that is always false!
        return LACK_SKILL;
    }
    return CAN_DO;
}

bool veh_interact::is_drive_conflict() {
    bool install_muscle_engine = (sel_vpart_info->fuel_type == "muscle");
    bool has_muscle_engine = veh->has_engine_type("muscle", false);
    bool can_install = !(has_muscle_engine && install_muscle_engine);

    if (!can_install) {
        werase (w_msg);
        fold_and_print(w_msg, 0, 1, getmaxx( w_msg ) - 2, c_light_red,
                       _("Only one muscle powered engine can be installed."));
        wrefresh (w_msg);
    }
    return !can_install;
}

bool veh_interact::can_install_part() {
    if( sel_vpart_info == NULL ) {
        werase (w_msg);
        wrefresh (w_msg);
        return false;
    }

    if( is_drive_conflict() ) {
        return false;
    }

    if( sel_vpart_info->has_flag( "FUNNEL" ) ) {
        if( std::none_of( parts_here.begin(), parts_here.end(), [&]( const int e ) {
            return veh->parts[e].is_tank();
        } ) ) {
            werase( w_msg );
            fold_and_print( w_msg, 0, 1, getmaxx( w_msg ) - 2, c_light_red,
                            _( "Funnels need to be installed over a tank." ) );
            wrefresh( w_msg );
            return false;
        }
    }

    if( sel_vpart_info->has_flag( "TURRET" ) ) {
        if( std::any_of( parts_here.begin(), parts_here.end(), [&]( const int e ) {
            return veh->parts[e].is_turret();
        } ) ) {
            werase (w_msg );
            fold_and_print( w_msg, 0, 1, getmaxx( w_msg) - 2, c_light_red,
                            _( "Can't install turret on another turret." ) );
            wrefresh( w_msg );
            return false;
        }
    }

    bool is_engine = sel_vpart_info->has_flag("ENGINE");
    bool install_muscle_engine = (sel_vpart_info->fuel_type == "muscle");
    //count current engines, muscle engines don't require higher skill
    int engines = 0;
    int dif_eng = 0;
    if (is_engine && !install_muscle_engine) {
        for (size_t p = 0; p < veh->parts.size(); p++) {
            if (veh->part_flag (p, "ENGINE") &&
                veh->part_info(p).fuel_type != "muscle")
            {
                engines++;
                dif_eng = dif_eng / 2 + 8;
            }
        }
    }

    int dif_steering = 0;
    if (sel_vpart_info->has_flag("STEERABLE")) {
        std::set<int> axles;
        for (auto &p : veh->steering) {
            if (!veh->part_flag(p, "TRACKED")) {
                // tracked parts don't contribute to axle complexity
                axles.insert(veh->parts[p].mount.x);
            }
        }

        if (axles.size() > 0 && axles.count(-ddx) == 0) {
            // Installing more than one steerable axle is hard
            // (but adding a wheel to an existing axle isn't)
            dif_steering = axles.size() + 5;
        }
    }

    const auto reqs = sel_vpart_info->install_requirements();

    std::ostringstream msg;
    bool ok = format_reqs( msg, reqs, sel_vpart_info->install_skills, sel_vpart_info->install_time( g->u ) );

    msg << _( "<color_white>Additional requirements:</color>\n" );

    if( dif_eng > 0 ) {
        if( g->u.get_skill_level( skill_mechanics ) < dif_eng ) {
            ok = false;
        }
        msg << string_format( _( "> <color_%1$s>%2$s %3$i</color> for extra engines." ),
                              status_color( g->u.get_skill_level( skill_mechanics ) >= dif_eng ),
                              skill_mechanics.obj().name().c_str(), dif_eng ) << "\n";
    }

    if( dif_steering > 0 ) {
        if( g->u.get_skill_level( skill_mechanics ) < dif_steering ) {
            ok = false;
        }
        msg << string_format( _( "> <color_%1$s>%2$s %3$i</color> for extra steering axles." ),
                              status_color( g->u.get_skill_level( skill_mechanics ) >= dif_steering ),
                              skill_mechanics.obj().name().c_str(), dif_steering ) << "\n";
    }

    int lvl = 0;
    int str = 0;
    quality_id qual;
    bool use_aid = false;
    bool use_str = false;
    item base( sel_vpart_info->item );
    if( base.is_wheel() ) {
        qual = JACK;
        lvl = jack_qality( *veh );
        str = veh->lift_strength();
        use_aid = max_jack >= lvl;
        use_str = g->u.can_lift( *veh );
    } else {
        qual = LIFT;
        lvl = std::ceil( units::quantity<double, units::mass::unit_type>( base.weight() ) / TOOL_LIFT_FACTOR );
        str = base.lift_strength();
        use_aid = max_lift >= lvl;
        use_str = g->u.can_lift( base );
    }

    if( !( use_aid || use_str ) ) {
        ok = false;
    }
    msg << string_format( _( "> <color_%1$s>1 tool with %2$s %3$i</color> <color_white>OR</color> <color_%4$s>strength %5$i</color>" ),
                          status_color( use_aid ), qual.obj().name.c_str(), lvl,
                          status_color( use_str ), str ) << "\n";

    std::string install_color = string_format( "<color_%1$s>",  status_color( ok || g->u.has_trait( trait_DEBUG_HS ) ) );
    sel_vpart_info->format_description( msg, install_color, getmaxx( w_msg ) - 4 );

    werase( w_msg );
    fold_and_print( w_msg, 0, 1, getmaxx( w_msg ) - 2, c_light_gray, msg.str() );
    wrefresh( w_msg );
    return ok || g->u.has_trait( trait_DEBUG_HS );
}

/**
 * Moves list of fuels up or down.
 * @param delta -1 if moving up,
 *              1 if moving down
 */
void veh_interact::move_fuel_cursor(int delta)
{
    int max_fuel_indicators = (int)veh->get_printable_fuel_types().size();
    int height = 5;
    fuel_index += delta;

    if(fuel_index < 0) {
        fuel_index = 0;
    } else if(fuel_index > max_fuel_indicators - height) {
        fuel_index = std::max(max_fuel_indicators - height, 0);
    }

    display_stats();
}

bool veh_interact::do_install( std::string &msg )
{
    switch( cant_do( 'i' ) ) {
        case LOW_MORALE:
            msg = _( "Your morale is too low to construct..." );
            return false;

        case INVALID_TARGET:
            msg = _( "Cannot install any part here." );
            return false;

        case MOVING_VEHICLE:
            msg = _( "You can't install parts while driving." );
            return false;

        default: break;
    }

    set_title( _( "Choose new part to install here:" ) );

    std::array<std::string,8> tab_list = { { pgettext("Vehicle Parts|","All"),
                                             pgettext("Vehicle Parts|","Cargo"),
                                             pgettext("Vehicle Parts|","Light"),
                                             pgettext("Vehicle Parts|","Util"),
                                             pgettext("Vehicle Parts|","Hull"),
                                             pgettext("Vehicle Parts|","Internal"),
                                             pgettext("Vehicle Parts|","Other"),
                                             pgettext("Vehicle Parts|","Filter")} };

    std::array<std::string,8> tab_list_short = { { pgettext("Vehicle Parts|","A"),
                                                   pgettext("Vehicle Parts|","C"),
                                                   pgettext("Vehicle Parts|","L"),
                                                   pgettext("Vehicle Parts|","U"),
                                                   pgettext("Vehicle Parts|","H"),
                                                   pgettext("Vehicle Parts|","I"),
                                                   pgettext("Vehicle Parts|","O"),
                                                   pgettext("Vehicle Parts|","F")} };

    std::array <std::function<bool(const vpart_info*)>,8> tab_filters; // filter for each tab, last one
    tab_filters[0] = [&](const vpart_info *) { return true; }; // All
    tab_filters[1] = [&](const vpart_info *p) { auto &part = *p;
                                                   return part.has_flag(VPFLAG_CARGO) && // Cargo
                                                   !part.has_flag("TURRET"); };
    tab_filters[2] = [&](const vpart_info *p) { auto &part = *p;
                                                   return part.has_flag(VPFLAG_LIGHT) || // Light
                                                   part.has_flag(VPFLAG_CONE_LIGHT) ||
                                                   part.has_flag(VPFLAG_CIRCLE_LIGHT) ||
                                                   part.has_flag(VPFLAG_DOME_LIGHT) ||
                                                   part.has_flag(VPFLAG_AISLE_LIGHT) ||
                                                   part.has_flag(VPFLAG_ATOMIC_LIGHT); };
    tab_filters[3] = [&](const vpart_info *p) { auto &part = *p;
                                                   return part.has_flag("TRACK") || //Util
                                                   part.has_flag(VPFLAG_FRIDGE) ||
                                                   part.has_flag("KITCHEN") ||
                                                   part.has_flag("WELDRIG") ||
                                                   part.has_flag("CRAFTRIG") ||
                                                   part.has_flag("CHEMLAB") ||
                                                   part.has_flag("FORGE") ||
                                                   part.has_flag("HORN") ||
                                                   part.has_flag("BEEPER") ||
                                                   part.has_flag("WATCH") ||
                                                   part.has_flag("ALARMCLOCK") ||
                                                   part.has_flag(VPFLAG_RECHARGE) ||
                                                   part.has_flag("VISION") ||
                                                   part.has_flag("POWER_TRANSFER") ||
                                                   part.has_flag("FAUCET") ||
                                                   part.has_flag("STEREO") ||
                                                   part.has_flag("CHIMES") ||
                                                   part.has_flag("MUFFLER") ||
                                                   part.has_flag("REMOTE_CONTROLS") ||
                                                   part.has_flag("CURTAIN") ||
                                                   part.has_flag("SEATBELT") ||
                                                   part.has_flag("SECURITY") ||
                                                   part.has_flag("SEAT") ||
                                                   part.has_flag("BED") ||
                                                   part.has_flag("DOOR_MOTOR") ||
                                                   part.has_flag("WATER_PURIFIER"); };
    tab_filters[4] = [&](const vpart_info *p) { auto &part = *p;
                                                   return(part.has_flag(VPFLAG_OBSTACLE) || // Hull
                                                   part.has_flag("ROOF") ||
                                                   part.has_flag(VPFLAG_ARMOR)) &&
                                                   !part.has_flag("WHEEL") &&
                                                   !tab_filters[3](p); };
    tab_filters[5] = [&](const vpart_info *p) { auto &part = *p;
                                                   return part.has_flag(VPFLAG_ENGINE) || // Internals
                                                   part.has_flag(VPFLAG_ALTERNATOR) ||
                                                   part.has_flag(VPFLAG_CONTROLS) ||
                                                   part.location == "fuel_source" ||
                                                   part.location == "on_battery_mount" ||
                                                   (part.location.empty() && part.has_flag("FUEL_TANK")); };

    // Other: everything that's not in the other filters
    tab_filters[tab_filters.size()-2] = [&](const vpart_info *part) {
        for (size_t i=1; i < tab_filters.size()-2; i++ ) {
            if( tab_filters[i](part) ) return false;
        }
        return true; };

    std::string filter; // The user specified filter
    tab_filters[7] = [&](const vpart_info *p){ return lcmatch( p->name(), filter ); };

    // full list of mountable parts, to be filtered according to tab
    std::vector<const vpart_info*> tab_vparts = can_mount;

    int pos = 0;
    size_t tab = 0;
    while (true) {
        display_list(pos, tab_vparts, 2);

        // draw tab menu
        int tab_x = 0;
        for( size_t i=0; i < tab_list.size(); i++ ){
            std::string tab_name = (tab == i) ? tab_list[i] : tab_list_short[i]; // full name for selected tab
            tab_x += (tab == i); // add a space before selected tab
            draw_subtab(w_list, tab_x, tab_name, tab == i, false);
            tab_x += ( 1 + utf8_width(tab_name) + (tab == i) ); // one space padding and add a space after selected tab
        }
        wrefresh(w_list);

        sel_vpart_info = tab_vparts.empty() ? nullptr : tab_vparts[pos]; // filtered list can be empty

        display_details( sel_vpart_info );

        bool can_install = can_install_part();

        const std::string action = main_context.handle_input();
        if ( action == "FILTER" ){
            string_input_popup()
            .title( _( "Search for part" ) )
            .width( 50 )
            .description( _( "Filter" ) )
            .max_length( 100 )
            .edit( filter );
            tab = 7; // Move to the user filter tab.
            display_grid();
            display_stats();
            display_veh(); // Fix the (currently) mangled windows
            move_cursor(0,0); // Wake up the vehicle display
        }
        if (action == "REPAIR" ){
            filter.clear();
            tab = 0;
        }
        if (action == "INSTALL" || action == "CONFIRM"){
            if (can_install) {
                const auto &shapes = vpart_shapes[ sel_vpart_info->name() + sel_vpart_info->item ];
                int selected_shape = -1;
                if ( shapes.size() > 1 ) { // more than one shape available, display selection
                    std::vector<uimenu_entry> shape_ui_entries;
                    for ( size_t i = 0; i < shapes.size(); i++ ) {
                        uimenu_entry entry = uimenu_entry( i, true, UIMENU_INVALID,
                                                           shapes[i]->name() );
                        entry.extratxt.left = 1;
                        entry.extratxt.sym = special_symbol( shapes[i]->sym );
                        entry.extratxt.color = shapes[i]->color;
                        shape_ui_entries.push_back( entry );
                    }
                    selected_shape = uimenu( true, getbegx( w_list ), getmaxx( w_list ), getbegy( w_list ),
                                             _("Choose shape:"), shape_ui_entries ).ret;
                } else { // only one shape available, default to first one
                    selected_shape = 0;
                }
                 if( 0 <= selected_shape && (size_t) selected_shape < shapes.size() ) {
                    sel_vpart_info = shapes[selected_shape];
                    sel_cmd = 'i';
                    return true; // force redraw
                }
            }
        } else if (action == "QUIT") {
            sel_vpart_info = NULL;
            werase (w_list);
            wrefresh (w_list);
            werase (w_msg);
            wrefresh(w_msg);
            break;
        } else if (action == "PREV_TAB" || action == "NEXT_TAB"|| action == "FILTER" || action == "REPAIR" ) {
            tab_vparts.clear();
            pos = 0;

            if(action == "PREV_TAB") {
                tab = ( tab < 1 ) ? tab_list.size() - 1 : tab - 1;
            } else if (action == "NEXT_TAB") {
                tab = ( tab < tab_list.size() - 1 ) ? tab + 1 : 0;
            }

            copy_if(can_mount.begin(), can_mount.end(), back_inserter(tab_vparts), tab_filters[tab]);
        }
        else {
            move_in_list(pos, action, tab_vparts.size(), 2);
        }
    }

    //destroy w_details
    werase(w_details);
    w_details = catacurses::window();

    //restore windows that had been covered by w_details
    display_stats();
    display_name();

    return false;
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

bool veh_interact::do_repair( std::string &msg )
{
    switch( cant_do( 'r' ) ) {
        case LOW_MORALE:
            msg = _( "Your morale is too low to repair..." );
            return false;

        case INVALID_TARGET:
            {
                vehicle_part *most_repairable = get_most_repariable_part();
                if( most_repairable ) {
                    move_cursor( most_repairable->mount.y + ddy, -( most_repairable->mount.x + ddx ) );
                    return false;
                } else {
                    msg = _( "There are no damaged parts on this vehicle." );
                    return false;
                }
            }

        case MOVING_VEHICLE:
            msg = _( "You can't repair stuff while driving." );
            return false;

        default: break;
    }

    set_title( _( "Choose a part here to repair:" ) );

    int pos = 0;
    while (true) {
        vehicle_part &pt = veh->parts[parts_here[need_repair[pos]]];
        const vpart_info &vp = pt.info();

        std::ostringstream msg;

        bool ok;
        if( pt.is_broken() ) {
            ok = format_reqs( msg, vp.install_requirements(), vp.install_skills, vp.install_time( g->u ) );
        } else {
            if( !vp.repair_requirements().is_empty() ) {
                ok = format_reqs( msg, vp.repair_requirements() * pt.base.damage(), vp.repair_skills,
                                  vp.repair_time( g->u ) * double( pt.base.damage() ) / pt.base.max_damage() );
            } else {
                msg << "<color_light_red>" << _( "This part cannot be repaired" ) << "</color>";
                ok = false;
            }
        }

        werase (w_msg);
        fold_and_print( w_msg, 0, 1, getmaxx( w_msg ) - 2, c_light_gray, msg.str() );
        wrefresh (w_msg);

        werase( w_parts );
        veh->print_part_list( w_parts, 0, getmaxy( w_parts ) - 1, getmaxx( w_parts ), cpart,
                              need_repair[pos] );
        wrefresh( w_parts );

        const std::string action = main_context.handle_input();
        if( ( action == "REPAIR" || action == "CONFIRM" ) && ok ) {
            sel_vehicle_part = &pt;
            sel_vpart_info = &vp;
            sel_cmd = 'r';
            break;

        } else if( action == "QUIT" ) {
            werase( w_parts );
            veh->print_part_list( w_parts, 0, getmaxy( w_parts ) - 1, getmaxx( w_parts ), cpart, -1 );
            wrefresh( w_parts );
            werase( w_msg );
            wrefresh( w_msg );
            break;

        } else {
            move_in_list(pos, action, need_repair.size());
        }
    }

    return false;
}

bool veh_interact::do_mend( std::string &msg )
{
    switch( cant_do( 'm' ) ) {
        case LOW_MORALE:
            msg = _( "Your morale is too low to mend..." );
            return false;

        case INVALID_TARGET:
            msg = _( "No faulty parts require mending." );
            return false;

        case MOVING_VEHICLE:
            msg = _( "You can't mend stuff while driving." );
            return false;

        default: break;
    }

    set_title( _( "Choose a part here to mend:" ) );

    auto sel = []( const vehicle_part &pt ) { return !pt.faults().empty(); };

    auto act = [&]( const vehicle_part &pt ) {
        g->u.mend_item( veh->part_base( veh->index_of_part( &pt ) ) );
        sel_cmd = 'q';
        return true; // force redraw
    };

    return overview( sel, act );
}

bool veh_interact::do_refill( std::string &msg )
{
    if( cant_do( 'f' ) ) {
        msg = _( "No parts can currently be refilled" );
        return false;
    }

    set_title( _( "Select part to refill:" ) );

    auto act = [&]( const vehicle_part &pt ) {
        auto validate = [&]( const item &obj ) {
            if( pt.is_tank() ) {
                // cannot refill using active liquids (those that rot) due to #18570
                if( obj.is_watertight_container() && !obj.contents.empty() && !obj.contents.front().active ) {
                    return pt.can_reload( obj.contents.front().typeId() );
                }
            } else if( pt.is_reactor() ) {
                return pt.can_reload( obj.typeId() );
            }
            return false;
        };

        target = g->inv_map_splice( validate, string_format( _( "Refill %s" ), pt.name().c_str() ), 1 );
        if( target ) {
            sel_vehicle_part = &pt;
            sel_vpart_info = &pt.info();
            sel_cmd = 'f';
        }

        return true; // force redraw
    };

    return overview( can_refill, act );
}

bool veh_interact::overview( std::function<bool( const vehicle_part &pt )> enable,
                             std::function<bool( vehicle_part &pt )> action )
{
    struct part_option {
        part_option( const std::string &key, vehicle_part *part, char hotkey,
                     std::function<void( const vehicle_part &pt, const catacurses::window &w, int y )> details ) :
            key( key ), part( part ), hotkey( hotkey ), details( details ) {}

        part_option( const std::string &key, vehicle_part *part, char hotkey,
                     std::function<void( const vehicle_part &pt, const catacurses::window &w, int y )> details,
                     std::function<void( const vehicle_part &pt )> message ) :
            key( key ), part( part ), hotkey( hotkey ), details( details ), message( message ) {}

        std::string key;
        vehicle_part *part;

        /** Can @param action be run for this entry? */
        char hotkey;

        /** Writes any extra details for this entry */
        std::function<void( const vehicle_part &pt, const catacurses::window &w, int y )> details;

        /** Writes to message window when part is selected */
        std::function<void( const vehicle_part &pt )> message;
    };

    std::vector<part_option> opts;

    std::map<std::string, std::function<void( const catacurses::window &, int )>> headers;

    headers["ENGINE"] = []( const catacurses::window &w, int y ) {
        trim_and_print( w, y, 1, getmaxx( w ) - 2, c_light_gray, _( "Engines" ) );
        right_print   ( w, y, 1, c_light_gray, _( "Fuel     Use" ) );
    };
    headers["TANK"] = []( const catacurses::window &w, int y ) {
        trim_and_print( w, y, 1, getmaxx( w ) - 2, c_light_gray, _( "Tanks" ) );
        right_print   ( w, y, 1, c_light_gray, _( "Contents     Qty" ) );
    };
    headers["BATTERY"] = []( const catacurses::window &w, int y ) {
        trim_and_print( w, y, 1, getmaxx( w ) - 2, c_light_gray, _( "Batteries" ) );
        right_print   ( w, y, 1, c_light_gray, _( "Capacity  Status" ) );
    };
    headers["REACTOR"] = []( const catacurses::window &w, int y ) {
        trim_and_print( w, y, 1, getmaxx( w ) - 2, c_light_gray, _( "Reactors" ) );
        right_print   ( w, y, 1, c_light_gray, _( "Contents     Qty" ) );
    };
    headers["TURRET"] = []( const catacurses::window &w, int y ) {
        trim_and_print( w, y, 1, getmaxx( w ) - 2, c_light_gray, _( "Turrets" ) );
        right_print   ( w, y, 1, c_light_gray, _( "Ammo     Qty" ) );
    };
    headers["SEAT"] = []( const catacurses::window &w, int y ) {
        trim_and_print( w, y, 1, getmaxx( w ) - 2, c_light_gray, _( "Seats" ) );
        right_print   ( w, y, 1, c_light_gray, _( "Who" ) );
    };

    char hotkey = 'a';

    for( auto &pt : veh->parts ) {
        if( pt.is_engine() && !pt.is_broken() ) {
            // if tank contains something then display the contents in milliliters
            auto details = []( const vehicle_part &pt, const catacurses::window &w, int y ) {
                right_print( w, y, 1, item::find_type( pt.ammo_current() )->color,
                             string_format( "%s     <color_light_gray>%3s</color>",
                             pt.ammo_current() != "null" ? item::nname( pt.ammo_current() ).c_str() : "",
                             pt.enabled ? _( "Yes" ) : _( "No" ) ) );
            };

            // display engine faults (if any)
            auto msg = [&]( const vehicle_part &pt ) {
                werase( w_msg );
                int y = 0;
                for( const auto& e : pt.faults() ) {
                    y += fold_and_print( w_msg, y, 1, getmaxx( w_msg ) - 2, c_red,
                                         _( "Faulty %1$s" ), e.obj().name().c_str() );
                    y += fold_and_print( w_msg, y, 3, getmaxx( w_msg ) - 4, c_light_gray, e.obj().description() );
                    y++;
                }
                wrefresh( w_msg );
            };
            opts.emplace_back( "ENGINE", &pt, action && enable && enable( pt ) ? hotkey++ : '\0', details, msg );
        }
    }

    for( auto &pt : veh->parts ) {
        if( pt.is_tank() && !pt.is_broken() ) {
            auto details = []( const vehicle_part &pt, const catacurses::window &w, int y ) {
                if( pt.ammo_current() != "null" ) {
                    auto stack = units::legacy_volume_factor / item::find_type( pt.ammo_current() )->stack_size;
                    right_print( w, y, 1, item::find_type( pt.ammo_current() )->color,
                                 string_format( "%s  %5.1fL", item::nname( pt.ammo_current() ),
                                 round_up( to_liter( pt.ammo_remaining() * stack ), 1 ) ) );
                }
            };
            opts.emplace_back( "TANK", &pt, action && enable && enable( pt ) ? hotkey++ : '\0', details );
        }
    }

    for( auto &pt : veh->parts ) {
        if( pt.is_battery() && !pt.is_broken() ) {
            // always display total battery capacity and percentage charge
            auto details = []( const vehicle_part &pt, const catacurses::window &w, int y ) {
                int pct = ( double( pt.ammo_remaining() ) / pt.ammo_capacity() ) * 100;
                right_print( w, y, 1, item::find_type( pt.ammo_current() )->color,
                             string_format( "%i    %3i%%", pt.ammo_capacity(), pct ) );
            };
           opts.emplace_back( "BATTERY", &pt, action && enable && enable( pt ) ? hotkey++ : '\0', details );
        }
    }

    auto details_ammo = []( const vehicle_part &pt, const catacurses::window &w, int y ) {
        if( pt.ammo_remaining() ) {
            right_print( w, y, 1, item::find_type( pt.ammo_current() )->color,
                         string_format( "%s   %5i", item::nname( pt.ammo_current() ).c_str(), pt.ammo_remaining() ) );
        }
    };

    for( auto &pt : veh->parts ) {
        if( pt.is_reactor() && !pt.is_broken() ) {
            opts.emplace_back( "REACTOR", &pt, action && enable && enable( pt ) ? hotkey++ : '\0', details_ammo );
        }
    }

    for( auto &pt : veh->parts ) {
        if( pt.is_turret() && !pt.is_broken() ) {
            opts.emplace_back( "TURRET", &pt, action && enable && enable( pt ) ? hotkey++ : '\0', details_ammo );
        }
    }

    for( auto &pt : veh->parts ) {
        auto details = []( const vehicle_part &pt, const catacurses::window &w, int y ) {
            const npc *who = pt.crew();
            if( who ) {
                right_print( w, y, 1, pt.passenger_id == who->getID() ? c_green : c_light_gray, who->name );
            }
        };
        if( pt.is_seat() && !pt.is_broken() ) {
            opts.emplace_back( "SEAT", &pt, action && enable && enable( pt ) ? hotkey++ : '\0', details );
        }
    }

    int pos = -1;
    if( enable && action ) {
        do {
            if( ++pos >= int( opts.size() ) ) {
                pos = -1;
                break; // nothing could be selected
            }
        } while( !opts[pos].hotkey );
    }

    bool redraw = false;
    while( true ) {
        werase( w_list );
        std::string last;
        int y = 0;
        for( int idx = 0; idx != int( opts.size() ); ++idx ) {
            const auto &pt = *opts[idx].part;

            // if this is a new section print a header row
            if( last != opts[idx].key ) {
                y += last.empty() ? 0 : 1;
                headers[opts[idx].key]( w_list, y );
                y += 2;
                last = opts[idx].key;
            }

            bool highlighted = false;
            // No action means no selecting, just highlight relevant ones
            if( pos < 0 && enable && !action ) {
                highlighted = enable( pt );
            } else if( pos == idx ) {
                highlighted = true;
            }

            // print part name
            nc_color col = opts[idx].hotkey ? c_white : c_dark_gray;
            trim_and_print( w_list, y, 1, getmaxx( w_list ) - 1,
                            highlighted ? hilite( col ) : col,
                            "<color_dark_gray>%c </color>%s",
                            opts[idx].hotkey ? opts[idx].hotkey : ' ', pt.name().c_str() );

            // print extra columns (if any)
            opts[idx].details( pt, w_list, y );
            y++;
        }
        wrefresh( w_list );

        if( !std::any_of( opts.begin(), opts.end(), []( const part_option & e ) {
        return e.hotkey;
    } ) ) {
            return false; // nothing is selectable
        }

        move_cursor( opts[pos].part->mount.y + ddy, -( opts[pos].part->mount.x + ddx ) );

        if( opts[pos].message ) {
            opts[pos].message( *opts[pos].part );
        }

        const std::string input = main_context.handle_input();
        if( input == "CONFIRM" && opts[pos].hotkey ) {
            redraw = action( *opts[pos].part );
            break;

        } else if( input == "QUIT" ) {
            break;

        } else if( input == "UP" ) {
            do {
                if( --pos < 0 ) {
                    pos = opts.size() - 1;
                }
            } while( !opts[pos].hotkey );

        } else if( input == "DOWN" ) {
            do {
                if( ++pos >= int( opts.size() ) ) {
                    pos = 0;
                }
            } while( !opts[pos].hotkey );

        } else {
            // did we try and activate a hotkey option?
            char hotkey = main_context.get_raw_input().get_first_input();
            if( hotkey ) {
                auto iter = std::find_if( opts.begin(), opts.end(), [&hotkey]( const part_option & e ) {
                    return e.hotkey == hotkey;
                } );
                if( iter != opts.end() ) {
                    action( *iter->part );
                    break;
                }
            }
        }
    }

    werase( w_list );
    wrefresh( w_list );
    return redraw;
}


vehicle_part *veh_interact::get_most_damaged_part() const
{
    auto part_damage_comparison = []( const vehicle_part &a, const vehicle_part &b )
    {
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

bool veh_interact::can_remove_part( int idx ) {
    sel_vehicle_part = &veh->parts[idx];
    sel_vpart_info = &sel_vehicle_part->info();

    const auto reqs = sel_vpart_info->removal_requirements();

    std::ostringstream msg;
    bool ok = format_reqs( msg, reqs, sel_vpart_info->removal_skills, sel_vpart_info->removal_time( g->u ) );

    msg << _( "<color_white>Additional requirements:</color>\n" );

    int lvl = 0;
    int str = 0;
    quality_id qual;
    bool use_aid = false;
    bool use_str = false;
    item base( sel_vpart_info->item );
    if( base.is_wheel() ) {
        qual = JACK;
        lvl = jack_qality( *veh );
        str = veh->lift_strength();
        use_aid = max_jack >= lvl;
        use_str = g->u.can_lift( *veh );
    } else {
        qual = LIFT;
        lvl = ceil( units::quantity<double, units::mass::unit_type>( base.weight() ) / TOOL_LIFT_FACTOR );
        str = base.lift_strength();
        use_aid = max_lift >= lvl;
        use_str = g->u.can_lift( base );
    }

    if( !( use_aid || use_str ) ) {
        ok = false;
    }
    msg << string_format( _( "> <color_%1$s>1 tool with %2$s %3$i</color> <color_white>OR</color> <color_%4$s>strength %5$i</color>" ),
                          status_color( use_aid ), qual.obj().name.c_str(), lvl,
                          status_color( use_str ), str ) << "\n";

    if( !veh->can_unmount( idx ) ) {
        msg << string_format( _( "> <color_%1$s>%2$s</color>" ), status_color( false ), _( "Remove attached parts first" ) ) << "\n";
        ok = false;
    }

    werase( w_msg );
    fold_and_print( w_msg, 0, 1, getmaxx( w_msg ) - 2, c_light_gray, msg.str() );
    wrefresh( w_msg );
    return ok || g->u.has_trait( trait_DEBUG_HS );
}

bool veh_interact::do_remove( std::string &msg )
{
    switch( cant_do( 'o' ) ) {
        case LOW_MORALE:
            msg = _( "Your morale is too low to construct..." );
            return false;

        case INVALID_TARGET:
            msg = _( "No parts here." );
            return false;

        case NOT_FREE:
            msg = _( "You cannot remove that part while something is attached to it." );
            return false;

        case MOVING_VEHICLE:
            msg = _( "Better not remove something while driving." );
            return false;

        default: break;
    }

    set_title( _( "Choose a part here to remove:" ) );

    int pos = 0;
    for( size_t i = 0; i < parts_here.size(); i++ ) {
        if( can_remove_part( parts_here[ i ] ) ) {
            pos = i;
            break;
        }
    }
    while (true) {
        //redraw list of parts
        werase (w_parts);
        veh->print_part_list( w_parts, 0, getmaxy( w_parts ) - 1, getmaxx( w_parts ), cpart, pos );
        wrefresh (w_parts);
        int part = parts_here[ pos ];

        bool can_remove = can_remove_part( part );

        auto sel = [&]( const vehicle_part &pt ) { return &pt == &veh->parts[part]; };
        overview( sel );

        //read input
        const std::string action = main_context.handle_input();
        if (can_remove && (action == "REMOVE" || action == "CONFIRM")) {
            sel_cmd = 'o';
            break;
        } else if( action == "QUIT" ) {
            werase( w_parts );
            veh->print_part_list( w_parts, 0, getmaxy( w_parts ) - 1, getmaxx( w_parts ), cpart, -1 );
            wrefresh( w_parts );
            werase( w_msg );
            wrefresh( w_msg );
            break;
        } else {
            move_in_list(pos, action, parts_here.size());
        }
    }

    return false;
}

bool veh_interact::do_siphon( std::string &msg )
{
    switch( cant_do( 's' ) ) {
        case INVALID_TARGET:
            msg = _( "The vehicle has no liquid fuel left to siphon." );
            return false;

        case LACK_TOOLS:
            msg = _( "You need a <color_red>hose</color> to siphon liquid fuel." );
            return false;

        case MOVING_VEHICLE:
            msg = _( "You can't siphon from a moving vehicle." );
            return false;

        default: break;
    }

    act_vehicle_siphon( veh );
    return true; // force redraw
}

bool veh_interact::do_tirechange( std::string &msg )
{
    switch( cant_do( 'c' ) ) {
        case INVALID_TARGET:
            msg = _( "There is no wheel to change here." );
            return false;

        case LACK_TOOLS:
            msg = string_format( _( "To change a wheel you need a <color_%1$s>wrench</color>, a <color_%2$s>wheel</color>, and either "
                                    "<color_%3$s>lifting equipment</color> or <color_%4$s>%5$d</color> strength." ),
                                 status_color( has_wrench ), status_color( has_wheel ), status_color( has_jack ),
                                 status_color( g->u.can_lift( *veh ) ), veh->lift_strength() );
            return false;

        case MOVING_VEHICLE:
            msg = _( "Who is driving while you work?" );
            return false;

        default: break;
    }

    set_title( _( "Choose wheel to use as replacement:" ) );

    int pos = 0;
    while (true) {
        sel_vpart_info = wheel_types[pos];
        bool is_wheel = sel_vpart_info->has_flag("WHEEL");
        display_list (pos, wheel_types);
        bool has_comps = crafting_inv.has_components( sel_vpart_info->item, 1 );
        werase (w_msg);
        wrefresh (w_msg);

        const std::string action = main_context.handle_input();
        if( ( action == "TIRE_CHANGE" || action == "CONFIRM" ) &&
            is_wheel && has_comps && has_wrench && ( g->u.can_lift( *veh ) || has_jack ) ) {
            sel_cmd = 'c';
            break;

        } else if (action == "QUIT") {
            werase (w_list);
            wrefresh (w_list);
            werase (w_msg);
            break;

        } else {
            move_in_list(pos, action, wheel_types.size());
        }
    }

    return false;
}

bool veh_interact::do_assign_crew( std::string &msg )
{
    if( cant_do( 'w' ) != CAN_DO ) {
        msg = _( "Need at least one seat and an ally to assign crew members." );
        return false;
    }

    set_title( _( "Assign crew positions:" ) );

    auto sel = []( const vehicle_part &pt ) { return pt.is_seat(); };

    auto act = [&]( vehicle_part &pt ) {
        uimenu menu;
        menu.text = _( "Select crew member" );
        menu.return_invalid = true;

        if( pt.crew() ) {
            menu.addentry( 0, true, 'c', _( "Clear assignment" ) );
        }

        for( const npc *e : g->allies() ) {
            menu.addentry( e->getID(), true, -1, e->name );
        }

        menu.query();
        if( menu.ret == 0 ) {
            pt.unset_crew();
        } else if( menu > 0 ) {
            const auto &who = *g->critter_by_id<npc>( menu.ret );
            veh->assign_seat( pt, who );
        }

        return true; // force redraw
    };

    return overview( sel, act );
}

bool veh_interact::do_rename( std::string & )
{
    std::string name = string_input_popup()
                       .title( _( "Enter new vehicle name:" ) )
                       .width( 20 )
                       .query_string();
    if( name.length() > 0 ) {
        veh->name = name;
        if( veh->tracking_on ) {
            overmap_buffer.remove_vehicle( veh );
            // Add the vehicle again, this time with the new name
            overmap_buffer.add_vehicle( veh );
        }
    }

    return true;
}

bool veh_interact::do_relabel( std::string &msg )
{
    if( cant_do( 'a' ) == INVALID_TARGET ) {
        msg = _( "There are no parts here to label." );
        return false;
    }

    const vpart_position vp( *veh, cpart );
    std::string text = string_input_popup()
                       .title( _( "New label:" ) )
                       .width( 20 )
                       .text( vp.get_label().value_or( "" ) )
                       .query_string();
    vp.set_label( text ); // empty input removes the label
    // refresh w_disp & w_part windows:
    move_cursor( 0, 0 );

    return false;
}

/**
 * Returns the first part on the vehicle at the given position.
 * @param dx The x-coordinate, relative to the viewport's 0-point (?)
 * @param dy The y-coordinate, relative to the viewport's 0-point (?)
 * @return The first vehicle part at the specified coordinates.
 */
int veh_interact::part_at( int dx, int dy )
{
    int vdx = -ddx - dy;
    int vdy = dx - ddy;
    return veh->part_displayed_at( vdx, vdy );
}

/**
 * Checks to see if you can potentially install this part at current position.
 * Affects coloring in display_list() and is also used to
 * sort can_mount so potentially installable parts come first.
 */
bool veh_interact::can_potentially_install(const vpart_info &vpart)
{
    return g->u.has_trait( trait_DEBUG_HS ) || vpart.install_requirements().can_make_with_inventory( crafting_inv );
}

/**
 * Moves the cursor on the vehicle editing window.
 * @param dx How far to move the cursor on the x-axis.
 * @param dy How far to move the cursor on the y-axis.
 * @param dstart_at How far to change the start position for vehicle part descriptions
 */
void veh_interact::move_cursor( int dx, int dy, int dstart_at )
{
    const int hw = getmaxx( w_disp ) / 2;
    const int hh = getmaxy( w_disp ) / 2;

    ddx += dy;
    ddy -= dx;
    if( dx || dy ) {
        start_limit = 0;
    } else {
        start_at += dstart_at;
    }

    display_veh();
    // Update the current active component index to the new position.
    cpart = part_at( 0, 0 );
    int vdx = -ddx;
    int vdy = -ddy;
    point q = veh->coord_translate( point( vdx, vdy ) );
    tripoint vehp = veh->global_pos3() + q;
    const bool has_critter = g->critter_at( vehp );
    bool obstruct = g->m.impassable_ter_furn( vehp );
    const optional_vpart_position ovp = g->m.veh_at( vehp );
    if( ovp && &ovp->vehicle() != veh ) {
        obstruct = true;
    }
    nc_color col = cpart >= 0 ? veh->part_color( cpart ) : c_black;
    long sym = cpart >= 0 ? veh->part_sym( cpart ) : ' ';
    mvwputch( w_disp, hh, hw, obstruct ? red_background( col ) : hilite( col ),
              special_symbol( sym ) );
    wrefresh( w_disp );
    werase( w_parts );
    veh->print_part_list( w_parts, 0, getmaxy( w_parts ) - 1, getmaxx( w_parts ), cpart, -1 );
    wrefresh( w_parts );

    werase( w_msg );
    veh->print_vparts_descs( w_msg, getmaxy( w_msg ), getmaxx( w_msg ), cpart, start_at, start_limit );
    wrefresh( w_msg );

    can_mount.clear();
    if( !obstruct ) {
        int divider_index = 0;
        for( const auto &e : vpart_info::all() ) {
            const vpart_info &vp = e.second;
            if( has_critter && vp.has_flag( VPFLAG_OBSTACLE ) ) {
                continue;
            }
            if( veh->can_mount( vdx, vdy, vp.get_id() ) ) {
                if( vp.get_id() != vpart_shapes[ vp.name() + vp.item ][ 0 ]->get_id() ) {
                    continue;    // only add first shape to install list
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
    wheel = NULL;
    if( cpart >= 0 ) {
        parts_here = veh->parts_at_relative( veh->parts[cpart].mount.x, veh->parts[cpart].mount.y );
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
}

void veh_interact::display_grid()
{
    // border window
    catacurses::window w_border = catacurses::newwin( TERMY, TERMX, 0, 0 );
    draw_border( w_border );

    // match grid lines
    const int y_mode = getmaxy( w_mode ) + 1;
    mvwputch( w_border, y_mode, 0, BORDER_COLOR, LINE_XXXO );         // |-
    mvwputch( w_border, y_mode, TERMX - 1, BORDER_COLOR, LINE_XOXX ); // -|
    const int y_list = getbegy( w_list ) + getmaxy( w_list );
    mvwputch( w_border, y_list, 0, BORDER_COLOR, LINE_XXXO );         // |-
    mvwputch( w_border, y_list, TERMX - 1, BORDER_COLOR, LINE_XOXX ); // -|
    wrefresh( w_border );
    w_border = catacurses::window(); //@todo: move code using w_border into a separate scope

    const int grid_w = getmaxx(w_grid);

    // Two lines dividing the three middle sections.
    for (int i = 1 + getmaxy( w_mode ); i < (1 + getmaxy( w_mode ) + page_size ); ++i) {
        mvwputch(w_grid, i, getmaxx( w_disp ), BORDER_COLOR, LINE_XOXO); // |
        mvwputch(w_grid, i, getmaxx( w_disp ) + 1 + getmaxx( w_list) , BORDER_COLOR, LINE_XOXO); // |
    }
    // Two lines dividing the vertical menu sections.
    for (int i = 0; i < grid_w; ++i) {
        mvwputch( w_grid, getmaxy( w_mode ), i, BORDER_COLOR, LINE_OXOX ); // -
        mvwputch( w_grid, getmaxy( w_mode ) + 1 + page_size, i, BORDER_COLOR, LINE_OXOX ); // -
    }
    // Fix up the line intersections.
    mvwputch(w_grid, getmaxy( w_mode ), getmaxx( w_disp ), BORDER_COLOR, LINE_OXXX);
    mvwputch(w_grid, getmaxy( w_mode ) + 1 + page_size, getmaxx( w_disp ), BORDER_COLOR, LINE_XXOX); // _|_
    mvwputch(w_grid, getmaxy( w_mode ), getmaxx( w_disp ) + 1 + getmaxx( w_list ), BORDER_COLOR, LINE_OXXX);
    mvwputch(w_grid, getmaxy( w_mode ) + 1 + page_size, getmaxx( w_disp ) + 1 + getmaxx( w_list ), BORDER_COLOR, LINE_XXOX); // _|_

    wrefresh(w_grid);
}

/**
 * Draws the viewport with the vehicle.
 */
void veh_interact::display_veh ()
{
    werase(w_disp);
    const int hw = getmaxx(w_disp) / 2;
    const int hh = getmaxy(w_disp) / 2;

    if (debug_mode) {
        // show CoM, pivot in debug mode

        const point &pivot = veh->pivot_point();
        const point &com = veh->local_center_of_mass();

        mvwprintz(w_disp, 0, 0, c_green, "CoM   %d,%d", com.x, com.y);
        mvwprintz(w_disp, 1, 0, c_red,   "Pivot %d,%d", pivot.x, pivot.y);

        int com_sx = com.y + ddy + hw;
        int com_sy = -(com.x + ddx) + hh;
        int pivot_sx = pivot.y + ddy + hw;
        int pivot_sy = -(pivot.x + ddx) + hh;

        for (int x = 0; x < getmaxx(w_disp); ++x) {
            if (x <= com_sx) {
                mvwputch (w_disp, com_sy, x, c_green, LINE_OXOX);
            }

            if (x >= pivot_sx) {
                mvwputch (w_disp, pivot_sy, x, c_red, LINE_OXOX);
            }
        }

        for (int y = 0; y < getmaxy(w_disp); ++y) {
            if (y <= com_sy) {
                mvwputch (w_disp, y, com_sx, c_green, LINE_XOXO);
            }

            if (y >= pivot_sy) {
                mvwputch (w_disp, y, pivot_sx, c_red, LINE_XOXO);
            }
        }
    }

    //Iterate over structural parts so we only hit each square once
    std::vector<int> structural_parts = veh->all_parts_at_location("structure");
    for( auto &structural_part : structural_parts ) {
        const int p = structural_part;
        long sym = veh->part_sym (p);
        nc_color col = veh->part_color (p);

        int x =   veh->parts[p].mount.y + ddy;
        int y = -(veh->parts[p].mount.x + ddx);

        if (x == 0 && y == 0) {
            col = hilite(col);
            cpart = p;
        }
        mvwputch (w_disp, hh + y, hw + x, col, special_symbol(sym));
    }
    wrefresh (w_disp);
}

static std::string wheel_state_description( const vehicle &veh )
{
    bool is_boat = !veh.all_parts_with_feature(VPFLAG_FLOATS).empty();
    bool is_land = !veh.all_parts_with_feature(VPFLAG_WHEEL).empty();

    bool suf_land = veh.sufficient_wheel_config( false );
    bool bal_land = veh.balanced_wheel_config( false );

    bool suf_boat = veh.sufficient_wheel_config( true );
    bool bal_boat = veh.balanced_wheel_config( true );
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
        boat_status = _( "<color_light_red>leaks</color>" );
    } else if( !bal_boat ) {
        boat_status = _( "<color_light_red>unbalanced</color>" );
    } else {
        boat_status = _( "<color_blue>swims</color>" );
    }

    if( is_boat && is_land ) {
        return string_format( _( "Wheels/boat: %s/%s" ), wheel_status.c_str(), boat_status.c_str() );
    }

    if( is_boat ) {
        return string_format( _( "Boat: %s" ), boat_status.c_str() );
    }

    return string_format( _( "Wheels: %s" ), wheel_status.c_str() );
}

/**
 * Displays the vehicle's stats at the bottom of the window.
 */
void veh_interact::display_stats()
{
    werase(w_stats);

    const int extraw = ((TERMX - FULL_SCREEN_WIDTH) / 4) * 2; // see exec()
    int x[18], y[18], w[18]; // 3 columns * 6 rows = 18 slots max

    std::vector<int> cargo_parts = veh->all_parts_with_feature("CARGO");
    units::volume total_cargo = 0;
    units::volume free_cargo = 0;
    for( const auto &p : cargo_parts ) {
        total_cargo += veh->max_volume(p);
        free_cargo += veh->free_volume(p);
    }

    const int second_column = 33 + (extraw / 4);
    const int third_column = 65 + (extraw / 2);
    for (int i = 0; i < 18; i++) {
        if (i < 6) { // First column
            x[i] = 1;
            y[i] = i;
            w[i] = second_column - 2;
        } else if (i < 13) { // Second column
            x[i] = second_column;
            y[i] = i - 6;
            w[i] = third_column - second_column - 1;
        } else { // Third column
            x[i] = third_column;
            y[i] = i - 13;
            w[i] = extraw - third_column - 2;
        }
    }

    fold_and_print( w_stats, y[0], x[0], w[0], c_light_gray,
                    _( "Safe/Top Speed: <color_light_green>%3d</color>/<color_light_red>%3d</color> %s" ),
                    int( convert_velocity( veh->safe_velocity( false ), VU_VEHICLE ) ),
                    int( convert_velocity( veh->max_velocity( false ), VU_VEHICLE ) ),
                    velocity_units( VU_VEHICLE ) );
    //TODO: extract accelerations units to its own function

    fold_and_print( w_stats, y[1], x[1], w[1], c_light_gray,
                    //~ /t means per turn
                    _( "Acceleration: <color_light_blue>%3d</color> %s/t" ),
                    int( convert_velocity( veh->acceleration( false ), VU_VEHICLE ) ),
                    velocity_units( VU_VEHICLE ) );
    fold_and_print( w_stats, y[2], x[2], w[2], c_light_gray,
                    _( "Mass: <color_light_blue>%5.0f</color> %s" ),
                    convert_weight( veh->total_mass() ), weight_units() );
    fold_and_print( w_stats, y[3], x[3], w[3], c_light_gray,
                    _( "Cargo Volume: <color_light_gray>%s/%s</color> %s" ),
                    format_volume( total_cargo - free_cargo ).c_str(),
                    format_volume( total_cargo ).c_str(),
                    volume_units_abbr() );
    // Write the overall damage
    mvwprintz(w_stats, y[4], x[4], c_light_gray, _("Status:"));
    x[4] += utf8_width(_("Status:")) + 1;
    fold_and_print(w_stats, y[4], x[4], w[4], totalDurabilityColor, totalDurabilityText);

    fold_and_print( w_stats, y[5], x[5], w[5], c_light_gray, wheel_state_description( *veh ).c_str() );


    //This lambda handles printing parts in the "Most damaged" and "Needs repair" cases
    //for the veh_interact ui
    auto print_part = [&]( const char * str, int slot, vehicle_part *pt )
    {
        mvwprintz( w_stats, y[slot], x[slot], c_light_gray, str);
        auto iw = utf8_width( str ) + 1;
        x[slot] += iw;
        w[slot] -= iw;

        const auto hoff = fold_and_print( w_stats, y[slot], x[slot], w[slot],
                                          pt->is_broken() ? c_dark_gray : pt->base.damage_color(), pt->name() );

        // If fold_and_print did write on the next line(s), shift the following entries,
        // hoff == 1 is already implied and expected - one line is consumed at least.
        for( size_t i = slot + 1; i < sizeof( y ) / sizeof( y[0] ); ++i ) {
            y[i] += hoff - 1;
        }
    };

    vehicle_part *mostDamagedPart = get_most_damaged_part();
    vehicle_part *most_repairable = get_most_repariable_part();

    // Write the most damaged part
    if( mostDamagedPart ) {
        char const *damaged_header = mostDamagedPart == most_repairable ?
                                            _( "Most damaged:" ) : _( "Most damaged (can't repair):" );
        print_part( damaged_header, 6, mostDamagedPart );
    }
    // Write the part that needs repair the most.
    if( most_repairable && most_repairable != mostDamagedPart ) {
        char const * needsRepair = _( "Needs repair:" );
        print_part( needsRepair, 7, most_repairable );
    }

    bool is_boat = !veh->all_parts_with_feature(VPFLAG_FLOATS).empty();

    fold_and_print(w_stats, y[8], x[8], w[8], c_light_gray,
                   _("K aerodynamics: <color_light_blue>%3d</color>%%"),
                   int(veh->k_aerodynamics() * 100));
    fold_and_print(w_stats, y[9], x[9], w[9], c_light_gray,
                   _("K friction:     <color_light_blue>%3d</color>%%"),
                   int(veh->k_friction() * 100));
    fold_and_print(w_stats, y[10], x[10], w[10], c_light_gray,
                   _("K mass:         <color_light_blue>%3d</color>%%"),
                   int(veh->k_mass() * 100));
    fold_and_print( w_stats, y[11], x[11], w[11], c_light_gray,
                    _("Offroad:        <color_light_blue>%3d</color>%%"),
                    int( veh->k_traction( veh->wheel_area( is_boat ) * 0.5f ) * 100 ) );

    // Print fuel percentage & type name only if it fits in the window, 10 is width of "E...F 100%"
    veh->print_fuel_indicators (w_stats, y[13], x[13], fuel_index, true,
                               ( x[ 13 ] + 10 < getmaxx( w_stats ) ),
                               ( x[ 13 ] + 10 < getmaxx( w_stats ) ) );

    wrefresh(w_stats);
}

void veh_interact::display_name()
{
    werase(w_name);
    mvwprintz(w_name, 0, 1, c_light_gray, _("Name: "));
    mvwprintz(w_name, 0, 1 + utf8_width(_("Name: ")), c_light_green, veh->name.c_str());
    wrefresh(w_name);
}

/**
 * Prints the list of usable commands, and highlights the hotkeys used to activate them.
 */
void veh_interact::display_mode()
{
    werase (w_mode);

    size_t esc_pos = display_esc(w_mode);

    // broken indentation preserved to avoid breaking git history for large number of lines
        const std::array<std::string, 10> actions = { {
            { _("<i>nstall") },
            { _("<r>epair") },
            { _("<m>end" ) },
            { _("re<f>ill") },
            { _("rem<o>ve") },
            { _("<s>iphon") },
            { _("<c>hange tire") },
            { _("cre<w>") },
            { _("r<e>name") },
            { _("l<a>bel") },
        } };

        const std::array<bool, std::tuple_size<decltype(actions)>::value> enabled = { {
            !cant_do('i'),
            !cant_do('r'),
            !cant_do('m'),
            !cant_do('f'),
            !cant_do('o'),
            !cant_do('s'),
            !cant_do('c'),
            !cant_do('w'),
            true,          // 'rename' is always available
            !cant_do('a'),
        } };

        int pos[std::tuple_size<decltype(actions)>::value + 1];
        pos[0] = 1;
        for (size_t i = 0; i < actions.size(); i++) {
            pos[i + 1] = pos[i] + utf8_width(actions[i]) - 2;
        }
        int spacing = int((esc_pos - 1 - pos[actions.size()]) / actions.size());
        int shift = int((esc_pos - pos[actions.size()] - spacing * (actions.size() - 1)) / 2) - 1;
        for (size_t i = 0; i < actions.size(); i++) {
            shortcut_print(w_mode, 0, pos[i] + spacing * i + shift,
                           enabled[i] ? c_light_gray : c_dark_gray, enabled[i] ? c_light_green : c_green,
                           actions[i]);
        }

    wrefresh (w_mode);
}

size_t veh_interact::display_esc( const catacurses::window &win )
{
    std::string backstr = _("<ESC>-back");
    size_t pos = getmaxx(win) - utf8_width(backstr) + 2;    // right text align
    shortcut_print(win, 0, pos, c_light_gray, c_light_green, backstr);
    wrefresh(win);
    return pos;
}

/**
 * Draws the list of parts that can be mounted in the selected square. Used
 * when installing new parts or changing tires.
 * @param pos The current cursor position in the list.
 * @param list The list to display parts from.
 * @param header Number of lines occupied by the list header
 */
void veh_interact::display_list(size_t pos, const std::vector<const vpart_info*> &list, const int header)
{
    werase (w_list);
    int lines_per_page = page_size - header;
    size_t page = pos / lines_per_page;
    for (size_t i = page * lines_per_page; i < (page + 1) * lines_per_page && i < list.size(); i++) {
        const vpart_info &info = *list[i];
        int y = i - page * lines_per_page + header;
        mvwputch( w_list, y, 1, info.color, special_symbol( info.sym ) );
        nc_color col = can_potentially_install( info ) ? c_white : c_dark_gray;
        trim_and_print( w_list, y, 3, getmaxx( w_list ) - 3, pos == i ? hilite( col ) : col,
                        info.name().c_str() );
    }
    wrefresh (w_list);
}

/**
 * Used when installing parts.
 * Opens up w_details containing info for part currently selected in w_list.
 */
void veh_interact::display_details( const vpart_info *part )
{

    if( !w_details ) { // create details window first if required

        // covers right part of w_name and w_stats in vertical/hybrid
        const int details_y = getbegy(w_name);
        const int details_x = getbegx(w_list);

        const int details_h = 7;
        const int details_w = getbegx(w_grid) + getmaxx(w_grid) - details_x;

        // clear rightmost blocks of w_stats to avoid overlap
        int stats_col_2 = 33;
        int stats_col_3 = 65 + ((TERMX - FULL_SCREEN_WIDTH) / 4);
        int clear_x = getmaxx( w_stats ) - details_w + 1 >= stats_col_3 ? stats_col_3 : stats_col_2;
        for( int i = 0; i < getmaxy( w_stats ); i++) {
            mvwhline(w_stats, i, clear_x, ' ', getmaxx( w_stats ) - clear_x);
        }

        wrefresh(w_stats);

        w_details = catacurses::newwin( details_h, details_w, details_y, details_x );
    }
    else {
        werase(w_details);
   }

    wborder(w_details, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);

    if ( part == NULL ) {
        wrefresh(w_details);
        return;
    }
    int details_w = getmaxx(w_details);
    int column_width = details_w / 2; // displays data in two columns
    int col_1 = 2;
    int col_2 = col_1 + column_width;
    int line = 0;
    bool small_mode = column_width < 20 ? true : false;

    // line 0: part name
    fold_and_print( w_details, line, col_1, details_w, c_light_green, part->name() );

    // line 1: (column 1) durability   (column 2) damage mod
    fold_and_print(w_details, line+1, col_1, column_width, c_white,
                   "%s: <color_light_gray>%d</color>",
                   small_mode ? _("Dur") : _("Durability"),
                   part->durability);
    fold_and_print(w_details, line+1, col_2, column_width, c_white,
                   "%s: <color_light_gray>%d%%</color>",
                   small_mode ? _("Dmg") : _("Damage"),
                   part->dmg_mod);

    // line 2: (column 1) weight   (column 2) folded volume (if applicable)
    fold_and_print(w_details, line+2, col_1, column_width, c_white,
                   "%s: <color_light_gray>%.1f%s</color>",
                   small_mode ? _("Wgt") : _("Weight"),
                   convert_weight( item::find_type( part->item )->weight ),
                   weight_units());
    if ( part->folded_volume != 0 ) {
        fold_and_print(w_details, line+2, col_2, column_width, c_white,
                       "%s: <color_light_gray>%s %s</color>",
                       small_mode ? _("FoldVol") : _("Folded Volume"),
                       format_volume( part->folded_volume ).c_str(),
                       volume_units_abbr() );
    }

    // line 3: (column 1) size, bonus, wheel diameter (if applicable)    (column 2) epower, wheel width (if applicable)
    if( part->size > 0 && part->has_flag( VPFLAG_CARGO ) ) {
        fold_and_print( w_details, line+3, col_1, column_width, c_white,
                       "%s: <color_light_gray>%d</color>", small_mode ? _( "Cap" ) : _( "Capacity" ),
                       to_milliliter( part->size ) );
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
                   part->has_flag( VPFLAG_CIRCLE_LIGHT ) || part->has_flag( VPFLAG_DOME_LIGHT ) ||
                   part->has_flag( VPFLAG_AISLE_LIGHT ) || part->has_flag( VPFLAG_EVENTURN ) ||
                   part->has_flag( VPFLAG_ODDTURN ) || part->has_flag( VPFLAG_ATOMIC_LIGHT ) ) {
            label = _( "Light" );
        }

        if( !label.empty() ) {
            fold_and_print( w_details, line+3, col_1, column_width, c_white,
                            "%s: <color_light_gray>%d</color>", label.c_str(),
                            part->bonus );
        }
    }

    if( part->has_flag( VPFLAG_WHEEL ) ) {
        cata::optional<islot_wheel> whl = item::find_type( part->item )->wheel;
        fold_and_print( w_details, line+3, col_1, column_width, c_white,
                            "%s: <color_light_gray>%d\"</color>", small_mode ? _( "Dia" ) : _( "Wheel Diameter" ),
                            whl->diameter );
        fold_and_print( w_details, line+3, col_2, column_width, c_white,
                            "%s: <color_light_gray>%d\"</color>", small_mode ? _( "Wdt" ) : _( "Wheel Width" ),
                            whl->width );
    }

    if ( part->epower != 0 ) {
        fold_and_print(w_details, line+3, col_2, column_width, c_white,
                       "%s: %c<color_light_gray>%d</color>",
                       small_mode ? _("Bat") : _("Battery"),
                       part->epower < 0 ? '-' : '+',
                       abs(part->epower));
    }

    // line 4 [horizontal]: fuel_type (if applicable)
    // line 4 [vertical/hybrid]: (column 1) fuel_type (if applicable)    (column 2) power (if applicable)
    // line 5 [horizontal]: power (if applicable)
    if ( part->fuel_type != "null" ) {
        fold_and_print( w_details, line+4, col_1, column_width,
                        c_white, _("Charge: <color_light_gray>%s</color>"),
                        item::nname( part->fuel_type ).c_str() );
    }
    int part_power = part->power;
    if( part_power == 0 ) {
        part_power = item( part->item ).engine_displacement();
    }
    if ( part_power != 0 ) {
        fold_and_print( w_details, line + 4, col_2, column_width, c_white, _( "Power: <color_light_gray>%d</color>" ), part_power );
    }

    // line 5 [vertical/hybrid] flags
    std::vector<std::string> flags = { { "OPAQUE", "OPENABLE", "BOARDABLE" } };
    std::vector<std::string> flag_labels = { { _("opaque"), _("openable"), _("boardable") } };
    std::string label;
    for ( size_t i = 0; i < flags.size(); i++ ) {
        if ( part->has_flag(flags[i]) ) {
            label += ( label.empty() ? "" : " " ) + flag_labels[i];
        }
    }
    // 6 [horizontal]: (column 1) flags    (column 2) battery capacity (if applicable)
    fold_and_print(w_details, line + 5, col_1, details_w, c_yellow, label);

    if( part->fuel_type == "battery" && !part->has_flag( VPFLAG_ENGINE ) && !part->has_flag( VPFLAG_ALTERNATOR ) ) {
        cata::optional<islot_magazine> battery = item::find_type( part->item )->magazine;
        fold_and_print( w_details, line + 5, col_2, column_width, c_white,
                        "%s: <color_light_gray>%d</color>", small_mode ? _( "BatCap" ) : _( "Battery Capacity" ),
                        battery->capacity );
    }

    wrefresh(w_details);
}

void veh_interact::countDurability()
{
    int qty = std::accumulate( veh->parts.begin(), veh->parts.end(), 0,
        []( int lhs, const vehicle_part &rhs ) {
            return lhs + std::max( rhs.base.damage(), 0 );
    } );

    int total = std::accumulate( veh->parts.begin(), veh->parts.end(), 0,
        []( int lhs, const vehicle_part &rhs ) {
            return lhs + rhs.base.max_damage();
    } );

    double pct = double( qty ) / double( total );

    if( pct < 0.05 ) {
        totalDurabilityText = _( "like new" );
        totalDurabilityColor = c_light_green;

    } else if( pct < 0.33 ) {
        totalDurabilityText = _( "dented" );
        totalDurabilityColor = c_yellow;

    } else if( pct < 0.66 ) {
        totalDurabilityText = _( "battered" );
        totalDurabilityColor = c_magenta;

    } else if( pct < 1.00 ) {
        totalDurabilityText = _( "wrecked" );
        totalDurabilityColor = c_red;

    } else {
        totalDurabilityText = _( "destroyed" );
        totalDurabilityColor = c_dark_gray;
    }
}

/**
 * Given a vpart id, gives the choice of inventory and nearby items to consume
 * for install/repair/etc. Doesn't use consume_items in crafting.cpp, as it got
 * into weird cases and doesn't consider properties like HP. The
 * item will be removed by this function.
 * @param vpid The id of the vpart type to look for.
 * @return The item that was consumed.
 */
item consume_vpart_item( const vpart_id &vpid )
{
    std::vector<bool> candidates;
    const itype_id itid = vpid.obj().item;

    if(g->u.has_trait( trait_DEBUG_HS )) {
        return item(itid, calendar::turn);
    }

    inventory map_inv;
    map_inv.form_from_map( g->u.pos(), PICKUP_RANGE );

    if( g->u.has_amount( itid, 1 ) ) {
        candidates.push_back( true );
    }
    if( map_inv.has_components( itid, 1 ) ) {
        candidates.push_back( false );
    }

    // bug?
    if(candidates.empty()) {
        debugmsg("Part not found!");
        return item();
    }

    int selection;
    // no choice?
    if(candidates.size() == 1) {
        selection = 0;
    } else {
        // popup menu!?
        std::vector<std::string> options;
        for( const auto candidate : candidates ) {
            const vpart_info &info = vpid.obj();
            if( candidate ) {
                // In inventory.
                options.emplace_back( info.name() );
            } else {
                // Nearby.
                options.emplace_back( info.name() + _(" (nearby)" ) );
            }
        }
        selection = menu_vec(false, _("Use which gizmo?"), options);
        selection -= 1;
    }
    std::list<item> item_used;
    //remove item from inventory. or map.
    if( candidates[selection] ) {
        item_used = g->u.use_amount( itid, 1 );
    } else {
        long quantity = 1;
        item_used = g->m.use_amount( g->u.pos(), PICKUP_RANGE, itid, quantity );
    }
    remove_ammo( item_used, g->u );

    return item_used.front();
}

void act_vehicle_siphon(vehicle* veh) {
    std::vector<itype_id> fuels;
    for( auto & e : veh->fuels_left() ) {
        const itype *type = item::find_type( e.first );
        if( type->phase != LIQUID ) {
            // This skips battery and plutonium cells
            continue;
        }
        fuels.push_back( e.first );
    }
    if( fuels.empty() ) {
        add_msg(m_info, _("The vehicle has no liquid fuel left to siphon."));
        return;
    }
    itype_id fuel;
    if( fuels.size() > 1 ) {
        uimenu smenu;
        smenu.text = _("Siphon what?");
        for( auto & fuel : fuels ) {
            smenu.addentry( item::nname( fuel ) );
        }
        smenu.addentry(_("Never mind"));
        smenu.query();
        if( static_cast<size_t>( smenu.ret ) >= fuels.size() ) {
            add_msg(m_info, _("Never mind."));
            return;
        }
        fuel = fuels[smenu.ret];
    } else {
        fuel = fuels.front();
    }

    g->u.siphon( *veh, fuel );
}

/**
 * Called when the activity timer for installing parts, repairing, etc times
 * out and the action is complete.
 */
void veh_interact::complete_vehicle()
{
    if (g->u.activity.values.size() < 7) {
        debugmsg ("Invalid activity ACT_VEHICLE values:%d", g->u.activity.values.size());
        return;
    }
    const optional_vpart_position vp = g->m.veh_at( tripoint( g->u.activity.values[0], g->u.activity.values[1], g->u.posz() ) );
    if( !vp ) {
        debugmsg ("Activity ACT_VEHICLE: vehicle not found");
        return;
    }
    vehicle *const veh = &vp->vehicle();

    int dx = g->u.activity.values[4];
    int dy = g->u.activity.values[5];
    int vehicle_part = g->u.activity.values[6];
    const vpart_id part_id( g->u.activity.str_values[0] );

    const vpart_info &vpinfo = part_id.obj();

    // cmd = Install Repair reFill remOve Siphon Changetire reName relAbel
    switch( (char) g->u.activity.index ) {

        case 'i': {
        auto inv = g->u.crafting_inventory();

        const auto reqs = vpinfo.install_requirements();
        if( !reqs.can_make_with_inventory( inv ) ) {
           add_msg( m_info, _( "You don't meet the requirements to install the %s." ), vpinfo.name().c_str() );
           break;
        }

        // consume items extracting a match for the parts base item
        item base;
        for( const auto& e : reqs.get_components() ) {
            for( auto& obj : g->u.consume_items( e ) ) {
                if( obj.typeId() == vpinfo.item ) {
                    base = obj;
                }
            }
        }
        if( base.is_null() ) {
            if( !g->u.has_trait( trait_DEBUG_HS ) ) {
                add_msg( m_info, _( "Could not find base part in requirements for %s." ), vpinfo.name().c_str() );
                break;
            } else {
                base = item( vpinfo.item );
            }
        }

        for( const auto& e : reqs.get_tools() ) {
            g->u.consume_tools( e );
        }

        g->u.invalidate_crafting_inventory();

        int partnum = !base.is_null() ? veh->install_part( dx, dy, part_id, std::move( base ) ) : -1;
        if(partnum < 0) {
            debugmsg( "complete_vehicle install part fails dx=%d dy=%d id=%s", dx, dy, part_id.c_str() );
            break;
        }

        // Need map-relative coordinates to compare to output of look_around.
        // Need to call coord_translate() directly since it's a new part.
        const point q = veh->coord_translate( point( dx, dy ) );

        if ( vpinfo.has_flag("CONE_LIGHT") ) {
            // Stash offset and set it to the location of the part so look_around will start there.
            int px = g->u.view_offset.x;
            int py = g->u.view_offset.y;
            g->u.view_offset.x = veh->global_x() + q.x - g->u.posx();
            g->u.view_offset.y = veh->global_y() + q.y - g->u.posy();
            popup(_("Choose a facing direction for the new headlight.  Press space to continue."));
            tripoint headlight_target = g->look_around(); // Note: no way to cancel
            // Restore previous view offsets.
            g->u.view_offset.x = px;
            g->u.view_offset.y = py;

            int dir = 0;
            if(headlight_target.x == INT_MIN) {
                dir = 0;
            } else {
                int delta_x = headlight_target.x - (veh->global_x() + q.x);
                int delta_y = headlight_target.y - (veh->global_y() + q.y);

                const double PI = 3.14159265358979f;
                dir = int(atan2(static_cast<float>(delta_y), static_cast<float>(delta_x)) * 180.0 / PI);
                dir -= veh->face.dir();
                while(dir < 0) {
                    dir += 360;
                }
                while(dir > 360) {
                    dir -= 360;
                }
            }

            veh->parts[partnum].direction = dir;
        }

        const tripoint vehp = { q.x + veh->global_x(), q.y + veh->global_y(), g->u.posz() };
        //@todo: allow boarding for non-players as well.
        player * const pl = g->critter_at<player>( vehp );
        if( vpinfo.has_flag( VPFLAG_BOARDABLE ) && pl ) {
            g->m.board_vehicle( vehp, pl );
        }

        add_msg( m_good, _("You install a %1$s into the %2$s." ), veh->parts[ partnum ].name().c_str(), veh->name.c_str() );

        for( const auto &sk : vpinfo.install_skills ) {
            g->u.practice( sk.first, veh_utils::calc_xp_gain( vpinfo, sk.first ) );
        }

        break;
    }

    case 'r': {
        veh_utils::repair_part( *veh, veh->parts[ vehicle_part ], g->u );
        break;
    }

    case 'f': {
        if( g->u.activity.targets.empty() || !g->u.activity.targets.front() ) {
            debugmsg( "Activity ACT_VEHICLE: missing refill source" );
            break;
        }

        auto &src = g->u.activity.targets.front();

        auto &pt = veh->parts[ vehicle_part ];
        if( pt.is_tank() && src->is_watertight_container() && !src->contents.empty() ) {

            pt.base.fill_with( src->contents.front() );

            if ( pt.ammo_remaining() != pt.ammo_capacity() ) {
                //~ 1$s vehicle name, 2$s tank name
                add_msg( m_good, _( "You refill the %1$s's %2$s." ),
                         veh->name.c_str(), pt.name().c_str() );
            } else {
                //~ 1$s vehicle name, 2$s tank name
                add_msg( m_good, _( "You completely refill the %1$s's %2$s." ),
                         veh->name.c_str(), pt.name().c_str() );
            }

            if( src->contents.front().charges == 0 ) {
                src->contents.erase( src->contents.begin() );
            } else {
                add_msg( m_good, _( "There's some left over!" ) );
            }

        } else if( pt.is_reactor() ) {
            auto qty = src->charges;
            pt.base.reload( g->u, std::move( src ), qty );

            //~ 1$s vehicle name, 2$s reactor name
            add_msg( m_good, _( "You refuel the %1$s's %2$s." ),
                     veh->name.c_str(), pt.name().c_str() );

        } else {
            debugmsg( "vehicle part is not reloadable" );
            break;
        }

        veh->invalidate_mass();
        break;
    }

    case 'o': {
        auto inv = g->u.crafting_inventory();

        const auto reqs = vpinfo.removal_requirements();
        if( !reqs.can_make_with_inventory( inv ) ) {
           add_msg( m_info, _( "You don't meet the requirements to remove the %s." ), vpinfo.name().c_str() );
           break;
        }

        for( const auto& e : reqs.get_components() ) {
            g->u.consume_items( e );
        }
        for( const auto& e : reqs.get_tools() ) {
            g->u.consume_tools( e );
        }

        g->u.invalidate_crafting_inventory();

        // Dump contents of part at player's feet, if any.
        vehicle_stack contents = veh->get_items( vehicle_part );
        for( auto iter = contents.begin(); iter != contents.end(); ) {
            g->m.add_item_or_charges( g->u.posx(), g->u.posy(), *iter );
            iter = contents.erase( iter );
        }

        // Power cables must remove parts from the target vehicle, too.
        if (veh->part_flag(vehicle_part, "POWER_TRANSFER")) {
            veh->remove_remote_part(vehicle_part);
        }

        bool broken = veh->parts[ vehicle_part ].is_broken();
        if (!broken) {
            g->m.add_item_or_charges( g->u.pos(), veh->parts[vehicle_part].properties_to_item() );
            for( const auto &sk : vpinfo.install_skills ) {
                // removal is half as educational as installation
                g->u.practice( sk.first, veh_utils::calc_xp_gain( vpinfo, sk.first ) / 2 );
            }

        } else {
            veh->break_part_into_pieces(vehicle_part, g->u.posx(), g->u.posy());
        }
        if (veh->parts.size() < 2) {
            add_msg (_("You completely dismantle the %s."), veh->name.c_str());
            g->u.activity.set_to_null();
            g->m.destroy_vehicle (veh);
        } else {
            if (broken) {
                add_msg( _( "You remove the broken %1$s from the %2$s." ),
                         veh->parts[ vehicle_part ].name().c_str(), veh->name.c_str() );
            } else {
                add_msg( _( "You remove the %1$s from the %2$s." ),
                         veh->parts[ vehicle_part ].name().c_str(), veh->name.c_str() );
            }
            veh->remove_part (vehicle_part);
            veh->part_removal_cleanup();
        }
        break;
    }

    case 'c':
        std::vector<int> parts = veh->parts_at_relative( dx, dy );
        if( parts.size() ) {
            item removed_wheel;
            int replaced_wheel = veh->part_with_feature( parts[0], "WHEEL", false );
            if( replaced_wheel == -1 ) {
                debugmsg( "no wheel to remove when changing wheels." );
                return;
            }
            bool broken = veh->parts[ replaced_wheel ].is_broken();
            removed_wheel = veh->parts[replaced_wheel].properties_to_item();
            veh->remove_part( replaced_wheel );
            veh->part_removal_cleanup();
            int partnum = veh->install_part( dx, dy, part_id, consume_vpart_item( part_id ) );
            if( partnum < 0 ) {
                debugmsg( "complete_vehicle tire change fails dx=%d dy=%d id=%s", dx, dy, part_id.c_str() );
            }
            // Place the removed wheel on the map last so consume_vpart_item() doesn't pick it.
            if ( !broken ) {
                g->m.add_item_or_charges( g->u.posx(), g->u.posy(), removed_wheel );
            }
            add_msg( _( "You replace one of the %1$s's tires with a %2$s." ),
                     veh->name.c_str(), veh->parts[ partnum ].name().c_str() );
        }
        break;
    }
    g->u.invalidate_crafting_inventory();
}
