#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "activity_handlers.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "flag.h"
#include "item.h"
#include "item_contents.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "map_selector.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"

static const activity_id ACT_REPAIR_ITEM( "ACT_REPAIR_ITEM" );

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_can_drink( "can_drink" );
static const itype_id itype_canvas_patch( "canvas_patch" );
static const itype_id itype_leather( "leather" );
static const itype_id itype_tailors_kit( "tailors_kit" );
static const itype_id itype_technician_shirt_gray( "technician_shirt_gray" );
static const itype_id itype_test_baseball( "test_baseball" );
static const itype_id itype_test_baseball_half_degradation( "test_baseball_half_degradation" );
static const itype_id itype_test_baseball_x2_degradation( "test_baseball_x2_degradation" );
static const itype_id itype_test_glock_degrade( "test_glock_degrade" );
static const itype_id itype_test_steelball( "test_steelball" );
static const itype_id itype_thread( "thread" );

static const skill_id skill_mechanics( "mechanics" );
static const skill_id skill_tailor( "tailor" );

static constexpr int max_iters = 4000;
static constexpr tripoint_bub_ms spawn_pos( HALF_MAPSIZE_X - 1, HALF_MAPSIZE_Y, 0 );

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

TEST_CASE( "Damage_indicator_thresholds", "[item][damage_level]" )
{
    struct damage_level_threshold {
        int min_damage;
        int max_damage;
        std::string expect_indicator;
    };
    const std::vector<damage_level_threshold> thresholds {
        {    0,    0, "<color_c_green>++</color>" },
        {    1,  999, "<color_c_light_green>||</color>" },
        { 1000, 1999, "<color_c_yellow>|\\</color>" },
        { 2000, 2999, "<color_c_light_red>|.</color>" },
        { 3000, 3999, "<color_c_red>\\.</color>" },
        { 4000, 4000, "<color_c_dark_gray>XX</color>" },
    };
    item it( itype_test_baseball );
    CHECK( it.damage() == 0 );
    CHECK( it.degradation() == 0 );
    for( int dmg = 0; dmg <= it.type->damage_max(); dmg++ ) {
        it.set_damage( dmg );
        CHECK( it.damage() == dmg );
        CAPTURE( it.damage() );
        CAPTURE( it.damage_indicator() );
        bool matched_threshold = false;
        for( size_t i = 0; i < thresholds.size(); i++ ) {
            const damage_level_threshold &threshold = thresholds[i];
            if( threshold.min_damage <= dmg && dmg <= threshold.max_damage ) {
                CHECK( it.damage_indicator() == threshold.expect_indicator );
                CHECK( it.damage_level() == static_cast<int>( i ) );
                matched_threshold = true;
                break;
            }
        }
        CHECK( matched_threshold );
    }
}

TEST_CASE( "only_degrade_items_with_defined_degradation", "[item][degradation]" )
{
    // can be removed once all items degrade
    item no_degradation_item( itype_tailors_kit );
    CHECK( no_degradation_item.degradation() == 0 );
    no_degradation_item.set_degradation( 1000 );
    CHECK( no_degradation_item.degradation() == 0 );
    no_degradation_item.rand_degradation();
    CHECK( no_degradation_item.degradation() == 0 );
}

TEST_CASE( "Degradation_on_spawned_items", "[item][degradation]" )
{
    clear_map();

    SECTION( "Non-spawned items have no degradation" ) {
        item norm( itype_test_baseball );
        item half( itype_test_baseball_half_degradation );
        item doub( itype_test_baseball_x2_degradation );
        CHECK( norm.degradation() == 0 );
        CHECK( half.degradation() == 0 );
        CHECK( doub.degradation() == 0 );
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

    SECTION( "Items spawned with 3000 damage" ) {
        float norm = get_avg_degradation( itype_test_baseball, max_iters, 3000 );
        float half = get_avg_degradation( itype_test_baseball_half_degradation, max_iters, 3000 );
        float doub = get_avg_degradation( itype_test_baseball_x2_degradation, max_iters, 3000 );
        CHECK( norm == Approx( 1500.0 ).epsilon( 0.1 ) );
        CHECK( half == Approx( 750.0 ).epsilon( 0.1 ) );
        // not 3000.0 as expected - as degradation calculation gets clamped to max 4000
        CHECK( doub == Approx( 2750.0 ).epsilon( 0.1 ) );
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

TEST_CASE( "Items_that_get_damaged_gain_degradation", "[item][degradation]" )
{
    GIVEN( "An item with default degradation rate" ) {
        item it( itype_test_baseball );
        it.mod_damage( 0 );
        REQUIRE( it.type->degrade_increments() == 50 );
        REQUIRE( it.degradation() == 0 );
        REQUIRE( it.damage() == 0 );
        REQUIRE( it.damage_level() == 0 );

        WHEN( "Item loses 10 damage levels" ) {
            add_x_dmg_levels( it, 10 );
            THEN( "Item gains 10 percent as degradation, 2 damage level" ) {
                CHECK( it.degradation() == 1040 );
                CHECK( it.damage_level() == 2 );
                CHECK( it.damage() == it.degradation() );
            }
        }

        WHEN( "Item loses 20 damage levels" ) {
            add_x_dmg_levels( it, 20 );
            THEN( "Item gains 10 percent as degradation, 3 damage level" ) {
                CHECK( it.degradation() == 2000 );
                CHECK( it.damage_level() == 3 );
                CHECK( it.damage() == it.degradation() );
            }
        }
    }

    GIVEN( "An item with half degradation rate" ) {
        item it( itype_test_baseball_half_degradation );
        it.mod_damage( 0 );
        REQUIRE( it.type->degrade_increments() == 100 );
        REQUIRE( it.degradation() == 0 );
        REQUIRE( it.damage() == 0 );
        REQUIRE( it.damage_level() == 0 );

        WHEN( "Item loses 20 damage levels" ) {
            add_x_dmg_levels( it, 20 );
            THEN( "Item gains 5 percent as degradation, 1 damage level" ) {
                CHECK( it.degradation() == 960 );
                CHECK( it.damage_level() == 1 );
                CHECK( it.damage() == it.degradation() );
            }
        }
    }

    GIVEN( "An item with double degradation rate" ) {
        item it( itype_test_baseball_x2_degradation );
        REQUIRE( it.type->degrade_increments() == 25 );
        REQUIRE( it.degradation() == 0 );
        REQUIRE( it.damage() == 0 );
        REQUIRE( it.damage_level() == 0 );

        WHEN( "Item loses 10 damage levels" ) {
            add_x_dmg_levels( it, 10 );
            THEN( "Item gains 20 percent as degradation, 3 damage level" ) {
                CHECK( it.degradation() == 2080 );
                CHECK( it.damage_level() == 3 );
                CHECK( it.damage() == it.degradation() );
            }
        }

        WHEN( "Item loses 20 damage levels" ) {
            add_x_dmg_levels( it, 20 );
            THEN( "Item gains 20 percent as degradation, 4 damage level" ) {
                CHECK( it.degradation() == 3040 );
                CHECK( it.damage_level() == 4 );
                CHECK( it.damage() == it.degradation() );
            }
        }
    }
}

static item_location setup_tailorkit( Character &u )
{
    map &m = get_map();
    const tripoint_bub_ms pos = spawn_pos;
    item &thread = m.add_item_or_charges( pos, item( itype_thread ) );
    item &tailor = m.add_item_or_charges( pos, item( itype_tailors_kit ) );
    thread.charges = 400;
    tailor.reload( u, { map_cursor( pos ), &thread }, 400 );
    REQUIRE( m.i_at( spawn_pos ).begin()->typeId() == tailor.typeId() );
    return item_location( map_cursor( pos ), &tailor );
}

static void setup_repair( item &fix, player_activity &act, Character &u )
{
    // Setup character
    clear_character( u, true );
    u.set_skill_level( skill_tailor, 10 );
    u.wield( fix );
    REQUIRE( u.get_wielded_item()->typeId() == fix.typeId() );

    // Setup tool
    item_location tailorloc = setup_tailorkit( u );

    // Setup materials
    item leather( itype_leather );
    leather.charges = 10;
    u.i_add_or_drop( leather, 10 );

    // Setup activity
    item_location fixloc( u, &fix );
    act.values.emplace_back( /* repeat_type::FULL */ 3 );
    act.str_values.emplace_back( "repair_fabric" );
    act.targets.emplace_back( tailorloc );
    act.targets.emplace_back( fixloc );
}

// Testing activity_handlers::repair_item_finish / repair_item_actor::repair
TEST_CASE( "Repairing_degraded_items", "[item][degradation]" )
{
    // Setup map
    clear_map();
    set_time_to_day();
    REQUIRE( static_cast<int>( get_map().light_at( spawn_pos ) ) > 2 );

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

            THEN( "Can't repair to less than 1000 damage due to degradation" ) {
                player_activity act( ACT_REPAIR_ITEM, 0, 0 );
                setup_repair( fix, act, u );
                while( !act.is_null() ) {
                    ::repair_item_finish( &act, &u, true );
                }
                CHECK( fix.damage() == 1000 );
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
                CHECK( fix.damage() == 1000 );
            }
        }

        WHEN( "2000 damage / 2000 degradation" ) {
            fix.set_damage( 2000 );
            fix.set_degradation( 2000 );
            REQUIRE( fix.damage() == 2000 );
            REQUIRE( fix.degradation() == 2000 );

            THEN( "Can't repair to less than 2000 damage due to degradation" ) {
                player_activity act( ACT_REPAIR_ITEM, 0, 0 );
                setup_repair( fix, act, u );
                while( !act.is_null() ) {
                    ::repair_item_finish( &act, &u, true );
                }
                CHECK( fix.damage() == 2000 );
            }
        }

        WHEN( "3999 damage / 2000 degradation" ) {
            fix.set_damage( 3999 );
            fix.set_degradation( 2000 );
            REQUIRE( fix.damage() == 3999 );
            REQUIRE( fix.degradation() == 2000 );

            THEN( "Repaired to 2000 damage" ) {
                player_activity act( ACT_REPAIR_ITEM, 0, 0 );
                setup_repair( fix, act, u );
                while( !act.is_null() ) {
                    ::repair_item_finish( &act, &u, true );
                }
                CHECK( fix.damage() == 2000 );
            }
        }

        WHEN( "3000 damage / 3000 degradation" ) {
            fix.set_damage( 3000 );
            fix.set_degradation( 3000 );
            REQUIRE( fix.damage() == 3000 );
            REQUIRE( fix.degradation() == 3000 );

            THEN( "Can't repair to less than 3000 damage due to degradation" ) {
                player_activity act( ACT_REPAIR_ITEM, 0, 0 );
                setup_repair( fix, act, u );
                while( !act.is_null() ) {
                    ::repair_item_finish( &act, &u, true );
                }
                CHECK( fix.damage() == 3000 );
            }
        }

        WHEN( "3999 damage / 3000 degradation" ) {
            fix.set_damage( 3999 );
            fix.set_degradation( 3000 );
            REQUIRE( fix.damage() == 3999 );
            REQUIRE( fix.degradation() == 3000 );

            THEN( "Repaired to 3000 damage" ) {
                player_activity act( ACT_REPAIR_ITEM, 0, 0 );
                setup_repair( fix, act, u );
                while( !act.is_null() ) {
                    ::repair_item_finish( &act, &u, true );
                }
                CHECK( fix.damage() == 3000 );
            }
        }

        WHEN( "4000 damage / 4000 degradation" ) {
            fix.set_damage( 4000 );
            fix.set_degradation( 4000 );
            REQUIRE( fix.damage() == 4000 );
            REQUIRE( fix.degradation() == 4000 );

            THEN( "Can't repair to less than 4000 damage due to degradation" ) {
                player_activity act( ACT_REPAIR_ITEM, 0, 0 );
                setup_repair( fix, act, u );
                while( !act.is_null() ) {
                    ::repair_item_finish( &act, &u, true );
                }
                CHECK( fix.damage() == 4000 );
            }
        }
    }
}

// Testing activity_handlers::repair_item_finish / repair_item_actor::repair
TEST_CASE( "Repairing_items_with_specific_requirements", "[item][degradation]" )
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
TEST_CASE( "Gun_repair_with_degradation", "[item][degradation]" )
{
    GIVEN( "Gun with normal degradation" ) {
        Character &u = get_player_character();
        item gun( itype_test_glock_degrade );
        clear_character( u );
        u.set_skill_level( skill_mechanics, 10 );
        clear_map();
        set_time_to_day();

        WHEN( "0 damage / 0 degradation" ) {
            gun.set_damage( 0 );
            gun.set_degradation( 0 );
            REQUIRE( gun.damage() == 0 );
            REQUIRE( gun.degradation() == 0 );
            THEN( "Can't repair to less than 0 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 0 );
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
            THEN( "Repaired to 3000 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 3000 );
            }
        }

        WHEN( "0 damage / 1000 degradation" ) {
            gun.set_damage( 0 );
            gun.set_degradation( 1000 );
            REQUIRE( gun.damage() == 1000 );
            REQUIRE( gun.degradation() == 1000 );
            THEN( "Can't repair to less than 1000 damage due to degradation" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 1000 );
            }
        }

        WHEN( "1000 damage / 1000 degradation" ) {
            gun.set_damage( 1000 );
            gun.set_degradation( 1000 );
            REQUIRE( gun.damage() == 1000 );
            REQUIRE( gun.degradation() == 1000 );
            THEN( "Can't repair to less than 1000 damage due to degradation" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 1000 );
            }
        }

        WHEN( "4000 damage / 1000 degradation" ) {
            gun.set_damage( 4000 );
            gun.set_degradation( 1000 );
            REQUIRE( gun.damage() == 4000 );
            REQUIRE( gun.degradation() == 1000 );
            THEN( "Repaired to 3000 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 3000 );
            }
        }

        WHEN( "1000 damage / 2000 degradation" ) {
            gun.set_damage( 1000 );
            gun.set_degradation( 2000 );
            REQUIRE( gun.damage() == 2000 );
            REQUIRE( gun.degradation() == 2000 );
            THEN( "Can't repair to less than 2000 damage due to degradation" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 2000 );
            }
        }

        WHEN( "2000 damage / 2000 degradation" ) {
            gun.set_damage( 2000 );
            gun.set_degradation( 2000 );
            REQUIRE( gun.damage() == 2000 );
            REQUIRE( gun.degradation() == 2000 );
            THEN( "Can't repair to less than 2000 damage due to degradation" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 2000 );
            }
        }

        WHEN( "4000 damage / 2000 degradation" ) {
            gun.set_damage( 4000 );
            gun.set_degradation( 2000 );
            REQUIRE( gun.damage() == 4000 );
            REQUIRE( gun.degradation() == 2000 );
            THEN( "Repaired to 2000 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 2000 );
            }
        }

        WHEN( "2000 damage / 3000 degradation" ) {
            gun.set_damage( 2000 );
            gun.set_degradation( 3000 );
            REQUIRE( gun.damage() == 3000 );
            REQUIRE( gun.degradation() == 3000 );
            THEN( "Can't repair to less than 3000 damage due to degradation" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 3000 );
            }
        }

        WHEN( "3000 damage / 3000 degradation" ) {
            gun.set_damage( 3000 );
            gun.set_degradation( 3000 );
            REQUIRE( gun.damage() == 3000 );
            REQUIRE( gun.degradation() == 3000 );
            THEN( "Can't repair to less than 3000 damage due to degradation" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 3000 );
            }
        }

        WHEN( "4000 damage / 3000 degradation" ) {
            gun.set_damage( 4000 );
            gun.set_degradation( 3000 );
            REQUIRE( gun.damage() == 4000 );
            REQUIRE( gun.degradation() == 3000 );
            THEN( "Repaired to 3000 damage" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 3000 );
            }
        }

        WHEN( "3000 damage / 4000 degradation" ) {
            gun.set_damage( 3000 );
            gun.set_degradation( 4000 );
            REQUIRE( gun.damage() == 4000 );
            REQUIRE( gun.degradation() == 4000 );
            THEN( "Can't repair to less than 4000 damage due to degradation" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 4000 );
            }
        }

        WHEN( "4000 damage / 4000 degradation" ) {
            gun.set_damage( 4000 );
            gun.set_degradation( 4000 );
            REQUIRE( gun.damage() == 4000 );
            REQUIRE( gun.degradation() == 4000 );
            THEN( "Can't repair to less than 4000 damage due to degradation" ) {
                u.wield( gun );
                item_location gun_loc( u, &gun );
                ::gun_repair( &u, &gun, gun_loc );
                CHECK( gun.damage() == 4000 );
            }
        }
    }
}

static player_activity setup_repair_activity( item_location &tailorloc, item_location &fixloc )
{
    player_activity act( ACT_REPAIR_ITEM );
    act.values.emplace_back( /* repeat_type::FULL */ 3 );
    act.str_values.emplace_back( "repair_fabric" );
    act.targets.emplace_back( tailorloc );
    act.targets.emplace_back( fixloc );
    return act;
}

static item_location put_in_container( item_location &container, const itype_id &type )
{
    ret_val<item *> inserted = container->get_contents().insert_item( item( type ),
                               pocket_type::CONTAINER );
    REQUIRE( inserted.success() );
    return item_location( container, inserted.value() );
}

// Reproduce previous segfault from https://github.com/CleverRaven/Cataclysm-DDA/issues/74254
TEST_CASE( "refit_item_inside_spillable_container", "[item][repair][container]" )
{
    clear_avatar();
    clear_map();
    set_time_to_day();
    REQUIRE( static_cast<int>( get_map().light_at( spawn_pos ) ) > 2 );

    Character &u = get_player_character();
    u.set_skill_level( skill_tailor, 10 );

    // Setup starting equipment
    item_location tailorkit = setup_tailorkit( u );
    REQUIRE( u.wear_item( item( itype_backpack ) ) );
    item_location backpack = u.top_items_loc().front();
    item canvas_patch( itype_canvas_patch );
    u.i_add_or_drop( canvas_patch, 100 );

    // Starting inventory looks like:
    //   backpack >
    //     100 canvas patch
    //     aluminum can >
    //       work t-shirt (poor fit)
    // It's a bit odd that we can put the shirt into the aluminum can, since it
    // would normally reject that with a message stating that it would spill.
    GIVEN( "backpack > aluminum can > work t-shirt (poor fit)" ) {
        item_location aluminum_can = put_in_container( backpack, itype_can_drink );
        item_location fix_tshirt = put_in_container( aluminum_can, itype_technician_shirt_gray );
        WHEN( "Refitting tshirt inside spillable container" ) {
            REQUIRE_FALSE( fix_tshirt->has_flag( flag_FIT ) );
            player_activity act = setup_repair_activity( tailorkit, fix_tshirt );
            while( !act.is_null() ) {
                ::repair_item_finish( &act, &u, true );
            }
            THEN( "tshirt should be refitted successfully" ) {
                const item *refitted_tshirt = backpack->get_item_with( [&]( const item & i ) {
                    return i.typeId() == itype_technician_shirt_gray;
                } );
                REQUIRE( refitted_tshirt != nullptr );
                CHECK( refitted_tshirt->has_flag( flag_FIT ) );
            }
        }

    }
}
