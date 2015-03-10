#include <algorithm>
#include "start_location.h"
#include "output.h"
#include "debug.h"
#include "map.h"
#include "game.h"
#include "overmapbuffer.h"

static location_map _locations;

start_location::start_location()
{
    _ident = "";
    _name = "null";
    _target = "shelter";
}

start_location::start_location( std::string ident, std::string name,
                                std::string target )
{
    _ident = ident;
    _name = name;
    _target = target;
}

std::string start_location::ident() const
{
    return _ident;
}

std::string start_location::name() const
{
    return _name;
}

std::string start_location::target() const
{
    return _target;
}

location_map::iterator start_location::begin()
{
    return _locations.begin();
}

location_map::iterator start_location::end()
{
    return _locations.end();
}

start_location *start_location::find( const std::string ident )
{
    location_map::iterator found = _locations.find( ident );
    if(found != _locations.end()) {
        return &(found->second);
    } else {
        debugmsg("Tried to get invalid location: %s", ident.c_str());
        static start_location null_location;
        return &null_location;
    }
}

const std::set<std::string> &start_location::flags() const {
    return _flags;
}

void start_location::load_location( JsonObject &jsonobj )
{
    start_location new_location;

    new_location._ident = jsonobj.get_string("ident");
    new_location._name = jsonobj.get_string("name");
    new_location._target = jsonobj.get_string("target");
    new_location._flags = jsonobj.get_tags("flags");

    _locations[new_location._ident] = new_location;
}

// check if tile at p should be boarded with some kind of furniture.
void add_boardable( map &m, const point &p, std::vector<point> &vec )
{
    if( m.has_furn( p.x, p.y ) ) {
        // Don't need to board this up, is already occupied
        return;
    }
    if( m.ter( p.x, p.y ) != t_floor ) {
        // Other terrain (door, wall, ...), not boarded either
        return;
    }
    if( m.is_outside( p.x, p.y ) ) {
        // Don't board up the outside
        return;
    }
    if( std::find( vec.begin(), vec.end(), p ) != vec.end() ) {
        // Already registered to be boarded
        return;
    }
    vec.push_back( p );
}

point get_random_from_vec( std::vector<point> &vec )
{
    if( vec.empty() ) {
        return point( -1, -1 );
    }
    const size_t i = rng( 0, vec.size() - 1 );
    const point p = vec[i];
    vec.erase( vec.begin() + i );
    return p;
}

void board_up( map &m, int sx, int sy, int dx, int dy )
{
    std::vector<point> furnitures1;
    std::vector<point> furnitures2;
    std::vector<point> boardables;
    for( int x = sx; x < sx + dx; x++ ) {
        for( int y = sy; y < sy + dy; y++ ) {
            bool must_board_around = false;
            const ter_id t = m.ter( x, y );
            if( t == t_window_domestic || t == t_window ) {
                // Windows are always to the outside and must be boarded
                must_board_around = true;
                m.ter_set( x, y, t_window_boarded );
            } else if( t == t_door_c || t == t_door_locked || t == t_door_c_peep ) {
                // Only board up doors that lead to the outside
                if( m.is_outside( x + 1, y ) || m.is_outside( x - 1, y ) ||
                    m.is_outside( x, y + 1 ) || m.is_outside( x, y - 1 ) ) {
                    m.ter_set( x, y, t_door_boarded );
                    must_board_around = true;
                } else {
                    // internal doors are opened instead
                    m.ter_set( x, y, t_door_o );
                }
            }
            if( must_board_around ) {
                // Board up the surroundings of the door/window
                add_boardable( m, point( x + 1, y ), boardables );
                add_boardable( m, point( x - 1, y ), boardables );
                add_boardable( m, point( x, y + 1 ), boardables );
                add_boardable( m, point( x, y - 1 ), boardables );
                add_boardable( m, point( x + 1, y + 1 ), boardables );
                add_boardable( m, point( x - 1, y + 1 ), boardables );
                add_boardable( m, point( x + 1, y - 1 ), boardables );
                add_boardable( m, point( x - 1, y - 1 ), boardables );
            }
        }
    }
    // Find all furniture that can be used to board up some place
    for( int x = sx; x < sx + dx; x++ ) {
        for( int y = sy; y < sy + dy; y++ ) {
            if( std::find( boardables.begin(), boardables.end(), point( x, y ) ) != boardables.end() ) {
                continue;
            }
            if( !m.has_furn( x, y ) ) {
                continue;
            }
            // If the furniture is movable and the character can move it, use it to barricade
            // g->u is workable here as NPCs by definition are not starting the game.  (Let's hope.)
            if( (m.furn_at( x, y ).move_str_req > 0) && (m.furn_at( x, y ).move_str_req < g->u.get_str() )) {
                if( m.furn_at( x, y ).movecost == 0 ) {
                    // Obstacles are better, prefer them
                    furnitures1.push_back( point( x, y ) );
                } else {
                    furnitures2.push_back( point( x, y ) );
                }
            }
        }
    }
    while( ( !furnitures1.empty() || !furnitures2.empty() ) && !boardables.empty() ) {
        const point fp = furnitures1.empty() ?
            get_random_from_vec( furnitures2 ) : get_random_from_vec( furnitures1 );
        const point bp = get_random_from_vec( boardables );
        m.furn_set( bp.x, bp.y, m.furn( fp.x, fp.y ) );
        m.furn_set( fp.x, fp.y, f_null );
        auto destination_items = m.i_at(bp.x, bp.y);
        for( auto moved_item : m.i_at(fp.x, fp.y) ) {
            destination_items.push_back( moved_item );
        }
        m.i_clear( fp.x, fp.y );
    }
}

void start_location::prepare_map( tinymap &m ) const
{
    if( flags().count( "BOARDED" ) > 0 ) {
        m.build_outside_cache();
        board_up( m, 0, 0, m.getmapsize() * SEEX, m.getmapsize() * SEEY );
    } else {
        m.translate( t_window_domestic, t_curtains );
    }
}

tripoint start_location::setup( overmap *&cur_om, int &levx, int &levy, int &levz ) const
{
    // We start in the (0,0,0) overmap.
    cur_om = &overmap_buffer.get( 0, 0 );
    tripoint omtstart = cur_om->find_random_omt( target() );
    if( omtstart == overmap::invalid_tripoint ) {
        // TODO (maybe): either regenerate the overmap (conflicts with existing characters there,
        // that has to be checked. Or look at the neighboring overmaps, but one has to stop
        // looking for it sometimes.
        debugmsg( "Could not find starting overmap terrain %s", target().c_str() );
        omtstart = tripoint( 0, 0, 0 );
    }

    // Now prepare the initial map (change terrain etc.)
    const point player_location = overmapbuffer::omt_to_sm_copy( omtstart.x, omtstart.y );
    tinymap player_start;
    player_start.load( player_location.x, player_location.y, omtstart.z, false, cur_om );
    prepare_map( player_start );
    player_start.save();

    // Setup game::levx/levy/levz - those are in submap coordinates!
    // And the player is centered in the map
    levx = player_location.x - ( MAPSIZE / 2 );
    levy = player_location.y - ( MAPSIZE / 2 );
    levz = omtstart.z;
    return omtstart;
}

void start_location::place_player( player &u ) const
{
    // Need the "real" map with it's inside/outside cache and the like.
    map &m = g->m;
    // Start us off somewhere in the shelter.
    u.setx( SEEX * int( MAPSIZE / 2 ) + 5 );
    u.sety( SEEY * int( MAPSIZE / 2 ) + 6 );

    m.build_map_cache();
    int tries = 0;
    const bool must_be_inside = flags().count( "ALLOW_OUTSIDE" ) == 0;
    while( ( ( must_be_inside && m.is_outside( u.posx(), u.posy() ) ) ||
             m.move_cost( u.posx(), u.posy() ) == 0 ) && tries < 1000 ) {
        tries++;
        u.setx( ( SEEX * int( MAPSIZE / 2 ) ) + rng( 0, SEEX * 2 ) );
        u.sety( ( SEEY * int( MAPSIZE / 2 ) ) + rng( 0, SEEY * 2 ) );
    }
    if( tries >= 1000 ) {
        debugmsg( "Could not find starting place for character" );
    }
}

void start_location::burn( overmap *&cur_om, tripoint &omtstart,
                           const size_t count, const int rad ) const {
    cur_om = &overmap_buffer.get( 0, 0 );
    if( omtstart == overmap::invalid_tripoint ) {
        omtstart = tripoint( 0, 0, 0 );
    }
    const point player_location = overmapbuffer::omt_to_sm_copy( omtstart.x, omtstart.y );
    tinymap m;
    m.load( player_location.x, player_location.y, omtstart.z, false, cur_om );
    m.build_outside_cache();
    const int ux = g->u.posx() % (SEEX * int( MAPSIZE / 2 ));
    const int uy = g->u.posy() % (SEEY * int( MAPSIZE / 2 ));
    std::vector<point> valid;
    for( int x = 0; x < m.getmapsize() * SEEX; x++ ) {
        for ( int y = 0; y < m.getmapsize() * SEEY; y++ ) {
            if ( !(m.has_flag_ter("DOOR", x, y) ||
                   m.has_flag_ter("OPENCLOSE_INSIDE", x, y) ||
                   m.is_outside(x, y) ||
                   (x >= ux - rad && x <= ux + rad && y >= uy - rad && y <= uy + rad )) ) {
                if ( m.has_flag("FLAMMABLE", x, y) || m.has_flag("FLAMMABLE_ASH", x, y) ) {
                    valid.push_back( point(x, y) );
                }
            }
        }
    }
    random_shuffle( valid.begin(), valid.end() );
    for ( size_t i = 0; i < std::min( count, valid.size() ); i++ ) {
        m.add_field( valid[i].x, valid[i].y, fd_fire, 3 );
    }
    m.save();
}
