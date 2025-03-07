#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "field_type.h"
#include "string_id_utils.h"
#include "type_id.h"

class json_flag;

TEST_CASE( "sizeof_new_id", "[.][int_id][string_id]" )
{
    DYNAMIC_SECTION( "sizeof: int_id: " << sizeof( flag_id ) ) {}
    DYNAMIC_SECTION( "sizeof: interned string_id: " << sizeof( flag_id ) ) {}
    DYNAMIC_SECTION( "sizeof: dynamic string_id: " << sizeof( trait_group::Trait_group_tag ) ) {}
}

TEST_CASE( "static_string_ids_equality_test", "[string_id]" )
{
    // NOLINTNEXTLINE(cata-static-string_id-constants)
    CHECK( fd_smoke == string_id<field_type>( "fd_smoke" ) );
    // NOLINTNEXTLINE(cata-static-string_id-constants)
    CHECK( fd_toxic_gas == string_id<field_type>( "fd_toxic_gas" ) );
    // NOLINTNEXTLINE(cata-static-string_id-constants)
    CHECK( fd_tear_gas == string_id<field_type>( "fd_tear_gas" ) );
    // NOLINTNEXTLINE(cata-static-string_id-constants)
    CHECK( fd_nuke_gas == string_id<field_type>( "fd_nuke_gas" ) );

    // NOLINTNEXTLINE(cata-static-string_id-constants)
    CHECK( fd_nuke_gas != string_id<field_type>( "fd_nuke_gas1" ) );
}

TEST_CASE( "string_ids_intern_test", "[string_id]" )
{
    static constexpr int num_ids = 100000;

    struct test_obj {};
    std::vector<string_id<test_obj>> ids;
    // lots of ids to make sure that "interning" map gets expanded
    ids.reserve( num_ids );
    for( int i = 0; i < num_ids; ++i ) {
        ids.emplace_back( "test_id" + std::to_string( i ) );
    }

    // check that interning works
    for( const auto &id : ids ) {
        CAPTURE( id );
        CHECK( id == id );
        CHECK_FALSE( id != id );
        CHECK_FALSE( id < id );

        CHECK( id == string_id<test_obj>( id.str() ) );
        CHECK_FALSE( id != string_id<test_obj>( id.str() ) );
        CHECK( &id.str() == &string_id<test_obj>( id.str() ).str() );
    }

    // comparison sanity check
    for( int i = 1; i < num_ids ; ++i ) {
        CAPTURE( ids[i - 1], ids[i] );
        CHECK( ids[i - 1] != ids[i] );
        // exactly one of two should be true
        CHECK( ( ids[i - 1] < ids[i] ) ^ ( ids[i] < ids[i - 1] ) );
    }
}

TEST_CASE( "string_ids_collection_equality", "[string_id]" )
{
    struct test_obj {};
    using id = string_id<test_obj>;

    // NOLINTNEXTLINE(cata-static-string_id-constants)
    CHECK( std::set<id> {id( "test1" ), id( "test3" ), id( "test2" )} ==
           // NOLINTNEXTLINE(cata-static-string_id-constants)
           std::set<id> {id( "test2" ), id( "test1" ), id( "test3" )} );

    // NOLINTNEXTLINE(cata-static-string_id-constants)
    CHECK( std::set<id> {id( "test1" ), id( "test3" ), id( "test2" )} !=
           // NOLINTNEXTLINE(cata-static-string_id-constants)
           std::set<id> {id( "test2" ), id( "test1" ), id( "test4" )} );

    // NOLINTNEXTLINE(cata-static-string_id-constants)
    std::vector<id> vec1 { id( "test4" ), id( "test2" ), id( "test3" ), id( "test1" ) };
    // NOLINTNEXTLINE(cata-static-string_id-constants)
    std::vector<id> vec2 { id( "test2" ), id( "test4" ), id( "test1" ), id( "test3" ) };

    CHECK( vec1 != vec2 );

    std::sort( vec1.begin(), vec1.end() );
    std::sort( vec2.begin(), vec2.end() );

    CHECK( vec1 == vec2 );
}

TEST_CASE( "string_id_sorting_test", "[string_id]" )
{
    struct test_obj {};
    using id = string_id<test_obj>;

    SECTION( "map sorting" ) {
        std::map<id, int> m;
        for( int i = 0; i < 10; ++i ) {
            m.insert( {id( "id" + std::to_string( i ) ), i} );
        }

        int i = 0;
        for( const std::pair<id, int> &p : sorted_lex( m ) ) {
            // values (and ids) are iterated in order
            CHECK( p.second == i );
            i++;
        }
    }

    SECTION( "vector of pairs sorting" ) {
        std::vector<std::pair<id, int>> vec;
        for( int i = 9; i >= 0; i-- ) {
            vec.emplace_back( id( "id" + std::to_string( i ) ), i );
        }

        int i = 0;
        for( const std::pair<id, int> &p : sorted_lex( vec ) ) {
            // values (and ids) are iterated in order
            CHECK( p.second == i );
            i++;
        }
    }

    SECTION( "vector ids sorting" ) {
        std::vector<id> vec;
        vec.reserve( 10 );
        for( int i = 0; i < 10; ++i ) {
            vec.emplace_back( "id" + std::to_string( i ) );
        }

        int i = 0;
        for( const id &id : sorted_lex( vec ) ) {
            // values (and ids) are iterated in order
            CHECK( id.str() == ( "id" + std::to_string( i ) ) );
            i++;
        }
    }

    SECTION( "set of pairs sorting" ) {
        std::set<id> s;
        for( int i = 0; i < 10; ++i ) {
            s.insert( id( "id" + std::to_string( i ) ) );
        }

        int i = 0;
        for( const id &id : sorted_lex( s ) ) {
            // values (and ids) are iterated in order
            CHECK( id.str() == ( "id" + std::to_string( i ) ) );
            i++;
        }
    }
}

TEST_CASE( "string_id_creation_benchmark", "[.][string_id][benchmark]" )
{
    static constexpr int num_test_strings = 30;
    std::vector<std::string> test_strings;
    test_strings.reserve( num_test_strings );
    for( int i = 0; i < num_test_strings; ++i ) {
        test_strings.push_back( "some_test_string_" + std::to_string( i ) );
    }

    // is_empty is chosen as the most lightweight method available
    // that hopefully prevents the inlining of string_id construction

    int i = 0;

    BENCHMARK( "interned string_id creation" ) {
        return string_id<json_flag>( test_strings[( i++ % num_test_strings )] ).is_empty();
    };

    BENCHMARK( "dynamic string_id creation" ) {
        return trait_group::Trait_group_tag( test_strings[( i++ % num_test_strings )] ).is_empty();
    };
}
