#include "active_item_cache.h"
#include "debug.h"

#include <algorithm>

void active_item_cache::remove( std::list<item>::iterator it, point location )
{
    const auto predicate = [&]( const item_reference & active_item ) {
        return location == active_item.location && active_item.item_iterator == it;
    };
    // The iterator is expected to be in this list, as it was added that way in `add`.
    // But the processing_speed may have changed, so if it's not in the expected container,
    // remove it from all containers to ensure no stale iterator remains.
    auto &expected = active_items[it->processing_speed()];
    auto iter = std::find_if( expected.begin(), expected.end(), predicate );
    if( iter != expected.end() ) {
        active_item_set.erase( iter->item_id );
        expected.erase( iter );
        return;
    }
    for( auto &e : active_items ) {
        iter = std::find_if( e.second.begin(), e.second.end(), predicate );
        if( iter != e.second.end() ) {
            active_item_set.erase( iter->item_id );
            e.second.erase( iter );
            return;
        }
    }
    debugmsg( "Critical: The item isn't there!" );
}

void active_item_cache::add( std::list<item>::iterator it, point location )
{
    // Active item processing should always remove the item, then sometimes add it back.
    // If this is changed, active_item_set (or how it's handled in this function) will need to be changed.
    // Just don't use iterators or pointers because they can and will cause undefined behavior.
    assert( active_item_set.count( next_free_id ) == 0 );

    active_items[it->processing_speed()].push_back( item_reference{ location, it, next_free_id } );
    active_item_set.insert( next_free_id );
    ++next_free_id; // Note: overflow is well-defined for unsigned ints.
}

bool active_item_cache::has( std::list<item>::iterator it, point ) const
{
    for( const auto &o : active_items ) {
        const bool found = std::any_of( o.second.begin(), o.second.end(), [&it]( const item_reference &ir ) {
            return it == ir.item_iterator;
        } );
        if( found ) {
            return true;
        }
    }
    return false;
}

bool active_item_cache::has( item_reference const &itm ) const
{
    return active_item_set.count( itm.item_id ) != 0;
}

bool active_item_cache::empty() const
{
    return active_items.empty();
}

// get() only returns the first size() / processing_speed() elements of each list, rounded up.
// It relies on the processing logic to remove and reinsert the items to they
// move to the back of their respective lists (or to new lists).
// Otherwise only the first n items will ever be processed.
std::list<item_reference> active_item_cache::get()
{
    std::list<item_reference> items_to_process;
    for( auto &tuple : active_items ) {
        // Rely on iteration logic to make sure the number is sane.
        int num_to_process = tuple.second.size() / tuple.first;
        for( auto &an_iter : tuple.second ) {
            items_to_process.push_back( an_iter );
            if( --num_to_process < 0 ) {
                break;
            }
        }
    }
    return items_to_process;
}
