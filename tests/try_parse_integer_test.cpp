#include "cata_catch.h"
#include "try_parse_integer.h"

TEMPLATE_TEST_CASE( "try_parse_int_simple_parsing", "[try_parse_integer]", int, long, long long )
{
    try {
        std::locale::global( std::locale( "en_US.UTF-8" ) );
    } catch( std::runtime_error & ) {
        // On platforms where we can't set the locale, the test should still
        // pass
    }
    CAPTURE( setlocale( LC_ALL, nullptr ) );
    CAPTURE( std::locale().name() );

    bool use_locale = GENERATE( false, true );
    CAPTURE( use_locale );

    SECTION( "successes" ) {
        {
            ret_val<TestType> result = try_parse_integer<TestType>( "1234", use_locale );
            CAPTURE( result.str() );
            CHECK( result.success() );
            CHECK( result.value() == 1234 );
        }
        {
            ret_val<TestType> result = try_parse_integer<TestType>( "-1234", use_locale );
            CAPTURE( result.str() );
            CHECK( result.success() );
            CHECK( result.value() == -1234 );
        }
        {
            ret_val<TestType> result = try_parse_integer<TestType>( "+1234", use_locale );
            CAPTURE( result.str() );
            CHECK( result.success() );
            CHECK( result.value() == +1234 );
        }
        {
            ret_val<TestType> result = try_parse_integer<TestType>( "  1234", use_locale );
            CAPTURE( result.str() );
            CHECK( result.success() );
            CHECK( result.value() == 1234 );
        }
    }

    SECTION( "errors" ) {
        {
            ret_val<TestType> result = try_parse_integer<TestType>( "", use_locale );
            CHECK( !result.success() );
            CHECK( result.str() == "Could not convert '' to an integer" );
        }
        {
            ret_val<TestType> result = try_parse_integer<TestType>( "a", use_locale );
            CHECK( !result.success() );
            CHECK( result.str() == "Could not convert 'a' to an integer" );
        }
        {
            // Verify that we detect overflow
            ret_val<TestType> result =
                try_parse_integer<TestType>( "999999999999999999999", use_locale );
            CHECK( !result.success() );
            CHECK( result.str() == "Could not convert '999999999999999999999' to an integer" );
        }
    }
}

TEMPLATE_TEST_CASE( "try_parse_int_locale_parsing", "[try_parse_integer]", int, long, long long )
{
    SECTION( "de_DE" ) {
        try {
            std::locale::global( std::locale( "de_DE.UTF-8" ) );
        } catch( std::runtime_error & ) {
            // On platforms where we can't set the locale, ignore this test
            return;
        }
        CAPTURE( setlocale( LC_ALL, nullptr ) );
        CAPTURE( std::locale().name() );
        // Disabling on Apple; seems like their C library doesn't do localized integer
        // parsing.
#ifndef __APPLE__
        {
            ret_val<TestType> result = try_parse_integer<TestType>( "1.234", true );
            CHECK( result.success() );
            CHECK( result.value() == 1234 );
        }
#endif
        {
            ret_val<TestType> result = try_parse_integer<TestType>( "1.234", false );
            CHECK( !result.success() );
            CHECK( result.str() == "Stray characters after integer in '1.234'" );
        }
    }

    SECTION( "en_US" ) {
        try {
            std::locale::global( std::locale( "en_US.UTF-8" ) );
        } catch( std::runtime_error & ) {
            // On platforms where we can't set the locale, ignore this test
            return;
        }
        CAPTURE( setlocale( LC_ALL, nullptr ) );
        CAPTURE( std::locale().name() );
#ifndef __APPLE__
        {
            ret_val<TestType> result = try_parse_integer<TestType>( "1,234", true );
            CHECK( result.success() );
            CHECK( result.value() == 1234 );
        }
#endif
        {
            ret_val<TestType> result = try_parse_integer<TestType>( "1,234", false );
            CHECK( !result.success() );
            CHECK( result.str() == "Stray characters after integer in '1,234'" );
        }
    }
}
