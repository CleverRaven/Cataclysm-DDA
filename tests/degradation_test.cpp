#include <string>

#include "activity_handlers.h"
#include "cata_catch.h"
#include "item.h"
#include "itype.h"
#include "iuse_actor.h"
#include "iuse.h"
#include "map_helpers.h"
#include "map.h"
#include "player_helpers.h"
#include "vehicle.h"
#include "veh_utils.h"
#include "veh_type.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const activity_id ACT_REPAIR_ITEM( "ACT_REPAIR_ITEM" );

static const itype_id itype_leather( "leather" );
static const itype_id itype_tailors_kit( "tailors_kit" );
static const itype_id itype_test_baseball( "test_baseball" );
static const itype_id itype_test_baseball_half_degradation( "test_baseball_half_degradation" );
static const itype_id itype_test_baseball_x2_degradation( "test_baseball_x2_degradation" );
static const itype_id itype_test_glock_degrade( "test_glock_degrade" );
static const itype_id itype_test_steelball( "test_steelball" );
static const itype_id itype_thread( "thread" );

static const skill_id skill_mechanics( "mechanics" );
static const skill_id skill_tailor( "tailor" );

static constexpr int max_iters = 4000;
static constexpr tripoint spawn_pos( HALF_MAPSIZE_X - 1, HALF_MAPSIZE_Y, 0 );

static float get_avg_degradation( const itype_id &it, int count, int damage )
{
    if( count <= 0 ) {
        return 0.f;
    }
    map &m = get_map();
    m.spawn_item( spawn_pos, it, count, 0, calendar::turn, damage );
    REQUIRE( m.i_at( spawn_pos ).size() == static_cast<unsigned>( count ) );
    float deg = 0.f;
    for( const item &it : m.i_at( spawn_pos ) ) {
        deg += it.degradation();
    }
    m.i_clear( spawn_pos );
    return deg / count;
}

TEST_CASE( "Degradation on spawned items", "[item][degradation]" )
{
    SECTION( "Non-spawned items have no degradation" ) {
        item norm( itype_test_baseball );
        item half( itype_test_baseball_half_degradation );
        item doub( itype_test_baseball_x2_degradation );
        CHECK( norm.degradation() == 0 );
        CHECK( half.degradation() == 0 );
        CHECK( doub.degradation() == 0 );
    }

    SECTION( "Items spawned with -1000 damage" ) {
        float norm = get_avg_degradation( itype_test_baseball, max_iters, -1000 );
        float half = get_avg_degradation( itype_test_baseball_half_degradation, max_iters, -1000 );
        float doub = get_avg_degradation( itype_test_baseball_x2_degradation, max_iters, -1000 );
        CHECK( norm == Approx( 0.0 ).epsilon( 0.001 ) );
        CHECK( half == Approx( 0.0 ).epsilon( 0.001 ) );
        CHECK( doub == Approx( 0.0 ).epsilon( 0.001 ) );
    }

    SECTION( "Items spawned with 0 damage" ) {
        float norm = get_avg_degradation( itype_test_baseball, max_iters, 0 );
        float half = get_avg_degradation( itype_test_baseball_half_degradation, max_iters, 0 );
        float doub = get_avg_degradation( itype_test_baseball_x2_degradation, max_iters, 0 );
        CHECK( norm == Approx( 0.0 ).epsilon( 0.001 ) );
        CHECK( half == Approx( 0.0 ).epsilon( 0.001 ) );
        CHECK( doub == Approx( 0.0 ).epsilon( 0.001 ) );
    }

    SECTION( "Items spawned with 2000 damage" ) {
        float norm = get_avg_degradation( itype_test_baseball, max_iters, 2000 );
        float half = get_avg_degradation( itype_test_baseball_half_degradation, max_iters, 2000 );
        float doub = get_avg_degradation( itype_test_baseball_x2_degradation, max_iters, 2000 );
        CHECK( norm == Approx( 1000.0 ).epsilon( 0.1 ) );
        CHECK( half == Approx( 500.0 ).epsilon( 0.1 ) );
        CHECK( doub == Approx( 2000.0 ).epsilon( 0.1 ) );
    }

    SECTION( "Items spawned with 4000 damage" ) {
        float norm = get_avg_degradation( itype_test_baseball, max_iters, 4000 );
        float half = get_avg_degradation( itype_test_baseball_half_degradation, max_iters, 4000 );
        float doub = get_avg_degradation( itype_test_baseball_x2_degradation, max_iters, 4000 );
        CHECK( norm == Approx( 2000.0 ).epsilon( 0.1 ) );
        CHECK( half == Approx( 1000.0 ).epsilon( 0.1 ) );
        CHECK( doub == Approx( 4000.0 ).epsilon( 0.1 ) );
    }
}

static void add_x_dmg_levels( item &it, int lvls )
{
    it.mod_damage( -5000 );
    for( int i = 0; i < lvls; i++ ) {
        it.mod_damage( 1000 );
        it.mod_damage( -1000 );
    }
}

TEST_CASE( "Items that get damaged gain degradation", "[item][degradation]" )
{
    GIVEN( "An item with default degradation rate" ) {
        item it( itype_test_baseball );
        it.mod_damage( -1000 );
        REQUIRE( it.degrade_increments() == 50 );
        REQUIRE( it.degradation() == 0 );
        REQUIRE( it.damage() == -1000 );
        REQUIRE( it.damage_level() == -1 );

        WHEN( "Item loses 2 damage levels" ) {
            add_x_dmg_levels( it, 2 );
            THEN( "Item gains 10 percent as degradation, -1 damage level" ) {
                CHECK( it.degradation() == 200 );
                CHECK( it.damage_level() == -1 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }

        WHEN( "Item loses 10 damage levels" ) {
            add_x_dmg_levels( it, 10 );
            THEN( "Item gains 10 percent as degradation, 0 damage level" ) {
                CHECK( it.degradation() == 1000 );
                CHECK( it.damage_level() == 0 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }

        WHEN( "Item loses 20 damage levels" ) {
            add_x_dmg_levels( it, 20 );
            THEN( "Item gains 10 percent as degradation, 1 damage level" ) {
                CHECK( it.degradation() == 2000 );
                CHECK( it.damage_level() == 1 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }
    }

    GIVEN( "An item with half degradation rate" ) {
        item it( itype_test_baseball_half_degradation );
        it.mod_damage( -1000 );
        REQUIRE( it.degrade_increments() == 100 );
        REQUIRE( it.degradation() == 0 );
        REQUIRE( it.damage() == -1000 );
        REQUIRE( it.damage_level() == -1 );

        WHEN( "Item loses 2 damage levels" ) {
            add_x_dmg_levels( it, 2 );
            THEN( "Item gains 5 percent as degradation, -1 damage level" ) {
                CHECK( it.degradation() == 100 );
                CHECK( it.damage_level() == -1 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }

        WHEN( "Item loses 10 damage levels" ) {
            add_x_dmg_levels( it, 10 );
            THEN( "Item gains 5 percent as degradation, -1 damage level" ) {
                CHECK( it.degradation() == 500 );
                CHECK( it.damage_level() == -1 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }

        WHEN( "Item loses 20 damage levels" ) {
            add_x_dmg_levels( it, 20 );
            THEN( "Item gains 5 percent as degradation, 0 damage level" ) {
                CHECK( it.degradation() == 1000 );
                CHECK( it.damage_level() == 0 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }
    }

    GIVEN( "An item with double degradation rate" ) {
        item it( itype_test_baseball_x2_degradation );
        it.mod_damage( -1000 );
        REQUIRE( it.degrade_increments() == 25 );
        REQUIRE( it.degradation() == 0 );
        REQUIRE( it.damage() == -1000 );
        REQUIRE( it.damage_level() == -1 );

        WHEN( "Item loses 2 damage levels" ) {
            add_x_dmg_levels( it, 2 );
            THEN( "Item gains 20 percent as degradation, -1 damage level" ) {
                CHECK( it.degradation() == 400 );
                CHECK( it.damage_level() == -1 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }

        WHEN( "Item loses 10 damage levels" ) {
            add_x_dmg_levels( it, 10 );
            THEN( "Item gains 20 percent as degradation, 1 damage level" ) {
                CHECK( it.degradation() == 2000 );
                CHECK( it.damage_level() == 1 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }

        WHEN( "Item loses 20 damage levels" ) {
            add_x_dmg_levels( it, 20 );
            THEN( "Item gains 20 percent as degradation, 3 damage level" ) {
                CHECK( it.degradation() == 4000 );
                CHECK( it.damage_level() == 3 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }
    }
}

static void setup_repair( item &fix, player_activity &act, Character &u )
{
    // Setup map
    map &m = get_map();
    set_time( calendar::turn_zero + 12_hours );
    REQUIRE( static_cast<int>( m.light_at( spawn_pos ) ) > 2 );

    // Setup character
    clear_character( u, true );
    u.set_skill_level( skill_tailor, 10 );
    u.wield( fix );
    REQUIRE( u.get_wielded_item()->typeId() == fix.typeId() );

    // Setup tool
    item &thread = m.add_item_or_charges( spawn_pos, item( itype_thread ) );
    item &tailor = m.add_item_or_charges( spawn_pos, item( itype_tailors_kit ) );
    thread.charges = 400;
    tailor.reload( u, { map_cursor( spawn_pos ), &thread }, 400 );
    REQUIRE( m.i_at( spawn_pos ).begin()->typeId() == tailor.typeId() );

    // Setup materials
    item leather( itype_leather );
    leather.charges = 10;
    u.i_add_or_drop( leather, 10 );

    // Setup activity
    item_location fixloc( u, &fix );
    item_location tailorloc( map_cursor( spawn_pos ), &tailor );
    act.values.emplace_back( 3 );
    act.str_values.emplace_back( "repair_fabric" );
    act.targets.emplace_back( tailorloc );
    act.targets.emplace_back( fixloc );
}

// Testing activity_handlers::repair_item_finish / repair_item_actor::repair
TEST_CASE( "Repairing degraded items", "[item][degradation]" )
{
    GIVEN( "Item with normal degradation" ) {
        Character &u = get_player_character();
        item fix( itype_test_baseball );

        WHEN( "1000 damage / 0 degradation" ) {
            fix.set_damage( 1000 );
            fix.set_degradation( 0 );
            REQUIRE( fix.damage() == 1000 );
            REQUIRE( fix.degradation() == 0 );

            THEN( "Repaired to 0 damage" ) {
                player_activity act( ACT_REPAIR_ITEM, 0, 0 );
                setup_repair( fix, act, u );
                while( !act.is_null() ) {
                    ::repair_item_finish( &act, &u, true );
                }
                CHECK( fix.damage() == 0 );
            }
        }

        WHEN( "3999 damage / 0 degradation" ) {
            fix.set_damage( 3999 );
            fix.set_degradation( 0 );
            REQUIRE( fix.damage() == 3999 );
            REQUIRE( fix.degradation() == 0 );

            THEN( "Repaired to 0 damage" ) {
                player_activity act( ACT_REPAIR_ITEM, 0, 0 );
                setup_repair( fix, act, u );
                while( !act.is_null() ) {
                    ::repair_item_finish( &act, &u, true );
                }
                CHECK( fix.damage() == 0 );
            }
        }

        WHEN( "1000 damage / 1000 degradation" ) {
            fix.set_damage( 1000 );
            fix.set_degradation( 1000 );
            REQUIRE( fix.damage() == 1000 );
            REQUIRE( fix.degradation() == 1000 );

            THEN( "Repaired to 0 damage" ) {
                player_activity act( ACT_REPAIR_ITEM, 0, 0 );
                setup_repair( fix, act, u );
                while( !act.is_null() ) {
                    ::repair_item_finish( &act, &u, true );
                }
                CHECK( fix.damage() == 0 );
            }
        }

        WHEN( "3999 damage / 1000 degradation" ) {
            fix.set_damage( 3999 );
            fix.set_degradation( 1000 );
            REQUIRE( fix.damage() == 3999 );
            REQUIRE( fix.degradation() == 1000 );

            THEN( "Repaired to 0 damage" ) {
                player_activity act( ACT_REPAIR_ITEM, 0, 0 );
                setup_repair( fix, act, u );
                while( !act.is_null() ) {
                    ::repair_item_finish( &act, &u, true );
                }
                CHECK( fix.damage() == 0 );
            }
        }

        WHEN( "2000 damage / 2000 degradation" ) {
            fix.set_damage( 2000 );
            fix.set_degradation( 2000 );
            REQUIRE( fix.damage() == 2000 );
            REQUIRE( fix.degradation() == 2000 );

            THEN( "Repaired to 1000 damage" ) {
                player_activity act( ACT_REPAIR_ITEM, 0, 0 );
                setup_repair( fix, act, u );
                while( !act.is_null() ) {
                    ::repair_item_finish( &act, &u, true );
                }
                CHECK( fix.damage() == 1000 );
            }
        }

        WHEN( "3999 damage / 2000 degradation" ) {
            fix.set_damage( 3999 );
            fix.set_degradation( 2000 );
            REQUIRE( fix.damage() == 3999 );
            REQUIRE( fix.degradation() == 2000 );

            THEN( "Repaired to 1000 damage" ) {
                player_activity act( ACT_REPAIR_ITEM, 0, 0 );
                setup_repair( fix, act, u );
                while( !act.is_null() ) {
                    ::repair_item_finish( &act, &u, true );
                }
                CHECK( fix.damage() == 1000 );
            }
        }

        WHEN( "3000 damage / 3000 degradation" ) {
            fix.set_damage( 3000 );
            fix.set_degradation( 3000 );
            REQUIRE( fix.damage() == 3000 );
            REQUIRE( fix.degradation() == 3000 );

            THEN( "Repaired to 2000 damage" ) {
                player_activity act( ACT_REPAIR_ITEM, 0, 0 );
                setup_repair( fix, act, u );
                while( !act.is_null() ) {
                    ::repair_item_finish( &act, &u, true );
                }
                CHECK( fix.damage() == 2000 );
            }
        }

        WHEN( "3999 damage / 3000 degradation" ) {
            fix.set_damage( 3999 );
            fix.set_degradation( 3000 );
            REQUIRE( fix.damage() == 3999 );
            REQUIRE( fix.degradation() == 3000 );

            THEN( "Repaired to 2000 damage" ) {
                player_activity act( ACT_REPAIR_ITEM, 0, 0 );
                setup_repair( fix, act, u );
                while( !act.is_null() ) {
                    ::repair_item_finish( &act, &u, true );
                }
                CHECK( fix.damage() == 2000 );
            }
        }

        WHEN( "4000 damage / 4000 degradation" ) {
            fix.set_damage( 4000 );
            fix.set_degradation( 4000 );
            REQUIRE( fix.damage() == 4000 );
            REQUIRE( fix.degradation() == 4000 );

            THEN( "Repaired to 3000 damage" ) {
                player_activity act( ACT_REPAIR_ITEM, 0, 0 );
                setup_repair( fix, act, u );
                while( !act.is_null() ) {
                    ::repair_item_finish( &act, &u, true );
                }
                CHECK( fix.damage() == 3000 );
            }
        }
    }
}

// Testing activity_handlers::repair_item_finish / repair_item_actor::repair
TEST_CASE( "Repairing items with specific requirements", "[item][degradation]" )
{
    Character &u = get_player_character();
    item fix( itype_test_steelball );

    fix.set_damage( 1000 );
    fix.set_degradation( 0 );
    REQUIRE( fix.damage() == 1000 );
    REQUIRE( fix.degradation() == 0 );

    THEN( "Try repairing and fail" ) {
        player_activity act( ACT_REPAIR_ITEM, 0, 0 );
        setup_repair( fix, act, u );
        while( !act.is_null() ) {
            ::repair_item_finish( &act, &u, true );
        }

        // shouldn't repair since the sewing kit can't repair steel ball
        CHECK( fix.damage() == 1000 );
    }
}

// Testing iuse::gun_repair
TEST_CASE( "Gun repair with degradation", "[item][degradation]" )
{
    GIVEN( "Gun with normal degradation" ) {
        Character &u = get_player_character();
        item gun( itype_test_glock_degrade );
        clear_character( u );
        u.set_skill_level( skill_mechanics, 10 );

        WHEN( "0 damage / 0 degradation" ) {
            gun.set_damage( 0 );
            gun.set_degradation( 0 );
            REQUIRE( gun.damage() == 0 );
            REQUIRE( gun.degradation() == 0 );
            THEN( "Repaired to -1000 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == -1000 );
            }
        }

        WHEN( "1000 damage / 0 degradation" ) {
            gun.set_damage( 1000 );
            gun.set_degradation( 0 );
            REQUIRE( gun.damage() == 1000 );
            REQUIRE( gun.degradation() == 0 );
            THEN( "Repaired to 0 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 0 );
            }
        }

        WHEN( "4000 damage / 0 degradation" ) {
            gun.set_damage( 4000 );
            gun.set_degradation( 0 );
            REQUIRE( gun.damage() == 4000 );
            REQUIRE( gun.degradation() == 0 );
            THEN( "Repaired to 3999 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 3999 );
            }
        }

        WHEN( "0 damage / 1000 degradation" ) {
            gun.set_damage( 0 );
            gun.set_degradation( 1000 );
            REQUIRE( gun.damage() == 0 );
            REQUIRE( gun.degradation() == 1000 );
            THEN( "Repaired to 0 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 0 );
            }
        }

        WHEN( "1000 damage / 1000 degradation" ) {
            gun.set_damage( 1000 );
            gun.set_degradation( 1000 );
            REQUIRE( gun.damage() == 1000 );
            REQUIRE( gun.degradation() == 1000 );
            THEN( "Repaired to 0 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 0 );
            }
        }

        WHEN( "4000 damage / 1000 degradation" ) {
            gun.set_damage( 4000 );
            gun.set_degradation( 1000 );
            REQUIRE( gun.damage() == 4000 );
            REQUIRE( gun.degradation() == 1000 );
            THEN( "Repaired to 3999 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 3999 );
            }
        }

        WHEN( "1000 damage / 2000 degradation" ) {
            gun.set_damage( 1000 );
            gun.set_degradation( 2000 );
            REQUIRE( gun.damage() == 1000 );
            REQUIRE( gun.degradation() == 2000 );
            THEN( "Repaired to 1000 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 1000 );
            }
        }

        WHEN( "2000 damage / 2000 degradation" ) {
            gun.set_damage( 2000 );
            gun.set_degradation( 2000 );
            REQUIRE( gun.damage() == 2000 );
            REQUIRE( gun.degradation() == 2000 );
            THEN( "Repaired to 1333 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 1333 );
            }
        }

        WHEN( "4000 damage / 2000 degradation" ) {
            gun.set_damage( 4000 );
            gun.set_degradation( 2000 );
            REQUIRE( gun.damage() == 4000 );
            REQUIRE( gun.degradation() == 2000 );
            THEN( "Repaired to 3999 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 3999 );
            }
        }

        WHEN( "2000 damage / 3000 degradation" ) {
            gun.set_damage( 2000 );
            gun.set_degradation( 3000 );
            REQUIRE( gun.damage() == 2000 );
            REQUIRE( gun.degradation() == 3000 );
            THEN( "Repaired to 2000 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 2000 );
            }
        }

        WHEN( "3000 damage / 3000 degradation" ) {
            gun.set_damage( 3000 );
            gun.set_degradation( 3000 );
            REQUIRE( gun.damage() == 3000 );
            REQUIRE( gun.degradation() == 3000 );
            THEN( "Repaired to 2666 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 2666 );
            }
        }

        WHEN( "4000 damage / 3000 degradation" ) {
            gun.set_damage( 4000 );
            gun.set_degradation( 3000 );
            REQUIRE( gun.damage() == 4000 );
            REQUIRE( gun.degradation() == 3000 );
            THEN( "Repaired to 3999 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 3999 );
            }
        }

        WHEN( "3000 damage / 4000 degradation" ) {
            gun.set_damage( 3000 );
            gun.set_degradation( 4000 );
            REQUIRE( gun.damage() == 3000 );
            REQUIRE( gun.degradation() == 4000 );
            THEN( "Repaired to 3000 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 3000 );
            }
        }

        WHEN( "4000 damage / 4000 degradation" ) {
            gun.set_damage( 4000 );
            gun.set_degradation( 4000 );
            REQUIRE( gun.damage() == 4000 );
            REQUIRE( gun.degradation() == 4000 );
            THEN( "Repaired to 3999 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 3999 );
            }
        }
    }
}
