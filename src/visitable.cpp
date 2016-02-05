#include "visitable.h"

#include "item.h"
#include "inventory.h"
#include "character.h"
#include "map_selector.h"
#include "map.h"

template <typename T>
bool visitable<T>::has_item_with( const std::function<bool(const item&)>& filter ) const {
    return visit_items( [&filter]( const item *node, const item * ) {
        return filter( *node ) ? VisitResponse::ABORT : VisitResponse::NEXT;
    }) == VisitResponse::ABORT;
}

template <typename T>
VisitResponse visitable<T>::visit_items( const std::function<VisitResponse(const item *, const item *)>& func ) const {
    return const_cast<visitable<T> *>( this )->visit_items( static_cast<const std::function<VisitResponse(item *, item *)>&>( func ) );
}

template <typename T>
VisitResponse visitable<T>::visit_items( const std::function<VisitResponse(const item *)>& func ) const {
    return const_cast<visitable<T> *>( this )->visit_items( static_cast<const std::function<VisitResponse(item *)>&>( func ) );
}

template <typename T>
VisitResponse visitable<T>::visit_items( const std::function<VisitResponse(item *)>& func ) {
    return visit_items( [&func]( item *it, item * ) {
        return func( it );
    } );
}

// Specialize visitable<T>::visit_items() for each class that will implement the visitable interface

static VisitResponse visit_internal( const std::function<VisitResponse(item *, item *)>& func, item *node, item *parent = nullptr ) {
    switch( func( node, parent ) ) {
        case VisitResponse::ABORT:
            return VisitResponse::ABORT;

        case VisitResponse::NEXT:
            for( auto& e : node->contents ) {
                if( visit_internal( func, &e, node ) == VisitResponse::ABORT ) {
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
VisitResponse visitable<item>::visit_items( const std::function<VisitResponse( item *, item * )>& func )
{
    auto it = static_cast<item *>( this );
    return visit_internal( func, it );
}

template <>
VisitResponse visitable<inventory>::visit_items( const std::function<VisitResponse( item *, item * )>& func )
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
VisitResponse visitable<Character>::visit_items( const std::function<VisitResponse( item *, item * )>& func )
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

    return ch->inv.visit_items( func );
}

template <>
VisitResponse visitable<map_selector>::visit_items( const std::function<VisitResponse( item *, item * )>& func )
{
    auto sel = static_cast<map_selector *>( this );

    for( const auto &pos : closest_tripoints_first( sel->radius, sel->pos ) ) {
        if( !sel->accessible || sel->m.accessible_items( sel->pos, pos, sel->radius ) ) {
            for( auto& e : sel->m.i_at( pos ) ) {
                if( visit_internal( func, &e ) == VisitResponse::ABORT ) {
                    return VisitResponse::ABORT;
                }
            }
        }
    }
    return VisitResponse::NEXT;
}

// explicit template initialization for all classes implementing the visitable interface
template class visitable<item>;
template class visitable<inventory>;
template class visitable<Character>;
template class visitable<map_selector>;
