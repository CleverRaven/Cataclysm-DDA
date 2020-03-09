#include <memory>
#include <string>

#include "avatar.h"
#include "cata_string_consts.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "flat_set.h"
#include "options_helpers.h"
#include "type_id.h"

TEST_CASE( "item sizing display", "[item][iteminfo][display_name][sizing]" )
{
    GIVEN( "player is a normal size" ) {
        g->u.empty_traits();

        WHEN( "the item is a normal size" ) {
            std::string name = item( "bookplate" ).display_name();
            THEN( "the item name has no qualifier" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>bookplate" );
            }
        }

        WHEN( "the item is oversized" ) {
            std::string name = item( "bootsheath" ).display_name();
            THEN( "the item name has no qualifier" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>ankle sheath" );
            }
        }

        WHEN( "the item is undersized" ) {
            item i = item( "tunic" );
            i.item_tags.insert( "UNDERSIZE" );
            i.item_tags.insert( "FIT" );
            std::string name = i.display_name();

            THEN( "we have the correct sizing" ) {
                const item::sizing sizing_level = i.get_sizing( g->u, true );
                CHECK( sizing_level == item::sizing::small_sized_human_char );
            }

            THEN( "the item name says its too small" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>tunic (too small)" );
            }
        }

    }

    GIVEN( "player is a huge size" ) {
        g->u.empty_traits();
        g->u.toggle_trait( trait_id( "HUGE_OK" ) );

        WHEN( "the item is a normal size" ) {
            std::string name = item( "bookplate" ).display_name();
            THEN( "the item name says its too small" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>bookplate (too small)" );
            }
        }

        WHEN( "the item is oversized" ) {
            std::string name = item( "bootsheath" ).display_name();
            THEN( "the item name has no qualifier" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>ankle sheath" );
            }
        }

        WHEN( "the item is undersized" ) {
            item i = item( "tunic" );
            i.item_tags.insert( "UNDERSIZE" );
            i.item_tags.insert( "FIT" );
            std::string name = i.display_name();

            THEN( "we have the correct sizing" ) {
                const item::sizing sizing_level = i.get_sizing( g->u, true );
                CHECK( sizing_level == item::sizing::small_sized_big_char );
            }

            THEN( "the item name says its tiny" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>tunic (tiny!)" );
            }
        }

    }

    GIVEN( "player is a small size" ) {
        g->u.empty_traits();
        g->u.toggle_trait( trait_id( "SMALL_OK" ) );

        WHEN( "the item is a normal size" ) {
            std::string name = item( "bookplate" ).display_name();
            THEN( "the item name says its too big" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>bookplate (too big)" );
            }
        }

        WHEN( "the item is oversized" ) {
            std::string name = item( "bootsheath" ).display_name();
            THEN( "the item name has no qualifier" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>ankle sheath (huge!)" );
            }
        }

        WHEN( "the item is undersized" ) {
            item i = item( "tunic" );
            i.item_tags.insert( "UNDERSIZE" );
            i.item_tags.insert( "FIT" );
            std::string name = i.display_name();

            THEN( "we have the correct sizing" ) {
                const item::sizing sizing_level = i.get_sizing( g->u, true );
                CHECK( sizing_level == item::sizing::small_sized_small_char );
            }

            THEN( "the item name has no qualifier" ) {
                CHECK( name == "<color_c_light_green>||\u00A0</color>tunic" );
            }
        }

    }
}

TEST_CASE( "custom named item", "[item][type_name][named]" )
{
    // Shop smart. Shop S-Mart.
    item shotgun( "shotgun_410" );
    shotgun.set_var( "name", "Boomstick" );
    REQUIRE( shotgun.get_var( "name" ) == "Boomstick" );

    CHECK( shotgun.type_name() == "Boomstick" );
}

TEST_CASE( "blood item", "[item][type_name][blood]" )
{
    static const mtype_id mon_zombie( "mon_zombie" );
    static const mtype_id mon_chicken( "mon_chicken" );

    SECTION( "blood from a zombie corpse" ) {
        item corpse = item::make_corpse( mon_zombie );
        item blood( "blood" );
        blood.set_mtype( corpse.get_mtype() );
        REQUIRE( blood.typeId() == "blood" );
        REQUIRE_FALSE( blood.is_corpse() );

        CHECK( blood.type_name() == "zombie blood" );
    }

    SECTION( "blood from a chicken corpse" ) {
        item corpse = item::make_corpse( mon_chicken );
        item blood( "blood" );
        blood.set_mtype( corpse.get_mtype() );
        REQUIRE( blood.typeId() == "blood" );
        REQUIRE_FALSE( blood.is_corpse() );

        CHECK( blood.type_name() == "chicken blood" );
    }

    SECTION( "blood from an unknown corpse" ) {
        item blood( "blood" );
        REQUIRE( blood.typeId() == "blood" );
        REQUIRE_FALSE( blood.is_corpse() );

        CHECK( blood.type_name() == "human blood" );
    }
}

TEST_CASE( "corpse item", "[item][type_name][corpse]" )
{
    static const mtype_id mon_zombie( "mon_zombie" );
    static const mtype_id mon_chicken( "mon_chicken" );

    // Anonymous corpses

    SECTION( "human corpse" ) {
        item corpse = item::make_corpse( mtype_id::NULL_ID(), calendar::turn, "" );
        REQUIRE( corpse.is_corpse() );
        REQUIRE( corpse.get_corpse_name().empty() );

        CHECK( corpse.type_name() == "corpse of a human" );
    }

    SECTION( "zombie corpse" ) {
        item corpse = item::make_corpse( mon_zombie, calendar::turn, "" );
        REQUIRE( corpse.is_corpse() );
        REQUIRE( corpse.get_corpse_name().empty() );

        CHECK( corpse.type_name() == "corpse of a zombie" );
    }

    SECTION( "chicken corpse" ) {
        item corpse = item::make_corpse( mon_chicken, calendar::turn, "" );
        REQUIRE( corpse.is_corpse() );
        REQUIRE( corpse.get_corpse_name().empty() );

        CHECK( corpse.type_name() == "corpse of a chicken" );
    }

    // Corpses with names

    SECTION( "human corpse with a name" ) {
        item corpse = item::make_corpse( mtype_id::NULL_ID(), calendar::turn, "Dead Dude" );
        REQUIRE( corpse.is_corpse() );
        REQUIRE_FALSE( corpse.get_corpse_name().empty() );

        CHECK( corpse.type_name() == "corpse of Dead Dude, human" );
    }

    SECTION( "zombie corpse with a name" ) {
        item corpse = item::make_corpse( mon_zombie, calendar::turn, "Deadite Jones" );
        REQUIRE( corpse.is_corpse() );
        REQUIRE_FALSE( corpse.get_corpse_name().empty() );

        CHECK( corpse.type_name() == "corpse of Deadite Jones, zombie" );
    }

    SECTION( "chicken corpse with a name" ) {
        item corpse = item::make_corpse( mon_chicken, calendar::turn, "Herb" );
        REQUIRE( corpse.is_corpse() );
        REQUIRE_FALSE( corpse.get_corpse_name().empty() );

        CHECK( corpse.type_name() == "corpse of Herb, chicken" );
    }
}

// Test conditional naming qualifiers
// (based on the "Conditional Naming" section of doc/JSON_INFO.md)
//
// FIXME: All but the base case (normal meat/fat) are failing
//
TEST_CASE( "conditionally named item", "[item][type_name][conditions][!mayfail]" )
{
    // Fully-stocked pantry
    item normal_meat( "meat", calendar::turn, 100 );
    item normal_fat( "fat", calendar::turn, 100 );

    item mutant_meat( "mutant_meat", calendar::turn, 100 );
    item mutant_fat( "mutant_fat", calendar::turn, 100 );

    item human_meat( "human_flesh", calendar::turn, 100 );
    item human_fat( "human_fat", calendar::turn, 100 );

    item mutant_human_meat( "mutant_human_flesh", calendar::turn, 100 );
    item mutant_human_fat( "mutant_human_fat", calendar::turn, 100 );

    item salt( "salt", calendar::turn, 100 );

    std::list<item> ingredients;
    std::vector<item_comp> selections;

    GIVEN( "a sausage recipe" ) {
        const recipe sausage_rec = recipe_id( "sausage_raw" ).obj();

        WHEN( "it is made from non-human, non-mutant meat and fat" ) {
            ingredients = { normal_meat, normal_fat, salt };
            item sausage = item( &sausage_rec, 1, ingredients, selections ).get_making().create_result();

            THEN( "type_name does not include any special qualifiers" ) {
                CHECK( sausage.type_name() == "raw sausage" );
            }
        }

        WHEN( "it is made from normal animal meat, but contains mutant fat" ) {
            ingredients = { normal_meat, mutant_fat, salt };
            item sausage = item( &sausage_rec, 1, ingredients, selections ).get_making().create_result();

            THEN( "type_name suggests the food contains mutant" ) {
                CHECK( sausage.type_name() == "sinister raw sausage" );
            }
        }

        WHEN( "it is made from normal animal meat, but contains human fat" ) {
            ingredients = { normal_meat, human_fat, salt };
            item sausage = item( &sausage_rec, 1, ingredients, selections ).get_making().create_result();

            THEN( "type_name suggests the food contains human" ) {
                CHECK( sausage.type_name() == "raw Mannwurst" );
            }
        }

        WHEN( "it is made from normal animal meat, but contains mutant human fat" ) {
            ingredients = { normal_meat, mutant_human_fat, salt };
            item sausage = item( &sausage_rec, 1, ingredients, selections ).get_making().create_result();

            THEN( "type_name suggests the food contains mutant human" ) {
                CHECK( sausage.type_name() == "sinister raw Mannwurst" );
            }
        }
    }
}

TEST_CASE( "items with hidden effects", "[item][tname][hidden]" )
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
// item health bar
// sizing level:
//   - (too big)
//   - (huge!)
//   - (too small)
//   - (tiny!)
//   - (poor fit)
//
