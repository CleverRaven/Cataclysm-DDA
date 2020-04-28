#include <functional>
#include <memory>
#include <string>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "map_helpers.h"
#include "rng.h"
#include "item_location.h"
#include "map.h"
#include "map_selector.h"
#include "optional.h"
#include "player_helpers.h"
#include "point.h"
#include "visitable.h"

TEST_CASE( "item_location_can_maintain_reference_despite_item_removal", "[item][item_location]" )
{
    clear_map();
    map &m = g->m;
    tripoint pos( 60, 60, 0 );
    m.i_clear( pos );
    m.add_item( pos, item( "jeans" ) );
    m.add_item( pos, item( "jeans" ) );
    m.add_item( pos, item( "tshirt" ) );
    m.add_item( pos, item( "jeans" ) );
    m.add_item( pos, item( "jeans" ) );
    map_cursor cursor( pos );
    item *tshirt = nullptr;
    cursor.visit_items( [&tshirt]( item * i ) {
        if( i->typeId() == "tshirt" ) {
            tshirt = i;
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } );
    REQUIRE( tshirt != nullptr );
    item_location item_loc( cursor, tshirt );
    REQUIRE( item_loc->typeId() == "tshirt" );
    for( int j = 0; j < 4; ++j ) {
        // Delete up to 4 random jeans
        map_stack stack = m.i_at( pos );
        REQUIRE( !stack.empty() );
        item *i = &random_entry_opt( stack )->get();
        if( i->typeId() == "jeans" ) {
            m.i_rem( pos, i );
        }
    }
    CAPTURE( m.i_at( pos ) );
    REQUIRE( item_loc );
    CHECK( item_loc->typeId() == "tshirt" );
}

TEST_CASE( "item_location_doesnt_return_stale_map_item", "[item][item_location]" )
{
    clear_map();
    map &m = g->m;
    tripoint pos( 60, 60, 0 );
    m.i_clear( pos );
    m.add_item( pos, item( "tshirt" ) );
    item_location item_loc( map_cursor( pos ), &m.i_at( pos ).only_item() );
    REQUIRE( item_loc->typeId() == "tshirt" );
    m.i_rem( pos, &*item_loc );
    m.add_item( pos, item( "jeans" ) );
    CHECK( !item_loc );
}

TEST_CASE( "item_in_container", "[item][item_location]" )
{
    avatar &dummy = g->u;
    clear_avatar();
    item &backpack = dummy.i_add( item( "backpack" ) );
    item jeans( "jeans" );

    REQUIRE( dummy.has_item( backpack ) );

    backpack.put_in( jeans, item_pocket::pocket_type::CONTAINER );

    item_location backpack_loc( dummy, & **dummy.wear( backpack ) );

    REQUIRE( dummy.has_item( *backpack_loc ) );

    item_location jeans_loc( backpack_loc, &backpack_loc->contents.only_item() );

    REQUIRE( backpack_loc.where() == item_location::type::character );
    REQUIRE( jeans_loc.where() == item_location::type::container );

    CHECK( backpack_loc.obtain_cost( dummy ) +
           backpack_loc->contents.obtain_cost( *jeans_loc ) ==
           jeans_loc.obtain_cost( dummy ) );

    CHECK( jeans_loc.parent_item() == backpack_loc );
}
