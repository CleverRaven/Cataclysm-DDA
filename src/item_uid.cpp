#include "item_uid.h"

#include "game.h"

item_uid::item_uid( const item_uid & )
{
    val = ++g->last_uid;
}

item_uid &item_uid::operator=( const item_uid & )
{
    val = ++g->last_uid;
    return *this;
}

void item_uid::serialize( JsonOut &js ) const
{
    js.write( val );
}

void item_uid::deserialize( JsonIn &js )
{
    js.read( val );
}
