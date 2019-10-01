#include "item_location.h"

#include <climits>
#include <iosfwd>
#include <vector>

#include "avatar.h"
#include "character.h"
#include "debug.h"
#include "game.h"
#include "game_constants.h"
#include "itype.h"
#include "iuse_actor.h"
#include "json.h"
#include "map.h"
#include "map_selector.h"
#include "player.h"
#include "translations.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"
#include "color.h"
#include "item.h"
#include "iuse.h"
#include "line.h"
#include "optional.h"
#include "visitable.h"
#include "point.h"
#include "safe_reference.h"

template <typename T>
static int find_index( const T &sel, const item *obj )
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
static item *retrieve_index( const T &sel, int idx )
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
    public:
        class nowhere;
        class item_on_map;
        class item_on_person;
        class item_on_vehicle;

        impl() = default;
        impl( item *i ) : what( i->get_safe_reference() ), needs_unpacking( false ) {}
        impl( int idx ) : idx( idx ), needs_unpacking( true ) {}

        virtual ~impl() = default;

        virtual type where() const = 0;
        virtual tripoint position() const = 0;
        virtual std::string describe( const Character * ) const = 0;
        virtual int obtain( Character &, int ) = 0;
        virtual int obtain_cost( const Character &, int ) const = 0;
        virtual void remove_item() = 0;
        virtual void serialize( JsonOut &js ) const = 0;
        virtual item *unpack( int ) const = 0;

        item *target() const {
            ensure_unpacked();
            return what.get();
        }

        bool valid() const {
            ensure_unpacked();
            return !!what;
        }
    private:
        void ensure_unpacked() const {
            if( needs_unpacking ) {
                if( item *i = unpack( idx ) ) {
                    what = i->get_safe_reference();
                } else {
                    debugmsg( "item_location lost its target item during a save/load cycle" );
                }
                needs_unpacking = false;
            }
        }
        mutable safe_reference<item> what;
        mutable int idx = -1;
        mutable bool needs_unpacking;

    public:
        //Flag that controls whether functions like obtain() should stack the obtained item
        //with similar existing items in the inventory or create a new stack for the item
        bool should_stack = true;
};

class item_location::impl::nowhere : public item_location::impl
{
    public:
        type where() const override {
            return type::invalid;
        }

        tripoint position() const override {
            debugmsg( "invalid use of nowhere item_location" );
            return tripoint_min;
        }

        std::string describe( const Character * ) const override {
            debugmsg( "invalid use of nowhere item_location" );
            return "";
        }

        int obtain( Character &, int ) override {
            debugmsg( "invalid use of nowhere item_location" );
            return INT_MIN;
        }

        int obtain_cost( const Character &, int ) const override {
            debugmsg( "invalid use of nowhere item_location" );
            return 0;
        }

        void remove_item() override {
            debugmsg( "invalid use of nowhere item_location" );
        }

        item *unpack( int ) const override {
            return nullptr;
        }

        void serialize( JsonOut &js ) const override {
            js.start_object();
            js.member( "type", "null" );
            js.end_object();
        }
};

class item_location::impl::item_on_map : public item_location::impl
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
            js.member( "idx", find_index( cur, target() ) );
            js.end_object();
        }

        item *unpack( int idx ) const override {
            return retrieve_index( cur, idx );
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

        int obtain( Character &ch, int qty ) override {
            ch.moves -= obtain_cost( ch, qty );

            item obj = target()->split( qty );
            if( !obj.is_null() ) {
                return ch.get_item_position( &ch.i_add( obj, should_stack ) );
            } else {
                int inv = ch.get_item_position( &ch.i_add( *target(), should_stack ) );
                remove_item();
                return inv;
            }
        }

        int obtain_cost( const Character &ch, int qty ) const override {
            if( !target() ) {
                return 0;
            }

            item obj = *target();
            obj = obj.split( qty );
            if( obj.is_null() ) {
                obj = *target();
            }

            int mv = dynamic_cast<const player *>( &ch )->item_handling_cost( obj, true, MAP_HANDLING_PENALTY );
            mv += 100 * rl_dist( ch.pos(), cur );

            // TODO: handle unpacking costs

            return mv;
        }

        void remove_item() override {
            cur.remove_item( *what );
        }
};

class item_location::impl::item_on_person : public item_location::impl
{
    private:
        Character &who;

    public:
        item_on_person( Character &who, item *which ) : impl( which ), who( who ) {}
        item_on_person( Character &who, int idx ) : impl( idx ), who( who ) {}

        void serialize( JsonOut &js ) const override {
            js.start_object();
            js.member( "type", "character" );
            js.member( "idx", find_index( who, target() ) );
            js.end_object();
        }

        item *unpack( int idx ) const override {
            return retrieve_index( who, idx );
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

        int obtain( Character &ch, int qty ) override {
            ch.mod_moves( -obtain_cost( ch, qty ) );

            if( &ch.i_at( ch.get_item_position( target() ) ) == target() ) {
                // item already in target characters inventory at base of stack
                return ch.get_item_position( target() );
            }

            item obj = target()->split( qty );
            if( !obj.is_null() ) {
                return ch.get_item_position( &ch.i_add( obj, should_stack ) );
            } else {
                int inv = ch.get_item_position( &ch.i_add( *target(), should_stack ) );
                remove_item();  // This also takes off the item from whoever wears it.
                return inv;
            }
        }

        int obtain_cost( const Character &ch, int qty ) const override {
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
                // if outermost parent item is worn status effects (e.g. GRABBED) are not applied
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
                    mv += dynamic_cast<player &>( who ).item_handling_cost( obj, false,
                            INVENTORY_HANDLING_PENALTY / 2 );
                }

            } else {
                // it is more expensive to obtain items from the inventory
                // TODO: calculate cost for searching in inventory proportional to item volume
                mv += dynamic_cast<player &>( who ).item_handling_cost( obj, true, INVENTORY_HANDLING_PENALTY );
            }

            if( &ch != &who ) {
                // TODO: implement movement cost for transferring item between characters
            }

            return mv;
        }

        void remove_item() override {
            who.remove_item( *what );
        }
};

class item_location::impl::item_on_vehicle : public item_location::impl
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
            if( target() != &cur.veh.parts[ cur.part ].base ) {
                js.member( "idx", find_index( cur, target() ) );
            }
            js.end_object();
        }

        item *unpack( int idx ) const override {
            return idx >= 0 ? retrieve_index( cur, idx ) : &cur.veh.parts[ cur.part ].base;
        }

        type where() const override {
            return type::vehicle;
        }

        tripoint position() const override {
            return cur.veh.global_part_pos3( cur.part );
        }

        std::string describe( const Character *ch ) const override {
            vpart_position part_pos( cur.veh, cur.part );
            std::string res;
            if( auto label = part_pos.get_label() ) {
                res = colorize( *label, c_light_blue ) + " ";
            }
            if( auto cargo_part = part_pos.part_with_feature( "CARGO", true ) ) {
                res += cargo_part->part().name();
            } else {
                debugmsg( "item in vehicle part without cargo storage" );
            }
            if( ch ) {
                res += " " + direction_suffix( ch->pos(), part_pos.pos() );
            }
            return res;
        }

        int obtain( Character &ch, int qty ) override {
            ch.moves -= obtain_cost( ch, qty );

            item obj = target()->split( qty );
            if( !obj.is_null() ) {
                return ch.get_item_position( &ch.i_add( obj, should_stack ) );
            } else {
                int inv = ch.get_item_position( &ch.i_add( *target(), should_stack ) );
                remove_item();
                return inv;
            }
        }

        int obtain_cost( const Character &ch, int qty ) const override {
            if( !target() ) {
                return 0;
            }

            item obj = *target();
            obj = obj.split( qty );
            if( obj.is_null() ) {
                obj = *target();
            }

            int mv = dynamic_cast<const player *>( &ch )->item_handling_cost( obj, true,
                     VEHICLE_HANDLING_PENALTY );
            mv += 100 * rl_dist( ch.pos(), cur.veh.global_part_pos3( cur.part ) );

            // TODO: handle unpacking costs

            return mv;
        }

        void remove_item() override {
            item &base = cur.veh.parts[ cur.part ].base;
            if( &base == target() ) {
                cur.veh.remove_part( cur.part ); // vehicle_part::base
            } else {
                cur.remove_item( *target() ); // item within CARGO
            }
            cur.veh.invalidate_mass();
        }
};

const item_location item_location::nowhere;

item_location::item_location()
    : ptr( new impl::nowhere() ) {}

item_location::item_location( const map_cursor &mc, item *which )
    : ptr( new impl::item_on_map( mc, which ) ) {}

item_location::item_location( Character &ch, item *which )
    : ptr( new impl::item_on_person( ch, which ) ) {}

item_location::item_location( const vehicle_cursor &vc, item *which )
    : ptr( new impl::item_on_vehicle( vc, which ) ) {}

bool item_location::operator==( const item_location &rhs ) const
{
    return ptr->target() == rhs.ptr->target();
}

bool item_location::operator!=( const item_location &rhs ) const
{
    return ptr->target() != rhs.ptr->target();
}

item_location::operator bool() const
{
    return ptr->valid();
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
    ptr->serialize( js );
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
        ptr.reset( new impl::item_on_person( g->u, idx ) );

    } else if( type == "map" ) {
        ptr.reset( new impl::item_on_map( pos, idx ) );

    } else if( type == "vehicle" ) {
        vehicle *const veh = veh_pointer_or_null( g->m.veh_at( pos ) );
        int part = obj.get_int( "part" );
        if( veh && part >= 0 && part < static_cast<int>( veh->parts.size() ) ) {
            ptr.reset( new impl::item_on_vehicle( vehicle_cursor( *veh, part ), idx ) );
        }
    }
}

item_location::type item_location::where() const
{
    return ptr->where();
}

tripoint item_location::position() const
{
    return ptr->position();
}

std::string item_location::describe( const Character *ch ) const
{
    return ptr->describe( ch );
}

int item_location::obtain( Character &ch, int qty )
{
    if( !ptr->valid() ) {
        debugmsg( "item location does not point to valid item" );
        return INT_MIN;
    }
    return ptr->obtain( ch, qty );
}

int item_location::obtain_cost( const Character &ch, int qty ) const
{
    return ptr->obtain_cost( ch, qty );
}

void item_location::remove_item()
{
    if( !ptr->valid() ) {
        debugmsg( "item location does not point to valid item" );
        return;
    }
    ptr->remove_item();
    ptr.reset( new impl::nowhere() );
}

item *item_location::get_item()
{
    return ptr->target();
}

const item *item_location::get_item() const
{
    return ptr->target();
}

void item_location::set_should_stack( bool should_stack ) const
{
    ptr->should_stack = should_stack;
}
