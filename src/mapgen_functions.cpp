#include "mapgen_functions.h"

#include <cstdlib>
#include <algorithm>
#include <array>
#include <iterator>
#include <initializer_list>
#include <map>
#include <ostream>
#include <queue>
#include <unordered_set>
#include <utility>
#include <vector>

#include "character_id.h"
#include "debug.h"
#include "field.h"
#include "flood_fill.h"
#include "item.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "mapgen.h"
#include "mapgendata.h"
#include "mapgenformat.h"
#include "omdata.h"
#include "options.h"
#include "overmap.h"
#include "trap.h"
#include "vehicle_group.h"
#include "calendar.h"
#include "game_constants.h"
#include "regional_settings.h"
#include "rng.h"
#include "string_id.h"
#include "int_id.h"
#include "enums.h"

class npc_template;

#define dbg(x) DebugLog((x),D_MAP_GEN) << __FILE__ << ":" << __LINE__ << ": "

const mtype_id mon_ant_larva( "mon_ant_larva" );
const mtype_id mon_ant_queen( "mon_ant_queen" );
const mtype_id mon_bat( "mon_bat" );
const mtype_id mon_bee( "mon_bee" );
const mtype_id mon_beekeeper( "mon_beekeeper" );
const mtype_id mon_fungaloid_queen( "mon_fungaloid_queen" );
const mtype_id mon_fungaloid_seeder( "mon_fungaloid_seeder" );
const mtype_id mon_fungaloid_tower( "mon_fungaloid_tower" );
const mtype_id mon_rat_king( "mon_rat_king" );
const mtype_id mon_sewer_rat( "mon_sewer_rat" );
const mtype_id mon_spider_widow_giant( "mon_spider_widow_giant" );
const mtype_id mon_spider_cellar_giant( "mon_spider_cellar_giant" );
const mtype_id mon_zombie_jackson( "mon_zombie_jackson" );

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
            { "crater",           &mapgen_crater },
            { "field",            &mapgen_field },
            { "forest",           &mapgen_forest },
            { "forest_trail_straight",    &mapgen_forest_trail_straight },
            { "forest_trail_curved",      &mapgen_forest_trail_curved },
            // TODO: Add a dedicated dead-end function. For now it copies the straight section above.
            { "forest_trail_end",         &mapgen_forest_trail_straight },
            { "forest_trail_tee",         &mapgen_forest_trail_tee },
            { "forest_trail_four_way",    &mapgen_forest_trail_four_way },
            { "hive",             &mapgen_hive },
            { "spider_pit",       &mapgen_spider_pit },
            { "fungal_bloom",     &mapgen_fungal_bloom },
            { "fungal_tower",     &mapgen_fungal_tower },
            { "fungal_flowers",   &mapgen_fungal_flowers },
            { "road_straight",    &mapgen_road },
            { "road_curved",      &mapgen_road },
            { "road_end",         &mapgen_road },
            { "road_tee",         &mapgen_road },
            { "road_four_way",    &mapgen_road },
            { "field",            &mapgen_field },
            { "bridge",           &mapgen_bridge },
            { "highway",          &mapgen_highway },
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
            { "parking_lot",      &mapgen_parking_lot },
            { "house_generic_boxy",      &mapgen_generic_house_boxy },
            { "house_generic_big_livingroom",      &mapgen_generic_house_big_livingroom },
            { "house_generic_center_hallway",      &mapgen_generic_house_center_hallway },
            { "spider_pit", mapgen_spider_pit },
            { "basement_generic_layout", &mapgen_basement_generic_layout }, // empty, not bound
            { "basement_junk", &mapgen_basement_junk },
            { "basement_spiders", &mapgen_basement_spiders },
            { "cave", &mapgen_cave },
            { "cave_rat", &mapgen_cave_rat },
            { "cavern", &mapgen_cavern },
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

            { "sewer_straight",    &mapgen_sewer_straight },
            { "sewer_curved",      &mapgen_sewer_curved },
            // TODO: Add a dedicated dead-end function. For now it copies the straight section above.
            { "sewer_end",         &mapgen_sewer_straight },
            { "sewer_tee",         &mapgen_sewer_tee },
            { "sewer_four_way",    &mapgen_sewer_four_way },

            { "ants_straight",    &mapgen_ants_straight },
            { "ants_curved",      &mapgen_ants_curved },
            // TODO: Add a dedicated dead-end function. For now it copies the straight section above.
            { "ants_end",         &mapgen_ants_straight },
            { "ants_tee",         &mapgen_ants_tee },
            { "ants_four_way",    &mapgen_ants_four_way },
            { "ants_food", &mapgen_ants_food },
            { "ants_larvae", &mapgen_ants_larvae },
            { "ants_queen", &mapgen_ants_queen },
            { "tutorial", &mapgen_tutorial },
            { "lake_shore", &mapgen_lake_shore },
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

void mapgen_rotate( map *m, oter_id terrain_type, bool north_is_down )
{
    const auto dir = terrain_type->get_dir();
    m->rotate( static_cast<int>( north_is_down ? om_direction::opposite( dir ) : dir ) );
}

#define autorotate(x) mapgen_rotate(m, terrain_type, x)
#define autorotate_down() mapgen_rotate(m, terrain_type, true)

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

void mapgen_crater( mapgendata &dat )
{
    map *const m = &dat.m;
    for( int i = 0; i < 4; i++ ) {
        if( dat.t_nesw[i] != "crater" ) {
            dat.set_dir( i, 6 );
        }
    }

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( rng( 0, dat.w_fac ) <= i && rng( 0, dat.e_fac ) <= SEEX * 2 - 1 - i &&
                rng( 0, dat.n_fac ) <= j && rng( 0, dat.s_fac ) <= SEEX * 2 - 1 - j ) {
                m->ter_set( point( i, j ), t_dirt );
                m->make_rubble( tripoint( i,  j, m->get_abs_sub().z ), f_rubble_rock, true );
                m->set_radiation( point( i, j ), rng( 0, 4 ) * rng( 0, 2 ) );
            } else {
                m->ter_set( point( i, j ), dat.groundcover() );
                m->set_radiation( point( i, j ), rng( 0, 2 ) * rng( 0, 2 ) * rng( 0, 2 ) );
            }
        }
    }
    m->place_items( "wreckage", 83, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true, dat.when() );
}

// TODO: make void map::ter_or_furn_set(const int x, const int y, const ter_furn_id & tfid);
static void ter_or_furn_set( map *m, const int x, const int y, const ter_furn_id &tfid )
{
    if( tfid.ter != t_null ) {
        m->ter_set( point( x, y ), tfid.ter );
    } else if( tfid.furn != f_null ) {
        m->furn_set( point( x, y ), tfid.furn );
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
    const bool boosted_vegetation = ( dat.region.field_coverage.boost_chance > rng( 0, 1000000 ) );
    const int &mpercent_bush = ( boosted_vegetation ?
                                 dat.region.field_coverage.boosted_mpercent_coverage :
                                 dat.region.field_coverage.mpercent_coverage
                               );

    ter_furn_id altbush = dat.region.field_coverage.pick(
                              true ); // one dominant plant type ( for boosted_vegetation == true )

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            m->ter_set( point( i, j ), dat.groundcover() ); // default is
            if( mpercent_bush > rng( 0, 1000000 ) ) { // yay, a shrub ( or tombstone )
                if( boosted_vegetation && dat.region.field_coverage.boosted_other_mpercent > rng( 0, 1000000 ) ) {
                    // already chose the lucky terrain/furniture/plant/rock/etc
                    ter_or_furn_set( m, i, j, altbush );
                } else {
                    // pick from weighted list
                    ter_or_furn_set( m, i, j, dat.region.field_coverage.pick( false ) );
                }
            }
        }
    }

    m->place_items( "field", 60, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true,
                    dat.when() ); // FIXME: take 'rock' out and add as regional biome setting
}

void mapgen_hive( mapgendata &dat )
{
    map *const m = &dat.m;
    // Start with a basic forest pattern
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            int rn = rng( 0, 14 );
            if( rn > 13 ) {
                m->ter_set( point( i, j ), t_tree );
            } else if( rn > 11 ) {
                m->ter_set( point( i, j ), t_tree_young );
            } else if( rn > 10 ) {
                m->ter_set( point( i, j ), t_underbrush );
            } else {
                m->ter_set( point( i, j ), dat.groundcover() );
            }
        }
    }

    // j and i loop through appropriate hive-cell center squares
    const bool is_center = dat.t_nesw[0] == "hive" && dat.t_nesw[1] == "hive" &&
                           dat.t_nesw[2] == "hive" && dat.t_nesw[3] == "hive";
    for( int j = 5; j < SEEY * 2 - 5; j += 6 ) {
        for( int i = ( j == 5 || j == 17 ? 3 : 6 ); i < SEEX * 2 - 5; i += 6 ) {
            if( !one_in( 8 ) ) {
                // Caps are always there
                m->ter_set( point( i, j - 5 ), t_wax );
                m->ter_set( point( i, j + 5 ), t_wax );
                for( int k = -2; k <= 2; k++ ) {
                    for( int l = -1; l <= 1; l++ ) {
                        m->ter_set( point( i + k, j + l ), t_floor_wax );
                    }
                }
                m->add_spawn( mon_bee, 2, point( i, j ) );
                m->add_spawn( mon_beekeeper, 1, point( i, j ) );
                m->ter_set( point( i, j - 3 ), t_floor_wax );
                m->ter_set( point( i, j + 3 ), t_floor_wax );
                m->ter_set( point( i - 1, j - 2 ), t_floor_wax );
                m->ter_set( point( i, j - 2 ), t_floor_wax );
                m->ter_set( point( i + 1, j - 2 ), t_floor_wax );
                m->ter_set( point( i - 1, j + 2 ), t_floor_wax );
                m->ter_set( point( i, j + 2 ), t_floor_wax );
                m->ter_set( point( i + 1, j + 2 ), t_floor_wax );

                // Up to two of these get skipped; an entrance to the cell
                int skip1 = rng( 0, SEEX * 2 - 1 );
                int skip2 = rng( 0, SEEY * 2 - 1 );

                m->ter_set( point( i - 1, j - 4 ), t_wax );
                m->ter_set( point( i, j - 4 ), t_wax );
                m->ter_set( point( i + 1, j - 4 ), t_wax );
                m->ter_set( point( i - 2, j - 3 ), t_wax );
                m->ter_set( point( i - 1, j - 3 ), t_wax );
                m->ter_set( point( i + 1, j - 3 ), t_wax );
                m->ter_set( point( i + 2, j - 3 ), t_wax );
                m->ter_set( point( i - 3, j - 2 ), t_wax );
                m->ter_set( point( i - 2, j - 2 ), t_wax );
                m->ter_set( point( i + 2, j - 2 ), t_wax );
                m->ter_set( point( i + 3, j - 2 ), t_wax );
                m->ter_set( point( i - 3, j - 1 ), t_wax );
                m->ter_set( point( i - 3, j ), t_wax );
                m->ter_set( point( i - 3, j - 1 ), t_wax );
                m->ter_set( point( i - 3, j + 1 ), t_wax );
                m->ter_set( point( i - 3, j ), t_wax );
                m->ter_set( point( i - 3, j + 1 ), t_wax );
                m->ter_set( point( i - 2, j + 3 ), t_wax );
                m->ter_set( point( i - 1, j + 3 ), t_wax );
                m->ter_set( point( i + 1, j + 3 ), t_wax );
                m->ter_set( point( i + 2, j + 3 ), t_wax );
                m->ter_set( point( i - 1, j + 4 ), t_wax );
                m->ter_set( point( i, j + 4 ), t_wax );
                m->ter_set( point( i + 1, j + 4 ), t_wax );

                if( skip1 == 0 || skip2 == 0 ) {
                    m->ter_set( point( i - 1, j - 4 ), t_floor_wax );
                }
                if( skip1 == 1 || skip2 == 1 ) {
                    m->ter_set( point( i, j - 4 ), t_floor_wax );
                }
                if( skip1 == 2 || skip2 == 2 ) {
                    m->ter_set( point( i + 1, j - 4 ), t_floor_wax );
                }
                if( skip1 == 3 || skip2 == 3 ) {
                    m->ter_set( point( i - 2, j - 3 ), t_floor_wax );
                }
                if( skip1 == 4 || skip2 == 4 ) {
                    m->ter_set( point( i - 1, j - 3 ), t_floor_wax );
                }
                if( skip1 == 5 || skip2 == 5 ) {
                    m->ter_set( point( i + 1, j - 3 ), t_floor_wax );
                }
                if( skip1 == 6 || skip2 == 6 ) {
                    m->ter_set( point( i + 2, j - 3 ), t_floor_wax );
                }
                if( skip1 == 7 || skip2 == 7 ) {
                    m->ter_set( point( i - 3, j - 2 ), t_floor_wax );
                }
                if( skip1 == 8 || skip2 == 8 ) {
                    m->ter_set( point( i - 2, j - 2 ), t_floor_wax );
                }
                if( skip1 == 9 || skip2 == 9 ) {
                    m->ter_set( point( i + 2, j - 2 ), t_floor_wax );
                }
                if( skip1 == 10 || skip2 == 10 ) {
                    m->ter_set( point( i + 3, j - 2 ), t_floor_wax );
                }
                if( skip1 == 11 || skip2 == 11 ) {
                    m->ter_set( point( i - 3, j - 1 ), t_floor_wax );
                }
                if( skip1 == 12 || skip2 == 12 ) {
                    m->ter_set( point( i - 3, j ), t_floor_wax );
                }
                if( skip1 == 13 || skip2 == 13 ) {
                    m->ter_set( point( i - 3, j - 1 ), t_floor_wax );
                }
                if( skip1 == 14 || skip2 == 14 ) {
                    m->ter_set( point( i - 3, j + 1 ), t_floor_wax );
                }
                if( skip1 == 15 || skip2 == 15 ) {
                    m->ter_set( point( i - 3, j ), t_floor_wax );
                }
                if( skip1 == 16 || skip2 == 16 ) {
                    m->ter_set( point( i - 3, j + 1 ), t_floor_wax );
                }
                if( skip1 == 17 || skip2 == 17 ) {
                    m->ter_set( point( i - 2, j + 3 ), t_floor_wax );
                }
                if( skip1 == 18 || skip2 == 18 ) {
                    m->ter_set( point( i - 1, j + 3 ), t_floor_wax );
                }
                if( skip1 == 19 || skip2 == 19 ) {
                    m->ter_set( point( i + 1, j + 3 ), t_floor_wax );
                }
                if( skip1 == 20 || skip2 == 20 ) {
                    m->ter_set( point( i + 2, j + 3 ), t_floor_wax );
                }
                if( skip1 == 21 || skip2 == 21 ) {
                    m->ter_set( point( i - 1, j + 4 ), t_floor_wax );
                }
                if( skip1 == 22 || skip2 == 22 ) {
                    m->ter_set( point( i, j + 4 ), t_floor_wax );
                }
                if( skip1 == 23 || skip2 == 23 ) {
                    m->ter_set( point( i + 1, j + 4 ), t_floor_wax );
                }

                if( is_center ) {
                    m->place_items( "hive_center", 90, point( i - 2, j - 2 ), point( i + 2, j + 2 ), false,
                                    dat.when() );
                } else {
                    m->place_items( "hive", 80, point( i - 2, j - 2 ), point( i + 2, j + 2 ), false, dat.when() );
                }
            }
        }
    }

    if( is_center ) {
        m->place_npc( point( SEEX, SEEY ), string_id<npc_template>( "apis" ) );
    }
}

void mapgen_spider_pit( mapgendata &dat )
{
    map *const m = &dat.m;
    // First generate a forest
    dat.fill( 4 );
    for( int i = 0; i < 4; i++ ) {
        if( dat.t_nesw[i] == "forest" || dat.t_nesw[i] == "forest_water" ) {
            dat.dir( i ) += 14;
        } else if( dat.t_nesw[i] == "forest_thick" ) {
            dat.dir( i ) += 18;
        }
    }
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            int forest_chance = 0;
            int num = 0;
            if( j < dat.n_fac ) {
                forest_chance += dat.n_fac - j;
                num++;
            }
            if( SEEX * 2 - 1 - i < dat.e_fac ) {
                forest_chance += dat.e_fac - ( SEEX * 2 - 1 - i );
                num++;
            }
            if( SEEY * 2 - 1 - j < dat.s_fac ) {
                forest_chance += dat.s_fac - ( SEEX * 2 - 1 - j );
                num++;
            }
            if( i < dat.w_fac ) {
                forest_chance += dat.w_fac - i;
                num++;
            }
            if( num > 0 ) {
                forest_chance /= num;
            }
            int rn = rng( 0, forest_chance );
            if( ( forest_chance > 0 && rn > 13 ) || one_in( 100 - forest_chance ) ) {
                m->ter_set( point( i, j ), t_tree );
            } else if( ( forest_chance > 0 && rn > 10 ) || one_in( 100 - forest_chance ) ) {
                m->ter_set( point( i, j ), t_tree_young );
            } else if( ( forest_chance > 0 && rn >  9 ) || one_in( 100 - forest_chance ) ) {
                m->ter_set( point( i, j ), t_underbrush );
            } else {
                m->ter_set( point( i, j ), dat.groundcover() );
            }
        }
    }
    m->place_items( "forest", 60, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true, dat.when() );
    // Next, place webs and sinkholes
    for( int i = 0; i < 4; i++ ) {
        int x = rng( 3, SEEX * 2 - 4 ), y = rng( 3, SEEY * 2 - 4 );
        if( i == 0 ) {
            m->ter_set( point( x, y ), t_slope_down );
        } else {
            m->ter_set( point( x, y ), dat.groundcover() );
            mtrap_set( m, x, y, tr_sinkhole );
        }
        for( int x1 = x - 3; x1 <= x + 3; x1++ ) {
            for( int y1 = y - 3; y1 <= y + 3; y1++ ) {
                madd_field( m, x1, y1, fd_web, rng( 2, 3 ) );
                if( m->ter( point( x1, y1 ) ) != t_slope_down ) {
                    m->ter_set( point( x1, y1 ), t_dirt );
                }
            }
        }
    }
}

void mapgen_fungal_bloom( mapgendata &dat )
{
    map *const m = &dat.m;
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( one_in( rl_dist( point( i, j ), point( 12, 12 ) ) * 4 ) ) {
                m->ter_set( point( i, j ), t_marloss );
            } else if( one_in( 10 ) ) {
                if( one_in( 3 ) ) {
                    m->ter_set( point( i, j ), t_tree_fungal );
                } else {
                    m->ter_set( point( i, j ), t_tree_fungal_young );
                }

            } else if( one_in( 5 ) ) {
                m->ter_set( point( i, j ), t_shrub_fungal );
            } else if( one_in( 10 ) ) {
                m->ter_set( point( i, j ), t_fungus_mound );
            } else {
                m->ter_set( point( i, j ), t_fungus );
            }
        }
    }
    square( m, t_fungus, SEEX - 2, SEEY - 2, SEEX + 2, SEEY + 2 );
    m->add_spawn( mon_fungaloid_queen, 1, point( 12, 12 ) );
}

void mapgen_fungal_tower( mapgendata &dat )
{
    map *const m = &dat.m;
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( one_in( 8 ) ) {
                if( one_in( 3 ) ) {
                    m->ter_set( point( i, j ), t_tree_fungal );
                } else {
                    m->ter_set( point( i, j ), t_tree_fungal_young );
                }

            } else if( one_in( 10 ) ) {
                m->ter_set( point( i, j ), t_fungus_mound );
            } else {
                m->ter_set( point( i, j ), t_fungus );
            }
        }
    }
    square( m, t_fungus, SEEX - 2, SEEY - 2, SEEX + 2, SEEY + 2 );
    m->add_spawn( mon_fungaloid_tower, 1, point( 12, 12 ) );
}

void mapgen_fungal_flowers( mapgendata &dat )
{
    map *const m = &dat.m;
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( one_in( rl_dist( point( i, j ), point( 12, 12 ) ) * 6 ) ) {
                m->ter_set( point( i, j ), t_fungus );
                m->furn_set( point( i, j ), f_flower_marloss );
            } else if( one_in( 10 ) ) {
                if( one_in( 3 ) ) {
                    m->ter_set( point( i, j ), t_fungus_mound );
                } else {
                    m->ter_set( point( i, j ), t_tree_fungal_young );
                }

            } else if( one_in( 5 ) ) {
                m->ter_set( point( i, j ), t_fungus );
                m->furn_set( point( i, j ), f_flower_fungal );
            } else if( one_in( 10 ) ) {
                m->ter_set( point( i, j ), t_shrub_fungal );
            } else {
                m->ter_set( point( i, j ), t_fungus );
            }
        }
    }
    square( m, t_fungus, SEEX - 2, SEEY - 2, SEEX + 2, SEEY + 2 );
    m->add_spawn( mon_fungaloid_seeder, 1, point( 12, 12 ) );
}

int terrain_type_to_nesw_array( oter_id terrain_type, bool array[4] )
{
    // count and mark which directions the road goes
    const auto &oter( *terrain_type );
    int num_dirs = 0;
    for( const auto dir : om_direction::all ) {
        num_dirs += ( array[static_cast<int>( dir )] = oter.has_connection( dir ) );
    }
    return num_dirs;
}

// perform dist counterclockwise rotations on a nesw or neswx array
template<typename T>
void nesw_array_rotate( T *array, size_t len, size_t dist )
{
    if( len == 4 ) {
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

// take x/y coordinates in a map and rotate them counterclockwise around the center
static void coord_rotate_cw( int &x, int &y, int rot )
{
    for( ; rot--; ) {
        int temp = y;
        y = x;
        x = ( SEEY * 2 - 1 ) - temp;
    }
}

static bool compare_neswx( bool *a1, std::initializer_list<int> a2 )
{
    return std::equal( std::begin( a2 ), std::end( a2 ), a1,
    []( int a, bool b ) {
        return static_cast<bool>( a ) == b;
    } );
}

// mapgen_road replaces previous mapgen_road_straight _end _curved _tee _four_way
void mapgen_road( mapgendata &dat )
{
    map *const m = &dat.m;
    // start by filling the whole map with grass/dirt/etc
    dat.fill_groundcover();

    // which and how many neighbors have sidewalks?
    bool sidewalks_neswx[8] = {};
    int neighbor_sidewalks = 0;
    for( int dir = 0; dir < 8; dir++ ) { // N E S W NE SE SW NW
        sidewalks_neswx[dir] = dat.t_nesw[dir]->has_flag( has_sidewalk );
        neighbor_sidewalks += sidewalks_neswx[dir];
    }

    // which of the cardinal directions get roads?
    bool roads_nesw[4] = {};
    int num_dirs = terrain_type_to_nesw_array( dat.terrain_type(), roads_nesw );
    // if this is a dead end, extend past the middle of the tile
    int dead_end_extension = ( num_dirs == 1 ? 8 : 0 );

    // which way should our roads curve, based on neighbor roads?
    int curvedir_nesw[4] = {};
    for( int dir = 0; dir < 4; dir++ ) { // N E S W
        if( !roads_nesw[dir] || dat.t_nesw[dir]->get_type_id().str() != "road" ) {
            continue;
        }

        // n_* contain details about the neighbor being considered
        bool n_roads_nesw[4] = {};
        // TODO: figure out how to call this function without creating a new oter_id object
        int n_num_dirs = terrain_type_to_nesw_array( dat.t_nesw[dir], n_roads_nesw );
        // if 2-way neighbor has a road facing us
        if( n_num_dirs == 2 && n_roads_nesw[( dir + 2 ) % 4] ) {
            // curve towards the direction the neighbor turns
            if( n_roads_nesw[( dir - 1 + 4 ) % 4] ) {
                curvedir_nesw[dir]--;    // our road curves counterclockwise
            }
            if( n_roads_nesw[( dir + 1 ) % 4] ) {
                curvedir_nesw[dir]++;    // our road curves clockwise
            }
        }
    }

    // calculate how far to rotate the map so we can work with just one orientation
    // also keep track of diagonal roads and plazas
    int rot = 0;
    bool diag = false;
    int plaza_dir = -1;
    bool fourways_neswx[8] = {};
    // TODO: reduce amount of logical/conditional constructs here
    // TODO: make plazas include adjacent tees
    switch( num_dirs ) {
        case 4: // 4-way intersection
            for( int dir = 0; dir < 8; dir++ ) {
                fourways_neswx[dir] = ( dat.t_nesw[dir].id() == "road_nesw" ||
                                        dat.t_nesw[dir].id() == "road_nesw_manhole" );
            }
            // is this the middle, or which side or corner, of a plaza?
            plaza_dir = compare_neswx( fourways_neswx, {1, 1, 1, 1, 1, 1, 1, 1} ) ? 8 :
                        compare_neswx( fourways_neswx, {0, 1, 1, 0, 0, 1, 0, 0} ) ? 7 :
                        compare_neswx( fourways_neswx, {1, 1, 0, 0, 1, 0, 0, 0} ) ? 6 :
                        compare_neswx( fourways_neswx, {1, 0, 0, 1, 0, 0, 0, 1} ) ? 5 :
                        compare_neswx( fourways_neswx, {0, 0, 1, 1, 0, 0, 1, 0} ) ? 4 :
                        compare_neswx( fourways_neswx, {1, 1, 1, 0, 1, 1, 0, 0} ) ? 3 :
                        compare_neswx( fourways_neswx, {1, 1, 0, 1, 1, 0, 0, 1} ) ? 2 :
                        compare_neswx( fourways_neswx, {1, 0, 1, 1, 0, 0, 1, 1} ) ? 1 :
                        compare_neswx( fourways_neswx, {0, 1, 1, 1, 0, 1, 1, 0} ) ? 0 :
                        -1;
            if( plaza_dir > -1 ) {
                rot = plaza_dir % 4;
            }
            break;
        case 3: // tee
            if( !roads_nesw[0] ) {
                rot = 2;    // E/S/W, rotate 180 degrees
                break;
            }
            if( !roads_nesw[1] ) {
                rot = 3;    // N/S/W, rotate 270 degrees
                break;
            }
            if( !roads_nesw[3] ) {
                rot = 1;    // N/E/S, rotate  90 degrees
                break;
            }
            break;                                  // N/E/W, don't rotate
        case 2: // straight or diagonal
            if( roads_nesw[1] && roads_nesw[3] ) {
                rot = 1;    // E/W, rotate  90 degrees
                break;
            }
            if( roads_nesw[1] && roads_nesw[2] ) {
                rot = 1;    // E/S, rotate  90 degrees
                diag = true;
                break;
            }
            if( roads_nesw[2] && roads_nesw[3] ) {
                rot = 2;    // S/W, rotate 180 degrees
                diag = true;
                break;
            }
            if( roads_nesw[3] && roads_nesw[0] ) {
                rot = 3;    // W/N, rotate 270 degrees
                diag = true;
                break;
            }
            if( roads_nesw[0] && roads_nesw[1] ) {
                diag = true;    // N/E, don't rotate
                break;
            }
            break;                                                               // N/S, don't rotate
        case 1: // dead end
            if( roads_nesw[1] ) {
                rot = 1;    // E, rotate  90 degrees
                break;
            }
            if( roads_nesw[2] ) {
                rot = 2;    // S, rotate 180 degrees
                break;
            }
            if( roads_nesw[3] ) {
                rot = 3;    // W, rotate 270 degrees
                break;
            }
            break;                               // N, don't rotate
    }

    // rotate the arrays left by rot steps
    nesw_array_rotate<bool>( sidewalks_neswx, 8, rot * 2 );
    nesw_array_rotate<bool>( roads_nesw,      4, rot );
    nesw_array_rotate<int> ( curvedir_nesw,   4, rot );

    // now we have only these shapes: '   |   '-   -'-   -|-

    if( diag ) { // diagonal roads get drawn differently from all other types
        // draw sidewalks if a S/SW/W neighbor has_sidewalk
        if( sidewalks_neswx[4] || sidewalks_neswx[5] || sidewalks_neswx[6] ) {
            for( int y = 0; y < SEEY * 2; y++ ) {
                for( int x = 0; x < SEEX * 2; x++ ) {
                    if( x > y - 4 && ( x < 4 || y > SEEY * 2 - 5 || y >= x ) ) {
                        m->ter_set( point( x, y ), t_sidewalk );
                    }
                }
            }
        }
        // draw diagonal road
        for( int y = 0; y < SEEY * 2; y++ ) {
            for( int x = 0; x < SEEX * 2; x++ ) {
                if( x > y && // definitely only draw in the upper right half of the map
                    ( ( x > 3 && y < ( SEEY * 2 - 4 ) ) || // middle, for both corners and diagonals
                      ( x < 4 && curvedir_nesw[0] < 0 ) || // diagonal heading northwest
                      ( y > ( SEEY * 2 - 5 ) && curvedir_nesw[1] > 0 ) ) ) { // diagonal heading southeast
                    if( ( x + rot / 2 ) % 4 && ( x - y == SEEX - 1 + ( 1 - ( rot / 2 ) ) ||
                                                 x - y == SEEX + ( 1 - ( rot / 2 ) ) ) ) {
                        m->ter_set( point( x, y ), t_pavement_y );
                    } else {
                        m->ter_set( point( x, y ), t_pavement );
                    }
                }
            }
        }
    } else { // normal road drawing
        bool cul_de_sac = false;
        // dead ends become cul de sacs, 1/3 of the time, if a neighbor has_sidewalk
        if( num_dirs == 1 && one_in( 3 ) && neighbor_sidewalks ) {
            cul_de_sac = true;
            fill_background( m, t_sidewalk );
        }

        // draw normal sidewalks
        for( int dir = 0; dir < 4; dir++ ) {
            if( roads_nesw[dir] ) {
                // sidewalk west of north road, etc
                if( sidewalks_neswx[( dir + 3 ) % 4     ] ||   // has_sidewalk west?
                    sidewalks_neswx[( dir + 3 ) % 4 + 4 ] ||   // has_sidewalk northwest?
                    sidewalks_neswx[   dir               ] ) { // has_sidewalk north?
                    int x1 = 0;
                    int y1 = 0;
                    int x2 = 3;
                    int y2 = SEEY - 1 + dead_end_extension;
                    coord_rotate_cw( x1, y1, dir );
                    coord_rotate_cw( x2, y2, dir );
                    square( m, t_sidewalk, x1, y1, x2, y2 );
                }
                // sidewalk east of north road, etc
                if( sidewalks_neswx[( dir + 1 ) % 4 ] ||   // has_sidewalk east?
                    sidewalks_neswx[   dir + 4       ] ||  // has_sidewalk northeast?
                    sidewalks_neswx[   dir           ] ) { // has_sidewalk north?
                    int x1 = SEEX * 2 - 5;
                    int y1 = 0;
                    int x2 = SEEX * 2 - 1;
                    int y2 = SEEY - 1 + dead_end_extension;
                    coord_rotate_cw( x1, y1, dir );
                    coord_rotate_cw( x2, y2, dir );
                    square( m, t_sidewalk, x1, y1, x2, y2 );
                }
            }
        }

        //draw dead end sidewalk
        if( dead_end_extension > 0 && sidewalks_neswx[ 2 ] ) {
            square( m, t_sidewalk, 0, SEEY + dead_end_extension, SEEX * 2 - 1, SEEY + dead_end_extension + 4 );
        }

        // draw 16-wide pavement from the middle to the edge in each road direction
        // also corner pieces to curve towards diagonal neighbors
        for( int dir = 0; dir < 4; dir++ ) {
            if( roads_nesw[dir] ) {
                int x1 = 4;
                int y1 = 0;
                int x2 = SEEX * 2 - 1 - 4;
                int y2 = SEEY - 1 + dead_end_extension;
                coord_rotate_cw( x1, y1, dir );
                coord_rotate_cw( x2, y2, dir );
                square( m, t_pavement, x1, y1, x2, y2 );
                if( curvedir_nesw[dir] != 0 ) {
                    for( int x = 1; x < 4; x++ ) {
                        for( int y = 0; y < x; y++ ) {
                            int ty = y, tx = ( curvedir_nesw[dir] == -1 ? x : SEEX * 2 - 1 - x );
                            coord_rotate_cw( tx, ty, dir );
                            m->ter_set( point( tx, ty ), t_pavement );
                        }
                    }
                }
            }
        }

        // draw yellow dots on the pavement
        for( int dir = 0; dir < 4; dir++ ) {
            if( roads_nesw[dir] ) {
                int max_y = SEEY;
                if( num_dirs == 4 || ( num_dirs == 3 && dir == 0 ) ) {
                    max_y = 4; // dots don't extend into some intersections
                }
                for( int x = SEEX - 1; x <= SEEX; x++ ) {
                    for( int y = 0; y < max_y; y++ ) {
                        if( ( y + ( ( dir + rot ) / 2 % 2 ) ) % 4 ) {
                            int xn = x;
                            int yn = y;
                            coord_rotate_cw( xn, yn, dir );
                            m->ter_set( point( xn, yn ), t_pavement_y );
                        }
                    }
                }
            }
        }

        // draw round pavement for cul de sac late, to overdraw the yellow dots
        if( cul_de_sac ) {
            circle( m, t_pavement, double( SEEX ) - 0.5, double( SEEY ) - 0.5, 11.0 );
        }

        // overwrite part of intersection with rotary/plaza
        if( plaza_dir > -1 ) {
            if( plaza_dir == 8 ) { // plaza center
                fill_background( m, t_sidewalk );
                // TODO: something interesting here
            } else if( plaza_dir < 4 ) { // plaza side
                square( m, t_pavement, 0, SEEY - 10, SEEX * 2 - 1, SEEY - 1 );
                square( m, t_sidewalk, 0, SEEY - 2, SEEX * 2 - 1, SEEY * 2 - 1 );
                if( one_in( 3 ) ) {
                    line( m, t_tree_young, 1, SEEY, SEEX * 2 - 2, SEEY );
                }
                if( one_in( 3 ) ) {
                    line_furn( m, f_bench, 2, SEEY + 2, 5, SEEY + 2 );
                    line_furn( m, f_bench, 10, SEEY + 2, 13, SEEY + 2 );
                    line_furn( m, f_bench, 18, SEEY + 2, 21, SEEY + 2 );
                }
            } else { // plaza corner
                circle( m, t_pavement, 0, SEEY * 2 - 1, 21 );
                circle( m, t_sidewalk, 0, SEEY * 2 - 1, 13 );
                if( one_in( 3 ) ) {
                    circle( m, t_tree_young, 0, SEEY * 2 - 1, 11 );
                    circle( m, t_sidewalk,   0, SEEY * 2 - 1, 10 );
                }
                if( one_in( 3 ) ) {
                    circle( m, t_water_sh, 4, SEEY * 2 - 5, 3 );
                }
            }
        }
    }

    // spawn some vehicles
    if( plaza_dir != 8 ) {
        vspawn_id( neighbor_sidewalks ? "default_city" : "default_country" ).obj().apply(
            *m,
            num_dirs == 4 ? "road_four_way" :
            num_dirs == 3 ? "road_tee"      :
            num_dirs == 1 ? "road_end"      :
            diag          ? "road_curved"   :
            "road_straight"
        );
    }

    // spawn some monsters
    if( neighbor_sidewalks ) {
        m->place_spawns( mongroup_id( "GROUP_ZOMBIE" ), 2, point_zero, point( SEEX * 2 - 1, SEEX * 2 - 1 ),
                         dat.monster_density() );
        // 1 per 10 overmaps
        if( one_in( 10000 ) ) {
            m->add_spawn( mon_zombie_jackson, 1, point( SEEX, SEEY ) );
        }
    }

    // add some items
    bool plaza = ( plaza_dir > -1 );
    m->place_items( plaza ? "trash" : "road", 5, point_zero, point( SEEX * 2 - 1, SEEX * 2 - 1 ), plaza,
                    dat.when() );

    // add a manhole if appropriate
    if( dat.terrain_type() == "road_nesw_manhole" ) {
        m->ter_set( point( rng( 6, SEEX * 2 - 6 ), rng( 6, SEEX * 2 - 6 ) ), t_manhole_cover );
    }

    // finally, unrotate the map
    m->rotate( rot );

}
///////////////////

void mapgen_subway( mapgendata &dat )
{
    map *const m = &dat.m;
    // start by filling the whole map with grass/dirt/etc
    dat.fill_groundcover();

    // which of the cardinal directions get subway?
    bool subway_nesw[4] = {};
    int num_dirs = terrain_type_to_nesw_array( dat.terrain_type(), subway_nesw );

    for( int dir = 0; dir < 4; dir++ ) { // N E S W
        if( dat.t_nesw[dir]->has_flag( subway_connection ) && !subway_nesw[dir] ) {
            num_dirs++;
            subway_nesw[dir] = true;
        }
    }

    // which way should our subway curve, based on neighbor subway?
    int curvedir_nesw[4] = {};
    for( int dir = 0; dir < 4; dir++ ) { // N E S W
        if( !subway_nesw[dir] ) {
            continue;
        }

        if( dat.t_nesw[dir]->get_type_id().str() != "subway" &&
            !dat.t_nesw[dir]->has_flag( subway_connection ) ) {
            continue;
        }
        // n_* contain details about the neighbor being considered
        bool n_subway_nesw[4] = {};
        // TODO: figure out how to call this function without creating a new oter_id object
        int n_num_dirs = terrain_type_to_nesw_array( dat.t_nesw[dir], n_subway_nesw );
        for( int dir = 0; dir < 4; dir++ ) {
            if( dat.t_nesw[dir]->has_flag( subway_connection ) && !n_subway_nesw[dir] ) {
                n_num_dirs++;
                n_subway_nesw[dir] = true;
            }
        }
        // if 2-way neighbor has a subway facing us
        if( n_num_dirs == 2 && n_subway_nesw[( dir + 2 ) % 4] ) {
            // curve towards the direction the neighbor turns
            if( n_subway_nesw[( dir - 1 + 4 ) % 4] ) {
                curvedir_nesw[dir]--;    // our subway curves counterclockwise
            }
            if( n_subway_nesw[( dir + 1 ) % 4] ) {
                curvedir_nesw[dir]++;    // our subway curves clockwise
            }
        }
    }

    // calculate how far to rotate the map so we can work with just one orientation
    // also keep track of diagonal subway
    int rot = 0;
    bool diag = false;
    // TODO: reduce amount of logical/conditional constructs here
    switch( num_dirs ) {
        case 4: // 4-way intersection
            break;
        case 3: // tee
            if( !subway_nesw[0] ) {
                rot = 2;    // E/S/W, rotate 180 degrees
                break;
            }
            if( !subway_nesw[1] ) {
                rot = 3;    // N/S/W, rotate 270 degrees
                break;
            }
            if( !subway_nesw[3] ) {
                rot = 1;    // N/E/S, rotate  90 degrees
                break;
            }
            break;                                    // N/E/W, don't rotate
        case 2: // straight or diagonal
            if( subway_nesw[1] && subway_nesw[3] ) {
                rot = 1;    // E/W, rotate  90 degrees
                break;
            }
            if( subway_nesw[1] && subway_nesw[2] ) {
                rot = 1;    // E/S, rotate  90 degrees
                diag = true;
                break;
            }
            if( subway_nesw[2] && subway_nesw[3] ) {
                rot = 2;    // S/W, rotate 180 degrees
                diag = true;
                break;
            }
            if( subway_nesw[3] && subway_nesw[0] ) {
                rot = 3;    // W/N, rotate 270 degrees
                diag = true;
                break;
            }
            if( subway_nesw[0] && subway_nesw[1] ) {
                diag = true;    // N/E, don't rotate
                break;
            }
            break;                                                                  // N/S, don't rotate
        case 1: // dead end
            if( subway_nesw[1] ) {
                rot = 1;    // E, rotate  90 degrees
                break;
            }
            if( subway_nesw[2] ) {
                rot = 2;    // S, rotate 180 degrees
                break;
            }
            if( subway_nesw[3] ) {
                rot = 3;    // W, rotate 270 degrees
                break;
            }
            break;                                   // N, don't rotate
    }

    // rotate the arrays left by rot steps
    nesw_array_rotate<bool>( subway_nesw, 4, rot );
    nesw_array_rotate<int> ( curvedir_nesw,  4, rot );

    // now we have only these shapes: '   |   '-   -'-   -|-

    switch( num_dirs ) {
        case 4: // 4-way intersection
            mapf::formatted_set_simple( m, 0, 0,
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
        case 3: // tee
            mapf::formatted_set_simple( m, 0, 0,
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
        case 2: // straight or diagonal
            if( diag ) { // diagonal subway get drawn differently from all other types
                mapf::formatted_set_simple( m, 0, 0,
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
                mapf::formatted_set_simple( m, 0, 0,
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
        case 1:  // dead end
            mapf::formatted_set_simple( m, 0, 0,
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
            VehicleSpawn::apply( vspawn_id( "default_subway_deadend" ), *m, "subway" );
            break;
    }

    // finally, unrotate the map
    m->rotate( rot );
}

void mapgen_sewer_straight( mapgendata &dat )
{
    map *const m = &dat.m;
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( i < SEEX - 2 || i > SEEX + 1 ) {
                m->ter_set( point( i, j ), t_rock );
            } else {
                m->ter_set( point( i, j ), t_sewage );
            }
        }
    }
    m->place_items( "sewer", 10, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true, dat.when() );
    if( dat.terrain_type() == "sewer_ew" ) {
        m->rotate( 1 );
    }
}

void mapgen_sewer_curved( mapgendata &dat )
{
    map *const m = &dat.m;
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( ( i > SEEX + 1 && j < SEEY - 2 ) || i < SEEX - 2 || j > SEEY + 1 ) {
                m->ter_set( point( i, j ), t_rock );
            } else {
                m->ter_set( point( i, j ), t_sewage );
            }
        }
    }
    m->place_items( "sewer", 18, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true, dat.when() );
    if( dat.terrain_type() == "sewer_es" ) {
        m->rotate( 1 );
    }
    if( dat.terrain_type() == "sewer_sw" ) {
        m->rotate( 2 );
    }
    if( dat.terrain_type() == "sewer_wn" ) {
        m->rotate( 3 );
    }
}

void mapgen_sewer_tee( mapgendata &dat )
{
    map *const m = &dat.m;
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( i < SEEX - 2 || ( i > SEEX + 1 && ( j < SEEY - 2 || j > SEEY + 1 ) ) ) {
                m->ter_set( point( i, j ), t_rock );
            } else {
                m->ter_set( point( i, j ), t_sewage );
            }
        }
    }
    m->place_items( "sewer", 23, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true, dat.when() );
    if( dat.terrain_type() == "sewer_esw" ) {
        m->rotate( 1 );
    }
    if( dat.terrain_type() == "sewer_nsw" ) {
        m->rotate( 2 );
    }
    if( dat.terrain_type() == "sewer_new" ) {
        m->rotate( 3 );
    }
}

void mapgen_sewer_four_way( mapgendata &dat )
{
    map *const m = &dat.m;
    int rn = rng( 0, 3 );
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( ( i < SEEX - 2 || i > SEEX + 1 ) && ( j < SEEY - 2 || j > SEEY + 1 ) ) {
                m->ter_set( point( i, j ), t_rock );
            } else {
                m->ter_set( point( i, j ), t_sewage );
            }
            if( rn == 0 && ( trig_dist( point( i, j ), point( SEEX - 1, SEEY - 1 ) ) <= 6 ||
                             trig_dist( point( i, j ), point( SEEX - 1, SEEY ) ) <= 6 ||
                             trig_dist( point( i, j ), point( SEEX, SEEY - 1 ) ) <= 6 ||
                             trig_dist( point( i, j ), point( SEEX, SEEY ) ) <= 6 ) ) {
                m->ter_set( point( i, j ), t_sewage );
            }
            if( rn == 0 && ( i == SEEX - 1 || i == SEEX ) && ( j == SEEY - 1 || j == SEEY ) ) {
                m->ter_set( point( i, j ), t_grate );
            }
        }
    }
    m->place_items( "sewer", 28, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true, dat.when() );
}

///////////////////
void mapgen_bridge( mapgendata &dat )
{
    map *const m = &dat.m;
    const auto is_river = [&]( const om_direction::type dir ) {
        return dat.t_nesw[static_cast<int>( om_direction::add( dir,
                                            dat.terrain_type()->get_dir() ) )]->is_river();
    };

    const bool river_west = is_river( om_direction::type::west );
    const bool river_east = is_river( om_direction::type::east );

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( i < 2 ) {
                m->ter_set( point( i, j ), river_west ? t_water_moving_dp : grass_or_dirt() );
            } else if( i >= SEEX * 2 - 2 ) {
                m->ter_set( point( i, j ), river_east ? t_water_moving_dp : grass_or_dirt() );
            } else if( i == 2 || i == SEEX * 2 - 3 ) {
                m->ter_set( point( i, j ), t_guardrail_bg_dp );
            } else if( i == 3 || i == SEEX * 2 - 4 ) {
                m->ter_set( point( i, j ), t_sidewalk_bg_dp );
            } else {
                if( ( i == SEEX - 1 || i == SEEX ) && j % 4 != 0 ) {
                    m->ter_set( point( i, j ), t_pavement_y_bg_dp );
                } else {
                    m->ter_set( point( i, j ), t_pavement_bg_dp );
                }
            }
        }
    }

    // spawn regular road out of fuel vehicles
    VehicleSpawn::apply( vspawn_id( "default_bridge" ), *m, "bridge" );

    m->rotate( static_cast<int>( dat.terrain_type()->get_dir() ) );
    m->place_items( "road", 5, point_zero, point( SEEX * 2 - 1, SEEX * 2 - 1 ), false, dat.when() );
}

void mapgen_highway( mapgendata &dat )
{
    map *const m = &dat.m;
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( i < 3 || i >= SEEX * 2 - 3 ) {
                m->ter_set( point( i, j ), dat.groundcover() );
            } else if( i == 3 || i == SEEX * 2 - 4 ) {
                m->ter_set( point( i, j ), t_railing );
            } else {
                if( ( i == SEEX - 1 || i == SEEX ) && j % 4 != 0 ) {
                    m->ter_set( point( i, j ), t_pavement_y );
                } else {
                    m->ter_set( point( i, j ), t_pavement );
                }
            }
        }
    }

    // spawn regular road out of fuel vehicles
    VehicleSpawn::apply( vspawn_id( "default_highway" ), *m, "highway" );

    if( dat.terrain_type() == "hiway_ew" ) {
        m->rotate( 1 );
    }
    m->place_items( "road", 8, point_zero, point( SEEX * 2 - 1, SEEX * 2 - 1 ), false, dat.when() );
}

// mapgen_railroad
// TODO: Refactor and combine with other similiar functions (e.g. road).
void mapgen_railroad( mapgendata &dat )
{
    map *const m = &dat.m;
    // start by filling the whole map with grass/dirt/etc
    dat.fill_groundcover();
    // which of the cardinal directions get railroads?
    bool railroads_nesw[4] = {};
    int num_dirs = terrain_type_to_nesw_array( dat.terrain_type(), railroads_nesw );
    // which way should our railroads curve, based on neighbor railroads?
    int curvedir_nesw[4] = {};
    for( int dir = 0; dir < 4; dir++ ) { // N E S W
        if( !railroads_nesw[dir] || dat.t_nesw[dir]->get_type_id().str() != "railroad" ) {
            continue;
        }
        // n_* contain details about the neighbor being considered
        bool n_railroads_nesw[4] = {};
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
        case 4: // 4-way intersection
            break;
        case 3: // tee
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
        case 2: // straight or diagonal
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
            if( railroads_nesw[0] && railroads_nesw[1] ) {
                diag = true;    // N/E, don't rotate
                break;
            }
            break;                                                                        // N/S, don't rotate
        case 1: // dead end
            if( railroads_nesw[1] ) {
                rot = 1;    // E, rotate  90 degrees
                break;
            }
            if( railroads_nesw[2] ) {
                rot = 2;    // S, rotate 180 degrees
                break;
            }
            if( railroads_nesw[3] ) {
                rot = 3;    // W, rotate 270 degrees
                break;
            }
            break;                               // N, don't rotate
    }
    // rotate the arrays left by rot steps
    nesw_array_rotate<bool>( railroads_nesw, 4, rot );
    nesw_array_rotate<int> ( curvedir_nesw,  4, rot );
    // now we have only these shapes: '   |   '-   -'-   -|-
    switch( num_dirs ) {
        case 4: // 4-way intersection
            mapf::formatted_set_simple( m, 0, 0,
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
        case 3: // tee
            mapf::formatted_set_simple( m, 0, 0,
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
        case 2: // straight or diagonal
            if( diag ) { // diagonal railroads get drawn differently from all other types
                mapf::formatted_set_simple( m, 0, 0,
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
                mapf::formatted_set_simple( m, 0, 0,
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
        case 1:  // dead end
            mapf::formatted_set_simple( m, 0, 0,
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
    mapf::formatted_set_simple( m, 0, 0,
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

    if( dat.terrain_type() == "river_c_not_se" ) {
        m->rotate( 1 );
    }
    if( dat.terrain_type() == "river_c_not_sw" ) {
        m->rotate( 2 );
    }
    if( dat.terrain_type() == "river_c_not_nw" ) {
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
        line( m, grass_or_dirt(), x, 0, x, ground_edge );
        if( one_in( 25 ) ) {
            m->ter_set( point( x, ++ground_edge ), clay_or_sand() );
        }
        line( m, t_water_moving_sh, x, ++ground_edge, x, shallow_edge );
    }

    if( dat.terrain_type() == "river_east" ) {
        m->rotate( 1 );
    }
    if( dat.terrain_type() == "river_south" ) {
        m->rotate( 2 );
    }
    if( dat.terrain_type() == "river_west" ) {
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
        line( m, grass_or_dirt(), x, 0, x, ground_edge );
        if( one_in( 25 ) ) {
            m->ter_set( point( x, ++ground_edge ), clay_or_sand() );
        }
        line( m, t_water_moving_sh, x, ++ground_edge, x, shallow_edge );
    }
    for( int y = 0; y < SEEY * 2; y++ ) {
        int ground_edge = rng( 19, 21 );
        int shallow_edge = rng( 16, 18 );
        line( m, grass_or_dirt(), ground_edge, y, SEEX * 2 - 1, y );
        if( one_in( 25 ) ) {
            m->ter_set( point( --ground_edge, y ), clay_or_sand() );
        }
        line( m, t_water_moving_sh, shallow_edge, y, --ground_edge, y );
    }

    if( dat.terrain_type() == "river_se" ) {
        m->rotate( 1 );
    }
    if( dat.terrain_type() == "river_sw" ) {
        m->rotate( 2 );
    }
    if( dat.terrain_type() == "river_nw" ) {
        m->rotate( 3 );
    }
}

void mapgen_parking_lot( mapgendata &dat )
{
    map *const m = &dat.m;
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( ( j == 5 || j == 9 || j == 13 || j == 17 || j == 21 ) &&
                ( ( i > 1 && i < 8 ) || ( i > 14 && i < SEEX * 2 - 2 ) ) ) {
                m->ter_set( point( i, j ), t_pavement_y );
            } else if( ( j < 2 && i > 7 && i < 17 ) || ( j >= 2 && j < SEEY * 2 - 2 && i > 1 &&
                       i < SEEX * 2 - 2 ) ) {
                m->ter_set( point( i, j ), t_pavement );
            } else {
                m->ter_set( point( i, j ), dat.groundcover() );
            }
        }
    }

    VehicleSpawn::apply( vspawn_id( "default_parkinglot" ), *m, "parkinglot" );

    m->place_items( "road", 8, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), false, dat.when() );
    for( int i = 1; i < 4; i++ ) {
        const std::string &id = dat.t_nesw[i].id().str();
        if( id.size() > 5 && id.find( "road_" ) == 0 ) {
            m->rotate( i );
        }
    }
}

void house_room( room_type type, int x1, int y1, int x2, int y2, mapgendata &dat )
{
    map *const m = &dat.m;
    int pos_x1 = 0;
    int pos_y1 = 0;

    if( type == room_backyard ) { //processing it separately
        m->furn_set( point( x1 + 2, y1 ), f_chair );
        m->furn_set( point( x1 + 2, y1 + 1 ), f_table );
        for( int i = x1; i <= x2; i++ ) {
            for( int j = y1; j <= y2; j++ ) {
                if( ( i == x1 ) || ( i == x2 || ( j == y2 ) ) ) {
                    m->ter_set( point( i, j ), t_fence );
                } else {
                    m->ter_set( point( i, j ), t_grass );
                    if( one_in( 35 ) && !m->has_furn( point( i, j ) ) ) {
                        m->ter_set( point( i, j ), t_tree_young );
                    } else if( one_in( 35 ) && !m->has_furn( point( i, j ) ) ) {
                        m->ter_set( point( i, j ), t_tree );
                    } else if( one_in( 25 ) ) {
                        m->ter_set( point( i, j ), t_dirt );
                    }
                }
            }
        }
        m->ter_set( point( ( x1 + x2 ) / 2, y2 ), t_fencegate_c );
        return;
    }

    for( int i = x1; i <= x2; i++ ) {
        for( int j = y1; j <= y2; j++ ) {
            if( dat.is_groundcover( m->ter( point( i, j ) ) ) ||
                //m->ter(i, j) == t_grass || m->ter(i, j) == t_dirt ||
                m->ter( point( i, j ) ) == t_floor ) {
                if( j == y1 || j == y2 ) {
                    m->ter_set( point( i, j ), t_wall );
                } else if( i == x1 || i == x2 ) {
                    m->ter_set( point( i, j ), t_wall );
                } else {
                    m->ter_set( point( i, j ), t_floor );
                }
            }
        }
    }
    for( int i = y1 + 1; i <= y2 - 1; i++ ) {
        m->ter_set( point( x1, i ), t_wall );
        m->ter_set( point( x2, i ), t_wall );
    }

    items_location placed = "none";
    int chance = 0;
    int rn = 0;
    switch( type ) {
        case room_study:
            placed = "livingroom";
            chance = 40;
            break;
        case room_living:
            placed = "livingroom";
            chance = 83;
            //choose random wall
            switch( rng( 1, 4 ) ) { //some bookshelves
                case 1:
                    pos_x1 = x1 + 2;
                    pos_y1 = y1 + 1;
                    m->furn_set( point( x1 + 2, y2 - 1 ), f_desk );
                    while( pos_x1 < x2 ) {
                        pos_x1 += 1;
                        if( m->ter( point( pos_x1, pos_y1 ) ) == t_wall ) {
                            break;
                        }
                        m->furn_set( point( pos_x1, pos_y1 ), f_bookcase );
                        pos_x1 += 1;
                        if( m->ter( point( pos_x1, pos_y1 ) ) == t_wall ) {
                            break;
                        }
                        m->furn_set( point( pos_x1, pos_y1 ), f_bookcase );
                        pos_x1 += 2;
                    }
                    break;
                case 2:
                    pos_x1 = x2 - 2;
                    pos_y1 = y1 + 1;
                    m->furn_set( point( x1 + 2, y2 - 1 ), f_desk );
                    while( pos_x1 > x1 ) {
                        pos_x1 -= 1;
                        if( m->ter( point( pos_x1, pos_y1 ) ) == t_wall ) {
                            break;
                        }
                        m->furn_set( point( pos_x1, pos_y1 ), f_bookcase );
                        pos_x1 -= 1;
                        if( m->ter( point( pos_x1, pos_y1 ) ) == t_wall ) {
                            break;
                        }
                        m->furn_set( point( pos_x1, pos_y1 ), f_bookcase );
                        pos_x1 -= 2;
                    }
                    break;
                case 3:
                    pos_x1 = x1 + 2;
                    pos_y1 = y2 - 1;
                    m->furn_set( point( x1 + 2, y2 - 1 ), f_desk );
                    while( pos_x1 < x2 ) {
                        pos_x1 += 1;
                        if( m->ter( point( pos_x1, pos_y1 ) ) == t_wall ) {
                            break;
                        }
                        m->furn_set( point( pos_x1, pos_y1 ), f_bookcase );
                        pos_x1 += 1;
                        if( m->ter( point( pos_x1, pos_y1 ) ) == t_wall ) {
                            break;
                        }
                        m->furn_set( point( pos_x1, pos_y1 ), f_bookcase );
                        pos_x1 += 2;
                    }
                    break;
                case 4:
                    pos_x1 = x2 - 2;
                    pos_y1 = y2 - 1;
                    m->furn_set( point( x1 + 2, y2 - 1 ), f_desk );
                    while( pos_x1 > x1 ) {
                        pos_x1 -= 1;
                        if( m->ter( point( pos_x1, pos_y1 ) ) == t_wall ) {
                            break;
                        }
                        m->furn_set( point( pos_x1, pos_y1 ), f_bookcase );
                        pos_x1 -= 1;
                        if( m->ter( point( pos_x1, pos_y1 ) ) == t_wall ) {
                            break;
                        }
                        m->furn_set( point( pos_x1, pos_y1 ), f_bookcase );
                        pos_x1 -= 2;
                    }
                    break;
            }

            break;
        case room_kitchen: {
            placed = "kitchen";
            chance = 75;
            m->place_items( "cleaning",  58, point( x1 + 1, y1 + 1 ), point( x2 - 1, y2 - 2 ), false,
                            dat.when() );
            m->place_items( "home_hw",   40, point( x1 + 1, y1 + 1 ), point( x2 - 1, y2 - 2 ), false,
                            dat.when() );
            int oven_x = -1;
            int oven_y = -1;
            int cupboard_x = -1;
            int cupboard_y = -1;

            switch( rng( 1, 4 ) ) { //fridge, sink, oven and some cupboards near them
                case 1:
                    m->furn_set( point( x1 + 2, y1 + 1 ), f_fridge );
                    m->place_items( "fridge", 82, point( x1 + 2, y1 + 1 ), point( x1 + 2, y1 + 1 ), false, dat.when() );
                    m->furn_set( point( x1 + 1, y1 + 1 ), f_sink );
                    if( x1 + 4 < x2 ) {
                        oven_x     = x1 + 3;
                        cupboard_x = x1 + 4;
                        oven_y = cupboard_y = y1 + 1;
                    }

                    break;
                case 2:
                    m->furn_set( point( x2 - 2, y1 + 1 ), f_fridge );
                    m->place_items( "fridge", 82, point( x2 - 2, y1 + 1 ), point( x2 - 2, y1 + 1 ), false, dat.when() );
                    m->furn_set( point( x2 - 1, y1 + 1 ), f_sink );
                    if( x2 - 4 > x1 ) {
                        oven_x     = x2 - 3;
                        cupboard_x = x2 - 4;
                        oven_y = cupboard_y = y1 + 1;
                    }
                    break;
                case 3:
                    m->furn_set( point( x1 + 2, y2 - 1 ), f_fridge );
                    m->place_items( "fridge", 82, point( x1 + 2, y2 - 1 ), point( x1 + 2, y2 - 1 ), false, dat.when() );
                    m->furn_set( point( x1 + 1, y2 - 1 ), f_sink );
                    if( x1 + 4 < x2 ) {
                        oven_x     = x1 + 3;
                        cupboard_x = x1 + 4;
                        oven_y = cupboard_y = y2 - 1;
                    }
                    break;
                case 4:
                    m->furn_set( point( x2 - 2, y2 - 1 ), f_fridge );
                    m->place_items( "fridge", 82, point( x2 - 2, y2 - 1 ), point( x2 - 2, y2 - 1 ), false, dat.when() );
                    m->furn_set( point( x2 - 1, y2 - 1 ), f_sink );
                    if( x2 - 4 > x1 ) {
                        oven_x     = x2 - 3;
                        cupboard_x = x2 - 4;
                        oven_y = cupboard_y = y2 - 1;
                    }
                    break;
            }

            // oven and it's contents
            if( oven_x != -1 && oven_y != -1 ) {
                m->furn_set( point( oven_x, oven_y ), f_oven );
                m->place_items( "oven",       70, point( oven_x, oven_y ), point( oven_x, oven_y ), false,
                                dat.when() );
            }

            // cupboard and it's contents
            if( cupboard_x != -1 && cupboard_y != -1 ) {
                m->furn_set( point( cupboard_x, cupboard_y ), f_cupboard );
                m->place_items( "cleaning",   30, point( cupboard_x, cupboard_y ), point( cupboard_x, cupboard_y ),
                                false, dat.when() );
                m->place_items( "home_hw",    30, point( cupboard_x, cupboard_y ), point( cupboard_x, cupboard_y ),
                                false, dat.when() );
                m->place_items( "cannedfood", 30, point( cupboard_x, cupboard_y ), point( cupboard_x, cupboard_y ),
                                false, dat.when() );
                m->place_items( "pasta",      30, point( cupboard_x, cupboard_y ), point( cupboard_x, cupboard_y ),
                                false, dat.when() );
            }

            if( one_in( 2 ) ) { //dining table in the kitchen
                square_furn( m, f_table, static_cast<int>( ( x1 + x2 ) / 2 ) - 1,
                             static_cast<int>( ( y1 + y2 ) / 2 ) - 1,
                             static_cast<int>( ( x1 + x2 ) / 2 ),
                             static_cast<int>( ( y1 + y2 ) / 2 ) );
                m->place_items( "dining", 20, point( static_cast<int>( ( x1 + x2 ) / 2 ) - 1,
                                                     static_cast<int>( ( y1 + y2 ) / 2 ) - 1 ),
                                point( ( x1 + x2 ) / 2, static_cast<int>( ( y1 + y2 ) / 2 ) ), false, dat.when() );
            }
            if( one_in( 2 ) ) {
                for( int i = 0; i <= 2; i++ ) {
                    pos_x1 = rng( x1 + 2, x2 - 2 );
                    pos_y1 = rng( y1 + 1, y2 - 1 );
                    if( m->ter( point( pos_x1, pos_y1 ) ) == t_floor &&
                        !( m->furn( point( pos_x1, pos_y1 ) ) == f_cupboard ||
                           m->furn( point( pos_x1, pos_y1 ) ) == f_oven || m->furn( point( pos_x1, pos_y1 ) ) == f_sink ||
                           m->furn( point( pos_x1, pos_y1 ) ) == f_fridge ) ) {
                        m->furn_set( point( pos_x1, pos_y1 ), f_chair );
                    }
                }
            }

        }
        break;
        case room_bedroom:
            placed = "bedroom";
            chance = 78;
            if( one_in( 14 ) ) {
                m->place_items( "homeguns", 58, point( x1 + 1, y1 + 1 ), point( x2 - 1, y2 - 1 ), false,
                                dat.when() );
            }
            if( one_in( 10 ) ) {
                m->place_items( "home_hw",  40, point( x1 + 1, y1 + 1 ), point( x2 - 1, y2 - 1 ), false,
                                dat.when() );
            }
            switch( rng( 1, 5 ) ) {
                case 1:
                    m->furn_set( point( x1 + 1, y1 + 2 ), f_bed );
                    m->furn_set( point( x1 + 1, y1 + 3 ), f_bed );
                    m->place_items( "bed", 60, point( x1 + 1, y1 + 2 ), point( x1 + 1, y1 + 2 ), false, dat.when() );
                    m->place_items( "bed", 60, point( x1 + 1, y1 + 3 ), point( x1 + 1, y1 + 3 ), false, dat.when() );
                    break;
                case 2:
                    m->furn_set( point( x1 + 2, y2 - 1 ), f_bed );
                    m->furn_set( point( x1 + 3, y2 - 1 ), f_bed );
                    m->place_items( "bed", 60, point( x1 + 2, y2 - 1 ), point( x1 + 2, y2 - 1 ), false, dat.when() );
                    m->place_items( "bed", 60, point( x1 + 2, y2 - 1 ), point( x1 + 2, y2 - 1 ), false, dat.when() );
                    break;
                case 3:
                    m->furn_set( point( x2 - 1, y2 - 3 ), f_bed );
                    m->furn_set( point( x2 - 1, y2 - 2 ), f_bed );
                    m->place_items( "bed", 60, point( x2 - 1, y2 - 3 ), point( x2 - 1, y2 - 3 ), false, dat.when() );
                    m->place_items( "bed", 60, point( x2 - 1, y2 - 2 ), point( x2 - 1, y2 - 2 ), false, dat.when() );
                    break;
                case 4:
                    m->furn_set( point( x2 - 3, y1 + 1 ), f_bed );
                    m->furn_set( point( x2 - 2, y1 + 1 ), f_bed );
                    m->place_items( "bed", 60, point( x2 - 3, y1 + 1 ), point( x2 - 3, y1 + 1 ), false, dat.when() );
                    m->place_items( "bed", 60, point( x2 - 2, y1 + 1 ), point( x2 - 2, y1 + 1 ), false, dat.when() );
                    break;
                case 5:
                    m->furn_set( point( ( x1 + x2 ) / 2, y2 - 1 ), f_bed );
                    m->furn_set( point( static_cast<int>( ( x1 + x2 ) / 2 ) + 1, y2 - 1 ), f_bed );
                    m->furn_set( point( ( x1 + x2 ) / 2, y2 - 2 ), f_bed );
                    m->furn_set( point( static_cast<int>( ( x1 + x2 ) / 2 ) + 1, y2 - 2 ), f_bed );
                    m->place_items( "bed", 60, point( ( x1 + x2 ) / 2, y2 - 1 ),
                                    point( ( x1 + x2 ) / 2, y2 - 1 ), false,
                                    dat.when() );
                    m->place_items( "bed", 60, point( static_cast<int>( ( x1 + x2 ) / 2 ) + 1, y2 - 1 ),
                                    point( static_cast<int>( ( x1 + x2 ) / 2 ) + 1, y2 - 1 ),
                                    false, dat.when() );
                    m->place_items( "bed", 60, point( ( x1 + x2 ) / 2, y2 - 2 ),
                                    point( ( x1 + x2 ) / 2, y2 - 2 ), false,
                                    dat.when() );
                    m->place_items( "bed", 60, point( static_cast<int>( ( x1 + x2 ) / 2 ) + 1, y2 - 2 ),
                                    point( static_cast<int>( ( x1 + x2 ) / 2 ) + 1, y2 - 2 ),
                                    false, dat.when() );
                    break;
            }
            switch( rng( 1, 4 ) ) {
                case 1:
                    m->furn_set( point( x1 + 2, y1 + 1 ), f_dresser );
                    m->place_items( "dresser", 80, point( x1 + 2, y1 + 1 ), point( x1 + 2, y1 + 1 ), false,
                                    dat.when() );
                    break;
                case 2:
                    m->furn_set( point( x2 - 2, y2 - 1 ), f_dresser );
                    m->place_items( "dresser", 80, point( x2 - 2, y2 - 1 ), point( x2 - 2, y2 - 1 ), false,
                                    dat.when() );
                    break;
                case 3:
                    rn = static_cast<int>( ( x1 + x2 ) / 2 );
                    m->furn_set( point( rn, y1 + 1 ), f_dresser );
                    m->place_items( "dresser", 80, point( rn, y1 + 1 ), point( rn, y1 + 1 ), false, dat.when() );
                    break;
                case 4:
                    rn = static_cast<int>( ( y1 + y2 ) / 2 );
                    m->furn_set( point( x1 + 1, rn ), f_dresser );
                    m->place_items( "dresser", 80, point( x1 + 1, rn ), point( x1 + 1, rn ), false, dat.when() );
                    break;
            }
            break;
        case room_bathroom:
            m->place_toilet( point( x2 - 1, y2 - 1 ) );
            m->place_items( "harddrugs", 18, point( x1 + 1, y1 + 1 ), point( x2 - 1, y2 - 2 ), false,
                            dat.when() );
            m->place_items( "cleaning",  48, point( x1 + 1, y1 + 1 ), point( x2 - 1, y2 - 2 ), false,
                            dat.when() );
            placed = "softdrugs";
            chance = 72;
            m->furn_set( point( x2 - 1, y2 - 2 ), f_bathtub );
            if( one_in( 3 ) && !( m->ter( point( x2 - 1, y2 - 3 ) ) == t_wall ) ) {
                m->furn_set( point( x2 - 1, y2 - 3 ), f_bathtub );
            }
            if( !( ( m->furn( point( x1 + 1, y2 - 2 ) ) == f_toilet ) ||
                   ( m->furn( point( x1 + 1, y2 - 2 ) ) == f_bathtub ) ) ) {
                m->furn_set( point( x1 + 1, y2 - 2 ), f_sink );
            }
            if( one_in( 4 ) ) {
                for( int x = x1 + 1; x <= x2 - 1; x++ ) {
                    for( int y = y1 + 1; y <= y2 - 1; y++ ) {
                        m->ter_set( point( x, y ), t_linoleum_white );
                    }
                }
            } else if( one_in( 4 ) ) {
                for( int x = x1 + 1; x <= x2 - 1; x++ ) {
                    for( int y = y1 + 1; y <= y2 - 1; y++ ) {
                        m->ter_set( point( x, y ), t_linoleum_gray );
                    }
                }
            }
            break;
        default:
            break;
    }
    m->place_items( placed, chance, point( x1 + 1, y1 + 1 ), point( x2 - 1, y2 - 1 ), false,
                    dat.when() );
}

void mapgen_generic_house_boxy( mapgendata &dat )
{
    mapgen_generic_house( dat, 1 );
}

void mapgen_generic_house_big_livingroom( mapgendata &dat )
{
    mapgen_generic_house( dat, 2 );
}

void mapgen_generic_house_center_hallway( mapgendata &dat )
{
    mapgen_generic_house( dat, 3 );
}

void mapgen_generic_house( mapgendata &dat, int variant )
{
    map *const m = &dat.m;
    int rn = 0;
    int lw = 0;
    int rw = 0;
    int mw = 0;
    int tw = 0;
    int bw = 0;
    int cw = 0;
    int actual_house_height = 0;
    int bw_old = 0;

    lw = rng( 0, 4 ); // West external wall
    // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
    mw = lw + rng( 7, 10 ); // Middle wall between bedroom & kitchen/bath
    rw = SEEX * 2 - rng( 1, 5 ); // East external wall
    tw = rng( 1, 6 ); // North external wall
    bw = SEEX * 2 - rng( 2, 5 ); // South external wall
    // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
    cw = tw + rng( 4, 7 ); // Middle wall between living room & kitchen/bed
    actual_house_height = bw - rng( 4,
                                    6 ); //reserving some space for backyard. Actual south external wall.
    bw_old = bw;

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( i > lw && i < rw && j > tw && j < bw ) {
                m->ter_set( point( i, j ), t_floor );
            } else {
                m->ter_set( point( i, j ), dat.groundcover() );
            }
            if( i >= lw && i <= rw && ( j == tw || j == bw ) ) { //placing north and south walls
                m->ter_set( point( i, j ), t_wall );
            }
            if( ( i == lw || i == rw ) && j > tw &&
                j < bw /*actual_house_height*/ ) { //placing west (lw) and east walls
                m->ter_set( point( i, j ), t_wall );
            }
        }
    }
    switch( variant ) {
        case 1: // Quadrants, essentially
            mw = rng( lw + 5, rw - 5 );
            cw = tw + rng( 4, 7 );
            house_room( room_living, mw, tw, rw, cw, dat );
            house_room( room_kitchen, lw, tw, mw, cw, dat );
            m->ter_set( point( mw, rng( tw + 2, cw - 2 ) ), ( one_in( 3 ) ? t_door_c : t_floor ) );
            rn = rng( lw + 1, mw - 2 );
            m->ter_set( point( rn, tw ), t_window_domestic );
            m->ter_set( point( rn + 1, tw ), t_window_domestic );
            rn = rng( mw + 1, rw - 2 );
            m->ter_set( point( rn, tw ), t_window_domestic );
            m->ter_set( point( rn + 1, tw ), t_window_domestic );
            rn = rng( lw + 3, rw - 3 ); // Bottom part mw
            if( rn <= lw + 5 ) {
                // Bedroom on right, bathroom on left
                house_room( room_bedroom, rn, cw, rw, bw, dat );

                // Put door between bedroom and living
                m->ter_set( point( rng( rw - 1, rn > mw ? rn + 1 : mw + 1 ), cw ), t_door_c );

                if( bw - cw >= 10 && rn - lw >= 6 ) {
                    // All fits, placing bathroom and 2nd bedroom
                    house_room( room_bathroom, lw, bw - 5, rn, bw, dat );
                    house_room( room_bedroom, lw, cw, rn, bw - 5, dat );

                    // Put door between bathroom and bedroom
                    m->ter_set( point( rn, rng( bw - 4, bw - 1 ) ), t_door_c );

                    if( one_in( 3 ) ) {
                        // Put door between 2nd bedroom and 1st bedroom
                        m->ter_set( point( rn, rng( cw + 1, bw - 6 ) ), t_door_c );
                    } else {
                        // ...Otherwise, between 2nd bedroom and kitchen
                        m->ter_set( point( rng( lw + 1, rn > mw ? mw - 1 : rn - 1 ), cw ), t_door_c );
                    }
                } else if( bw - cw > 4 ) {
                    // Too big for a bathroom, not big enough for 2nd bedroom
                    // Make it a bathroom anyway, but give the excess space back to
                    // the kitchen.
                    house_room( room_bathroom, lw, bw - 4, rn, bw, dat );
                    for( int i = lw + 1; i < mw && i < rn; i++ ) {
                        m->ter_set( point( i, cw ), t_floor );
                    }

                    // Put door between excess space and bathroom
                    m->ter_set( point( rng( lw + 1, rn - 1 ), bw - 4 ), t_door_c );

                    // Put door between excess space and bedroom
                    m->ter_set( point( rn, rng( cw + 1, bw - 5 ) ), t_door_c );
                } else {
                    // Small enough to be a bathroom; make it one.
                    house_room( room_bathroom, lw, cw, rn, bw, dat );

                    if( one_in( 5 ) ) {
                        // Put door between bathroom and kitchen with low chance
                        m->ter_set( point( rng( lw + 1, rn > mw ? mw - 1 : rn - 1 ), cw ), t_door_c );
                    } else {
                        // ...Otherwise, between bathroom and bedroom
                        m->ter_set( point( rn, rng( cw + 1, bw - 1 ) ), t_door_c );
                    }
                }
                // Point on bedroom wall, for window
                rn = rng( rn + 2, rw - 2 );
            } else {
                // Bedroom on left, bathroom on right
                house_room( room_bedroom, lw, cw, rn, bw, dat );

                // Put door between bedroom and kitchen
                m->ter_set( point( rng( lw + 1, rn > mw ? mw - 1 : rn - 1 ), cw ), t_door_c );

                if( bw - cw >= 10 && rw - rn >= 6 ) {
                    // All fits, placing bathroom and 2nd bedroom
                    house_room( room_bathroom, rn, bw - 5, rw, bw, dat );
                    house_room( room_bedroom, rn, cw, rw, bw - 5, dat );

                    // Put door between bathroom and bedroom
                    m->ter_set( point( rn, rng( bw - 4, bw - 1 ) ), t_door_c );

                    if( one_in( 3 ) ) {
                        // Put door between 2nd bedroom and 1st bedroom
                        m->ter_set( point( rn, rng( cw + 1, bw - 6 ) ), t_door_c );
                    } else {
                        // ...Otherwise, between 2nd bedroom and living
                        m->ter_set( point( rng( rw - 1, rn > mw ? rn + 1 : mw + 1 ), cw ), t_door_c );
                    }
                } else if( bw - cw > 4 ) {
                    // Too big for a bathroom, not big enough for 2nd bedroom
                    // Make it a bathroom anyway, but give the excess space back to
                    // the living.
                    house_room( room_bathroom, rn, bw - 4, rw, bw, dat );
                    for( int i = rw - 1; i > rn && i > mw; i-- ) {
                        m->ter_set( point( i, cw ), t_floor );
                    }

                    // Put door between excess space and bathroom
                    m->ter_set( point( rng( rw - 1, rn + 1 ), bw - 4 ), t_door_c );

                    // Put door between excess space and bedroom
                    m->ter_set( point( rn, rng( cw + 1, bw - 5 ) ), t_door_c );
                } else {
                    // Small enough to be a bathroom; make it one.
                    house_room( room_bathroom, rn, cw, rw, bw, dat );

                    if( one_in( 5 ) ) {
                        // Put door between bathroom and living with low chance
                        m->ter_set( point( rng( rw - 1, rn > mw ? rn + 1 : mw + 1 ), cw ), t_door_c );
                    } else {
                        // ...Otherwise, between bathroom and bedroom
                        m->ter_set( point( rn, rng( cw + 1, bw - 1 ) ), t_door_c );
                    }
                }
                // Point on bedroom wall, for window
                rn = rng( lw + 2, rn - 2 );
            }
            m->ter_set( point( rn, bw ), t_window_domestic );
            m->ter_set( point( rn + 1, bw ), t_window_domestic );
            if( !one_in( 3 ) && rw < SEEX * 2 - 1 ) { // Potential side windows
                rn = rng( tw + 2, bw - 6 );
                m->ter_set( point( rw, rn ), t_window_domestic );
                m->ter_set( point( rw, rn + 4 ), t_window_domestic );
            }
            if( !one_in( 3 ) && lw > 0 ) { // Potential side windows
                rn = rng( tw + 2, bw - 6 );
                m->ter_set( point( lw, rn ), t_window_domestic );
                m->ter_set( point( lw, rn + 4 ), t_window_domestic );
            }
            if( one_in( 2 ) ) { // Placement of the main door
                m->ter_set( point( rng( lw + 2, mw - 1 ), tw ),
                            ( one_in( 6 ) ? ( one_in( 6 ) ? t_door_c : t_door_c_peep ) : ( one_in(
                                        6 ) ? t_door_locked : t_door_locked_peep ) ) );
                if( one_in( 5 ) ) { // Placement of side door
                    m->ter_set( point( rw, rng( tw + 2, cw - 2 ) ), ( one_in( 6 ) ? t_door_c : t_door_locked ) );
                }
            } else {
                m->ter_set( point( rng( mw + 1, rw - 2 ), tw ),
                            ( one_in( 6 ) ? ( one_in( 6 ) ? t_door_c : t_door_c_peep ) : ( one_in(
                                        6 ) ? t_door_locked : t_door_locked_peep ) ) );
                if( one_in( 5 ) ) {
                    m->ter_set( point( lw, rng( tw + 2, cw - 2 ) ), ( one_in( 6 ) ? t_door_c : t_door_locked ) );
                }
            }
            break;

        case 2: // Old-style; simple;
            //Modified by Jovan in 28 Aug 2013
            //Long narrow living room in front, big kitchen and HUGE bedroom
            bw = SEEX * 2 - 2;
            cw = tw + rng( 3, 6 );
            mw = rng( lw + 7, rw - 4 );
            //int actual_house_height=bw-rng(4,6);
            //in some rare cases some rooms (especially kitchen and living room) may get rather small
            if( ( tw <= 3 ) && ( abs( ( actual_house_height - 3 ) - cw ) >= 3 ) ) {
                //everything is fine
                house_room( room_backyard, lw, actual_house_height + 1, rw, bw, dat );
                //door from bedroom to backyard
                m->ter_set( point( ( lw + mw ) / 2, actual_house_height ), t_door_c );
            } else { //using old layout
                actual_house_height = bw_old;
            }
            // Plop down the rooms
            house_room( room_living, lw, tw, rw, cw, dat );
            house_room( room_kitchen, mw, cw, rw, actual_house_height - 3, dat );
            house_room( room_bedroom, lw, cw, mw, actual_house_height, dat ); //making bedroom smaller
            house_room( room_bathroom, mw, actual_house_height - 3, rw, actual_house_height, dat );

            // Space between kitchen & living room:
            rn = rng( mw + 1, rw - 3 );
            m->ter_set( point( rn, cw ), t_floor );
            m->ter_set( point( rn + 1, cw ), t_floor );
            // Front windows
            rn = rng( 2, 5 );
            m->ter_set( point( lw + rn, tw ), t_window_domestic );
            m->ter_set( point( lw + rn + 1, tw ), t_window_domestic );
            m->ter_set( point( rw - rn, tw ), t_window_domestic );
            m->ter_set( point( rw - rn + 1, tw ), t_window_domestic );
            // Front door
            m->ter_set( point( rng( lw + 4, rw - 4 ), tw ), ( one_in( 6 ) ? t_door_c : t_door_locked ) );
            if( one_in( 3 ) ) { // Kitchen windows
                rn = rng( cw + 1, actual_house_height - 5 );
                m->ter_set( point( rw, rn ), t_window_domestic );
                m->ter_set( point( rw, rn + 1 ), t_window_domestic );
            }
            if( one_in( 3 ) ) { // Bedroom windows
                rn = rng( cw + 1, actual_house_height - 2 );
                m->ter_set( point( lw, rn ), t_window_domestic );
                m->ter_set( point( lw, rn + 1 ), t_window_domestic );
            }
            // Door to bedroom
            if( one_in( 4 ) ) {
                m->ter_set( point( rng( lw + 1, mw - 1 ), cw ), t_door_c );
            } else {
                m->ter_set( point( mw, rng( cw + 3, actual_house_height - 4 ) ), t_door_c );
            }
            // Door to bathroom
            if( one_in( 4 ) ) {
                m->ter_set( point( mw, actual_house_height - 1 ), t_door_c );
            } else {
                m->ter_set( point( rng( mw + 2, rw - 2 ), actual_house_height - 3 ), t_door_c );
            }
            // Back windows
            rn = rng( lw + 1, mw - 2 );
            m->ter_set( point( rn, actual_house_height ), t_window_domestic );
            m->ter_set( point( rn + 1, actual_house_height ), t_window_domestic );
            rn = rng( mw + 1, rw - 1 );
            m->ter_set( point( rn, actual_house_height ), t_window_domestic );
            break;

        case 3: // Long center hallway, kitchen, living room and office
            mw = static_cast<int>( ( lw + rw ) / 2 );
            cw = bw - rng( 5, 7 );
            // Hallway doors and windows
            m->ter_set( point( mw, tw ), ( one_in( 6 ) ? t_door_c : t_door_locked ) );
            if( one_in( 4 ) ) {
                m->ter_set( point( mw - 1, tw ), t_window_domestic );
                m->ter_set( point( mw + 1, tw ), t_window_domestic );
            }
            for( int i = tw + 1; i < cw; i++ ) { // Hallway walls
                m->ter_set( point( mw - 2, i ), t_wall );
                m->ter_set( point( mw + 2, i ), t_wall );
            }
            if( one_in( 2 ) ) { // Front rooms are kitchen or living room
                house_room( room_living, lw, tw, mw - 2, cw, dat );
                house_room( room_kitchen, mw + 2, tw, rw, cw, dat );
            } else {
                house_room( room_kitchen, lw, tw, mw - 2, cw, dat );
                house_room( room_living, mw + 2, tw, rw, cw, dat );
            }
            // Front windows
            rn = rng( lw + 1, mw - 4 );
            m->ter_set( point( rn, tw ), t_window_domestic );
            m->ter_set( point( rn + 1, tw ), t_window_domestic );
            rn = rng( mw + 3, rw - 2 );
            m->ter_set( point( rn, tw ), t_window_domestic );
            m->ter_set( point( rn + 1, tw ), t_window_domestic );
            if( one_in( 3 ) && lw > 0 ) { // Side windows?
                rn = rng( tw + 1, cw - 2 );
                m->ter_set( point( lw, rn ), t_window_domestic );
                m->ter_set( point( lw, rn + 1 ), t_window_domestic );
            }
            if( one_in( 3 ) && rw < SEEX * 2 - 1 ) { // Side windows?
                rn = rng( tw + 1, cw - 2 );
                m->ter_set( point( rw, rn ), t_window_domestic );
                m->ter_set( point( rw, rn + 1 ), t_window_domestic );
            }
            if( one_in( 2 ) ) { // Bottom rooms are bedroom or bathroom
                //bathroom to the left (eastern wall), study to the right
                //house_room(m, room_bedroom, lw, cw, rw - 3, bw, dat);
                house_room( room_bedroom, mw - 2, cw, rw - 3, bw, dat );
                house_room( room_bathroom, rw - 3, cw, rw, bw, dat );
                house_room( room_study, lw, cw, mw - 2, bw, dat );
                //===Study Room Furniture==
                m->ter_set( point( mw - 2, ( bw + cw ) / 2 ), t_door_o );
                m->furn_set( point( lw + 1, cw + 1 ), f_chair );
                m->furn_set( point( lw + 1, cw + 2 ), f_table );
                m->ter_set( point( lw + 1, cw + 3 ), t_console_broken );
                m->furn_set( point( lw + 3, bw - 1 ), f_bookcase );
                m->place_items( "magazines", 30,  point( lw + 3, bw - 1 ), point( lw + 3, bw - 1 ), false,
                                dat.when() );
                m->place_items( "novels", 40,  point( lw + 3, bw - 1 ), point( lw + 3, bw - 1 ), false,
                                dat.when() );
                m->place_items( "alcohol", 20,  point( lw + 3, bw - 1 ), point( lw + 3, bw - 1 ), false,
                                dat.when() );
                m->place_items( "manuals", 30,  point( lw + 3, bw - 1 ), point( lw + 3, bw - 1 ), false,
                                dat.when() );
                //=========================
                m->ter_set( point( rng( lw + 2, mw - 3 ), cw ), t_door_c );
                if( one_in( 4 ) ) {
                    m->ter_set( point( rng( rw - 2, rw - 1 ), cw ), t_door_c );
                } else {
                    m->ter_set( point( rw - 3, rng( cw + 2, bw - 2 ) ), t_door_c );
                }
                rn = rng( mw, rw - 5 ); //bedroom windows
                m->ter_set( point( rn, bw ), t_window_domestic );
                m->ter_set( point( rn + 1, bw ), t_window_domestic );
                m->ter_set( point( rng( lw + 2, mw - 3 ), bw ), t_window_domestic ); //study window

                if( one_in( 4 ) ) {
                    m->ter_set( point( rng( rw - 2, rw - 1 ), bw ), t_window_domestic );
                } else {
                    m->ter( point( rw, rng( cw + 1, bw - 1 ) ) );
                }
            } else { //bathroom to the right
                house_room( room_bathroom, lw, cw, lw + 3, bw, dat );
                //house_room(m, room_bedroom, lw + 3, cw, rw, bw, dat);
                house_room( room_bedroom, lw + 3, cw, mw + 2, bw, dat );
                house_room( room_study, mw + 2, cw, rw, bw, dat );
                //===Study Room Furniture==
                m->ter_set( point( mw + 2, ( bw + cw ) / 2 ), t_door_c );
                m->furn_set( point( rw - 1, cw + 1 ), f_chair );
                m->furn_set( point( rw - 1, cw + 2 ), f_table );
                m->ter_set( point( rw - 1, cw + 3 ), t_console_broken );
                m->furn_set( point( rw - 3, bw - 1 ), f_bookcase );
                m->place_items( "magazines", 40,  point( rw - 3, bw - 1 ), point( rw - 3, bw - 1 ), false,
                                dat.when() );
                m->place_items( "novels", 40,  point( rw - 3, bw - 1 ), point( rw - 3, bw - 1 ), false,
                                dat.when() );
                m->place_items( "alcohol", 20,  point( rw - 3, bw - 1 ), point( rw - 3, bw - 1 ), false,
                                dat.when() );
                m->place_items( "manuals", 20,  point( rw - 3, bw - 1 ), point( rw - 3, bw - 1 ), false,
                                dat.when() );
                //=========================

                if( one_in( 4 ) ) {
                    m->ter_set( point( rng( lw + 1, lw + 2 ), cw ), t_door_c );
                } else {
                    m->ter_set( point( lw + 3, rng( cw + 2, bw - 2 ) ), t_door_c );
                }
                rn = rng( lw + 4, mw ); //bedroom windows
                m->ter_set( point( rn, bw ), t_window_domestic );
                m->ter_set( point( rn + 1, bw ), t_window_domestic );
                m->ter_set( point( rng( mw + 3, rw - 1 ), bw ), t_window_domestic ); //study window
                if( one_in( 4 ) ) {
                    m->ter_set( point( rng( lw + 1, lw + 2 ), bw ), t_window_domestic );
                } else {
                    m->ter( point( lw, rng( cw + 1, bw - 1 ) ) );
                }
            }
            // Doors off the sides of the hallway
            m->ter_set( point( mw - 2, rng( tw + 3, cw - 3 ) ), t_door_c );
            m->ter_set( point( mw + 2, rng( tw + 3, cw - 3 ) ), t_door_c );
            m->ter_set( point( mw, cw ), t_door_c );
            break;
    } // Done with the various house structures
    //////
    if( rng( 2, 7 ) < tw ) { // Big front yard has a chance for a fence
        for( int i = lw; i <= rw; i++ ) {
            m->ter_set( point( i, 0 ), t_fence );
        }
        for( int i = 1; i < tw; i++ ) {
            m->ter_set( point( lw, i ), t_fence );
        }
        int hole = rng( SEEX - 3, SEEX + 2 );
        m->ter_set( point( hole, 0 ), t_dirt );
        m->ter_set( point( hole + 1, 0 ), t_dirt );
        if( one_in( tw ) ) {
            m->ter_set( point( hole - 1, 1 ), t_tree_young );
            m->ter_set( point( hole + 2, 1 ), t_tree_young );
        }
    }

    place_stairs( dat );

    // Just boring old zombies
    m->place_spawns( mongroup_id( "GROUP_ZOMBIE" ), 2, point_zero, point( SEEX * 2 - 1, SEEX * 2 - 1 ),
                     dat.monster_density() );

    m->rotate( static_cast<int>( dat.terrain_type()->get_dir() ) );
}

///////////////////////////////////////////////////////////
void mapgen_basement_generic_layout( mapgendata &dat )
{
    map *const m = &dat.m;
    const ter_id t_rock_smooth( "t_rock_smooth" );
    const int up = 0;
    const int left = 0;
    const int down = SEEY * 2 - 5;
    const int right = SEEX * 2 - 1;
    square( m, t_rock, left, down, right, SEEY * 2 - 1 );
    square( m, t_rock_floor, 1, 1, right - 1, down - 1 );
    line( m, t_rock_smooth, left, up, right, up );
    line( m, t_rock_smooth, left, down, right, down );
    line( m, t_rock_smooth, left, up, left, down );
    line( m, t_rock_smooth, right, up, right, down );
    m->ter_set( point( SEEX - 1, down - 1 ), t_stairs_up );
    m->ter_set( point( SEEX, down - 1 ), t_stairs_up );
    line( m, t_rock_smooth, SEEX - 2, down - 1, SEEX - 2, down - 3 );
    line( m, t_rock_smooth, SEEX + 1, down - 1, SEEX + 1, down - 3 );
    line( m, t_door_locked, SEEX - 1, down - 3, SEEX, down - 3 );

    // Rotate randomly, now that other basements are more generic
    m->rotate( rng( 0, 3 ) );
}

namespace furn_space
{
static bool clear( const map &m, const tripoint &from, const tripoint &to )
{
    for( const auto &p : m.points_in_rectangle( from, to ) ) {
        if( m.ter( p ).obj().movecost == 0 ) {
            return false;
        }
    }

    return true;
}

static point best_expand( const map &m, const tripoint &from, int maxx, int maxy )
{
    if( clear( m, from, from + point( maxx, maxy ) ) ) {
        // Common case
        return point( maxx, maxy );
    }

    // Brute force all the combinations for the one with biggest area
    int best_area = 0;
    point best;
    for( int x = 0; x <= maxx; x++ ) {
        for( int y = 0; y <= maxy; y++ ) {
            int area = x * y;
            if( area <= best_area ) {
                continue;
            }

            if( clear( m, from, from + point( x, y ) ) ) {
                best_area = area;
                best = point( x, y );
            }
        }
    }

    return best;
}
} // namespace furn_space

void mapgen_basement_junk( mapgendata &dat )
{
    map *const m = &dat.m;
    // Junk!
    mapgen_basement_generic_layout( dat );
    //makes a square of randomly thrown around furniture and places stuff.
    const int z = m->get_abs_sub().z;
    for( const auto &p : m->points_in_rectangle( tripoint( 1, 1, z ), tripoint( 22, 22, z ) ) ) {
        if( m->ter( p ).obj().movecost == 0 ) {
            // Wall, skip
            continue;
        }

        if( one_in( 1600 ) ) {
            m->furn_set( p, furn_str_id( "f_gun_safe_el" ) );
            m->place_items( "basement_op_guns", 96,  p.xy(), p.xy(), false, dat.when() );
            m->place_items( "ammo", 90,  p.xy(), p.xy(), false, dat.when() );
        }
        if( one_in( 20 ) ) {
            int rn = rng( 1, 8 );
            if( rn == 1 ) {
                m->furn_set( p, f_dresser );
                m->place_items( "dresser", 30,  p.xy(), p.xy(), false, dat.when() );
                m->place_items( "trash_forest", 60,  p.xy(), p.xy(), false, dat.when() );
            } else if( rn == 2 ) {
                m->furn_set( p, f_chair );
            } else if( rn == 3 ) {
                m->furn_set( p, f_cupboard );
                m->place_items( "trash", 60,  p.xy(), p.xy(), false, dat.when() );
                m->place_items( "dining", 40,  p.xy(), p.xy(), false, dat.when() );
            } else if( rn == 4 ) {
                tripoint rs = p + furn_space::best_expand( *m, p, rng( 0, 4 ), 0 );
                square_furn( m, f_bookcase, p.x, p.y, rs.x, rs.y );
                m->place_items( "novels", 60,  p.xy(), rs.xy(), false, dat.when() );
                m->place_items( "magazines", 20,  p.xy(), rs.xy(), false, dat.when() );
            } else if( rn == 5 ) {
                tripoint rs = p + furn_space::best_expand( *m, p, 0, rng( 0, 4 ) );
                square_furn( m, f_bookcase, p.x, p.y, rs.x, rs.y );
                m->place_items( "novels", 60,  p.xy(), rs.xy(), false, dat.when() );
                m->place_items( "magazines", 20,  p.xy(), rs.xy(), false, dat.when() );
            } else if( rn == 6 ) {
                tripoint rs = p + furn_space::best_expand( *m, p, rng( 0, 2 ), 0 );
                square_furn( m, f_locker, p.x, p.y, rs.x, rs.y );
                m->place_items( "trash", 60, p.xy(), rs.xy(), false, dat.when() );
                m->place_items( "home_hw", 20, p.xy(), rs.xy(), false, dat.when() );
            } else if( rn == 7 ) {
                tripoint rs = p + furn_space::best_expand( *m, p, 0, rng( 0, 2 ) );
                square_furn( m, f_locker, p.x, p.y, rs.x, rs.y );
                m->place_items( "trash", 60, p.xy(), rs.xy(), false, dat.when() );
                m->place_items( "home_hw", 20, p.xy(), rs.xy(), false, dat.when() );
            } else {
                tripoint rs = p + furn_space::best_expand( *m, p, rng( 0, 2 ), rng( 0, 2 ) );
                square_furn( m, f_table, p.x, p.y, rs.x, rs.y );
            }
        }
    }

    m->place_items( "bedroom", 60, point_south_east, point( SEEX * 2 - 2, SEEY * 2 - 2 ), false,
                    dat.when() );
    m->place_items( "home_hw", 80, point_south_east, point( SEEX * 2 - 2, SEEY * 2 - 2 ), false,
                    dat.when() );
    m->place_items( "homeguns", 10, point_south_east, point( SEEX * 2 - 2, SEEY * 2 - 2 ), false,
                    dat.when() );
    // Chance of zombies in the basement
    m->place_spawns( mongroup_id( "GROUP_ZOMBIE" ), 2, point_south_east, point( SEEX * 2 - 2,
                     SEEY * 2 - 2 ), dat.monster_density() );
}

void mapgen_basement_spiders( mapgendata &dat )
{
    map *const m = &dat.m;
    // Oh no! A spider nest!
    mapgen_basement_junk( dat );
    auto spider_type = mon_spider_widow_giant;
    auto egg_type = f_egg_sackbw;
    if( one_in( 2 ) ) {
        spider_type = mon_spider_cellar_giant;
        egg_type = f_egg_sackcs;
    }
    for( int i = 1; i < 22; i++ ) {
        for( int j = 1; j < 22; j++ ) {
            if( m->ter( point( i, j ) ).obj().movecost == 0 ) {
                // Wall, skip
                continue;
            }

            if( !one_in( 3 ) ) {
                madd_field( m, i, j, fd_web, rng( 1, 3 ) );
            }
            if( one_in( 30 ) && m->passable( point( i, j ) ) ) {
                m->furn_set( point( i, j ), egg_type );
                m->add_spawn( spider_type, rng( 1, 2 ), point( i, j ) ); //hope you like'em spiders
                m->remove_field( { i, j, m->get_abs_sub().z }, fd_web );
            }
        }
    }
    m->place_items( "rare", 70, point_south_east, point( SEEX * 2 - 1, SEEY * 2 - 5 ), false,
                    dat.when() );
}

void mapgen_cave( mapgendata &dat )
{
    map *const m = &dat.m;
    if( dat.above() == "cave" ) {
        // We're underground! // FIXME: y u no use z-level
        for( int i = 0; i < SEEX * 2; i++ ) {
            for( int j = 0; j < SEEY * 2; j++ ) {
                bool floorHere = ( rng( 0, 6 ) < i || SEEX * 2 - rng( 1, 7 ) > i ||
                                   rng( 0, 6 ) < j || SEEY * 2 - rng( 1, 7 ) > j );
                if( floorHere ) {
                    m->ter_set( point( i, j ), t_rock_floor );
                } else {
                    m->ter_set( point( i, j ), t_rock );
                }
            }
        }
        square( m, t_slope_up, SEEX - 1, SEEY - 1, SEEX, SEEY );
        switch( rng( 1, 10 ) ) {
            case 1:
                // natural refuse, chance of minerals
                m->place_items( "cave_minerals", 50, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true,
                                dat.when() );
                m->place_items( "monparts", 80, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true, dat.when() );
                break;
            case 2:
                // trash, minerals less likely
                m->place_items( "cave_minerals", 25, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true,
                                dat.when() );
                m->place_items( "trash", 70, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true, dat.when() );
                break;
            case 3:
                // bat corpses
                m->place_items( "cave_minerals", 50, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true,
                                dat.when() );
                for( int i = rng( 1, 12 ); i > 0; i-- ) {
                    m->add_item_or_charges( point( rng( 1, SEEX * 2 - 1 ), rng( 1, SEEY * 2 - 1 ) ),
                                            item::make_corpse( mon_bat ) );
                }
                break;
            case 4:
                // ant food, chance of 80
                m->place_items( "cave_minerals", 25, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true,
                                dat.when() );
                m->place_items( "ant_food", 85, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true, dat.when() );
                break;
            case 5: {
                // hermitage
                int origx = rng( SEEX - 1, SEEX ),
                    origy = rng( SEEY - 1, SEEY ),
                    hermx = rng( SEEX - 6, SEEX + 5 ),
                    hermy = rng( SEEX - 6, SEEY + 5 );
                std::vector<point> bloodline = line_to( point( origx, origy ), point( hermx, hermy ), 0 );
                for( auto &ii : bloodline ) {
                    madd_field( m, ii.x, ii.y, fd_blood, 2 );
                }
                m->add_item_or_charges( point( hermx, hermy ), item::make_corpse() );
                // This seems verbose.  Maybe a function to spawn from a list of item groups?
                m->place_items( "stash_food", 50, point( hermx - 1, hermy - 1 ), point( hermx + 1, hermy + 1 ),
                                true, dat.when() );
                m->place_items( "gear_survival", 50, point( hermx - 1, hermy - 1 ), point( hermx + 1, hermy + 1 ),
                                true, dat.when() );
                m->place_items( "survival_armor", 50, point( hermx - 1, hermy - 1 ), point( hermx + 1, hermy + 1 ),
                                true, dat.when() );
                m->place_items( "weapons", 40, point( hermx - 1, hermy - 1 ), point( hermx + 1, hermy + 1 ), true,
                                dat.when() );
                m->place_items( "magazines", 40, point( hermx - 1, hermy - 1 ), point( hermx + 1, hermy + 1 ), true,
                                dat.when() );
                m->place_items( "rare", 30, point( hermx - 1, hermy - 1 ), point( hermx + 1, hermy + 1 ), true,
                                dat.when() );
                break;
            }
            default:
                // nothing except maybe minerals, default occurs half the time
                m->place_items( "cave_minerals", 50, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true,
                                dat.when() );
                break;
        }
        m->place_spawns( mongroup_id( "GROUP_CAVE" ), 2, point( 6, 6 ), point( 18, 18 ), 1.0 );
    } else { // We're above ground!
        // First, draw a forest
        mapgendata forest_mapgen_dat( dat, oter_str_id( "forest" ).id() );
        mapgen_forest( forest_mapgen_dat );
        // Clear the center with some rocks
        square( m, t_rock, SEEX - 6, SEEY - 6, SEEX + 5, SEEY + 5 );
        int pathx = 0;
        int pathy = 0;
        if( one_in( 2 ) ) {
            pathx = rng( SEEX - 6, SEEX + 5 );
            pathy = ( one_in( 2 ) ? SEEY - 8 : SEEY + 7 );
        } else {
            pathx = ( one_in( 2 ) ? SEEX - 8 : SEEX + 7 );
            pathy = rng( SEEY - 6, SEEY + 5 );
        }
        std::vector<point> pathline = line_to( point( pathx, pathy ), point( SEEX - 1, SEEY - 1 ), 0 );
        for( auto &ii : pathline ) {
            square( m, t_dirt, ii.x, ii.y,
                    ii.x + 1, ii.y + 1 );
        }
        while( !one_in( 8 ) ) {
            m->ter_set( point( rng( SEEX - 6, SEEX + 5 ), rng( SEEY - 6, SEEY + 5 ) ), t_dirt );
        }
        square( m, t_slope_down, SEEX - 1, SEEY - 1, SEEX, SEEY );
    }

}

void mapgen_cave_rat( mapgendata &dat )
{
    map *const m = &dat.m;

    fill_background( m, t_rock );

    if( dat.above() == "cave_rat" ) { // Finale
        rough_circle( m, t_rock_floor, SEEX, SEEY, 8 );
        square( m, t_rock_floor, SEEX - 1, SEEY, SEEX, SEEY * 2 - 2 );
        line( m, t_slope_up, SEEX - 1, SEEY * 2 - 3, SEEX, SEEY * 2 - 2 );
        for( int i = SEEX - 4; i <= SEEX + 4; i++ ) {
            for( int j = SEEY - 4; j <= SEEY + 4; j++ ) {
                if( ( i <= SEEX - 2 || i >= SEEX + 2 ) && ( j <= SEEY - 2 || j >= SEEY + 2 ) ) {
                    m->add_spawn( mon_sewer_rat, 1, point( i, j ) );
                }
            }
        }
        m->add_spawn( mon_rat_king, 1, point( SEEX, SEEY ) );
        m->place_items( "rare", 75, point( SEEX - 4, SEEY - 4 ), point( SEEX + 4, SEEY + 4 ), true,
                        dat.when() );
    } else { // Level 1
        int cavex = SEEX;
        int cavey = SEEY * 2 - 3;
        int stairsx = SEEX - 1, stairsy = 1; // Default stairs location--may change
        int centerx = 0;
        do {
            cavex += rng( -1, 1 );
            cavey -= rng( 0, 1 );
            for( int cx = cavex - 1; cx <= cavex + 1; cx++ ) {
                for( int cy = cavey - 1; cy <= cavey + 1; cy++ ) {
                    m->ter_set( point( cx, cy ), t_rock_floor );
                    if( one_in( 10 ) ) {
                        madd_field( m, cx, cy, fd_blood, rng( 1, 3 ) );
                    }
                    if( one_in( 20 ) ) {
                        m->add_spawn( mon_sewer_rat, 1, point( cx, cy ) );
                    }
                }
            }
            if( cavey == SEEY - 1 ) {
                centerx = cavex;
            }
        } while( cavey > 2 );
        // Now draw some extra passages!
        do {
            int tox = ( one_in( 2 ) ? 2 : SEEX * 2 - 3 ), toy = rng( 2, SEEY * 2 - 3 );
            std::vector<point> path = line_to( point( centerx, SEEY - 1 ), point( tox, toy ), 0 );
            for( auto &i : path ) {
                for( int cx = i.x - 1; cx <= i.x + 1; cx++ ) {
                    for( int cy = i.y - 1; cy <= i.y + 1; cy++ ) {
                        m->ter_set( point( cx, cy ), t_rock_floor );
                        if( one_in( 10 ) ) {
                            madd_field( m, cx, cy, fd_blood, rng( 1, 3 ) );
                        }
                        if( one_in( 20 ) ) {
                            m->add_spawn( mon_sewer_rat, 1, point( cx, cy ) );
                        }
                    }
                }
            }
            if( one_in( 2 ) ) {
                stairsx = tox;
                stairsy = toy;
            }
        } while( one_in( 2 ) );
        // Finally, draw the stairs up and down.
        m->ter_set( point( SEEX - 1, SEEX * 2 - 2 ), t_slope_up );
        m->ter_set( point( SEEX, SEEX * 2 - 2 ), t_slope_up );
        m->ter_set( point( stairsx, stairsy ), t_slope_down );
    }
}

void mapgen_cavern( mapgendata &dat )
{
    map *const m = &dat.m;

    for( int i = 0; i < 4;
         i++ ) { // FIXME: don't look at me like that, this was messed up before I touched it :P - AD
        dat.set_dir( i,
                     ( dat.t_nesw[i] == "cavern" || dat.t_nesw[i] == "subway_ns" ||
                       dat.t_nesw[i] == "subway_ew" ? 0 : 3 )
                   );
    }
    dat.e_fac = SEEX * 2 - 1 - dat.e_fac;
    dat.s_fac = SEEY * 2 - 1 - dat.s_fac;

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( ( j < dat.n_fac || j > dat.s_fac || i < dat.w_fac || i > dat.e_fac ) &&
                ( !one_in( 3 ) || j == 0 || j == SEEY * 2 - 1 || i == 0 || i == SEEX * 2 - 1 ) ) {
                m->ter_set( point( i, j ), t_rock );
            } else {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
    }

    int rn = rng( 0, 2 ) * rng( 0, 3 ) + rng( 0, 1 ); // Number of pillars
    for( int n = 0; n < rn; n++ ) {
        int px = rng( 5, SEEX * 2 - 6 );
        int py = rng( 5, SEEY * 2 - 6 );
        for( int i = px - 1; i <= px + 1; i++ ) {
            for( int j = py - 1; j <= py + 1; j++ ) {
                m->ter_set( point( i, j ), t_rock );
            }
        }
    }

    if( connects_to( dat.north(), 2 ) ) {
        for( int i = SEEX - 2; i <= SEEX + 3; i++ ) {
            for( int j = 0; j <= SEEY; j++ ) {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
    }
    if( connects_to( dat.east(), 3 ) ) {
        for( int i = SEEX; i <= SEEX * 2 - 1; i++ ) {
            for( int j = SEEY - 2; j <= SEEY + 3; j++ ) {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
    }
    if( connects_to( dat.south(), 0 ) ) {
        for( int i = SEEX - 2; i <= SEEX + 3; i++ ) {
            for( int j = SEEY; j <= SEEY * 2 - 1; j++ ) {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
    }
    if( connects_to( dat.west(), 1 ) ) {
        for( int i = 0; i <= SEEX; i++ ) {
            for( int j = SEEY - 2; j <= SEEY + 3; j++ ) {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
    }
    m->place_items( "cavern", 60, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), false, dat.when() );
    if( one_in( 6 ) ) { // Miner remains
        int x = 0;
        int y = 0;
        do {
            x = rng( 0, SEEX * 2 - 1 );
            y = rng( 0, SEEY * 2 - 1 );
        } while( m->impassable( point( x, y ) ) );
        if( !one_in( 3 ) ) {
            m->spawn_item( point( x, y ), "jackhammer" );
        }
        if( one_in( 3 ) ) {
            m->spawn_item( point( x, y ), "mask_dust" );
        }
        if( one_in( 2 ) ) {
            m->spawn_item( point( x, y ), "hat_hard" );
        }
        while( !one_in( 3 ) ) {
            for( int i = 0; i < 3; ++i ) {
                m->put_items_from_loc( "cannedfood", tripoint( x, y, m->get_abs_sub().z ), dat.when() );
            }
        }
    }

}

void mapgen_rock_partial( mapgendata &dat )
{
    map *const m = &dat.m;
    fill_background( m, t_rock );
    for( int i = 0; i < 4; i++ ) {
        if( dat.t_nesw[i] == "cavern" || dat.t_nesw[i] == "slimepit" ||
            dat.t_nesw[i] == "slimepit_down" ) {
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

    if( dat.north() != "rift" && dat.north() != "hellmouth" ) {
        if( connects_to( dat.north(), 2 ) ) {
            dat.n_fac = rng( -6, -2 );
        } else {
            dat.n_fac = rng( 2, 6 );
        }
    }
    if( dat.east() != "rift" && dat.east() != "hellmouth" ) {
        if( connects_to( dat.east(), 3 ) ) {
            dat.e_fac = rng( -6, -2 );
        } else {
            dat.e_fac = rng( 2, 6 );
        }
    }
    if( dat.south() != "rift" && dat.south() != "hellmouth" ) {
        if( connects_to( dat.south(), 0 ) ) {
            dat.s_fac = rng( -6, -2 );
        } else {
            dat.s_fac = rng( 2, 6 );
        }
    }
    if( dat.west() != "rift" && dat.west() != "hellmouth" ) {
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
        if( dat.t_nesw[i] != "rift" && dat.t_nesw[i] != "hellmouth" ) {
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

void mapgen_ants_curved( mapgendata &dat )
{
    map *const m = &dat.m;
    int x = SEEX;
    int y = 1;
    int rn = 0;
    // First, set it all to rock
    fill_background( m, t_rock );

    for( int i = SEEX - 2; i <= SEEX + 3; i++ ) {
        m->ter_set( point( i, 0 ), t_rock_floor );
        m->ter_set( point( i, 1 ), t_rock_floor );
        m->ter_set( point( i, 2 ), t_rock_floor );
        m->ter_set( point( SEEX * 2 - 1, i ), t_rock_floor );
        m->ter_set( point( SEEX * 2 - 2, i ), t_rock_floor );
        m->ter_set( point( SEEX * 2 - 3, i ), t_rock_floor );
    }
    do {
        for( int i = x - 2; i <= x + 3; i++ ) {
            for( int j = y - 2; j <= y + 3; j++ ) {
                if( i > 0 && i < SEEX * 2 - 1 && j > 0 && j < SEEY * 2 - 1 ) {
                    m->ter_set( point( i, j ), t_rock_floor );
                }
            }
        }
        if( rn < SEEX ) {
            x += rng( -1, 1 );
            y++;
        } else {
            x++;
            if( !one_in( x - SEEX ) ) {
                y += rng( -1, 1 );
            } else if( y < SEEY ) {
                y++;
            } else if( y > SEEY ) {
                y--;
            }
        }
        rn++;
    } while( x < SEEX * 2 - 1 || y != SEEY );
    for( int i = x - 2; i <= x + 3; i++ ) {
        for( int j = y - 2; j <= y + 3; j++ ) {
            if( i > 0 && i < SEEX * 2 - 1 && j > 0 && j < SEEY * 2 - 1 ) {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
    }
    if( dat.terrain_type() == "ants_es" ) {
        m->rotate( 1 );
    }
    if( dat.terrain_type() == "ants_sw" ) {
        m->rotate( 2 );
    }
    if( dat.terrain_type() == "ants_wn" ) {
        m->rotate( 3 );
    }

}

void mapgen_ants_four_way( mapgendata &dat )
{
    map *const m = &dat.m;
    fill_background( m, t_rock );
    int x = SEEX;
    for( int j = 0; j < SEEY * 2; j++ ) {
        for( int i = x - 2; i <= x + 3; i++ ) {
            if( i >= 1 && i < SEEX * 2 - 1 ) {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
        x += rng( -1, 1 );
        while( abs( SEEX - x ) > SEEY * 2 - j - 1 ) {
            if( x < SEEX ) {
                x++;
            }
            if( x > SEEX ) {
                x--;
            }
        }
    }

    int y = SEEY;
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = y - 2; j <= y + 3; j++ ) {
            if( j >= 1 && j < SEEY * 2 - 1 ) {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
        y += rng( -1, 1 );
        while( abs( SEEY - y ) > SEEX * 2 - i - 1 ) {
            if( y < SEEY ) {
                y++;
            }
            if( y > SEEY ) {
                y--;
            }
        }
    }

}

void mapgen_ants_straight( mapgendata &dat )
{
    map *const m = &dat.m;
    int x = SEEX;
    fill_background( m, t_rock );
    for( int j = 0; j < SEEY * 2; j++ ) {
        for( int i = x - 2; i <= x + 3; i++ ) {
            if( i >= 1 && i < SEEX * 2 - 1 ) {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
        x += rng( -1, 1 );
        while( abs( SEEX - x ) > SEEX * 2 - j - 1 ) {
            if( x < SEEX ) {
                x++;
            }
            if( x > SEEX ) {
                x--;
            }
        }
    }
    if( dat.terrain_type() == "ants_ew" ) {
        m->rotate( 1 );
    }

}

void mapgen_ants_tee( mapgendata &dat )
{
    map *const m = &dat.m;
    fill_background( m, t_rock );
    int x = SEEX;
    for( int j = 0; j < SEEY * 2; j++ ) {
        for( int i = x - 2; i <= x + 3; i++ ) {
            if( i >= 1 && i < SEEX * 2 - 1 ) {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
        x += rng( -1, 1 );
        while( abs( SEEX - x ) > SEEY * 2 - j - 1 ) {
            if( x < SEEX ) {
                x++;
            }
            if( x > SEEX ) {
                x--;
            }
        }
    }
    int y = SEEY;
    for( int i = SEEX; i < SEEX * 2; i++ ) {
        for( int j = y - 2; j <= y + 3; j++ ) {
            if( j >= 1 && j < SEEY * 2 - 1 ) {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
        y += rng( -1, 1 );
        while( abs( SEEY - y ) > SEEX * 2 - 1 - i ) {
            if( y < SEEY ) {
                y++;
            }
            if( y > SEEY ) {
                y--;
            }
        }
    }
    if( dat.terrain_type() == "ants_new" ) {
        m->rotate( 3 );
    }
    if( dat.terrain_type() == "ants_nsw" ) {
        m->rotate( 2 );
    }
    if( dat.terrain_type() == "ants_esw" ) {
        m->rotate( 1 );
    }

}

static void mapgen_ants_generic( mapgendata &dat )
{
    map *const m = &dat.m;

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( i < SEEX - 4 || i > SEEX + 5 || j < SEEY - 4 || j > SEEY + 5 ) {
                m->ter_set( point( i, j ), t_rock );
            } else {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
    }
    int rn = rng( 10, 20 );
    int x = 0;
    int y = 0;
    for( int n = 0; n < rn; n++ ) {
        int cw = rng( 1, 8 );
        do {
            x = rng( 1 + cw, SEEX * 2 - 2 - cw );
            y = rng( 1 + cw, SEEY * 2 - 2 - cw );
        } while( m->ter( point( x, y ) ) == t_rock );
        for( int i = x - cw; i <= x + cw; i++ ) {
            for( int j = y - cw; j <= y + cw; j++ ) {
                if( trig_dist( point( x, y ), point( i, j ) ) <= cw ) {
                    m->ter_set( point( i, j ), t_rock_floor );
                }
            }
        }
    }
    if( connects_to( dat.north(), 2 ) ||
        is_ot_match( "ants_lab", dat.north(), ot_match_type::contains ) ) {
        for( int i = SEEX - 2; i <= SEEX + 3; i++ ) {
            for( int j = 0; j <= SEEY; j++ ) {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
    }
    if( connects_to( dat.east(), 3 ) ||
        is_ot_match( "ants_lab", dat.east(), ot_match_type::contains ) ) {
        for( int i = SEEX; i <= SEEX * 2 - 1; i++ ) {
            for( int j = SEEY - 2; j <= SEEY + 3; j++ ) {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
    }
    if( connects_to( dat.south(), 0 ) ||
        is_ot_match( "ants_lab", dat.south(), ot_match_type::contains ) ) {
        for( int i = SEEX - 2; i <= SEEX + 3; i++ ) {
            for( int j = SEEY; j <= SEEY * 2 - 1; j++ ) {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
    }
    if( connects_to( dat.west(), 1 ) ||
        is_ot_match( "ants_lab", dat.west(), ot_match_type::contains ) ) {
        for( int i = 0; i <= SEEX; i++ ) {
            for( int j = SEEY - 2; j <= SEEY + 3; j++ ) {
                m->ter_set( point( i, j ), t_rock_floor );
            }
        }
    }
    if( dat.terrain_type() == "ants_food" ) {
        m->place_items( "ant_food", 92, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true, dat.when() );
    } else {
        m->place_items( "ant_egg",  98, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true, dat.when() );
    }
    if( dat.terrain_type() == "ants_queen" ) {
        m->add_spawn( mon_ant_queen, 1, point( SEEX, SEEY ) );
    } else if( dat.terrain_type() == "ants_larvae" ) {
        m->add_spawn( mon_ant_larva, 10, point( SEEX, SEEY ) );
    }

}

void mapgen_ants_food( mapgendata &dat )
{
    mapgen_ants_generic( dat );
    dat.m.place_items( "ant_food", 92, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true,
                       dat.when() );
}

void mapgen_ants_larvae( mapgendata &dat )
{
    mapgen_ants_generic( dat );
    dat.m.place_items( "ant_egg",  98, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true,
                       dat.when() );
    dat.m.add_spawn( mon_ant_larva, 10, point( SEEX, SEEY ) );
}

void mapgen_ants_queen( mapgendata &dat )
{
    mapgen_ants_generic( dat );
    dat.m.place_items( "ant_egg",  98, point_zero, point( SEEX * 2 - 1, SEEY * 2 - 1 ), true,
                       dat.when() );
    dat.m.add_spawn( mon_ant_queen, 1, point( SEEX, SEEY ) );
}

void mapgen_tutorial( mapgendata &dat )
{
    map *const m = &dat.m;
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( j == 0 || j == SEEY * 2 - 1 ) {
                m->ter_set( point( i, j ), t_wall );
            } else if( i == 0 || i == SEEX * 2 - 1 ) {
                m->ter_set( point( i, j ), t_wall );
            } else if( j == SEEY ) {
                if( i % 4 == 2 ) {
                    m->ter_set( point( i, j ), t_door_c );
                } else if( i % 5 == 3 ) {
                    m->ter_set( point( i, j ), t_window_domestic );
                } else {
                    m->ter_set( point( i, j ), t_wall );
                }
            } else {
                m->ter_set( point( i, j ), t_floor );
            }
        }
    }
    m->furn_set( point( 7, SEEY * 2 - 4 ), f_rack );
    m->place_gas_pump( point( SEEX * 2 - 2, SEEY * 2 - 4 ), rng( 500, 1000 ) );
    if( dat.zlevel() < 0 ) {
        m->ter_set( point( SEEX - 2, SEEY + 2 ), t_stairs_up );
        m->ter_set( point( 2, 2 ), t_water_sh );
        m->ter_set( point( 2, 3 ), t_water_sh );
        m->ter_set( point( 3, 2 ), t_water_sh );
        m->ter_set( point( 3, 3 ), t_water_sh );
    } else {
        m->spawn_item( point( 5, SEEY + 1 ), "helmet_bike" );
        m->spawn_item( point( 4, SEEY + 1 ), "backpack" );
        m->spawn_item( point( 3, SEEY + 1 ), "pants_cargo" );
        m->spawn_item( point( 7, SEEY * 2 - 4 ), "machete" );
        m->spawn_item( point( 7, SEEY * 2 - 4 ), "9mm" );
        m->spawn_item( point( 7, SEEY * 2 - 4 ), "9mmP" );
        m->spawn_item( point( 7, SEEY * 2 - 4 ), "uzi" );
        m->spawn_item( point( 7, SEEY * 2 - 4 ), "uzimag" );
        m->spawn_item( point( SEEX * 2 - 2, SEEY + 5 ), "bubblewrap" );
        m->spawn_item( point( SEEX * 2 - 2, SEEY + 6 ), "grenade" );
        m->spawn_item( point( SEEX * 2 - 3, SEEY + 6 ), "flashlight" );
        m->spawn_item( point( SEEX * 2 - 2, SEEY + 7 ), "cig" );
        m->spawn_item( point( SEEX * 2 - 2, SEEY + 7 ), "codeine" );
        m->spawn_item( point( SEEX * 2 - 3, SEEY + 7 ), "water" );
        m->ter_set( point( SEEX - 2, SEEY + 2 ), t_stairs_down );
    }
}

void mapgen_forest( mapgendata &dat )
{
    map *const m = &dat.m;
    // Adjacency factor is basically used to weight the frequency of a feature
    // being placed by the relative sparseness of the current terrain to its
    // neighbors. For example, a forest_thick surrounded by forest_thick on
    // all sides can be much more dense than a forest_water surrounded by
    // fields on all sides. It's a little magic-number-y but somewhat replicates
    // the behavior of the previous forest mapgen when fading forest terrains
    // into each other and non-forest terrains.

    const auto get_sparseness_adjacency_factor = [&dat]( const oter_id & ot ) {
        const auto biome = dat.region.forest_composition.biomes.find( ot );
        if( biome == dat.region.forest_composition.biomes.end() ) {
            // If there is no defined biome for this oter, use 0. It's possible
            // to specify biomes in the forest regional settings that are not
            // rendered by this forest map gen method, in order to control
            // how terrains are blended together (e.g. specify roads with an equal
            // sparsness adjacency factor to forests so that forests don't fade out
            // as they transition to roads.
            return 0;
        }
        return biome->second.sparseness_adjacency_factor;
    };

    const auto fill_adjacency_factor = [&dat, &get_sparseness_adjacency_factor]( int self_factor ) {
        dat.fill( self_factor );
        for( int i = 0; i < 4; i++ ) {
            dat.dir( i ) += get_sparseness_adjacency_factor( dat.t_nesw[i] );
        }
    };

    const ter_furn_id no_ter_furn = ter_furn_id();

    const auto get_feature_for_neighbor = [&dat,
                                           &no_ter_furn]( const std::map<oter_id, ter_furn_id> &biome_features,
    const om_direction::type dir ) {
        const oter_id dir_ot = dat.neighbor_at( dir );
        const auto feature = biome_features.find( dir_ot );
        if( feature == biome_features.end() ) {
            // If we have no biome for this neighbor, then we just return any empty feature.
            // As with the sparseness adjacency factor, it's possible to define non-forest
            // biomes in the regional settings so that they provide neighbor features
            // here for blending purposes (e.g. define dirt terrain for roads so that the
            // ground fades from forest ground cover to dirt as blends with roads.
            return no_ter_furn;
        }
        return feature->second;
    };

    // The max sparseness is calculated across all the possible biomes, not just the adjacent ones.
    const auto get_max_sparseness_adjacency_factor = [&dat]() {
        if( dat.region.forest_composition.biomes.empty() ) {
            return 0;
        }
        std::vector<int> factors;
        for( auto &b : dat.region.forest_composition.biomes ) {
            factors.push_back( b.second.sparseness_adjacency_factor );
        }
        return *max_element( std::begin( factors ), std::end( factors ) );
    };

    // Get the sparesness factor for this terrain, and fill it.
    const int factor = get_sparseness_adjacency_factor( dat.terrain_type() );
    fill_adjacency_factor( factor );

    const int max_factor = get_max_sparseness_adjacency_factor();

    // Our margins for blending divide the overmap terrain into nine sections.
    static constexpr int margin_x = SEEX * 2 / 3;
    static constexpr int margin_y = SEEY * 2 / 3;

    const auto get_blended_feature = [&no_ter_furn, &max_factor, &factor,
                  &get_feature_for_neighbor, &dat]( int x, int y ) {
        // Pick one random feature from each biome according to the biome defs and save it into a lookup.
        // We'll blend these features together below based on the current and adjacent terrains.
        std::map<oter_id, ter_furn_id> biome_features;
        for( auto &b : dat.region.forest_composition.biomes ) {
            biome_features[b.first] = b.second.pick();
        }

        // Get a feature for ourself and each of the adjacent overmap terrains.
        const ter_furn_id east_feature = get_feature_for_neighbor( biome_features,
                                         om_direction::type::east );
        const ter_furn_id west_feature = get_feature_for_neighbor( biome_features,
                                         om_direction::type::west );
        const ter_furn_id north_feature = get_feature_for_neighbor( biome_features,
                                          om_direction::type::north );
        const ter_furn_id south_feature = get_feature_for_neighbor( biome_features,
                                          om_direction::type::south );
        const ter_furn_id self_feature = biome_features[dat.terrain_type()];

        // We'll use our margins and the four adjacent overmap terrains to pick a blended
        // feature based on the features we picked above and a linear weight as we
        // transition through the margins.
        //
        // (0,0)     NORTH
        //      ---------------
        //      | NW | W | NE |
        //      |----|---|----|
        // WEST | W  |   | E  |  EAST
        //      |----|---|----|
        //      | SW | S | SE |
        //      ---------------
        //           SOUTH      (SEEX * 2, SEEY * 2)

        const int west_weight = std::max( margin_x - x, 0 );
        const int east_weight = std::max( x - ( SEEX * 2 - margin_x ) + 1, 0 );
        const int north_weight = std::max( margin_y - y, 0 );
        const int south_weight = std::max( y - ( SEEY * 2 - margin_y ) + 1, 0 );

        // We'll build a weighted list of features to pull from at the end.
        weighted_int_list<const ter_furn_id> feature_pool;

        // W sections
        if( x < margin_x ) {
            // NW corner - blend N, W, and self
            if( y < margin_y ) {
                feature_pool.add( no_ter_furn, 3 * max_factor - ( dat.n_fac + dat.w_fac + factor * 2 ) );
                feature_pool.add( self_feature, 1 );
                feature_pool.add( west_feature, west_weight );
                feature_pool.add( north_feature, north_weight );
            }
            // SW corner - blend S, W, and self
            else if( y > SEEY * 2 - margin_y ) {
                feature_pool.add( no_ter_furn, 3 * max_factor - ( dat.s_fac + dat.w_fac + factor * 2 ) );
                feature_pool.add( self_feature, factor );
                feature_pool.add( west_feature, west_weight );
                feature_pool.add( south_feature, south_weight );
            }
            // W edge - blend W and self
            else {
                feature_pool.add( no_ter_furn, 2 * max_factor - ( dat.w_fac + factor * 2 ) );
                feature_pool.add( self_feature, factor );
                feature_pool.add( west_feature, west_weight );
            }
        }
        // E sections
        else if( x > SEEX * 2 - margin_x ) {
            // NE corner - blend N, E, and self
            if( y < margin_y ) {
                feature_pool.add( no_ter_furn, 3 * max_factor - ( dat.n_fac + dat.e_fac + factor * 2 ) );
                feature_pool.add( self_feature, factor );
                feature_pool.add( east_feature, east_weight );
                feature_pool.add( north_feature, north_weight );
            }
            // SE corner - blend S, E, and self
            else if( y > SEEY * 2 - margin_y ) {
                feature_pool.add( no_ter_furn, 3 * max_factor - ( dat.s_fac + dat.e_fac + factor * 2 ) );
                feature_pool.add( self_feature, factor );
                feature_pool.add( east_feature, east_weight );
                feature_pool.add( south_feature, south_weight );
            }
            // E edge - blend E and self
            else {
                feature_pool.add( no_ter_furn, 2 * max_factor - ( dat.e_fac + factor * 2 ) );
                feature_pool.add( self_feature, factor );
                feature_pool.add( east_feature, east_weight );
            }
        }
        // Central sections
        else {
            // N edge - blend N and self
            if( y < margin_y ) {
                feature_pool.add( no_ter_furn, 2 * max_factor - ( dat.n_fac + factor * 2 ) );
                feature_pool.add( self_feature, factor );
                feature_pool.add( north_feature, north_weight );
            }
            // S edge - blend S, and self
            else if( y > SEEY * 2 - margin_y ) {
                feature_pool.add( no_ter_furn, 2 * max_factor - ( dat.s_fac + factor * 2 ) );
                feature_pool.add( self_feature, factor );
                feature_pool.add( south_feature, south_weight );
            }
            // center - no blending
            else {
                feature_pool.add( no_ter_furn, max_factor - factor * 2 );
                feature_pool.add( self_feature, factor );
            }
        }

        // Pick a single feature from the pool we built above and return it.
        const ter_furn_id *feature = feature_pool.pick();
        if( feature == nullptr ) {
            return no_ter_furn;
        } else {
            return *feature;
        }
    };

    // Get the current biome def for this terrain.
    const auto current_biome_def_it = dat.region.forest_composition.biomes.find( dat.terrain_type() );

    // If there is no biome def for this terrain, fill in with the region's default ground cover
    // and bail--nothing more to be done.
    if( current_biome_def_it == dat.region.forest_composition.biomes.end() ) {
        dat.fill_groundcover();
        return;
    }

    const forest_biome current_biome_def = current_biome_def_it->second;

    // If this biome does not define its own groundcover, then fill with the region's ground
    // cover. Otherwise, fill with the biome defs groundcover.
    if( current_biome_def.groundcover.empty() ) {
        dat.fill_groundcover();
    } else {
        m->draw_fill_background( current_biome_def.groundcover );
    }

    // There is a chance of placing terrain dependent furniture, e.g. f_cattails on t_water_sh.
    const auto set_terrain_dependent_furniture = [&current_biome_def, &m]( const ter_id & tid,
    const int x, const int y ) {
        const auto terrain_dependent_furniture_it = current_biome_def.terrain_dependent_furniture.find(
                    tid );
        if( terrain_dependent_furniture_it == current_biome_def.terrain_dependent_furniture.end() ) {
            // No terrain dependent furnitures for this terrain, so bail.
            return;
        }

        const forest_biome_terrain_dependent_furniture tdf = terrain_dependent_furniture_it->second;
        if( tdf.furniture.get_weight() <= 0 ) {
            // We've got furnitures, but their weight is 0 or less, so bail.
            return;
        }

        if( one_in( tdf.chance ) ) {
            // Pick a furniture and set it on the map right now.
            const auto fid = tdf.furniture.pick();
            m->furn_set( point( x, y ), *fid );
        }
    };

    // Loop through each location in this overmap terrain and attempt to place a feature and
    // terrain dependent furniture.
    for( int x = 0; x < SEEX * 2; x++ ) {
        for( int y = 0; y < SEEY * 2; y++ ) {
            const ter_furn_id feature = get_blended_feature( x, y );
            ter_or_furn_set( m, x, y, feature );
            set_terrain_dependent_furniture( feature.ter, x, y );
        }
    }

    // Place items on this terrain as defined in the biome.
    for( int i = 0; i < current_biome_def.item_spawn_iterations; i++ ) {
        m->place_items( current_biome_def.item_group, current_biome_def.item_group_chance, point_zero,
                        point( SEEX * 2 - 1, SEEY * 2 - 1 ), true, dat.when() );
    }
}

void mapgen_forest_trail_straight( mapgendata &dat )
{
    map *const m = &dat.m;
    mapgendata forest_mapgen_dat( dat, oter_str_id( "forest_thick" ).id() );
    mapgen_forest( forest_mapgen_dat );

    const auto center_offset = [&dat]() {
        return rng( -dat.region.forest_trail.trail_center_variance,
                    dat.region.forest_trail.trail_center_variance );
    };

    const auto width_offset = [&dat]() {
        return rng( dat.region.forest_trail.trail_width_offset_min,
                    dat.region.forest_trail.trail_width_offset_max );
    };

    int center_x = SEEX + center_offset();
    int center_y = SEEY + center_offset();

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( i > center_x - width_offset() && i < center_x + width_offset() ) {
                m->furn_set( point( i, j ), f_null );
                m->ter_set( point( i, j ), *dat.region.forest_trail.trail_terrain.pick() );
            }
        }
    }

    if( dat.terrain_type() == "forest_trail_ew" || dat.terrain_type() == "forest_trail_end_east" ||
        dat.terrain_type() == "forest_trail_end_west" ) {
        m->rotate( 1 );
    }

    m->place_items( "forest_trail", 75, point( center_x - 2, center_y - 2 ), point( center_x + 2,
                    center_y + 2 ), true,
                    dat.when() );
}

void mapgen_forest_trail_curved( mapgendata &dat )
{
    map *const m = &dat.m;
    mapgendata forest_mapgen_dat( dat, oter_str_id( "forest_thick" ).id() );
    mapgen_forest( forest_mapgen_dat );

    const auto center_offset = [&dat]() {
        return rng( -dat.region.forest_trail.trail_center_variance,
                    dat.region.forest_trail.trail_center_variance );
    };

    const auto width_offset = [&dat]() {
        return rng( dat.region.forest_trail.trail_width_offset_min,
                    dat.region.forest_trail.trail_width_offset_max );
    };

    int center_x = SEEX + center_offset();
    int center_y = SEEY + center_offset();

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( ( i > center_x - width_offset() && i < center_x + width_offset() &&
                  j < center_y + width_offset() ) ||
                ( j > center_y - width_offset() && j < center_y + width_offset() &&
                  i > center_x - width_offset() ) ) {
                m->furn_set( point( i, j ), f_null );
                m->ter_set( point( i, j ), *dat.region.forest_trail.trail_terrain.pick() );
            }
        }
    }

    if( dat.terrain_type() == "forest_trail_es" ) {
        m->rotate( 1 );
    }
    if( dat.terrain_type() == "forest_trail_sw" ) {
        m->rotate( 2 );
    }
    if( dat.terrain_type() == "forest_trail_wn" ) {
        m->rotate( 3 );
    }

    m->place_items( "forest_trail", 75, point( center_x - 2, center_y - 2 ), point( center_x + 2,
                    center_y + 2 ), true,
                    dat.when() );
}

void mapgen_forest_trail_tee( mapgendata &dat )
{
    map *const m = &dat.m;
    mapgendata forest_mapgen_dat( dat, oter_str_id( "forest_thick" ).id() );
    mapgen_forest( forest_mapgen_dat );

    const auto center_offset = [&dat]() {
        return rng( -dat.region.forest_trail.trail_center_variance,
                    dat.region.forest_trail.trail_center_variance );
    };

    const auto width_offset = [&dat]() {
        return rng( dat.region.forest_trail.trail_width_offset_min,
                    dat.region.forest_trail.trail_width_offset_max );
    };

    int center_x = SEEX + center_offset();
    int center_y = SEEY + center_offset();

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( ( i > center_x - width_offset() && i < center_x + width_offset() ) ||
                ( j > center_y - width_offset() &&
                  j < center_y + width_offset() && i > center_x - width_offset() ) ) {
                m->furn_set( point( i, j ), f_null );
                m->ter_set( point( i, j ), *dat.region.forest_trail.trail_terrain.pick() );
            }
        }
    }

    if( dat.terrain_type() == "forest_trail_esw" ) {
        m->rotate( 1 );
    }
    if( dat.terrain_type() == "forest_trail_nsw" ) {
        m->rotate( 2 );
    }
    if( dat.terrain_type() == "forest_trail_new" ) {
        m->rotate( 3 );
    }

    m->place_items( "forest_trail", 75, point( center_x - 2, center_y - 2 ), point( center_x + 2,
                    center_y + 2 ), true,
                    dat.when() );
}

void mapgen_forest_trail_four_way( mapgendata &dat )
{
    map *const m = &dat.m;
    mapgendata forest_mapgen_dat( dat, oter_str_id( "forest_thick" ).id() );
    mapgen_forest( forest_mapgen_dat );

    const auto center_offset = [&dat]() {
        return rng( -dat.region.forest_trail.trail_center_variance,
                    dat.region.forest_trail.trail_center_variance );
    };

    const auto width_offset = [&dat]() {
        return rng( dat.region.forest_trail.trail_width_offset_min,
                    dat.region.forest_trail.trail_width_offset_max );
    };

    int center_x = SEEX + center_offset();
    int center_y = SEEY + center_offset();

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            if( ( i > center_x - width_offset() && i < center_x + width_offset() ) ||
                ( j > center_y - width_offset() &&
                  j < center_y + width_offset() ) ) {
                m->furn_set( point( i, j ), f_null );
                m->ter_set( point( i, j ), *dat.region.forest_trail.trail_terrain.pick() );
            }
        }
    }

    m->place_items( "forest_trail", 75, point( center_x - 2, center_y - 2 ), point( center_x + 2,
                    center_y + 2 ), true,
                    dat.when() );
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
            for( auto &alias : dat.region.overmap_lake.shore_extendable_overmap_terrain_aliases ) {
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

    const oter_id river_center( "river_center" );

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
    // are welcome. The basic jist is as follows:
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
    static constexpr point nw_corner( point_zero );
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

    static constexpr rectangle map_boundaries( nw_corner, se_corner );

    // This will draw our shallow water coastline from the "from" point to the "to" point.
    // It buffers the points a bit for a thicker line. It also clears any furniture that might
    // be in the location as a result of our extending adjacent mapgen.
    const auto draw_shallow_water = [&]( const point & from, const point & to ) {
        std::vector<point> points = line_to( from, to );
        for( auto &p : points ) {
            std::vector<point> buffered_points = closest_points_first( 1, p );
            for( const point &bp : buffered_points ) {
                if( !map_boundaries.contains_inclusive( bp ) ) {
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
        if( !map_boundaries.contains_inclusive( p ) ) {
            return false;
        }
        return m->ter( p ) != t_null;
    };

    const auto fill_deep_water = [&]( const point & starting_point ) {
        std::vector<point> water_points = ff::point_flood_fill_4_connected( starting_point, visited,
                                          should_fill );
        for( auto &wp : water_points ) {
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

void mremove_trap( map *m, int x, int y )
{
    tripoint actual_location( x, y, m->get_abs_sub().z );
    m->remove_trap( actual_location );
}

void mtrap_set( map *m, int x, int y, trap_id type )
{
    tripoint actual_location( x, y, m->get_abs_sub().z );
    m->trap_set( actual_location, type );
}

void madd_field( map *m, int x, int y, field_type_id type, int intensity )
{
    tripoint actual_location( x, y, m->get_abs_sub().z );
    m->add_field( actual_location, type, intensity, 0_turns );
}

static bool is_suitable_for_stairs( const map *const m, const tripoint &p )
{
    const ter_t &p_ter = m->ter( p ).obj();

    return
        p_ter.has_flag( "INDOORS" ) &&
        p_ter.has_flag( "FLAT" ) &&
        m->furn( p ) == f_null;
}

static void stairs_debug_log( const map *const m, const std::string &msg, const tripoint &p,
                              DebugLevel level = D_INFO )
{
    const ter_t &p_ter = m->ter( p ).obj();

    dbg( level )
            << msg
            << " tripoint: " << p
            << " terrain: " << p_ter.name()
            << " movecost: " << p_ter.movecost
            << " furniture: " << m->furn( p )
            << " indoors: " << p_ter.has_flag( "INDOORS" )
            << " flat: " << p_ter.has_flag( "FLAT" )
            ;
}

void place_stairs( mapgendata &dat )
{
    if( !dat.has_basement() ) {
        return;
    }

    map *const m = &dat.m;
    const bool force = get_option<bool>( "ALIGN_STAIRS" );

    const tripoint abs_sub_here = m->get_abs_sub();

    tinymap basement;
    basement.load( tripoint( abs_sub_here.xy(), abs_sub_here.z - 1 ), false );

    const tripoint from( 0, 0, abs_sub_here.z );
    const tripoint to( SEEX * 2, SEEY * 2, abs_sub_here.z );
    tripoint_range tr = m->points_in_rectangle( from, to );
    std::vector<tripoint> stairs;
    std::vector<tripoint> tripoints;

    // Find the basement's stairs first.
    for( auto &&p : tr ) { // *NOPAD*
        if( basement.has_flag( TFLAG_GOES_UP, p + tripoint_below ) ) {
            const tripoint rotated = om_direction::rotate( p, dat.terrain_type()->get_dir() );
            stairs.emplace_back( rotated );
            stairs_debug_log( m, "basement stairs:", rotated );
        }

        if( is_suitable_for_stairs( m, p ) ) {
            tripoints.emplace_back( p );
        }
    }

    if( stairs.empty() ) {
        dbg( D_INFO ) << "no stairs found downstairs";
        return;
    }

    // Shuffle tripoints so that the stairs are not always similarly placed.
    std::shuffle( std::begin( tripoints ), std::end( tripoints ), rng_get_engine() );

    bool all_can_be_placed = false;
    tripoint shift;
    int match_count = 0;

    // Find a tripoint where all the underground tripoints for stairs are on
    // suitable locations aboveground.
    for( auto &&p : tripoints ) { // *NOPAD*
        int count = 1;
        all_can_be_placed = true;
        stairs_debug_log( m, "ok first:", p );

        // First element can be ignored as p is already ok.
        for( auto it = stairs.begin() + 1; it != stairs.end(); ++it ) {
            // Align tripoint with the first underground tripoint.
            const tripoint &stair = *it - stairs.front() + p;

            if( !is_suitable_for_stairs( m, stair ) ) {
                stairs_debug_log( m, "not ok:", stair );
                all_can_be_placed = false;

                if( match_count < count ) {
                    match_count = count;
                    shift = p - stairs.front();
                    dbg( D_INFO ) << "partial match shift tripoint: " << shift;
                }

                break;
            }

            stairs_debug_log( m, "ok:", stair );
            ++count;
        }

        if( all_can_be_placed ) {
            shift = p - stairs.front();
            dbg( D_INFO ) << "full match shift tripoint: " << shift;
            break;
        }
    }

    if( !( all_can_be_placed || force ) ) {
        dbg( D_WARNING ) << "no stairs were placed";
        return;
    }

    if( !all_can_be_placed ) {
        dbg( D_WARNING ) << "Some stairs can be placed to suitable locations "
                         << "and the rest may end up in odd locations.";
    }

    for( auto &&p : stairs ) { // *NOPAD*
        tripoint stair = p + shift;

        if( m->ter_set( stair, t_stairs_down ) ) {
            stairs_debug_log( m, "stairs placed:", stair );
        } else {
            stairs_debug_log( m, "stairs not placed:", stair, D_WARNING );
        }
    }
}
