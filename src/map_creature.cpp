#include "map.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <optional>
#include <ostream>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#include "active_item_cache.h"
#include "avatar.h"
#include "basecamp.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_type_traits.h"
#include "character.h"
#include "character_id.h"
#include "clzones.h"
#include "colony.h"
#include "color.h"
#include "construction.h"
#include "coordinates.h"
#include "craft_reservation.h"
#include "creature.h"
#include "creature_tracker.h"
#include "cuboid_rectangle.h"
#include "current_map.h"
#include "cursesdef.h"
#include "debug.h"
#include "drawing_primitives.h"
#include "enum_conversions.h"
#include "enums.h"
#include "explosion.h"
#include "field.h"
#include "field_type.h"
#include "flag.h"
#include "game.h"
#include "harvest.h"
#include "input.h"
#include "item.h"
#include "item_category.h"
#include "item_factory.h"
#include "item_group.h"
#include "item_location.h"
#include "item_stack_use_charges.h"
#include "item_transformation.h"
#include "item_wakeup.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "lightmap.h"
#include "line.h"
#include "magic_ter_furn_transform.h"
#include "map_iterator.h"
#include "map_memory.h"
#include "map_selector.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "mapgen_primitives.h"
#include "memory_fast.h"
#include "messages.h"
#include "mission.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "omdata.h"
#include "output.h"
#include "overmap_map_data_cache.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "pocket_type.h"
#include "relic.h"
#include "ret_val.h"
#include "rng.h"
#include "safe_reference.h"
#include "scent_map.h"
#include "sounds.h"
#include "string_formatter.h"
#include "submap.h"
#include "translations.h"
#include "trap.h"
#include "uilist.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "viewer.h"
#include "visitable.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"
#include "weighted_list.h"

class weather_generator;

static const ammotype ammo_battery( "battery" );

static const efftype_id effect_crushed( "crushed" );
static const efftype_id effect_fake_common_cold( "fake_common_cold" );
static const efftype_id effect_fake_flu( "fake_flu" );
static const efftype_id effect_gliding( "gliding" );
static const efftype_id effect_incorporeal( "incorporeal" );
static const efftype_id effect_pet( "pet" );

static const flag_id json_flag_AVATAR_ONLY( "AVATAR_ONLY" );
static const flag_id json_flag_DIGGABLE( "DIGGABLE" );
static const flag_id json_flag_EASY_DECONSTRUCT( "EASY_DECONSTRUCT" );
static const flag_id json_flag_FLAT( "FLAT" );
static const flag_id json_flag_PHASE_BACK( "PHASE_BACK" );
static const flag_id json_flag_PLOWABLE( "PLOWABLE" );
static const flag_id json_flag_PROXIMITY( "PROXIMITY" );
static const flag_id json_flag_UNDODGEABLE( "UNDODGEABLE" );

static const furn_str_id furn_f_clear( "f_clear" );
static const furn_str_id furn_f_rubble( "f_rubble" );
static const furn_str_id furn_f_rubble_rock( "f_rubble_rock" );
static const furn_str_id furn_f_wreckage( "f_wreckage" );

static const item_group_id Item_spawn_data_default_zombie_clothes( "default_zombie_clothes" );
static const item_group_id Item_spawn_data_default_zombie_items( "default_zombie_items" );

static const itype_id itype_HEW_printout_data_amigara( "HEW_printout_data_amigara" );
static const itype_id
itype_HEW_printout_data_barricaded_church( "HEW_printout_data_barricaded_church" );
static const itype_id itype_HEW_printout_data_cabin_reverb( "HEW_printout_data_cabin_reverb" );
static const itype_id
itype_HEW_printout_data_cabin_reverb_dimension( "HEW_printout_data_cabin_reverb_dimension" );
static const itype_id itype_HEW_printout_data_exodii( "HEW_printout_data_exodii" );
static const itype_id itype_HEW_printout_data_highlands( "HEW_printout_data_highlands" );
static const itype_id
itype_HEW_printout_data_inner_cabins_home_cabin( "HEW_printout_data_inner_cabins_home_cabin" );
static const itype_id
itype_HEW_printout_data_inner_cabins_open_field( "HEW_printout_data_inner_cabins_open_field" );
static const itype_id
itype_HEW_printout_data_inner_cabins_warped_cabin( "HEW_printout_data_inner_cabins_warped_cabin" );
static const itype_id itype_HEW_printout_data_labyrinth( "HEW_printout_data_labyrinth" );
static const itype_id itype_HEW_printout_data_lixa( "HEW_printout_data_lixa" );
static const itype_id itype_HEW_printout_data_monster_corpse( "HEW_printout_data_monster_corpse" );
static const itype_id itype_HEW_printout_data_morgantown( "HEW_printout_data_morgantown" );
static const itype_id itype_HEW_printout_data_physics_lab( "HEW_printout_data_physics_lab" );
static const itype_id itype_HEW_printout_data_portal( "HEW_printout_data_portal" );
static const itype_id itype_HEW_printout_data_portal_storm( "HEW_printout_data_portal_storm" );
static const itype_id
itype_HEW_printout_data_portal_storm_dungeon( "HEW_printout_data_portal_storm_dungeon" );
static const itype_id itype_HEW_printout_data_radiosphere( "HEW_printout_data_radiosphere" );
static const itype_id itype_HEW_printout_data_spiral_mine( "HEW_printout_data_spiral_mine" );
static const itype_id itype_HEW_printout_data_strange_temple( "HEW_printout_data_strange_temple" );
static const itype_id
itype_HEW_printout_data_string_dimension( "HEW_printout_data_string_dimension" );
static const itype_id itype_HEW_printout_data_vitrified( "HEW_printout_data_vitrified" );
static const itype_id
itype_HEW_printout_data_void_spider_lair( "HEW_printout_data_void_spider_lair" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_maple_sap( "maple_sap" );
static const itype_id itype_mws_monster_corpse_weather_data( "mws_monster_corpse_weather_data" );
static const itype_id itype_mws_portal_storm_weather_data( "mws_portal_storm_weather_data" );
static const itype_id itype_mws_weather_data( "mws_weather_data" );
static const itype_id itype_mws_weather_data_incomplete( "mws_weather_data_incomplete" );
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

static const species_id species_FERAL( "FERAL" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_dirtmound( "t_dirtmound" );
static const ter_str_id ter_t_grass( "t_grass" );
static const ter_str_id ter_t_open_air( "t_open_air" );
static const ter_str_id ter_t_rock_floor( "t_rock_floor" );
static const ter_str_id ter_t_rootcellar( "t_rootcellar" );
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

static const trap_str_id tr_portal( "tr_portal" );
static const trap_str_id tr_unfinished_construction( "tr_unfinished_construction" );

#define dbg(x) DebugLog((x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

static cata::colony<item> nulitems;          // Returned when &i_at() is asked for an OOB value
static field              nulfield;          // Returned when &field_at() is asked for an OOB value

namespace
{

void for_each_item_at( map &here, const tripoint_bub_ms &p, const Character *ch,
                       bool filter_vehicle_ownership, bool skip_ground_liquids,
                       const std::function<void( const item & )> &fn )
{
    if( here.has_items( p ) && here.accessible_items( p ) ) {
        for( const item &it : here.i_at( p ) ) {
            if( ch && !it.is_owned_by( *ch, true ) ) {
                continue;
            }
            if( !skip_ground_liquids || !it.made_of( phase_id::LIQUID ) ) {
                fn( it );
            }
        }
    }
    if( const std::optional<vpart_reference> vp = here.veh_at( p ).cargo() ) {
        for( const item &it : vp->items() ) {
            if( filter_vehicle_ownership && ch && !it.is_owned_by( *ch, true ) ) {
                continue;
            }
            fn( it );
        }
    }
}

} // namespace

// Map stack methods.
map_stack::iterator map_stack::erase( map_stack::const_iterator it )
{
    return myorigin->i_rem( location, it );
}

void map_stack::insert( map &, const item &newitem )
{
    myorigin->add_item_or_charges( location, newitem );
}

void map_stack::insert( const item &newitem )
{
    myorigin->add_item_or_charges( location, newitem );
}

units::volume map_stack::max_volume() const
{
    if( !myorigin->inbounds( location ) ) {
        return 0_ml;
    } else if( myorigin->has_furn( location ) ) {
        return myorigin->furn( location ).obj().max_volume;
    }
    return myorigin->ter( location ).obj().max_volume;
}

// Map class methods.

map::map( int mapsize, bool zlev ) : my_MAPSIZE( mapsize ), my_HALF_MAPSIZE( mapsize / 2 ),
    zlevels( zlev )
{

    if( zlevels ) {
        grid.resize( static_cast<size_t>( my_MAPSIZE ) * my_MAPSIZE * OVERMAP_LAYERS, nullptr );
    } else {
        grid.resize( static_cast<size_t>( my_MAPSIZE ) * my_MAPSIZE, nullptr );
    }

    for( auto &ptr : pathfinding_caches ) {
        ptr = std::make_unique<pathfinding_cache>();
    }

    dbg( D_INFO ) << "map::map(): my_MAPSIZE: " << my_MAPSIZE << " z-levels enabled:" << zlevels;
    traplocs.resize( trap::count() );
}

map::~map()
{
    if( ( _main_requires_cleanup && !_main_cleanup_override ) ||
        ( _main_cleanup_override && *_main_cleanup_override ) ) {
        map &bubble_map = reality_bubble();

        bubble_map.reset_vehicles_sm_pos();
        bubble_map.rebuild_vehicle_level_caches();
        g->load_npcs();
    }
}
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
map &map::operator=( map && ) = default;

static submap null_submap;

const_maptile map::maptile_at( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return const_maptile( &null_submap, point_sm_ms::zero );
    }

    return maptile_at_internal( p );
}

maptile map::maptile_at( const tripoint_bub_ms &p )
{
    if( !inbounds( p ) ) {
        return maptile( &null_submap, point_sm_ms::zero );
    }

    return maptile_at_internal( p );
}

const_maptile map::maptile_at_internal( const tripoint_bub_ms &p ) const
{
    point_sm_ms l;
    const submap *const sm = get_submap_at( p, l );

    return const_maptile( sm, l );
}

maptile map::maptile_at_internal( const tripoint_bub_ms &p )
{
    point_sm_ms l;
    submap *const sm = get_submap_at( p, l );

    return maptile( sm, l );
}

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

// End of 3D vehicle

void map::set( const tripoint_bub_ms &p, const ter_id &new_terrain, const furn_id &new_furniture )
{
    furn_set( p, new_furniture );
    ter_set( p, new_terrain );
}

std::string map::name( const tripoint_bub_ms &p )
{
    return has_furn( p ) ? furnname( p ) : tername( p );
}

std::string map::disp_name( const tripoint_bub_ms &p )
{
    return string_format( _( "the %s" ), name( p ) );
}

bool map::has_furn( const tripoint_bub_ms &p ) const
{
    return furn( p ) != furn_str_id::NULL_ID();
}

furn_id map::furn( const tripoint_bub_ms p ) const
{
    if( !inbounds( p ) ) {
        return furn_str_id::NULL_ID();
    }

    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried process furniture at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return furn_str_id::NULL_ID();
    }

    return current_submap->get_furn( l );
}

bool map::furn_set( const tripoint_bub_ms &p, const furn_id &new_furniture, const bool furn_reset,
                    bool avoid_creatures, bool allow_on_open_air )
{
    if( !inbounds( p ) ) {
        debugmsg( "map::furn_set %s out of bounds", p.to_string() );
        return false;
    }
    if( avoid_creatures ) {
        Creature *c = get_creature_tracker().creature_at( get_abs( p ), true );
        if( c ) {
            return false;
        }
    }
    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to set furniture at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return false;
    }
    const furn_id &new_target_furniture = new_furniture == furn_f_clear ? furn_str_id::NULL_ID() :
                                          new_furniture;
    const furn_id &old_id = current_submap->get_furn( l );
    if( old_id == new_target_furniture ) {
        // Nothing changed
        return true;
    }

    if( !mapgen_in_progress ) {
        // TODO: Consider evaluating whether this is a "meaningful" adjustment instead of just any edit.
        current_submap->player_adjusted_map = true;
    }
    current_submap->set_furn( l, new_target_furniture );
    current_submap->set_map_damage( point_sm_ms( l ), 0 );
    clear_original_terrain_at( p );

    // Set the dirty flags
    const furn_t &old_f = old_id.obj();
    const furn_t &new_f = new_target_furniture.obj();
    bump_geometry_revision( p.z() );
    if( cast_reached( p ) ) {
        get_cache( p.z() ).seen_cache_dirty = true;
    }

    bool result = true;

    if( !allow_on_open_air && current_submap->is_open_air( l ) &&
        !new_f.has_flag( ter_furn_flag::TFLAG_ALLOW_ON_OPEN_AIR ) &&
        !new_f.has_flag( ter_furn_flag::TFLAG_FLOATS_IN_AIR ) &&
        new_target_furniture != furn_str_id::NULL_ID() ) {
        const ter_id &current_ter = current_submap->get_ter( l );
        debugmsg( "Setting furniture %s at %s where terrain is %s (which is_open_air)\n"
                  "If this is intentional, set the ALLOW_ON_OPEN_AIR flag on the furniture",
                  new_target_furniture.id().str(), p.to_string(), current_ter.id().str() );
        result = false;
    }

    if( new_f.has_flag( ter_furn_flag::TFLAG_PLANT ) ) {
        if( current_submap->get_ter( l ) == ter_t_dirtmound ) {
            ter_set( p, ter_t_dirt );
        }
    }

    avatar &player_character = get_avatar();
    // If player has grabbed this furniture and it's no longer grabbable, release the grab.
    if( player_character.get_grab_type() == object_type::FURNITURE &&
        !furn_reset &&
        player_character.pos_bub() + player_character.grab_point == p && !new_f.is_movable() ) {
        add_msg( _( "The %s you were grabbing is destroyed!" ), old_f.name() );
        player_character.grab( object_type::NONE );
    }
    // If a creature was crushed under a rubble -> free it
    if( old_id == furn_f_rubble && new_target_furniture == furn_str_id::NULL_ID() ) {
        Creature *c = get_creature_tracker().creature_at( p );
        if( c ) {
            c->remove_effect( effect_crushed );
        }
    }
    if( !new_f.emissions.empty() ) {
        field_furn_locs.push_back( p );
    }
    if( old_f.transparent != new_f.transparent ||
        old_f.has_flag( ter_furn_flag::TFLAG_TRANSLUCENT ) != new_f.has_flag(
            ter_furn_flag::TFLAG_TRANSLUCENT ) ) {
        set_transparency_cache_dirty( p );
    }

    if( old_f.light_emitted != new_f.light_emitted ||
        old_f.light_color != new_f.light_color ) {
        set_lightmap_cache_dirty( p.z() );
    }

    if( old_f.has_flag( ter_furn_flag::TFLAG_INDOORS ) != new_f.has_flag(
            ter_furn_flag::TFLAG_INDOORS ) ) {
        set_outside_cache_dirty( p.z() );
    }

    if( old_f.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) != new_f.has_flag(
            ter_furn_flag::TFLAG_NO_FLOOR ) ) {
        set_floor_cache_dirty( p.z() );
        get_creature_tracker().invalidate_reachability_cache();
    }

    if( old_f.has_flag( ter_furn_flag::TFLAG_SUN_ROOF_ABOVE ) != new_f.has_flag(
            ter_furn_flag::TFLAG_SUN_ROOF_ABOVE ) ) {
        set_floor_cache_dirty( p.z() + 1 );
    }

    invalidate_max_populated_zlev( p.z() );

    memory_cache_dec_set_dirty( p, true );
    if( player_character.sees( *this, p ) ) {
        player_character.memorize_clear_decoration( get_abs( p ), "f_" );
    }

    if( ( old_f.movecost < 0 ) != ( new_f.movecost < 0 ) ) {
        get_creature_tracker().invalidate_reachability_cache();
    }
    // TODO: Limit to changes that affect move cost, traps and stairs
    set_pathfinding_cache_dirty( p );

    // Make sure the furniture falls if it needs to
    support_dirty( p );
    tripoint_bub_ms above( p.xy(), p.z() + 1 );
    // Make sure that if we supported something and no longer do so, it falls down
    support_dirty( above );

    return result;
}

bool map::can_move_furniture( const tripoint_bub_ms &pos, Character *you ) const
{
    if( !you ) {
        return false;
    }
    const furn_t &furniture_type = furn( pos ).obj();
    int required_str = furniture_type.move_str_req;

    // Object can not be moved (or nothing there)
    if( required_str < 0 ) {
        return false;
    }

    ///\EFFECT_STR determines what furniture the player can move
    int adjusted_str = you->get_str();
    if( you->is_mounted() ) {
        auto *mons = you->mounted_creature.get();
        if( mons->has_flag( mon_flag_RIDEABLE_MECH ) && mons->mech_str_addition() != 0 ) {
            adjusted_str = mons->mech_str_addition();
        }
    }
    return adjusted_str >= required_str;
}

std::string map::furnname( const tripoint_bub_ms &p )
{
    const furn_t &f = furn( p ).obj();
    if( f.has_flag( ter_furn_flag::TFLAG_PLANT ) ) {
        // Can't use item_stack::only_item() since there might be fertilizer
        map_stack items = i_at( p );
        const map_stack::iterator seed = std::find_if( items.begin(), items.end(), []( const item & it ) {
            return it.is_seed();
        } );
        if( seed == items.end() ) {
            debugmsg( "Missing seed for plant at (%d, %d, %d)", p.x(), p.y(), p.z() );
            return "null";
        }
        const std::string &plant = seed->get_plant_name();
        return string_format( "%s (%s)", f.name(), plant );
    } else {
        return f.name();
    }
}

/*
 * Get the terrain integer id. This is -not- a number guaranteed to remain
 * the same across revisions; it is a load order, and can change when mods
 * are loaded or removed. The old t_floor style constants will still work but
 * are -not- guaranteed; if a mod removes t_lava, t_lava will equal t_null;
 * New terrains added to the core game generally do not need this, it's
 * retained for high performance comparisons, save/load, and gradual transition
 * to string terrain.id
 */
ter_id map::ter( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return ter_str_id::NULL_ID().id();
    }

    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to process terrain at (%d,%d) of submap of (%d,%d,%d) but the submap is not loaded.  my_MAPSIZE: %d",
                  l.x(), l.y(), p.x(), p.y(), p.z(), my_MAPSIZE );
        return ter_str_id::NULL_ID().id();
    }

    return current_submap->get_ter( l );
}

bool map::has_original_terrain_at( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }
    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Called has_original_terrain_at for unloaded submap" );
        return false;
    }
    return current_submap->has_original_ter( l );
}

ter_id map::get_original_terrain_at( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return ter_id();
    }
    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Called get_original_terrain_at for unloaded submap" );
        return ter_id();
    }
    return current_submap->get_original_ter( l );
}

void map::set_original_terrain_at( const tripoint_bub_ms &p, const ter_id &t )
{
    if( !inbounds( p ) ) {
        return;
    }
    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Called set_original_terrain_at for unloaded submap" );
        return;
    }
    current_submap->set_original_ter( l, t );
}

void map::clear_original_terrain_at( const tripoint_bub_ms &p )
{
    if( !inbounds( p ) ) {
        return;
    }
    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Called clear_original_terrain_at for unloaded submap" );
        return;
    }
    current_submap->clear_original_ter( l );
}

uint8_t map::get_known_connections( const tripoint_bub_ms &p,
                                    const std::bitset<NUM_TERCONN> &connect_group,
                                    const std::map<tripoint_bub_ms, ter_id> &override ) const
{
    if( connect_group.none() ) {
        return 0;
    }

    const level_cache &ch = access_cache( p.z() );
    uint8_t val = 0;
    std::function<bool( const tripoint_bub_ms & )> is_memorized;
    avatar &player_character = get_avatar();
#ifdef TILES
    if( use_tiles ) {
        is_memorized =
        [&]( const tripoint_bub_ms & q ) {
            return !player_character.get_memorized_tile( get_abs( q ) ).get_ter_id().empty();
        };
    } else {
#endif
        is_memorized =
        [&]( const tripoint_bub_ms & q ) {
            return player_character.get_memorized_tile( get_abs( q ) ).symbol != 0;
        };
#ifdef TILES
    }
#endif

    const bool overridden = override.find( p ) != override.end();
    const bool is_transparent = ch.transparency_cache[p.x()][p.y()] > LIGHT_TRANSPARENCY_SOLID;

    // populate connection information
    for( int i = 0; i < 4; ++i ) {
        tripoint_bub_ms neighbour = p + offsets[i];
        if( !inbounds( neighbour ) ) {
            continue;
        }
        const auto neighbour_override = override.find( neighbour );
        const bool neighbour_overridden = neighbour_override != override.end();
        // if there's some non-memory terrain to show at the neighboring tile
        const bool may_connect = neighbour_overridden ||
                                 get_visibility( ch.visibility_cache[neighbour.x()][neighbour.y()],
                                         get_visibility_variables_cache() ) == visibility_type::CLEAR ||
                                 // or if an actual center tile is transparent or next to a memorized tile
                                 ( !overridden && ( is_transparent || is_memorized( neighbour ) ) );
        if( may_connect ) {
            const ter_t &neighbour_terrain = neighbour_overridden ?
                                             neighbour_override->second.obj() : ter( neighbour ).obj();
            if( neighbour_terrain.in_connect_groups( connect_group ) ) {
                val += 1 << i;
            }
        }
    }

    return val;
}

uint8_t map::get_known_rotates_to( const tripoint_bub_ms &p,
                                   const std::bitset<NUM_TERCONN> &rotate_to_group,
                                   const std::map<tripoint_bub_ms, ter_id> &override ) const
{
    if( rotate_to_group.none() ) {
        return CHAR_MAX;
    }

    uint8_t val = 0;

    // populate connection information
    for( int i = 0; i < 4; ++i ) {
        tripoint_bub_ms neighbour = p + offsets[i];
        if( !inbounds( neighbour ) ) {
            continue;
        }
        const auto neighbour_override = override.find( neighbour );
        const bool neighbour_overridden = neighbour_override != override.end();

        const ter_t &neighbour_terrain = neighbour_overridden ?
                                         neighbour_override->second.obj() : ter( neighbour ).obj();
        if( neighbour_terrain.in_connect_groups( rotate_to_group ) ) {
            val += 1 << i;
        }
    }

    return val;
}

uint8_t map::get_known_connections_f( const tripoint_bub_ms &p,
                                      const std::bitset<NUM_TERCONN> &connect_group,
                                      const std::map<tripoint_bub_ms, furn_id> &override ) const
{
    if( connect_group.none() ) {
        return 0;
    }

    const level_cache &ch = access_cache( p.z() );
    uint8_t val = 0;
    std::function<bool( const tripoint_bub_ms & )> is_memorized;
    avatar &player_character = get_avatar();
#ifdef TILES
    if( use_tiles ) {
        is_memorized = [&]( const tripoint_bub_ms & q ) {
            return !player_character.get_memorized_tile( get_abs( q ) ).get_dec_id().empty();
        };
    } else {
#endif
        is_memorized = [&]( const tripoint_bub_ms & q ) {
            return player_character.get_memorized_tile( get_abs( q ) ).symbol != 0;
        };
#ifdef TILES
    }
#endif

    const bool overridden = override.find( p ) != override.end();
    const bool is_transparent = ch.transparency_cache[p.x()][p.y()] > LIGHT_TRANSPARENCY_SOLID;

    // populate connection information
    for( int i = 0; i < 4; ++i ) {
        tripoint_bub_ms pt = p + offsets[i];
        if( !inbounds( pt ) ) {
            continue;
        }
        const auto neighbour_override = override.find( pt );
        const bool neighbour_overridden = neighbour_override != override.end();
        // if there's some non-memory terrain to show at the neighboring tile
        const bool may_connect = neighbour_overridden ||
                                 get_visibility( ch.visibility_cache[pt.x()][pt.y()],
                                         get_visibility_variables_cache() ) ==
                                 visibility_type::CLEAR ||
                                 // or if an actual center tile is transparent or
                                 // next to a memorized tile
                                 ( !overridden && ( is_transparent || is_memorized( pt ) ) );
        if( may_connect ) {
            const furn_t &neighbour_furn = neighbour_overridden ?
                                           neighbour_override->second.obj() : furn( pt ).obj();
            if( neighbour_furn.in_connect_groups( connect_group ) ) {
                val += 1 << i;
            }
        }
    }

    return val;
}

uint8_t map::get_known_rotates_to_f( const tripoint_bub_ms &p,
                                     const std::bitset<NUM_TERCONN> &rotate_to_group,
                                     const std::map<tripoint_bub_ms, ter_id> &override,
                                     const std::map<tripoint_bub_ms, furn_id> &override_f ) const
{
    if( rotate_to_group.none() ) {
        return CHAR_MAX;
    }

    uint8_t val = 0;

    // populate connection information
    for( int i = 0; i < 4; ++i ) {
        tripoint_bub_ms pt = p + offsets[i];
        if( !inbounds( pt ) ) {
            continue;
        }
        const auto neighbour_override = override.find( pt );
        const bool neighbour_overridden = neighbour_override != override.end();

        const auto neighbour_override_f = override_f.find( pt );
        const bool neighbour_overridden_f = neighbour_override_f != override_f.end();

        const ter_t &neighbour = neighbour_overridden ?
                                 neighbour_override->second.obj() : ter( pt ).obj();

        const furn_t &neighbour_f = neighbour_overridden_f ?
                                    neighbour_override_f->second.obj() : furn( pt ).obj();

        if( neighbour.in_connect_groups( rotate_to_group ) ||
            neighbour_f.in_connect_groups( rotate_to_group ) ) {
            val += 1 << i;
        }
    }

    return val;
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

/*
 * Get the terrain transforms_into id (what will the terrain transforms into)
 */
ter_id map::get_ter_transforms_into( const tripoint_bub_ms &p ) const
{
    return ter( p ).obj().transforms_into.id();
}

/**
 * Examines the tile pos, with character as the "examinator"
 * Casts Character to player because player/NPC split isn't done yet
 */
void map::examine( Character &you, const tripoint_bub_ms &pos ) const
{
    const furn_t furn_here = furn( pos ).obj();
    if( furn_here.can_examine( pos ) ) {
        furn_here.examine( dynamic_cast<Character &>( you ), pos );
    } else {
        ter( pos ).obj().examine( dynamic_cast<Character &>( you ), pos );
    }
}

bool map::is_harvestable( const tripoint_bub_ms &pos ) const
{
    const harvest_id &harvest_here = get_harvest( pos );
    return !harvest_here.is_null() && !harvest_here->empty();
}

/*
 * set terrain via string; this works for -any- terrain id
 */
bool map::ter_set( const tripoint_bub_ms &p, const ter_id &new_terrain, bool avoid_creatures )
{
    if( !inbounds( p ) ) {
        return false;
    }
    if( avoid_creatures ) {
        Creature *c = get_creature_tracker().creature_at( get_abs( p ), true );
        if( c ) {
            return false;
        }
    }

    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( tripoint_bub_ms( p ), l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to set terrain at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return true;
    }
    const ter_id &old_id = current_submap->get_ter( l );
    if( old_id == new_terrain ) {
        // Nothing changed
        return false;
    }

    if( !mapgen_in_progress ) {
        // TODO: Consider evaluating whether this is a "meaningful" adjustment instead of just any edit.
        current_submap->player_adjusted_map = true;
    }
    current_submap->set_ter( l, new_terrain );
    current_submap->set_map_damage( point_sm_ms( l ), 0 );
    // Clear any recorded original terrain when terrain is explicitly set here.
    clear_original_terrain_at( p );

    // Set the dirty flags
    const ter_t &old_t = old_id.obj();
    const ter_t &new_t = new_terrain.obj();
    bump_geometry_revision( p.z() );
    if( cast_reached( p ) ) {
        get_cache( p.z() ).seen_cache_dirty = true;
    }

    if( current_submap->is_open_air( l ) ) {
        const furn_id &current_furn = current_submap->get_furn( l );
        if( current_furn != furn_str_id::NULL_ID() &&
            !current_furn->has_flag( ter_furn_flag::TFLAG_ALLOW_ON_OPEN_AIR ) ) {
            debugmsg( "Setting terrain %s at %s where furniture is %s.  Terrain is_open_air\n"
                      "If this is intentional, set the ALLOW_ON_OPEN_AIR flag on the furniture",
                      new_terrain.id().str(), p.to_string(), current_furn.id().str() );
        }
    }

    if( new_t.trap != tr_null ) {
        traplocs[new_t.trap.to_i()].push_back( p );
    }
    if( !new_t.emissions.empty() ) {
        field_ter_locs.push_back( p );
    }
    if( old_t.transparent != new_t.transparent ||
        old_t.has_flag( ter_furn_flag::TFLAG_TRANSLUCENT ) != new_t.has_flag(
            ter_furn_flag::TFLAG_TRANSLUCENT ) ) {
        set_transparency_cache_dirty( p );
    }

    if( old_t.light_emitted != new_t.light_emitted ||
        old_t.light_color != new_t.light_color ) {
        set_lightmap_cache_dirty( p.z() );
    }

    if( old_t.has_flag( ter_furn_flag::TFLAG_INDOORS ) != new_t.has_flag(
            ter_furn_flag::TFLAG_INDOORS ) ) {
        set_outside_cache_dirty( p.z() );
    }

    if( has_floor_gap_flag( new_t ) != has_floor_gap_flag( old_t ) ) {
        set_floor_cache_dirty( p.z() );
        // It's a set, not a flag
        support_cache_dirty.insert( p );
    }

    if( !new_t.liquid_source_item_id.is_null() &&
        new_t.liquid_source_count != std::make_pair( 0, 0 ) ) {
        item water( new_t.liquid_source_item_id, calendar::start_of_cataclysm );
        water.charges = rng( new_t.liquid_source_count.first, new_t.liquid_source_count.second );
        add_item( p, water );
    }

    invalidate_max_populated_zlev( p.z() );

    memory_cache_dec_set_dirty( p, true );
    memory_cache_ter_set_dirty( p, true );
    avatar &player_character = get_avatar();
    if( player_character.sees( *this, p ) ) {
        player_character.memorize_clear_decoration( get_abs( p ), "t_" );
    }

    if( ( old_t.movecost == 0 ) != ( new_t.movecost == 0 ) ) {
        get_creature_tracker().invalidate_reachability_cache();
    }
    // TODO: Limit to changes that affect move cost, traps and stairs
    set_pathfinding_cache_dirty( tripoint_bub_ms( p ) );

    tripoint_bub_ms above( p.xy(), p.z() + 1 );
    // Make sure that if we supported something and no longer do so, it falls down
    support_dirty( above );

    return true;
}

void map::kill_creature( const tripoint_bub_ms &p, bool remove_corpse )
{
    Creature *tmp_critter = get_creature_tracker().creature_at( get_abs( p ), true );
    if( tmp_critter && !tmp_critter->is_avatar() ) {
        if( remove_corpse ) {
            if( monster *mon = tmp_critter->as_monster(); mon != nullptr ) {
                mon->death_drops = false;
                mon->no_corpse_quiet = true;

            } else if( npc *npc = tmp_critter->as_npc(); npc != nullptr ) {
                npc->death_drops = false;
                npc->quiet_death = true;
            }
        }
        tmp_critter->die( this, nullptr );
    }
}

std::string map::tername( const tripoint_bub_ms &p ) const
{
    return ter( p ).obj().name();
}

std::string map::features( const tripoint_bub_ms &p ) const
{
    std::string result;
    const auto add = [&]( const std::string & text ) {
        if( !result.empty() ) {
            result += " ";
        }
        result += text;
    };
    const auto add_if = [&]( const bool cond, const std::string & text ) {
        if( cond ) {
            add( text );
        }
    };
    // This is used in an info window that is 46 characters wide, and is expected
    // to take up one line.  So, make sure it does that.
    // FIXME: can't control length of localized text.
    add_if( is_bashable( p ), _( "Smashable." ) );
    add_if( has_flag( ter_furn_flag::TFLAG_DIGGABLE, p ), json_flag_DIGGABLE->name() );
    add_if( has_flag( ter_furn_flag::TFLAG_PLOWABLE, p ), json_flag_PLOWABLE->name() );
    add_if( has_flag( ter_furn_flag::TFLAG_PHASE_BACK, p ), json_flag_PHASE_BACK->name() );
    add_if( has_flag( ter_furn_flag::TFLAG_ROUGH, p ), _( "Rough." ) );
    add_if( has_flag( ter_furn_flag::TFLAG_UNSTABLE, p ), _( "Unstable." ) );
    add_if( has_flag( ter_furn_flag::TFLAG_SHARP, p ), _( "Sharp." ) );
    add_if( has_flag( ter_furn_flag::TFLAG_FLAT, p ), json_flag_FLAT->name() );
    add_if( has_flag( ter_furn_flag::TFLAG_EASY_DECONSTRUCT, p ),
            json_flag_EASY_DECONSTRUCT->name() );
    add_if( has_flag( ter_furn_flag::TFLAG_MOUNTABLE, p ), _( "Mountable." ) );
    add_if( has_flag( ter_furn_flag::TFLAG_FLAMMABLE, p ) ||
            has_flag( ter_furn_flag::TFLAG_FLAMMABLE_ASH, p ) ||
            has_flag( ter_furn_flag::TFLAG_FLAMMABLE_HARD, p ), _( "Flammable." ) );
    return result;
}

int map::move_cost_internal( const furn_t &furniture, const ter_t &terrain, const field &field,
                             const vehicle *veh,
                             const int vpart ) const
{
    if( terrain.movecost == 0 || ( furniture.id && furniture.movecost < 0 ) ||
        field.total_move_cost() < 0 ) {
        return 0;
    }

    if( veh != nullptr ) {
        const vpart_position vp( const_cast<vehicle &>( *veh ), vpart );
        return vp.get_movecost();
    }
    int movecost = std::max( terrain.movecost + field.total_move_cost(), 0 );

    if( furniture.id ) {
        if( furniture.has_flag( "BRIDGE" ) ) {
            movecost = 2 + std::max( furniture.movecost, 0 );
        } else {
            movecost += std::max( furniture.movecost, 0 );
        }
    }

    return movecost;
}

bool map::is_wall_adjacent( const tripoint_bub_ms &center ) const
{
    for( const tripoint_bub_ms &p : points_in_radius( center, 1 ) ) {
        if( p != center && impassable( p ) ) {
            return true;
        }
    }
    return false;
}

bool map::is_open_air( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }
    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        return false;
    }
    return current_submap->is_open_air( l );
}

// Move cost: 3D

int map::move_cost( const tripoint_bub_ms &p, const vehicle *ignored_vehicle,
                    const bool ignore_fields ) const
{
    // To save all of the bound checks and submaps fetching, we extract it
    // here instead of using furn(), field_at() and ter().
    if( !inbounds( p ) ) {
        return 0;
    }
    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        return 0;
    }

    field static nofield;

    const furn_t &furniture = current_submap->get_furn( l ).obj();
    const ter_t &terrain = current_submap->get_ter( l ).obj();
    const field &field = current_submap->get_field( l );
    const optional_vpart_position vp = veh_at( p );
    vehicle *const veh = ( !vp || &vp->vehicle() == ignored_vehicle ) ? nullptr : &vp->vehicle();
    const int part = veh ? vp->part_index() : -1;

    return move_cost_internal( furniture, terrain,
                               ( !ignore_fields ? field : nofield ), veh, part );
}

bool map::impassable( const tripoint_bub_ms &p ) const
{
    return !passable( p );
}

bool map::passable( const tripoint_bub_ms &p ) const
{
    return move_cost( p ) != 0;
}

bool map::passable_through( const tripoint_bub_ms &p ) const
{
    return passable( p ) && ( has_floor( p ) || has_floor_or_support( p ) );
}

bool map::passable_skip_fields( const tripoint_bub_ms &p ) const
{
    return move_cost( p, static_cast<const vehicle *>( nullptr ), true ) != 0;
}

int map::move_cost_ter_furn( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return 0;
    }

    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried process terrain at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return 0;
    }

    const int tercost = current_submap->get_ter( l ).obj().movecost;
    if( tercost == 0 ) {
        return 0;
    }

    const int furncost = current_submap->get_furn( l ).obj().movecost;
    if( furncost < 0 ) {
        return 0;
    }

    const int cost = tercost + furncost;
    return cost > 0 ? cost : 0;
}

bool map::impassable_ter_furn( const tripoint_bub_ms &p ) const
{
    return !passable_ter_furn( p );
}

bool map::passable_ter_furn( const tripoint_bub_ms &p ) const
{
    return move_cost_ter_furn( p ) != 0;
}

int map::combined_movecost( const tripoint_bub_ms &from, const tripoint_bub_ms &to,
                            const vehicle *ignored_vehicle,
                            const int modifier, const bool flying, const bool via_ramp, const bool ignore_fields ) const
{
    static constexpr std::array<int, 4> mults = { 0, 50, 71, 100 };
    const int cost1 = move_cost( from, ignored_vehicle, ignore_fields );
    const int cost2 = move_cost( to, ignored_vehicle, ignore_fields );
    // Multiply cost depending on the number of differing axes
    // 0 if all axes are equal, 100% if only 1 differs, 141% for 2, 200% for 3
    size_t match = trigdist ? ( from.x() != to.x() ) + ( from.y() != to.y() ) +
                   ( from.z() != to.z() ) : 1;
    if( flying || from.z() == to.z() ) {
        return ( cost1 + cost2 + modifier ) * mults[match] / 2;
    }

    // Inter-z-level movement by foot (not flying)
    if( !valid_move( from, to, false, via_ramp ) ) {
        return 0;
    }

    // TODO: Penalize for using stairs
    return ( cost1 + cost2 + modifier ) * mults[match] / 2;
}

bool map::valid_move( const tripoint_bub_ms &from, const tripoint_bub_ms &to,
                      const bool bash, const bool flying, const bool via_ramp ) const
{
    // Used to account for the fact that older versions of GCC can trip on the if statement here.
    cata_assert( to.z() > std::numeric_limits<int>::min() );
    // Note: no need to check inbounds here, because maptile_at will do that
    // If oob tile is supplied, the maptile_at will be an unpassable "null" tile
    if( std::abs( from.x() - to.x() ) > 1 || std::abs( from.y() - to.y() ) > 1 ||
        std::abs( from.z() - to.z() ) > 1 ) {
        return false;
    }

    if( from.z() == to.z() ) {
        // But here we need to, to prevent bashing critters
        return passable( to ) || ( bash && inbounds( to ) );
    } else if( !zlevels ) {
        return false;
    }

    const bool going_up = from.z() < to.z();

    const tripoint_bub_ms &up_p = going_up ? to : from;
    const tripoint_bub_ms &down_p = going_up ? from : to;

    const const_maptile up = maptile_at( up_p );
    const ter_t &up_ter = up.get_ter_t();
    if( up_ter.id.is_null() ) {
        return false;
    }
    // Checking for ledge is a workaround for the case when mapgen doesn't
    // actually make a valid ledge drop location with zlevels on, this forces
    // at least one zlevel drop and if down_ter is impassable it's probably
    // inside a wall, we could workaround that further but it's unnecessary.
    const bool up_is_ledge = is_open_air( up_p );

    if( up_ter.movecost == 0 ) {
        // Unpassable tile
        return false;
    }

    const const_maptile down = maptile_at( down_p );
    const ter_t &down_ter = down.get_ter_t();
    if( down_ter.id.is_null() ) {
        return false;
    }

    if( !up_is_ledge && down_ter.movecost == 0 ) {
        // Unpassable tile
        return false;
    }

    if( has_floor( up_p ) && !up_ter.has_flag( ter_furn_flag::TFLAG_GOES_DOWN ) &&
        !up_is_ledge && !via_ramp ) {
        // Can't move from up to down
        if( std::abs( from.x() - to.x() ) == 1 || std::abs( from.y() - to.y() ) == 1 ) {
            // Break the move into two - vertical then horizontal
            tripoint_bub_ms midpoint( down_p.xy(), up_p.z() );
            return valid_move( down_p, midpoint, bash, flying, via_ramp ) &&
                   valid_move( midpoint, up_p, bash, flying, via_ramp );
        }
        return false;
    }

    if( !flying && !down_ter.has_flag( ter_furn_flag::TFLAG_GOES_UP ) &&
        !down_ter.has_flag( ter_furn_flag::TFLAG_SWIMMABLE ) &&
        !down_ter.has_flag( ter_furn_flag::TFLAG_RAMP ) &&
        !up_is_ledge && !via_ramp ) {
        // Can't safely reach the lower tile
        return false;
    }

    if( bash ) {
        return true;
    }

    int part_up;
    const vehicle *veh_up = veh_at_internal( up_p, part_up );
    if( veh_up != nullptr ) {
        // TODO: Hatches below the vehicle, passable frames
        return false;
    }

    int part_down;
    const vehicle *veh_down = veh_at_internal( down_p, part_down );
    if( veh_down != nullptr && veh_down->roof_at_part( part_down ) >= 0 ) {
        // TODO: OPEN (and only open) hatches from above
        return false;
    }

    // Currently only furniture can block movement if everything else is OK
    // TODO: Vehicles with boards in the given spot
    return up.get_furn_t().movecost >= 0;
}

// End of move cost

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

bool map::has_flag( const std::string &flag, const tripoint_bub_ms &p ) const
{
    return has_flag_ter_or_furn( flag, p ); // Does bound checking
}

bool map::can_put_items( const tripoint_bub_ms &p ) const
{
    return can_put_items_ter_furn( p ) || veh_at( p ).cargo().has_value();
}

bool map::can_put_items_ter_furn( const tripoint_bub_ms &p ) const
{
    return !has_flag( ter_furn_flag::TFLAG_NOITEM, p ) && !has_flag( ter_furn_flag::TFLAG_SEALED, p );
}

bool map::has_flag_ter( const std::string &flag, const tripoint_bub_ms &p ) const
{
    return ter( p ).obj().has_flag( flag );
}

bool map::has_flag_furn( const std::string &flag, const tripoint_bub_ms &p ) const
{
    return furn( p ).obj().has_flag( flag );
}

bool map::has_flag_ter_or_furn( const std::string &flag, const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }

    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to process terrain at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return false;
    }

    return current_submap->get_ter( l ).obj().has_flag( flag ) ||
           current_submap->get_furn( l ).obj().has_flag( flag );
}

bool map::has_flag( const ter_furn_flag flag, const tripoint_bub_ms &p ) const
{
    return has_flag_ter_or_furn( flag, p ); // Does bound checking
}

bool map::has_flag_ter( const ter_furn_flag flag, const tripoint_bub_ms &p ) const
{
    return ter( p ).obj().has_flag( flag );
}

bool map::has_flag_furn( const ter_furn_flag flag, const tripoint_bub_ms &p ) const
{
    return furn( p ).obj().has_flag( flag );
}

bool map::has_flag_ter_or_furn( const ter_furn_flag flag, const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }

    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to process terrain at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return false;
    }

    return current_submap->get_ter( l ).obj().has_flag( flag ) ||
           current_submap->get_furn( l ).obj().has_flag( flag );
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

// End of 3D flags

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

bool map::has_nearby_ter( const tripoint_bub_ms &p, const ter_id &type, int radius ) const
{
    for( const tripoint_bub_ms &pt : points_in_radius( p, radius ) ) {
        if( ter( pt ) == type ) {
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

void map::translate( const ter_id &from, const ter_id &to )
{
    if( from == to ) {
        debugmsg( "map::translate %s => %s",
                  from.obj().name(),
                  from.obj().name() );
        return;
    }
    for( const tripoint_bub_ms &p : points_on_zlevel() ) {
        if( ter( p ) == from ) {
            ter_set( p, to );
        }
    }
}

//This function performs the translate function within a given radius of the player.
void map::translate_radius( const ter_id &from, const ter_id &to, float radi,
                            const tripoint_bub_ms &p,
                            const bool same_submap, const bool toggle_between )
{
    if( from == to ) {
        debugmsg( "map::translate %s => %s", from.obj().name(), to.obj().name() );
        return;
    }

    const tripoint_abs_omt abs_omt_p = coords::project_to<coords::omt>( get_abs( p ) );
    for( const tripoint_bub_ms &t : points_on_zlevel() ) {
        const tripoint_abs_omt abs_omt_t = coords::project_to<coords::omt>( get_abs( t ) );
        const float radiX = trig_dist( p, t );
        if( ter( t ) == from ) {
            // within distance, and either no submap limitation or same overmap coords.
            if( radiX <= radi && ( !same_submap || abs_omt_t == abs_omt_p ) ) {
                ter_set( t, to );
            }
        } else if( toggle_between && ter( t ) == to ) {
            if( radiX <= radi && ( !same_submap || abs_omt_t == abs_omt_p ) ) {
                ter_set( t, from );
            }
        }
    }
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void map::transform_radius( const ter_furn_transform_id &transform, int radi,
                            const tripoint_abs_ms &p )
{
    if( !inbounds( p - point( radi, radi ) ) || !inbounds( p + point( radi, radi ) ) ) {
        debugmsg( "transform_radius called for area out of bounds" );
    }
    tripoint_bub_ms const loc = get_bub( p );
    for( tripoint_bub_ms const &t : points_in_radius( loc, radi, 0 ) ) {
        if( trig_dist( loc, t ) <= radi ) {
            transform->transform( *this, t );
        }
    }
}

void map::transform_line( const ter_furn_transform_id &transform, const tripoint_abs_ms &first,
                          const tripoint_abs_ms &second )
{
    if( !inbounds( first ) || !inbounds( second ) ) {
        debugmsg( "transform_line called for line out of bounds" );
    }
    for( const tripoint_abs_ms &t : line_to( first, second ) ) {
        transform->transform( *this, get_bub( t ) );
    }
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

std::unordered_set<item_location> map::all_items( Character &who, accessor_flags flags )
{
    // Includes dummy filter that all items pass, only accessor_flags determines what gets returned.
    return all_items( return_true<item>, who, flags );
}

std::unordered_set<item_location> map::all_items( const std::function<bool( const item & )> &filter,
        Character &who, accessor_flags flags )
{
    std::unordered_set<item_location> ret;

    auto recursive_add_contained_items = [&]( const std::function<bool( const item & )> &filter,
    item_location & it ) {
        it.visit_items( [&]( const item_location & content_item ) {
            if( !content_item.has_parent() ) {
                // This is itself the top-level item.
                // E.g. Calling visit_items() on a backpack > 2 soaps would first visit the backpack, which has no parent (it is not contained in itself)
                // So just skip to the actual contents, rather than trying to say the backpack is in itself.
                return VisitResponse::NEXT;
            }
            if( filter( *content_item ) ) {
                ret.emplace( content_item );
            }
            return VisitResponse::NEXT;
        } );
    };


    if( flags & Access_Inventory )  {
        who.visit_items( [&filter, &ret]( const item_location & it ) {
            if( filter( *it ) ) {
                ret.emplace( it );
            }
            return VisitResponse::NEXT;
        } );
    }


    if( flags & Access_Map_All )  {
        for( int i = -OVERMAP_DEPTH; i <= OVERMAP_HEIGHT; i++ ) {
            for( const tripoint_bub_ms &pt : points_on_zlevel( who.posz() ) ) {
                for( item &it : i_at( pt ) ) {
                    // We always want to recurse even if the top-level item doesn't pass our filter - the contained items still might!
                    item_location there( map_cursor( this, pt ), &it );
                    recursive_add_contained_items( filter, there );
                    if( filter( it ) ) {
                        ret.emplace( there );
                    }
                }
            }
        }
    } else if( flags & Access_Map_Current_Z )  {
        for( const tripoint_bub_ms &pt : points_on_zlevel( who.posz() ) ) {
            for( item &it : i_at( pt ) ) {
                // We always want to recurse even if the top-level item doesn't pass our filter - the contained items still might!
                item_location there( map_cursor( this, pt ), &it );
                recursive_add_contained_items( filter, there );
                if( filter( it ) ) {
                    ret.emplace( there );
                }
            }
        }
    } else if( flags & Access_Map_Around )  {
        for( const tripoint_bub_ms &pt : reachable_flood_steps( who.pos_bub(), PICKUP_RANGE ) ) {
            for( item &it : i_at( pt ) ) {
                // We always want to recurse even if the top-level item doesn't pass our filter - the contained items still might!
                item_location there( map_cursor( this, pt ), &it );
                recursive_add_contained_items( filter, there );
                if( filter( it ) ) {
                    ret.emplace( there );
                }
            }
        }
    }

    if( flags & Access_Vehicle )  {
        for( wrapped_vehicle &v : get_vehicles() ) {
            vehicle *veh = v.v;
            if( veh ) {
                for( vpart_reference vp : veh->get_all_parts() ) {
                    for( item &it : veh->get_items( vp.part() ) ) {
                        // We always want to recurse even if the top-level item doesn't pass our filter - the contained items still might!
                        item_location there( vehicle_cursor( *veh, vp.part_index() ), &it );
                        recursive_add_contained_items( filter, there );
                        if( filter( it ) ) {
                            ret.emplace( there );
                        }
                    }
                }
            }
        }
    }

    return ret;
}

// Items: 3D

map_stack map::i_at( const tripoint_bub_ms &p )
{
    if( !inbounds( p ) ) {
        nulitems.clear();
        return map_stack{ &nulitems, p, this};
    }

    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to get items at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        nulitems.clear();
        return map_stack{ &nulitems, p, this};
    }

    return map_stack{ &current_submap->get_items( l ), p, this};
}

map_stack::iterator map::i_rem( const tripoint_bub_ms &p, const map_stack::const_iterator &it )
{
    point_sm_ms l;
    submap *const current_submap = get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to remove items at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        nulitems.clear();
        return map_stack{ &nulitems, p, this} .begin();
    }

    current_submap->update_lum_rem( l, *it );
    if( it->is_emissive() ) {
        set_lightmap_cache_dirty( p.z() );
    }

    return current_submap->get_items( l ).erase( it );
}

void map::i_rem( const tripoint_bub_ms &p, item *it )
{
    map_stack map_items = i_at( p );
    map_stack::const_iterator iter = map_items.get_iterator_from_pointer( it );
    if( iter != map_items.end() ) {
        i_rem( p, iter );
    }
}

void map::i_clear( const tripoint_bub_ms &p )
{
    point_sm_ms l;
    submap *const current_submap = get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to clear items at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return;
    }

    if( current_submap->get_lum( l ) > 0 ) {
        set_lightmap_cache_dirty( p.z() );
    }
    current_submap->set_lum( l, 0 );
    current_submap->get_items( l ).reset();
}

std::vector<item *> map::spawn_items( const tripoint_bub_ms &p, const std::vector<item> &new_items )
{
    std::vector<item *> ret;
    if( !inbounds( p ) || has_flag( ter_furn_flag::TFLAG_DESTROY_ITEM, p ) ) {
        return ret;
    }
    const bool swimmable = has_flag( ter_furn_flag::TFLAG_SWIMMABLE, p );
    for( const item &new_item : new_items ) {

        if( new_item.made_of( phase_id::LIQUID ) && swimmable ) {
            continue;
        }
        item &it = add_item_or_charges( p, new_item );
        if( !it.is_null() ) {
            ret.push_back( &it );
        }
    }

    return ret;
}

void map::spawn_artifact( const tripoint_bub_ms &p, const relic_procgen_id &id,
                          const int max_attributes,
                          const int power_level, const int max_negative_power, const bool is_resonant )
{
    relic_procgen_data::generation_rules rules;
    rules.max_attributes = max_attributes;
    rules.power_level = power_level;
    rules.max_negative_power = max_negative_power;
    rules.resonant = is_resonant;

    add_item_or_charges( p, id->create_item( rules ) );
}

void map::spawn_item( const tripoint_bub_ms &p, const itype_id &type_id, const unsigned quantity,
                      const int charges, const time_point &birthday, const int damlevel, const std::set<flag_id> &flags,
                      const std::string &variant, const std::string &faction )
{
    if( type_id.is_null() ) {
        return;
    }

    if( item_is_blacklisted( type_id ) ) {
        return;
    }
    // recurse to spawn (quantity - 1) items
    for( size_t i = 1; i < quantity; i++ ) {
        spawn_item( p, type_id, 1, charges, birthday, damlevel, flags, variant, faction );
    }
    // migrate and spawn the item
    itype_id mig_type_id = item_controller->migrate_id( type_id );
    item new_item( mig_type_id, birthday );
    new_item.set_itype_variant( variant );
    new_item.set_owner( faction_id( faction ) );
    if( one_in( 3 ) && new_item.has_flag( flag_VARSIZE ) ) {
        new_item.set_flag( flag_FIT );
    }

    if( charges && new_item.charges > 0 ) {
        //let's fail silently if we specify charges for an item that doesn't support it
        new_item.charges = charges;
    }
    new_item = new_item.in_its_container();
    new_item.set_owner( faction_id( faction ) ); // Set faction to the container as well
    if( ( new_item.made_of( phase_id::LIQUID ) && has_flag( ter_furn_flag::TFLAG_SWIMMABLE, p ) ) ||
        has_flag( ter_furn_flag::TFLAG_DESTROY_ITEM, p ) ) {
        return;
    }

    new_item.set_damage( damlevel );
    new_item.rand_degradation();
    for( const flag_id &flag : flags ) {
        new_item.set_flag( flag );
    }

    add_item_or_charges( p, new_item );
}

units::volume map::max_volume( const tripoint_bub_ms &p )
{
    return i_at( p ).max_volume();
}

// total volume of all the things
units::volume map::stored_volume( const tripoint_bub_ms &p )
{
    return i_at( p ).stored_volume();
}

// free space
units::volume map::free_volume( const tripoint_bub_ms &p )
{
    return i_at( p ).free_volume();
}

item_location map::add_item_or_charges_ret_loc( const tripoint_bub_ms &pos, item obj,
        bool overflow )
{
    int copies = 1;
    std::pair<item *, tripoint_bub_ms> ret = _add_item_or_charges( pos, std::move( obj ), copies,
            overflow );
    if( ret.first != nullptr && !ret.first->is_null() ) {
        return item_location{ map_cursor{ get_abs( ret.second ) }, ret.first };
    }

    return {};
}

item &map::add_item_or_charges( const tripoint_bub_ms &pos, item obj, bool overflow )
{
    int copies = 1;
    return *_add_item_or_charges( pos, std::move( obj ), copies, overflow ).first;
}

// clang-tidy is confused and thinks obj can be made into a const reference, but it can't
// on_drop is not a const function
// NOLINTNEXTLINE(performance-unnecessary-value-param)
std::pair<item *, tripoint_bub_ms> map::_add_item_or_charges( const tripoint_bub_ms &pos, item obj,
        int &copies_remaining, bool overflow )
{
    // Checks if item would not be destroyed if added to this tile
    auto valid_tile = [&]( const tripoint_bub_ms & e ) {
        if( !inbounds( e ) ) {
            dbg( D_INFO ) << e; // should never happen
            return false;
        }

        // Some tiles destroy items (e.g. lava)
        if( has_flag( ter_furn_flag::TFLAG_DESTROY_ITEM, e ) ) {
            return false;
        }

        // Cannot drop liquids into tiles that are comprised of liquid
        if( obj.made_of_from_type( phase_id::LIQUID ) && has_flag( ter_furn_flag::TFLAG_SWIMMABLE, e ) ) {
            return false;
        }

        return true;
    };

    // Get how many copies of the item can fit in a tile
    auto how_many_copies_fit = [&]( const tripoint_bub_ms & e ) {
        return std::min( { copies_remaining,
                           obj.volume() == 0_ml ? INT_MAX : free_volume( e ) / obj.volume(),
                           static_cast<int>( MAX_ITEM_IN_SQUARE - i_at( e ).size() ) } );
    };

    // Performs the actual insertion of the object onto the map
    auto place_item = [&]( const tripoint_bub_ms & tile, int &copies ) -> item& {
        if( obj.count_by_charges() )
        {
            for( item &e : i_at( tile ) ) {
                if( e.merge_charges( obj ) ) {
                    return e;
                }
            }
        }

        support_dirty( tile );
        return add_item( tile, obj, copies );
    };

    if( item_is_blacklisted( obj.typeId() ) ) {
        return { &null_item_reference(), tripoint_bub_ms::invalid };
    }

    // Some items never exist on map as a discrete item (must be contained by another item)
    if( obj.has_flag( flag_NO_DROP ) ) {
        return { &null_item_reference(), tripoint_bub_ms::invalid };
    }

    // If intended drop tile destroys the item then we don't attempt to overflow
    if( !valid_tile( pos ) ) {
        return { &null_item_reference(), tripoint_bub_ms::invalid };
    }

    std::optional<std::pair<item *, tripoint_bub_ms>> first_added;
    int copies_to_add_here = how_many_copies_fit( pos );

    if( ( !has_flag( ter_furn_flag::TFLAG_NOITEM, pos ) ||
          ( has_flag( ter_furn_flag::TFLAG_LIQUIDCONT, pos ) && obj.made_of( phase_id::LIQUID ) ) ) &&
        copies_to_add_here > 0 ) {
        // Pass map into on_drop, because this map may not be the global map object (in mapgen, for instance).
        if( obj.made_of( phase_id::LIQUID ) || !obj.has_flag( flag_DROP_ACTION_ONLY_IF_LIQUID ) ) {
            if( obj.on_drop( pos, *this ) ) {
                return { &null_item_reference(), tripoint_bub_ms::invalid };
            }
        }
        // If tile can contain items place here...
        copies_remaining -= copies_to_add_here;
        first_added = first_added ? first_added : std::make_pair( &place_item( pos, copies_to_add_here ),
                      pos );
    }
    if( overflow && copies_remaining > 0 ) {
        // ...otherwise try to overflow to adjacent tiles (if permitted)
        const int max_dist = 2;
        std::vector<tripoint_bub_ms> tiles = closest_points_first( pos, 1, max_dist );
        const int max_path_length = 4 * max_dist;
        const pathfinding_settings setting( {}, max_dist, max_path_length, 0, false, false, true, false,
                                            false, false );
        for( const tripoint_bub_ms &e : tiles ) {
            if( copies_remaining <= 0 ) {
                break;
            }
            if( !inbounds( e ) ) {
                continue;
            }
            //must be a path to the target tile
            if( route( pos, pathfinding_target::point( e ), setting ).empty() ) {
                continue;
            }
            if( obj.made_of( phase_id::LIQUID ) || !obj.has_flag( flag_DROP_ACTION_ONLY_IF_LIQUID ) ) {
                if( obj.on_drop( e, *this ) ) {
                    return first_added ? first_added.value() : std::make_pair( &null_item_reference(),
                            tripoint_bub_ms::invalid );
                }
            }

            copies_to_add_here = how_many_copies_fit( e );
            if( !valid_tile( e ) || copies_to_add_here <= 0 ||
                has_flag( ter_furn_flag::TFLAG_NOITEM, e ) || has_flag( ter_furn_flag::TFLAG_SEALED, e ) ) {
                continue;
            }
            copies_remaining -= copies_to_add_here;
            std::pair<item *, tripoint_bub_ms> new_item = { &place_item( e, copies_to_add_here ), e };
            first_added = first_added ? first_added : new_item;
        }
    }

    // If first_added has no value, no items were added due to lack of space at target tile (+/- overflow tiles)
    return first_added ? first_added.value() : std::make_pair( &null_item_reference(),
            tripoint_bub_ms::invalid );
}

item &map::add_item_or_charges( const tripoint_bub_ms &pos, item obj, int &copies_remaining,
                                bool overflow )
{
    return *_add_item_or_charges( pos, std::move( obj ), copies_remaining, overflow ).first;
}

float map::item_category_spawn_rate( const item &itm )
{
    const item_category_id &cat = itm.get_category_of_contents().id;
    const float spawn_rate = cat.obj().get_spawn_rate();

    return spawn_rate > 1.0f ? roll_remainder( spawn_rate ) : spawn_rate;
}

item &map::add_item( const tripoint_bub_ms &p, item new_item )
{
    int copies = 1;
    return add_item( p, std::move( new_item ), copies );
}

item &map::add_item( const tripoint_bub_ms &p, item new_item, int copies )
{
    if( item_is_blacklisted( new_item.typeId() ) ) {
        return null_item_reference();
    }

    if( copies <= 0 ) {
        return null_item_reference();
    }

    if( !inbounds( p ) ) {
        return null_item_reference();
    }

    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to add items at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return null_item_reference();
    }

    // Process foods and temperature tracked items when they are added to the map, here instead of add_item_at()
    // to avoid double processing food during active item processing.
    if( new_item.has_temperature() && !new_item.is_corpse() ) {
        new_item.process( *this, nullptr, p );
    }

    if( new_item.made_of( phase_id::LIQUID ) && has_flag( ter_furn_flag::TFLAG_SWIMMABLE, p ) ) {
        return null_item_reference();
    }

    if( has_flag( ter_furn_flag::TFLAG_DESTROY_ITEM, p ) ) {
        return null_item_reference();
    }

    if( new_item.has_flag( flag_ACT_IN_FIRE ) && get_field( p, fd_fire ) != nullptr ) {
        if( new_item.has_flag( flag_BOMB ) && new_item.is_transformable() ) {
            //Convert a bomb item into its transformable version, e.g. incendiary grenade -> active incendiary grenade
            new_item.convert( dynamic_cast<const iuse_transform *>
                              ( new_item.type->get_use( "transform" )->get_actor_ptr() )->transform.target );
        }
        new_item.active = true;
    }

    new_item.preserve_location( get_abs( p ) );

    if( new_item.has_flag( flag_ACTIVATE_ON_PLACE ) ) {
        new_item.activate();
    }

    current_submap->ensure_nonuniform();
    invalidate_max_populated_zlev( p.z() );

    current_submap->update_lum_add( l, new_item );
    if( new_item.is_emissive() ) {
        set_lightmap_cache_dirty( p.z() );
    }

    const map_stack::iterator new_pos = current_submap->get_items( l ).insert( new_item );
    while( --copies > 0 ) {
        current_submap->get_items( l ).insert( new_item );
    }

    if( current_submap->active_items.add( *new_pos, l ) ) {
        // TODO: fix point types
        tripoint_abs_sm const loc( abs_sub.x() + p.x() / SEEX, abs_sub.y() + p.y() / SEEY, p.z() );
        submaps_with_active_items_dirty.insert( loc );

        map &bubble_map = reality_bubble();

        if( this != &bubble_map && bubble_map.inbounds( loc ) ) {
            bubble_map.make_active( loc );
        }
    }

    return *new_pos;
}

// NOLINTNEXTLINE(readability-make-member-function-const)
item map::liquid_from( const tripoint_bub_ms &p ) const
{
    weather_manager &weather = get_weather();
    ter_t source_terrain = ter( p ).obj();

    if( !source_terrain.liquid_source_item_id.is_null() &&
        source_terrain.liquid_source_count == std::make_pair( 0, 0 ) ) {
        item ret( source_terrain.liquid_source_item_id, calendar::turn, item::INFINITE_CHARGES );
        ret.set_item_temperature( std::max( weather.get_temperature( p ),
                                            units::from_celsius( source_terrain.liquid_source_min_temp ) ) );
        return ret;
    }
    return item();
}

void map::make_active( item_location &loc )
{
    item *target = loc.get_item();

    point_sm_ms l;
    submap *const current_submap = get_submap_at( loc.pos_bub( *this ), l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to make active at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return;
    }
    cata::colony<item> &item_stack = current_submap->get_items( l );
    cata::colony<item>::iterator iter = item_stack.get_iterator( target );

    if( current_submap->active_items.add( *iter, l ) ) {
        tripoint_abs_sm const smloc( abs_sub.x() + loc.pos_bub( *this ).x() / SEEX,
                                     abs_sub.y() + loc.pos_bub( *this ).y() / SEEY, loc.pos_abs().z() );
        submaps_with_active_items_dirty.insert( smloc );

        map &bubble_map = reality_bubble();

        if( this != &bubble_map && bubble_map.inbounds( smloc ) ) {
            bubble_map.make_active( smloc );
        }
    }
}

void map::make_active( tripoint_abs_sm const &loc )
{
    submaps_with_active_items_dirty.insert( loc );
}

void map::update_lum( item_location &loc, bool add )
{
    item *target = loc.get_item();

    // if the item is not emissive, do nothing
    if( !target->is_emissive() ) {
        return;
    }

    point_sm_ms l;
    submap *const current_submap = get_submap_at( loc.pos_bub( *this ), l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to update lum at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return;
    }

    if( add ) {
        current_submap->update_lum_add( l, *target );
    } else {
        current_submap->update_lum_rem( l, *target );
    }
    set_lightmap_cache_dirty( loc.pos_bub( *this ).z() );
}

template <typename veh_or_map_cursor>
static bool process_map_items( map &here, item_stack &items, safe_reference<item> &item_ref,
                               item *parent, const tripoint_bub_ms &location, const veh_or_map_cursor &cur, float insulation,
                               temperature_flag flag, float spoil_multiplier, bool watertight_container )
{
    if( item_ref->process( here, nullptr, location, insulation, flag, spoil_multiplier,
                           watertight_container, false ) ) {
        // Item is to be destroyed so erase it from the map stack
        // unless it was already destroyed by processing.
        if( item_ref ) {
            item_ref->spill_contents( location );
            if( parent != nullptr ) {
                item_location( cur, parent ).remove_item( *item_ref );
            } else {
                items.erase( items.get_iterator_from_pointer( item_ref.get() ) );
            }
        }
        if( parent != nullptr ) {
            parent->on_contents_changed();
        }
        return true;
    }
    // Item not destroyed
    return false;
}

static void process_vehicle_items( vehicle &cur_veh, int part )
{
    map &here = get_map();

    vehicle_part &vp = cur_veh.part( part );
    const vpart_info &vpi = vp.info();

    const bool washer_here = vp.enabled &&
                             ( vpi.has_flag( VPFLAG_WASHING_MACHINE ) ||
                               vpi.has_flag( VPFLAG_DISHWASHER ) );

    if( washer_here ) {
        bool washing_machine_finished = false;
        for( item &n : cur_veh.get_items( vp ) ) {
            const time_duration washing_time = 90_minutes;
            const time_duration time_left = washing_time - n.age();
            if( time_left <= 0_turns ) {
                n.unset_flag( flag_FILTHY );
                washing_machine_finished = true;
                vp.enabled = false;
            } else if( calendar::once_every( 15_minutes ) ) {
                //~ %1$d: Number of minutes remaining, %2$s: Name of the vehicle
                add_msg( _( "It should take %1$d minutes to finish washing items in the %2$s." ),
                         to_minutes<int>( time_left ) + 1, cur_veh.name );
                break;
            }
        }
        if( washing_machine_finished && !vpi.has_flag( VPFLAG_APPLIANCE ) ) {
            //~ %1$s: Cleaner, %2$s: Name of the vehicle
            add_msg( _( "The %1$s in the %2$s has finished washing." ), vp.name( false ), cur_veh.name );
        } else if( washing_machine_finished ) {
            add_msg( _( "The %1$s has finished washing." ), vp.name( false ) );
        }
    }

    const bool autoclave_here = vpi.has_flag( VPFLAG_AUTOCLAVE ) && vp.enabled;
    if( autoclave_here ) {
        bool autoclave_finished = false;
        for( item &n : cur_veh.get_items( vp ) ) {
            const time_duration cycle_time = 90_minutes;
            const time_duration time_left = cycle_time - n.age();
            if( time_left <= 0_turns ) {
                if( !n.has_flag( flag_NO_PACKED ) ) {
                    n.unset_flag( flag_NO_STERILE );
                }
                autoclave_finished = true;
                vp.enabled = false;
            } else if( calendar::once_every( 15_minutes ) ) {
                const int minutes = to_minutes<int>( time_left ) + 1;
                //~ %1$d: Number of minutes remaining, %2$s: Name of the vehicle
                add_msg( n_gettext( "It should take %1$d minute to finish sterilizing items in the %2$s.",
                                    "It should take %1$d minutes to finish sterilizing items in the %2$s.", minutes ),
                         minutes, cur_veh.name );
                break;
            }
        }
        if( autoclave_finished && !vpi.has_flag( VPFLAG_APPLIANCE ) ) {
            add_msg( _( "The autoclave in the %s has finished its cycle." ), cur_veh.name );
        } else if( autoclave_finished ) {
            add_msg( _( "The autoclave has finished its cycle." ) );
        }
    }

    const bool mws_here = vpi.has_flag( VPFLAG_MWS ) && vp.enabled;
    if( mws_here ) {
        bool mws_finished = false;
        vehicle_stack icheck = cur_veh.get_items( vp );
        if( icheck.empty() ) {
            add_msg( _( "The mobile weather station malfunctions and aborts the scan!" ) );
            vp.enabled = false;
        }
        for( item &n : cur_veh.get_items( vp ) ) {
            const time_duration cycle_time = 60_minutes;
            const time_duration time_left = cycle_time - n.age();
            if( ( n.typeId() == itype_mws_weather_data_incomplete ) &&
                ( ( current_weather( cur_veh.pos_abs() ).str() == "portal_storm" ) ||
                  ( current_weather( cur_veh.pos_abs() ).str() == "early_portal_storm" ) ||
                  ( current_weather( cur_veh.pos_abs() ).str() == "distant_portal_storm" ) ||
                  ( current_weather( cur_veh.pos_abs() ).str() == "close_portal_storm" ) ) ) {
                n.set_flag( flag_MWS_PORTAL_STORM_DATA );
            }
            if( time_left <= 0_turns ) {
                mws_finished = true;
                vp.enabled = false;
                if( n.typeId() == itype_mws_weather_data_incomplete ) {
                    if( n.has_flag( flag_MWS_PORTAL_STORM_DATA ) ) {
                        if( vpi.has_flag( VPFLAG_ADVANCED_MWS ) ) {
                            cur_veh.add_item( here, vp, item( itype_HEW_printout_data_portal_storm, calendar::turn_zero ) );
                        } else {
                            cur_veh.add_item( here, vp, item( itype_mws_portal_storm_weather_data, calendar::turn_zero ) );
                        }
                    }
                    cur_veh.remove_item( vp, &n );
                }
            } else if( calendar::once_every( 15_minutes ) ) {
                add_msg( _( "The instruments on the mobile weather station silently rotate." ) );
                break;
            }
        }
        if( mws_finished ) {
            add_msg( _( "The mobile weather station has finished its recording.  The printer whirs and the report sits ready to be torn away." ) );
            cur_veh.add_item( here, vp, item( itype_mws_weather_data, calendar::turn_zero ) );
            if( vpi.has_flag( VPFLAG_ADVANCED_MWS ) ) {
                const tripoint_abs_omt veh_position = cur_veh.pos_abs_omt();
                const tripoint_bub_ms veh_position_bub = cur_veh.pos_bub( here );
                bool portal_nearby = false;
                for( const tripoint_bub_ms &p : points_in_radius( veh_position_bub, 50 ) ) {
                    if( here.tr_at( p ).id == tr_portal ) {
                        portal_nearby = true;
                        break;
                    }
                }
                const tripoint_abs_omt closest_vitrified_farm = overmap_buffer.find_closest( veh_position,
                        "unvitrified_orchard", 10, false );
                const tripoint_abs_omt closest_lixa = overmap_buffer.find_closest( veh_position, "LIXA_surface_1a",
                                                      10, false );
                const tripoint_abs_omt closest_temple = overmap_buffer.find_closest( veh_position, "temple_stairs",
                                                        10, false );
                const tripoint_abs_omt closest_highlands = overmap_buffer.find_closest( veh_position,
                        "highlands", 10, false );
                const tripoint_abs_omt closest_exodii = overmap_buffer.find_closest( veh_position,
                                                        "exodii_base_x0y0z0", 10, false );
                const tripoint_abs_omt closest_labyrinth = overmap_buffer.find_closest( veh_position,
                        "labyrinth_entrance", 10, false );
                const tripoint_abs_omt closest_physics_lab = overmap_buffer.find_closest( veh_position,
                        "microlab_portal_security1", 10, false );
                const tripoint_abs_omt closest_amigara = overmap_buffer.find_closest( veh_position,
                        "mine_amigara_finale_central", 10, false );
                const tripoint_abs_omt closest_radiosphere = overmap_buffer.find_closest( veh_position,
                        "radiosphere_radio_tower_top", 10, false );
                const tripoint_abs_omt closest_barricaded_church = overmap_buffer.find_closest( veh_position,
                        "xedra_rural_church", 10, false );
                const tripoint_abs_omt closest_morgantown = overmap_buffer.find_closest( veh_position,
                        "morgantown_2b", 10, false );
                const tripoint_abs_omt closest_cabin_reverb = overmap_buffer.find_closest( veh_position,
                        "cabin_3_reverberation", 10, false );
                const tripoint_abs_omt closest_cabin_reverb_hint = overmap_buffer.find_closest( veh_position,
                        "cabin_3_reverberation_hint", 10, false );
                const tripoint_abs_omt closest_monster_corpse = overmap_buffer.find_closest( veh_position,
                        "corpse_bowels_mid", 10, false );
                const tripoint_abs_omt closest_spiral_mine = overmap_buffer.find_closest( veh_position,
                        "mine_spiral_finale_s", 10, false );
                const tripoint_abs_omt closest_string_dimension = overmap_buffer.find_closest( veh_position,
                        "string_dimension_crossroads", 10, false );
                const tripoint_abs_omt closest_void_spider_lair = overmap_buffer.find_closest( veh_position,
                        "void_spider_lair", 10, false );
                const tripoint_abs_omt closest_inner_cabins_open_land = overmap_buffer.find_closest( veh_position,
                        "inner_cabins", 10, false );
                const tripoint_abs_omt closest_inner_cabins_warped_cabin = overmap_buffer.find_closest(
                            veh_position,
                            "inner_cabins_warped_cabin_10", 10, false );
                const tripoint_abs_omt closest_inner_cabins_home_cabin = overmap_buffer.find_closest( veh_position,
                        "inner_cabins_cabin", 10, false );
                const tripoint_abs_omt closest_portal_storm_dungeon = overmap_buffer.find_closest( veh_position,
                        "default_portal_storm_dungeon_overmap", 10, false );
                if( portal_nearby ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_portal, calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_vitrified_farm ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_vitrified, calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_lixa ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_lixa, calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_temple ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_strange_temple, calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_highlands ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_highlands, calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_exodii ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_exodii, calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_labyrinth ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_labyrinth, calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_physics_lab ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_physics_lab, calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_amigara ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_amigara, calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_barricaded_church ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_barricaded_church,
                                                      calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_morgantown ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_morgantown, calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_cabin_reverb ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_cabin_reverb_dimension,
                                                      calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_cabin_reverb_hint ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_cabin_reverb, calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_radiosphere ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_radiosphere, calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_spiral_mine ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_spiral_mine, calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_string_dimension ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_string_dimension, calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_void_spider_lair ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_void_spider_lair, calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_portal_storm_dungeon ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_portal_storm_dungeon,
                                                      calendar::turn_zero ) );
                }
                if( trig_dist( veh_position, closest_inner_cabins_open_land ) <= 10 ) {
                    if( trig_dist( veh_position, closest_inner_cabins_home_cabin ) <= 10 ) {
                        cur_veh.add_item( here, vp, item( itype_HEW_printout_data_inner_cabins_home_cabin,
                                                          calendar::turn_zero ) );
                    } else if( trig_dist( veh_position, closest_inner_cabins_warped_cabin ) <= 10 ) {
                        cur_veh.add_item( here, vp, item( itype_HEW_printout_data_inner_cabins_warped_cabin,
                                                          calendar::turn_zero ) );
                    } else {
                        cur_veh.add_item( here, vp, item( itype_HEW_printout_data_inner_cabins_open_field,
                                                          calendar::turn_zero ) );
                    }
                }
                if( trig_dist( veh_position, closest_monster_corpse ) < 1 ) {
                    cur_veh.add_item( here, vp, item( itype_mws_monster_corpse_weather_data, calendar::turn_zero ) );
                } else if( trig_dist( veh_position, closest_monster_corpse ) <= 10 ) {
                    cur_veh.add_item( here, vp, item( itype_HEW_printout_data_monster_corpse, calendar::turn_zero ) );
                }
            }
        }
    }

    const int recharge_part_idx = cur_veh.part_with_feature( part, VPFLAG_RECHARGE, true );
    if( recharge_part_idx >= 0 ) {

        vehicle_part &recharge_part = cur_veh.part( recharge_part_idx );

        int turns_elapsed = to_turns<int>( calendar::turn - recharge_part.last_charged );
        recharge_part.last_charged = calendar::turn;

        if( !recharge_part.removed && recharge_part.enabled  && ( turns_elapsed > 0 ) &&
            cur_veh.is_battery_available( here ) ) {

            int dischargeable = turns_elapsed * recharge_part.info().bonus;
            // Convert to kilojoule
            dischargeable = ( dischargeable / 1000 ) + x_in_y( dischargeable % 1000, 1000 );

            bool is_running = false;

            for( item &n : cur_veh.get_items( vp ) ) {

                // "dischargeable" could be 0 even if the battery charger is actually running,
                // because of the rounding when the power is below a kilojoule. Before breaking,
                // I need to know if there is at least a non-full battery, for the purpose of
                // displaying the correct power draw.
                if( is_running && dischargeable <= 0 ) {
                    break;
                }

                if( !n.has_flag( flag_RECHARGE ) && !n.has_flag( flag_USE_UPS ) ) {
                    continue;
                }

                // TODO: BATTERIES this should be rewritten when vehicle power and items both use energy quantities
                int chargeable = {};
                if( n.is_vehicle_battery() ) {
                    chargeable = std::ceil( units::to_kilojoule<double>( n.type->battery->max_capacity -
                                            n.energy_remaining( nullptr ) ) );
                } else if( n.ammo_capacity( ammo_battery ) ) {
                    chargeable = n.ammo_capacity( ammo_battery ) - n.ammo_remaining( );
                }

                if( !is_running && chargeable > 0 ) {
                    is_running = true;
                    if( dischargeable <= 0 ) {
                        break;
                    }
                }

                if( chargeable > 0 ) {

                    // Around 85% efficient; a few of the discharges don't actually recharge
                    const int needed_for_full_charge = ( chargeable * 7 / 6 ) + x_in_y( chargeable * 7 % 6, 6 );
                    const int to_discharge = std::min( needed_for_full_charge, dischargeable );
                    const int discharged = to_discharge - cur_veh.discharge_battery( here, to_discharge );

                    int charged = {};
                    if( discharged < to_discharge  || needed_for_full_charge >= dischargeable ) {
                        charged = ( discharged * 6 / 7 ) + x_in_y( discharged * 6 % 7, 7 );
                        dischargeable = 0;
                    } else {
                        charged = chargeable;
                        dischargeable -= needed_for_full_charge;
                    }

                    if( n.is_vehicle_battery() ) {
                        n.mod_energy( units::from_kilojoule( static_cast<std::int64_t>( charged ) ) );
                    } else {
                        n.ammo_set( itype_battery, n.ammo_remaining( ) + charged );
                    }

                }

            }

            if( is_running ) {
                cur_veh.recharge_epower_this_turn -= units::from_watt( static_cast<std::int64_t>
                                                     ( recharge_part.info().bonus ) );
            }

        }
    }
}

void map::process_items()
{
    const int minz = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
    const int maxz = zlevels ? OVERMAP_HEIGHT : abs_sub.z();
    for( int gz = minz; gz <= maxz; ++gz ) {
        level_cache &cache = access_cache( gz );
        std::set<tripoint> submaps_with_vehicles;
        for( vehicle *this_vehicle : cache.vehicle_list ) {
            tripoint_bub_ms pos = this_vehicle->pos_bub( *this );
            submaps_with_vehicles.emplace( pos.x() / SEEX, pos.y() / SEEY, pos.z() );
        }
        for( const tripoint &pos : submaps_with_vehicles ) {
            submap *const current_submap = get_submap_at_grid( tripoint_rel_sm( pos ) );
            if( current_submap == nullptr ) {
                debugmsg( "Tried to process items at (%d,%d,%d) but the submap is not loaded", pos.x, pos.y,
                          pos.z );
                continue;
            }
            // Vehicles first in case they get blown up and drop active items on the map.
            process_items_in_vehicles( *current_submap );
        }
    }
    update_submaps_with_active_items();
    for( auto iter = submaps_with_active_items.begin(); iter != submaps_with_active_items.end(); ) {
        tripoint_abs_sm const abs_pos = *iter;
        if( !inbounds( project_to<coords::ms>( abs_pos ) ) ) {
            iter = submaps_with_active_items.erase( iter );
            continue;
        }
        const tripoint_rel_sm local_pos = abs_pos - abs_sub.xy();
        submap *const current_submap = get_submap_at_grid( local_pos );
        if( current_submap == nullptr ) {
            debugmsg( "Tried to process items at %s but the submap is not loaded",
                      local_pos.to_string() );
            continue;
        }
        // TODO: fix point types
        process_items_in_submap( *current_submap, local_pos );
        if( current_submap->active_items.empty() ) {
            iter = submaps_with_active_items.erase( iter );
        } else {
            ++iter;
        }
    }
}

void map::process_items_in_submap( submap &current_submap, const tripoint_rel_sm &gridp )
{
    // Get a COPY of the active item list for this submap.
    // If more are added as a side effect of processing, they are ignored this turn.
    // If they are destroyed before processing, they don't get processed.
    std::vector<item_reference> active_items = current_submap.active_items.get_for_processing();
    const point_bub_ms grid_offset( gridp.x() * SEEX, gridp.y() * SEEY );
    for( item_reference &active_item_ref : active_items ) {
        if( !active_item_ref.item_ref ) {
            // The item was destroyed, so skip it.
            continue;
        }

        const tripoint_bub_ms map_location = tripoint_bub_ms( grid_offset + active_item_ref.location,
                                             gridp.z() );
        const furn_t &furn = this->furn( map_location ).obj();

        if( furn.has_flag( ter_furn_flag::TFLAG_DONT_REMOVE_ROTTEN ) ) {
            // plants contain a seed item which must not be removed under any circumstances.
            // Lets not process it at all.
            continue;
        }
        // root cellars are special
        temperature_flag flag = temperature_flag::NORMAL;
        if( ter( map_location ) == ter_t_rootcellar ) {
            flag = temperature_flag::ROOT_CELLAR;
        }

        float spoil_multiplier = 1.0f;

        if( has_flag( ter_furn_flag::TFLAG_NO_SPOIL, map_location ) ) {
            spoil_multiplier = 0.0f;
        }

        bool furniture_is_sealed = has_flag( ter_furn_flag::TFLAG_SEALED, map_location );

        map_stack items = i_at( map_location );
        process_map_items( *this, items, active_item_ref.item_ref, active_item_ref.parent, map_location,
                           map_cursor( this, map_location ), active_item_ref.insulation(), flag,
                           spoil_multiplier * active_item_ref.spoil_multiplier(), furniture_is_sealed ||
                           active_item_ref.has_watertight_container() );
    }
}

std::vector<item_reference> map::item_network_connections( vehicle *power_grid )
{
    std::vector<item_reference> result;
    for( const auto &iter : submaps_with_active_items ) {
        tripoint_abs_sm const abs_pos = iter;
        const tripoint_rel_sm local_pos = abs_pos - abs_sub.xy();
        submap *const current_submap = get_submap_at_grid( local_pos );
        std::vector<item_reference> active_items = current_submap->active_items.get_for_processing();
        for( item_reference &active_item_ref : active_items ) {
            if( !active_item_ref.item_ref ) {
                continue;
            }

            if( active_item_ref.item_ref->has_link_data() &&
                active_item_ref.item_ref->link().t_veh &&
                active_item_ref.item_ref->link().t_veh->pos_abs() == power_grid->pos_abs() ) {
                result.emplace_back( active_item_ref );
            }
        }
    }

    return result;
}

void map::process_items_in_vehicles( submap &current_submap )
{
    // a copy, important if the vehicle list changes because a
    // vehicle got destroyed by a bomb (an active item!), this list
    // won't change, but veh_in_nonant will change.
    std::vector<vehicle *> vehicles;
    vehicles.reserve( current_submap.vehicles.size() );
    for( const auto &veh : current_submap.vehicles ) {
        vehicles.push_back( veh.get() );
    }
    for( vehicle *&cur_veh : vehicles ) {
        if( !current_submap.contains_vehicle( cur_veh ) ) {
            // vehicle not in the vehicle list of the nonant, has been
            // destroyed (or moved to another nonant?)
            // Can't be sure that it still exists, so skip it
            continue;
        }

        process_items_in_vehicle( *cur_veh, current_submap );
    }
}

void map::process_items_in_vehicle( vehicle &cur_veh, submap &current_submap )
{
    const bool engine_heater_is_on = cur_veh.has_part( "E_HEATER", true ) && cur_veh.engine_on;
    for( const vpart_reference &vp : cur_veh.get_any_parts( VPFLAG_FLUIDTANK ) ) {
        vp.part().process_contents( *this, vp.pos_bub( *this ), engine_heater_is_on );
    }

    auto cargo_parts = cur_veh.get_parts_including_carried( VPFLAG_CARGO );
    for( const vpart_reference &vp : cargo_parts ) {
        process_vehicle_items( cur_veh, vp.part_index() );
    }

    for( item_reference &active_item_ref : cur_veh.active_items.get_for_processing() ) {
        if( empty( cargo_parts ) ) {
            return;
        } else if( !active_item_ref.item_ref ) {
            // The item was destroyed, so skip it.
            continue;
        }
        const auto it = std::find_if( begin( cargo_parts ),
        end( cargo_parts ), [&]( const vpart_reference & part ) {
            return active_item_ref.location == part.mount_pos();
        } );

        if( it == end( cargo_parts ) ) {
            continue; // Can't find a cargo part matching the active item.
        }
        const item &target = *active_item_ref.item_ref;
        // Find the cargo part and coordinates corresponding to the current active item.
        const vehicle_part &pt = it->part();
        const tripoint_bub_ms item_loc = it->pos_bub( *this );
        const vehicle_cursor item_cur( it->vehicle(), it->part_index() );
        vehicle_stack items = cur_veh.get_items( pt );
        float it_insulation = 1.0f;
        temperature_flag flag = temperature_flag::NORMAL;
        if( target.has_temperature() || target.is_food_container() ) {
            const vpart_info &pti = pt.info();
            if( engine_heater_is_on ) {
                flag = temperature_flag::HEATER;
            }
            // some vehicle parts provide insulation, default is 1
            it_insulation = item::find_type( pti.base_item )->insulation_factor;

            if( pt.enabled && pti.has_flag( VPFLAG_FRIDGE ) ) {
                it_insulation = 1; // ignore fridge insulation if on
                flag = temperature_flag::FRIDGE;
            } else if( pt.enabled && pti.has_flag( VPFLAG_FREEZER ) ) {
                it_insulation = 1; // ignore freezer insulation if on
                flag = temperature_flag::FREEZER;
            } else if( pt.enabled && pti.has_flag( VPFLAG_HEATED_TANK ) ) {
                it_insulation = 1; // ignore tank insulation if on
                flag = temperature_flag::HEATER;
            }
        }
        bool in_tank = pt.info().has_flag( VPFLAG_FLUIDTANK );
        if( !process_map_items( *this, items, active_item_ref.item_ref, active_item_ref.parent, item_loc,
                                item_cur, it_insulation, flag, active_item_ref.spoil_multiplier(), in_tank ||
                                active_item_ref.has_watertight_container() ) ) {
            // If the item was NOT destroyed, we can skip the remainder,
            // which handles fallout from the vehicle being damaged.
            continue;
        }

        // item does not exist anymore, might have been an exploding bomb,
        // check if the vehicle is still valid (does exist)
        if( !current_submap.contains_vehicle( &cur_veh ) ) {
            // Nope, vehicle is not in the vehicle list of the submap,
            // it might have moved to another submap (unlikely)
            // or be destroyed, anyway it does not need to be processed here
            return;
        }

        // Vehicle still valid, reload the list of cargo parts,
        // the list of cargo parts might have changed (imagine a part with
        // a low index has been removed by an explosion, all the other
        // parts would move up to fill the gap).
        cargo_parts = cur_veh.get_any_parts( VPFLAG_CARGO );
    }
}

// Crafting/item finding functions

// Note: this is called quite a lot when drawing tiles
// Console build has the most expensive parts optimized out
bool map::sees_some_items( const tripoint_bub_ms &p, const Creature &who ) const
{
    // Can only see items if there are any items.
    return has_items( p ) && could_see_items( p, who.pos_bub() );
}

bool map::sees_some_items( const tripoint_bub_ms &p, const tripoint_bub_ms &from ) const
{
    return has_items( p ) && could_see_items( p, from );
}

bool map::could_see_items( const tripoint_bub_ms &p, const Creature &who ) const
{
    return could_see_items( p, who.pos_bub() );
}

bool map::could_see_items( const tripoint_bub_ms &p, const tripoint_bub_ms &from ) const
{
    static const std::string container_string( "CONTAINER" );
    const bool container = has_flag_ter_or_furn( container_string, p );
    const bool sealed = has_flag_ter_or_furn( ter_furn_flag::TFLAG_SEALED, p );
    if( sealed && container ) {
        // never see inside of sealed containers
        return false;
    }
    if( container ) {
        // can see inside of containers if adjacent or
        // on top of the container
        return std::abs( p.x() - from.x() ) <= 1 &&
               std::abs( p.y() - from.y() ) <= 1 &&
               std::abs( p.z() - from.z() ) <= 1;
    }
    return true;
}

bool map::has_items( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }

    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to check items at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return false;
    }

    return !current_submap->get_items( l ).empty();
}

bool map::only_liquid_in_liquidcont( const tripoint_bub_ms &p )
{
    if( has_flag( ter_furn_flag::TFLAG_LIQUIDCONT, p ) ) {
        for( const item &it : i_at( p ) ) {
            if( it.made_of( phase_id::SOLID ) ) {
                return false;
            }
        }
        return true;
    }
    return false;
}

template <typename Stack, typename veh_or_map_cursor>
static std::list<item> use_amount_stack( const veh_or_map_cursor &cur, Stack stack,
        const itype_id &type, int &quantity, const std::function<bool( const item & )> &filter )
{
    std::list<item> ret;
    for( auto a = stack.begin(); a != stack.end() && quantity > 0; ) {
        // item::use_amount flattens contents before the filter sees anything, so a
        // reserved provider has to be pruned here, where the root is still one thing.
        if( craft_reservation::contains_reserved( *a ) ) {
            ++a;
            continue;
        }
        if( a->use_amount( item_location( cur, &*a ), type, quantity, ret, filter ) ) {
            a = stack.erase( a );
        } else {
            ++a;
        }
    }
    return ret;
}

std::list<item> map::use_amount_square( const tripoint_bub_ms &p, const itype_id &type,
                                        int &quantity, const std::function<bool( const item & )> &filter )
{
    std::list<item> ret;
    // Handle infinite map sources.
    item water = liquid_from( p );
    if( water.typeId() == type && water.charges == item::INFINITE_CHARGES ) {
        ret.push_back( water );
        quantity = 0;
        return ret;
    }

    if( const std::optional<vpart_reference> ovp = veh_at( p ).cargo() ) {
        const vehicle_cursor cur( ovp->vehicle(), ovp->part_index() );
        std::list<item> tmp = use_amount_stack( cur, ovp->items(), type, quantity, filter );
        ret.splice( ret.end(), tmp );
    }
    map_cursor cur( p );
    std::list<item> tmp = use_amount_stack( cur, i_at( p ), type, quantity, filter );
    ret.splice( ret.end(), tmp );
    return ret;
}

std::list<item_location> map::items_with( const tripoint_bub_ms &p,
        const std::function<bool( const item & )> &filter )
{
    std::list<item_location> ret;
    if( const std::optional<vpart_reference> vp = veh_at( p ).cargo() ) {
        for( item &it : vp->items() ) {
            if( filter( it ) ) {
                ret.emplace_back( vehicle_cursor( vp->vehicle(), vp->part_index() ), &it );
            }
        }
    }
    for( item &it : i_at( p ) ) {
        if( filter( it ) ) {
            ret.emplace_back( map_cursor( this, p ), &it );
        }
    }
    return ret;
}

std::list<item> map::use_amount( const std::vector<tripoint_bub_ms> &reachable_pts,
                                 const itype_id &type,
                                 int &quantity, const std::function<bool( const item & )> &filter, bool select_ind )
{
    std::list<item> ret;
    if( select_ind && !type->count_by_charges() ) {
        std::vector<item_location> locs;
        for( const tripoint_bub_ms &p : reachable_pts ) {
            std::list<item_location> tmp = items_with( p, [&filter, &type]( const item & it ) -> bool {
                return filter( it ) && it.typeId() == type;
            } );
            locs.insert( locs.end(), tmp.begin(), tmp.end() );
        }
        while( quantity != static_cast<int>( locs.size() ) && quantity > 0 && !locs.empty() ) {
            uilist imenu;
            //~ Select components from the map to consume. %d = number of components left to consume.
            imenu.title = string_format( _( "Select which component to use (%d left)" ), quantity );
            for( const item_location &loc : locs ) {
                imenu.addentry( loc->display_name() + " (" + loc.describe() + ")" );
            }
            imenu.query();
            if( imenu.ret < 0 || static_cast<size_t>( imenu.ret ) >= locs.size() ) {
                break;
            }
            locs[imenu.ret]->use_amount( locs[imenu.ret], type, quantity, ret, filter );
            locs[imenu.ret].remove_item();
            locs.erase( locs.begin() + imenu.ret );
        }
    }
    for( const tripoint_bub_ms &p : reachable_pts ) {
        if( accessible_items( p ) ) {
            std::list<item> tmp = use_amount_square( p, type, quantity, filter );
            ret.splice( ret.end(), tmp );
        }
    }
    return ret;
}

std::list<item> map::use_amount( const tripoint_bub_ms &origin, const int range,
                                 const itype_id &type,
                                 int &quantity, const std::function<bool( const item & )> &filter, bool select_ind )
{
    const std::vector<tripoint_bub_ms> &reachable_pts = reachable_flood_steps( origin, range );
    return use_amount( reachable_pts, type, quantity, filter, select_ind );
}

static void use_charges_from_furn( const furn_t &f, const itype_id &type, int &quantity,
                                   map *m, const tripoint_bub_ms &p, std::list<item> &ret,
                                   const std::function<bool( const item & )> &filter, bool in_tools )
{
    if( m->has_flag( ter_furn_flag::TFLAG_LIQUIDCONT, p ) ) {
        map_stack item_list = m->i_at( p );
        auto current_item = item_list.begin();
        for( ; current_item != item_list.end(); ++current_item ) {
            // looking for a liquid that matches
            if( filter( *current_item ) && current_item->made_of( phase_id::LIQUID ) &&
                type == current_item->typeId() ) {
                ret.push_back( *current_item );
                if( current_item->charges - quantity > 0 ) {
                    // Update the returned liquid amount to match the requested amount
                    ret.back().charges = quantity;
                    // Update the liquid item in the world to contain the leftover liquid
                    current_item->charges -= quantity;
                    // All the liquid needed was found, no other sources will be needed
                    quantity = 0;
                } else {
                    // The liquid copy in ret already contains how much was available
                    // The leftover quantity returned will check other sources
                    quantity -= current_item->charges;
                    // Remove liquid item from the world
                    item_list.erase( current_item );
                }
                return;
            }
        }
    }

    // TODO(multimag): furniture pseudo-tool is built here with a single
    // ammo_set; multimag furniture-tools need every MAGAZINE_WELL populated.
    // Workbench/forge-class items can't be converted to multimag yet.
    const itype *itt = f.crafting_pseudo_item_type();
    if( itt != nullptr && itt->tool && !itt->tool->ammo_id.empty() ) {
        const bool using_ammotype = f.has_flag( ter_furn_flag::TFLAG_AMMOTYPE_RELOAD );
        map_stack stack = m->i_at( p );
        for( const itype *ammo_itype : f.crafting_ammo_item_types() ) {
            itype_id ammo = ammo_itype->get_id();
            auto iter = std::find_if( stack.begin(), stack.end(),
            [ammo, using_ammotype]( const item & i ) {
                if( using_ammotype && i.type->ammo && ammo->ammo ) {
                    return i.type->ammo->type == ammo->ammo->type;
                } else {
                    return i.typeId() == ammo;
                }
            } );
            if( iter != stack.end() ) {
                // pseudo tools are per-call and have no uid, so an item filter can
                // never reject them.  guard on the tile they come from instead.
                if( get_craft_reservations().provider_tile_reserved( m->get_abs( p ) ) ) {
                    return;
                }
                item furn_item( itt, calendar::turn_zero );
                furn_item.ammo_set( ammo, iter->charges );

                if( !filter( furn_item ) ) {
                    return;
                }
                item_location loc( map_cursor( m, p ), &furn_item );
                if( furn_item.use_charges( loc, type, quantity, ret, p, return_true<item>, in_tools ) ) {
                    stack.erase( iter );
                } else {
                    iter->charges = furn_item.ammo_remaining( );
                }
            }
        }
    }
}

std::list<item> map::use_charges( const std::vector<tripoint_bub_ms> &reachable_pts,
                                  const itype_id &type, int &quantity,
                                  const std::function<bool( const item & )> &filter,
                                  basecamp *bcp, bool in_tools )
{
    std::list<item> ret;

    // We prefer infinite map sources where available, so search for those
    // first
    for( const tripoint_bub_ms &p : reachable_pts ) {
        // Handle infinite map sources.
        item water = liquid_from( p );
        if( water.typeId() == type && water.charges == item::INFINITE_CHARGES ) {
            water.charges = quantity;
            ret.push_back( water );
            quantity = 0;
            return ret;
        }
    }

    if( bcp ) {
        ret = bcp->use_charges( type, quantity );
        if( quantity <= 0 ) {
            return ret;
        }
    }

    for( const tripoint_bub_ms &p : reachable_pts ) {
        if( accessible_items( p ) ) {
            const map_cursor cur( this, p );
            std::list<item> tmp = i_at( p ).use_charges( type, quantity, p, cur, filter,
                                  in_tools );
            ret.splice( ret.end(), tmp );
            if( quantity <= 0 ) {
                return ret;
            }
        }

        if( has_furn( p ) ) {
            use_charges_from_furn( furn( p ).obj(), type, quantity, this, p, ret, filter, in_tools );
            if( quantity <= 0 ) {
                return ret;
            }
        }

        const optional_vpart_position vp = veh_at( p );
        if( vp ) {
            std::list<item> tmp = vp->vehicle().use_charges( *this, *vp, type, quantity, filter, in_tools );
            ret.splice( ret.end(), tmp );
            if( quantity <= 0 ) {
                return ret;
            }
        }
    }

    return ret;
}

std::list<item> map::use_charges( const tripoint_bub_ms &origin, const int range,
                                  const itype_id &type, int &quantity,
                                  const std::function<bool( const item & )> &filter,
                                  basecamp *bcp, bool in_tools )
{
    // populate a grid of spots that can be reached
    const std::vector<tripoint_bub_ms> &reachable_pts = reachable_flood_steps( origin, range );
    return use_charges( reachable_pts, type, quantity, filter, bcp, in_tools );
}

units::energy map::consume_ups( const std::vector<tripoint_bub_ms> &reachable_pts,
                                units::energy qty )
{
    const units::energy wanted_qty = qty;

    // populate a grid of spots that can be reached

    for( const tripoint_bub_ms &p : reachable_pts ) {
        if( accessible_items( p ) ) {

            map_stack items = i_at( p );
            for( item &elem : items ) {
                if( elem.has_flag( flag_IS_UPS ) ) {
                    qty -= elem.energy_consume( qty, p, nullptr );
                    if( qty <= 0_J ) {
                        break;
                    }
                }
            }
        }
    }

    return wanted_qty - qty;
}

units::energy map::consume_ups( const tripoint_bub_ms &origin, const int range, units::energy qty )
{
    // populate a grid of spots that can be reached
    const std::vector<tripoint_bub_ms> &reachable_pts = reachable_flood_steps( origin, range );
    return consume_ups( reachable_pts, qty );
}

std::list<std::pair<tripoint_bub_ms, item *> > map::get_rc_items( const tripoint_bub_ms &p )
{
    std::list<std::pair<tripoint_bub_ms, item *> > rc_pairs;
    tripoint_bub_ms pos;
    pos.z() = abs_sub.z();
    for( pos.x() = 0; pos.x() < MAPSIZE_X; pos.x()++ ) {
        if( p.x() != -1 && p.x() != pos.x() ) {
            continue;
        }
        for( pos.y() = 0; pos.y() < MAPSIZE_Y; pos.y()++ ) {
            if( p.y() != -1 && p.y() != pos.y() ) {
                continue;
            }
            map_stack items = i_at( pos );
            for( item &elem : items ) {
                if( elem.has_flag( flag_RADIO_ACTIVATION ) || elem.has_flag( flag_RADIO_CONTAINER ) ) {
                    rc_pairs.emplace_back( pos, &elem );
                }
            }
        }
    }

    return rc_pairs;
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

void map::update_submaps_with_active_items()
{
    std::move( submaps_with_active_items_dirty.begin(), submaps_with_active_items_dirty.end(),
               std::inserter( submaps_with_active_items, submaps_with_active_items.begin() ) );
    submaps_with_active_items_dirty.clear();
}

static std::optional<char32_t> get_memory_at( const tripoint_bub_ms &p )
{
    const memorized_tile &mt = get_avatar().get_memorized_tile( get_map().get_abs( p ) );
    if( mt.symbol != 0 ) {
        return mt.symbol;
    }
    return std::nullopt;
}

void map::draw( const catacurses::window &w, const tripoint_bub_ms &center )
{
    // We only need to draw anything if we're not in tiles mode.
    if( is_draw_tiles_mode() ) {
        return;
    }

    g->reset_light_level();

    update_visibility_cache( center.z() );
    const visibility_variables &cache = get_visibility_variables_cache();

    const auto &visibility_cache = get_cache_ref( center.z() ).visibility_cache;

    int wnd_h = getmaxy( w );
    int wnd_w = getmaxx( w );
    const tripoint_bub_ms offs = center - tripoint_rel_ms( wnd_w / 2, wnd_h / 2, 0 );

    // Map memory should be at least the size of the view range
    // so that new tiles can be memorized, and at least the size of the terminal
    // since displayed area may be bigger than view range.
    const point_bub_ms min_mm_reg{
        std::min( 0, offs.x() ),
        std::min( 0, offs.y() )
    };
    const point_bub_ms max_mm_reg{
        std::max( MAPSIZE_X, offs.x() + wnd_w ),
        std::max( MAPSIZE_Y, offs.y() + wnd_h )
    };
    avatar &player_character = get_avatar();
    player_character.prepare_map_memory_region(
        get_abs( { min_mm_reg, center.z() } ),
        get_abs( { max_mm_reg, center.z() } ) );

    const auto draw_background = [&]( const tripoint_bub_ms & p ) {
        int sym = ' ';
        nc_color col = c_black;
        if( const std::optional<char32_t> memorized_symbol = get_memory_at( p ) ) {
            sym = *memorized_symbol;
            col = c_brown;
        }
        wputch( w, col, sym );
    };

    const auto draw_vision_effect = [&]( const visibility_type vis ) -> bool {
        int sym = '#';
        nc_color col;
        switch( vis )
        {
            case visibility_type::LIT:
                // can only tell that this square is bright
                col = c_light_gray;
                break;
            case visibility_type::BOOMER:
                col = c_pink;
                break;
            case visibility_type::BOOMER_DARK:
                col = c_magenta;
                break;
            default:
                return false;
        }
        wputch( w, col, sym );
        return true;
    };

    drawsq_params params = drawsq_params().memorize( true );
    for( int wy = 0; wy < wnd_h; wy++ ) {
        for( int wx = 0; wx < wnd_w; wx++ ) {
            wmove( w, point( wx, wy ) );
            const tripoint_bub_ms p = offs + tripoint_rel_ms( wx, wy, 0 );
            if( !inbounds( p ) ) {
                draw_background( p );
                continue;
            }

            const lit_level lighting = visibility_cache[p.x()][p.y()];
            const visibility_type vis = get_visibility( lighting, cache );

            if( draw_vision_effect( vis ) ) {
                continue;
            }

            if( vis == visibility_type::HIDDEN || vis == visibility_type::DARK ) {
                draw_background( p );
                continue;
            }

            const const_maptile curr_maptile = maptile_at_internal( p );
            params
            .low_light( lighting == lit_level::LOW )
            .bright_light( lighting == lit_level::BRIGHT );
            if( draw_maptile( w, p, curr_maptile, params ) ) {
                continue;
            }
            const maptile tile_below = maptile_at_internal( p + tripoint::below );
            draw_from_above( w, p + tripoint::below, tile_below, params );
        }
    }

    // Memorize off-screen tiles
    half_open_rectangle<point_bub_ms> display( offs.xy(), offs.xy() + point( wnd_w, wnd_h ) );
    drawsq_params mm_params = drawsq_params().memorize( true ).output( false );
    for( int y = 0; y < MAPSIZE_Y; y++ ) {
        for( int x = 0; x < MAPSIZE_X; x++ ) {
            const tripoint_bub_ms p( x, y, center.z() );
            if( display.contains( p.xy() ) ) {
                // Have been memorized during display loop
                continue;
            }

            const lit_level lighting = visibility_cache[p.x()][p.y()];
            const visibility_type vis = get_visibility( lighting, cache );

            if( vis != visibility_type::CLEAR ) {
                continue;
            }

            const maptile curr_maptile = maptile_at_internal( p );
            mm_params
            .low_light( lighting == lit_level::LOW )
            .bright_light( lighting == lit_level::BRIGHT );

            draw_maptile( w, p, curr_maptile, mm_params );
        }
    }
}

void map::drawsq( const catacurses::window &w, const tripoint_bub_ms &p,
                  const drawsq_params &params ) const
{
    // If we are in tiles mode, the only thing we want to potentially draw is a highlight
    if( is_draw_tiles_mode() ) {
        if( params.highlight() ) {
            g->draw_highlight( p );
        }
        return;
    }

    if( !inbounds( p ) ) {
        return;
    }

    const tripoint_bub_ms view_center = params.center();
    const int k = p.x() + getmaxx( w ) / 2 - view_center.x();
    const int j = p.y() + getmaxy( w ) / 2 - view_center.y();
    if( k < 0 || k >= getmaxx( w ) || j < 0 || j >= getmaxy( w ) ) {
        return;
    }
    wmove( w, point( k, j ) );

    const const_maptile tile = maptile_at( p );
    if( draw_maptile( w, p, tile, params ) ) {
        return;
    }

    tripoint_bub_ms below( p + tripoint::below );
    const const_maptile tile_below = maptile_at( below );
    draw_from_above( w, below, tile_below, params );
}

// a check to see if the lower floor needs to be rendered in tiles
bool map::dont_draw_lower_floor( const tripoint_bub_ms &p ) const
{
    if( !zlevels || p.z() <= -OVERMAP_DEPTH ) {
        return true;
    } else if( !inbounds( p ) ) {
        return false;
    } else {
        return get_cache( p.z() ).floor_cache[p.x()][p.y()];
    }
}

bool map::draw_maptile( const catacurses::window &w, const tripoint_bub_ms &p,
                        const const_maptile &curr_maptile, const drawsq_params &params ) const
{
    drawsq_params param = params;
    nc_color tercol;
    const ter_t &curr_ter = params.terrain_override().is_null()
                            ? curr_maptile.get_ter_t() : params.terrain_override().obj();
    const furn_t &curr_furn = params.furniture_override().is_null()
                              ? curr_maptile.get_furn_t() : params.furniture_override().obj();
    const trap &curr_trap = curr_maptile.get_trap().obj();
    const field &curr_field = curr_maptile.get_field();
    int sym;
    int memory_sym;
    bool hi = false;
    bool graf = false;
    bool draw_item_sym = false;

    if( curr_ter.has_flag( ter_furn_flag::TFLAG_AUTO_WALL_SYMBOL ) ) {
        memory_sym = sym = determine_wall_corner( p );
        tercol = curr_ter.color();
    } else {
        memory_sym = sym = curr_ter.symbol();
        tercol = curr_ter.color();
    }

    avatar &player_character = get_avatar();
    if( curr_furn.id ) {
        sym = curr_furn.symbol();
        tercol = curr_furn.color();
        if( !( player_character.get_grab_type() == object_type::FURNITURE
               && p == player_character.pos_bub() + player_character.grab_point ) ) {
            memory_sym = sym;
        }
    }
    if( curr_ter.has_flag( ter_furn_flag::TFLAG_SWIMMABLE ) &&
        curr_ter.has_flag( ter_furn_flag::TFLAG_DEEP_WATER ) &&
        !player_character.is_underwater() ) {
        param.show_items( false ); // Can only see underwater items if WE are underwater
    }
    // If there's a trap here, and we have sufficient perception, draw that instead
    if( curr_trap.can_see( p, player_character ) ) {
        tercol = curr_trap.color;
        if( curr_trap.sym == '%' ) {
            switch( rng( 1, 5 ) ) {
                case 1:
                    memory_sym = sym = '*';
                    break;
                case 2:
                    memory_sym = sym = '0';
                    break;
                case 3:
                    memory_sym = sym = '8';
                    break;
                case 4:
                    memory_sym = sym = '&';
                    break;
                case 5:
                    memory_sym = sym = '+';
                    break;
            }
        } else {
            memory_sym = sym = curr_trap.sym;
        }
    }
    if( const_cast<map *>( this )->partial_con_at( p ) != nullptr ) {
        tercol = tr_unfinished_construction->color;
        memory_sym = sym = tr_unfinished_construction->sym;
    }
    if( curr_field.field_count() > 0 ) {
        const field_type_id &fid = curr_field.displayed_field_type();
        const field_entry *fe = curr_field.find_field( fid );
        const std::string field_symbol = fid->get_symbol();
        if( field_symbol == "&" || fe == nullptr ) {
            // Do nothing, a '&' indicates invisible fields.
        } else if( field_symbol == "*" ) {
            // A random symbol.
            switch( rng( 1, 5 ) ) {
                case 1:
                    memory_sym = sym = '*';
                    break;
                case 2:
                    memory_sym = sym = '0';
                    break;
                case 3:
                    memory_sym = sym = '8';
                    break;
                case 4:
                    memory_sym = sym = '&';
                    break;
                case 5:
                    memory_sym = sym = '+';
                    break;
            }
        } else {
            // A field symbol '%' indicates the field should not hide
            // items/terrain. When the symbol is not '%' it will
            // hide items (the color is still inverted if there are items,
            // but the tile symbol is not changed).
            // draw_item_sym indicates that the item symbol should be used
            // even if sym is not '.'.
            // As we don't know at this stage if there are any items
            // (that are visible to the player!), we always set the symbol.
            // If there are items and the field does not hide them,
            // the code handling items will override it.
            draw_item_sym = ( field_symbol == "%" );
            // If field display_priority is > 1, and the field is set to hide items,
            //draw the field as it obscures what's under it.
            if( ( field_symbol != "%" && fid.obj().priority > 1 ) || ( field_symbol != "%" &&
                    sym == '.' ) )  {
                // default terrain '.' and
                // non-default field symbol -> field symbol overrides terrain
                memory_sym = sym = static_cast<uint8_t>( field_symbol[0] );
            }
            tercol = fe->color();
        }
    }

    // TODO: change the local variable sym to std::string and use it instead of this hack.
    // Currently this are different variables because terrain/... uses int as symbol type and
    // item now use string. Ideally they should all be strings.
    std::string item_sym;

    // If there are items here, draw those instead
    if( param.show_items() && curr_maptile.get_item_count() > 0 &&
        sees_some_items( p, player_character ) ) {
        // if there's furniture/terrain/trap/fields (sym!='.')
        // and we should not override it, then only highlight the square
        if( sym != '.' && sym != '%' && !draw_item_sym ) {
            hi = true;
        } else {
            // otherwise override with the symbol of the last item
            item_sym = curr_maptile.get_uppermost_item().symbol();
            if( !draw_item_sym ) {
                tercol = curr_maptile.get_uppermost_item().color();
            }
            if( curr_maptile.get_item_count() > 1 ) {
                param.highlight( !param.highlight() );
            }
        }
    }

    int veh_part = 0;
    const vehicle *veh = veh_at_internal( p, veh_part );
    if( veh != nullptr ) {
        const vpart_display vd = veh->get_display_of_tile( veh->part( veh_part ).mount );
        sym = vd.symbol_curses;
        tercol = vd.color;
        item_sym.clear(); // clear the item symbol so `sym` is used instead.

        if( !veh->forward_velocity() && !veh->player_in_control( *this, player_character )
            && !( player_character.get_grab_type() == object_type::VEHICLE
                  && veh->get_points().count( ( player_character.pos_abs() +
                                                player_character.grab_point ) ) ) ) {
            memory_sym = sym;
        }
        if( !vd.carried_furn.empty() ) {
            furn_str_id furn( vd.carried_furn );
            sym = furn->symbol();
            tercol = furn->color();
        }
    }

    if( param.memorize() && memory_cache_ter_is_dirty( p ) ) {
        player_character.memorize_symbol( get_abs( p ), memory_sym );
        memory_cache_ter_set_dirty( p, false );
    }

    // If there's graffiti here, change background color
    if( curr_maptile.has_graffiti() ) {
        graf = true;
    }

    const auto u_vision = player_character.get_vision_modes();
    if( u_vision[BOOMERED] ) {
        tercol = c_magenta;
    } else if( u_vision[NV_GOGGLES] ) {
        tercol = param.bright_light() ? c_white : c_light_green;
    } else if( param.low_light() || u_vision[DARKNESS] ) {
        tercol = c_dark_gray;
    }

    if( param.highlight() ) {
        tercol = invert_color( tercol );
    } else if( hi ) {
        tercol = hilite( tercol );
    } else if( graf ) {
        tercol = red_background( tercol );
    }

    if( item_sym.empty() && sym == ' ' ) {
        if( !zlevels || p.z() <= -OVERMAP_DEPTH || !curr_ter.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) ) {
            // Print filler symbol
            tercol = c_black;
        } else {
            // Draw tile underneath this one instead
            return false;
        }
    }

    if( params.output() ) {
        if( item_sym.empty() ) {
            wputch( w, tercol, sym );
        } else {
            wprintz( w, tercol, item_sym );
        }
    }
    return true;
}

void map::draw_from_above( const catacurses::window &w, const tripoint_bub_ms &p,
                           const const_maptile &curr_tile, const drawsq_params &params ) const
{
    static const int AUTO_WALL_PLACEHOLDER = 2; // this should never appear as a real symbol!

    nc_color tercol = c_dark_gray;
    int sym = ' ';

    const ter_t &curr_ter = curr_tile.get_ter_t();
    const furn_t &curr_furn = curr_tile.get_furn_t();
    int part_below;
    const vehicle *veh;
    if( curr_furn.has_flag( ter_furn_flag::TFLAG_SEEN_FROM_ABOVE ) ) {
        sym = curr_furn.symbol();
        tercol = curr_furn.color();
    } else if( curr_furn.movecost < 0 ) {
        sym = '.';
        tercol = curr_furn.color();
    } else if( ( veh = veh_at_internal( p, part_below ) ) != nullptr ) {
        const vpart_position vpp( const_cast<vehicle &>( *veh ), part_below );
        const vpart_display vd = veh->get_display_of_tile( vpp.mount_pos(), true, true, true );
        const int roof = veh->roof_at_part( part_below );
        sym = vd.symbol_curses;
        tercol = roof >= 0 || vpp.obstacle_at_part() ? c_light_gray : c_light_gray_cyan;
    } else if( curr_ter.has_flag( ter_furn_flag::TFLAG_SEEN_FROM_ABOVE ) ) {
        if( curr_ter.has_flag( ter_furn_flag::TFLAG_AUTO_WALL_SYMBOL ) ) {
            sym = AUTO_WALL_PLACEHOLDER;
        } else if( curr_ter.has_flag( ter_furn_flag::TFLAG_RAMP ) ) {
            sym = '>';
        } else {
            sym = curr_ter.symbol();
        }
        tercol = curr_ter.color();
    } else if( curr_ter.movecost == 0 ) {
        sym = '.';
        tercol = curr_ter.color();
    } else if( !curr_ter.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) ) {
        sym = '.';
        if( curr_ter.color() != c_cyan ) {
            // Need a special case here, it doesn't cyanize well
            tercol = cyan_background( curr_ter.color() );
        } else {
            tercol = c_black_cyan;
        }
    } else {
        sym = curr_ter.symbol();
        tercol = curr_ter.color();
    }

    if( sym == AUTO_WALL_PLACEHOLDER ) {
        sym = determine_wall_corner( p );
    }

    const std::bitset<NUM_VISION_MODES> &u_vision = get_player_character().get_vision_modes();
    if( u_vision[BOOMERED] ) {
        tercol = c_magenta;
    } else if( u_vision[NV_GOGGLES] ) {
        tercol = params.bright_light() ? c_white : c_light_green;
    } else if( params.low_light() || u_vision[DARKNESS] ) {
        tercol = c_dark_gray;
    }

    if( params.highlight() ) {
        tercol = invert_color( tercol );
    }

    if( params.output() ) {
        wputch( w, tercol, sym );
    }
}

// This method tries a bunch of initial offsets for the line to try and find a clear one.
// Basically it does, "Find a line from any point in the source that ends up in the target square".
std::vector<tripoint_bub_ms> map::find_clear_path( const tripoint_bub_ms &source,
        const tripoint_bub_ms &destination, bool empty_on_fail ) const
{
    // TODO: Push this junk down into the Bresenham method, it's already doing it.
    const point d( destination.xy().raw() - source.xy().raw() );
    const point a( std::abs( d.x ) * 2, std::abs( d.y ) * 2 );
    const int dominant = std::max( a.x, a.y );
    const int minor = std::min( a.x, a.y );
    // This seems to be the method for finding the ideal start value for the error value.
    const int ideal_start_offset = minor - dominant / 2;
    const int start_sign = ( ideal_start_offset > 0 ) - ( ideal_start_offset < 0 );
    // Not totally sure of the derivation.
    const int max_start_offset = std::abs( ideal_start_offset ) * 2 + 1;
    for( int horizontal_offset = -1; horizontal_offset <= max_start_offset; ++horizontal_offset ) {
        int candidate_offset = horizontal_offset * ( start_sign == 0 ? 1 : start_sign );
        if( sees( source, destination, rl_dist( source, destination ),
                  candidate_offset, /*with_fields=*/true, /*allow_cached=*/false, los_trace::physical ) ) {
            return line_to( source, destination, candidate_offset, 0 );
        }
    }
    // If we couldn't find a clear LoS, depending on empty_on_fail
    // return either the ideal one or an empty vector.
    if( empty_on_fail ) {
        return std::vector<tripoint_bub_ms>();
    } else {
        return line_to( source, destination, ideal_start_offset, 0 );
    }
}

std::vector<tripoint_bub_ms> map::reachable_flood_steps( const tripoint_bub_ms &f, int range,
        const int cost_min, const int cost_max ) const
{
    struct pq_item {
        int dist;
        int ndx;
    };
    struct pq_item_comp {
        bool operator()( const pq_item &left, const pq_item &right ) {
            return left.dist > right.dist;
        }
    };
    using PQ_type = std::priority_queue< pq_item, std::vector<pq_item>, pq_item_comp>;

    // temp buffer for grid
    const int grid_dim = range * 2 + 1;
    const size_t grid_area = static_cast<size_t>( grid_dim ) * grid_dim;
    // init to -1 as "not visited yet"
    std::vector<int> t_grid( grid_area, -1 );
    const tripoint_rel_ms origin_offset = { range, range, 0 };
    const int initial_visit_distance = range * range; // Large unreachable value

    // Fill positions that are visitable with initial_visit_distance
    for( const tripoint_bub_ms &p : points_in_radius( f, range ) ) {
        const tripoint_bub_ms tp = { p.xy(), f.z()};
        const int tp_cost = move_cost( tp );
        // rejection conditions
        if( tp_cost < cost_min || tp_cost > cost_max || is_open_air( tp ) ) {
            continue;
        }
        // set initial cost for grid point
        tripoint_rel_ms origin_relative = tp - f;
        origin_relative += origin_offset;
        int ndx = origin_relative.x() + origin_relative.y() * grid_dim;
        t_grid[ndx] = initial_visit_distance;
    }

    auto gen_neighbors = []( const pq_item & elem, int grid_dim, std::array<pq_item, 8> &neighbors ) {
        // Up to 8 neighbors
        int new_cost = elem.dist + 1;
        // *INDENT-OFF*
        const std::array<int, 8> ox = {
            -1, 0, 1,
            -1,    1,
            -1, 0, 1
        };
        const std::array<int, 8> oy = {
            -1, -1, -1,
            0,      0,
            1,  1,  1
        };
        // *INDENT-ON*

        point e( elem.ndx % grid_dim, elem.ndx / grid_dim );
        for( int i = 0; i < 8; ++i ) {
            point n( e + point( ox[i], oy[i] ) );

            int ndx = n.x + n.y * grid_dim;
            neighbors[i] = { new_cost, ndx };
        }
    };

    PQ_type pq( pq_item_comp{} );
    pq_item first_item{ 0, range + range * grid_dim };
    pq.push( first_item );
    std::array<pq_item, 8> neighbor_elems;

    while( !pq.empty() ) {
        const pq_item item = pq.top();
        pq.pop();

        if( t_grid[item.ndx] == initial_visit_distance ) {
            t_grid[item.ndx] = item.dist;
            if( item.dist + 1 < range ) {
                gen_neighbors( item, grid_dim, neighbor_elems );
                for( pq_item neighbor_elem : neighbor_elems ) {
                    pq.push( neighbor_elem );
                }
            }
        }
    }
    std::vector<bool> o_grid( grid_area );
    int count = 0;
    for( int y = 0, ndx = 0; y < grid_dim; ++y ) {
        for( int x = 0; x < grid_dim; ++x, ++ndx ) {
            if( t_grid[ndx] != -1 && t_grid[ndx] < initial_visit_distance ) {
                // set self and neighbors to 1
                for( int y2 = y - 1; y2 <= y + 1; ++y2 ) {
                    for( int x2 = x - 1; x2 <= x + 1; ++x2 ) {
                        if( x2 >= 0 && x2 < grid_dim && y2 >= 0 && y2 < grid_dim ) {
                            const int index = x2 + ( y2 * grid_dim );
                            if( !o_grid[ index ] ) {
                                count++;
                                o_grid[ index ] = true;
                            }
                        }
                    }
                }
            }
        }
    }


    std::vector<tripoint_bub_ms> reachable_pts;
    if( count != 0 ) {
        reachable_pts.reserve( count );
        // Now go over again to pull out all of the reachable points
        for( int y = 0, ndx = 0; y < grid_dim; ++y ) {
            for( int x = 0; x < grid_dim; ++x, ++ndx ) {
                if( o_grid[ndx] ) {
                    tripoint_bub_ms t = f - origin_offset + tripoint{ x, y, 0 };
                    reachable_pts.push_back( t );
                }
            }
        }
    }
    return reachable_pts;
}

bool map::clear_path( const tripoint_bub_ms &f, const tripoint_bub_ms &t, const int range,
                      const int cost_min, const int cost_max ) const
{
    if( std::abs( f.z() - t.z() ) > fov_3d_z_range ) {
        return false;
    }

    // Ugly `if` for now
    if( f.z() == t.z() ) {
        if( ( range >= 0 && range < rl_dist( f.xy(), t.xy() ) ) ||
            !inbounds( t ) ) {
            return false; // Out of range!
        }
        bool is_clear = true;
        bresenham( f.xy(), t.xy(), 0,
        [this, &is_clear, cost_min, cost_max, &t]( const point_bub_ms & new_point ) {
            // Exit before checking the last square, it's still reachable even if it is an obstacle.
            if( new_point.x() == t.x() && new_point.y() == t.y() ) {
                return false;
            }

            const int cost = this->move_cost( new_point );
            if( cost < cost_min || cost > cost_max ) {
                is_clear = false;
                return false;
            }
            return true;
        } );
        return is_clear;
    }

    if( ( range >= 0 && range < rl_dist( f, t ) ) ||
        !inbounds( t ) ) {
        return false; // Out of range!
    }
    bool is_clear = true;
    tripoint_bub_ms last_point = f;
    bresenham( f, t, 0, 0,
    [this, &is_clear, cost_min, cost_max, t, &last_point]( const tripoint_bub_ms & new_point ) {
        // Exit before checking the last square, it's still reachable even if it is an obstacle.
        if( new_point == t ) {
            return false;
        }

        // We have to check a weird case where the move is both vertical and horizontal
        if( new_point.z() == last_point.z() ) {
            const int cost = move_cost( new_point );
            if( cost < cost_min || cost > cost_max ) {
                is_clear = false;
                return false;
            }
        } else {
            bool this_clear = false;
            const int max_z = std::max( new_point.z(), last_point.z() );
            if( !has_floor_or_support( { point_bub_ms( new_point.xy() ), max_z } ) ) {
                const int cost = move_cost( tripoint_bub_ms{ new_point.xy(), last_point.z()} );
                if( cost > cost_min && cost < cost_max ) {
                    this_clear = true;
                }
            }

            if( !this_clear && has_floor_or_support( { point_bub_ms( last_point.xy() ), max_z } ) ) {
                const int cost = move_cost( tripoint_bub_ms{ last_point.xy(), new_point.z()} );
                if( cost > cost_min && cost < cost_max ) {
                    this_clear = true;
                }
            }

            if( !this_clear ) {
                is_clear = false;
                return false;
            }
        }

        last_point = new_point;
        return true;
    } );
    return is_clear;
}

bool map::accessible_items( const tripoint_bub_ms &t ) const
{
    return !has_flag( ter_furn_flag::TFLAG_SEALED, t ) ||
           has_flag( ter_furn_flag::TFLAG_LIQUIDCONT, t );
}

void map::for_each_reachable_item( const tripoint_bub_ms &center, int radius,
                                   const Character *ch,
                                   const std::function<void( const item & )> &fn )
{
    for( const tripoint_bub_ms &p : reachable_flood_steps( center, radius ) ) {
        for_each_item_at( *this, p, ch, false, true, fn );
    }
}

void map::for_each_visible_item( const tripoint_bub_ms &center, int radius,
                                 const Character *ch,
                                 const std::function<void( const item & )> &fn )
{
    for( const tripoint_bub_ms &p : points_in_radius( center, radius ) ) {
        if( ch == nullptr || ch->sees( *this, p ) ) {
            for_each_item_at( *this, p, ch, true, false, fn );
        }
    }
}

std::vector<tripoint_bub_ms> map::get_dir_circle( const tripoint_bub_ms &f,
        const tripoint_bub_ms &t ) const
{
    std::vector<tripoint_bub_ms> circle;
    circle.resize( 8 );

    // The line below can be crazy expensive - we only take the FIRST point of it
    const std::vector<tripoint_bub_ms> line = line_to( f, t, 0, 0 );
    const std::vector<tripoint_bub_ms> spiral = closest_points_first( f, 1 );
    const std::vector<int> pos_index{ 1, 2, 4, 6, 8, 7, 5, 3 };

    //  All possible constellations (closest_points_first goes clockwise)
    //  753  531  312  124  246  468  687  875
    //  8 1  7 2  5 4  3 6  1 8  2 7  4 5  6 3
    //  642  864  786  578  357  135  213  421

    size_t pos_offset = 0;
    for( size_t i = 1; i < spiral.size(); i++ ) {
        if( spiral[i] == line[0] ) {
            pos_offset = i - 1;
            break;
        }
    }

    for( size_t i = 1; i < spiral.size(); i++ ) {
        if( pos_offset >= pos_index.size() ) {
            pos_offset = 0;
        }

        circle[pos_index[pos_offset++] - 1] = spiral[i];
    }

    return circle;
}

void map::save()
{
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            if( zlevels ) {
                for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
                    saven( { gridx, gridy, gridz } );
                }
            } else {
                saven( { gridx, gridy, abs_sub.z() } );
            }
        }
    }
}

void map::load( const tripoint_abs_sm &w, const bool update_vehicle,
                const bool pump_events )
{
    map &main_map = get_map();
    // It used to be unsafe to load a map that overlaps with the primary map;
    // Show an info line in tests to help track new errors
    if( test_mode && this != &main_map && main_map.inbounds( project_to<coords::ms>( w ) ) ) {
        DebugLog( D_INFO, DC_ALL )
                << "loading non-main map at " << w.to_string()
                << " which overlaps with main map (abs_sub = " << main_map.abs_sub.to_string() << ")";
    }
    for( auto &traps : traplocs ) {
        traps.clear();
    }
    field_furn_locs.clear();
    field_ter_locs.clear();
    submaps_with_active_items.clear();
    submaps_with_active_items_dirty.clear();
    set_abs_sub( w );
    clear_vehicle_level_caches();
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            loadn( { gridx, gridy }, update_vehicle );
            if( pump_events ) {
                inp_mngr.pump_events();
            }
        }
    }
    rebuild_vehicle_level_caches();

    if( !explosion_handler::explosion_processing_active() ) {
        // actualize after loading all submaps to prevent errors
        // with entities at the edges
        for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
            for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
                const int zmin = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
                const int zmax = zlevels ? OVERMAP_HEIGHT : abs_sub.z();
                for( int gridz = zmin; gridz <= zmax; gridz++ ) {
                    actualize( { gridx, gridy, gridz } );
                    if( pump_events ) {
                        inp_mngr.pump_events();
                    }
                }
            }
        }
    }

    // tinymap derives from map, and a remote load would otherwise wipe the bubble's locks.
    reconcile_loaded_items( this == &get_map()
                            ? reconcile_scope::full_rebuild
                            : reconcile_scope::additive );
}

void map::reconcile_loaded_items( const reconcile_scope scope )
{
    item_wakeup_manager &wakeups = get_item_wakeups();
    craft_reservation_index &index = get_craft_reservations();
    if( scope == reconcile_scope::full_rebuild ) {
        index.clear();
    }

    std::vector<int64_t> seen;
    auto reconcile_recursive = [&wakeups, &index, &seen]( item_location loc ) -> VisitResponse {
        if( !loc.valid() )
        {
            return VisitResponse::SKIP;
        }
        if( loc->is_craft() )
        {
            // On the token as well as the live step: the stale branch keeps the token and
            // clears passive_started_at, and that record still needs cleaning.
            const bool live = loc->get_passive_started_at() != calendar::before_time_starts;
            const int64_t token = loc->peek_reservation_owner_token();
            if( live || token != 0 ) {
                // A pre-feature save was stamped before reservations existed, so nothing
                // would ever call acquisition and the step would run unreserved to
                // completion.  The wakeup rebuild below picks up the cursor set here.
                if( live && token == 0 &&
                    loc->get_env_check_at() == calendar::before_time_starts ) {
                    loc->set_env_check_at( calendar::turn );
                }
                index.rebuild_for_craft( loc );
                if( loc->peek_reservation_owner_token() != 0 ) {
                    seen.push_back( loc->peek_reservation_owner_token() );
                }
            }
        }
        wakeups.rebuild_for_item( loc );
        return VisitResponse::NEXT;
    };

    // Submaps rather than tiles, and the z range guarded by zlevels: get_nonant drops the z
    // index on a map that does not support z levels, so every level would alias the one the
    // map holds and the walk would repeat itself once per level.
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            const int zmin = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
            const int zmax = zlevels ? OVERMAP_HEIGHT : abs_sub.z();
            for( int gridz = zmin; gridz <= zmax; gridz++ ) {
                const tripoint_rel_sm grid( gridx, gridy, gridz );
                submap *const sm = get_submap_at_grid( grid );
                if( sm == nullptr ) {
                    continue;
                }
                for( int sx = 0; sx < SEEX; ++sx ) {
                    for( int sy = 0; sy < SEEY; ++sy ) {
                        const tripoint_bub_ms p( sx + gridx * SEEX,
                                                 sy + gridy * SEEY, gridz );
                        const map_cursor cur( this, p );
                        // rather than visiting the map_cursor, we need to pull the items out
                        // otherwise we get an infinite loop with reconcile since map_cursor::visit
                        // loads a tinymap off-bubble, and that load reconciles again
                        for( item &it : sm->get_items( { sx, sy } ) ) {
                            item_location( cur, &it ).visit_items( reconcile_recursive );
                        }
                    }
                }
            }
        }
    }

    for( wrapped_vehicle &wv : get_vehicles() ) {
        if( wv.v == nullptr ) {
            continue;
        }
        for( const vpart_reference &vpr : wv.v->get_all_parts() ) {
            vehicle_cursor( *wv.v, vpr.part_index() ).visit_items( reconcile_recursive );
        }
    }

    get_avatar().visit_carried( reconcile_recursive );
    for( npc &n : g->all_npcs() ) {
        n.visit_carried( reconcile_recursive );
    }

    // Crafts this pass did not walk are left alone: the index retains earlier entries.
    for( const int64_t token : seen ) {
        const craft_reservation_index::record *rec = index.find( token );
        if( rec != nullptr && rec->empty() ) {
            index.erase( token );
        }
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

template<int SIZE, int MULTIPLIER>
void shift_bitset_cache( std::bitset<SIZE *SIZE> &cache, const point_rel_sm &s )
{
    // sx shifts by MULTIPLIER rows, sy shifts by MULTIPLIER columns.
    int shift_amount = s.x() * MULTIPLIER + s.y() * SIZE * MULTIPLIER;
    if( shift_amount > 0 ) {
        cache >>= static_cast<size_t>( shift_amount );
    } else if( shift_amount < 0 ) {
        cache <<= static_cast<size_t>( -shift_amount );
    }
    // Shifting in the y direction shifted in 0 values, no no additional clearing is necessary, but
    // a shift in the x direction makes values "wrap" to the next row, and they need to be zeroed.
    if( s.x() == 0 ) {
        return;
    }
    const size_t x_offset = s.x() > 0 ? SIZE - MULTIPLIER : 0;
    for( size_t y = 0; y < SIZE; ++y ) {
        size_t y_offset = y * SIZE;
        for( size_t x = 0; x < MULTIPLIER; ++x ) {
            cache.reset( y_offset + x_offset + x );
        }
    }
}

template void
shift_bitset_cache<MAPSIZE_X, SEEX>( std::bitset<MAPSIZE_X *MAPSIZE_X> &cache,
                                     const point_rel_sm &s );
template void
shift_bitset_cache<MAPSIZE, 1>( std::bitset<MAPSIZE *MAPSIZE> &cache, const point_rel_sm &s );

void map::shift( const point_rel_sm &sp )
{
    if( !zlevels ) {
        debugmsg( "map::shift called from map that doesn't support Z levels" );
        return;
    }

    // Special case of 0-shift; refresh the map
    if( sp == point_rel_sm::zero ) {
        return; // Skip this?
    }

    if( std::abs( sp.x() ) > 1 || std::abs( sp.y() ) > 1 ) {
        debugmsg( "map::shift called with a shift of more than one submap" );
        return;
    }

    const tripoint_abs_sm abs = get_abs_sub();
    std::vector<tripoint_rel_sm> loaded_grids;

    const int zmin = -OVERMAP_DEPTH;
    const int zmax = OVERMAP_HEIGHT;

    const int x_start = sp.x() >= 0 ? 0 : my_MAPSIZE - 1;
    const int x_stop = sp.x() >= 0 ? my_MAPSIZE : -1;
    const int x_step = sp.x() >= 0 ? 1 : -1;
    const int y_start = sp.y() >= 0 ? 0 : my_MAPSIZE - 1;
    const int y_stop = sp.y() >= 0 ? my_MAPSIZE : -1;
    const int y_step = sp.y() >= 0 ? 1 : -1;

    // Have to run on_unload before changing the abs_sub for the map.
    // TODO: can probably skip a bunch of iteration here.
    for( int gridx = x_start; gridx != x_stop; gridx += x_step ) {
        for( int gridy = y_start; gridy != y_stop; gridy += y_step ) {
            // The sp.x() and sp.y() check here establishes that we are moving
            // in the x or y direction respectively. If we are not moving in the x direction,
            // we don't want to call on_unload on grid points where gridx == x_start.
            if( ( sp.x() && gridx == x_start ) || ( sp.y() && gridy == y_start ) ) {
                for( int gridz = zmin; gridz <= zmax; gridz++ ) {
                    const tripoint_rel_sm grid( gridx, gridy, gridz );
                    on_unload( grid );
                }
            }
        }
    }
    set_abs_sub( abs + sp );

    g->shift_destination_preview( { -sp.x() * SEEX, -sp.y() * SEEY } );

    shift_traps( sp );

    vehicle *remoteveh = g->remoteveh();

    for( int gridz = zmin; gridz <= zmax; gridz++ ) {
        level_cache *cache = get_cache_lazy( gridz );
        if( !cache ) {
            continue;
        }
        for( vehicle *veh : cache->vehicle_list ) {
            veh->zones_dirty = true;
        }
    }

    // Shift the map sx submaps to the right and sy submaps down.
    // sx and sy should never be bigger than +/-1.
    // absx and absy are our position in the world, for saving/loading purposes.
    clear_vehicle_level_caches();

    for( int gridz = zmin; gridz <= zmax; gridz++ ) {
        // Clear vehicle list and rebuild after shift
        // mlangsdorf 2020 - this is kind of insane, building the cache is not free, why are
        // we doing this?
        clear_vehicle_list( gridz );
        level_cache *cache = get_cache_lazy( gridz );
        if( cache ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( cache->map_memory_cache_dec, sp );
            shift_bitset_cache<MAPSIZE_X, SEEX>( cache->map_memory_cache_ter, sp );
            cache->map_memory_sweep_pending = true;
            shift_bitset_cache<MAPSIZE, 1>( cache->field_cache, sp );
        }
    }

    for( int gridx = x_start; gridx != x_stop; gridx += x_step ) {
        for( int gridy = y_start; gridy != y_stop; gridy += y_step ) {
            if( gridx + sp.x() != x_stop && gridy + sp.y() != y_stop ) {
                for( int gridz = zmin; gridz <= zmax; gridz++ ) {
                    const tripoint_rel_sm grid( gridx, gridy, gridz );

                    copy_grid( grid, grid + sp );
                    submap *const cur_submap = get_submap_at_grid( grid );
                    if( cur_submap == nullptr ) {
                        debugmsg( "Tried to update vehicle list at %s but the submap is not loaded", grid.to_string() );
                        continue;
                    }
                    update_vehicle_list( cur_submap, gridz );
                }
            } else {
                loadn( { gridx, gridy }, true );

                for( int gridz = zmin; gridz <= zmax; gridz++ ) {
                    loaded_grids.emplace_back( gridx, gridy, gridz );
                }
            }
        }
    }

    rebuild_vehicle_level_caches();

    g->setremoteveh( remoteveh );

    if( !support_cache_dirty.empty() ) {
        std::set<tripoint_bub_ms> old_cache = std::move( support_cache_dirty );
        support_cache_dirty.clear();
        for( const tripoint_bub_ms &pt : old_cache ) {
            support_cache_dirty.insert( pt + point( -sp.x() * SEEX, -sp.y() * SEEY ) );
        }
    }
    // actualize after loading all submaps to prevent errors
    // with entities at the edges
    for( tripoint_rel_sm loaded_grid : loaded_grids ) {
        actualize( loaded_grid );
    }
    if( !loaded_grids.empty() ) {
        // A shift walks the bubble it now has, so the crafts it scrolled away from are
        // not seen and must keep the claims their leases still cover.
        reconcile_loaded_items( reconcile_scope::additive );
    }
}

void map::vertical_shift( const int newz )
{
    if( !zlevels ) {
        debugmsg( "Called map::vertical_shift in a non-z-level world" );
        return;
    }

    if( newz < -OVERMAP_DEPTH || newz > OVERMAP_HEIGHT ) {
        debugmsg( "Tried to get z-level %d outside allowed range of %d-%d",
                  newz, -OVERMAP_DEPTH, OVERMAP_HEIGHT );
        return;
    }

    tripoint_abs_sm trp = get_abs_sub();
    set_abs_sub( tripoint_abs_sm( trp.xy(), newz ) );

    // TODO: Remove the function when it's safe
}

// saven saves a single nonant.  worldx and worldy are used for the file
// name and specifies where in the world this nonant is.  gridx and gridy are
// the offset from the top left nonant:
// 0,0 1,0 2,0
// 0,1 1,1 2,1
// 0,2 1,2 2,2
// (worldx,worldy,worldz) denotes the absolute coordinate of the submap
// in grid[0].
void map::saven( const tripoint_bub_sm &grid )
{
    dbg( D_INFO ) << "map::saven(worldx[" << abs_sub.x() << "], worldy[" << abs_sub.y()
                  << "], worldz[" << abs_sub.z()
                  << "], gridx[" << grid.x() << "], gridy[" << grid.y() << "], gridz[" << grid.z() << "])";
    const int gridn = get_nonant( rebase_rel( grid ) );
    submap *submap_to_save = getsubmap( gridn );
    if( submap_to_save == nullptr ) {
        debugmsg( "Tried to save submap node (%d) but it's not loaded", gridn );
        return;
    }

    tripoint_rel_sm relative_origin = rebase_rel( grid );
    on_unload( relative_origin );
    const tripoint_abs_sm abs = abs_sub.xy() + relative_origin;

    if( !zlevels && grid.z() != abs_sub.z() ) {
        debugmsg( "Tried to save submap (%d,%d,%d) as (%d,%d,%d), which isn't supported in non-z-level builds",
                  abs.x(), abs.y(), abs_sub.z(), abs.x(), abs.y(), grid.z() );
    }

    dbg( D_INFO ) << "map::saven abs: " << abs
                  << "  gridn: " << gridn;
    submap_to_save->last_touched = calendar::turn;
    MAPBUFFER.add_submap( abs, submap_to_save );
}

// Optimized mapgen function that only works properly for very simple overmap types
// Does not create or require a temporary map and does its own saving.
// Note that it assumes the map doesn't exist: it's an error to call this when it does.
bool generate_uniform( const tripoint_abs_sm &p, const ter_str_id &ter )
{
    if( MAPBUFFER.submap_exists( p ) ) {
        return false;
    }

    std::unique_ptr<submap> sm = std::make_unique<submap>();
    sm->set_all_ter( ter, true );
    sm->last_touched = calendar::turn;
    return MAPBUFFER.add_submap( p, sm );
}

bool generate_uniform_omt( const tripoint_abs_sm &p, const oter_id &terrain_type )
{
    dbg( D_INFO ) << "generate_uniform p: " << p
                  << "  terrain_type: " << terrain_type.id().str();

    const std::optional<ter_str_id> ter = terrain_type->get_uniform_terrain();

    if( !ter ) {
        return false;
    }

    bool ret = true;
    for( int xd = 0; xd <= 1; xd++ ) {
        for( int yd = 0; yd <= 1; yd++ ) {
            ret &= generate_uniform( p + point_rel_sm( xd, yd ), *ter );
        }
    }
    return ret;
}

void map::loadn( const point_bub_sm &grid, bool update_vehicles )
{
    dbg( D_INFO ) << "map::loadn(game[" << g.get() << "], worldx[" << abs_sub.x()
                  << "], worldy[" << abs_sub.y() << "], grid " << grid << ")";

    const tripoint_abs_sm grid_abs_sub = abs_sub + rebase_rel( grid );
    const tripoint_abs_omt grid_abs_omt = project_to<coords::omt>( grid_abs_sub );
    // Get the base submap "grid" is an offset from.
    const tripoint_abs_sm grid_sm_base = project_to<coords::sm>( grid_abs_omt );
    bool map_incomplete = false;

    map &bubble_map = reality_bubble();

    bool const main_inbounds =
        this != &bubble_map && bubble_map.inbounds( project_to<coords::ms>( grid_abs_sub ) );

    // It might be possible to just check the (0, 0) submap as we should never have
    // a case where only one submap is missing from an OMT level.
    for( int gridx = 0; !map_incomplete && gridx <= 1; gridx++ ) {
        for( int gridy = 0; !map_incomplete && gridy <= 1; gridy++ ) {
            for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
                const tripoint grid_pos( gridx, gridy, gridz );
                if( !MAPBUFFER.submap_exists( grid_sm_base.xy() + grid_pos ) ) {
                    map_incomplete = true;
                    break;
                }
            }
        }
    }

    if( map_incomplete ) {
        smallmap tmp_map;
        swap_map swap( *tmp_map.cast_to_map() );
        tmp_map.main_cleanup_override( false );
        tmp_map.generate( grid_abs_omt, calendar::turn, true );
        _main_requires_cleanup |= main_inbounds && tmp_map.is_main_cleanup_queued();

        for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
            const tripoint_abs_sm pos = {grid_sm_base.xy(), gridz };
            if( !MAPBUFFER.submap_exists( pos ) ) {
                dbg( D_ERROR ) << "failed to generate a submap at " << pos;
                debugmsg( "failed to generate a submap at %s", pos.to_string() );
                return;
            }
        }
    }

    const int start_z = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
    const int stop_z = zlevels ? OVERMAP_HEIGHT : abs_sub.z();

    submap *tmpsub;

    for( int z = start_z; z <= stop_z; z++ ) {
        const tripoint_abs_sm pos = { grid_abs_sub.xy(), z };
        // New submap changes the content of the map and all caches must be recalculated
        set_transparency_cache_dirty( z );
        set_seen_cache_dirty( z );
        set_outside_cache_dirty( z );
        set_floor_cache_dirty( z );
        set_pathfinding_cache_dirty( z );
        tmpsub = MAPBUFFER.lookup_submap( pos );
        setsubmap( get_nonant( tripoint_rel_sm{ grid.x(), grid.y(), z} ), tmpsub );
        if( !tmpsub->active_items.empty() ) {
            submaps_with_active_items_dirty.emplace( pos );
        }
        if( tmpsub->field_count > 0 ) {
            get_cache( z ).field_cache.set( grid.x() + grid.y() * MAPSIZE );
        }

        // Destroy bugged no-part vehicles
        auto &veh_vec = tmpsub->vehicles;
        for( auto iter = veh_vec.begin(); iter != veh_vec.end(); ) {
            vehicle *veh = iter->get();
            if( veh->part_count() > 0 ) {
                // Always fix submap coordinates for easier Z-level-related operations
                veh->sm_pos = abs_sub.xy() + tripoint_rel_sm{ rebase_rel( grid ), z};
                iter++;
                if( main_inbounds ) {
                    _main_requires_cleanup = true;
                }
            } else {
                if( veh->tracking_on ) {
                    overmap_buffer.remove_vehicle( veh );
                }
                dirty_vehicle_list.erase( veh );
                iter = veh_vec.erase( iter );
            }
        }

        // Update vehicle data
        if( update_vehicles ) {
            level_cache &map_cache = get_cache( z );
            for( const auto &veh : tmpsub->vehicles ) {
                // Only add if not tracking already.
                if( map_cache.vehicle_list.find( veh.get() ) == map_cache.vehicle_list.end() ) {
                    map_cache.vehicle_list.insert( veh.get() );
                    if( !veh->loot_zones.empty() ) {
                        map_cache.zone_vehicles.insert( veh.get() );
                    }
                }
            }
        }

        if( zlevels ) {
            add_tree_tops( { grid.x(), grid.y(), z } );
        }
        // When a submap is loaded into the reality bubble, apply
        // temperature-based phase changes
        const weather_generator &wgen = get_weather().get_cur_weather_gen();
        const tripoint_abs_sm pos_sm = { grid_abs_sub.xy(), z };
        const tripoint_abs_ms sm_abs_ms = project_to<coords::ms>( pos_sm );
        const tripoint_bub_ms sm_origin = get_bub( sm_abs_ms );
        for( int sx = 0; sx < SEEX; ++sx ) {
            for( int sy = 0; sy < SEEX; ++sy ) {
                temp_based_phase_change_at( sm_origin + point( sx, sy ), wgen );
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

void map::actualize( const tripoint_rel_sm &grid )
{
    submap *const tmpsub = get_submap_at_grid( grid );
    if( tmpsub == nullptr ) {
        debugmsg( "Actualize called on null submap (%d,%d,%d)", grid.x(), grid.y(), grid.z() );
        return;
    }

    for( const std::unique_ptr<vehicle> &veh : tmpsub->vehicles ) {
        // spill out items too large, MIGRATION pockets etc from vehicle parts
        for( const vpart_reference &vp : veh->get_all_parts() ) {
            const item &base_const = vp.part().get_base();
            const_cast<item &>( base_const ).overflow( *this, vp.pos_bub( *this ) );
        }
        veh->refresh( );
    }

    const time_duration time_since_last_actualize = calendar::turn - tmpsub->last_touched;
    const bool do_funnels = grid.z() >= 0;

    // check spoiled stuff, and fill up funnels while we're at it
    process_items_in_vehicles( *tmpsub );
    process_items_in_submap( *tmpsub, grid );
    explosion_handler::process_explosions();
    for( int x = 0; x < SEEX; x++ ) {
        for( int y = 0; y < SEEY; y++ ) {
            const tripoint_bub_ms pnt =  rebase_bub( coords::project_to<coords::ms>( grid ) + point( x, y ) );
            const point_sm_ms p( x, y );
            const furn_t &furn = *this->furn( pnt );
            const ter_t &terr = *this->ter( pnt );
            if( !furn.emissions.empty() ) {
                field_furn_locs.push_back( pnt );
            }
            if( !terr.emissions.empty() ) {
                field_ter_locs.push_back( pnt );
            }

            const trap_id trap_here = tmpsub->get_trap( p );
            if( trap_here != tr_null ) {
                traplocs[trap_here.to_i()].push_back( pnt );
            }
            const ter_t &ter = tmpsub->get_ter( p ).obj();
            if( ter.trap != tr_null ) {
                traplocs[ter.trap.to_i()].push_back( pnt );
            }

            if( do_funnels ) {
                fill_funnels( pnt, tmpsub->last_touched );
            }

            grow_plant( pnt );

            restock_fruits( pnt, time_since_last_actualize );

            produce_sap( pnt, time_since_last_actualize );

            rad_scorch( pnt, time_since_last_actualize );

            decay_cosmetic_fields( pnt, time_since_last_actualize );
        }
    }

    // the last time we touched the submap, is right now.
    tmpsub->last_touched = calendar::turn;
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

void map::on_unload( const tripoint_rel_sm &loc )
{
    // Look up the existing map data summary entry, if it's already dynamic we don't
    // need to check all the submaps, just update it.
    tripoint_abs_sm submap_abs_origin = get_abs_sub().xy() + loc;
    tripoint_abs_omt omt_abs_origin;
    point_omt_sm offset_within_omt;
    std::tie( omt_abs_origin, offset_within_omt ) = project_remain<coords::omt>( submap_abs_origin );
    // uuuugh this should be const?
    std::shared_ptr<map_data_summary> const_summary = overmap_buffer.get_omt_summary( omt_abs_origin );
    bool adjusted = !const_summary->placeholder;


    if( !adjusted ) {
        tripoint_rel_omt omt_origin = project_to<coords::omt>( loc );
        tripoint_rel_sm sm_origin = project_to<coords::sm>( omt_origin );
        std::vector<point_rel_sm> offsets;
        offsets.reserve( 4 );
        offsets.push_back( point_rel_sm::zero );
        offsets.push_back( point_rel_sm::south );
        offsets.push_back( point_rel_sm::east );
        offsets.push_back( point_rel_sm::south_east );
        for( const point_rel_sm &offset : offsets ) {
            tripoint_rel_sm submap_offset = sm_origin + offset;
            // Is there a better option than this manual check?
            if( submap_offset.x() < 0 || submap_offset.x() >= my_MAPSIZE ||
                submap_offset.y() < 0 || submap_offset.y() >= my_MAPSIZE ||
                submap_offset.z() < -OVERMAP_DEPTH || submap_offset.z() > OVERMAP_HEIGHT ) {
                continue;
            }
            // Handle null submap ptr, it can definitely happen and we just ignore it.
            if( submap *sm = get_submap_at_grid( submap_offset ); sm && sm->player_adjusted_map ) {
                adjusted = true;
                break;
            }

        }
    }
    // Update just the quadrant of the map_data_summary matching the current submap.
    if( adjusted ) {
        // Just the passable data for now, in future might want to copy the whole thing.
        // TODO: The interface here should be "retrieve writable summary" that handles returning a
        // new summary if the existing one is a placeholder.
        std::shared_ptr<map_data_summary> new_summary = std::make_shared<map_data_summary>
                ( const_summary->passable );
        tripoint_bub_ms submap_origin = get_bub( project_to<coords::ms>( submap_abs_origin ) );
        point summary_origin = offset_within_omt.raw();
        for( point cursor; cursor.y < 12; cursor.y++ ) {
            int y_component = summary_origin.y * 12 + cursor.y;
            for( cursor.x = 0; cursor.x < 12; cursor.x++ ) {
                int x_component = summary_origin.x * 12 + cursor.x;
                bool passable_value = passable( submap_origin + cursor );
                new_summary->passable.set( ( y_component * 24 ) + x_component, passable_value );
            }
        }
        // TODO: minimize copying?
        overmap_buffer.set_passable( omt_abs_origin, new_summary->passable );
    }
}

void map::copy_grid( const tripoint_rel_sm &to, const tripoint_rel_sm &from )
{
    submap *smap = get_submap_at_grid( from );
    if( smap == nullptr ) {
        debugmsg( "Tried to copy grid from (%d,%d,%d) but the submap is not loaded", from.x(), from.y(),
                  from.z() );
        return;
    }
    setsubmap( get_nonant( to ), smap );
    for( auto &it : smap->vehicles ) {
        it->sm_pos = abs_sub.xy() + to;
    }
}

void map::spawn_monsters_submap_group( const tripoint_rel_sm &gp, mongroup &group,
                                       bool ignore_sight )
{
    Character &player_character = get_player_character();
    const int s_range = std::min( HALF_MAPSIZE_X,
                                  player_character.sight_range( g->light_level( player_character.posz() ) ) );
    int pop = group.population;
    std::vector<tripoint_bub_ms> locations;
    if( !ignore_sight ) {
        // If the submap is one of the outermost submaps, assume that monsters are
        // invisible there.
        if( gp.x() == 0 || gp.y() == 0 || gp.x() + 1 == MAPSIZE || gp.y() + 1 == MAPSIZE ) {
            ignore_sight = true;
        }
    }

    if( gp.z() != player_character.posz() ) {
        // Note: this is only OK because 3D vision isn't a thing yet. 3D vision is a thing! Is this still OK?
        ignore_sight = true;
    }

    const auto allow_on_terrain = [&]( const tripoint_bub_ms & p ) {
        // TODO: flying creatures should be allowed to spawn without a floor,
        // but the new creature is created *after* determining the terrain, so
        // we can't check for it here.
        return passable( p ) && has_floor_or_water( p );
    };

    // If the submap is uniform, we can skip many checks
    const submap *current_submap = get_submap_at_grid( gp );
    if( current_submap == nullptr ) {
        debugmsg( "Tried spawn monster group at (%d,%d,%d) but the submap is not loaded", gp.x(), gp.y(),
                  gp.z() );
        return;
    }
    bool ignore_terrain_checks = false;
    bool ignore_inside_checks = gp.z() < 0;
    if( current_submap->is_uniform() ) {
        const tripoint_bub_ms upper_left{ SEEX * gp.x(), SEEY * gp.y(), gp.z()};
        if( !allow_on_terrain( upper_left ) ||
            ( !ignore_inside_checks && has_flag_ter_or_furn( ter_furn_flag::TFLAG_INDOORS, upper_left ) ) ) {
            const tripoint_abs_ms glp = get_abs( tripoint_bub_ms( gp.x() * SEEX, gp.y() * SEEY, gp.z() ) );
            dbg( D_WARNING ) << "Empty locations for group " << group.type.str() <<
                             " at uniform submap " << gp.x() << "," << gp.y() << "," << gp.z() <<
                             " global " << glp.x() << "," << glp.y() << "," << glp.z();
            return;
        }

        ignore_terrain_checks = true;
        ignore_inside_checks = true;
    }

    creature_tracker &creatures = get_creature_tracker();
    for( int x = 0; x < SEEX; ++x ) {
        for( int y = 0; y < SEEY; ++y ) {
            point_bub_ms f( x + SEEX * gp.x(), y + SEEY * gp.y() );
            tripoint_bub_ms fp{ f, gp.z()};
            if( creatures.creature_at( fp ) != nullptr ) {
                continue; // there is already some creature
            }

            if( !ignore_terrain_checks && !allow_on_terrain( fp ) ) {
                continue; // solid area, impassable
            }

            if( !ignore_sight && sees( player_character.pos_bub(), fp, s_range ) ) {
                continue; // monster must spawn outside the viewing range of the player
            }

            if( !ignore_inside_checks && has_flag_ter_or_furn( ter_furn_flag::TFLAG_INDOORS, fp ) ) {
                continue; // monster must spawn outside.
            }

            locations.push_back( fp );
        }
    }

    if( locations.empty() ) {
        // TODO: what now? there is no possible place to spawn monsters, most
        // likely because the player can see all the places.
        const tripoint_abs_ms glp = get_abs( tripoint_bub_ms( gp.x() * SEEX, gp.y() * SEEY, gp.z() ) );
        dbg( D_WARNING ) << "Empty locations for group " << group.type.str() <<
                         " at " << gp.x() << "," << gp.y() << "," << gp.z() <<
                         " global " << glp.x() << "," << glp.y() << "," << glp.z();
        // Just kill the group. It's not like we're removing existing monsters
        // Unless it's a horde - then don't kill it and let it spawn behind a tree or smoke cloud
        if( !group.horde ) {
            group.clear();
        }

        return;
    }

    if( pop ) {
        // Populate the group from its population variable.
        for( int m = 0; m < pop; m++ ) {
            std::vector<MonsterGroupResult> spawn_details =
                MonsterGroupManager::GetResultFromGroup( group.type, &pop );
            for( const MonsterGroupResult &mgr : spawn_details ) {
                if( !mgr.id ) {
                    continue;
                }
                monster tmp( mgr.id );

                // If a monster came from a horde population, configure them to always be willing to rejoin a horde.
                if( group.horde ) {
                    tmp.set_horde_attraction( MHA_ALWAYS );
                }
                for( int i = 0; i < mgr.pack_size; i++ ) {
                    group.monsters.push_back( tmp );
                }
            }
        }
    }

    // Find horde's target submap
    for( monster &tmp : group.monsters ) {
        for( int tries = 0; tries < 10 && !locations.empty(); tries++ ) {
            const tripoint_bub_ms local_pos = random_entry_removed( locations );
            const tripoint_abs_ms abs_pos = get_abs( local_pos );
            if( !tmp.can_move_to( local_pos ) ) {
                continue; // target can not contain the monster
            }
            if( group.horde ) {
                // Give monster a random point near horde's expected destination
                const tripoint_sm_ms pos_in_sm( rng( 0, SEEX ), rng( 0, SEEY ), local_pos.z() );
                const tripoint_abs_ms rand_dest = project_combine( group.target, pos_in_sm );

                // The monster will wander to the rand_dest for several turns which is determined by the distance and interest
                const int turns = rl_dist( abs_pos, rand_dest ) + group.interest;
                tmp.wander_to( rand_dest, turns );
                add_msg_debug( debugmode::DF_MAP, "%s targeting %s", tmp.disp_name(),
                               tmp.wander_pos.to_string_writable() );
            }

            monster *const placed = g->place_critter_at( make_shared_fast<monster>( tmp ),
                                    local_pos );
            if( placed ) {
                placed->on_load();
            }
            break;
        }
    }
    // indicates the group is empty, and can be removed later
    group.clear();
}

void map::spawn_monsters_submap( const tripoint_rel_sm &gp, bool ignore_sight, bool spawn_nonlocal )
{
    const tripoint_abs_sm submap_pos( gp + abs_sub.xy() );
    // Load unloaded monsters
    overmap_buffer.spawn_monster( submap_pos, spawn_nonlocal );
    // Only spawn new monsters after existing monsters are loaded.
    std::vector<mongroup *> groups = overmap_buffer.groups_at( submap_pos );
    for( mongroup *&mgp : groups ) {
        spawn_monsters_submap_group( gp, *mgp, ignore_sight );
    }

    submap *const current_submap = get_submap_at_grid( gp );
    if( current_submap == nullptr ) {
        debugmsg( "Tried spawn monsters at (%d,%d,%d) but the submap is not loaded", gp.x(), gp.y(),
                  gp.z() );
        return;
    }
    const tripoint_bub_ms gp_ms = rebase_bub( coords::project_to<coords::ms>( gp ) );

    // The list of spawns on the submap might be updated while we are iterating it.
    // For example, `monster::on_load` -> `monster::try_reproduce` calls `map::add_spawn`.
    // Therefore, this intentionally uses old-school indexed for-loop with re-check against `.size()` each step.
    // NOLINTNEXTLINE(modernize-loop-convert)
    for( size_t sp_i = 0; sp_i < current_submap->spawns.size(); ++sp_i ) {
        const spawn_point i = current_submap->spawns[sp_i]; // intentional copy
        const tripoint_bub_ms center = gp_ms + rebase_rel( i.pos );
        const tripoint_range<tripoint_bub_ms> points = points_in_radius( center, 3 );

        for( int j = 0; j < i.count; j++ ) {
            monster tmp( i.type );

            if( tmp.type->in_species( species_FERAL ) ) {
                if( one_in( 100 ) ) {
                    // Same chances and duration of flu vs. cold as for the player.
                    if( one_in( 6 ) ) {
                        tmp.add_effect( effect_fake_flu, rng( 3_days, 10_days ) );
                    } else {
                        tmp.add_effect( effect_fake_common_cold, rng( 1_days, 14_days ) );
                    }
                }
            }

            if( i.mission_id > 0 ) {
                mission *found_mission = mission::find( i.mission_id );
                if( found_mission != nullptr ) {
                    tmp.mission_ids = { i.mission_id };
                    if( found_mission->get_type().goal == MGOAL_KILL_MONSTERS ) {
                        found_mission->register_kill_needed();
                    }
                }
            }
            if( i.name.has_value() ) {
                tmp.unique_name = i.name.value();
            }
            if( i.friendly ) {
                tmp.friendly = -1;
                tmp.add_effect( effect_pet, 1_turns, true );
            }
            if( !i.data.ammo.empty() ) {
                for( std::pair<itype_id, jmapgen_int> ap : i.data.ammo ) {
                    tmp.ammo.emplace( ap.first, ap.second.get() );
                }
            } else {
                tmp.ammo = tmp.type->starting_ammo;
            }

            // This can fail, but there isn't much we can do about it if it does. We could output some
            // kind of info message, but, again, it's unlikely to result in anything that can be acted on.
            g->place_critter_at_or_within( make_shared_fast<monster>( tmp ), this, center, points );
        }
    }
    current_submap->spawns.clear();
}

void map::spawn_monsters( bool ignore_sight, bool spawn_nonlocal )
{
    const int zmin = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
    const int zmax = zlevels ? OVERMAP_HEIGHT : abs_sub.z();
    tripoint_rel_sm gp;
    int &gx = gp.x();
    int &gy = gp.y();
    int &gz = gp.z();
    for( gz = zmin; gz <= zmax; gz++ ) {
        for( gx = 0; gx < my_MAPSIZE; gx++ ) {
            for( gy = 0; gy < my_MAPSIZE; gy++ ) {
                spawn_monsters_submap( gp, ignore_sight, spawn_nonlocal );
            }
        }
    }
}

void map::clear_spawns()
{
    for( submap *&smap : grid ) {
        smap->spawns.clear();
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

bool map::inbounds( const tripoint_abs_ms &p ) const
{
    return inbounds( get_bub( p ) );
}

bool map::inbounds( const tripoint_bub_ms &p ) const
{
    return p.x() >= 0 && p.x() < my_MAPSIZE * SEEX &&
           p.y() >= 0 && p.y() < my_MAPSIZE * SEEY &&
           p.z() >= -OVERMAP_DEPTH && p.z() <= OVERMAP_HEIGHT;
    // && ( zlevels || p.z == get_abs_sub().z() );
    // Cannot actually let inbounds check the bounds for maps not supporting Z levels, as
    // tests explicitly expect other Z level coordinates to return t_null when read
    // and write terrain to the Z level of the map rather than fail or do nothing.
}

bool map::inbounds( const tripoint_abs_omt &p ) const
{
    const tripoint_abs_omt map_origin = project_to<coords::omt>( abs_sub );
    return  p.z() >= -OVERMAP_DEPTH && p.z() <= OVERMAP_HEIGHT &&
            ( zlevels || p.z() == get_abs_sub().z() ) &&
            p.x() >= map_origin.x() &&
            p.y() >= map_origin.y() &&
            p.x() <= map_origin.x() + my_HALF_MAPSIZE &&
            p.y() <= map_origin.y() + my_HALF_MAPSIZE;
}

tripoint_range<tripoint_omt_ms> tinymap::points_on_zlevel() const
{

    const tripoint_range <tripoint_bub_ms> temp = map::points_on_zlevel();
    return tripoint_range <tripoint_omt_ms>( rebase_omt( temp.min() ), rebase_omt( temp.max() ) );
}

tripoint_range<tripoint_omt_ms> tinymap::points_on_zlevel( int z ) const
{
    const tripoint_range<tripoint_bub_ms> temp = map::points_on_zlevel( z );
    return tripoint_range <tripoint_omt_ms>( rebase_omt( temp.min() ), rebase_omt( temp.max() ) );
}

tripoint_range<tripoint_omt_ms> tinymap::points_in_rectangle(
    const tripoint_omt_ms &from, const tripoint_omt_ms &to ) const
{
    const tripoint_range<tripoint_bub_ms> preliminary_result = map::points_in_rectangle( rebase_bub(
                from ),
            rebase_bub( to ) );
    return tripoint_range<tripoint_omt_ms>( rebase_omt( preliminary_result.min() ),
                                            rebase_omt( preliminary_result.max() ) );
}

tripoint_range<tripoint_omt_ms> tinymap::points_in_radius(
    const tripoint_omt_ms &center, size_t radius, size_t radiusz ) const
{
    const tripoint_range<tripoint_bub_ms> preliminary_result = map::points_in_radius( rebase_bub(
                center ), radius,
            radiusz );
    return tripoint_range<tripoint_omt_ms>( rebase_omt( preliminary_result.min() ),
                                            rebase_omt( preliminary_result.max() ) );
}

maptile tinymap::maptile_at( const tripoint_omt_ms &p )
{
    return map::maptile_at( rebase_bub( p ) );
}


// set up a map just long enough scribble on it
// this tinymap should never, ever get saved
fake_map::fake_map( const ter_id &ter_type )
{
    const tripoint_abs_sm tripoint_below_zero( 0, 0, fake_map_z );

    set_abs_sub( tripoint_below_zero );
    for( int gridx = 0; gridx < get_my_MAPSIZE(); gridx++ ) {
        for( int gridy = 0; gridy < get_my_MAPSIZE(); gridy++ ) {
            std::unique_ptr<submap> sm = std::make_unique<submap>();

            sm->set_all_ter( ter_type );
            sm->set_all_furn( furn_str_id::NULL_ID() );
            sm->set_all_traps( tr_null );

            setsubmap( get_nonant( tripoint_rel_sm{ gridx, gridy, fake_map_z } ), sm.get() );

            temp_submaps_.emplace_back( std::move( sm ) );
        }
    }
}

fake_map::~fake_map() = default;

small_fake_map::small_fake_map( const ter_id &ter_type )
{
    set_abs_sub( tripoint_abs_sm::zero );

    for( int gridx = 0; gridx < get_my_MAPSIZE(); gridx++ ) {
        for( int gridy = 0; gridy < get_my_MAPSIZE(); gridy++ ) {
            for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
                std::unique_ptr<submap> sm = std::make_unique<submap>();

                sm->set_all_ter( ter_type );
                sm->set_all_furn( furn_str_id::NULL_ID() );
                sm->set_all_traps( tr_null );

                setsubmap( get_nonant( tripoint_rel_sm{ gridx, gridy, gridz } ), sm.get() );

                temp_submaps_.emplace_back( std::move( sm ) );
            }
        }
    }
}

small_fake_map::~small_fake_map() = default;

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

int map::determine_wall_corner( const tripoint_bub_ms &p ) const
{
    const std::bitset<NUM_TERCONN> &test_connect_group = ter( p ).obj().connect_to_groups;
    uint8_t connections = get_known_connections( p, test_connect_group );
    // The bits in connections are SEWN, whereas the characters in LINE_
    // constants are NESW, so we want values in 8 | 2 | 1 | 4 order.
    switch( connections ) {
        case 8 | 2 | 1 | 4:
            return LINE_XXXX;
        case 0 | 2 | 1 | 4:
            return LINE_OXXX;

        case 8 | 0 | 1 | 4:
            return LINE_XOXX;
        case 0 | 0 | 1 | 4: // NOLINT(misc-redundant-expression)
            return LINE_OOXX;

        case 8 | 2 | 0 | 4:
            return LINE_XXOX;
        case 0 | 2 | 0 | 4: // NOLINT(misc-redundant-expression)
            return LINE_OXOX;
        case 8 | 0 | 0 | 4: // NOLINT(misc-redundant-expression)
            return LINE_XOOX;
        case 0 | 0 | 0 | 4: // NOLINT(misc-redundant-expression)
            return LINE_OXOX; // LINE_OOOX would be better

        case 8 | 2 | 1 | 0:
            return LINE_XXXO;
        case 0 | 2 | 1 | 0: // NOLINT(misc-redundant-expression)
            return LINE_OXXO;
        case 8 | 0 | 1 | 0: // NOLINT(bugprone-branch-clone,misc-redundant-expression)
            return LINE_XOXO;
        case 0 | 0 | 1 | 0: // NOLINT(misc-redundant-expression)
            return LINE_XOXO; // LINE_OOXO would be better
        case 8 | 2 | 0 | 0: // NOLINT(misc-redundant-expression)
            return LINE_XXOO;
        case 0 | 2 | 0 | 0: // NOLINT(misc-redundant-expression)
            return LINE_OXOX; // LINE_OXOO would be better
        case 8 | 0 | 0 | 0: // NOLINT(misc-redundant-expression)
            return LINE_XOXO; // LINE_XOOO would be better

        case 0 | 0 | 0 | 0: // NOLINT(misc-redundant-expression)
            return ter( p ).obj().symbol(); // technically just a column

        default:
            // cata_assert( false );
            // this shall not happen
            return '?';
    }
}

//////////
///// coordinate helpers

tripoint_abs_ms map::get_abs( const tripoint_bub_ms &p ) const
{
    return tripoint_abs_ms{ p.x() + abs_ms.x(), p.y() + abs_ms.y(), p.z() };
}

tripoint_bub_ms map::get_bub( const tripoint_abs_ms &p ) const
{
    return tripoint_bub_ms { p.x() - abs_ms.x(), p.y() - abs_ms.y(), p.z()};
}

void map::set_abs_sub( const tripoint_abs_sm &p )
{
    abs_sub = p;
    abs_ms = point_abs_ms{ project_to<coords::ms>( abs_sub.xy() ) };
}

tripoint_abs_sm map::get_abs_sub() const
{
    return abs_sub;
}

template<typename Array>
static cata::copy_const<Array, submap> *getsubmap_helper(
    const Array &grid, const size_t grididx )
{
    if( grididx >= grid.size() ) {
        debugmsg( "Tried to access invalid grid index %d. Grid size: %d", grididx, grid.size() );
        return nullptr;
    }
    return grid[grididx];
}

submap *map::getsubmap( const size_t grididx )
{
    return getsubmap_helper( grid, grididx );
}

const submap *map::getsubmap( const size_t grididx ) const
{
    return getsubmap_helper( grid, grididx );
}

submap *map::unsafe_get_submap_at( const tripoint_bub_ms &p )
{
    cata_assert( inbounds( p ) );
    return get_submap_at_grid( tripoint_rel_sm{ p.x() / SEEX, p.y() / SEEY, p.z() } );
}

const submap *map::unsafe_get_submap_at( const tripoint_bub_ms &p ) const
{
    cata_assert( inbounds( p ) );
    return get_submap_at_grid( { p.x() / SEEX, p.y() / SEEY, p.z() } );
}

void map::setsubmap( const size_t grididx, submap *const smap )
{
    if( grididx >= grid.size() ) {
        debugmsg( "Tried to access invalid grid index %d", grididx );
        return;
    } else if( smap == nullptr ) {
        debugmsg( "Tried to set NULL submap pointer at index %d", grididx );
        return;
    }
    grid[grididx] = smap;
}

submap *map::get_submap_at( const tripoint_bub_ms &p )
{
    if( !inbounds( p ) ) {
        debugmsg( "Tried to access invalid map position %s", p.to_string() );
        return nullptr;
    }
    return unsafe_get_submap_at( p );
}

const submap *map::get_submap_at( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        debugmsg( "Tried to access invalid map position %s", p.to_string() );
        return nullptr;
    }
    return unsafe_get_submap_at( p );
}

submap *map::get_submap_at_grid( const tripoint_rel_sm &gridp )
{
    return getsubmap( get_nonant( gridp ) );
}

const submap *map::get_submap_at_grid( const tripoint_rel_sm &gridp ) const
{
    return getsubmap( get_nonant( gridp ) );
}

size_t map::get_nonant( const tripoint_rel_sm &gridp ) const
{
    if( gridp.x() < 0 || gridp.x() >= my_MAPSIZE ||
        gridp.y() < 0 || gridp.y() >= my_MAPSIZE ||
        gridp.z() < -OVERMAP_DEPTH || gridp.z() > OVERMAP_HEIGHT ) {
        debugmsg( "Tried to access invalid map position at grid (%d,%d,%d)", gridp.x(), gridp.y(),
                  gridp.z() );
        return 0;
    }

    if( zlevels ) {
        const int indexz = gridp.z() + OVERMAP_DEPTH; // Can't be lower than 0
        return indexz + ( gridp.x() + gridp.y() * my_MAPSIZE ) * OVERMAP_LAYERS;
    } else {
        return gridp.x() + gridp.y() * my_MAPSIZE;
    }
}

void map::draw_line_ter( const ter_id &type, const point_bub_ms &p1, const point_bub_ms &p2,
                         const int z,
                         bool avoid_creatures )
{
    draw_line( [this, type, avoid_creatures, z]( const point_bub_ms & p ) {
        this->ter_set( tripoint_bub_ms( p, z ), type, avoid_creatures );
    }, p1, p2 );
}

void map::draw_line_furn( const furn_id &type, const point_bub_ms &p1, const point_bub_ms &p2,
                          const int z,
                          bool avoid_creatures )
{
    draw_line( [this, type, avoid_creatures, z]( const point_bub_ms & p ) {
        this->furn_set( tripoint_bub_ms( p, z ), type, avoid_creatures );
    }, p1, p2 );
}

void map::draw_fill_background( const ter_id &type )
{
    // Need to explicitly set caches dirty - set_ter would do it before
    set_transparency_cache_dirty( abs_sub.z() );
    set_seen_cache_dirty( abs_sub.z() );
    set_outside_cache_dirty( abs_sub.z() );
    set_pathfinding_cache_dirty( abs_sub.z() );

    // Fill each submap rather than each tile
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            submap *sm = get_submap_at_grid( point_rel_sm{gridx, gridy} );
            if( sm == nullptr ) {
                debugmsg( "Tried to fill background at (%d,%d) but the submap is not loaded", gridx, gridy );
                continue;
            }
            sm->set_all_ter( type, true );
        }
    }
}

void map::draw_fill_background( ter_id( *f )() )
{
    draw_square_ter( f, point_bub_ms::zero, point_bub_ms( SEEX * my_MAPSIZE - 1,
                     SEEY * my_MAPSIZE - 1 ) );
}
void map::draw_fill_background( const weighted_int_list<ter_id> &f )
{
    draw_square_ter( f, point_bub_ms::zero, point_bub_ms( SEEX * my_MAPSIZE - 1,
                     SEEY * my_MAPSIZE - 1 ) );
}

void map::draw_square_ter( const ter_id &type, const point_bub_ms &p1, const point_bub_ms &p2,
                           const int z,
                           bool avoid_creatures )
{
    draw_square( [this, type, avoid_creatures, z]( const point_bub_ms & p ) {
        this->ter_set( tripoint_bub_ms( p, z ), type, avoid_creatures );
    }, p1, p2 );
}

void map::draw_square_furn( const furn_id &type, const point_bub_ms &p1, const point_bub_ms &p2,
                            const int z,
                            bool avoid_creatures )
{
    draw_square( [this, type, avoid_creatures, z]( const point_bub_ms & p ) {
        this->furn_set( tripoint_bub_ms( p, z ), type, avoid_creatures );
    }, p1, p2 );
}

void map::draw_square_ter( ter_id( *f )(), const point_bub_ms &p1, const point_bub_ms &p2,
                           bool avoid_creatures )
{
    draw_square( [this, f, avoid_creatures]( const point_bub_ms & p ) {
        this->ter_set( p, f(), avoid_creatures );
    }, p1, p2 );
}

void map::draw_square_ter( const weighted_int_list<ter_id> &f, const point_bub_ms &p1,
                           const point_bub_ms &p2, bool avoid_creatures )
{
    draw_square( [this, f, avoid_creatures]( const point_bub_ms & p ) {
        const ter_id *tid = f.pick();
        this->ter_set( p, tid != nullptr ? *tid : ter_str_id::NULL_ID(), avoid_creatures );
    }, p1, p2 );
}

void map::draw_rough_circle_ter( const ter_id &type, const point_bub_ms &p, int rad )
{
    draw_rough_circle( [this, type]( const point_bub_ms & q ) {
        this->ter_set( q, type );
    }, p, rad );
}

void map::draw_rough_circle_furn( const furn_id &type, const point_bub_ms &p, int rad )
{
    draw_rough_circle( [this, type]( const point_bub_ms & q ) {
        if( !is_open_air( tripoint_bub_ms( q, abs_sub.z() ) ) ) {
            this->furn_set( q, type );
        }
    }, p, rad );
}

void map::draw_circle_ter( const ter_id &type, const rl_vec2d &p, double rad )
{
    draw_circle( [this, type]( const point_bub_ms & q ) {
        this->ter_set( q, type );
    }, p, rad );
}

void map::draw_circle_ter( const ter_id &type, const point_bub_ms &p, int rad )
{
    draw_circle( [this, type]( const point_bub_ms & q ) {
        this->ter_set( q, type );
    }, p, rad );
}

void map::draw_circle_furn( const furn_id &type, const point_bub_ms &p, int rad )
{
    draw_circle( [this, type]( const point_bub_ms & q ) {
        this->furn_set( q, type );
    }, p, rad );
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

tripoint_range<tripoint_bub_ms> map::points_in_rectangle( const tripoint_bub_ms &from,
        const tripoint_bub_ms &to ) const
{
    const tripoint_bub_ms min( std::max( 0, std::min( from.x(), to.x() ) ), std::max( 0,
                               std::min( from.y(),
                                         to.y() ) ), std::max( -OVERMAP_DEPTH, std::min( from.z(), to.z() ) ) );
    const tripoint_bub_ms max( std::min( SEEX * my_MAPSIZE - 1, std::max( from.x(), to.x() ) ),
                               std::min( SEEX * my_MAPSIZE - 1, std::max( from.y(), to.y() ) ), std::min( OVERMAP_HEIGHT,
                                       std::max( from.z(), to.z() ) ) );
    return tripoint_range<tripoint_bub_ms>( min, max );
}

tripoint_range<tripoint_bub_ms> map::points_in_radius(
    const tripoint_bub_ms &center, size_t radius, size_t radiusz ) const
{
    const tripoint_bub_ms min(
        std::max<int>( 0, center.x() - radius ), std::max<int>( 0, center.y() - radius ),
        clamp<int>( center.z() - radiusz, -OVERMAP_DEPTH, OVERMAP_HEIGHT ) );
    const tripoint_bub_ms max(
        std::min<int>( SEEX * my_MAPSIZE - 1, center.x() + radius ),
        std::min<int>( SEEX * my_MAPSIZE - 1, center.y() + radius ),
        clamp<int>( center.z() + radiusz, -OVERMAP_DEPTH, OVERMAP_HEIGHT ) );
    return tripoint_range<tripoint_bub_ms>( min, max );
}

tripoint_range<tripoint_bub_ms> map::points_on_zlevel( const int z ) const
{
    if( z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT ) {
        // TODO: need a default constructor that creates an empty range.
        return tripoint_range<tripoint_bub_ms>( tripoint_bub_ms::zero,
                                                tripoint_bub_ms::zero - tripoint_rel_ms::above );
    }
    return tripoint_range<tripoint_bub_ms>(
    { 0, 0, z }, { SEEX * my_MAPSIZE - 1, SEEY * my_MAPSIZE - 1, z } );
}

tripoint_range<tripoint_bub_ms> map::points_on_zlevel() const
{
    return points_on_zlevel( abs_sub.z() );

}

std::list<item_location> map::get_active_items_in_radius(
    const tripoint_bub_ms &center, int radius )
{
    return get_active_items_in_radius( center, radius, special_item_type::none );
}

std::list<item_location> map::get_active_items_in_radius( const tripoint_bub_ms &center, int radius,
        special_item_type type )
{
    std::list<item_location> result;

    const point_bub_ms minp( center.xy() + point( -radius, -radius ) );
    const point_bub_ms maxp( center.xy() + point( radius, radius ) );

    const point_sm_ms ming( std::max( minp.x() / SEEX, 0 ),
                            std::max( minp.y() / SEEY, 0 ) );
    const point_sm_ms maxg( std::min( maxp.x() / SEEX, my_MAPSIZE - 1 ),
                            std::min( maxp.y() / SEEY, my_MAPSIZE - 1 ) );

    for( const tripoint_abs_sm &abs_submap_loc : submaps_with_active_items ) {
        const tripoint_rel_sm submap_loc = ( abs_submap_loc - abs_sub.xy() );
        if( submap_loc.x() < ming.x() || submap_loc.y() < ming.y() ||
            submap_loc.x() > maxg.x() || submap_loc.y() > maxg.y() ) {
            continue;
        }
        const point_bub_ms sm_offset( submap_loc.x() * SEEX, submap_loc.y() * SEEY );

        submap *sm = get_submap_at_grid( submap_loc );
        if( sm == nullptr ) {
            debugmsg( "Tried get active items in radius of (%d,%d,%d) but the submap is not loaded",
                      submap_loc.x(), submap_loc.y(), submap_loc.z() );
            continue;
        }
        std::vector<item_reference> items = type == special_item_type::none ? sm->active_items.get() :
                                            sm->active_items.get_special( type );
        for( const item_reference &elem : items ) {
            const tripoint_bub_ms pos( sm_offset + elem.location, submap_loc.z() );

            if( rl_dist( pos, center ) > radius ) {
                continue;
            }

            if( elem.item_ref ) {
                result.emplace_back( map_cursor( this, pos ), elem.item_ref.get() );
            }
        }
    }

    return result;
}

std::list<tripoint_bub_ms> map::find_furnitures_with_flag_in_radius( const tripoint_bub_ms &center,
        size_t radius,
        const std::string &flag,
        size_t radiusz ) const
{
    std::list<tripoint_bub_ms> furn_locs;
    for( const tripoint_bub_ms &furn_loc : points_in_radius( center, radius, radiusz ) ) {
        if( has_flag_furn( flag, furn_loc ) ) {
            furn_locs.push_back( furn_loc );
        }
    }
    return furn_locs;
}

std::list<tripoint_bub_ms> map::find_furnitures_with_flag_in_radius( const tripoint_bub_ms &center,
        size_t radius,
        const ter_furn_flag flag,
        size_t radiusz ) const
{
    std::list<tripoint_bub_ms> furn_locs;
    for( const tripoint_bub_ms &furn_loc : points_in_radius( center, radius, radiusz ) ) {
        if( has_flag_furn( flag, furn_loc ) ) {
            furn_locs.push_back( furn_loc );
        }
    }
    return furn_locs;
}

std::list<Creature *> map::get_creatures_in_radius( const tripoint_bub_ms &center, size_t radius,
        size_t radiusz ) const
{
    creature_tracker &creatures = get_creature_tracker();
    std::list<Creature *> creature_list;
    for( const tripoint_bub_ms &loc : points_in_radius( center, radius, radiusz ) ) {
        Creature *tmp_critter = creatures.creature_at( loc );
        if( tmp_critter != nullptr ) {
            creature_list.push_back( tmp_critter );
        }

    }
    return creature_list;
}

std::list<Creature *> map::get_creatures_in_radius_circ( const tripoint_bub_ms &center,
        size_t radius, size_t radiusz ) const
{
    creature_tracker &creatures = get_creature_tracker();
    std::list<Creature *> creature_list;
    for( const tripoint_bub_ms &loc : points_in_radius_circ( center, radius, radiusz ) ) {
        Creature *tmp_critter = creatures.creature_at( loc );
        if( tmp_critter != nullptr ) {
            creature_list.push_back( tmp_critter );
        }

    }
    return creature_list;
}

pathfinding_cache::pathfinding_cache()
{
    dirty = true;
}

pathfinding_cache &map::get_pathfinding_cache( int zlev ) const
{
    return *pathfinding_caches[zlev + OVERMAP_DEPTH];
}

void map::set_pathfinding_cache_dirty( const int zlev )
{
    if( inbounds_z( zlev ) ) {
        get_pathfinding_cache( zlev ).dirty = true;
    }
}

void map::set_pathfinding_cache_dirty( const tripoint_bub_ms &p )
{
    if( inbounds( p ) ) {
        get_pathfinding_cache( p.z() ).dirty_points.insert( p.xy() );
    }
}

void map::queue_main_cleanup()
{
    if( this != &reality_bubble() ) {
        _main_requires_cleanup = true;
    }
}

bool map::is_main_cleanup_queued() const
{
    return _main_requires_cleanup;
}

void map::main_cleanup_override( bool over )
{
    _main_cleanup_override = over;
}

const pathfinding_cache &map::get_pathfinding_cache_ref( int zlev ) const
{
    if( !inbounds_z( zlev ) ) {
        debugmsg( "Tried to get pathfinding cache for out of bounds z-level %d", zlev );
        return *pathfinding_caches[ OVERMAP_DEPTH ];
    }
    pathfinding_cache &cache = get_pathfinding_cache( zlev );
    if( cache.dirty || !cache.dirty_points.empty() ) {
        update_pathfinding_cache( zlev );
    }

    return cache;
}

void map::update_pathfinding_cache( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return;
    }
    pathfinding_cache &cache = get_pathfinding_cache( p.z() );
    PathfindingFlags cur_value = PathfindingFlag::Ground;

    const_maptile tile = maptile_at_internal( p );

    const ter_t &terrain = tile.get_ter_t();
    const furn_t &furniture = tile.get_furn_t();
    const field &field = tile.get_field();
    int part;
    const vehicle *veh = veh_at_internal( p, part );

    const int cost = move_cost_internal( furniture, terrain, field, veh, part );

    if( cost > 2 ) {
        cur_value |= PathfindingFlag::Slow;
    } else if( cost <= 0 ) {
        cur_value |= PathfindingFlag::Obstacle;
        if( terrain.has_flag( ter_furn_flag::TFLAG_CLIMBABLE ) ) {
            cur_value |= PathfindingFlag::Climbable;
        }
    }

    if( veh != nullptr ) {
        cur_value |= PathfindingFlag::Vehicle;
    }

    for( const auto &fld : tile.get_field() ) {
        const field_entry &cur = fld.second;
        if( cur.is_dangerous() ) {
            cur_value |= PathfindingFlag::DangerousField;
        }
    }

    if( ( !tile.get_trap_t().is_benign() || !terrain.trap.obj().is_benign() ) &&
        !this->has_vehicle_floor( p ) ) {
        cur_value |= PathfindingFlag::DangerousTrap;
    }

    if( terrain.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) && !this->has_vehicle_floor( p ) ) {
        cur_value |= PathfindingFlag::Air;
    }

    if( terrain.has_flag( ter_furn_flag::TFLAG_GOES_UP ) ) {
        cur_value |= PathfindingFlag::GoesUp;
    }

    if( terrain.has_flag( ter_furn_flag::TFLAG_GOES_DOWN ) ) {
        cur_value |= PathfindingFlag::GoesDown;
    }

    if( terrain.has_flag( ter_furn_flag::TFLAG_RAMP ) ||
        terrain.has_flag( ter_furn_flag::TFLAG_RAMP_UP ) ) {
        cur_value |= PathfindingFlag::GoesUp | PathfindingFlag::RampUp;
    }

    if( terrain.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN ) ) {
        cur_value |= PathfindingFlag::GoesDown | PathfindingFlag::RampDown;
    }

    if( terrain.has_flag( ter_furn_flag::TFLAG_SHARP ) && !this->has_vehicle_floor( p ) ) {
        cur_value |= PathfindingFlag::Sharp;
    }

    if( terrain.has_flag( ter_furn_flag::TFLAG_SMALL_PASSAGE ) ) {
        cur_value |= ( PathfindingFlag::RestrictLarge | PathfindingFlag::RestrictHuge );
    }

    cache.special[p.x()][p.y()] = cur_value;
}

void map::update_pathfinding_cache( int zlev ) const
{
    pathfinding_cache &cache = get_pathfinding_cache( zlev );

    if( cache.dirty ) {
        const int size = getmapsize();
        for( int x = 0; x < size * SEEX; ++x ) {
            for( int y = 0; y < size * SEEX; ++y ) {
                update_pathfinding_cache( { x, y, zlev } );
            }
        }
        cache.dirty = false;
    } else {
        for( const point_bub_ms &p : cache.dirty_points ) {
            update_pathfinding_cache( { p, zlev } );
        }
    }
    cache.dirty_points.clear();
}

void map::clip_to_bounds( tripoint_bub_ms &p ) const
{
    clip_to_bounds( p.x(), p.y(), p.z() );
}

void map::clip_to_bounds( int &x, int &y ) const
{
    if( x < 0 ) {
        x = 0;
    } else if( x >= SEEX * my_MAPSIZE ) {
        x = SEEX * my_MAPSIZE - 1;
    }

    if( y < 0 ) {
        y = 0;
    } else if( y >= SEEY * my_MAPSIZE ) {
        y = SEEY * my_MAPSIZE - 1;
    }
}

void map::clip_to_bounds( int &x, int &y, int &z ) const
{
    clip_to_bounds( x, y );
    if( z < -OVERMAP_DEPTH ) {
        z = -OVERMAP_DEPTH;
    } else if( z > OVERMAP_HEIGHT ) {
        z = OVERMAP_HEIGHT;
    }
}

bool map::is_cornerfloor( const tripoint_bub_ms &p ) const
{
    if( impassable( p ) ) {
        return false;
    }
    std::set<tripoint_bub_ms> impassable_adjacent;
    for( const tripoint_bub_ms &pt : points_in_radius( p, 1 ) ) {
        if( impassable( pt ) ) {
            impassable_adjacent.insert( pt );
        }
    }
    if( !impassable_adjacent.empty() ) {
        //to check if a floor is a corner we first search if any of its diagonal adjacent points is impassable
        std::set< tripoint_bub_ms> diagonals = { p + tripoint::north_east, p + tripoint::north_west, p + tripoint::south_east, p + tripoint::south_west };
        for( const tripoint_bub_ms &impassable_diagonal : diagonals ) {
            if( impassable_adjacent.count( impassable_diagonal ) != 0 ) {
                //for every impassable diagonal found, we check if that diagonal terrain has at least two impassable neighbors that also neighbor point p
                int f = 0;
                for( const tripoint_bub_ms &l : points_in_radius( impassable_diagonal, 1 ) ) {
                    if( impassable_adjacent.count( l ) != 0 ) {
                        f++;
                    }
                    if( f > 2 ) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

int map::calc_max_populated_zlev()
{
    // cache is filled and valid, skip recalculation
    if( max_populated_zlev && max_populated_zlev->first == get_abs_sub() ) {
        return max_populated_zlev->second;
    }

    // We'll assume ground level is populated
    int max_z = 0;

    for( int sz = 1; sz <= OVERMAP_HEIGHT; sz++ ) {
        bool level_done = false;
        for( int sx = 0; sx < my_MAPSIZE; sx++ ) {
            for( int sy = 0; sy < my_MAPSIZE; sy++ ) {
                const submap *sm = get_submap_at_grid( tripoint_rel_sm{ sx, sy, sz } );
                if( sm == nullptr ) {
                    debugmsg( "Tried to calc max populated zlev at (%d,%d,%d) but the submap is not loaded", sx, sy,
                              sz );
                    continue;
                }
                if( !sm->is_uniform() ) {
                    max_z = sz;
                    level_done = true;
                    break;
                }
            }
            if( level_done ) {
                break;
            }
        }
    }

    max_populated_zlev = std::make_pair( get_abs_sub(), max_z );
    return max_z;
}

void map::invalidate_max_populated_zlev( int zlev )
{
    if( max_populated_zlev && max_populated_zlev->second < zlev ) {
        max_populated_zlev->second = zlev;
    }
}

static bool is_haulable( const item &it )
{
    // Liquid cannot be picked up
    return !it.made_of_from_type( phase_id::LIQUID );
}

bool map::has_haulable_items( const tripoint_bub_ms &pos )
{
    const map_stack items = i_at( pos );
    for( const item &it : items ) {
        if( is_haulable( it ) ) {
            return true;
        }
    }
    return false;
}

std::vector<item_location> map::get_haulable_items( const tripoint_bub_ms &pos )
{
    std::vector<item_location> target_items;
    map_stack items = i_at( pos );
    target_items.reserve( items.size() );
    for( item &it : items ) {
        if( is_haulable( it ) ) {
            target_items.emplace_back( map_cursor( this, pos ), &it );
        }
    }
    return target_items;
}

tripoint_bub_ms drawsq_params::center() const
{
    if( view_center.raw() == tripoint::min ) {
        avatar &player_character = get_avatar();
        return player_character.pos_bub() + player_character.view_offset;
    } else {
        return view_center;
    }
}

const ter_str_id &drawsq_params::terrain_override() const
{
    return ter_override;
}

const furn_str_id &drawsq_params::furniture_override() const
{
    return furn_override;
}
