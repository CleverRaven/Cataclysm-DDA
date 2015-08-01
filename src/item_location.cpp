#include "item_location.h"

#include "enums.h"
#include "debug.h"
#include "game.h"
#include "map.h"
#include "character.h"
#include "player.h"
#include "vehicle.h"
#include <climits>

class item_location::impl
{
protected:
    const item *what;
public:
    virtual ~impl() = default;
    /** Removes the selected item from the game */
    virtual void remove_item() = 0;
    /** Gets the selected item or nullptr */
    virtual item *get_item() = 0;
    /** Gets the position of item in character's inventory or INT_MIN */
    virtual int get_inventory_position()
    {
        return INT_MIN;
    }
};

class item_location::item_is_null : public item_location::impl {
public:
    item_is_null()
    {
        what = nullptr;
    }

    void remove_item() override
    { }

    item *get_item() override
    {
        return nullptr;
    }
};

class item_location::item_on_map : public item_location::impl {
private:
    tripoint location;
public:
    item_on_map( const tripoint &p, const item *which )
    {
        location = p;
        what = nullptr;
        // Ensure the item exists where we want it
        const auto items = g->m.i_at( p );
        for( auto &i : items ) {
            if( &i == which ) {
                what = &i;
                return;
            }
        }

        debugmsg( "Tried to get an item from point %d,%d,%d, but it wasn't there",
                  location.x, location.y, location.z );
    }

    void remove_item() override
    {
        if( what == nullptr ) {
            return;
        }

        g->m.i_rem( location, what );
        what = nullptr;
        // Can't do a sanity check here: i_rem is void
    }

    item *get_item() override
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

        debugmsg( "Tried to get an item from point %d,%d,%d, but it wasn't there",
                  location.x, location.y, location.z );
        return nullptr;
    }
};

class item_location::item_on_person : public item_location::impl {
private:
    Character *who;
public:
    item_on_person( Character &ch, const item *which )
    {
        who = &ch;
        auto the_item = ch.items_with( [which]( const item &it ) {
            return &it == which;
        } );

        if( !the_item.empty() ) {
            what = the_item[0];
        } else {
            debugmsg( "Tried to get an item from a character who doesn't have it" );
            what = nullptr;
        }
    }

    int get_inventory_position() override
    {
        if( what == nullptr ) {
            return INT_MIN;
        }

        // Most of the relevant methods are in Character, just not this one...
        player *pl = dynamic_cast<player*>( who );

        const int inv_pos = pl != nullptr ? pl->get_item_position( what ) : INT_MIN;
        if( inv_pos == INT_MIN ) {
            debugmsg( "Tried to get inventory position of item not on character" );
        }

        return inv_pos;
    }

    void remove_item() override
    {
        if( what == nullptr ) {
            return;
        }

        const auto removed = who->remove_items_with( [this]( const item &it ) {
            return &it == what;
        } );

        what = nullptr;
        if( removed.empty() ) {
            debugmsg( "Tried to remove an item from a character who doesn't have it" );
        }
    }

    item *get_item() override
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
            debugmsg( "Tried to get an item from a character who doesn't have it" );
            return nullptr;
        }
    }
};

class item_location::item_on_vehicle : public item_location::impl {
private:
    vehicle *veh;
    point local_coords;
public:
    item_on_vehicle( vehicle &v, const point &where, const item *which )
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

        debugmsg( "Tried to find an item on vehicle %s, tile %d:%d, but it wasn't there",
                  veh->name.c_str(), local_coords.x, local_coords.y );
        what = nullptr;
    }

    void remove_item() override
    {
        if( what == nullptr ) {
            return;
        }

        const auto parts = veh->parts_at_relative( local_coords.x, local_coords.y );
        for( const int i : parts ) {
            if( veh->remove_item( i, what ) ) {
                what = nullptr;
                return;
            }
        }

        debugmsg( "Tried to remove an item from vehicle %s, tile %d:%d, but it wasn't there",
                  veh->name.c_str(), local_coords.x, local_coords.y );
    }

    item *get_item() override
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

        debugmsg( "Tried to find an item on vehicle %s, tile %d:%d, but it wasn't there",
                  veh->name.c_str(), local_coords.x, local_coords.y );
        what = nullptr;
        return nullptr;
    }
};

item_location::item_location( item_location &&other )
{
    ptr = std::move( other.ptr );
}

item_location::~item_location()
{
}

void item_location::remove_item()
{
    ptr->remove_item();
}

item *item_location::get_item()
{
    return ptr->get_item();
}

int item_location::get_inventory_position()
{
    return ptr->get_inventory_position();
}

item_location::item_location( impl *in )
{
    ptr = std::unique_ptr<impl>( in );
}

item_location item_location::nowhere()
{
    return item_location( new item_is_null() );
}

item_location item_location::on_map( const tripoint &p, const item *which )
{
    return item_location( new item_on_map( p, which ) );
}

item_location item_location::on_character( Character &ch, const item *which )
{
    return item_location( new item_on_person( ch, which ) );
}

item_location item_location::on_vehicle( vehicle &v, const point &where, const item *which )
{
    return item_location( new item_on_vehicle( v, where, which ) );
}
