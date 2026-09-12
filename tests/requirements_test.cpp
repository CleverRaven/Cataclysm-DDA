#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "butchery.h"
#include "butchery_requirements.h"
#include "cata_catch.h"
#include "cata_utility.h"
#include "color.h"
#include "creature.h"
#include "harvest.h"
#include "inventory.h"
#include "item.h"
#include "npc.h"
#include "player_helpers.h"
#include "requirements.h"
#include "type_id.h"

static const butchery_requirements_id butchery_requirements_default( "default" );

static const itype_id itype_ash( "ash" );
static const itype_id itype_chem_sulphuric_acid( "chem_sulphuric_acid" );
static const itype_id itype_debug_backpack( "debug_backpack" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_lye( "lye" );
static const itype_id itype_metal_tank_test( "metal_tank_test" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_soap( "soap" );
static const itype_id itype_test_egg( "test_egg" );
static const itype_id itype_test_glass_pipe_mostly_glass( "test_glass_pipe_mostly_glass" );
static const itype_id itype_test_glass_pipe_mostly_steel( "test_glass_pipe_mostly_steel" );
static const itype_id itype_test_pipe( "test_pipe" );
static const itype_id itype_yarn( "yarn" );

static const quality_id qual_BUTCHER( "BUTCHER" );
static const quality_id qual_HAMMER( "HAMMER" );

static const requirement_id requirement_data_eggs_bird( "eggs_bird" );
static const requirement_id
requirement_data_explosives_casting_standard( "explosives_casting_standard" );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );
static const trait_id trait_MANDIBLES( "MANDIBLES" );

static void test_requirement_deduplication(
    const requirement_data::alter_item_comp_vector &before,
    std::vector<requirement_data::alter_item_comp_vector> after
)
{
    requirement_data in( {}, {}, before );
    deduped_requirement_data out( in, recipe_id::NULL_ID() );
    CHECK( out.alternatives().size() == after.size() );
    while( after.size() < out.alternatives().size() ) {
        after.emplace_back();
    }

    for( size_t i = 0; i < out.alternatives().size(); ++i ) {
        CAPTURE( i );
        requirement_data this_expected( {}, {}, after[i] );
        CHECK( out.alternatives()[i].list_all() == this_expected.list_all() );
    }
}

TEST_CASE( "simple_requirements_dont_multiply", "[requirement]" )
{
    test_requirement_deduplication( { { { itype_rock, 1 } } }, { { { { itype_rock, 1 } } } } );
}

TEST_CASE( "survivor_telescope_inspired_example", "[requirement]" )
{
    test_requirement_deduplication(
    { { { itype_rock, 1 }, { itype_soap, 1 } }, { { itype_rock, 1 } } }, {
        { { { itype_soap, 1 } }, { { itype_rock, 1 } } },
        { { { itype_rock, 2 } } }
    } );
}

TEST_CASE( "survivor_telescope_inspired_example_2", "[requirement]" )
{
    test_requirement_deduplication(
    { { { itype_ash, 1 } }, { { itype_rock, 1 }, { itype_soap, 1 } }, { { itype_rock, 1 } }, { { itype_lye, 1 } } }, {
        { { { itype_ash, 1 } }, { { itype_soap, 1 } }, { { itype_rock, 1 } }, { { itype_lye, 1 } } },
        { { { itype_ash, 1 } }, { { itype_rock, 2 } }, { { itype_lye, 1 } } }
    } );
}

TEST_CASE( "woods_soup_inspired_example", "[requirement]" )
{
    test_requirement_deduplication(
    { { { itype_rock, 1 }, { itype_soap, 1 } }, { { itype_rock, 1 }, { itype_yarn, 1 } } }, {
        { { { itype_soap, 1 } }, { { itype_rock, 1 }, { itype_yarn, 1 } } },
        { { { itype_rock, 1 } }, { { itype_yarn, 1 } } },
        { { { itype_rock, 2 } } }
    } );
}

TEST_CASE( "triple_overlap_1", "[requirement]" )
{
    test_requirement_deduplication( {
        { { itype_rock, 1 }, { itype_soap, 1 } },
        { { itype_rock, 1 } },
        { { itype_soap, 1 } }
    }, {
        { { { itype_rock, 1 } }, { { itype_soap, 2 } } },
        { { { itype_rock, 2 } }, { { itype_soap, 1 } } },
    } );
}

TEST_CASE( "triple_overlap_2", "[requirement]" )
{
    test_requirement_deduplication( {
        { { itype_rock, 1 }, { itype_soap, 1 } },
        { { itype_rock, 1 }, { itype_yarn, 1 } },
        { { itype_soap, 1 }, { itype_chem_sulphuric_acid, 1 } }
    }, {
        { { { itype_soap, 1 } }, { { itype_rock, 1 }, { itype_yarn, 1 } }, { { itype_chem_sulphuric_acid, 1 } } },
        { { { itype_rock, 1 }, { itype_yarn, 1 } }, { { itype_soap, 2 } } },
        { { { itype_rock, 1 } }, { { itype_yarn, 1 } }, { { itype_chem_sulphuric_acid, 1 }, { itype_soap, 1 } } },
        { { { itype_rock, 2 } }, { { itype_chem_sulphuric_acid, 1 }, { itype_soap, 1 } } },
    } );
}

TEST_CASE( "triple_overlap_3", "[requirement]" )
{
    test_requirement_deduplication( {
        { { itype_rock, 1 }, { itype_soap, 1 } },
        { { itype_rock, 1 }, { itype_yarn, 1 } },
        { { itype_soap, 1 }, { itype_yarn, 1 } }
    }, {
        // These results are not ideal.  Two of them are equivalent and
        // another two could be merged.  But they are correct, and that
        // seems good enough for now.  I don't anticipate any real recipes
        // being as complicated to resolve as this one.
        { { { itype_soap, 1 } }, { { itype_rock, 1 } }, { { itype_yarn, 1 } } },
        { { { itype_soap, 1 } }, { { itype_yarn, 2 } } },
        { { { itype_rock, 1 }, { itype_yarn, 1 } }, { { itype_soap, 2 } } },
        { { { itype_rock, 1 } }, { { itype_yarn, 1 } }, { { itype_soap, 1 } } },
        { { { itype_rock, 1 } }, { { itype_yarn, 2 } } },
        { { { itype_rock, 2 } }, { { itype_yarn, 1 }, { itype_soap, 1 } } },
    } );
}

TEST_CASE( "deduplicate_repeated_requirements", "[requirement]" )
{
    test_requirement_deduplication( {
        { { itype_rock, 1 } }, { { itype_yarn, 1 } }, { { itype_rock, 1 } }, { { itype_yarn, 1 } }
    }, {
        { { { itype_rock, 2 } }, { { itype_yarn, 2 } } },
    } );
}

TEST_CASE( "requirement_extension", "[requirement]" )
{
    SECTION( "basic_component_extension" ) {
        const std::vector<std::vector<item_comp>> &req_comp = requirement_data_eggs_bird->get_components();

        REQUIRE( !req_comp.empty() );
        REQUIRE( req_comp.size() == 1 );
        REQUIRE( req_comp.front().size() > 1 );

        bool found_extended_comp = false;
        for( const item_comp &comp : req_comp[0] ) {
            if( comp.type == itype_test_egg ) {
                found_extended_comp = true;
                CHECK( comp.count == 2 );
            }
        }
        CHECK( found_extended_comp );
    }

    SECTION( "multigroup_tool_extension" ) {
        const std::vector<std::vector<tool_comp>> &req_tool =
                requirement_data_explosives_casting_standard->get_tools();

        REQUIRE( !req_tool.empty() );
        REQUIRE( req_tool.size() == 2 );
        REQUIRE( req_tool[0].size() > 1 );
        REQUIRE( req_tool[1].size() > 3 );

        std::vector<std::map<const itype_id, bool>> found_itype_maps = {
            {
                { itype_metal_tank_test, false }
            },
            {
                { itype_test_pipe, false },
                { itype_test_glass_pipe_mostly_steel, false },
                { itype_test_glass_pipe_mostly_glass, false }
            }
        };

        for( int i = 0; i < static_cast<int>( req_tool.size() ); i++ ) {
            for( const tool_comp &tool : req_tool[i] ) {
                for( std::pair<const itype_id, bool> &f : found_itype_maps[i] ) {
                    if( tool.type == f.first ) {
                        f.second = true;
                    }
                }
            }
        }

        for( const std::map<const itype_id, bool> &f : found_itype_maps ) {
            for( const std::pair<const itype_id, bool> &found : f ) {
                CAPTURE( found.first.c_str() );
                CHECK( found.second );
            }
        }
    }
}

TEST_CASE( "requirement_checks_follow_their_actor_mode", "[requirement]" )
{
    clear_avatar();
    avatar &u = get_avatar();
    const inventory empty_inv;

    GIVEN( "a requirement demanding a quality, a tool and a component" ) {
        const requirement_data req( { { tool_comp( itype_hammer, -1 ) } },
        { { quality_requirement( qual_HAMMER, 1, 1 ) } },
        { { item_comp( itype_rock, 1 ) } } );
        const quality_requirement qual_req( qual_HAMMER, 1, 1 );
        const tool_comp tool_req( itype_hammer, -1 );
        const item_comp comp_req( itype_rock, 1 );

        WHEN( "the avatar carries a qualifying tool and the component" ) {
            u.wear_item( item( itype_debug_backpack ) );
            u.i_add( item( itype_hammer ) );
            u.i_add( item( itype_rock ) );

            THEN( "an inventory-only check is decided on the passed inventory alone" ) {
                CHECK_FALSE( req.can_make_with_inventory( nullptr, empty_inv,
                             return_true<item> ) );
                CHECK_FALSE( qual_req.has( nullptr, empty_inv, return_true<item> ) );
            }
        }

        WHEN( "the avatar has debug hammerspace" ) {
            u.set_mutation( trait_DEBUG_HS );

            THEN( "an inventory-only check grants none of it, at any level" ) {
                CHECK_FALSE( req.can_make_with_inventory( nullptr, empty_inv,
                             return_true<item> ) );
                CHECK_FALSE( qual_req.has( nullptr, empty_inv, return_true<item> ) );
                CHECK_FALSE( tool_req.has( nullptr, empty_inv, return_true<item> ) );
                CHECK_FALSE( comp_req.has( nullptr, empty_inv, return_true<item> ) );
            }

            THEN( "the display path agrees with the refusal" ) {
                CHECK( qual_req.get_color( nullptr, false, empty_inv,
                                           return_true<item> ) != c_green );
            }

            THEN( "the same check for the avatar as actor is granted" ) {
                CHECK( req.can_make_with_inventory( &u, empty_inv, return_true<item> ) );
            }
        }

        WHEN( "an NPC without the trait stands beside a hammerspace avatar" ) {
            u.set_mutation( trait_DEBUG_HS );
            standard_npc guy( "ModeTester" );

            THEN( "the NPC is not granted the avatar's hammerspace" ) {
                CHECK_FALSE( req.can_make_with_inventory( &guy, empty_inv,
                             return_true<item> ) );
            }

            THEN( "the NPC's own trait is what grants it" ) {
                guy.set_mutation( trait_DEBUG_HS );
                CHECK( req.can_make_with_inventory( &guy, empty_inv, return_true<item> ) );
            }
        }
    }

    GIVEN( "a crafter whose only butchering quality is a mutation grant" ) {
        u.set_mutation( trait_MANDIBLES );
        const quality_requirement butcher_req( qual_BUTCHER, 1, 1 );

        THEN( "the actor-aware check reaches the intrinsic grant" ) {
            CHECK( butcher_req.has( &u, empty_inv, return_true<item> ) );
        }

        THEN( "the inventory-only check has no actor to reach it through" ) {
            CHECK_FALSE( butcher_req.has( nullptr, empty_inv, return_true<item> ) );
        }
    }

    GIVEN( "the default butchery ladder, whose tiers differ in speed" ) {
        u.set_mutation( trait_DEBUG_HS );
        standard_npc guy( "ModeTester" );

        THEN( "the actor decides which tier is reachable" ) {
            // The hammerspace avatar satisfies the first tier; the bare NPC falls
            // through to the fastest-tier fallback, so the pairs differ.  Depends on
            // the "default" ladder carrying more than one speed tier.
            const std::pair<float, requirement_id> for_avatar =
                butchery_requirements_default.obj().get_fastest_requirements(
                    &u, empty_inv, creature_size::medium, butcher_type::FULL );
            const std::pair<float, requirement_id> for_npc =
                butchery_requirements_default.obj().get_fastest_requirements(
                    &guy, empty_inv, creature_size::medium, butcher_type::FULL );
            CHECK( for_avatar != for_npc );
        }
    }
}
