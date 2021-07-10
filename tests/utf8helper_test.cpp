#include "catch/catch.hpp"
#include "utf8helper.h"

static const utf8helper utf8;

TEST_CASE( "utf8 compare", "[utf8helper]" )
{
    // Latin ASCII
    std::string A = "A";
    std::string a = "a";
    std::string B = "B";
    std::string b = "b";
    std::string Ab = "Ab";
    std::string aB = "aB";
    SECTION( "compare ASCII" ) {
        SECTION( "compare ASCII case insensitive" ) {
            CHECK( !utf8( a, a ) );
            CHECK( !utf8( a, A ) );
            CHECK( !utf8( A, a ) );
            CHECK( utf8( a, B ) );
            CHECK( utf8( A, b ) );
            CHECK( !utf8( B, a ) );
            CHECK( !utf8( b, A ) );
            CHECK( !utf8( Ab, aB ) );
            CHECK( utf8( A, Ab ) );
            CHECK( !utf8( Ab, a ) );
        }
        SECTION( "compare ASCII case sensitive" ) {
            CHECK( !utf8( a, a, true ) );
            CHECK( !utf8( a, A, true ) );
            CHECK( utf8( A, a, true ) );
            CHECK( utf8( a, B, true ) );
            CHECK( utf8( A, b, true ) );
            CHECK( !utf8( B, a, true ) );
            CHECK( !utf8( b, A, true ) );
            CHECK( utf8( Ab, aB, true ) );
            CHECK( utf8( A, Ab, true ) );
            CHECK( utf8( Ab, a, true ) );
        }
    }
    // Cyrillic
    std::string Cyrillic_A = "А";
    std::string Cyrillic_a = "а";
    std::string Cyrillic_B = "Б";
    std::string Cyrillic_b = "б";
    std::string Cyrillic_Ab = "Аб";
    std::string Cyrillic_aB = "аБ";
    SECTION( "compare Cyrillic" ) {
        SECTION( "compare Cyrillic case insensitive" ) {
            CHECK( !utf8( Cyrillic_a, Cyrillic_a ) );
            CHECK( !utf8( Cyrillic_a, Cyrillic_A ) );
            CHECK( !utf8( Cyrillic_A, Cyrillic_a ) );
            CHECK( utf8( Cyrillic_a, Cyrillic_B ) );
            CHECK( utf8( Cyrillic_A, Cyrillic_b ) );
            CHECK( !utf8( Cyrillic_B, Cyrillic_a ) );
            CHECK( !utf8( Cyrillic_b, Cyrillic_A ) );
            CHECK( !utf8( Cyrillic_Ab, Cyrillic_aB ) );
            CHECK( utf8( Cyrillic_A, Cyrillic_Ab ) );
            CHECK( !utf8( Cyrillic_Ab, Cyrillic_a ) );
        }
        SECTION( "compare Cyrillic case sensitive" ) {
            CHECK( !utf8( Cyrillic_a, Cyrillic_a, true ) );
            CHECK( !utf8( Cyrillic_a, Cyrillic_A, true ) );
            CHECK( utf8( Cyrillic_A, Cyrillic_a, true ) );
            CHECK( utf8( Cyrillic_a, Cyrillic_B, true ) );
            CHECK( utf8( Cyrillic_A, Cyrillic_b, true ) );
            CHECK( !utf8( Cyrillic_B, Cyrillic_a, true ) );
            CHECK( !utf8( Cyrillic_b, Cyrillic_A, true ) );
            CHECK( utf8( Cyrillic_Ab, Cyrillic_aB, true ) );
            CHECK( utf8( Cyrillic_A, Cyrillic_Ab, true ) );
            CHECK( utf8( Cyrillic_Ab, Cyrillic_a, true ) );
        }
    }
    SECTION( "compare ASCII and Cyrillic" ) {
        CHECK( utf8( b, Cyrillic_A ) );
        CHECK( !utf8( Cyrillic_A, aB ) );
        CHECK( utf8( b, Cyrillic_A, true ) );
        CHECK( !utf8( Cyrillic_A, aB, true ) );
    }
    // Extended Latin
    std::string _A = "À";
    std::string _a = "à";
    std::string A_ = "Ą";
    std::string a_ = "ą";
    std::string _C = "Ć";
    std::string _c = "ć";
    SECTION( "compare Extended Latin" ) {
        SECTION( "compare Extended Latin case insensitive" ) {
            CHECK( !utf8( _a, _a ) );
            CHECK( !utf8( _a, _A ) );
            CHECK( !utf8( _A, _a ) );
            CHECK( utf8( _a, A_ ) );
            CHECK( utf8( a_, _C ) );
            CHECK( !utf8( _C, a_ ) );
        }
        SECTION( "compare Extended Latin case sensitive" ) {
            CHECK( !utf8( _a, _a, true ) );
            CHECK( !utf8( _a, _A, true ) );
            CHECK( utf8( _A, _a, true ) );
            CHECK( utf8( _a, A_, true ) );
            CHECK( utf8( a_, _C, true ) );
            CHECK( !utf8( _C, a_, true ) );
        }
    }
    SECTION( "compare ASCII and Extended Latin" ) {
        CHECK( utf8( a, _a ) );
        CHECK( utf8( a, _A ) );
        CHECK( utf8( a_, b ) );
        CHECK( utf8( a_, B ) );
        CHECK( utf8( b, _C ) );
        CHECK( utf8( a, _a, true ) );
        CHECK( utf8( a, _A, true ) );
        CHECK( utf8( a_, b, true ) );
        CHECK( utf8( a_, B, true ) );
        CHECK( utf8( b, _C, true ) );
    }
    SECTION( "compare Cyrillic and Extended Latin" ) {
        CHECK( utf8( _c, Cyrillic_A ) );
        CHECK( !utf8( Cyrillic_A, _c ) );
        CHECK( utf8( _c, Cyrillic_A, true ) );
        CHECK( !utf8( Cyrillic_A, _c, true ) );
    }
}

TEST_CASE( "utf8 convert", "[utf8helper]" )
{
    std::string A = "A";
    std::string a = "a";
    std::string Cyrillic_A = "А";
    std::string Cyrillic_a = "а";
    std::string A_ = "Ą";
    std::string a_ = "ą";
    std::string Yo = "Ë";
    std::string yo = "ë";
    // Warning: MacOS (<=10.15.6) has bug: Latin Ë instead Cyrillic Ё in Russian keyboard layout
    // Don't type Russian Ё from Mac keyboard!
    std::string Cyrillic_Yo = "Ё";
    std::string Cyrillic_yo = "ё";
    REQUIRE( Cyrillic_Yo != Yo );
    REQUIRE( Cyrillic_yo != yo );
    SECTION( "to_lower_case" ) {
        CHECK( utf8.to_lower_case( A ) == a );
        CHECK( utf8.to_lower_case( Cyrillic_A ) == Cyrillic_a );
        CHECK( utf8.to_lower_case( A_ ) == a_ );
        CHECK( utf8.to_lower_case( Yo ) == yo );
        CHECK( utf8.to_lower_case( Cyrillic_Yo ) == Cyrillic_yo );
    }
    SECTION( "to_upper_case" ) {
        CHECK( utf8.to_upper_case( a ) == A );
        CHECK( utf8.to_upper_case( Cyrillic_a ) == Cyrillic_A );
        CHECK( utf8.to_upper_case( a_ ) == A_ );
        CHECK( utf8.to_upper_case( yo ) == Yo );
        CHECK( utf8.to_upper_case( Cyrillic_yo ) == Cyrillic_Yo );
    }
    SECTION( "for_search" ) {
        std::string Cyrillic_E = "Е";
        std::string Cyrillic_e = "е";
        CHECK( utf8.for_search( A ) == a );
        CHECK( utf8.for_search( Cyrillic_A ) == Cyrillic_a );
        CHECK( utf8.for_search( A_ ) == a_ );
        CHECK( utf8.for_search( Yo ) == yo );
        CHECK( utf8.for_search( Cyrillic_E ) == Cyrillic_e );
        CHECK( utf8.for_search( Cyrillic_e ) == Cyrillic_e );
        CHECK( utf8.for_search( Cyrillic_Yo ) == Cyrillic_e );
        CHECK( utf8.for_search( Cyrillic_yo ) == Cyrillic_e );
    }
}
