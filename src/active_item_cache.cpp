#include "active_item_cache.h"

void active_item_cache::remove( std::list<item>::iterator it, point location )
{
    active_items[it->processing_speed()].remove_if( [&]( const item_reference & active_item ) {
        return location == active_item.location && active_item.item_iterator == it;
    } );
    active_item_set.erase( &*it );
}

void active_item_cache::add( std::list<item>::iterator it, point location )
{
    active_items[it->processing_speed()].push_back( item_reference{ location, it, &*it } );
    active_item_set.insert( &*it );
}

bool active_item_cache::has( std::list<item>::iterator it, point ) const
{
    return active_item_set.count( &*it ) != 0;
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
