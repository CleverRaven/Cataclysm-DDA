#include "morale_types.h"

#include "generic_factory.h"
#include "itype.h"
#include "string_formatter.h"

namespace
{

generic_factory<morale_type_data> morale_data( "morale type" );

} // namespace

template<>
const morale_type_data &morale_type::obj() const
{
    return morale_data.obj( *this );
}

template<>
bool morale_type::is_valid() const
{
    return morale_data.is_valid( *this );
}

void morale_type_data::load_type( const JsonObject &jo, const std::string &src )
{
    morale_data.load( jo, src );
}

void morale_type_data::check_all()
{
    morale_data.check();
}

void morale_type_data::reset()
{
    morale_data.reset();
}

void morale_type_data::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "text", text );

    optional( jo, was_loaded, "permanent", permanent, false );
}

void morale_type_data::check() const
{
}

std::string morale_type_data::describe( const itype *it ) const
{
    if( it ) {
        return string_format( text, it->nname( 1 ) );
    } else {
        // if `msg` contains conversion specification (e.g. %s) but `it` is nullptr,
        // `string_format` will return an error message
        return string_format( text );
    }
}
