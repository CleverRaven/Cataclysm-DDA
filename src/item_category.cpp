#include "item_category.h"

#include "generic_factory.h"

namespace
{
generic_factory<item_category> item_category_factory( "item_category" );
} // namespace

template<>
const item_category &string_id<item_category>::obj() const
{
    return item_category_factory.obj( *this );
}

template<>
bool string_id<item_category>::is_valid() const
{
    return item_category_factory.is_valid( *this );
}

void item_category::load_item_cat( JsonObject &jo, const std::string &src )
{
    item_category_factory.load( jo, src );
}

void item_category::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "name", name_ );
    mandatory( jo, was_loaded, "sort_rank", sort_rank_ );
}

bool item_category::operator<( const item_category &rhs ) const
{
    if( sort_rank_ != rhs.sort_rank_ ) {
        return sort_rank_ < rhs.sort_rank_;
    }
    if( name_.translated_ne( rhs.name_ ) ) {
        return name_.translated_lt( rhs.name_ );
    }
    return id < rhs.id;
}

bool item_category::operator==( const item_category &rhs ) const
{
    return sort_rank_ == rhs.sort_rank_ && name_.translated_eq( rhs.name_ ) && id == rhs.id;
}

bool item_category::operator!=( const item_category &rhs ) const
{
    return !operator==( rhs );
}

std::string item_category::name() const
{
    return name_.translated();
}

item_category_id item_category::get_id() const
{
    return id;
}

int item_category::sort_rank() const
{
    return sort_rank_;
}
