#include "map.h" // IWYU pragma: associated

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "active_item_cache.h"
#include "avatar.h"
#include "basecamp.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "character_id.h"
#include "colony.h"
#include "coordinates.h"
#include "craft_reservation.h"
#include "creature.h"
#include "debug.h"
#include "enums.h"
#include "field_type.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "item.h"
#include "item_category.h"
#include "item_factory.h"
#include "item_location.h"
#include "item_stack.h"
#include "item_stack_use_charges.h"
#include "item_transformation.h"
#include "item_wakeup.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "level_cache.h"
#include "lightmap.h"
#include "line.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "map_selector.h"
#include "mapdata.h"
#include "messages.h"
#include "npc.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "point.h"
#include "relic.h"
#include "rng.h"
#include "safe_reference.h"
#include "string_formatter.h"
#include "submap.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
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

#define dbg(x) DebugLog((x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

static cata::colony<item> nulitems; // Returned when &i_at() is asked for an OOB value

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

    if( flags & Access_Vehicle ) {
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

    if( flags & Access_Vehicle_Around ) {
        for( const tripoint_bub_ms &pt : reachable_flood_steps( who.pos_bub(), PICKUP_RANGE ) ) {
            if( const optional_vpart_position vp = veh_at( pt ) ) {
                if( std::optional<vpart_reference> cargo = vp.cargo() ) {
                    for( item &it : cargo->items() ) {
                        // We always want to recurse even if the top-level item doesn't pass our filter - the contained items still might!
                        item_location there( vehicle_cursor( vp->vehicle(), cargo->part_index() ), &it );
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
