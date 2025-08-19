#include "speed_description.h"

#include <algorithm>

#include "flexbuffer_json.h"
#include "generic_factory.h"

namespace
{
generic_factory<speed_description> speed_description_factory( "speed_description" );
} // namespace

template<>
const speed_description &speed_description_id::obj() const
{
    return speed_description_factory.obj( *this );
}

/** @relates string_id */
template<>
bool speed_description_id::is_valid() const
{
    return speed_description_factory.is_valid( *this );
}

void speed_description::load_speed_descriptions( const JsonObject &jo, const std::string &src )
{
    speed_description_factory.load( jo, src );
}

void speed_description::reset()
{
    speed_description_factory.reset();
}

void speed_description::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "values", values_ );
    std::sort( values_.begin(), values_.end(),
    []( const speed_description_value & valueA, const speed_description_value & valueB ) {
        return valueA.value() > valueB.value();
    } );
}

const std::vector<speed_description> &speed_description::get_all()
{
    return speed_description_factory.get_all();
}

void speed_description_value::load( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "value", value_ );
    if( value_ < 0.00 ) {
        jo.throw_error_at( "value", "value outside supported range" );
    }
    if( jo.has_array( "descriptions" ) ) {
        optional( jo, was_loaded, "descriptions", descriptions_ );
    } else if( jo.has_string( "descriptions" ) ) {
        translation description;
        optional( jo, was_loaded, "descriptions", description );
        descriptions_.emplace_back( description );
    }
}

void speed_description_value::deserialize( const JsonObject &data )
{
    load( data );
}
