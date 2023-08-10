#include "mapgen_functions.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <initializer_list>
#include <iterator>
#include <map>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "calendar.h"
#include "character_id.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "enums.h"
#include "field_type.h"
#include "flood_fill.h"
#include "game_constants.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "mapgen.h"
#include "mapgendata.h"
#include "mapgenformat.h"
#include "omdata.h"
#include "overmap.h"
#include "point.h"
#include "regional_settings.h"
#include "rng.h"
#include "submap.h"
#include "trap.h"
#include "vehicle_group.h"
#include "weighted_list.h"
#include "creature_tracker.h"

static const item_group_id Item_spawn_data_field( "field" );
static const item_group_id Item_spawn_data_forest_trail( "forest_trail" );

static const oter_str_id oter_forest_thick( "forest_thick" );
static const oter_str_id oter_forest_trail_end_east( "forest_trail_end_east" );
static const oter_str_id oter_forest_trail_end_west( "forest_trail_end_west" );
static const oter_str_id oter_forest_trail_es( "forest_trail_es" );
static const oter_str_id oter_forest_trail_esw( "forest_trail_esw" );
static const oter_str_id oter_forest_trail_ew( "forest_trail_ew" );
static const oter_str_id oter_forest_trail_new( "forest_trail_new" );
static const oter_str_id oter_forest_trail_nsw( "forest_trail_nsw" );
static const oter_str_id oter_forest_trail_sw( "forest_trail_sw" );
static const oter_str_id oter_forest_trail_wn( "forest_trail_wn" );
static const oter_str_id oter_hellmouth( "hellmouth" );
static const oter_str_id oter_rift( "rift" );
static const oter_str_id oter_river_c_not_nw( "river_c_not_nw" );
static const oter_str_id oter_river_c_not_se( "river_c_not_se" );
static const oter_str_id oter_river_c_not_sw( "river_c_not_sw" );
static const oter_str_id oter_river_center( "river_center" );
static const oter_str_id oter_river_east( "river_east" );
static const oter_str_id oter_river_nw( "river_nw" );
static const oter_str_id oter_river_se( "river_se" );
static const oter_str_id oter_river_south( "river_south" );
static const oter_str_id oter_river_sw( "river_sw" );
static const oter_str_id oter_river_west( "river_west" );
static const oter_str_id oter_slimepit( "slimepit" );
static const oter_str_id oter_slimepit_down( "slimepit_down" );

static const oter_type_str_id oter_type_railroad( "railroad" );

static const vspawn_id VehicleSpawn_default_subway_deadend( "default_subway_deadend" );

class npc_template;

tripoint rotate_point( const tripoint &p, int rotations )
{
    if( p.x < 0 || p.x >= SEEX * 2 ||
        p.y < 0 || p.y >= SEEY * 2 ) {
        debugmsg( "Point out of range: %d,%d,%d", p.x, p.y, p.z );
        // Mapgen is vulnerable, don't supply invalid points, debugmsg is enough
        return tripoint( 0, 0, p.z );
    }

    rotations = rotations % 4;

    tripoint ret = p;
    switch( rotations ) {
        case 0:
            break;
        case 1:
            ret.x = p.y;
            ret.y = SEEX * 2 - 1 - p.x;
            break;
        case 2:
            ret.x = SEEX * 2 - 1 - p.x;
            ret.y = SEEY * 2 - 1 - p.y;
            break;
        case 3:
            ret.x = SEEY * 2 - 1 - p.y;
            ret.y = p.x;
            break;
    }

    return ret;
}

building_gen_pointer get_mapgen_cfunction( const std::string &ident )
{
    static const std::map<std::string, building_gen_pointer> pointers = { {
            { "null",             &mapgen_null },
            { "field",            &mapgen_field },
            { "forest",           &mapgen_forest },
            { "forest_trail_straight",    &mapgen_forest_trail_straight },
            { "forest_trail_curved",      &mapgen_forest_trail_curved },
            // TODO: Add a dedicated dead-end function. For now it copies the straight section above.
            { "forest_trail_end",         &mapgen_forest_trail_straight },
            { "forest_trail_tee",         &mapgen_forest_trail_tee },
            { "forest_trail_four_way",    &mapgen_forest_trail_four_way },
            { "field",            &mapgen_field },
            { "railroad_straight", &mapgen_railroad },
            { "railroad_curved",   &mapgen_railroad },
            { "railroad_end",      &mapgen_railroad },
            { "railroad_tee",      &mapgen_railroad },
            { "railroad_four_way", &mapgen_railroad },
            { "railroad_bridge",   &mapgen_railroad_bridge },
            { "river_center", &mapgen_river_center },
            { "river_curved_not", &mapgen_river_curved_not },
            { "river_straight",   &mapgen_river_straight },
            { "river_curved",     &mapgen_river_curved },
            { "open_air", &mapgen_open_air },
            { "rift", &mapgen_rift },
            { "hellmouth", &mapgen_hellmouth },
            // New rock function - should be default, but isn't yet for compatibility reasons (old overmaps)
            { "empty_rock", &mapgen_rock },
            // Old rock behavior, for compatibility and near caverns and slime pits
            { "rock", &mapgen_rock_partial },

            { "subway_straight",    &mapgen_subway },
            { "subway_curved",      &mapgen_subway },
            // TODO: Add a dedicated dead-end function. For now it copies the straight section above.
            { "subway_end",         &mapgen_subway },
            { "subway_tee",         &mapgen_subway },
            { "subway_four_way",    &mapgen_subway },

            { "lake_shore", &mapgen_lake_shore },
            { "ravine_edge", &mapgen_ravine_edge },
        }
    };
    const auto iter = pointers.find( ident );
    return iter == pointers.end() ? nullptr : iter->second;
}

ter_id grass_or_dirt()
{
    if( one_in( 4 ) ) {
        return t_grass;
    }
    return t_dirt;
}

ter_id clay_or_sand()
{
    if( one_in( 16 ) ) {
        return t_sand;
    }
    return t_clay;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
///// builtin terrain-specific mapgen functions. big multi-overmap-tile terrains are located in
///// mapgen_functions_big.cpp

void mapgen_null( mapgendata &dat )
{
    debugmsg( "Generating null terrain, please report this as a bug" );
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            dat.m.ter_set( point( i, j ), t_null );
            dat.m.set_radiation( point( i, j ), 0 );
        }
    }
}

// TODO: make void map::ter_or_furn_set(const int x, const int y, const ter_furn_id & tfid);
static void ter_or_furn_set( map *m, const point &p, const ter_furn_id &tfid )
{
    if( tfid.ter != t_null ) {
        m->ter_set( p, tfid.ter );
    } else if( tfid.furn != f_null ) {
        m->furn_set( p, tfid.furn );
    }
}

/*
 * Default above ground non forested 'blank' area; typically a grassy field with a scattering of shrubs,
 *  but changes according to dat->region
 */
void mapgen_field( mapgendata &dat )
{
    map *const m = &dat.m;
    // random area of increased vegetation. Or lava / toxic sludge / etc
    const bool boosted_vegetation = dat.region.field_coverage.boost_chance > rng( 0, 1000000 );
    const int &mpercent_bush = boosted_vegetation ?
                               dat.region.field_coverage.boosted_mpercent_coverage :
                               dat.region.field_coverage.mpercent_coverage;

    // one dominant plant type ( for boosted_vegetation == true )
    ter_furn_id altbush = dat.region.field_coverage.pick( true );

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            // default is
            m->ter_set( point( i, j ), dat.groundcover() );
            // yay, a shrub ( or tombstone )
            if( mpercent_bush > rng( 0, 1000000 ) ) {
                if( boosted_vegetation && dat.region.field_coverage.boosted_other_mpercent > rng( 0, 1000000 ) ) {
                    // already chose the lucky terrain/furniture/plant/rock/etc
                    ter_or_furn_set( m, point( i, j ), altbush );
                } else {
                    // pick from weighted list
                    ter_or_furn_set( m, point( i, j ), dat.region.field_coverage.pick( false ) );
                }
            }
        }
    }

    // FIXME: take 'rock' out and add as regional biome setting
    m->place_items( Item_spawn_data_field, 60, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ),
                    true, dat.when() );
}

int terrain_type_to_nesw_array( oter_id terrain_type, std::array<bool, 4> &array )
{
    // count and mark which directions the road goes
    const oter_t &oter( *terrain_type );
    int num_dirs = 0;
    for( const om_direction::type dir : om_direction::all ) {
        num_dirs += ( array[static_cast<int>( dir )] = oter.has_connection( dir ) );
    }
    return num_dirs;
}

// perform dist counterclockwise rotations on a nesw or neswx array
template<typename T, size_t N>
void nesw_array_rotate( std::array<T, N> &array, size_t dist )
{
    static_assert( N == 8 || N == 4, "Only arrays of size 4 and 8 are supported" );
    if( N == 4 ) {
        while( dist-- ) {
            T temp = array[0];
            array[0] = array[1];
            array[1] = array[2];
            array[2] = array[3];
            array[3] = temp;
        }
    } else {
        while( dist-- ) {
            // N E S W NE SE SW NW
            T temp = array[0];
            array[0] = array[4];
            array[4] = array[1];
            array[1] = array[5];
            array[5] = array[2];
            array[2] = array[6];
            array[6] = array[3];
            array[3] = array[7];
            array[7] = temp;
        }
    }
}

void mapgen_subway( mapgendata &dat )
{
    map *const m = &dat.m;
    // start by filling the whole map with grass/dirt/etc
    dat.fill_groundcover();

    // which of the cardinal directions get subway?
    std::array<bool, 4> subway_nesw = {};
    int num_dirs = terrain_type_to_nesw_array( dat.terrain_type(), subway_nesw );

    // N E S W
    for( int dir = 0; dir < 4; dir++ ) {
        if( dat.t_nesw[dir]->has_flag( oter_flags::subway_connection ) && !subway_nesw[dir] ) {
            num_dirs++;
            subway_nesw[dir] = true;
        }
    }

    // which way should our subway curve, based on neighbor subway?
    std::array<int, 4> curvedir_nesw = {};
    // N E S W
    for( int dir = 0; dir < 4; dir++ ) {
        if( !subway_nesw[dir] ) {
            continue;
        }

        if( dat.t_nesw[dir]->get_type_id().str() != "subway" &&
            !dat.t_nesw[dir]->has_flag( oter_flags::subway_connection ) ) {
            continue;
        }
        // n_* contain details about the neighbor being considered
        std::array<bool, 4> n_subway_nesw = {};
        // TODO: figure out how to call this function without creating a new oter_id object
        int n_num_dirs = terrain_type_to_nesw_array( dat.t_nesw[dir], n_subway_nesw );
        for( int dir = 0; dir < 4; dir++ ) {
            if( dat.t_nesw[dir]->has_flag( oter_flags::subway_connection ) && !n_subway_nesw[dir] ) {
                n_num_dirs++;
                n_subway_nesw[dir] = true;
            }
        }
        // if 2-way neighbor has a subway facing us
        if( n_num_dirs == 2 && n_subway_nesw[( dir + 2 ) % 4] ) {
            // curve towards the direction the neighbor turns
            // our subway curves counterclockwise
            if( n_subway_nesw[( dir - 1 + 4 ) % 4] ) {
                curvedir_nesw[dir]--;
            }
            // our subway curves clockwise
            if( n_subway_nesw[( dir + 1 ) % 4] ) {
                curvedir_nesw[dir]++;
            }
        }
    }

    // calculate how far to rotate the map so we can work with just one orientation
    // also keep track of diagonal subway
    int rot = 0;
    bool diag = false;
    // TODO: reduce amount of logical/conditional constructs here
    switch( num_dirs ) {
        case 4:
            // 4-way intersection
            break;
        case 3:
            // tee
            // E/S/W, rotate 180 degrees
            if( !subway_nesw[0] ) {
                rot = 2;
                break;
            }
            // N/S/W, rotate 270 degrees
            if( !subway_nesw[1] ) {
                rot = 3;
                break;
            }
            // N/E/S, rotate  90 degrees
            if( !subway_nesw[3] ) {
                rot = 1;
                break;
            }
            // N/E/W, don't rotate
            break;
        case 2:
            // straight or diagonal
            // E/W, rotate  90 degrees
            if( subway_nesw[1] && subway_nesw[3] ) {
                rot = 1;
                break;
            }
            // E/S, rotate  90 degrees
            if( subway_nesw[1] && subway_nesw[2] ) {
                rot = 1;
                diag = true;
                break;
            }
            // S/W, rotate 180 degrees
            if( subway_nesw[2] && subway_nesw[3] ) {
                rot = 2;
                diag = true;
                break;
            }
            // W/N, rotate 270 degrees
            if( subway_nesw[3] && subway_nesw[0] ) {
                rot = 3;
                diag = true;
                break;
            }
            // N/E, don't rotate
            if( subway_nesw[0] && subway_nesw[1] ) {
                diag = true;
                break;
            }
            break;                                                                  // N/S, don't rotate
        case 1:
            // dead end
            // E, rotate  90 degrees
            if( subway_nesw[1] ) {
                rot = 1;
                break;
            }
            // S, rotate 180 degrees
            if( subway_nesw[2] ) {
                rot = 2;
                break;
            }
            // W, rotate 270 degrees
            if( subway_nesw[3] ) {
                rot = 3;
                break;
            }
            // N, don't rotate
            break;
    }

    // rotate the arrays left by rot steps
    nesw_array_rotate( subway_nesw, rot );
    nesw_array_rotate( curvedir_nesw, rot );

    // now we have only these shapes: '   |   '-   -'-   -|-

    switch( num_dirs ) {
        case 4:
            // 4-way intersection
            mapf::formatted_set_simple( m, point_zero,
                                        "..^/D^^/D^....^D/^^D/^..\n"
                                        ".^/DX^/DX......XD/^XD/^.\n"
                                        "^/D^X/D^X......X^D/X^D/^\n"
                                        "/D^^XD^.X......X.^DX^^D/\n"
                                        "DXXDDXXXXXXXXXXXXXXDDXXD\n"
                                        "^^/DX^^^X^^^^^^X^^^XD/^^\n"
                                        "^/D^X^^^X^^^^^^X^^^X^D/^\n"
                                        "/D^^X^^^X^^^^^^X^^^X^^D/\n"
                                        "DXXXXXXXXXXXXXXXXXXXXXXD\n"
                                        "^^^^X^^^X^^^^^^X^^^X^^^^\n"
                                        "...^X^^^X^....^X^^^X^...\n"
                                        "...^X^^^X^....^X^^^X^...\n"
                                        "...^X^^^X^....^X^^^X^...\n"
                                        "...^X^^^X^....^X^^^X^...\n"
                                        "^^^^X^^^X^^^^^^X^^^X^^^^\n"
                                        "DXXXXXXXXXXXXXXXXXXXXXXD\n"
                                        "/D^^X^^^X^^^^^^X^^^X^^D/\n"
                                        "^/D^X^^^X^^^^^^X^^^X^D/^\n"
                                        "^^/DX^^^X^^^^^^X^^^XD/^^\n"
                                        "DXXDDXXXXXXXXXXXXXXDDDDD\n"
                                        "/D^^XD^.X......X.^DX^^D/\n"
                                        "^/D^X/D^X......X^D/X^D/^\n"
                                        ".^/DX^/DX......XD/^XD/^.\n"
                                        "..^/D^^/D^....^D/^^D/^..",
                                        mapf::ter_bind( ". # ^ / D X",
                                                t_rock_floor,
                                                t_rock,
                                                t_railroad_rubble,
                                                t_railroad_tie_d,
                                                t_railroad_track_d,
                                                t_railroad_track ),
                                        mapf::furn_bind( ". # ^ / D X",
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null ) );
            break;
        case 3:
            // tee
            mapf::formatted_set_simple( m, point_zero,
                                        "..^/D^^/D^...^/D^^/D^...\n"
                                        ".^/D^^/D^...^/D^^/D^....\n"
                                        "^/D^^/D^...^/D^^/D^.....\n"
                                        "/D^^/D^^^^^/D^^/D^^^^^^^\n"
                                        "DXXXDXXXXXXDXXXDXXXXXXXX\n"
                                        "^^/D^^^^^^^^^/D^^^^^^^^^\n"
                                        "^/D^^^^^^^^^/D^^^^^^^^^^\n"
                                        "/D^^^^^^^^^/D^^^^^^^^^^^\n"
                                        "DXXXXXXDXXXDXXXXXXXXXXXX\n"
                                        "^^^^^/D^^/D^^^^^^^^^^^^^\n"
                                        "...^/D^^/D^.............\n"
                                        "..^/D^^/D^..............\n"
                                        ".^/D^^/D^...............\n"
                                        "^/D^^/D^................\n"
                                        "/D^^/D^^^^|^^|^^|^^|^^|^\n"
                                        "DXXXDXXXXXxXXxXXxXXxXXxX\n"
                                        "^^/D^^^^^^|^^|^^|^^|^^|^\n"
                                        "^/D^^^^^^^|^^|^^|^^|^^|^\n"
                                        "/D^^^^^^^^|^^|^^|^^|^^|^\n"
                                        "DXXXXXXXXXxXXxXXxXXxXXxX\n"
                                        "^^^^^^^^^^|^^|^^|^^|^^|^\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................",
                                        mapf::ter_bind( ". # ^ | X x / D",
                                                t_rock_floor,
                                                t_rock,
                                                t_railroad_rubble,
                                                t_railroad_tie,
                                                t_railroad_track,
                                                t_railroad_track_on_tie,
                                                t_railroad_tie_d,
                                                t_railroad_track_d ),
                                        mapf::furn_bind( ". # ^ | X x / D",
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null ) );
            break;
        case 2:
            // straight or diagonal
            if( diag ) { // diagonal subway get drawn differently from all other types
                mapf::formatted_set_simple( m, point_zero,
                                            "...^DD^^DD^...^DD^^DD^..\n"
                                            "....^DD^^DD^...^DD^^DD^.\n"
                                            ".....^DD^^DD^...^DD^^DD^\n"
                                            "......^DD^^DD^...^DD^^DD\n"
                                            ".......^DD^^DD^...^DD^^D\n"
                                            "#.......^DD^^DD^...^DD^^\n"
                                            "##.......^DD^^DD^...^DD^\n"
                                            "###.......^DD^^DD^...^DD\n"
                                            "####.......^DD^^DD^...^D\n"
                                            "#####.......^DD^^DD^...^\n"
                                            "######.......^DD^^DD^...\n"
                                            "#######.......^DD^^DD^..\n"
                                            "########.......^DD^^DD^.\n"
                                            "#########.......^DD^^DD^\n"
                                            "##########.......^DD^^DD\n"
                                            "###########.......^DD^^D\n"
                                            "############.......^DD^^\n"
                                            "#############.......^DD^\n"
                                            "##############.......^DD\n"
                                            "###############.......^D\n"
                                            "################.......^\n"
                                            "#################.......\n"
                                            "##################......\n"
                                            "###################.....",
                                            mapf::ter_bind( ". # ^ D",
                                                    t_rock_floor,
                                                    t_rock,
                                                    t_railroad_rubble,
                                                    t_railroad_track_d ),
                                            mapf::furn_bind( ". # ^ D",
                                                    f_null,
                                                    f_null,
                                                    f_null,
                                                    f_null ) );
            } else { // normal subway drawing
                mapf::formatted_set_simple( m, point_zero,
                                            "...^X^^^X^....^X^^^X^...\n"
                                            "...-x---x-....-x---x-...\n"
                                            "...^X^^^X^....^X^^^X^...\n"
                                            "...^X^^^X^....^X^^^X^...\n"
                                            "...-x---x-....-x---x-...\n"
                                            "...^X^^^X^....^X^^^X^...\n"
                                            "...^X^^^X^....^X^^^X^...\n"
                                            "...-x---x-....-x---x-...\n"
                                            "...^X^^^X^....^X^^^X^...\n"
                                            "...^X^^^X^....^X^^^X^...\n"
                                            "...-x---x-....-x---x-...\n"
                                            "...^X^^^X^....^X^^^X^...\n"
                                            "...^X^^^X^....^X^^^X^...\n"
                                            "...-x---x-....-x---x-...\n"
                                            "...^X^^^X^....^X^^^X^...\n"
                                            "...^X^^^X^....^X^^^X^...\n"
                                            "...-x---x-....-x---x-...\n"
                                            "...^X^^^X^....^X^^^X^...\n"
                                            "...^X^^^X^....^X^^^X^...\n"
                                            "...-x---x-....-x---x-...\n"
                                            "...^X^^^X^....^X^^^X^...\n"
                                            "...^X^^^X^....^X^^^X^...\n"
                                            "...-x---x-....-x---x-...\n"
                                            "...^X^^^X^....^X^^^X^...",
                                            mapf::ter_bind( ". # ^ - X x",
                                                    t_rock_floor,
                                                    t_rock,
                                                    t_railroad_rubble,
                                                    t_railroad_tie,
                                                    t_railroad_track,
                                                    t_railroad_track_on_tie ),
                                            mapf::furn_bind( ". # ^ - X x",
                                                    f_null,
                                                    f_null,
                                                    f_null,
                                                    f_null,
                                                    f_null,
                                                    f_null ) );
            }
            break;
        case 1:
            // dead end
            mapf::formatted_set_simple( m, point_zero,
                                        "...^X^^^X^..../D^^/D^...\n"
                                        "...-x---x-.../DX^/DX^...\n"
                                        "...^X^^^X^../D^X/D^X^...\n"
                                        "...^X^^^X^./D.^XD^^X^...\n"
                                        "...^X^^^X^/D../D^^^X^...\n"
                                        "...^X^^^X/D../DX^^^X^...\n"
                                        "...^X^^^XD../D^X^^^X^...\n"
                                        "...^X^^/D^./D.-x---x-...\n"
                                        "...^X^/DX^/D..^X^^^X^...\n"
                                        "...^X/D^X/D...^X^^^X^...\n"
                                        "...^XD^^XD....-x---x-...\n"
                                        "...^D^^^D^....^X^^^X^...\n"
                                        "...^X^^^X^....^X^^^X^...\n"
                                        "...-x---x-....-x---x-...\n"
                                        "...^X^^^X^....^X^^^X^...\n"
                                        "...^X^^^X^....^X^^^X^...\n"
                                        "...-x---x-....-x---x-...\n"
                                        "...^X^^^X^....^X^^^X^...\n"
                                        "...^X^^^X^....^X^^^X^...\n"
                                        "...^S^^^S^....^S^^^S^...\n"
                                        "...^^^^^^^....^^^^^^^...\n"
                                        "#......................#\n"
                                        "##....................##\n"
                                        "########################",
                                        mapf::ter_bind( ". # S ^ - / D X x",
                                                t_rock_floor,
                                                t_rock,
                                                t_buffer_stop,
                                                t_railroad_rubble,
                                                t_railroad_tie,
                                                t_railroad_tie_d,
                                                t_railroad_track_d,
                                                t_railroad_track,
                                                t_railroad_track_on_tie ),
                                        mapf::furn_bind( ". # S ^ - / D X x",
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null ) );
            VehicleSpawn::apply( VehicleSpawn_default_subway_deadend, *m, "subway" );
            break;
    }

    // finally, unrotate the map
    m->rotate( rot );
}

// mapgen_railroad
// TODO: Refactor and combine with other similar functions (e.g. road).
void mapgen_railroad( mapgendata &dat )
{
    map *const m = &dat.m;
    // start by filling the whole map with grass/dirt/etc
    dat.fill_groundcover();
    // which of the cardinal directions get railroads?
    std::array<bool, 4> railroads_nesw = {};
    int num_dirs = terrain_type_to_nesw_array( dat.terrain_type(), railroads_nesw );
    // which way should our railroads curve, based on neighbor railroads?
    std::array<int, 4> curvedir_nesw = {};
    for( int dir = 0; dir < 4; dir++ ) { // N E S W
        if( !railroads_nesw[dir] || dat.t_nesw[dir]->get_type_id() != oter_type_railroad ) {
            continue;
        }
        // n_* contain details about the neighbor being considered
        std::array<bool, 4> n_railroads_nesw = {};
        // TODO: figure out how to call this function without creating a new oter_id object
        int n_num_dirs = terrain_type_to_nesw_array( dat.t_nesw[dir], n_railroads_nesw );
        // if 2-way neighbor has a railroad facing us
        if( n_num_dirs == 2 && n_railroads_nesw[( dir + 2 ) % 4] ) {
            // curve towards the direction the neighbor turns
            if( n_railroads_nesw[( dir - 1 + 4 ) % 4] ) {
                curvedir_nesw[dir]--;    // our railroad curves counterclockwise
            }
            if( n_railroads_nesw[( dir + 1 ) % 4] ) {
                curvedir_nesw[dir]++;    // our railroad curves clockwise
            }
        }
    }
    // calculate how far to rotate the map so we can work with just one orientation
    // also keep track of diagonal railroads
    int rot = 0;
    bool diag = false;
    // TODO: reduce amount of logical/conditional constructs here
    switch( num_dirs ) {
        case 4:
            // 4-way intersection
            break;
        case 3:
            // tee
            if( !railroads_nesw[0] ) {
                rot = 2;    // E/S/W, rotate 180 degrees
                break;
            }
            if( !railroads_nesw[1] ) {
                rot = 3;    // N/S/W, rotate 270 degrees
                break;
            }
            if( !railroads_nesw[3] ) {
                rot = 1;    // N/E/S, rotate  90 degrees
                break;
            }
            break;                                       // N/E/W, don't rotate
        case 2:
            // straight or diagonal
            if( railroads_nesw[1] && railroads_nesw[3] ) {
                rot = 1;    // E/W, rotate  90 degrees
                break;
            }
            if( railroads_nesw[1] && railroads_nesw[2] ) {
                rot = 1;    // E/S, rotate  90 degrees
                diag = true;
                break;
            }
            if( railroads_nesw[2] && railroads_nesw[3] ) {
                rot = 2;    // S/W, rotate 180 degrees
                diag = true;
                break;
            }
            if( railroads_nesw[3] && railroads_nesw[0] ) {
                rot = 3;    // W/N, rotate 270 degrees
                diag = true;
                break;
            }
            // N/E, don't rotate
            if( railroads_nesw[0] && railroads_nesw[1] ) {
                diag = true;
                break;
            }
            // N/S, don't rotate
            break;
        case 1:
            // dead end
            // E, rotate  90 degrees
            if( railroads_nesw[1] ) {
                rot = 1;
                break;
            }
            // S, rotate 180 degrees
            if( railroads_nesw[2] ) {
                rot = 2;
                break;
            }
            // W, rotate 270 degrees
            if( railroads_nesw[3] ) {

                rot = 3;
                break;
            }
            // N, don't rotate
            break;
    }
    // rotate the arrays left by rot steps
    nesw_array_rotate( railroads_nesw, rot );
    nesw_array_rotate( curvedir_nesw, rot );
    // now we have only these shapes: '   |   '-   -'-   -|-
    switch( num_dirs ) {
        case 4:
            // 4-way intersection
            mapf::formatted_set_simple( m, point_zero,
                                        ".DD^^DD^........^DD^^DD.\n"
                                        "DD^^DD^..........^DD^^DD\n"
                                        "D^^DD^............^DD^^D\n"
                                        "^^DD^..............^DD^^\n"
                                        "^DD^................^DD^\n"
                                        "DD^..................^DD\n"
                                        "D^....................^D\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "D^....................^D\n"
                                        "DD^..................^DD\n"
                                        "^DD^................^DD^\n"
                                        "^^DD^..............^DD^^\n"
                                        "D^^DD^............^DD^^D\n"
                                        "DD^^DD^..........^DD^^DD\n"
                                        ".DD^^DD^........^DD^^DD.",
                                        mapf::ter_bind( ". ^ D",
                                                t_dirt,
                                                t_railroad_rubble,
                                                t_railroad_track_d ),
                                        mapf::furn_bind( ". ^ D",
                                                f_null,
                                                f_null,
                                                f_null ) );
            break;
        case 3:
            // tee
            mapf::formatted_set_simple( m, point_zero,
                                        ".DD^^DD^........^DD^^DD.\n"
                                        "DD^^DD^..........^DD^^DD\n"
                                        "D^^DD^............^DD^^D\n"
                                        "^^DD^..............^DD^^\n"
                                        "^DD^................^DD^\n"
                                        "DD^..................^DD\n"
                                        "D^....................^D\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "^|^^|^^|^^|^^|^^|^^|^^|^\n"
                                        "XxXXxXXxXXxXXxXXxXXxXXxX\n"
                                        "^|^^|^^|^^|^^|^^|^^|^^|^\n"
                                        "^|^^|^^|^^|^^|^^|^^|^^|^\n"
                                        "^|^^|^^|^^|^^|^^|^^|^^|^\n"
                                        "XxXXxXXxXXxXXxXXxXXxXXxX\n"
                                        "^|^^|^^|^^|^^|^^|^^|^^|^\n"
                                        "........................",
                                        mapf::ter_bind( ". ^ | X x / D",
                                                t_dirt,
                                                t_railroad_rubble,
                                                t_railroad_tie,
                                                t_railroad_track,
                                                t_railroad_track_on_tie,
                                                t_railroad_tie_d,
                                                t_railroad_track_d ),
                                        mapf::furn_bind( ". ^ | X x / D",
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null ) );
            break;
        case 2:
            // straight or diagonal
            if( diag ) {
                // diagonal railroads get drawn differently from all other types
                mapf::formatted_set_simple( m, point_zero,
                                            ".^DD^^DD^.......^DD^^DD^\n"
                                            "..^DD^^DD^.......^DD^^DD\n"
                                            "...^DD^^DD^.......^DD^^D\n"
                                            "....^DD^^DD^.......^DD^^\n"
                                            ".....^DD^^DD^.......^DD^\n"
                                            "......^DD^^DD^.......^DD\n"
                                            ".......^DD^^DD^.......^D\n"
                                            "........^DD^^DD^.......^\n"
                                            ".........^DD^^DD^.......\n"
                                            "..........^DD^^DD^......\n"
                                            "...........^DD^^DD^.....\n"
                                            "............^DD^^DD^....\n"
                                            ".............^DD^^DD^...\n"
                                            "..............^DD^^DD^..\n"
                                            "...............^DD^^DD^.\n"
                                            "................^DD^^DD^\n"
                                            ".................^DD^^DD\n"
                                            "..................^DD^^D\n"
                                            "...................^DD^^\n"
                                            "....................^DD^\n"
                                            ".....................^DD\n"
                                            "......................^D\n"
                                            ".......................^\n"
                                            "........................",
                                            mapf::ter_bind( ". ^ D",
                                                    t_dirt,
                                                    t_railroad_rubble,
                                                    t_railroad_track_d ),
                                            mapf::furn_bind( ". ^ D",
                                                    f_null,
                                                    f_null,
                                                    f_null ) );
            } else { // normal railroads drawing
                mapf::formatted_set_simple( m, point_zero,
                                            ".^X^^^X^........^X^^^X^.\n"
                                            ".-x---x-........-x---x-.\n"
                                            ".^X^^^X^........^X^^^X^.\n"
                                            ".^X^^^X^........^X^^^X^.\n"
                                            ".-x---x-........-x---x-.\n"
                                            ".^X^^^X^........^X^^^X^.\n"
                                            ".^X^^^X^........^X^^^X^.\n"
                                            ".-x---x-........-x---x-.\n"
                                            ".^X^^^X^........^X^^^X^.\n"
                                            ".^X^^^X^........^X^^^X^.\n"
                                            ".-x---x-........-x---x-.\n"
                                            ".^X^^^X^........^X^^^X^.\n"
                                            ".^X^^^X^........^X^^^X^.\n"
                                            ".-x---x-........-x---x-.\n"
                                            ".^X^^^X^........^X^^^X^.\n"
                                            ".^X^^^X^........^X^^^X^.\n"
                                            ".-x---x-........-x---x-.\n"
                                            ".^X^^^X^........^X^^^X^.\n"
                                            ".^X^^^X^........^X^^^X^.\n"
                                            ".-x---x-........-x---x-.\n"
                                            ".^X^^^X^........^X^^^X^.\n"
                                            ".^X^^^X^........^X^^^X^.\n"
                                            ".-x---x-........-x---x-.\n"
                                            ".^X^^^X^........^X^^^X^.",
                                            mapf::ter_bind( ". ^ - X x",
                                                    t_dirt,
                                                    t_railroad_rubble,
                                                    t_railroad_tie,
                                                    t_railroad_track,
                                                    t_railroad_track_on_tie ),
                                            mapf::furn_bind( ". ^ - X x",
                                                    f_null,
                                                    f_null,
                                                    f_null,
                                                    f_null,
                                                    f_null ) );
            }
            break;
        case 1:
            // dead end
            mapf::formatted_set_simple( m, point_zero,
                                        ".^X^^^X^........^X^^^X^.\n"
                                        ".-x---x-........-x---x-.\n"
                                        ".^X^^^X^........^X^^^X^.\n"
                                        ".^X^^^X^........^X^^^X^.\n"
                                        ".-x---x-........-x---x-.\n"
                                        ".^X^^^X^........^X^^^X^.\n"
                                        ".^X^^^X^........^X^^^X^.\n"
                                        ".-x---x-........-x---x-.\n"
                                        ".^X^^^X^........^X^^^X^.\n"
                                        ".^X^^^X^........^X^^^X^.\n"
                                        ".-x---x-........-x---x-.\n"
                                        ".^X^^^X^........^X^^^X^.\n"
                                        ".^S^^^S^........^S^^^S^.\n"
                                        ".^^^^^^^........^^^^^^^.\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................\n"
                                        "........................",
                                        mapf::ter_bind( ". ^ S - X x",
                                                t_dirt,
                                                t_railroad_rubble,
                                                t_buffer_stop,
                                                t_railroad_tie,
                                                t_railroad_track,
                                                t_railroad_track_on_tie ),
                                        mapf::furn_bind( ". ^ S - X x",
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null,
                                                f_null ) );
            break;
    }
    // finally, unrotate the map
    m->rotate( rot );
}
///////////////////
void mapgen_railroad_bridge( mapgendata &dat )
{
    map *const m = &dat.m;
    mapf::formatted_set_simple( m, point_zero,
                                "r^X^^^X^________^X^^^X^r\n"
                                "r-x---x-________-x---x-r\n"
                                "r^X^^^X^________^X^^^X^r\n"
                                "r^X^^^X^________^X^^^X^r\n"
                                "r-x---x-________-x---x-r\n"
                                "r^X^^^X^________^X^^^X^r\n"
                                "r^X^^^X^________^X^^^X^r\n"
                                "r-x---x-________-x---x-r\n"
                                "r^X^^^X^________^X^^^X^r\n"
                                "r^X^^^X^________^X^^^X^r\n"
                                "r-x---x-________-x---x-r\n"
                                "r^X^^^X^________^X^^^X^r\n"
                                "r^X^^^X^________^X^^^X^r\n"
                                "r-x---x-________-x---x-r\n"
                                "r^X^^^X^________^X^^^X^r\n"
                                "r^X^^^X^________^X^^^X^r\n"
                                "r-x---x-________-x---x-r\n"
                                "r^X^^^X^________^X^^^X^r\n"
                                "r^X^^^X^________^X^^^X^r\n"
                                "r-x---x-________-x---x-r\n"
                                "r^X^^^X^________^X^^^X^r\n"
                                "r^X^^^X^________^X^^^X^r\n"
                                "r-x---x-________-x---x-r\n"
                                "r^X^^^X^________^X^^^X^r",
                                mapf::ter_bind( ". _ r ^ - X x", t_dirt, t_concrete, t_railing, t_railroad_rubble, t_railroad_tie,
                                        t_railroad_track, t_railroad_track_on_tie ),
                                mapf::furn_bind( ". _ r ^ - X x", f_null, f_null, f_null, f_null, f_null, f_null, f_null )
                              );
    m->rotate( static_cast<int>( dat.terrain_type()->get_dir() ) );
}

void mapgen_river_center( mapgendata &dat )
{
    fill_background( &dat.m, t_water_moving_dp );
}

void mapgen_river_curved_not( mapgendata &dat )
{
    map *const m = &dat.m;
    fill_background( m, t_water_moving_dp );
    // this is not_ne, so deep on all sides except ne corner, which is shallow
    // shallow is 20,0, 23,4
    int north_edge = rng( 16, 18 );
    int east_edge = rng( 4, 8 );

    for( int x = north_edge; x < SEEX * 2; x++ ) {
        for( int y = 0; y < east_edge; y++ ) {
            int circle_edge = ( ( SEEX * 2 - x ) * ( SEEX * 2 - x ) ) + ( y * y );
            if( circle_edge <= 8 ) {
                m->ter_set( point( x, y ), grass_or_dirt() );
            }
            if( circle_edge == 9 && one_in( 25 ) ) {
                m->ter_set( point( x, y ), clay_or_sand() );
            } else if( circle_edge <= 36 ) {
                m->ter_set( point( x, y ), t_water_moving_sh );
            }
        }
    }

    if( dat.terrain_type() == oter_river_c_not_se ) {
        m->rotate( 1 );
    }
    if( dat.terrain_type() == oter_river_c_not_sw ) {
        m->rotate( 2 );
    }
    if( dat.terrain_type() == oter_river_c_not_nw ) {
        m->rotate( 3 );
    }
}

void mapgen_river_straight( mapgendata &dat )
{
    map *const m = &dat.m;
    fill_background( m, t_water_moving_dp );

    for( int x = 0; x < SEEX * 2; x++ ) {
        int ground_edge = rng( 1, 3 );
        int shallow_edge = rng( 4, 6 );
        line( m, grass_or_dirt(), point( x, 0 ), point( x, ground_edge ) );
        if( one_in( 25 ) ) {
            m->ter_set( point( x, ++ground_edge ), clay_or_sand() );
        }
        line( m, t_water_moving_sh, point( x, ++ground_edge ), point( x, shallow_edge ) );
    }

    if( dat.terrain_type() == oter_river_east ) {
        m->rotate( 1 );
    }
    if( dat.terrain_type() == oter_river_south ) {
        m->rotate( 2 );
    }
    if( dat.terrain_type() == oter_river_west ) {
        m->rotate( 3 );
    }
}

void mapgen_river_curved( mapgendata &dat )
{
    map *const m = &dat.m;
    fill_background( m, t_water_moving_dp );
    // NE corner deep, other corners are shallow.  do 2 passes: one x, one y
    for( int x = 0; x < SEEX * 2; x++ ) {
        int ground_edge = rng( 1, 3 );
        int shallow_edge = rng( 4, 6 );
        line( m, grass_or_dirt(), point( x, 0 ), point( x, ground_edge ) );
        if( one_in( 25 ) ) {
            m->ter_set( point( x, ++ground_edge ), clay_or_sand() );
        }
        line( m, t_water_moving_sh, point( x, ++ground_edge ), point( x, shallow_edge ) );
    }
    for( int y = 0; y < SEEY * 2; y++ ) {
        int ground_edge = rng( 19, 21 );
        int shallow_edge = rng( 16, 18 );
        line( m, grass_or_dirt(), point( ground_edge, y ), point( SEEX * 2 - 1, y ) );
        if( one_in( 25 ) ) {
            m->ter_set( point( --ground_edge, y ), clay_or_sand() );
        }
        line( m, t_water_moving_sh, point( shallow_edge, y ), point( --ground_edge, y ) );
    }

    if( dat.terrain_type() == oter_river_se ) {
        m->rotate( 1 );
    }
    if( dat.terrain_type() == oter_river_sw ) {
        m->rotate( 2 );
    }
    if( dat.terrain_type() == oter_river_nw ) {
        m->rotate( 3 );
    }
}

void mapgen_rock_partial( mapgendata &dat )
{
    map *const m = &dat.m;
    fill_background( m, t_rock );
    for( int i = 0; i < 4; i++ ) {
        if( dat.t_nesw[i] == oter_slimepit || dat.t_nesw[i] == oter_slimepit_down ) {
            dat.dir( i ) = 6;
        } else {
            dat.dir( i ) = 0;
        }
    }

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( rng( 0, dat.n_fac ) > j || rng( 0, dat.s_fac ) > SEEY * 2 - 1 - j ||
                rng( 0, dat.w_fac ) > i || rng( 0, dat.e_fac ) > SEEX * 2 - 1 - i ) {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
    }
}

void mapgen_rock( mapgendata &dat )
{
    fill_background( &dat.m, t_rock );
}

void mapgen_open_air( mapgendata &dat )
{
    fill_background( &dat.m, t_open_air );
}

void mapgen_rift( mapgendata &dat )
{
    map *const m = &dat.m;

    if( dat.north() != oter_rift && dat.north() != oter_hellmouth ) {
        if( connects_to( dat.north(), 2 ) ) {
            dat.n_fac = rng( -6, -2 );
        } else {
            dat.n_fac = rng( 2, 6 );
        }
    }
    if( dat.east() != oter_rift && dat.east() != oter_hellmouth ) {
        if( connects_to( dat.east(), 3 ) ) {
            dat.e_fac = rng( -6, -2 );
        } else {
            dat.e_fac = rng( 2, 6 );
        }
    }
    if( dat.south() != oter_rift && dat.south() != oter_hellmouth ) {
        if( connects_to( dat.south(), 0 ) ) {
            dat.s_fac = rng( -6, -2 );
        } else {
            dat.s_fac = rng( 2, 6 );
        }
    }
    if( dat.west() != oter_rift && dat.west() != oter_hellmouth ) {
        if( connects_to( dat.west(), 1 ) ) {
            dat.w_fac = rng( -6, -2 );
        } else {
            dat.w_fac = rng( 2, 6 );
        }
    }
    // Negative *_fac values indicate rock floor connection, otherwise solid rock
    // Of course, if we connect to a rift, *_fac = 0, and thus lava extends all the
    //  way.
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( ( dat.n_fac < 0 && j < dat.n_fac * -1 ) || ( dat.s_fac < 0 && j >= SEEY * 2 - dat.s_fac ) ||
                ( dat.w_fac < 0 && i < dat.w_fac * -1 ) || ( dat.e_fac < 0 && i >= SEEX * 2 - dat.e_fac ) ) {
                m->ter_set( point( i, j ), t_rock_floor );
            } else if( j < dat.n_fac || j >= SEEY * 2 - dat.s_fac ||
                       i < dat.w_fac || i >= SEEX * 2 - dat.e_fac ) {
                m->ter_set( point( i, j ), t_rock );
            } else {
                m->ter_set( point( i, j ), t_lava );
            }
        }
    }

}

void mapgen_hellmouth( mapgendata &dat )
{
    map *const m = &dat.m;
    // what is this, doom?
    // .. seriously, though...
    for( int i = 0; i < 4; i++ ) {
        if( dat.t_nesw[i] != oter_rift && dat.t_nesw[i] != oter_hellmouth ) {
            dat.dir( i ) = 6;
        }
    }

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( j < dat.n_fac || j >= SEEY * 2 - dat.s_fac || i < dat.w_fac || i >= SEEX * 2 - dat.e_fac ||
                ( i >= 6 && i < SEEX * 2 - 6 && j >= 6 && j < SEEY * 2 - 6 ) ) {
                m->ter_set( point( i, j ), t_rock_floor );
            } else {
                m->ter_set( point( i, j ), t_lava );
            }
            if( i >= SEEX - 1 && i <= SEEX && j >= SEEY - 1 && j <= SEEY ) {
                m->ter_set( point( i, j ), t_slope_down );
            }
        }
    }
    switch( rng( 0, 4 ) ) { // Randomly chosen "altar" design
        case 0:
            for( int i = 7; i <= 16; i += 3 ) {
                m->ter_set( point( i, 6 ), t_rock );
                m->ter_set( point( i, 17 ), t_rock );
                m->ter_set( point( 6, i ), t_rock );
                m->ter_set( point( 17, i ), t_rock );
                if( i > 7 && i < 16 ) {
                    m->ter_set( point( i, 10 ), t_rock );
                    m->ter_set( point( i, 13 ), t_rock );
                } else {
                    m->ter_set( point( i - 1, 6 ), t_rock );
                    m->ter_set( point( i - 1, 10 ), t_rock );
                    m->ter_set( point( i - 1, 13 ), t_rock );
                    m->ter_set( point( i - 1, 17 ), t_rock );
                }
            }
            break;
        case 1:
            for( int i = 6; i < 11; i++ ) {
                m->ter_set( point( i, i ), t_lava );
                m->ter_set( point( SEEX * 2 - 1 - i, i ), t_lava );
                m->ter_set( point( i, SEEY * 2 - 1 - i ), t_lava );
                m->ter_set( point( SEEX * 2 - 1 - i, SEEY * 2 - 1 - i ), t_lava );
                if( i < 10 ) {
                    m->ter_set( point( i + 1, i ), t_lava );
                    m->ter_set( point( SEEX * 2 - i, i ), t_lava );
                    m->ter_set( point( i + 1, SEEY * 2 - 1 - i ), t_lava );
                    m->ter_set( point( SEEX * 2 - i, SEEY * 2 - 1 - i ), t_lava );

                    m->ter_set( point( i, i + 1 ), t_lava );
                    m->ter_set( point( SEEX * 2 - 1 - i, i + 1 ), t_lava );
                    m->ter_set( point( i, SEEY * 2 - i ), t_lava );
                    m->ter_set( point( SEEX * 2 - 1 - i, SEEY * 2 - i ), t_lava );
                }
                if( i < 9 ) {
                    m->ter_set( point( i + 2, i ), t_rock );
                    m->ter_set( point( SEEX * 2 - i + 1, i ), t_rock );
                    m->ter_set( point( i + 2, SEEY * 2 - 1 - i ), t_rock );
                    m->ter_set( point( SEEX * 2 - i + 1, SEEY * 2 - 1 - i ), t_rock );

                    m->ter_set( point( i, i + 2 ), t_rock );
                    m->ter_set( point( SEEX * 2 - 1 - i, i + 2 ), t_rock );
                    m->ter_set( point( i, SEEY * 2 - i + 1 ), t_rock );
                    m->ter_set( point( SEEX * 2 - 1 - i, SEEY * 2 - i + 1 ), t_rock );
                }
            }
            break;
        case 2:
            for( int i = 7; i < 17; i++ ) {
                m->ter_set( point( i, 6 ), t_rock );
                m->ter_set( point( 6, i ), t_rock );
                m->ter_set( point( i, 17 ), t_rock );
                m->ter_set( point( 17, i ), t_rock );
                if( i != 7 && i != 16 && i != 11 && i != 12 ) {
                    m->ter_set( point( i, 8 ), t_rock );
                    m->ter_set( point( 8, i ), t_rock );
                    m->ter_set( point( i, 15 ), t_rock );
                    m->ter_set( point( 15, i ), t_rock );
                }
                if( i == 11 || i == 12 ) {
                    m->ter_set( point( i, 10 ), t_rock );
                    m->ter_set( point( 10, i ), t_rock );
                    m->ter_set( point( i, 13 ), t_rock );
                    m->ter_set( point( 13, i ), t_rock );
                }
            }
            break;
        case 3:
            for( int i = 6; i < 11; i++ ) {
                for( int j = 6; j < 11; j++ ) {
                    m->ter_set( point( i, j ), t_lava );
                    m->ter_set( point( SEEX * 2 - 1 - i, j ), t_lava );
                    m->ter_set( point( i, SEEY * 2 - 1 - j ), t_lava );
                    m->ter_set( point( SEEX * 2 - 1 - i, SEEY * 2 - 1 - j ), t_lava );
                }
            }
            break;
    }

}

void mapgen_forest( mapgendata &dat )
{
    map *const m = &dat.m;

    // perimeter_size is a useful shorthand that should not be changed:
    static constexpr int perimeter_size = SEEX * 4 + SEEY * 4;
    // The following constexpr terms would do well to be json-ized.
    // The average mid-way point between the forest and its adjacent biomes:
    static constexpr int average_depth = 5 * SEEX / 12;
    // The average number of complete oscillations which should occur in the biome border per map-tile:
    static constexpr int average_extra_critical_points = 1;
    // The intensity of the oscillations along forest borders:
    static constexpr int biome_transition_abruptness = 1;
    // The standard deviation of the lengthwise distribution of the oscillations:
    static constexpr int border_deviation = 3;
    // Higher values = more pronounced curves, obscures seams:
    static constexpr float border_curviness = 2;
    // The standard deviation of the depthwise distribution of the oscillations' peaks:
    static constexpr float depth_deviation = 1;
    // Scalar for the amount of space to add between the groundcover and feature margins:
    static constexpr int groundcover_margin = average_depth / 2;
    // Scaling factor to apply to the weighting of the self-terrain over adjacent ones:
    static constexpr float self_scalar = 3.;

    // Adjacency factor is used to weight the frequency of a feature
    // being placed by the relative density of the current terrain to its
    // neighbors. For example, a forest_thick surrounded by forest_thick on
    // all sides can be much more dense than a forest_water surrounded by
    // fields on all sides. The properties of this density and blending would
    // do well to be encoded in JSON for the regional and biome settings, but
    // for now use the general hardcoded pattern from previous generations of the
    // algorithm.

    // "Sparsity Factor" is a misnomer carried over from JSON; the value reflects
    // the density of the terrain, not the sparsity.

    /**
    * Determines the density of natural features in \p ot.
    *
    * If there is no defind biome for \p ot, returns a sparsity factor
    * of 0. It's possible to specify biomes in the forest regional settings
    * that are not rendered by this forest map gen method, in order to control
    * how terrains are blended together (e.g. specify roads with an equal
    * biome to forests so that forests don't fade out as they transition to roads).
    *
    * @param ot The type of terrain to determine the sparseness of.
    * @return A discrete scale of the density of natural features occurring in \p ot.
    */
    const auto get_sparseness_adjacency_factor = [&dat]( const oter_id & ot ) {
        const auto biome = dat.region.forest_composition.biomes.find( ot );
        if( biome == dat.region.forest_composition.biomes.end() ) {
            return 0;
        }
        return biome->second.sparseness_adjacency_factor;
    };

    const ter_furn_id no_ter_furn = ter_furn_id();

    // Calculate the maximum possible sparseness factor (density) for the region.
    int max_factor = 0;
    if( !dat.region.forest_composition.biomes.empty() ) {
        std::vector<int> factors;
        factors.reserve( dat.region.forest_composition.biomes.size() );
        for( const auto &b : dat.region.forest_composition.biomes ) {
            factors.push_back( b.second.sparseness_adjacency_factor );
        }
        max_factor = *max_element( std::begin( factors ), std::end( factors ) );
    }

    // Get the sparseness factor (density) for this and adjacent terrain.
    const int factor = get_sparseness_adjacency_factor( dat.terrain_type() );
    for( int i = 0; i < 4; i++ ) {
        dat.dir( i ) = get_sparseness_adjacency_factor( dat.t_nesw[i] );
    }

    // The index of the following structures are defined in dat.dir():
    //
    // (0,0)     NORTH
    //      ---------------
    //      | 7  | 0 | 4  |
    //      |----|---|----|
    // WEST | 3  |   | 1  |  EAST
    //      |----|---|----|
    //      | 6  | 2 | 5  |
    //      ---------------
    //           SOUTH      (SEEX * 2, SEEY * 2)

    // In order to feather (blend) this overmap tile with adjacent ones, the general composition thereof must be known.
    // This can be calculated once from dat.t_nesw, and stored here:
    std::array<const forest_biome *, 8> adjacent_biomes;
    for( int d = 0; d < 7; d++ ) {
        auto lookup = dat.region.forest_composition.biomes.find( dat.t_nesw[d] );
        if( lookup != dat.region.forest_composition.biomes.end() ) {
            adjacent_biomes[d] = &( lookup->second );
        } else {
            adjacent_biomes[d] = nullptr;
        }
    }

    // Keep track of the "true perimeter" of the biome. It has a curve to make it seem natural.
    // The depth of the perimeter at each border of the forest being generated:
    std::array<int, 8> border_depth;

    for( int bd_x = 0; bd_x < 2; bd_x++ )
        for( int bd_y = 0; bd_y < 2; bd_y++ ) {
            // Use the corners of the overmap tiles as hash seeds.
            point global_corner = m->getabs( point( bd_x * SEEX * 2, bd_y * SEEY * 2 ) );
            uint32_t net_hash = std::hash<uint32_t> {}( global_corner.x ) ^ ( std::hash<int> {}( global_corner.y )
                                << 1 );
            uint32_t h_hash = net_hash;
            uint32_t v_hash = std::hash<uint32_t> {}( net_hash );
            float h_unit_hash = static_cast<float>( h_hash ) / static_cast<float>( UINT32_MAX );
            float v_unit_hash = static_cast<float>( v_hash ) / static_cast<float>( UINT32_MAX );
            // Apply the box-muller transform to produce a gaussian distribution with the desired mean and deviation.
            float mag = depth_deviation * std::sqrt( -2. * log( h_unit_hash ) );
            int h_norm_transform = static_cast<int>( clamp( std::round( mag * std::cos(
                                       2 * M_PI * v_unit_hash ) + average_depth ), static_cast<double>( INT32_MIN ),
                                   static_cast<double>( INT32_MAX ) ) );
            int v_norm_transform = static_cast<int>( clamp( std::round( mag * std::sin(
                                       2 * M_PI * v_unit_hash ) + average_depth ), static_cast<double>( INT32_MIN ),
                                   static_cast<double>( INT32_MAX ) ) );

            int corner_index = bd_x ? 1 + bd_y : 3 * bd_y; // Counterclockwise labeling.
            border_depth[corner_index] = std::abs( h_norm_transform );
            border_depth[corner_index + 4] = std::abs( v_norm_transform );
        }

    // Indicies of border_depth accessible by dat.dir() nomenclature, [h_idx 0..4 : v_idx 0..4]:
    static constexpr std::array<int, 8> edge_corner_mappings = {0, 5, 3, 4, 1, 6, 2, 7};

    // Now, generate a curve along the border of the biome, which will be used to calculate each cardinally
    // adjacent biome's relative impact.
    // Format: [ SEEX * 2 (North) : SEEY * 2 (East) : SEEX * 2 (South) : SEEX * 2 (West) ] (order from dat.dir())
    std::array<int, perimeter_size> perimeter_depth;
    for( int edge = 0; edge < 4; edge++ ) {
        int perimeter_depth_offset = ( SEEX * 2 ) * ( ( edge + 1 ) / 2 ) + ( SEEY * 2 ) * ( edge / 2 );
        int edge_length = edge % 2 == 0 ? SEEX * 2 : SEEY * 2;
        int interediary_crit_point_count = std::round( rng_normal( 0, average_extra_critical_points * 2 ) );
        int *critical_point_displacements = new int[interediary_crit_point_count + 2];
        int *critical_point_depths = new int[interediary_crit_point_count + 2];
        critical_point_displacements[0] = 0;
        critical_point_displacements[interediary_crit_point_count + 1] =
            edge_length; // first coordinate of next overmap location
        critical_point_depths[0] = border_depth[edge_corner_mappings[edge]];
        critical_point_depths[interediary_crit_point_count + 1] = border_depth[edge_corner_mappings[edge +
                4]];
        // Generate critical points in-order displacement-wise.
        for( int ci = 1; ci < interediary_crit_point_count + 1; ci++ ) {
            critical_point_displacements[ci] = clamp<int>( std::abs( static_cast<int>( std::round( normal_roll(
                                                   static_cast<double>(
                                                           edge_length * ( ci - 1 ) ) / ( interediary_crit_point_count + 2 ), border_deviation ) ) ) ), 1,
                                               edge_length - 2 );
            critical_point_depths[ci] = static_cast<int>( std::round( abs( normal_roll( average_depth,
                                        depth_deviation ) ) ) );
            // Ensure order by swapping:
            if( critical_point_displacements[ci] < critical_point_displacements[ci - 1] ) {
                int buffer = critical_point_displacements[ci];
                critical_point_displacements[ci] = critical_point_displacements[ci - 1];
                critical_point_displacements[ci - 1] = buffer;
            }
        }

        // By only considering the left and right two points, transitioning the curve across map borders becomes unnoticable.
        int ubidx = 1; // Upper bound index
        for( int step = 0; step < edge_length; step++ ) {
            while( step > critical_point_displacements[ubidx] ) {
                if( ++ubidx > interediary_crit_point_count + 1 ) {
                    // Should never happen, but good future proofing.
                    ubidx = interediary_crit_point_count + 1;
                    break;
                }
            }
            float w_left = std::pow( std::abs( critical_point_displacements[ubidx - 1] - step ) + 1,
                                     -border_curviness );
            float w_right = std::pow( std::abs( critical_point_displacements[ubidx] - step ) + 1,
                                      -border_curviness );
            perimeter_depth[perimeter_depth_offset + step] = std::round( ( w_left * critical_point_depths[ubidx
                    - 1]
                    + w_right * critical_point_depths[ubidx] ) / ( w_left + w_right ) );

            if( perimeter_depth[perimeter_depth_offset + step] < 1 ) {
                perimeter_depth[perimeter_depth_offset + step] =
                    1;    // Clamp depth to within the overmap tile. Makes math pretty.
            }
        }
        delete[] critical_point_displacements;
        delete[] critical_point_depths;
    }

    // Get the current biome definition for this terrain.
    const auto current_biome_def_it = dat.region.forest_composition.biomes.find( dat.terrain_type() );

    // If there is no biome definition for this terrain, fill in with the region's default ground cover
    // and bail--nothing more to be done. Should not continue with terrain feathering if there is
    // nothing to feather into. Generally, this should never happen.
    if( current_biome_def_it == dat.region.forest_composition.biomes.end() ) {
        dat.fill_groundcover();
        return;
    }
    forest_biome self_biome = current_biome_def_it->second;

    /**
    * Modifies border weights to conform to biome conditions along corners.
    *
    * Ensures that adjacent forest_mapgen() dependent terrains do not generate outcroppings into
    * continuous biomes that they are mutually adjacent to, focusing on details provided for a
    * corner of the map.
    *
    * @param ccw The forest biome counter-clockwise of the evaluated corner.
    * @param corner The forest biome in the evaluated corner (i.e. NW, NE, SW, or SE).
    * @param cw The forest biome clockwise of the evaluated corner.
    * @param ccw_weight The relative impact of the counterclockwise biome on generation.
    * @param cw_weight The relative impact of the clockwise biome on generation.
    * @param self_weight The relative impact of the original biome on generation.
    */
    const auto unify_continuous_border = [&self_biome]( const forest_biome * ccw,
                                         const forest_biome * corner, const forest_biome * cw, float * ccw_weight, float * cw_weight,
    float * self_weight ) {
        if( ccw != cw ) {
            if( ccw == corner && cw == &self_biome ) {
                *cw_weight = *cw_weight / ( *self_weight + *cw_weight );
                *self_weight = *self_weight - *cw_weight;
            } else if( cw == corner && ccw == &self_biome ) {
                *ccw_weight = *ccw_weight / ( *self_weight + *ccw_weight );
                *self_weight = *self_weight - *cw_weight;
            }
        }
    };

    /**
    * Prevents the generation of outcroppings from biomes across contiguous adjacencies.
    *
    * Ensures that adjacent forest_mapgen() dependent terrains do not generate outcroppings into
    * continuous biomes that they are mutually adjacent to, checking weights in each corner.
    *
    * @param cardinal_four_weights The relative impacts of the cardinally adjacent biomes on generation.
    * @param p the point in the terrain being weighted from the cardinally adjacent biomes.
    */
    const auto unify_all_borders = [&unify_continuous_border,
                                    &adjacent_biomes]( std::array<float, 4> &cardinal_four_weights, float * self_weight,
    const point & p ) {
        // Refer to dat.dir() convention.
        if( p.x < SEEX ) {
            if( p.y < SEEY ) {
                unify_continuous_border( adjacent_biomes[3], adjacent_biomes[7], adjacent_biomes[0],
                                         // NOLINTNEXTLINE(readability-container-data-pointer)
                                         &cardinal_four_weights[3], &cardinal_four_weights[0], self_weight );
            } else {
                // NOLINTNEXTLINE(readability-container-data-pointer)
                unify_continuous_border( adjacent_biomes[0], adjacent_biomes[4], adjacent_biomes[1],
                                         &cardinal_four_weights[0], &cardinal_four_weights[1], self_weight );
            }
        } else {
            if( p.y < SEEY ) {
                unify_continuous_border( adjacent_biomes[1], adjacent_biomes[5], adjacent_biomes[2],
                                         &cardinal_four_weights[1], &cardinal_four_weights[2], self_weight );
            } else {
                unify_continuous_border( adjacent_biomes[2], adjacent_biomes[6], adjacent_biomes[3],
                                         &cardinal_four_weights[2], &cardinal_four_weights[3], self_weight );
            }
        }
    };

    /**
    * Calculates a weighted proximity of a point to the perimeter in each cardinal direction.
    *
    * Format is that defined in dir(), i.e. clockwise starting from north.
    *
    * @param p The point to evaluate the weights of adjacent directions on.
    * @param scaling_factor The scaling factor to apply to the weights.
    * @param weights A 4 element array of floats to store the output in, dat.dir() order.
    * @param root_depth_offset an offset to apply to the already-generated perimeter_depth
    * @return The sum of all of the weights written to \p weights.
    */
    const auto nesw_weights = [&perimeter_depth, &adjacent_biomes]( const point & p,
    float scaling_factor, std::array<float, 4> &weights, float root_depth_offset = 0. ) {
        float net_weight = 0.;
        std::array<float, 4> perimeter_depths;
        std::array<int, 4> point_depths;
        point_depths[0] = p.y;
        point_depths[1] = SEEX * 2 - p.x - 1;
        point_depths[2] = SEEY * 2 - p.y - 1;
        point_depths[3] = p.x;
        perimeter_depths[0] = std::max<int>( perimeter_depth[p.x] + root_depth_offset, 1 );
        perimeter_depths[1] = std::max<int>( perimeter_depth[SEEX * 2 + p.y] + root_depth_offset, 1 );
        perimeter_depths[2] = std::max<int>( perimeter_depth[SEEX * 2 + SEEY * 2 + p.x] + root_depth_offset,
                                             1 );
        perimeter_depths[3] = std::max<int>( perimeter_depth[SEEX * 4 + SEEY * 2 + p.y] + root_depth_offset,
                                             1 );
        for( int wi = 0; wi < 4; wi++ )
            if( adjacent_biomes[wi] == nullptr ) {
                // Biome-less terrain does not feather on its side, but biome-owning terrain is presumed to.
                // In order to account for this, the border of the map generation function must be determined
                // to correspond either to the half-way point between biomes or the end-point of the transition.
                weights[wi] = std::pow( static_cast<float>( perimeter_depths[wi] ) / ( point_depths[wi] + 1 ),
                                        biome_transition_abruptness ) * scaling_factor;
                net_weight += weights[wi];
            } else {
                weights[wi] = std::pow( static_cast<float>( perimeter_depths[wi] ) / ( point_depths[wi] + 1 ),
                                        biome_transition_abruptness ) * scaling_factor / std::pow( static_cast<float>
                                                ( perimeter_depths[wi] ),
                                                biome_transition_abruptness );
                net_weight += weights[wi];
            }
        return net_weight;
    };

    /**
    * Determines the groundcover that should be placed at a furniture-less point in a forest.
    *
    * Similar to the get_feathered_feature lambda with a different weighting algorithm,
    * which favors the biome of this terrain over that of adjacent ones by fixed margin.
    * Only selects groundcover, rather than biome-appropriate furniture.
    *
    * @return The groundcover to be placed at the specified point in the forest.
    */
    const auto get_feathered_groundcover = [&max_factor, &factor, &self_biome,
                 &adjacent_biomes, &nesw_weights, &unify_all_borders, &dat]( const point & p ) {
        std::array<float, 4> adj_weights;
        float net_weight = nesw_weights( p, factor, adj_weights, -groundcover_margin );
        float self_weight = self_scalar;
        unify_all_borders( adj_weights, &self_weight, p );
        static constexpr int no_dir = -1;
        static constexpr int empty = -2;
        weighted_float_list<const int> direction_pool;
        direction_pool.add( 0, adj_weights[0] );
        direction_pool.add( 1, adj_weights[1] );
        direction_pool.add( 2, adj_weights[2] );
        direction_pool.add( 3, adj_weights[3] );
        direction_pool.add( no_dir, self_weight );
        direction_pool.add( empty, ( net_weight + self_weight ) * max_factor -
                            ( adj_weights[0] * dat.n_fac + adj_weights[1] * dat.e_fac + adj_weights[2] * dat.s_fac +
                              adj_weights[3] * dat.w_fac + self_weight * factor ) );

        const int feather_selection = *direction_pool.pick();
        switch( feather_selection ) {
            case no_dir:
                return *self_biome.groundcover.pick();
            case empty:
                return *dat.region.default_groundcover.pick();
            default:
                if( adjacent_biomes[feather_selection] != nullptr ) {
                    return *adjacent_biomes[feather_selection]->groundcover.pick();
                } else {
                    return *dat.region.default_groundcover.pick();
                }
        }
    };

    /**
    * Determines the feature that should be placed at a point in the forest.
    *
    * Determines the position of \p p relative to the calculated perimeter,
    * and from this picks an item from a weighted feature pool in the involved
    * biomes.
    * In the case of corners,
    *
    * @param p the point to check to place a feature at.
    */
    const auto get_feathered_feature = [&no_ter_furn, &max_factor, &factor, &self_biome,
                                                      &adjacent_biomes, &nesw_weights, &get_feathered_groundcover, &unify_all_borders,
                  &dat]( const point & p ) {
        std::array<float, 4> adj_weights;
        float net_weight = nesw_weights( p, factor, adj_weights );
        float self_weight = self_scalar;
        unify_all_borders( adj_weights, &self_weight, p );

        // Pool together all of the biomes which the target point might constitute.
        weighted_float_list<const int>
        direction_pool; // Style guidelines say use int, but it is necessary for this weighting algorithm.
        // Two special cases: default and sparsity:
        static constexpr int no_dir = -1;
        static constexpr int empty = -2;
        direction_pool.add( 0, adj_weights[0] );
        direction_pool.add( 1, adj_weights[1] );
        direction_pool.add( 2, adj_weights[2] );
        direction_pool.add( 3, adj_weights[3] );
        direction_pool.add( no_dir, self_weight );
        direction_pool.add( empty, ( net_weight + self_weight )* max_factor -
                            ( adj_weights[0] * dat.n_fac + adj_weights[1] * dat.e_fac + adj_weights[2] * dat.s_fac +
                              adj_weights[3] * dat.w_fac + self_weight * factor ) );

        // Pick from the direction pool, and calculate + return the appropriate terrain:
        ter_furn_id feature;
        const int feather_selection = *direction_pool.pick();
        switch( feather_selection ) {
            case no_dir:
                feature = self_biome.pick();
                break;
            case empty:
                feature = no_ter_furn;
                break;
            default:
                if( adjacent_biomes[feather_selection] != nullptr ) {
                    feature = adjacent_biomes[feather_selection]->pick();

                } else {
                    feature = no_ter_furn;
                }
                break;
        }
        if( feature.ter == no_ter_furn.ter ) {
            feature.ter = get_feathered_groundcover( p );
        }
        return feature;
    };

    /**
    * Attempt, randomly, to place a terrain dependent furniture.
    *
    * For example, f_cattails on t_water_sh.
    * @param tid: The terrain to place a dependent feature on.
    * @param p: The point to place the dependent feature, if one is selected.
    */
    const auto set_terrain_dependent_furniture =
    [&self_biome, &m]( const ter_id & tid, const point & p ) {
        const auto terrain_dependent_furniture_it = self_biome.terrain_dependent_furniture.find(
                    tid );
        if( terrain_dependent_furniture_it == self_biome.terrain_dependent_furniture.end() ) {
            // No terrain dependent furnitures for this terrain.
            return;
        }

        const forest_biome_terrain_dependent_furniture tdf = terrain_dependent_furniture_it->second;
        if( tdf.furniture.get_weight() <= 0 ) {
            // We've got furnitures, but their weight is 0 or less.
            return;
        }

        if( one_in( tdf.chance ) ) {
            // Pick a furniture and set it on the map right now.
            const furn_id *fid = tdf.furniture.pick();
            m->furn_set( p, *fid );
        }
    };

    // For each location within this overmap terrain,
    // Lay groundcover, place a feature, and place terrain dependent furniture.
    for( int x = 0; x < SEEX * 2; x++ ) {
        for( int y = 0; y < SEEY * 2; y++ ) {
            const ter_furn_id feature = get_feathered_feature( point( x, y ) );
            m->ter_set( point( x, y ), feature.ter );
            m->furn_set( point( x, y ), feature.furn );
            set_terrain_dependent_furniture( feature.ter, point( x, y ) );
        }
    }

    // Place items on this terrain as defined in the biome.
    for( int i = 0; i < self_biome.item_spawn_iterations; i++ ) {
        m->place_items( self_biome.item_group, self_biome.item_group_chance,
                        point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true, dat.when() );
    }
}

void mapgen_forest_trail_straight( mapgendata &dat )
{
    map *const m = &dat.m;
    mapgendata forest_mapgen_dat( dat, oter_forest_thick.id() );
    mapgen_forest( forest_mapgen_dat );

    const auto center_offset = [&dat]() {
        return rng( -dat.region.forest_trail.trail_center_variance,
                    dat.region.forest_trail.trail_center_variance );
    };

    const auto width_offset = [&dat]() {
        return rng( dat.region.forest_trail.trail_width_offset_min,
                    dat.region.forest_trail.trail_width_offset_max );
    };

    point center( SEEX + center_offset(), SEEY + center_offset() );

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( i > center.x - width_offset() && i < center.x + width_offset() ) {
                m->furn_set( point( i, j ), f_null );
                m->ter_set( point( i, j ), *dat.region.forest_trail.trail_terrain.pick() );
            }
        }
    }

    if( dat.terrain_type() == oter_forest_trail_ew
        || dat.terrain_type() == oter_forest_trail_end_east
        || dat.terrain_type() == oter_forest_trail_end_west ) {
        m->rotate( 1 );
    }

    m->place_items( Item_spawn_data_forest_trail, 75, center + point( -2, -2 ),
                    center + point( 2, 2 ), true, dat.when() );
}

void mapgen_forest_trail_curved( mapgendata &dat )
{
    map *const m = &dat.m;
    mapgendata forest_mapgen_dat( dat, oter_forest_thick.id() );
    mapgen_forest( forest_mapgen_dat );

    const auto center_offset = [&dat]() {
        return rng( -dat.region.forest_trail.trail_center_variance,
                    dat.region.forest_trail.trail_center_variance );
    };

    const auto width_offset = [&dat]() {
        return rng( dat.region.forest_trail.trail_width_offset_min,
                    dat.region.forest_trail.trail_width_offset_max );
    };

    point center( SEEX + center_offset(), SEEY + center_offset() );

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( ( i > center.x - width_offset() && i < center.x + width_offset() &&
                  j < center.y + width_offset() ) ||
                ( j > center.y - width_offset() && j < center.y + width_offset() &&
                  i > center.x - width_offset() ) ) {
                m->furn_set( point( i, j ), f_null );
                m->ter_set( point( i, j ), *dat.region.forest_trail.trail_terrain.pick() );
            }
        }
    }

    if( dat.terrain_type() == oter_forest_trail_es ) {
        m->rotate( 1 );
    }
    if( dat.terrain_type() == oter_forest_trail_sw ) {
        m->rotate( 2 );
    }
    if( dat.terrain_type() == oter_forest_trail_wn ) {
        m->rotate( 3 );
    }

    m->place_items( Item_spawn_data_forest_trail, 75, center + point( -2, -2 ),
                    center + point( 2, 2 ), true, dat.when() );
}

void mapgen_forest_trail_tee( mapgendata &dat )
{
    map *const m = &dat.m;
    mapgendata forest_mapgen_dat( dat, oter_forest_thick.id() );
    mapgen_forest( forest_mapgen_dat );

    const auto center_offset = [&dat]() {
        return rng( -dat.region.forest_trail.trail_center_variance,
                    dat.region.forest_trail.trail_center_variance );
    };

    const auto width_offset = [&dat]() {
        return rng( dat.region.forest_trail.trail_width_offset_min,
                    dat.region.forest_trail.trail_width_offset_max );
    };

    point center( SEEX + center_offset(), SEEY + center_offset() );

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( ( i > center.x - width_offset() && i < center.x + width_offset() ) ||
                ( j > center.y - width_offset() &&
                  j < center.y + width_offset() && i > center.x - width_offset() ) ) {
                m->furn_set( point( i, j ), f_null );
                m->ter_set( point( i, j ), *dat.region.forest_trail.trail_terrain.pick() );
            }
        }
    }

    if( dat.terrain_type() == oter_forest_trail_esw ) {
        m->rotate( 1 );
    }
    if( dat.terrain_type() == oter_forest_trail_nsw ) {
        m->rotate( 2 );
    }
    if( dat.terrain_type() == oter_forest_trail_new ) {
        m->rotate( 3 );
    }

    m->place_items( Item_spawn_data_forest_trail, 75, center + point( -2, -2 ),
                    center + point( 2, 2 ), true, dat.when() );
}

void mapgen_forest_trail_four_way( mapgendata &dat )
{
    map *const m = &dat.m;
    mapgendata forest_mapgen_dat( dat, oter_forest_thick.id() );
    mapgen_forest( forest_mapgen_dat );

    const auto center_offset = [&dat]() {
        return rng( -dat.region.forest_trail.trail_center_variance,
                    dat.region.forest_trail.trail_center_variance );
    };

    const auto width_offset = [&dat]() {
        return rng( dat.region.forest_trail.trail_width_offset_min,
                    dat.region.forest_trail.trail_width_offset_max );
    };

    point center( SEEX + center_offset(), SEEY + center_offset() );

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( ( i > center.x - width_offset() && i < center.x + width_offset() ) ||
                ( j > center.y - width_offset() &&
                  j < center.y + width_offset() ) ) {
                m->furn_set( point( i, j ), f_null );
                m->ter_set( point( i, j ), *dat.region.forest_trail.trail_terrain.pick() );
            }
        }
    }

    m->place_items( Item_spawn_data_forest_trail, 75, center + point( -2, -2 ),
                    center + point( 2, 2 ), true, dat.when() );
}

void mapgen_lake_shore( mapgendata &dat )
{
    map *const m = &dat.m;
    // Our lake shores may "extend" adjacent terrain, if the adjacent types are defined as being
    // extendable in our regional settings. What this effectively means is that if the lake shore is
    // adjacent to one of these, e.g. a forest, then rather than the lake shore simply having the
    // region's default groundcover for the land parts of the terrain, instead we run the mapgen
    // for this location as if it were the adjacent terrain, and then carve our water out of it as
    // per usual. I think it looks a lot nicer, e.g. in the case of a forest, to have the trees and
    // ground clutter of the forest abutting the water rather than simply some empty ground.

    // To accomplish this extension, we simply count up the adjacent terrains that are in the
    // defined extendable terrain setting, choose the most common one, and then run its mapgen.
    bool did_extend_adjacent_terrain = false;
    if( !dat.region.overmap_lake.shore_extendable_overmap_terrain.empty() ) {
        std::map<oter_id, int> adjacent_type_count;
        for( oter_id &adjacent : dat.t_nesw ) {
            // Define the terrain we'll look for a match on.
            oter_id match = adjacent;

            // Check if this terrain has an alias to something we actually will extend, and if so, use it.
            for( const shore_extendable_overmap_terrain_alias &alias :
                 dat.region.overmap_lake.shore_extendable_overmap_terrain_aliases ) {
                if( is_ot_match( alias.overmap_terrain, adjacent, alias.match_type ) ) {
                    match = alias.alias;
                    break;
                }
            }

            if( std::find( dat.region.overmap_lake.shore_extendable_overmap_terrain.begin(),
                           dat.region.overmap_lake.shore_extendable_overmap_terrain.end(),
                           match ) != dat.region.overmap_lake.shore_extendable_overmap_terrain.end() ) {
                adjacent_type_count[match] += 1;
            }
        }

        if( !adjacent_type_count.empty() ) {
            const auto most_common_adjacent = std::max_element( std::begin( adjacent_type_count ),
                                              std::end( adjacent_type_count ), []( const std::pair<oter_id, int> &p1,
            const std::pair<oter_id, int> &p2 ) {
                return p1.second < p2.second;
            } );

            mapgendata forest_mapgen_dat( dat, most_common_adjacent->first );
            mapgen_forest( forest_mapgen_dat );
            did_extend_adjacent_terrain = run_mapgen_func( most_common_adjacent->first.id().str(),
                                          forest_mapgen_dat );

            // One fun side effect of running another mapgen here is that it may have placed items in locations
            // that we're later going to turn into water. Let's just remove all items.
            if( did_extend_adjacent_terrain ) {
                for( int x = 0; x < SEEX * 2; x++ ) {
                    for( int y = 0; y < SEEY * 2; y++ ) {
                        m->i_clear( point( x, y ) );
                    }
                }
            }
        }
    }

    // If we didn't extend an adjacent terrain, then just fill this entire location with the default
    // groundcover for the region.
    if( !did_extend_adjacent_terrain ) {
        dat.fill_groundcover();
    }

    const oter_id river_center = oter_river_center.id();

    auto is_lake = [&]( const oter_id & id ) {
        // We want to consider river_center as a lake as well, so that the confluence of a
        // river and a lake is a continuous water body.
        return id.obj().is_lake() || id == river_center;
    };

    const auto is_shore = [&]( const oter_id & id ) {
        return id.obj().is_lake_shore();
    };

    const auto is_river_bank = [&]( const oter_id & id ) {
        return id != river_center && id.obj().is_river();
    };

    const bool n_lake  = is_lake( dat.north() );
    const bool e_lake  = is_lake( dat.east() );
    const bool s_lake  = is_lake( dat.south() );
    const bool w_lake  = is_lake( dat.west() );
    const bool nw_lake = is_lake( dat.nwest() );
    const bool ne_lake = is_lake( dat.neast() );
    const bool se_lake = is_lake( dat.seast() );
    const bool sw_lake = is_lake( dat.swest() );

    // If we don't have any adjacent lakes, then we don't need to worry about a shoreline,
    // and are done at this point.
    const bool no_adjacent_water = !n_lake && !e_lake && !s_lake && !w_lake && !nw_lake && !ne_lake &&
                                   !se_lake && !sw_lake;
    if( no_adjacent_water ) {
        return;
    }

    // I'm pretty unhappy with this block of if statements that follows, but got frustrated/sidetracked
    // in finding a more elegant solution. This is functional, but improvements that maintain the result
    // are welcome. The basic gist is as follows:
    //
    // Given our current location and the 8 adjacent locations, we classify them all as lake, lake shore,
    // river bank, or something else that we don't care about. We then create a polygon with four points,
    // one in each corner of our location. Then, based on the permutations of possible adjacent location
    // types, we manipulate the four points of our polygon to generate the rough outline of our shore. The
    // area inside the polygon will retain our ground we generated, while the area outside will get turned
    // into the shoreline with shallow and deep water.
    //
    // For example, if we have forests to the west, the lake to the east, and more shore north and south of
    // us, like this...
    //
    //     | --- | --- | --- |
    //     | F   | S   | L   |
    //     | --- | --- | --- |
    //     | F   | S   | L   |
    //     | --- | --- | --- |
    //     | F   | S   | L   |
    //     | --- | --- | --- |
    //
    // ...then what we want to do with our polygon is push our eastern points to the west. If the north location
    // were instead a continuation of the lake, with a commensurate shoreline like this...
    //
    //     | --- | --- | --- |
    //     | S   | L   | L   |
    //     | --- | --- | --- |
    //     | S   | S   | L   |
    //     | --- | --- | --- |
    //     | F   | S   | L   |
    //     | --- | --- | --- |
    //
    // ...then we still need to push our eastern points to the west, but we also need to push our northern
    // points south, and since we don't want such a blocky shoreline at our corners, we also want to
    // push the north-eastern point even further south-west.
    //
    // Things get even more complicated when we transition into a river bank--they have their own style of
    // mapgen, and while things don't have to be seamless, I did want the lake shores to fairly smoothly
    // transition into them, so if we have a river bank adjacent like this...
    //
    //     | --- | --- | --- |
    //     | F   | R   | L   |
    //     | --- | --- | --- |
    //     | F   | S   | L   |
    //     | --- | --- | --- |
    //     | F   | S   | L   |
    //     | --- | --- | --- |
    //
    // ...then we need to push our adjacent corners in even more for a good transition.
    //
    // At the end of all this, we'll have our basic four point polygon that we'll then inspect and use
    // to create the line-segments that will form our shoreline, but more on in a bit.

    const bool n_shore = is_shore( dat.north() );
    const bool e_shore = is_shore( dat.east() );
    const bool s_shore = is_shore( dat.south() );
    const bool w_shore = is_shore( dat.west() );

    const bool n_river_bank = is_river_bank( dat.north() );
    const bool e_river_bank = is_river_bank( dat.east() );
    const bool s_river_bank = is_river_bank( dat.south() );
    const bool w_river_bank = is_river_bank( dat.west() );

    // This is length we end up pushing things about by as a baseline.
    const int sector_length = SEEX * 2 / 3;

    // Define the corners of the map. These won't change.
    static constexpr point nw_corner{};
    static constexpr point ne_corner( SEEX * 2 - 1, 0 );
    static constexpr point se_corner( SEEX * 2 - 1, SEEY * 2 - 1 );
    static constexpr point sw_corner( 0, SEEY * 2 - 1 );

    // Define the four points that make up our polygon that we'll later pull line segments from for
    // the actual shoreline.
    point nw = nw_corner;
    point ne = ne_corner;
    point se = se_corner;
    point sw = sw_corner;

    std::vector<std::vector<point>> line_segments;

    // This section is about pushing the straight N, S, E, or W borders inward when adjacent to an actual lake.
    if( n_lake ) {
        nw.y += sector_length;
        ne.y += sector_length;
    }

    if( s_lake ) {
        sw.y -= sector_length;
        se.y -= sector_length;
    }

    if( w_lake ) {
        nw.x += sector_length;
        sw.x += sector_length;
    }

    if( e_lake ) {
        ne.x -= sector_length;
        se.x -= sector_length;
    }

    // This section is about pushing the corners inward when adjacent to a lake that curves into a river bank.
    if( n_river_bank ) {
        if( w_lake && nw_lake ) {
            nw.x += sector_length;
        }

        if( e_lake && ne_lake ) {
            ne.x -= sector_length;
        }
    }

    if( e_river_bank ) {
        if( s_lake && se_lake ) {
            se.y -= sector_length;
        }

        if( n_lake && ne_lake ) {
            ne.y += sector_length;
        }
    }

    if( s_river_bank ) {
        if( w_lake && sw_lake ) {
            sw.x += sector_length;
        }

        if( e_lake && se_lake ) {
            se.x -= sector_length;
        }
    }

    if( w_river_bank ) {
        if( s_lake && sw_lake ) {
            sw.y -= sector_length;
        }

        if( n_lake && nw_lake ) {
            nw.y += sector_length;
        }
    }

    // This section is about pushing the corners inward when we've got a lake in the corner that
    // either has lake adjacent to it and us, or more shore adjacent to it and us. Note that in the
    // case of having two shores adjacent, we end up adding a new line segment that isn't part of
    // our original set--we end up cutting the corner off our polygonal box.
    if( nw_lake ) {
        if( n_lake && w_lake ) {
            nw.x += sector_length / 2;
            nw.y += sector_length / 2;
        } else if( n_shore && w_shore ) {
            point n = nw_corner;
            point w = nw_corner;

            n.x += sector_length;
            w.y += sector_length;

            line_segments.push_back( { n, w } );
        }
    }

    if( ne_lake ) {
        if( n_lake && e_lake ) {
            ne.x -= sector_length / 2;
            ne.y += sector_length / 2;
        } else if( n_shore && e_shore ) {
            point n = ne_corner;
            point e = ne_corner;

            n.x -= sector_length;
            e.y += sector_length;

            line_segments.push_back( { n, e } );
        }
    }

    if( sw_lake ) {
        if( s_lake && w_lake ) {
            sw.x += sector_length / 2;
            sw.y -= sector_length / 2;
        } else if( s_shore && w_shore ) {
            point s = sw_corner;
            point w = sw_corner;

            s.x += sector_length;
            w.y -= sector_length;

            line_segments.push_back( { s, w } );
        }
    }

    if( se_lake ) {
        if( s_lake && e_lake ) {
            se.x -= sector_length / 2;
            se.y -= sector_length / 2;
        } else if( s_shore && e_shore ) {
            point s = se_corner;
            point e = se_corner;

            s.x -= sector_length;
            e.y -= sector_length;

            line_segments.push_back( { s, e } );
        }
    }

    // Ok, all of the fiddling with the polygon corners is done.
    // At this point we've got four points that make up four line segments that started out
    // at the map boundaries, but have subsequently been perturbed by the adjacent terrains.
    // Let's look at them and see which ones differ from their original state and should
    // form our shoreline.
    if( nw.y != nw_corner.y || ne.y != ne_corner.y ) {
        line_segments.push_back( { nw, ne } );
    }

    if( ne.x != ne_corner.x || se.x != se_corner.x ) {
        line_segments.push_back( { ne, se } );
    }

    if( se.y != se_corner.y || sw.y != sw_corner.y ) {
        line_segments.push_back( { se, sw } );
    }

    if( sw.x != sw_corner.x || nw.x != nw_corner.x ) {
        line_segments.push_back( { sw, nw } );
    }

    static constexpr inclusive_rectangle<point> map_boundaries( nw_corner, se_corner );

    // This will draw our shallow water coastline from the "from" point to the "to" point.
    // It buffers the points a bit for a thicker line. It also clears any furniture that might
    // be in the location as a result of our extending adjacent mapgen.
    const auto draw_shallow_water = [&]( const point & from, const point & to ) {
        std::vector<point> points = line_to( from, to );
        for( point &p : points ) {
            for( const point &bp : closest_points_first( p, 1 ) ) {
                if( !map_boundaries.contains( bp ) ) {
                    continue;
                }
                // Use t_null for now instead of t_water_sh, because sometimes our extended terrain
                // has put down a t_water_sh, and we need to be able to flood-fill over that.
                m->ter_set( bp, t_null );
                m->furn_set( bp, f_null );
            }
        }
    };

    // Given two points, return a point that is midway between the two points and then
    // jittered by a random amount in proportion to the length of the line segment.
    const auto jittered_midpoint = [&]( const point & from, const point & to ) {
        const int jitter = rl_dist( from, to ) / 4;
        const point midpoint( ( from.x + to.x ) / 2 + rng( -jitter, jitter ),
                              ( from.y + to.y ) / 2 + rng( -jitter, jitter ) );
        return midpoint;
    };

    // For each of our valid shoreline line segments, generate a slightly more interesting
    // set of line segments by splitting the line into four segments with jittered
    // midpoints, and then draw shallow water for four each of those.
    for( auto &ls : line_segments ) {
        const point mp1 = jittered_midpoint( ls[0], ls[1] );
        const point mp2 = jittered_midpoint( ls[0], mp1 );
        const point mp3 = jittered_midpoint( mp1, ls[1] );

        draw_shallow_water( ls[0], mp2 );
        draw_shallow_water( mp2, mp1 );
        draw_shallow_water( mp1, mp3 );
        draw_shallow_water( mp3, ls[1] );
    }

    // Now that we've done our ground mapgen and laid down a contiguous shoreline of shallow water,
    // we'll floodfill the sections adjacent to the lake with deep water. As before, we also clear
    // out any furniture that we placed by the extended mapgen.
    std::unordered_set<point> visited;

    const auto should_fill = [&]( const point & p ) {
        if( !map_boundaries.contains( p ) ) {
            return false;
        }
        return m->ter( p ) != t_null;
    };

    const auto fill_deep_water = [&]( const point & starting_point ) {
        std::vector<point> water_points = ff::point_flood_fill_4_connected( starting_point, visited,
                                          should_fill );
        for( point &wp : water_points ) {
            m->ter_set( wp, t_water_dp );
            m->furn_set( wp, f_null );
        }
    };

    // We'll flood fill from the four corners, using the corner if any of the locations
    // adjacent to it were a lake.
    if( n_lake || nw_lake || w_lake ) {
        fill_deep_water( nw_corner );
    }

    if( s_lake || sw_lake || w_lake ) {
        fill_deep_water( sw_corner );
    }

    if( n_lake || ne_lake || e_lake ) {
        fill_deep_water( ne_corner );
    }

    if( s_lake || se_lake || e_lake ) {
        fill_deep_water( se_corner );
    }

    // We previously placed our shallow water but actually did a t_null instead to make sure that we didn't
    // pick up shallow water from our extended terrain. Now turn those nulls into t_water_sh.
    m->translate( t_null, t_water_sh );
}

void mapgen_ravine_edge( mapgendata &dat )
{
    map *const m = &dat.m;
    // A solid chunk of z layer appropriate wall or floor is first generated to carve the cliffside off from
    if( dat.zlevel() == 0 ) {
        dat.fill_groundcover();
    } else {
        run_mapgen_func( dat.region.default_oter[ OVERMAP_DEPTH + dat.zlevel() ].id()->get_mapgen_id(),
                         dat );
    }

    const auto is_ravine = [&]( const oter_id & id ) {
        return id.obj().is_ravine();
    };

    const auto is_ravine_edge = [&]( const oter_id & id ) {
        return id.obj().is_ravine_edge();
    };
    // Since this terrain is directionless, we look at its immediate neighbors to determine whether a straight
    // or curved ravine edge should be generated. And to then apply the correct rotation.
    const bool n_ravine  = is_ravine( dat.north() );
    const bool e_ravine  = is_ravine( dat.east() );
    const bool s_ravine  = is_ravine( dat.south() );
    const bool w_ravine  = is_ravine( dat.west() );
    const bool nw_ravine = is_ravine( dat.nwest() );
    const bool ne_ravine = is_ravine( dat.neast() );
    const bool se_ravine = is_ravine( dat.seast() );

    const bool n_ravine_edge = is_ravine_edge( dat.north() );
    const bool e_ravine_edge = is_ravine_edge( dat.east() );
    const bool s_ravine_edge = is_ravine_edge( dat.south() );
    const bool w_ravine_edge = is_ravine_edge( dat.west() );

    const auto any_orthogonal_ravine = [&]() {
        return n_ravine || s_ravine || w_ravine || e_ravine;
    };

    const bool straight = ( ( n_ravine_edge && s_ravine_edge ) || ( e_ravine_edge &&
                            w_ravine_edge ) ) && any_orthogonal_ravine();
    const bool interior_corner = !straight && !any_orthogonal_ravine();
    const bool exterior_corner = !straight && any_orthogonal_ravine();

    //With that done, we generate the maps.
    if( straight ) {
        for( int x = 0; x < SEEX * 2; x++ ) {
            int ground_edge = 12 + rng( 1, 3 );
            line( m, t_null, point( x, ++ground_edge ), point( x, SEEY * 2 ) );
        }
        if( w_ravine ) {
            m->rotate( 1 );
        }
        if( n_ravine ) {
            m->rotate( 2 );
        }
        if( e_ravine ) {
            m->rotate( 3 );
        }
    } else if( interior_corner ) {
        for( int x = 0; x < SEEX * 2; x++ ) {
            int ground_edge = 12 + rng( 1, 3 ) + x;
            line( m, t_null, point( x, ++ground_edge ), point( x, SEEY * 2 ) );
        }
        if( nw_ravine ) {
            m->rotate( 1 );
        }
        if( ne_ravine ) {
            m->rotate( 2 );
        }
        if( se_ravine ) {
            m->rotate( 3 );
        }
    } else if( exterior_corner ) {
        for( int x = 0; x < SEEX * 2; x++ ) {
            int ground_edge =  12  + rng( 1, 3 ) - x;
            line( m, t_null, point( x, --ground_edge ), point( x, SEEY * 2 - 1 ) );
        }
        if( w_ravine && s_ravine ) {
            m->rotate( 1 );
        }
        if( w_ravine && n_ravine ) {
            m->rotate( 2 );
        }
        if( e_ravine && n_ravine ) {
            m->rotate( 3 );
        }
    }
    // The placed t_null terrains are converted into the regional groundcover in the ravine's bottom level,
    // in the other levels they are converted into open air to generate the cliffside.
    if( dat.zlevel() == dat.region.overmap_ravine.ravine_depth ) {
        m->translate( t_null, dat.groundcover() );
    } else {
        m->translate( t_null, t_open_air );
    }
}

void mremove_trap( map *m, const point &p, trap_id type )
{
    tripoint actual_location( p, m->get_abs_sub().z() );
    const trap_id trap_at_loc = m->maptile_at( actual_location ).get_trap().id();
    if( type == tr_null || trap_at_loc == type ) {
        m->remove_trap( actual_location );
    }
}

void mtrap_set( map *m, const point &p, trap_id type, bool avoid_creatures )
{
    if( avoid_creatures ) {
        Creature *c = get_creature_tracker().creature_at( tripoint_abs_ms( m->getabs( tripoint( p,
                      m->get_abs_sub().z() ) ) ), true );
        if( c ) {
            return;
        }
    }
    tripoint actual_location( p, m->get_abs_sub().z() );
    m->trap_set( actual_location, type );
}

void madd_field( map *m, const point &p, field_type_id type, int intensity )
{
    tripoint actual_location( p, m->get_abs_sub().z() );
    m->add_field( actual_location, type, intensity, 0_turns );
}

void mremove_fields( map *m, const point &p )
{
    tripoint actual_location( p, m->get_abs_sub().z() );
    m->clear_fields( actual_location );
}

void resolve_regional_terrain_and_furniture( const mapgendata &dat )
{
    for( const tripoint &p : dat.m.points_on_zlevel() ) {
        const ter_id tid_before = dat.m.ter( p );
        const ter_id tid_after = dat.region.region_terrain_and_furniture.resolve( tid_before );
        if( tid_after != tid_before ) {
            dat.m.ter_set( p, tid_after );
        }
        const furn_id fid_before = dat.m.furn( p );
        const furn_id fid_after = dat.region.region_terrain_and_furniture.resolve( fid_before );
        if( fid_after != fid_before ) {
            dat.m.furn_set( p, fid_after );
        }
    }
}
