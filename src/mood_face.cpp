#include "mood_face.h"

#include "generic_factory.h"
#include "json.h"

namespace
{
generic_factory<mood_face> mood_face_factory( "mood_face" );
} // namespace

template<>
const mood_face &mood_face_id::obj() const
{
    return mood_face_factory.obj( *this );
}

/** @relates string_id */
template<>
bool mood_face_id::is_valid() const
{
    return mood_face_factory.is_valid( *this );
}

void mood_face::load_mood_faces( const JsonObject &jo, const std::string &src )
{
    mood_face_factory.load( jo, src );
}

void mood_face::reset()
{
    mood_face_factory.reset();
}

void mood_face::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "values", values_ );
    std::sort( values_.begin(), values_.end(),
    []( const mood_face_value & valueA, const mood_face_value & valueB ) {
        return valueA.value() > valueB.value();
    } );
}

const std::vector<mood_face> &mood_face::get_all()
{
    return mood_face_factory.get_all();
}

void mood_face_value::load( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "value", value_ );
    mandatory( jo, was_loaded, "face", face_ );
}

void mood_face_value::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    load( data );
}
