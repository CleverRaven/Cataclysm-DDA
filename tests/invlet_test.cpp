#include <cstddef>
#include <sstream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "map.h"
#include "options.h"
#include "player.h"
#include "map_helpers.h"
#include "inventory.h"
#include "item.h"
#include "player_activity.h"
#include "type_id.h"
#include "item_location.h"
#include "map_selector.h"
#include "point.h"
#include "visitable.h"

const trait_id trait_debug_storage( "DEBUG_STORAGE" );

enum inventory_location {
    GROUND,
    INVENTORY,
    WORN,
    WIELDED_OR_WORN,
    INV_LOCATION_NUM,
};

enum invlet_state {
    UNEXPECTED = -1,
    NONE = 0,
    CACHED,
    ASSIGNED,
    INVLET_STATE_NUM,
};

enum test_action {
    REMOVE_1ST_REMOVE_2ND_ADD_1ST_ADD_2ND,
    REMOVE_1ST_REMOVE_2ND_ADD_2ND_ADD_1ST,
    REMOVE_1ST_ADD_1ST,
    TEST_ACTION_NUM,
};

// This is a massive hack that makes this test work without totally rewriting it after #31406
// The number of null items in item.components is used as a unique id for the purposes of this test
//
// The reason components is used instead of some other property of items is that this isn't checked
// when determining if two items stack and therefore has no side effects.
static void set_id( item &it, int id )
{
    it.components = std::list<item>( static_cast<size_t>( id ), item() );
}

static int get_id( const item &it )
{
    return static_cast<int>( it.components.size() );
}

template <typename T>
static item *retrieve_item( const T &sel, int id )
{
    item *obj = nullptr;
    sel.visit_items( [&id, &obj]( const item * e ) {
        if( get_id( *e ) == id ) {
            obj = const_cast<item *>( e );
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } );
    return obj;
}

static std::string location_desc( const inventory_location loc )
{
    switch( loc ) {
        case GROUND:
            return "the ground";
        case INVENTORY:
            return "inventory";
        case WORN:
            return "worn items";
        case WIELDED_OR_WORN:
            return "wielded or worn items";
        default:
            break;
    }
    return "unknown location";
}

static std::string move_action_desc( const int pos, const inventory_location from,
                                     const inventory_location to )
{
    std::stringstream ss;
    ss << "move ";
    switch( pos ) {
        case 0:
            ss << "1st";
            break;
        case 1:
            ss << "2nd";
            break;
        default:
            return "unimplemented";
    }
    ss << " item ";
    switch( from ) {
        case GROUND:
            ss << "on the ground";
            break;
        case INVENTORY:
            ss << "in inventory";
            break;
        case WORN:
            ss << "worn";
            break;
        case WIELDED_OR_WORN:
            ss << "wielded or worn";
            break;
        default:
            return "unimplemented";
    }
    ss << " to ";
    switch( to ) {
        case GROUND:
            ss << "the ground";
            break;
        case INVENTORY:
            ss << "inventory";
            break;
        case WORN:
            ss << "worn items";
            break;
        case WIELDED_OR_WORN:
            ss << "wielded or worn items";
            break;
        default:
            return "unimplemented";
    }
    return ss.str();
}

static std::string invlet_state_desc( const invlet_state invstate )
{
    switch( invstate ) {
        case NONE:
            return "none";
        case CACHED:
            return "cached";
        case ASSIGNED:
            return "assigned";
        default:
            break;
    }
    return "unexpected";
}

static std::string test_action_desc(
    const test_action action, const inventory_location from, const inventory_location to,
    const invlet_state first_invlet_state, const invlet_state second_invlet_state,
    const invlet_state expected_first_invlet_state,
    const invlet_state expected_second_invlet_state,
    const invlet_state final_first_invlet_state, const invlet_state final_second_invlet_state )
{
    std::stringstream ss;
    ss << "1. add 1st item to " << location_desc( to ) << std::endl;
    ss << "2. add 2nd item to " << location_desc( to ) << std::endl;
    ss << "3. set 1st item's invlet to " << invlet_state_desc( first_invlet_state ) << std::endl;
    ss << "4. " << move_action_desc( 0, to, from ) << std::endl;
    ss << "5. set 2nd item's invlet to " << invlet_state_desc( second_invlet_state ) << std::endl;
    switch( action ) {
        case REMOVE_1ST_REMOVE_2ND_ADD_1ST_ADD_2ND:
            ss << "6. " << move_action_desc( 1, to, from ) << std::endl;
            ss << "7. " << move_action_desc( 0, from, to ) << std::endl;
            ss << "8. " << move_action_desc( 1, from, to ) << std::endl;
            break;
        case REMOVE_1ST_REMOVE_2ND_ADD_2ND_ADD_1ST:
            ss << "6. " << move_action_desc( 1, to, from ) << std::endl;
            ss << "7. " << move_action_desc( 1, from, to ) << std::endl;
            ss << "8. " << move_action_desc( 0, from, to ) << std::endl;
            break;
        case REMOVE_1ST_ADD_1ST:
            ss << "6. " << move_action_desc( 0, from, to ) << std::endl;
            break;
        default:
            return "unimplemented";
    }
    ss << "expect 1st item to have " << invlet_state_desc( expected_first_invlet_state ) << " invlet" <<
       std::endl;
    ss << "1st item actually has " << invlet_state_desc( final_first_invlet_state ) << " invlet" <<
       std::endl;
    ss << "expect 2nd item to have " << invlet_state_desc( expected_second_invlet_state ) << " invlet"
       << std::endl;
    ss << "2nd item actually has " << invlet_state_desc( final_second_invlet_state ) << " invlet" <<
       std::endl;

    return ss.str();
}

static void assign_invlet( player &p, item &it, const char invlet, const invlet_state invstate )
{
    p.reassign_item( it, '\0' );
    switch( invstate ) {
        case NONE:
            break;
        case CACHED:
            // assigning it twice makes it a cached but non-player-assigned invlet
            p.reassign_item( it, invlet );
            p.reassign_item( it, invlet );
            break;
        case ASSIGNED:
            p.reassign_item( it, invlet );
            break;
        default:
            FAIL( "unimplemented" );
    }
}

static invlet_state check_invlet( player &p, item &it, const char invlet )
{
    if( it.invlet == '\0' ) {
        return NONE;
    } else if( it.invlet == invlet ) {
        if( p.inv.assigned_invlet.find( invlet ) != p.inv.assigned_invlet.end() &&
            p.inv.assigned_invlet[invlet] == it.typeId() ) {
            return ASSIGNED;
        } else {
            return CACHED;
        }
    }
    return UNEXPECTED;
}

static void drop_at_feet( player &p, const int id )
{
    size_t size_before = g->m.i_at( p.pos() ).size();

    item *found = retrieve_item( p, id );
    REQUIRE( found );
    item_location loc( p, found );
    p.moves = 100;
    p.drop( loc, p.pos() );
    p.activity.do_turn( p );

    REQUIRE( g->m.i_at( p.pos() ).size() == size_before + 1 );
}

static void pick_up_from_feet( player &p, int id )
{
    map_stack items = g->m.i_at( p.pos() );
    size_t size_before = items.size();

    item *found = retrieve_item( map_cursor( p.pos() ), id );
    REQUIRE( found );

    p.moves = 100;
    p.assign_activity( activity_id( "ACT_PICKUP" ) );
    p.activity.targets.emplace_back( map_cursor( p.pos() ), found );
    p.activity.values.push_back( 0 );
    p.activity.do_turn( p );

    REQUIRE( items.size() == size_before - 1 );
}

static void wear_from_feet( player &p, int id )
{
    map_stack items = g->m.i_at( p.pos() );
    size_t size_before = items.size();

    item *found = retrieve_item( map_cursor( p.pos() ), id );
    REQUIRE( found );

    p.wear_item( *found, false );
    g->m.i_rem( p.pos(), found );

    REQUIRE( items.size() == size_before - 1 );
}

static void wield_from_feet( player &p, int id )
{
    map_stack items = g->m.i_at( p.pos() );
    size_t size_before = items.size();

    item *found = retrieve_item( map_cursor( p.pos() ), id );
    REQUIRE( found );

    p.wield( *found );
    g->m.i_rem( p.pos(), found );

    REQUIRE( items.size() == size_before - 1 );
}

static void add_item( player &p, item &it, const inventory_location loc )
{
    switch( loc ) {
        case GROUND:
            g->m.add_item( p.pos(), it );
            break;
        case INVENTORY:
            p.i_add( it );
            break;
        case WORN:
            p.wear_item( it );
            break;
        case WIELDED_OR_WORN:
            if( !p.is_armed() ) {
                p.wield( it );
            } else {
                // since we can only wield one item, wear the item instead
                p.wear_item( it );
            }
            break;
        default:
            FAIL( "unimplemented" );
            break;
    }
}

static item &item_at( player &p, const int id, const inventory_location loc )
{
    switch( loc ) {
        case GROUND: {
            item *found = retrieve_item( map_cursor( p.pos() ), id );
            REQUIRE( found );
            return *found;
        }
        case INVENTORY:
        case WORN:
        case WIELDED_OR_WORN: {
            item *found = retrieve_item( p, id );
            REQUIRE( found );
            return *found;
        }
        default:
            FAIL( "unimplemented" );
            break;
    }
    return null_item_reference();
}

static void move_item( player &p, const int id, const inventory_location from,
                       const inventory_location to )
{
    switch( from ) {
        case GROUND:
            switch( to ) {
                case GROUND:
                default:
                    FAIL( "unimplemented" );
                    break;
                case INVENTORY:
                    pick_up_from_feet( p, id );
                    break;
                case WORN:
                    wear_from_feet( p, id );
                    break;
                case WIELDED_OR_WORN:
                    if( p.weapon.is_null() ) {
                        wield_from_feet( p, id );
                    } else {
                        // since we can only wield one item, wear the item instead
                        wear_from_feet( p, id );
                    }
                    break;
            }
            break;
        case INVENTORY:
            switch( to ) {
                case GROUND:
                    drop_at_feet( p, id );
                    break;
                case INVENTORY:
                default:
                    FAIL( "unimplemented" );
                    break;
                case WORN:
                    p.wear( item_at( p, id, from ), false );
                    break;
                case WIELDED_OR_WORN:
                    if( p.weapon.is_null() ) {
                        p.wield( item_at( p, id, from ) );
                    } else {
                        // since we can only wield one item, wear the item instead
                        p.wear( item_at( p, id, from ), false );
                    }
                    break;
            }
            break;
        case WORN:
            switch( to ) {
                case GROUND:
                    drop_at_feet( p, id );
                    break;
                case INVENTORY:
                    p.takeoff( item_at( p, id, from ) );
                    break;
                case WORN:
                case WIELDED_OR_WORN:
                default:
                    FAIL( "unimplemented" );
                    break;
            }
            break;
        case WIELDED_OR_WORN:
            switch( to ) {
                case GROUND:
                    drop_at_feet( p, id );
                    if( !p.is_armed() && !p.worn.empty() ) {
                        // wield the first worn item
                        p.wield( p.worn.front() );
                    }
                    break;
                case INVENTORY:
                    if( p.is_wielding( item_at( p, id, from ) ) ) {
                        p.i_add( p.i_rem( &item_at( p, id, from ) ) );
                    } else {
                        p.takeoff( item_at( p, id, from ) );
                    }
                    if( !p.is_armed() && !p.worn.empty() ) {
                        // wield the first worn item
                        p.wield( p.worn.front() );
                    }
                    break;
                case WORN:
                case WIELDED_OR_WORN:
                default:
                    FAIL( "unimplemented" );
                    break;
            }
            break;
        default:
            FAIL( "unimplemented" );
            break;
    }
}

static void invlet_test( player &dummy, const inventory_location from, const inventory_location to )
{
    // invlet to assign
    constexpr char invlet = '|';

    // iterate through all permutations of test actions
    for( int id = 0; id < INVLET_STATE_NUM * INVLET_STATE_NUM * TEST_ACTION_NUM; ++id ) {
        // how to assign invlet to the first item
        const invlet_state first_invlet_state = invlet_state( id % INVLET_STATE_NUM );
        // how to assign invlet to the second item
        const invlet_state second_invlet_state = invlet_state( id / INVLET_STATE_NUM % INVLET_STATE_NUM );
        // the test steps
        const test_action action = test_action( id / INVLET_STATE_NUM / INVLET_STATE_NUM %
                                                TEST_ACTION_NUM );

        // the final expected invlet state of the two items
        invlet_state expected_first_invlet_state = second_invlet_state == NONE ? first_invlet_state : NONE;
        invlet_state expected_second_invlet_state = second_invlet_state;

        // remove all items
        dummy.inv.clear();
        dummy.worn.clear();
        dummy.remove_weapon();
        g->m.i_clear( dummy.pos() );

        // some two items that can be wielded, worn, and picked up
        item tshirt( "tshirt" );
        item jeans( "jeans" );

        set_id( tshirt, 1 );
        set_id( jeans, 2 );

        // add the items to the starting position
        add_item( dummy, tshirt, to );
        add_item( dummy, jeans, to );

        // assign invlet to the first item
        assign_invlet( dummy, item_at( dummy, 1, to ), invlet, first_invlet_state );

        // remove the first item
        move_item( dummy, 1, to, from );

        // assign invlet to the second item
        assign_invlet( dummy, item_at( dummy, 2, to ), invlet, second_invlet_state );

        item *final_first = nullptr;
        item *final_second = nullptr;
        switch( action ) {
            case REMOVE_1ST_REMOVE_2ND_ADD_1ST_ADD_2ND:
                move_item( dummy, 2, to, from );
                move_item( dummy, 1, from, to );
                move_item( dummy, 2, from, to );
                final_first = &item_at( dummy, 1, to );
                final_second = &item_at( dummy, 2, to );
                break;
            case REMOVE_1ST_REMOVE_2ND_ADD_2ND_ADD_1ST:
                move_item( dummy, 2, to, from );
                move_item( dummy, 2, from, to );
                move_item( dummy, 1, from, to );
                final_first = &item_at( dummy, 1, to );
                final_second = &item_at( dummy, 2, to );
                break;
            case REMOVE_1ST_ADD_1ST:
                move_item( dummy, 1, from, to );
                final_first = &item_at( dummy, 1, to );
                final_second = &item_at( dummy, 2, to );
                break;
            default:
                FAIL( "unimplemented" );
                break;
        }

        invlet_state final_first_invlet_state = check_invlet( dummy, *final_first, invlet ),
                     final_second_invlet_state = check_invlet( dummy, *final_second, invlet );

        INFO( test_action_desc( action, from, to, first_invlet_state, second_invlet_state,
                                expected_first_invlet_state, expected_second_invlet_state, final_first_invlet_state,
                                final_second_invlet_state ) );
        REQUIRE( final_first->typeId() == tshirt.typeId() );
        REQUIRE( final_second->typeId() == jeans.typeId() );
        CHECK( final_first_invlet_state == expected_first_invlet_state );
        CHECK( final_second_invlet_state == expected_second_invlet_state );

        // clear invlets
        assign_invlet( dummy, *final_first, invlet, NONE );
        assign_invlet( dummy, *final_second, invlet, NONE );
    }
}

static void stack_invlet_test( player &dummy, inventory_location from, inventory_location to )
{
    // invlet to assign
    constexpr char invlet = '|';

    // duplication will most likely only happen if the stack is in the inventory
    // and is subsequently wielded or worn
    if( from != INVENTORY || ( to != WORN && to != WIELDED_OR_WORN ) ) {
        FAIL( "unimplemented" );
    }

    // remove all items
    dummy.inv.clear();
    dummy.worn.clear();
    dummy.remove_weapon();
    g->m.i_clear( dummy.pos() );

    // some stackable item that can be wielded and worn
    item tshirt1( "tshirt" );
    item tshirt2( "tshirt" );

    set_id( tshirt1, 1 );
    set_id( tshirt2, 2 );

    // add two such items to the starting position
    add_item( dummy, tshirt1, from );
    add_item( dummy, tshirt2, from );

    // assign the stack with invlet
    assign_invlet( dummy, item_at( dummy, 1, from ), invlet, CACHED );

    // wield or wear the first item
    move_item( dummy, 1, from, to );

    std::stringstream ss;
    ss << "1. add a stack of two same items to " << location_desc( from ) << std::endl;
    ss << "2. assign the stack with an invlet" << std::endl;
    ss << "3. " << move_action_desc( 0, from, to ) << std::endl;
    ss << "expect the two items to have different invlets" << std::endl;
    ss << "actually the two items have " <<
       ( item_at( dummy, 1, to ).invlet != item_at( dummy, 2, from ).invlet ? "different" : "the same" ) <<
       " invlets" << std::endl;
    INFO( ss.str() );
    REQUIRE( item_at( dummy, 1, to ).typeId() == tshirt1.typeId() );
    REQUIRE( item_at( dummy, 2, from ).typeId() == tshirt2.typeId() );
    // the wielded/worn item should have different invlet from the remaining item
    CHECK( item_at( dummy, 1, to ).invlet != item_at( dummy, 2, from ).invlet );

    // clear invlets
    assign_invlet( dummy, item_at( dummy, 1, to ), invlet, NONE );
    assign_invlet( dummy, item_at( dummy, 2, from ), invlet, NONE );
}

static void swap_invlet_test( player &dummy, inventory_location loc )
{
    // invlet to assign
    constexpr char invlet_1 = '{';
    constexpr char invlet_2 = '}';

    // cannot swap invlets of items on the ground
    REQUIRE( loc != GROUND );

    // remove all items
    dummy.inv.clear();
    dummy.worn.clear();
    dummy.remove_weapon();
    g->m.i_clear( dummy.pos() );

    // two items of the same type that do not stack
    item tshirt1( "tshirt" );
    item tshirt2( "tshirt" );
    tshirt2.mod_damage( -1 );

    set_id( tshirt1, 1 );
    set_id( tshirt2, 2 );

    // add the items
    add_item( dummy, tshirt1, loc );
    add_item( dummy, tshirt2, loc );

    // assign the items with invlets
    assign_invlet( dummy, item_at( dummy, 1, loc ), invlet_1, CACHED );
    assign_invlet( dummy, item_at( dummy, 2, loc ), invlet_2, CACHED );

    // swap the invlets (invoking twice to make the invlet non-player-assigned)
    dummy.reassign_item( item_at( dummy, 1, loc ), invlet_2 );
    dummy.reassign_item( item_at( dummy, 1, loc ), invlet_2 );

    // drop the items
    move_item( dummy, 1, loc, GROUND );
    move_item( dummy, 2, loc, GROUND );

    // get them again
    move_item( dummy, 1, GROUND, loc );
    move_item( dummy, 2, GROUND, loc );

    std::stringstream ss;
    ss << "1. add two items of the same type to " << location_desc( loc ) <<
       ", and ensure them do not stack" << std::endl;
    ss << "2. assign different invlets to the two items" << std::endl;
    ss << "3. swap the invlets by assign one of the items with the invlet of the other item" <<
       std::endl;
    ss << "4. move the items to " << location_desc( GROUND ) << std::endl;
    ss << "5. move the items to " << location_desc( loc ) << " again" << std::endl;
    ss << "expect the items to keep their swapped invlets" << std::endl;
    if( item_at( dummy, 1, loc ).invlet == invlet_2 && item_at( dummy, 2, loc ).invlet == invlet_1 ) {
        ss << "the items actually keep their swapped invlets" << std::endl;
    } else {
        ss << "the items actually does not keep their swapped invlets" << std::endl;
    }
    INFO( ss.str() );
    REQUIRE( item_at( dummy, 1, loc ).typeId() == tshirt1.typeId() );
    REQUIRE( item_at( dummy, 2, loc ).typeId() == tshirt2.typeId() );
    // invlets should not disappear and should still be swapped
    CHECK( item_at( dummy, 1, loc ).invlet == invlet_2 );
    CHECK( item_at( dummy, 2, loc ).invlet == invlet_1 );
    CHECK( check_invlet( dummy, item_at( dummy, 1, loc ), invlet_2 ) == CACHED );
    CHECK( check_invlet( dummy, item_at( dummy, 2, loc ), invlet_1 ) == CACHED );

    // clear invlets
    assign_invlet( dummy, item_at( dummy, 1, loc ), invlet_2, NONE );
    assign_invlet( dummy, item_at( dummy, 2, loc ), invlet_1, NONE );
}

static void merge_invlet_test( player &dummy, inventory_location from )
{
    // invlet to assign
    constexpr char invlet_1 = '{';
    constexpr char invlet_2 = '}';

    // should merge from a place other than the inventory
    REQUIRE( from != INVENTORY );
    // cannot assign invlet to items on the ground
    REQUIRE( from != GROUND );

    for( int id = 0; id < INVLET_STATE_NUM * INVLET_STATE_NUM; ++id ) {
        // how to assign invlet to the first item
        invlet_state first_invlet_state = invlet_state( id % INVLET_STATE_NUM );
        // how to assign invlet to the second item
        invlet_state second_invlet_state = invlet_state( id / INVLET_STATE_NUM );
        // what the invlet should be for the merged stack
        invlet_state expected_merged_invlet_state = first_invlet_state != NONE ? first_invlet_state :
                second_invlet_state;
        char expected_merged_invlet = first_invlet_state != NONE ? invlet_1 : second_invlet_state != NONE ?
                                      invlet_2 : 0;

        // remove all items
        dummy.inv.clear();
        dummy.worn.clear();
        dummy.remove_weapon();
        g->m.i_clear( dummy.pos() );

        // some stackable item
        item tshirt1( "tshirt" );
        item tshirt2( "tshirt" );

        set_id( tshirt1, 1 );
        set_id( tshirt2, 2 );

        // add the item
        add_item( dummy, tshirt1, INVENTORY );
        add_item( dummy, tshirt2, from );

        // assign the items with invlets
        assign_invlet( dummy, item_at( dummy, 1, INVENTORY ), invlet_1, first_invlet_state );
        assign_invlet( dummy, item_at( dummy, 2, from ), invlet_2, second_invlet_state );

        // merge the second item into inventory
        move_item( dummy, 2, from, INVENTORY );

        item &merged_item = item_at( dummy, 1, INVENTORY );
        invlet_state merged_invlet_state = check_invlet( dummy, merged_item, expected_merged_invlet );
        char merged_invlet = merged_item.invlet;

        std::stringstream ss;
        ss << "1. add two stackable items to the inventory and " << location_desc( from ) << std::endl;
        ss << "2. assign " << invlet_state_desc( first_invlet_state ) << " invlet " << invlet_1 <<
           " to the item in the inventory " << std::endl;
        ss << "3. assign " << invlet_state_desc( second_invlet_state ) << " invlet " << invlet_2 <<
           " to the " << location_desc( from ) << std::endl;
        ss << "4. " << move_action_desc( 0, from, INVENTORY ) << std::endl;
        ss << "expect the stack in the inventory to have " << invlet_state_desc(
               expected_merged_invlet_state ) << " invlet " << expected_merged_invlet << std::endl;
        ss << "the stack actually has " << invlet_state_desc( merged_invlet_state ) << " invlet " <<
           merged_invlet << std::endl;
        INFO( ss.str() );
        REQUIRE( merged_item.typeId() == tshirt1.typeId() );
        CHECK( merged_invlet_state == expected_merged_invlet_state );
        CHECK( merged_invlet == expected_merged_invlet );
    }
}

#define invlet_test_autoletter_off( name, dummy, from, to ) \
    SECTION( std::string( name ) + " (auto letter off)" ) { \
        get_options().get_option( "AUTO_INV_ASSIGN" ).setValue( "disabled" ); \
        invlet_test( dummy, from, to ); \
    }

#define stack_invlet_test_autoletter_off( name, dummy, from, to ) \
    SECTION( std::string( name ) + " (auto letter off)" ) { \
        get_options().get_option( "AUTO_INV_ASSIGN" ).setValue( "disabled" ); \
        stack_invlet_test( dummy, from, to ); \
    }

#define swap_invlet_test_autoletter_off( name, dummy, loc ) \
    SECTION( std::string( name ) + " (auto letter off)" ) { \
        get_options().get_option( "AUTO_INV_ASSIGN" ).setValue( "disabled" ); \
        swap_invlet_test( dummy, loc ); \
    }

#define merge_invlet_test_autoletter_off( name, dummy, from ) \
    SECTION( std::string( name ) + " (auto letter off)" ) { \
        get_options().get_option( "AUTO_INV_ASSIGN" ).setValue( "disabled" ); \
        merge_invlet_test( dummy, from ); \
    }

TEST_CASE( "Inventory letter test", "[invlet]" )
{
    player &dummy = g->u;
    const tripoint spot( 60, 60, 0 );
    clear_map();
    dummy.setpos( spot );
    g->m.ter_set( spot, ter_id( "t_dirt" ) );
    g->m.furn_set( spot, furn_id( "f_null" ) );
    if( !dummy.has_trait( trait_debug_storage ) ) {
        dummy.set_mutation( trait_debug_storage );
    }

    invlet_test_autoletter_off( "Picking up items from the ground", dummy, GROUND, INVENTORY );
    invlet_test_autoletter_off( "Wearing items from the ground", dummy, GROUND, WORN );
    invlet_test_autoletter_off( "Wielding and wearing items from the ground", dummy, GROUND,
                                WIELDED_OR_WORN );
    invlet_test_autoletter_off( "Wearing items from inventory", dummy, INVENTORY, WORN );

    stack_invlet_test_autoletter_off( "Wearing item from a stack in inventory", dummy, INVENTORY,
                                      WORN );
    stack_invlet_test_autoletter_off( "Wielding item from a stack in inventory", dummy, INVENTORY,
                                      WIELDED_OR_WORN );

    swap_invlet_test_autoletter_off( "Swapping invlets of two worn items of the same type", dummy,
                                     WORN );

    merge_invlet_test_autoletter_off( "Merging wielded item into an inventory stack", dummy,
                                      WIELDED_OR_WORN );
    merge_invlet_test_autoletter_off( "Merging worn item into an inventory stack", dummy, WORN );
}

static void verify_invlet_consistency( const invlet_favorites &fav )
{
    for( const auto &p : fav.get_invlets_by_id() ) {
        for( const char invlet : p.second ) {
            CHECK( fav.contains( invlet, p.first ) );
        }
    }
}

TEST_CASE( "invlet_favourites_can_erase", "[invlet]" )
{
    invlet_favorites fav;
    fav.set( 'a', "a" );
    verify_invlet_consistency( fav );
    CHECK( fav.invlets_for( "a" ) == "a" );
    fav.erase( 'a' );
    verify_invlet_consistency( fav );
    CHECK( fav.invlets_for( "a" ).empty() );
}

TEST_CASE( "invlet_favourites_removes_clashing_on_insertion", "[invlet]" )
{
    invlet_favorites fav;
    fav.set( 'a', "a" );
    verify_invlet_consistency( fav );
    CHECK( fav.invlets_for( "a" ) == "a" );
    CHECK( fav.invlets_for( "b" ).empty() );
    fav.set( 'a', "b" );
    verify_invlet_consistency( fav );
    CHECK( fav.invlets_for( "a" ).empty() );
    CHECK( fav.invlets_for( "b" ) == "a" );
}

TEST_CASE( "invlet_favourites_retains_order_on_insertion", "[invlet]" )
{
    invlet_favorites fav;
    fav.set( 'a', "a" );
    fav.set( 'b', "a" );
    fav.set( 'c', "a" );
    verify_invlet_consistency( fav );
    CHECK( fav.invlets_for( "a" ) == "abc" );
    fav.set( 'b', "a" );
    verify_invlet_consistency( fav );
    CHECK( fav.invlets_for( "a" ) == "abc" );
}
