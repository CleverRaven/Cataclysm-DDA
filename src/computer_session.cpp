#include "computer_session.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "character_id.h"
#include "color.h"
#include "computer.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "explosion.h"
#include "field_type.h"
#include "flag.h"
#include "game.h"
#include "game_inventory.h"
#include "input.h"
#include "input_context.h"
#include "input_enums.h"
#include "item.h"
#include "item_location.h"
#include "line.h"
#include "localized_comparator.h"
#include "map.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "messages.h"
#include "mission.h"
#include "monster.h"
#include "mtype.h"
#include "mutation.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "timed_event.h"
#include "translation.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "uilist.h"
#include "ui_manager.h"
#include "units.h"

static const efftype_id effect_amigara( "amigara" );

static const furn_str_id furn_f_centrifuge( "f_centrifuge" );
static const furn_str_id furn_f_console_broken( "f_console_broken" );
static const furn_str_id furn_f_counter( "f_counter" );
static const furn_str_id furn_f_rubble_rock( "f_rubble_rock" );

static const itype_id itype_black_box( "black_box" );
static const itype_id itype_black_box_transcript( "black_box_transcript" );
static const itype_id itype_blood( "blood" );
static const itype_id itype_blood_tainted( "blood_tainted" );
static const itype_id itype_c4( "c4" );
static const itype_id itype_cobalt_60( "cobalt_60" );
static const itype_id itype_mininuke( "mininuke" );
static const itype_id itype_mininuke_act( "mininuke_act" );
static const itype_id itype_radio_repeater_mod( "radio_repeater_mod" );
static const itype_id itype_sarcophagus_access_code( "sarcophagus_access_code" );
static const itype_id itype_sewage( "sewage" );
static const itype_id itype_software_blood_data( "software_blood_data" );
static const itype_id itype_usb_drive( "usb_drive" );
static const itype_id itype_vacutainer( "vacutainer" );

static const mission_type_id
mission_MISSION_OLD_GUARD_NEC_COMMO_2( "MISSION_OLD_GUARD_NEC_COMMO_2" );
static const mission_type_id
mission_MISSION_OLD_GUARD_NEC_COMMO_3( "MISSION_OLD_GUARD_NEC_COMMO_3" );
static const mission_type_id
mission_MISSION_OLD_GUARD_NEC_COMMO_4( "MISSION_OLD_GUARD_NEC_COMMO_4" );
static const mission_type_id mission_MISSION_OLD_GUARD_REPEATER( "MISSION_OLD_GUARD_REPEATER" );
static const mission_type_id
mission_MISSION_OLD_GUARD_REPEATER_BEGIN( "MISSION_OLD_GUARD_REPEATER_BEGIN" );
static const mission_type_id mission_MISSION_REACH_REFUGEE_CENTER( "MISSION_REACH_REFUGEE_CENTER" );

static const mtype_id mon_manhack( "mon_manhack" );
static const mtype_id mon_secubot( "mon_secubot" );

static const oter_type_str_id oter_type_sewer( "sewer" );
static const oter_type_str_id oter_type_subway( "subway" );

static const overmap_special_id overmap_special_Crater( "Crater" );

static const skill_id skill_computer( "computer" );

static const species_id species_HUMAN( "HUMAN" );
static const species_id species_ZOMBIE( "ZOMBIE" );

static const ter_str_id ter_t_concrete( "t_concrete" );
static const ter_str_id ter_t_concrete_wall( "t_concrete_wall" );
static const ter_str_id ter_t_door_metal_c( "t_door_metal_c" );
static const ter_str_id ter_t_door_metal_locked( "t_door_metal_locked" );
static const ter_str_id ter_t_elevator( "t_elevator" );
static const ter_str_id ter_t_elevator_control( "t_elevator_control" );
static const ter_str_id ter_t_elevator_control_off( "t_elevator_control_off" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_floor_blue( "t_floor_blue" );
static const ter_str_id ter_t_floor_green( "t_floor_green" );
static const ter_str_id ter_t_floor_red( "t_floor_red" );
static const ter_str_id ter_t_grate( "t_grate" );
static const ter_str_id ter_t_metal_floor( "t_metal_floor" );
static const ter_str_id ter_t_missile( "t_missile" );
static const ter_str_id ter_t_open_air( "t_open_air" );
static const ter_str_id ter_t_rad_platform( "t_rad_platform" );
static const ter_str_id ter_t_radio_tower( "t_radio_tower" );
static const ter_str_id ter_t_reinforced_glass_lab( "t_reinforced_glass_lab" );
static const ter_str_id ter_t_reinforced_glass_shutter( "t_reinforced_glass_shutter" );
static const ter_str_id ter_t_reinforced_glass_shutter_open( "t_reinforced_glass_shutter_open" );
static const ter_str_id ter_t_sewage( "t_sewage" );
static const ter_str_id ter_t_sewage_pipe( "t_sewage_pipe" );
static const ter_str_id ter_t_sewage_pump( "t_sewage_pump" );
static const ter_str_id ter_t_thconc_floor( "t_thconc_floor" );
static const ter_str_id ter_t_vat( "t_vat" );
static const ter_str_id ter_t_wall_glass( "t_wall_glass" );
static const ter_str_id ter_t_wall_metal( "t_wall_metal" );
static const ter_str_id ter_t_water_pool( "t_water_pool" );

static const trap_str_id tr_portal( "tr_portal" );

static catacurses::window init_window()
{
    const int width = FULL_SCREEN_WIDTH;
    const int height = FULL_SCREEN_HEIGHT;
    const point p( std::max( 0, ( TERMX - width ) / 2 ), std::max( 0, ( TERMY - height ) / 2 ) );
    return catacurses::newwin( height, width, p );
}

computer_session::computer_session( computer &comp ) : comp( comp ),
    win( init_window() ), left( 1 ), top( 1 ), width( getmaxx( win ) - 2 ),
    height( getmaxy( win ) - 2 )
{
}

void computer_session::use()
{
    ui_adaptor ui;
    ui.on_screen_resize( [this]( ui_adaptor & ui ) {
        const int width = getmaxx( win );
        const int height = getmaxy( win );
        const point p( std::max( 0, ( TERMX - width ) / 2 ), std::max( 0, ( TERMY - height ) / 2 ) );
        win = catacurses::newwin( height, width, p );
        ui.position_from_window( win );
    } );
    ui.mark_resize();
    ui.on_redraw( [this]( const ui_adaptor & ) {
        refresh();
    } );

    avatar &player_character = get_avatar();
    // Login
    print_line( _( "Logging into %s…" ), comp.name );
    if( comp.security > 0 ) {
        if( calendar::turn < comp.next_attempt ) {
            print_error( _( "Access is temporary blocked for security purposes." ) );
            query_any( _( "Please contact the system administrator." ) );
            reset_terminal();
            return;
        }
        print_error( "%s", comp.access_denied );
        switch( query_ynq( _( "Bypass security?" ) ) ) {
            case ynq::quit:
                reset_terminal();
                return;

            case ynq::no:
                query_any( _( "Shutting down… press any key." ) );
                reset_terminal();
                return;

            case ynq::yes:
                if( !hack_attempt( player_character ) ) {
                    if( comp.failures.empty() ) {
                        query_any( _( "Maximum login attempts exceeded.  Press any key…" ) );
                        reset_terminal();
                        return;
                    }
                    activate_random_failure();
                    reset_terminal();
                    return;
                } else { // Successful hack attempt
                    comp.security = 0;
                    query_any( _( "Login successful.  Press any key…" ) );
                    reset_terminal();
                }
        }
    } else { // No security
        query_any( _( "Login successful.  Press any key…" ) );
        reset_terminal();
    }

    // Main computer loop
    int sel = 0;
    while( true ) {
        size_t options_size = comp.options.size();

        uilist computer_menu;
        computer_menu.text = string_format( _( "%s - Root Menu" ), comp.name );
        computer_menu.selected = sel;
        computer_menu.fselected = sel;

        for( size_t i = 0; i < options_size; i++ ) {
            bool activatable = can_activate( comp.options[i].action );
            std::string action_name = comp.options[i].name;
            if( !activatable ) {
                action_name = string_format( _( "%s (UNAVAILABLE)" ), action_name );
            }
            computer_menu.addentry( i, activatable, MENU_AUTOASSIGN, action_name );
        }

        ui_manager::redraw();
        computer_menu.query();
        if( computer_menu.ret < 0 || static_cast<size_t>( computer_menu.ret ) >= comp.options.size() ) {
            break;
        }

        sel = computer_menu.ret;
        computer_option current = comp.options[sel];
        reset_terminal();
        // Once you trip the security, you have to roll every time you want to do something
        if( current.security + comp.alerts > 0 ) {
            print_error( _( "Password required." ) );
            if( query_bool( _( "Hack into system?" ) ) ) {
                if( !hack_attempt( player_character, current.security ) ) {
                    activate_random_failure();
                    reset_terminal();
                    return;
                } else {
                    // Successfully hacked function
                    comp.options[sel].security = 0;
                    activate_function( current.action );
                }
            }
        } else { // No need to hack, just activate
            activate_function( current.action );
        }
        reset_terminal();
        // Done processing a selected option.
    }

    reset_terminal(); // This should have been done by now, but just in case.
}

bool computer_session::hack_attempt( Character &you, int Security ) const
{
    if( Security == -1 ) {
        Security = comp.security;    // Set to main system security if no value passed
    }
    const int hack_skill = round( you.get_skill_level( skill_computer ) );

    // Every time you dig for lab notes, (or, in future, do other suspicious stuff?)
    // +2 dice to the system's hack-resistance
    // So practical max files from a given terminal = 5, at 10 Computer
    if( comp.alerts > 0 ) {
        Security += ( comp.alerts * 2 );
    }

    you.mod_moves( -( 10 * ( 5 + Security * 2 ) / std::max( 1, hack_skill + 1 ) ) );

    int player_roll = round( you.get_greater_skill_or_knowledge_level(
                                 skill_computer ) ); //this relates to the success of the roll for hacking - the practical skill covers the time.
    ///\EFFECT_INT <8 randomly penalizes hack attempts, 50% of the time
    if( you.int_cur < 8 && one_in( 2 ) ) {
        player_roll -= rng( 0, 8 - you.int_cur );
        ///\EFFECT_INT >8 randomly benefits hack attempts, 33% of the time
    } else if( you.int_cur > 8 && one_in( 3 ) ) {
        player_roll += rng( 0, you.int_cur - 8 );
    }

    ///\EFFECT_COMPUTER increases chance of successful hack attempt, vs Security level
    bool successful_attempt = dice( player_roll, 6 ) >= dice( Security, 6 );
    you.practice( skill_computer, successful_attempt ? ( 15 + Security * 3 ) : 7 );
    return successful_attempt;
}

static item *pick_estorage( units::ememory can_hold_ememory )
{
    auto filter = [&can_hold_ememory]( const item & it ) {
        return it.is_estorage() && it.remaining_ememory() >= can_hold_ememory;
    };

    item_location loc = game_menus::inv::titled_filter_menu( filter, get_avatar(),
                        _( "Choose device:" ) );
    if( loc ) {
        return &*loc;
    }
    return nullptr;
}

static void remove_submap_turrets()
{
    Character &player_character = get_player_character();
    map &here = get_map();
    for( monster &critter : g->all_monsters() ) {
        // Check 1) same overmap coords, 2) turret, 3) hostile
        if( coords::project_to<coords::omt>( here.get_abs( critter.pos_bub() ) ) ==
            coords::project_to<coords::omt>( here.get_abs(
                    player_character.pos_bub() ) ) &&
            critter.has_flag( mon_flag_CONSOLE_DESPAWN ) &&
            critter.attitude_to( player_character ) == Creature::Attitude::HOSTILE ) {
            g->remove_zombie( critter );
        }
    }
}

const std::map<computer_action, void ( computer_session::* )()>
computer_session::computer_action_functions = {
    { COMPACT_AMIGARA_LOG, &computer_session::action_amigara_log },
    { COMPACT_AMIGARA_START, &computer_session::action_amigara_start },
    { COMPACT_BLOOD_ANAL, &computer_session::action_blood_anal },
    { COMPACT_CASCADE, &computer_session::action_cascade },
    { COMPACT_COMPLETE_DISABLE_EXTERNAL_POWER, &computer_session::action_complete_disable_external_power },
    { COMPACT_CONVEYOR, &computer_session::action_conveyor },
    { COMPACT_DATA_ANAL, &computer_session::action_data_anal },
    { COMPACT_DEACTIVATE_SHOCK_VENT, &computer_session::action_deactivate_shock_vent },
    { COMPACT_DISCONNECT, &computer_session::action_disconnect },
    { COMPACT_DOWNLOAD_SOFTWARE, &computer_session::action_download_software },
    { COMPACT_ELEVATOR_ON, &computer_session::action_elevator_on },
    { COMPACT_EMERG_MESS, &computer_session::action_emerg_mess },
    { COMPACT_EMERG_REF_CENTER, &computer_session::action_emerg_ref_center },
    { COMPACT_EXTRACT_RAD_SOURCE, &computer_session::action_extract_rad_source },
    { COMPACT_GEIGER, &computer_session::action_geiger },
    { COMPACT_IRRADIATOR, &computer_session::action_irradiator },
    { COMPACT_LIST_BIONICS, &computer_session::action_list_bionics },
    { COMPACT_LIST_MUTATIONS, &computer_session::action_list_mutations },
    { COMPACT_LOCK, &computer_session::action_lock },
    { COMPACT_MAP_SEWER, &computer_session::action_map_sewer },
    { COMPACT_MAP_SUBWAY, &computer_session::action_map_subway },
    { COMPACT_MAPS, &computer_session::action_maps },
    { COMPACT_MISS_DISARM, &computer_session::action_miss_disarm },
    { COMPACT_MISS_LAUNCH, &computer_session::action_miss_launch },
    { COMPACT_OPEN, &computer_session::action_open },
    { COMPACT_OPEN_GATE, &computer_session::action_open_gate },
    { COMPACT_CLOSE_GATE, &computer_session::action_close_gate },
    { COMPACT_OPEN_DISARM, &computer_session::action_open_disarm },
    { COMPACT_PORTAL, &computer_session::action_portal },
    { COMPACT_RADIO_ARCHIVE, &computer_session::action_radio_archive },
    { COMPACT_RELEASE, &computer_session::action_release },
    { COMPACT_RELEASE_BIONICS, &computer_session::action_release_bionics },
    { COMPACT_RELEASE_DISARM, &computer_session::action_release_disarm },
    { COMPACT_REPEATER_MOD, &computer_session::action_repeater_mod },
    { COMPACT_RESEARCH, &computer_session::action_research },
    { COMPACT_SAMPLE, &computer_session::action_sample },
    { COMPACT_SHUTTERS, &computer_session::action_shutters },
    { COMPACT_SR1_MESS, &computer_session::action_sr1_mess },
    { COMPACT_SR2_MESS, &computer_session::action_sr2_mess },
    { COMPACT_SR3_MESS, &computer_session::action_sr3_mess },
    { COMPACT_SR4_MESS, &computer_session::action_sr4_mess },
    { COMPACT_SRCF_1_MESS, &computer_session::action_srcf_1_mess },
    { COMPACT_SRCF_2_MESS, &computer_session::action_srcf_2_mess },
    { COMPACT_SRCF_3_MESS, &computer_session::action_srcf_3_mess },
    { COMPACT_SRCF_ELEVATOR, &computer_session::action_srcf_elevator },
    { COMPACT_SRCF_SEAL, &computer_session::action_srcf_seal },
    { COMPACT_SRCF_SEAL_ORDER, &computer_session::action_srcf_seal_order },
    { COMPACT_TERMINATE, &computer_session::action_terminate },
    { COMPACT_TOLL, &computer_session::action_toll },
    { COMPACT_TOWER_UNRESPONSIVE, &computer_session::action_tower_unresponsive },
    { COMPACT_UNLOCK, &computer_session::action_unlock },
    { COMPACT_UNLOCK_DISARM, &computer_session::action_unlock_disarm },
};

bool computer_session::can_activate( computer_action action )
{
    map &here = get_map();

    switch( action ) {
        case COMPACT_LOCK:
            return here.has_nearby_ter( get_player_character().pos_bub(), ter_t_door_metal_c, 8 );

        case COMPACT_RELEASE:
        case COMPACT_RELEASE_DISARM:
            return here.has_nearby_ter( get_player_character().pos_bub(), ter_t_reinforced_glass_lab,
                                        25 );

        case COMPACT_RELEASE_BIONICS:
            return here.has_nearby_ter( get_player_character().pos_bub(), ter_t_reinforced_glass_lab, 3 );

        case COMPACT_TERMINATE: {
            creature_tracker &creatures = get_creature_tracker();
            for( const tripoint_bub_ms &p : here.points_on_zlevel() ) {
                monster *const mon = creatures.creature_at<monster>( p );
                if( !mon ) {
                    continue;
                }
                const ter_id &t_north = here.ter( p + tripoint::north );
                const ter_id &t_south = here.ter( p + tripoint::south );
                if( ( t_north == ter_t_reinforced_glass_lab &&
                      t_south == ter_t_concrete_wall ) ||
                    ( t_south == ter_t_reinforced_glass_lab &&
                      t_north == ter_t_concrete_wall ) ) {
                    return true;
                }
            }
            return false;
        }

        case COMPACT_UNLOCK:
        case COMPACT_UNLOCK_DISARM:
            return here.has_nearby_ter( get_player_character().pos_bub(), ter_t_door_metal_locked, 8 );

        default:
            return true;
    }
}

void computer_session::activate_function( computer_action action )
{
    const auto it = computer_action_functions.find( action );
    if( it != computer_action_functions.end() ) {
        // Token move cost for any action, if an action takes longer decrement moves further.
        get_player_character().mod_moves( -to_moves<int>( 1_seconds ) * 0.3 );
        ( this->*( it->second ) )();
    }
}

void computer_session::action_open_disarm()
{
    remove_submap_turrets();
    action_open();
}

void computer_session::action_open()
{
    get_map().translate_radius( ter_t_door_metal_locked, ter_t_floor, 25.0,
                                get_player_character().pos_bub(),
                                true );
    query_any( _( "Doors opened.  Press any key…" ) );
}

void computer_session::action_open_gate()
{
    get_map().translate_radius( ter_t_wall_metal, ter_t_metal_floor, 8.0,
                                get_player_character().pos_bub(),
                                true );
    query_any( _( "Gates opened.  Press any key…" ) );
}

void computer_session::action_close_gate()
{
    get_map().translate_radius( ter_t_metal_floor, ter_t_wall_metal, 8.0,
                                get_player_character().pos_bub(),
                                true );
    query_any( _( "Gates closed.  Press any key…" ) );
}

//LOCK AND UNLOCK are used to build more complex buildings
// that can have multiple doors that can be locked and be
// unlocked by different computers.
//Simply uses translate_radius which take a given radius and
// player position to determine which terrain tiles to edit.
void computer_session::action_lock()
{
    get_map().translate_radius( ter_t_door_metal_c, ter_t_door_metal_locked, 8.0,
                                get_player_character().pos_bub(),
                                true );
    query_any( _( "Lock enabled.  Press any key…" ) );
}

void computer_session::action_unlock_disarm()
{
    remove_submap_turrets();
    action_unlock();
}

void computer_session::action_unlock()
{
    get_map().translate_radius( ter_t_door_metal_locked, ter_t_door_metal_c, 8.0,
                                get_player_character().pos_bub(),
                                true );
    query_any( _( "Lock disabled.  Press any key…" ) );
}

//Toll is required for the church/school computer/mechanism to function
void computer_session::action_toll()
{
    if( calendar::turn < comp.next_attempt ) {
        print_error( _( "[Bellsystem 1.2] is currently in use." ) );
        query_any( _( "Please wait for at least one minute." ) );
        reset_terminal();
    } else {
        comp.next_attempt = calendar::turn + 1_minutes;
        sounds::sound( get_player_character().pos_bub(), 120, sounds::sound_t::alarm,
                       //~ the sound of a church bell ringing
                       _( "Bohm…  Bohm…  Bohm…" ), true, "environment", "church_bells" );

        query_any( _( "[Bellsystem 1.2] activated.  Have a nice day." ) );
    }
}

void computer_session::action_sample()
{
    get_player_character().mod_moves( -to_moves<int>( 1_seconds ) * 0.3 );
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_on_zlevel() ) {
        if( here.ter( p ) != ter_t_sewage_pump ) {
            continue;
        }
        for( const tripoint_bub_ms &n : here.points_in_radius( p, 1 ) ) {
            if( here.furn( n ) != furn_f_counter ) {
                continue;
            }
            bool found_item = false;
            item sewage( itype_sewage, calendar::turn );
            for( item &elem : here.i_at( n ) ) {
                int capa = elem.get_remaining_capacity_for_liquid( sewage );
                if( capa <= 0 ) {
                    continue;
                }
                sewage.charges = std::min( sewage.charges, capa );
                if( elem.can_contain( sewage ).success() ) {
                    elem.put_in( sewage, pocket_type::CONTAINER );
                }
                found_item = true;
                break;
            }
            if( !found_item ) {
                here.add_item_or_charges( n, sewage );
            }
        }
    }
}

void computer_session::action_release()
{
    get_event_bus().send<event_type::releases_subspace_specimens>();
    Character &player_character = get_player_character();
    sounds::sound( player_character.pos_bub(), 40, sounds::sound_t::alarm, _( "an alarm sound!" ),
                   false,
                   "environment",
                   "alarm" );
    get_map().translate_radius( ter_t_reinforced_glass_lab, ter_t_thconc_floor, 25.0,
                                player_character.pos_bub(),
                                true );
    query_any( _( "Containment shields opened.  Press any key…" ) );
}

void computer_session::action_release_disarm()
{
    remove_submap_turrets();
    action_release_bionics();
}

void computer_session::action_release_bionics()
{
    Character &player_character = get_player_character();
    sounds::sound( player_character.pos_bub(), 40, sounds::sound_t::alarm, _( "an alarm sound!" ),
                   false,
                   "environment",
                   "alarm" );
    get_map().translate_radius( ter_t_reinforced_glass_lab, ter_t_thconc_floor, 3.0,
                                player_character.pos_bub(),
                                true );
    query_any( _( "Containment shields opened.  Press any key…" ) );
}

void computer_session::action_terminate()
{
    get_event_bus().send<event_type::terminates_subspace_specimens>();
    Character &player_character = get_player_character();
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint_bub_ms &p : here.points_on_zlevel() ) {
        monster *const mon = creatures.creature_at<monster>( p );
        if( !mon ) {
            continue;
        }
        const ter_id &t_north = here.ter( p + tripoint::north );
        const ter_id &t_south = here.ter( p + tripoint::south );
        if( ( t_north == ter_t_reinforced_glass_lab &&
              t_south == ter_t_concrete_wall ) ||
            ( t_south == ter_t_reinforced_glass_lab &&
              t_north == ter_t_concrete_wall ) ) {
            mon->die( &here, &player_character );
        }
    }
    query_any( _( "Subjects terminated.  Press any key…" ) );
}

void computer_session::action_portal()
{
    get_event_bus().send<event_type::opens_portal>();
    map &here = get_map();
    for( const tripoint_bub_ms &tmp : here.points_on_zlevel() ) {
        int numtowers = 0;
        for( const tripoint_bub_ms &tmp2 : here.points_in_radius( tmp, 2 ) ) {
            if( here.ter( tmp2 ) == ter_t_radio_tower ) {
                numtowers++;
            }
        }
        if( numtowers >= 4 ) {
            if( here.tr_at( tmp ).id == tr_portal ) {
                here.remove_trap( tmp );
            } else {
                here.trap_set( tmp, tr_portal );
            }
        }
    }
}

void computer_session::action_cascade()
{
    if( !query_bool( _( "WARNING: Resonance cascade carries severe risk!  Continue?" ) ) ) {
        return;
    }
    get_event_bus().send<event_type::causes_resonance_cascade>();
    tripoint_bub_ms player_pos = get_player_character().pos_bub();
    map &here = get_map();
    std::vector<tripoint_bub_ms> cascade_points;
    for( const tripoint_bub_ms &dest : here.points_in_radius( player_pos, 10 ) ) {
        if( here.ter( dest ) == ter_t_radio_tower ) {
            cascade_points.push_back( dest );
        }
    }
    explosion_handler::resonance_cascade( random_entry( cascade_points, player_pos ) );
}

void computer_session::action_research()
{
    map &here = get_map();
    // TODO: seed should probably be a member of the computer, or better: of the computer action.
    // It is here to ensure one computer reporting the same text on each invocation.
    const int seed = std::hash<tripoint_abs_sm> {}( here.get_abs_sub() ) + comp.alerts;
    std::optional<translation> log = SNIPPET.random_from_category( "lab_notes", seed );
    if( !log.has_value() ) {
        log = to_translation( "No data found." );
    } else {
        get_player_character().mod_moves( -to_moves<int>( 1_seconds ) * 0.7 );
    }

    print_text( "%s", log.value() );
    // One's an anomaly
    if( comp.alerts == 0 ) {
        query_any( _( "Local data-access error logged, alerting helpdesk.  Press any key…" ) );
        comp.alerts ++;
    } else {
        // Two's a trend.
        query_any(
            _( "Warning: anomalous archive-access activity detected at this node.  Press any key…" ) );
        comp.alerts ++;
    }
}

void computer_session::action_radio_archive()
{
    get_player_character().mod_moves( -to_moves<int>( 3_seconds ) );
    sfx::fade_audio_channel( sfx::channel::radio, 100 );
    sfx::play_ambient_variant_sound( "radio", "inaudible_chatter", 100, sfx::channel::radio,
                                     2000 );
    print_text( "Accessing archive.  Playing audio recording nr %d.\n%s", rng( 1, 9999 ),
                SNIPPET.random_from_category( "radio_archive" ).value_or( translation() ) );
    if( one_in( 3 ) ) {
        query_any( _( "Warning: restricted data access.  Attempt logged.  Press any key…" ) );
        comp.alerts ++;
    } else {
        query_any( _( "Press any key…" ) );
    }
    sfx::fade_audio_channel( sfx::channel::radio, 100 );
}

void computer_session::action_maps()
{
    Character &player_character = get_player_character();
    player_character.mod_moves( -to_moves<int>( 1_seconds ) * 0.3 );
    const tripoint_abs_omt center = player_character.pos_abs_omt();
    overmap_buffer.reveal( center.xy(), 40, 0 );
    query_any(
        _( "Surface map data downloaded.  Local anomalous-access error logged.  Press any key…" ) );
    comp.remove_option( COMPACT_MAPS );
    comp.alerts ++;
}

void computer_session::action_map_sewer()
{
    Character &player_character = get_player_character();
    player_character.mod_moves( -to_moves<int>( 1_seconds ) * 0.3 );
    const tripoint_abs_omt center = player_character.pos_abs_omt();
    for( int i = -60; i <= 60; i++ ) {
        for( int j = -60; j <= 60; j++ ) {
            point offset( i, j );
            const oter_id &oter = overmap_buffer.ter( center + offset );
            if( ( oter->get_type_id() == oter_type_sewer ) ||
                is_ot_match( "sewage", oter, ot_match_type::prefix ) ) {
                overmap_buffer.set_seen( center + offset, om_vision_level::details );
            }
        }
    }
    query_any( _( "Sewage map data downloaded.  Press any key…" ) );
    comp.remove_option( COMPACT_MAP_SEWER );
}

void computer_session::action_map_subway()
{
    Character &player_character = get_player_character();
    player_character.mod_moves( -to_moves<int>( 1_seconds ) * 0.3 );
    const tripoint_abs_omt center = player_character.pos_abs_omt();
    for( int i = -60; i <= 60; i++ ) {
        for( int j = -60; j <= 60; j++ ) {
            point offset( i, j );
            const oter_id &oter = overmap_buffer.ter( center + offset );
            if( ( oter->get_type_id() == oter_type_subway ) ||
                is_ot_match( "lab_train_depot", oter, ot_match_type::contains ) ) {
                overmap_buffer.set_seen( center + offset, om_vision_level::details );
            }
        }
    }
    query_any( _( "Subway map data downloaded.  Press any key…" ) );
    comp.remove_option( COMPACT_MAP_SUBWAY );
}

void computer_session::action_miss_disarm()
{
    // TODO: stop the nuke from creating radioactive clouds.
    if( query_yn( _( "Disarm missile." ) ) ) {
        get_event_bus().send<event_type::disarms_nuke>();
        add_msg( m_info, _( "Nuclear missile disarmed!" ) );
        //disable missile.
        comp.options.clear();
        activate_failure( COMPFAIL_SHUTDOWN );
    } else {
        add_msg( m_neutral, _( "Nuclear missile remains active." ) );
        return;
    }
}

void computer_session::action_miss_launch()
{
    map &here = get_map();

    // Target Acquisition.
    const tripoint_abs_omt target( ui::omap::choose_point(
                                       _( "Choose a target for the nuclear missile." ), 0 ) );
    if( target.is_invalid() ) {
        add_msg( m_info, _( "Target acquisition canceled." ) );
        return;
    }

    if( query_yn( _( "Confirm nuclear missile launch." ) ) ) {
        add_msg( m_info, _( "Nuclear missile launched!" ) );
        //Remove the option to fire another missile.
        comp.options.clear();
    } else {
        add_msg( m_info, _( "Nuclear missile launch aborted." ) );
        return;
    }

    //Put some smoke gas and explosions at the nuke location.
    const tripoint_bub_ms nuke_location = { get_player_character().pos_bub() - point( 12, 0 ) };
    for( const tripoint_bub_ms &loc : here.points_in_radius( nuke_location, 5, 0 ) ) {
        if( one_in( 4 ) ) {
            here.add_field( loc, fd_smoke, rng( 1, 9 ) );
        }
    }

    //Only explode once. But make it large.
    explosion_handler::explosion( &get_player_character(), nuke_location, 2000, 0.7, true );

    //...ERASE MISSILE, OPEN SILO, DISABLE COMPUTER
    // For each level between here and the surface, remove the missile
    for( int level = here.get_abs_sub().z(); level <= 0; level++ ) {
        map tmpmap;
        tmpmap.load( tripoint_abs_sm( here.get_abs_sub().xy(), level ),
                     false );

        if( level < 0 ) {
            tmpmap.translate( ter_t_missile, ter_t_open_air );
        } else {
            tmpmap.translate( ter_t_metal_floor, ter_t_open_air );
        }
        tmpmap.save();
    }

    overmap_buffer.place_special( *overmap_special_Crater, target, om_direction::type::north, false,
                                  true );

    activate_failure( COMPFAIL_SHUTDOWN );
}

void computer_session::action_list_bionics()
{
    get_player_character().mod_moves( -to_moves<int>( 1_seconds ) * 0.3 );
    std::vector<std::string> names;
    int more = 0;
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_on_zlevel() ) {
        for( item &elem : here.i_at( p ) ) {
            if( elem.is_bionic() ) {
                if( static_cast<int>( names.size() ) < TERMY - 8 ) {
                    names.push_back( elem.type_name() );
                } else {
                    more++;
                }
            }
        }
    }

    reset_terminal();

    print_newline();
    print_line( _( "Bionic access - Manifest:" ) );
    print_newline();

    for( auto &name : names ) {
        print_line( "%s", name );
    }
    if( more > 0 ) {
        print_line( n_gettext( "%d OTHER FOUND…", "%d OTHERS FOUND…", more ), more );
    }

    print_newline();
    query_any( _( "Press any key…" ) );
}

void computer_session::action_list_mutations()
{
    uilist ssmenu;
    std::vector<std::pair<std::string, mutation_category_id>> mutation_categories_list;
    mutation_categories_list.reserve( mutations_category.size() );
    for( const std::pair<const mutation_category_id, std::vector<trait_id> > &mut_cat :
         mutations_category ) {
        mutation_categories_list.emplace_back( mut_cat.first.c_str(), mut_cat.first );
    }
    ssmenu.text = _( "Choose mutation category" );
    std::sort( mutation_categories_list.begin(), mutation_categories_list.end(), localized_compare );
    int menu_ind = 0;
    for( const std::pair<std::string, mutation_category_id> &mut_cat : mutation_categories_list ) {
        ssmenu.addentry( menu_ind, true, MENU_AUTOASSIGN, mut_cat.first );
        ++menu_ind;
    }
    ssmenu.query();

    if( ssmenu.ret >= 0 && ssmenu.ret < static_cast< int >( mutation_categories_list.size() ) ) {
        const mutation_category_trait &category = mutation_category_trait::get_category(
                    mutation_categories_list[ssmenu.ret].second );
        const std::vector<trait_id> category_mutations = mutations_category[category.id];

        uilist wmenu;

        for( const trait_id &traits_iter : category_mutations ) {
            wmenu.addentry( -1, true, -2, traits_iter.obj().name() );
        }

        do {
            wmenu.query();

            if( wmenu.ret >= 0 && wmenu.ret < static_cast< int >( category_mutations.size() ) ) {
                const mutation_branch &mdata = category_mutations[wmenu.ret].obj();
                reset_terminal();
                print_text( _( "Description: %s" ), colorize( mdata.desc(), c_white ) );

                if( !mdata.replacements.empty() ) {
                    print_indented_line( 1, width - 2, _( "Changes to:" ) );
                    for( const trait_id &replacement : mdata.replacements ) {
                        print_indented_line( 1, width - 2, _( "%s" ), colorize( replacement->name(), c_white ) );
                    }
                    print_newline();
                }

                if( !mdata.cancels.empty() ) {
                    print_indented_line( 1, width - 2, _( "Cancels:" ) );
                    for( const trait_id &cancel : mdata.cancels ) {
                        print_indented_line( 1, width - 2, _( "%s" ), colorize( cancel->name(), c_white ) );
                    }
                }
            }
        } while( wmenu.ret >= 0 );
    }
}

void computer_session::action_elevator_on()
{
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_on_zlevel() ) {
        if( here.ter( p ) == ter_t_elevator_control_off ) {
            here.ter_set( p, ter_t_elevator_control );
        }
    }
    query_any( _( "Elevator activated.  Press any key…" ) );
}

void computer_session::action_amigara_log()
{
    get_timed_events().add( timed_event_type::AMIGARA_WHISPERS, calendar::turn + 5_minutes );

    Character &player_character = get_player_character();
    player_character.mod_moves( -to_moves<int>( 1_seconds ) * 0.3 );
    reset_terminal();
    tripoint_abs_sm abs_loc = get_map().get_abs_sub();
    point_abs_sm abs_sub = abs_loc.xy();
    print_line( _( "NEPower Mine%s Log" ), abs_sub.to_string() );
    print_text( "%s", SNIPPET.random_from_category( "amigara1" ).value_or( translation() ) );

    if( !query_bool( _( "Continue reading?" ) ) ) {
        return;
    }
    player_character.mod_moves( -to_moves<int>( 1_seconds ) * 0.3 );
    reset_terminal();
    print_line( _( "NEPower Mine%s Log" ), abs_sub.to_string() );
    print_text( "%s", SNIPPET.random_from_category( "amigara2" ).value_or( translation() ) );

    if( !query_bool( _( "Continue reading?" ) ) ) {
        return;
    }
    player_character.mod_moves( -to_moves<int>( 1_seconds ) * 0.3 );
    reset_terminal();
    print_line( _( "NEPower Mine%s Log" ), abs_sub.to_string() );
    print_text( "%s", SNIPPET.random_from_category( "amigara3" ).value_or( translation() ) );

    if( !query_bool( _( "Continue reading?" ) ) ) {
        return;
    }
    reset_terminal();
    for( int i = 0; i < 10; i++ ) {
        print_gibberish_line();
    }
    print_newline();
    print_newline();
    print_newline();
    print_line( _( "AMIGARA PROJECT" ) );
    print_newline();
    print_newline();
    if( !query_bool( _( "Continue reading?" ) ) ) {
        return;
    }
    player_character.mod_moves( -to_moves<int>( 1_seconds ) * 0.3 );
    reset_terminal();
    print_line( _( "SITE %d%d%d\n"
                   "PERTINENT FOREMAN LOGS WILL BE PREPENDED TO NOTES" ),
                abs_loc.x(), abs_loc.y(), std::abs( abs_loc.z() ) );
    print_text( "%s", SNIPPET.random_from_category( "amigara4" ).value_or( translation() ) );
    print_gibberish_line();
    print_gibberish_line();
    print_newline();
    print_error( _( "FILE CORRUPTED, PRESS ANY KEY…" ) );
    query_any();
    reset_terminal();
}

void computer_session::action_amigara_start()
{
    get_timed_events().add( timed_event_type::AMIGARA, calendar::turn + 10_seconds );
    // Disable this action to prevent further amigara events, which would lead to
    // further amigara monster, which would lead to further artifacts.
    comp.remove_option( COMPACT_AMIGARA_START );
}

void computer_session::action_complete_disable_external_power()
{
    for( mission *miss : get_avatar().get_active_missions() ) {
        static const mission_type_id commo_2 = mission_MISSION_OLD_GUARD_NEC_COMMO_2;
        if( miss->mission_id() == commo_2 ) {
            print_error( _( "--ACCESS GRANTED--" ) );
            print_error( _( "Mission Complete!" ) );
            miss->step_complete( 1 );
            query_any();
            return;
        }
    }
    print_error( _( "ACCESS DENIED" ) );
    query_any();
}

void computer_session::action_repeater_mod()
{
    avatar &pc = get_avatar();
    static const mission_type_id commo_3 = mission_MISSION_OLD_GUARD_NEC_COMMO_3;
    static const mission_type_id commo_4 = mission_MISSION_OLD_GUARD_NEC_COMMO_4;
    static const mission_type_id repeat = mission_MISSION_OLD_GUARD_REPEATER;
    static const mission_type_id repeatb = mission_MISSION_OLD_GUARD_REPEATER_BEGIN;
    if( pc.has_amount( itype_radio_repeater_mod, 1 ) ) {
        if( !( pc.has_mission_id( commo_3 ) || pc.has_mission_id( commo_4 ) ||
               pc.has_mission_id( repeat ) || pc.has_mission_id( repeatb ) ) ) {
            print_error( _( "You wouldn't gain anything from installing this repeater.  "
                            "However, someone else in the wasteland might reward you "
                            "for doing so." ) );
            query_any();
        }
        for( mission *miss : pc.get_active_missions() ) {
            if( miss->mission_id() == commo_3 || miss->mission_id() == commo_4 ) {
                miss->step_complete( 1 );
                print_line( _( "Repeater mod installed…" ) );
                print_line( _( "Mission Complete!" ) );
                pc.use_amount( itype_radio_repeater_mod, 1 );
                query_any();
                comp.options.clear();
                activate_failure( COMPFAIL_SHUTDOWN );
                break;
            } else if( miss->mission_id() == repeat || miss->mission_id() == repeatb ) {
                miss->step_complete( 1 );
                print_line( _( "Repeater mod installed…" ) );
                print_line( _( "Mission Complete!" ) );
                pc.use_amount( itype_radio_repeater_mod, 1 );
                query_any();
                comp.options.clear();
                activate_failure( COMPFAIL_SHUTDOWN );
                break;
            }
        }

    } else {
        print_error( _( "You do not have a repeater mod to install…" ) );
        query_any();
    }
}

void computer_session::action_download_software()
{
    mission *miss = mission::find( comp.mission_id );
    if( miss == nullptr ) {
        debugmsg( _( "Computer couldn't find its mission!" ) );
        return;
    }
    item software( miss->get_item_id(), calendar::turn_zero );
    units::ememory downloaded_size = software.ememory_size();

    if( item *const estorage = pick_estorage( downloaded_size ) ) {
        get_player_character().mod_moves( -to_moves<int>( 1_seconds ) * 0.3 );
        software.mission_id = comp.mission_id;
        estorage->put_in( software, pocket_type::E_FILE_STORAGE );
        print_line( string_format( _( "%s downloaded." ), software.tname() ) );
    } else {
        print_error( string_format( _( "Electronic storage device with %s free required!" ),
                                    units::display( downloaded_size ) ) );
    }
    query_any();
}

void computer_session::action_blood_anal()
{
    Character &player_character = get_player_character();
    player_character.mod_moves( -to_moves<int>( 1_seconds ) * 0.7 );
    map &here = get_map();
    for( const tripoint_bub_ms &dest : here.points_in_radius( player_character.pos_bub(), 2 ) ) {
        if( here.furn( dest ) == furn_f_centrifuge ) {
            map_stack items = here.i_at( dest );
            if( items.empty() ) {
                print_error( _( "ERROR: Please place sample in centrifuge." ) );
            } else if( items.size() > 1 ) {
                print_error( _( "ERROR: Please remove all but one sample from centrifuge." ) );
            } else if( items.only_item().empty() ) {
                print_error( _( "ERROR: Please only use container with blood sample." ) );
            } else if( items.only_item().legacy_front().typeId() != itype_blood &&
                       items.only_item().legacy_front().typeId() != itype_blood_tainted ) {
                print_error( _( "ERROR: Please only use blood samples." ) );
            } else if( items.only_item().legacy_front().rotten() ) {
                print_error( _( "ERROR: Please only use fresh blood samples." ) );
            }  else { // Success!
                const item &blood = items.only_item().legacy_front();
                const mtype *mt = blood.get_mtype();
                if( mt == nullptr || mt->id == mtype_id::NULL_ID() ) {
                    print_line( _( "Result: Human blood, no pathogens found." ) );
                } else if( mt->in_species( species_ZOMBIE ) ) {
                    if( mt->in_species( species_HUMAN ) ) {
                        print_line( _( "Result: Human blood.  Unknown pathogen found." ) );
                    } else {
                        print_line( _( "Result: Unknown blood type.  Unknown pathogen found." ) );
                    }
                    print_line( _( "Pathogen bonded to erythrocytes and leukocytes." ) );
                    item software( itype_software_blood_data, calendar::turn_zero );
                    units::ememory downloaded_size = software.ememory_size();
                    if( query_bool( _( "Download data?" ) ) ) {
                        if( item *const estorage = pick_estorage( downloaded_size ) ) {
                            item software( itype_software_blood_data, calendar::turn_zero );
                            estorage->put_in( software, pocket_type::E_FILE_STORAGE );
                            print_line( string_format( _( "%s downloaded." ), software.tname() ) );
                        } else {
                            print_error( string_format( _( "Electronic storage device with %s free required!" ),
                                                        units::display( downloaded_size ) ) );
                        }
                    }
                } else {
                    print_line( _( "Result: Unknown blood type.  Test non-conclusive." ) );
                }
            }
        }
    }
    query_any( _( "Press any key…" ) );
}

void computer_session::action_data_anal()
{
    Character &player_character = get_player_character();
    player_character.mod_moves( -to_moves<int>( 1_seconds ) * 0.3 );
    map &here = get_map();
    for( const tripoint_bub_ms &dest : here.points_in_radius( player_character.pos_bub(), 2 ) ) {
        if( here.ter( dest ) == ter_t_floor_blue ) {
            print_error( _( "PROCESSING DATA" ) );
            map_stack items = here.i_at( dest );
            if( items.empty() ) {
                print_error( _( "ERROR: Please place memory bank in scan area." ) );
            } else if( items.size() > 1 ) {
                print_error( _( "ERROR: Please only scan one item at a time." ) );
            } else if( items.only_item().typeId() != itype_usb_drive &&
                       items.only_item().typeId() != itype_black_box ) {
                print_error( _( "ERROR: Memory bank destroyed or not present." ) );
            } else if( items.only_item().typeId() == itype_usb_drive &&
                       items.only_item().empty() ) {
                print_error( _( "ERROR: Memory bank is empty." ) );
            } else { // Success!
                if( items.only_item().typeId() == itype_black_box ) {
                    print_line( _( "Memory Bank: Military Hexron Encryption\nPrinting Transcript\n" ) );
                    item transcript( itype_black_box_transcript, calendar::turn );
                    here.add_item_or_charges( player_character.pos_bub(), transcript );
                } else {
                    print_line( _( "Memory Bank: Unencrypted\nNothing of interest.\n" ) );
                }
            }
        }
    }
    query_any( _( "Press any key…" ) );
}

void computer_session::action_disconnect()
{
    reset_terminal();
    print_line( _( "\n"
                   "ERROR: NETWORK DISCONNECT\n"
                   "UNABLE TO REACH NETWORK ROUTER OR PROXY.  PLEASE CONTACT YOUR\n"
                   "SYSTEM ADMINISTRATOR TO RESOLVE THIS ISSUE.\n"
                   "  \n" ) );
    query_any( _( "Press any key to continue…" ) );
}

void computer_session::action_emerg_mess()
{
    print_line( _( "GREETINGS CITIZEN.  A BIOLOGICAL ATTACK HAS TAKEN PLACE AND A STATE OF\n"
                   "EMERGENCY HAS BEEN DECLARED.  EMERGENCY PERSONNEL WILL BE AIDING YOU\n"
                   "SHORTLY.  TO ENSURE YOUR SAFETY PLEASE FOLLOW THE STEPS BELOW.\n"
                   "\n"
                   "1. DO NOT PANIC.\n"
                   "2. REMAIN INSIDE THE BUILDING.\n"
                   "3. SEEK SHELTER IN THE BASEMENT.\n"
                   "4. USE PROVIDED GAS MASKS.\n"
                   "5. AWAIT FURTHER INSTRUCTIONS.\n"
                   "\n"
                   "  \n" ) );
    query_any( _( "Press any key to continue…" ) );
}

void computer_session::action_tower_unresponsive()
{
    print_line( _( "  WARNING, RADIO TOWER IS UNRESPONSIVE.\n"
                   "  \n"
                   "  BACKUP POWER INSUFFICIENT TO MEET BROADCASTING REQUIREMENTS.\n"
                   "  IN THE EVENT OF AN EMERGENCY, CONTACT LOCAL NATIONAL GUARD\n"
                   "  UNITS TO RECEIVE PRIORITY WHEN GENERATORS ARE BEING DEPLOYED.\n"
                   "  \n"
                   "  \n" ) );
    query_any( _( "Press any key to continue…" ) );
}

void computer_session::action_sr1_mess()
{
    reset_terminal();
    print_text( "%s", SNIPPET.random_from_category( "sr1_mess" ).value_or( translation() ) );
    query_any( _( "Press any key to continue…" ) );
}

void computer_session::action_sr2_mess()
{
    reset_terminal();
    print_text( "%s", SNIPPET.random_from_category( "sr2_mess" ).value_or( translation() ) );
    query_any( _( "Press any key to continue…" ) );
}

void computer_session::action_sr3_mess()
{
    reset_terminal();
    print_text( "%s", SNIPPET.random_from_category( "sr3_mess" ).value_or( translation() ) );
    query_any( _( "Press any key to continue…" ) );
}

void computer_session::action_sr4_mess()
{
    reset_terminal();
    print_text( "%s", SNIPPET.random_from_category( "sr4_mess" ).value_or( translation() ) );
    query_any( _( "Press any key to continue…" ) );
}

void computer_session::action_srcf_1_mess()
{
    reset_terminal();
    print_text( "%s", SNIPPET.random_from_category( "scrf_1_mess" ).value_or( translation() ) );
    query_any( _( "Press any key to continue…" ) );
}

void computer_session::action_srcf_2_mess()
{
    reset_terminal();
    print_text( "%s", SNIPPET.random_from_category( "scrf_2_1_mess" ).value_or( translation() ) );
    query_any( _( "Press any key to continue…" ) );
    reset_terminal();
    print_text( "%s", SNIPPET.random_from_category( "scrf_2_2_mess" ).value_or( translation() ) );
    query_any( _( "Press any key to continue…" ) );
}

void computer_session::action_srcf_3_mess()
{
    reset_terminal();
    print_text( "%s", SNIPPET.random_from_category( "scrf_3_mess" ).value_or( translation() ) );
    query_any( _( "Press any key to continue…" ) );
}

void computer_session::action_srcf_seal_order()
{
    reset_terminal();
    print_text( "%s", SNIPPET.random_from_category( "scrf_seal_order" ).value_or( translation() ) );
    query_any( _( "Press any key to continue…" ) );
}

void computer_session::action_srcf_seal()
{
    get_event_bus().send<event_type::seals_hazardous_material_sarcophagus>();
    print_line( _( "Charges Detonated" ) );
    print_line( _( "Backup Generator Power Failing" ) );
    print_line( _( "Evacuate Immediately" ) );
    add_msg( m_warning, _( "Evacuate Immediately!" ) );
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_on_zlevel() ) {
        const ter_id &t = here.ter( p );
        if( t == ter_t_elevator || t == ter_t_vat ) {
            here.make_rubble( p, furn_f_rubble_rock, true );
            explosion_handler::explosion( &get_player_character(), p, 40, 0.7, true );
        } else if( t == ter_t_wall_glass || t == ter_t_sewage_pipe ||
                   t == ter_t_sewage || t == ter_t_grate ) {
            here.make_rubble( p, furn_f_rubble_rock, true );
        } else if( t == ter_t_sewage_pump ) {
            here.make_rubble( p, furn_f_rubble_rock, true );
            explosion_handler::explosion( &get_player_character(), p, 50, 0.7, true );
        }
    }
    comp.options.clear(); // Disable the terminal.
    activate_failure( COMPFAIL_SHUTDOWN );
}

void computer_session::action_srcf_elevator()
{
    Character &player_character = get_player_character();
    map &here = get_map();
    tripoint_bub_ms surface_elevator;
    tripoint_bub_ms underground_elevator;
    bool is_surface_elevator_on = false;
    bool is_surface_elevator_exist = false;
    bool is_underground_elevator_on = false;
    bool is_underground_elevator_exist = false;

    for( const tripoint_bub_ms &p : here.points_on_zlevel( 0 ) ) {
        const ter_id &t = here.ter( p );
        if( t == ter_t_elevator_control_off || t == ter_t_elevator_control ) {
            surface_elevator = p;
            is_surface_elevator_on = t == ter_t_elevator_control;
            is_surface_elevator_exist = true;
        }
    }
    for( const tripoint_bub_ms &p : here.points_on_zlevel( -2 ) ) {
        const ter_id &t = here.ter( p );
        if( t == ter_t_elevator_control_off || t == ter_t_elevator_control ) {
            underground_elevator = p;
            is_underground_elevator_on = t == ter_t_elevator_control;
            is_underground_elevator_exist = true;
        }
    }

    //If some are destroyed
    if( !is_surface_elevator_exist || !is_underground_elevator_exist ) {
        reset_terminal();
        print_error(
            _( "\nElevator control network unreachable!\n\n" ) );
    }

    //If both are disabled try to enable
    else if( !is_surface_elevator_on && !is_underground_elevator_on ) {
        if( !player_character.has_amount( itype_sarcophagus_access_code, 1 ) ) {
            print_error( _( "Access code required!\n\n" ) );
        } else {
            player_character.use_amount( itype_sarcophagus_access_code, 1 );
            here.ter_set( surface_elevator, ter_t_elevator_control );
            is_surface_elevator_on = true;
            here.ter_set( underground_elevator, ter_t_elevator_control );
            is_underground_elevator_on = true;
        }
    }

    //If only one is enabled, enable the other one. Fix for before this change
    else if( is_surface_elevator_on && !is_underground_elevator_on && is_underground_elevator_exist ) {
        here.ter_set( underground_elevator, ter_t_elevator_control );
        is_underground_elevator_on = true;
    }

    else if( is_underground_elevator_on && !is_surface_elevator_on && is_surface_elevator_exist ) {
        here.ter_set( surface_elevator, ter_t_elevator_control );
        is_surface_elevator_on = true;
    }

    //If the elevator is working
    if( is_surface_elevator_on && is_underground_elevator_on ) {
        reset_terminal();
        print_line(
            _( "\nPower:         Backup Only\nRadiation Level:  Very Dangerous\nOperational:   Overridden\n\n" ) );
    }

    query_any( _( "Press any key…" ) );
}

//irradiates food at ter_t_rad_platform, adds radiation
void computer_session::action_irradiator()
{
    Character &player_character = get_player_character();
    player_character.mod_moves( -to_moves<int>( 1_seconds ) * 0.3 );
    bool error = false;
    bool platform_exists = false;
    map &here = get_map();
    for( const tripoint_bub_ms &dest : here.points_in_radius( player_character.pos_bub(), 10 ) ) {
        if( here.ter( dest ) == ter_t_rad_platform ) {
            platform_exists = true;
            map_stack ms = here.i_at( dest );
            if( ms.empty() ) {
                print_error( _( "ERROR: Processing platform empty." ) );
            } else {
                player_character.mod_moves( -to_moves<int>( 3_seconds ) );
                for( auto it = ms.begin(); it != ms.end(); ++it ) {
                    // actual food processing
                    if( !it->rotten() ) {
                        it->set_flag( flag_IRRADIATED );
                    }
                    // critical failure - radiation spike sets off electronic detonators
                    if( it->typeId() == itype_mininuke || it->typeId() == itype_mininuke_act ||
                        it->typeId() == itype_c4 ) {
                        explosion_handler::explosion( &get_player_character(), dest, 40 );
                        reset_terminal();
                        print_error( _( "WARNING [409]: Primary sensors offline!" ) );
                        print_error( _( "  >> Initialize secondary sensors: Geiger profiling…" ) );
                        print_error( _( "  >> Radiation spike detected!\n" ) );
                        print_error( _( "WARNING [912]: Catastrophic malfunction!  Contamination detected!" ) );
                        print_error( _( "EMERGENCY PROCEDURE [1]:  Evacuate.  Evacuate.  Evacuate.\n" ) );
                        sounds::sound( player_character.pos_bub(), 30, sounds::sound_t::alarm, _( "an alarm sound!" ),
                                       false,
                                       "environment",
                                       "alarm" );
                        here.i_rem( dest, it );
                        here.make_rubble( dest );
                        here.propagate_field( dest, fd_nuke_gas, 100, 3 );
                        here.translate_radius( ter_t_water_pool, ter_t_sewage, 8.0, dest, true );
                        here.adjust_radiation( dest, rng( 50, 500 ) );
                        for( const tripoint_bub_ms &radorigin : here.points_in_radius( dest, 5 ) ) {
                            here.adjust_radiation( radorigin, rng( 50, 500 ) / ( rl_dist( radorigin,
                                                   dest ) > 0 ? rl_dist( radorigin, dest ) : 1 ) );
                        }
                        if( here.pl_sees( dest, 10 ) ) {
                            player_character.irradiate( rng_float( 50, 250 ) / rl_dist( player_character.pos_bub(), dest ) );
                        } else {
                            player_character.irradiate( rng_float( 20, 100 ) / rl_dist( player_character.pos_bub(), dest ) );
                        }
                        query_any( _( "EMERGENCY SHUTDOWN!  Press any key…" ) );
                        error = true;
                        comp.options.clear(); // Disable the terminal.
                        activate_failure( COMPFAIL_SHUTDOWN );
                        break;
                    }
                    here.adjust_radiation( dest, rng( 20, 50 ) );
                    for( const tripoint_bub_ms &radorigin : here.points_in_radius( dest, 5 ) ) {
                        here.adjust_radiation( radorigin, rng( 20, 50 ) / ( rl_dist( radorigin,
                                               dest ) > 0 ? rl_dist( radorigin, dest ) : 1 ) );
                    }
                    // if unshielded, rad source irradiates player directly, reduced by distance to source
                    if( here.pl_sees( dest, 10 ) ) {
                        player_character.irradiate( rng_float( 5, 25 ) / rl_dist( player_character.pos_bub(), dest ) );
                    }
                }
                if( !error && platform_exists ) {
                    print_error( _( "PROCESSING…  CYCLE COMPLETE." ) );
                    print_error( _( "GEIGER COUNTER @ PLATFORM: %s mSv/h." ), here.get_radiation( dest ) );
                }
            }
        }
    }
    if( !platform_exists ) {
        print_error(
            _( "CRITICAL ERROR…  RADIATION PLATFORM UNRESPONSIVE.  COMPLY TO PROCEDURE RP_M_01_rev.03." ) );
    }
    if( !error ) {
        query_any( _( "Press any key…" ) );
    }
}

// geiger counter for irradiator, primary measurement at ter_t_rad_platform, secondary at player location
void computer_session::action_geiger()
{
    Character &player_character = get_player_character();
    player_character.mod_moves( -to_moves<int>( 1_seconds ) * 0.3 );
    tripoint_bub_ms platform;
    bool source_exists = false;
    int sum_rads = 0;
    int peak_rad = 0;
    int tiles_counted = 0;
    map &here = get_map();
    print_error( _( "RADIATION MEASUREMENTS:" ) );
    for( const tripoint_bub_ms &dest : here.points_in_radius( player_character.pos_bub(), 10 ) ) {
        if( here.ter( dest ) == ter_t_rad_platform ) {
            source_exists = true;
            platform = dest;
        }
    }
    if( source_exists ) {
        for( const tripoint_bub_ms &dest : here.points_in_radius( platform, 3 ) ) {
            sum_rads += here.get_radiation( dest );
            tiles_counted ++;
            if( here.get_radiation( dest ) > peak_rad ) {
                peak_rad = here.get_radiation( dest );
            }
            sum_rads += here.get_radiation( platform );
            tiles_counted ++;
            if( here.get_radiation( platform ) > peak_rad ) {
                peak_rad = here.get_radiation( platform );
            }
        }
        print_error( _( "GEIGER COUNTER @ ZONE:… AVG %s mSv/h." ), sum_rads / tiles_counted );
        print_error( _( "GEIGER COUNTER @ ZONE:… MAX %s mSv/h." ), peak_rad );
        print_newline();
    }
    print_error( _( "GEIGER COUNTER @ CONSOLE:… %s mSv/h." ),
                 here.get_radiation( player_character.pos_bub() ) );
    print_error( _( "PERSONAL DOSIMETRY:… %s mSv." ), player_character.get_rad() );
    print_newline();
    query_any( _( "Press any key…" ) );
}

// imitates item movement through conveyor belt through 3 different loading/unloading bays
// ensure only bay of each type in range
void computer_session::action_conveyor()
{
    Character &player_character = get_player_character();
    player_character.mod_moves( -to_moves<int>( 3_seconds ) );
    tripoint_bub_ms loading; // red tile = loading bay
    tripoint_bub_ms unloading; // green tile = unloading bay
    tripoint_bub_ms platform; // radiation platform = middle point
    bool l_exists = false;
    bool u_exists = false;
    bool p_exists = false;
    map &here = get_map();
    for( const tripoint_bub_ms &dest : here.points_in_radius( player_character.pos_bub(), 10 ) ) {
        const ter_id &t = here.ter( dest );
        if( t == ter_t_rad_platform ) {
            platform = dest;
            p_exists = true;
        } else if( t == ter_t_floor_red ) {
            loading = dest;
            l_exists = true;
        } else if( t == ter_t_floor_green ) {
            unloading = dest;
            u_exists = true;
        }
    }
    if( !l_exists || !p_exists || !u_exists ) {
        print_error( _( "Conveyor belt malfunction.  Consult maintenance team." ) );
        query_any( _( "Press any key…" ) );
        return;
    }
    map_stack items = here.i_at( platform );
    if( !items.empty() ) {
        print_line( _( "Moving items: PLATFORM --> UNLOADING BAY." ) );
    } else {
        print_line( _( "No items detected at: PLATFORM." ) );
    }
    for( const item &it : items ) {
        here.add_item_or_charges( unloading, it );
    }
    here.i_clear( platform );
    items = here.i_at( loading );
    if( !items.empty() ) {
        print_line( _( "Moving items: LOADING BAY --> PLATFORM." ) );
    } else {
        print_line( _( "No items detected at: LOADING BAY." ) );
    }
    for( const item &it : items ) {
        if( !it.made_of_from_type( phase_id::LIQUID ) ) {
            here.add_item_or_charges( platform, it );
        }
    }
    here.i_clear( loading );
    query_any( _( "Conveyor belt cycle complete.  Press any key…" ) );
}

// toggles reinforced glass shutters open->closed and closed->open depending on their current state
void computer_session::action_shutters()
{
    Character &player_character = get_player_character();
    player_character.mod_moves( -to_moves<int>( 3_seconds ) );
    get_map().translate_radius( ter_t_reinforced_glass_shutter_open, ter_t_reinforced_glass_shutter,
                                8.0,
                                player_character.pos_bub(),
                                true, true );
    query_any( _( "Toggling shutters.  Press any key…" ) );
}

// extract radiation source material from irradiator
void computer_session::action_extract_rad_source()
{
    Character &player_character = get_player_character();
    if( query_yn( _( "Operation irreversible.  Extract radioactive material?" ) ) ) {
        player_character.mod_moves( -to_moves<int>( 3_seconds ) );
        tripoint_bub_ms platform;
        bool p_exists = false;
        map &here = get_map();
        for( const tripoint_bub_ms &dest : here.points_in_radius( player_character.pos_bub(), 10 ) ) {
            if( here.ter( dest ) == ter_t_rad_platform ) {
                platform = dest;
                p_exists = true;
            }
        }
        if( p_exists ) {
            here.spawn_item( platform, itype_cobalt_60, rng( 8, 15 ) );
            here.translate_radius( ter_t_rad_platform, ter_t_concrete, 8.0, player_character.pos_bub(), true );
            comp.remove_option( COMPACT_IRRADIATOR );
            comp.remove_option( COMPACT_EXTRACT_RAD_SOURCE );
            query_any( _( "Extraction sequence complete…  Press any key." ) );
        } else {
            query_any( _( "ERROR!  Radiation platform unresponsive…  Press any key." ) );
        }
    }
}

// remove shock vent fields; check for existing plutonium generators in radius
void computer_session::action_deactivate_shock_vent()
{
    Character &player_character = get_player_character();
    player_character.mod_moves( -to_moves<int>( 1_seconds ) * 0.3 );
    bool has_vent = false;
    map &here = get_map();
    for( const tripoint_bub_ms &dest : here.points_in_radius( player_character.pos_bub(), 10 ) ) {
        if( here.get_field( dest, fd_shock_vent ) != nullptr ) {
            has_vent = true;
        }
        here.remove_field( dest, fd_shock_vent );
    }
    print_line( _( "Initiating POWER-DIAG ver.2.34…" ) );
    if( has_vent ) {
        print_error( _( "Short circuit detected!" ) );
        print_error( _( "Short circuit rerouted." ) );
        print_error( _( "Fuse reset." ) );
        print_error( _( "Ground re-enabled." ) );
    } else {
        print_line( _( "Internal power lines status: 85%% OFFLINE.  Reason: DAMAGED." ) );
    }
    print_line(
        _( "External power lines status: 100%% OFFLINE.  Reason: NO EXTERNAL POWER DETECTED." ) );
    print_line( _( "Backup power status: STANDBY MODE." ) );
    query_any( _( "Press any key…" ) );
}

void computer_session::activate_random_failure()
{
    comp.next_attempt = calendar::turn + 45_minutes;
    static const computer_failure default_failure( COMPFAIL_SHUTDOWN );
    const computer_failure &fail = random_entry( comp.failures, default_failure );
    activate_failure( fail.type );
}

const std::map<computer_failure_type, void( computer_session::* )()>
computer_session::computer_failure_functions = {
    { COMPFAIL_ALARM, &computer_session::failure_alarm },
    { COMPFAIL_AMIGARA, &computer_session::failure_amigara },
    { COMPFAIL_DAMAGE, &computer_session::failure_damage },
    { COMPFAIL_DESTROY_BLOOD, &computer_session::failure_destroy_blood },
    { COMPFAIL_DESTROY_DATA, &computer_session::failure_destroy_data },
    { COMPFAIL_MANHACKS, &computer_session::failure_manhacks },
    { COMPFAIL_PUMP_EXPLODE, &computer_session::failure_pump_explode },
    { COMPFAIL_PUMP_LEAK, &computer_session::failure_pump_leak },
    { COMPFAIL_SECUBOTS, &computer_session::failure_secubots },
    { COMPFAIL_SHUTDOWN, &computer_session::failure_shutdown },
};

void computer_session::activate_failure( computer_failure_type fail )
{
    const auto it = computer_failure_functions.find( fail );
    if( it != computer_failure_functions.end() ) {
        ( this->*( it->second ) )();
    }
}

void computer_session::failure_shutdown()
{
    bool found_tile = false;
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_in_radius( get_player_character().pos_bub(), 1 ) ) {
        if( here.has_flag( ter_furn_flag::TFLAG_CONSOLE, p ) ) {
            here.furn_set( p, furn_f_console_broken );
            add_msg( m_bad, _( "The console shuts down." ) );
            found_tile = true;
        }
    }
    if( found_tile ) {
        return;
    }
    for( const tripoint_bub_ms &p : here.points_on_zlevel() ) {
        if( here.has_flag( ter_furn_flag::TFLAG_CONSOLE, p ) ) {
            here.furn_set( p, furn_f_console_broken );
            add_msg( m_bad, _( "The console shuts down." ) );
        }
    }
}

void computer_session::failure_alarm()
{
    Character &player_character = get_player_character();
    get_event_bus().send<event_type::triggers_alarm>( player_character.getID() );
    sounds::sound( player_character.pos_bub(), 60, sounds::sound_t::alarm, _( "an alarm sound!" ),
                   false,
                   "environment",
                   "alarm" );
}

void computer_session::failure_manhacks()
{
    int num_robots = rng( 4, 8 );
    const tripoint_range<tripoint_bub_ms> range =
        get_map().points_in_radius( get_player_character().pos_bub(), 3 );
    for( int i = 0; i < num_robots; i++ ) {
        if( g->place_critter_within( mon_manhack, range ) ) {
            add_msg( m_warning, _( "Manhacks drop from compartments in the ceiling." ) );
        }
    }
}

void computer_session::failure_secubots()
{
    const tripoint_range<tripoint_bub_ms> range =
        get_map().points_in_radius( get_player_character().pos_bub(), 3 );
    if( g->place_critter_within( mon_secubot, range ) ) {
        add_msg( m_warning, _( "A secubot emerges from a compartment in the floor." ) );
    }
}

void computer_session::failure_damage()
{
    add_msg( m_neutral, _( "The console shocks you." ) );
    Character &player_character = get_player_character();
    if( player_character.is_elec_immune() ) {
        add_msg( m_good, _( "You're protected from electric shocks." ) );
    } else {
        add_msg( m_bad, _( "Your body is damaged by the electric shock!" ) );
        player_character.hurtall( rng( 1, 10 ), nullptr );
    }
}

void computer_session::failure_pump_explode()
{
    add_msg( m_warning, _( "The pump explodes!" ) );
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_on_zlevel() ) {
        if( here.ter( p ) == ter_t_sewage_pump ) {
            here.make_rubble( p );
            explosion_handler::explosion( &get_player_character(), p, 10 );
        }
    }
}

void computer_session::failure_pump_leak()
{
    add_msg( m_warning, _( "Sewage leaks!" ) );
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_on_zlevel() ) {
        if( here.ter( p ) != ter_t_sewage_pump ) {
            continue;
        }
        const int leak_size = rng( 4, 10 );
        for( int i = 0; i < leak_size; i++ ) {
            std::vector<tripoint_bub_ms> next_move;
            if( here.passable( p + point::north ) ) {
                next_move.push_back( p + point::north );
            }
            if( here.passable( p + point::east ) ) {
                next_move.push_back( p + point::east );
            }
            if( here.passable( p + point::south ) ) {
                next_move.push_back( p + point::south );
            }
            if( here.passable( p + point::west ) ) {
                next_move.push_back( p + point::west );
            }
            if( next_move.empty() ) {
                break;
            }
            here.ter_set( random_entry( next_move ), ter_t_sewage );
        }
    }
}

void computer_session::failure_amigara()
{
    get_timed_events().add( timed_event_type::AMIGARA, calendar::turn + 30_seconds );
    get_player_character().add_effect( effect_amigara, 2_minutes );
    map &here = get_map();
    explosion_handler::explosion( &get_player_character(),
                                  tripoint_bub_ms( rng( 0, MAPSIZE_X ), rng( 0, MAPSIZE_Y ), here.get_abs_sub().z() ), 10, 0.7, false,
                                  10 );
    explosion_handler::explosion( &get_player_character(),
                                  tripoint_bub_ms( rng( 0, MAPSIZE_X ), rng( 0, MAPSIZE_Y ), here.get_abs_sub().z() ), 10, 0.7, false,
                                  10 );
    comp.remove_option( COMPACT_AMIGARA_START );
}

void computer_session::failure_destroy_blood()
{
    print_error( _( "ERROR: Disruptive Spin" ) );
    map &here = get_map();
    for( const tripoint_bub_ms &dest : here.points_in_radius( get_player_character().pos_bub(), 2 ) ) {
        if( here.furn( dest ) == furn_f_centrifuge ) {
            map_stack items = here.i_at( dest );
            if( items.empty() ) {
                print_error( _( "ERROR: Please place sample in centrifuge." ) );
            } else if( items.size() > 1 ) {
                print_error( _( "ERROR: Please remove all but one sample from centrifuge." ) );
            } else if( items.only_item().typeId() != itype_vacutainer ) {
                print_error( _( "ERROR: Please use blood-contained samples." ) );
            } else if( items.only_item().empty() ) {
                print_error( _( "ERROR: Blood draw kit, empty." ) );
            } else if( items.only_item().legacy_front().typeId() != itype_blood &&
                       items.only_item().legacy_front().typeId() != itype_blood_tainted ) {
                print_error( _( "ERROR: Please only use blood samples." ) );
            } else {
                print_error( _( "ERROR: Blood sample destroyed." ) );
                here.i_clear( dest );
            }
        }
    }
    query_any();
}

void computer_session::failure_destroy_data()
{
    print_error( _( "ERROR: ACCESSING DATA MALFUNCTION" ) );
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_in_radius( get_player_character().pos_bub(), 2 ) ) {
        if( here.ter( p ) == ter_t_floor_blue ) {
            map_stack items = here.i_at( p );
            if( items.empty() ) {
                print_error( _( "ERROR: Please place memory bank in scan area." ) );
            } else if( items.size() > 1 ) {
                print_error( _( "ERROR: Please only scan one item at a time." ) );
            } else if( items.only_item().typeId() != itype_usb_drive ) {
                print_error( _( "ERROR: Memory bank destroyed or not present." ) );
            } else if( items.only_item().empty() ) {
                print_error( _( "ERROR: Memory bank is empty." ) );
            } else {
                print_error( _( "ERROR: Data bank destroyed." ) );
                here.i_clear( p );
            }
        }
    }
    query_any();
}

void computer_session::action_emerg_ref_center()
{
    reset_terminal();
    print_line( _( "SEARCHING FOR NEAREST REFUGEE CENTER, PLEASE WAIT…" ) );

    tripoint_abs_omt mission_target;
    avatar &player_character = get_avatar();
    // Check completed missions too, so people can't repeatedly get the mission.
    const std::vector<mission *> completed_missions = player_character.get_completed_missions();
    std::vector<mission *> missions = player_character.get_active_missions();
    missions.insert( missions.end(), completed_missions.begin(), completed_missions.end() );

    auto is_refugee_mission = [ &mission_target ]( mission * mission ) {
        if( mission->get_type().id == mission_MISSION_REACH_REFUGEE_CENTER ) {
            mission_target = mission->get_target();
            return true;
        }

        return false;
    };

    const bool has_mission = std::any_of( missions.begin(), missions.end(), is_refugee_mission );

    if( !has_mission ) {
        mission *new_mission =
            mission::reserve_new( mission_MISSION_REACH_REFUGEE_CENTER, character_id() );
        new_mission->assign( player_character );
        mission_target = new_mission->get_target();
    }

    //~555-0164 is a fake phone number in the US, please replace it with a number that will not cause issues in your locale if possible.
    print_line( _( "\nREFUGEE CENTER FOUND!  LOCATION: %d %s\n\n"
                   "IF YOU HAVE ANY FEEDBACK CONCERNING YOUR VISIT PLEASE CONTACT\n"
                   "THE DEPARTMENT OF EMERGENCY MANAGEMENT PUBLIC AFFAIRS OFFICE.\n"
                   "THE LOCAL OFFICE CAN BE REACHED BETWEEN THE HOURS OF 9AM AND\n"
                   "4PM AT 555-0164.\n"
                   "\n"
                   "IF YOU WOULD LIKE TO SPEAK WITH SOMEONE IN PERSON OR WOULD LIKE\n"
                   "TO WRITE US A LETTER PLEASE SEND IT TO…\n" ),
                rl_dist( player_character.pos_abs_omt(), mission_target ),
                direction_name_short(
                    direction_from( player_character.pos_abs_omt(), mission_target ) ) );

    query_any( _( "Press any key to continue…" ) );
    reset_terminal();
}

template<typename ...Args>
bool computer_session::query_bool( const std::string &text, Args &&... args )
{
    return query_ynq( text, std::forward<Args>( args )... ) == ynq::yes;
}

template<typename ...Args>
bool computer_session::query_any( const std::string &text, Args &&... args )
{
    print_indented_line( 0, width, text, std::forward<Args>( args )... );
    return query_any();
}

bool computer_session::query_any()
{
    ui_manager::redraw();
    inp_mngr.wait_for_any_key();
    return true;
}

template<typename ...Args>
computer_session::ynq computer_session::query_ynq( const std::string &text, Args &&... args )
{
    const std::string formatted_text = string_format( text, std::forward<Args>( args )... );
    const bool force_uc = get_option<bool>( "FORCE_CAPITAL_YN" );
    const auto &allow_key = force_uc ? input_context::disallow_lower_case_or_non_modified_letters
                            : input_context::allow_all_keys;
    input_context ctxt( "YESNOQUIT", keyboard_mode::keycode );
    ctxt.register_action( "YES" );
    ctxt.register_action( "NO" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    print_indented_line( 0, width, force_uc && !is_keycode_mode_supported()
                         //~ 1st: query string, 2nd-4th: keybinding descriptions
                         ? pgettext( "query_ynq", "%s %s, %s, %s (Case sensitive)" )
                         //~ 1st: query string, 2nd-4th: keybinding descriptions
                         : pgettext( "query_ynq", "%s %s, %s, %s" ),
                         formatted_text,
                         ctxt.describe_key_and_name( "YES", allow_key ),
                         ctxt.describe_key_and_name( "NO", allow_key ),
                         ctxt.describe_key_and_name( "QUIT", allow_key ) );

    do {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( allow_key( ctxt.get_raw_input() ) ) {
            if( action == "YES" ) {
                return ynq::yes;
            } else if( action == "NO" ) {
                return ynq::no;
            } else if( action == "QUIT" ) {
                return ynq::quit;
            }
        }
    } while( true );
}

void computer_session::refresh()
{
    werase( win );
    draw_border( win );
    for( size_t i = 0; i < lines.size(); ++i ) {
        nc_color dummy = c_green;
        print_colored_text( win, point( left + lines[i].first, top + static_cast<int>( i ) ),
                            dummy, dummy, lines[i].second );
    }
    wnoutrefresh( win );
}

template<typename ...Args>
void computer_session::print_indented_line( const int indent, const int text_width,
        const std::string &text, Args &&... args )
{
    if( text_width <= 0 || height <= 0 ) {
        return;
    }
    const size_t uheight = static_cast<size_t>( height );
    const std::string formatted_text = string_format( text, std::forward<Args>( args )... );
    std::vector<std::string> folded = foldstring( formatted_text, text_width );
    if( folded.size() >= uheight ) {
        lines.clear();
    } else if( lines.size() + folded.size() > uheight ) {
        lines.erase( lines.begin(), lines.begin() + ( lines.size() + folded.size() - uheight ) );
    }
    lines.reserve( uheight );
    for( auto it = folded.size() >= uheight ? folded.end() - uheight : folded.begin();
         it < folded.end(); ++it ) {
        lines.emplace_back( indent, *it );
    }
}

template<typename ...Args>
void computer_session::print_line( const std::string &text, Args &&... args )
{
    print_indented_line( 0, width, text, std::forward<Args>( args )... );
}

template<typename ...Args>
void computer_session::print_error( const std::string &text, Args &&... args )
{
    const std::string formatted_text = string_format( text, std::forward<Args>( args )... );
    print_indented_line( 0, width, "%s", colorize( formatted_text, c_red ) );
}

template<typename ...Args>
void computer_session::print_text( const std::string &text, Args &&... args )
{
    print_indented_line( 1, width - 2, text, std::forward<Args>( args )... );
    print_newline();
}

void computer_session::print_gibberish_line()
{
    std::string gibberish;
    int length = rng( 50, 70 );
    for( int i = 0; i < length; i++ ) {
        switch( rng( 0, 4 ) ) {
            case 0:
                gibberish += static_cast<char>( '0' + rng( 0, 9 ) );
                break;
            case 1:
            case 2:
                gibberish += static_cast<char>( 'a' + rng( 0, 25 ) );
                break;
            case 3:
            case 4:
                gibberish += static_cast<char>( 'A' + rng( 0, 25 ) );
                break;
        }
    }
    print_indented_line( 0, width, "%s", colorize( gibberish, c_yellow ) );
}

void computer_session::reset_terminal()
{
    lines.clear();
}

void computer_session::print_newline()
{
    if( height <= 0 ) {
        return;
    }
    const size_t uheight = static_cast<size_t>( height );
    if( lines.size() >= uheight ) {
        lines.erase( lines.begin(), lines.end() - ( uheight - 1 ) );
    }
    lines.emplace_back();
}
