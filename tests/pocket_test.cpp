#include "ammo.h"
#include "item.h"
#include "item_pocket.h"

#include "catch/catch.hpp"

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
            box.put_in( rod_15, item_pocket::pocket_type::CONTAINER );
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
        data_ammo_box.ammo_restriction.emplace( ammotype( "9mm" ), 200 );
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
        data_mag.ammo_restriction.emplace( ammotype( "9mm" ), full_clip_qty );
        item_pocket pocket_mag( &data_mag );

        WHEN( "it does not already contain any ammo" ) {
            REQUIRE( pocket_mag.empty() );

            THEN( "it can contain a full clip of 9mm ammo" ) {
                const item ammo_9mm( "test_9mm_ammo", 0, full_clip_qty );
                expect_can_contain( pocket_mag, ammo_9mm );
            }

            THEN( "it cannot contain items of the wrong ammo type" ) {
                item rock( "test_rock" );
                item ammo_45( "test_45_ammo", 0, 1 );
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
            item ammo_9mm_half_clip( "test_9mm_ammo", 0, half_clip_qty );
            expect_can_insert( pocket_mag, ammo_9mm_half_clip );

            THEN( "it can contain more of the same ammo" ) {
                item ammo_9mm_refill( "test_9mm_ammo", 0, full_clip_qty - half_clip_qty );
                expect_can_contain( pocket_mag, ammo_9mm_refill );
            }

            THEN( "it cannot contain more ammo than ammo_restriction allows" ) {
                item ammo_9mm_overfill( "test_9mm_ammo", 0, full_clip_qty - half_clip_qty + 1 );
                expect_cannot_contain( pocket_mag, ammo_9mm_overfill,
                                       "tried to put too many charges of ammo in item",
                                       item_pocket::contain_code::ERR_NO_SPACE );
            }
        }

        WHEN( "it is completely full of ammo" ) {
            item ammo_9mm_full_clip( "test_9mm_ammo", 0, full_clip_qty );
            expect_can_insert( pocket_mag, ammo_9mm_full_clip );

            THEN( "it cannot contain any more of the same ammo" ) {
                item ammo_9mm_bullet( "test_9mm_ammo", 0, 1 );
                expect_cannot_contain( pocket_mag, ammo_9mm_bullet,
                                       "tried to put too many charges of ammo in item",
                                       item_pocket::contain_code::ERR_NO_SPACE );
            }
        }
    }
}

// Flag restriction
// ----------------
// The "flag_restriction" list from pocket data JSON gives compatible item flag(s) for this pocket.
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
        data_belt.flag_restriction.push_back( "BELT_CLIP" );
        item_pocket pocket_belt( &data_belt );

        GIVEN( "item has BELT_CLIP flag" ) {
            REQUIRE( screwdriver.has_flag( "BELT_CLIP" ) );
            REQUIRE( sonic.has_flag( "BELT_CLIP" ) );
            REQUIRE( halligan.has_flag( "BELT_CLIP" ) );
            REQUIRE( axe.has_flag( "BELT_CLIP" ) );

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
            REQUIRE_FALSE( rag.has_flag( "BELT_CLIP" ) );
            REQUIRE_FALSE( rock.has_flag( "BELT_CLIP" ) );
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
    item ketchup( "ketchup", 0, item::default_charges_tag{} );
    item mustard( "mustard", 0, item::default_charges_tag{} );

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
    item gas( "test_gas", 0, item::default_charges_tag{} );

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
