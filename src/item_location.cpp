#include "item_location.h"

#include "enums.h"
#include "game.h"
#include "map.h"
#include "Character.h"
#include "player.h"
#include "vehicle.h"
#include <map>

int item_location::get_inventory_position()
{
    return INT_MIN;
}

item_is_null::item_is_null()
{
    what = nullptr;
}

void item_is_null::remove_item()
{ }

item *item_is_null::get_item()
{
    return nullptr;
}


item_on_map::item_on_map( const tripoint &p, const item *which )
{
    location = p;
    what = nullptr;
    // Ensure the item exists where we want it
    const auto items = g->m.i_at( p );
    for( auto &i : items ) {
        if( &i == which ) {
            what = &i;
        }
    }
}

void item_on_map::remove_item()
{
    if( what == nullptr ) {
        return;
    }

    g->m.i_rem( location, what );
    what = nullptr;
}

item *item_on_map::get_item()
{
    if( what == nullptr ) {
        return nullptr;
    }

    auto items = g->m.i_at( location );
    for( auto &i : items ) {
        if( &i == what ) {
            return &i;
        }
    }

    return nullptr;
}


item_on_person::item_on_person( Character &ch, const item *which )
{
    who = &ch;
    auto the_item = ch.items_with( [which]( const item &it ) {
        return &it == which;
    } );

    if( !the_item.empty() ) {
        what = the_item[0];
    } else {
        what = nullptr;
    }
}

void item_on_person::remove_item()
{
    if( what == nullptr ) {
        return;
    }

    who->remove_items_with( [this]( const item &it ) {
        return &it == what;
    } );

    what = nullptr;
}

item *item_on_person::get_item()
{
    if( what == nullptr ) {
        return nullptr;
    }

    auto items = who->items_with( [this]( const item &it ) {
        return &it == what;
    } );

    if( !items.empty() ) {
        return items[0];
    } else {
        what = nullptr;
        return nullptr;
    }
}

int item_on_person::get_inventory_position()
{
    if( what == nullptr ) {
        return INT_MIN;
    }

    // Most of the relevant methods are in Character, just not this one...
    player *pl = dynamic_cast<player*>( who );
    if( pl == nullptr ) {
        return INT_MIN;
    }

    return pl->get_item_position( what );
}


item_on_vehicle::item_on_vehicle( vehicle &v, const point &where, const item *which )
{
    veh = &v;
    local_coords = where;
    const auto parts = v.parts_at_relative( where.x, where.y );
    for( const int i : parts ) {
        for( item &it : v.get_items( i ) ) {
            if( &it == which ) {
                what = &it;
                return;
            }
        }
    }

    what = nullptr;
}

void item_on_vehicle::remove_item()
{
    if( what == nullptr ) {
        return;
    }

    const auto parts = veh->parts_at_relative( local_coords.x, local_coords.y );
    for( const int i : parts ) {
        for( item &it : veh->get_items( i ) ) {
            if( veh->remove_item( i, what ) ) {
                what = nullptr;
                return;
            }
        }
    }
}

item *item_on_vehicle::get_item()
{
    if( what == nullptr ) {
        return nullptr;
    }

    const auto parts = veh->parts_at_relative( local_coords.x, local_coords.y );
    for( const int i : parts ) {
        for( item &it : veh->get_items( i ) ) {
            if( &it == what ) {
                return &it;
            }
        }
    }

    what = nullptr;
    return nullptr;
}
