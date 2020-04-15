#include "construction_category.h"

#include <set>
#include <string>

#include "generic_factory.h"
#include "int_id.h"
#include "json.h"
#include "string_id.h"

namespace
{

generic_factory<construction_category> all_construction_categories( "construction categories" );

} // namespace

/** @relates string_id */
template<>
bool string_id<construction_category>::is_valid() const
{
    return all_construction_categories.is_valid( *this );
}

/** @relates string_id */
template<>
const construction_category &string_id<construction_category>::obj() const
{
    return all_construction_categories.obj( *this );
}

template<>
int_id<construction_category> string_id<construction_category>::id() const
{
    return all_construction_categories.convert( *this, int_id<construction_category>( -1 ) );
}

template<>
bool int_id<construction_category>::is_valid() const
{
    return all_construction_categories.is_valid( *this );
}

template<>
const construction_category &int_id<construction_category>::obj() const
{
    return all_construction_categories.obj( *this );
}

template<>
const string_id<construction_category> &int_id<construction_category>::id() const
{
    return all_construction_categories.convert( *this );
}

void construction_category::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "name", _name );
}

size_t construction_category::count()
{
    return all_construction_categories.size();
}

void construction_categories::load( const JsonObject &jo, const std::string &src )
{
    all_construction_categories.load( jo, src );
}

void construction_categories::reset()
{
    all_construction_categories.reset();
}

const std::vector<construction_category> &construction_categories::get_all()
{
    return all_construction_categories.get_all();
}
