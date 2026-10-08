#pragma once
#ifndef CATA_SRC_ITEM_STACK_USE_CHARGES_H
#define CATA_SRC_ITEM_STACK_USE_CHARGES_H

#include "item_stack.h"
#include "map.h"

template <typename veh_or_map_cursor>
std::list<item> item_stack::use_charges( const itype_id &type, int &quantity,
        const tripoint_bub_ms &pos, const veh_or_map_cursor &cur,
        const std::function<bool( const item & )> &filter, bool in_tools )
{
    std::list<item> ret;
    for( auto a = this->begin(); a != this->end() && quantity > 0; ) {
        if( craft_reservation::contains_reserved( *a ) ) {
            ++a;
            continue;
        }
        // Liquid items on the ground could only be used if they're stored on terrain or furniture with LIQUIDCONT flag
        if( ( !a->made_of( phase_id::LIQUID ) ||
              ( a->made_of( phase_id::LIQUID ) &&
                get_map().has_flag( ter_furn_flag::TFLAG_LIQUIDCONT, pos ) ) ) &&
            a->use_charges( item_location( cur, &*a ), type, quantity, ret, pos, filter,
                            in_tools ) ) {
            a = this->erase( a );
        } else {
            ++a;
        }
    }
    return ret;
}

#endif // CATA_SRC_ITEM_STACK_USE_CHARGES_H
