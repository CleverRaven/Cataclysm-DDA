#include "active_item_cache.h"

#include <algorithm>
#include <utility>

#include "item.h"
#include "safe_reference.h"

void active_item_cache::remove( const item *it )
{
    active_items[it->processing_speed()].remove_if( [it]( const item_reference & active_item ) {
        item *const target = active_item.item_ref.get();
        return !target || target == it;
    } );
}

void active_item_cache::add( item &it, point location )
{
    // If the item is alread in the cache for some reason, don't add a second reference
    std::list<item_reference> &target_list = active_items[it.processing_speed()];
    if( std::find_if( target_list.begin(),
    target_list.end(), [&it]( const item_reference & active_item_ref ) {
    return &it == active_item_ref.item_ref.get();
    } ) != target_list.end() ) {
        return;
    }
    target_list.push_back( item_reference{ location, it.get_safe_reference() } );
}

bool active_item_cache::empty() const
{
    return active_items.empty();
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
                it = kv.second.erase( it );
            }
        }
    }
    return all_cached_items;
}

std::vector<item_reference> active_item_cache::get_for_processing()
{
    std::vector<item_reference> items_to_process;
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
                it = kv.second.erase( it );
            }
        }
        // Rotate the returned items to the end of their list so that the items that weren't
        // returned this time will be first in line on the next call
        kv.second.splice( kv.second.end(), kv.second, kv.second.begin(), it );
    }
    return items_to_process;
}

void active_item_cache::subtract_locations( const point &delta )
{
    for( auto &pair : active_items ) {
        for( item_reference &ir : pair.second ) {
            ir.location -= delta;
        }
    }
}

void active_item_cache::rotate_locations( int turns, const point &dim )
{
    for( auto &pair : active_items ) {
        for( item_reference &ir : pair.second ) {
            ir.location = ir.location.rotate( turns, dim );
        }
    }
}
