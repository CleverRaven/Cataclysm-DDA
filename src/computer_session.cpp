#include "computer_session.h"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <string>
#include <utility>

#include "avatar.h"
#include "basecamp.h"
#include "calendar.h"
#include "colony.h"
#include "color.h"
#include "coordinate_conversions.h"
#include "creature.h"
#include "debug.h"
#include "enums.h"
#include "event_bus.h"
#include "explosion.h"
#include "field.h"
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "input.h"
#include "int_id.h"
#include "item.h"
#include "item_factory.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "mission.h"
#include "monster.h"
#include "mtype.h"
#include "omdata.h"
#include "output.h"
#include "overmap.h"
#include "overmap_ui.h"
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

static const mtype_id mon_manhack( "mon_manhack" );
static const mtype_id mon_secubot( "mon_secubot" );
static const mtype_id mon_turret_rifle( "mon_turret_rifle" );
static const mtype_id mon_turret_bmg( "mon_turret_bmg" );
static const mtype_id mon_crows_m240( "mon_crows_m240" );

static const skill_id skill_computer( "computer" );

static const species_id ZOMBIE( "ZOMBIE" );
static const species_id HUMAN( "HUMAN" );

static const efftype_id effect_amigara( "amigara" );

static int alerts = 0;

computer_session::computer_session( computer &comp ) : comp( comp ),
    w_terminal( catacurses::window() ), w_border( catacurses::window() )
{
}

void computer_session::shutdown_terminal()
{
    // So yeah, you can reset the term by logging off.
    // Otherwise, it's persistent across all terms.
    // Decided to go easy on people for now.
    alerts = 0;
    werase( w_terminal );
    w_terminal = catacurses::window();
    werase( w_border );
    w_border = catacurses::window();
}

void computer_session::use()
{
    if( !w_border ) {
        w_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                       point( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                                              TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ) );
    }
    if( !w_terminal ) {
        w_terminal = catacurses::newwin( getmaxy( w_border ) - 2, getmaxx( w_border ) - 2,
                                         point( getbegx( w_border ) + 1, getbegy( w_border ) + 1 ) );
    }
    draw_border( w_border );
    wrefresh( w_border );

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
            case 'q':
            case 'Q':
                shutdown_terminal();
                return;

            case 'n':
            case 'N':
                query_any( _( "Shutting down… press any key." ) );
                shutdown_terminal();
                return;

            case 'y':
            case 'Y':
                if( !hack_attempt( g->u ) ) {
                    if( comp.failures.empty() ) {
                        query_any( _( "Maximum login attempts exceeded.  Press any key…" ) );
                        shutdown_terminal();
                        return;
                    }
                    activate_random_failure();
                    shutdown_terminal();
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

        computer_menu.query();
        if( computer_menu.ret < 0 || static_cast<size_t>( computer_menu.ret ) >= comp.options.size() ) {
            break;
        }

        sel = computer_menu.ret;
        computer_option current = comp.options[sel];
        reset_terminal();
        // Once you trip the security, you have to roll every time you want to do something
        if( current.security + alerts > 0 ) {
            print_error( _( "Password required." ) );
            if( query_bool( _( "Hack into system?" ) ) ) {
                if( !hack_attempt( g->u, current.security ) ) {
                    activate_random_failure();
                    shutdown_terminal();
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

    shutdown_terminal(); // This should have been done by now, but just in case.
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
    if( alerts > 0 ) {
        Security += ( alerts * 2 );
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
        return it.typeId() == "usb_drive";
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
            ( critter.type->id == mon_turret_rifle || critter.type->id == mon_turret_bmg ||
              critter.type->id == mon_crows_m240 ) &&
            critter.attitude_to( g->u ) == Creature::Attitude::A_HOSTILE ) {
            g->remove_zombie( critter );
        }
    }
}

void computer_session::activate_function( computer_action action )
{
    // Token move cost for any action, if an action takes longer decrement moves further.
    g->u.moves -= 30;
    switch( action ) {

        case COMPACT_NULL:
        // Unknown action.
        case NUM_COMPUTER_ACTIONS:
            // Suppress compiler warning [-Wswitch]
            break;

        // OPEN_DISARM falls through to just OPEN
        case COMPACT_OPEN_DISARM:
            remove_submap_turrets();
        /* fallthrough */
        case COMPACT_OPEN:
            g->m.translate_radius( t_door_metal_locked, t_floor, 25.0, g->u.pos(), true );
            query_any( _( "Doors opened.  Press any key…" ) );
            break;

        //LOCK AND UNLOCK are used to build more complex buildings
        // that can have multiple doors that can be locked and be
        // unlocked by different computers.
        //Simply uses translate_radius which take a given radius and
        // player position to determine which terrain tiles to edit.
        case COMPACT_LOCK:
            g->m.translate_radius( t_door_metal_c, t_door_metal_locked, 8.0, g->u.pos(), true );
            query_any( _( "Lock enabled.  Press any key…" ) );
            break;

        // UNLOCK_DISARM falls through to just UNLOCK
        case COMPACT_UNLOCK_DISARM:
            remove_submap_turrets();
        /* fallthrough */
        case COMPACT_UNLOCK:
            g->m.translate_radius( t_door_metal_locked, t_door_metal_c, 8.0, g->u.pos(), true );
            query_any( _( "Lock disabled.  Press any key…" ) );
            break;

        //Toll is required for the church computer/mechanism to function
        case COMPACT_TOLL:
            sounds::sound( g->u.pos(), 120, sounds::sound_t::music,
                           //~ the sound of a church bell ringing
                           _( "Bohm…  Bohm…  Bohm…" ), true, "environment", "church_bells" );
            break;

        case COMPACT_SAMPLE:
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
                    item sewage( "sewage", calendar::turn );
                    for( item &elem : g->m.i_at( n ) ) {
                        int capa = elem.get_remaining_capacity_for_liquid( sewage );
                        if( capa <= 0 ) {
                            continue;
                        }
                        capa = std::min( sewage.charges, capa );
                        if( elem.contents.empty() ) {
                            elem.put_in( sewage );
                            elem.contents.front().charges = capa;
                        } else {
                            elem.contents.front().charges += capa;
                        }
                        found_item = true;
                        break;
                    }
                    if( !found_item ) {
                        g->m.add_item_or_charges( n, sewage );
                    }
                }
            }
            break;

        case COMPACT_RELEASE:
            g->events().send<event_type::releases_subspace_specimens>();
            sounds::sound( g->u.pos(), 40, sounds::sound_t::alarm, _( "an alarm sound!" ), false, "environment",
                           "alarm" );
            g->m.translate_radius( t_reinforced_glass, t_thconc_floor, 25.0, g->u.pos(), true );
            query_any( _( "Containment shields opened.  Press any key…" ) );
            break;

        // COMPACT_RELEASE_DISARM falls through to just COMPACT_RELEASE_BIONICS
        case COMPACT_RELEASE_DISARM:
            remove_submap_turrets();
        /* fallthrough */
        case COMPACT_RELEASE_BIONICS:
            sounds::sound( g->u.pos(), 40, sounds::sound_t::alarm, _( "an alarm sound!" ), false, "environment",
                           "alarm" );
            g->m.translate_radius( t_reinforced_glass, t_thconc_floor, 3.0, g->u.pos(), true );
            query_any( _( "Containment shields opened.  Press any key…" ) );
            break;

        case COMPACT_TERMINATE:
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
            break;

        case COMPACT_PORTAL: {
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
        break;

        case COMPACT_CASCADE: {
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
        break;

        case COMPACT_RESEARCH: {
            // TODO: seed should probably be a member of the computer, or better: of the computer action.
            // It is here to ensure one computer reporting the same text on each invocation.
            const int seed = g->get_levx() + g->get_levy() + g->get_levz() + alerts;
            cata::optional<translation> log = SNIPPET.random_from_category( "lab_notes", seed );
            if( !log.has_value() ) {
                log = to_translation( "No data found." );
            } else {
                g->u.moves -= 70;
            }

            print_text( "%s", log.value() );
            // One's an anomaly
            if( alerts == 0 ) {
                query_any( _( "Local data-access error logged, alerting helpdesk.  Press any key…" ) );
                alerts ++;
            } else {
                // Two's a trend.
                query_any(
                    _( "Warning: anomalous archive-access activity detected at this node.  Press any key…" ) );
                alerts ++;
            }
        }
        break;

        case COMPACT_RADIO_ARCHIVE: {
            g->u.moves -= 300;
            sfx::fade_audio_channel( sfx::channel::radio, 100 );
            sfx::play_ambient_variant_sound( "radio", "inaudible_chatter", 100, sfx::channel::radio,
                                             2000 );
            print_text( "Accessing archive.  Playing audio recording nr %d.\n%s", rng( 1, 9999 ),
                        SNIPPET.random_from_category( "radio_archive" ).value_or( translation() ) );
            if( one_in( 3 ) ) {
                query_any( _( "Warning: resticted data access.  Attempt logged.  Press any key…" ) );
                alerts ++;
            } else {
                query_any( _( "Press any key…" ) );
            }
            sfx::fade_audio_channel( sfx::channel::radio, 100 );
        }
        break;

        case COMPACT_MAPS: {
            g->u.moves -= 30;
            const tripoint center = g->u.global_omt_location();
            overmap_buffer.reveal( center.xy(), 40, 0 );
            query_any(
                _( "Surface map data downloaded.  Local anomalous-access error logged.  Press any key…" ) );
            comp.remove_option( COMPACT_MAPS );
            alerts ++;
        }
        break;

        case COMPACT_MAP_SEWER: {
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
        break;

        case COMPACT_MAP_SUBWAY: {
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
        break;

        case COMPACT_OBSOLETE:
            break;
        case COMPACT_MISS_DISARM:
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
            break;

        case COMPACT_LIST_BIONICS: {
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
        break;

        case COMPACT_ELEVATOR_ON:
            for( const tripoint &p : g->m.points_on_zlevel() ) {
                if( g->m.ter( p ) == t_elevator_control_off ) {
                    g->m.ter_set( p, t_elevator_control );
                }
            }
            query_any( _( "Elevator activated.  Press any key…" ) );
            break;

        case COMPACT_AMIGARA_LOG: {
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
                        g->get_levx(), g->get_levy(), abs( g->get_levz() ) );
            print_text( "%s", SNIPPET.random_from_category( "amigara4" ).value_or( translation() ) );
            print_gibberish_line();
            print_gibberish_line();
            print_newline();
            print_error( _( "FILE CORRUPTED, PRESS ANY KEY…" ) );
            inp_mngr.wait_for_any_key();
            reset_terminal();
            break;
        }

        case COMPACT_AMIGARA_START:
            g->timed_events.add( TIMED_EVENT_AMIGARA, calendar::turn + 1_minutes );
            if( !g->u.has_artifact_with( AEP_PSYSHIELD ) ) {
                g->u.add_effect( effect_amigara, 2_minutes );
            }
            // Disable this action to prevent further amigara events, which would lead to
            // further amigara monster, which would lead to further artifacts.
            comp.remove_option( COMPACT_AMIGARA_START );
            break;

        case COMPACT_COMPLETE_DISABLE_EXTERNAL_POWER:
            for( auto miss : g->u.get_active_missions() ) {
                static const mission_type_id commo_2 = mission_type_id( "MISSION_OLD_GUARD_NEC_COMMO_2" );
                if( miss->mission_id() == commo_2 ) {
                    print_error( _( "--ACCESS GRANTED--" ) );
                    print_error( _( "Mission Complete!" ) );
                    miss->step_complete( 1 );
                    inp_mngr.wait_for_any_key();
                    return;
                    //break;
                }
            }
            print_error( _( "ACCESS DENIED" ) );
            inp_mngr.wait_for_any_key();
            break;

        case COMPACT_REPEATER_MOD:
            if( g->u.has_amount( "radio_repeater_mod", 1 ) ) {
                for( auto miss : g->u.get_active_missions() ) {
                    static const mission_type_id commo_3 = mission_type_id( "MISSION_OLD_GUARD_NEC_COMMO_3" ),
                                                 commo_4 = mission_type_id( "MISSION_OLD_GUARD_NEC_COMMO_4" );
                    if( miss->mission_id() == commo_3 || miss->mission_id() == commo_4 ) {
                        miss->step_complete( 1 );
                        print_error( _( "Repeater mod installed…" ) );
                        print_error( _( "Mission Complete!" ) );
                        g->u.use_amount( "radio_repeater_mod", 1 );
                        inp_mngr.wait_for_any_key();
                        comp.options.clear();
                        activate_failure( COMPFAIL_SHUTDOWN );
                        break;
                    }
                }
            } else {
                print_error( _( "You do not have a repeater mod to install…" ) );
                inp_mngr.wait_for_any_key();
                break;
            }
            break;

        case COMPACT_DOWNLOAD_SOFTWARE:
            if( item *const usb = pick_usb() ) {
                mission *miss = mission::find( comp.mission_id );
                if( miss == nullptr ) {
                    debugmsg( _( "Computer couldn't find its mission!" ) );
                    return;
                }
                g->u.moves -= 30;
                item software( miss->get_item_id(), 0 );
                software.mission_id = comp.mission_id;
                usb->contents.clear();
                usb->put_in( software );
                print_line( _( "Software downloaded." ) );
            } else {
                print_error( _( "USB drive required!" ) );
            }
            inp_mngr.wait_for_any_key();
            break;

        case COMPACT_BLOOD_ANAL:
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
                    } else if( items.only_item().contents.front().typeId() != "blood" ) {
                        print_error( _( "ERROR: Please only use blood samples." ) );
                    } else { // Success!
                        const item &blood = items.only_item().contents.front();
                        const mtype *mt = blood.get_mtype();
                        if( mt == nullptr || mt->id == mtype_id::NULL_ID() ) {
                            print_line( _( "Result: Human blood, no pathogens found." ) );
                        } else if( mt->in_species( ZOMBIE ) ) {
                            if( mt->in_species( HUMAN ) ) {
                                print_line( _( "Result: Human blood.  Unknown pathogen found." ) );
                            } else {
                                print_line( _( "Result: Unknown blood type.  Unknown pathogen found." ) );
                            }
                            print_line( _( "Pathogen bonded to erythrocytes and leukocytes." ) );
                            if( query_bool( _( "Download data?" ) ) ) {
                                if( item *const usb = pick_usb() ) {
                                    item software( "software_blood_data", 0 );
                                    usb->contents.clear();
                                    usb->put_in( software );
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
            break;

        case COMPACT_DATA_ANAL:
            g->u.moves -= 30;
            for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 2 ) ) {
                if( g->m.ter( dest ) == t_floor_blue ) {
                    print_error( _( "PROCESSING DATA" ) );
                    map_stack items = g->m.i_at( dest );
                    if( items.empty() ) {
                        print_error( _( "ERROR: Please place memory bank in scan area." ) );
                    } else if( items.size() > 1 ) {
                        print_error( _( "ERROR: Please only scan one item at a time." ) );
                    } else if( items.only_item().typeId() != "usb_drive" &&
                               items.only_item().typeId() != "black_box" ) {
                        print_error( _( "ERROR: Memory bank destroyed or not present." ) );
                    } else if( items.only_item().typeId() == "usb_drive" && items.only_item().contents.empty() ) {
                        print_error( _( "ERROR: Memory bank is empty." ) );
                    } else { // Success!
                        if( items.only_item().typeId() == "black_box" ) {
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
            break;

        case COMPACT_DISCONNECT:
            reset_terminal();
            print_line( _( "\n"
                           "ERROR: NETWORK DISCONNECT\n"
                           "UNABLE TO REACH NETWORK ROUTER OR PROXY.  PLEASE CONTACT YOUR\n"
                           "SYSTEM ADMINISTRATOR TO RESOLVE THIS ISSUE.\n"
                           "  \n" ) );
            query_any( _( "Press any key to continue…" ) );
            break;

        case COMPACT_EMERG_MESS:
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
            break;

        case COMPACT_EMERG_REF_CENTER:
            reset_terminal();
            mark_refugee_center();
            reset_terminal();
            break;

        case COMPACT_TOWER_UNRESPONSIVE:
            print_line( _( "  WARNING, RADIO TOWER IS UNRESPONSIVE.\n"
                           "  \n"
                           "  BACKUP POWER INSUFFICIENT TO MEET BROADCASTING REQUIREMENTS.\n"
                           "  IN THE EVENT OF AN EMERGENCY, CONTACT LOCAL NATIONAL GUARD\n"
                           "  UNITS TO RECEIVE PRIORITY WHEN GENERATORS ARE BEING DEPLOYED.\n"
                           "  \n"
                           "  \n" ) );
            query_any( _( "Press any key to continue…" ) );
            break;

        case COMPACT_SR1_MESS:
            reset_terminal();
            print_text( "%s", SNIPPET.random_from_category( "sr1_mess" ).value_or( translation() ) );
            query_any( _( "Press any key to continue…" ) );
            break;

        case COMPACT_SR2_MESS:
            reset_terminal();
            print_text( "%s", SNIPPET.random_from_category( "sr2_mess" ).value_or( translation() ) );
            query_any( _( "Press any key to continue…" ) );
            break;

        case COMPACT_SR3_MESS:
            reset_terminal();
            print_text( "%s", SNIPPET.random_from_category( "sr3_mess" ).value_or( translation() ) );
            query_any( _( "Press any key to continue…" ) );
            break;

        case COMPACT_SR4_MESS:
            reset_terminal();
            print_text( "%s", SNIPPET.random_from_category( "sr4_mess" ).value_or( translation() ) );
            query_any( _( "Press any key to continue…" ) );
            break;

        case COMPACT_SRCF_1_MESS:
            reset_terminal();
            print_text( "%s", SNIPPET.random_from_category( "scrf_1_mess" ).value_or( translation() ) );
            query_any( _( "Press any key to continue…" ) );
            break;

        case COMPACT_SRCF_2_MESS:
            reset_terminal();
            print_text( "%s", SNIPPET.random_from_category( "scrf_2_1_mess" ).value_or( translation() ) );
            query_any( _( "Press any key to continue…" ) );
            reset_terminal();
            print_text( "%s", SNIPPET.random_from_category( "scrf_2_2_mess" ).value_or( translation() ) );
            query_any( _( "Press any key to continue…" ) );
            break;

        case COMPACT_SRCF_3_MESS:
            reset_terminal();
            print_text( "%s", SNIPPET.random_from_category( "scrf_3_mess" ).value_or( translation() ) );
            query_any( _( "Press any key to continue…" ) );
            break;

        case COMPACT_SRCF_SEAL_ORDER:
            reset_terminal();
            print_text( "%s", SNIPPET.random_from_category( "scrf_seal_order" ).value_or( translation() ) );
            query_any( _( "Press any key to continue…" ) );
            break;

        case COMPACT_SRCF_SEAL:
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
            break;

        case COMPACT_SRCF_ELEVATOR:
            if( !g->u.has_amount( "sarcophagus_access_code", 1 ) ) {
                print_error( _( "Access code required!" ) );
            } else {
                g->u.use_amount( "sarcophagus_access_code", 1 );
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
            break;

        //irradiates food at t_rad_platform, adds radiation
        case COMPACT_IRRADIATOR: {
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
                            if( !it->rotten() && item_controller->has_template( "irradiated_" + it->typeId() ) ) {
                                it->convert( "irradiated_" + it->typeId() );
                            }
                            // critical failure - radiation spike sets off electronic detonators
                            if( it->typeId() == "mininuke" || it->typeId() == "mininuke_act" || it->typeId() == "c4" ) {
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
            break;
        }

        // geiger counter for irradiator, primary measurement at t_rad_platform, secondary at player loacation
        case COMPACT_GEIGER: {
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
            break;
        }

        // imitates item movement through conveyor belt through 3 different loading/unloading bays
        // ensure only bay of each type in range
        case COMPACT_CONVEYOR: {
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
                break;
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
                if( !it.made_of_from_type( LIQUID ) ) {
                    g->m.add_item_or_charges( platform, it );
                }
            }
            g->m.i_clear( loading );
            query_any( _( "Conveyor belt cycle complete.  Press any key…" ) );
            break;
        }
        // toggles reinforced glass shutters open->closed and closed->open depending on their current state
        case COMPACT_SHUTTERS:
            g->u.moves -= 300;
            g->m.translate_radius( t_reinforced_glass_shutter_open, t_reinforced_glass_shutter, 8.0, g->u.pos(),
                                   true, true );
            query_any( _( "Toggling shutters.  Press any key…" ) );
            break;
        // extract radiation source material from irradiator
        case COMPACT_EXTRACT_RAD_SOURCE:
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
                    g->m.spawn_item( platform, "cobalt_60", rng( 8, 15 ) );
                    g->m.translate_radius( t_rad_platform, t_concrete, 8.0, g->u.pos(), true );
                    comp.remove_option( COMPACT_IRRADIATOR );
                    comp.remove_option( COMPACT_EXTRACT_RAD_SOURCE );
                    query_any( _( "Extraction sequence complete…  Press any key." ) );
                } else {
                    query_any( _( "ERROR!  Radiation platform unresponsive…  Press any key." ) );
                }
            }
            break;
        // remove shock vent fields; check for existing plutonium generators in radius
        case COMPACT_DEACTIVATE_SHOCK_VENT:
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
                print_error( _( "Fuse reseted." ) );
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
            break;

    } // switch (action)
}

void computer_session::activate_random_failure()
{
    comp.next_attempt = calendar::turn + 45_minutes;
    static const computer_failure default_failure( COMPFAIL_SHUTDOWN );
    const computer_failure &fail = random_entry( comp.failures, default_failure );
    activate_failure( fail.type );
}

void computer_session::activate_failure( computer_failure_type fail )
{
    bool found_tile = false;
    switch( fail ) {

        case COMPFAIL_NULL:
        // Unknown action.
        case NUM_COMPUTER_FAILURES:
            // Suppress compiler warning [-Wswitch]
            break;

        case COMPFAIL_SHUTDOWN:
            for( const tripoint &p : g->m.points_in_radius( g->u.pos(), 1 ) ) {
                if( g->m.has_flag( "CONSOLE", p ) ) {
                    g->m.ter_set( p, t_console_broken );
                    add_msg( m_bad, _( "The console shuts down." ) );
                    found_tile = true;
                }
            }
            if( found_tile ) {
                break;
            }
            for( const tripoint &p : g->m.points_on_zlevel() ) {
                if( g->m.has_flag( "CONSOLE", p ) ) {
                    g->m.ter_set( p, t_console_broken );
                    add_msg( m_bad, _( "The console shuts down." ) );
                }
            }
            break;

        case COMPFAIL_ALARM:
            g->events().send<event_type::triggers_alarm>( g->u.getID() );
            sounds::sound( g->u.pos(), 60, sounds::sound_t::alarm, _( "an alarm sound!" ), false, "environment",
                           "alarm" );
            if( g->get_levz() > 0 && !g->timed_events.queued( TIMED_EVENT_WANTED ) ) {
                g->timed_events.add( TIMED_EVENT_WANTED, calendar::turn + 30_minutes, 0,
                                     g->u.global_sm_location() );
            }
            break;

        case COMPFAIL_MANHACKS: {
            int num_robots = rng( 4, 8 );
            const tripoint_range range = g->m.points_in_radius( g->u.pos(), 3 );
            for( int i = 0; i < num_robots; i++ ) {
                if( g->place_critter_within( mon_manhack, range ) ) {
                    add_msg( m_warning, _( "Manhacks drop from compartments in the ceiling." ) );
                }
            }
        }
        break;

        case COMPFAIL_SECUBOTS: {
            int num_robots = 1;
            const tripoint_range range = g->m.points_in_radius( g->u.pos(), 3 );
            for( int i = 0; i < num_robots; i++ ) {
                if( g->place_critter_within( mon_secubot, range ) ) {
                    add_msg( m_warning, _( "Secubots emerge from compartments in the floor." ) );
                }
            }
        }
        break;

        case COMPFAIL_DAMAGE:
            add_msg( m_neutral, _( "The console shocks you." ) );
            if( g->u.is_elec_immune() ) {
                add_msg( m_good, _( "You're protected from electric shocks." ) );
            } else {
                add_msg( m_bad, _( "Your body is damaged by the electric shock!" ) );
                g->u.hurtall( rng( 1, 10 ), nullptr );
            }
            break;

        case COMPFAIL_PUMP_EXPLODE:
            add_msg( m_warning, _( "The pump explodes!" ) );
            for( const tripoint &p : g->m.points_on_zlevel() ) {
                if( g->m.ter( p ) == t_sewage_pump ) {
                    g->m.make_rubble( p );
                    explosion_handler::explosion( p, 10 );
                }
            }
            break;

        case COMPFAIL_PUMP_LEAK:
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
            break;

        case COMPFAIL_AMIGARA:
            g->timed_events.add( TIMED_EVENT_AMIGARA, calendar::turn + 30_seconds );
            g->u.add_effect( effect_amigara, 2_minutes );
            explosion_handler::explosion( tripoint( rng( 0, MAPSIZE_X ), rng( 0, MAPSIZE_Y ), g->get_levz() ),
                                          10,
                                          0.7, false, 10 );
            explosion_handler::explosion( tripoint( rng( 0, MAPSIZE_X ), rng( 0, MAPSIZE_Y ), g->get_levz() ),
                                          10,
                                          0.7, false, 10 );
            comp.remove_option( COMPACT_AMIGARA_START );
            break;

        case COMPFAIL_DESTROY_BLOOD:
            print_error( _( "ERROR: Disruptive Spin" ) );
            for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 2 ) ) {
                if( g->m.ter( dest ) == t_centrifuge ) {
                    map_stack items = g->m.i_at( dest );
                    if( items.empty() ) {
                        print_error( _( "ERROR: Please place sample in centrifuge." ) );
                    } else if( items.size() > 1 ) {
                        print_error( _( "ERROR: Please remove all but one sample from centrifuge." ) );
                    } else if( items.only_item().typeId() != "vacutainer" ) {
                        print_error( _( "ERROR: Please use blood-contained samples." ) );
                    } else if( items.only_item().contents.empty() ) {
                        print_error( _( "ERROR: Blood draw kit, empty." ) );
                    } else if( items.only_item().contents.front().typeId() != "blood" ) {
                        print_error( _( "ERROR: Please only use blood samples." ) );
                    } else {
                        print_error( _( "ERROR: Blood sample destroyed." ) );
                        g->m.i_clear( dest );
                    }
                }
            }
            inp_mngr.wait_for_any_key();
            break;

        case COMPFAIL_DESTROY_DATA:
            print_error( _( "ERROR: ACCESSING DATA MALFUNCTION" ) );
            for( const tripoint &p : g->m.points_in_radius( g->u.pos(), 24 ) ) {
                if( g->m.ter( p ) == t_floor_blue ) {
                    map_stack items = g->m.i_at( p );
                    if( items.empty() ) {
                        print_error( _( "ERROR: Please place memory bank in scan area." ) );
                    } else if( items.size() > 1 ) {
                        print_error( _( "ERROR: Please only scan one item at a time." ) );
                    } else if( items.only_item().typeId() != "usb_drive" ) {
                        print_error( _( "ERROR: Memory bank destroyed or not present." ) );
                    } else if( items.only_item().contents.empty() ) {
                        print_error( _( "ERROR: Memory bank is empty." ) );
                    } else {
                        print_error( _( "ERROR: Data bank destroyed." ) );
                        g->m.i_clear( p );
                    }
                }
            }
            inp_mngr.wait_for_any_key();
            break;

    }// switch (fail)
}

void computer_session::mark_refugee_center()
{
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
}

template<typename ...Args>
bool computer_session::query_bool( const char *const text, Args &&... args )
{
    const std::string formatted_text = string_format( text, std::forward<Args>( args )... );
    print_line( "%s (Y/N/Q)", formatted_text );
    char ret;
#if defined(__ANDROID__)
    input_context ctxt( "COMPUTER_YESNO" );
    ctxt.register_manual_key( 'Y' );
    ctxt.register_manual_key( 'N' );
    ctxt.register_manual_key( 'Q' );
#endif
    do {
        // TODO: use input context
        ret = inp_mngr.get_input_event().get_first_input();
    } while( ret != 'y' && ret != 'Y' && ret != 'n' && ret != 'N' && ret != 'q' &&
             ret != 'Q' );
    return ( ret == 'y' || ret == 'Y' );
}

template<typename ...Args>
bool computer_session::query_any( const char *const text, Args &&... args )
{
    const std::string formatted_text = string_format( text, std::forward<Args>( args )... );
    print_line( "%s", formatted_text );
    inp_mngr.wait_for_any_key();
    return true;
}

template<typename ...Args>
char computer_session::query_ynq( const char *const text, Args &&... args )
{
    const std::string formatted_text = string_format( text, std::forward<Args>( args )... );
    print_line( "%s (Y/N/Q)", formatted_text );
    char ret;
#if defined(__ANDROID__)
    input_context ctxt( "COMPUTER_YESNO" );
    ctxt.register_manual_key( 'Y' );
    ctxt.register_manual_key( 'N' );
    ctxt.register_manual_key( 'Q' );
#endif
    do {
        // TODO: use input context
        ret = inp_mngr.get_input_event().get_first_input();
    } while( ret != 'y' && ret != 'Y' && ret != 'n' && ret != 'N' && ret != 'q' &&
             ret != 'Q' );
    return ret;
}

template<typename ...Args>
void computer_session::print_line( const char *const text, Args &&... args )
{
    const std::string formatted_text = string_format( text, std::forward<Args>( args )... );
    wprintz( w_terminal, c_green, formatted_text );
    print_newline();
    wrefresh( w_terminal );
}

template<typename ...Args>
void computer_session::print_error( const char *const text, Args &&... args )
{
    const std::string formatted_text = string_format( text, std::forward<Args>( args )... );
    wprintz( w_terminal, c_red, formatted_text );
    print_newline();
    wrefresh( w_terminal );
}

template<typename ...Args>
void computer_session::print_text( const char *const text, Args &&... args )
{
    const std::string formated_text = string_format( text, std::forward<Args>( args )... );
    int y = getcury( w_terminal );
    int w = getmaxx( w_terminal ) - 2;
    fold_and_print( w_terminal, point( 1, y ), w, c_green, formated_text );
    print_newline();
    print_newline();
    wrefresh( w_terminal );
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
    wprintz( w_terminal, c_yellow, gibberish );
    print_newline();
    wrefresh( w_terminal );
}

void computer_session::reset_terminal()
{
    werase( w_terminal );
    wmove( w_terminal, point_zero );
    wrefresh( w_terminal );
}

void computer_session::print_newline()
{
    wprintz( w_terminal, c_green, "\n" );
}
