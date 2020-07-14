#include "proficiency.h"

#include "generic_factory.h"

namespace
{
generic_factory<proficiency> proficiency_factory( "proficiency" );
} // namespace

template<>
const proficiency &proficiency_id::obj() const
{
    return proficiency_factory.obj( *this );
}

template<>
bool proficiency_id::is_valid() const
{
    return proficiency_factory.is_valid( *this );
}

void proficiency::load_proficiencies( const JsonObject &jo, const std::string &src )
{
    proficiency_factory.load( jo, src );
}

void proficiency::reset()
{
    proficiency_factory.reset();
}

void proficiency::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "name", _name );
}

std::string proficiency::name() const
{
    return _name.translated();
}
