#include "mapgen_post_process_generators.h"

#include <algorithm>
#include <functional>
#include <stddef.h>

#include "condition.h"
#include "calendar.h"
#include "cata_utility.h"
#include "coordinates.h"
#include "coords_fwd.h"
#include "debug.h"
#include "drawing_primitives.h"
#include "avatar.h"
#include "generic_factory.h"
#include "item.h"
#include "map.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "omdata.h"
#include "overmapbuffer.h"
#include "point.h"
#include "rng.h"

static const field_type_str_id field_fd_blood( "fd_blood" );
static const field_type_str_id field_fd_fire( "fd_fire" );

static const furn_str_id furn_f_wreckage( "f_wreckage" );

static const oter_str_id oter_afs_ruins_dynamic( "afs_ruins_dynamic" );
static const oter_str_id oter_open_air( "open_air" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_floor_burnt( "t_floor_burnt" );
static const ter_str_id ter_t_metal_floor( "t_metal_floor" );
static const ter_str_id ter_t_snow( "t_snow" );
static const ter_str_id ter_t_snow_metal_floor( "t_snow_metal_floor" );
static const ter_str_id ter_t_wall_burnt( "t_wall_burnt" );
static const ter_str_id ter_t_wall_prefab_metal( "t_wall_prefab_metal" );



void generator_instance::load( const JsonObject &jo, std::string_view )
{
    percent_chance = get_dbl_or_var( jo, "chance", false, 100.0 );
    optional( jo, was_loaded, "type", type );
    optional( jo, was_loaded, "attempts", num_attempts );
    optional( jo, was_loaded, "min_intensity", min_intensity );
    optional( jo, was_loaded, "max_intensity", max_intensity );
}

namespace
{
generic_factory<pp_generator> pp_generator_factory( "pp_generator" );
} // namespace

void pp_generator::load_pp_generator( const JsonObject &jo, const std::string &src )
{
    pp_generator_factory.load( jo, src );
}

/** @relates string_id */
template<>
const pp_generator &string_id<pp_generator>::obj() const
{
    return pp_generator_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<pp_generator>::is_valid() const
{
    return pp_generator_factory.is_valid( *this );
}

namespace io
{

template<>
std::string enum_to_string<map_generator>( map_generator data )
{
    switch( data ) {
            // *INDENT-OFF*
        case map_generator::bash_damage: return "bash_damage";
        case map_generator::move_items: return "move_items";
        case map_generator::add_fire: return "add_fire";
        case map_generator::pre_burn: return "pre_burn";
        case map_generator::place_blood: return "place_blood";
        case map_generator::afs_ruin: return "afs_ruin";
            // *INDENT-ON*
        case map_generator::last:
            break;
    }
    cata_fatal( "Invalid map_generator" );
}

} // namespace io

void pp_generator::load( const JsonObject &jo, std::string_view )
{
    // that looks like a wrong way to load array using generic_factory
    for( JsonObject job : jo.get_array( "generators" ) ) {
        generator_instance gi;
        gi.load( job, std::string_view{} );
        all_generators.emplace_back( gi );
    }
}

void pp_generator::apply_pp_generators( map &md, std::vector<generator_instance> gen,
                                        tripoint_abs_omt omt_point )
{

    std::list<tripoint_bub_ms> all_points_in_map;

    // Placeholder / FIXME
    // This assumes that we're only dealing with regular 24x24 OMTs. That is likely not the case.
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int n = 0; n < SEEY * 2; n++ ) {
            tripoint_bub_ms current_tile( i, n, omt_point.z() );
            all_points_in_map.push_back( current_tile );
        }
    }

    for( const generator_instance &gi : gen ) {
        switch( gi.type ) {
            case map_generator::bash_damage:
                gi.bash_damage( md, all_points_in_map );
                break;
            case map_generator::move_items:
                gi.move_items( md, all_points_in_map );
                break;
            case map_generator::add_fire:
                gi.add_fire( md, all_points_in_map );
                break;
            case map_generator::pre_burn:
                gi.pre_burn( md, all_points_in_map );
                break;
            case map_generator::place_blood:
                gi.place_blood( md, all_points_in_map );
                break;
            case map_generator::afs_ruin:
                gi.aftershock_ruin( md, all_points_in_map, omt_point );
                break;
            default:
                debugmsg( "Unknown post-process generator" );
                break;
        }
    }
}

enum class blood_trail_direction : int {
    first = 1,
    NORTH = 1,
    SOUTH = 2,
    EAST = 3,
    WEST = 4,
    last = 4
};

static tripoint_bub_ms get_point_from_direction( int direction,
        const tripoint_bub_ms &current_tile )
{
    switch( static_cast<blood_trail_direction>( direction ) ) {
        case blood_trail_direction::NORTH:
            return tripoint_bub_ms( current_tile.x(), current_tile.y() - 1, current_tile.z() );
        case blood_trail_direction::SOUTH:
            return tripoint_bub_ms( current_tile.x(), current_tile.y() + 1, current_tile.z() );
        case blood_trail_direction::WEST:
            return tripoint_bub_ms( current_tile.x() - 1, current_tile.y(), current_tile.z() );
        case blood_trail_direction::EAST:
            return tripoint_bub_ms( current_tile.x() + 1, current_tile.y(), current_tile.z() );
    }
    // This shouldn't happen unless the function is used incorrectly
    debugmsg( "Attempted to get point from invalid direction.  %d", direction );
    return current_tile;
}

static bool tile_can_have_blood( map &md, const tripoint_bub_ms &current_tile,
                                 bool wall_streak, int days_since_cataclysm )
{
    // Wall streaks stick to walls, like blood was splattered against the surface
    // Floor streaks avoid obstacles to look more like a person left the streak behind (Not walking through walls, closed doors, windows, or furniture)
    if( wall_streak ) {
        return md.has_flag_ter( ter_furn_flag::TFLAG_WALL, current_tile ) &&
               !md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile );
    } else {
        if( !md.has_flag_ter( ter_furn_flag::TFLAG_INDOORS, current_tile ) &&
            x_in_y( days_since_cataclysm, 30 ) ) {
            // Placement of blood outdoors scales down over the course of 30 days until no further blood is placed.
            return false;
        }

        return !md.has_flag_ter( ter_furn_flag::TFLAG_WALL, current_tile ) &&
               !md.has_flag_ter( ter_furn_flag::TFLAG_WINDOW, current_tile ) &&
               !md.has_flag_ter( ter_furn_flag::TFLAG_DOOR, current_tile ) &&
               !md.has_furn( current_tile );
    }

}

static void place_blood_on_adjacent( map &md, const tripoint_bub_ms &current_tile, int chance,
                                     int days_since_catacylsm )
{
    for( int i = static_cast<int>( blood_trail_direction::first );
         i <= static_cast<int>( blood_trail_direction::last ); i++ ) {
        tripoint_bub_ms adjacent_tile = get_point_from_direction( i, current_tile );

        if( !tile_can_have_blood( md, adjacent_tile, false, days_since_catacylsm ) ) {
            continue;
        }
        if( rng( 1, 100 ) < chance ) {
            md.add_field( adjacent_tile, field_fd_blood );
        }
    }
}

static void place_blood_streaks( map &md, const tripoint_bub_ms &current_tile,
                                 int days_since_cataclysm )
{
    int streak_length = rng( 3, 12 );
    int streak_direction = rng( static_cast<int>( blood_trail_direction::first ),
                                static_cast<int>( blood_trail_direction::last ) );

    bool wall_streak = md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_WALL, current_tile );

    if( !tile_can_have_blood( md, current_tile, wall_streak, days_since_cataclysm ) ) {
        // Quick check the tile is valid.
        return;
    }

    md.add_field( current_tile, field_fd_blood );
    tripoint_bub_ms last_tile = current_tile;

    for( int i = 0; i < streak_length; i++ ) {
        tripoint_bub_ms destination_tile = get_point_from_direction( streak_direction, last_tile );

        if( !tile_can_have_blood( md, destination_tile, wall_streak, days_since_cataclysm ) ) {
            // We hit a non-valid tile. Try to find a new direction otherwise just terminate the streak.
            bool terminate_streak = true;

            for( int ii = static_cast<int>( blood_trail_direction::first );
                 ii <= static_cast<int>( blood_trail_direction::last );
                 ii++ ) {
                if( ii == streak_direction ) {
                    // We don't want to check the direction we came from. No turning around!
                    continue;
                }
                tripoint_bub_ms adjacent_tile = get_point_from_direction( ii, last_tile );

                if( tile_can_have_blood( md, adjacent_tile, wall_streak, days_since_cataclysm ) ) {
                    streak_direction = ii;
                    destination_tile = adjacent_tile;
                    terminate_streak = false;
                    break;
                }
            }

            if( terminate_streak ) {
                break;
            }
        }

        if( rng( 1, 100 ) < 5 + i * 3 ) {
            // Sometimes a streak should skip a tile, the chance increases with each step
            last_tile = destination_tile;
            continue;
        }

        if( rng( 1, 100 ) < 10 + i * 5 ) {
            // Sometimes a streak should end early, the chance increases with each step.
            // This is just a hack to further weight the distribution in favor of short streaks over long ones.
            break;
        }

        md.add_field( destination_tile, field_fd_blood );
        last_tile = destination_tile;


        if( ( rng( 1, 100 ) < 30 + i * 3 ) && !wall_streak ) {
            // Floor streaks can meander and the probability of meandering increases with each step
            // Long straight streaks aren't visually interesting. So sometimes a streak will curve by meandering to the side.
            int new_direction = 0;
            if( streak_direction == static_cast<int>( blood_trail_direction::NORTH ) ||
                streak_direction == static_cast<int>( blood_trail_direction::SOUTH ) ) {
                new_direction = one_in( 2 ) ? static_cast<int>( blood_trail_direction::EAST ) : static_cast<int>(
                                    blood_trail_direction::WEST );
            } else {
                new_direction = one_in( 2 ) ? static_cast<int>( blood_trail_direction::NORTH ) : static_cast<int>(
                                    blood_trail_direction::SOUTH );
            }

            tripoint_bub_ms adjacent_tile = get_point_from_direction( new_direction, last_tile );
            if( tile_can_have_blood( md, adjacent_tile, wall_streak, days_since_cataclysm ) ) {
                md.add_field( adjacent_tile, field_fd_blood );
                last_tile = adjacent_tile;
            }
        }
    }
}

static void place_blood_pools( map &md, const tripoint_bub_ms &current_tile,
                               int days_since_cataclysm )
{
    if( !tile_can_have_blood( md, current_tile, false, days_since_cataclysm ) ) {
        // Quick check the first tile is valid for placement
        return;
    }

    md.add_field( current_tile, field_fd_blood );
    place_blood_on_adjacent( md, current_tile, 60, days_since_cataclysm );

    for( int i = static_cast<int>( blood_trail_direction::first );
         i <= static_cast<int>( blood_trail_direction::last ); i++ ) {
        tripoint_bub_ms adjacent_tile = get_point_from_direction( i, current_tile );
        if( !tile_can_have_blood( md, adjacent_tile, false, days_since_cataclysm ) ) {
            continue;
        }
        place_blood_on_adjacent( md, adjacent_tile, 30, days_since_cataclysm );
    }
}

void generator_instance::bash_damage( map &md, std::list<tripoint_bub_ms> &all_points_in_map ) const
{
    dialogue d;
    const int chance = percent_chance.evaluate( d );

    for( int i = 0; i < num_attempts; i++ ) {
        if( !x_in_y( chance, 100 ) ) {
            continue; // failed roll
        }
        const tripoint_bub_ms current_tile = random_entry( all_points_in_map );
        if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ) {
            continue;
        }
        md.bash( current_tile, rng( min_intensity, max_intensity ) );
    }
}

void generator_instance::move_items( map &md, std::list<tripoint_bub_ms> &all_points_in_map ) const
{
    const int chance_ = chance();

    for( int i = 0; i < num_attempts; i++ ) {
        const tripoint_bub_ms current_tile = random_entry( all_points_in_map );
        if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ) {
            continue;
        }
        auto item_iterator = md.i_at( current_tile.xy() ).begin();
        while( item_iterator != md.i_at( current_tile.xy() ).end() ) {
            // Some items must not be moved out of SEALED CONTAINER
            if( md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_SEALED, current_tile ) &&
                md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_CONTAINER, current_tile ) ) {
                // Seed should stay in their planter for map::grow_plant to grow them
                if( md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_PLANT, current_tile ) &&
                    item_iterator->is_seed() ) {
                    item_iterator++;
                    continue;
                }
            }

            if( x_in_y( chance_, 100 ) ) {
                // pick a new spot...
                tripoint_bub_ms destination_tile(
                    current_tile.x() + rng( -max_intensity, max_intensity ),
                    current_tile.y() + rng( -max_intensity, max_intensity ),
                    current_tile.z() );
                // oops, don't place out of bounds. just skip moving
                const bool outbounds_X = destination_tile.x() < 0 || destination_tile.x() >= SEEX * 2;
                const bool outbounds_Y = destination_tile.y() < 0 || destination_tile.y() >= SEEY * 2;
                const bool cannot_place = md.has_flag( ter_furn_flag::TFLAG_DESTROY_ITEM, destination_tile ) ||
                                          md.has_flag( ter_furn_flag::TFLAG_NOITEM, destination_tile ) || !md.has_floor( destination_tile );
                if( outbounds_X || outbounds_Y || cannot_place ) {
                    item_iterator++;
                    continue;
                } else {
                    item copy( *item_iterator );
                    // add a copy of our item to the destination...
                    md.add_item( destination_tile, copy );
                    // and erase the one at our source.
                    item_iterator = md.i_at( current_tile.xy() ).erase( item_iterator );
                }
            } else {
                item_iterator++;
            }

        }
    }

}

void generator_instance::add_fire( map &md, std::list<tripoint_bub_ms> &all_points_in_map ) const
{
    const int chance_ = chance();

    for( int i = 0; i < num_attempts; i++ ) {
        const tripoint_bub_ms current_tile = random_entry( all_points_in_map );
        if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ) {
            continue;
        }

        if( x_in_y( chance_, 100 ) ) {
            // FIXME: Magic number 3. Replace with some value loaded into pp_generator?
            if( md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FLAMMABLE, current_tile ) ||
                md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FLAMMABLE_ASH, current_tile ) ||
                md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FLAMMABLE_HARD, current_tile ) ) {
                // Only place fire on flammable surfaces
                // Note that most floors are FLAMMABLE_HARD, this is fine. This check is primarily geared
                // at preventing fire in the middle of roads or parking lots.
                md.add_field( current_tile, field_fd_fire, rng( min_intensity, max_intensity ) );
            }
        }

    }

}

void generator_instance::pre_burn( map &md, std::list<tripoint_bub_ms> &all_points_in_map ) const
{
    const int chance_ = chance();

    for( int i = 0; i < num_attempts; i++ ) {
        for( tripoint_bub_ms current_tile : all_points_in_map ) {
            debugmsg( "%s", current_tile.abs().to_string() );
            if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ||
                md.has_flag_ter( ter_furn_flag::TFLAG_GOES_DOWN, current_tile ) ||
                md.has_flag_ter( ter_furn_flag::TFLAG_GOES_UP, current_tile ) ) {
                // skip natural underground walls, or any stairs. (Even man-made or wooden stairs)
                continue;
            }
            if( md.has_flag_ter( ter_furn_flag::TFLAG_WALL, current_tile ) ) {
                // burnt wall
                md.ter_set( current_tile.xy(), ter_t_wall_burnt );
            } else if( md.has_flag_ter( ter_furn_flag::TFLAG_INDOORS, current_tile ) ||
                       md.has_flag_ter( ter_furn_flag::TFLAG_DOOR, current_tile ) ) {
                // if we're indoors but we're not a wall, then we must be a floor.
                // doorways also get burned to the ground.
                md.ter_set( current_tile.xy(), ter_t_floor_burnt );
            } else if( !md.has_flag_ter( ter_furn_flag::TFLAG_INDOORS, current_tile ) ) {
                // if we're outside on ground level, burn it to dirt.
                if( current_tile.z() == 0 ) {
                    md.ter_set( current_tile.xy(), ter_t_dirt );
                }
            }

            // destroy any furniture that is in the tile. it's been burned, after all.
            md.furn_set( current_tile.xy(), furn_str_id::NULL_ID() );

            // destroy all items in the tile.
            md.i_clear( current_tile.xy() );
        }
    }
}

void generator_instance::place_blood( map &md, std::list<tripoint_bub_ms> &all_points_in_map ) const
{
    // todo replace days_since_cataclysm with percent_chance
    const int days_since_cataclysm = 0;

    const int chance_ = chance();

    // NOTE: Below currently only runs for bloodstains.
    for( size_t i = 0; i < all_points_in_map.size(); i++ ) {
        // Pick a tile at random!
        tripoint_bub_ms current_tile = random_entry( all_points_in_map );

        // Do nothing at random!;
        if( x_in_y( 10, 100 ) ) {
            continue;
        }
        // Skip naturally occuring underground wall tiles
        if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ) {
            continue;
        }
        // Set some fields at random!
        if( x_in_y( chance_, 100 ) ) {
            const int behavior_roll = rng( 1, 100 );

            if( behavior_roll <= 20 ) {
                if( tile_can_have_blood( md, current_tile, md.has_flag_ter( ter_furn_flag::TFLAG_WALL,
                                         current_tile ), days_since_cataclysm ) ) {
                    md.add_field( current_tile, field_fd_blood );
                }
            } else if( behavior_roll <= 60 ) {
                place_blood_streaks( md, current_tile, days_since_cataclysm );
            } else {
                place_blood_pools( md, current_tile, days_since_cataclysm );
            }
        }
    }
}

void generator_instance::aftershock_ruin( map &md, std::list<tripoint_bub_ms> &all_points_in_map,
        const tripoint_abs_omt &p )
{
    bool above_open_air = overmap_buffer.ter( tripoint_abs_omt( p.x(), p.y(),
                          p.z() + 1 ) ) == oter_open_air;
    bool above_ruined = overmap_buffer.ter( tripoint_abs_omt( p.x(), p.y(),
                                            p.z() + 1 ) ) == oter_afs_ruins_dynamic;

    //fully ruined
    if( ( x_in_y( 1, 2 ) && above_open_air ) || above_ruined ) {
        overmap_buffer.ter_set( p, oter_afs_ruins_dynamic );

        for( const tripoint_bub_ms &current_tile : all_points_in_map ) {
            if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ||
                md.has_flag_ter( ter_furn_flag::TFLAG_GOES_DOWN, current_tile ) ||
                md.has_flag_ter( ter_furn_flag::TFLAG_GOES_UP, current_tile ) ) {
                // skip natural underground walls, or any stairs.
                continue;
            }
            if( !one_in( 8 ) ) {
                md.furn_set( current_tile.xy(), furn_str_id::NULL_ID() );
            }
            if( md.has_flag_ter( ter_furn_flag::TFLAG_WALL, current_tile ) ) {
                !one_in( 5 ) ? md.ter_set( current_tile.xy(),
                                           ter_t_wall_prefab_metal ) : md.ter_set( current_tile.xy(), ter_t_metal_floor );
            }
            if( md.has_flag_ter( ter_furn_flag::TFLAG_INDOORS, current_tile ) ) {
                if( !one_in( 4 ) ) {
                    p.z() == 0 ? md.ter_set( current_tile.xy(),
                                             ter_t_snow_metal_floor ) : md.ter_set( current_tile.xy(), ter_t_snow );

                }
            }
            if( x_in_y( 5, 1000 ) ) {
                md.bash( current_tile, 9999, true, true );
                draw_rough_circle( [&md, current_tile]( const point_bub_ms & p ) {
                    if( !md.inbounds( p ) || !md.has_flag_ter( ter_furn_flag::TFLAG_FLAT, p ) ) {
                        return;
                    }
                    md.bash( tripoint_bub_ms( p.x(), p.y(), current_tile.z() ), 9999, true, true );
                }, current_tile.xy(), 3 );
            }
            if( !md.has_furn( current_tile.xy() ) ) {
                md.i_clear( current_tile.xy() );
            }
        }
        if( p.z() == 0 ) {
            for( const tripoint_bub_ms &current_tile : all_points_in_map ) {
                if( md.is_open_air( current_tile ) ) {
                    md.ter_set( current_tile, ter_t_snow, false );
                }
            }
        }

        return;
    }

    //just smashed up
    for( size_t i = 0; i < all_points_in_map.size(); i++ ) {

        const tripoint_bub_ms current_tile = random_entry( all_points_in_map );

        if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ||
            md.is_open_air( current_tile ) ) {
            continue;
        }

        if( x_in_y( 5, 1000 ) ) {
            md.bash( current_tile, 9999, true, true );
            draw_rough_circle( [&md, current_tile]( const point_bub_ms & p ) {
                if( !md.inbounds( p ) || !md.has_flag_ter( ter_furn_flag::TFLAG_FLAT, p ) ) {
                    return;
                }
                md.bash( tripoint_bub_ms( p.x(), p.y(), current_tile.z() ), 9999, true, true );
            }, current_tile.xy(), 3 );
        }

        if( x_in_y( 5, 1000 ) ) {
            md.bash( current_tile, 9999, true, true );
            draw_rough_circle( [&md]( const point_bub_ms & p ) {
                if( !md.inbounds( p ) || !md.has_flag_ter( ter_furn_flag::TFLAG_FLAT, p ) ) {
                    return;
                }
                md.furn_set( p, furn_str_id::NULL_ID() );
                md.furn_set( p, furn_f_wreckage );
            }, current_tile.xy(), 3 );
        }
    }
}

int generator_instance::chance() const
{
    dialogue d( get_talker_for( get_avatar() ), get_talker_for( get_avatar() ) );
    return percent_chance.evaluate( d );
}
