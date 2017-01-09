#include "active_item_cache.h"
#include "debug.h"

#include <algorithm>
#include <cassert>

void active_item_cache::remove( const std::list<item>::iterator it, const point &location )
{
    std::list<item_reference>::const_iterator interior;
    const int key = find( interior, it, location );
    if( key != INT_MIN ) {
        active_item_set.erase( interior->item_id );

        // Workaround for a bug in clang- const_iterators can't be erased
        std::list<item_reference>::iterator clang = active_items[key].begin();
        std::advance( clang, std::distance<std::list<item_reference>::const_iterator>( clang, interior ) );

        active_items[key].erase( clang );
    } else {
        debugmsg( "Critical: The item isn't there!" );
    }
}

int active_item_cache::find( std::list<item_reference>::const_iterator &found,
                             const std::list<item>::iterator target, const point &target_loc ) const
{
    const auto predicate = [&target, &target_loc]( const item_reference & ir ) {
        // Check the point first- comparing iterators from different sequences is undefined
        return target_loc == ir.location && target == ir.item_iterator;
    };

    const auto expected = active_items.find( target->processing_speed() );
    if( expected != active_items.end() ) {
        found = std::find_if( expected->second.begin(), expected->second.end(), predicate );
        if( found != expected->second.end() ) {
            return expected->first;
        }
    }
    // In rare cases, the item_reference will be in the wrong list (see #17364)
    for( auto elem = active_items.begin(); elem != active_items.end(); ++elem ) {
        found = std::find_if( elem->second.begin(), elem->second.end(), predicate );
        if( found != elem->second.end() ) {
            return elem->first;
        }
    }
    return INT_MIN;
}

void active_item_cache::add( const std::list<item>::iterator it, const point &location )
{
    // Uniqueness is guaranteed because active item processing always removes the item,
    // processes it, then sometimes add it back.
    assert( active_item_set.count( next_free_id ) == 0 );

    active_items[it->processing_speed()].push_back( item_reference { location, it, next_free_id } );
    active_item_set.insert( next_free_id );
    // Blindly incrementing is safe because the C++ standard says that unsigned ints wrap around.
    ++next_free_id;
}

bool active_item_cache::has( const std::list<item>::iterator it, const point &p ) const
{
    std::list<item_reference>::const_iterator dummy_found;
    return find( dummy_found, it, p ) != INT_MIN;
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
