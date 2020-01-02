#include "requirements.h"

#include "catch/catch.hpp"

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
    test_requirement_deduplication( { { { "rock", 1 } } }, { { { { "rock", 1 } } } } );
}

TEST_CASE( "survivor_telescope_inspired_example", "[requirement]" )
{
    requirement_data::alter_item_comp_vector before;
    test_requirement_deduplication(
    { { { "rock", 1 }, { "soap", 1 } }, { { "rock", 1 } } }, {
        { { { "soap", 1 } }, { { "rock", 1 } } },
        { { { "rock", 2 } } }
    } );
}

TEST_CASE( "survivor_telescope_inspired_example_2", "[requirement]" )
{
    requirement_data::alter_item_comp_vector before;
    test_requirement_deduplication(
    { { { "ash", 1 } }, { { "rock", 1 }, { "soap", 1 } }, { { "rock", 1 } }, { { "lye", 1 } } }, {
        { { { "ash", 1 } }, { { "soap", 1 } }, { { "rock", 1 } }, { { "lye", 1 } } },
        { { { "ash", 1 } }, { { "rock", 2 } }, { { "lye", 1 } } }
    } );
}

TEST_CASE( "woods_soup_inspired_example", "[requirement]" )
{
    requirement_data::alter_item_comp_vector before;
    test_requirement_deduplication(
    { { { "rock", 1 }, { "soap", 1 } }, { { "rock", 1 }, { "yarn", 1 } } }, {
        { { { "soap", 1 } }, { { "rock", 1 }, { "yarn", 1 } } },
        { { { "rock", 1 } }, { { "yarn", 1 } } },
        { { { "rock", 2 } } }
    } );
}

TEST_CASE( "triple_overlap_1", "[requirement]" )
{
    requirement_data::alter_item_comp_vector before;
    test_requirement_deduplication( {
        { { "rock", 1 }, { "soap", 1 } },
        { { "rock", 1 } },
        { { "soap", 1 } }
    }, {
        { { { "rock", 1 } }, { { "soap", 2 } } },
        { { { "rock", 2 } }, { { "soap", 1 } } },
    } );
}

TEST_CASE( "triple_overlap_2", "[requirement]" )
{
    requirement_data::alter_item_comp_vector before;
    test_requirement_deduplication( {
        { { "rock", 1 }, { "soap", 1 } },
        { { "rock", 1 }, { "yarn", 1 } },
        { { "soap", 1 }, { "acid", 1 } }
    }, {
        { { { "soap", 1 } }, { { "rock", 1 }, { "yarn", 1 } }, { { "acid", 1 } } },
        { { { "rock", 1 }, { "yarn", 1 } }, { { "soap", 2 } } },
        { { { "rock", 1 } }, { { "yarn", 1 } }, { { "acid", 1 }, { "soap", 1 } } },
        { { { "rock", 2 } }, { { "acid", 1 }, { "soap", 1 } } },
    } );
}

TEST_CASE( "triple_overlap_3", "[requirement]" )
{
    requirement_data::alter_item_comp_vector before;
    test_requirement_deduplication( {
        { { "rock", 1 }, { "soap", 1 } },
        { { "rock", 1 }, { "yarn", 1 } },
        { { "soap", 1 }, { "yarn", 1 } }
    }, {
        // These results are not ideal.  Two of them are equivalent and
        // another two could be merged.  But they are correct, and that
        // seems good enough for now.  I don't anticipate any real recipes
        // being as complicated to resolve as this one.
        { { { "soap", 1 } }, { { "rock", 1 } }, { { "yarn", 1 } } },
        { { { "soap", 1 } }, { { "yarn", 2 } } },
        { { { "rock", 1 }, { "yarn", 1 } }, { { "soap", 2 } } },
        { { { "rock", 1 } }, { { "yarn", 1 } }, { { "soap", 1 } } },
        { { { "rock", 1 } }, { { "yarn", 2 } } },
        { { { "rock", 2 } }, { { "yarn", 1 }, { "soap", 1 } } },
    } );
}

TEST_CASE( "deduplicate_repeated_requirements", "[requirement]" )
{
    requirement_data::alter_item_comp_vector before;
    test_requirement_deduplication( {
        { { "rock", 1 } }, { { "yarn", 1 } }, { { "rock", 1 } }, { { "yarn", 1 } }
    }, {
        { { { "rock", 2 } }, { { "yarn", 2 } } },
    } );
}
