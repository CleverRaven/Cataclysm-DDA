#include "item_uid.h"

#include <memory>
#include <ostream>

#include "game.h"
#include "json.h"

int64_t generate_next_item_uid()
{
    if( !g ) {
        return 0;
    }
    return g->assign_item_uid();
}

void item_uid::serialize( JsonOut &jsout ) const
{
    jsout.write( value );
}

void item_uid::deserialize( int64_t i )
{
    value = i;
}

std::ostream &operator<<( std::ostream &o, const item_uid &id )
{
    return o << id.get_value();
}
