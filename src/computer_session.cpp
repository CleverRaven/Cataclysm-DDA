#include "computer_session.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "avatar.h"
#include "calendar.h"
#include "character_id.h"
#include "colony.h"
#include "color.h"
#include "coordinate_conversions.h"
#include "creature.h"
#include "debug.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "explosion.h"
#include "field_type.h"
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "input.h"
#include "int_id.h"
#include "item.h"
#include "item_contents.h"
#include "item_factory.h"
#include "item_location.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "mission.h"
#include "monster.h"
#include "mtype.h"
#include "optional.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "player.h"
#include "point.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_id.h"
#include "text_snippets.h"
#include "timed_event.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "ui.h"
#include "ui_manager.h"

static const efftype_id effect_amigara( "amigara" );

static const itype_id itype_black_box( "black_box" );
static const itype_id itype_blood( "blood" );
static const itype_id itype_c4( "c4" );
static const itype_id itype_cobalt_60( "cobalt_60" );
static const itype_id itype_mininuke( "mininuke" );
static const itype_id itype_mininuke_act( "mininuke_act" );
static const itype_id itype_radio_repeater_mod( "radio_repeater_mod" );
static const itype_id itype_sarcophagus_access_code( "sarcophagus_access_code" );
static const itype_id itype_sewage( "sewage" );
static const itype_id itype_usb_drive( "usb_drive" );
static const itype_id itype_vacutainer( "vacutainer" );

static const skill_id skill_computer( "computer" );

static const species_id species_HUMAN( "HUMAN" );
static const species_id species_ZOMBIE( "ZOMBIE" );

static const mtype_id mon_manhack( "mon_manhack" );
static const mtype_id mon_secubot( "mon_secubot" );

static const std::string flag_CONSOLE( "CONSOLE" );

static catacurses::window init_window()
{
    const int width = FULL_SCREEN_WIDTH;
    const int height = FULL_SCREEN_HEIGHT;
    const int x = std::max( 0, ( TERMX - width ) / 2 );
    const int y = std::max( 0, ( TERMY - height ) / 2 );
    return catacurses::newwin( height, width, point( x, y ) );
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
        const int x = std::max( 0, ( TERMX - width ) / 2 );
        const int y = std::max( 0, ( TERMY - height ) / 2 );
        win = catacurses::newwin( height, width, point( x, y ) );
        ui.position_from_window( win );
    } );
    ui.mark_resize();
    ui.on_redraw( [this]( const ui_adaptor & ) {
        refresh();
    } );

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
                if( !hack_attempt( g->u ) ) {
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
            computer_menu.addentry( i, true, MENU_AUTOASSIGN, comp.options[i].name );
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
                if( !hack_attempt( g->u, current.security ) ) {
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

bool computer_session::hack_attempt( player &p, int Security )
{
    if( Security == -1 ) {
        Security = comp.security;    // Set to main system security if no value passed
    }
    const int hack_skill = p.get_skill_level( skill_computer );

    // Every time you dig for lab notes, (or, in future, do other suspicious stuff?)
    // +2 dice to the system's hack-resistance
    // So practical max files from a given terminal = 5, at 10 Computer
    if( comp.alerts > 0 ) {
        Security += ( comp.alerts * 2 );
    }

    p.moves -= 10 * ( 5 + Security * 2 ) / std::max( 1, hack_skill + 1 );
    int player_roll = hack_skill;
    ///\EFFECT_INT <8 randomly penalizes hack attempts, 50% of the time
    if( p.int_cur < 8 && one_in( 2 ) ) {
        player_roll -= rng( 0, 8 - p.int_cur );
        ///\EFFECT_INT >8 randomly benefits hack attempts, 33% of the time
    } else if( p.int_cur > 8 && one_in( 3 ) ) {
        player_roll += rng( 0, p.int_cur - 8 );
    }

    ///\EFFECT_COMPUTER increases chance of successful hack attempt, vs Security level
    bool successful_attempt = ( dice( player_roll, 6 ) >= dice( Security, 6 ) );
    p.practice( skill_computer, successful_attempt ? ( 15 + Security * 3 ) : 7 );
    return successful_attempt;
}

static item *pick_usb()
{
    auto filter = []( const item & it ) {
        return it.typeId() == itype_usb_drive;
    };

    item_location loc = game_menus::inv::titled_filter_menu( filter, g->u, _( "Choose drive:" ) );
    if( loc ) {
        return &*loc;
    }
    return nullptr;
}

static void remove_submap_turrets()
{
    for( monster &critter : g->all_monsters() ) {
        // Check 1) same overmap coords, 2) turret, 3) hostile
        if( ms_to_omt_copy( g->m.getabs( critter.pos() ) ) == ms_to_omt_copy( g->m.getabs( g->u.pos() ) ) &&
            critter.has_flag( MF_CONSOLE_DESPAWN ) &&
            critter.attitude_to( g->u ) == Creature::Attitude::A_HOSTILE ) {
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
    { COMPACT_LOCK, &computer_session::action_lock },
    { COMPACT_MAP_SEWER, &computer_session::action_map_sewer },
    { COMPACT_MAP_SUBWAY, &computer_session::action_map_subway },
    { COMPACT_MAPS, &computer_session::action_maps },
    { COMPACT_MISS_DISARM, &computer_session::action_miss_disarm },
    { COMPACT_OPEN, &computer_session::action_open },
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

void computer_session::activate_function( computer_action action )
{
    const auto it = computer_action_functions.find( action );
    if( it != computer_action_functions.end() ) {
        // Token move cost for any action, if an action takes longer decrement moves further.
        g->u.moves -= 30;
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
    g->m.translate_radius( t_door_metal_locked, t_floor, 25.0, g->u.pos(), true );
    query_any( _( "Doors opened.  Press any key…" ) );
}

//LOCK AND UNLOCK are used to build more complex buildings
// that can have multiple doors that can be locked and be
// unlocked by different computers.
//Simply uses translate_radius which take a given radius and
// player position to determine which terrain tiles to edit.
void computer_session::action_lock()
{
    g->m.translate_radius( t_door_metal_c, t_door_metal_locked, 8.0, g->u.pos(), true );
    query_any( _( "Lock enabled.  Press any key…" ) );
}

void computer_session::action_unlock_disarm()
{
    remove_submap_turrets();
    action_unlock();
}

void computer_session::action_unlock()
{
    g->m.translate_radius( t_door_metal_locked, t_door_metal_c, 8.0, g->u.pos(), true );
    query_any( _( "Lock disabled.  Press any key…" ) );
}

//Toll is required for the church computer/mechanism to function
void computer_session::action_toll()
{
    sounds::sound( g->u.pos(), 120, sounds::sound_t::music,
                   //~ the sound of a church bell ringing
                   _( "Bohm…  Bohm…  Bohm…" ), true, "environment", "church_bells" );
}

void computer_session::action_sample()
{
    g->u.moves -= 30;
    for( const tripoint &p : g->m.points_on_zlevel() ) {
        if( g->m.ter( p ) != t_sewage_pump ) {
            continue;
        }
        for( const tripoint &n : g->m.points_in_radius( p, 1 ) ) {
            if( g->m.furn( n ) != f_counter ) {
                continue;
            }
            bool found_item = false;
            item sewage( itype_sewage, calendar::turn );
            for( item &elem : g->m.i_at( n ) ) {
                int capa = elem.get_remaining_capacity_for_liquid( sewage );
                if( capa <= 0 ) {
                    continue;
                }
                sewage.charges = std::min( sewage.charges, capa );
                if( elem.can_contain( sewage ) ) {
                    elem.put_in( sewage, item_pocket::pocket_type::CONTAINER );
                }
                found_item = true;
                break;
            }
            if( !found_item ) {
                g->m.add_item_or_charges( n, sewage );
            }
        }
    }
}

void computer_session::action_release()
{
    g->events().send<event_type::releases_subspace_specimens>();
    sounds::sound( g->u.pos(), 40, sounds::sound_t::alarm, _( "an alarm sound!" ), false, "environment",
                   "alarm" );
    g->m.translate_radius( t_reinforced_glass, t_thconc_floor, 25.0, g->u.pos(), true );
    query_any( _( "Containment shields opened.  Press any key…" ) );
}

void computer_session::action_release_disarm()
{
    remove_submap_turrets();
    action_release_bionics();
}

void computer_session::action_release_bionics()
{
    sounds::sound( g->u.pos(), 40, sounds::sound_t::alarm, _( "an alarm sound!" ), false, "environment",
                   "alarm" );
    g->m.translate_radius( t_reinforced_glass, t_thconc_floor, 3.0, g->u.pos(), true );
    query_any( _( "Containment shields opened.  Press any key…" ) );
}

void computer_session::action_terminate()
{
    g->events().send<event_type::terminates_subspace_specimens>();
    for( const tripoint &p : g->m.points_on_zlevel() ) {
        monster *const mon = g->critter_at<monster>( p );
        if( !mon ) {
            continue;
        }
        if( ( g->m.ter( p + tripoint_north ) == t_reinforced_glass &&
              g->m.ter( p + tripoint_south ) == t_concrete_wall ) ||
            ( g->m.ter( p + tripoint_south ) == t_reinforced_glass &&
              g->m.ter( p + tripoint_north ) == t_concrete_wall ) ) {
            mon->die( &g->u );
        }
    }
    query_any( _( "Subjects terminated.  Press any key…" ) );
}

void computer_session::action_portal()
{
    g->events().send<event_type::opens_portal>();
    for( const tripoint &tmp : g->m.points_on_zlevel() ) {
        int numtowers = 0;
        for( const tripoint &tmp2 : g->m.points_in_radius( tmp, 2 ) ) {
            if( g->m.ter( tmp2 ) == t_radio_tower ) {
                numtowers++;
            }
        }
        if( numtowers >= 4 ) {
            if( g->m.tr_at( tmp ).id == trap_str_id( "tr_portal" ) ) {
                g->m.remove_trap( tmp );
            } else {
                g->m.trap_set( tmp, tr_portal );
            }
        }
    }
}

void computer_session::action_cascade()
{
    if( !query_bool( _( "WARNING: Resonance cascade carries severe risk!  Continue?" ) ) ) {
        return;
    }
    g->events().send<event_type::causes_resonance_cascade>();
    std::vector<tripoint> cascade_points;
    for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 10 ) ) {
        if( g->m.ter( dest ) == t_radio_tower ) {
            cascade_points.push_back( dest );
        }
    }
    explosion_handler::resonance_cascade( random_entry( cascade_points, g->u.pos() ) );
}

void computer_session::action_research()
{
    // TODO: seed should probably be a member of the computer, or better: of the computer action.
    // It is here to ensure one computer reporting the same text on each invocation.
    const int seed = g->get_levx() + g->get_levy() + g->get_levz() + comp.alerts;
    cata::optional<translation> log = SNIPPET.random_from_category( "lab_notes", seed );
    if( !log.has_value() ) {
        log = to_translation( "No data found." );
    } else {
        g->u.moves -= 70;
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
    g->u.moves -= 300;
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
    g->u.moves -= 30;
    const tripoint center = g->u.global_omt_location();
    overmap_buffer.reveal( center.xy(), 40, 0 );
    query_any(
        _( "Surface map data downloaded.  Local anomalous-access error logged.  Press any key…" ) );
    comp.remove_option( COMPACT_MAPS );
    comp.alerts ++;
}

void computer_session::action_map_sewer()
{
    g->u.moves -= 30;
    const tripoint center = g->u.global_omt_location();
    for( int i = -60; i <= 60; i++ ) {
        for( int j = -60; j <= 60; j++ ) {
            point offset( i, j );
            const oter_id &oter = overmap_buffer.ter( center + offset );
            if( is_ot_match( "sewer", oter, ot_match_type::type ) ||
                is_ot_match( "sewage", oter, ot_match_type::prefix ) ) {
                overmap_buffer.set_seen( center + offset, true );
            }
        }
    }
    query_any( _( "Sewage map data downloaded.  Press any key…" ) );
    comp.remove_option( COMPACT_MAP_SEWER );
}

void computer_session::action_map_subway()
{
    g->u.moves -= 30;
    const tripoint center = g->u.global_omt_location();
    for( int i = -60; i <= 60; i++ ) {
        for( int j = -60; j <= 60; j++ ) {
            point offset( i, j );
            const oter_id &oter = overmap_buffer.ter( center + offset );
            if( is_ot_match( "subway", oter, ot_match_type::type ) ||
                is_ot_match( "lab_train_depot", oter, ot_match_type::contains ) ) {
                overmap_buffer.set_seen( center + offset, true );
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
        g->events().send<event_type::disarms_nuke>();
        add_msg( m_info, _( "Nuclear missile disarmed!" ) );
        //disable missile.
        comp.options.clear();
        activate_failure( COMPFAIL_SHUTDOWN );
    } else {
        add_msg( m_neutral, _( "Nuclear missile remains active." ) );
        return;
    }
}

void computer_session::action_list_bionics()
{
    g->u.moves -= 30;
    std::vector<std::string> names;
    int more = 0;
    for( const tripoint &p : g->m.points_on_zlevel() ) {
        for( item &elem : g->m.i_at( p ) ) {
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
        print_line( ngettext( "%d OTHER FOUND…", "%d OTHERS FOUND…", more ), more );
    }

    print_newline();
    query_any( _( "Press any key…" ) );
}

void computer_session::action_elevator_on()
{
    for( const tripoint &p : g->m.points_on_zlevel() ) {
        if( g->m.ter( p ) == t_elevator_control_off ) {
            g->m.ter_set( p, t_elevator_control );
        }
    }
    query_any( _( "Elevator activated.  Press any key…" ) );
}

void computer_session::action_amigara_log()
{
    g->u.moves -= 30;
    reset_terminal();
    print_line( _( "NEPower Mine(%d:%d) Log" ), g->get_levx(), g->get_levy() );
    print_text( "%s", SNIPPET.random_from_category( "amigara1" ).value_or( translation() ) );

    if( !query_bool( _( "Continue reading?" ) ) ) {
        return;
    }
    g->u.moves -= 30;
    reset_terminal();
    print_line( _( "NEPower Mine(%d:%d) Log" ), g->get_levx(), g->get_levy() );
    print_text( "%s", SNIPPET.random_from_category( "amigara2" ).value_or( translation() ) );

    if( !query_bool( _( "Continue reading?" ) ) ) {
        return;
    }
    g->u.moves -= 30;
    reset_terminal();
    print_line( _( "NEPower Mine(%d:%d) Log" ), g->get_levx(), g->get_levy() );
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
    g->u.moves -= 30;
    reset_terminal();
    print_line( _( "SITE %d%d%d\n"
                   "PERTINENT FOREMAN LOGS WILL BE PREPENDED TO NOTES" ),
                g->get_levx(), g->get_levy(), std::abs( g->get_levz() ) );
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
    g->timed_events.add( TIMED_EVENT_AMIGARA, calendar::turn + 1_minutes );
    if( !g->u.has_artifact_with( AEP_PSYSHIELD ) ) {
        g->u.add_effect( effect_amigara, 2_minutes );
    }
    // Disable this action to prevent further amigara events, which would lead to
    // further amigara monster, which would lead to further artifacts.
    comp.remove_option( COMPACT_AMIGARA_START );
}

void computer_session::action_complete_disable_external_power()
{
    for( auto miss : g->u.get_active_missions() ) {
        static const mission_type_id commo_2 = mission_type_id( "MISSION_OLD_GUARD_NEC_COMMO_2" );
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
    if( g->u.has_amount( itype_radio_repeater_mod, 1 ) ) {
        for( auto miss : g->u.get_active_missions() ) {
            static const mission_type_id commo_3 = mission_type_id( "MISSION_OLD_GUARD_NEC_COMMO_3" ),
                                         commo_4 = mission_type_id( "MISSION_OLD_GUARD_NEC_COMMO_4" );
            if( miss->mission_id() == commo_3 || miss->mission_id() == commo_4 ) {
                miss->step_complete( 1 );
                print_error( _( "Repeater mod installed…" ) );
                print_error( _( "Mission Complete!" ) );
                g->u.use_amount( itype_radio_repeater_mod, 1 );
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
    if( item *const usb = pick_usb() ) {
        mission *miss = mission::find( comp.mission_id );
        if( miss == nullptr ) {
            debugmsg( _( "Computer couldn't find its mission!" ) );
            return;
        }
        g->u.moves -= 30;
        item software( miss->get_item_id(), 0 );
        software.mission_id = comp.mission_id;
        usb->contents.clear_items();
        usb->put_in( software, item_pocket::pocket_type::SOFTWARE );
        print_line( _( "Software downloaded." ) );
    } else {
        print_error( _( "USB drive required!" ) );
    }
    query_any();
}

void computer_session::action_blood_anal()
{
    g->u.moves -= 70;
    for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 2 ) ) {
        if( g->m.ter( dest ) == t_centrifuge ) {
            map_stack items = g->m.i_at( dest );
            if( items.empty() ) {
                print_error( _( "ERROR: Please place sample in centrifuge." ) );
            } else if( items.size() > 1 ) {
                print_error( _( "ERROR: Please remove all but one sample from centrifuge." ) );
            } else if( items.only_item().contents.empty() ) {
                print_error( _( "ERROR: Please only use container with blood sample." ) );
            } else if( items.only_item().contents.legacy_front().typeId() != itype_blood ) {
                print_error( _( "ERROR: Please only use blood samples." ) );
            } else { // Success!
                const item &blood = items.only_item().contents.legacy_front();
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
                    if( query_bool( _( "Download data?" ) ) ) {
                        if( item *const usb = pick_usb() ) {
                            item software( "software_blood_data", 0 );
                            usb->contents.clear_items();
                            usb->put_in( software, item_pocket::pocket_type::SOFTWARE );
                            print_line( _( "Software downloaded." ) );
                        } else {
                            print_error( _( "USB drive required!" ) );
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
    g->u.moves -= 30;
    for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 2 ) ) {
        if( g->m.ter( dest ) == t_floor_blue ) {
            print_error( _( "PROCESSING DATA" ) );
            map_stack items = g->m.i_at( dest );
            if( items.empty() ) {
                print_error( _( "ERROR: Please place memory bank in scan area." ) );
            } else if( items.size() > 1 ) {
                print_error( _( "ERROR: Please only scan one item at a time." ) );
            } else if( items.only_item().typeId() != itype_usb_drive &&
                       items.only_item().typeId() != itype_black_box ) {
                print_error( _( "ERROR: Memory bank destroyed or not present." ) );
            } else if( items.only_item().typeId() == itype_usb_drive &&
                       items.only_item().contents.empty() ) {
                print_error( _( "ERROR: Memory bank is empty." ) );
            } else { // Success!
                if( items.only_item().typeId() == itype_black_box ) {
                    print_line( _( "Memory Bank: Military Hexron Encryption\nPrinting Transcript\n" ) );
                    item transcript( "black_box_transcript", calendar::turn );
                    g->m.add_item_or_charges( g->u.pos(), transcript );
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
    g->events().send<event_type::seals_hazardous_material_sarcophagus>();
    print_line( _( "Charges Detonated" ) );
    print_line( _( "Backup Generator Power Failing" ) );
    print_line( _( "Evacuate Immediately" ) );
    add_msg( m_warning, _( "Evacuate Immediately!" ) );
    for( const tripoint &p : g->m.points_on_zlevel() ) {
        if( g->m.ter( p ) == t_elevator || g->m.ter( p ) == t_vat ) {
            g->m.make_rubble( p, f_rubble_rock, true );
            explosion_handler::explosion( p, 40, 0.7, true );
        }
        if( g->m.ter( p ) == t_wall_glass ) {
            g->m.make_rubble( p, f_rubble_rock, true );
        }
        if( g->m.ter( p ) == t_sewage_pipe || g->m.ter( p ) == t_sewage || g->m.ter( p ) == t_grate ) {
            g->m.make_rubble( p, f_rubble_rock, true );
        }
        if( g->m.ter( p ) == t_sewage_pump ) {
            g->m.make_rubble( p, f_rubble_rock, true );
            explosion_handler::explosion( p, 50, 0.7, true );
        }
    }
    comp.options.clear(); // Disable the terminal.
    activate_failure( COMPFAIL_SHUTDOWN );
}

void computer_session::action_srcf_elevator()
{
    if( !g->u.has_amount( itype_sarcophagus_access_code, 1 ) ) {
        print_error( _( "Access code required!" ) );
    } else {
        g->u.use_amount( itype_sarcophagus_access_code, 1 );
        reset_terminal();
        print_line(
            _( "\nPower:         Backup Only\nRadiation Level:  Very Dangerous\nOperational:   Overridden\n\n" ) );
        for( const tripoint &p : g->m.points_on_zlevel() ) {
            if( g->m.ter( p ) == t_elevator_control_off ) {
                g->m.ter_set( p, t_elevator_control );
            }
        }
    }
    query_any( _( "Press any key…" ) );
}

//irradiates food at t_rad_platform, adds radiation
void computer_session::action_irradiator()
{
    g->u.moves -= 30;
    bool error = false;
    bool platform_exists = false;
    for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 10 ) ) {
        if( g->m.ter( dest ) == t_rad_platform ) {
            platform_exists = true;
            if( g->m.i_at( dest ).empty() ) {
                print_error( _( "ERROR: Processing platform empty." ) );
            } else {
                g->u.moves -= 300;
                for( auto it = g->m.i_at( dest ).begin(); it != g->m.i_at( dest ).end(); ++it ) {
                    // actual food processing
                    itype_id irradiated_type( "irradiated_" + it->typeId().str() );
                    if( !it->rotten() && item_controller->has_template( irradiated_type ) ) {
                        it->convert( irradiated_type );
                    }
                    // critical failure - radiation spike sets off electronic detonators
                    if( it->typeId() == itype_mininuke || it->typeId() == itype_mininuke_act ||
                        it->typeId() == itype_c4 ) {
                        explosion_handler::explosion( dest, 40 );
                        reset_terminal();
                        print_error( _( "WARNING [409]: Primary sensors offline!" ) );
                        print_error( _( "  >> Initialize secondary sensors: Geiger profiling…" ) );
                        print_error( _( "  >> Radiation spike detected!\n" ) );
                        print_error( _( "WARNING [912]: Catastrophic malfunction!  Contamination detected!" ) );
                        print_error( _( "EMERGENCY PROCEDURE [1]:  Evacuate.  Evacuate.  Evacuate.\n" ) );
                        sounds::sound( g->u.pos(), 30, sounds::sound_t::alarm, _( "an alarm sound!" ), false, "environment",
                                       "alarm" );
                        g->m.i_rem( dest, it );
                        g->m.make_rubble( dest );
                        g->m.propagate_field( dest, fd_nuke_gas, 100, 3 );
                        g->m.translate_radius( t_water_pool, t_sewage, 8.0, dest, true );
                        g->m.adjust_radiation( dest, rng( 50, 500 ) );
                        for( const tripoint &radorigin : g->m.points_in_radius( dest, 5 ) ) {
                            g->m.adjust_radiation( radorigin, rng( 50, 500 ) / ( rl_dist( radorigin,
                                                   dest ) > 0 ? rl_dist( radorigin, dest ) : 1 ) );
                        }
                        if( g->m.pl_sees( dest, 10 ) ) {
                            g->u.irradiate( rng_float( 50, 250 ) / rl_dist( g->u.pos(), dest ) );
                        } else {
                            g->u.irradiate( rng_float( 20, 100 ) / rl_dist( g->u.pos(), dest ) );
                        }
                        query_any( _( "EMERGENCY SHUTDOWN!  Press any key…" ) );
                        error = true;
                        comp.options.clear(); // Disable the terminal.
                        activate_failure( COMPFAIL_SHUTDOWN );
                        break;
                    }
                    g->m.adjust_radiation( dest, rng( 20, 50 ) );
                    for( const tripoint &radorigin : g->m.points_in_radius( dest, 5 ) ) {
                        g->m.adjust_radiation( radorigin, rng( 20, 50 ) / ( rl_dist( radorigin,
                                               dest ) > 0 ? rl_dist( radorigin, dest ) : 1 ) );
                    }
                    // if unshielded, rad source irradiates player directly, reduced by distance to source
                    if( g->m.pl_sees( dest, 10 ) ) {
                        g->u.irradiate( rng_float( 5, 25 ) / rl_dist( g->u.pos(), dest ) );
                    }
                }
                if( !error && platform_exists ) {
                    print_error( _( "PROCESSING…  CYCLE COMPLETE." ) );
                    print_error( _( "GEIGER COUNTER @ PLATFORM: %s mSv/h." ), g->m.get_radiation( dest ) );
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

// geiger counter for irradiator, primary measurement at t_rad_platform, secondary at player loacation
void computer_session::action_geiger()
{
    g->u.moves -= 30;
    tripoint platform;
    bool source_exists = false;
    int sum_rads = 0;
    int peak_rad = 0;
    int tiles_counted = 0;
    print_error( _( "RADIATION MEASUREMENTS:" ) );
    for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 10 ) ) {
        if( g->m.ter( dest ) == t_rad_platform ) {
            source_exists = true;
            platform = dest;
        }
    }
    if( source_exists ) {
        for( const tripoint &dest : g->m.points_in_radius( platform, 3 ) ) {
            sum_rads += g->m.get_radiation( dest );
            tiles_counted ++;
            if( g->m.get_radiation( dest ) > peak_rad ) {
                peak_rad = g->m.get_radiation( dest );
            }
            sum_rads += g->m.get_radiation( platform );
            tiles_counted ++;
            if( g->m.get_radiation( platform ) > peak_rad ) {
                peak_rad = g->m.get_radiation( platform );
            }
        }
        print_error( _( "GEIGER COUNTER @ ZONE:… AVG %s mSv/h." ), sum_rads / tiles_counted );
        print_error( _( "GEIGER COUNTER @ ZONE:… MAX %s mSv/h." ), peak_rad );
        print_newline();
    }
    print_error( _( "GEIGER COUNTER @ CONSOLE:… %s mSv/h." ), g->m.get_radiation( g->u.pos() ) );
    print_error( _( "PERSONAL DOSIMETRY:… %s mSv." ), g->u.get_rad() );
    print_newline();
    query_any( _( "Press any key…" ) );
}

// imitates item movement through conveyor belt through 3 different loading/unloading bays
// ensure only bay of each type in range
void computer_session::action_conveyor()
{
    g->u.moves -= 300;
    tripoint loading; // red tile = loading bay
    tripoint unloading; // green tile = unloading bay
    tripoint platform; // radiation platform = middle point
    bool l_exists = false;
    bool u_exists = false;
    bool p_exists = false;
    for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 10 ) ) {
        if( g->m.ter( dest ) == t_rad_platform ) {
            platform = dest;
            p_exists = true;
        } else if( g->m.ter( dest ) == t_floor_red ) {
            loading = dest;
            l_exists = true;
        } else if( g->m.ter( dest ) == t_floor_green ) {
            unloading = dest;
            u_exists = true;
        }
    }
    if( !l_exists || !p_exists || !u_exists ) {
        print_error( _( "Conveyor belt malfunction.  Consult maintenance team." ) );
        query_any( _( "Press any key…" ) );
        return;
    }
    auto items = g->m.i_at( platform );
    if( !items.empty() ) {
        print_line( _( "Moving items: PLATFORM --> UNLOADING BAY." ) );
    } else {
        print_line( _( "No items detected at: PLATFORM." ) );
    }
    for( const auto &it : items ) {
        g->m.add_item_or_charges( unloading, it );
    }
    g->m.i_clear( platform );
    items = g->m.i_at( loading );
    if( !items.empty() ) {
        print_line( _( "Moving items: LOADING BAY --> PLATFORM." ) );
    } else {
        print_line( _( "No items detected at: LOADING BAY." ) );
    }
    for( const auto &it : items ) {
        if( !it.made_of_from_type( phase_id::LIQUID ) ) {
            g->m.add_item_or_charges( platform, it );
        }
    }
    g->m.i_clear( loading );
    query_any( _( "Conveyor belt cycle complete.  Press any key…" ) );
}

// toggles reinforced glass shutters open->closed and closed->open depending on their current state
void computer_session::action_shutters()
{
    g->u.moves -= 300;
    g->m.translate_radius( t_reinforced_glass_shutter_open, t_reinforced_glass_shutter, 8.0, g->u.pos(),
                           true, true );
    query_any( _( "Toggling shutters.  Press any key…" ) );
}

// extract radiation source material from irradiator
void computer_session::action_extract_rad_source()
{
    if( query_yn( _( "Operation irreversible.  Extract radioactive material?" ) ) ) {
        g->u.moves -= 300;
        tripoint platform;
        bool p_exists = false;
        for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 10 ) ) {
            if( g->m.ter( dest ) == t_rad_platform ) {
                platform = dest;
                p_exists = true;
            }
        }
        if( p_exists ) {
            g->m.spawn_item( platform, itype_cobalt_60, rng( 8, 15 ) );
            g->m.translate_radius( t_rad_platform, t_concrete, 8.0, g->u.pos(), true );
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
    g->u.moves -= 30;
    bool has_vent = false;
    bool has_generator = false;
    for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 10 ) ) {
        if( g->m.get_field( dest, fd_shock_vent ) != nullptr ) {
            has_vent = true;
        }
        if( g->m.ter( dest ) == t_plut_generator ) {
            has_generator = true;
        }
        g->m.remove_field( dest, fd_shock_vent );
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
    if( has_generator ) {
        print_line( _( "Backup power status: STANDBY MODE." ) );
    } else {
        print_error( _( "Backup power status: OFFLINE.  Reason: UNKNOWN" ) );
    }
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
    for( const tripoint &p : g->m.points_in_radius( g->u.pos(), 1 ) ) {
        if( g->m.has_flag( flag_CONSOLE, p ) ) {
            g->m.ter_set( p, t_console_broken );
            add_msg( m_bad, _( "The console shuts down." ) );
            found_tile = true;
        }
    }
    if( found_tile ) {
        return;
    }
    for( const tripoint &p : g->m.points_on_zlevel() ) {
        if( g->m.has_flag( flag_CONSOLE, p ) ) {
            g->m.ter_set( p, t_console_broken );
            add_msg( m_bad, _( "The console shuts down." ) );
        }
    }
}

void computer_session::failure_alarm()
{
    g->events().send<event_type::triggers_alarm>( g->u.getID() );
    sounds::sound( g->u.pos(), 60, sounds::sound_t::alarm, _( "an alarm sound!" ), false, "environment",
                   "alarm" );
    if( g->get_levz() > 0 && !g->timed_events.queued( TIMED_EVENT_WANTED ) ) {
        g->timed_events.add( TIMED_EVENT_WANTED, calendar::turn + 30_minutes, 0,
                             g->u.global_sm_location() );
    }
}

void computer_session::failure_manhacks()
{
    int num_robots = rng( 4, 8 );
    const tripoint_range range = g->m.points_in_radius( g->u.pos(), 3 );
    for( int i = 0; i < num_robots; i++ ) {
        if( g->place_critter_within( mon_manhack, range ) ) {
            add_msg( m_warning, _( "Manhacks drop from compartments in the ceiling." ) );
        }
    }
}

void computer_session::failure_secubots()
{
    int num_robots = 1;
    const tripoint_range range = g->m.points_in_radius( g->u.pos(), 3 );
    for( int i = 0; i < num_robots; i++ ) {
        if( g->place_critter_within( mon_secubot, range ) ) {
            add_msg( m_warning, _( "Secubots emerge from compartments in the floor." ) );
        }
    }
}

void computer_session::failure_damage()
{
    add_msg( m_neutral, _( "The console shocks you." ) );
    if( g->u.is_elec_immune() ) {
        add_msg( m_good, _( "You're protected from electric shocks." ) );
    } else {
        add_msg( m_bad, _( "Your body is damaged by the electric shock!" ) );
        g->u.hurtall( rng( 1, 10 ), nullptr );
    }
}

void computer_session::failure_pump_explode()
{
    add_msg( m_warning, _( "The pump explodes!" ) );
    for( const tripoint &p : g->m.points_on_zlevel() ) {
        if( g->m.ter( p ) == t_sewage_pump ) {
            g->m.make_rubble( p );
            explosion_handler::explosion( p, 10 );
        }
    }
}

void computer_session::failure_pump_leak()
{
    add_msg( m_warning, _( "Sewage leaks!" ) );
    for( const tripoint &p : g->m.points_on_zlevel() ) {
        if( g->m.ter( p ) != t_sewage_pump ) {
            continue;
        }
        const int leak_size = rng( 4, 10 );
        for( int i = 0; i < leak_size; i++ ) {
            std::vector<tripoint> next_move;
            if( g->m.passable( p + point_north ) ) {
                next_move.push_back( p + point_north );
            }
            if( g->m.passable( p + point_east ) ) {
                next_move.push_back( p + point_east );
            }
            if( g->m.passable( p + point_south ) ) {
                next_move.push_back( p + point_south );
            }
            if( g->m.passable( p + point_west ) ) {
                next_move.push_back( p + point_west );
            }
            if( next_move.empty() ) {
                break;
            }
            g->m.ter_set( random_entry( next_move ), t_sewage );
        }
    }
}

void computer_session::failure_amigara()
{
    g->timed_events.add( TIMED_EVENT_AMIGARA, calendar::turn + 30_seconds );
    g->u.add_effect( effect_amigara, 2_minutes );
    explosion_handler::explosion( tripoint( rng( 0, MAPSIZE_X ), rng( 0, MAPSIZE_Y ), g->get_levz() ),
                                  10,
                                  0.7, false, 10 );
    explosion_handler::explosion( tripoint( rng( 0, MAPSIZE_X ), rng( 0, MAPSIZE_Y ), g->get_levz() ),
                                  10,
                                  0.7, false, 10 );
    comp.remove_option( COMPACT_AMIGARA_START );
}

void computer_session::failure_destroy_blood()
{
    print_error( _( "ERROR: Disruptive Spin" ) );
    for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 2 ) ) {
        if( g->m.ter( dest ) == t_centrifuge ) {
            map_stack items = g->m.i_at( dest );
            if( items.empty() ) {
                print_error( _( "ERROR: Please place sample in centrifuge." ) );
            } else if( items.size() > 1 ) {
                print_error( _( "ERROR: Please remove all but one sample from centrifuge." ) );
            } else if( items.only_item().typeId() != itype_vacutainer ) {
                print_error( _( "ERROR: Please use blood-contained samples." ) );
            } else if( items.only_item().contents.empty() ) {
                print_error( _( "ERROR: Blood draw kit, empty." ) );
            } else if( items.only_item().contents.legacy_front().typeId() != itype_blood ) {
                print_error( _( "ERROR: Please only use blood samples." ) );
            } else {
                print_error( _( "ERROR: Blood sample destroyed." ) );
                g->m.i_clear( dest );
            }
        }
    }
    query_any();
}

void computer_session::failure_destroy_data()
{
    print_error( _( "ERROR: ACCESSING DATA MALFUNCTION" ) );
    for( const tripoint &p : g->m.points_in_radius( g->u.pos(), 2 ) ) {
        if( g->m.ter( p ) == t_floor_blue ) {
            map_stack items = g->m.i_at( p );
            if( items.empty() ) {
                print_error( _( "ERROR: Please place memory bank in scan area." ) );
            } else if( items.size() > 1 ) {
                print_error( _( "ERROR: Please only scan one item at a time." ) );
            } else if( items.only_item().typeId() != itype_usb_drive ) {
                print_error( _( "ERROR: Memory bank destroyed or not present." ) );
            } else if( items.only_item().contents.empty() ) {
                print_error( _( "ERROR: Memory bank is empty." ) );
            } else {
                print_error( _( "ERROR: Data bank destroyed." ) );
                g->m.i_clear( p );
            }
        }
    }
    query_any();
}

void computer_session::action_emerg_ref_center()
{
    reset_terminal();
    print_line( _( "SEARCHING FOR NEAREST REFUGEE CENTER, PLEASE WAIT…" ) );

    const mission_type_id &mission_type = mission_type_id( "MISSION_REACH_REFUGEE_CENTER" );
    tripoint mission_target;
    // Check completed missions too, so people can't repeatedly get the mission.
    const std::vector<mission *> completed_missions = g->u.get_completed_missions();
    std::vector<mission *> missions = g->u.get_active_missions();
    missions.insert( missions.end(), completed_missions.begin(), completed_missions.end() );

    const bool has_mission = std::any_of( missions.begin(), missions.end(), [ &mission_type,
    &mission_target ]( mission * mission ) {
        if( mission->get_type().id == mission_type ) {
            mission_target = mission->get_target();
            return true;
        }

        return false;
    } );

    if( !has_mission ) {
        const auto mission = mission::reserve_new( mission_type, character_id() );
        mission->assign( g->u );
        mission_target = mission->get_target();
    }

    //~555-0164 is a fake phone number in the US, please replace it with a number that will not cause issues in your locale if possible.
    print_line( _( "\nREFUGEE CENTER FOUND!  LOCATION: %d %s\n\n"
                   "IF YOU HAVE ANY FEEDBACK CONCERNING YOUR VISIT PLEASE CONTACT\n"
                   "THE DEPARTMENT OF EMERGENCY MANAGEMENT PUBLIC AFFAIRS OFFICE.\n"
                   "THE LOCAL OFFICE CAN BE REACHED BETWEEN THE HOURS OF 9AM AND\n"
                   "4PM AT 555-0164.\n"
                   "\n"
                   "IF YOU WOULD LIKE TO SPEAK WITH SOMEONE IN PERSON OR WOULD LIKE\n"
                   "TO WRITE US A LETTER PLEASE SEND IT TO…\n" ), rl_dist( g->u.pos(), mission_target ),
                direction_name_short( direction_from( g->u.pos(), mission_target ) ) );

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
    const auto &allow_key = force_uc ? input_context::disallow_lower_case
                            : input_context::allow_all_keys;
    input_context ctxt( "YESNOQUIT" );
    ctxt.register_action( "YES" );
    ctxt.register_action( "NO" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    print_indented_line( 0, width, force_uc
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
        if( action == "HELP_KEYBINDINGS" ) {
            refresh();
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
    wrefresh( win );
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
