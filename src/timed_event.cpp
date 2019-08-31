#include "timed_event.h"

#include <array>
#include <memory>

#include "avatar.h"
#include "avatar_action.h"
#include "debug.h"
#include "event_bus.h"
#include "game.h"
#include "line.h"
#include "map.h"
#include "mapdata.h"
#include "memorial_logger.h"
#include "messages.h"
#include "map_iterator.h"
#include "morale_types.h"
#include "options.h"
#include "rng.h"
#include "sounds.h"
#include "translations.h"
#include "game_constants.h"
#include "int_id.h"
#include "type_id.h"
#include "enums.h"

const mtype_id mon_amigara_horror( "mon_amigara_horror" );
const mtype_id mon_copbot( "mon_copbot" );
const mtype_id mon_dark_wyrm( "mon_dark_wyrm" );
const mtype_id mon_dermatik( "mon_dermatik" );
const mtype_id mon_eyebot( "mon_eyebot" );
const mtype_id mon_riotbot( "mon_riotbot" );
const mtype_id mon_sewer_snake( "mon_sewer_snake" );
const mtype_id mon_spider_widow_giant( "mon_spider_widow_giant" );
const mtype_id mon_spider_cellar_giant( "mon_spider_cellar_giant" );

timed_event::timed_event( timed_event_type e_t, const time_point &w, int f_id, tripoint p )
    : type( e_t )
    , when( w )
    , faction_id( f_id )
    , map_point( p )
{
}

void timed_event::actualize()
{
    switch( type ) {
        case TIMED_EVENT_HELP:
            debugmsg( "Currently disabled while NPC and monster factions are being rewritten." );
            break;

        case TIMED_EVENT_ROBOT_ATTACK: {
            const auto u_pos = g->u.global_sm_location();
            if( rl_dist( u_pos, map_point ) <= 4 ) {
                const mtype_id &robot_type = one_in( 2 ) ? mon_copbot : mon_riotbot;

                g->events().send<event_type::becomes_wanted>( g->u.getID() );
                int robx = u_pos.x > map_point.x ? 0 - SEEX * 2 : SEEX * 4;
                int roby = u_pos.y > map_point.y ? 0 - SEEY * 2 : SEEY * 4;
                g->summon_mon( robot_type, tripoint( robx, roby, g->u.posz() ) );
            }
        }
        break;

        case TIMED_EVENT_SPAWN_WYRMS: {
            if( g->get_levz() >= 0 ) {
                return;
            }
            g->memorial().add(
                pgettext( "memorial_male", "Drew the attention of more dark wyrms!" ),
                pgettext( "memorial_female", "Drew the attention of more dark wyrms!" ) );
            int num_wyrms = rng( 1, 4 );
            for( int i = 0; i < num_wyrms; i++ ) {
                if( monster *const mon = g->place_critter_around( mon_dark_wyrm, g->u.pos(), 2 ) ) {
                    g->m.ter_set( mon->pos(), t_rock_floor );
                }
            }
            // You could drop the flag, you know.
            if( g->u.has_amount( "petrified_eye", 1 ) ) {
                sounds::sound( g->u.pos(), 60, sounds::sound_t::alert, _( "a tortured scream!" ), false, "shout",
                               "scream_tortured" );
                if( !g->u.is_deaf() ) {
                    add_msg( _( "The eye you're carrying lets out a tortured scream!" ) );
                    g->u.add_morale( MORALE_SCREAM, -15, 0, 30_minutes, 30_seconds );
                }
            }
            if( !one_in( 25 ) ) { // They just keep coming!
                g->timed_events.add( TIMED_EVENT_SPAWN_WYRMS,
                                     calendar::turn + rng( 1_minutes, 3_minutes ) );
            }
        }
        break;

        case TIMED_EVENT_AMIGARA: {
            g->events().send<event_type::angers_amigara_horrors>();
            int num_horrors = rng( 3, 5 );
            cata::optional<tripoint> fault_point;
            bool horizontal = false;
            for( const tripoint &p : g->m.points_on_zlevel() ) {
                if( g->m.ter( p ) == t_fault ) {
                    fault_point = p;
                    horizontal = g->m.ter( p + tripoint_east ) == t_fault || g->m.ter( p + tripoint_west ) == t_fault;
                    break;
                }
            }
            for( int i = 0; fault_point && i < num_horrors; i++ ) {
                for( int tries = 0; tries < 10; ++tries ) {
                    tripoint monp = g->u.pos();
                    if( horizontal ) {
                        monp.x = rng( fault_point->x, fault_point->x + 2 * SEEX - 8 );
                        for( int n = -1; n <= 1; n++ ) {
                            if( g->m.ter( point( monp.x, fault_point->y + n ) ) == t_rock_floor ) {
                                monp.y = fault_point->y + n;
                            }
                        }
                    } else { // Vertical fault
                        monp.y = rng( fault_point->y, fault_point->y + 2 * SEEY - 8 );
                        for( int n = -1; n <= 1; n++ ) {
                            if( g->m.ter( point( fault_point->x + n, monp.y ) ) == t_rock_floor ) {
                                monp.x = fault_point->x + n;
                            }
                        }
                    }
                    if( g->place_critter_at( mon_amigara_horror, monp ) ) {
                        break;
                    }
                }
            }
        }
        break;

        case TIMED_EVENT_ROOTS_DIE:
            g->events().send<event_type::destroys_triffid_grove>();
            for( const tripoint &p : g->m.points_on_zlevel() ) {
                if( g->m.ter( p ) == t_root_wall && one_in( 3 ) ) {
                    g->m.ter_set( p, t_underbrush );
                }
            }
            break;

        case TIMED_EVENT_TEMPLE_OPEN: {
            g->events().send<event_type::opens_temple>();
            bool saw_grate = false;
            for( const tripoint &p : g->m.points_on_zlevel() ) {
                if( g->m.ter( p ) == t_grate ) {
                    g->m.ter_set( p, t_stairs_down );
                    if( !saw_grate && g->u.sees( p ) ) {
                        saw_grate = true;
                    }
                }
            }
            if( saw_grate ) {
                add_msg( _( "The nearby grates open to reveal a staircase!" ) );
            }
        }
        break;

        case TIMED_EVENT_TEMPLE_FLOOD: {
            bool flooded = false;

            ter_id flood_buf[MAPSIZE_X][MAPSIZE_Y];
            for( const tripoint &p : g->m.points_on_zlevel() ) {
                flood_buf[p.x][p.y] = g->m.ter( p );
            }
            for( const tripoint &p : g->m.points_on_zlevel() ) {
                if( g->m.ter( p ) == t_water_sh ) {
                    bool deepen = false;
                    for( const tripoint &w : points_in_radius( p, 1 ) ) {
                        if( g->m.ter( w ) == t_water_dp ) {
                            deepen = true;
                            break;
                        }
                    }
                    if( deepen ) {
                        flood_buf[p.x][p.y] = t_water_dp;
                        flooded = true;
                    }
                } else if( g->m.ter( p ) == t_rock_floor ) {
                    bool flood = false;
                    for( const tripoint &w : points_in_radius( p, 1 ) ) {
                        if( g->m.ter( w ) == t_water_dp || g->m.ter( w ) == t_water_sh ) {
                            flood = true;
                            break;
                        }
                    }
                    if( flood ) {
                        flood_buf[p.x][p.y] = t_water_sh;
                        flooded = true;
                    }
                }
            }
            if( !flooded ) {
                return;    // We finished flooding the entire chamber!
            }
            // Check if we should print a message
            if( flood_buf[g->u.posx()][g->u.posy()] != g->m.ter( g->u.pos() ) ) {
                if( flood_buf[g->u.posx()][g->u.posy()] == t_water_sh ) {
                    add_msg( m_warning, _( "Water quickly floods up to your knees." ) );
                    g->memorial().add(
                        pgettext( "memorial_male", "Water level reached knees." ),
                        pgettext( "memorial_female", "Water level reached knees." ) );
                } else { // Must be deep water!
                    add_msg( m_warning, _( "Water fills nearly to the ceiling!" ) );
                    g->memorial().add(
                        pgettext( "memorial_male", "Water level reached the ceiling." ),
                        pgettext( "memorial_female", "Water level reached the ceiling." ) );
                    avatar_action::swim( g->m, g->u, g->u.pos() );
                }
            }
            // flood_buf is filled with correct tiles; now copy them back to g->m
            for( const tripoint &p : g->m.points_on_zlevel() ) {
                g->m.ter_set( p, flood_buf[p.x][p.y] );
            }
            g->timed_events.add( TIMED_EVENT_TEMPLE_FLOOD,
                                 calendar::turn + rng( 2_turns, 3_turns ) );
        }
        break;

        case TIMED_EVENT_TEMPLE_SPAWN: {
            static const std::array<mtype_id, 4> temple_monsters = { {
                    mon_sewer_snake, mon_dermatik, mon_spider_widow_giant, mon_spider_cellar_giant
                }
            };
            const mtype_id &montype = random_entry( temple_monsters );
            g->place_critter_around( montype, g->u.pos(), 2 );
        }
        break;

        default:
            break; // Nothing happens for other events
    }
}

void timed_event::per_turn()
{
    switch( type ) {
        case TIMED_EVENT_WANTED: {
            // About once every 5 minutes. Suppress in classic zombie mode.
            if( g->get_levz() >= 0 && one_in( 50 ) && !get_option<bool>( "DISABLE_ROBOT_RESPONSE" ) ) {
                point place = g->m.random_outdoor_tile();
                if( place.x == -1 && place.y == -1 ) {
                    return; // We're safely indoors!
                }
                g->summon_mon( mon_eyebot, tripoint( place, g->u.posz() ) );
                if( g->u.sees( tripoint( place, g->u.posz() ) ) ) {
                    add_msg( m_warning, _( "An eyebot swoops down nearby!" ) );
                }
                // One eyebot per trigger is enough, really
                when = calendar::turn;
            }
        }
        break;

        case TIMED_EVENT_SPAWN_WYRMS:
            if( g->get_levz() >= 0 ) {
                when -= 1_turns;
                return;
            }
            if( calendar::once_every( 3_turns ) && !g->u.is_deaf() ) {
                add_msg( m_warning, _( "You hear screeches from the rock above and around you!" ) );
            }
            break;

        case TIMED_EVENT_AMIGARA:
            add_msg( m_warning, _( "The entire cavern shakes!" ) );
            break;

        case TIMED_EVENT_TEMPLE_OPEN:
            add_msg( m_warning, _( "The earth rumbles." ) );
            break;

        default:
            break; // Nothing happens for other events
    }
}

void timed_event_manager::process()
{
    for( auto it = events.begin(); it != events.end(); ) {
        it->per_turn();
        if( it->when <= calendar::turn ) {
            it->actualize();
            it = events.erase( it );
        } else {
            it++;
        }
    }
}

void timed_event_manager::add( const timed_event_type type, const time_point &when,
                               const int faction_id )
{
    add( type, when, faction_id, g->u.global_sm_location() );
}

void timed_event_manager::add( const timed_event_type type, const time_point &when,
                               const int faction_id,
                               const tripoint &where )
{
    events.emplace_back( type, when, faction_id, where );
}

bool timed_event_manager::queued( const timed_event_type type ) const
{
    return const_cast<timed_event_manager &>( *this ).get( type ) != nullptr;
}

timed_event *timed_event_manager::get( const timed_event_type type )
{
    for( auto &e : events ) {
        if( e.type == type ) {
            return &e;
        }
    }
    return nullptr;
}
