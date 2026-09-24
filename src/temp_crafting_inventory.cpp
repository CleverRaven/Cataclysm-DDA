
#include "temp_crafting_inventory.h"

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <utility>

#include "calendar.h"
#include "character.h"
#include "character_attire.h"
#include "coordinates.h"
#include "craft_reservation.h"
#include "enums.h"
#include "flag.h"
#include "iexamine.h"
#include "inventory.h"
#include "itype.h"
#include "map.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapdata.h"
#include "pocket_type.h"
#include "point.h"
#include "type_id.h"
#include "value_ptr.h"
#include "vehicle_selector.h"
#include "vpart_position.h"
#include "weather.h"

class vehicle;

static const flag_id json_flag_PSEUDO( "PSEUDO" );

static const itype_id itype_brick_oven_pseudo( "brick_oven_pseudo" );
static const itype_id itype_butchery_tree_pseudo( "butchery_tree_pseudo" );
static const itype_id itype_fire( "fire" );

namespace
{
int &scope_depth()
{
    static int depth = 0;
    return depth;
}

// Changes when the outermost scope opens and closes, so caches never outlive their scope.
uint64_t &current_epoch()
{
    static uint64_t epoch = 0;
    return epoch;
}
} // namespace

temp_crafting_inventory::query_cache_scope::query_cache_scope()
{
    if( scope_depth()++ == 0 ) {
        ++current_epoch();
    }
}

temp_crafting_inventory::query_cache_scope::~query_cache_scope()
{
    if( --scope_depth() == 0 ) {
        ++current_epoch();
    }
}

item_location temp_crafting_inventory::root_ref::get() const
{
    return raw.valid() ? raw : loc;
}

bool temp_crafting_inventory::prepare_query_cache() const
{
    if( scope_depth() == 0 ) {
        return false;
    }
    if( cache_epoch != current_epoch() ) {
        drop_caches();
        cache_epoch = current_epoch();
    }
    return true;
}

const temp_crafting_inventory::type_index *temp_crafting_inventory::cached_index() const
{
    if( !prepare_query_cache() ) {
        return nullptr;
    }
    if( !index ) {
        type_index &idx = index.emplace();
        const auto add_root = [&idx]( const root_ref & ref ) {
            const item_location root = ref.get();
            if( !root.valid() ) {
                return;
            }
            bool holds_ups = false;
            root.visit_items( [&]( const item_location & node ) {
                std::vector<root_ref> &roots = idx.by_type[node->typeId()];
                if( roots.empty() || !( roots.back() == ref ) ) {
                    roots.push_back( ref );
                }
                holds_ups = holds_ups || node->has_flag( flag_IS_UPS );
                return VisitResponse::NEXT;
            } );
            if( holds_ups ) {
                idx.ups.push_back( ref );
            }
        };
        for( const item_location &it : item_copies ) {
            add_root( { it, item_location()} );
        }
        for( const item_location &loc : items_loc ) {
            add_root( { item_location(), loc} );
        }
    }
    return &*index;
}

void temp_crafting_inventory::drop_caches() const
{
    index.reset();
    provider_quality_answers.clear();
}

std::optional<bool> temp_crafting_inventory::recall_provider_quality(
    const provider_quality_key &key ) const
{
    if( !prepare_query_cache() ) {
        return std::nullopt;
    }
    const auto found = provider_quality_answers.find( key );
    if( found == provider_quality_answers.end() ) {
        return std::nullopt;
    }
    return found->second;
}

void temp_crafting_inventory::remember_provider_quality( const provider_quality_key &key,
        bool answer ) const
{
    if( prepare_query_cache() ) {
        provider_quality_answers[key] = answer;
    }
}

temp_crafting_inventory::temp_crafting_inventory( const temp_crafting_inventory &v )
{
    for( const item_location &it : v.item_copies ) {
        add_item_copy( *it );
    }
    items_loc = v.items_loc;
    max_empty_liq_cont = v.max_empty_liq_cont;
}

temp_crafting_inventory &temp_crafting_inventory::operator=( const temp_crafting_inventory &v )
{
    if( this == &v ) {
        return *this;
    }
    clear();
    for( const item_location &it : v.item_copies ) {
        add_item_copy( *it );
    }
    items_loc = v.items_loc;
    max_empty_liq_cont = v.max_empty_liq_cont;
    return *this;
}

size_t temp_crafting_inventory::size() const
{
    return items_loc.size() + item_copies.size();
}

void temp_crafting_inventory::clear()
{
    drop_caches();
    items_loc.clear();
    item_copies.clear();
    temp_owned_items.clear();
    max_empty_liq_cont.clear();
    pseudo_items.clear();
}

void temp_crafting_inventory::add_item_loc( const item_location &loc )
{
    drop_caches();
    items_loc.insert( loc );
}

item &temp_crafting_inventory::add_item_copy( const item &item )
{
    drop_caches();
    const auto iter = temp_owned_items.insert( item );
    item_copies.insert( item_location( *this, &*iter ) );
    return *iter;
}

item &temp_crafting_inventory::add_pseudo_item( const itype_id &id )
{
    auto iter = pseudo_items.find( id );
    if( iter == pseudo_items.end() ) {
        item it( id );
        it.set_flag( json_flag_PSEUDO );
        item &it_copy = add_item_copy( it );
        pseudo_items[it.typeId()] = &it_copy;
        return it_copy;
    } else {
        return *iter->second;
    }
}

item &temp_crafting_inventory::add_pseudo_item( const item &it )
{
    auto iter = pseudo_items.find( it.typeId() );
    if( iter == pseudo_items.end() ) {
        item &it_copy = add_item_copy( it );
        pseudo_items[it.typeId()] = &it_copy;
        if( it.has_ammo() && !it.loaded_ammo().is_null() ) {
            if( it.uses_magazine() ) {
                it_copy.force_insert_item( it.loaded_ammo(), pocket_type::MAGAZINE_WELL );
            } else {
                it_copy.force_insert_item( it.loaded_ammo(), pocket_type::MAGAZINE );
            }
        }
        return it_copy;
    } else {
        return *iter->second;
    }
}

void temp_crafting_inventory::add_all_ref( const read_only_visitable &v )
{
    v.visit_items( [&]( item_location node ) {
        add_item_loc( node );
        return VisitResponse::SKIP;
    } );
}

void temp_crafting_inventory::add_all_ref( const Character &guy )
{
    // we are getting item_location here which requires non-const
    Character &non_const_guy = const_cast<Character &>( guy );
    if( guy.is_armed() ) {
        add_item_loc( non_const_guy.get_wielded_item() );
    }
    for( const item_location &fit : non_const_guy.worn.top_items_loc( non_const_guy ) ) {
        add_item_loc( fit );
    }
}

void temp_crafting_inventory::add_all_ref( const map_cursor &cur )
{
    cur.visit_items(
    [ & ]( item_location node ) {
        add_item_loc( node );
        return VisitResponse::SKIP;
    }
    );
}

void temp_crafting_inventory::add_all_ref( const vehicle_cursor &cur )
{
    cur.visit_items(
    [ & ]( item_location node ) {
        add_item_loc( node );
        return VisitResponse::SKIP;
    }
    );
}

void temp_crafting_inventory::visit_roots_holding( const itype_id &id,
        const std::function<VisitResponse( item_location )> &func ) const
{
    const type_index *idx = cached_index();
    if( idx == nullptr ) {
        visit_items( func );
        return;
    }
    const auto found = idx->by_type.find( id );
    if( found == idx->by_type.end() ) {
        return;
    }
    for( const root_ref &ref : found->second ) {
        const item_location root = ref.get();
        if( !root.valid() && root.visit_items( func ) == VisitResponse::ABORT ) {
            return;
        }
    }
}

int temp_crafting_inventory::count_item( const itype_id &item_type ) const
{
    int num = 0;
    visit_roots_holding( item_type, [&]( const item_location & node ) {
        if( node->typeId() == item_type ) {
            num += node->count();
        }
        return VisitResponse::NEXT;
    } );
    return num;
}

void temp_crafting_inventory::form_from_zone( map &m, std::unordered_set<tripoint_abs_ms> &zone_pts,
        const Character *pl )
{
    std::vector<tripoint_bub_ms> pts;
    pts.reserve( zone_pts.size() );
    for( const tripoint_abs_ms &elem : zone_pts ) {
        pts.push_back( m.get_bub( elem ) );
    }
    form_from_map( m, pts, pl );
}

void temp_crafting_inventory::form_from_map( const tripoint_bub_ms &origin, int range,
        const Character *pl,
        bool clear_path )
{
    temp_crafting_inventory::form_from_map( &get_map(), origin, range, pl, clear_path );
}

void temp_crafting_inventory::form_from_map( map *here, const tripoint_bub_ms &origin, int range,
        const Character *pl,
        bool clear_path )
{
    // Populate a grid of spots that can be reached
    // If we need a clear path we care about the reachability of points
    if( clear_path ) {
        const std::vector<tripoint_bub_ms> &reachable_pts = here->reachable_flood_steps( origin, range );
        form_from_map( *here, reachable_pts, pl );
    } else {
        std::vector<tripoint_bub_ms> reachable_pts;
        // Fill reachable points with points_in_radius
        tripoint_range<tripoint_bub_ms> in_radius = here->points_in_radius( origin, range );
        for( const tripoint_bub_ms &p : in_radius ) {
            reachable_pts.emplace_back( p );
        }
        form_from_map( *here, reachable_pts, pl );
    }
}

bool tile_has_sufficient_sunlight( const map &m, const tripoint_bub_ms &p )
{
    if( !m.is_outside( p ) || p.z() < 0 ) {
        return false;
    }
    const weather_type_id wtype = current_weather( m.get_abs( p ), calendar::turn );
    return incident_sun_irradiance( wtype, calendar::turn ) > irradiance::high;
}

void temp_crafting_inventory::form_from_map( map &m, std::vector<tripoint_bub_ms> pts,
        const Character *pl )
{
    clear();

    std::set<vehicle *> vehicles_found;
    for( const tripoint_bub_ms &p : pts ) {
        // only this tile's pseudo tools, not its items, fire or cargo. not the union
        // with craft sites, or a craft would hide the bench it stands on
        const bool pseudo_tools_reserved =
            get_craft_reservations().provider_tile_reserved( m.get_abs( p ) );
        const ter_id &t = m.ter( p );
        // a temporary hack while trees are terrain
        if( !pseudo_tools_reserved && t->has_flag( ter_furn_flag::TFLAG_TREE ) ) {
            add_pseudo_item( itype_butchery_tree_pseudo );
        }
        // Another terrible hack, as terrain can't provide pseudo items, and construction can't do multi-step furniture
        ter_id brick_oven( "t_brick_oven" );
        if( !pseudo_tools_reserved && t == brick_oven ) {
            add_pseudo_item( itype_brick_oven_pseudo );
        }
        const furn_id &f = m.furn( p );
        const furn_t &fo = f.obj();
        const itype_id &pseudo_id = fo.crafting_pseudo_item;
        if( !pseudo_tools_reserved && pseudo_id.is_valid() ) {
            if(
                pseudo_id->has_flag( flag_NEEDS_SUNLIGHT ) &&
                !tile_has_sufficient_sunlight( m, p ) ) {
                // Not enough sunlight for this tool
            } else {
                item &furn_item = add_pseudo_item( fo.crafting_pseudo_item );
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
        }
        if( m.accessible_items( p ) ) {
            map_stack items_here = m.i_at( p );
            for( item &i : items_here ) {
                // if it's *the* player requesting this from from map inventory
                // then don't allow items owned by another faction to be factored into recipe components etc.
                if( pl && !i.is_owned_by( *pl, true ) ) {
                    continue;
                }
                // crafting query walks the whole tree under each entry, so a container holding
                // a reserved provider is hidden with it. before the liquid count too
                if( craft_reservation::contains_reserved( item_location( map_cursor( p ), &i ) ) ) {
                    continue;
                }
                if( !i.made_of( phase_id::LIQUID ) ) {
                    if( i.empty_container() && i.is_watertight_container() ) {
                        const int count = i.count_by_charges() ? i.charges : 1;
                        update_liq_container_count( i.typeId(), count );
                    }
                    add_item_loc( item_location( map_cursor( p ), &i ) );
                }
            }
        }
        // Kludges for now!
        if( m.has_nearby_fire( p, 0 ) ) {
            item &fire = add_pseudo_item( itype_fire );
            fire.charges = 1;
        }
        // Handle any water from map sources.
        item water = m.liquid_from( p );
        if( !water.is_null() ) {
            add_pseudo_item( water );
        }

        // keg-kludge
        if( f->has_examine( iexamine::keg ) ) {
            map_stack liq_contained = m.i_at( p );
            for( item &i : liq_contained ) {
                if( i.made_of( phase_id::LIQUID ) ) {
                    add_item_loc( item_location( map_cursor( p ),  &i ) );
                }
            }
        }

        // form from vehicle
        if( optional_vpart_position vp = m.veh_at( p ) ) {
            vp->form_inventory( m, *this, vehicles_found );
        }
    }

    pts.clear();
}

void temp_crafting_inventory::dump( std::vector<item *> &dest ) const
{
    visit_items(
    [&]( item_location node ) {
        dest.push_back( node.get_item() );
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
    visit_roots_holding( id, [&]( const item_location & node ) {
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
    } );
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
