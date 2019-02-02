#if !defined(_MSC_VER)
// the rewrite_vsnprintf function is explicitly defined for non-MS compilers in output.cpp

#include "catch/catch.hpp"

std::string rewrite_vsnprintf( const char *msg );

TEST_CASE( "Test vsnprintf_rewrite" )
{
    CHECK( rewrite_vsnprintf( "%%hello%%" ) == "%%hello%%" );
    CHECK( rewrite_vsnprintf( "hello" ) == "hello" );
    CHECK( rewrite_vsnprintf( "%%" ) == "%%" );
    CHECK( rewrite_vsnprintf( "" ) == "" );
    CHECK( rewrite_vsnprintf( "%s" ) == "%s" );
    CHECK( rewrite_vsnprintf( "%1s" ) == "%1s" );
    CHECK( rewrite_vsnprintf( "%27s" ) == "%27s" );
    CHECK( rewrite_vsnprintf( "%1$s" ) == "%s" );
    CHECK( rewrite_vsnprintf( "%2$s" ) == "%2$s" );
    CHECK( rewrite_vsnprintf( "%1$0.4f" ) == "%0.4f" );
    CHECK( rewrite_vsnprintf( "%1$0.4f %2f" ) == "%1$0.4f %2f" );
    CHECK( rewrite_vsnprintf( "%1$0.4f %2$f" ) == "%0.4f %f" );
    CHECK( rewrite_vsnprintf( "%" ) == "%" );
    CHECK( rewrite_vsnprintf( "%1$s %1$s" ) == "%1$s %1$s" );
    CHECK( rewrite_vsnprintf( "%2$s %1$s %3$s %4$s %5$s %6$s %7$s %8$s %9$4s %10$40s %11$40s" ) ==
           "%2$s %1$s %3$s %4$s %5$s %6$s %7$s %8$s %9$4s <formatting error> <formatting error>" );

    CHECK( rewrite_vsnprintf( "%1$s %2$s %3$s %4$s %5$s %6$s %7$s %8$s %9$4s %10$40s %11$40s" ) ==
           "%s %s %s %s %s %s %s %s %4s %40s %40s" );

    CHECK( rewrite_vsnprintf( "Needs <color_%1$s>%2$s</color>, a <color_%3$s>wrench</color>, "
                              "either a <color_%4$s>powered welder</color> "
                              "(and <color_%5$s>welding goggles</color>) or "
                              "<color_%6$s>duct tape</color>, and level <color_%7$s>%8$d</color> "
                              "skill in mechanics.%9$s%10$s" )  ==
           "Needs <color_%s>%s</color>, a <color_%s>wrench</color>, either a "
           "<color_%s>powered welder</color> (and <color_%s>welding goggles</color>) or "
           "<color_%s>duct tape</color>, and level <color_%s>%d</color> skill in mechanics.%s%s" );
}

#endif
