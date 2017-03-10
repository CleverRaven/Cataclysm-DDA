#include "catch/catch.hpp"
#include "translations.h"
#include <stdio.h>
#include <string.h>

void check_eq( std::string a, std::string b )
{
    CHECK( a == b );
}

// Note: we can't test positional arguments here yet: the behaviors differs depending on localization
TEST_CASE( "Test translate macro" ) {
    check_eq( _("%%hello%%"), "%%hello%%" );
    check_eq( _("hello"), "hello" );
    check_eq( _("%%"), "%%" );
    check_eq( _(""), "" );
    check_eq( _("%s"), "%s" );
}

#ifndef LOCALIZE

void check_strip( std::vector<char> &buffer, const std::string &input_str, const std::string &output_str )
{
    char *str = &buffer[0];
    strcpy( str, input_str.c_str() );
    check_eq( _( str ), output_str );
}

TEST_CASE( "Test positional argument stripping" ) {
    check_eq( _("%1$s"), "%s" );
    check_eq( _("%27s"), "%27s" );
    check_eq( _("%1$s"), "%s" );
    check_eq( _("%2$s"), "%s" );
    check_eq( _("%1$0.4f"), "%0.4f" );
    check_eq( _("%1$0.4f %2f"), "%0.4f %2f");
    check_eq( _("%1$0.4f %2$f"), "%0.4f %f");
    check_eq( _("%"), "%");
    check_eq( _("%1$s %1$s"), "%s %s");
    // We are intentionally reusing the same buffer here, so that the address of C string stays the same
    std::vector<char> buffer( 1024, '\0' );
    check_strip( buffer, "A text that lacks positional arguments.", "A text that lacks positional arguments." );
    check_strip( buffer, "%1$s %2$s %3$s", "%s %s %s" );
    check_strip( buffer, "%3$s %2$s %1$s", "%s %s %s" );
}

#endif
