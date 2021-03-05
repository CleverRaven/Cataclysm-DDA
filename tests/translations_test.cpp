#include "catch/catch.hpp"

#include "language.h"
#include "translations.h"

// wrapping in another macro to prevent collection of the test string for translation
#ifndef TRANSLATE_MACRO
#define TRANSLATE_MACRO(msg) _( msg )
#endif

// wrapping in another macro to prevent collection of the test string for translation
#ifndef TRANSLATE_TRANSLATION
#define TRANSLATE_TRANSLATION(msg) to_translation( msg ).translated()
#endif

TEST_CASE( "translations_sanity_test", "[translations]" )
{
    const std::string test_string = "__untranslated_test_string__";

    CHECK( TRANSLATE_MACRO( test_string ) == test_string );
    CHECK( TRANSLATE_TRANSLATION( test_string ) == test_string );
}

// assuming [en] language is used for this test
// test should succeed both with and without the LOCALIZE
TEST_CASE( "translations_macro_string_stability", "[translations]" )
{
    std::vector<std::string> test_strings;

    SECTION( "untranslated strings" ) {
        test_strings = { "__untranslated_string1__", "__untranslated_string2__" };
    }

    SECTION( "translated strings" ) {
        test_strings = { "thread", "yarn" };
    }

    std::string test_arg;
    //note: this TRANSLATE_MACRO called in different iterations of the `for` loop will have
    // the same static cache
    for( const std::string &test_string : test_strings ) {
        test_arg.assign( test_string );
        // translation result should be assignable to the `const std::string &` local variable
        const std::string &res1 = TRANSLATE_MACRO( test_arg );
        CHECK( res1 == test_string );
    }
}

// assuming [en] language is used for this test
// test should succeed both with and without the LOCALIZE
TEST_CASE( "translations_macro_char_address", "[translations]" )
{
    SECTION( "address should be same when translation is absent" ) {
        const char *test_string = "__untranslated_string1__";
        CHECK( TRANSLATE_MACRO( test_string ) == test_string );
    }

    SECTION( "current address of the arg should be returned when translation is absent" ) {
        // wrapped in lambda to ensure that different calls hit the same internal cache
        const auto translate = []( const char *msg ) {
            return TRANSLATE_MACRO( msg );
        };

        // same content, different address (std::string is used to ensure that)
        const std::string test_string1 = "__untranslated_string1__";
        const std::string test_string2 = "__untranslated_string1__";
        // address should be different!
        REQUIRE( test_string1.c_str() != test_string2.c_str() );
        // note: address comparison is intended
        CHECK( translate( test_string1.c_str() ) == test_string1.c_str() );
        CHECK( translate( test_string2.c_str() ) == test_string2.c_str() );
        CHECK( translate( test_string1.c_str() ) == test_string1.c_str() );
    }
}

#ifdef LOCALIZE
// this test will only succeed when LOCALIZE is enabled
// assuming [en] language is used for this test
// requires .mo file for "en" language
TEST_CASE( "translations_macro_char_address_translated", "[.][translations]" )
{
    set_language();
    // translated string
    const char *test_string = "thread";

    // should return a stable address of translation that is different from the argument
    CHECK( TRANSLATE_MACRO( test_string ) != test_string );
}
#endif
