#include <sstream>

#include "catch/catch.hpp"
#include "game.h"
#include "map.h"
#include "options.h"
#include "player.h"
#include "map_helpers.h"

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

std::string location_desc( const inventory_location loc )
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

std::string move_action_desc( const int pos, const inventory_location from,
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

std::string invlet_state_desc( const invlet_state invstate )
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

std::string test_action_desc( const test_action action, const inventory_location from,
                              const inventory_location to,
                              const invlet_state first_invlet_state, const invlet_state second_invlet_state,
                              const invlet_state expected_first_invlet_state, const invlet_state expected_second_invlet_state,
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

void assign_invlet( player &p, item &it, const char invlet, const invlet_state invstate )
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

invlet_state check_invlet( player &p, item &it, const char invlet )
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

void drop_at_feet( player &p, const int pos )
{
    auto size_before = g->m.i_at( p.pos() ).size();
    p.moves = 100;
    p.drop( pos, p.pos() );
    p.activity.do_turn( p );
    REQUIRE( g->m.i_at( p.pos() ).size() == size_before + 1 );
}

void pick_up_from_feet( player &p, int pos )
{
    auto size_before = g->m.i_at( p.pos() ).size();
    REQUIRE( size_before > pos );
    p.moves = 100;
    p.assign_activity( activity_id( "ACT_PICKUP" ) );
    p.activity.placement = tripoint( 0, 0, 0 );
    p.activity.values.push_back( false );   // not from vehicle
    p.activity.values.push_back( pos );     // index of item to pick up
    p.activity.values.push_back( 0 );
    p.activity.do_turn( p );
    REQUIRE( g->m.i_at( p.pos() ).size() == size_before - 1 );
}

void wear_from_feet( player &p, int pos )
{
    auto size_before = g->m.i_at( p.pos() ).size();
    REQUIRE( size_before > pos );
    p.wear_item( g->m.i_at( p.pos() )[pos], false );
    g->m.i_rem( p.pos(), pos );
}

void wield_from_feet( player &p, int pos )
{
    auto size_before = g->m.i_at( p.pos() ).size();
    REQUIRE( size_before > pos );
    p.wield( g->m.i_at( p.pos() )[pos] );
    g->m.i_rem( p.pos(), pos );
}

void add_item( player &p, item &it, const inventory_location loc )
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
            if( p.weapon.is_null() ) {
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

item &item_at( player &p, const int pos, const inventory_location loc )
{
    switch( loc ) {
        case GROUND:
            return g->m.i_at( p.pos() )[pos];
        case INVENTORY:
            return p.i_at( pos );
        case WORN:
            return p.i_at( -2 - pos );
        case WIELDED_OR_WORN:
            return p.i_at( -1 - pos );
        default:
            FAIL( "unimplemented" );
            break;
    }
    return null_item_reference();
}

void move_item( player &p, const int pos, const inventory_location from,
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
                    pick_up_from_feet( p, pos );
                    break;
                case WORN:
                    wear_from_feet( p, pos );
                    break;
                case WIELDED_OR_WORN:
                    if( p.weapon.is_null() ) {
                        wield_from_feet( p, pos );
                    } else {
                        // since we can only wield one item, wear the item instead
                        wear_from_feet( p, pos );
                    }
                    break;
            }
            break;
        case INVENTORY:
            switch( to ) {
                case GROUND:
                    drop_at_feet( p, pos );
                    break;
                case INVENTORY:
                default:
                    FAIL( "unimplemented" );
                    break;
                case WORN:
                    p.wear( pos, false );
                    break;
                case WIELDED_OR_WORN:
                    if( p.weapon.is_null() ) {
                        p.wield( p.i_at( pos ) );
                    } else {
                        // since we can only wield one item, wear the item instead
                        p.wear( pos, false );
                    }
                    break;
            }
            break;
        case WORN:
            switch( to ) {
                case GROUND:
                    drop_at_feet( p, -2 - pos );
                    break;
                case INVENTORY:
                    p.takeoff( -2 - pos );
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
                    drop_at_feet( p, -1 - pos );
                    if( pos == 0 && !p.worn.empty() ) {
                        // wield the first worn item
                        p.wield( p.i_at( -2 ) );
                    }
                    break;
                case INVENTORY:
                    if( pos == 0 ) {
                        p.i_add( p.i_rem( -1 ) );
                    } else {
                        p.takeoff( -1 - pos );
                    }
                    if( pos == 0 && !p.worn.empty() ) {
                        // wield the first worn item
                        p.wield( p.i_at( -2 ) );
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

void invlet_test( player &dummy, const inventory_location from, const inventory_location to )
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

        // add the items to the starting position
        add_item( dummy, tshirt, to );
        add_item( dummy, jeans, to );

        // assign invlet to the first item
        assign_invlet( dummy, item_at( dummy, 0, to ), invlet, first_invlet_state );

        // remove the first item
        move_item( dummy, 0, to, from );

        // assign invlet to the second item
        assign_invlet( dummy, item_at( dummy, 0, to ), invlet, second_invlet_state );

        item *final_first = nullptr, *final_second = nullptr;
        switch( action ) {
            case REMOVE_1ST_REMOVE_2ND_ADD_1ST_ADD_2ND:
                move_item( dummy, 0, to, from );
                move_item( dummy, 0, from, to );
                move_item( dummy, 0, from, to );
                final_first = &item_at( dummy, 0, to );
                final_second = &item_at( dummy, 1, to );
                break;
            case REMOVE_1ST_REMOVE_2ND_ADD_2ND_ADD_1ST:
                move_item( dummy, 0, to, from );
                move_item( dummy, 1, from, to );
                move_item( dummy, 0, from, to );
                final_first = &item_at( dummy, 1, to );
                final_second = &item_at( dummy, 0, to );
                break;
            case REMOVE_1ST_ADD_1ST:
                move_item( dummy, 0, from, to );
                final_first = &item_at( dummy, 1, to );
                final_second = &item_at( dummy, 0, to );
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

void stack_invlet_test( player &dummy, inventory_location from, inventory_location to )
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
    item tshirt( "tshirt" );

    // add two such items to the starting position
    add_item( dummy, tshirt, from );
    add_item( dummy, tshirt, from );

    // assign the stack with invlet
    assign_invlet( dummy, item_at( dummy, 0, from ), invlet, CACHED );

    // wield or wear one of the items
    move_item( dummy, 0, from, to );

    std::stringstream ss;
    ss << "1. add a stack of two same items to " << location_desc( from ) << std::endl;
    ss << "2. assign the stack with an invlet" << std::endl;
    ss << "3. " << move_action_desc( 0, from, to ) << std::endl;
    ss << "expect the two items to have different invlets" << std::endl;
    ss << "actually the two items have " <<
       ( item_at( dummy, 0, to ).invlet != item_at( dummy, 0, from ).invlet ? "different" : "the same" ) <<
       " invlets" << std::endl;
    INFO( ss.str() );
    REQUIRE( item_at( dummy, 0, from ).typeId() == tshirt.typeId() );
    REQUIRE( item_at( dummy, 0, to ).typeId() == tshirt.typeId() );
    // the wielded/worn item should have different invlet from the remaining item
    CHECK( item_at( dummy, 0, to ).invlet != item_at( dummy, 0, from ).invlet );

    // clear invlets
    assign_invlet( dummy, item_at( dummy, 0, from ), invlet, NONE );
    assign_invlet( dummy, item_at( dummy, 0, to ), invlet, NONE );
}

void swap_invlet_test( player &dummy, inventory_location loc )
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

    // add the items
    add_item( dummy, tshirt1, loc );
    add_item( dummy, tshirt2, loc );

    // assign the items with invlets
    assign_invlet( dummy, item_at( dummy, 0, loc ), invlet_1, CACHED );
    assign_invlet( dummy, item_at( dummy, 1, loc ), invlet_2, CACHED );

    // swap the invlets (invoking twice to make the invlet non-player-assigned)
    dummy.reassign_item( item_at( dummy, 0, loc ), invlet_2 );
    dummy.reassign_item( item_at( dummy, 0, loc ), invlet_2 );

    // drop the items
    move_item( dummy, 0, loc, GROUND );
    move_item( dummy, 0, loc, GROUND );

    // get them again
    move_item( dummy, 0, GROUND, loc );
    move_item( dummy, 0, GROUND, loc );

    std::stringstream ss;
    ss << "1. add two items of the same type to " << location_desc( loc ) <<
       ", and ensure them do not stack" << std::endl;
    ss << "2. assign different invlets to the two items" << std::endl;
    ss << "3. swap the invlets by assign one of the items with the invlet of the other item" <<
       std::endl;
    ss << "4. move the items to " << location_desc( GROUND ) << std::endl;
    ss << "5. move the items to " << location_desc( loc ) << " again" << std::endl;
    ss << "expect the items to keep their swapped invlets" << std::endl;
    if( item_at( dummy, 0, loc ).invlet == invlet_2 && item_at( dummy, 1, loc ).invlet == invlet_1 ) {
        ss << "the items actually keep their swapped invlets" << std::endl;
    } else {
        ss << "the items actually does not keep their swapped invlets" << std::endl;
    }
    INFO( ss.str() );
    REQUIRE( item_at( dummy, 0, loc ).typeId() == tshirt1.typeId() );
    REQUIRE( item_at( dummy, 1, loc ).typeId() == tshirt2.typeId() );
    // invlets should not disappear and should still be swapped
    CHECK( item_at( dummy, 0, loc ).invlet == invlet_2 );
    CHECK( item_at( dummy, 1, loc ).invlet == invlet_1 );
    CHECK( check_invlet( dummy, item_at( dummy, 0, loc ), invlet_2 ) == CACHED );
    CHECK( check_invlet( dummy, item_at( dummy, 1, loc ), invlet_1 ) == CACHED );

    // clear invlets
    assign_invlet( dummy, item_at( dummy, 0, loc ), invlet_2, NONE );
    assign_invlet( dummy, item_at( dummy, 1, loc ), invlet_1, NONE );
}

void merge_invlet_test( player &dummy, inventory_location from )
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
        item tshirt( "tshirt" );

        // add the item
        add_item( dummy, tshirt, INVENTORY );
        add_item( dummy, tshirt, from );

        // assign the items with invlets
        assign_invlet( dummy, item_at( dummy, 0, INVENTORY ), invlet_1, first_invlet_state );
        assign_invlet( dummy, item_at( dummy, 0, from ), invlet_2, second_invlet_state );

        // merge the second item into inventory
        move_item( dummy, 0, from, INVENTORY );

        item &merged_item = item_at( dummy, 0, INVENTORY );
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
        REQUIRE( merged_item.typeId() == tshirt.typeId() );
        CHECK( merged_invlet_state == expected_merged_invlet_state );
        CHECK( merged_invlet == expected_merged_invlet );
    }
}

#define invlet_test_autoletter_off( name, dummy, from, to ) \
    SECTION( std::string( name ) + " (auto letter off)" ) { \
        get_options().get_option( "AUTO_INV_ASSIGN" ).setValue( "false" ); \
        invlet_test( dummy, from, to ); \
    }

#define stack_invlet_test_autoletter_off( name, dummy, from, to ) \
    SECTION( std::string( name ) + " (auto letter off)" ) { \
        get_options().get_option( "AUTO_INV_ASSIGN" ).setValue( "false" ); \
        stack_invlet_test( dummy, from, to ); \
    }

#define swap_invlet_test_autoletter_off( name, dummy, loc ) \
    SECTION( std::string( name ) + " (auto letter off)" ) { \
        get_options().get_option( "AUTO_INV_ASSIGN" ).setValue( "false" ); \
        swap_invlet_test( dummy, loc ); \
    }

#define merge_invlet_test_autoletter_off( name, dummy, from ) \
    SECTION( std::string( name ) + " (auto letter off)" ) { \
        get_options().get_option( "AUTO_INV_ASSIGN" ).setValue( "false" ); \
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

void verify_invlet_consistency( const invlet_favorites &fav )
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
    CHECK( fav.invlets_for( "a" ) == "" );
}

TEST_CASE( "invlet_favourites_removes_clashing_on_insertion", "[invlet]" )
{
    invlet_favorites fav;
    fav.set( 'a', "a" );
    verify_invlet_consistency( fav );
    CHECK( fav.invlets_for( "a" ) == "a" );
    CHECK( fav.invlets_for( "b" ) == "" );
    fav.set( 'a', "b" );
    verify_invlet_consistency( fav );
    CHECK( fav.invlets_for( "a" ) == "" );
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
