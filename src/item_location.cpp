#include "item_location.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <iterator>
#include <list>
#include <string>
#include <vector>

#include "character.h"
#include "character_id.h"
#include "color.h"
#include "debug.h"
#include "game.h"
#include "game_constants.h"
#include "item.h"
#include "item_contents.h"
#include "item_pocket.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "map_selector.h"
#include "optional.h"
#include "point.h"
#include "ret_val.h"
#include "safe_reference.h"
#include "string_formatter.h"
#include "translations.h"
#include "units.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "visitable.h"
#include "vpart_position.h"

template <typename T>
static int find_index( const T &sel, const item *obj )
{
    int idx = -1;
    sel.visit_items( [&idx, &obj]( const item * e, item * ) {
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
    sel.visit_items( [&idx, &obj]( const item * e, item * ) {
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
        class item_in_container;
        class item_on_map;
        class item_on_person;
        class item_on_vehicle;
        class nowhere;

        impl() = default;
        explicit impl( item *i ) : what( i->get_safe_reference() ) {}
        explicit impl( int idx ) : idx( idx ), needs_unpacking( true ) {}

        virtual ~impl() = default;

        virtual type where() const = 0;
        virtual type where_recursive() const {
            return where();
        }
        virtual item_location parent_item() const {
            return item_location();
        }
        virtual tripoint position() const = 0;
        virtual std::string describe( const Character * ) const = 0;
        virtual item_location obtain( Character &, int ) = 0;
        virtual units::volume volume_capacity() const = 0;
        virtual units::mass weight_capacity() const = 0;
        virtual int obtain_cost( const Character &, int ) const = 0;
        virtual void remove_item() = 0;
        virtual void on_contents_changed() = 0;
        virtual void serialize( JsonOut &js ) const = 0;
        virtual item *unpack( int ) const = 0;

        item *target() const {
            ensure_unpacked();
            return what.get();
        }

        virtual bool valid() const {
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
        mutable bool needs_unpacking = false;

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

        item_location obtain( Character &, int ) override {
            debugmsg( "invalid use of nowhere item_location" );
            return item_location();
        }

        int obtain_cost( const Character &, int ) const override {
            debugmsg( "invalid use of nowhere item_location" );
            return 0;
        }

        void remove_item() override {
            debugmsg( "invalid use of nowhere item_location" );
        }

        void on_contents_changed() override {
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

        units::volume volume_capacity() const override {
            return units::volume();
        }

        units::mass weight_capacity() const override {
            return units::mass();
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
            return cur.pos();
        }

        std::string describe( const Character *ch ) const override {
            std::string res = get_map().name( cur.pos() );
            if( ch ) {
                res += std::string( " " ) += direction_suffix( ch->pos(), cur.pos() );
            }
            return res;
        }

        item_location obtain( Character &ch, int qty ) override {
            ch.moves -= obtain_cost( ch, qty );

            on_contents_changed();
            item obj = target()->split( qty );
            const auto get_local_location = []( Character & ch, item * it ) {
                if( ch.has_item( *it ) ) {
                    return item_location( ch, it );
                } else {
                    return item_location{};
                }
            };
            if( !obj.is_null() ) {
                return get_local_location( ch, &ch.i_add( obj, should_stack ) );
            } else {
                item *inv = &ch.i_add( *target(), should_stack );
                remove_item();
                return get_local_location( ch, inv );
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

            int mv = ch.item_handling_cost( obj, true, MAP_HANDLING_PENALTY );
            mv += 100 * rl_dist( ch.pos(), cur.pos() );

            // TODO: handle unpacking costs

            return mv;
        }

        void remove_item() override {
            on_contents_changed();
            cur.remove_item( *what );
        }

        void on_contents_changed() override {
            target()->on_contents_changed();
        }

        units::volume volume_capacity() const override {
            map_stack stack = get_map().i_at( cur.pos() );
            return stack.free_volume();
        }

        units::mass weight_capacity() const override {
            return units::mass_max;
        }
};

class item_location::impl::item_on_person : public item_location::impl
{
    private:
        character_id who_id;
        mutable Character *who;

        bool ensure_who_unpacked() const {
            if( !who ) {
                who = g->critter_by_id<Character>( who_id );
                if( !who ) {
                    // If we failed to find it throw a debug message cause we're probably going to crash soon
                    debugmsg( "Failed to find item_location owner with character_id %d", who_id.get_value() );
                    return false;
                }
            }
            return true;
        }

    public:
        item_on_person( Character &who, item *which ) : impl( which ) {
            who_id = who.getID();
            this->who = &who;
        }

        item_on_person( character_id who_id, int idx ) : impl( idx ), who_id( who_id ), who( nullptr ) {}

        void serialize( JsonOut &js ) const override {
            if( !ensure_who_unpacked() ) {
                // Write an invalid item_location to avoid invalid json
                js.start_object();
                js.member( "type", "null" );
                js.end_object();
                return;
            }
            js.start_object();
            js.member( "type", "character" );
            js.member( "character", who_id );
            js.member( "idx", find_index( *who, target() ) );
            js.end_object();
        }

        item *unpack( int idx ) const override {
            if( !ensure_who_unpacked() ) {
                return nullptr;
            }
            return retrieve_index( *who, idx );
        }

        type where() const override {
            return type::character;
        }

        tripoint position() const override {
            if( !ensure_who_unpacked() ) {
                return tripoint_zero;
            }
            return who->pos();
        }

        std::string describe( const Character *ch ) const override {
            if( !target() || !ensure_who_unpacked() ) {
                return std::string();
            }

            if( ch == who ) {
                auto parents = who->parents( *target() );
                if( !parents.empty() && who->is_worn( *parents.back() ) ) {
                    return parents.back()->type_name();

                } else if( who->is_worn( *target() ) ) {
                    return _( "worn" );

                } else {
                    return _( "inventory" );
                }

            } else {
                return who->name;
            }
        }

        item_location obtain( Character &ch, int qty ) override {
            ch.mod_moves( -obtain_cost( ch, qty ) );

            on_contents_changed();
            if( &ch.i_at( ch.get_item_position( target() ) ) == target() ) {
                // item already in target characters inventory at base of stack
                return item_location( ch, target() );
            }

            item obj = target()->split( qty );
            if( !obj.is_null() ) {
                return item_location( ch, &ch.i_add( obj, should_stack ) );
            } else {
                item *inv = &ch.i_add( *target(), should_stack );
                remove_item();  // This also takes off the item from whoever wears it.
                return item_location( ch, inv );
            }
        }

        int obtain_cost( const Character &ch, int qty ) const override {
            if( !target() || !ensure_who_unpacked() ) {
                return 0;
            }

            int mv = 0;

            item obj = *target();
            obj = obj.split( qty );
            if( obj.is_null() ) {
                obj = *target();
            }

            item &target_ref = *target();
            if( who->is_wielding( target_ref ) ) {
                mv = who->item_handling_cost( obj, false, 0 );
            } else {
                // then we are wearing it
                mv = who->item_handling_cost( obj, true, INVENTORY_HANDLING_PENALTY / 2 );
            }

            if( &ch != who ) {
                // TODO: implement movement cost for transferring item between characters
            }

            return mv;
        }

        void remove_item() override {
            if( !ensure_who_unpacked() ) {
                return;
            }
            on_contents_changed();
            who->remove_item( *what );
        }

        void on_contents_changed() override {
            target()->on_contents_changed();
        }

        bool valid() const override {
            ensure_who_unpacked();
            ensure_unpacked();
            return !!what && !!who;
        }

        units::volume volume_capacity() const override {
            return units::volume_max;
        }

        units::mass weight_capacity() const override {
            return units::mass_max;
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
            if( target() != &cur.veh.part( cur.part ).base ) {
                js.member( "idx", find_index( cur, target() ) );
            }
            js.end_object();
        }

        item *unpack( int idx ) const override {
            return idx >= 0 ? retrieve_index( cur, idx ) : &cur.veh.part( cur.part ).base;
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

        item_location obtain( Character &ch, int qty ) override {
            ch.moves -= obtain_cost( ch, qty );

            on_contents_changed();
            item obj = target()->split( qty );
            if( !obj.is_null() ) {
                return item_location( ch, &ch.i_add( obj, should_stack ) );
            } else {
                item *inv = &ch.i_add( *target(), should_stack );
                remove_item();
                return item_location( ch, inv );
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

            int mv = ch.item_handling_cost( obj, true, VEHICLE_HANDLING_PENALTY );
            mv += 100 * rl_dist( ch.pos(), cur.veh.global_part_pos3( cur.part ) );

            // TODO: handle unpacking costs

            return mv;
        }

        void remove_item() override {
            on_contents_changed();
            item &base = cur.veh.part( cur.part ).base;
            if( &base == target() ) {
                cur.veh.remove_part( cur.part ); // vehicle_part::base
            } else {
                cur.remove_item( *target() ); // item within CARGO
            }
        }

        void on_contents_changed() override {
            target()->on_contents_changed();
            cur.veh.invalidate_mass();
        }

        units::volume volume_capacity() const override {
            return cur.veh.free_volume( cur.part );
        }

        units::mass weight_capacity() const override {
            return units::mass_max;
        }
};

class item_location::impl::item_in_container : public item_location::impl
{
    private:
        item_location container;

        // figures out the index for the item, which is where it is in the total list of contents
        // note: could be a better way of handling this?
        int calc_index() const {
            int idx = 0;
            for( const item *it : container->all_items_top() ) {
                if( target() == it ) {
                    return idx;
                }
                idx++;
            }
            if( container->empty() ) {
                return -1;
            }
            return idx;
        }
    public:
        item_location parent_item() const override {
            return container;
        }

        item_in_container( const item_location &container, item *which ) :
            impl( which ), container( container ) {}

        void serialize( JsonOut &js ) const override {
            js.start_object();
            js.member( "idx", calc_index() );
            js.member( "type", "in_container" );
            js.member( "parent", container );
            js.end_object();
        }

        item *unpack( int idx ) const override {
            if( idx < 0 || static_cast<size_t>( idx ) >= target()->num_item_stacks() ) {
                return nullptr;
            }
            std::list<const item *> all_items = container->all_items_ptr();
            auto iter = all_items.begin();
            std::advance( iter, idx );
            if( iter != all_items.end() ) {
                return const_cast<item *>( *iter );
            } else {
                return nullptr;
            }
        }

        std::string describe( const Character * ) const override {
            if( !target() ) {
                return std::string();
            }
            return string_format( _( "inside %s" ), container->tname() );
        }

        type where() const override {
            return type::container;
        }

        type where_recursive() const override {
            return container.where_recursive();
        }

        tripoint position() const override {
            return container.position();
        }

        void remove_item() override {
            on_contents_changed();
            container->remove_item( *target() );
        }

        void on_contents_changed() override {
            target()->on_contents_changed();
            container->on_contents_changed();
        }

        item_location obtain( Character &ch, const int qty ) override {
            ch.mod_moves( -obtain_cost( ch, qty ) );

            on_contents_changed();
            if( container.held_by( ch ) ) {
                // we don't need to move it in this case, it's in a pocket
                // we just charge the obtain cost and leave it in place. otherwise
                // it's liable to end up back in the same pocket, where shenanigans ensue
                return item_location( container, target() );
            }
            const item obj = target()->split( qty );
            if( !obj.is_null() ) {
                return item_location( ch, &ch.i_add( obj, should_stack,
                                                     /*avoid=*/nullptr,
                                                     /*allow_drop=*/false ) );
            } else {
                item *const inv = &ch.i_add( *target(), should_stack,
                                             /*avoid=*/nullptr,
                                             /*allow_drop=*/false );
                if( inv->is_null() ) {
                    debugmsg( "failed to add item to character inventory while obtaining from container" );
                }
                remove_item();
                return item_location( ch, inv );
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

            const int container_mv = container->contents.obtain_cost( *target() );
            if( container_mv == 0 ) {
                debugmsg( "ERROR: %s does not contain %s", container->tname(), target()->tname() );
                return 0;
            }

            int primary_cost = ch.mutation_value( "obtain_cost_multiplier" ) * ch.item_handling_cost( *target(),
                               true, container_mv );
            int parent_obtain_cost = container.obtain_cost( ch, qty );
            if( container->get_use( "holster" ) ) {
                if( ch.is_worn( *container ) ) {
                    primary_cost = ch.item_retrieve_cost( *target(), *container, false, container_mv );
                } else {
                    primary_cost = ch.item_retrieve_cost( *target(), *container );
                }
                // for holsters, we should not include the cost of wielding the holster itself
                parent_obtain_cost = 0;
            } else if( container.where() != item_location::type::container ) {
                // a little bonus for grabbing something from what you're wearing
                parent_obtain_cost /= 2;
            }
            return primary_cost + parent_obtain_cost;
        }

        units::volume volume_capacity() const override {
            return container->contained_where( *target() )->remaining_volume();
        }

        units::mass weight_capacity() const override {
            return container->contained_where( *target() )->remaining_weight();
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

item_location::item_location( const item_location &container, item *which )
    : ptr( new impl::item_in_container( container, which ) ) {}

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
    JsonObject obj = js.get_object();
    auto type = obj.get_string( "type" );

    int idx = -1;
    tripoint pos = tripoint_min;

    obj.read( "idx", idx );
    obj.read( "pos", pos );

    if( type == "character" ) {
        character_id who_id;
        if( obj.has_member( "character" ) ) {
            obj.read( "character", who_id );
        } else {
            // This is for migrating saves before npc item locations were supported and all
            // character item locations were assumed to be on g->u
            who_id = get_player_character().getID();
        }
        ptr.reset( new impl::item_on_person( who_id, idx ) );

    } else if( type == "map" ) {
        ptr.reset( new impl::item_on_map( map_cursor( pos ), idx ) );

    } else if( type == "vehicle" ) {
        vehicle *const veh = veh_pointer_or_null( get_map().veh_at( pos ) );
        int part = obj.get_int( "part" );
        if( veh && part >= 0 && part < veh->part_count() ) {
            ptr.reset( new impl::item_on_vehicle( vehicle_cursor( *veh, part ), idx ) );
        }
    } else if( type == "in_container" ) {
        item_location parent;
        obj.read( "parent", parent );
        if( !parent.ptr->valid() ) {
            debugmsg( "parent location does not point to valid item" );
            ptr.reset( new impl::item_on_map( map_cursor( pos ), idx ) ); // drop on ground
            return;
        }
        const std::list<item *> parent_contents = parent->all_items_top();
        if( idx > -1 && idx < static_cast<int>( parent_contents.size() ) ) {
            auto iter = parent_contents.begin();
            std::advance( iter, idx );
            ptr.reset( new impl::item_in_container( parent, *iter ) );
        } else {
            // probably pointing to the wrong item
            debugmsg( "contents index greater than contents size" );
        }
    }
}

item_location item_location::parent_item() const
{
    if( where() == type::container ) {
        return ptr->parent_item();
    }
    debugmsg( "this item location type has no parent" );
    return item_location::nowhere;
}

bool item_location::has_parent() const
{
    if( where() == type::container ) {
        return !!ptr->parent_item();
    }
    return false;
}

bool item_location::parents_can_contain_recursive( item *it ) const
{
    if( !has_parent() ) {
        return true;
    }

    item_location parent = parent_item();
    item_pocket *pocket = parent->contents.contained_where( *get_item() );

    if( pocket->can_contain( *it ).success() ) {
        return parent.parents_can_contain_recursive( it );
    }

    return false;
}

int item_location::max_charges_by_parent_recursive( const item &it ) const
{
    if( !has_parent() ) {
        return item::INFINITE_CHARGES;
    }

    item_location parent = parent_item();
    item_pocket *pocket = parent->contents.contained_where( *get_item() );

    return std::min( { it.charges_per_volume( pocket->remaining_volume() ),
                       it.charges_per_weight( pocket->remaining_weight() ),
                       pocket->rigid() ? item::INFINITE_CHARGES : parent.max_charges_by_parent_recursive( it )
                     } );
}

bool item_location::eventually_contains( item_location loc ) const
{
    while( loc.has_parent() ) {
        if( ( loc = loc.parent_item() ) == *this ) {
            return true;
        }
    }
    return false;
}

item_location::type item_location::where() const
{
    return ptr->where();
}

item_location::type item_location::where_recursive() const
{
    return ptr->where_recursive();
}

tripoint item_location::position() const
{
    return ptr->position();
}

std::string item_location::describe( const Character *ch ) const
{
    return ptr->describe( ch );
}

item_location item_location::obtain( Character &ch, int qty )
{
    if( !ptr->valid() ) {
        debugmsg( "item location does not point to valid item" );
        return item_location();
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

void item_location::on_contents_changed()
{
    if( !ptr->valid() ) {
        debugmsg( "item location does not point to valid item" );
        return;
    }
    ptr->on_contents_changed();
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

bool item_location::held_by( Character &who ) const
{
    if( where() == type::character && g->critter_at<Character>( position() ) == &who ) {
        return true;
    } else if( has_parent() ) {
        return parent_item().held_by( who );
    }
    return false;
}

units::volume item_location::volume_capacity() const
{
    return ptr->volume_capacity();
}

units::mass item_location::weight_capacity() const
{
    return ptr->weight_capacity();
}

bool item_location::protected_from_liquids() const
{
    // check if inside a watertight which is not an open_container
    if( has_parent() ) {
        item_location parent = parent_item();

        // parent can protect the item against water
        if( parent->is_watertight_container() && !parent->will_spill() ) {
            return true;
        }

        // check the parent's parent
        return parent.protected_from_liquids();
    }

    // we recursively checked all containers
    // none are closed watertight containers
    return false;
}
