#include "item_stack.h"

#include <algorithm>

#include "debug.h"
#include "enums.h"
#include "item.h"
#include "map.h"
#include "mapdata.h"
#include "output.h"
#include "units.h"

size_t item_stack::size() const
{
    return items->size();
}

bool item_stack::empty() const
{
    return items->empty();
}

void item_stack::clear()
{
    // An acceptable implementation for list; would be bad for vector
    while( !empty() ) {
        erase( begin() );
    }
}

item_stack::iterator item_stack::begin()
{
    return items->begin();
}

item_stack::iterator item_stack::end()
{
    return items->end();
}

item_stack::const_iterator item_stack::begin() const
{
    return items->cbegin();
}

item_stack::const_iterator item_stack::end() const
{
    return items->cend();
}

item_stack::reverse_iterator item_stack::rbegin()
{
    return items->rbegin();
}

item_stack::reverse_iterator item_stack::rend()
{
    return items->rend();
}

item_stack::const_reverse_iterator item_stack::rbegin() const
{
    return items->crbegin();
}

item_stack::const_reverse_iterator item_stack::rend() const
{
    return items->crend();
}

item_stack::iterator item_stack::get_iterator_from_pointer( item *it )
{
    return items->get_iterator_from_pointer( it );
}

item_stack::iterator item_stack::get_iterator_from_index( size_t idx )
{
    return items->get_iterator_from_index( idx );
}

size_t item_stack::get_index_from_iterator( const item_stack::const_iterator &it )
{
    return items->get_index_from_iterator( it );
}

item &item_stack::only_item()
{
    if( empty() ) {
        debugmsg( "Missing item at target location" );
        return null_item_reference();
    } else if( size() > 1 ) {
        debugmsg( "More than one item at target location: %s", enumerate_as_string( begin(),
        end(), []( const item & it ) {
            return it.typeId();
        } ) );
        return null_item_reference();
    }
    return *items->begin();
}

units::volume item_stack::stored_volume() const
{
    units::volume ret = 0_ml;
    for( const item &it : *items ) {
        ret += it.volume();
    }
    return ret;
}

int item_stack::amount_can_fit( const item &it ) const
{
    // Without stacking charges, would we violate the count limit?
    const bool violates_count = size() >= static_cast<size_t>( count_limit() );
    const item *here = it.count_by_charges() ? stacks_with( it ) : nullptr;

    if( violates_count && !here ) {
        return 0;
    }
    // Call max because a tile may have been overfilled to begin with (e.g. #14115)
    const int ret = std::max( 0, it.charges_per_volume( free_volume() ) );
    return it.count_by_charges() ? std::min( ret, it.charges ) : ret;
}

item *item_stack::stacks_with( const item &it )
{
    for( item &here : *items ) {
        if( here.stacks_with( it ) ) {
            return &here;
        }
    }
    return nullptr;
}

const item *item_stack::stacks_with( const item &it ) const
{
    for( const item &here : *items ) {
        if( here.stacks_with( it ) ) {
            return &here;
        }
    }
    return nullptr;
}

std::list<item> item_stack::use_charges( const itype_id &type, int &quantity,
        const tripoint_bub_ms &pos,
        const std::function<bool( const item & )> &filter, bool in_tools )
{
    std::list<item> ret;
    for( auto a = this->begin(); a != this->end() && quantity > 0; ) {
        // Liquid items on the ground could only be used if they're stored on terrain or furniture with LIQUIDCONT flag
        if( ( !a->made_of( phase_id::LIQUID ) ||
              ( a->made_of( phase_id::LIQUID ) &&
                get_map().has_flag( ter_furn_flag::TFLAG_LIQUIDCONT, pos ) ) ) &&
            a->use_charges( type, quantity, ret, pos, filter, nullptr, in_tools ) ) {
            a = this->erase( a );
        } else {
            ++a;
        }
    }
    return ret;
}

units::volume item_stack::free_volume() const
{
    return max_volume() - stored_volume();
}
