#include "disease.h"

#include "assign.h"
#include "debug.h"
#include "generic_factory.h"
#include "string_id.h"

namespace
{
generic_factory<disease_type> disease_factory( "disease_type" );
} // namespace

template<>
const disease_type &string_id<disease_type>::obj() const
{
    return disease_factory.obj( *this );
}

template<>
bool string_id<disease_type>::is_valid() const
{
    return disease_factory.is_valid( *this );
}

void disease_type::load_disease_type( const JsonObject &jo, const std::string &src )
{
    disease_factory.load( jo, src );
}

void disease_type::load( const JsonObject &jo, const std::string & )
{
    disease_type new_disease;

    assign( jo, "id", id );
    assign( jo, "min_duration", min_duration );
    assign( jo, "max_duration", max_duration );
    assign( jo, "min_intensity", min_intensity );
    assign( jo, "max_intensity", max_intensity );
    assign( jo, "health_threshold", health_threshold );
    assign( jo, "symptoms", symptoms );

    JsonArray jsr = jo.get_array( "affected_bodyparts" );
    while( jsr.has_more() ) {
        new_disease.affected_bodyparts.emplace( get_body_part_token( jsr.next_string() ) );
    }

}

const std::vector<disease_type> &disease_type::get_all()
{
    return disease_factory.get_all();
}

void disease_type::check_disease_consistency()
{
    for( const disease_type &dis : get_all() ) {
        const efftype_id &symp = dis.symptoms;
        if( !symp.is_valid() ) {
            debugmsg( "disease_type %s has invalid efftype_id %s in symptoms", dis.id.c_str(),  symp.c_str() );
        }
    }
}

