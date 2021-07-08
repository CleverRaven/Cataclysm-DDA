#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "requirements.h"
#include "type_id.h"

static const itype_id itype_chem_sulphuric_acid( "chem_sulphuric_acid" );
static const itype_id itype_ash( "ash" );
static const itype_id itype_lye( "lye" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_soap( "soap" );
static const itype_id itype_yarn( "yarn" );

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
