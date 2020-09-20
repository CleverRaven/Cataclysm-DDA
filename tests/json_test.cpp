#include "catch/catch.hpp"
#include "json.h"

#include <array>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "mutation.h"
#include "colony.h"
#include "string_formatter.h"
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

TEST_CASE( "avoid_serializing_default_values", "[json]" )
{
    std::ostringstream os;
    JsonOut jsout( os );
    jsout.member( "foo", "foo", "foo" );
    jsout.member( "bar", "foo", "bar" );
    REQUIRE( os.str() == "\"bar\":\"foo\"" );
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
    std::map<trigger_type, std::string> enum_map = { { HUNGER, "foo_val" } };
    test_serialization( enum_map, R"({"HUNGER":"foo_val"})" );
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
    std::set<trigger_type> enum_set = { HUNGER };
    test_serialization( enum_set, string_format( R"([%d])", static_cast<int>( HUNGER ) ) );
}

template<typename Matcher>
static void test_translation_text_style_check( Matcher &&matcher, const std::string &json )
{
    std::istringstream iss( json );
    JsonIn jsin( iss );
    translation trans;
    const std::string dmsg = capture_debugmsg_during( [&]() {
        jsin.read( trans );
    } );
    CHECK_THAT( dmsg, matcher );
}

TEST_CASE( "translation_text_style_check", "[json][translation]" )
{
    // this test case is mainly for checking the caret position.
    // the text style check itself is tested in the lit test of clang-tidy.

    // string, ascii
    test_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:5: insufficient spaces at this location.  2 required, but only 1 found.)"
            "\n"
            R"(    Suggested fix: insert " ")" "\n"
            R"(    At the following position (marked with caret))" "\n"
            R"()" "\n"
            R"("foo.)" "\n"
            R"(    ^)" "\n"
            R"(      bar.")" "\n" ),
        R"("foo. bar.")" ); // NOLINT(cata-text-style)
    // string, unicode
    test_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:8: insufficient spaces at this location.  2 required, but only 1 found.)"
            "\n"
            R"(    Suggested fix: insert " ")" "\n"
            R"(    At the following position (marked with caret))" "\n"
            R"()" "\n"
            R"("…foo.)" "\n"
            R"(       ^)" "\n"
            R"(         bar.")" "\n" ),
        R"("…foo. bar.")" ); // NOLINT(cata-text-style)
    // string, escape sequence
    test_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:11: insufficient spaces at this location.  2 required, but only 1 found.)"
            "\n"
            R"(    Suggested fix: insert " ")" "\n"
            R"(    At the following position (marked with caret))" "\n"
            R"()" "\n"
            R"("\u2026foo.)" "\n"
            R"(          ^)" "\n"
            R"(            bar.")" "\n" ),
        R"("\u2026foo. bar.")" ); // NOLINT(cata-text-style)
    // object, ascii
    test_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:13: insufficient spaces at this location.  2 required, but only 1 found.)"
            "\n"
            R"(    Suggested fix: insert " ")" "\n"
            R"(    At the following position (marked with caret))" "\n"
            R"()" "\n"
            R"({"str": "foo.)" "\n"
            R"(            ^)" "\n"
            R"(              bar."})" "\n" ),
        R"({"str": "foo. bar."})" ); // NOLINT(cata-text-style)
    // object, unicode
    test_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:16: insufficient spaces at this location.  2 required, but only 1 found.)"
            "\n"
            R"(    Suggested fix: insert " ")" "\n"
            R"(    At the following position (marked with caret))" "\n"
            R"()" "\n"
            R"({"str": "…foo.)" "\n"
            R"(               ^)" "\n"
            R"(                 bar."})" "\n" ),
        R"({"str": "…foo. bar."})" ); // NOLINT(cata-text-style)
    // object, escape sequence
    test_translation_text_style_check(
        Catch::Equals(
            R"((json-error))" "\n"
            R"(Json error: <unknown source file>:1:19: insufficient spaces at this location.  2 required, but only 1 found.)"
            "\n"
            R"(    Suggested fix: insert " ")" "\n"
            R"(    At the following position (marked with caret))" "\n"
            R"()" "\n"
            R"({"str": "\u2026foo.)" "\n"
            R"(                  ^)" "\n"
            R"(                    bar."})" "\n" ),
        R"({"str": "\u2026foo. bar."})" ); // NOLINT(cata-text-style)
}

static void test_get_string( const std::string &str, const std::string &json )
{
    CAPTURE( json );
    std::istringstream iss( json );
    JsonIn jsin( iss );
    CHECK( jsin.get_string() == str );
}

template<typename Matcher>
static void test_get_string_throws_matches( Matcher &&matcher, const std::string &json )
{
    CAPTURE( json );
    std::istringstream iss( json );
    JsonIn jsin( iss );
    CHECK_THROWS_MATCHES( jsin.get_string(), JsonError, matcher );
}

template<typename Matcher>
static void test_string_error_throws_matches( Matcher &&matcher, const std::string &json,
        const int offset )
{
    CAPTURE( json );
    CAPTURE( offset );
    std::istringstream iss( json );
    JsonIn jsin( iss );
    CHECK_THROWS_MATCHES( jsin.string_error( "<message>", offset ), JsonError, matcher );
}

TEST_CASE( "jsonin_get_string", "[json]" )
{
    // read plain text
    test_get_string( "foo", R"("foo")" );
    // ignore starting spaces
    test_get_string( "bar", R"(  "bar")" );
    // read unicode characters
    test_get_string( "……", R"("……")" );
    test_get_string( "……", "\"\u2026\u2026\"" );
    test_get_string( "\xe2\x80\xa6", R"("…")" );
    test_get_string( "\u00A0", R"("\u00A0")" );
    test_get_string( "\u00A0", R"("\u00a0")" );
    // read escaped unicode
    test_get_string( "…", R"("\u2026")" );
    // read utf8 sequence
    test_get_string( "…", "\"\xe2\x80\xa6\"" );
    // read newline
    test_get_string( "a\nb\nc", R"("a\nb\nc")" );
    // read slash
    test_get_string( "foo\\bar", R"("foo\\bar")" );
    // read escaped characters
    // NOLINTNEXTLINE(cata-text-style)
    test_get_string( "\"\\/\b\f\n\r\t\u2581", R"("\"\\\/\b\f\n\r\t\u2581")" );

    // empty json
    test_get_string_throws_matches(
        Catch::Message(
            "Json error: <unknown source file>:EOF: couldn't find end of string, reached EOF." ),
        std::string() );
    // no starting quote
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:1: expected string but got 'a')" "\n"
            R"()" "\n"
            R"(a)" "\n"
            R"(^)" "\n"
            R"( bc)" "\n" ),
        R"(abc)" );
    // no ending quote
    test_get_string_throws_matches(
        Catch::Message(
            "Json error: <unknown source file>:EOF: couldn't find end of string, reached EOF." ),
        R"(")" );
    test_get_string_throws_matches(
        Catch::Message(
            "Json error: <unknown source file>:EOF: couldn't find end of string, reached EOF." ),
        R"("foo)" );
    // incomplete escape sequence and no ending quote
    test_get_string_throws_matches(
        Catch::Message(
            "Json error: <unknown source file>:EOF: couldn't find end of string, reached EOF." ),
        R"("\)" );
    test_get_string_throws_matches(
        Catch::Message(
            "Json error: <unknown source file>:EOF: couldn't find end of string, reached EOF." ),
        R"("\u12)" );
    // incorrect escape sequence
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:3: invalid escape sequence)" "\n"
            R"()" "\n"
            R"("\.)" "\n"
            R"(  ^)" "\n"
            R"(   ")" "\n" ),
        R"("\.")" );
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:7: expected hex digit)" "\n"
            R"()" "\n"
            R"("\uDEFG)" "\n"
            R"(      ^)" "\n"
            R"(       ")" "\n" ),
        R"("\uDEFG")" );
    // not a valid utf8 sequence
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:2: invalid utf8 sequence)" "\n"
            R"()" "\n"
            "\"\x80\n"
            R"( ^)" "\n"
            R"(  ")" "\n" ),
        "\"\x80\"" );
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:4: invalid utf8 sequence)" "\n"
            R"()" "\n"
            "\"\xFC\x80\"\n"
            R"(   ^)" "\n" ),
        "\"\xFC\x80\"" );
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:7: invalid unicode codepoint)" "\n"
            R"()" "\n"
            "\"\xFD\x80\x80\x80\x80\x80\n"
            R"(      ^)" "\n"
            R"(       ")" "\n" ),
        "\"\xFD\x80\x80\x80\x80\x80\"" );
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:7: invalid utf8 sequence)" "\n"
            R"()" "\n"
            "\"\xFC\x80\x80\x80\x80\xC0\n"
            R"(      ^)" "\n"
            R"(       ")" "\n" ),
        "\"\xFC\x80\x80\x80\x80\xC0\"" );
    // end of line
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:3: reached end of line without closing string)" "\n"
            R"()" "\n"
            R"("a)" "\n"
            R"(  ^)" "\n"
            R"(")" "\n" ),
        "\"a\n\"" );
    test_get_string_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:3: reached end of line without closing string)" "\n"
            R"()" "\n"
            R"("b)" "\n"
            R"(  ^)" "\n"
            R"(")" "\n" ),
        "\"b\r\"" ); // NOLINT(cata-text-style)

    // test throwing error after the given number of unicode characters
    // ascii
    test_string_error_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:1: <message>)" "\n"
            R"()" "\n"
            R"(")" "\n"
            R"(^)" "\n"
            R"( foobar")" "\n" ),
        R"("foobar")", 0 );
    test_string_error_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:4: <message>)" "\n"
            R"()" "\n"
            R"("foo)" "\n"
            R"(   ^)" "\n"
            R"(    bar")" "\n" ),
        R"("foobar")", 3 );
    // unicode
    test_string_error_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:4: <message>)" "\n"
            R"()" "\n"
            R"("foo)" "\n"
            R"(   ^)" "\n"
            R"(    …bar1")" "\n" ),
        R"("foo…bar1")", 3 );
    test_string_error_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:7: <message>)" "\n"
            R"()" "\n"
            R"("foo…)" "\n"
            R"(      ^)" "\n"
            R"(       bar2")" "\n" ),
        R"("foo…bar2")", 4 );
    test_string_error_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:8: <message>)" "\n"
            R"()" "\n"
            R"("foo…b)" "\n"
            R"(       ^)" "\n"
            R"(        ar3")" "\n" ),
        R"("foo…bar3")", 5 );
    // escape sequence
    test_string_error_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:11: <message>)" "\n"
            R"()" "\n"
            R"("foo\u2026b)" "\n"
            R"(          ^)" "\n"
            R"(           ar")" "\n" ),
        R"("foo\u2026bar")", 5 );
    test_string_error_throws_matches(
        Catch::Message(
            R"(Json error: <unknown source file>:1:7: <message>)" "\n"
            R"()" "\n"
            R"("foo\nb)" "\n"
            R"(      ^)" "\n"
            R"(       ar")" "\n" ),
        R"("foo\nbar")", 5 );
}
