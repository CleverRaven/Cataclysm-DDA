#include <limits>

#include "catch/catch.hpp"
#include "output.h"
#include "string_formatter.h"

// Same as @ref string_format, but does not swallow errors and throws them instead.
template<typename ...Args>
std::string throwing_string_format( const char *const format, Args &&...args )
{
    cata::string_formatter formatter( format );
    formatter.parse( std::forward<Args>( args )... );
    return formatter.get_output();
}

template<typename ...Args>
void importet_test( const int serial, const char *const expected, const char *const format,
                    Args &&... args )
{
    CAPTURE( serial );
    CAPTURE( format );

    const std::string original_result = cata::string_formatter::raw_string_format( format,
                                        std::forward<Args>( args )... );
    const std::string new_result = throwing_string_format( format, std::forward<Args>( args )... );

    // The expected string *is* what the raw printf would return.
    CHECK( original_result == expected );
    CHECK( original_result == new_result );
}

template<typename ...Args>
void test_for_expected( const std::string &expected, const char *const format, Args &&... args )
{
    CAPTURE( format );

    const std::string result = throwing_string_format( format, std::forward<Args>( args )... );
    CHECK( result == expected );
}

template<typename ...Args>
void test_for_error( const char *const format, Args &&... args )
{
    CAPTURE( format );

    const std::string result = throwing_string_format( format, std::forward<Args>( args )... );
    CAPTURE( result );
}
// Test whether raw printf and string_formatter return the same string (using
// old_pattern in both). Test whether the new pattern (if any) yields the same
// output via string_formatter (not used on raw printf).
template<typename ...Args>
void test_new_old_pattern( const char *const old_pattern, const char *const new_pattern,
                           Args &&...args )
{
    CAPTURE( old_pattern );
    CAPTURE( new_pattern );

    std::string original_result = cata::string_formatter::raw_string_format( old_pattern,
                                  std::forward<Args>( args )... );
    std::string old_result = throwing_string_format( old_pattern, std::forward<Args>( args )... );
    CHECK( original_result == old_result );

    if( new_pattern ) {
        std::string new_result = throwing_string_format( new_pattern, std::forward<Args>( args )... );
        CHECK( original_result == new_result );
    }
}
// Test that supplying `T&&`, `T&` and `const T&` as arguments will work the same.
template<typename T>
void test_lvalues( const std::string &expected, const char *const pattern, const T &value )
{
    test_for_expected( expected, pattern, T( value ) ); // T &&
    T lvalue( value );
    test_for_expected( expected, pattern, lvalue ); // T &
    const T const_lvalue( value );
    test_for_expected( expected, pattern, const_lvalue ); // const T &
}
// Test whether formatting min and max values of a given type works. Uses old_pattern
// for raw printf (which is assumed to match type) and new_pattern for use with
// string_formatter, which may be different (e.g. "%lli" requires a long long int
// in raw printf, but string_formatter will accept an int as well.
template<typename T>
void test_typed_printf( const char *const old_pattern, const char *const new_pattern )
{
    test_new_old_pattern( old_pattern, new_pattern, std::numeric_limits<T>::min() );
    test_new_old_pattern( old_pattern, new_pattern, std::numeric_limits<T>::max() );
}

template<typename T>
void mingw_test( const char *const old_pattern, const char *const new_pattern, const T &value )
{
    CAPTURE( old_pattern );
    CAPTURE( new_pattern );
    std::string original_result = cata::string_formatter::raw_string_format( old_pattern, value );
    std::string new_result = throwing_string_format( new_pattern, value );
    CHECK( original_result == new_result );
}

TEST_CASE( "string_formatter" )
{
    test_typed_printf<signed char>( "%hhi", "%i" );
    test_typed_printf<unsigned char>( "%hhu", "%u" );

    test_typed_printf<signed short int>( "%hi", "%i" );
    test_typed_printf<unsigned short int>( "%hu", "%u" );

    test_typed_printf<signed int>( "%i", nullptr );
    test_typed_printf<unsigned int>( "%u", nullptr );


#if !defined(__USE_MINGW_ANSI_STDIO) && (defined(__MINGW32__) || defined(__MINGW64__))
    mingw_test( "%I32i", "%i", std::numeric_limits<signed long int>::max() );
    mingw_test( "%I32u", "%u", std::numeric_limits<unsigned long int>::max() );
#else
    test_typed_printf<signed long int>( "%li", "%i" );
    test_typed_printf<unsigned long int>( "%lu", "%u" );
#endif

#if !defined(__USE_MINGW_ANSI_STDIO) && (!defined(__MINGW32__) && defined(__MINGW64__))
    mingw_test( "%I64i", "%i", std::numeric_limits<signed long long int>::max() );
    mingw_test( "%I64u", "%u", std::numeric_limits<unsigned long long int>::max() );
#else
    test_typed_printf<signed long long int>( "%lli", "%i" );
    test_typed_printf<unsigned long long int>( "%llu", "%u" );
#endif

    test_typed_printf<float>( "%f", "%f" );
    test_typed_printf<double>( "%f", "%f" );

    // format string with width and precision
    test_new_old_pattern( "%-*.*f", nullptr, 4, 7, 100.44 );

    // sprintf of some systems doesn't support the 'N$' syntax, if it's
    // not supported, the result is either empty, or the input string
    if( cata::string_formatter::raw_string_format( "%2$s||%1$s", "", "" ) == "||" ) {
        test_new_old_pattern( "%6$-*5$.*4$f%3$s%2$s%1$s", "%6$-*5$.*4$f", "", "", "", 7, 4, 100.44 );
    }
    CHECK_THROWS( test_for_error( "%6$-*5$.*4$f", 1, 2, 3 ) );
    CHECK_THROWS( test_for_error( "%6$-*5$.*4$f", 1, 2, 3, 4 ) );
    CHECK_THROWS( test_for_error( "%6$-*5$.*4$f", 1, 2, 3, 4, 5 ) );

    // invalid format specifier
    CHECK_THROWS( test_for_error( "%k" ) );
    // can't print a void pointer
    CHECK_THROWS( test_for_error( "%s", static_cast<void *>( nullptr ) ) );
    CHECK_THROWS( test_for_error( "%d", static_cast<void *>( nullptr ) ) );
    CHECK_THROWS( test_for_error( "%d", "some string" ) );

    test_for_expected( "", "", "whatever", 5, 0.4 );
    CHECK_THROWS( test_for_error( "%d %d %d %d %d", 1, 2, 3, 4 ) );
    test_for_expected( "1 2 3 4 5", "%d %d %d %d %d", 1, 2, 3, 4, 5 );

    // test automatic type conversion
    test_for_expected( "53", "%d", '5' );
    test_for_expected( "5", "%c", '5' );
    test_for_expected( "5", "%s", '5' );
    test_for_expected( "5", "%c", '5' );
    test_for_expected( "5", "%c", static_cast<int>( '5' ) );
    test_for_expected( "5", "%s", '5' );
    test_for_expected( "55", "%s", "55" );
    test_for_expected( "5", "%s", 5 );

    test_lvalues<std::string>( "foo", "%s", "foo" );
    test_lvalues<const char *>( "bar", "%s", "bar" );
    test_lvalues<int>( "0", "%i", 0 );

    {
        std::string long_string( 100000, 'a' );
        const std::string expected = "b" + long_string + "b";
        // moving into string_format should *not* consume the string.
        test_for_expected( expected, "b%sb", std::move( long_string ) );
        CHECK( long_string.size() == 100000 );
    }

    // These calls should cause *compile* errors. Try it out.
#if 0
    string_format( "", std::vector<int>() );
    string_format( "", std::pair<int, int>() );
    string_format( "", true );
#endif

    importet_test( 1, "0.33", "%.*f", 2, 0.33333333 );
    importet_test( 2, "foo", "%.3s", "foobar" );
    importet_test( 3, "     00004", "%10.5d", 4 );
    importet_test( 416, "hi x\\n", "%*sx\\n", -3, "hi" );
    importet_test( 4, " 42", "% d", 42 );
    importet_test( 5, "-42", "% d", -42 );
    importet_test( 6, "   42", "% 5d", 42 );
    importet_test( 7, "  -42", "% 5d", -42 );
    importet_test( 8, "             42", "% 15d", 42 );
    importet_test( 9, "            -42", "% 15d", -42 );
    importet_test( 10, "+42", "%+d", 42 );
    importet_test( 11, "-42", "%+d", -42 );
    importet_test( 12, "  +42", "%+5d", 42 );
    importet_test( 13, "  -42", "%+5d", -42 );
    importet_test( 14, "            +42", "%+15d", 42 );
    importet_test( 15, "            -42", "%+15d", -42 );
    importet_test( 16, "42", "%0d", 42 );
    importet_test( 17, "-42", "%0d", -42 );
    importet_test( 18, "00042", "%05d", 42 );
    importet_test( 19, "-0042", "%05d", -42 );
    importet_test( 20, "000000000000042", "%015d", 42 );
    importet_test( 21, "-00000000000042", "%015d", -42 );
    importet_test( 22, "42", "%-d", 42 );
    importet_test( 23, "-42", "%-d", -42 );
    importet_test( 24, "42   ", "%-5d", 42 );
    importet_test( 25, "-42  ", "%-5d", -42 );
    importet_test( 26, "42             ", "%-15d", 42 );
    importet_test( 27, "-42            ", "%-15d", -42 );
    importet_test( 28, "42", "%-0d", 42 );
    importet_test( 29, "-42", "%-0d", -42 );
    importet_test( 30, "42   ", "%-05d", 42 );
    importet_test( 31, "-42  ", "%-05d", -42 );
    importet_test( 32, "42             ", "%-015d", 42 );
    importet_test( 33, "-42            ", "%-015d", -42 );
    importet_test( 34, "42", "%0-d", 42 );
    importet_test( 35, "-42", "%0-d", -42 );
    importet_test( 36, "42   ", "%0-5d", 42 );
    importet_test( 37, "-42  ", "%0-5d", -42 );
    importet_test( 38, "42             ", "%0-15d", 42 );
    importet_test( 39, "-42            ", "%0-15d", -42 );
    importet_test( 43, "42.90", "%.2f", 42.8952 );
    importet_test( 44, "42.90", "%.2F", 42.8952 );
    importet_test( 45, "42.8952000000", "%.10f", 42.8952 );
    importet_test( 46, "42.90", "%1.2f", 42.8952 );
    importet_test( 47, " 42.90", "%6.2f", 42.8952 );
    importet_test( 49, "+42.90", "%+6.2f", 42.8952 );
    importet_test( 50, "42.8952000000", "%5.10f", 42.8952 );
    /* 51: anti-test */
    /* 52: anti-test */
    /* 53: excluded for C */
    importet_test( 55, "Hot Pocket", "%1$s %2$s", "Hot", "Pocket" );
    importet_test( 56, "12.0 Hot Pockets", "%1$.1f %2$s %3$ss", 12.0, "Hot", "Pocket" );
    /* 58: anti-test */
    // importet_test( 59, "%(foo", "%(foo" ); // format is invalid
    importet_test( 60, " foo", "%*s", 4, "foo" );
    importet_test( 61, "      3.14", "%*.*f", 10, 2, 3.14159265 );
    importet_test( 63, "3.14      ", "%-*.*f", 10, 2, 3.14159265 );
    /* 64: anti-test */
    /* 65: anti-test */
    importet_test( 66, "+hello+", "+%s+", "hello" );
    importet_test( 67, "+10+", "+%d+", 10 );
    importet_test( 68, "a", "%c", 'a' );
    importet_test( 69, " ", "%c", 32 );
    importet_test( 70, "$", "%c", 36 );
    importet_test( 71, "10", "%d", 10 );
    /* 72: anti-test */
    /* 73: anti-test */
    /* 74: excluded for C */
    /* 75: excluded for C */
    importet_test( 81, "    +100", "%+8lld", 100LL );
    importet_test( 82, "+00000100", "%+.8lld", 100LL );
    importet_test( 83, " +00000100", "%+10.8lld", 100LL );
    // importet_test( 84, "%_1lld", "%_1lld", 100LL ); // format is invalid
    importet_test( 85, "-00100", "%-1.5lld", -100LL );
    importet_test( 86, "  100", "%5lld", 100LL );
    importet_test( 87, " -100", "%5lld", -100LL );
    importet_test( 88, "100  ", "%-5lld", 100LL );
    importet_test( 89, "-100 ", "%-5lld", -100LL );
    importet_test( 90, "00100", "%-.5lld", 100LL );
    importet_test( 91, "-00100", "%-.5lld", -100LL );
    importet_test( 92, "00100   ", "%-8.5lld", 100LL );
    importet_test( 93, "-00100  ", "%-8.5lld", -100LL );
    importet_test( 94, "00100", "%05lld", 100LL );
    importet_test( 95, "-0100", "%05lld", -100LL );
    importet_test( 96, " 100", "% lld", 100LL );
    importet_test( 97, "-100", "% lld", -100LL );
    importet_test( 98, "  100", "% 5lld", 100LL );
    importet_test( 99, " -100", "% 5lld", -100LL );
    importet_test( 100, " 00100", "% .5lld", 100LL );
    importet_test( 101, "-00100", "% .5lld", -100LL );
    importet_test( 102, "   00100", "% 8.5lld", 100LL );
    importet_test( 103, "  -00100", "% 8.5lld", -100LL );
    importet_test( 104, "", "%.0lld", 0LL );
    importet_test( 105, " 0x00ffffffffffffff9c", "%#+21.18llx", -100LL );
    importet_test( 106, "0001777777777777777777634", "%#.25llo", -100LL );
    importet_test( 107, " 01777777777777777777634", "%#+24.20llo", -100LL );
    importet_test( 108, "0X00000FFFFFFFFFFFFFF9C", "%#+18.21llX", -100LL );
    importet_test( 109, "001777777777777777777634", "%#+20.24llo", -100LL );
    importet_test( 113, "  -0000000000000000000001", "%+#25.22lld", -1LL );
    importet_test( 114, "00144   ", "%#-8.5llo", 100LL );
    importet_test( 115, "+00100  ", "%#-+ 08.5lld", 100LL );
    importet_test( 116, "+00100  ", "%#-+ 08.5lld", 100LL );
    importet_test( 117, "0000000000000000000000000000000000000001", "%.40lld", 1LL );
    importet_test( 118, " 0000000000000000000000000000000000000001", "% .40lld", 1LL );
    importet_test( 119, " 0000000000000000000000000000000000000001", "% .40d", 1 );
    /* 121: excluded for C */
    /* 124: excluded for C */
    importet_test( 125, " 1", "% d", 1 );
    importet_test( 126, "+1", "%+ d", 1 );
    importet_test( 129, "0x0000000001", "%#012x", 1 );
    importet_test( 130, "0x00000001", "%#04.8x", 1 );
    importet_test( 131, "0x01    ", "%#-08.2x", 1 );
    importet_test( 132, "00000001", "%#08o", 1 );
    importet_test( 142, "f", "%.1s", "foo" );
    importet_test( 143, "f", "%.*s", 1, "foo" );
    importet_test( 144, "foo  ", "%*s", -5, "foo" );
    // We are warning/erroring when not passing a format argument, i.e.
    // error: format not a string literal and no format arguments [-Werror=format-security]
    // importet_test( 145, "hello", "hello" );
    // importet_test( 147, "%b", "%b" ); // 'b' is not valid format specifier
    importet_test( 148, "  a", "%3c", 'a' );
    importet_test( 149, "1234", "%3d", 1234 );
    /* 150: excluded for C */
    importet_test( 152, "2", "%-1d", 2 );
    importet_test( 153, "8.6000", "%2.4f", 8.6 );
    importet_test( 154, "0.600000", "%0f", 0.6 );
    importet_test( 155, "1", "%.0f", 0.6 );
    importet_test( 161, "8.6", "%2.4g", 8.6 );
    importet_test( 162, "-1", "%-i", -1 );
    importet_test( 163, "1", "%-i", 1 );
    importet_test( 164, "+1", "%+i", 1 );
    importet_test( 165, "12", "%o", 10 );
    /* 166: excluded for C */
    /* 167: excluded for C */
    importet_test( 169, "%%%%", "%s", "%%%%" );
    // importet_test( 170, "4294967295", "%u", -1 ); // -1 is not a unsigned int
    // importet_test( 171, "%w", "%w", -1 ); // 'w' is not a valid format specifier
    /* 172: excluded for C */
    /* 173: excluded for C */
    /* 174: excluded for C */
    // importet_test( 176, "%H", "%H", -1 ); // 'H' is not a valid format specifier
    // We are warning/erroring when not passing a format argument, i.e.
    // error: format not a string literal and no format arguments [-Werror=format-security]
    // importet_test( 177, "%0", "%%0" );
    // importet_test( 178, "2345", "%hx", 74565 ); // 74565 is not a valid short int, as required by %hx
    importet_test( 179, "61", "%hhx", 'a' );
    // We are warning/erroring when not passing a format argument, i.e.
    // error: format not a string literal and no format arguments [-Werror=format-security]
    // importet_test( 181, "Hallo heimur", "Hallo heimur" );
    importet_test( 182, "Hallo heimur", "%s", "Hallo heimur" );
    importet_test( 183, "1024", "%d", 1024 );
    importet_test( 184, "-1024", "%d", -1024 );
    importet_test( 185, "1024", "%i", 1024 );
    importet_test( 186, "-1024", "%i", -1024 );
    importet_test( 187, "1024", "%u", 1024 );
    importet_test( 188, "4294966272", "%u", 4294966272U );
    importet_test( 189, "777", "%o", 511 );
    importet_test( 190, "37777777001", "%o", 4294966785U );
    importet_test( 191, "1234abcd", "%x", 305441741 );
    importet_test( 192, "edcb5433", "%x", 3989525555U );
    importet_test( 193, "1234ABCD", "%X", 305441741 );
    importet_test( 194, "EDCB5433", "%X", 3989525555U );
    importet_test( 195, "x", "%c", 'x' );
    // We are warning/erroring when not passing a format argument, i.e.
    // error: format not a string literal and no format arguments [-Werror=format-security]
    // importet_test( 196, "%", "%%" );
    importet_test( 197, "Hallo heimur", "%+s", "Hallo heimur" );
    importet_test( 198, "+1024", "%+d", 1024 );
    importet_test( 199, "-1024", "%+d", -1024 );
    importet_test( 200, "+1024", "%+i", 1024 );
    importet_test( 201, "-1024", "%+i", -1024 );
    importet_test( 204, "777", "%+o", 511 );
    importet_test( 205, "37777777001", "%+o", 4294966785U );
    importet_test( 206, "1234abcd", "%+x", 305441741 );
    importet_test( 207, "edcb5433", "%+x", 3989525555U );
    importet_test( 208, "1234ABCD", "%+X", 305441741 );
    importet_test( 209, "EDCB5433", "%+X", 3989525555U );
    importet_test( 210, "x", "%+c", 'x' );
    importet_test( 211, "Hallo heimur", "% s", "Hallo heimur" );
    importet_test( 212, " 1024", "% d", 1024 );
    importet_test( 213, "-1024", "% d", -1024 );
    importet_test( 214, " 1024", "% i", 1024 );
    importet_test( 215, "-1024", "% i", -1024 );
    importet_test( 218, "777", "% o", 511 );
    importet_test( 219, "37777777001", "% o", 4294966785U );
    importet_test( 220, "1234abcd", "% x", 305441741 );
    importet_test( 221, "edcb5433", "% x", 3989525555U );
    importet_test( 222, "1234ABCD", "% X", 305441741 );
    importet_test( 223, "EDCB5433", "% X", 3989525555U );
    importet_test( 224, "x", "% c", 'x' );
    importet_test( 225, "Hallo heimur", "%+ s", "Hallo heimur" );
    importet_test( 226, "+1024", "%+ d", 1024 );
    importet_test( 227, "-1024", "%+ d", -1024 );
    importet_test( 228, "+1024", "%+ i", 1024 );
    importet_test( 229, "-1024", "%+ i", -1024 );
    importet_test( 232, "777", "%+ o", 511 );
    importet_test( 233, "37777777001", "%+ o", 4294966785U );
    importet_test( 234, "1234abcd", "%+ x", 305441741 );
    importet_test( 235, "edcb5433", "%+ x", 3989525555U );
    importet_test( 236, "1234ABCD", "%+ X", 305441741 );
    importet_test( 237, "EDCB5433", "%+ X", 3989525555U );
    importet_test( 238, "x", "%+ c", 'x' );
    importet_test( 239, "0777", "%#o", 511 );
    importet_test( 240, "037777777001", "%#o", 4294966785U );
    importet_test( 241, "0x1234abcd", "%#x", 305441741 );
    importet_test( 242, "0xedcb5433", "%#x", 3989525555U );
    importet_test( 243, "0X1234ABCD", "%#X", 305441741 );
    importet_test( 244, "0XEDCB5433", "%#X", 3989525555U );
    importet_test( 245, "0", "%#o", 0U );
    importet_test( 246, "0", "%#x", 0U );
    importet_test( 247, "0", "%#X", 0U );
    importet_test( 248, "Hallo heimur", "%1s", "Hallo heimur" );
    importet_test( 249, "1024", "%1d", 1024 );
    importet_test( 250, "-1024", "%1d", -1024 );
    importet_test( 251, "1024", "%1i", 1024 );
    importet_test( 252, "-1024", "%1i", -1024 );
    importet_test( 253, "1024", "%1u", 1024 );
    importet_test( 254, "4294966272", "%1u", 4294966272U );
    importet_test( 255, "777", "%1o", 511 );
    importet_test( 256, "37777777001", "%1o", 4294966785U );
    importet_test( 257, "1234abcd", "%1x", 305441741 );
    importet_test( 258, "edcb5433", "%1x", 3989525555U );
    importet_test( 259, "1234ABCD", "%1X", 305441741 );
    importet_test( 260, "EDCB5433", "%1X", 3989525555U );
    importet_test( 261, "x", "%1c", 'x' );
    importet_test( 262, "               Hallo", "%20s", "Hallo" );
    importet_test( 263, "                1024", "%20d", 1024 );
    importet_test( 264, "               -1024", "%20d", -1024 );
    importet_test( 265, "                1024", "%20i", 1024 );
    importet_test( 266, "               -1024", "%20i", -1024 );
    importet_test( 267, "                1024", "%20u", 1024 );
    importet_test( 268, "          4294966272", "%20u", 4294966272U );
    importet_test( 269, "                 777", "%20o", 511 );
    importet_test( 270, "         37777777001", "%20o", 4294966785U );
    importet_test( 271, "            1234abcd", "%20x", 305441741 );
    importet_test( 272, "            edcb5433", "%20x", 3989525555U );
    importet_test( 273, "            1234ABCD", "%20X", 305441741 );
    importet_test( 274, "            EDCB5433", "%20X", 3989525555U );
    importet_test( 275, "                   x", "%20c", 'x' );
    importet_test( 276, "Hallo               ", "%-20s", "Hallo" );
    importet_test( 277, "1024                ", "%-20d", 1024 );
    importet_test( 278, "-1024               ", "%-20d", -1024 );
    importet_test( 279, "1024                ", "%-20i", 1024 );
    importet_test( 280, "-1024               ", "%-20i", -1024 );
    importet_test( 281, "1024                ", "%-20u", 1024 );
    importet_test( 282, "4294966272          ", "%-20u", 4294966272U );
    importet_test( 283, "777                 ", "%-20o", 511 );
    importet_test( 284, "37777777001         ", "%-20o", 4294966785U );
    importet_test( 285, "1234abcd            ", "%-20x", 305441741 );
    importet_test( 286, "edcb5433            ", "%-20x", 3989525555U );
    importet_test( 287, "1234ABCD            ", "%-20X", 305441741 );
    importet_test( 288, "EDCB5433            ", "%-20X", 3989525555U );
    importet_test( 289, "x                   ", "%-20c", 'x' );
    importet_test( 290, "00000000000000001024", "%020d", 1024 );
    importet_test( 291, "-0000000000000001024", "%020d", -1024 );
    importet_test( 292, "00000000000000001024", "%020i", 1024 );
    importet_test( 293, "-0000000000000001024", "%020i", -1024 );
    importet_test( 294, "00000000000000001024", "%020u", 1024 );
    importet_test( 295, "00000000004294966272", "%020u", 4294966272U );
    importet_test( 296, "00000000000000000777", "%020o", 511 );
    importet_test( 297, "00000000037777777001", "%020o", 4294966785U );
    importet_test( 298, "0000000000001234abcd", "%020x", 305441741 );
    importet_test( 299, "000000000000edcb5433", "%020x", 3989525555U );
    importet_test( 300, "0000000000001234ABCD", "%020X", 305441741 );
    importet_test( 301, "000000000000EDCB5433", "%020X", 3989525555U );
    importet_test( 302, "                0777", "%#20o", 511 );
    importet_test( 303, "        037777777001", "%#20o", 4294966785U );
    importet_test( 304, "          0x1234abcd", "%#20x", 305441741 );
    importet_test( 305, "          0xedcb5433", "%#20x", 3989525555U );
    importet_test( 306, "          0X1234ABCD", "%#20X", 305441741 );
    importet_test( 307, "          0XEDCB5433", "%#20X", 3989525555U );
    importet_test( 308, "00000000000000000777", "%#020o", 511 );
    importet_test( 309, "00000000037777777001", "%#020o", 4294966785U );
    importet_test( 310, "0x00000000001234abcd", "%#020x", 305441741 );
    importet_test( 311, "0x0000000000edcb5433", "%#020x", 3989525555U );
    importet_test( 312, "0X00000000001234ABCD", "%#020X", 305441741 );
    importet_test( 313, "0X0000000000EDCB5433", "%#020X", 3989525555U );
    importet_test( 314, "Hallo               ", "%0-20s", "Hallo" );
    importet_test( 315, "1024                ", "%0-20d", 1024 );
    importet_test( 316, "-1024               ", "%0-20d", -1024 );
    importet_test( 317, "1024                ", "%0-20i", 1024 );
    importet_test( 318, "-1024               ", "%0-20i", -1024 );
    importet_test( 319, "1024                ", "%0-20u", 1024 );
    importet_test( 320, "4294966272          ", "%0-20u", 4294966272U );
    importet_test( 321, "777                 ", "%-020o", 511 );
    importet_test( 322, "37777777001         ", "%-020o", 4294966785U );
    importet_test( 323, "1234abcd            ", "%-020x", 305441741 );
    importet_test( 324, "edcb5433            ", "%-020x", 3989525555U );
    importet_test( 325, "1234ABCD            ", "%-020X", 305441741 );
    importet_test( 326, "EDCB5433            ", "%-020X", 3989525555U );
    importet_test( 327, "x                   ", "%-020c", 'x' );
    importet_test( 328, "               Hallo", "%*s", 20, "Hallo" );
    importet_test( 329, "                1024", "%*d", 20, 1024 );
    importet_test( 330, "               -1024", "%*d", 20, -1024 );
    importet_test( 331, "                1024", "%*i", 20, 1024 );
    importet_test( 332, "               -1024", "%*i", 20, -1024 );
    importet_test( 333, "                1024", "%*u", 20, 1024 );
    importet_test( 334, "          4294966272", "%*u", 20, 4294966272U );
    importet_test( 335, "                 777", "%*o", 20, 511 );
    importet_test( 336, "         37777777001", "%*o", 20, 4294966785U );
    importet_test( 337, "            1234abcd", "%*x", 20, 305441741 );
    importet_test( 338, "            edcb5433", "%*x", 20, 3989525555U );
    importet_test( 339, "            1234ABCD", "%*X", 20, 305441741 );
    importet_test( 340, "            EDCB5433", "%*X", 20, 3989525555U );
    importet_test( 341, "                   x", "%*c", 20, 'x' );
    importet_test( 342, "Hallo heimur", "%.20s", "Hallo heimur" );
    importet_test( 343, "00000000000000001024", "%.20d", 1024 );
    importet_test( 344, "-00000000000000001024", "%.20d", -1024 );
    importet_test( 345, "00000000000000001024", "%.20i", 1024 );
    importet_test( 346, "-00000000000000001024", "%.20i", -1024 );
    importet_test( 347, "00000000000000001024", "%.20u", 1024 );
    importet_test( 348, "00000000004294966272", "%.20u", 4294966272U );
    importet_test( 349, "00000000000000000777", "%.20o", 511 );
    importet_test( 350, "00000000037777777001", "%.20o", 4294966785U );
    importet_test( 351, "0000000000001234abcd", "%.20x", 305441741 );
    importet_test( 352, "000000000000edcb5433", "%.20x", 3989525555U );
    importet_test( 353, "0000000000001234ABCD", "%.20X", 305441741 );
    importet_test( 354, "000000000000EDCB5433", "%.20X", 3989525555U );
    importet_test( 355, "               Hallo", "%20.5s", "Hallo heimur" );
    importet_test( 356, "               01024", "%20.5d", 1024 );
    importet_test( 357, "              -01024", "%20.5d", -1024 );
    importet_test( 358, "               01024", "%20.5i", 1024 );
    importet_test( 359, "              -01024", "%20.5i", -1024 );
    importet_test( 360, "               01024", "%20.5u", 1024 );
    importet_test( 361, "          4294966272", "%20.5u", 4294966272U );
    importet_test( 362, "               00777", "%20.5o", 511 );
    importet_test( 363, "         37777777001", "%20.5o", 4294966785U );
    importet_test( 364, "            1234abcd", "%20.5x", 305441741 );
    importet_test( 365, "          00edcb5433", "%20.10x", 3989525555U );
    importet_test( 366, "            1234ABCD", "%20.5X", 305441741 );
    importet_test( 367, "          00EDCB5433", "%20.10X", 3989525555U );
    importet_test( 369, "               01024", "%020.5d", 1024 );
    importet_test( 370, "              -01024", "%020.5d", -1024 );
    importet_test( 371, "               01024", "%020.5i", 1024 );
    importet_test( 372, "              -01024", "%020.5i", -1024 );
    importet_test( 373, "               01024", "%020.5u", 1024 );
    importet_test( 374, "          4294966272", "%020.5u", 4294966272U );
    importet_test( 375, "               00777", "%020.5o", 511 );
    importet_test( 376, "         37777777001", "%020.5o", 4294966785U );
    importet_test( 377, "            1234abcd", "%020.5x", 305441741 );
    importet_test( 378, "          00edcb5433", "%020.10x", 3989525555U );
    importet_test( 379, "            1234ABCD", "%020.5X", 305441741 );
    importet_test( 380, "          00EDCB5433", "%020.10X", 3989525555U );
    importet_test( 381, "", "%.0s", "Hallo heimur" );
    importet_test( 382, "                    ", "%20.0s", "Hallo heimur" );
    importet_test( 383, "", "%.s", "Hallo heimur" );
    importet_test( 384, "                    ", "%20.s", "Hallo heimur" );
    importet_test( 385, "                1024", "%20.0d", 1024 );
    importet_test( 386, "               -1024", "%20.d", -1024 );
    importet_test( 387, "                    ", "%20.d", 0 );
    importet_test( 388, "                1024", "%20.0i", 1024 );
    importet_test( 389, "               -1024", "%20.i", -1024 );
    importet_test( 390, "                    ", "%20.i", 0 );
    importet_test( 391, "                1024", "%20.u", 1024 );
    importet_test( 392, "          4294966272", "%20.0u", 4294966272U );
    importet_test( 393, "                    ", "%20.u", 0U );
    importet_test( 394, "                 777", "%20.o", 511 );
    importet_test( 395, "         37777777001", "%20.0o", 4294966785U );
    importet_test( 396, "                    ", "%20.o", 0U );
    importet_test( 397, "            1234abcd", "%20.x", 305441741 );
    importet_test( 398, "            edcb5433", "%20.0x", 3989525555U );
    importet_test( 399, "                    ", "%20.x", 0U );
    importet_test( 400, "            1234ABCD", "%20.X", 305441741 );
    importet_test( 401, "            EDCB5433", "%20.0X", 3989525555U );
    importet_test( 402, "                    ", "%20.X", 0U );
    importet_test( 403, "Hallo               ", "% -0+*.*s", 20, 5, "Hallo heimur" );
    importet_test( 404, "+01024              ", "% -0+*.*d", 20, 5, 1024 );
    importet_test( 405, "-01024              ", "% -0+*.*d", 20, 5, -1024 );
    importet_test( 406, "+01024              ", "% -0+*.*i", 20, 5, 1024 );
    importet_test( 407, "-01024              ", "% 0-+*.*i", 20, 5, -1024 );
    importet_test( 410, "00777               ", "%+ -0*.*o", 20, 5, 511 );
    importet_test( 411, "37777777001         ", "%+ -0*.*o", 20, 5, 4294966785U );
    importet_test( 412, "1234abcd            ", "%+ -0*.*x", 20, 5, 305441741 );
    importet_test( 413, "00edcb5433          ", "%+ -0*.*x", 20, 10, 3989525555U );
    importet_test( 414, "1234ABCD            ", "% -+0*.*X", 20, 5, 305441741 );
    importet_test( 415, "00EDCB5433          ", "% -+0*.*X", 20, 10, 3989525555U );
}
