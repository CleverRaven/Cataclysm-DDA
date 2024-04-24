#include "active_item_cache.h"

#include <algorithm>
#include <numeric>
#include <utility>

#include "item.h"
#include "item_pocket.h"
#include "safe_reference.h"

float item_reference::spoil_multiplier()
{
    return std::accumulate(
               pocket_chain.begin(), pocket_chain.end(), 1.0F,
    []( float a, item_pocket const * pk ) {
        return a * pk->spoil_multiplier();
    } );
}

bool active_item_cache::add( item &it, point location, item *parent,
                             std::vector<item_pocket const *> const &pocket_chain )
{
    std::vector<item_pocket const *> pockets = pocket_chain;
    bool ret = false;
    for( item_pocket *pk : it.get_all_standard_pockets() ) {
        pockets.emplace_back( pk );
        for( item *pkit : pk->all_items_top() ) {
            ret |= add( *pkit, location, &it, pockets );
        }
    }
    int speed = it.processing_speed();
    if( speed == item::NO_PROCESSING ) {
        return ret;
    }
    std::unordered_map<item *, safe_reference<item>> &target_index = active_items_index[speed];
    std::list<item_reference> &target_list = active_items[speed];
    if( target_index.empty() && !target_index.empty() ) {
        // If the index has been cleared, rebuild it first.
        for( item_reference &iter : target_list ) {
            // Omit those expired references
            if( iter.item_ref ) {
                target_index.emplace( iter.item_ref.get(), iter.item_ref );
            }
        }
    }
    // If the item is already in the cache for some reason, don't add a second reference
    auto iter = target_index.find( &it );
    if( iter != target_index.end() ) {
        // Ensure it's really what we want, and hasn't expired
        if( iter->second && iter->second.get() == &it ) {
            return true;
        }
    }
    item_reference ref{ location, it.get_safe_reference(), parent, pocket_chain };
    if( it.can_revive() ) {
        special_items[special_item_type::corpse].emplace_back( ref );
    }
    if( it.get_use( "explosion" ) ) {
        special_items[special_item_type::explosive].emplace_back( ref );
    }
    target_list.emplace_back( std::move( ref ) );
    target_index.emplace( &it, it.get_safe_reference() );
    return true;
}

bool active_item_cache::empty() const
{
    return std::all_of( active_items.begin(), active_items.end(), []( const auto & active_queue ) {
        return active_queue.second.empty();
    } );
}

std::vector<item_reference> active_item_cache::get()
{
    std::vector<item_reference> all_cached_items;
    for( std::pair<const int, std::list<item_reference>> &kv : active_items ) {
        for( std::list<item_reference>::iterator it = kv.second.begin(); it != kv.second.end(); ) {
            if( it->item_ref ) {
                all_cached_items.emplace_back( *it );
                ++it;
            } else {
                active_items_index[kv.first].clear();
                it = kv.second.erase( it );
            }
        }
    }
    return all_cached_items;
}

std::vector<item_reference> active_item_cache::get_for_processing()
{
    std::vector<item_reference> items_to_process;
    items_to_process.reserve( std::accumulate( active_items.begin(), active_items.end(), std::size_t{ 0 },
    []( size_t prev, const auto & kv ) {
        return prev + kv.second.size() / static_cast<size_t>( kv.first );
    } ) );
    for( std::pair<const int, std::list<item_reference>> &kv : active_items ) {
        // Rely on iteration logic to make sure the number is sane.
        int num_to_process = kv.second.size() / kv.first;
        std::list<item_reference>::iterator it = kv.second.begin();
        for( ; it != kv.second.end() && num_to_process >= 0; ) {
            if( it->item_ref ) {
                items_to_process.push_back( *it );
                --num_to_process;
                ++it;
            } else {
                // The item has been destroyed, so remove the reference from the cache
                active_items_index[kv.first].clear();
                it = kv.second.erase( it );
            }
        }
        // Rotate the returned items to the end of their list so that the items that weren't
        // returned this time will be first in line on the next call
        kv.second.splice( kv.second.end(), kv.second, kv.second.begin(), it );
    }
    return items_to_process;
}

std::vector<item_reference> active_item_cache::get_special( special_item_type type )
{
    std::vector<item_reference> matching_items;
    std::list<item_reference> &items = special_items[type];
    for( auto it = items.begin(); it != items.end(); ) {
        if( it->item_ref ) {
            matching_items.push_back( *it );
            ++it;
        } else {
            it = items.erase( it );
        }
    }
    return matching_items;
}

void active_item_cache::subtract_locations( const point &delta )
{
    for( std::pair<const int, std::list<item_reference>> &pair : active_items ) {
        for( item_reference &ir : pair.second ) {
            ir.location -= delta;
        }
    }
}

void active_item_cache::rotate_locations( int turns, const point &dim )
{
    for( std::pair<const int, std::list<item_reference>> &pair : active_items ) {
        for( item_reference &ir : pair.second ) {
            ir.location = ir.location.rotate( turns, dim );
        }
    }
}

void active_item_cache::mirror( const point &dim, bool horizontally )
{
    for( std::pair<const int, std::list<item_reference>> &pair : active_items ) {
        for( item_reference &ir : pair.second ) {
            if( horizontally ) {
                ir.location.x = dim.x - 1 - ir.location.x;
            } else {
                ir.location.y = dim.y - 1 - ir.location.y;
            }
        }
    }
}
