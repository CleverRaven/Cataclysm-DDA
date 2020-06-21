#include "active_item_cache.h"

#include <algorithm>
#include <utility>

#include "item.h"
#include "safe_reference.h"

void active_item_cache::remove( const item *it )
{
    active_items.remove_if( [it]( const item_reference & active_item ) {
        item *const target = active_item.item_ref.get();
        return !target || target == it;
    } );
    if( it->can_revive() ) {
        special_items[ special_item_type::corpse ].remove_if( [it]( const item_reference & active_item ) {
            item *const target = active_item.item_ref.get();
            return !target || target == it;
        } );
    }
    if( it->get_use( "explosion" ) ) {
        special_items[ special_item_type::explosive ].remove_if( [it]( const item_reference &
        active_item ) {
            item *const target = active_item.item_ref.get();
            return !target || target == it;
        } );
    }
}

void active_item_cache::add( item &it, point location )
{
    // If the item is alread in the cache for some reason, don't add a second reference
    if( std::find_if( active_items.begin(),
    active_items.end(), [&it]( const item_reference & active_item_ref ) {
    return &it == active_item_ref.item_ref.get();
    } ) != active_items.end() ) {
        return;
    }
    if( it.can_revive() ) {
        special_items[ special_item_type::corpse ].push_back( item_reference{ location, it.get_safe_reference() } );
    }
    if( it.get_use( "explosion" ) ) {
        special_items[ special_item_type::explosive ].push_back( item_reference{ location, it.get_safe_reference() } );
    }
    active_items.push_back( item_reference{ location, it.get_safe_reference() } );
}

bool active_item_cache::empty() const
{
    return active_items.empty();
}

std::vector<item_reference> active_item_cache::get()
{
    std::vector<item_reference> all_cached_items;
    for( std::list<item_reference>::iterator it = active_items.begin(); it != active_items.end(); ) {
        if( it->item_ref ) {
            all_cached_items.emplace_back( *it );
            ++it;
        } else {
            it = active_items.erase( it );
        }
    }
    return all_cached_items;
}


std::vector<item_reference> active_item_cache::get_special( special_item_type type )
{
    std::vector<item_reference> matching_items;
    for( const item_reference &it : special_items[type] ) {
        matching_items.push_back( it );
    }
    return matching_items;
}

void active_item_cache::subtract_locations( const point &delta )
{
    for( item_reference &ir : active_items ) {
        ir.location -= delta;
    }
}

void active_item_cache::rotate_locations( int turns, const point &dim )
{
    for( item_reference &ir : active_items ) {
        ir.location = ir.location.rotate( turns, dim );
    }
}
