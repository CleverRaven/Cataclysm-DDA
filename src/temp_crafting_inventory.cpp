
#include "temp_crafting_inventory.h"

#include <functional>
#include <memory>
#include <optional>
#include <utility>

#include "calendar.h"
#include "character.h"
#include "character_attire.h"
#include "coordinates.h"
#include "enums.h"
#include "flag.h"
#include "iexamine.h"
#include "inventory.h"
#include "itype.h"
#include "map.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapdata.h"
#include "pimpl.h"
#include "pocket_type.h"
#include "point.h"
#include "type_id.h"
#include "value_ptr.h"
#include "vehicle_selector.h"
#include "vpart_position.h"
#include "weather.h"

static const itype_id itype_brick_oven_pseudo( "brick_oven_pseudo" );
static const itype_id itype_butchery_tree_pseudo( "butchery_tree_pseudo" );
static const itype_id itype_fire( "fire" );

temp_crafting_inventory::temp_crafting_inventory( const read_only_visitable &v )
{
    add_all_ref( v );
}

size_t temp_crafting_inventory::size() const
{
    return items.size();
}

void temp_crafting_inventory::clear()
{
    items.clear();
    items_loc.clear();
    temp_owned_items.clear();
}

void temp_crafting_inventory::add_item_ref( item &item )
{
    items.insert( &item );
}

void temp_crafting_inventory::add_item_loc( item_location loc )
{
    items_loc.insert( loc );
}

item &temp_crafting_inventory::add_item_copy( const item &item )
{
    const auto iter = temp_owned_items.insert( item );
    items.insert( &( *iter ) );
    return *iter;
}

void temp_crafting_inventory::add_all_ref( const read_only_visitable &v )
{
    v.visit_items( [&]( item * it, item * ) {
        add_item_ref( *it );
        return VisitResponse::SKIP;
    } );
}

void temp_crafting_inventory::add_all_ref( const Character &guy )
{
    // we are getting item_location here which requires non-const
    Character &non_const_guy = const_cast<Character &>( guy );
    add_item_loc( non_const_guy.get_wielded_item() );
    for( item_location fit : non_const_guy.worn.top_items_loc( non_const_guy ) ) {
        add_item_loc( fit );
    }
    non_const_guy.inv->visit_items(
    [&]( item * node, item * ) {
        add_item_loc( item_location( non_const_guy, node ) );
        return VisitResponse::SKIP;
    }
    );
}

void temp_crafting_inventory::add_all_ref( const map_cursor &cur )
{
    cur.visit_items(
    [ & ]( item * node, item * ) {
        add_item_loc( item_location( cur, node ) );
        return VisitResponse::SKIP;
    }
    );
}

void temp_crafting_inventory::add_all_ref( const vehicle_cursor &cur )
{
    cur.visit_items(
    [ & ]( item * node, item * ) {
        add_item_loc( item_location( cur, node ) );
        return VisitResponse::SKIP;
    }
    );
}

int temp_crafting_inventory::count_item( const itype_id &item_type ) const
{
    int num = 0;
    visit_items(
    [&]( item * node, item * ) {
        num += node->count();
        return VisitResponse::NEXT;
    }
    );
    return num;
}

void temp_crafting_inventory::form_from_zone( map &m, std::unordered_set<tripoint_abs_ms> &zone_pts,
        const Character *pl, bool assign_invlet )
{
    std::vector<tripoint_bub_ms> pts;
    pts.reserve( zone_pts.size() );
    for( const tripoint_abs_ms &elem : zone_pts ) {
        pts.push_back( m.get_bub( elem ) );
    }
    form_from_map( m, pts, pl, assign_invlet );
}

void temp_crafting_inventory::form_from_map( const tripoint_bub_ms &origin, int range,
        const Character *pl,
        bool assign_invlet,
        bool clear_path )
{
    temp_crafting_inventory::form_from_map( &get_map(), origin, range, pl, assign_invlet, clear_path );
}

void temp_crafting_inventory::form_from_map( map *here, const tripoint_bub_ms &origin, int range,
        const Character *pl,
        bool assign_invlet,
        bool clear_path )
{
    // Populate a grid of spots that can be reached
    // If we need a clear path we care about the reachability of points
    if( clear_path ) {
        const std::vector<tripoint_bub_ms> &reachable_pts = here->reachable_flood_steps( origin, range );
        form_from_map( *here, reachable_pts, pl, assign_invlet );
    } else {
        std::vector<tripoint_bub_ms> reachable_pts;
        // Fill reachable points with points_in_radius
        tripoint_range<tripoint_bub_ms> in_radius = here->points_in_radius( origin, range );
        for( const tripoint_bub_ms &p : in_radius ) {
            reachable_pts.emplace_back( p );
        }
        form_from_map( *here, reachable_pts, pl, assign_invlet );
    }
}

static bool tile_has_sufficient_sunlight( const map &m, const tripoint_bub_ms &p )
{
    if( !m.is_outside( p ) || p.z() < 0 ) {
        return false;
    }
    const weather_type_id wtype = current_weather( m.get_abs( p ), calendar::turn );
    return incident_sun_irradiance( wtype, calendar::turn ) > irradiance::high;
}

static int count_charges_in_list( const itype *type, const map_stack &items )
{
    for( const item &candidate : items ) {
        if( candidate.type == type ) {
            return candidate.charges;
        }
    }
    return 0;
}

/**
* Finds the number of charges of the first item that matches ammotype.
*
* @param ammotype   Search target.
* @param items      Stack of items. Search stops at first match.
* @param [out] item_type Matching type.
*
* @return           Number of charges.
* */
static int count_charges_in_list( const ammotype *ammotype, const map_stack &items,
                                  itype_id &item_type )
{
    for( const item &candidate : items ) {
        if( candidate.is_ammo() && candidate.type->ammo->type == *ammotype ) {
            item_type = candidate.typeId();
            return candidate.charges;
        }
    }
    return 0;
}

void temp_crafting_inventory::form_from_map( map &m, std::vector<tripoint_bub_ms> pts,
        const Character *pl,
        bool assign_invlet )
{
    clear();

    const bool bulk_eligible = !assign_invlet;
    std::vector<item> bulk_batch;

    for( const tripoint_bub_ms &p : pts ) {
        const ter_id &t = m.ter( p );
        // a temporary hack while trees are terrain
        if( t->has_flag( ter_furn_flag::TFLAG_TREE ) ) {
            add_item_copy( item( itype_butchery_tree_pseudo ) );
        }
        // Another terrible hack, as terrain can't provide pseudo items, and construction can't do multi-step furniture
        ter_id brick_oven( "t_brick_oven" );
        if( t == brick_oven ) {
            add_item_copy( item( itype_brick_oven_pseudo ) );
        }
        const furn_id &f = m.furn( p );
        const furn_t &fo = f.obj();
        const itype_id &pseudo_id = fo.crafting_pseudo_item;
        if( pseudo_id.is_valid() &&
            pseudo_id->has_flag( flag_NEEDS_SUNLIGHT ) &&
            !tile_has_sufficient_sunlight( m, p ) ) {
            // Not enough sunlight for this tool
        } else {
            item &furn_item = add_item_copy( item( fo.crafting_pseudo_item ) );
            for( const itype *ammo : fo.crafting_ammo_item_types() ) {
                if( furn_item.has_pocket_type( pocket_type::MAGAZINE ) ) {
                    // NOTE: This only works if the pseudo item has a MAGAZINE pocket, not a MAGAZINE_WELL!
                    const bool using_ammotype = fo.has_flag( ter_furn_flag::TFLAG_AMMOTYPE_RELOAD );
                    int amount = 0;
                    itype_id ammo_id = ammo->get_id();
                    // Some furniture can consume more than one item type.
                    // This might be redundant now that we iterate over the ammotypes.
                    if( using_ammotype ) {
                        amount = count_charges_in_list( &ammo->ammo->type, m.i_at( p ), ammo_id );
                    } else {
                        amount = count_charges_in_list( ammo, m.i_at( p ) );
                    }
                    if( amount > 0 ) {
                        item furn_ammo( ammo_id, calendar::turn, amount );
                        furn_item.force_insert_item( furn_ammo, pocket_type::MAGAZINE );
                    }
                }
            }
        }
        if( m.accessible_items( p ) ) {
            map_stack items_here = m.i_at( p );
            for( item &i : items_here ) {
                // if it's *the* player requesting this from from map inventory
                // then don't allow items owned by another faction to be factored into recipe components etc.
                if( pl && !i.is_owned_by( *pl, true ) ) {
                    continue;
                }
                if( !i.made_of( phase_id::LIQUID ) ) {
                    if( i.empty_container() && i.is_watertight_container() ) {
                        const int count = i.count_by_charges() ? i.charges : 1;
                        update_liq_container_count( i.typeId(), count );
                    }
                    if( bulk_eligible ) {
                        bulk_batch.emplace_back( i );
                    } else {
                        add_item_ref( i );
                    }
                }
            }
        }
        // Kludges for now!
        if( m.has_nearby_fire( p, 0 ) ) {
            item &fire = add_item_copy( item( itype_fire ) );
            fire.charges = 1;
        }
        // Handle any water from map sources.
        item water = m.liquid_from( p );
        if( !water.is_null() ) {
            add_item_ref( water );
        }

        // keg-kludge
        if( f->has_examine( iexamine::keg ) ) {
            map_stack liq_contained = m.i_at( p );
            for( item &i : liq_contained ) {
                if( i.made_of( phase_id::LIQUID ) ) {
                    add_item_ref( i );
                }
            }
        }

        // form from vehicle
        if( optional_vpart_position vp = m.veh_at( p ) ) {
            vp->form_inventory( m, *this );
        }
    }

    if( bulk_eligible && !bulk_batch.empty() ) {
        for( item &it : bulk_batch ) {
            add_item_ref( it );
        }
    }

    pts.clear();
}

void temp_crafting_inventory::dump( std::vector<item *> &dest )
{
    visit_items(
    [&]( item * node, item * ) {
        dest.push_back( node );
        return VisitResponse::NEXT;
    }
    );
}

void temp_crafting_inventory::dump( std::vector<const item *> &dest ) const
{
    visit_items(
    [&]( item * node, item * ) {
        dest.push_back( node );
        return VisitResponse::NEXT;
    }
    );
}

void temp_crafting_inventory::update_liq_container_count( const itype_id &id, int count )
{
    max_empty_liq_cont[id] += count;
}

bool temp_crafting_inventory::must_use_liq_container( const itype_id &id, int to_use ) const
{
    const int total = count_item( id );
    auto iter = max_empty_liq_cont.find( id );
    if( iter == max_empty_liq_cont.end() ) {
        return total > 0;
    }
    const int leftover = iter->second - to_use;
    return leftover < 0 && leftover * -1 <= total - iter->second;
}

bool temp_crafting_inventory::must_use_hallu_poison( const itype_id &id, int to_use ) const
{
    const int total = count_item( id );
    int bad = 0;
    visit_items(
    [&]( item * node, item * ) {
        const item &it = *node;
        if( it.typeId() == id && ( it.has_flag( flag_HIDDEN_POISON ) ||
                                   it.has_flag( flag_HIDDEN_HALLU ) ) ) {
            if( it.count_by_charges() ) {
                bad += it.charges;
            } else {
                bad += it.count();
            }
        }
        return VisitResponse::NEXT;
    }
    );
    return total - bad < to_use;
}

void temp_crafting_inventory::replace_liq_container_count( const std::map<itype_id, int> &newmap,
        bool use_max )
{
    for( const auto &it : newmap ) {
        if( !use_max || max_empty_liq_cont.find( it.first ) == max_empty_liq_cont.end() ||
            max_empty_liq_cont.at( it.first ) < it.second ) {
            max_empty_liq_cont[it.first] = it.second;
        }
    }
}
