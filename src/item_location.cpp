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
#include "translations.h"

#include <climits>

template <typename T>
static unsigned do_pack( const T &sel, const item *obj )
{
    int idx = -1;
    sel.visit_items( [&idx, &obj]( const item * e ) {
        idx++;
        if( e == obj ) {
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } );
    return idx;
}

template <typename T>
static item *do_unpack( const T &sel, int idx )
{
    item *obj = nullptr;
    sel.visit_items( [&idx, &obj]( const item * e ) {
        if( idx-- == 0 ) {
            obj = const_cast<item *>( e );
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } );
    return obj;
}

class item_location::impl
{
        friend item_location;

    public:
        impl( item *what ) : what( what ) {}
        impl( int idx ) : idx( idx ) {}

        virtual ~impl() = default;
        virtual type where() const = 0;
        virtual tripoint position() const = 0;
        virtual std::string describe( const Character * ) const = 0;
        virtual int obtain( Character &ch, long qty ) = 0;
        virtual int obtain_cost( const Character &ch, long qty ) const = 0;
        virtual void remove_item() = 0;

    protected:
        virtual void serialize( JsonOut &js ) const = 0;
        virtual item *unpack( int idx ) const = 0;

        item *target() const {
            if( idx >= 0 ) {
                what = unpack( idx );
                idx = -1;
            }
            if( !what ) {
                debugmsg( "item location does not point to valid item" );
            }
            return what;
        }

    private:
        mutable item *what = nullptr;
        mutable int idx = -1;
};

class item_location::item_on_map : public item_location::impl
{
    private:
        map_cursor cur;

    public:
        item_on_map( const map_cursor &cur, item *which ) : impl( which ), cur( cur ) {}
        item_on_map( const map_cursor &cur, int idx ) : impl( idx ), cur( cur ) {}

        void serialize( JsonOut &js ) const override {
            js.start_object();
            js.member( "type", "map" );
            js.member( "pos", position() );
            js.member( "idx", do_pack( cur, target() ) );
            js.end_object();
        }

        item *unpack( int idx ) const override {
            return do_unpack( cur, idx );
        }

        type where() const override {
            return type::map;
        }

        tripoint position() const override {
            return cur;
        }

        std::string describe( const Character *ch ) const override {
            std::string res = g->m.name( cur );
            if( ch ) {
                res += std::string( " " ) += direction_suffix( ch->pos(), cur );
            }
            return res;
        }

        int obtain( Character &ch, long qty ) override {
            if( !target() ) {
                return INT_MIN;
            }

            ch.moves -= obtain_cost( ch, qty );

            item obj = target()->split( qty );
            if( !obj.is_null() ) {
                return ch.get_item_position( &ch.i_add( obj ) );
            } else {
                int inv = ch.get_item_position( &ch.i_add( *target() ) );
                remove_item();
                return inv;
            }
        }

        int obtain_cost( const Character &ch, long qty ) const override {
            if( !target() ) {
                return 0;
            }

            item obj = *target();
            obj = obj.split( qty );
            if( obj.is_null() ) {
                obj = *target();
            }

            int mv = dynamic_cast<const player *>( &ch )->item_handling_cost( obj );
            mv *= square_dist( ch.pos(), cur ) + 1;
            mv *= MAP_HANDLING_FACTOR;

            //@ todo handle unpacking costs

            return mv;
        }

        void remove_item() override {
            if( target() ) {
                cur.remove_item( *what );
            }
        }
};

class item_location::item_on_person : public item_location::impl
{
    private:
        Character &who;

    public:
        item_on_person( Character &who, item *which ) : impl( which ), who( who ) {}
        item_on_person( Character &who, int idx ) : impl( idx ), who( who ) {}

        void serialize( JsonOut &js ) const override {
            js.start_object();
            js.member( "type", "character" );
            js.member( "idx", do_pack( who, target() ) );
            js.end_object();
        }

        item *unpack( int idx ) const override {
            return do_unpack( who, idx );
        }

        type where() const override {
            return type::character;
        }

        tripoint position() const override {
            return who.pos();
        }

        std::string describe( const Character *ch ) const override {
            if( !target() ) {
                return std::string();
            }

            if( ch == &who ) {
                auto parents = who.parents( *target() );
                if( !parents.empty() && who.is_worn( *parents.back() ) ) {
                    return parents.back()->type_name();

                } else if( who.is_worn( *target() ) ) {
                    return _( "worn" );

                } else {
                    return _( "inventory" );
                }

            } else {
                return who.name;
            }
        }

        int obtain( Character &ch, long qty ) override {
            if( !target() ) {
                return INT_MIN;
            }

            ch.moves -= obtain_cost( ch, qty );

            if( who.is_worn( *target() ) ) {
                target()->on_takeoff( dynamic_cast<player &>( who ) );
            }

            if( &ch.i_at( ch.get_item_position( target() ) ) == target() ) {
                // item already in target characters inventory at base of stack
                return ch.get_item_position( target() );
            }

            item obj = target()->split( qty );
            if( !obj.is_null() ) {
                return ch.get_item_position( &ch.i_add( obj ) );
            } else {
                int inv = ch.get_item_position( &ch.i_add( *target() ) );
                remove_item();
                return inv;
            }
        }


        int obtain_cost( const Character &ch, long qty ) const override {
            if( !target() ) {
                return 0;
            }

            int mv = 0;

            item obj = *target();
            obj = obj.split( qty );
            if( obj.is_null() ) {
                obj = *target();
            }

            auto parents = who.parents( *target() );
            if( !parents.empty() && who.is_worn( *parents.back() ) ) {
                // if outermost parent item is worn status effects (eg. GRABBED) are not applied
                // holsters may also adjust the volume cost factor

                if( parents.back()->can_holster( obj, true ) ) {
                    auto ptr = dynamic_cast<const holster_actor *>
                               ( parents.back()->type->get_use( "holster" )->get_actor_ptr() );
                    mv += dynamic_cast<player &>( who ).item_handling_cost( obj, false, ptr->draw_cost );

                } else if( parents.back()->is_bandolier() ) {
                    auto ptr = dynamic_cast<const bandolier_actor *>
                               ( parents.back()->type->get_use( "bandolier" )->get_actor_ptr() );
                    mv += dynamic_cast<player &>( who ).item_handling_cost( obj, false, ptr->draw_cost );

                } else {
                    mv += dynamic_cast<player &>( who ).item_handling_cost( obj, false );
                }

            } else {
                // it is more expensive to obtain items from the inventory
                // @todo calculate cost for searching in inventory proportional to item volume
                mv += dynamic_cast<player &>( who ).item_handling_cost( obj );
                mv *= INVENTORY_HANDLING_FACTOR;
            }

            if( &ch != &who ) {
                // @todo implement movement cost for transfering item between characters
            }

            return mv;
        }

        void remove_item() override {
            if( target() ) {
                who.remove_item( *what );
            }
        }
};

class item_location::item_on_vehicle : public item_location::impl
{
    private:
        vehicle_cursor cur;

    public:
        item_on_vehicle( const vehicle_cursor &cur, item *which ) : impl( which ), cur( cur ) {}
        item_on_vehicle( const vehicle_cursor &cur, int idx ) : impl( idx ), cur( cur ) {}

        void serialize( JsonOut &js ) const override {
            js.start_object();
            js.member( "type", "vehicle" );
            js.member( "pos", position() );
            js.member( "part", cur.part );
            js.member( "idx", do_pack( cur, target() ) );
            js.end_object();
        }

        item *unpack( int idx ) const override {
            return do_unpack( cur, idx );
        }

        type where() const override {
            return type::vehicle;
        }

        tripoint position() const override {
            return cur.veh.global_part_pos3( cur.part );
        }

        std::string describe( const Character *ch ) const override {
            std::string res = cur.veh.parts[ cur.part ].name();
            if( ch ) {
                res += std::string( " " ) += direction_suffix( ch->pos(), cur.veh.global_part_pos3( cur.part ) );
            }
            return res;
        }

        int obtain( Character &ch, long qty ) override {
            if( !target() ) {
                return INT_MIN;
            }

            ch.moves -= obtain_cost( ch, qty );

            item obj = target()->split( qty );
            if( !obj.is_null() ) {
                return ch.get_item_position( &ch.i_add( obj ) );
            } else {
                int inv = ch.get_item_position( &ch.i_add( *target() ) );
                remove_item();
                return inv;
            }
        }

        int obtain_cost( const Character &ch, long qty ) const override {
            if( !target() ) {
                return 0;
            }

            item obj = *target();
            obj = obj.split( qty );
            if( obj.is_null() ) {
                obj = *target();
            }

            int mv = dynamic_cast<const player *>( &ch )->item_handling_cost( obj );
            mv *= square_dist( ch.pos(), cur.veh.global_part_pos3( cur.part ) ) + 1;
            mv *= VEHICLE_HANDLING_FACTOR;

            //@ todo handle unpacking costs

            return mv;
        }

        void remove_item() override {
            if( !target() ) {
                return;
            }
            if( &cur.veh.parts[ cur.part ].base == target() ) {
                cur.veh.remove_part( cur.part );
            } else {
                cur.remove_item( *target() );
            }
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
    return ( ptr ? ptr->target() : nullptr ) == ( rhs.ptr ? rhs.ptr->target() : nullptr );
}

bool item_location::operator!=( const item_location &rhs ) const
{
    return ( ptr ? ptr->target() : nullptr ) != ( rhs.ptr ? rhs.ptr->target() : nullptr );
}

item_location::operator bool() const
{
    return ptr && ptr->target();
}

item &item_location::operator*()
{
    return *ptr->target();
}

const item &item_location::operator*() const
{
    return *ptr->target();
}

item *item_location::operator->()
{
    return ptr->target();
}

const item *item_location::operator->() const
{
    return ptr->target();
}

void item_location::serialize( JsonOut &js ) const
{
    if( ptr ) {
        ptr->serialize( js );
    } else {
        js.start_object();
        js.member( "type", "null" );
        js.end_object();
    }
}

void item_location::deserialize( JsonIn &js )
{
    auto obj = js.get_object();
    auto type = obj.get_string( "type" );

    int idx = -1;
    tripoint pos = tripoint_min;

    obj.read( "idx", idx );
    obj.read( "pos", pos );

    if( type == "character" ) {
        ptr.reset( new item_on_person( g->u, idx ) );

    } else if( type == "map" ) {
        ptr.reset( new item_on_map( pos, idx ) );

    } else if( type == "vehicle" ) {
        auto *veh = g->m.veh_at( pos );
        auto cur = vehicle_cursor( *veh, obj.get_int( "part" ) );
        if( veh ) {
            ptr.reset( new item_on_vehicle( cur, idx ) );
        }
    }
}

item_location::type item_location::where() const
{
    return ptr ? ptr->where() : type::invalid;
}

tripoint item_location::position() const
{
    return ptr ? ptr->position() : tripoint_min;
}

std::string item_location::describe( const Character *ch ) const
{
    return ptr ? ptr->describe( ch ) : std::string();
}

int item_location::obtain( Character &ch, long qty )
{
    return ptr ? ptr->obtain( ch, qty ) : INT_MIN;
}

int item_location::obtain_cost( const Character &ch, long qty ) const
{
    return ptr ? ptr->obtain_cost( ch, qty ) : 0;
}

void item_location::remove_item()
{
    if( ptr ) {
        ptr->remove_item();
        ptr = nullptr;
    }
}

item *item_location::get_item()
{
    return ptr ? ptr->target() : nullptr;
}

const item *item_location::get_item() const
{
    return ptr ? ptr->target() : nullptr;
}

item_location item_location::clone() const
{
    item_location res;
    res.ptr = ptr;
    return res;
}
