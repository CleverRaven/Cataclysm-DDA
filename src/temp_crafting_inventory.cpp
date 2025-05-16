
#include "temp_crafting_inventory.h"

#include <functional>

temp_crafting_inventory::temp_crafting_inventory( const read_only_visitable &v )
{
    add_all_ref( v );
}

size_t temp_crafting_inventory::size() const
{
    return items.size();
}

void temp_crafting_inventory::clear()
{
    items.clear();
    temp_owned_items.clear();
}

void temp_crafting_inventory::add_item_ref( item &item )
{
    items.insert( &item );
}

item &temp_crafting_inventory::add_item_copy( const item &item )
{
    const auto iter = temp_owned_items.insert( item );
    items.insert( &( *iter ) );
    return *iter;
}

void temp_crafting_inventory::add_all_ref( const read_only_visitable &v )
{
    v.visit_items( [&]( item * it, item * ) {
        add_item_ref( *it );
        return VisitResponse::SKIP;
    } );
}
