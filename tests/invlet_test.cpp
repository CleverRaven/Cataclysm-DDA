#include "catch/catch.hpp"

#include <iostream>
#include <sstream>
#include <functional>

#include "player.h"
#include "game.h"
#include "map.h"
#include "options.h"

const trait_id trait_debug_storage( "DEBUG_STORAGE" );

enum invlet_state {
    UNEXPECTED = -1,    // unexpected result
    NONE = 0,           // no invlet
    CACHED,             // non player-assigned invlet
    ASSIGNED,           // player-assigned invlet
};

const char *invlet_state_name( invlet_state invstate ) {
    static const char *const invlet_state_name_[] {
        "unexpected",
        "none",
        "cached",
        "assigned",
    };
    
    return invlet_state_name_[invstate + 1];
}

void assign_invlet( player &p, item &it, char invlet, invlet_state invstate ) {
    p.reassign_item( it, '\0' );
    switch( invstate ) {
    case NONE:
        break;
    case CACHED:
        p.reassign_item( it, invlet );
        p.reassign_item( it, invlet );
        break;
    case ASSIGNED:
    default:
        p.reassign_item( it, invlet );
        break;
    }
}

invlet_state check_invlet( player &p, item &it, char invlet ) {
    if( it.invlet == '\0' ) {
        return NONE;
    } else if( it.invlet == invlet ) {
        if( p.assigned_invlet.find( invlet ) != p.assigned_invlet.end() &&
                p.assigned_invlet[invlet] == it.typeId() ) {
            return ASSIGNED;
        } else {
            return CACHED;
        }
    }
    return UNEXPECTED;
}

void drop_at_feet( player &p, int pos ) {
    auto size_before = g->m.i_at( p.pos() ).size();
    p.moves = 100;
    p.drop( pos, p.pos() );
    p.activity.do_turn( &p );
    REQUIRE( g->m.i_at( p.pos() ).size() == size_before + 1 );
}

void wear_from_feet( player &p, int pos ) {
    auto size_before = g->m.i_at( p.pos() ).size();
    REQUIRE( size_before > pos );
    p.wear_item( g->m.i_at( p.pos() )[pos], false );
    g->m.i_rem( p.pos(), pos );
}

void wield_from_feet( player &p, int pos ) {
    auto size_before = g->m.i_at( p.pos() ).size();
    REQUIRE( size_before > pos );
    p.wield( g->m.i_at( p.pos() )[pos] );
    g->m.i_rem( p.pos(), pos );
}

void pick_up_from_feet( player &p, int pos ) {
    auto size_before = g->m.i_at( p.pos() ).size();
    REQUIRE( size_before > pos );
    p.moves = 100;
    p.assign_activity( activity_id( "ACT_PICKUP" ) );
    p.activity.placement = tripoint(0, 0, 0);
    p.activity.values.push_back( false );   // not from vehicle
    p.activity.values.push_back( pos );     // index of item to pick up
    p.activity.values.push_back( 0 );
    p.activity.do_turn( &p );
    REQUIRE( g->m.i_at( p.pos() ).size() == size_before - 1 );
}

std::string third_action_name( int action, std::function<std::string( int )> remove_action_name, std::function<std::string( int )> add_action_name ) {
    std::stringstream name;
    switch( action ) {
    case 0:
        name << remove_action_name( 1 ) << ", " << add_action_name( 0 ) << ", " << add_action_name( 1 );
        break;
    case 1:
        name << remove_action_name( 1 ) << ", " << add_action_name( 1 ) << ", " << add_action_name( 0 );
        break;
    case 2:
        name << add_action_name( 0 );
        break;
    default:
        name << "unknown";
        break;
    }
    return name.str();
}

void invlet_test( std::function<void( player &, item & )> item_add, std::function<item &( player &, int )> added_item,
                  std::function<void( player &, int, char, invlet_state )> assign_invlet,
                  std::function<void( player &, int )> remove_action, std::function<void( player &, int )> add_action,
                  std::function<std::string( int )> remove_action_name, std::function<std::string( int )> add_action_name,
                  player &dummy, const invlet_state expected_invlet_state[27][2] ) {
    constexpr char invlet = '|';

    std::cout << std::setw( 5 ) << "id" << std::setw( 15 ) << "set invlet 1" << std::setw( 10 ) << "" <<
                 std::setw( 15 ) << "set invlet 2" << std::setw( 30 ) << "" << std::setw( 20 ) << "final invlet 1" <<
                 std::setw( 20 ) << "final invlet 2" << std::setw( 10 ) << "failed?" << std::endl;

    for( int id = 0; id < 27; ++id ) {
        invlet_state first_invlet_state = invlet_state( id % 3 );
        invlet_state second_invlet_state = invlet_state( id / 3 % 3 );
        int third_action = id / 9 % 3;

        // Remove all items
        dummy.inv.clear();
        dummy.worn.clear();
        dummy.remove_weapon();
        g->m.i_clear( dummy.pos() );

        item tshirt( "tshirt" );
        item jeans( "jeans" );
        item_add( dummy, tshirt );
        item_add( dummy, jeans );

        // Assign invlet to the first item
        assign_invlet( dummy, 0, invlet, first_invlet_state );

        // Remove the first item
        remove_action( dummy, 0 );

        // Assign invlet to the second item
        assign_invlet( dummy, 0, invlet, second_invlet_state );

        item *final_first, *final_second;
        switch( third_action ) {
        case 0: // Remove second, add first, add second
            remove_action( dummy, 0 );
            add_action( dummy, 0 );
            add_action( dummy, 0 );
            final_first = &added_item( dummy, 0 );
            final_second = &added_item( dummy, 1 );
            break;
        case 1: // Remove second, add second, add first
            remove_action( dummy, 0 );
            add_action( dummy, 1 );
            add_action( dummy, 0 );
            final_first = &added_item( dummy, 1 );
            final_second = &added_item( dummy, 0 );
            break;
        case 2:
        default: // Add first
            add_action( dummy, 0 );
            final_first = &added_item( dummy, 1 );
            final_second = &added_item( dummy, 0 );
            break;
        }

        invlet_state first_invlet_state_now = check_invlet( dummy, *final_first, invlet ),
                     second_invlet_state_now = check_invlet( dummy, *final_second, invlet );

        std::cout << std::setw( 5 ) << id << std::setw( 15 ) << invlet_state_name( first_invlet_state ) << std::setw( 10 ) << remove_action_name( 0 ) <<
                     std::setw( 15 ) << invlet_state_name( second_invlet_state ) << std::setw( 30 ) << third_action_name( third_action, remove_action_name, add_action_name ) <<
                     std::setw( 20 ) << invlet_state_name( first_invlet_state_now ) << std::setw( 20 ) << invlet_state_name( second_invlet_state_now ) <<
                     std::setw( 10 ) << ( ( first_invlet_state_now == expected_invlet_state[id][0] &&
                                        second_invlet_state_now == expected_invlet_state[id][1] ) ?
                                        "success" : "failed" ) << std::endl;

        CHECK( first_invlet_state_now == expected_invlet_state[id][0] );
        CHECK( second_invlet_state_now == expected_invlet_state[id][1] );

        dummy.reassign_item( *final_first, '\0' );
        dummy.reassign_item( *final_second, '\0' );
    }
}

TEST_CASE( "Inventory letter test", "[invlet]" ) {
    /*
     *  expected[3rd,2nd,1st][item]
     *  1st:
     *      0       No invlet for the first item, then drop/take-off it
     *      1       Cached invlet for the first item, then drop/take-off it
     *      2       Assigned invlet for the first item, then drop/take-off it
     *  2nd:
     *      0       No invlet for the second item
     *      1       Cached invlet for the second item
     *      2       Assigned invlet for the second item
     *  3rd:
     *      0       Drop/take-off second, pickup/wear first, pickup/wear second
     *      1       Drop/take-off second, pickup/wear second, pickup/wear first
     *      2       Pickup/wear first
     */
    static const invlet_state expected_invlet_state[27][2] {
        {NONE    , NONE    },
        {CACHED  , NONE    },
        {ASSIGNED, NONE    },
        {NONE    , CACHED  },
        {NONE    , CACHED  },
        {NONE    , CACHED  },
        {NONE    , ASSIGNED},
        {NONE    , ASSIGNED},
        {NONE    , ASSIGNED},
        {NONE    , NONE    },
        {CACHED  , NONE    },
        {ASSIGNED, NONE    },
        {NONE    , CACHED  },
        {NONE    , CACHED  },
        {NONE    , CACHED  },
        {NONE    , ASSIGNED},
        {NONE    , ASSIGNED},
        {NONE    , ASSIGNED},
        {NONE    , NONE    },
        {CACHED  , NONE    },
        {ASSIGNED, NONE    },
        {NONE    , CACHED  },
        {NONE    , CACHED  },
        {NONE    , CACHED  },
        {NONE    , ASSIGNED},
        {NONE    , ASSIGNED},
        {NONE    , ASSIGNED},
    };

    player &dummy = g->u;
    const tripoint spot( 60, 60, 0 );
    dummy.setpos( spot );
    g->m.ter_set( spot, ter_id( "t_dirt" ) );
    g->m.furn_set( spot, furn_id( "f_null" ) );
    if( !dummy.has_trait( trait_debug_storage ) ) {
        dummy.set_mutation( trait_debug_storage );
    }

    SECTION( "Drop and wear (auto letter off)" ) {
        get_options().get_option( "AUTO_INV_ASSIGN" ).setValue( "false" );

        invlet_test( []( player &p, item &it ) {
            p.wear_item( it );
        }, []( player &p, int pos )->item & {
            return p.i_at( -2 - pos );
        }, []( player &p, int pos, char invlet, invlet_state state ) {
            item &it = p.i_at( -2 - pos );
            assign_invlet( p, it, invlet, state );
        }, []( player &p, int pos ) {
            drop_at_feet( p, -2 - pos );
        }, []( player &p, int pos ) {
            wear_from_feet( p, pos );
        }, []( int idx )->std::string {
            switch( idx ) {
            case 0:
                return "drop 1st";
                break;
            case 1:
                return "drop 2nd";
                break;
            default:
                return "drop unk";
                break;
            }
        }, []( int idx )->std::string {
            switch( idx ) {
            case 0:
                return "wear 1st";
                break;
            case 1:
                return "wear 2nd";
                break;
            default:
                return "wear unk";
                break;
            }
        }, dummy, expected_invlet_state );
    }
    
    SECTION( "Drop and pick-up (auto letter off)" ) {
        get_options().get_option( "AUTO_INV_ASSIGN" ).setValue( "false" );

        invlet_test( []( player &p, item &it ) {
            p.i_add( it );
        }, []( player &p, int pos )->item & {
            return p.i_at( pos );
        }, []( player &p, int pos, char invlet, invlet_state state ) {
            item &it = p.i_at( pos );
            assign_invlet( p, it, invlet, state );
        }, []( player &p, int pos ) {
            drop_at_feet( p, pos );
        }, []( player &p, int pos ) {
            pick_up_from_feet( p, pos );
        }, []( int idx )->std::string {
            switch( idx ) {
            case 0:
                return "drop 1st";
                break;
            case 1:
                return "drop 2nd";
                break;
            default:
                return "drop unk";
                break;
            }
        }, []( int idx )->std::string {
            switch( idx ) {
            case 0:
                return "pick 1st";
                break;
            case 1:
                return "pick 2nd";
                break;
            default:
                return "pick unk";
                break;
            }
        }, dummy, expected_invlet_state );
    }

    SECTION( "Drop and wield+wear (auto letter off)" ) {
        get_options().get_option( "AUTO_INV_ASSIGN" ).setValue( "false" );

        invlet_test( []( player &p, item &it ) {
            if( p.weapon.is_null() ) {
                p.wield( it );
            } else {
                p.wear_item( it );
            }
        }, []( player &p, int pos )->item & {
            return p.i_at( -1 - pos );
        }, []( player &p, int pos, char invlet, invlet_state state ) {
            item &it = p.i_at( -1 - pos );
            assign_invlet( p, it, invlet, state );
        }, []( player &p, int pos ) {
            drop_at_feet( p, -1 - pos );
            if( pos == 0 && !p.worn.empty() ) {
                p.wield( p.i_at( -2 ) );
            }
        }, []( player &p, int pos ) {
            if( p.weapon.is_null() ) {
                wield_from_feet( p, pos );
            } else {
                wear_from_feet( p, pos );
            }
        }, []( int idx )->std::string {
            switch( idx ) {
            case 0:
                return "drop 1st";
                break;
            case 1:
                return "drop 2nd";
                break;
            default:
                return "drop unk";
                break;
            }
        }, []( int idx )->std::string {
            switch( idx ) {
            case 0:
                return "get 1st";
                break;
            case 1:
                return "get 2nd";
                break;
            default:
                return "get unk";
                break;
            }
        }, dummy, expected_invlet_state );
    }

    SECTION( "Take-off and wear (auto letter off)" ) {
        get_options().get_option( "AUTO_INV_ASSIGN" ).setValue( "false" );

        invlet_test( []( player &p, item &it ) {
            p.wear_item( it );
        }, []( player &p, int pos )->item & {
            return p.i_at( -2 - pos );
        }, []( player &p, int pos, char invlet, invlet_state state ) {
            item &it = p.i_at( -2 - pos );
            assign_invlet( p, it, invlet, state );
        }, []( player &p, int pos ) {
            p.takeoff( -2 - pos );
        }, []( player &p, int pos ) {
            p.wear( pos, false );
        }, []( int idx )->std::string {
            switch( idx ) {
            case 0:
                return "tkoff 1st";
                break;
            case 1:
                return "tkoff 2nd";
                break;
            default:
                return "tkoff unk";
                break;
            }
        }, []( int idx )->std::string {
            switch( idx ) {
            case 0:
                return "wear 1st";
                break;
            case 1:
                return "wear 2nd";
                break;
            default:
                return "wear unk";
                break;
            }
        }, dummy, expected_invlet_state );
    }
}
