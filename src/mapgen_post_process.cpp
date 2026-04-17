#include "mapgen_post_process.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <list>
#include <string>
#include <string_view>

#include "calendar.h"
#include "cata_utility.h"
#include "coordinates.h"
#include "debug.h"
#include "drawing_primitives.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "item.h"
#include "item_stack.h"
#include "map.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "overmapbuffer.h"
#include "point.h"
#include "rng.h"
#include "type_id.h"

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

namespace
{
generic_factory<pp_generator> pp_generator_factory( "pp_generator" );
} // namespace

void pp_generators::load( const JsonObject &jo, const std::string &src )
{
    pp_generator_factory.load( jo, src );
}

void pp_generators::reset()
{
    pp_generator_factory.reset();
}

void pp_generators::finalize()
{
    pp_generator_factory.finalize();
}

void pp_generators::check_consistency()
{
    pp_generator_factory.check();
}

/** @relates string_id */
template<>
bool string_id<pp_generator>::is_valid() const
{
    return pp_generator_factory.is_valid( *this );
}

/** @relates string_id */
template<>
const pp_generator &string_id<pp_generator>::obj() const
{
    return pp_generator_factory.obj( *this );
}

namespace io
{
template<>
std::string enum_to_string<sub_generator_type>( sub_generator_type data )
{
    switch( data ) {
        // *INDENT-OFF*
        case sub_generator_type::bash_damage: return "bash_damage";
        case sub_generator_type::move_items: return "move_items";
        case sub_generator_type::add_fire: return "add_fire";
        case sub_generator_type::pre_burn: return "pre_burn";
        case sub_generator_type::place_blood: return "place_blood";
        case sub_generator_type::aftershock_ruin: return "aftershock_ruin";
        // *INDENT-ON*
        case sub_generator_type::last:
            break;
    }
    cata_fatal( "Invalid sub_generator_type" );
}
} // namespace io

void pp_generator::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "id", id );

    if( jo.has_array( "sub_generators" ) ) {
        sub_generators_.clear();
        for( const JsonValue &sg_jo : jo.get_array( "sub_generators" ) ) {
            pp_sub_generator sg;
            sg.load( sg_jo );
            sub_generators_.push_back( sg );
        }
    }
}

void pp_sub_generator::load( const JsonObject &jo )
{
    mandatory( jo, false, "type", type );
    optional( jo, false, "attempts", attempts, 0 );
    optional( jo, false, "chance", chance, 0 );
    optional( jo, false, "min_intensity", min_intensity, 0 );
    optional( jo, false, "max_intensity", max_intensity, 0 );
    optional( jo, false, "scaling_days_start", scaling_days_start, 0 );
    optional( jo, false, "scaling_days_end", scaling_days_end, 0 );
}

void pp_generator::check() const
{
    if( sub_generators_.empty() ) {
        debugmsg( "pp_generator '%s' has no sub_generators", id.str() );
    }
    for( const pp_sub_generator &sg : sub_generators_ ) {
        sg.check( id.str() );
    }
}

void pp_sub_generator::check( const std::string &ctx ) const
{
    if( attempts < 0 ) {
        debugmsg( "pp_generator '%s': negative attempts", ctx );
    }

    const std::string tname = io::enum_to_string( type );

    switch( type ) {
        case sub_generator_type::bash_damage:
            if( chance < 0 || chance > 100 ) {
                debugmsg( "pp_generator '%s' %s: chance %d not in [0,100]", ctx, tname, chance );
            }
            if( scaling_days_start != 0 || scaling_days_end != 0 ) {
                debugmsg( "pp_generator '%s' %s: scaling_days_* ignored for this type",
                          ctx, tname );
            }
            break;
        case sub_generator_type::move_items:
            if( chance < 0 || chance > 100 ) {
                debugmsg( "pp_generator '%s' %s: chance %d not in [0,100]", ctx, tname, chance );
            }
            if( min_intensity != 0 ) {
                debugmsg( "pp_generator '%s' %s: min_intensity is not implemented, "
                          "but is set to %d", ctx, tname, min_intensity );
            }
            if( scaling_days_start != 0 || scaling_days_end != 0 ) {
                debugmsg( "pp_generator '%s' %s: scaling_days_* ignored for this type",
                          ctx, tname );
            }
            break;
        case sub_generator_type::add_fire:
            if( chance != 0 ) {
                debugmsg( "pp_generator '%s' %s: chance is computed from scaling_days_end, "
                          "but is set to %d", ctx, tname, chance );
            }
            if( scaling_days_start != 0 ) {
                debugmsg( "pp_generator '%s' %s: scaling_days_start ignored for this type",
                          ctx, tname );
            }
            if( scaling_days_end <= 0 ) {
                debugmsg( "pp_generator '%s' %s: scaling_days_end must be > 0", ctx, tname );
            }
            break;
        case sub_generator_type::pre_burn:
            if( chance != 0 ) {
                debugmsg( "pp_generator '%s' %s: chance is computed from scaling/intensity, "
                          "but is set to %d", ctx, tname, chance );
            }
            if( scaling_days_end <= 0 ) {
                debugmsg( "pp_generator '%s' %s: scaling_days_end must be > 0", ctx, tname );
            }
            if( scaling_days_start > scaling_days_end ) {
                debugmsg( "pp_generator '%s' %s: scaling_days_start > scaling_days_end",
                          ctx, tname );
            }
            break;
        case sub_generator_type::place_blood:
            if( chance < 0 || chance > 1000 ) {
                debugmsg( "pp_generator '%s' %s: chance %d not in [0,1000]", ctx, tname, chance );
            }
            if( min_intensity != 0 || max_intensity != 0 ) {
                debugmsg( "pp_generator '%s' %s: min/max_intensity ignored for this type",
                          ctx, tname );
            }
            if( scaling_days_start != 0 || scaling_days_end != 0 ) {
                debugmsg( "pp_generator '%s' %s: scaling_days_* ignored for this type",
                          ctx, tname );
            }
            break;
        case sub_generator_type::aftershock_ruin:
            if( attempts != 0 || chance != 0 || min_intensity != 0 ||
                max_intensity != 0 || scaling_days_start != 0 || scaling_days_end != 0 ) {
                debugmsg( "pp_generator '%s' %s: all numeric fields ignored for this type",
                          ctx, tname );
            }
            break;
        case sub_generator_type::last:
            debugmsg( "pp_generator '%s': invalid sub_generator_type 'last'", ctx );
            break;
    }

    if( min_intensity > max_intensity && max_intensity != 0 ) {
        debugmsg( "pp_generator '%s' %s: min_intensity > max_intensity",
                  ctx, tname );
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
    debugmsg( "Attempted to get point from invalid direction.  %d", direction );
    return current_tile;
}

static bool tile_can_have_blood( map &md, const tripoint_bub_ms &current_tile,
                                 bool wall_streak, int days_since_cataclysm )
{
    if( wall_streak ) {
        return md.has_flag_ter( ter_furn_flag::TFLAG_WALL, current_tile ) &&
               !md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile );
    } else {
        if( !md.has_flag_ter( ter_furn_flag::TFLAG_INDOORS, current_tile ) &&
            x_in_y( days_since_cataclysm, 30 ) ) {
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
        return;
    }

    md.add_field( current_tile, field_fd_blood );
    tripoint_bub_ms last_tile = current_tile;

    for( int i = 0; i < streak_length; i++ ) {
        tripoint_bub_ms destination_tile = get_point_from_direction( streak_direction, last_tile );

        if( !tile_can_have_blood( md, destination_tile, wall_streak, days_since_cataclysm ) ) {
            bool terminate_streak = true;

            for( int ii = static_cast<int>( blood_trail_direction::first );
                 ii <= static_cast<int>( blood_trail_direction::last );
                 ii++ ) {
                if( ii == streak_direction ) {
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
            last_tile = destination_tile;
            continue;
        }

        if( rng( 1, 100 ) < 10 + i * 5 ) {
            break;
        }

        md.add_field( destination_tile, field_fd_blood );
        last_tile = destination_tile;


        if( ( rng( 1, 100 ) < 30 + i * 3 ) && !wall_streak ) {
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

static void place_bool_pools( map &md, const tripoint_bub_ms &current_tile,
                              int days_since_cataclysm )
{
    if( !tile_can_have_blood( md, current_tile, false, days_since_cataclysm ) ) {
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

static void execute_bash_damage( map &md,
                                 std::list<tripoint_bub_ms> &all_points_in_map,
                                 const pp_sub_generator &sg )
{
    for( int i = 0; i < sg.attempts; i++ ) {
        if( !x_in_y( sg.chance, 100 ) ) {
            continue;
        }
        const tripoint_bub_ms current_tile = random_entry( all_points_in_map );
        if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ) {
            continue;
        }
        md.bash( current_tile, rng( sg.min_intensity, sg.max_intensity ) );
    }
}

static void execute_move_items( map &md,
                                std::list<tripoint_bub_ms> &all_points_in_map,
                                const pp_sub_generator &sg )
{
    for( int i = 0; i < sg.attempts; i++ ) {
        const tripoint_bub_ms current_tile = random_entry( all_points_in_map );
        if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ) {
            continue;
        }
        item_stack::iterator item_iterator = md.i_at( current_tile.xy() ).begin();
        while( item_iterator != md.i_at( current_tile.xy() ).end() ) {
            if( md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_SEALED, current_tile ) &&
                md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_CONTAINER, current_tile ) ) {
                if( md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_PLANT, current_tile ) &&
                    item_iterator->is_seed() ) {
                    item_iterator++;
                    continue;
                }
            }

            if( x_in_y( sg.chance, 100 ) ) {
                tripoint_bub_ms destination_tile(
                    current_tile.x() + rng( -sg.max_intensity, sg.max_intensity ),
                    current_tile.y() + rng( -sg.max_intensity, sg.max_intensity ),
                    current_tile.z() );
                const bool outbounds_X = destination_tile.x() < 0 || destination_tile.x() >= SEEX * 2;
                const bool outbounds_Y = destination_tile.y() < 0 || destination_tile.y() >= SEEY * 2;
                const bool cannot_place = md.has_flag( ter_furn_flag::TFLAG_DESTROY_ITEM, destination_tile ) ||
                                          md.has_flag( ter_furn_flag::TFLAG_NOITEM, destination_tile ) || !md.has_floor( destination_tile );
                if( outbounds_X || outbounds_Y || cannot_place ) {
                    item_iterator++;
                    continue;
                } else {
                    item copy( *item_iterator );
                    md.add_item( destination_tile, copy );
                    item_iterator = md.i_at( current_tile.xy() ).erase( item_iterator );
                }
            } else {
                item_iterator++;
            }

        }
    }

}

static void execute_add_fire( map &md,
                              std::list<tripoint_bub_ms> &all_points_in_map,
                              int days_since_cataclysm,
                              const pp_sub_generator &sg )
{
    const int percent_chance = std::max( sg.scaling_days_end - days_since_cataclysm, 0 );

    for( int i = 0; i < sg.attempts; i++ ) {
        if( !x_in_y( percent_chance, 100 ) ) {
            continue;
        }
        const tripoint_bub_ms current_tile = random_entry( all_points_in_map );
        if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ) {
            continue;
        }

        if( md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FLAMMABLE, current_tile ) ||
            md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FLAMMABLE_ASH, current_tile ) ||
            md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FLAMMABLE_HARD, current_tile ) ||
            days_since_cataclysm < 3 ) {
            md.add_field( current_tile, field_fd_fire,
                          rng( sg.min_intensity, sg.max_intensity ) );
        }

    }

}

static void execute_pre_burn( map &md,
                              std::list<tripoint_bub_ms> &all_points_in_map,
                              int days_since_cataclysm,
                              const pp_sub_generator &sg )
{
    double lerp_scalar = static_cast<double>(
                             static_cast<double>( days_since_cataclysm - sg.scaling_days_start ) /
                             static_cast<double>( sg.scaling_days_end - sg.scaling_days_start ) );
    int percent_chance = lerp( sg.min_intensity, sg.max_intensity, lerp_scalar );
    if( days_since_cataclysm < sg.scaling_days_start ) {
        percent_chance = 0;
    } else if( days_since_cataclysm >= sg.scaling_days_end ) {
        percent_chance = sg.max_intensity;
    }

    for( int i = 0; i < sg.attempts; i++ ) {
        if( !x_in_y( percent_chance, 100 ) ) {
            continue;
        }
        for( tripoint_bub_ms current_tile : all_points_in_map ) {
            if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ||
                md.has_flag_ter( ter_furn_flag::TFLAG_GOES_DOWN, current_tile ) ||
                md.has_flag_ter( ter_furn_flag::TFLAG_GOES_UP, current_tile ) ) {
                continue;
            }
            if( md.has_flag_ter( ter_furn_flag::TFLAG_WALL, current_tile ) ) {
                md.ter_set( current_tile.xy(), ter_t_wall_burnt );
            } else if( md.has_flag_ter( ter_furn_flag::TFLAG_INDOORS, current_tile ) ||
                       md.has_flag_ter( ter_furn_flag::TFLAG_DOOR, current_tile ) ) {
                md.ter_set( current_tile.xy(), ter_t_floor_burnt );
            } else if( !md.has_flag_ter( ter_furn_flag::TFLAG_INDOORS, current_tile ) ) {
                if( current_tile.z() == 0 ) {
                    md.ter_set( current_tile.xy(), ter_t_dirt );
                }
            }

            md.furn_set( current_tile.xy(), furn_str_id::NULL_ID() );
            md.i_clear( current_tile.xy() );
        }
    }
}

static void execute_place_blood( map &md,
                                 std::list<tripoint_bub_ms> &all_points_in_map,
                                 int days_since_cataclysm,
                                 const pp_sub_generator &sg )
{
    for( int i = 0; i < sg.attempts; i++ ) {
        tripoint_bub_ms current_tile = random_entry( all_points_in_map );

        if( x_in_y( 10, 100 ) ) {
            continue;
        }
        if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ) {
            continue;
        }
        if( x_in_y( sg.chance, 1000 ) ) {
            int behavior_roll = rng( 1, 100 );

            if( behavior_roll <= 20 ) {
                if( tile_can_have_blood( md, current_tile, md.has_flag_ter( ter_furn_flag::TFLAG_WALL,
                                         current_tile ), days_since_cataclysm ) ) {
                    md.add_field( current_tile, field_fd_blood );
                }
            } else if( behavior_roll <= 60 ) {
                place_blood_streaks( md, current_tile, days_since_cataclysm );
            } else {
                place_bool_pools( md, current_tile, days_since_cataclysm );
            }
        }
    }
}

static void execute_aftershock_ruin( map &md, const tripoint_abs_omt &p )
{
    std::list<tripoint_bub_ms> all_points_in_map;

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int n = 0; n < SEEY * 2; n++ ) {
            tripoint_bub_ms current_tile( i, n, p.z() );
            all_points_in_map.push_back( current_tile );
        }
    }
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

void pp_generator::execute( map &md, const tripoint_abs_omt &p ) const
{
    std::list<tripoint_bub_ms> all_points_in_map;

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int n = 0; n < SEEY * 2; n++ ) {
            all_points_in_map.emplace_back( i, n, p.z() );
        }
    }

    const int days_since_cataclysm = to_days<int>( calendar::turn - calendar::start_of_cataclysm );

    for( const pp_sub_generator &sg : sub_generators_ ) {
        switch( sg.type ) {
            case sub_generator_type::bash_damage:
                execute_bash_damage( md, all_points_in_map, sg );
                break;
            case sub_generator_type::move_items:
                execute_move_items( md, all_points_in_map, sg );
                break;
            case sub_generator_type::add_fire:
                execute_add_fire( md, all_points_in_map, days_since_cataclysm, sg );
                break;
            case sub_generator_type::pre_burn:
                execute_pre_burn( md, all_points_in_map, days_since_cataclysm, sg );
                break;
            case sub_generator_type::place_blood:
                execute_place_blood( md, all_points_in_map, days_since_cataclysm, sg );
                break;
            case sub_generator_type::aftershock_ruin:
                execute_aftershock_ruin( md, p );
                break;
            case sub_generator_type::last:
                debugmsg( "Invalid sub_generator_type in pp_generator '%s'", id.str() );
                break;
        }
    }
}
