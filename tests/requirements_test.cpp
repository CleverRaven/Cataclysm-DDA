#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "make_static.h"
#include "requirements.h"
#include "type_id.h"

static const itype_id itype_ash( "ash" );
static const itype_id itype_chem_sulphuric_acid( "chem_sulphuric_acid" );
static const itype_id itype_lye( "lye" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_soap( "soap" );
static const itype_id itype_yarn( "yarn" );

static const requirement_id requirement_data_eggs_bird( "eggs_bird" );
static const requirement_id
requirement_data_explosives_casting_standard( "explosives_casting_standard" );

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
            if( comp.type == STATIC( itype_id( "test_egg" ) ) ) {
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
                { STATIC( itype_id( "metal_tank_test" ) ), false }
            },
            {
                { STATIC( itype_id( "test_pipe" ) ), false },
                { STATIC( itype_id( "test_glass_pipe_mostly_steel" ) ), false },
                { STATIC( itype_id( "test_glass_pipe_mostly_glass" ) ), false }
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
