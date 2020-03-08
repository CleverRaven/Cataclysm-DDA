#include <memory>
#include <string>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "flat_set.h"
#include "type_id.h"

TEST_CASE( "item_name_check", "[item][iteminfo]" )
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

TEST_CASE( "item type name", "[item][type_name]" )
{
    SECTION( "named item" ) {
        // Shop smart. Shop S-Mart.
        item shotgun( "shotgun_410" );
        shotgun.set_var( "name", "Boomstick" );
        REQUIRE( shotgun.get_var( "name" ) == "Boomstick" );

        CHECK( shotgun.type_name() == "Boomstick" );
    }

    static const mtype_id mon_zombie( "mon_zombie" );
    static const mtype_id mon_chicken( "mon_chicken" );

    SECTION( "blood" ) {

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

    SECTION( "corpses" ) {

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
}

// Test conditional naming qualifiers
// (based on the "Conditional Naming" section of doc/JSON_INFO.md)
//
// FIXME: All but the base case (normal meat/fat) are failing
//
TEST_CASE( "item type name with conditions", "[item][type_name][conditions][!mayfail]" )
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

