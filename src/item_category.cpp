#include "item_category.h"

bool item_category::operator<( const item_category &rhs ) const
{
    if( sort_rank_ != rhs.sort_rank_ ) {
        return sort_rank_ < rhs.sort_rank_;
    }
    if( name_ != rhs.name_ ) {
        return name_ < rhs.name_;
    }
    return id_ < rhs.id_;
}

bool item_category::operator==( const item_category &rhs ) const
{
    return sort_rank_ == rhs.sort_rank_ && name_ == rhs.name_ && id_ == rhs.id_;
}

bool item_category::operator!=( const item_category &rhs ) const
{
    return !operator==( rhs );
}

std::string item_category::name() const
{
    return name_;
}

std::string item_category::id() const
{
    return id_;
}

int item_category::sort_rank() const
{
    return sort_rank_;
}
