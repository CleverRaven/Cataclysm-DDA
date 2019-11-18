#include "json.h"

#include <list>
#include <sstream>

#include "bodypart.h"
#include "catch/catch.hpp"
#include "type_id.h"

template<typename T>
void test_serialization( const T &val, const std::string &s )
{
    CAPTURE( val );
    {
        INFO( "test_serialization" );
        std::ostringstream os;
        JsonOut jsout( os );
        jsout.write( val );
        CHECK( os.str() == s );
    }
    {
        INFO( "test_deserialization" );
        std::istringstream is( s );
        JsonIn jsin( is );
        T read_val;
        CHECK( jsin.read( read_val ) );
        CHECK( val == read_val );
    }
}

TEST_CASE( "serialize_colony", "[json]" )
{
    cata::colony<std::string> c = { "foo", "bar" };
    test_serialization( c, R"(["foo","bar"])" );
}

TEST_CASE( "serialize_map", "[json]" )
{
    std::map<std::string, std::string> s_map = { { "foo", "foo_val" }, { "bar", "bar_val" } };
    test_serialization( s_map, R"({"bar":"bar_val","foo":"foo_val"})" );
    std::map<mtype_id, std::string> string_id_map = { { mtype_id( "foo" ), "foo_val" } };
    test_serialization( string_id_map, R"({"foo":"foo_val"})" );
    std::map<body_part, std::string> enum_map = { { bp_foot_l, "foo_val" } };
    test_serialization( enum_map, R"({"FOOT_L":"foo_val"})" );
}

TEST_CASE( "serialize_pair", "[json]" )
{
    std::pair<std::string, int> p = { "foo", 42 };
    test_serialization( p, R"(["foo",42])" );
}

TEST_CASE( "serialize_sequences", "[json]" )
{
    std::vector<std::string> v = { "foo", "bar" };
    test_serialization( v, R"(["foo","bar"])" );
    std::array<std::string, 2> a = {{ "foo", "bar" }};
    test_serialization( a, R"(["foo","bar"])" );
    std::list<std::string> l = { "foo", "bar" };
    test_serialization( l, R"(["foo","bar"])" );
}

TEST_CASE( "serialize_set", "[json]" )
{
    std::set<std::string> s_set = { "foo", "bar" };
    test_serialization( s_set, R"(["bar","foo"])" );
    std::set<mtype_id> string_id_set = { mtype_id( "foo" ) };
    test_serialization( string_id_set, R"(["foo"])" );
    std::set<body_part> enum_set = { bp_foot_l };
    test_serialization( enum_set, string_format( R"([%d])", static_cast<int>( bp_foot_l ) ) );
}
