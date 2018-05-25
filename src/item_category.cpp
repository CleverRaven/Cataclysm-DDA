#include "item_category.h"

bool item_category::operator<( const item_category &rhs ) const
{
    if( sort_rank != rhs.sort_rank ) {
        return sort_rank < rhs.sort_rank;
    }
    if( name != rhs.name ) {
        return name < rhs.name;
    }
    return id < rhs.id;
}

bool item_category::operator==( const item_category &rhs ) const
{
    return sort_rank == rhs.sort_rank && name == rhs.name && id == rhs.id;
}

bool item_category::operator!=( const item_category &rhs ) const
{
    return !operator==( rhs );
}
