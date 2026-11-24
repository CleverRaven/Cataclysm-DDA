#include "map.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <ostream>
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
#include "coordinates.h"
#include "craft_reservation.h"
#include "creature.h"
#include "cuboid_rectangle.h"
#include "current_map.h"
#include "cursesdef.h"
#include "debug.h"
#include "drawing_primitives.h"
#include "enums.h"
#include "explosion.h"
#include "field.h"
#include "field_type.h"
#include "flag.h"
#include "game.h"
#include "input.h"
#include "item.h"
#include "item_category.h"
#include "item_factory.h"
#include "item_location.h"
#include "item_stack_use_charges.h"
#include "item_transformation.h"
#include "item_wakeup.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "lightmap.h"
#include "line.h"
#include "map_iterator.h"
#include "map_memory.h"
#include "map_selector.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "messages.h"
#include "npc.h"
#include "omdata.h"
#include "output.h"
#include "overmap_map_data_cache.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "relic.h"
#include "rng.h"
#include "safe_reference.h"
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
#include "visitable.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"
#include "weighted_list.h"

class weather_generator;

static const ammotype ammo_battery( "battery" );

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
static const itype_id itype_mws_monster_corpse_weather_data( "mws_monster_corpse_weather_data" );
static const itype_id itype_mws_portal_storm_weather_data( "mws_portal_storm_weather_data" );
static const itype_id itype_mws_weather_data( "mws_weather_data" );
static const itype_id itype_mws_weather_data_incomplete( "mws_weather_data_incomplete" );

static const ter_str_id ter_t_rootcellar( "t_rootcellar" );

static const trap_str_id tr_portal( "tr_portal" );
static const trap_str_id tr_unfinished_construction( "tr_unfinished_construction" );

#define dbg(x) DebugLog((x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

static cata::colony<item> nulitems;          // Returned when &i_at() is asked for an OOB value

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
