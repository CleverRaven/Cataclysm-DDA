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
    // Uniqueness is guaranteed because active item processing always removes the item,
    // processes it, then sometimes add it back.
    assert( active_item_set.count( next_free_id ) == 0 );

    active_items[it->processing_speed()].push_back( item_reference { location, it, next_free_id } );
    active_item_set.insert( next_free_id );
    // Blindly incrementing is safe because the C++ standard says that unsigned ints wrap around.
    ++next_free_id;
}

bool active_item_cache::has( std::list<item>::iterator it, point p ) const
{
    const auto predicate = [&]( const item_reference & ir ) {
        // Comparing iterators from different sequences is undefined, so check the point first
        return ir.location == p && it == ir.item_iterator;
    };
    return std::any_of( active_items.begin(), active_items.end(),
    [&]( const std::pair< int, std::list<item_reference> > &elem ) {
        return std::any_of( elem.second.begin(), elem.second.end(), predicate );
    } );
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
