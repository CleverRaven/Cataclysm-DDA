#include <algorithm>
#include <bitset>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "cata_catch.h"
#include "colony_list_test_helpers.h"
#include "flat_set.h"
#include "generic_factory.h"
#include "type_id.h"

#ifdef _MSC_VER
#  include <intrin.h>

#  define __builtin_popcount __popcnt
#endif

namespace
{
struct test_obj;

using test_obj_id = string_id<test_obj>;

struct test_obj {
    test_obj_id id;
    std::string value;
};

} // namespace

static const test_obj_id test_obj_id_0( "id_0" );
static const test_obj_id test_obj_id_1( "id_1" );
static const test_obj_id test_obj_id_2( "id_2" );
static const test_obj_id test_obj_non_existent_id( "non_existent_id" );

TEST_CASE( "generic_factory_insert_convert_valid", "[generic_factory]" )
{
    test_obj_id id_0( "id_0" );
    test_obj_id id_1( "id_1" );
    test_obj_id id_2( "id_2" );

    generic_factory<test_obj> test_factory( "test_factory" );

    REQUIRE_FALSE( test_factory.is_valid( id_0 ) );
    REQUIRE_FALSE( test_factory.is_valid( id_1 ) );
    REQUIRE_FALSE( test_factory.is_valid( id_2 ) );

    test_factory.insert( {test_obj_id_0, "value_0"} );
    test_factory.insert( {test_obj_id_1, "value_1"} );
    test_factory.insert( {test_obj_id_2, "value_2"} );

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

    CHECK_FALSE( test_factory.is_valid( test_obj_non_existent_id ) );

    int_id<test_obj> int_id_1 = test_factory.convert( test_obj_id_1,
                                int_id<test_obj>( -1 ) );
    CHECK( int_id_1.to_i() == 1 );

    CHECK( test_factory.convert( int_id_1 ) == id_1 );
    CHECK( test_factory.convert( int_id_1 ) == test_obj_id_1 );

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

    test_factory.insert( { test_obj_id_1, "1" } );
    CHECK( test_factory.is_valid( id_1 ) );
    CHECK( test_factory.obj( id_1 ).value == "1" );

    test_factory.insert( { test_obj_id_1, "2" } );
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
        test_factory.insert( { test_obj_id_1, value } );
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

namespace
{
struct dyn_test_obj {};
using dyn_str_id = string_id<dyn_test_obj>;

struct stat_test_obj {
    string_id<stat_test_obj> id;
    bool was_loaded = false;
};
using stat_str_id = string_id<stat_test_obj>;
using stat_int_id = int_id<stat_test_obj>;
int_id<stat_test_obj> stat_int_id_null( -1 );

generic_factory<stat_test_obj> stat_test_obj_factory( "stat_test_obj" );
} // namespace

// standard "generic_factory" methods to support the benchmark below
template<> int_id<stat_test_obj> string_id<stat_test_obj>::id() const
{
    return stat_test_obj_factory.convert( *this, stat_int_id_null );
}
template<> int_id<stat_test_obj>::int_id( const string_id<stat_test_obj> &id ) : _id( id.id() ) {}
template<> const stat_test_obj &string_id<stat_test_obj>::obj() const
{
    return stat_test_obj_factory.obj( *this );
}

// mark string_id<dyn_test_obj> as dynamic/non-interned
template<> struct string_id_params<dyn_test_obj> {
    static constexpr bool dynamic = true;
};

// compares the lookup performance of different flag container implementations
TEST_CASE( "string_and_int_ids_benchmark", "[.][generic_factory][int_id][string_id][benchmark]" )
{
    stat_test_obj_factory.reset();
    for( int i = 0; i < 1000; i ++ ) {
        stat_test_obj_factory.insert( {
            string_id<stat_test_obj>( "stat_test_id_" + std::to_string( i ) )
        } );
    }

    static constexpr int bit_flags_size = 100;
    static constexpr int bloom_size = 64;

    INFO( "bloom_size must be a power of two" );
    REQUIRE( __builtin_popcount( bloom_size ) == 1 );

    const unsigned int flags_in_item = GENERATE( 2, 8, 32, 128 );

    struct fake_item {
        int dummy;
        std::bitset<bit_flags_size> bit_flags;
        std::set<stat_str_id> std_set_string_ids;
        cata::flat_set<stat_str_id> flat_set_string_ids;
        std::unordered_set<stat_str_id> uo_set_string_ids;
        std::set<dyn_str_id> std_set_dyn_string_ids;
        cata::flat_set<dyn_str_id> flat_set_dyn_string_ids;
        std::unordered_set<dyn_str_id> uo_set_dyn_string_ids;
        std::bitset<bloom_size> bloom;
        std::set<stat_int_id> std_set_int_ids;
        std::unordered_set<stat_int_id> std_uo_set_int_ids;
        std::set<std::string> std_set_std_strings;
        cata::flat_set<std::string> flat_set_std_strings;
        cata::flat_set<stat_int_id> flat_set_int_ids;
        std::vector<stat_int_id> vector_int_ids;

        void set_flag( const stat_str_id &flag ) {
            const auto int_id = flag.id();
            const int i = int_id.to_i();
            bit_flags[i % bit_flags_size] = true;
            bloom[i % bloom_size] = true;
            flat_set_string_ids.insert( flag );
            std_set_string_ids.insert( flag );
            uo_set_string_ids.insert( flag );
            std_set_dyn_string_ids.insert( dyn_str_id( flag.str() ) );
            flat_set_dyn_string_ids.insert( dyn_str_id( flag.str() ) );
            uo_set_dyn_string_ids.insert( dyn_str_id( flag.str() ) );
            std_set_std_strings.emplace( flag.str() );
            flat_set_std_strings.insert( flag.str() );
            flat_set_int_ids.insert( int_id );
            std_set_int_ids.emplace( int_id );
            std_uo_set_int_ids.emplace( int_id );
            // do not add duplicates
            if( std_uo_set_int_ids.size() > vector_int_ids.size() ) {
                vector_int_ids.emplace_back( flag );
            }
        }
    } item;

    const auto &all_flags = stat_test_obj_factory.get_all();
    const int all_flags_n = static_cast<int>( all_flags.size() );
    REQUIRE( all_flags_n >= 10 );

    for( const auto &f : all_flags ) {
        if( xor_rand() % all_flags_n <= flags_in_item ) {
            item.set_flag( f.id );
        }
    }
    while( item.std_set_string_ids.size() < flags_in_item ) {
        item.set_flag( all_flags[xor_rand() % all_flags_n].id );
    }

    std::vector<stat_str_id> test_flags;
    bool hits = GENERATE( false, true );
    if( hits ) {
        // populate only with flags that exist in the item
        for( const auto &f : item.std_set_string_ids ) {
            test_flags.push_back( f );
        }
    } else {
        // populate with random flags
        for( int i = 0; i < 20; i++ ) {
            test_flags.push_back( all_flags[xor_rand() % all_flags_n].id );
        }
    }
    const int test_flags_size = test_flags.size();
    std::vector<dyn_str_id > test_dyn_str_ids;
    test_dyn_str_ids.reserve( test_flags.size() );
    for( const auto &f : test_flags ) {
        test_dyn_str_ids.emplace_back( f.str() );
    }

    DYNAMIC_SECTION( "number_ids_in_collection: " << flags_in_item << "; only_hits: " << hits ) {
    }

    int i = 0;

    BENCHMARK( "baseline: string_id access" ) {
        return test_flags[( i++ ) % test_flags_size].str().size();
    };

    BENCHMARK( "baseline: string_id::obj()" ) {
        return test_flags[0].obj().was_loaded;
    };

    BENCHMARK( "baseline: string_id -> int_id" ) {
        return test_flags[0].id().to_i();
    };

    BENCHMARK( "\"enum\" bitset" ) {
        return !!item.bit_flags[( i++ ) % bit_flags_size];
    };

    BENCHMARK( "std::set<std::string>" ) {
        return item.std_set_std_strings.count( test_flags[( i++ ) % test_flags_size].str() );
    };

    BENCHMARK( "flat_set<std::string>" ) {
        return item.flat_set_std_strings.count( test_flags[( i++ ) % test_flags_size].str() );
    };

    BENCHMARK( "flat_set<string_id>" ) {
        return item.flat_set_string_ids.count( test_flags[( i++ ) % test_flags_size] );
    };

    BENCHMARK( "std::set<string_id>" ) {
        return item.std_set_string_ids.count( test_flags[( i++ ) % test_flags_size] );
    };

    BENCHMARK( "std::unordered_set<string_id>" ) {
        return item.uo_set_string_ids.count( test_flags[( i++ ) % test_flags_size] );
    };

    BENCHMARK( "flat_set<dyn_string_id>" ) {
        return item.flat_set_dyn_string_ids.count( test_dyn_str_ids[( i++ ) % test_flags_size] );
    };

    BENCHMARK( "std::set<dyn_string_id>" ) {
        return item.std_set_dyn_string_ids.count( test_dyn_str_ids[( i++ ) % test_flags_size] );
    };

    BENCHMARK( "std::unordered_set<dyn_string_id>" ) {
        return item.uo_set_dyn_string_ids.count( test_dyn_str_ids[( i++ ) % test_flags_size] );
    };

    BENCHMARK( "flat_set<int_id>" ) {
        return item.flat_set_int_ids.count( test_flags[( i++ ) % test_flags_size] );
    };

    BENCHMARK( "std::set<int_id>" ) {
        return item.std_set_int_ids.count( test_flags[( i++ ) % test_flags_size] );
    };

    BENCHMARK( "std::unordered_set<int_id>" ) {
        return item.std_uo_set_int_ids.count( test_flags[( i++ ) % test_flags_size] );
    };

    BENCHMARK( "std::vector<int_id>" ) {
        const std::vector<stat_int_id> &v = item.vector_int_ids;
        return std::find( v.begin(), v.end(), test_flags[( i++ ) % test_flags_size] ) != v.end();
    };
}
