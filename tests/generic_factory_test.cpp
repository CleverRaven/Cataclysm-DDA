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