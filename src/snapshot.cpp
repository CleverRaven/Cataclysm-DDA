#include "snapshot.h"

#include "game.h"
#include "player.h"
#include "map_selector.h"
#include "vehicle_selector.h"
#include "item.h"
#include "itype.h"

static void clean_invalid( std::set<item_location> &src, const std::set<item_location> &invalid )
{
    for( const auto &e : invalid ) {
        src.erase( e );
    }
}

template <typename T>
static void find_items( T &src, std::map<itype_id, std::set<item_location>> &items )
{
    src.visit_items( [&src, &items]( item * e ) {
        items[ e->typeId() ].emplace( src, e );

        // recurse through any nested containers
        return VisitResponse::NEXT;
    } );
}

template <typename T>
static void find_charges( T &src, std::map<std::string, std::set<item_location>> &charges )
{
    src.visit_items( [&src, &charges]( item * e ) {
        if( e->is_tool() ) {
            // for tools we also need to handle subtypes
            charges[ e->typeId() ].emplace( src, e );
            charges[ e->type->tool->subtype ].emplace( src, e );
            // includes charges from any contained magazine so skip these
            return VisitResponse::SKIP;

        } else if( e->count_by_charges() ) {
            charges[ e->typeId() ].emplace( src, e );
            // items counted by charges are not themselves expected to be containers
            return VisitResponse::SKIP;
        }

        // recurse through any nested containers
        return VisitResponse::NEXT;
    } );
}

template <typename T>
static void find_qualities( T &src, std::map<quality_id, std::set<item_location>> &quals )
{
    src.visit_items( [&src, &quals]( item * e ) {
        for( const auto &q : e->type->qualities ) {
            quals[ q.first ].emplace( src, e );
        }

        // recurse through any nested containers
        return VisitResponse::NEXT;
    } );
}

void snapshot::refresh()
{
    items.clear();
    charges.clear();
    quals.clear();

    find_items( g->u, items );
    find_charges( g->u, charges );
    find_qualities( g->u, quals );

    for( auto &e : map_selector( pos, range ) ) {
        find_items( e, items );
        find_charges( e, charges );
        find_qualities( e, quals );
    }

    for( auto &e : vehicle_selector( pos, range ) ) {
        find_items( e, items );
        find_charges( e, charges );
        find_qualities( e, quals );
    }
}

int snapshot::charges_of( const std::string &what, int limit ) const
{
    auto &src = charges[ what ];
    clean_invalid( src, invalid );

    int qty = 0;
    for( const auto &e : src ) {
        if( qty >= limit ) {
            break;
        }
        qty += e->is_tool() ? e->ammo_remaining() : e->charges;
    }
    return std::min( qty, limit );
}
