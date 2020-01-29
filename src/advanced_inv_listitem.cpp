#include "auto_pickup.h"
#include "avatar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "input.h"
#include "item_category.h"
#include "item_search.h"
#include "item_stack.h"
#include "options.h"
#include "output.h"
#include "player.h"
#include "player_activity.h"
#include "string_formatter.h"
#include "color.h"
#include "int_id.h"
#include "item.h"
#include "ret_val.h"
#include "type_id.h"
#include "enums.h"
#include "faction.h"
#include "material.h"
#include "advanced_inv_listitem.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <numeric>

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
    , cat( &an_item->get_category() )
    , from_vehicle( from_vehicle )
{
    items.push_back( an_item );
    assert( stacks >= 1 );
}

advanced_inv_listitem::advanced_inv_listitem( const std::list<item *> &list, int index,
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
    cat( &list.front()->get_category() ),
    from_vehicle( from_vehicle )
{
    assert( stacks >= 1 );
}

advanced_inv_listitem::advanced_inv_listitem()
    : idx()
    , area()
    , id( "null" )
    , autopickup()
    , stacks()
    , cat( nullptr )
{
}

advanced_inv_listitem::advanced_inv_listitem( const item_category *cat )
    : idx()
    , area()
    , id( "null" )
    , name( cat->name() )
    , autopickup()
    , stacks()
    , cat( cat )
{
}

bool advanced_inv_listitem::is_category_header() const
{
    return items.empty() && cat != nullptr;
}

bool advanced_inv_listitem::is_item_entry() const
{
    return !items.empty();
}
