#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "debug.h"
#include "enums.h"
#include "flag.h"
#include "item.h"
#include "item_category.h"
#include "item_contents.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "optional.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

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
    ret_val<item_pocket::contain_code> rate_can = pocket.insert_item( it );
    CHECK( rate_can.success() );
    CHECK( rate_can.str().empty() );
    CHECK( rate_can.value() == item_pocket::contain_code::SUCCESS );
}

// Call pocket.insert_item( it ) and expect it to fail, with an expected reason and contain_code
static void expect_cannot_insert( item_pocket &pocket, const item &it,
                                  const std::string &expect_reason,
                                  item_pocket::contain_code expect_code )
{
    CAPTURE( it.tname() );
    ret_val<item_pocket::contain_code> rate_can = pocket.insert_item( it );
    CHECK_FALSE( rate_can.success() );
    CHECK( rate_can.str() == expect_reason );
    CHECK( rate_can.value() == expect_code );
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
TEST_CASE( "max item length", "[pocket][max_item_length]" )
{
    // Test items with different lengths
    item screwdriver( "test_screwdriver" );
    item sonic( "test_sonic_screwdriver" );
    item sword( "test_clumsy_sword" );

    // Sheath that may contain items
    pocket_data data_sheath( item_pocket::pocket_type::CONTAINER );
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
            box.put_in( rod_14, item_pocket::pocket_type::CONTAINER );
            // Item went into the box
            CHECK_FALSE( box.is_container_empty() );
        }

        THEN( "it cannot hold an item 15 cm in length" ) {
            item rod_15( "test_rod_15cm" );
            REQUIRE( rod_15.length() == 15_cm );

            REQUIRE( box.is_container_empty() );
            std::string dmsg = capture_debugmsg_during( [&box, &rod_15]() {
                ret_val<bool> result = box.put_in( rod_15, item_pocket::pocket_type::CONTAINER );
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
TEST_CASE( "max item volume", "[pocket][max_item_volume]" )
{
    // Test items
    item screwdriver( "test_screwdriver" );
    item rag( "test_rag" );
    item rock( "test_rock" );
    item gas( "test_gas" );
    item liquid( "test_liquid" );

    // Air-tight, water-tight, jug-style container
    pocket_data data_jug( item_pocket::pocket_type::CONTAINER );
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
            expect_cannot_contain( pocket_jug, rock, "item too big",
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
TEST_CASE( "max container volume", "[pocket][max_contains_volume]" )
{
    // TODO: Add tests for having multiple ammo types in the ammo_restriction

    WHEN( "pocket has no ammo_restriction" ) {
        pocket_data data_box( item_pocket::pocket_type::CONTAINER );
        // Just a normal 1-liter box
        data_box.volume_capacity = 1_liter;
        REQUIRE( data_box.ammo_restriction.empty() );

        THEN( "max_contains_volume is the pocket's volume_capacity" ) {
            CHECK( data_box.max_contains_volume() == 1_liter );
        }
    }

    WHEN( "pocket has ammo_restriction" ) {
        pocket_data data_ammo_box( item_pocket::pocket_type::CONTAINER );

        // 9mm ammo is 50 rounds per 250ml (or 200 rounds per liter), so this ammo box
        // should be exactly 1 liter in size, so it can contain this much ammo.
        data_ammo_box.ammo_restriction.emplace( ammotype( "test_9mm" ), 200 );
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
TEST_CASE( "magazine with ammo restriction", "[pocket][magazine][ammo_restriction]" )
{
    pocket_data data_mag( item_pocket::pocket_type::MAGAZINE );

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
        data_mag.ammo_restriction.emplace( ammotype( "test_9mm" ), full_clip_qty );
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
                expect_cannot_contain( pocket_mag, rag, "item is not an ammo",
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
TEST_CASE( "pocket with item flag restriction", "[pocket][flag_restriction]" )
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
    pocket_data data_belt( item_pocket::pocket_type::CONTAINER );

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
                    expect_cannot_contain( pocket_belt, axe, "item too big",
                                           item_pocket::contain_code::ERR_TOO_BIG );
                }

                THEN( "item cannot be inserted into the pocket" ) {
                    expect_cannot_insert( pocket_belt, axe, "item too big",
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
                expect_cannot_contain( pocket_belt, rag, "item does not have correct flag",
                                       item_pocket::contain_code::ERR_FLAG );
                expect_cannot_contain( pocket_belt, rock, "item does not have correct flag",
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
TEST_CASE( "holster can contain one fitting item", "[pocket][holster]" )
{
    // Start with a basic test handgun from data/mods/TEST_DATA/items.json
    item glock( "test_glock" );

    // Construct data for a holster to perfectly fit this gun
    pocket_data data_holster( item_pocket::pocket_type::CONTAINER );
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
            expect_cannot_contain( pocket_holster, glock, "item too big",
                                   item_pocket::contain_code::ERR_TOO_BIG );
        }

        THEN( "item cannot be successfully inserted" ) {
            expect_cannot_insert( pocket_holster, glock, "item too big",
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
TEST_CASE( "pockets containing liquids", "[pocket][watertight][liquid]" )
{
    // Liquids
    item ketchup( "ketchup", calendar::turn_zero, item::default_charges_tag{} );
    item mustard( "mustard", calendar::turn_zero, item::default_charges_tag{} );

    // Non-liquids
    item rock( "test_rock" );
    item glock( "test_glock" );

    // Large watertight container
    pocket_data data_bucket( item_pocket::pocket_type::CONTAINER );
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
TEST_CASE( "pockets containing gases", "[pocket][airtight][gas]" )
{
    item gas( "test_gas", calendar::turn_zero, item::default_charges_tag{} );

    // A potentially airtight container
    pocket_data data_balloon( item_pocket::pocket_type::CONTAINER );

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
TEST_CASE( "rigid and non-rigid or flexible pockets", "[pocket][rigid][flexible]" )
{
    item rock( "test_rock" );

    // Pocket with enough space for 2 rocks
    pocket_data data_sock( item_pocket::pocket_type::CONTAINER );
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
TEST_CASE( "corpse can contain anything", "[pocket][corpse]" )
{
    item rock( "test_rock" );
    item glock( "test_glock" );

    pocket_data data_corpse( item_pocket::pocket_type::CORPSE );

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
TEST_CASE( "sealed containers", "[pocket][seal]" )
{
    item water( "water" );

    GIVEN( "sealable can" ) {
        item can( "test_can_drink" );

        // Ensure it has exactly one contained pocket, and get that pocket for testing
        ret_val<std::vector<item_pocket *>> can_pockets = can.get_all_contained_pockets();
        REQUIRE( can_pockets.success() );
        REQUIRE( can_pockets.value().size() == 1 );
        item_pocket &pocket = *can_pockets.value().front();
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
        item jug( "test_jug_plastic" );

        // Ensure it has exactly one contained pocket, and get that pocket for testing
        ret_val<std::vector<item_pocket *>> jug_pockets = jug.get_all_contained_pockets();
        REQUIRE( jug_pockets.success() );
        REQUIRE( jug_pockets.value().size() == 1 );
        item_pocket &pocket = *jug_pockets.value().front();
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
TEST_CASE( "when one pocket is better than another", "[pocket][better]" )
{
    // TODO:
    // settings.is_better_favorite() is top priority
    // settings.priority() takes next priority
    // has_item_stacks_with(it) is better than one that doesn't stack
    // pockets restricted by ammo should try to get filled first
    // pockets restricted by flag should try to get filled first
    // if remaining volume is equal, lower obtain_cost is better

    // A and B: Two generic sets of pocket data for comparison
    pocket_data data_a( item_pocket::pocket_type::CONTAINER );
    pocket_data data_b( item_pocket::pocket_type::CONTAINER );

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
    ret_val<std::vector<item_pocket *>> pockets = container.get_all_contained_pockets();
    REQUIRE( pockets.value().size() == 1 );
    return pockets.value()[0];
}

TEST_CASE( "best pocket in item contents", "[pocket][item][best]" )
{
    item_location loc;

    // Waterskins can be best pockets for liquids
    SECTION( "item with one watertight pocket has best_pocket for liquid" ) {
        // Must have a CONTAINER pocket, first and foremost
        item skin( "test_waterskin" );
        REQUIRE( skin.is_container() );
        // Prerequisite: It can contain water
        item liquid( "test_liquid" );
        REQUIRE( skin.can_contain( liquid ) );

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
        REQUIRE( util_belt.can_contain( screwdriver ) );
        REQUIRE( util_belt.can_contain( halligan ) );
        // It can contain liquid and gas
        item liquid( "test_liquid" );
        item gas( "test_gas", calendar::turn_zero, item::default_charges_tag{} );
        REQUIRE( util_belt.can_contain( liquid ) );
        REQUIRE( util_belt.can_contain( gas ) );

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
        REQUIRE( glock.has_pocket_type( item_pocket::pocket_type::MAGAZINE_WELL ) );
        // Empty magazine
        item glockmag( "test_glockmag", calendar::turn, 0 );
        REQUIRE( glockmag.has_pocket_type( item_pocket::pocket_type::MAGAZINE ) );
        REQUIRE( glockmag.ammo_remaining() == 0 );
        // A single 9mm bullet
        item glockammo( "test_9mm_ammo", calendar::turn, 1 );
        REQUIRE( glockammo.is_ammo() );
        REQUIRE( glockammo.charges == 1 );

        // Although gun can contain magazine, and magazine can contain bullet...
        REQUIRE( glock.can_contain( glockmag ) );
        REQUIRE( glockmag.can_contain( glockammo ) );
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
        REQUIRE( can.can_contain( liquid ) );

        // Before being sealed, it can be best pocket for liquid
        CHECK( has_best_pocket( can, liquid ) );
        // Fill with liquid, seal it, and ensure success
        can.put_in( liquid, item_pocket::pocket_type::CONTAINER );
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

TEST_CASE( "pocket favorites allow or restrict items", "[pocket][item][best]" )
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

// Character::best_pocket
// - See if wielded item can hold it - start with this as default
// - For each worn item, see if best_pocket is better; if so, use it
// + Return the item_location of the item that has the best pocket
//
// What is the best pocket to put @it into? the pockets in @avoid do not count
// Character::best_pocket( it, avoid )
// NOTE: different syntax than item_contents::best_pocket
// (Second argument is `avoid` item pointer, not parent item location)
TEST_CASE( "character best pocket", "[pocket][character][best]" )
{
    // TODO:
    // Includes wielded item
    // Includes worn items
    // Uses best of multiple worn items
    // Skips avoided items
}

TEST_CASE( "guns and gunmods", "[pocket][gunmod]" )
{
    item m4a1( "nato_assault_rifle" );
    item strap( "shoulder_strap" );
    // Guns cannot "contain" gunmods, but gunmods can be inserted into guns
    CHECK_FALSE( m4a1.contents.can_contain( strap ).success() );
    CHECK( m4a1.contents.insert_item( strap, item_pocket::pocket_type::MOD ).success() );
}

TEST_CASE( "usb drives and software", "[pocket][software]" )
{
    item usb( "usb_drive" );
    item software( "software_math" );
    // USB drives aren't containers, and cannot "contain" software, but software can be inserted
    CHECK_FALSE( usb.contents.can_contain( software ).success() );
    CHECK( usb.contents.insert_item( software, item_pocket::pocket_type::SOFTWARE ).success() );
}

