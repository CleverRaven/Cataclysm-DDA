#include "computer.h"

#include <algorithm>
#include <sstream>
#include <string>

#include "coordinate_conversions.h"
#include "debug.h"
#include "effect.h"
#include "event.h"
#include "field.h"
#include "game.h"
#include "input.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "mission.h"
#include "monster.h"
#include "mtype.h"
#include "output.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "player.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translations.h"
#include "trap.h"

const mtype_id mon_manhack( "mon_manhack" );
const mtype_id mon_secubot( "mon_secubot" );
const mtype_id mon_turret( "mon_turret" );
const mtype_id mon_turret_rifle( "mon_turret_rifle" );

const skill_id skill_computer( "computer" );

const species_id ZOMBIE( "ZOMBIE" );
const species_id HUMAN( "HUMAN" );

const efftype_id effect_amigara( "amigara" );
const efftype_id effect_mending( "mending" );

int alerts = 0;

computer_option::computer_option()
    : name( "Unknown" ), action( COMPACT_NULL ), security( 0 )
{
}

computer_option::computer_option( std::string N, computer_action A, int S )
    : name( N ), action( A ), security( S )
{
}

computer::computer( const std::string &new_name, int new_security ): name( new_name )
{
    security = new_security;
    mission_id = -1;
}

computer::computer( const computer &rhs )
{
    *this = rhs;
}

computer::~computer() = default;

computer &computer::operator=( const computer &rhs )
{
    security = rhs.security;
    name = rhs.name;
    mission_id = rhs.mission_id;
    options = rhs.options;
    failures = rhs.failures;
    w_terminal = catacurses::window();
    w_border = catacurses::window();
    return *this;
}

void computer::set_security( int Security )
{
    security = Security;
}

void computer::add_option( const computer_option &opt )
{
    options.emplace_back( opt );
}

void computer::add_option( const std::string &opt_name, computer_action action,
                           int security )
{
    add_option( computer_option( opt_name, action, security ) );
}

void computer::add_failure( const computer_failure &failure )
{
    failures.emplace_back( failure );
}

void computer::add_failure( computer_failure_type failure )
{
    add_failure( computer_failure( failure ) );
}

void computer::shutdown_terminal()
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

void computer::use()
{
    if( !w_border ) {
        w_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                       ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0,
                                       ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 );
    }
    if( !w_terminal ) {
        w_terminal = catacurses::newwin( getmaxy( w_border ) - 2, getmaxx( w_border ) - 2,
                                         getbegy( w_border ) + 1, getbegx( w_border ) + 1 );
    }
    draw_border( w_border );
    wrefresh( w_border );

    // Login
    print_line( _( "Logging into %s..." ), _( name.c_str() ) );
    if( security > 0 ) {
        if( calendar::turn < next_attempt ) {
            print_error( _( "Access is temporary blocked for security purposes." ) );
            query_any( _( "Please contact the system administrator." ) );
            reset_terminal();
            return;
        }
        print_error( _( "ERROR!  Access denied!" ) );
        switch( query_ynq( _( "Bypass security?" ) ) ) {
            case 'q':
            case 'Q':
                shutdown_terminal();
                return;

            case 'n':
            case 'N':
                query_any( _( "Shutting down... press any key." ) );
                shutdown_terminal();
                return;

            case 'y':
            case 'Y':
                if( !hack_attempt( g->u ) ) {
                    if( failures.empty() ) {
                        query_any( _( "Maximum login attempts exceeded. Press any key..." ) );
                        shutdown_terminal();
                        return;
                    }
                    activate_random_failure();
                    shutdown_terminal();
                    return;
                } else { // Successful hack attempt
                    security = 0;
                    query_any( _( "Login successful.  Press any key..." ) );
                    reset_terminal();
                }
        }
    } else { // No security
        query_any( _( "Login successful.  Press any key..." ) );
        reset_terminal();
    }

    // Main computer loop
    while( true ) {
        //reset_terminal();
        size_t options_size = options.size();
        print_newline();
        print_line( "%s - %s", _( name.c_str() ), _( "Root Menu" ) );
#ifdef __ANDROID__
        input_context ctxt( "COMPUTER_MAINLOOP" );
#endif
        for( size_t i = 0; i < options_size; i++ ) {
            print_line( "%d - %s", i + 1, _( options[i].name.c_str() ) );
#ifdef __ANDROID__
            ctxt.register_manual_key( '1' + i, options[i].name.c_str() );
#endif
        }
        print_line( "Q - %s", _( "Quit and shut down" ) );
        print_newline();
#ifdef __ANDROID__
        ctxt.register_manual_key( 'Q', _( "Quit and shut down" ) );
#endif
        char ch;
        do {
            // TODO: use input context
            ch = inp_mngr.get_input_event().get_first_input();
        } while( ch != 'q' && ch != 'Q' && ( ch < '1' || ch - '1' >= static_cast<char>( options_size ) ) );
        if( ch == 'q' || ch == 'Q' ) {
            break; // Exit from main computer loop
        } else { // We selected an option other than quit.
            ch -= '1'; // So '1' -> 0; index in options.size()
            computer_option current = options[ch];
            // Once you trip the security, you have to roll every time you want to do something
            if( ( current.security + ( alerts ) ) > 0 ) {
                print_error( _( "Password required." ) );
                if( query_bool( _( "Hack into system?" ) ) ) {
                    if( !hack_attempt( g->u, current.security ) ) {
                        activate_random_failure();
                        shutdown_terminal();
                        return;
                    } else {
                        // Successfully hacked function
                        options[ch].security = 0;
                        activate_function( current.action );
                    }
                }
            } else { // No need to hack, just activate
                activate_function( current.action );
            }
            reset_terminal();
        } // Done processing a selected option.
    }

    shutdown_terminal(); // This should have been done by now, but just in case.
}

bool computer::hack_attempt( player &p, int Security )
{
    if( Security == -1 ) {
        Security = security;    // Set to main system security if no value passed
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

std::string computer::save_data() const
{
    std::ostringstream data;

    data.imbue( std::locale::classic() );

    data
            << string_replace( name, " ", "_" ) << ' '
            << security << ' '
            << mission_id << ' '
            << options.size() << ' ';

    for( auto &elem : options ) {
        data
                << string_replace( elem.name, " ", "_" ) << ' '
                << static_cast<int>( elem.action ) << ' '
                << elem.security << ' ';
    }

    data << failures.size() << ' ';
    for( auto &elem : failures ) {
        data << static_cast<int>( elem.type ) << ' ';
    }

    return data.str();
}

void computer::load_data( const std::string &data )
{
    options.clear();
    failures.clear();

    std::istringstream dump( data );
    dump.imbue( std::locale::classic() );

    dump >> name >> security >> mission_id;

    name = string_replace( name, "_", " " );

    // Pull in options
    int optsize;
    dump >> optsize;
    for( int n = 0; n < optsize; n++ ) {
        std::string tmpname;

        int tmpaction;
        int tmpsec;

        dump >> tmpname >> tmpaction >> tmpsec;

        add_option( string_replace( tmpname, "_", " " ), computer_action( tmpaction ), tmpsec );
    }

    // Pull in failures
    int failsize;
    dump >> failsize;
    for( int n = 0; n < failsize; n++ ) {
        int tmpfail;
        dump >> tmpfail;
        add_failure( computer_failure_type( tmpfail ) );
    }
}

static item *pick_usb()
{
    const int pos = g->inv_for_id( itype_id( "usb_drive" ), _( "Choose drive:" ) );
    if( pos != INT_MIN ) {
        return &g->u.i_at( pos );
    }
    return nullptr;
}

static void remove_submap_turrets()
{
    for( monster &critter : g->all_monsters() ) {
        // Check 1) same overmap coords, 2) turret, 3) hostile
        if( ms_to_omt_copy( g->m.getabs( critter.pos() ) ) == ms_to_omt_copy( g->m.getabs( g->u.pos() ) ) &&
            ( critter.type->id == mon_turret ||
              critter.type->id == mon_turret_rifle ) &&
            critter.attitude_to( g->u ) == Creature::Attitude::A_HOSTILE ) {
            g->remove_zombie( critter );
        }
    }
}

void computer::activate_function( computer_action action )
{
    // Token move cost for any action, if an action takes longer decrement moves further.
    g->u.moves -= 30;
    switch( action ) {

        case COMPACT_NULL: // Unknown action.
        case NUM_COMPUTER_ACTIONS: // Suppress compiler warning [-Wswitch]
            break;

        // OPEN_DISARM falls through to just OPEN
        case COMPACT_OPEN_DISARM:
            remove_submap_turrets();
        /* fallthrough */
        case COMPACT_OPEN:
            g->m.translate_radius( t_door_metal_locked, t_floor, 25.0, g->u.pos(), true );
            query_any( _( "Doors opened.  Press any key..." ) );
            break;

        //LOCK AND UNLOCK are used to build more complex buildings
        // that can have multiple doors that can be locked and be
        // unlocked by different computers.
        //Simply uses translate_radius which take a given radius and
        // player position to determine which terrain tiles to edit.
        case COMPACT_LOCK:
            g->m.translate_radius( t_door_metal_c, t_door_metal_locked, 8.0, g->u.pos(), true );
            query_any( _( "Lock enabled.  Press any key..." ) );
            break;

        // UNLOCK_DISARM falls through to just UNLOCK
        case COMPACT_UNLOCK_DISARM:
            remove_submap_turrets();
        /* fallthrough */
        case COMPACT_UNLOCK:
            g->m.translate_radius( t_door_metal_locked, t_door_metal_c, 8.0, g->u.pos(), true );
            query_any( _( "Lock disabled.  Press any key..." ) );
            break;

        //Toll is required for the church computer/mechanism to function
        case COMPACT_TOLL:
            //~ the sound of a church bell ringing
            sounds::sound( g->u.pos(), 120, sounds::sound_t::music,
                           _( "Bohm... Bohm... Bohm..." ) );
            break;

        case COMPACT_SAMPLE:
            g->u.moves -= 30;
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    if( g->m.ter( x, y ) == t_sewage_pump ) {
                        for( int x1 = x - 1; x1 <= x + 1; x1++ ) {
                            for( int y1 = y - 1; y1 <= y + 1; y1++ ) {
                                if( g->m.furn( x1, y1 ) == f_counter ) {
                                    bool found_item = false;
                                    item sewage( "sewage", calendar::turn );
                                    auto candidates = g->m.i_at( x1, y1 );
                                    for( auto &candidate : candidates ) {
                                        long capa = candidate.get_remaining_capacity_for_liquid( sewage );
                                        if( capa <= 0 ) {
                                            continue;
                                        }
                                        item &elem = candidate;
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
                                        g->m.add_item_or_charges( x1, y1, sewage );
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;

        case COMPACT_RELEASE:
            g->u.add_memorial_log( pgettext( "memorial_male", "Released subspace specimens." ),
                                   pgettext( "memorial_female", "Released subspace specimens." ) );
            sounds::sound( g->u.pos(), 40, sounds::sound_t::alarm, _( "an alarm sound!" ) );
            g->m.translate_radius( t_reinforced_glass, t_thconc_floor, 25.0, g->u.pos(), true );
            query_any( _( "Containment shields opened.  Press any key..." ) );
            break;

        // COMPACT_RELEASE_DISARM falls through to just COMPACT_RELEASE_BIONICS
        case COMPACT_RELEASE_DISARM:
            remove_submap_turrets();
        /* fallthrough */
        case COMPACT_RELEASE_BIONICS:
            sounds::sound( g->u.pos(), 40, sounds::sound_t::alarm, _( "an alarm sound!" ) );
            g->m.translate_radius( t_reinforced_glass, t_thconc_floor, 3.0, g->u.pos(), true );
            query_any( _( "Containment shields opened.  Press any key..." ) );
            break;

        case COMPACT_TERMINATE:
            g->u.add_memorial_log( pgettext( "memorial_male", "Terminated subspace specimens." ),
                                   pgettext( "memorial_female", "Terminated subspace specimens." ) );
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    tripoint p( x, y, g->u.posz() );
                    monster *const mon = g->critter_at<monster>( p );
                    if( mon &&
                        ( ( g->m.ter( x, y - 1 ) == t_reinforced_glass &&
                            g->m.ter( x, y + 1 ) == t_concrete_wall ) ||
                          ( g->m.ter( x, y + 1 ) == t_reinforced_glass &&
                            g->m.ter( x, y - 1 ) == t_concrete_wall ) ) ) {
                        mon->die( &g->u );
                    }
                }
            }
            query_any( _( "Subjects terminated.  Press any key..." ) );
            break;

        case COMPACT_PORTAL: {
            g->u.add_memorial_log( pgettext( "memorial_male", "Opened a portal." ),
                                   pgettext( "memorial_female", "Opened a portal." ) );
            tripoint tmp = g->u.pos();
            int &i = tmp.x;
            int &j = tmp.y;
            for( i = 0; i < MAPSIZE_X; i++ ) {
                for( j = 0; j < MAPSIZE_Y; j++ ) {
                    int numtowers = 0;
                    tripoint tmp2 = tmp;
                    int &xt = tmp2.x;
                    int &yt = tmp2.y;
                    for( xt = i - 2; xt <= i + 2; xt++ ) {
                        for( yt = j - 2; yt <= j + 2; yt++ ) {
                            if( g->m.ter( tmp2 ) == t_radio_tower ) {
                                numtowers++;
                            }
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
        }
        break;

        case COMPACT_CASCADE: {
            if( !query_bool( _( "WARNING: Resonance cascade carries severe risk!  Continue?" ) ) ) {
                return;
            }
            g->u.add_memorial_log( pgettext( "memorial_male", "Caused a resonance cascade." ),
                                   pgettext( "memorial_female", "Caused a resonance cascade." ) );
            std::vector<tripoint> cascade_points;
            for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 10 ) ) {
                if( g->m.ter( dest ) == t_radio_tower ) {
                    cascade_points.push_back( dest );
                }
            }
            g->resonance_cascade( random_entry( cascade_points, g->u.pos() ) );
        }
        break;

        case COMPACT_RESEARCH: {
            // TODO: seed should probably be a member of the computer, or better: of the computer action.
            // It is here to ensure one computer reporting the same text on each invocation.
            const int seed = g->get_levx() + g->get_levy() + g->get_levz() + alerts;
            std::string log = SNIPPET.get( SNIPPET.assign( "lab_notes", seed ) );
            if( log.empty() ) {
                log = _( "No data found." );
            } else {
                g->u.moves -= 70;
            }

            print_text( "%s", log.c_str() );
            // One's an anomaly
            if( alerts == 0 ) {
                query_any( _( "Local data-access error logged, alerting helpdesk. Press any key..." ) );
                alerts ++;
            } else {
                // Two's a trend.
                query_any(
                    _( "Warning: anomalous archive-access activity detected at this node. Press any key..." ) );
                alerts ++;
            }
        }
        break;

        case COMPACT_MAPS: {
            g->u.moves -= 30;
            const tripoint center = g->u.global_omt_location();
            overmap_buffer.reveal( point( center.x, center.y ), 40, 0 );
            query_any(
                _( "Surface map data downloaded.  Local anomalous-access error logged.  Press any key..." ) );
            remove_option( COMPACT_MAPS );
            alerts ++;
        }
        break;

        case COMPACT_MAP_SEWER: {
            g->u.moves -= 30;
            const tripoint center = g->u.global_omt_location();
            for( int i = -60; i <= 60; i++ ) {
                for( int j = -60; j <= 60; j++ ) {
                    const oter_id &oter = overmap_buffer.ter( center.x + i, center.y + j, center.z );
                    if( is_ot_type( "sewer", oter ) || is_ot_type( "sewage", oter ) ) {
                        overmap_buffer.set_seen( center.x + i, center.y + j, center.z, true );
                    }
                }
            }
            query_any( _( "Sewage map data downloaded.  Press any key..." ) );
            remove_option( COMPACT_MAP_SEWER );
        }
        break;

        case COMPACT_MAP_SUBWAY: {
            g->u.moves -= 30;
            const tripoint center = g->u.global_omt_location();
            for( int i = -60; i <= 60; i++ ) {
                for( int j = -60; j <= 60; j++ ) {
                    const oter_id &oter = overmap_buffer.ter( center.x + i, center.y + j, center.z );
                    if( is_ot_type( "subway", oter ) || is_ot_subtype( "lab_train_depot", oter ) ) {
                        overmap_buffer.set_seen( center.x + i, center.y + j, center.z, true );
                    }
                }
            }
            query_any( _( "Subway map data downloaded.  Press any key..." ) );
            remove_option( COMPACT_MAP_SUBWAY );
        }
        break;

        case COMPACT_MISS_LAUNCH: {
            // Target Acquisition.
            tripoint target = ui::omap::choose_point( 0 );
            if( target == overmap::invalid_tripoint ) {
                add_msg( m_info, _( "Target acquisition canceled." ) );
                return;
            }
            if( query_yn( _( "Confirm nuclear missile launch." ) ) ) {
                add_msg( m_info, _( "Nuclear missile launched!" ) );
                options.clear();//Remove the option to fire another missile.
            } else {
                add_msg( m_info, _( "Nuclear missile launch aborted." ) );
                return;
            }
            g->refresh_all();

            //Put some smoke gas and explosions at the nuke location.
            for( int i = g->u.posx() + 8; i < g->u.posx() + 15; i++ ) {
                for( int j = g->u.posy() + 3; j < g->u.posy() + 12; j++ )
                    if( !one_in( 4 ) ) {
                        tripoint dest( i + rng( -2, 2 ), j + rng( -2, 2 ), g->u.posz() );
                        g->m.add_field( dest, fd_smoke, rng( 1, 9 ) );
                    }
            }

            g->explosion( tripoint( g->u.posx() + 10, g->u.posx() + 21, g->get_levz() ), 200, 0.7,
                          true ); //Only explode once. But make it large.

            //...ERASE MISSILE, OPEN SILO, DISABLE COMPUTER
            // For each level between here and the surface, remove the missile
            for( int level = g->get_levz(); level <= 0; level++ ) {
                map tmpmap;
                tmpmap.load( g->get_levx(), g->get_levy(), level, false );

                if( level < 0 ) {
                    tmpmap.translate( t_missile, t_hole );
                } else {
                    tmpmap.translate( t_metal_floor, t_hole );
                }
                tmpmap.save();
            }

            const oter_id oter = overmap_buffer.ter( target.x, target.y, 0 );
            //~ %s is terrain name
            g->u.add_memorial_log( pgettext( "memorial_male", "Launched a nuke at a %s." ),
                                   pgettext( "memorial_female", "Launched a nuke at a %s." ),
                                   oter->get_name().c_str() );
            for( int x = target.x - 2; x <= target.x + 2; x++ ) {
                for( int y = target.y - 2; y <= target.y + 2; y++ ) {
                    // give it a nice rounded shape
                    if( !( x == ( target.x - 2 ) && ( y == ( target.y - 2 ) ) ) &&
                        !( x == ( target.x - 2 ) && ( y == ( target.y + 2 ) ) ) &&
                        !( x == ( target.x + 2 ) && ( y == ( target.y - 2 ) ) ) &&
                        !( x == ( target.x + 2 ) && ( y == ( target.y + 2 ) ) ) ) {
                        // TODO: Z
                        g->nuke( tripoint( x, y, 0 ) );
                    }

                }
            }

            activate_failure( COMPFAIL_SHUTDOWN );
        }
        break;

        case COMPACT_MISS_DISARM: // TODO: stop the nuke from creating radioactive clouds.
            if( query_yn( _( "Disarm missile." ) ) ) {
                g->u.add_memorial_log( pgettext( "memorial_male", "Disarmed a nuclear missile." ),
                                       pgettext( "memorial_female", "Disarmed a nuclear missile." ) );
                add_msg( m_info, _( "Nuclear missile disarmed!" ) );
                options.clear();//disable missile.
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
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    for( auto &elem : g->m.i_at( x, y ) ) {
                        if( elem.is_bionic() ) {
                            if( static_cast<int>( names.size() ) < TERMY - 8 ) {
                                names.push_back( elem.tname() );
                            } else {
                                more++;
                            }
                        }
                    }
                }
            }

            reset_terminal();

            print_newline();
            print_line( _( "Bionic access - Manifest:" ) );
            print_newline();

            for( auto &name : names ) {
                print_line( "%s", name.c_str() );
            }
            if( more > 0 ) {
                print_line( ngettext( "%d OTHER FOUND...", "%d OTHERS FOUND...", more ), more );
            }

            print_newline();
            query_any( _( "Press any key..." ) );
        }
        break;

        case COMPACT_ELEVATOR_ON:
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    if( g->m.ter( x, y ) == t_elevator_control_off ) {
                        g->m.ter_set( x, y, t_elevator_control );
                    }
                }
            }
            query_any( _( "Elevator activated.  Press any key..." ) );
            break;

        case COMPACT_AMIGARA_LOG: // TODO: This is static, move to data file?
            g->u.moves -= 30;
            reset_terminal();
            print_line( _( "NEPower Mine(%d:%d) Log" ), g->get_levx(), g->get_levy() );
            print_line( _( "\
ENTRY 47:\n\
Our normal mining routine has unearthed a hollow chamber.  This would not be\n\
out of the ordinary, save for the odd, perfectly vertical faultline found.\n\
This faultline has several odd concavities in it which have the more\n\
superstitious crew members alarmed; they seem to be of human origin.\n\
\n\
ENTRY 48:\n\
The concavities are between 10 and 20 feet tall, and run the length of the\n\
faultline.  Each one is vaguely human in shape, but with the proportions of\n\
the limbs, neck and head greatly distended, all twisted and curled in on\n\
themselves.\n" ) );
            if( !query_bool( _( "Continue reading?" ) ) ) {
                return;
            }
            g->u.moves -= 30;
            reset_terminal();
            print_line( _( "NEPower Mine(%d:%d) Log" ), g->get_levx(), g->get_levy() );
            print_line( _( "\
ENTRY 49:\n\
We've stopped mining operations in this area, obviously, until archaeologists\n\
have the chance to inspect the area.  This is going to set our schedule back\n\
by at least a week.  This stupid artifact-preservation law has been in place\n\
for 50 years, and hasn't even been up for termination despite the fact that\n\
these mining operations are the backbone of our economy.\n\
\n\
ENTRY 52:\n\
Still waiting on the archaeologists.  We've done a little light inspection of\n\
the faultline; our sounding equipment is insufficient to measure the depth of\n\
the concavities.  The equipment is rated at 15 miles depth, but it isn't made\n\
for such narrow tunnels, so it's hard to say exactly how far back they go.\n" ) );
            if( !query_bool( _( "Continue reading?" ) ) ) {
                return;
            }
            g->u.moves -= 30;
            reset_terminal();
            print_line( _( "NEPower Mine(%d:%d) Log" ), g->get_levx(), g->get_levy() );
            print_line( _( "\
ENTRY 54:\n\
I noticed a couple of the guys down in the chamber with a chisel, breaking\n\
off a piece of the sheer wall.  I'm looking the other way.  It's not like\n\
the eggheads are going to notice a little piece missing.  Fuck em.\n\
\n\
ENTRY 55:\n\
Well, the archaeologists are down there now with a couple of the boys as\n\
guides.  They're hardly Indiana Jones types; I doubt they been below 20\n\
feet.  I hate taking guys off assignment just to babysit the scientists, but\n\
if they get hurt we'll be shut down for god knows how long.\n\
\n\
ENTRY 58:\n\
They're bringing in ANOTHER CREW?  Christ, it's just some cave carvings!  I\n\
know that's sort of a big deal, but come on, these guys can't handle it?\n" ) );
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
            print_line( _( "\
SITE %d%d%d\n\
PERTINENT FOREMAN LOGS WILL BE PREPENDED TO NOTES" ),
                        g->get_levx(), g->get_levy(), abs( g->get_levz() ) );
            print_line( _( "\n\
MINE OPERATIONS SUSPENDED; CONTROL TRANSFERRED TO AMIGARA PROJECT UNDER\n\
   IMPERATIVE 2:07B\n\
FAULTLINE SOUNDING HAS PLACED DEPTH AT 30.09 KM\n\
DAMAGE TO FAULTLINE DISCOVERED; NEPOWER MINE CREW PLACED UNDER ARREST FOR\n\
   VIOLATION OF REGULATION 87.08 AND TRANSFERRED TO LAB 89-C FOR USE AS\n\
   SUBJECTS\n\
QUALITY OF FAULTLINE NOT COMPROMISED\n\
INITIATING STANDARD TREMOR TEST..." ) );
            print_gibberish_line();
            print_gibberish_line();
            print_newline();
            print_error( _( "FILE CORRUPTED, PRESS ANY KEY..." ) );
            inp_mngr.wait_for_any_key();
            reset_terminal();
            break;

        case COMPACT_AMIGARA_START:
            g->events.add( EVENT_AMIGARA, calendar::turn + 10_turns );
            if( !g->u.has_artifact_with( AEP_PSYSHIELD ) ) {
                g->u.add_effect( effect_amigara, 2_minutes );
            }
            // Disable this action to prevent further amigara events, which would lead to
            // further amigara monster, which would lead to further artifacts.
            remove_option( COMPACT_AMIGARA_START );
            break;

        case COMPACT_BONESETTING:
            for( int i = 0; i < num_hp_parts; i++ ) {
                const bool broken = g->u.get_hp( static_cast<hp_part>( i ) ) <= 0;
                body_part part = g->u.hp_to_bp( static_cast<hp_part>( i ) );
                effect &existing_effect = g->u.get_effect( effect_mending, part );
                if( !broken || ( !existing_effect.is_null() &&
                                 existing_effect.get_duration() <
                                 existing_effect.get_max_duration() - 5_days ) ) {
                    continue;
                }
                g->u.moves -= 500;
                print_line( _( "The machine rapidly sets and splints your broken %s." ),
                            body_part_name( part ).c_str() );
                // TODO: fail here if unable to perform the action, i.e. can't wear more, trait mismatch.
                if( !g->u.worn_with_flag( "SPLINT", part ) ) {
                    item splint;
                    if( i == hp_arm_l || i == hp_arm_r ) {
                        splint = item( "arm_splint", 0 );
                    } else if( i == hp_leg_l || i == hp_leg_r ) {
                        splint = item( "leg_splint", 0 );
                    }
                    item &equipped_splint = g->u.i_add( splint );
                    cata::optional<std::list<item>::iterator> worn_item =
                        g->u.wear( equipped_splint, false );
                    if( worn_item && !g->u.worn_with_flag( "SPLINT", part ) ) {
                        g->u.change_side( **worn_item, false );
                    }
                }
                g->u.add_effect( effect_mending, 0, part, true );
                effect &mending_effect = g->u.get_effect( effect_mending, part );
                mending_effect.set_duration( mending_effect.get_max_duration() - 5_days );
            }
            query_any( _( "Press any key..." ) );
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
                        print_error( _( "Repeater mod installed..." ) );
                        print_error( _( "Mission Complete!" ) );
                        g->u.use_amount( "radio_repeater_mod", 1 );
                        inp_mngr.wait_for_any_key();
                        options.clear();
                        activate_failure( COMPFAIL_SHUTDOWN );
                        break;
                    }
                }
            } else {
                print_error( _( "You do not have a repeater mod to install..." ) );
                inp_mngr.wait_for_any_key();
                break;
            }
            break;

        case COMPACT_DOWNLOAD_SOFTWARE:
            if( item *const usb = pick_usb() ) {
                mission *miss = mission::find( mission_id );
                if( miss == nullptr ) {
                    debugmsg( _( "Computer couldn't find its mission!" ) );
                    return;
                }
                g->u.moves -= 30;
                item software( miss->get_item_id(), 0 );
                software.mission_id = mission_id;
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
                    if( g->m.i_at( dest ).empty() ) {
                        print_error( _( "ERROR: Please place sample in centrifuge." ) );
                    } else if( g->m.i_at( dest ).size() > 1 ) {
                        print_error( _( "ERROR: Please remove all but one sample from centrifuge." ) );
                    } else if( g->m.i_at( dest )[0].contents.empty() ) {
                        print_error( _( "ERROR: Please only use container with blood sample." ) );
                    } else if( g->m.i_at( dest )[0].contents.front().typeId() != "blood" ) {
                        print_error( _( "ERROR: Please only use blood samples." ) );
                    } else { // Success!
                        const item &blood = g->m.i_at( dest ).front().contents.front();
                        const mtype *mt = blood.get_mtype();
                        if( mt == nullptr || mt->id == mtype_id::NULL_ID() ) {
                            print_line( _( "Result:  Human blood, no pathogens found." ) );
                        } else if( mt->in_species( ZOMBIE ) ) {
                            if( mt->in_species( HUMAN ) ) {
                                print_line( _( "Result:  Human blood.  Unknown pathogen found." ) );
                            } else {
                                print_line( _( "Result:  Unknown blood type.  Unknown pathogen found." ) );
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
            query_any( _( "Press any key..." ) );
            break;

        case COMPACT_DATA_ANAL:
            g->u.moves -= 30;
            for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 2 ) ) {
                if( g->m.ter( dest ) == t_floor_blue ) {
                    print_error( _( "PROCESSING DATA" ) );
                    if( g->m.i_at( dest ).empty() ) {
                        print_error( _( "ERROR: Please place memory bank in scan area." ) );
                    } else if( g->m.i_at( dest ).size() > 1 ) {
                        print_error( _( "ERROR: Please only scan one item at a time." ) );
                    } else if( g->m.i_at( dest )[0].typeId() != "usb_drive" &&
                               g->m.i_at( dest )[0].typeId() != "black_box" ) {
                        print_error( _( "ERROR: Memory bank destroyed or not present." ) );
                    } else if( g->m.i_at( dest )[0].typeId() == "usb_drive" && g->m.i_at( dest )[0].contents.empty() ) {
                        print_error( _( "ERROR: Memory bank is empty." ) );
                    } else { // Success!
                        if( g->m.i_at( dest )[0].typeId() == "black_box" ) {
                            print_line( _( "Memory Bank:  Military Hexron Encryption\nPrinting Transcript\n" ) );
                            item transcript( "black_box_transcript", calendar::turn );
                            g->m.add_item_or_charges( g->u.posx(), g->u.posy(), transcript );
                        } else {
                            print_line( _( "Memory Bank:  Unencrypted\nNothing of interest.\n" ) );
                        }
                    }
                }
            }
            query_any( _( "Press any key..." ) );
            break;

        case COMPACT_DISCONNECT:
            reset_terminal();
            print_line( _( "\n\
ERROR:  NETWORK DISCONNECT \n\
UNABLE TO REACH NETWORK ROUTER OR PROXY.  PLEASE CONTACT YOUR\n\
SYSTEM ADMINISTRATOR TO RESOLVE THIS ISSUE.\n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_EMERG_MESS:
            print_line( _( "\
GREETINGS CITIZEN. A BIOLOGICAL ATTACK HAS TAKEN PLACE AND A STATE OF \n\
EMERGENCY HAS BEEN DECLARED. EMERGENCY PERSONNEL WILL BE AIDING YOU \n\
SHORTLY. TO ENSURE YOUR SAFETY PLEASE FOLLOW THE BELOW STEPS. \n\
\n\
1. DO NOT PANIC. \n\
2. REMAIN INSIDE THE BUILDING. \n\
3. SEEK SHELTER IN THE BASEMENT. \n\
4. USE PROVIDED GAS MASKS. \n\
5. AWAIT FURTHER INSTRUCTIONS \n\
\n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_EMERG_REF_CENTER:
            reset_terminal();
            mark_refugee_center();
            reset_terminal();
            break;

        case COMPACT_TOWER_UNRESPONSIVE:
            print_line( _( "\
  WARNING, RADIO TOWER IS UNRESPONSIVE. \n\
  \n\
  BACKUP POWER INSUFFICIENT TO MEET BROADCASTING REQUIREMENTS. \n\
  IN THE EVENT OF AN EMERGENCY, CONTACT LOCAL NATIONAL GUARD \n\
  UNITS TO RECEIVE PRIORITY WHEN GENERATORS ARE BEING DEPLOYED. \n\
  \n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SR1_MESS:
            reset_terminal();
            print_line( _( "\n\
  Subj: Security Reminder\n\
  To: all SRCF staff\n\
  From: Constantine Dvorak, Undersecretary of Nuclear Security\n\
  \n\
      I want to remind everyone on staff: Do not open or examine\n\
  containers above your security-clearance.  If you have some\n\
  question about safety protocols or shipping procedures, please\n\
  contact your SRCF administrator or on-site military officer.\n\
  When in doubt, assume all containers are Class-A Biohazards\n\
  and highly toxic. Take full precautions!\n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SR2_MESS:
            reset_terminal();
            print_line( _( "\n\
  Subj: Security Reminder\n\
  To: all SRCF staff\n\
  From: Constantine Dvorak, Undersecretary of Nuclear Security\n\
  \n\
  From today onward medical wastes are not to be stored anywhere\n\
  near radioactive materials.  All containers are to be\n\
  re-arranged according to these new regulations.  If your\n\
  facility currently has these containers stored in close\n\
  proximity, you are to work with armed guards on duty at all\n\
  times. Report any unusual activity to your SRCF administrator\n\
  at once.\n\
  " ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SR3_MESS:
            reset_terminal();
            print_line( _( "\n\
  Subj: Security Reminder\n\
  To: all SRCF staff\n\
  From: Constantine Dvorak, Undersecretary of Nuclear Security\n\
  \n\
  Worker health and safety is our number one concern!  As such,\n\
  we are instituting weekly health examinations for all SRCF\n\
  employees.  Report any unusual symptoms or physical changes\n\
  to your SRCF administrator at once.\n\
  " ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SR4_MESS:
            reset_terminal();
            print_line( _( "\n\
  Subj: Security Reminder\n\
  To: all SRCF staff\n\
  From:  Constantine Dvorak, Undersecretary of Nuclear Security\n\
  \n\
  All compromised facilities will remain under lock down until\n\
  further notice.  Anyone who has seen or come in direct contact\n\
  with the creatures is to report to the home office for a full\n\
  medical evaluation and security debriefing.\n\
  " ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SRCF_1_MESS:
            reset_terminal();
            print_line( _( "\n\
  Subj: EPA: Report All Potential Containment Breaches 3873643\n\
  To: all SRCF staff\n\
  From:  Robert Shane, Director of the EPA\n\
  \n\
  All hazardous waste dumps and sarcophagi must submit three\n\
  samples from each operational leache system to the following\n\
  addresses:\n\
  \n\
  CDC Bioterrorism Lab \n\
  Building 10\n\
  Corporate Square Boulevard\n\
  Atlanta, GA 30329\n\
  \n\
  EPA Region 8 Laboratory\n\
  16194 W. 45th Drive\n\
  Golden, Colorado 80403\n\
  \n\
  These samples must be accurate and any attempts to cover\n\
  incompetencies will result in charges of Federal Corruption\n\
  and potentially Treason.\n" ) );
            query_any( _( "Press any key to continue..." ) );
            reset_terminal();
            print_line( _( "Director of the EPA,\n\
  Robert Shane\n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SRCF_2_MESS:
            reset_terminal();
            print_line( _( " Subj: SRCF: Internal Memo, EPA [2918024]\n\
  To: all SRCF admin staff\n\
  From:  Constantine Dvorak, Undersecretary of Nuclear Security\n\
  \n\
  Director Grimes has released a new series of accusations that\n\
  will soon be investigated by a Congressional committee.  Below\n\
  is the message that he sent me.\n\
  \n\
  --------------------------------------------------------------\n\
  Subj: Congressional Investigations\n\
  To: Constantine Dvorak, Undersecretary of Nuclear Safety\n\
  From: Robert Shane, director of the EPA\n\
  \n\
      The EPA has opposed the Security-Restricted Containment\n\
  Facility (SRCF) project from its inception.  We were horrified\n\
  that these facilities would be constructed so close to populated\n\
  areas, and only agreed to sign-off on the project if we were\n\
  allowed to freely examine and monitor the sarcophagi.  But that\n\
  has not happened.  Since then, the DoE has employed any and all\n\
  means to keep EPA agents from visiting the SRCFs, using military\n\
  secrecy, emergency powers, and inter-departmental gag orders to\n" ) );
            query_any( _( "Press any key to continue..." ) );
            reset_terminal();
            print_line( _( " surround the project with an impenetrable thicket of red tape.\n\
  \n\
      Although our agents have not been allowed inside, our atmospheric\n\
  testers in nearby communities have detected high levels of toxins\n\
  and radiation, and we've found dozens of potentially dangerous\n\
  unidentified compounds in the ground water.  We now have\n\
  conclusive evidence that the SRCFs are a threat to the public\n\
  safety.  We are taking these data to state representatives and\n\
  petitioning for a full Congressional inquiry.  They should be\n\
  able to force open your secret vaults, and the world will see\n\
  what you've been hiding.\n\
  \n\
  If you had any hand in this outbreak I hope you rot in hell.\n\
  \n\
  Director of the EPA,\n\
  Robert Shane\n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SRCF_3_MESS:
            reset_terminal();
            print_line( _( " Subj: CDC: Internal Memo, Standby [2918115]\n\
  To: all SRCF staff\n\
  From:  Ellen Grimes, Director of the CDC\n\
  \n\
      Your site along with many others has been found to be\n\
  contaminated with what we will now refer to as [redacted].\n\
  It is vital that you standby for further orders.  We are\n\
  currently awaiting the President to decide our course of\n\
  action in this national crisis.  You will proceed with fail-\n\
  safe procedures and rig the sarcophagus with C-4 as outlined\n\
  in Publication 4423.  We will send you orders to either detonate\n\
  and seal the sarcophagus or remove the charges.  It is of the\n\
  utmost importance that the facility be sealed immediately when\n\
  the orders are given.  We have been alerted by Homeland Security\n\
  that there are potential terrorist suspects that are being\n\
  detained in connection with the recent national crisis.\n\
  \n\
  Director of the CDC,\n\
  Ellen Grimes\n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SRCF_SEAL_ORDER:
            reset_terminal();
            print_line( _( " Subj: USARMY: SEAL SRCF [987167]\n\
  To: all SRCF staff\n\
  From:  Major General Cornelius, U.S. Army\n\
  \n\
    As a general warning to all civilian staff: the 10th Mountain\n\
  Division has been assigned to oversee the sealing of the SRCF\n\
  facilities.  By direct order, all non-essential staff must vacate\n\
  at the earliest possible opportunity to prevent potential\n\
  contamination.  Low yield tactical nuclear demolition charges\n\
  will be deployed in the lower tunnels to ensure that recovery\n\
  of hazardous material is impossible.  The Army Corps of Engineers\n\
  will then dump concrete over the rubble so that we can redeploy \n\
  the 10th Mountain into the greater Boston area.\n\
  \n\
  Cornelius,\n\
  Major General, U.S. Army\n\
  Commander of the 10th Mountain Division\n\
  \n" ) );
            query_any( _( "Press any key to continue..." ) );
            break;

        case COMPACT_SRCF_SEAL:
            g->u.add_memorial_log( pgettext( "memorial_male", "Sealed a Hazardous Material Sarcophagus." ),
                                   pgettext( "memorial_female", "Sealed a Hazardous Material Sarcophagus." ) );
            print_line( _( "Charges Detonated" ) );
            print_line( _( "Backup Generator Power Failing" ) );
            print_line( _( "Evacuate Immediately" ) );
            add_msg( m_warning, _( "Evacuate Immediately!" ) );
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    tripoint p( x, y, g->get_levz() );
                    if( g->m.ter( x, y ) == t_elevator || g->m.ter( x, y ) == t_vat ) {
                        g->m.make_rubble( p, f_rubble_rock, true );
                        g->explosion( p, 40, 0.7, true );
                    }
                    if( g->m.ter( x, y ) == t_wall_glass ) {
                        g->m.make_rubble( p, f_rubble_rock, true );
                    }
                    if( g->m.ter( x, y ) == t_sewage_pipe || g->m.ter( x, y ) == t_sewage ||
                        g->m.ter( x, y ) == t_grate ) {
                        g->m.make_rubble( p, f_rubble_rock, true );
                    }
                    if( g->m.ter( x, y ) == t_sewage_pump ) {
                        g->m.make_rubble( p, f_rubble_rock, true );
                        g->explosion( p, 50, 0.7, true );
                    }
                }
            }
            options.clear(); // Disable the terminal.
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
                for( int x = 0; x < MAPSIZE_X; x++ ) {
                    for( int y = 0; y < MAPSIZE_Y; y++ ) {
                        if( g->m.ter( x, y ) == t_elevator_control_off ) {
                            g->m.ter_set( x, y, t_elevator_control );

                        }
                    }
                }
            }
            query_any( _( "Press any key..." ) );
            break;

    } // switch (action)
}

void computer::activate_random_failure()
{
    next_attempt = calendar::turn + 450_turns;
    static const computer_failure default_failure( COMPFAIL_SHUTDOWN );
    const computer_failure &fail = random_entry( failures, default_failure );
    activate_failure( fail.type );
}

void computer::activate_failure( computer_failure_type fail )
{
    bool found_tile = false;
    switch( fail ) {

        case COMPFAIL_NULL: // Unknown action.
        case NUM_COMPUTER_FAILURES: // Suppress compiler warning [-Wswitch]
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
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    if( g->m.has_flag( "CONSOLE", x, y ) ) {
                        g->m.ter_set( x, y, t_console_broken );
                        add_msg( m_bad, _( "The console shuts down." ) );
                    }
                }
            }
            break;

        case COMPFAIL_ALARM:
            g->u.add_memorial_log( pgettext( "memorial_male", "Set off an alarm." ),
                                   pgettext( "memorial_female", "Set off an alarm." ) );
            sounds::sound( g->u.pos(), 60, sounds::sound_t::alarm, _( "an alarm sound!" ) );
            if( g->get_levz() > 0 && !g->events.queued( EVENT_WANTED ) ) {
                g->events.add( EVENT_WANTED, calendar::turn + 30_minutes, 0, g->u.global_sm_location() );
            }
            break;

        case COMPFAIL_MANHACKS: {
            int num_robots = rng( 4, 8 );
            for( int i = 0; i < num_robots; i++ ) {
                tripoint mp( 0, 0, g->u.posz() );
                int tries = 0;
                do {
                    mp.x = rng( g->u.posx() - 3, g->u.posx() + 3 );
                    mp.y = rng( g->u.posy() - 3, g->u.posy() + 3 );
                    tries++;
                } while( !g->is_empty( mp ) && tries < 10 );
                if( tries != 10 ) {
                    add_msg( m_warning, _( "Manhacks drop from compartments in the ceiling." ) );
                    g->summon_mon( mon_manhack, mp );
                }
            }
        }
        break;

        case COMPFAIL_SECUBOTS: {
            int num_robots = 1;
            for( int i = 0; i < num_robots; i++ ) {
                tripoint mp( 0, 0, g->u.posz() );
                int tries = 0;
                do {
                    mp.x = rng( g->u.posx() - 3, g->u.posx() + 3 );
                    mp.y = rng( g->u.posy() - 3, g->u.posy() + 3 );
                    tries++;
                } while( !g->is_empty( mp ) && tries < 10 );
                if( tries != 10 ) {
                    add_msg( m_warning, _( "Secubots emerge from compartments in the floor." ) );
                    g->summon_mon( mon_secubot, mp );
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
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    if( g->m.ter( x, y ) == t_sewage_pump ) {
                        tripoint p( x, y, g->get_levz() );
                        g->m.make_rubble( p );
                        g->explosion( p, 10 );
                    }
                }
            }
            break;

        case COMPFAIL_PUMP_LEAK:
            add_msg( m_warning, _( "Sewage leaks!" ) );
            for( int x = 0; x < MAPSIZE_X; x++ ) {
                for( int y = 0; y < MAPSIZE_Y; y++ ) {
                    if( g->m.ter( x, y ) == t_sewage_pump ) {
                        point p( x, y );
                        int leak_size = rng( 4, 10 );
                        for( int i = 0; i < leak_size; i++ ) {
                            std::vector<point> next_move;
                            if( g->m.passable( p.x, p.y - 1 ) ) {
                                next_move.push_back( point( p.x, p.y - 1 ) );
                            }
                            if( g->m.passable( p.x + 1, p.y ) ) {
                                next_move.push_back( point( p.x + 1, p.y ) );
                            }
                            if( g->m.passable( p.x, p.y + 1 ) ) {
                                next_move.push_back( point( p.x, p.y + 1 ) );
                            }
                            if( g->m.passable( p.x - 1, p.y ) ) {
                                next_move.push_back( point( p.x - 1, p.y ) );
                            }

                            if( next_move.empty() ) {
                                i = leak_size;
                            } else {
                                p = random_entry( next_move );
                                g->m.ter_set( p.x, p.y, t_sewage );
                            }
                        }
                    }
                }
            }
            break;

        case COMPFAIL_AMIGARA:
            g->events.add( EVENT_AMIGARA, calendar::turn + 5_turns );
            g->u.add_effect( effect_amigara, 2_minutes );
            g->explosion( tripoint( rng( 0, MAPSIZE_X ), rng( 0, MAPSIZE_Y ), g->get_levz() ), 10,
                          0.7, false, 10 );
            g->explosion( tripoint( rng( 0, MAPSIZE_X ), rng( 0, MAPSIZE_Y ), g->get_levz() ), 10,
                          0.7, false, 10 );
            remove_option( COMPACT_AMIGARA_START );
            break;

        case COMPFAIL_DESTROY_BLOOD:
            print_error( _( "ERROR: Disruptive Spin" ) );
            for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 2 ) ) {
                if( g->m.ter( dest ) == t_centrifuge ) {
                    if( g->m.i_at( dest ).empty() ) {
                        print_error( _( "ERROR: Please place sample in centrifuge." ) );
                    } else if( g->m.i_at( dest ).size() > 1 ) {
                        print_error( _( "ERROR: Please remove all but one sample from centrifuge." ) );
                    } else if( g->m.i_at( dest )[0].typeId() != "vacutainer" ) {
                        print_error( _( "ERROR: Please use blood-contained samples." ) );
                    } else if( g->m.i_at( dest )[0].contents.empty() ) {
                        print_error( _( "ERROR: Blood draw kit, empty." ) );
                    } else if( g->m.i_at( dest )[0].contents.front().typeId() != "blood" ) {
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
            for( int x = 0; x < SEEX * 2; x++ ) {
                for( int y = 0; y < SEEY * 2; y++ ) {
                    if( g->m.ter( x, y ) == t_floor_blue ) {
                        if( g->m.i_at( x, y ).empty() ) {
                            print_error( _( "ERROR: Please place memory bank in scan area." ) );
                        } else if( g->m.i_at( x, y ).size() > 1 ) {
                            print_error( _( "ERROR: Please only scan one item at a time." ) );
                        } else if( g->m.i_at( x, y )[0].typeId() != "usb_drive" ) {
                            print_error( _( "ERROR: Memory bank destroyed or not present." ) );
                        } else if( g->m.i_at( x, y )[0].contents.empty() ) {
                            print_error( _( "ERROR: Memory bank is empty." ) );
                        } else {
                            print_error( _( "ERROR: Data bank destroyed." ) );
                            g->m.i_clear( x, y );
                        }
                    }
                }
            }
            inp_mngr.wait_for_any_key();
            break;

    }// switch (fail)
}

void computer::remove_option( computer_action const action )
{
    for( auto it = options.begin(); it != options.end(); ++it ) {
        if( it->action == action ) {
            options.erase( it );
            break;
        }
    }
}

void computer::mark_refugee_center()
{
    print_line( _( "\
SEARCHING FOR NEAREST REFUGEE CENTER, PLEASE WAIT ... " ) );

    const mission_type_id &mission_type = mission_type_id( "MISSION_REACH_REFUGEE_CENTER" );
    const std::vector<mission *> missions = g->u.get_active_missions();
    tripoint mission_target;

    const bool has_mission = std::any_of( missions.begin(), missions.end(), [ &mission_type,
    &mission_target ]( mission * mission ) {
        if( mission->get_type().id == mission_type ) {
            mission_target = mission->get_target();
            return true;
        }

        return false;
    } );

    if( !has_mission ) {
        const auto mission = mission::reserve_new( mission_type, -1 );
        mission->assign( g->u );
        mission_target = mission->get_target();
    }

    //~555-0164 is a fake phone number in the US, please replace it with a number that will not cause issues in your locale if possible.
    print_line( _( "\
\nREFUGEE CENTER FOUND! LOCATION: %d %s\n\n\
IF YOU HAVE ANY FEEDBACK CONCERNING YOUR VISIT PLEASE CONTACT \n\
THE DEPARTMENT OF EMERGENCY MANAGEMENT PUBLIC AFFAIRS OFFICE. \n\
THE LOCAL OFFICE CAN BE REACHED BETWEEN THE HOURS OF 9AM AND  \n\
4PM AT 555-0164.                                              \n\
\n\
IF YOU WOULD LIKE TO SPEAK WITH SOMEONE IN PERSON OR WOULD LIKE\n\
TO WRITE US A LETTER PLEASE SEND IT TO...\n" ), rl_dist( g->u.pos(), mission_target ),
                direction_name_short( direction_from( g->u.pos(), mission_target ) ) );

    query_any( _( "Press any key to continue..." ) );
}

template<typename ...Args>
bool computer::query_bool( const char *const text, Args &&... args )
{
    const std::string formatted_text = string_format( text, std::forward<Args>( args )... );
    print_line( "%s (Y/N/Q)", formatted_text.c_str() );
    char ret;
#ifdef __ANDROID__
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
bool computer::query_any( const char *const text, Args &&... args )
{
    const std::string formatted_text = string_format( text, std::forward<Args>( args )... );
    print_line( "%s", formatted_text .c_str() );
    inp_mngr.wait_for_any_key();
    return true;
}

template<typename ...Args>
char computer::query_ynq( const char *const text, Args &&... args )
{
    const std::string formatted_text = string_format( text, std::forward<Args>( args )... );
    print_line( "%s (Y/N/Q)", formatted_text.c_str() );
    char ret;
#ifdef __ANDROID__
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
void computer::print_line( const char *const text, Args &&... args )
{
    const std::string formatted_text = string_format( text, std::forward<Args>( args )... );
    wprintz( w_terminal, c_green, formatted_text );
    print_newline();
    wrefresh( w_terminal );
}

template<typename ...Args>
void computer::print_error( const char *const text, Args &&... args )
{
    const std::string formatted_text = string_format( text, std::forward<Args>( args )... );
    wprintz( w_terminal, c_red, formatted_text );
    print_newline();
    wrefresh( w_terminal );
}

template<typename ...Args>
void computer::print_text( const char *const text, Args &&... args )
{
    const std::string formated_text = string_format( text, std::forward<Args>( args )... );
    int y = getcury( w_terminal );
    int w = getmaxx( w_terminal ) - 2;
    fold_and_print( w_terminal, y, 1, w, c_green, formated_text );
    print_newline();
    print_newline();
    wrefresh( w_terminal );
}

void computer::print_gibberish_line()
{
    std::string gibberish;
    int length = rng( 50, 70 );
    for( int i = 0; i < length; i++ ) {
        switch( rng( 0, 4 ) ) {
            case 0:
                gibberish += '0' + rng( 0, 9 );
                break;
            case 1:
            case 2:
                gibberish += 'a' + rng( 0, 25 );
                break;
            case 3:
            case 4:
                gibberish += 'A' + rng( 0, 25 );
                break;
        }
    }
    wprintz( w_terminal, c_yellow, gibberish );
    print_newline();
    wrefresh( w_terminal );
}

void computer::reset_terminal()
{
    werase( w_terminal );
    wmove( w_terminal, 0, 0 );
    wrefresh( w_terminal );
}

void computer::print_newline()
{
    wprintz( w_terminal, c_green, "\n" );
}

computer_option computer_option::from_json( JsonObject &jo )
{
    std::string name = jo.get_string( "name" );
    computer_action action = computer_action_from_string( jo.get_string( "action" ) );
    int sec = jo.get_int( "security", 0 );
    return computer_option( name, action, sec );
}

computer_failure computer_failure::from_json( JsonObject &jo )
{
    computer_failure_type type = computer_failure_type_from_string( jo.get_string( "action" ) );
    return computer_failure( type );
}

computer_action computer_action_from_string( const std::string &str )
{
    static const std::map<std::string, computer_action> actions = {{
            { "null", COMPACT_NULL },
            { "open", COMPACT_OPEN },
            { "open_disarm", COMPACT_OPEN_DISARM },
            { "lock", COMPACT_LOCK },
            { "unlock", COMPACT_UNLOCK },
            { "unlock_disarm", COMPACT_UNLOCK_DISARM },
            { "toll", COMPACT_TOLL },
            { "sample", COMPACT_SAMPLE },
            { "release", COMPACT_RELEASE },
            { "release_bionics", COMPACT_RELEASE_BIONICS },
            { "release_disarm", COMPACT_RELEASE_DISARM },
            { "terminate", COMPACT_TERMINATE },
            { "portal", COMPACT_PORTAL },
            { "cascade", COMPACT_CASCADE },
            { "research", COMPACT_RESEARCH },
            { "maps", COMPACT_MAPS },
            { "map_sewer", COMPACT_MAP_SEWER },
            { "map_subway", COMPACT_MAP_SUBWAY },
            { "miss_launch", COMPACT_MISS_LAUNCH },
            { "miss_disarm", COMPACT_MISS_DISARM },
            { "list_bionics", COMPACT_LIST_BIONICS },
            { "elevator_on", COMPACT_ELEVATOR_ON },
            { "amigara_log", COMPACT_AMIGARA_LOG },
            { "amigara_start", COMPACT_AMIGARA_START },
            { "complete_disable_external_power", COMPACT_COMPLETE_DISABLE_EXTERNAL_POWER },
            { "repeater_mod", COMPACT_REPEATER_MOD },
            { "download_software", COMPACT_DOWNLOAD_SOFTWARE },
            { "blood_anal", COMPACT_BLOOD_ANAL },
            { "data_anal", COMPACT_DATA_ANAL },
            { "disconnect", COMPACT_DISCONNECT },
            { "bonesetting", COMPACT_BONESETTING },
            { "emerg_mess", COMPACT_EMERG_MESS },
            { "emerg_ref_center", COMPACT_EMERG_REF_CENTER },
            { "tower_unresponsive", COMPACT_TOWER_UNRESPONSIVE },
            { "sr1_mess", COMPACT_SR1_MESS },
            { "sr2_mess", COMPACT_SR2_MESS },
            { "sr3_mess", COMPACT_SR3_MESS },
            { "sr4_mess", COMPACT_SR4_MESS },
            { "srcf_1_mess", COMPACT_SRCF_1_MESS },
            { "srcf_2_mess", COMPACT_SRCF_2_MESS },
            { "srcf_3_mess", COMPACT_SRCF_3_MESS },
            { "srcf_seal_order", COMPACT_SRCF_SEAL_ORDER },
            { "srcf_seal", COMPACT_SRCF_SEAL },
            { "srcf_elevator", COMPACT_SRCF_ELEVATOR }
        }
    };

    const auto iter = actions.find( str );
    if( iter != actions.end() ) {
        return iter->second;
    }

    debugmsg( "Invalid computer action %s", str.c_str() );
    return COMPACT_NULL;
}

computer_failure_type computer_failure_type_from_string( const std::string &str )
{
    static const std::map<std::string, computer_failure_type> fails = {{
            { "null", COMPFAIL_NULL },
            { "shutdown", COMPFAIL_SHUTDOWN },
            { "alarm", COMPFAIL_ALARM },
            { "manhacks", COMPFAIL_MANHACKS },
            { "secubots", COMPFAIL_SECUBOTS },
            { "damage", COMPFAIL_DAMAGE },
            { "pump_explode", COMPFAIL_PUMP_EXPLODE },
            { "pump_leak", COMPFAIL_PUMP_LEAK },
            { "amigara", COMPFAIL_AMIGARA },
            { "destroy_blood", COMPFAIL_DESTROY_BLOOD },
            { "destroy_data", COMPFAIL_DESTROY_DATA }
        }
    };

    const auto iter = fails.find( str );
    if( iter != fails.end() ) {
        return iter->second;
    }

    debugmsg( "Invalid computer failure %s", str.c_str() );
    return COMPFAIL_NULL;
}
