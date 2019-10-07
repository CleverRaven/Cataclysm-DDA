#include "construction_category.h"

#include <string>
#include <set>

#include "generic_factory.h"

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

void construction_category::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "name", name );
}

size_t construction_category::count()
{
    return all_construction_categories.size();
}

void construction_categories::load( JsonObject &jo, const std::string &src )
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
