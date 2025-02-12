#include <iosfwd>
#include <limits>
#include <vector>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "cata_catch.h"
#include "clzones.h"
#include "item.h"
#include "item_category.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"

static const activity_id ACT_MOVE_LOOT( "ACT_MOVE_LOOT" );

static const faction_id faction_your_followers( "your_followers" );

static const itype_id itype_556( "556" );
static const itype_id itype_ammolink223( "ammolink223" );
static const itype_id itype_belt223( "belt223" );
static const itype_id itype_test_apple( "test_apple" );
static const itype_id itype_test_bitter_almond( "test_bitter_almond" );
static const itype_id itype_test_milk( "test_milk" );
static const itype_id
itype_test_watertight_open_sealed_container_250ml( "test_watertight_open_sealed_container_250ml" );
static const itype_id itype_test_wine( "test_wine" );

static const vproto_id vehicle_prototype_shopping_cart( "shopping_cart" );

static const zone_type_id zone_type_LOOT_AMMO( "LOOT_AMMO" );
static const zone_type_id zone_type_LOOT_DEFAULT( "LOOT_DEFAULT" );
static const zone_type_id zone_type_LOOT_DRINK( "LOOT_DRINK" );
static const zone_type_id zone_type_LOOT_FOOD( "LOOT_FOOD" );
static const zone_type_id zone_type_LOOT_PDRINK( "LOOT_PDRINK" );
static const zone_type_id zone_type_LOOT_PFOOD( "LOOT_PFOOD" );
static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );
static const zone_type_id zone_type_UNLOAD_ALL( "UNLOAD_ALL" );

namespace
{
template <class T>
int _count_items_or_charges( const T &items, const itype_id &id )
{
    int n = 0;
    for( const item &it : items ) {
        if( it.typeId() == id ) {
            n += it.count();
        }
    }
    return n;
}

int count_items_or_charges( const tripoint_bub_ms src, const itype_id &id,
                            const std::optional<vpart_reference> &vp )
{
    if( vp ) {
        return _count_items_or_charges( vp->vehicle().get_items( vp->part() ), id );
    }
    return _count_items_or_charges( get_map().i_at( src ), id );
}

void create_tile_zone( const std::string &name, const zone_type_id &zone_type, tripoint_abs_ms pos,
                       bool veh = false )
{
    zone_manager &zm = zone_manager::get_manager();
    zm.add( name, zone_type, faction_your_followers, false, true, pos, pos, nullptr, veh );
}

} // namespace

TEST_CASE( "zone_unloading_ammo_belts", "[zones][items][ammo_belt][activities][unload]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();
    std::optional<vpart_reference> vp;
    bool const in_vehicle = GENERATE( false, true );
    CAPTURE( in_vehicle );

    clear_avatar();
    clear_map();

    tripoint_abs_ms const start = here.get_abs( tripoint_bub_ms::zero + tripoint::east );
    bool const move_act = GENERATE( true, false );
    dummy.set_pos_abs_only( start );

    if( in_vehicle ) {
        REQUIRE( here.add_vehicle( vehicle_prototype_shopping_cart, tripoint_bub_ms::zero + tripoint::east,
                                   0_degrees, 0, 0 ) );
        vp = here.veh_at( start ).cargo();
        REQUIRE( vp );
        vp->vehicle().set_owner( dummy );
    }

    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start, in_vehicle );
    create_tile_zone( "Unload All", zone_type_UNLOAD_ALL, start, in_vehicle );

    item ammo_belt = item( itype_belt223, calendar::turn );
    ammo_belt.ammo_set( ammo_belt.ammo_default() );
    int belt_ammo_count_before_unload = ammo_belt.ammo_remaining();

    REQUIRE( belt_ammo_count_before_unload > 0 );

    WHEN( "unloading ammo belts using UNLOAD_ALL " ) {
        if( in_vehicle ) {
            vp->vehicle().add_item( here, vp->part(), ammo_belt );
        } else {
            here.add_item_or_charges( tripoint_bub_ms( tripoint::east ), ammo_belt );
        }
        if( move_act ) {
            dummy.assign_activity( player_activity( ACT_MOVE_LOOT ) );
        } else {
            dummy.assign_activity( unload_loot_activity_actor() );
        }
        CAPTURE( dummy.activity.id() );
        process_activity( dummy );

        THEN( "check that the ammo and linkages are both unloaded and the ammo belt is removed" ) {
            CHECK( count_items_or_charges( tripoint_bub_ms::zero + tripoint::east, itype_belt223, vp ) == 0 );
            CHECK( count_items_or_charges( tripoint_bub_ms::zero + tripoint::east,
                                           itype_ammolink223, vp ) == belt_ammo_count_before_unload );
            CHECK( count_items_or_charges( tripoint_bub_ms::zero + tripoint::east, itype_556,
                                           vp ) == belt_ammo_count_before_unload );
        }
    }
}

// Comestibles sorting is a bit awkward. Unlike other loot, they're almost
// always inside of a container, and their sort zone changes based on their
// shelf life and whether the container prevents rotting.
TEST_CASE( "zone_sorting_comestibles_", "[zones][items][food][activities]" )
{
    clear_map();
    zone_manager &zm = zone_manager::get_manager();

    const tripoint_abs_ms origin_pos;
    create_tile_zone( "Food", zone_type_LOOT_FOOD, tripoint_abs_ms::zero + tripoint::east );
    create_tile_zone( "Drink", zone_type_LOOT_DRINK, tripoint_abs_ms::zero + tripoint::west );

    SECTION( "without perishable zones" ) {
        GIVEN( "a non-perishable food" ) {
            item nonperishable_food( itype_test_bitter_almond );
            REQUIRE_FALSE( nonperishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( nonperishable_food, origin_pos ).id == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a non-perishable drink" ) {
            item nonperishable_drink( itype_test_wine );
            REQUIRE_FALSE( nonperishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( nonperishable_drink,
                                                           origin_pos ).id == zone_type_LOOT_DRINK );
                }
            }
        }

        GIVEN( "a perishable food" ) {
            item perishable_food( itype_test_apple );
            REQUIRE( perishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( perishable_food, origin_pos ).id == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a perishable drink" ) {
            item perishable_drink( itype_test_milk );
            REQUIRE( perishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( perishable_drink, origin_pos ).id == zone_type_LOOT_DRINK );
                }
            }
        }
    }

    SECTION( "with perishable zones" ) {
        create_tile_zone( "PFood", zone_type_LOOT_PFOOD, tripoint_abs_ms::zero + tripoint::north );
        create_tile_zone( "PDrink", zone_type_LOOT_PDRINK, tripoint_abs_ms::zero + tripoint::south );

        GIVEN( "a non-perishable food" ) {
            item nonperishable_food( itype_test_bitter_almond );
            REQUIRE_FALSE( nonperishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( nonperishable_food, origin_pos ).id == zone_type_LOOT_FOOD );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( nonperishable_food, pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( container, origin_pos ).id == zone_type_LOOT_FOOD );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( nonperishable_food, pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_all_contained_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( container, origin_pos ).id == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a non-perishable drink" ) {
            item nonperishable_drink( itype_test_wine );
            REQUIRE_FALSE( nonperishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( nonperishable_drink,
                                                           origin_pos ).id == zone_type_LOOT_DRINK );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( nonperishable_drink, pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( container, origin_pos ).id == zone_type_LOOT_DRINK );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( nonperishable_drink, pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_all_contained_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( container, origin_pos ).id == zone_type_LOOT_DRINK );
                }
            }
        }

        GIVEN( "a perishable food" ) {
            item perishable_food( itype_test_apple );
            REQUIRE( perishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the perishable food zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( perishable_food, origin_pos ).id == zone_type_LOOT_PFOOD );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( perishable_food, pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the perishable food zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( container, origin_pos ).id == zone_type_LOOT_PFOOD );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( perishable_food, pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_all_contained_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( container, origin_pos ).id == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a perishable drink" ) {
            item perishable_drink( itype_test_milk );
            REQUIRE( perishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the perishable drink zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( perishable_drink, origin_pos ).id == zone_type_LOOT_PDRINK );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( perishable_drink, pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the perishable drink zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( container, origin_pos ).id == zone_type_LOOT_PDRINK );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( perishable_drink, pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_all_contained_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_best_zone_type_for_item( container, origin_pos ).id == zone_type_LOOT_DRINK );
                }
            }
        }
    }
}

TEST_CASE( "zone_sorting_priority", "[zones][items][activities]" )
{
    clear_map();
    zone_manager &zm = zone_manager::get_manager();
    const tripoint_abs_ms origin_pos = tripoint_abs_ms::zero;

    item some_ammo( itype_556 );
    item a_food( itype_test_bitter_almond );
    item an_empty_container( itype_test_watertight_open_sealed_container_250ml );

    GIVEN( "some test items" ) {
        REQUIRE_FALSE( a_food.is_ammo() );
        REQUIRE( a_food.is_food() );
        REQUIRE( some_ammo.is_ammo() );
        REQUIRE_FALSE( some_ammo.is_food() );
        REQUIRE_FALSE( an_empty_container.is_food() );
        REQUIRE_FALSE( an_empty_container.is_ammo() );

        WHEN( "all zones have default priority" ) {
            zm.clear();
            zm.add( "Default", zone_type_LOOT_DEFAULT, faction_your_followers, false, true,
                    origin_pos + tripoint::north, origin_pos + tripoint::north );
            zm.add( "Ammo", zone_type_LOOT_AMMO, faction_your_followers, false, true,
                    origin_pos + tripoint::south, origin_pos + tripoint::south );
            zm.add( "Food", zone_type_LOOT_FOOD, faction_your_followers, false, true,
                    origin_pos + tripoint::east, origin_pos + tripoint::east );

            THEN( "food should be assigned to the food zone which has priority 0" ) {
                zone_type_id_and_priority found = zm.get_best_zone_type_for_item( a_food, origin_pos );
                CHECK( found.id == zone_type_LOOT_FOOD );
                CHECK( found.priority == 0 );
            }

            THEN( "ammo should be assigned to the ammo zone which has priority 0" ) {
                zone_type_id_and_priority found = zm.get_best_zone_type_for_item( some_ammo, origin_pos );
                CHECK( found.id == zone_type_LOOT_AMMO );
                CHECK( found.priority == 0 );
            }

            THEN( "an empty container should be assigned to the default loot zone which has priority 0" ) {
                zone_type_id_and_priority found = zm.get_best_zone_type_for_item( an_empty_container, origin_pos );
                CHECK( found.id == zone_type_LOOT_DEFAULT );
                CHECK( found.priority == 0 );
            }
        }

        WHEN( "all zones are created with priority 5" ) {
            zm.clear();
            zm.add( "Default", zone_type_LOOT_DEFAULT, faction_your_followers, false, true,
                    origin_pos + tripoint::north, origin_pos + tripoint::north, nullptr, false, nullptr, 5 );
            zm.add( "Ammo", zone_type_LOOT_AMMO, faction_your_followers, false, true,
                    origin_pos + tripoint::south, origin_pos + tripoint::south, nullptr, false, nullptr, 5 );
            zm.add( "Food", zone_type_LOOT_FOOD, faction_your_followers, false, true,
                    origin_pos + tripoint::east, origin_pos + tripoint::east, nullptr, false, nullptr, 5 );

            THEN( "food should be assigned to the food zone which has priority 5" ) {
                zone_type_id_and_priority found = zm.get_best_zone_type_for_item( a_food, origin_pos );
                CHECK( found.id == zone_type_LOOT_FOOD );
                CHECK( found.priority == 5 );
            }

            THEN( "ammo should be assigned to the ammo zone which has priority 5" ) {
                zone_type_id_and_priority found = zm.get_best_zone_type_for_item( some_ammo, origin_pos );
                CHECK( found.id == zone_type_LOOT_AMMO );
                CHECK( found.priority == 5 );
            }

            THEN( "an empty container should be assigned to the default loot zone which has priority 5" ) {
                zone_type_id_and_priority found = zm.get_best_zone_type_for_item( an_empty_container, origin_pos );
                CHECK( found.id == zone_type_LOOT_DEFAULT );
                CHECK( found.priority == 5 );
            }
        }

        WHEN( "a higher priority default zone exists" ) {
            zm.clear();
            zm.add( "Default", zone_type_LOOT_DEFAULT, faction_your_followers, false, true,
                    origin_pos + tripoint::north, origin_pos + tripoint::north, nullptr, false, nullptr, 5 );
            zm.add( "Ammo", zone_type_LOOT_AMMO, faction_your_followers, false, true,
                    origin_pos + tripoint::south, origin_pos + tripoint::south, nullptr, false, nullptr, 5 );
            zm.add( "Food", zone_type_LOOT_FOOD, faction_your_followers, false, true,
                    origin_pos + tripoint::east, origin_pos + tripoint::east, nullptr, false, nullptr, 5 );
            zm.add( "Default2", zone_type_LOOT_DEFAULT, faction_your_followers, false, true,
                    origin_pos + tripoint::north, origin_pos + tripoint::north, nullptr, false, nullptr, 10 );

            THEN( "food should be assigned to the higher priority default zone" ) {
                zone_type_id_and_priority found = zm.get_best_zone_type_for_item( a_food, origin_pos );
                CHECK( found.id == zone_type_LOOT_DEFAULT );
                CHECK( found.priority == 10 );
            }

            THEN( "ammo should be assigned to the higher priority default zone" ) {
                zone_type_id_and_priority found = zm.get_best_zone_type_for_item( some_ammo, origin_pos );
                CHECK( found.id == zone_type_LOOT_DEFAULT );
                CHECK( found.priority == 10 );
            }

            THEN( "an empty container should be assigned to the higher priority default zone" ) {
                zone_type_id_and_priority found = zm.get_best_zone_type_for_item( an_empty_container, origin_pos );
                CHECK( found.id == zone_type_LOOT_DEFAULT );
                CHECK( found.priority == 10 );
            }
        }
    }

    WHEN( "a negative priority zone is the best match for ammo and no default zone is added" ) {
        zm.clear();
        zm.add( "Ammo", zone_type_LOOT_AMMO, faction_your_followers, false, true,
                origin_pos + tripoint::south, origin_pos + tripoint::south, nullptr, false, nullptr, -5 );
        zm.add( "Food", zone_type_LOOT_FOOD, faction_your_followers, false, true,
                origin_pos + tripoint::east, origin_pos + tripoint::east, nullptr, false, nullptr, 1 );

        THEN( "food should be assigned to the food zone" ) {
            zone_type_id_and_priority found = zm.get_best_zone_type_for_item( a_food, origin_pos );
            CHECK( found.id == zone_type_LOOT_FOOD );
            CHECK( found.priority == 1 );
        }

        THEN( "ammo should be assigned to the ammo zone" ) {
            zone_type_id_and_priority found = zm.get_best_zone_type_for_item( some_ammo, origin_pos );
            CHECK( found.id == zone_type_LOOT_AMMO );
            CHECK( found.priority == -5 );
        }

        THEN( "an empty container should not go anywhere" ) {
            zone_type_id_and_priority found = zm.get_best_zone_type_for_item( an_empty_container, origin_pos );
            CHECK( !found.id.is_valid() );
            CHECK( found.priority == std::numeric_limits<int>::min() );
        }
    }
}


TEST_CASE( "zone_containment", "[zones]" )
{
    tripoint_abs_ms min[5] = { { 5, 5, 0 }, { -20, -10, 0 }, { 4, -10, 0 }, { -3, -6, 0 }, { 5, 5, 2 } };
    tripoint_abs_ms max[5] = { { 10, 15, 0 }, { -4, -8, 0 }, { 12, -8, 0 }, { 5, 4, 0 }, { 10, 15, 2 } };

    zone_data zones[5] = { { "NE", zone_type_LOOT_DEFAULT, faction_your_followers, false, true, min[0], max[0] },
        { "SW", zone_type_LOOT_DEFAULT, faction_your_followers, false, true, min[1], max[1] },
        { "SE", zone_type_LOOT_DEFAULT, faction_your_followers, false, true, min[2], max[2] },
        { "C",  zone_type_LOOT_DEFAULT, faction_your_followers, false, true, min[3], max[3] },
        { "UP", zone_type_LOOT_DEFAULT, faction_your_followers, false, true, min[4], max[4] }
    };

    int const short_distance = 4;

    WHEN( "a point lies inside a zone" ) {
        THEN( "it is matched by has_inside and has_within_square_dist" ) {
            for( int zone = 0; zone < 5; ++zone ) {
                int z = min[zone].z();
                REQUIRE( z == max[zone].z() );
                for( int x = min[zone].x() + 1; x < max[zone].x(); ++x ) {
                    for( int y = min[zone].y() + 1; y < max[zone].y(); ++y ) {
                        tripoint_abs_ms point_inside( x, y, z );
                        CHECK( zones[zone].has_inside( point_inside ) );
                        CHECK( zones[zone].has_within_square_dist( point_inside, short_distance ) );
                    }
                }
            }
        }
    }

    WHEN( "a point lies on the edge of a zone" ) {
        THEN( "it is matched by has_inside and has_within_square_dist" ) {
            for( int zone = 0; zone < 5; ++zone ) {
                int z = min[zone].z();
                REQUIRE( z == max[zone].z() );

                for( int x = min[zone].x(); x <= max[zone].x(); ++x ) {
                    tripoint_abs_ms point_on_north_edge( x, max[zone].y(), z );
                    tripoint_abs_ms point_on_south_edge( x, min[zone].y(), z );
                    CHECK( zones[zone].has_inside( point_on_north_edge ) );
                    CHECK( zones[zone].has_inside( point_on_south_edge ) );
                    CHECK( zones[zone].has_within_square_dist( point_on_north_edge, short_distance ) );
                    CHECK( zones[zone].has_within_square_dist( point_on_south_edge, short_distance ) );
                }
                for( int y = min[zone].y(); y <= max[zone].y(); ++y ) {
                    tripoint_abs_ms point_on_west_edge( max[zone].x(), y, z );
                    tripoint_abs_ms point_on_east_edge( min[zone].x(), y, z );
                    CHECK( zones[zone].has_inside( point_on_west_edge ) );
                    CHECK( zones[zone].has_inside( point_on_east_edge ) );
                    CHECK( zones[zone].has_within_square_dist( point_on_west_edge, short_distance ) );
                    CHECK( zones[zone].has_within_square_dist( point_on_east_edge, short_distance ) );
                }
            }
        }
    }

    WHEN( "a point lies outside but close to a zone" ) {
        THEN( "it is matched by has_within_square_dist but not has_inside" ) {
            for( int zone = 0; zone < 5; ++zone ) {
                int z = min[zone].z();
                REQUIRE( z == max[zone].z() );

                // Note we use square distance in the game, not real distance, regardless of user settings.

                for( int x = min[zone].x() - short_distance; x <= max[zone].x() + short_distance; ++x ) {
                    for( int y = min[zone].y() - short_distance; y <= max[zone].y() + short_distance; ++y ) {
                        if( x < min[zone].x() || x > max[zone].x() || y < min[zone].y() || y > max[zone].y() ) {
                            tripoint_abs_ms near_point( x, y, z );
                            CHECK( !zones[zone].has_inside( near_point ) );
                            CHECK( zones[zone].has_within_square_dist( near_point, short_distance ) );
                        }
                    }
                }
            }

        }
    }

    WHEN( "a point lies outside and far from a zone" ) {
        THEN( "it is not matched by has_inside or has_within_square_dist" ) {
            for( int zone = 0; zone < 5; ++zone ) {
                int z = min[zone].z();
                REQUIRE( z == max[zone].z() );

                for( int x = min[zone].x() - 1; x <= max[zone].x() + 1; ++x ) {
                    tripoint_abs_ms just_far_north( x, max[zone].y() + short_distance + 1, z );
                    tripoint_abs_ms just_far_south( x, min[zone].y() - short_distance - 1, z );
                    CHECK( !zones[zone].has_inside( just_far_north ) );
                    CHECK( !zones[zone].has_inside( just_far_south ) );
                    CHECK( !zones[zone].has_within_square_dist( just_far_north, short_distance ) );
                    CHECK( !zones[zone].has_within_square_dist( just_far_south, short_distance ) );
                }
                for( int y = min[zone].y() - 1; y <= max[zone].y() + 1; ++y ) {
                    tripoint_abs_ms just_far_east( max[zone].x() + short_distance + 1, y, z );
                    tripoint_abs_ms just_far_west( min[zone].x() - short_distance - 1, y, z );
                    CHECK( !zones[zone].has_inside( just_far_east ) );
                    CHECK( !zones[zone].has_inside( just_far_west ) );
                    CHECK( !zones[zone].has_within_square_dist( just_far_east, short_distance ) );
                    CHECK( !zones[zone].has_within_square_dist( just_far_west, short_distance ) );
                }

                tripoint_abs_ms far_above = zones[zone].get_center_point() +
                                            ( short_distance + 1 ) * tripoint::above;
                tripoint_abs_ms far_below = zones[zone].get_center_point() +
                                            ( short_distance + 1 ) * tripoint::below;
                CHECK( !zones[zone].has_inside( far_above ) );
                CHECK( !zones[zone].has_inside( far_below ) );
                CHECK( !zones[zone].has_within_square_dist( far_above, short_distance ) );
                CHECK( !zones[zone].has_within_square_dist( far_below, short_distance ) );

                tripoint_abs_ms really_far( max[zone].x() + 123456, min[zone].y() - 123456, z );
                CHECK( !zones[zone].has_inside( really_far ) );
                CHECK( !zones[zone].has_within_square_dist( really_far, short_distance ) );
            }
        }
    }

    WHEN( "a point lies close above or below a zone" ) {
        THEN( "it is matched by has_within_square_dist but not by has_inside" ) {
            for( int zone = 0; zone < 5; ++zone ) {
                int z_up = max[zone].z() + short_distance;
                int z_down = min[zone].z() - short_distance;

                for( int x = min[zone].x() + 1; x < max[zone].x(); ++x ) {
                    for( int y = min[zone].y() + 1; y < max[zone].y(); ++y ) {
                        tripoint_abs_ms point_above( x, y, z_up );
                        tripoint_abs_ms point_below( x, y, z_down );
                        CHECK( !zones[zone].has_inside( point_above ) );
                        CHECK( !zones[zone].has_inside( point_below ) );
                        CHECK( zones[zone].has_within_square_dist( point_above, short_distance ) );
                        CHECK( zones[zone].has_within_square_dist( point_below, short_distance ) );
                    }
                }
            }
        }
    }
}
