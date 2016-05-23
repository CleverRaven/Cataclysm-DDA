#include "uid.h"

#include "game.h"
#include "player.h"

unsigned long long uid_factory::next_id()
{
    unsigned long long res = ++val;

    res |= static_cast<unsigned long long>( 0 )            << 55; // high byte is version
    res |= static_cast<unsigned long long>( g->u.getID() ) << 39; // next two bytes are player id

    return res;
}

uid uid_factory::assign()
{
    return uid( this, next_id() );
};

void uid_factory::serialize( JsonOut &js ) const
{
    js.write( val );
}

void uid_factory::deserialize( JsonIn &js )
{
    js.read( val );
}

uid::uid( const uid & )
{
    val = factory ? factory->next_id() : 0;
}

uid &uid::operator=( const uid & )
{
    val = factory ? factory->next_id() : 0;
    return *this;
}

void uid::serialize( JsonOut &js ) const
{
    js.write( val );
}

void uid::deserialize( JsonIn &js )
{
    js.read( val );
}

uid uid::clone() const
{
    return uid( factory, val );
}