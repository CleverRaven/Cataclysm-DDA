#include "item_location.h"

#include "game_constants.h"
#include "enums.h"
#include "debug.h"
#include "game.h"
#include "map.h"
#include "map_selector.h"
#include "character.h"
#include "player.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "veh_type.h"
#include "itype.h"
#include "iuse_actor.h"
#include <climits>

class item_location::impl
{
        friend item_location;

    public:
        virtual ~impl() = default;
        virtual std::string describe( const Character * ) const = 0;
        virtual int obtain( Character &ch, long qty ) = 0;
        virtual void remove_item() = 0;

    protected:
        item *what = nullptr;
};

class item_location::item_on_map : public item_location::impl
{
    private:
        const map_cursor cur;

    public:
        item_on_map( const map_cursor &cur, item *which ) : cur( cur ) {
            if( !cur.has_item( *which ) ) {
                debugmsg( "Cannot locate item on map at %d,%d,%d", cur.x, cur.y, cur.z );
            } else {
                what = which;
            }
        }

        std::string describe( const Character *ch ) const override {
            std::string res = g->m.name( cur );
            if( ch ) {
                res += std::string( " " ) += direction_suffix( ch->pos(), cur );
            }
            return res;
        }

        int obtain( Character &ch, long qty ) override {
            if( !what ) {
                return INT_MIN;
            }

            int mv = 0;

            //@ todo handle unpacking costs

            mv += dynamic_cast<player *>( &ch )->item_handling_cost( *what ) * ( square_dist( ch.pos(),
                    cur ) + 1 );
            mv *= MAP_HANDLING_FACTOR;

            ch.moves -= mv;


            item obj = what->split( qty );
            if( !obj.is_null() ) {
                return ch.get_item_position( &ch.i_add( obj ) );
            } else {
                int inv = ch.get_item_position( &ch.i_add( *what ) );
                remove_item();
                return inv;
            }
        }

        void remove_item() override {
            if( what == nullptr ) {
                return;
            }
            g->m.i_rem( cur, what );
            what = nullptr;
        }
};

class item_location::item_on_person : public item_location::impl
{
    private:
        Character &who;

    public:
        item_on_person( Character &who, item *which ) : who( who ) {
            if( !who.has_item( *which ) ) {
                debugmsg( "Cannot locate item on character: %s", who.name.c_str() );
            } else {
                what = which;
            }
        }

        std::string describe( const Character *ch ) const override {
            if( !what ) {
                return std::string();
            }

            if( ch == &who ) {
                auto parents = who.parents( *what );
                if( !parents.empty() && who.is_worn( *parents.back() ) ) {
                    return parents.back()->type_name();

                } else if( who.is_worn( *what ) ) {
                    return _( "worn" );

                } else {
                    return _( "inventory" );
                }

            } else {
                return who.name;
            }
        }

        int obtain( Character &ch, long qty ) override {
            if( !what ) {
                return INT_MIN;
            }

            int mv = 0;
            bool was_worn = false;

            item *holster = who.find_parent( *what );
            if( holster && who.is_worn( *holster ) && holster->can_holster( *what, true ) ) {
                // Immediate parent is a worn holster capable of holding this item
                auto ptr = dynamic_cast<const holster_actor *>
                           ( holster->type->get_use( "holster" )->get_actor_ptr() );
                mv += dynamic_cast<player &>( who ).item_handling_cost( *what, false, ptr->draw_cost );
                was_worn = true;
            } else {
                // Unpack the object followed by any nested containers starting with the innermost
                mv += dynamic_cast<player &>( who ).item_handling_cost( *what );
                for( auto obj = who.find_parent( *what ); obj &&
                     who.find_parent( *obj ); obj = who.find_parent( *obj ) ) {
                    mv += dynamic_cast<player &>( who ).item_handling_cost( *obj );
                }
            }

            if( who.is_worn( *what ) ) {
                what->on_takeoff( dynamic_cast<player &>( who ) );
            } else if( !was_worn ) {
                mv *= INVENTORY_HANDLING_FACTOR;
            }

            if( &ch != &who ) {
                // @todo implement movement cost for transfering item between characters
            }

            who.moves -= mv;

            if( &ch.i_at( ch.get_item_position( what ) ) == what ) {
                // item already in target characters inventory at base of stack
                return ch.get_item_position( what );
            }

            item obj = what->split( qty );
            if( !obj.is_null() ) {
                return ch.get_item_position( &ch.i_add( obj ) );
            } else {
                int inv = ch.get_item_position( &ch.i_add( *what ) );
                remove_item();
                return inv;
            }
        }

        void remove_item() override {
            if( what == nullptr ) {
                return;
            }

            const auto removed = who.remove_items_with( [this]( const item & it ) {
                return &it == what;
            } );

            what = nullptr;
            if( removed.empty() ) {
                debugmsg( "Tried to remove an item from a character who doesn't have it" );
            }
        }
};

class item_location::item_on_vehicle : public item_location::impl
{
    private:
        const vehicle_cursor cur;

    public:
        item_on_vehicle( const vehicle_cursor &cur, item *which ) : cur( cur ) {
            if( !cur.has_item( *which ) ) {
                debugmsg( "Cannot locate item on vehicle: %s", cur.veh.name.c_str() );
            } else {
                what = which;
            }
        }

        std::string describe( const Character *ch ) const override {
            std::string res = cur.veh.parts[ cur.part ].info().name;
            if( ch ) {
                res += std::string( " " ) += direction_suffix( ch->pos(), cur.veh.global_part_pos3( cur.part ) );
            }
            return res;
        }

        int obtain( Character &ch, long qty ) override {
            if( !what ) {
                return INT_MIN;
            }

            int mv = 0;

            // @todo handle unpacking costs
            // @todo account for distance

            mv += dynamic_cast<player *>( &ch )->item_handling_cost( *what );
            mv *= VEHICLE_HANDLING_FACTOR;

            ch.moves -= mv;

            item obj = what->split( qty );
            if( !obj.is_null() ) {
                return ch.get_item_position( &ch.i_add( obj ) );
            } else {
                int inv = ch.get_item_position( &ch.i_add( *what ) );
                remove_item();
                return inv;
            }
        }

        void remove_item() override {
            if( what == nullptr ) {
                return;
            }
            cur.veh.remove_item( cur.part, what );
        }
};

// use of std::unique_ptr<impl> forces these definitions within the implementation
item_location::item_location() = default;
item_location::item_location( item_location && ) = default;
item_location &item_location::operator=( item_location && ) = default;
item_location::~item_location() = default;

const item_location item_location::nowhere;

item_location::item_location( const map_cursor &mc, item *which )
    : ptr( new item_on_map( mc, which ) ) {}

item_location::item_location( Character &ch, item *which )
    : ptr( new item_on_person( ch, which ) ) {}

item_location::item_location( const vehicle_cursor &vc, item *which )
    : ptr( new item_on_vehicle( vc, which ) ) {}

bool item_location::operator==( const item_location &rhs ) const
{
    return ( ptr ? ptr->what : nullptr ) == ( rhs.ptr ? rhs.ptr->what : nullptr );
}

bool item_location::operator!=( const item_location &rhs ) const
{
    return ( ptr ? ptr->what : nullptr ) != ( rhs.ptr ? rhs.ptr->what : nullptr );
}

item_location::operator bool() const
{
    return ptr && ptr->what;
}

item &item_location::operator*()
{
    return *ptr->what;
}

const item &item_location::operator*() const
{
    return *ptr->what;
}

item *item_location::operator->()
{
    return ptr->what;
}

const item *item_location::operator->() const
{
    return ptr->what;
}

std::string item_location::describe( const Character *ch ) const
{
    return ptr ? ptr->describe( ch ) : std::string();
}

int item_location::obtain( Character &ch, long qty )
{
    return ptr ? ptr->obtain( ch, qty ) : INT_MIN;
}

void item_location::remove_item()
{
    if( ptr ) {
        ptr->remove_item();
    }
}

item *item_location::get_item()
{
    return ptr ? ptr->what : nullptr;
}

const item *item_location::get_item() const
{
    return ptr ? ptr->what : nullptr;
}
