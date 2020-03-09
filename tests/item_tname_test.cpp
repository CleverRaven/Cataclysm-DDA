#include "avatar.h"
#include "cata_string_consts.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "options_helpers.h"

TEST_CASE( "food with hidden effects", "[item][tname][hidden]" )
{
    g->u.empty_traits();

    GIVEN( "food with hidden poison" ) {
        item coffee = item( "coffee_pod" );
        REQUIRE( coffee.is_food() );
        REQUIRE( coffee.has_flag( flag_HIDDEN_POISON ) );

        WHEN( "avatar has level 2 survival skill" ) {
            g->u.set_skill_level( skill_survival, 2 );
            REQUIRE( g->u.get_skill_level( skill_survival ) == 2 );

            THEN( "they cannot see it is poisonous" ) {
                CHECK( coffee.tname() == "Kentucky coffee pod" );
            }
        }

        WHEN( "avatar has level 3 survival skill" ) {
            g->u.set_skill_level( skill_survival, 3 );
            REQUIRE( g->u.get_skill_level( skill_survival ) == 3 );

            THEN( "they see it is poisonous" ) {
                CHECK( coffee.tname() == "Kentucky coffee pod (poisonous)" );
            }
        }
    }

    GIVEN( "food with hidden hallucinogen" ) {
        item mushroom = item( "mushroom" );
        mushroom.set_flag( flag_HIDDEN_HALLU );
        REQUIRE( mushroom.is_food() );
        REQUIRE( mushroom.has_flag( flag_HIDDEN_HALLU ) );

        WHEN( "avatar has level 4 survival skill" ) {
            g->u.set_skill_level( skill_survival, 4 );
            REQUIRE( g->u.get_skill_level( skill_survival ) == 4 );

            THEN( "they cannot see it is hallucinogenic" ) {
                CHECK( mushroom.tname() == "mushroom (fresh)" );
            }
        }

        WHEN( "avatar has level 5 survival skill" ) {
            g->u.set_skill_level( skill_survival, 5 );
            REQUIRE( g->u.get_skill_level( skill_survival ) == 5 );

            THEN( "they see it is hallucinogenic" ) {
                CHECK( mushroom.tname() == "mushroom (hallucinogenic) (fresh)" );
            }
        }
    }
}

TEST_CASE( "items with a temperature flag", "[item][tname][temperature]" )
{
    GIVEN( "food that can melt" ) {
        item shake( "milkshake" );
        REQUIRE( shake.is_food() );
        REQUIRE( shake.has_flag( flag_MELTS ) );

        WHEN( "frozen" ) {
            shake.set_flag( flag_FROZEN );
            REQUIRE( shake.has_flag( flag_FROZEN ) );

            THEN( "it appears frozen and not melted" ) {
                CHECK( shake.tname() == "milkshake (fresh) (frozen)" );
            }
        }
        WHEN( "cold" ) {
            shake.set_flag( flag_COLD );
            REQUIRE( shake.has_flag( flag_COLD ) );

            THEN( "it appears cold and melted" ) {
                CHECK( shake.tname() == "milkshake (fresh) (cold) (melted)" );
            }
        }
        WHEN( "hot" ) {
            shake.set_flag( flag_HOT );
            REQUIRE( shake.has_flag( flag_HOT ) );

            THEN( "it appears hot and melted" ) {
                CHECK( shake.tname() == "milkshake (fresh) (hot) (melted)" );
            }
        }
    }

    GIVEN( "food that cannot melt" ) {
        item nut( "pine_nuts" );
        REQUIRE( nut.is_food() );
        REQUIRE_FALSE( nut.has_flag( flag_MELTS ) );

        WHEN( "frozen" ) {
            nut.set_flag( flag_FROZEN );
            REQUIRE( nut.has_flag( flag_FROZEN ) );

            THEN( "it appears frozen" ) {
                CHECK( nut.tname() == "pine nuts (fresh) (frozen)" );
            }
        }
        WHEN( "cold" ) {
            nut.set_flag( flag_COLD );
            REQUIRE( nut.has_flag( flag_COLD ) );

            THEN( "it appears cold" ) {
                CHECK( nut.tname() == "pine nuts (fresh) (cold)" );
            }
        }

        WHEN( "hot" ) {
            nut.set_flag( flag_HOT );
            REQUIRE( nut.has_flag( flag_HOT ) );

            THEN( "it appears hot" ) {
                CHECK( nut.tname() == "pine nuts (fresh) (hot)" );
            }
        }
    }

    GIVEN( "a human corpse" ) {
        item corpse = item::make_corpse( mtype_id::NULL_ID(), calendar::turn, "" );
        REQUIRE( corpse.is_corpse() );

        WHEN( "frozen" ) {
            corpse.set_flag( flag_FROZEN );
            REQUIRE( corpse.has_flag( flag_FROZEN ) );

            THEN( "it appears frozen" ) {
                CHECK( corpse.tname() == "corpse of a human (fresh) (frozen)" );
            }
        }
        WHEN( "cold" ) {
            corpse.set_flag( flag_COLD );
            REQUIRE( corpse.has_flag( flag_COLD ) );

            THEN( "it appears cold" ) {
                CHECK( corpse.tname() == "corpse of a human (fresh) (cold)" );
            }
        }

        WHEN( "hot" ) {
            corpse.set_flag( flag_HOT );
            REQUIRE( corpse.has_flag( flag_HOT ) );

            THEN( "it appears hot" ) {
                CHECK( corpse.tname() == "corpse of a human (fresh) (hot)" );
            }
        }
    }

    GIVEN( "an item that is not food or a corpse" ) {
        item hammer( "hammer" );
        REQUIRE_FALSE( hammer.is_food() );
        REQUIRE_FALSE( hammer.is_corpse() );

        WHEN( "it is hot" ) {
            hammer.set_flag( flag_HOT );
            REQUIRE( hammer.has_flag( flag_HOT ) );

            THEN( "it does not appear hot" ) {
                CHECK( hammer.tname() == "hammer" );
            }
        }

        WHEN( "it is cold" ) {
            hammer.set_flag( flag_COLD );
            REQUIRE( hammer.has_flag( flag_COLD ) );

            THEN( "it does not appear cold" ) {
                CHECK( hammer.tname() == "hammer" );
            }
        }

        WHEN( "it is frozen" ) {
            hammer.set_flag( flag_FROZEN );
            REQUIRE( hammer.has_flag( flag_FROZEN ) );

            THEN( "it does not appear frozen" ) {
                CHECK( hammer.tname() == "hammer" );
            }
        }
    }
}

TEST_CASE( "wet item", "[item][tname][wet]" )
{
    item rag( "rag" );
    rag.set_flag( flag_WET );
    REQUIRE( rag.has_flag( flag_WET ) );

    CHECK( rag.tname() == "rag (wet)" );
}

TEST_CASE( "filthy item", "[item][tname][filthy]" )
{
    override_option opt( "FILTHY_MORALE", "true" );
    item rag( "rag" );
    rag.set_flag( flag_FILTHY );
    REQUIRE( rag.is_filthy() );

    CHECK( rag.tname() == "rag (filthy)" );
}

TEST_CASE( "diamond item", "[item][tname][diamond]" )
{
    item katana( "katana" );
    katana.set_flag( flag_DIAMOND );
    REQUIRE( katana.has_flag( flag_DIAMOND ) );

    CHECK( katana.tname() == "diamond katana" );
}

TEST_CASE( "truncated item name", "[item][tname][truncate]" )
{
    SECTION( "plain item name can be truncated" ) {
        item katana( "katana" );

        CHECK( katana.tname() == "katana" );
        CHECK( katana.tname( 1, false, 5 ) == "katan" );
    }
    // color-coded or otherwise embellished item name can be truncated
}

// item health bar

// clothing with +1 suffix

// - ethereal: (X turns)

// is_bionic: (sterile), (packed)

// is_tool, UPS: (UPS)


// tname tests to consider:
// - indicates gun fouling level
// - includes (faulty) for faults
// - shows engine displacement (L) for engines
// - shows wheel diameter for wheels
// - indicates "burnt" or "badly burnt"
//
// - special cases for
//   - gun/tool/magazine
//   - armor/clothing mod
//   - craft
//   - contents.size == 1
//   - contents.empty

// (these are a mixture of tags, flags, and function calls)
// - is_food or goes_bad
//   - (dirty)
//   - (rotten)
//   - (mushy)
//   - (old)
//   - (fresh)

//
// RADIO_MOD (RBG)
//
// misc flags: used, lit, plugged in, active, sawn-off
// favorite: *

// ALREADY COVERED
// sizing level:
//   - (too big)
//   - (huge!)
//   - (too small)
//   - (tiny!)
//   - (poor fit)
//
