#include "emit.h"

#include <map>

#include "debug.h"
#include "generic_factory.h"
#include "json.h"

static std::map<emit_id, emit> emits_all;

/** @relates string_id */
template<>
bool string_id<emit>::is_valid() const
{
    const auto found = emits_all.find( *this );
    if( found == emits_all.end() ) {
        return false;
    }
    return found->second.field() != fd_null;
}

/** @relates string_id */
template<>
const emit &string_id<emit>::obj() const
{
    const auto found = emits_all.find( *this );
    if( found == emits_all.end() ) {
        debugmsg( "Tried to get invalid emission data: %s", c_str() );
        static const emit null_emit{};
        return null_emit;
    }
    return found->second;
}

emit::emit() : id_( emit_id::NULL_ID() ) {}

bool emit::is_null() const
{
    return id_ == emit_id::NULL_ID();
}

void emit::load_emit( JsonObject &jo )
{
    emit et;

    et.id_ = emit_id( jo.get_string( "id" ) );
    et.field_name = jo.get_string( "field" );

    jo.read( "density", et.density_ );
    jo.read( "qty", et.qty_ );
    jo.read( "chance", et.chance_ );

    emits_all[ et.id_ ] = et;
}

const std::map<emit_id, emit> &emit::all()
{
    return emits_all;
}

void emit::check_consistency()
{
    for( auto &e : emits_all ) {
        e.second.field_ = field_from_ident( e.second.field_name );

        if( e.second.density_ > MAX_FIELD_DENSITY || e.second.density_ < 1 ) {
            debugmsg( "emission density of %s out of range", e.second.id_.c_str() );
            e.second.density_ = std::max( std::min( e.second.density_, MAX_FIELD_DENSITY ), 1 );
        }
        if( e.second.qty_ <= 0 ) {
            debugmsg( "emission qty of %s out of range", e.second.id_.c_str() );
        }
        if( e.second.chance_ > 100 || e.second.chance_ <= 0 ) {
            e.second.density_ = std::max( std::min( e.second.chance_, 100 ), 1 );
            debugmsg( "emission chance of %s out of range", e.second.id_.c_str() );
        }
    }
}

void emit::reset()
{
    emits_all.clear();
}
