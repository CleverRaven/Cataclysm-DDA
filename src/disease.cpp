#include "disease.h"

#include "debug.h"
#include "flexbuffer_json-inl.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "init.h"
#include "json_error.h"

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

void disease_type::load( const JsonObject &jo, const std::string_view )
{
    disease_type new_disease;

    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "min_duration", min_duration );
    mandatory( jo, was_loaded, "max_duration", max_duration );
    mandatory( jo, was_loaded, "min_intensity", min_intensity );
    mandatory( jo, was_loaded, "max_intensity", max_intensity );
    mandatory( jo, was_loaded, "symptoms", symptoms );

    optional( jo, was_loaded, "health_threshold", health_threshold );
    optional( jo, was_loaded, "affected_bodyparts", affected_bodyparts );

}

void disease_type::reset()
{
    disease_factory.reset();
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

