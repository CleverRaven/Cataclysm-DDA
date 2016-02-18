#include "visitable.h"

#include "debug.h"
#include "item.h"
#include "inventory.h"
#include "character.h"
#include "map_selector.h"
#include "vehicle_selector.h"
#include "map.h"
#include "vehicle.h"
#include "game.h"

template <typename T>
item * visitable<T>::find_parent( const item& it )
{
    item *res = nullptr;
    if( visit_items_with_loc( [&]( item *node, item *parent, const tripoint * ){
        if( node == &it ) {
            res = parent;
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } ) != VisitResponse::ABORT ) {
        debugmsg( "Tried to find item parent using an object that doesn't contain it" );
    }
    return res;
}

template <typename T>
const item * visitable<T>::find_parent( const item& it ) const
{
    return const_cast<visitable<T> *>( this )->find_parent( it );
}

template <typename T>
std::vector<item *> visitable<T>::parents( const item& it )
{
    std::vector<item *> res;
    for( item *obj = find_parent( it ); obj; obj = find_parent( *obj ) ) {
        res.push_back( obj );
    }
    return res;
}

template <typename T>
std::vector<const item *> visitable<T>::parents( const item& it ) const
{
    std::vector<const item *> res;
    for( const item *obj = find_parent( it ); obj; obj = find_parent( *obj ) ) {
        res.push_back( obj );
    }
    return res;
}

template <typename T>
bool visitable<T>::has_item( const item& it ) const {
    return visit_items_const( [&it]( const item *node ) {
        return node == &it ? VisitResponse::ABORT : VisitResponse::NEXT;
    }) == VisitResponse::ABORT;
}

template <typename T>
bool visitable<T>::has_item_with( const std::function<bool(const item&)>& filter ) const {
    return visit_items_const( [&filter]( const item *node ) {
        return filter( *node ) ? VisitResponse::ABORT : VisitResponse::NEXT;
    }) == VisitResponse::ABORT;
}

template <typename T>
VisitResponse visitable<T>::visit_items_with_loc_const(
    const std::function<VisitResponse(const item *, const item *, const tripoint *)>& func ) const {
    return const_cast<visitable<T> *>( this )->visit_items_with_loc(
        static_cast<const std::function<VisitResponse(item *, item *, const tripoint *)>&>( func ) );
}

template <typename T>
VisitResponse visitable<T>::visit_items_const( const std::function<VisitResponse(const item *)>& func ) const {
    return const_cast<visitable<T> *>( this )->visit_items( static_cast<const std::function<VisitResponse(item *)>&>( func ) );
}

template <typename T>
VisitResponse visitable<T>::visit_items( const std::function<VisitResponse(item *)>& func ) {
    return visit_items_with_loc( [&func]( item *it, item *, const tripoint * ) {
        return func( it );
    } );
}

// Specialize visitable<T>::visit_items() for each class that will implement the visitable interface

static VisitResponse visit_internal( const std::function<VisitResponse(item *, item *, const tripoint *pos)>& func, item *node,
                                     item *parent = nullptr, const tripoint *pos = nullptr )
{
    switch( func( node, parent, pos ) ) {
        case VisitResponse::ABORT:
            return VisitResponse::ABORT;

        case VisitResponse::NEXT:
            for( auto& e : node->contents ) {
                if( visit_internal( func, &e, node, pos ) == VisitResponse::ABORT ) {
                    return VisitResponse::ABORT;
                }
            }
        /* intentional fallthrough */

        case VisitResponse::SKIP:
            return VisitResponse::NEXT;
    }

    /* never reached but suppresses GCC warning */
    return VisitResponse::ABORT;
}

template <>
VisitResponse visitable<item>::visit_items_with_loc(
    const std::function<VisitResponse( item *, item *, const tripoint * )>& func )
{
    auto it = static_cast<item *>( this );
    return visit_internal( func, it );
}

template <>
VisitResponse visitable<inventory>::visit_items_with_loc(
    const std::function<VisitResponse( item *, item *, const tripoint * )>& func )
{
    auto inv = static_cast<inventory *>( this );
    for( auto& stack : inv->items ) {
        for( auto& it : stack ) {
            if( visit_internal( func, &it ) == VisitResponse::ABORT ) {
                return VisitResponse::ABORT;
            }
        }
    }
    return VisitResponse::NEXT;
}

template <>
VisitResponse visitable<Character>::visit_items_with_loc(
    const std::function<VisitResponse( item *, item *, const tripoint * )>& func )
{
    auto ch = static_cast<Character *>( this );

    if( !ch->weapon.is_null() && visit_internal( func, &ch->weapon, nullptr ) == VisitResponse::ABORT ) {
        return VisitResponse::ABORT;
    }

    for( auto& e : ch->worn ) {
        if( visit_internal( func, &e ) == VisitResponse::ABORT ) {
            return VisitResponse::ABORT;
        }
    }

    return ch->inv.visit_items_with_loc( func );
}

template <>
VisitResponse visitable<map_cursor>::visit_items_with_loc(
    const std::function<VisitResponse( item *, item *, const tripoint* )>& func ) {
    auto cur = static_cast<map_cursor *>( this );

    for( auto& e : g->m.i_at( *cur ) ) {
        if( visit_internal( func, &e, nullptr, cur ) == VisitResponse::ABORT ) {
            return VisitResponse::ABORT;
        }
    }
    return VisitResponse::NEXT;
}

template <>
VisitResponse visitable<map_selector>::visit_items_with_loc(
    const std::function<VisitResponse( item *, item *, const tripoint* )>& func )
{
    for( auto &cursor : static_cast<map_selector&>( *this ) ) {
        if( cursor.visit_items_with_loc( func ) == VisitResponse::ABORT ) {
            return VisitResponse::ABORT;
        }
    }
    return VisitResponse::NEXT;
}

template <>
VisitResponse visitable<vehicle_cursor>::visit_items_with_loc(
    const std::function<VisitResponse( item *, item *, const tripoint* )>& func ) {
    auto self = static_cast<vehicle_cursor *>( this );

    for( auto &e : self->veh.get_items( self->part ) ) {
        auto pos = self->veh.global_part_pos3( self->part );
        if( visit_internal( func, &e, nullptr, &pos ) == VisitResponse::ABORT ) {
            return VisitResponse::ABORT;
        }
    }
    return VisitResponse::NEXT;
}

template <>
VisitResponse visitable<vehicle_selector>::visit_items_with_loc(
    const std::function<VisitResponse( item *, item *, const tripoint* )>& func )
{
    for( auto &cursor : static_cast<vehicle_selector&>( *this ) ) {
        if( cursor.visit_items_with_loc( func ) == VisitResponse::ABORT ) {
            return VisitResponse::ABORT;
        }
    }
    return VisitResponse::NEXT;
}

// explicit template initialization for all classes implementing the visitable interface
template class visitable<item>;
template class visitable<inventory>;
template class visitable<Character>;
template class visitable<map_selector>;
template class visitable<map_cursor>;
template class visitable<vehicle_selector>;
template class visitable<vehicle_cursor>;
