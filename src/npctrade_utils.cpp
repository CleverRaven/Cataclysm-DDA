#include "npctrade_utils.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "calendar.h"
#include "clzones.h"
#include "coordinates.h"
#include "debug.h"
#include "game_constants.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "mapdata.h"
#include "npc.h"
#include "npc_class.h"
#include "pathfinding.h"
#include "pocket_type.h"
#include "point.h"
#include "rng.h"
#include "shop_cons_rate.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"

static const ter_str_id ter_t_floor( "t_floor" );

static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );

namespace
{
using consume_cache = std::map<itype_id, int>;
using consume_queue = std::vector<item_location>;
using dest_t = std::vector<tripoint_abs_ms>;

void _consume_item( item_location elem, consume_queue &consumed, consume_cache &cache, npc &guy,
                    time_duration const &elapsed )
{
    if( !elem->is_owned_by( guy ) ) {
        return;
    }
    std::list<item *> const contents = elem->all_items_top( pocket_type::CONTAINER );
    if( contents.empty() ) {
        auto it = cache.find( elem->typeId() );
        if( it == cache.end() ) {
            int const rate = guy.myclass->get_shopkeeper_cons_rates().get_rate( *elem, guy );
            int const rate_init = rate >= 0 ? rate * to_days<int>( elapsed ) : -1;
            it = cache.emplace( elem->typeId(), rate_init ).first;
        }
        if( it->second != 0 ) {
            consumed.push_back( elem );
            it->second--;
        }
    } else {
        for( item *it : contents ) {
            _consume_item( item_location( elem, it ), consumed, cache, guy, elapsed );
        }
    }
}

dest_t _get_shuffled_point_set( std::unordered_set<tripoint_abs_ms> const &set )
{
    dest_t ret;
    std::copy( set.begin(), set.end(), std::back_inserter( ret ) );
    std::shuffle( ret.begin(), ret.end(), rng_get_engine() );
    return ret;
}

// returns true if item wasn't placed
bool _to_map( item const &it, map &here, tripoint_bub_ms const &dpoint_here )
{
    if( here.can_put_items_ter_furn( dpoint_here ) &&
        here.free_volume( dpoint_here ) >= it.volume() ) {
        item const &ret = here.add_item_or_charges( dpoint_here, it, false );
        return ret.is_null();
    }
    return true;
}

bool _to_veh( item const &it, std::optional<vpart_reference> const &vp )
{
    map &here = get_map();

    if( vp->items().free_volume() >= it.volume() ) {
        std::optional<vehicle_stack::iterator> const ret = vp->vehicle().add_item( here, vp->part(), it );
        return !ret.has_value();
    }
    return true;
}

} // namespace

void add_fallback_zone( npc &guy )
{
    zone_manager &zmgr = zone_manager::get_manager();
    tripoint_abs_ms const loc = guy.pos_abs();
    faction_id const &fac_id = guy.get_fac_id();
    map &here = get_map();

    if( zmgr.has_near( zone_type_LOOT_UNSORTED, loc, PICKUP_RANGE, fac_id ) ) {
        return;
    }

    std::vector<tripoint_abs_ms> points;
    for( tripoint_abs_ms const &t : closest_points_first( loc, PICKUP_RANGE ) ) {
        tripoint_bub_ms const t_here = here.get_bub( t );
        const pathfinding_target pf_t = pathfinding_target::point( t_here );
        const furn_id &f = here.furn( t_here );
        if( f != furn_str_id::NULL_ID() &&
            ( f->max_volume > ter_t_floor->max_volume ||
              f->has_flag( ter_furn_flag::TFLAG_CONTAINER ) ) &&
            here.can_put_items_ter_furn( t_here ) &&
            !here.route( guy, pf_t ).empty() ) {
            points.emplace_back( t );
        }
    }

    if( points.empty() ) {
        zmgr.add( fallback_name, zone_type_LOOT_UNSORTED, fac_id, false, true,
                  loc + tripoint::north_west, loc + tripoint::south_east );
    } else {
        for( tripoint_abs_ms const &t : points ) {
            zmgr.add( fallback_name, zone_type_LOOT_UNSORTED, fac_id, false, true, t,
                      t );
        }
    }
    DebugLog( DebugLevel::D_WARNING, DebugClass::D_GAME )
            << "Added fallack loot zones for NPC trader " << guy.name;
}

std::list<item> distribute_items_to_npc_zones( std::list<item> &items, npc &guy )
{
    zone_manager &zmgr = zone_manager::get_manager();
    map &here = get_map();
    tripoint_abs_ms const loc_abs = guy.pos_abs();
    faction_id const &fac_id = guy.get_fac_id();

    std::list<item> leftovers;
    dest_t const fallback = _get_shuffled_point_set(
                                zmgr.get_near( zone_type_LOOT_UNSORTED, loc_abs, PICKUP_RANGE, nullptr, fac_id ) );
    for( item const &it : items ) {
        zone_type_id const zid =
            zmgr.get_near_zone_type_for_item( it, loc_abs, PICKUP_RANGE, fac_id );

        dest_t dest = zid.is_valid() ? _get_shuffled_point_set( zmgr.get_near(
                          zid, loc_abs, PICKUP_RANGE, &it, fac_id ) )
                      : dest_t();
        std::copy( fallback.begin(), fallback.end(), std::back_inserter( dest ) );

        bool leftover = true;
        for( tripoint_abs_ms const &dpoint : dest ) {
            tripoint_bub_ms const dpoint_here = here.get_bub( dpoint );
            std::optional<vpart_reference> const vp = here.veh_at( dpoint_here ).cargo();
            if( vp && vp->vehicle().get_owner() == fac_id ) {
                leftover = _to_veh( it, vp );
            } else {
                leftover = _to_map( it, here, dpoint_here );
            }
            if( !leftover ) {
                break;
            }
        }
        if( leftover ) {
            leftovers.emplace_back( it );
        }
    }

    return leftovers;
}

void consume_items_in_zones( npc &guy, time_duration const &elapsed )
{
    std::unordered_set<tripoint_bub_ms> const src = zone_manager::get_manager().get_point_set_loot(
                guy.pos_abs(), PICKUP_RANGE, guy.get_fac_id() );

    consume_cache cache;
    map &here = get_map();

    for( tripoint_bub_ms const &pt : src ) {
        consume_queue consumed;
        std::list<item_location> stack =
        here.items_with( pt, [&guy]( item const & it ) {
            return it.is_owned_by( guy );
        } );
        for( item_location &elem : stack ) {
            _consume_item( elem, consumed, cache, guy, elapsed );
        }
        for( item_location &it : consumed ) {
            it.remove_item();
        }
    }
}
