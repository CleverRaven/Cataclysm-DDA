#include <functional>
#include <list>
#include <optional>

#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "rng.h"
#include "type_id.h"
#include "visitable.h"

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_jeans( "jeans" );
static const itype_id itype_tshirt( "tshirt" );

TEST_CASE( "item_location_can_maintain_reference_despite_item_removal", "[item][item_location]" )
{
    clear_map();
    map &m = get_map();
    tripoint_bub_ms pos( 60, 60, 0 );
    m.i_clear( pos );
    m.add_item( pos, item( itype_jeans ) );
    m.add_item( pos, item( itype_jeans ) );
    m.add_item( pos, item( itype_tshirt ) );
    m.add_item( pos, item( itype_jeans ) );
    m.add_item( pos, item( itype_jeans ) );
    map_cursor cursor( pos );
    item *tshirt = nullptr;
    cursor.visit_items( [&tshirt]( item * i, item * ) {
        if( i->typeId() == itype_tshirt ) {
            tshirt = i;
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } );
    REQUIRE( tshirt != nullptr );
    item_location item_loc( cursor, tshirt );
    REQUIRE( item_loc->typeId() == itype_tshirt );
    for( int j = 0; j < 4; ++j ) {
        // Delete up to 4 random jeans
        map_stack stack = m.i_at( pos );
        REQUIRE( !stack.empty() );
        item *i = &random_entry_opt( stack )->get();
        if( i->typeId() == itype_jeans ) {
            m.i_rem( pos, i );
        }
    }
    CAPTURE( m.i_at( pos ) );
    REQUIRE( item_loc );
    CHECK( item_loc->typeId() == itype_tshirt );
}

TEST_CASE( "item_location_doesnt_return_stale_map_item", "[item][item_location]" )
{
    clear_map();
    map &m = get_map();
    tripoint_bub_ms pos( 60, 60, 0 );
    m.i_clear( pos );
    m.add_item( pos, item( itype_tshirt ) );
    item_location item_loc( map_cursor( pos ), &m.i_at( pos ).only_item() );
    REQUIRE( item_loc->typeId() == itype_tshirt );
    m.i_rem( pos, &*item_loc );
    m.add_item( pos, item( itype_jeans ) );
    CHECK( !item_loc );
}

TEST_CASE( "item_in_container", "[item][item_location]" )
{
    Character &dummy = get_player_character();
    clear_avatar();
    item_location backpack = dummy.i_add( item( itype_backpack ) );
    item jeans( itype_jeans );

    REQUIRE( dummy.has_item( *backpack ) );

    backpack->put_in( jeans, pocket_type::CONTAINER );

    item_location backpack_loc( dummy, & **dummy.wear_item( *backpack ) );

    REQUIRE( dummy.has_item( *backpack_loc ) );

    item_location jeans_loc( backpack_loc, &backpack_loc->only_item() );

    REQUIRE( backpack_loc.where() == item_location::type::character );
    REQUIRE( jeans_loc.where() == item_location::type::container );
    const int obtain_cost_calculation = dummy.item_handling_cost( *jeans_loc, true,
                                        backpack_loc->obtain_cost( *jeans_loc ) );
    CHECK( obtain_cost_calculation == jeans_loc.obtain_cost( dummy ) );

    CHECK( jeans_loc.parent_item() == backpack_loc );
}
