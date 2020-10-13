#include "catch/catch.hpp"

#include "generic_factory.h"

namespace
{
struct test_obj;
using test_obj_id = string_id<test_obj>;

struct test_obj {
    test_obj_id id;
    std::string value;
};

} // namespace

TEST_CASE( "generic_factory_insert_convert_valid", "[generic_factory]" )
{
    test_obj_id id_0( "id_0" );
    test_obj_id id_1( "id_1" );
    test_obj_id id_2( "id_2" );

    generic_factory<test_obj> test_factory( "test_factory" );

    REQUIRE_FALSE( test_factory.is_valid( id_0 ) );
    REQUIRE_FALSE( test_factory.is_valid( id_1 ) );
    REQUIRE_FALSE( test_factory.is_valid( id_2 ) );

    test_factory.insert( {test_obj_id( "id_0" ), "value_0"} );
    test_factory.insert( {test_obj_id( "id_1" ), "value_1"} );
    test_factory.insert( {test_obj_id( "id_2" ), "value_2"} );

    // testing correctness when finalized was both called and not called
    if( GENERATE( false, true ) ) {
        INFO( "Calling finalize" );
        test_factory.finalize();
    }

    CHECK( test_factory.is_valid( id_0 ) );
    CHECK( test_factory.is_valid( id_1 ) );
    CHECK( test_factory.is_valid( id_2 ) );
    CHECK( test_factory.size() == 3 );
    CHECK_FALSE( test_factory.empty() );

    CHECK_FALSE( test_factory.is_valid( test_obj_id( "non_existent_id" ) ) );

    int_id<test_obj> int_id_1 = test_factory.convert( test_obj_id( "id_1" ), int_id<test_obj>( -1 ) );
    CHECK( int_id_1.to_i() == 1 );

    CHECK( test_factory.convert( int_id_1 ) == id_1 );
    CHECK( test_factory.convert( int_id_1 ) == test_obj_id( "id_1" ) );

    CHECK( test_factory.obj( id_0 ).value == "value_0" );
    CHECK( test_factory.obj( id_1 ).value == "value_1" );
    CHECK( test_factory.obj( id_2 ).value == "value_2" );
    CHECK( test_factory.obj( int_id_1 ).value == "value_1" );

    test_factory.reset();
    // factory is be empty, all ids must be invalid
    CHECK( test_factory.empty() );
    CHECK( test_factory.size() == 0 ); // NOLINT

    // technically, lookup by non-valid int_id may result undefined behavior, if factory is non-empty
    // this is expected
    REQUIRE_FALSE( test_factory.is_valid( int_id_1 ) );
    REQUIRE_FALSE( test_factory.is_valid( id_0 ) );
    REQUIRE_FALSE( test_factory.is_valid( id_1 ) );
    REQUIRE_FALSE( test_factory.is_valid( id_2 ) );
}

TEST_CASE( "generic_factory_repeated_overwrite", "[generic_factory]" )
{
    test_obj_id id_1( "id_1" );
    generic_factory<test_obj> test_factory( "test_factory" );
    REQUIRE_FALSE( test_factory.is_valid( id_1 ) );

    test_factory.insert( { test_obj_id( "id_1" ), "1" } );
    CHECK( test_factory.is_valid( id_1 ) );
    CHECK( test_factory.obj( id_1 ).value == "1" );

    test_factory.insert( { test_obj_id( "id_1" ), "2" } );
    CHECK( test_factory.is_valid( id_1 ) );
    CHECK( test_factory.obj( id_1 ).value == "2" );
}

TEST_CASE( "generic_factory_repeated_invalidation", "[generic_factory]" )
{
    // if id is static, factory must be static (or singleton by other means)
    static test_obj_id id_1( "id_1" );
    static generic_factory<test_obj> test_factory( "test_factory" );

    for( int i = 0; i < 10; i++ ) {
        INFO( "iteration: " << i );
        test_factory.reset();
        REQUIRE_FALSE( test_factory.is_valid( id_1 ) );
        std::string value = "value_" + std::to_string( i );
        test_factory.insert( { test_obj_id( "id_1" ), value } );
        CHECK( test_factory.is_valid( id_1 ) );
        CHECK( test_factory.obj( id_1 ).value == value );
    }
}

TEST_CASE( "generic_factory_common_null_ids", "[generic_factory]" )
{
    CHECK( itype_id::NULL_ID().is_null() );
    CHECK( itype_id::NULL_ID().is_valid() );

    CHECK( field_type_str_id::NULL_ID().is_null() );
    CHECK( field_type_str_id::NULL_ID().is_valid() );
}

TEST_CASE( "generic_factory_version_wrapper", "[generic_factory]" )
{
    generic_factory<test_obj> test_factory( "test_factory" );
    generic_factory<test_obj>::Version v1;
    generic_factory<test_obj>::Version v2;

    CHECK_FALSE( test_factory.is_valid( v1 ) );
    test_factory.reset();
    CHECK_FALSE( test_factory.is_valid( v1 ) );

    v1 = test_factory.get_version();

    CHECK( test_factory.is_valid( v1 ) );
    CHECK_FALSE( v1 == v2 );
    CHECK( v1 != v2 );

    v2 = v1;

    CHECK( v1 == v2 );
    CHECK_FALSE( v1 != v2 );

    test_factory.reset();
    CHECK_FALSE( test_factory.is_valid( v1 ) );
    CHECK_FALSE( test_factory.is_valid( v2 ) );
}

TEST_CASE( "string_ids_comparison", "[generic_factory][string_id]" )
{
    //  checks equality correctness for the following combinations of parameters:
    bool equal = GENERATE( true, false ); // whether ids are equal
    bool first_cached = GENERATE( true, false ); // whether _version is current (first id)
    bool first_valid = GENERATE( true, false ); // whether id is present in the factory (first id)
    bool second_cached = GENERATE( true, false ); // --- second id
    bool second_valid = GENERATE( true, false ); // --- second id

    test_obj_id id1( "id1" );
    test_obj_id id2( ( equal ?  "id1" : "id2" ) );

    generic_factory<test_obj> test_factory( "test_factory" );

    // both ids must be inserted first and then cached for the test to be valid
    // as `insert` invalidates the cache
    if( first_valid ) {
        test_factory.insert( { id1, "value" } );
    }
    if( second_valid ) {
        test_factory.insert( { id2, "value" } );
    }
    if( first_cached ) {
        // is_valid should update cache internally
        test_factory.is_valid( id1 );
    }
    if( second_cached ) {
        test_factory.is_valid( id2 );
    }

    const auto id_info = []( bool cached, bool valid ) {
        std::ostringstream ret;
        ret << "cached_" << cached << "__" "valid_" << valid;
        return ret.str();
    };

    DYNAMIC_SECTION( ( equal ? "equal" : "not_equal" ) << "___" <<
                     "first_" << id_info( first_cached, first_valid ) << "___"
                     "second_" << id_info( second_cached, second_valid )
                   ) {
        if( equal ) {
            CHECK( id1 == id2 );
        } else {
            CHECK_FALSE( id1 == id2 );
        }
    }
}

// Benchmarks are skipped by default by using [.] tag
TEST_CASE( "generic_factory_lookup_benchmark", "[.][generic_factory][benchmark]" )
{
    test_obj_id id_200( "id_200" );

    generic_factory<test_obj> test_factory( "test_factory" );

    for( int i = 0; i < 1000; ++i ) {
        std::string id = "id_" + std::to_string( i );
        std::string value = "value_" + std::to_string( i );
        test_factory.insert( {test_obj_id( id ), value} );
    }

    BENCHMARK( "single lookup" ) {
        return test_factory.obj( id_200 ).value;
    };
}

TEST_CASE( "string_id_compare_benchmark", "[.][generic_factory][string_id][benchmark]" )
{
    std::string prefix;
    SECTION( "short id" ) {
    }
    SECTION( "long id" ) {
        prefix = "log_common_prefix_";
    }

    test_obj_id id_200( prefix + "id_200" );
    test_obj_id id_200_( prefix + "id_200" );
    test_obj_id id_300( prefix + "id_300" );

    generic_factory<test_obj> test_factory( "test_factory" );

    CHECK( id_200 == id_200_ );
    BENCHMARK( "ids are equal, invalid version" ) {
        return id_200 == id_200_;
    };

    CHECK_FALSE( id_200 == id_300 );
    BENCHMARK( "ids are not equal, invalid version" ) {
        return id_200 == id_300;
    };

    test_factory.insert( {test_obj_id( id_200 ), "value_200"} );
    test_factory.insert( {test_obj_id( id_200_ ), "value_200_"} );
    test_factory.insert( {test_obj_id( id_300 ), "value_300"} );
    // make _version inside the ids valid
    test_factory.is_valid( id_200 );
    test_factory.is_valid( id_200_ );
    test_factory.is_valid( id_300 );

    CHECK( id_200 == id_200_ );
    BENCHMARK( "ids are equal, valid version" ) {
        return id_200 == id_200_;
    };

    CHECK_FALSE( id_200 == id_300 );
    BENCHMARK( "ids are not equal, valid version" ) {
        return id_200 == id_300;
    };
}
