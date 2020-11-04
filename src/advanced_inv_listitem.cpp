#include <algorithm>

#include "advanced_inv_listitem.h"
#include "auto_pickup.h"
#include "cata_assert.h"
#include "item.h"

advanced_inv_listitem::advanced_inv_listitem( item *an_item, int index, int count,
        aim_location area, bool from_vehicle )
    : idx( index )
    , area( area )
    , id( an_item->typeId() )
    , name( an_item->tname( count ) )
    , name_without_prefix( an_item->tname( 1, false ) )
    , autopickup( get_auto_pickup().has_rule( an_item ) )
    , stacks( count )
    , volume( an_item->volume() * stacks )
    , weight( an_item->weight() * stacks )
    , cat( &an_item->get_category_of_contents() )
    , from_vehicle( from_vehicle )
{
    items.push_back( an_item );
    cata_assert( stacks >= 1 );
}

advanced_inv_listitem::advanced_inv_listitem( const std::vector<item *> &list, int index,
        aim_location area, bool from_vehicle ) :
    idx( index ),
    area( area ),
    id( list.front()->typeId() ),
    items( list ),
    name( list.front()->tname( list.size() ) ),
    name_without_prefix( list.front()->tname( 1, false ) ),
    autopickup( get_auto_pickup().has_rule( list.front() ) ),
    stacks( list.size() ),
    volume( list.front()->volume() * stacks ),
    weight( list.front()->weight() * stacks ),
    cat( &list.front()->get_category_of_contents() ),
    from_vehicle( from_vehicle )
{
    cata_assert( stacks >= 1 );
}
