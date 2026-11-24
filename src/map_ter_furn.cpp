#include "map.h" // IWYU pragma: associated

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <list>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "enums.h"
#include "field.h"
#include "flag.h"
#include "item.h"
#include "level_cache.h"
#include "lightmap.h"
#include "line.h"
#include "magic_ter_furn_transform.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "output.h"
#include "point.h"
#include "rng.h"
#include "string_formatter.h"
#include "submap.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "vpart_position.h"

class vehicle;

static const efftype_id effect_crushed( "crushed" );

static const flag_id json_flag_DIGGABLE( "DIGGABLE" );
static const flag_id json_flag_EASY_DECONSTRUCT( "EASY_DECONSTRUCT" );
static const flag_id json_flag_FLAT( "FLAT" );
static const flag_id json_flag_PHASE_BACK( "PHASE_BACK" );
static const flag_id json_flag_PLOWABLE( "PLOWABLE" );

static const furn_str_id furn_f_clear( "f_clear" );
static const furn_str_id furn_f_rubble( "f_rubble" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_dirtmound( "t_dirtmound" );

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

// End of 3D flags

bool map::has_nearby_ter( const tripoint_bub_ms &p, const ter_id &type, int radius ) const
{
    for( const tripoint_bub_ms &pt : points_in_radius( p, radius ) ) {
        if( ter( pt ) == type ) {
            return true;
        }
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
