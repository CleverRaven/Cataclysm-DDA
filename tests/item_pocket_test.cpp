#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "debug.h"
#include "enums.h"
#include "flag.h"
#include "item.h"
#include "item_category.h"
#include "item_factory.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "iuse_actor.h"
#include "map.h"
#include "map_helpers.h"
#include "mapgen_helpers.h"
#include "player_helpers.h"
#include "ret_val.h"
#include "test_data.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "weather.h"

static const ammotype ammo_test_9mm( "test_9mm" );

static const item_group_id Item_spawn_data_wallet_duct_tape_full( "wallet_duct_tape_full" );
static const item_group_id Item_spawn_data_wallet_full( "wallet_full" );
static const item_group_id Item_spawn_data_wallet_industrial_full( "wallet_industrial_full" );
static const item_group_id
Item_spawn_data_wallet_industrial_leather_full( "wallet_industrial_leather_full" );
static const item_group_id Item_spawn_data_wallet_large_full( "wallet_large_full" );
static const item_group_id Item_spawn_data_wallet_leather_full( "wallet_leather_full" );
static const item_group_id Item_spawn_data_wallet_military_full( "wallet_military_full" );
static const item_group_id
Item_spawn_data_wallet_military_leather_full( "wallet_military_leather_full" );
static const item_group_id Item_spawn_data_wallet_science_full( "wallet_science_full" );
static const item_group_id
Item_spawn_data_wallet_science_leather_full( "wallet_science_leather_full" );
static const item_group_id
Item_spawn_data_wallet_science_stylish_full( "wallet_science_stylish_full" );
static const item_group_id Item_spawn_data_wallet_stylish_full( "wallet_stylish_full" );

static const itype_id itype_test_backpack( "test_backpack" );
static const itype_id itype_test_jug_plastic( "test_jug_plastic" );
static const itype_id itype_test_socks( "test_socks" );
static const itype_id
itype_test_watertight_open_sealed_container_1L( "test_watertight_open_sealed_container_1L" );

static const nested_mapgen_id nested_mapgen_auto_wl_test( "auto_wl_test" );

static const pocket_type pocket_container = pocket_type::CONTAINER;

// Pocket Tests
// ------------
//
// These tests are focused on the `item_pocket` and `pocket_data` classes, and how they implement
// the "pocket_data" section of JSON item definitions (as documented in doc/JSON_FLAGS.md).
//
// To run all tests in this file:
//
//      tests/cata_test [pocket]
//
// Other tags used include: [holster], [magazine], [airtight], [watertight], [rigid], and more.
//
// Most of the items used for testing here are defined in data/mods/TEST_DATA/items.json, to have
// a predictable set of data unaffected by in-game balance changes.

// TODO: Add tests focusing on these JSON pocket_data fields:
// -  pocket_type
// -  spoil_multiplier
// -  weight_multiplier
// -  magazine_well
// -  fire_protection
// -  open_container
// -  sealed_data
// -  moves

// FIXME: The failure messaging from these helper functions is pretty terrible, due to the multiple
// CHECKs in each function. Capturing it.tname() helps only slightly. The `rate_can.value()` is an
// integer, so it is difficult to see when a failure reason is returned instead of success.

// Expect pocket.can_contain( it ) to be successful (contain_code::SUCCESS)
static void expect_can_contain( const item_pocket &pocket, const item &it )
{
    CAPTURE( it.tname() );
    const ret_val<item_pocket::contain_code> rate_can = pocket.can_contain( it );
    INFO( rate_can.str() );
    CHECK( rate_can.success() );
    CHECK( rate_can.str().empty() );
    CHECK( rate_can.value() == item_pocket::contain_code::SUCCESS );
}

// Expect pocket.can_contain( it ) to fail, with an expected reason and contain_code
static void expect_cannot_contain( const item_pocket &pocket, const item &it,
                                   const std::string &expect_reason,
                                   item_pocket::contain_code expect_code )
{
    CAPTURE( it.tname() );
    ret_val<item_pocket::contain_code> rate_can = pocket.can_contain( it );
    CHECK_FALSE( rate_can.success() );
    CHECK( rate_can.str() == expect_reason );
    CHECK( rate_can.value() == expect_code );
}

// Call pocket.insert_item( it ) and expect it to be successful (contain_code::SUCCESS)
static void expect_can_insert( item_pocket &pocket, const item &it )
{
    CAPTURE( it.tname() );
    CHECK( pocket.can_contain( it ).value() == item_pocket::contain_code::SUCCESS );
    ret_val<item *> rate_can = pocket.insert_item( it );
    CHECK( rate_can.success() );
    CHECK( rate_can.str().empty() );
    REQUIRE( rate_can.value() != nullptr );
    CHECK( rate_can.value()->stacks_with( it ) );
}

// Call pocket.insert_item( it ) and expect it to fail, with an expected reason and contain_code
static void expect_cannot_insert( item_pocket &pocket, const item &it,
                                  const std::string &expect_reason,
                                  item_pocket::contain_code expect_code )
{
    CAPTURE( it.tname() );
    CHECK( pocket.can_contain( it ).value() == expect_code );
    ret_val<item *> rate_can = pocket.insert_item( it );
    CHECK_FALSE( rate_can.success() );
    CHECK( rate_can.str() == expect_reason );
    CHECK( rate_can.value() == nullptr );
}

// Max item length
// ---------------
// Pocket "max_item_length" is the biggest item (by longest side) that can fit.
//
// Related JSON fields:
// "max_item_length"
//
// Functions:
// item_pocket::can_contain
//
TEST_CASE( "max_item_length", "[pocket][max_item_length]" )
{
    // Test items with different lengths
    item screwdriver( "test_screwdriver" );
    item sonic( "test_sonic_screwdriver" );
    item sword( "test_clumsy_sword" );

    // Sheath that may contain items
    pocket_data data_sheath( pocket_type::CONTAINER );
    // Has plenty of weight/ volume, since we're only testing length
    data_sheath.volume_capacity = 10_liter;
    data_sheath.max_contains_weight = 10_kilogram;

    GIVEN( "pocket has max_item_length enough for some items, but not others" ) {
        // Sheath is just long enough for sonic screwdriver
        const units::length max_length = sonic.length();
        data_sheath.max_item_length = max_length;
        item_pocket pocket_sheath( &data_sheath );

        THEN( "it can contain an item with longest_side shorter than max_item_length" ) {
            REQUIRE( screwdriver.length() < max_length );
            expect_can_contain( pocket_sheath, screwdriver );
        }

        THEN( "it can contain an item with longest_side equal to max_item_length" ) {
            REQUIRE( sonic.length() == max_length );
            expect_can_contain( pocket_sheath, sonic );
        }

        THEN( "it cannot contain an item with longest_side longer than max_item_length" ) {
            REQUIRE( sword.length() > max_length );
            expect_cannot_contain( pocket_sheath, sword, "item is too long",
                                   item_pocket::contain_code::ERR_TOO_BIG );
        }
    }

    // Default max_item_length assumes pocket is a cube, limiting max length to the diagonal
    // dimension of one side of the cube (the opening into the box, so to speak).
    //
    // For a 1-liter volume, a 10 cm cube encloses it, so the longest diagonal is:
    //
    //      10 cm * sqrt(2) =~ 14.14 cm
    //
    // This test ensures that a 1-liter box with unspecified max_item_length can indeed contain
    // items up to 14 cm in length, but not ones that are 15 cm.
    //
    // NOTE: In theory, the interior space of the cube can accommodate a longer item between its
    // opposite diagonals, assuming it is infinitely thin:
    //
    //      10 cm * sqrt(3) =~ 17.32 cm
    //
    // Items of such length are not currently allowed in a 1-liter pocket.

    GIVEN( "a 1-liter box without a defined max_item_length" ) {
        item box( "test_box" );
        REQUIRE( box.get_total_capacity() == 1_liter );

        THEN( "it can hold an item 14 cm in length" ) {
            item rod_14( "test_rod_14cm" );
            REQUIRE( rod_14.length() == 14_cm );

            REQUIRE( box.is_container_empty() );
            box.put_in( rod_14, pocket_type::CONTAINER );
            // Item went into the box
            CHECK_FALSE( box.is_container_empty() );
        }

        THEN( "it cannot hold an item 15 cm in length" ) {
            item rod_15( "test_rod_15cm" );
            REQUIRE( rod_15.length() == 15_cm );

            REQUIRE( box.is_container_empty() );
            std::string dmsg = capture_debugmsg_during( [&box, &rod_15]() {
                ret_val<void> result = box.put_in( rod_15, pocket_type::CONTAINER );
                CHECK_FALSE( result.success() );
            } );
            CHECK_THAT( dmsg, Catch::EndsWith( "item is too long" ) );
            // Box should still be empty
            CHECK( box.is_container_empty() );
        }
    }
}

// Max item volume
// ---------------
// "max_item_volume" is the size of the mouth or opening into the pocket. Liquids and gases, as well
// as "soft" items, can always fit through. All other items must fit within "max_item_volume" to be
// put in the pocket.
//
// Related JSON fields:
// "max_item_volume"
//
// Functions:
// item_pocket::can_contain
//
TEST_CASE( "max_item_volume", "[pocket][max_item_volume]" )
{
    // Test items
    item screwdriver( "test_screwdriver" );
    item rag( "test_rag" );
    item rock( "test_rock" );
    item gas( "test_gas" );
    item liquid( "test_liquid" );

    // Air-tight, water-tight, jug-style container
    pocket_data data_jug( pocket_type::CONTAINER );
    data_jug.airtight = true;
    data_jug.watertight = true;

    // No significant limits on length or weight
    data_jug.max_item_length = 1_meter;
    data_jug.max_contains_weight = 10_kilogram;

    // Jug capacity is more than enough for several rocks, rags, or screwdrivers
    data_jug.volume_capacity = 2_liter;
    REQUIRE( data_jug.volume_capacity > 5 * rock.volume() );
    REQUIRE( data_jug.volume_capacity > 5 * rag.volume() );
    REQUIRE( data_jug.volume_capacity > 10 * screwdriver.volume() );

    // But it has a narrow mouth, just barely enough to fit a screwdriver
    const units::volume max_volume = screwdriver.volume();
    data_jug.max_item_volume = max_volume;

    // The screwdriver will fit
    REQUIRE( screwdriver.volume() <= max_volume );
    // All other items have too much volume to fit the opening
    REQUIRE( rag.volume() > max_volume );
    REQUIRE( rock.volume() > max_volume );
    REQUIRE( gas.volume() > max_volume );
    REQUIRE( liquid.volume() > max_volume );

    GIVEN( "large airtight and watertight pocket with a narrow opening" ) {
        item_pocket pocket_jug( &data_jug );

        THEN( "it can contain liquids" ) {
            REQUIRE( liquid.made_of( phase_id::LIQUID ) );
            expect_can_contain( pocket_jug, liquid );
        }
        THEN( "it can contain gases" ) {
            REQUIRE( gas.made_of( phase_id::GAS ) );
            expect_can_contain( pocket_jug, gas );
        }
        THEN( "it can contain solid items that fit through the opening" ) {
            REQUIRE_FALSE( screwdriver.is_soft() );
            expect_can_contain( pocket_jug, screwdriver );
        }
        THEN( "it can contain soft items larger than the opening" ) {
            REQUIRE( rag.is_soft() );
            expect_can_contain( pocket_jug, rag );
        }
        THEN( "it cannot contain solid items larger than the opening" ) {
            REQUIRE_FALSE( rock.is_soft() );
            expect_cannot_contain( pocket_jug, rock, "item is too big",
                                   item_pocket::contain_code::ERR_TOO_BIG );
        }
    }
}

// Max container volume
// --------------------
// Normally, the "max_contains_volume" from pocket data JSON is the largest total volume of
// items the pocket can hold. It is returned by the pocket_data::max_contains_volume() function.
//
// However, pockets with an "ammo_restriction", the pocket_data::max_contains_volume() is derived
// from the size of the ammo given in the restriction.
//
// Related JSON fields:
// "max_contains_volume"
// "ammo_restriction"
//
// Functions:
// pocket_data::max_contains_volume
//
TEST_CASE( "max_container_volume", "[pocket][max_contains_volume]" )
{
    // TODO: Add tests for having multiple ammo types in the ammo_restriction

    WHEN( "pocket has no ammo_restriction" ) {
        pocket_data data_box( pocket_type::CONTAINER );
        // Just a normal 1-liter box
        data_box.volume_capacity = 1_liter;
        REQUIRE( data_box.ammo_restriction.empty() );

        THEN( "max_contains_volume is the pocket's volume_capacity" ) {
            CHECK( data_box.max_contains_volume() == 1_liter );
        }
    }

    WHEN( "pocket has ammo_restriction" ) {
        pocket_data data_ammo_box( pocket_type::CONTAINER );

        // 9mm ammo is 50 rounds per 250ml (or 200 rounds per liter), so this ammo box
        // should be exactly 1 liter in size, so it can contain this much ammo.
        data_ammo_box.ammo_restriction.emplace( ammo_test_9mm, 200 );
        REQUIRE_FALSE( data_ammo_box.ammo_restriction.empty() );

        // And because actual volume is derived from ammo needs, this volume should be ignored.
        data_ammo_box.volume_capacity = 1_ml;

        THEN( "max_contains_volume is based on the pocket's ammo type" ) {
            CHECK( data_ammo_box.max_contains_volume() == 1_liter );
        }
    }
}

// Ammo restriction
// ----------------
// Pockets with "ammo_restriction" defined have capacity based on those restrictions (from the
// type(s) of ammo and quantity of each. They can only contain ammo of those types and counts,
// and mixing different ammos in the same pocket is prohibited.
//
// Related JSON fields:
// "ammo_restriction"
//
// Functions:
// item_pocket::can_contain
// item_pocket::insert_item
//
TEST_CASE( "magazine_with_ammo_restriction", "[pocket][magazine][ammo_restriction]" )
{
    pocket_data data_mag( pocket_type::MAGAZINE );

    // ammo_restriction takes precedence over volume/length/weight,
    // so it doesn't matter what these are set to - they can even be 0.
    data_mag.volume_capacity = 0_liter;
    data_mag.min_item_volume = 0_liter;
    data_mag.max_item_length = 0_meter;
    data_mag.max_contains_weight = 0_gram;

    // TODO: Add tests for multiple ammo types in the same clip

    GIVEN( "magazine has 9mm ammo_restriction" ) {
        // Magazine ammo_restriction may be a single { ammotype, count } or a list of ammotypes and
        // counts. This mag 10 rounds of 9mm, as if it were defined in JSON as:
        //
        //      "ammo_restriction": { "9mm", 10 }
        //
        const int full_clip_qty = 10;
        data_mag.ammo_restriction.emplace( ammo_test_9mm, full_clip_qty );
        item_pocket pocket_mag( &data_mag );

        WHEN( "it does not already contain any ammo" ) {
            REQUIRE( pocket_mag.empty() );

            THEN( "it can contain a full clip of 9mm ammo" ) {
                const item ammo_9mm( "test_9mm_ammo", calendar::turn_zero, full_clip_qty );
                expect_can_contain( pocket_mag, ammo_9mm );
            }

            THEN( "it cannot contain items of the wrong ammo type" ) {
                item rock( "test_rock" );
                item ammo_45( "test_45_ammo", calendar::turn_zero, 1 );
                expect_cannot_contain( pocket_mag, rock, "item is not the correct ammo type",
                                       item_pocket::contain_code::ERR_AMMO );
                expect_cannot_contain( pocket_mag, ammo_45, "item is not the correct ammo type",
                                       item_pocket::contain_code::ERR_AMMO );
            }

            THEN( "it cannot contain items that are not ammo" ) {
                item rag( "test_rag" );
                expect_cannot_contain( pocket_mag, rag, "item is not ammunition",
                                       item_pocket::contain_code::ERR_AMMO );
            }
        }

        WHEN( "it is partly full of ammo" ) {
            const int half_clip_qty = full_clip_qty / 2;
            item ammo_9mm_half_clip( "test_9mm_ammo", calendar::turn_zero, half_clip_qty );
            expect_can_insert( pocket_mag, ammo_9mm_half_clip );

            THEN( "it can contain more of the same ammo" ) {
                item ammo_9mm_refill( "test_9mm_ammo", calendar::turn_zero,
                                      full_clip_qty - half_clip_qty );
                expect_can_contain( pocket_mag, ammo_9mm_refill );
            }

            THEN( "it cannot contain more ammo than ammo_restriction allows" ) {
                item ammo_9mm_overfill( "test_9mm_ammo", calendar::turn_zero,
                                        full_clip_qty - half_clip_qty + 1 );
                expect_cannot_contain( pocket_mag, ammo_9mm_overfill,
                                       "tried to put too many charges of ammo in item",
                                       item_pocket::contain_code::ERR_NO_SPACE );
            }
        }

        WHEN( "it is completely full of ammo" ) {
            item ammo_9mm_full_clip( "test_9mm_ammo", calendar::turn_zero, full_clip_qty );
            expect_can_insert( pocket_mag, ammo_9mm_full_clip );

            THEN( "it cannot contain any more of the same ammo" ) {
                item ammo_9mm_bullet( "test_9mm_ammo", calendar::turn_zero, 1 );
                expect_cannot_contain( pocket_mag, ammo_9mm_bullet,
                                       "tried to put too many charges of ammo in item",
                                       item_pocket::contain_code::ERR_NO_SPACE );
            }
        }
    }
}

// Flag restriction
// ----------------
// The "get_flag_restrictions" list from pocket data JSON gives compatible item flag(s) for this pocket.
// An item with any one of those tags may be inserted into the pocket.
//
// Related JSON fields:
// "flag_restriction"
// "min_item_volume"
// "max_contains_volume"
//
// Functions:
// item_pocket::can_contain
// pocket_data::max_contains_volume
//
TEST_CASE( "pocket_with_item_flag_restriction", "[pocket][flag_restriction]" )
{
    // Items with BELT_CLIP flag
    item screwdriver( "test_screwdriver" );
    item sonic( "test_sonic_screwdriver" );
    item halligan( "test_halligan" );
    item axe( "test_fire_ax" );

    // Items without BELT_CLIP flag
    item rag( "test_rag" );
    item rock( "test_rock" );

    // Ensure the tools have expected volume relationships
    // (too small, minimum size, maximum size, too big)
    REQUIRE( screwdriver.volume() < sonic.volume() );
    REQUIRE( sonic.volume() < halligan.volume() );
    REQUIRE( halligan.volume() < axe.volume() );

    // Test pocket with BELT_CLIP flag
    pocket_data data_belt( pocket_type::CONTAINER );

    // Sonic screwdriver is the smallest item it can hold
    data_belt.min_item_volume = sonic.volume();
    // Halligan bar is the largest item it can hold
    data_belt.volume_capacity = halligan.volume();

    // Plenty of length and weight, since we only care about flag restrictions here
    data_belt.max_item_length = 10_meter;
    data_belt.max_contains_weight = 10_kilogram;

    // TODO: Add test for pocket with multiple flag_restriction entries, and ensure
    // items with any of those flags can be inserted in the pocket.

    GIVEN( "pocket with BELT_CLIP flag restriction" ) {
        data_belt.add_flag_restriction( flag_BELT_CLIP );
        item_pocket pocket_belt( &data_belt );

        GIVEN( "item has BELT_CLIP flag" ) {
            REQUIRE( screwdriver.has_flag( flag_BELT_CLIP ) );
            REQUIRE( sonic.has_flag( flag_BELT_CLIP ) );
            REQUIRE( halligan.has_flag( flag_BELT_CLIP ) );
            REQUIRE( axe.has_flag( flag_BELT_CLIP ) );

            WHEN( "item volume is less than min_item_volume" ) {
                REQUIRE( screwdriver.volume() < data_belt.min_item_volume );

                THEN( "pocket cannot contain it, because it is too small" ) {
                    expect_cannot_contain( pocket_belt, screwdriver, "item is too small",
                                           item_pocket::contain_code::ERR_TOO_SMALL );
                }

                THEN( "item cannot be inserted into the pocket" ) {
                    expect_cannot_insert( pocket_belt, screwdriver, "item is too small",
                                          item_pocket::contain_code::ERR_TOO_SMALL );
                }
            }

            WHEN( "item volume is equal to min_item_volume" ) {
                REQUIRE( sonic.volume() == data_belt.min_item_volume );

                THEN( "pocket can contain it" ) {
                    expect_can_contain( pocket_belt, sonic );
                }

                THEN( "item can be inserted successfully" ) {
                    expect_can_insert( pocket_belt, sonic );
                }
            }

            WHEN( "item volume is equal to max_contains_volume" ) {
                REQUIRE( halligan.volume() == data_belt.max_contains_volume() );

                THEN( "pocket can contain it" ) {
                    expect_can_contain( pocket_belt, halligan );
                }

                THEN( "item can be inserted successfully" ) {
                    expect_can_insert( pocket_belt, halligan );
                }
            }

            WHEN( "item volume is greater than max_contains_volume" ) {
                REQUIRE( axe.volume() > data_belt.max_contains_volume() );

                THEN( "pocket cannot contain it, because it is too big" ) {
                    expect_cannot_contain( pocket_belt, axe, "item is too big",
                                           item_pocket::contain_code::ERR_TOO_BIG );
                }

                THEN( "item cannot be inserted into the pocket" ) {
                    expect_cannot_insert( pocket_belt, axe, "item is too big",
                                          item_pocket::contain_code::ERR_TOO_BIG );
                }
            }
        }

        GIVEN( "item without BELT_CLIP flag" ) {
            REQUIRE_FALSE( rag.has_flag( flag_BELT_CLIP ) );
            REQUIRE_FALSE( rock.has_flag( flag_BELT_CLIP ) );
            // Ensure they are not too large otherwise
            REQUIRE_FALSE( rag.volume() > data_belt.max_contains_volume() );
            REQUIRE_FALSE( rock.volume() > data_belt.max_contains_volume() );

            THEN( "pocket cannot contain it, because it does not have the flag" ) {
                expect_cannot_contain( pocket_belt, rag, "holster does not accept this item type or form factor",
                                       item_pocket::contain_code::ERR_FLAG );
                expect_cannot_contain( pocket_belt, rock, "holster does not accept this item type or form factor",
                                       item_pocket::contain_code::ERR_FLAG );
            }
        }
    }
}

// Holster
// -------
// Pockets with the "holster" attribute set to true may hold only a single item or stack.
//
// Related JSON fields:
// "holster"
// "max_item_length"
// "max_contains_volume"
// "max_contains_weight"
//
// Functions:
// item_pocket::can_contain
// item_pocket::insert_item
//
TEST_CASE( "holster_can_contain_one_fitting_item", "[pocket][holster]" )
{
    // Start with a basic test handgun from data/mods/TEST_DATA/items.json
    item glock( "test_glock" );

    // Construct data for a holster to perfectly fit this gun
    pocket_data data_holster( pocket_type::CONTAINER );
    data_holster.holster = true;
    data_holster.volume_capacity = glock.volume();
    data_holster.max_item_length = glock.length();
    data_holster.max_contains_weight = glock.weight();

    GIVEN( "holster has enough volume, length, and weight capacity" ) {
        // Use the perfectly-fitted holster data
        item_pocket pocket_holster( &data_holster );

        THEN( "it can contain the item" ) {
            expect_can_contain( pocket_holster, glock );
        }

        THEN( "item can be successfully inserted" ) {
            expect_can_insert( pocket_holster, glock );
        }
    }

    GIVEN( "holster has not enough weight capacity" ) {
        // Reduce perfect holster weight capacity by 1 gram
        data_holster.max_contains_weight = glock.weight() - 1_gram;
        item_pocket pocket_holster( &data_holster );

        THEN( "it cannot contain the item, because it is too heavy" ) {
            expect_cannot_contain( pocket_holster, glock, "item is too heavy",
                                   item_pocket::contain_code::ERR_TOO_HEAVY );
        }

        THEN( "item cannot be successfully inserted" ) {
            expect_cannot_insert( pocket_holster, glock, "item is too heavy",
                                  item_pocket::contain_code::ERR_TOO_HEAVY );
        }
    }

    GIVEN( "holster has not enough volume capacity" ) {
        // Reduce perfect holster volume capacity by 1 ml
        data_holster.volume_capacity = glock.volume() - 1_ml;
        item_pocket pocket_holster( &data_holster );

        THEN( "it cannot contain the item, because it is too big" ) {
            expect_cannot_contain( pocket_holster, glock, "item is too big",
                                   item_pocket::contain_code::ERR_TOO_BIG );
        }

        THEN( "item cannot be successfully inserted" ) {
            expect_cannot_insert( pocket_holster, glock, "item is too big",
                                  item_pocket::contain_code::ERR_TOO_BIG );
        }
    }

    // TODO: Add test for item with longest_side greater than max_item_length

    GIVEN( "holster already contains an item" ) {
        // Put another item in the holster first
        item_pocket pocket_holster( &data_holster );
        expect_can_insert( pocket_holster, item( "test_glock" ) );

        THEN( "it cannot contain the item, because holster can only hold one item" ) {
            expect_cannot_contain( pocket_holster, glock, "holster already contains an item",
                                   item_pocket::contain_code::ERR_NO_SPACE );
        }

        THEN( "another item cannot be successfully inserted" ) {
            expect_cannot_insert( pocket_holster, glock, "holster already contains an item",
                                  item_pocket::contain_code::ERR_NO_SPACE );
        }
    }
}

// Watertight pockets
// ------------------
// To contain liquids, a pocket must have the "watertight" attribute set to true. After a pocket has
// been partially filled with a liquid, adding a different liquid to it is prohibited, since liquids
// can't mix.
//
// Related JSON fields:
// "watertight"
//
// Functions:
// item_pocket::watertight
//
TEST_CASE( "pockets_containing_liquids", "[pocket][watertight][liquid]" )
{
    // Liquids
    item ketchup( "ketchup", calendar::turn_zero, item::default_charges_tag{} );
    item mustard( "mustard", calendar::turn_zero, item::default_charges_tag{} );

    // Non-liquids
    item rock( "test_rock" );
    item glock( "test_glock" );

    // Large watertight container
    pocket_data data_bucket( pocket_type::CONTAINER );
    data_bucket.watertight = true;
    data_bucket.volume_capacity = 10_liter;
    data_bucket.max_item_length = 1_meter;
    data_bucket.max_contains_weight = 10_kilogram;

    GIVEN( "pocket is watertight" ) {
        item_pocket pocket_bucket( &data_bucket );
        REQUIRE( pocket_bucket.watertight() );

        WHEN( "pocket is empty" ) {
            REQUIRE( pocket_bucket.empty() );

            THEN( "it can contain liquid items" ) {
                expect_can_contain( pocket_bucket, ketchup );
                expect_can_contain( pocket_bucket, mustard );
            }

            THEN( "it can contain non-liquid items" ) {
                expect_can_contain( pocket_bucket, rock );
                expect_can_contain( pocket_bucket, glock );
            }
        }

        WHEN( "pocket already contains some liquid" ) {
            expect_can_insert( pocket_bucket, ketchup );
            REQUIRE_FALSE( pocket_bucket.empty() );

            THEN( "it can contain more of the same liquid" ) {
                expect_can_contain( pocket_bucket, ketchup );
            }

            THEN( "it cannot contain a different liquid" ) {
                expect_cannot_contain( pocket_bucket, mustard, "can't mix liquid with contained item",
                                       item_pocket::contain_code::ERR_LIQUID );
            }

            THEN( "it cannot contain a non-liquid" ) {
                expect_cannot_contain( pocket_bucket, rock, "can't put non liquid into pocket with liquid",
                                       item_pocket::contain_code::ERR_LIQUID );
            }
        }

        WHEN( "pocket already contains a non-liquid" ) {
            expect_can_insert( pocket_bucket, rock );
            REQUIRE_FALSE( pocket_bucket.empty() );

            THEN( "it can contain other non-liquids" ) {
                expect_can_contain( pocket_bucket, glock );
            }

            THEN( "it cannot contain a liquid" ) {
                expect_cannot_contain( pocket_bucket, ketchup, "can't mix liquid with contained item",
                                       item_pocket::contain_code::ERR_LIQUID );
                expect_cannot_contain( pocket_bucket, mustard, "can't mix liquid with contained item",
                                       item_pocket::contain_code::ERR_LIQUID );
            }
        }
    }

    GIVEN( "pocket is not watertight" ) {
        // Poke a hole in the bucket
        data_bucket.watertight = false;
        item_pocket pocket_bucket( &data_bucket );

        THEN( "it cannot contain liquid items" ) {
            expect_cannot_contain( pocket_bucket, ketchup, "can't contain liquid",
                                   item_pocket::contain_code::ERR_LIQUID );
            expect_cannot_contain( pocket_bucket, mustard, "can't contain liquid",
                                   item_pocket::contain_code::ERR_LIQUID );
        }

        THEN( "it can still contain non-liquid items" ) {
            expect_can_contain( pocket_bucket, rock );
            expect_can_contain( pocket_bucket, glock );
        }
    }
}

// Airtight pockets
// ----------------
// To contain gases, a pocket must have the "airtight" attribute set to true.
//
// Related JSON fields:
// "airtight"
//
// Functions:
// item_pocket::airtight
//
TEST_CASE( "pockets_containing_gases", "[pocket][airtight][gas]" )
{
    item gas( "test_gas", calendar::turn_zero, item::default_charges_tag{} );

    // A potentially airtight container
    pocket_data data_balloon( pocket_type::CONTAINER );

    // Has capacity for several charges of gas
    data_balloon.volume_capacity = 4 * gas.volume();
    data_balloon.max_contains_weight = 4 * gas.weight();
    // Avoid any length restrictions
    data_balloon.max_item_length = 1_meter;

    GIVEN( "pocket is airtight" ) {
        data_balloon.airtight = true;
        item_pocket pocket_balloon( &data_balloon );
        REQUIRE( pocket_balloon.airtight() );

        THEN( "it can contain a gas" ) {
            expect_can_contain( pocket_balloon, gas );
        }

        // TODO: Test that mixing gases in a pocket is prohibited, like it is for liquids.
    }

    GIVEN( "pocket is not airtight" ) {
        // Leaky balloon!
        data_balloon.airtight = false;
        item_pocket pocket_balloon( &data_balloon );
        REQUIRE_FALSE( pocket_balloon.airtight() );

        THEN( "it cannot contain a gas" ) {
            expect_cannot_contain( pocket_balloon, gas, "can't contain gas",
                                   item_pocket::contain_code::ERR_GAS );
        }
    }
}

// Pocket rigidity
// ---------------
// When a pocket is "rigid", the total volume of the pocket itself does not change, like it were a
// glass jar (having the same outer volume whether full or empty).
//
// When "rigid" is false (the default), the pocket grows larger as items fill it up.
//
// Related JSON fields:
// "rigid"
//
// Functions:
// item_pocket::can_contain
// item_pocket::insert_item
// item_pocket::item_size_modifier
//
TEST_CASE( "rigid_and_non-rigid_or_flexible_pockets", "[pocket][rigid][flexible]" )
{
    item rock( "test_rock" );

    // Pocket with enough space for 2 rocks
    pocket_data data_sock( pocket_type::CONTAINER );
    data_sock.volume_capacity = 2 * rock.volume();

    // Can hold plenty of length and weight (we only care about volume here)
    data_sock.max_item_length = 5 * rock.length();
    data_sock.max_contains_weight = 10 * rock.weight();

    GIVEN( "a non-rigid pocket" ) {
        // Sock is freshly washed, so it's non-rigid
        data_sock.rigid = false;
        item_pocket pocket_sock( &data_sock );

        WHEN( "pocket is empty" ) {
            REQUIRE( pocket_sock.empty() );
            THEN( "it can contain an item" ) {
                expect_can_contain( pocket_sock, rock );
            }

            THEN( "item_size_modifier is zero" ) {
                CHECK( pocket_sock.item_size_modifier() == 0_ml );
            }
        }

        WHEN( "pocket is partially filled" ) {
            // One rock should fill the sock half-way
            expect_can_insert( pocket_sock, item( "test_rock" ) );
            REQUIRE_FALSE( pocket_sock.empty() );

            THEN( "it can contain another item" ) {
                expect_can_contain( pocket_sock, rock );
            }

            THEN( "item_size_modifier is the contained item volume" ) {
                CHECK( pocket_sock.item_size_modifier() == rock.volume() );
            }
        }

        WHEN( "pocket is full" ) {
            // Two rocks should be enough to fill it
            expect_can_insert( pocket_sock, item( "test_rock" ) );
            expect_can_insert( pocket_sock, item( "test_rock" ) );

            REQUIRE_FALSE( pocket_sock.empty() );
            REQUIRE( pocket_sock.full( true ) );

            THEN( "it cannot contain another item" ) {
                expect_cannot_contain( pocket_sock, rock, "not enough space",
                                       item_pocket::contain_code::ERR_NO_SPACE );
            }

            THEN( "item_size_modifier is the total of contained item volumes" ) {
                CHECK( pocket_sock.item_size_modifier() == 2 * rock.volume() );
            }
        }
    }

    GIVEN( "a rigid pocket" ) {
        // Our sock got all crusty with zombie guts and is rigid now.
        data_sock.rigid = true;
        item_pocket pocket_sock( &data_sock );
        REQUIRE( pocket_sock.rigid() );

        WHEN( "pocket is empty" ) {
            REQUIRE( pocket_sock.empty() );

            THEN( "it can contain an item" ) {
                expect_can_contain( pocket_sock, rock );
            }

            THEN( "item_size_modifier is zero" ) {
                CHECK( pocket_sock.item_size_modifier() == 0_ml );
            }
        }

        WHEN( "pocket is full" ) {
            expect_can_insert( pocket_sock, item( "test_rock" ) );
            expect_can_insert( pocket_sock, item( "test_rock" ) );

            REQUIRE_FALSE( pocket_sock.empty() );
            REQUIRE( pocket_sock.full( true ) );

            THEN( "it cannot contain another item" ) {
                expect_cannot_contain( pocket_sock, rock, "not enough space",
                                       item_pocket::contain_code::ERR_NO_SPACE );
            }

            THEN( "item_size_modifier is still zero" ) {
                CHECK( pocket_sock.item_size_modifier() == 0_ml );
            }
        }
    }
}

// Corpses
// -------
//
// Functions:
// item_pocket::can_contain
// item_pocket::insert_item
//
TEST_CASE( "corpse_can_contain_anything", "[pocket][corpse]" )
{
    item rock( "test_rock" );
    item glock( "test_glock" );

    pocket_data data_corpse( pocket_type::CORPSE );

    GIVEN( "a corpse" ) {
        item_pocket pocket_corpse( &data_corpse );

        // For corpses, can_contain is true to prevent unnecessary "spilling"
        THEN( "it can contain items" ) {
            expect_can_contain( pocket_corpse, rock );
            expect_can_contain( pocket_corpse, glock );
        }

        THEN( "items can be added to the corpse" ) {
            expect_can_insert( pocket_corpse, rock );
            expect_can_insert( pocket_corpse, glock );
        }
    }
}

// Sealed pockets
// --------------
//
// Functions:
// item_pocket::seal
// item_pocket::unseal
// item_pocket::sealed
// item_pocket::sealable
//
TEST_CASE( "sealed_containers", "[pocket][seal]" )
{
    item water( "water" );

    GIVEN( "sealable can" ) {
        item can( "test_can_drink" );

        // Ensure it has exactly one contained pocket, and get that pocket for testing
        std::vector<item_pocket *> can_pockets = can.get_all_contained_pockets();
        REQUIRE( !can_pockets.empty() );
        REQUIRE( can_pockets.size() == 1 );
        item_pocket &pocket = *can_pockets.front();
        // Must be sealable, but not sealed initially
        REQUIRE( pocket.sealable() );
        REQUIRE_FALSE( pocket.sealed() );

        // Sealing does not work when empty
        WHEN( "pocket is empty" ) {
            REQUIRE( pocket.empty() );
            THEN( "it cannot be sealed" ) {
                CHECK_FALSE( pocket.seal() );
                CHECK_FALSE( pocket.sealed() );
                // Remains sealable
                CHECK( pocket.sealable() );
            }
        }

        // But after inserting something, sealing can succeed
        WHEN( "pocket contains something" ) {
            REQUIRE( pocket.insert_item( water ).success() );
            THEN( "it can be sealed" ) {
                CHECK( pocket.seal() );
                CHECK( pocket.sealed() );
                // Remains sealable
                CHECK( pocket.sealable() );
            }
        }

        GIVEN( "pocket is already sealed" ) {
            REQUIRE( pocket.insert_item( water ).success() );
            REQUIRE( pocket.seal() );
            REQUIRE( pocket.sealed() );

            WHEN( "it becomes unsealed" ) {
                pocket.unseal();
                THEN( "it is no longer sealed" ) {
                    CHECK_FALSE( pocket.sealed() );
                    // Remains sealable
                    CHECK( pocket.sealable() );
                }
            }
        }
    }

    GIVEN( "non-sealable jug" ) {
        item jug( itype_test_jug_plastic );

        // Ensure it has exactly one contained pocket, and get that pocket for testing
        std::vector<item_pocket *>jug_pockets = jug.get_all_contained_pockets();
        REQUIRE( !jug_pockets.empty() );
        REQUIRE( jug_pockets.size() == 1 );
        item_pocket &pocket = *jug_pockets.front();
        // Must NOT be sealable
        REQUIRE_FALSE( pocket.sealable() );
        REQUIRE_FALSE( pocket.sealed() );

        THEN( "it cannot be sealed" ) {
            // Sealing fails when empty
            REQUIRE( pocket.empty() );
            CHECK_FALSE( pocket.seal() );
            CHECK_FALSE( pocket.sealed() );
            // Sealing also fails after inserting item
            REQUIRE( pocket.insert_item( water ).success() );
            CHECK_FALSE( pocket.seal() );
            CHECK_FALSE( pocket.sealed() );
        }
    }
}

// Better pockets
// --------------
// Pockets are ranked according to item_pocket::better_pocket, which considers player-defined
// favorites, ammo restrictions, spoilage, watertightness, available volume and other factors.
//
// Functions:
// item_pocket::better_pocket
//
TEST_CASE( "when_one_pocket_is_better_than_another", "[pocket][better]" )
{
    // TODO:
    // settings.is_better_favorite() is top priority
    // settings.priority() takes next priority
    // has_item_stacks_with(it) is better than one that doesn't stack
    // pockets restricted by ammo should try to get filled first
    // pockets restricted by flag should try to get filled first
    // if remaining volume is equal, lower obtain_cost is better
    // pockets with less extra encumbrance should be prioritized (#53162)
    // pockets without ripoff chance should be prioritized (#53162)

    // A and B: Two generic sets of pocket data for comparison
    pocket_data data_a( pocket_type::CONTAINER );
    pocket_data data_b( pocket_type::CONTAINER );

    // Candidate items to compare pockets with
    item liquid( "test_liquid" );
    item apple( "test_apple" );
    item rock( "test_rock" );

    SECTION( "for perishable food, lower spoil_multiplier is better" ) {
        REQUIRE( apple.is_comestible() );
        REQUIRE( apple.get_comestible()->spoils != 0_seconds );
        data_a.spoil_multiplier = 1.0;
        data_b.spoil_multiplier = 0.5;
        CHECK( item_pocket( &data_a ).better_pocket( item_pocket( &data_b ), apple ) );
    }

    SECTION( "for solid item, non-watertight pocket is better" ) {
        REQUIRE( rock.made_of( phase_id::SOLID ) );
        data_a.watertight = true;
        data_b.watertight = false;
        CHECK( item_pocket( &data_a ).better_pocket( item_pocket( &data_b ), rock ) );
    }

    SECTION( "rigid pockets are preferable to non-rigid ones" ) {
        data_a.rigid = false;
        data_b.rigid = true;
        CHECK( item_pocket( &data_a ).better_pocket( item_pocket( &data_b ), rock ) );
    }

    SECTION( "pocket with less remaining volume is better" ) {
        data_a.volume_capacity = 2_liter;
        data_b.volume_capacity = 1_liter;
        CHECK( item_pocket( &data_a ).better_pocket( item_pocket( &data_b ), rock ) );
    }
}

// Best pocket
// -----------
// The "best pocket" for an item is the most appropriate pocket for containing it. Only CONTAINER
// type pockets are eligible; other pocket types such as MAGAZINE or MOD cannot be "best".
//
// Functions:
// item::best_pocket
// item_contents::best_pocket
//
static bool has_best_pocket( item &container, const item &thing )
{
    item_location loc;
    return container.best_pocket( thing, loc ).second != nullptr;
}

/** Returns the only pocket for an item. */
static item_pocket *get_only_pocket( item &container )
{
    std::vector<item_pocket *> pockets = container.get_all_contained_pockets();
    REQUIRE( pockets.size() == 1 );
    return pockets[0];
}

TEST_CASE( "best_pocket_in_item_contents", "[pocket][item][best]" )
{
    item_location loc;

    // Waterskins can be best pockets for liquids
    SECTION( "item with one watertight pocket has best_pocket for liquid" ) {
        // Must have a CONTAINER pocket, first and foremost
        item skin( "test_waterskin" );
        REQUIRE( skin.is_container() );
        // Prerequisite: It can contain water
        item liquid( "test_liquid" );
        REQUIRE( skin.can_contain( liquid ).success() );

        // Has a best pocket for liquid
        CHECK( has_best_pocket( skin, liquid ) );
    }

    // Test utility belt has pockets best for holding small and large tools, liquids and gases
    SECTION( "item with many different pockets can have best_pocket for different items" ) {
        // Utility belt has CONTAINER pockets
        item util_belt( "test_utility_belt" );
        REQUIRE( util_belt.is_container() );
        // It can contain small and large tools
        item screwdriver( "test_screwdriver" );
        item halligan( "test_halligan" );
        REQUIRE( util_belt.can_contain( screwdriver ).success() );
        REQUIRE( util_belt.can_contain( halligan ).success() );
        // It can contain liquid and gas
        item liquid( "test_liquid" );
        item gas( "test_gas", calendar::turn_zero, item::default_charges_tag{} );
        REQUIRE( util_belt.can_contain( liquid ).success() );
        REQUIRE( util_belt.can_contain( gas ).success() );

        // Utility belt has best_pocket for all these things
        CHECK( has_best_pocket( util_belt, screwdriver ) );
        CHECK( has_best_pocket( util_belt, halligan ) );
        CHECK( has_best_pocket( util_belt, liquid ) );
        CHECK( has_best_pocket( util_belt, gas ) );
    }

    // Because best_pocket only works for CONTAINER pockets, a gun item with a MAGAZINE_WELL pocket
    // is not best_pocket for a compatible magazine, and a mag item with a MAGAZINE pocket is not
    // best_pocket for compatible ammos.
    SECTION( "non-container pockets cannot be best_pocket" ) {
        // Gun that accepts magazines
        item glock( "test_glock" );
        REQUIRE( glock.has_pocket_type( pocket_type::MAGAZINE_WELL ) );
        // Empty magazine
        item glockmag( "test_glockmag", calendar::turn, 0 );
        REQUIRE( glockmag.has_pocket_type( pocket_type::MAGAZINE ) );
        REQUIRE( glockmag.ammo_remaining() == 0 );
        // A single 9mm bullet
        item glockammo( "test_9mm_ammo", calendar::turn, 1 );
        REQUIRE( glockammo.is_ammo() );
        REQUIRE( glockammo.charges == 1 );

        // Although gun can contain magazine, and magazine can contain bullet...
        REQUIRE( glock.can_contain( glockmag ).success() );
        REQUIRE( glockmag.can_contain( glockammo ).success() );
        // Gun is not best_pocket for magazine, and magazine is not best_pocket for bullet.
        CHECK_FALSE( has_best_pocket( glock, glockmag ) );
        CHECK_FALSE( has_best_pocket( glockmag, glockammo ) );
    }

    // sealable pockets may be filled and sealed, to spawn with a "factory seal" in-game.
    // While sealed, they clearly cannot be the best pocket for storing anything.
    SECTION( "sealed pockets cannot be best_pocket" ) {
        // Regular aluminum beverage can and something to fill it with
        item can( "test_can_drink" );
        REQUIRE( can.is_container() );
        item liquid( "test_liquid" );
        REQUIRE( can.can_contain( liquid ).success() );

        // Before being sealed, it can be best pocket for liquid
        CHECK( has_best_pocket( can, liquid ) );
        // Fill with liquid, seal it, and ensure success
        can.put_in( liquid, pocket_type::CONTAINER );
        REQUIRE( can.seal() ); // This must succeed, or next assertion is meaningless
        // Now sealed, the can cannot be best_pocket for liquid
        CHECK_FALSE( has_best_pocket( can, liquid ) );
    }

    SECTION( "pockets with favorite settings" ) {
        item liquid( "test_liquid" );

        WHEN( "item is blacklisted" ) {
            item skin( "test_waterskin" );
            REQUIRE( has_best_pocket( skin, liquid ) );
            get_only_pocket( skin )->settings.blacklist_item( liquid.typeId() );

            THEN( "pocket cannot be best pocket" ) {
                CHECK_FALSE( has_best_pocket( skin, liquid ) );
            }
        }

        WHEN( "item is whitelisted" ) {
            item skin( "test_waterskin" );
            REQUIRE( has_best_pocket( skin, liquid ) );
            get_only_pocket( skin )->settings.whitelist_item( liquid.typeId() );

            THEN( "pocket can be best pocket" ) {
                CHECK( has_best_pocket( skin, liquid ) );
            }
        }
    }
}

// Pocket favorites
// ----------------
// Item pockets can be configured to only allow certain items automatically.
// The rules defined are also known as pocket autopickup rules.
//
// Functions:
// item_pocket::favorite_settings::accepts_item
// item_pocket::favorite_settings::blacklist_item
// item_pocket::favorite_settings::whitelist_item
// item_pocket::favorite_settings::get_item_whitelist
// item_pocket::favorite_settings::get_item_blacklist
//
TEST_CASE( "pocket_favorites_allow_or_restrict_items", "[pocket][favorite][item]" )
{
    item_location loc;

    item test_item( "test_rock" );

    item test_item_same_category( "test_rag" );
    REQUIRE( test_item.get_category_shallow().id ==
             test_item_same_category.get_category_shallow().id );
    REQUIRE_FALSE( test_item.typeId() == test_item_same_category.typeId() );

    item test_item_different_category( "test_apple" );
    REQUIRE_FALSE( test_item.get_category_shallow().id ==
                   test_item_different_category.get_category_shallow().id );
    REQUIRE_FALSE( test_item.typeId() == test_item_different_category.typeId() );

    // Default settings allow all items
    SECTION( "no favourites" ) {
        WHEN( "no whitelist or blacklist specified" ) {
            item_pocket::favorite_settings settings;
            THEN( "all items allowed" ) {
                CHECK( settings.accepts_item( test_item ) );
                CHECK( settings.accepts_item( test_item_same_category ) );
                CHECK( settings.accepts_item( test_item_different_category ) );
            }
        }
    }

    SECTION( "blacklists" ) {
        WHEN( "item category blacklisted" ) {
            item_pocket::favorite_settings settings;
            settings.blacklist_category( test_item.get_category_shallow().id );
            THEN( "that category blocked" ) {
                REQUIRE_FALSE( settings.accepts_item( test_item ) );
                REQUIRE_FALSE( settings.accepts_item( test_item_same_category ) );
                REQUIRE( settings.accepts_item( test_item_different_category ) );
            }
        }

        WHEN( "item blacklisted" ) {
            item_pocket::favorite_settings settings;
            settings.blacklist_item( test_item.typeId() );
            THEN( "that item blocked" ) {
                REQUIRE_FALSE( settings.accepts_item( test_item ) );
                REQUIRE( settings.accepts_item( test_item_same_category ) );
                REQUIRE( settings.accepts_item( test_item_different_category ) );
            }
        }
    }

    SECTION( "whitelists" ) {
        WHEN( "item category whitelisted" ) {
            item_pocket::favorite_settings settings;
            settings.whitelist_category( test_item.get_category_shallow().id );
            THEN( "other categories blocked" ) {
                REQUIRE( settings.accepts_item( test_item ) );
                REQUIRE( settings.accepts_item( test_item_same_category ) );
                REQUIRE_FALSE( settings.accepts_item( test_item_different_category ) );
            }
        }

        WHEN( "item whitelisted" ) {
            item_pocket::favorite_settings settings;
            settings.whitelist_item( test_item.typeId() );
            THEN( "other items blocked" ) {
                REQUIRE( settings.accepts_item( test_item ) );
                REQUIRE_FALSE( settings.accepts_item( test_item_same_category ) );
                REQUIRE_FALSE( settings.accepts_item( test_item_different_category ) );
            }
        }
    }

    SECTION( "mixing whitelist and blacklists" ) {
        WHEN( "same item first whitelisted then blacklisted" ) {
            item_pocket::favorite_settings settings;
            settings.whitelist_item( test_item.typeId() );
            REQUIRE( settings.get_item_whitelist().count( test_item.typeId() ) );
            settings.blacklist_item( test_item.typeId() );
            THEN( "item should be removed from the whitelist" ) {
                REQUIRE_FALSE( settings.get_item_whitelist().count( test_item.typeId() ) );
            }
        }

        WHEN( "same item first blacklisted then whitelisted" ) {
            item_pocket::favorite_settings settings;
            settings.blacklist_item( test_item.typeId() );
            REQUIRE( settings.get_item_blacklist().count( test_item.typeId() ) );
            settings.whitelist_item( test_item.typeId() );
            THEN( "item should be removed from the blacklist" ) {
                REQUIRE_FALSE( settings.get_item_blacklist().count( test_item.typeId() ) );
            }
        }

        WHEN( "same category first whitelisted then blacklisted" ) {
            item_pocket::favorite_settings settings;
            settings.whitelist_category( test_item.get_category_shallow().id );
            REQUIRE( settings.get_category_whitelist().count( test_item.get_category_shallow().id ) );
            settings.blacklist_category( test_item.get_category_shallow().id );
            THEN( "category should be removed from the whitelist" ) {
                REQUIRE_FALSE( settings.get_category_whitelist().count( test_item.get_category_shallow().id ) );
            }
        }

        WHEN( "same category first blacklisted then whitelisted" ) {
            item_pocket::favorite_settings settings;
            settings.blacklist_category( test_item.get_category_shallow().id );
            REQUIRE( settings.get_category_blacklist().count( test_item.get_category_shallow().id ) );
            settings.whitelist_category( test_item.get_category_shallow().id );
            THEN( "category should be removed from the blacklist" ) {
                REQUIRE_FALSE( settings.get_category_blacklist().count( test_item.get_category_shallow().id ) );
            }
        }

        WHEN( "category whitelisted but item blacklisted" ) {
            item_pocket::favorite_settings settings;
            settings.whitelist_category( test_item.get_category_shallow().id );
            settings.blacklist_item( test_item.typeId() );
            THEN( "item blacklist override category allownace" ) {
                REQUIRE_FALSE( settings.accepts_item( test_item ) );
                REQUIRE( settings.accepts_item( test_item_same_category ) );
                // Not blacklisted, but not whitelisted either
                REQUIRE_FALSE( settings.accepts_item( test_item_different_category ) );
            }
        }

        WHEN( "category whitelisted and item whitelisted" ) {
            item_pocket::favorite_settings settings;
            settings.whitelist_category( test_item.get_category_shallow().id );
            settings.whitelist_item( test_item.typeId() );
            THEN( "either category or item must match" ) {
                REQUIRE( settings.accepts_item( test_item ) );
                REQUIRE( settings.accepts_item( test_item_same_category ) );
                REQUIRE_FALSE( settings.accepts_item( test_item_different_category ) );
            }
        }

        WHEN( "category blacklisted but item whitelisted" ) {
            item_pocket::favorite_settings settings;
            settings.blacklist_category( test_item.get_category_shallow().id );
            settings.whitelist_item( test_item.typeId() );
            THEN( "item allowance override category restrictions" ) {
                REQUIRE( settings.accepts_item( test_item ) );
                REQUIRE_FALSE( settings.accepts_item( test_item_same_category ) );
                // Not blacklisted, there's no category whitelist
                REQUIRE( settings.accepts_item( test_item_different_category ) );
            }
        }

        WHEN( "category blacklisted and item blacklisted" ) {
            item_pocket::favorite_settings settings;
            settings.blacklist_category( test_item.get_category_shallow().id );
            settings.blacklist_item( test_item.typeId() );
            THEN( "both category and item are blocked" ) {
                REQUIRE_FALSE( settings.accepts_item( test_item ) );
                REQUIRE_FALSE( settings.accepts_item( test_item_same_category ) );
                // Not blacklisted
                REQUIRE( settings.accepts_item( test_item_different_category ) );
            }
        }
    }
}

TEST_CASE( "pocket_favorites_allow_or_restrict_containers", "[pocket][favorite][item]" )
{
    item_pocket::favorite_settings settings;

    GIVEN( "item container is empty" ) {
        item item_plastic_bag = item( "bag_plastic" );
        REQUIRE( item_plastic_bag.empty() );

        WHEN( "item container is whitelisted" ) {
            settings.whitelist_item( item_plastic_bag.typeId() );

            THEN( "item container should be accepted" ) {
                REQUIRE( settings.accepts_item( item_plastic_bag ) );
            }
        }
        WHEN( "item container is blacklisted" ) {
            settings.blacklist_item( item_plastic_bag.typeId() );

            THEN( "item container should not be accepted" ) {
                REQUIRE_FALSE( settings.accepts_item( item_plastic_bag ) );
            }
        }
        WHEN( "item container category is whitelisted" ) {
            settings.whitelist_category( item_plastic_bag.get_category_shallow().id );

            THEN( "item container should be accepted" ) {
                REQUIRE( settings.accepts_item( item_plastic_bag ) );
            }
        }
        WHEN( "item container category is blacklisted" ) {
            settings.blacklist_category( item_plastic_bag.get_category_shallow().id );

            THEN( "item container should not be accepted" ) {
                REQUIRE_FALSE( settings.accepts_item( item_plastic_bag ) );
            }
        }
        WHEN( "item container is not listed in rules" ) {
            THEN( "item container should be accepted" ) {
                REQUIRE( settings.accepts_item( item_plastic_bag ) );
            }
        }
    }

    GIVEN( "item container is not empty" ) {
        item item_plastic_bag = item( "bag_plastic" );
        item item_paper = item( "paper" );
        item item_pencil = item( "pencil" );

        item_plastic_bag.force_insert_item( item_paper, pocket_container );
        item_plastic_bag.force_insert_item( item_pencil, pocket_container );

        WHEN( "all items in container are whitelisted" ) {
            settings.whitelist_item( item_paper.typeId() );
            settings.whitelist_item( item_pencil.typeId() );

            THEN( "container should be accepted" ) {
                REQUIRE( settings.accepts_item( item_plastic_bag ) );
                REQUIRE_FALSE( settings.accepts_item( item( "bag_plastic" ) ) );
            }
            WHEN( "item container is blacklisted" ) {
                settings.blacklist_item( item_plastic_bag.typeId() );

                THEN( "item container should not be accepted" ) {
                    REQUIRE_FALSE( settings.accepts_item( item_plastic_bag ) );
                }
            }
        }
        WHEN( "only some items in container are whitelisted" ) {
            settings.whitelist_item( item_paper.typeId() );

            THEN( "container should not be accepted" ) {
                REQUIRE_FALSE( settings.accepts_item( item_plastic_bag ) );
            }
            WHEN( "item container is whitelisted" ) {
                settings.whitelist_item( item_plastic_bag.typeId() );

                THEN( "item container should be accepted" ) {
                    REQUIRE( settings.accepts_item( item_plastic_bag ) );
                }
            }
        }
        WHEN( "all item categories in container are whitelisted" ) {
            settings.whitelist_category( item_paper.get_category_shallow().id );
            settings.whitelist_category( item_pencil.get_category_shallow().id );

            THEN( "container should be accepted" ) {
                REQUIRE( settings.accepts_item( item_plastic_bag ) );
                REQUIRE_FALSE( settings.accepts_item( item( "bag_plastic" ) ) );
            }
            WHEN( "item container category is blacklisted" ) {
                settings.blacklist_category( item_plastic_bag.get_category_shallow().id );

                THEN( "item container should not be accepted" ) {
                    REQUIRE_FALSE( settings.accepts_item( item_plastic_bag ) );
                }
            }
        }
        WHEN( "only some item categories in container are whitelisted" ) {
            settings.whitelist_category( item_paper.get_category_shallow().id );

            THEN( "container should not be accepted" ) {
                REQUIRE_FALSE( settings.accepts_item( item_plastic_bag ) );
            }
            WHEN( "item container category is whitelisted" ) {
                settings.whitelist_category( item_plastic_bag.get_category_shallow().id );

                THEN( "item container should be accepted" ) {
                    REQUIRE( settings.accepts_item( item_plastic_bag ) );
                }
            }
        }
        WHEN( "no items in container are listed in rules" ) {
            THEN( "container should be accepted" ) {
                REQUIRE( settings.accepts_item( item_plastic_bag ) );
            }
        }
    }

    GIVEN( "item container contains liquid" ) {
        item item_clean_water = item( "water_clean" );
        item item_bottled_water = item( "bottle_plastic" );
        REQUIRE( item_bottled_water.fill_with( item_clean_water ) > 0 );

        WHEN( "liquid item is whitelisted" ) {
            settings.whitelist_item( item_clean_water.typeId() );
            REQUIRE( settings.accepts_item( item_clean_water ) );

            THEN( "item container containing liquid item should be accepted" ) {
                REQUIRE( settings.accepts_item( item_bottled_water ) );
                REQUIRE_FALSE( settings.accepts_item( item( "bottle_plastic" ) ) );
            }
        }
        WHEN( "liquid item is blacklisted" ) {
            settings.blacklist_item( item_clean_water.typeId() );
            REQUIRE_FALSE( settings.accepts_item( item_clean_water ) );

            THEN( "item container containing liquid item should not be accepted" ) {
                REQUIRE_FALSE( settings.accepts_item( item_bottled_water ) );
                REQUIRE( settings.accepts_item( item( "bottle_plastic" ) ) );
            }
        }
        WHEN( "liquid item is not listed in rules" ) {
            settings.whitelist_item( item( "paper" ).typeId() );

            THEN( "item container containing liquid item should not be accepted" ) {
                REQUIRE_FALSE( settings.accepts_item( item_bottled_water ) );
            }
        }
    }
}

/**
  * Add the given item to the best pocket of the dummy character.
  * @param dummy The character to add the item to.
  * @param it The item to add to the characters' pockets.
  *
  * @returns A pointer to the newly added item, if successfull.
  */
static item *add_item_to_best_pocket( Character &dummy, const item &it )
{
    item_pocket *pocket = dummy.best_pocket( it ).second;
    REQUIRE( pocket );

    item *ret = nullptr;
    pocket->add( it, &ret );
    REQUIRE( pocket->has_item( *ret ) );

    return ret;
}

// Character::best_pocket
// - See if wielded item can hold it - start with this as default
// - For each worn item, see if best_pocket is better; if so, use it
// + Return the item_location of the item that has the best pocket
//
// What is the best pocket to put @it into? the pockets in @avoid do not count
// Character::best_pocket( it, avoid )
// NOTE: different syntax than item_contents::best_pocket
// (Second argument is `avoid` item pointer, not parent item location)
TEST_CASE( "character_best_pocket", "[pocket][character][best]" )
{
    item_location loc;
    Character &dummy = get_player_character();

    // reset dummy equipment before each test.
    clear_avatar();

    WHEN( "the player is wielding a container" ) {
        item socks( itype_test_socks );
        item container( itype_test_watertight_open_sealed_container_1L );

        // wield the container item.
        dummy.set_wielded_item( container );

        THEN( "the player can stash an item in it" ) {
            // check to see if the item can be stored in any pocket.
            CHECK( dummy.can_stash_partial( socks ) );

            // and check to see if the item is actually stored in the wielded container.
            std::pair<item_location, item_pocket *> pocket = dummy.best_pocket( socks );
            REQUIRE( pocket.second );
            CHECK( *pocket.second == *get_only_pocket( container ) );
        }
    }

    WHEN( "the player is wearing a container" ) {
        item socks( itype_test_socks );
        item backpack( itype_test_backpack );

        // wear the backpack item.
        REQUIRE( dummy.wear_item( backpack, false, false ) );

        THEN( "the player can stash an item in it" ) {
            // check to see if the item can be stored in any pocket.
            CHECK( dummy.can_stash_partial( socks ) );

            // and check to see if the item is actually stored in the wielded container.
            std::pair<item_location, item_pocket *> pocket = dummy.best_pocket( socks );
            REQUIRE( pocket.second );
            CHECK( *pocket.second == *get_only_pocket( backpack ) );
        }
    }

    WHEN( "wielding- and wearing a container" ) {
        item socks( itype_test_socks );
        item container_wear( itype_test_watertight_open_sealed_container_1L );
        item container_wield( itype_test_watertight_open_sealed_container_1L );

        // wield- and wear the respective container items.
        REQUIRE( dummy.wield( container_wield ) );
        REQUIRE( dummy.wear_item( container_wear, false, false ) );

        THEN( "the wielded container has priority" ) {
            // check to see if the item can be stored in any pocket.
            CHECK( dummy.can_stash_partial( socks ) );

            // and try to find a fitting pocket.
            // then check to see if it is, indeed, the wielded container.
            std::pair<item_location, item_pocket *> pocket = dummy.best_pocket( socks );
            REQUIRE( pocket.second );
            CHECK( *pocket.second == *get_only_pocket( container_wield ) );
        }
    }

    WHEN( "wearing a container with a nested rigid container" ) {
        item socks( itype_test_socks );
        item backpack( itype_test_backpack );
        item container( itype_test_jug_plastic );
        item filler( "test_rag" );

        // wear the backpack item.
        REQUIRE( dummy.wear_item( backpack ) );

        // nest the rigid container inside of the worn backpack.
        add_item_to_best_pocket( dummy, container );

        // fill the parent container
        dummy.i_add_or_drop( filler, 55 );

        THEN( "best pocket will be in nested rigid container" ) {
            // check to see if the item can be stored in any pocket.
            CHECK( dummy.can_stash_partial( socks ) );

            // and try to find a fitting pocket.
            // then check to see if it is, indeed, the nested container.
            std::pair<item_location, item_pocket *> best_pocket = dummy.best_pocket( socks );
            REQUIRE( best_pocket.second );
            CHECK( *best_pocket.second == *get_only_pocket( container ) );
        }
    }

    WHEN( "wearing a container with a nested non-rigid container" ) {
        item socks( itype_test_socks );
        item backpack( itype_test_backpack );
        item backpack_nested( itype_test_backpack );

        // wear the backpack item.
        REQUIRE( dummy.wear_item( backpack ) );

        // nest the non-rigid container inside of the worn backpack.
        add_item_to_best_pocket( dummy, backpack_nested );

        THEN( "best pocket will be in worn container" ) {
            // check to see if the item can be stored in any pocket.
            CHECK( dummy.can_stash_partial( socks ) );

            // and try to find a fitting pocket.
            // then check to see if it is, indeed, the worn container.
            std::pair<item_location, item_pocket *> best_pocket = dummy.best_pocket( socks );
            REQUIRE( best_pocket.second );
            CHECK( *best_pocket.second == *get_only_pocket( backpack ) );
        }
    }

    WHEN( "wearing a container with a nested rigid container which should be avoided" ) {
        item socks( itype_test_socks );
        item backpack( itype_test_backpack );
        item container( itype_test_jug_plastic );

        // wear the backpack item.
        REQUIRE( dummy.wear_item( backpack ) );

        // nest the rigid container inside of the worn backpack.
        item *nested_container = add_item_to_best_pocket( dummy, container );

        THEN( "best pocket for new item will be in worn container" ) {
            // check to see if the item can be stored in any pocket.
            CHECK( dummy.can_stash_partial( socks ) );

            // and try to find a fitting pocket.
            // then check to see if it is, indeed, the worn backpack
            // when the nested (preferable) container should be avoided.
            std::pair<item_location, item_pocket *> best_pocket = dummy.best_pocket( socks, nested_container );
            REQUIRE( best_pocket.second );
            CHECK( *best_pocket.second == *get_only_pocket( backpack ) );
        }
    }
}

TEST_CASE( "guns_and_gunmods", "[pocket][gunmod]" )
{
    item m4a1( "modular_m4_carbine" );
    item strap( "shoulder_strap" );
    // Guns cannot "contain" gunmods, but gunmods can be inserted into guns
    CHECK_FALSE( m4a1.can_contain( strap ).success() );
    CHECK( m4a1.put_in( strap, pocket_type::MOD ).success() );
}

TEST_CASE( "usb_drives_and_software", "[pocket][software]" )
{
    item usb( "usb_drive" );
    item software( "software_math" );
    // USB drives aren't containers, and cannot "contain" software, but software can be inserted
    CHECK_FALSE( usb.can_contain( software ).success() );
    CHECK( usb.put_in( software, pocket_type::SOFTWARE ).success() );
}

static void test_pickup_autoinsert_results( Character &u, bool wear, const item_location &nested,
        size_t on_ground, size_t in_top, size_t in_nested, bool count_by_charges = false )
{
    map &m = get_map();
    u.set_moves( 100 );
    while( !u.activity.is_null() ) {
        u.activity.do_turn( u );
    }
    if( count_by_charges ) {
        size_t charges_on_ground = 0;
        for( item &it : m.i_at( u.pos() ) ) {
            charges_on_ground += it.charges;
        }
        CHECK( charges_on_ground == on_ground );
    } else {
        CHECK( m.i_at( u.pos() ).size() == on_ground );
    }
    if( !wear ) {
        CHECK( !!u.get_wielded_item() );
        if( count_by_charges ) {
            size_t charges_in_top = -1;
            for( item *it : u.get_wielded_item()->all_items_top() ) {
                if( !nested || it->typeId() != nested->typeId() ) {
                    charges_in_top = it->charges;
                }
            }
            CHECK( charges_in_top == in_top );
        } else {
            CHECK( u.get_wielded_item()->all_items_top().size() == in_top );
        }
        CHECK( u.top_items_loc().empty() );
    } else {
        CHECK( !u.get_wielded_item() );
        CHECK( u.top_items_loc().size() == 1 );
        if( count_by_charges ) {
            size_t charges_in_top = -1;
            for( item *it : u.top_items_loc().front()->all_items_top() ) {
                if( !nested || it->typeId() != nested->typeId() ) {
                    charges_in_top = it->charges;
                }
            }
            CHECK( charges_in_top == in_top );
        } else {
            CHECK( u.top_items_loc().front()->all_items_top().size() == in_top );
        }
    }
    if( !!nested ) {
        if( count_by_charges ) {
            size_t charges_in_top = 0;
            for( const item *it : nested->all_items_top() ) {
                charges_in_top += it->charges;
            }
            CHECK( charges_in_top == in_nested );
        } else {
            CHECK( nested->all_items_top().size() == in_nested );
        }
        item *top_it = wear ? &u.worn.front() : &*u.get_wielded_item();
        // top-level container still contains nested container
        CHECK( !!top_it->contained_where( *nested.get_item() ) );
    }
}

static item_location give_item_to_char( Character &u, item_location &i )
{
    std::string id = random_string( 10 );
    i->set_var( "uid", id );
    i.obtain( u );
    return item_location( u, u.items_with( [id]( const item & it ) {
        return it.get_var( "uid" ) == id;
    } ).front() );
}

static void test_pickup_autoinsert_sub_sub( bool autopickup, bool wear, bool soft_nested )
{
    item cont_top_soft( "test_backpack" );
    item cont_nest_soft( "test_balloon" );
    item cont_nest_rigid( "test_jug_large_open" );
    item rigid_obj( "test_rock" );
    item soft_obj( "test_rag" );

    map &m = get_map();
    Character &u = get_player_character();
    clear_map();
    clear_character( u, true );
    item_location cont1( map_cursor( u.pos() ), &m.add_item_or_charges( u.pos(), cont_nest_rigid ) );
    item_location cont2( map_cursor( u.pos() ), &m.add_item_or_charges( u.pos(), cont_nest_soft ) );
    item_location obj1( map_cursor( u.pos() ), &m.add_item_or_charges( u.pos(), rigid_obj ) );
    item_location obj2( map_cursor( u.pos() ), &m.add_item_or_charges( u.pos(), soft_obj ) );
    pickup_activity_actor act_actor( { obj1, obj2 }, { 1, 1 }, u.pos(), autopickup );
    u.assign_activity( act_actor );

    item_location pack;
    if( wear ) {
        u.wear_item( cont_top_soft, false );
        pack = item_location( u.top_items_loc().front() );
        REQUIRE( pack.get_item() != nullptr );
        REQUIRE( m.i_at( u.pos() ).size() == 4 );
        REQUIRE( !u.get_wielded_item() );
        REQUIRE( u.top_items_loc().size() == 1 );
        REQUIRE( u.top_items_loc().front()->all_items_top().empty() );
    } else {
        u.wield( cont_top_soft );
        pack = u.get_wielded_item();
        REQUIRE( pack.get_item() != nullptr );
        REQUIRE( m.i_at( u.pos() ).size() == 4 );
        REQUIRE( !!u.get_wielded_item() );
        REQUIRE( u.get_wielded_item()->all_items_top().empty() );
        REQUIRE( u.top_items_loc().empty() );
    }

    WHEN( "backpack autoinsert as normal" ) {
        WHEN( "space available in backpack" ) {
            THEN( "pickup all" ) {
                test_pickup_autoinsert_results( u, wear, {}, 2, 2, 0 );
            }
        }
        WHEN( "no space available in backpack" ) {
            pack->fill_with( soft_obj, 60, false, false, true );
            THEN( "pickup none" ) {
                test_pickup_autoinsert_results( u, wear, {}, 4, 60, 0 );
            }
        }
    }

    WHEN( "no nested, backpack autoinsert disabled" ) {
        WHEN( "space available in backpack" ) {
            for( item_pocket *&pkts : pack->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_disabled( true );
            }
            THEN( "pickup none" ) {
                test_pickup_autoinsert_results( u, wear, {}, 4, 0, 0 );
            }
        }
        WHEN( "no space available in backpack" ) {
            pack->fill_with( soft_obj, 60, false, false, true );
            for( item_pocket *&pkts : pack->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_disabled( true );
            }
            THEN( "pickup none" ) {
                test_pickup_autoinsert_results( u, wear, {}, 4, 60, 0 );
            }
        }
    }

    WHEN( "no nested, all autoinsert settings as normal" ) {
        WHEN( "space available in backpack" ) {
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            THEN( "pickup all, nested empty" ) {
                test_pickup_autoinsert_results( u, wear, c, 1, 3, 0 );
            }
        }
        WHEN( "no space available in backpack" ) {
            pack->fill_with( soft_obj, soft_nested ? 59 : 44, false, false, true );
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            THEN( ( soft_nested ? "pickup none, nested empty" : "pickup all, nested filled" ) ) {
                if( soft_nested ) {
                    test_pickup_autoinsert_results( u, wear, c, 3, 60, 0 );
                } else {
                    test_pickup_autoinsert_results( u, wear, c, 1, 45, 2 );
                }
            }
        }
    }

    WHEN( "all autoinsert settings disabled" ) {
        WHEN( "space available in backpack" ) {
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            for( item_pocket *&pkts : pack->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_disabled( true );
            }
            for( item_pocket *&pkts : c->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_disabled( true );
            }
            THEN( "pickup none, nested empty" ) {
                test_pickup_autoinsert_results( u, wear, c, 3, 1, 0 );
            }
        }
        WHEN( "no space available in backpack" ) {
            pack->fill_with( soft_obj, soft_nested ? 59 : 44, false, false, true );
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            for( item_pocket *&pkts : pack->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_disabled( true );
            }
            for( item_pocket *&pkts : c->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_disabled( true );
            }
            THEN( "pickup none, nested empty" ) {
                test_pickup_autoinsert_results( u, wear, c, 3, soft_nested ? 60 : 45, 0 );
            }
        }
    }

    WHEN( "top container autoinsert settings disabled" ) {
        WHEN( "space available in backpack" ) {
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            for( item_pocket *&pkts : pack->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_disabled( true );
            }
            THEN( "pickup all, nested filled" ) {
                test_pickup_autoinsert_results( u, wear, c, 1, 1, 2 );
            }
        }
        WHEN( "no space available in backpack" ) {
            pack->fill_with( soft_obj, soft_nested ? 59 : 44, false, false, true );
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            for( item_pocket *&pkts : pack->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_disabled( true );
            }
            THEN( ( soft_nested ? "pickup none, nested empty" : "pickup all, nested filled" ) ) {
                if( soft_nested ) {
                    test_pickup_autoinsert_results( u, wear, c, 3, 60, 0 );
                } else {
                    test_pickup_autoinsert_results( u, wear, c, 1, 45, 2 );
                }
            }
        }
    }

    WHEN( "nested container autoinsert settings disabled" ) {
        WHEN( "space available in backpack" ) {
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            for( item_pocket *&pkts : c->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_disabled( true );
            }
            THEN( "pickup all, nested empty" ) {
                test_pickup_autoinsert_results( u, wear, c, 1, 3, 0 );
            }
        }
        WHEN( "no space available in backpack" ) {
            pack->fill_with( soft_obj, soft_nested ? 59 : 44, false, false, true );
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            for( item_pocket *&pkts : c->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_disabled( true );
            }
            THEN( "pickup none, nested empty" ) {
                test_pickup_autoinsert_results( u, wear, c, 3, soft_nested ? 60 : 45, 0 );
            }
        }
    }

    WHEN( "nested container whitelisting 1 item" ) {
        WHEN( "space available in backpack" ) {
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            for( item_pocket *&pkts : c->get_contents().get_all_contained_pockets() ) {
                pkts->settings.whitelist_item( obj1->typeId() );
                REQUIRE( pkts->settings.get_item_whitelist().count( obj1->typeId() ) == 1 );
                REQUIRE( pkts->settings.get_item_whitelist().count( obj2->typeId() ) == 0 );
            }
            THEN( "pickup all, nested partly filled" ) {
                test_pickup_autoinsert_results( u, wear, c, 1, 2, 1 );
            }
        }
        WHEN( "no space available in backpack" ) {
            pack->fill_with( soft_obj, soft_nested ? 59 : 44, false, false, true );
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            for( item_pocket *&pkts : c->get_contents().get_all_contained_pockets() ) {
                pkts->settings.whitelist_item( obj1->typeId() );
                REQUIRE( pkts->settings.get_item_whitelist().count( obj1->typeId() ) == 1 );
                REQUIRE( pkts->settings.get_item_whitelist().count( obj2->typeId() ) == 0 );
            }
            THEN( ( soft_nested ? "pickup none, nested empty" : "pickup one, nested partly filled" ) ) {
                if( soft_nested ) {
                    test_pickup_autoinsert_results( u, wear, c, 3, 60, 0 );
                } else {
                    test_pickup_autoinsert_results( u, wear, c, 2, 45, 1 );
                }
            }
        }
    }

    WHEN( "nested container whitelisting 2 items" ) {
        WHEN( "space available in backpack" ) {
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            for( item_pocket *&pkts : c->get_contents().get_all_contained_pockets() ) {
                pkts->settings.whitelist_item( obj1->typeId() );
                pkts->settings.whitelist_item( obj2->typeId() );
                REQUIRE( pkts->settings.get_item_whitelist().count( obj1->typeId() ) == 1 );
                REQUIRE( pkts->settings.get_item_whitelist().count( obj2->typeId() ) == 1 );
            }
            THEN( "pickup all, nested filled" ) {
                test_pickup_autoinsert_results( u, wear, c, 1, 1, 2 );
            }
        }
        WHEN( "no space available in backpack" ) {
            pack->fill_with( soft_obj, soft_nested ? 59 : 44, false, false, true );
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            for( item_pocket *&pkts : c->get_contents().get_all_contained_pockets() ) {
                pkts->settings.whitelist_item( obj1->typeId() );
                pkts->settings.whitelist_item( obj2->typeId() );
                REQUIRE( pkts->settings.get_item_whitelist().count( obj1->typeId() ) == 1 );
                REQUIRE( pkts->settings.get_item_whitelist().count( obj2->typeId() ) == 1 );
            }
            THEN( ( soft_nested ? "pickup none, nested empty" : "pickup all, nested filled" ) ) {
                if( soft_nested ) {
                    test_pickup_autoinsert_results( u, wear, c, 3, 60, 0 );
                } else {
                    test_pickup_autoinsert_results( u, wear, c, 1, 45, 2 );
                }
            }
        }
    }

    WHEN( "nested container accepts item stack" ) {
        obj1.remove_item();
        obj2.remove_item();
        if( soft_nested ) {
            cont1.remove_item();
        } else {
            cont2.remove_item();
        }
        item stack( "test_pine_nuts" );
        item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
        WHEN( "item stack too large to fit in top-level container" ) {
            stack.charges = 300;
            item_location obj3( map_cursor( u.pos() ), &m.add_item_or_charges( u.pos(), stack ) );
            REQUIRE( obj3->charges == 300 );
            u.cancel_activity();
            pickup_activity_actor new_actor( { obj3 }, { 300 }, u.pos(), autopickup );
            u.assign_activity( new_actor );
            THEN( ( soft_nested ? "pickup most, nested empty" : "pickup all, overflow into nested" ) ) {
                if( soft_nested ) {
                    test_pickup_autoinsert_results( u, wear, c, 61, 239, 0, true );
                } else {
                    test_pickup_autoinsert_results( u, wear, c, 64, 176, 60, true );
                }
            }
        }
    }

    WHEN( "nested container blacklisting item stack" ) {
        obj1.remove_item();
        obj2.remove_item();
        if( soft_nested ) {
            cont1.remove_item();
        } else {
            cont2.remove_item();
        }
        item stack( "test_pine_nuts" );
        item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
        for( item_pocket *&pkts : c->get_contents().get_all_contained_pockets() ) {
            pkts->settings.blacklist_item( stack.typeId() );
            REQUIRE( pkts->settings.get_item_blacklist().count( stack.typeId() ) == 1 );
        }
        WHEN( "item stack too large to fit in top-level container" ) {
            stack.charges = 300;
            item_location obj3( map_cursor( u.pos() ), &m.add_item_or_charges( u.pos(), stack ) );
            REQUIRE( obj3->charges == 300 );
            u.cancel_activity();
            pickup_activity_actor new_actor( { obj3 }, { 300 }, u.pos(), autopickup );
            u.assign_activity( new_actor );
            THEN( "pickup most, nested empty" ) {
                if( soft_nested ) {
                    test_pickup_autoinsert_results( u, wear, c, 61, 239, 0, true );
                } else {
                    test_pickup_autoinsert_results( u, wear, c, 124, 176, 0, true );
                }
            }
        }
    }

    WHEN( "nested container whitelisting something other than item stack" ) {
        obj1.remove_item();
        if( soft_nested ) {
            cont1.remove_item();
        } else {
            cont2.remove_item();
        }
        item stack( "test_pine_nuts" );
        item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
        for( item_pocket *&pkts : c->get_contents().get_all_contained_pockets() ) {
            pkts->settings.whitelist_item( obj2->typeId() );
            REQUIRE( pkts->settings.get_item_whitelist().count( obj2->typeId() ) == 1 );
            REQUIRE( pkts->settings.get_item_whitelist().count( stack.typeId() ) == 0 );
        }
        obj2.remove_item();
        WHEN( "item stack too large to fit in top-level container" ) {
            stack.charges = 300;
            item_location obj3( map_cursor( u.pos() ), &m.add_item_or_charges( u.pos(), stack ) );
            REQUIRE( obj3->charges == 300 );
            u.cancel_activity();
            pickup_activity_actor new_actor( { obj3 }, { 300 }, u.pos(), autopickup );
            u.assign_activity( new_actor );
            THEN( "pickup most, nested empty" ) {
                if( soft_nested ) {
                    test_pickup_autoinsert_results( u, wear, c, 61, 239, 0, true );
                } else {
                    test_pickup_autoinsert_results( u, wear, c, 124, 176, 0, true );
                }
            }
        }
    }

    WHEN( "nested container high priority" ) {
        WHEN( "space available in backpack" ) {
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            for( item_pocket *&pkts : c->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_priority( 50 );
                REQUIRE( pkts->settings.priority() == 50 );
            }
            for( item_pocket *&pkts : pack->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_priority( 10 );
                REQUIRE( pkts->settings.priority() == 10 );
            }
            THEN( "pickup all, nested filled" ) {
                test_pickup_autoinsert_results( u, wear, c, 1, 1, 2 );
            }
        }
        WHEN( "no space available in backpack" ) {
            pack->fill_with( soft_obj, soft_nested ? 59 : 44, false, false, true );
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            for( item_pocket *&pkts : c->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_priority( 50 );
                REQUIRE( pkts->settings.priority() == 50 );
            }
            for( item_pocket *&pkts : pack->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_priority( 10 );
                REQUIRE( pkts->settings.priority() == 10 );
            }
            THEN( ( soft_nested ? "pickup none, nested empty" : "pickup all, nested filled" ) ) {
                if( soft_nested ) {
                    test_pickup_autoinsert_results( u, wear, c, 3, 60, 0 );
                } else {
                    test_pickup_autoinsert_results( u, wear, c, 1, 45, 2 );
                }
            }
        }
    }

    WHEN( "nested container same priority as top container" ) {
        WHEN( "space available in backpack" ) {
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            for( item_pocket *&pkts : c->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_priority( 50 );
                REQUIRE( pkts->settings.priority() == 50 );
            }
            for( item_pocket *&pkts : pack->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_priority( 50 );
                REQUIRE( pkts->settings.priority() == 50 );
            }
            THEN( "pickup all, nested empty" ) {
                test_pickup_autoinsert_results( u, wear, c, 1, 3, 0 );
            }
        }
        WHEN( "no space available in backpack" ) {
            pack->fill_with( soft_obj, soft_nested ? 59 : 44, false, false, true );
            item_location c = give_item_to_char( u, soft_nested ? cont2 : cont1 );
            for( item_pocket *&pkts : c->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_priority( 50 );
                REQUIRE( pkts->settings.priority() == 50 );
            }
            for( item_pocket *&pkts : pack->get_contents().get_all_contained_pockets() ) {
                pkts->settings.set_priority( 50 );
                REQUIRE( pkts->settings.priority() == 50 );
            }
            THEN( ( soft_nested ? "pickup none, nested empty" : "pickup all, nested filled" ) ) {
                if( soft_nested ) {
                    test_pickup_autoinsert_results( u, wear, c, 3, 60, 0 );
                } else {
                    test_pickup_autoinsert_results( u, wear, c, 1, 45, 2 );
                }
            }
        }
    }
}

static void test_pickup_autoinsert_sub( bool autopickup, bool wear )
{
    GIVEN( "backpack contains rigid nested container" ) {
        test_pickup_autoinsert_sub_sub( autopickup, wear, false );
    }

    GIVEN( "backpack contains soft nested container" ) {
        test_pickup_autoinsert_sub_sub( autopickup, wear, true );
    }
}

static void test_pickup_autoinsert( bool autopickup )
{
    GIVEN( "player wearing backpack, items on ground" ) {
        test_pickup_autoinsert_sub( autopickup, true );
    }

    GIVEN( "player holding backpack, items on ground" ) {
        test_pickup_autoinsert_sub( autopickup, false );
    }
}

TEST_CASE( "picking_up_items_respects_pocket_autoinsert_settings", "[pocket][item]" )
{
    GIVEN( "autopickup" ) {
        test_pickup_autoinsert( true );
    }

    GIVEN( "manual pickup" ) {
        test_pickup_autoinsert( false );
    }
}

TEST_CASE( "multipocket_liquid_transfer_test", "[pocket][item][liquid]" )
{
    clear_map();
    clear_avatar();
    map &m = get_map();
    Character &u = get_player_character();
    item water( "water" );
    item cont_jug( itype_test_jug_plastic );
    item cont_suit( "test_robofac_armor_rig" );

    // Place a container at the character's feet
    item_location jug_w_water( map_cursor( u.pos() ), &m.add_item_or_charges( u.pos(), cont_jug ) );

    GIVEN( "character wearing a multipocket liquid container" ) {
        item_location suit( u, & **u.wear_item( cont_suit, false ) );
        REQUIRE( !!suit );
        REQUIRE( suit->is_container_empty() );

        WHEN( "unloading liquid from a full container into worn container" ) {
            jug_w_water->fill_with( water );
            REQUIRE( jug_w_water->all_items_top().size() == 1 );
            REQUIRE( jug_w_water->all_items_top().front()->charges == 15 );
            struct liquid_dest_opt liquid_target;
            liquid_target.pos = jug_w_water.position();
            liquid_target.dest_opt = LD_ITEM;
            liquid_target.item_loc = suit;
            u.set_moves( 100 );
            liquid_handler::perform_liquid_transfer( *jug_w_water->all_items_top().front(), nullptr,
                    nullptr, -1, nullptr, liquid_target );
            THEN( "liquid fills the worn container's pockets, some left over" ) {
                CHECK( u.get_moves() == 0 );
                CHECK( !jug_w_water->only_item().is_null() );
                CHECK( jug_w_water->only_item().charges == 3 );
                CHECK( suit->all_items_top().size() == 2 );
                int total = 0;
                for( item *&it : suit->all_items_top() ) {
                    CHECK( it->charges == 6 );
                    total += it->charges;
                }
                total += jug_w_water->only_item().charges;
                CHECK( total == 15 );
            }
        }

        WHEN( "unloading liquid from a full container into partly filled worn container" ) {
            suit->fill_with( water, 4 );
            jug_w_water->fill_with( water );
            REQUIRE( suit->only_item().charges == 4 );
            REQUIRE( jug_w_water->all_items_top().size() == 1 );
            REQUIRE( jug_w_water->all_items_top().front()->charges == 15 );
            struct liquid_dest_opt liquid_target;
            liquid_target.pos = jug_w_water.position();
            liquid_target.dest_opt = LD_ITEM;
            liquid_target.item_loc = suit;
            u.set_moves( 100 );
            liquid_handler::perform_liquid_transfer( *jug_w_water->all_items_top().front(), nullptr,
                    nullptr, -1, nullptr, liquid_target );
            THEN( "liquid fills the worn container's pockets, some left over" ) {
                CHECK( u.get_moves() == 0 );
                CHECK( !jug_w_water->only_item().is_null() );
                CHECK( jug_w_water->only_item().charges == 7 );
                CHECK( suit->all_items_top().size() == 2 );
                int total = 0;
                for( item *&it : suit->all_items_top() ) {
                    CHECK( it->charges == 6 );
                    total += it->charges;
                }
                total += jug_w_water->only_item().charges;
                CHECK( total == 19 );
            }
        }

        WHEN( "unloading liquid from an almost empty container into worn container" ) {
            jug_w_water->fill_with( water, 2 );
            REQUIRE( jug_w_water->all_items_top().size() == 1 );
            REQUIRE( jug_w_water->all_items_top().front()->charges == 2 );
            struct liquid_dest_opt liquid_target;
            liquid_target.pos = jug_w_water.position();
            liquid_target.dest_opt = LD_ITEM;
            liquid_target.item_loc = suit;
            u.set_moves( 100 );
            liquid_handler::perform_liquid_transfer( *jug_w_water->all_items_top().front(), nullptr,
                    nullptr, -1, nullptr, liquid_target );
            m.make_active( jug_w_water );
            CHECK( jug_w_water->only_item().charges == 0 );
            jug_w_water->remove_item( jug_w_water->only_item() );
            THEN( "liquid fills one of the worn container's pockets, none left over" ) {
                CHECK( u.get_moves() == 0 );
                CHECK( jug_w_water->is_container_empty() );
                CHECK( suit->all_items_top().size() == 1 );
                CHECK( suit->all_items_top().front()->charges == 2 );
            }
        }

        WHEN( "unloading liquid from an almost empty container into partly filled worn container" ) {
            suit->fill_with( water, 5 );
            jug_w_water->fill_with( water, 2 );
            REQUIRE( suit->only_item().charges == 5 );
            REQUIRE( jug_w_water->all_items_top().size() == 1 );
            REQUIRE( jug_w_water->all_items_top().front()->charges == 2 );
            struct liquid_dest_opt liquid_target;
            liquid_target.pos = jug_w_water.position();
            liquid_target.dest_opt = LD_ITEM;
            liquid_target.item_loc = suit;
            u.set_moves( 100 );
            liquid_handler::perform_liquid_transfer( *jug_w_water->all_items_top().front(), nullptr,
                    nullptr, -1, nullptr, liquid_target );
            m.make_active( jug_w_water );
            CHECK( jug_w_water->only_item().charges == 0 );
            jug_w_water->remove_item( jug_w_water->only_item() );
            THEN( "liquid fills one of the worn container's pockets, none left over" ) {
                CHECK( u.get_moves() == 0 );
                CHECK( jug_w_water->is_container_empty() );
                CHECK( suit->all_items_top().size() == 2 );
                int total = 0;
                for( item *&it : suit->all_items_top() ) {
                    total += it->charges;
                    CHECK( it->charges > 0 );
                    CHECK( it->charges <= 6 );
                }
                CHECK( total == 7 );
            }
        }

        WHEN( "unloading liquid from worn container into empty container" ) {
            REQUIRE( jug_w_water->is_container_empty() );
            suit->fill_with( water );
            REQUIRE( suit->all_items_top().size() == 2 );
            struct liquid_dest_opt liquid_target;
            liquid_target.pos = suit.position();
            liquid_target.dest_opt = LD_ITEM;
            liquid_target.item_loc = jug_w_water;
            for( item *&it : suit->all_items_top() ) {
                u.set_moves( 100 );
                REQUIRE( it->charges == 6 );
                liquid_handler::perform_liquid_transfer( *it, nullptr, nullptr, -1, nullptr, liquid_target );
                CHECK( u.get_moves() == 0 );
                CHECK( it->charges == 0 );
                suit->remove_item( *it );
            }
            THEN( "liquid fills most of the empty container, none left over" ) {
                CHECK( u.get_moves() == 0 );
                CHECK( !jug_w_water->only_item().is_null() );
                CHECK( jug_w_water->only_item().charges == 12 );
                CHECK( suit->is_container_empty() );
            }
        }

        WHEN( "unloading liquid from worn container into partly filled container" ) {
            suit->fill_with( water );
            jug_w_water->fill_with( water, 8 );
            REQUIRE( suit->all_items_top().size() == 2 );
            REQUIRE( jug_w_water->only_item().charges == 8 );
            struct liquid_dest_opt liquid_target;
            liquid_target.pos = suit.position();
            liquid_target.dest_opt = LD_ITEM;
            liquid_target.item_loc = jug_w_water;
            for( item *&it : suit->all_items_top() ) {
                u.set_moves( 100 );
                REQUIRE( it->charges == 6 );
                liquid_handler::perform_liquid_transfer( *it, nullptr, nullptr, -1, nullptr, liquid_target );
                if( u.get_moves() == 0 && it->charges == 0 ) {
                    suit->remove_item( *it );
                }
            }
            THEN( "liquid fills the container, some left over" ) {
                CHECK( jug_w_water->only_item().charges == 15 );
                CHECK( suit->all_items_top().size() == 1 );
                CHECK( suit->all_items_top().front()->charges == 5 );
            }
        }
    }
}

static bool test_wallet_filled( Item_spawn_data *wallet_group )
{
    itype_id wallet_t( wallet_group->container_item.value() );
    Item_spawn_data::ItemList dummy;
    dummy.reserve( 20 );
    wallet_group->create( dummy, calendar::turn, spawn_flags::maximized );
    REQUIRE( dummy.size() == 1 );
    int wallets = 0;
    for( const item &it : dummy ) {
        if( it.typeId() == wallet_t ) {
            wallets++;
            CHECK( !it.empty_container() );
        }
    }
    return wallets == 1;
}

TEST_CASE( "full_wallet_spawn_test", "[pocket][item]" )
{
    const int iters = 100;
    const std::vector<Item_spawn_data *> groups = {
        item_controller->get_group( Item_spawn_data_wallet_duct_tape_full ),
        item_controller->get_group( Item_spawn_data_wallet_full ),
        item_controller->get_group( Item_spawn_data_wallet_industrial_full ),
        item_controller->get_group( Item_spawn_data_wallet_industrial_leather_full ),
        item_controller->get_group( Item_spawn_data_wallet_large_full ),
        item_controller->get_group( Item_spawn_data_wallet_leather_full ),
        item_controller->get_group( Item_spawn_data_wallet_military_full ),
        item_controller->get_group( Item_spawn_data_wallet_military_leather_full ),
        item_controller->get_group( Item_spawn_data_wallet_science_full ),
        item_controller->get_group( Item_spawn_data_wallet_science_leather_full ),
        item_controller->get_group( Item_spawn_data_wallet_science_stylish_full ),
        item_controller->get_group( Item_spawn_data_wallet_stylish_full )
    };
    for( Item_spawn_data *wg : groups ) {
        REQUIRE( wg->container_item.has_value() );
    }

    for( int i = 0; i < iters; i++ ) {
        for( Item_spawn_data *wg : groups ) {
            CHECK( test_wallet_filled( wg ) );
        }
    }
}

TEST_CASE( "best_pocket_for_pocket-holster_mix", "[pocket][item]" )
{
    avatar &u = get_avatar();
    item tool_belt( "test_tool_belt_pocket_mix" );
    item flashlight( "test_flashlight" );

    GIVEN( "character wearing a tool belt" ) {
        clear_avatar();
        u.wield( flashlight );
        item_location fl = u.get_wielded_item();
        item_location tb( u, & **u.wear_item( tool_belt, false ) );
        REQUIRE( !!tb.get_item() );
        REQUIRE( tb->typeId() == tool_belt.typeId() );

        WHEN( "attaching flashlight to tool belt" ) {
            REQUIRE( tb->can_holster( *fl ) );
            const holster_actor *ptr = dynamic_cast<const holster_actor *>
                                       ( tb->type->get_use( "holster" )->get_actor_ptr() );
            REQUIRE( !!ptr );
            CHECK( ptr->store( u, *tb, *fl ) );

            THEN( "flashlight stored in smallest available holster" ) {
                bool found = false;
                const std::list<std::string> valid_pkts = { "P4", "P5", "P6" };
                for( const item_pocket *pkt : tb->get_all_contained_pockets() ) {
                    if( !pkt->empty() && pkt->front().typeId() == flashlight.typeId() ) {
                        CAPTURE( pkt->get_pocket_data()->pocket_name.translated() );
                        CHECK( !found ); // we shouldn't find this item in multiple pockets
                        found = true;
                        bool in_valid_pkt = false;
                        for( const std::string &n : valid_pkts ) {
                            if( pkt->get_pocket_data()->pocket_name.translated() == n ) {
                                in_valid_pkt = true;
                            }
                        }
                        CHECK( in_valid_pkt );
                    }
                }
                CHECK( found );
            }
        }

        WHEN( "attaching flashlight to tool belt, whitelisted pocket 1" ) {
            for( item_pocket *pkt : tb->get_all_contained_pockets() ) {
                if( pkt->get_pocket_data()->pocket_name.translated() == "P1" ) {
                    pkt->settings.whitelist_item( flashlight.typeId() );
                }
            }
            REQUIRE( tb->can_holster( *fl ) );
            const holster_actor *ptr = dynamic_cast<const holster_actor *>
                                       ( tb->type->get_use( "holster" )->get_actor_ptr() );
            REQUIRE( !!ptr );
            CHECK( ptr->store( u, *tb, *fl ) );

            THEN( "flashlight stored in pocket 1" ) {
                bool found = false;
                const std::list<std::string> valid_pkts = { "P1" };
                for( const item_pocket *pkt : tb->get_all_contained_pockets() ) {
                    if( !pkt->empty() && pkt->front().typeId() == flashlight.typeId() ) {
                        CAPTURE( pkt->get_pocket_data()->pocket_name.translated() );
                        CHECK( !found ); // we shouldn't find this item in multiple pockets
                        found = true;
                        bool in_valid_pkt = false;
                        for( const std::string &n : valid_pkts ) {
                            if( pkt->get_pocket_data()->pocket_name.translated() == n ) {
                                in_valid_pkt = true;
                            }
                        }
                        CHECK( in_valid_pkt );
                    }
                }
                CHECK( found );
            }
        }

        WHEN( "attaching flashlight to tool belt, whitelisted pocket 2" ) {
            for( item_pocket *pkt : tb->get_all_contained_pockets() ) {
                if( pkt->get_pocket_data()->pocket_name.translated() == "P2" ) {
                    pkt->settings.whitelist_item( flashlight.typeId() );
                }
            }
            REQUIRE( tb->can_holster( *fl ) );
            const holster_actor *ptr = dynamic_cast<const holster_actor *>
                                       ( tb->type->get_use( "holster" )->get_actor_ptr() );
            REQUIRE( !!ptr );
            CHECK( ptr->store( u, *tb, *fl ) );

            THEN( "flashlight stored in pocket 2" ) {
                bool found = false;
                const std::list<std::string> valid_pkts = { "P2" };
                for( const item_pocket *pkt : tb->get_all_contained_pockets() ) {
                    if( !pkt->empty() && pkt->front().typeId() == flashlight.typeId() ) {
                        CAPTURE( pkt->get_pocket_data()->pocket_name.translated() );
                        CHECK( !found ); // we shouldn't find this item in multiple pockets
                        found = true;
                        bool in_valid_pkt = false;
                        for( const std::string &n : valid_pkts ) {
                            if( pkt->get_pocket_data()->pocket_name.translated() == n ) {
                                in_valid_pkt = true;
                            }
                        }
                        CHECK( in_valid_pkt );
                    }
                }
                CHECK( found );
            }
        }

        WHEN( "attaching flashlight to tool belt, whitelisted pocket 5" ) {
            for( item_pocket *pkt : tb->get_all_contained_pockets() ) {
                if( pkt->get_pocket_data()->pocket_name.translated() == "P5" ) {
                    pkt->settings.whitelist_item( flashlight.typeId() );
                }
            }
            REQUIRE( tb->can_holster( *fl ) );
            const holster_actor *ptr = dynamic_cast<const holster_actor *>
                                       ( tb->type->get_use( "holster" )->get_actor_ptr() );
            REQUIRE( !!ptr );
            CHECK( ptr->store( u, *tb, *fl ) );

            THEN( "flashlight stored in pocket 5" ) {
                bool found = false;
                const std::list<std::string> valid_pkts = { "P5" };
                for( const item_pocket *pkt : tb->get_all_contained_pockets() ) {
                    if( !pkt->empty() && pkt->front().typeId() == flashlight.typeId() ) {
                        CAPTURE( pkt->get_pocket_data()->pocket_name.translated() );
                        CHECK( !found ); // we shouldn't find this item in multiple pockets
                        found = true;
                        bool in_valid_pkt = false;
                        for( const std::string &n : valid_pkts ) {
                            if( pkt->get_pocket_data()->pocket_name.translated() == n ) {
                                in_valid_pkt = true;
                            }
                        }
                        CHECK( in_valid_pkt );
                    }
                }
                CHECK( found );
            }
        }
    }
}

TEST_CASE( "item_cannot_contain_contents_it_already_has", "[item][pocket]" )
{
    item backpack( "test_backpack" );
    item bottle( "bottle_plastic" );
    item water( "water" );

    water.charges = 1;
    bottle.fill_with( water, 1 );
    REQUIRE( !bottle.is_container_empty() );
    REQUIRE( bottle.only_item().typeId() == water.typeId() );
    backpack.put_in( bottle, pocket_type::CONTAINER );
    REQUIRE( !backpack.is_container_empty() );
    REQUIRE( backpack.only_item().typeId() == bottle.typeId() );

    const tripoint ipos = get_player_character().pos();
    map &m = get_map();
    clear_map();

    item_location backpack_loc( map_cursor( ipos ), &m.add_item( ipos, backpack ) );
    item_location bottle_loc( backpack_loc, &backpack_loc->only_item() );
    item_location water_loc( bottle_loc, &bottle_loc->only_item() );

    REQUIRE( water_loc->count() == 1 );

    const item &water_item = *water_loc;

    // Check bottle containing water
    bool in_top = false;
    for( const item *contained : bottle_loc->all_items_top() ) {
        if( contained == water_loc.get_item() ) {
            in_top = true;
        }
    }
    CHECK( in_top );
    CHECK( bottle_loc->can_contain( water_item ).success() );
    CHECK( !bottle_loc->can_contain( water_item, false, false, true, false, bottle_loc ).success() );

    // Check backpack containing bottle containing water
    in_top = false;
    for( const item *contained : backpack_loc->all_items_top() ) {
        if( contained == water_loc.get_item() ) {
            in_top = true;
        }
    }
    CHECK( !in_top );
    CHECK( backpack_loc->can_contain( water_item ).success() );
    CHECK( !backpack_loc->can_contain( water_item, false, false, true, false, bottle_loc ).success() );
}

TEST_CASE( "Sawed_off_fits_in_large_holster", "[item][pocket]" )
{
    item double_barrel( "shotgun_d" );
    item large_holster( "XL_holster" );

    //add the mods
    double_barrel.put_in( item( "stock_none", calendar::turn ), pocket_type::MOD );
    double_barrel.put_in( item( "barrel_small", calendar::turn ), pocket_type::MOD );

    CHECK( large_holster.can_contain( double_barrel ).success() );

}

// this tests for cases where we try to find a nested pocket for items (when a parent pocket has some restrictions) and find a massive bag inside the parent pocket
// need to make sure we don't try to fit things larger than the parent pockets remaining volume inside the child pocket if it is non-rigid
TEST_CASE( "bag_with_restrictions_and_nested_bag_does_not_fit_too_large_items", "[item][pocket]" )
{
    item backpack( "test_backpack" );
    item backpack_two( "test_backpack" );
    item mini_backpack( "test_mini_backpack" );

    mini_backpack.put_in( backpack, pocket_type::CONTAINER );
    REQUIRE( !mini_backpack.is_container_empty() );
    REQUIRE( mini_backpack.only_item().typeId() == backpack.typeId() );

    // need to set a setting on the pocket for this to work since that's when nesting starts trying weird stuff
    mini_backpack.get_contents().get_all_standard_pockets().front()->settings.set_disabled( true );

    // check if the game thinks the mini bag can contain the second bag (it can't)
    // but that the bag could otherwise fit if not in the parent pocket
    CHECK( backpack.can_contain( backpack_two ).success() );
    CHECK( !mini_backpack.can_contain( backpack_two ).success() );

}

TEST_CASE( "pocket_leak" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    item backpack( "test_backpack" );
    item water( "water" );
    water.set_item_temperature( water.get_freeze_point() );
    REQUIRE( water.is_frozen_liquid() );
    REQUIRE( backpack.put_in( water, pocket_type::CONTAINER ).success() );

    WHEN( "single container" ) {
        auto backpack_iter = *u.wear_item( backpack );
        item &bkit = *backpack_iter;
        item &waterit = bkit.only_item();
        waterit.set_item_temperature( water.get_freeze_point() + units::from_celsius_delta( 10 ) );
        REQUIRE( !waterit.is_frozen_liquid() );
        REQUIRE( here.i_at( u.pos_bub() ).empty() );
        u.process_turn();
        CHECK( here.i_at( u.pos_bub() ).begin()->typeId() == water.typeId() );
        CHECK( bkit.empty() );
    }

    WHEN( "nested container" ) {
        bool const top_watertight = GENERATE( true, false );
        CAPTURE( top_watertight );
        item top( top_watertight ? "55gal_drum" : "test_backpack" );
        REQUIRE( top.is_watertight_container() == top_watertight );
        REQUIRE( top.put_in( backpack, pocket_type::CONTAINER ).success() );
        u.wield( top );
        item &topit = *u.get_wielded_item();
        item &bkit = topit.only_item();
        item &wit = bkit.only_item();
        wit.set_item_temperature( water.get_freeze_point() + units::from_celsius_delta( 10 ) );
        REQUIRE( !wit.is_frozen_liquid() );
        REQUIRE( here.i_at( u.pos_bub() ).empty() );
        u.process_turn();
        if( top_watertight ) {
            CHECK( here.i_at( u.pos_bub() ).empty() );
        } else {
            CHECK( here.i_at( u.pos_bub() ).begin()->typeId() == water.typeId() );
        }
        CHECK( bkit.empty() == !top_watertight );
        bool bkit_has_water = false;
        for( item const *it : bkit.all_items_top() ) {
            bkit_has_water |= it->typeId() == water.typeId();
        }
        CHECK( bkit_has_water == top_watertight );
    }
}

namespace
{
void check_whitelist( item const &it, bool should, itype_id const &id )
{
    REQUIRE( it.get_all_contained_pockets().size() == 1 );
    if( should ) {
        REQUIRE( !it.get_all_contained_pockets().front()->settings.get_item_whitelist().empty() );
        CHECK( *it.get_all_contained_pockets().front()->settings.get_item_whitelist().begin() ==
               id );
    } else {
        REQUIRE( it.empty_container() );
        CHECK( it.get_all_contained_pockets().front()->settings.get_item_whitelist().empty() );
    }
}
} // namespace

TEST_CASE( "auto_whitelist", "[item][pocket][item_spawn]" )
{
    clear_avatar();
    clear_map();
    tripoint_abs_omt const this_omt =
        project_to<coords::omt>( get_avatar().get_location() );
    tripoint const this_bub = get_map().getlocal( project_to<coords::ms>( this_omt ) );
    manual_nested_mapgen( this_omt, nested_mapgen_auto_wl_test );
    REQUIRE( !get_map().i_at( this_bub + tripoint_zero ).empty() );
    REQUIRE( !get_map().i_at( this_bub + tripoint_east ).empty() );
    REQUIRE( !get_map().i_at( this_bub + tripoint_south ).empty() );
    item_location spawned_in_def_container( map_cursor{ this_bub + tripoint_zero },
                                            &get_map().i_at( this_bub + tripoint_zero ).only_item() );
    item_location spawned_w_modifier( map_cursor{ this_bub + tripoint_east },
                                      &get_map().i_at( this_bub + tripoint_east ).only_item() );
    item_location spawned_w_custom_container( map_cursor{ this_bub + tripoint_south },
            &get_map().i_at( this_bub + tripoint_south ).only_item() );
    check_whitelist( *spawned_in_def_container, true,
                     spawned_in_def_container->get_contents().first_item().typeId() );
    check_whitelist( *spawned_w_modifier, true,
                     spawned_w_modifier->get_contents().first_item().typeId() );
    check_whitelist( *spawned_w_custom_container, true,
                     spawned_w_custom_container->get_contents().first_item().typeId() );

    bool const edited = GENERATE( false, true );
    CAPTURE( edited );
    if( edited ) {
        spawned_in_def_container->get_all_contained_pockets().front()->settings.set_was_edited();
        spawned_w_modifier->get_all_contained_pockets().front()->settings.set_was_edited();
        spawned_w_custom_container->get_all_contained_pockets().front()->settings.set_was_edited();
    }

    SECTION( "container emptied by avatar" ) {
        avatar &u = get_avatar();
        itype_id const id = spawned_in_def_container->get_contents().first_item().typeId();
        unload_activity_actor::unload( u, spawned_in_def_container );
        REQUIRE( spawned_in_def_container->empty_container() );
        check_whitelist( *spawned_in_def_container, edited, id );
    }

    SECTION( "container emptied by processing" ) {
        itype_id const id = spawned_w_modifier->get_contents().first_item().typeId();
        get_map().i_clear( spawned_w_custom_container.position() );
        get_map().i_clear( spawned_in_def_container.position() );
        restore_on_out_of_scope<std::optional<units::temperature>> restore_temp(
                    get_weather().forced_temperature );
        get_weather().forced_temperature = units::from_celsius( 21 );
        spawned_w_modifier->only_item().set_relative_rot( 10 );
        REQUIRE( spawned_w_modifier->only_item().has_rotten_away() );
        spawned_w_modifier->only_item().set_last_temp_check( calendar::turn_zero );
        calendar::turn += 15_minutes;
        get_map().process_items();
        REQUIRE( spawned_w_modifier->empty_container() );
        check_whitelist( *spawned_w_modifier, edited, id );
    }
}

static void compare_pockets( item &it, pocket_mod_test_data &pocket_mod_data, bool mod_inserted )
{
    std::vector<item_pocket *> new_pockets( it.get_contents().get_pockets( [](
    item_pocket const & ) {
        return true;
    } ) );

    for( std::pair<const pocket_type, std::vector<uint64_t>> &expected :
         pocket_mod_data.expected_pockets ) {
        const pocket_type type = expected.first;
        uint64_t count = expected.second[mod_inserted ? 1 : 0];
        std::vector<item_pocket *> pockets( it.get_contents().get_pockets( [type](
        item_pocket const & pock ) {
            return pock.is_type( type );
        } ) );
        CAPTURE( type );
        CHECK( count == pockets.size() );
    }

    bool same_pocket_data = new_pockets.size() == it.type->pockets.size();
    if( same_pocket_data ) {
        for( const item_pocket *pocket : new_pockets ) {
            if( std::find( it.type->pockets.begin(), it.type->pockets.end(),
                           *pocket->get_pocket_data() ) == it.type->pockets.end() ) {
                same_pocket_data = false;
                break;
            }
        }
    }

    CHECK( same_pocket_data ^ mod_inserted );
}

TEST_CASE( "pocket_mods", "[pocket][toolmod][gunmod]" )
{
    for( std::pair<const std::string, pocket_mod_test_data> &pocket_mod_data :
         test_data::pocket_mod_data ) {
        SECTION( pocket_mod_data.first ) {
            item base_it( pocket_mod_data.second.base_item );
            item mod_it( pocket_mod_data.second.mod_item );

            base_it.put_in( mod_it, pocket_type::MOD );

            SECTION( "after inserting the mod" ) {
                compare_pockets( base_it, pocket_mod_data.second, true );
            }

            SECTION( "after removing the mod" ) {
                base_it.remove_items_with( [mod_it]( const item & it ) {
                    return mod_it.type == it.type;
                } );
                compare_pockets( base_it, pocket_mod_data.second, false );
            }
        }
    }
}
