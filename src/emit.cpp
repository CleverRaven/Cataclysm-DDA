#include "emit.h"

#include <map>
#include <utility>

#include "condition.h"
#include "debug.h"
#include "flexbuffer_json.h"

static std::map<emit_id, emit> emits_all;

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

/** @relates string_id */
template<>
bool string_id<emit>::is_valid() const
{
    const auto found = emits_all.find( *this );
    return found != emits_all.end();
}

emit::emit() : id_( emit_id::NULL_ID() ) {}

bool emit::is_null() const
{
    return id_ == emit_id::NULL_ID();
}

void emit::load_emit( const JsonObject &jo )
{
    emit et;

    et.id_ = emit_id( jo.get_string( "id" ) );
    et.field_ = get_str_or_var( jo.get_member( "field" ), "field" );
    et.intensity_ = get_dbl_or_var( jo, "intensity", false, 1.0 );
    et.qty_ = get_dbl_or_var( jo, "qty", false, 1.0 );
    et.chance_ = get_dbl_or_var( jo, "chance", false, 100.0 );

    emits_all[ et.id_ ] = et;
}

const std::map<emit_id, emit> &emit::all()
{
    return emits_all;
}

void emit::finalize()
{
}

void emit::check_consistency()
{
}

void emit::reset()
{
    emits_all.clear();
}
