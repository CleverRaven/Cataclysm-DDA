#include "start_location.h"

#include "coordinate_conversions.h"
#include "debug.h"
#include "enums.h"
#include "field.h"
#include "game.h"
#include "generic_factory.h"
#include "json.h"
#include "map.h"
#include "mapgen.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "player.h"

#include <algorithm>

const efftype_id effect_bleed( "bleed" );


namespace
{
generic_factory<start_location> all_starting_locations( "starting location", "ident" );
}

/** @relates string_id */
template<>
const start_location &string_id<start_location>::obj() const
{
    return all_starting_locations.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<start_location>::is_valid() const
{
    return all_starting_locations.is_valid( *this );
}

start_location::start_location()
    : _name( "null" ), _target( "shelter" )
{
}

const string_id<start_location> &start_location::ident() const
{
    return id;
}

std::string start_location::name() const
{
    return _name;
}

std::string start_location::target() const
{
    return _target;
}

const std::vector<start_location> &start_location::get_all()
{
    return all_starting_locations.get_all();
}

const std::set<std::string> &start_location::flags() const
{
    return _flags;
}

void start_location::load_location( JsonObject &jo, const std::string &src )
{
    all_starting_locations.load( jo, src );
}

void start_location::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "name", _name, translated_string_reader );
    mandatory( jo, was_loaded, "target", _target );
    optional( jo, was_loaded, "flags", _flags, auto_flags_reader<> {} );
}

void start_location::reset()
{
    all_starting_locations.reset();
}

// check if tile at p should be boarded with some kind of furniture.
void add_boardable( map &m, const tripoint &p, std::vector<tripoint> &vec )
{
    if( m.has_furn( p ) ) {
        // Don't need to board this up, is already occupied
        return;
    }
    if( m.ter( p ) != t_floor ) {
        // Other terrain (door, wall, ...), not boarded either
        return;
    }
    if( m.is_outside( p ) ) {
        // Don't board up the outside
        return;
    }
    if( std::find( vec.begin(), vec.end(), p ) != vec.end() ) {
        // Already registered to be boarded
        return;
    }
    vec.push_back( p );
}

void board_up( map &m, const tripoint &start, const tripoint &end )
{
    std::vector<tripoint> furnitures1;
    std::vector<tripoint> furnitures2;
    std::vector<tripoint> boardables;
    tripoint p;
    p.z = m.get_abs_sub().z;
    int &x = p.x;
    int &y = p.y;
    int &z = p.z;
    for( x = start.x; x < end.x; x++ ) {
        for( y = start.y; y < end.y; y++ ) {
            bool must_board_around = false;
            const ter_id t = m.ter( x, y );
            if( t == t_window_domestic || t == t_window || t == t_window_no_curtains ) {
                // Windows are always to the outside and must be boarded
                must_board_around = true;
                m.ter_set( p, t_window_boarded );
            } else if( t == t_door_c || t == t_door_locked || t == t_door_c_peep ) {
                // Only board up doors that lead to the outside
                if( m.is_outside( tripoint( x + 1, y, z ) ) ||
                    m.is_outside( tripoint( x - 1, y, z ) ) ||
                    m.is_outside( tripoint( x, y + 1, z ) ) ||
                    m.is_outside( tripoint( x, y - 1, z ) ) ) {
                    m.ter_set( p, t_door_boarded );
                    must_board_around = true;
                } else {
                    // internal doors are opened instead
                    m.ter_set( p, t_door_o );
                }
            }
            if( must_board_around ) {
                // Board up the surroundings of the door/window
                add_boardable( m, tripoint( x + 1, y, z ), boardables );
                add_boardable( m, tripoint( x - 1, y, z ), boardables );
                add_boardable( m, tripoint( x, y + 1, z ), boardables );
                add_boardable( m, tripoint( x, y - 1, z ), boardables );
                add_boardable( m, tripoint( x + 1, y + 1, z ), boardables );
                add_boardable( m, tripoint( x - 1, y + 1, z ), boardables );
                add_boardable( m, tripoint( x + 1, y - 1, z ), boardables );
                add_boardable( m, tripoint( x - 1, y - 1, z ), boardables );
            }
        }
    }
    // Find all furniture that can be used to board up some place
    for( x = start.x; x < end.x; x++ ) {
        for( y = start.y; y < end.y; y++ ) {
            if( std::find( boardables.begin(), boardables.end(), p ) != boardables.end() ) {
                continue;
            }
            if( !m.has_furn( p ) ) {
                continue;
            }
            // If the furniture is movable and the character can move it, use it to barricade
            // g->u is workable here as NPCs by definition are not starting the game.  (Let's hope.)
            ///\EFFECT_STR determines what furniture might be used as a starting area barricade
            if( m.furn( p ).obj().move_str_req > 0 && m.furn( p ).obj().move_str_req < g->u.get_str() ) {
                if( m.furn( p ).obj().movecost == 0 ) {
                    // Obstacles are better, prefer them
                    furnitures1.push_back( p );
                } else {
                    furnitures2.push_back( p );
                }
            }
        }
    }
    while( ( !furnitures1.empty() || !furnitures2.empty() ) && !boardables.empty() ) {
        const tripoint fp = random_entry_removed( furnitures1.empty() ? furnitures2 : furnitures1 );
        const tripoint bp = random_entry_removed( boardables );
        m.furn_set( bp, m.furn( fp ) );
        m.furn_set( fp, f_null );
        auto destination_items = m.i_at( bp );
        for( auto moved_item : m.i_at( fp ) ) {
            destination_items.push_back( moved_item );
        }
        m.i_clear( fp );
    }
}

void start_location::prepare_map( tinymap &m ) const
{
    const int z = m.get_abs_sub().z;
    if( flags().count( "BOARDED" ) > 0 ) {
        m.build_outside_cache( z );
        board_up( m, tripoint( 0, 0, z ), tripoint( m.getmapsize() * SEEX, m.getmapsize() * SEEY, z ) );
    } else {
        m.translate( t_window_domestic, t_curtains );
    }
}

tripoint start_location::find_player_initial_location() const
{
    const bool using_existing_initial_overmap = overmap_buffer.has( 0, 0 );
    // The coordinates of an overmap that is known to *not* exist. We can regenerate this
    // as often we like.
    point non_existing_omt = point( 0, 0 );

    if( using_existing_initial_overmap ) {
        // arbitrary, should be large enough to include all overmaps ever created
        const int radius = 32;
        for( const point omp : closest_points_first( radius, point( 0, 0 ) ) ) {
            const overmap *omap = overmap_buffer.get_existing( omp.x, omp.y );
            if( omap == nullptr ) {
                if( non_existing_omt == point( 0, 0 ) ) {
                    non_existing_omt = omp;
                }
                continue;
            }
            const tripoint omtstart = omap->find_random_omt( target() );
            if( omtstart != overmap::invalid_tripoint ) {
                return omtstart + point( omp.x * OMAPX, omp.y * OMAPY );
            }
        }
    }

    while( true ) {
        popup_nowait( _( "Please wait as we build your world" ) );
        const overmap &initial_overmap = overmap_buffer.get( non_existing_omt.x, non_existing_omt.y );
        const tripoint omtstart = initial_overmap.find_random_omt( target() );
        if( omtstart != overmap::invalid_tripoint ) {
            return omtstart + point( non_existing_omt.x * OMAPX, non_existing_omt.y * OMAPY );
        }
        if( !query_yn(
                _( "The game could not create a world with a suitable starting location.\n\n"
                   "Depending on the world options, the starting location may never appear. If the problem persists, you can try another starting location, or change the world options.\n\n"
                   "Try again?" ) ) ) {
            return overmap::invalid_tripoint;
        }
        overmap_buffer.clear();
    }
}

void start_location::prepare_map( const tripoint &omtstart ) const
{
    // Now prepare the initial map (change terrain etc.)
    const point player_location = omt_to_sm_copy( omtstart.x, omtstart.y );
    tinymap player_start;
    player_start.load( player_location.x, player_location.y, omtstart.z, false );
    prepare_map( player_start );
    player_start.save();
}

/** Helper for place_player
 * Flood-fills the area from a given point, then returns the area it found.
 * Considers unpassable tiles passable if they can be bashed down or opened.
 * Will return INT_MAX if it reaches upstairs or map edge.
 * We can't really use map::route here, because it expects a destination
 * Maybe TODO: Allow "picking up" items or parts of bashable furniture
 *             and using them to help with bash attempts.
 */
int rate_location( map &m, const tripoint &p, const bool must_be_inside,
                   const int bash_str, const int attempt,
                   int ( &checked )[MAPSIZE * SEEX][MAPSIZE * SEEY] )
{
    if( ( must_be_inside && m.is_outside( p ) ) ||
        m.impassable( p ) ||
        checked[p.x][p.y] > 0 ) {
        return 0;
    }

    // Vector that will be used as a stack
    std::vector<tripoint> st;
    st.reserve( MAPSIZE * SEEX * MAPSIZE * SEEY );
    st.push_back( p );

    // If not checked yet and either can be moved into, can be bashed down or opened,
    // add it on the top of the stack.
    const auto maybe_add = [&]( const int x, const int y, const tripoint & from ) {
        if( checked[x][y] >= attempt ) {
            return;
        }

        const tripoint pt( x, y, p.z );
        if( m.passable( pt ) ||
            m.bash_resistance( pt ) <= bash_str ||
            m.open_door( pt, !m.is_outside( from ), true ) ) {
            st.push_back( pt );
        }
    };

    int area = 0;
    while( !st.empty() ) {
        area++;
        const tripoint cur = st.back();
        st.pop_back();

        checked[cur.x][cur.y] = attempt;
        if( cur.x == 0 || cur.x == SEEX * MAPSIZE - 1 ||
            cur.y == 0 || cur.y == SEEY * MAPSIZE - 1 ||
            m.has_flag( "GOES_UP", cur ) ) {
            return INT_MAX;
        }

        maybe_add( cur.x - 1, cur.y, cur );
        maybe_add( cur.x, cur.y - 1, cur );
        maybe_add( cur.x + 1, cur.y, cur );
        maybe_add( cur.x, cur.y + 1, cur );
        maybe_add( cur.x - 1, cur.y - 1, cur );
        maybe_add( cur.x + 1, cur.y - 1, cur );
        maybe_add( cur.x - 1, cur.y + 1, cur );
        maybe_add( cur.x + 1, cur.y + 1, cur );
    }

    return area;
}

void start_location::place_player( player &u ) const
{
    // Need the "real" map with it's inside/outside cache and the like.
    map &m = g->m;
    // Start us off somewhere in the center of the map
    u.setx( SEEX * int( MAPSIZE / 2 ) + 5 );
    u.sety( SEEY * int( MAPSIZE / 2 ) + 6 );
    u.setz( g->get_levz() );

    m.build_map_cache( m.get_abs_sub().z );
    const bool must_be_inside = flags().count( "ALLOW_OUTSIDE" ) == 0;
    ///\EFFECT_STR allows player to start behind less-bashable furniture and terrain
    const int bash = u.get_str(); // TODO: Allow using items here

    // Remember biggest found location
    // Sometimes it may be impossible to automatically found an ideal location
    // but the player may be more creative than this algorithm and do away with just "good"
    int best_rate = 0;
    // In which attempt did this area get checked?
    // We can overwrite earlier attempts, but not start in them
    int checked[SEEX * MAPSIZE][SEEY * MAPSIZE];
    std::fill_n( &checked[0][0], SEEX * MAPSIZE * SEEY * MAPSIZE, 0 );

    bool found_good_spot = false;
    // Try some random points at start

    int tries = 0;
    const auto check_spot = [&]( const tripoint & pt ) {
        tries++;
        const int rate = rate_location( m, pt, must_be_inside, bash, tries, checked );
        if( best_rate < rate ) {
            best_rate = rate;
            u.setpos( pt );
            if( rate == INT_MAX ) {
                found_good_spot = true;
            }
        }
    };

    while( !found_good_spot && tries < 100 ) {
        tripoint rand_point( ( SEEX * int( MAPSIZE / 2 ) ) + rng( 0, SEEX * 2 ),
                             ( SEEY * int( MAPSIZE / 2 ) ) + rng( 0, SEEY * 2 ),
                             u.posz() );
        check_spot( rand_point );
    }
    // If we haven't got a good location by now, screw it and brute force it
    // This only happens in exotic locations (deep of a science lab), but it does happen
    if( !found_good_spot ) {
        tripoint tmp = u.pos();
        int &x = tmp.x;
        int &y = tmp.y;
        for( x = 0; x < SEEX * MAPSIZE; x++ ) {
            for( y = 0; y < SEEY * MAPSIZE; y++ ) {
                check_spot( tmp );
            }
        }
    }

    if( !found_good_spot ) {
        debugmsg( "Could not find a good starting place for character" );
    }
}

void start_location::burn( const tripoint &omtstart,
                           const size_t count, const int rad ) const
{
    const tripoint player_location = omt_to_sm_copy( omtstart );
    tinymap m;
    m.load( player_location.x, player_location.y, player_location.z, false );
    m.build_outside_cache( m.get_abs_sub().z );
    const int ux = g->u.posx() % ( SEEX * int( MAPSIZE / 2 ) );
    const int uy = g->u.posy() % ( SEEY * int( MAPSIZE / 2 ) );
    std::vector<tripoint> valid;
    tripoint p = player_location;
    int &x = p.x;
    int &y = p.y;
    for( x = 0; x < m.getmapsize() * SEEX; x++ ) {
        for( y = 0; y < m.getmapsize() * SEEY; y++ ) {
            if( !( m.has_flag_ter( "DOOR", p ) ||
                   m.has_flag_ter( "OPENCLOSE_INSIDE", p ) ||
                   m.is_outside( p ) ||
                   ( x >= ux - rad && x <= ux + rad && y >= uy - rad && y <= uy + rad ) ) ) {
                if( m.has_flag( "FLAMMABLE", p ) || m.has_flag( "FLAMMABLE_ASH", p ) ) {
                    valid.push_back( p );
                }
            }
        }
    }
    random_shuffle( valid.begin(), valid.end() );
    for( size_t i = 0; i < std::min( count, valid.size() ); i++ ) {
        m.add_field( valid[i], fd_fire, 3, 0 );
    }
    m.save();
}

void start_location::add_map_special( const tripoint &omtstart,
                                      const std::string &map_special ) const
{
    const tripoint player_location = omt_to_sm_copy( omtstart );
    tinymap m;
    m.load( player_location.x, player_location.y, player_location.z, false );

    const auto ptr = MapExtras::get_function( map_special );
    ptr( m, player_location );

    m.save();
}

void start_location::handle_heli_crash( player &u ) const
{
    for( int i = 2; i < num_hp_parts; i++ ) { // Skip head + torso for balance reasons.
        auto part = hp_part( i );
        auto bp_part = u.hp_to_bp( part );
        int roll = int( rng( 1, 8 ) );
        switch( roll ) {
            case 1:
            case 2:// Damage + Bleed
                u.add_effect( effect_bleed, 60, bp_part );
            case 3:
            case 4:
            case 5: { // Just damage
                auto maxHp = u.get_hp_max( part );
                // Body part health will range from 33% to 66% with occasional bleed
                int dmg = int( rng( maxHp / 3, maxHp * 2 / 3 ) );
                u.apply_damage( nullptr, bp_part, dmg );
                break;
            }
            default: // No damage
                break;
        }
    }
}
