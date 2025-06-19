#include "construction_group.h"

#include "generic_factory.h"

namespace
{

generic_factory<construction_group> all_construction_groups( "construction groups" );

} // namespace

/** @relates string_id */
template<>
bool string_id<construction_group>::is_valid() const
{
    return all_construction_groups.is_valid( *this );
}

/** @relates string_id */
template<>
const construction_group &string_id<construction_group>::obj() const
{
    return all_construction_groups.obj( *this );
}

template<>
int_id<construction_group> string_id<construction_group>::id() const
{
    return all_construction_groups.convert( *this, int_id<construction_group>( -1 ) );
}

template<>
bool int_id<construction_group>::is_valid() const
{
    return all_construction_groups.is_valid( *this );
}

template<>
const construction_group &int_id<construction_group>::obj() const
{
    return all_construction_groups.obj( *this );
}

template<>
const string_id<construction_group> &int_id<construction_group>::id() const
{
    return all_construction_groups.convert( *this );
}

void construction_group::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "name", _name );
}

std::string construction_group::name() const
{
    return _name.translated();
}

size_t construction_group::count()
{
    return all_construction_groups.size();
}

void construction_groups::load( const JsonObject &jo, const std::string &src )
{
    all_construction_groups.load( jo, src );
}

void construction_groups::reset()
{
    all_construction_groups.reset();
}

const std::vector<construction_group> &construction_groups::get_all()
{
    return all_construction_groups.get_all();
}
