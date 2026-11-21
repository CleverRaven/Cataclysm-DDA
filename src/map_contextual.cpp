/**
* Functions for specific terrain/furniture; any map features that aren't terrain/furniture
* e.g. fields, traps, constructions, temperature, radiation
*/

#include "map.h" // IWYU pragma: associated

#include <algorithm>
#include <bitset>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "active_item_cache.h"
#include "avatar.h"
#include "basecamp.h"
#include "calendar.h"
#include "cata_type_traits.h"
#include "character.h"
#include "character_id.h"
#include "construction.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "effect_source.h"
#include "enum_conversions.h"
#include "enums.h"
#include "field.h"
#include "field_type.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "harvest.h"
#include "item.h"
#include "item_group.h"
#include "itype.h"
#include "level_cache.h"
#include "line.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "mdarray.h"
#include "messages.h"
#include "mongroup.h"
#include "mtype.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "rng.h"
#include "scent_map.h"
#include "sounds.h"
#include "submap.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "viewer.h"
#include "vpart_position.h"
#include "weather.h"

class computer;

static const efftype_id effect_gliding( "gliding" );
static const efftype_id effect_incorporeal( "incorporeal" );

static const flag_id json_flag_AVATAR_ONLY( "AVATAR_ONLY" );
static const flag_id json_flag_PROXIMITY( "PROXIMITY" );
static const flag_id json_flag_UNDODGEABLE( "UNDODGEABLE" );

static const furn_str_id furn_f_rubble( "f_rubble" );
static const furn_str_id furn_f_rubble_rock( "f_rubble_rock" );
static const furn_str_id furn_f_wreckage( "f_wreckage" );

static const item_group_id Item_spawn_data_default_zombie_clothes( "default_zombie_clothes" );
static const item_group_id Item_spawn_data_default_zombie_items( "default_zombie_items" );

static const itype_id itype_maple_sap( "maple_sap" );
static const itype_id itype_nail( "nail" );
static const itype_id itype_pipe( "pipe" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_scrap( "scrap" );
static const itype_id itype_splinter( "splinter" );
static const itype_id itype_steel_chunk( "steel_chunk" );
static const itype_id itype_wire( "wire" );

static const json_character_flag
json_flag_TEMPORARY_SHAPESHIFT_NO_HANDS( "TEMPORARY_SHAPESHIFT_NO_HANDS" );

static const mtype_id mon_zombie( "mon_zombie" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_grass( "t_grass" );
static const ter_str_id ter_t_open_air( "t_open_air" );
static const ter_str_id ter_t_rock_floor( "t_rock_floor" );
static const ter_str_id ter_t_stump( "t_stump" );
static const ter_str_id ter_t_tree_birch( "t_tree_birch" );
static const ter_str_id ter_t_tree_birch_harvested( "t_tree_birch_harvested" );
static const ter_str_id ter_t_tree_dead( "t_tree_dead" );
static const ter_str_id ter_t_tree_deadpine( "t_tree_deadpine" );
static const ter_str_id ter_t_tree_hickory( "t_tree_hickory" );
static const ter_str_id ter_t_tree_hickory_dead( "t_tree_hickory_dead" );
static const ter_str_id ter_t_tree_hickory_harvested( "t_tree_hickory_harvested" );
static const ter_str_id ter_t_tree_maple_tapped( "t_tree_maple_tapped" );
static const ter_str_id ter_t_tree_pine( "t_tree_pine" );
static const ter_str_id ter_t_tree_willow( "t_tree_willow" );
static const ter_str_id ter_t_tree_willow_harvested( "t_tree_willow_harvested" );
static const ter_str_id ter_t_tree_young( "t_tree_young" );
static const ter_str_id ter_t_trunk( "t_trunk" );
static const ter_str_id ter_t_water_sh( "t_water_sh" );

#define dbg(x) DebugLog((x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

static field nulfield; // Returned when &field_at() is asked for an OOB value

bool map::displace_water( const tripoint_bub_ms &p )
{
    // Check for shallow water
    if( has_flag_ter( ter_furn_flag::TFLAG_SHALLOW_WATER, p ) ) {
        int dis_places = 0;
        int sel_place = 0;
        for( int pass = 0; pass < 2; pass++ ) {
            // we do 2 passes.
            // first, count how many non-water places around
            // then choose one within count and fill it with water on second pass
            if( pass != 0 ) {
                sel_place = rng( 0, dis_places - 1 );
                dis_places = 0;
            }
            for( const tripoint_bub_ms &temp : points_in_radius( p, 1 ) ) {
                if( temp != p
                    || impassable_ter_furn( temp )
                    || has_flag( ter_furn_flag::TFLAG_DEEP_WATER, temp ) ) {
                    continue;
                }
                if( has_flag_ter( ter_furn_flag::TFLAG_SHALLOW_WATER, p ) ||
                    has_flag_ter( ter_furn_flag::TFLAG_DEEP_WATER, p ) ) {
                    continue;
                }
                if( pass != 0 && dis_places == sel_place ) {
                    ter_set( temp, ter_t_water_sh );
                    ter_set( temp, ter_t_dirt );
                    return true;
                }

                dis_places++;
            }
        }
    }
    return false;
}

/*
 * Get the results of harvesting this tile's furniture or terrain
 */
const harvest_id &map::get_harvest( const tripoint_bub_ms &pos ) const
{
    const harvest_id &furn_harvest = furn( pos )->get_harvest();
    if( !furn_harvest.is_null() ) {
        return furn_harvest;
    }
    return ter( pos )->get_harvest();
}

const std::set<std::string> &map::get_harvest_names( const tripoint_bub_ms &pos ) const
{
    const furn_id &furn_here = furn( pos );
    if( furn_here->can_examine( pos ) ) {
        return furn_here->get_harvest_names();
    }
    return ter( pos )->get_harvest_names();
}

bool map::is_harvestable( const tripoint_bub_ms &pos ) const
{
    const harvest_id &harvest_here = get_harvest( pos );
    return !harvest_here.is_null() && !harvest_here->empty();
}

double map::ranged_target_size( const tripoint_bub_ms &p ) const
{
    if( impassable( p ) ) {
        return 1.0;
    }

    if( !has_floor( p ) ) {
        return 0.0;
    }

    // TODO: Handle cases like shrubs, trees, furniture, sandbags...
    return 0.1;
}

int map::climb_difficulty( const tripoint_bub_ms &p ) const
{
    if( p.z() > OVERMAP_HEIGHT || p.z() < -OVERMAP_DEPTH ) {
        debugmsg( "climb_difficulty on out of bounds point: %d, %d, %d", p.x(), p.y(), p.z() );
        return INT_MAX;
    }

    int best_difficulty = INT_MAX;
    int blocks_movement = 0;
    if( has_flag( ter_furn_flag::TFLAG_LADDER, p ) ) {
        // Really easy, but you have to stand on the tile
        return 1;
    } else if( has_flag( ter_furn_flag::TFLAG_RAMP, p ) ||
               has_flag( ter_furn_flag::TFLAG_RAMP_UP, p ) ||
               has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, p ) ) {
        // We're on something stair-like, so halfway there already
        best_difficulty = 7;
    }

    for( const tripoint_bub_ms &pt : points_in_radius( p, 1 ) ) {
        if( impassable_ter_furn( pt ) ) {
            // TODO: Non-hardcoded climbability
            best_difficulty = std::min( best_difficulty, 10 );
            blocks_movement++;
        } else if( veh_at( pt ) ) {
            // Vehicle tiles are quite good for climbing
            // TODO: Penalize spiked parts?
            best_difficulty = std::min( best_difficulty, 7 );
        }

        if( best_difficulty > 5 && has_flag( ter_furn_flag::TFLAG_CLIMBABLE, pt ) ) {
            best_difficulty = 5;
        }
    }

    // TODO: Make this more sensible - check opposite sides, not just movement blocker count
    return std::max( 0, best_difficulty - blocks_movement );
}

bool map::on_matching_stairs( const tripoint_bub_ms &a, const tripoint_bub_ms &b ) const
{
    if( a.xy() != b.xy() ) {
        return false;
    }
    const int dz = b.z() - a.z();
    if( std::abs( dz ) != 1 ) {
        return false;
    }
    const tripoint_bub_ms &lower = dz > 0 ? a : b;
    const tripoint_bub_ms &upper = dz > 0 ? b : a;
    if( has_flag( ter_furn_flag::TFLAG_DIFFICULT_Z, lower ) ||
        has_flag( ter_furn_flag::TFLAG_DIFFICULT_Z, upper ) ) {
        return false;
    }
    return has_flag( ter_furn_flag::TFLAG_GOES_UP, lower ) &&
           has_flag( ter_furn_flag::TFLAG_GOES_DOWN, upper );
}

void map::make_rubble( const tripoint_bub_ms &p, const furn_id &rubble_type, const bool items,
                       const ter_id &floor_type, bool overwrite )
{
    if( overwrite ) {
        ter_set( p, floor_type );
        furn_set( p, rubble_type );
    } else {
        // First see if there is existing furniture to destroy
        if( is_bashable_furn( p ) ) {
            destroy_furn( p, true );
        }
        // Leave the terrain alone unless it interferes with furniture placement
        if( impassable( p ) && is_bashable_ter( p ) ) {
            destroy( p, true );
        }
        // Check again for new terrain after potential destruction
        if( impassable( p ) ) {
            ter_set( p, floor_type );
        }

        if( !is_open_air( p ) ) {
            furn_set( p, rubble_type );
        }
    }

    if( !items || is_open_air( p ) ) {
        return;
    }

    //Still hardcoded, but a step up from the old stuff due to being in only one place
    if( rubble_type == furn_f_wreckage ) {
        item chunk( itype_steel_chunk, calendar::turn );
        item scrap( itype_scrap, calendar::turn );
        add_item_or_charges( p, chunk );
        add_item_or_charges( p, scrap );
        if( one_in( 5 ) ) {
            item pipe( itype_pipe, calendar::turn );
            item wire( itype_wire, calendar::turn );
            add_item_or_charges( p, pipe );
            add_item_or_charges( p, wire );
        }
    } else if( rubble_type == furn_f_rubble_rock ) {
        item rock( itype_rock, calendar::turn );
        int rock_count = rng( 1, 3 );
        for( int i = 0; i < rock_count; i++ ) {
            add_item_or_charges( p, rock );
        }
    } else if( rubble_type == furn_f_rubble ) {
        item splinter( itype_splinter, calendar::turn );
        int splinter_count = rng( 2, 8 );
        for( int i = 0; i < splinter_count; i++ ) {
            add_item_or_charges( p, splinter );
        }
        spawn_item( p, itype_nail, 1, rng( 20, 50 ) );
    }
}

bool map::is_water_shallow_current( const tripoint_bub_ms &p ) const
{
    return has_flag( ter_furn_flag::TFLAG_CURRENT, p ) &&
           !has_flag( ter_furn_flag::TFLAG_DEEP_WATER, p );
}

bool map::is_divable( const tripoint_bub_ms &p ) const
{
    return has_flag( ter_furn_flag::TFLAG_SWIMMABLE, p ) &&
           has_flag( ter_furn_flag::TFLAG_DEEP_WATER, p );
}

bool map::tinder_at( const tripoint_bub_ms &p )
{
    for( const item &i : i_at( p ) ) {
        if( i.has_flag( flag_TINDER ) ) {
            return true;
        }
    }
    return false;
}

bool map::flammable_items_at( const tripoint_bub_ms &p, int threshold )
{
    if( !has_items( p ) ||
        ( has_flag( ter_furn_flag::TFLAG_SEALED, p ) &&
          !has_flag( ter_furn_flag::TFLAG_ALLOW_FIELD_EFFECT, p ) ) ) {
        // Sealed containers don't allow fire, so shouldn't allow setting the fire either
        return false;
    }

    for( const item &i : i_at( p ) ) {
        if( i.flammable( threshold ) ) {
            return true;
        }
    }

    return false;
}

bool map::is_flammable( const tripoint_bub_ms &p )
{
    // No fires on water tiles regardless of other factors
    if( has_flag_ter( ter_furn_flag::TFLAG_DEEP_WATER, p ) ||
        has_flag_ter( ter_furn_flag::TFLAG_SHALLOW_WATER, p ) ) {
        return false;
    }

    if( has_flag( ter_furn_flag::TFLAG_FLAMMABLE, p ) ) {
        return true;
    }

    if( has_flag( ter_furn_flag::TFLAG_FLAMMABLE_ASH, p ) ) {
        return true;
    }

    if( get_field_intensity( p, fd_web ) > 0 ) {
        return true;
    }

    if( flammable_items_at( p ) ) {
        return true;
    }

    return false;
}

void map::decay_fields_and_scent( const time_duration &amount )
{
    // TODO: Make this happen on all z-levels

    // Decay scent separately, so that later we can use field count to skip empty submaps
    get_scent().decay();

    // Coordinate code copied from lightmap calculations
    // TODO: Z
    const int smz = abs_sub.z();
    const auto &outside_cache = get_cache_ref( smz ).outside_cache;
    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            submap *cur_submap = get_submap_at_grid( tripoint_rel_sm{ smx, smy, smz } );
            if( cur_submap == nullptr ) {
                debugmsg( "Tried to process field at (%d,%d,%d) but the submap is not loaded", smx, smy, smz );
                continue;
            }
            int to_proc = cur_submap->field_count;
            if( to_proc < 1 ) {
                if( to_proc < 0 ) {
                    cur_submap->field_count = 0;
                    dbg( D_ERROR ) << "map::decay_fields_and_scent: submap at "
                                   << ( abs_sub + point( smx, smy ) ).to_string()
                                   << "has " << to_proc << " field_count";
                }
                get_cache( smz ).field_cache.reset( smx + ( smy * MAPSIZE ) );
                // This submap has no fields
                continue;
            }

            for( int sx = 0; sx < SEEX; ++sx ) {
                if( to_proc < 1 ) {
                    // This submap had some fields, but all got proc'd already
                    break;
                }

                for( int sy = 0; sy < SEEY; ++sy ) {
                    const point p( sx + smx * SEEX, sy + smy * SEEY );

                    const field &fields = cur_submap->get_field( { sx, sy} );
                    if( !outside_cache[p.x][p.y] ) {
                        to_proc -= fields.field_count();
                        continue;
                    }

                    for( const auto &fp : fields ) {
                        to_proc--;
                        field_entry cur = fp.second;
                        const field_type_id type = cur.get_field_type();
                        const int decay_amount_factor =  type.obj().decay_amount_factor;
                        if( decay_amount_factor != 0 ) {
                            const time_duration decay_amount = amount / decay_amount_factor;
                            cur.set_field_age( cur.get_field_age() + decay_amount );
                        }
                    }
                }
            }

            if( to_proc > 0 ) {
                cur_submap->field_count = cur_submap->field_count - to_proc;
                dbg( D_ERROR ) << "map::decay_fields_and_scent: submap at "
                               << abs_sub.to_string()
                               << "has " << cur_submap->field_count - to_proc << "fields, but "
                               << cur_submap->field_count << " field_count";
            }
        }
    }
}

bool map::has_nearby_fire( const tripoint_bub_ms &p, int radius ) const
{
    for( const tripoint_bub_ms &pt : points_in_radius( p, radius ) ) {
        if( has_field_at( pt, fd_fire ) ) {
            return true;
        }
        if( has_flag_ter_or_furn( ter_furn_flag::TFLAG_USABLE_FIRE, pt ) ) {
            return true;
        }
    }
    return false;
}

bool map::has_nearby_table( const tripoint_bub_ms &p, int radius ) const
{
    for( const tripoint_bub_ms &pt : points_in_radius( p, radius ) ) {
        if( has_flag( ter_furn_flag::TFLAG_FLAT_SURF, pt ) ) {
            return true;
        }
        const optional_vpart_position vp = veh_at( pt );
        if( vp && vp->part_with_feature( "FLAT_SURF", true ) ) {
            return true;
        }
    }
    return false;
}

bool map::has_nearby_chair( const tripoint_bub_ms &p, int radius ) const
{
    for( const tripoint_bub_ms &pt : points_in_radius( p, radius ) ) {
        const optional_vpart_position vp = veh_at( pt );
        if( has_flag( ter_furn_flag::TFLAG_CAN_SIT, pt ) ) {
            return true;
        }
        if( vp && vp->vehicle().has_part( "SEAT" ) ) {
            return true;
        }
    }
    return false;
}

bool map::terrain_moppable( const tripoint_bub_ms &p )
{
    // Moppable items ( spills )
    if( !has_flag( ter_furn_flag::TFLAG_LIQUIDCONT, p ) ) {
        map_stack items = i_at( p );
        auto found = std::find_if( items.begin(), items.end(), []( const item & it ) {
            return it.made_of( phase_id::LIQUID );
        } );

        if( found != items.end() ) {
            return true;
        }
    }

    // Moppable fields ( blood )
    for( const std::pair<const field_type_id, field_entry> &pr : field_at( p ) ) {
        if( pr.first->phase == phase_id::LIQUID || pr.first->moppable ) {
            return true;
        }
    }

    // Moppable vehicles ( blood splatter )
    if( const optional_vpart_position ovp = veh_at( p ) ) {
        vehicle *const veh = &ovp->vehicle();
        for( const int elem : veh->parts_at_relative( ovp->mount_pos(), true ) ) {
            const vehicle_part &vp = veh->part( elem );
            if( vp.blood > 0 ) {
                return true;
            }

            const vehicle_stack items = veh->get_items( vp );
            auto found = std::find_if( items.begin(), items.end(), []( const item & it ) {
                return it.made_of( phase_id::LIQUID );
            } );

            if( found != items.end() ) {
                return true;
            }
        }
    }

    return false;
}

bool map::mop_spills( const tripoint_bub_ms &p )
{
    bool retval = false;

    if( !has_flag( ter_furn_flag::TFLAG_LIQUIDCONT, p ) ) {
        map_stack items = i_at( p );
        auto new_end = std::remove_if( items.begin(), items.end(), []( const item & it ) {
            return it.made_of( phase_id::LIQUID );
        } );
        retval = new_end != items.end();
        while( new_end != items.end() ) {
            new_end = items.erase( new_end );
        }
    }

    field &fld = field_at( p );
    for( const auto &it : fld ) {
        if( it.first->phase == phase_id::LIQUID || it.first->moppable ) {
            remove_field( p, it.first );
            retval = true;
        }
    }

    if( const optional_vpart_position ovp = veh_at( p ) ) {
        vehicle *const veh = &ovp->vehicle();
        for( const int elem : veh->parts_at_relative( ovp->mount_pos(), true ) ) {
            vehicle_part &vp = veh->part( elem );
            if( vp.blood > 0 ) {
                vp.blood = 0;
                retval = true;
            }
            //remove any liquids that somehow didn't fall through to the ground
            vehicle_stack here = veh->get_items( vp );
            auto new_end = std::remove_if( here.begin(), here.end(), []( const item & it ) {
                return it.made_of( phase_id::LIQUID );
            } );
            retval |= ( new_end != here.end() );
            while( new_end != here.end() ) {
                new_end = here.erase( new_end );
            }
        }
    } // if veh != 0
    return retval;
}

bool map::open_door( Creature const &u, const tripoint_bub_ms &p, const bool inside,
                     const bool check_only )
{
    if( u.has_effect( effect_incorporeal ) || impassable_field_at( p ) ||
        u.has_flag( json_flag_TEMPORARY_SHAPESHIFT_NO_HANDS ) ) {
        return false;
    }

    const ter_t &ter = this->ter( p ).obj();
    const furn_t &furn = this->furn( p ).obj();
    if( ter.open ) {
        if( has_flag( ter_furn_flag::TFLAG_OPENCLOSE_INSIDE, p ) && !inside ) {
            return false;
        }

        if( !check_only ) {
            sounds::sound( p, 6, sounds::sound_t::movement, _( "swish" ), true,
                           "open_door", ter.id.str() );
            ter_set( p, ter.open );

            if( u.is_avatar() && u.as_avatar()->schizo_symptoms( 50 ) &&
                !ter.has_flag( ter_furn_flag::TFLAG_TRANSPARENT ) ) {
                tripoint_bub_ms mp = p + point_rel_ms( -2 * u.pos_bub().xy().raw() ) + tripoint_rel_ms{ 2 * p.x(), 2 * p.y(), p.z() };
                g->spawn_hallucination( mp );
            }
        }

        return true;
    } else if( furn.open ) {
        if( has_flag( ter_furn_flag::TFLAG_OPENCLOSE_INSIDE, p ) && !inside ) {
            return false;
        }

        if( !check_only ) {
            sounds::sound( p, 6, sounds::sound_t::movement, _( "swish" ), true,
                           "open_door", furn.id.str() );
            furn_set( p, furn.open );
        }

        return true;
    } else if( const optional_vpart_position vp = veh_at( p ) ) {
        const optional_vpart_position creature_veh = veh_at( u.pos_bub() );
        const bool creature_outside = !creature_veh.has_value() ||
                                      &creature_veh->vehicle() != &veh_at( p )->vehicle();

        const int openable = vp->vehicle().next_part_to_open( vp->part_index(), creature_outside );
        if( openable >= 0 ) {
            if( !check_only ) {
                if( ( u.is_npc() || u.is_avatar() ) &&
                    !vp->vehicle().handle_potential_theft( *u.as_character() ) ) {
                    return false;
                }
                vp->vehicle().open_all_at( *this, openable );
            }

            return true;
        }

        return false;
    }

    return false;
}

bool map::close_door( const tripoint_bub_ms &p, const bool inside, const bool check_only )
{
    if( has_flag( ter_furn_flag::TFLAG_OPENCLOSE_INSIDE, p ) && !inside ) {
        return false;
    }

    const ter_t &ter = this->ter( p ).obj();
    const furn_t &furn = this->furn( p ).obj();
    if( ter.close && !furn.id ) {
        if( !check_only ) {
            sounds::sound( p, 10, sounds::sound_t::movement, _( "swish" ), true,
                           "close_door", ter.id.str() );
            ter_set( p, ter.close );
        }
        return true;
    } else if( furn.close ) {
        if( !check_only ) {
            sounds::sound( p, 10, sounds::sound_t::movement, _( "swish" ), true,
                           "close_door", furn.id.str() );
            furn_set( p, furn.close );
        }
        return true;
    }
    return false;
}

std::string map::get_signage( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return "";
    }

    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to get signage at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return std::string();
    }

    return current_submap->get_signage( l );
}

void map::set_signage( const tripoint_bub_ms &p, const std::string &message )
{
    if( !inbounds( p ) ) {
        return;
    }

    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried set signage at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return;
    }

    current_submap->set_signage( l, message );
}

void map::delete_signage( const tripoint_bub_ms &p )
{
    if( !inbounds( p ) ) {
        return;
    }

    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to delete signage at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return;
    }

    current_submap->delete_signage( l );
}

int map::get_radiation( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return 0;
    }

    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to get radiation at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return 0;
    }

    return current_submap->get_radiation( l );
}

void map::set_radiation( const tripoint_bub_ms &p, const int value )
{
    if( !inbounds( p ) ) {
        return;
    }

    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to set radiation at (%d,%d,%d) but the submap is not loaded", l.x(), l.y() );
        return;
    }

    current_submap->set_radiation( l, value );
}

void map::adjust_radiation( const tripoint_bub_ms &p, const int delta )
{
    if( !inbounds( p ) ) {
        return;
    }

    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to adjust radiation at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return;
    }

    int current_radiation = current_submap->get_radiation( l );
    current_submap->set_radiation( l, current_radiation + delta );
}

units::temperature_delta map::get_temperature_mod( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return units::from_kelvin_delta( 0 );
    }

    const submap *const current_submap = unsafe_get_submap_at( p );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to get temperature at (%d,%d,%d) but the submap is not loaded", p.x(), p.y(),
                  p.z() );
        return units::from_kelvin_delta( 0 );
    }

    return current_submap->get_temperature_mod();
}

void map::set_temperature_mod( const tripoint_bub_ms &p,
                               units::temperature_delta new_temperature_mod )
{
    if( !inbounds( p ) ) {
        return;
    }
    submap *const current_submap = unsafe_get_submap_at( p );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to set temperature at (%d,%d,%d) but the submap is not loaded", p.x(), p.y(),
                  p.z() );
        return;
    }

    current_submap->set_temperature_mod( new_temperature_mod );
}

bool map::can_see_trap_at( const tripoint_bub_ms &p, const Character &c ) const
{
    return tr_at( p ).can_see( p, c );
}

const trap &map::tr_at( const tripoint_abs_ms &p ) const
{
    return tr_at( get_bub( p ) );
}

const trap &map::tr_at( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return tr_null.obj();
    }

    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to get trap at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return tr_null.obj();
    }

    const trap_id &builtin_trap = current_submap->get_ter( l )->trap;
    if( builtin_trap != tr_null ) {
        return *builtin_trap;
    }

    return current_submap->get_trap( l ).obj();
}

partial_con *map::partial_con_at( const tripoint_bub_ms &p )
{
    if( !inbounds( p ) ) {
        return nullptr;
    }
    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to get construction at %s but the submap is not loaded", l.to_string() );
        return nullptr;
    }
    auto it = current_submap->partial_constructions.find( tripoint_sm_ms( l, p.z() ) );
    if( it != current_submap->partial_constructions.end() ) {
        return &it->second;
    }
    return nullptr;
}

void map::partial_con_remove( const tripoint_bub_ms &p )
{
    partial_con_remove_impl( p );
    memory_cache_dec_set_dirty( p, true );
    avatar &player_character = get_avatar();
    if( player_character.sees( *this, p ) ) {
        player_character.memorize_clear_decoration( get_abs( p ), "tr_" );
    }
}

void map::partial_con_remove_no_vision_for_testing( const tripoint_bub_ms &p )
{
    partial_con_remove_impl( p );
}

void map::partial_con_remove_impl( const tripoint_bub_ms &p )
{
    if( !inbounds( p ) ) {
        return;
    }
    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to remove construction at %s but the submap is not loaded", l.to_string() );
        return;
    }
    current_submap->partial_constructions.erase( tripoint_sm_ms( l, p.z() ) );
}

void map::partial_con_set( const tripoint_bub_ms &p, const partial_con &con )
{
    if( !inbounds( p ) ) {
        return;
    }
    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to set construction at %s but the submap is not loaded", l.to_string() );
        return;
    }
    if( !current_submap->partial_constructions.emplace( tripoint_sm_ms( l, p.z() ), con ).second ) {
        debugmsg( "set partial con on top of terrain which already has a partial con" );
    }
}

void map::trap_set( const tripoint_bub_ms &p, const trap_id &type )
{
    if( !inbounds( p ) ) {
        return;
    }

    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to set trap at %s but the submap is not loaded", l.to_string() );
        return;
    }
    const ter_t &ter = current_submap->get_ter( l ).obj();
    if( ter.trap != tr_null ) {
        debugmsg( "set trap %s (%s) at %s on top of terrain %s (%s) which already has a "
                  "built-in trap", type.id().str(), type->name(), p.to_string(),
                  ter.id.str(), ter.name() );
        return;
    }

    memory_cache_dec_set_dirty( p, true );
    avatar &player_character = get_avatar();
    if( player_character.sees( *this, p ) ) {
        player_character.memorize_clear_decoration( get_abs( p ), "tr_" );
    }
    // If there was already a trap here, remove it.
    if( current_submap->get_trap( l ) != tr_null ) {
        remove_trap( p );
    }

    current_submap->set_trap( l, type );
    if( type != tr_null ) {
        traplocs[type.to_i()].push_back( p );
    }
}

void map::remove_trap( const tripoint_bub_ms &p )
{
    if( !inbounds( p ) ) {
        return;
    }

    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to remove trap at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return;
    }

    trap_id tid = current_submap->get_trap( l );
    if( tid != tr_null ) {
        if( g != nullptr && this == &reality_bubble() ) {
            memory_cache_dec_set_dirty( p, true );
            avatar &player_character = get_avatar();
            if( player_character.sees( *this,  p ) ) {
                player_character.memorize_clear_decoration( get_abs( p ), "tr_" );
            }
            player_character.add_known_trap( p, tr_null.obj() );
        }

        current_submap->set_trap( l, tr_null );
        auto &traps = traplocs[tid.to_i()];
        const auto iter = std::find( traps.begin(), traps.end(), p );
        if( iter != traps.end() ) {
            traps.erase( iter );
        }
        // If this tile is a phased terrain placeholder (PHASE_BACK), restore
        // the original terrain stored for this tile when removing the trap.
        if( ter( p )->has_flag( "PHASE_BACK" ) ) {
            if( has_original_terrain_at( p ) ) {
                const ter_id orig = get_original_terrain_at( p );
                ter_set( p, orig );
            } else {
                ter_set( p, ter( p ).obj().bash->ter_set );
            }
        }
    }
}

/*
 * Get wrapper for all fields at xyz
 */
const field &map::field_at( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        nulfield = field();
        return nulfield;
    }

    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to get field at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        nulfield = field();
        return nulfield;
    }

    return current_submap->get_field( l );
}

/*
 * As above, except not const
 */
field &map::field_at( const tripoint_bub_ms &p )
{
    if( !inbounds( p ) ) {
        nulfield = field();
        return nulfield;
    }

    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to get field at (%d,%d,%d) but the submap is not loaded", p.x(), p.y(), p.z() );
        nulfield = field();
        return nulfield;
    }

    return current_submap->get_field( l );
}

time_duration map::mod_field_age( const tripoint_bub_ms &p, const field_type_id &type,
                                  const time_duration &offset )
{
    return set_field_age( p, type, offset, true );
}

int map::mod_field_intensity( const tripoint_bub_ms &p, const field_type_id &type,
                              const int offset )
{
    return set_field_intensity( p, type, offset, true );
}

time_duration map::set_field_age( const tripoint_bub_ms &p, const field_type_id &type,
                                  const time_duration &age, const bool isoffset )
{
    if( field_entry *const field_ptr = get_field( p, type ) ) {
        return field_ptr->set_field_age( ( isoffset ? field_ptr->get_field_age() : 0_turns ) + age );
    }
    return -1_turns;
}

/*
 * set intensity of field type at point, creating if not present, removing if intensity is 0
 * returns resulting intensity, or 0 for not present
 */
int map::set_field_intensity( const tripoint_bub_ms &p, const field_type_id &type,
                              const int new_intensity,
                              bool isoffset )
{
    field_entry *field_ptr = get_field( p, type );
    if( field_ptr != nullptr ) {
        int adj = ( isoffset && field_ptr->is_field_alive() ?
                    field_ptr->get_field_intensity() : 0 ) + new_intensity;
        on_field_modified( p, *type );
        field_ptr->set_field_intensity( adj );
        return adj;
    } else if( new_intensity > 0 ) {
        return add_field( p, type, new_intensity ) ? new_intensity : 0;
    }

    return 0;
}

time_duration map::get_field_age( const tripoint_bub_ms &p, const field_type_id &type ) const
{
    const field_entry *field_ptr = field_at( p ).find_field( type );
    return field_ptr == nullptr ? -1_turns : field_ptr->get_field_age();
}

int map::get_field_intensity( const tripoint_bub_ms &p, const field_type_id &type ) const
{
    const field_entry *field_ptr = get_field( p, type );
    return field_ptr == nullptr ? 0 : field_ptr->get_field_intensity();
}

bool map::has_field_at( const tripoint_bub_ms &p, bool check_bounds ) const
{
    const tripoint_bub_sm sm = coords::project_to<coords::sm>( p );
    return ( !check_bounds || inbounds( p ) ) &&
           get_cache( p.z() ).field_cache[sm.x() + sm.y() * MAPSIZE];
}

bool map::has_field_at( const tripoint_bub_ms &p, const field_type_id &type ) const
{
    return get_field( p, type ) != nullptr;
}

template<typename Map>
cata::copy_const<Map, field_entry> *map::get_field_helper(
    Map &m, const tripoint_bub_ms &p, const field_type_id &type )
{
    if( !m.inbounds( p ) || !m.has_field_at( p, false ) ) {
        return nullptr;
    }

    point_sm_ms l;
    auto *const current_submap = m.unsafe_get_submap_at( tripoint_bub_ms( p ), l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to get field at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return nullptr;
    }

    return current_submap->get_field( l ).find_field( type );
}

field_entry *map::get_field( const tripoint_bub_ms &p, const field_type_id &type )
{
    return get_field_helper( *this, p, type );
}

const field_entry *map::get_field( const tripoint_bub_ms &p, const field_type_id &type ) const
{
    return get_field_helper( *this, p, type );
}

std::optional<field_entry> map::get_impassable_field_at( const tripoint_bub_ms &p )
{
    std::optional<field_entry> potential_field;
    for( auto &pr : field_at( p ) ) {
        field_entry &fd = pr.second;
        if( fd.get_intensity_level().move_cost < 0 ) {
            return fd;
        }
    }
    return potential_field;
}

bool map::impassable_field_at( const tripoint_bub_ms &p )
{
    for( auto &pr : field_at( p ) ) {
        field_entry &fd = pr.second;
        if( fd.get_intensity_level().move_cost < 0 ) {
            return true;
        }
    }
    return false;
}

std::vector<field_type_id> map::get_impassable_field_type_ids_at( const tripoint_bub_ms &p )
{
    std::vector<field_type_id> fields;
    for( auto &fa : field_at( p ) ) {
        if( fa.second.get_intensity_level().move_cost < 0 ) {
            fields.emplace_back( fa.first );
        }
    }
    return fields;
}

bool map::dangerous_field_at( const tripoint_bub_ms &p )
{
    for( auto &pr : field_at( p ) ) {
        field_entry &fd = pr.second;
        if( fd.is_dangerous() ) {
            return true;
        }
    }
    return false;
}

bool map::mopsafe_field_at( const tripoint_bub_ms &p )
{
    for( const std::pair<const int_id<field_type>, field_entry> &pr : field_at( p ) ) {
        const field_entry &fd = pr.second;
        if( !fd.is_mopsafe() ) {
            return false;
        }
    }
    return true;
}

bool map::add_field( const tripoint_bub_ms &p, const field_type_id &type_id, int intensity,
                     const time_duration &age, bool hit_player, effect_source source )
{
    if( !inbounds( p ) ) {
        return false;
    }

    if( !type_id ) {
        return false;
    }

    // Don't spawn non-gaseous fields on open air
    if( has_flag( ter_furn_flag::TFLAG_NO_FLOOR, p ) && type_id.obj().phase != phase_id::GAS ) {
        return false;
    }

    // Don't spawn liquid fields on water tiles
    if( has_flag( ter_furn_flag::TFLAG_SWIMMABLE, p ) && type_id.obj().phase == phase_id::LIQUID ) {
        return false;
    }

    // Hacky way to force electricity fields to become unlit electricity fields
    const field_type_id &converted_type_id = ( type_id == fd_electricity ||
            type_id == fd_electricity_unlit ) ? get_applicable_electricity_field( p ) : type_id;
    const field_type &fd_type = *converted_type_id;

    intensity = std::min( intensity, fd_type.get_max_intensity() );
    if( intensity <= 0 ) {
        return false;
    }

    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to add field at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return false;
    }
    current_submap->ensure_nonuniform();
    invalidate_max_populated_zlev( p.z() );

    if( current_submap->get_field( l ).add_field( converted_type_id, intensity, age, source ) ) {
        //Only adding it to the count if it doesn't exist.
        if( !current_submap->field_count++ ) {
            get_cache( p.z() ).field_cache.set(
                static_cast<size_t>( p.x() / SEEX ) + ( ( p.y() / SEEX ) * MAPSIZE ) );
        }
    }

    if( hit_player ) {
        Character &player_character = get_player_character();
        if( g != nullptr && this == &reality_bubble() && p == player_character.pos_bub() ) {
            //Hit the player with the field if it spawned on top of them.
            creature_in_field( player_character );
        }
    }

    on_field_modified( p, fd_type );

    return true;
}

void map::remove_field( const tripoint_bub_ms &p, const field_type_id &field_to_remove )
{
    set_field_intensity( p, field_to_remove, 0 );
}

void map::delete_field( const tripoint_bub_ms &p, const field_type_id &field_to_remove )
{
    submap *current_submap = this->unsafe_get_submap_at( p );
    field &curfield = this->get_field( p );

    // when displayed_field_type == fd_null it means that `curfield` has no fields inside
    // avoids instantiating (relatively) expensive map iterator
    if( !curfield.displayed_field_type() ) {
        return;
    }

    for( auto it = curfield.begin(); it != curfield.end(); it++ ) {
        if( it->second.get_field_type() == field_to_remove ) {
            --current_submap->field_count;
            curfield.remove_field( it );
            set_lightmap_cache_dirty( p.z() );
            get_cache( p.z() ).field_revision = next_cache_generation();
            set_transparency_cache_dirty( p, true );
            break;
        }
    }
}

void map::clear_fields( const tripoint_bub_ms &p )
{
    if( !inbounds( p ) ) {
        return;
    }

    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    current_submap->clear_fields( l );
    set_lightmap_cache_dirty( p.z() );
    get_cache( p.z() ).field_revision = next_cache_generation();
    set_transparency_cache_dirty( p, true );
}

void map::on_field_modified( const tripoint_bub_ms &p, const field_type &fd_type )
{
    invalidate_max_populated_zlev( p.z() );
    set_lightmap_cache_dirty( p.z() );
    get_cache( p.z() ).field_revision = next_cache_generation();

    get_cache( p.z() ).field_cache.set(
        static_cast<size_t>( p.x() / SEEX ) + ( ( p.y() / SEEX ) * MAPSIZE ) );

    // Dirty the transparency cache now that field processing doesn't always do it
    if( fd_type.dirty_transparency_cache || !fd_type.is_transparent() ) {
        set_transparency_cache_dirty( p, true );
    }

    if( fd_type.is_dangerous() ) {
        set_pathfinding_cache_dirty( p );
    }

    // Ensure blood type fields don't hang in the air
    if( zlevels && fd_type.accelerated_decay ) {
        support_dirty( p );
    }
}

void map::add_splatter( const field_type_id &type, const tripoint_bub_ms &where, int intensity )
{
    if( intensity <= 0 ) {
        return;
    }
    if( type.obj().is_splattering ) {
        if( const optional_vpart_position vp = veh_at( where ) ) {
            vehicle *const veh = &vp->vehicle();
            // Might be -1 if all the vehicle's parts at where are marked for removal
            const int part = veh->part_displayed_at( vp->mount_pos(), true );
            if( part != -1 ) {
                veh->part( part ).blood += 200 * std::min( intensity, 3 ) / 3;
                return;
            }
        }
    }
    mod_field_intensity( where, type, intensity );
}

void map::add_splatter_trail( const field_type_id &type, const tripoint_bub_ms &from,
                              const tripoint_bub_ms &to )
{
    if( !type.id() ) {
        return;
    }
    const auto trail = line_to( from, to );
    int remainder = trail.size();
    for( const tripoint_bub_ms &elem : trail ) {
        add_splatter( type, elem );
        remainder--;
        if( impassable( elem ) ) { // Blood splatters stop at walls.
            add_splatter( type, elem, remainder );
            return;
        }
    }
}

void map::add_splash( const field_type_id &type, const tripoint_bub_ms &center, int radius,
                      int intensity )
{
    if( !type.id() ) {
        return;
    }
    // TODO: use Bresenham here and take obstacles into account
    for( const tripoint_bub_ms &pnt : points_in_radius( center, radius ) ) {
        if( trig_dist( pnt, center ) <= radius && !one_in( intensity ) ) {
            add_splatter( type, pnt );
        }
    }
}

computer *map::computer_at( const tripoint_bub_ms &p )
{
    if( !inbounds( p ) ) {
        return nullptr;
    }

    point_sm_ms l;
    submap *const sm = unsafe_get_submap_at( p, l );
    if( sm == nullptr ) {
        debugmsg( "Tried to get computer at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return nullptr;
    }
    return sm->get_computer( l );
}

void map::remove_submap_camp( const tripoint_bub_ms &p )
{
    submap *const current_submap = get_submap_at( p );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to remove camp at (%d,%d,%d) but the submap is not loaded", p.x(), p.y(), p.z() );
        return;
    }
    current_submap->camp.reset();
}

basecamp map::hoist_submap_camp( const tripoint_bub_ms &p )
{
    submap *const current_submap = get_submap_at( p );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to hoist camp at (%d,%d,%d) but the submap is not loaded", p.x(), p.y(), p.z() );
        return basecamp();
    }
    basecamp *pcamp = current_submap->camp.get();
    return pcamp ? *pcamp : basecamp();
}

void map::add_camp( const tripoint_abs_omt &omt_pos, const std::string &name, bool need_validate )
{
    basecamp temp_camp = basecamp( name, omt_pos );
    overmap_buffer.add_camp( temp_camp );
    get_player_character().camps.insert( omt_pos );
    // Mapgen-spawned camps never need or want to validate, since they'll always be on newly generated OMTs
    if( need_validate ) {
        g->validate_camps();
    }
}

void map::shift_traps( const point_rel_sm &shift )
{
    // Offset needs to have sign opposite to shift direction
    const tripoint_rel_ms offset( -shift.x() * SEEX, -shift.y() * SEEY, 0 );
    for( auto iter = field_furn_locs.begin(); iter != field_furn_locs.end(); ) {
        tripoint_bub_ms &pos = *iter;
        pos += offset;
        if( inbounds( pos ) ) {
            ++iter;
        } else {
            iter = field_furn_locs.erase( iter );
        }
    }
    for( auto iter = field_ter_locs.begin(); iter != field_ter_locs.end(); ) {
        tripoint_bub_ms &pos = *iter;
        pos += offset;
        if( inbounds( pos ) ) {
            ++iter;
        } else {
            iter = field_ter_locs.erase( iter );
        }
    }
    for( auto &traps : traplocs ) {
        for( auto iter = traps.begin(); iter != traps.end(); ) {
            tripoint_bub_ms &pos = *iter;
            pos += offset;
            if( inbounds( pos ) ) {
                ++iter;
            } else {
                // Theoretical enhancement: if this is not the last entry of the vector,
                // move the last entry into pos and remove the last entry instead of iter.
                // This would avoid moving all the remaining entries.
                iter = traps.erase( iter );
            }
        }
    }
}

void map::handle_decayed_corpse( const item &it, const tripoint_abs_ms &pnt )
{
    if( !it.is_corpse() ) {
        debugmsg( "Tried to decay a non-corpse item %s. Aborted", it.tname() );
        return;
    }
    const mtype *dead_monster = it.get_corpse_mon();
    if( !dead_monster ) {
        debugmsg( "Corpse at abs_ms %s has no associated monster?!", pnt.to_string() );
        return;
    }

    //FIXME: Get this working using is_null(). harvest_list and harvest_id resolving is bizarre
    if( dead_monster->decay.is_empty() ) {
        return;
    }
    int decayed_weight_grams = to_gram( dead_monster->weight ); // corpse might have stuff in it!
    decayed_weight_grams += std::round( decayed_weight_grams * rng_float( -0.1, 0.1 ) );
    bool notify_player = false;
    if( calendar::once_every( 30_minutes ) ) {
        //one corpse max in 30 minutes will notify if seen, for *all* the items it left
        notify_player = true;
    }

    bool anything_left = false;
    for( const harvest_entry &entry : *dead_monster->decay ) {
        item harvest = item( itype_id( entry.drop ) );
        const float random_decay_modifier = rng_float( 0.0f, static_cast<float>( MAX_SKILL ) );
        const float min_num = entry.scale_num.first * random_decay_modifier + entry.base_num.first;
        const float max_num = entry.scale_num.second * random_decay_modifier + entry.base_num.second;
        int roll = 0;
        if( entry.mass_ratio != 0.00f ) {
            roll = static_cast<int>( std::round( entry.mass_ratio * decayed_weight_grams ) );
            roll = std::ceil( static_cast<double>( roll ) / to_gram( harvest.type->weight ) );
        } else {
            roll = std::min<int>( entry.max, std::round( rng_float( min_num, max_num ) ) );
        }
        anything_left = roll > 0;
        for( int i = 0; i < roll; i++ ) {
            if( harvest.has_temperature() ) {
                harvest.set_item_temperature( get_weather().get_area_temperature( project_to<coords::omt>
                                              ( pnt ) ) );
            }
            add_item_or_charges( get_bub( pnt ), harvest, false );
            if( anything_left && notify_player ) {
                add_msg_if_player_sees( get_bub( pnt ),
                                        _( "You notice a %1$s has rotted away, leaving a %2$s." ),
                                        it.tname(), harvest.tname() );
            }
        }
    }
}

void map::rotten_item_spawn( const item &item, const tripoint_bub_ms &pnt )
{
    if( get_creature_tracker().creature_at( pnt ) != nullptr ) {
        return;
    }
    const auto &comest = item.get_comestible();
    mongroup_id mgroup = comest->rot_spawn.rot_spawn_group;
    mtype_id monster = comest->rot_spawn.rot_spawn_monster;
    if( mgroup.is_null() && monster.is_null() ) {
        return;
    }

    if( rng( 0, 100 ) < comest->rot_spawn.rot_spawn_chance ) {
        if( !mgroup.is_null() ) {
            std::vector<MonsterGroupResult> spawn_details = MonsterGroupManager::GetResultFromGroup( mgroup );
            for( const MonsterGroupResult &mgr : spawn_details ) {
                add_spawn( mgr, tripoint_bub_ms( pnt ) );
            }
        }
        if( !monster.is_null() ) {
            add_spawn( monster, rng( comest->rot_spawn.rot_spawn_monster_amount.first,
                                     comest->rot_spawn.rot_spawn_monster_amount.second ), tripoint_bub_ms( pnt ) );
        }
        if( get_player_view().sees( *this,  pnt ) ) {
            if( item.is_seed() ) {
                add_msg( m_warning, _( "Something has crawled out of the %s plants!" ), item.get_plant_name() );
            } else {
                add_msg( m_warning, _( "Something has crawled out of the %s!" ), item.tname() );
            }
        }
    }
}

void map::fill_funnels( const tripoint_bub_ms &p, const time_point &since )
{
    const trap &tr = tr_at( p );
    if( !tr.is_funnel() ) {
        return;
    }
    // Note: the inside/outside cache might not be correct at this time
    if( has_flag_ter_or_furn( ter_furn_flag::TFLAG_INDOORS, p ) ) {
        return;
    }
    map_stack items = i_at( p );
    units::volume maxvolume = 0_ml;
    auto biggest_container = items.end();
    for( auto candidate = items.begin(); candidate != items.end(); ++candidate ) {
        if( candidate->is_funnel_container( maxvolume ) ) {
            biggest_container = candidate;
        }
    }
    if( biggest_container != items.end() ) {
        retroactively_fill_from_funnel( *biggest_container, tr, since, calendar::turn, get_abs( p ) );
    }
}

void map::grow_plant( const tripoint_bub_ms &p )
{
    const furn_t &furn = this->furn( p ).obj();
    if( !furn.has_flag( ter_furn_flag::TFLAG_PLANT ) ) {
        return;
    }
    // Can't use item_stack::only_item() since there might be fertilizer
    map_stack items = i_at( p );
    map_stack::iterator seed = std::find_if( items.begin(), items.end(), []( const item & it ) {
        return it.is_seed();
    } );

    if( seed == items.end() ) {
        // No seed there anymore, we don't know what kind of plant it was.
        // TODO: Fix point types
        const tripoint_abs_ms ms_pos( get_abs( p ) );
        const tripoint_abs_sm sm_pos = project_to<coords::sm>( ms_pos );
        const oter_id ot = overmap_buffer.ter( project_to<coords::omt>( ms_pos ) );
        dbg( D_ERROR ) << "plant furniture has no seed item.  "
                       << "furniture: " << furn.id.str()
                       << ", submap absolute: " << sm_pos
                       << ", map square absolute: " << ms_pos
                       << ", class map map square relative: " << p
                       << ", overmap terrain: " << ot.id().str()
        << ", other items: " << enumerate_as_string( items, []( const item & it ) {
            return it.display_name();
        } );
        i_clear( p );
        furn_set( p, furn_str_id::NULL_ID() );
        return;
    }

    const std::vector<std::pair<flag_id, time_duration>> &growth_stages =
                seed->type->seed->get_growth_stages();

    flag_id current_stage = flag_id( io::enum_to_string<ter_furn_flag>
                                     ( ter_furn_flag::TFLAG_GROWTH_SEED ) );
    flag_id target_stage = flag_id( io::enum_to_string<ter_furn_flag>
                                    ( ter_furn_flag::TFLAG_GROWTH_SEED ) );
    time_duration time_to_grow_to_this_stage = 0_seconds;

    for( const auto &pair : growth_stages ) {
        if( has_flag_furn( pair.first.str(), p ) ) {
            current_stage = pair.first;
        }
        if( seed->age() >= time_to_grow_to_this_stage ) {
            target_stage = pair.first;
        } // Don't break the loop for the case where time has been rewound.
        // Advance to the time of the next stage for the next iteration.
        time_to_grow_to_this_stage += pair.second;
    }

    const auto check_flag = []( const std::string & to_check ) {
        return [&to_check]( const auto & pair_flag_and_dur ) {
            return pair_flag_and_dur.first.str() == to_check;
        };
    };

    const int stages_to_advance = std::distance(
                                      std::find_if( growth_stages.begin(), growth_stages.end(), check_flag( current_stage.str() ) ),
                                      std::find_if( growth_stages.begin(), growth_stages.end(), check_flag( target_stage.str() ) ) );

    add_msg_debug( debugmode::DF_MAP,
                   "Checking growth on a %s, aged %s. Current stage: %s. Target stage: %s. Advancing %d stages.",
                   // Human readable time please. '7863287 s' from to_string_writable() is not a reasonable output.
                   //NOLINTNEXTLINE(cata-translations-in-debug-messages)
                   seed->tname(), to_string( seed->age() ), current_stage.c_str(), target_stage.c_str(),
                   stages_to_advance );

    if( stages_to_advance <= 0 ) {
        // We don't have the logic to reverse growth transforms, so leave the stage as is on a time rewind.
        return;
    }

    for( int i = 0; i < stages_to_advance; i++ ) {
        // Remove fertilizer if any
        map_stack::iterator fertilizer = std::find_if( items.begin(), items.end(), []( const item & it ) {
            return it.has_flag( flag_FERTILIZER );
        } );
        if( fertilizer != items.end() ) {
            items.erase( fertilizer );
        }
        // spawn appropriate amount of rot_spawn, equivalent to number of times we iterate this loop
        rotten_item_spawn( *seed, tripoint_bub_ms( p ) );
        // Get an updated reference to the furniture each time we go through this loop, to make sure we transform each step in turn
        const furn_t &current_furn = this->furn( p ).obj();
        furn_set( p, furn_str_id( current_furn.plant->transform ) );
    }
}

void map::restock_fruits( const tripoint_bub_ms &p, const time_duration &time_since_last_actualize )
{
    const ter_t &ter = this->ter( p ).obj();
    if( !ter.has_flag( ter_furn_flag::TFLAG_HARVESTED ) ) {
        return; // Already harvestable. Do nothing.
    }
    // Make it harvestable again if the last actualization was during a different season or year.
    const time_point last_touched = calendar::turn - time_since_last_actualize;
    if( season_of_year( calendar::turn ) != season_of_year( last_touched ) ||
        time_since_last_actualize >= calendar::season_length() ) {
        ter_set( p, ter.transforms_into );
    }
}

void map::produce_sap( const tripoint_bub_ms &p, const time_duration &time_since_last_actualize )
{
    if( time_since_last_actualize <= 0_turns ) {
        return;
    }

    if( !( ter( p ) == ter_t_tree_maple_tapped ) ) {
        return;
    }

    // Amount of maple sap liters produced per season per tap
    static const int maple_sap_per_season = 56;

    // How many turns to produce 1 charge (250 ml) of sap?
    const time_duration producing_length = 0.75 * calendar::season_length();

    const time_duration turns_to_produce = producing_length / ( maple_sap_per_season * 4 );

    // How long of this time_since_last_actualize have we been in the producing period (late winter, early spring)?
    time_duration time_producing = 0_turns;

    if( time_since_last_actualize >= calendar::year_length() ) {
        time_producing = producing_length;
    } else {
        // We are only producing sap on the intersection with the sap producing season.
        const time_duration early_spring_end = 0.5f * calendar::season_length();
        const time_duration late_winter_start = 3.75f * calendar::season_length();

        const time_point last_actualize = calendar::turn - time_since_last_actualize;
        const time_duration last_actualize_tof = time_past_new_year( last_actualize );
        bool last_producing = last_actualize_tof >= late_winter_start ||
                              last_actualize_tof < early_spring_end;
        const time_duration current_tof = time_past_new_year( calendar::turn );
        bool current_producing = current_tof >= late_winter_start ||
                                 current_tof < early_spring_end;

        const time_duration non_producing_length = 3.25 * calendar::season_length();

        if( last_producing && current_producing ) {
            if( time_since_last_actualize < non_producing_length ) {
                time_producing = time_since_last_actualize;
            } else {
                time_producing = time_since_last_actualize - non_producing_length;
            }
        } else if( !last_producing && !current_producing ) {
            if( time_since_last_actualize > non_producing_length ) {
                time_producing = time_since_last_actualize - non_producing_length;
            }
        } else if( last_producing && !current_producing ) {
            // We hit the end of early spring
            if( last_actualize_tof < early_spring_end ) {
                time_producing = early_spring_end - last_actualize_tof;
            } else {
                time_producing = calendar::year_length() - last_actualize_tof + early_spring_end;
            }
        } else if( !last_producing && current_producing ) {
            // We hit the start of late winter
            if( current_tof >= late_winter_start ) {
                time_producing = current_tof - late_winter_start;
            } else {
                time_producing = 0.25f * calendar::season_length() + current_tof;
            }
        }
    }

    int new_charges = roll_remainder( time_producing / turns_to_produce );
    // Not enough time to produce 1 charge of sap
    if( new_charges <= 0 ) {
        return;
    }

    item sap( itype_maple_sap, calendar::turn );

    sap.set_item_temperature( get_weather().get_temperature( p ) );
    sap.charges = new_charges;

    // Is there a proper container?
    map_stack items = i_at( p );
    for( item &it : items ) {
        if( it.will_spill() || it.is_watertight_container() ) {
            const int capacity = it.get_remaining_capacity_for_liquid( sap, true );
            if( capacity > 0 ) {
                sap.charges = std::min( sap.charges, capacity );

                // The environment might have poisoned the sap with animals passing by, insects, leaves or contaminants in the ground
                sap.poison = one_in( 10 ) ? 1 : 0;

                it.put_in( sap, pocket_type::CONTAINER );
            }
            // Only fill up the first container.
            break;
        }
    }
}

void map::cut_down_tree( tripoint_bub_ms p, point_rel_ms dir )
{
    if( !zlevels ) {
        debugmsg( "Call to cut_down_tree from a map that doesn't support zlevels." );
        return;
    }

    if( !ter( p ).obj().has_flag( ter_furn_flag::TFLAG_TREE ) ) {
        debugmsg( "Call to cut_down_tree on a tile that doesn't contain a tree." );
        return;
    }

    tripoint_bub_ms to = p + 3 * dir + point( rng( -1, 1 ), rng( -1, 1 ) );

    // TODO: make line_to type aware.
    std::vector<tripoint_bub_ms> tree = line_to( p, to, rng( 1, 8 ) );
    bool shattered_tree = false;
    const int bash_strength = 79; // arbitrary bash strength, not enough to ever destroy concrete wall
    const int bash_attempts = 1;
    for( tripoint_bub_ms &elem : tree ) {
        const ter_id original_terrain_at = ter( elem.xy() );
        bool did_bash_this_loop = false;
        if( !shattered_tree ) {
            // if this terrain is open, it's fine if we don't bash it.
            const bool open_ground = has_flag( ter_furn_flag::TFLAG_FLAT, elem );
            did_bash_this_loop = true;
            batter( elem, bash_strength, bash_attempts );
            const bool smashed_thru = ter( elem.xy() ) != original_terrain_at;
            if( ( open_ground || smashed_thru ) && passable( elem.xy() ) ) {
                ter_set( elem, ter_t_trunk ); // effectively continues, no further execution this loop.
                continue;                     // but just to be sure :)
            } else {
                shattered_tree = true;
            }
        }
        // completely new if block, instead of if-else. In case we set shattered_tree in this same pass through the for-loop.
        if( shattered_tree ) {
            // Then we do some shuffling to "simulate" the trunk being destroyed.
            // We bash the terrain normally once, and we pretend the trunk itself was bashed by spawning its bash results.
            // But we never set the trunk terrain, because a lower part of the trunk was destroyed.
            if( !did_bash_this_loop ) {
                batter( elem, bash_strength, bash_attempts );
            }
            // safety: trunk might not have bash data! It does right now, but maybe we remove that in the future(??)
            if( ter_t_trunk->bash ) {
                spawn_items( elem, item_group::items_from( ter_t_trunk->bash->drop_group, calendar::turn ) );
            }
        }
    }
    ter_set( p, ter_t_stump );
}

void map::rad_scorch( const tripoint_bub_ms &p, const time_duration &time_since_last_actualize )
{
    const int rads = get_radiation( p );
    if( rads == 0 ) {
        return;
    }

    // TODO: More interesting rad scorch chance - base on season length?
    if( !x_in_y( 1.0 * rads * rads * time_since_last_actualize, 91_days ) ) {
        return;
    }

    // First destroy the farmable plants (those are furniture)
    // TODO: Rad-resistant mutant plants (that produce radioactive fruit)
    const furn_t &fid = furn( p ).obj();
    if( fid.has_flag( ter_furn_flag::TFLAG_PLANT ) ) {
        i_clear( p );
        furn_set( p, furn_str_id::NULL_ID() );
    }

    const ter_id &tid = ter( p );
    // TODO: De-hardcode this
    static const std::map<ter_id, ter_str_id> dies_into {{
            {ter_t_grass, ter_t_dirt},
            {ter_t_tree_young, ter_t_dirt},
            {ter_t_tree_pine, ter_t_tree_deadpine},
            {ter_t_tree_birch, ter_t_tree_birch_harvested},
            {ter_t_tree_willow, ter_t_tree_willow_harvested},
            {ter_t_tree_hickory, ter_t_tree_hickory_dead},
            {ter_t_tree_hickory_harvested, ter_t_tree_hickory_dead},
        }};

    const auto iter = dies_into.find( tid );
    if( iter != dies_into.end() ) {
        ter_set( p, iter->second );
        return;
    }

    const ter_t &tr = tid.obj();
    if( tr.has_flag( ter_furn_flag::TFLAG_SHRUB ) ) {
        ter_set( p, ter_t_dirt );
    } else if( tr.has_flag( ter_furn_flag::TFLAG_TREE ) ) {
        ter_set( p, ter_t_tree_dead );
    }
}

void map::decay_cosmetic_fields( const tripoint_bub_ms &p,
                                 const time_duration &time_since_last_actualize )
{
    for( auto &pr : field_at( p ) ) {
        field_entry &fd = pr.second;
        const time_duration hl = fd.get_field_type().obj().half_life;
        if( !fd.get_field_type()->accelerated_decay || hl <= 0_turns ) {
            continue;
        }

        const time_duration added_age = 2 * time_since_last_actualize / rng( 2, 4 );
        fd.mod_field_age( added_age );
        const int intensity_drop = fd.get_field_age() / hl;
        if( intensity_drop > 0 ) {
            fd.set_field_intensity( fd.get_field_intensity() - intensity_drop );
            fd.mod_field_age( -hl * intensity_drop );
        }
    }
}

void map::add_tree_tops( const tripoint_rel_sm &grid )
{
    if( !zlevels ) {
        // Can't add things on the level above when the map doesn't contain that level.
        return;
    }

    submap *const sub_here = get_submap_at_grid( grid );
    if( sub_here == nullptr ) {
        debugmsg( "Tried to add tree tops on null submap on %d,%d,%d",
                  grid.x(), grid.y(), grid.z() );
        return;
    }

    bool check_tree_tops = grid.z() > -OVERMAP_DEPTH;

    submap *const sub_below = check_tree_tops ? get_submap_at_grid( grid + tripoint_rel_sm::below ) :
                              nullptr;

    if( check_tree_tops && sub_below == nullptr ) {
        debugmsg( "Tried to add tree tops to sm at %d,%d,%d, but sm below doesn't exist",
                  grid.x(), grid.y(), grid.z() );
        return;
    }

    for( int x = 0; x < SEEX; x++ ) {
        for( int y = 0; y < SEEY; y++ ) {
            const ter_id &ter_here = sub_here->get_ter( { x, y } );

            if( !ter_here.id()->has_flag( "EMPTY_SPACE" ) ) {
                continue;
            }

            if( !check_tree_tops ) {
                // Make sure we don't have empty space at lowest z-level
                sub_here->set_ter( { x, y }, ter_t_rock_floor );
                continue;
            }

            const ter_t &ter_below = sub_below->get_ter( { x, y } ).obj();
            if( ter_below.has_flag( "TREE" ) && ter_below.roof ) {
                sub_here->set_ter( { x, y }, ter_below.roof.id() );
            } else
                // This code is needed to handle bashing during mapgen, because the Z level below
                // hasn't yet been generated when the bashing occurs.
                if( ter_here.id() == ter_t_open_air ) {
                    if( ter_below.has_flag( "NATURAL_UNDERGROUND" ) ) {
                        sub_here->set_ter( {x, y}, ter_below.roof );
                    }
                }
        }
    }
}

void map::clear_traps()
{
    for( submap *&smap : grid ) {
        for( int x = 0; x < SEEX; x++ ) {
            for( int y = 0; y < SEEY; y++ ) {
                const point_sm_ms p( x, y );
                smap->set_trap( p, tr_null );
            }
        }
    }

    // Forget about all trap locations.
    for( auto &i : traplocs ) {
        i.clear();
    }
}

const std::vector<tripoint_bub_ms> &map::get_furn_field_locations() const
{
    return field_furn_locs;
}

const std::vector<tripoint_bub_ms> &map::get_ter_field_locations() const
{
    return field_ter_locs;
}

const std::vector<tripoint_bub_ms> &map::trap_locations( const trap_id &type ) const
{
    return traplocs[type.to_i()];
}

void map::set_graffiti( const tripoint_bub_ms &p, const std::string &contents )
{
    if( !inbounds( p ) ) {
        return;
    }
    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to set graffiti at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return;
    }
    current_submap->set_graffiti( l, contents );
}

void map::delete_graffiti( const tripoint_bub_ms &p )
{
    if( !inbounds( p ) ) {
        return;
    }
    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to delete graffiti at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return;
    }
    current_submap->delete_graffiti( l );
}

const std::string &map::graffiti_at( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        static const std::string empty_string;
        return empty_string;
    }
    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to get graffiti at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        static const std::string empty_string;
        return empty_string;
    }
    return current_submap->get_graffiti( l );
}

bool map::has_graffiti_at( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }
    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to get graffiti at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return false;
    }
    return current_submap->has_graffiti( l );
}

void map::add_corpse( const tripoint_bub_ms &p )
{
    item body;

    const bool isReviveSpecial = one_in( 10 );

    if( !isReviveSpecial ) {
        body = item::make_corpse();
    } else {
        body = item::make_corpse( mon_zombie );
        body.set_flag( flag_REVIVE_SPECIAL );
    }

    put_items_from_loc( Item_spawn_data_default_zombie_clothes, p, calendar::turn_zero );
    if( one_in( 3 ) ) {
        put_items_from_loc( Item_spawn_data_default_zombie_items, p, calendar::turn_zero );
    }

    add_item_or_charges( p, body );
}

field &map::get_field( const tripoint_bub_ms &p )
{
    return field_at( p );
}

void map::creature_on_trap( Creature &c, const bool may_avoid ) const
{
    // gliding or boarded in a vehicle means the player is above the trap
    // like a flying monster and can never trigger the trap.
    const Character *const you = c.as_character();
    if( you != nullptr && ( you->in_vehicle || you->has_effect( effect_gliding ) ) ) {
        return;
    }

    tripoint_bub_ms pos = c.pos_bub();
    // proximity traps
    std::vector<tripoint_bub_ms> tr_proximity;
    // find proximity traps in adjacent tiles
    for( int x = pos.x() - 1; x <= pos.x() + 1; x++ ) {
        for( int y = pos.y() - 1; y <= pos.y() + 1; y++ ) {
            if( x == pos.x() && y == pos.y() ) {
                continue;
            }
            const tripoint_bub_ms loc{ x, y, pos.z()};
            const trap *trap_here = &tr_at( loc );
            if( trap_here->has_flag( json_flag_PROXIMITY ) ) {
                tr_proximity.push_back( loc );
            }
        }
    }
    // first trigger proximity traps
    for( tripoint_bub_ms &loc : tr_proximity ) {
        maybe_trigger_prox_trap( loc, c, may_avoid );
    }
    // then traps we stepped on
    maybe_trigger_trap( pos, c, may_avoid );
}


void map::maybe_trigger_prox_trap( const tripoint_bub_ms &pos, Creature &c,
                                   const bool may_avoid ) const
{
    const trap &tr = tr_at( pos );
    if( tr.is_null() ) {
        return;
    }

    //Don't trigger benign traps like cots and funnels
    if( tr.is_benign() ) {
        return;
    }

    if( tr.has_flag( json_flag_AVATAR_ONLY ) && !c.is_avatar() ) {
        return;
    }

    if( !tr.has_flag( json_flag_UNDODGEABLE ) && may_avoid && c.avoid_trap( pos, tr ) ) {
        Character *const pl = c.as_character();
        if( !tr.is_always_invisible() && pl && !pl->knows_trap( pos ) ) {
            pl->add_msg_if_player( _( "You've spotted a %1$s!" ), tr.name() );
            pl->add_known_trap( pos, tr );
        }
        return;
    }

    if( !tr.is_always_invisible() && tr.has_trigger_msg() ) {
        c.add_msg_player_or_npc( m_bad, tr.get_trigger_message_u(), tr.get_trigger_message_npc(),
                                 tr.name() );
    }
    tr.trigger( pos, c );
}

void map::maybe_trigger_trap( const tripoint_bub_ms &pos, Creature &c, const bool may_avoid ) const
{
    const trap &tr = tr_at( pos );
    if( tr.is_null() ) {
        return;
    }

    //Don't trigger benign traps like cots and funnels
    if( tr.is_benign() ) {
        return;
    }

    if( tr.has_flag( json_flag_AVATAR_ONLY ) && !c.is_avatar() ) {
        return;
    }

    if( !tr.has_flag( json_flag_UNDODGEABLE ) && may_avoid && c.avoid_trap( pos, tr ) ) {
        Character *const pl = c.as_character();
        if( !tr.is_always_invisible() && pl && !pl->knows_trap( pos ) ) {
            pl->add_msg_if_player( _( "You've spotted a %1$s!" ), tr.name() );
            pl->add_known_trap( pos, tr );
        }
        return;
    }

    if( !tr.is_always_invisible() && tr.has_trigger_msg() ) {
        c.add_msg_player_or_npc( m_bad, tr.get_trigger_message_u(), tr.get_trigger_message_npc(),
                                 tr.name() );
    }
    tr.trigger( c.pos_bub(), c );
}
