#include <algorithm>
#include <clocale>
#include <cmath>
#include <functional>
#include <locale>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "avatar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "coordinates.h"
#include "debug.h"
#include "dialogue.h"
#include "global_vars.h"
#include "math_parser.h"
#include "math_parser_diag_value.h"
#include "math_parser_func.h"
#include "math_parser_type.h"
#include "npc.h"
#include "point.h"
#include "talker.h"

// NOLINTNEXTLINE(readability-function-cognitive-complexity): false positive
TEST_CASE( "math_parser_parsing", "[math_parser]" )
{
    dialogue d( std::make_unique<talker>(), std::make_unique<talker>() );
    math_exp testexp;

    std::locale const &oldloc = std::locale();
    on_out_of_scope reset_loc( [&oldloc]() {
        std::locale::global( oldloc );
        char *discard [[maybe_unused]] = std::setlocale( LC_ALL, oldloc.name().c_str() );
    } );
    try {
        std::locale::global( std::locale( "de_DE.UTF-8" ) );
        char *discard [[maybe_unused]] = std::setlocale( LC_ALL, "de_DE.UTF-8" );
    } catch( std::runtime_error &e ) {
        WARN( "couldn't set locale for math_parser test: " << e.what() );
    }
    CAPTURE( std::setlocale( LC_ALL, nullptr ), std::locale().name(), std::to_string( 1.2 ) );

    CHECK_FALSE( testexp.parse( "" ) );
    CHECK( testexp.eval( d ) == Approx( 0.0 ) );
    CHECK( testexp.parse( "50" ) );
    CHECK( testexp.eval( d ) == Approx( 50 ) );

    // binary
    CHECK( testexp.parse( "50 + 2" ) );
    CHECK( testexp.eval( d ) == Approx( 52 ) );
    CHECK( testexp.parse( "50 + 2 * 3" ) );
    CHECK( testexp.eval( d ) == Approx( 56 ) );
    CHECK( testexp.parse( "50 + 2 * 3 ^ 2" ) );
    CHECK( testexp.eval( d ) == Approx( 68 ) );
    CHECK( testexp.parse( "50 + ( 2 * 3 ) ^ 2" ) );
    CHECK( testexp.eval( d ) == Approx( 86 ) );
    CHECK( testexp.parse( "(((50 + (((((( 2 * 4 ))))) ^ 2))))" ) );
    CHECK( testexp.eval( d ) == Approx( 114 ) );
    CHECK( testexp.parse( "50 % 3" ) );
    CHECK( testexp.eval( d ) == Approx( 2 ) );
    // boolean
    CHECK( testexp.parse( "1 > 2" ) );
    CHECK( testexp.eval( d ) == Approx( 0 ) );
    CHECK( testexp.parse( "(1 <= 2) * 3" ) );
    CHECK( testexp.eval( d ) == Approx( 3 ) );

    // unary
    CHECK( testexp.parse( "-50" ) );
    CHECK( testexp.eval( d ) == Approx( -50 ) );
    CHECK( testexp.parse( "+50" ) );
    CHECK( testexp.eval( d ) == Approx( 50 ) );
    CHECK( testexp.parse( "-3^2" ) );
    CHECK( testexp.eval( d ) == Approx( 9 ) );
    CHECK( testexp.parse( "-(3^2)" ) );
    CHECK( testexp.eval( d ) == Approx( -9 ) );
    CHECK( testexp.parse( "-3^3" ) );
    CHECK( testexp.eval( d ) == Approx( -27 ) );
    CHECK( testexp.parse( "(-3)^2" ) );
    CHECK( testexp.eval( d ) == Approx( 9 ) );
    CHECK( testexp.parse( "-3^-2" ) );
    CHECK( testexp.eval( d ) == Approx( std::pow( -3, -2 ) ) );
    CHECK( testexp.parse( "2++3" ) ); // equivalent to 2+(+3)
    CHECK( testexp.eval( d ) == Approx( 5 ) );
    CHECK( testexp.parse( "2+-3" ) ); // equivalent to 2+(-3)
    CHECK( testexp.eval( d ) == Approx( -1 ) );
    CHECK( testexp.parse( "!1" ) );
    CHECK( testexp.eval( d ) == Approx( 0 ) );
    CHECK( testexp.parse( "!(1 == 0)" ) );
    CHECK( testexp.eval( d ) == Approx( 1 ) );

    //ternary
    CHECK( testexp.parse( "0?1:2" ) );
    CHECK( testexp.eval( d ) == Approx( 2 ) );
    CHECK( testexp.parse( "0==0?1:2" ) );
    CHECK( testexp.eval( d ) == Approx( 1 ) );
    CHECK( testexp.parse( "1?0?-1:-2:1" ) );
    CHECK( testexp.eval( d ) == Approx( -2 ) );
    CHECK( testexp.parse( "1?1?-1:-2:1" ) );
    CHECK( testexp.eval( d ) == Approx( -1 ) );
    CHECK( testexp.parse( "0?0?-1:-2:1" ) );
    CHECK( testexp.eval( d ) == Approx( 1 ) );
    CHECK( testexp.parse( "0?(0?(-1):-2):1" ) );
    CHECK( testexp.eval( d ) == Approx( 1 ) );
    CHECK( testexp.parse( "1==1?2:3?4:5" ) );
    CHECK( testexp.eval( d ) == Approx( 2 ) );
    CHECK( testexp.parse( "0?2:3?4:5" ) );
    CHECK( testexp.eval( d ) == Approx( 4 ) );
    CHECK( testexp.parse( "0?2:0?4:5" ) );
    CHECK( testexp.eval( d ) == Approx( 5 ) );
    CHECK( testexp.parse( "(1==1?2:3)?4:5" ) );
    CHECK( testexp.eval( d ) == Approx( 4 ) );
    CHECK( testexp.parse( "1==1?2:(3?4:5)" ) );
    CHECK( testexp.eval( d ) == Approx( 2 ) );

    // functions
    CHECK( testexp.parse( "_test_()" ) ); // nullary test function
    CHECK( testexp.parse( "max()" ) ); // variadic called with zero arguments
    CHECK( testexp.parse( "clamp( 1, 2, 3 )" ) ); // arguments passed in the right order 🤦
    CHECK( testexp.eval( d ) == Approx( 2 ) );
    CHECK( testexp.parse( "sin(1)" ) );
    CHECK( testexp.eval( d ) == Approx( std::sin( 1.0 ) ) );
    CHECK( testexp.parse( "sin((1))" ) );
    CHECK( testexp.eval( d ) == Approx( std::sin( 1.0 ) ) );
    CHECK( testexp.parse( "cos( sin( 1 ) )" ) );
    CHECK( testexp.eval( d ) == Approx( std::cos( std::sin( 1.0 ) ) ) );
    CHECK( testexp.parse( "cos( (sin( 1 )) )" ) );
    CHECK( testexp.eval( d ) == Approx( std::cos( std::sin( 1.0 ) ) ) );
    CHECK( testexp.parse( "cos( (sin( (1) )) )" ) );
    CHECK( testexp.eval( d ) == Approx( std::cos( std::sin( 1.0 ) ) ) );
    CHECK( testexp.parse( "sin(-(-(-(-2))))" ) );
    CHECK( testexp.eval( d ) == Approx( std::sin( 2 ) ) );
    CHECK( testexp.parse( "max( 1, 2, 3, 4, 5, 6 )" ) );
    CHECK( testexp.eval( d ) == Approx( 6 ) );
    CHECK( testexp.parse( "cos( sin( min( 1 + 2, -50 ) ) )" ) );
    CHECK( testexp.eval( d ) == Approx( std::cos( std::sin( std::min( 1 + 2, -50 ) ) ) ) );

    // whitespace
    CHECK( testexp.parse( "       50              +  (  2 *                3 ) ^                2             " ) );
    CHECK( testexp.eval( d ) == Approx( 86 ) );

    // constants
    CHECK( testexp.parse( "π" ) );
    CHECK( testexp.eval( d ) == Approx( math_constants::pi_v ) );
    CHECK( testexp.parse( "pi" ) );
    CHECK( testexp.eval( d ) == Approx( math_constants::pi_v ) );
    CHECK( testexp.parse( "e^2" ) );
    CHECK( testexp.eval( d ) == Approx( std::pow( math_constants::e_v, 2 ) ) );

    // from https://raw.githubusercontent.com/ArashPartow/math-parser-benchmark-project/master/bench_expr_complete.txt
    CHECK( testexp.parse( "((((((((5+7)*7.123)-3)-((5+7)-(7.123*3)))-((5*(7-(7.123*3)))/((5*7)+(7.123+3))))-((((5+7)-(7.123*3))+((5/7)+(7.123+3)))+(((5*7)+(7.123+3))*((5/7)/(7.123-3)))))-(((((5/7)-(7.321/3))*((5-7)+(7.321+3)))*(((5-7)-(7.321+3))+((5-7)*(7.321/3))))*((((5-7)+(7.321+3))-((5*7)*(7.321+3)))-(((5-7)*(7.321/3))/(5+((7/7.321)+3)))))))" ) );
    CHECK( testexp.eval( d ) == Approx( 87139.7243098 ) );

    // locale-independent decimal point
    CHECK( testexp.parse( "2 * 1.5" ) );
    CHECK( testexp.eval( d ) == Approx( 3 ) );
    CHECK( testexp.parse( "2 * .5" ) );
    CHECK( testexp.eval( d ) == Approx( 1 ) );
    // E-notation
    CHECK( testexp.parse( ".5e1" ) );
    CHECK( testexp.eval( d ) == Approx( .5e1 ) );
    CHECK( testexp.parse( ".5e1+1" ) );
    CHECK( testexp.eval( d ) == Approx( .5e1 + 1 ) );
    CHECK( testexp.parse( "1.5E+10 + 1" ) );
    CHECK( testexp.eval( d ) == Approx( 1.5e+10 + 1 ) );
    CHECK( testexp.parse( "1.5e-10+1" ) );
    CHECK( testexp.eval( d ) == Approx( 1.5e-10 + 1 ) );

    // nan and inf
    CHECK( testexp.parse( "1 / 0" ) );
    CHECK( std::isinf( testexp.eval( d ) ) );
    CHECK( testexp.parse( "-1 ^ 0.5" ) );
    CHECK( std::isnan( testexp.eval( d ) ) );

    // diag functions. _test_diag adds up all the (kw)args except for "test_unused_kwarg"
    CHECK( testexp.parse( "_test_diag_(1, 2, 3*4)" ) ); // mixed arg types
    CHECK( testexp.eval( d ) == Approx( 15 ) );
    CHECK( testexp.parse( "_test_diag_('1+2*3:() sin(1)')" ) ); // string with token delimiters
    CHECK( testexp.parse( "_test_diag_(sin(pi), _test_diag_(1))" ) ); // composition
    CHECK( testexp.eval( d ) == Approx( 1 ) );
    CHECK( testexp.parse( "_test_diag_(1, 2, '1':2, '3':4)" ) );  // kwargs after positional
    CHECK( testexp.eval( d ) == Approx( 9 ) );
    CHECK( testexp.parse( "_test_diag_('1':2)" ) );  // double kwarg
    CHECK( testexp.eval( d ) == Approx( 2 ) );
    CHECK( testexp.parse( "_test_diag_('1':3.5*sin(pi/2))" ) );  // sub-expression kwarg
    CHECK( testexp.eval( d ) == Approx( 3.5 ) );
    CHECK( testexp.parse( "_test_diag_('1':0==0?1:2)" ) );  // ternary kwarg
    CHECK( testexp.eval( d ) == Approx( 1 ) );
    CHECK( testexp.parse( "_test_diag_('1':2*_test_diag_('2':3))" ) );  // kwarg composition
    CHECK( testexp.eval( d ) == Approx( 6 ) );
    CHECK( testexp.parse( "_test_diag_([1,2,2+1])" ) );  // array arg
    CHECK( testexp.eval( d ) == Approx( 6 ) );
    CHECK( testexp.parse( "_test_diag_([1,1+1,3], 'blorg': [3+1,5,6])" ) );  // array kwarg
    CHECK( testexp.eval( d ) == Approx( 21 ) );
    CHECK( testexp.parse( "_test_str_len_(['1','2'], '1': ['one','two'])" ) );  // str array
    CHECK( testexp.eval( d ) == Approx( 8 ) );
    CHECK( testexp.parse( "_test_str_len_(['don\\'t'])" ) );  // escaped quote
    CHECK( testexp.eval( d ) == Approx( 5 ) );
    CHECK( testexp.parse( "_test_diag_([1,2,3], 'blorg': [4,5,_test_diag_([6,7,8], '2':[9])])" ) );  // yo dawg
    CHECK( testexp.eval( d ) == Approx( 45 ) );
    CHECK( testexp.parse( "_test_diag_([[0,-1],[2,0],[3,4]])" ) );  // nested arrays
    CHECK( testexp.parse( "_test_diag_([[0,-1],[2,0],[3,(3+1)]])" ) );
    CHECK( testexp.eval( d ) == Approx( 8 ) );
    CHECK( testexp.parse( "_test_diag_([],1)" ) );  // empty array
    CHECK( testexp.eval( d ) == Approx( 1 ) );
    CHECK( testexp.parse( "_test_diag_([0?1:2,3,1?4:5])" ) );  // ternaries in array
    CHECK( testexp.eval( d ) == Approx( 9 ) );
    CHECK( testexp.parse( "_test_str_len_((['1','2']))" ) ); // pointless parens
    CHECK( testexp.eval( d ) == Approx( 2 ) );
    CHECK( testexp.parse( "_test_str_len_((['1',('2')]))" ) ); // pointless parens
    CHECK( testexp.eval( d ) == Approx( 2 ) );

    // dot operator
    get_globals().set_global_value( "mytripoint", tripoint_abs_ms{ 1, 2, 3 } );
    CHECK( testexp.parse( "mytripoint.x" ) );
    CHECK( testexp.eval( d ) == Approx( 1 ) );
    CHECK( testexp.parse( "mytripoint.y" ) );
    CHECK( testexp.eval( d ) == Approx( 2 ) );
    CHECK( testexp.parse( "mytripoint.z" ) );
    CHECK( testexp.eval( d ) == Approx( 3 ) );
    CHECK( testexp.parse( "mytripoint.z = 4" ) );
    testexp.eval( d );
    CHECK( get_globals().get_global_value( "mytripoint" ).tripoint().z() == 4 );

    // failed validation
    // NOLINTNEXTLINE(readability-function-cognitive-complexity): false positive
    std::string dmsg = capture_debugmsg_during( [&testexp, &d]() {
        CHECK_FALSE( testexp.parse( "2+" ) );
        CHECK_FALSE( testexp.parse( "2e" ) );
        CHECK_FALSE( testexp.parse( "2e+" ) );
        CHECK_FALSE( testexp.parse( "2e1.1" ) );
        CHECK_FALSE( testexp.parse( "2e+1.1" ) );
        CHECK_FALSE( testexp.parse( "1.2.3" ) );
        CHECK_FALSE( testexp.parse( "." ) );
        CHECK_FALSE( testexp.parse( "1+." ) );
        CHECK_FALSE( testexp.parse( "2stupiddogs" ) );
        CHECK_FALSE( testexp.parse( ")" ) );
        CHECK_FALSE( testexp.parse( "(" ) );
        CHECK_FALSE( testexp.parse( "[" ) );
        CHECK_FALSE( testexp.parse( "]" ) );
        CHECK_FALSE( testexp.parse( "()" ) );
        CHECK_FALSE( testexp.parse( "[]" ) );
        CHECK_FALSE( testexp.parse( "[2]" ) ); // not diag function
        CHECK_FALSE( testexp.parse( "(2+2))" ) );
        CHECK_FALSE( testexp.parse( "(2+2^)" ) );
        CHECK_FALSE( testexp.parse( "((2+2)" ) );
        CHECK_FALSE( testexp.parse( "(2+2,3)" ) );
        CHECK_FALSE( testexp.parse( "sin(1,2)" ) ); // too many arguments
        CHECK_FALSE( testexp.parse( "rng(1)" ) );   // not enough arguments
        CHECK_FALSE( testexp.parse( "rng(1,[2,3])" ) );   // not diag function
        CHECK_FALSE( testexp.parse( "sin((1+2,3))" ) );
        CHECK_FALSE( testexp.parse( "sin(1" ) );
        CHECK_FALSE( testexp.parse( "sin" ) );
        CHECK_FALSE( testexp.parse( "sin('1')" ) );
        CHECK_FALSE( testexp.parse( "sin(+)" ) );
        CHECK_FALSE( testexp.parse( "sin(-)" ) );
        CHECK_FALSE( testexp.parse( "_test_(-)" ) );
        CHECK_FALSE( testexp.parse( "_test_(1)" ) );
        CHECK_FALSE( testexp.parse( "'string'" ) );
        CHECK_FALSE( testexp.parse( "('wrong')" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_('wr'ong')" ) ); // stray ' inside string
        CHECK_FALSE( testexp.parse( "_test_diag_('wrong)" ) ); // unterminated string
        CHECK_FALSE( testexp.parse( "_test_diag_('1'+'2')" ) );  // no string operators (yet)
        CHECK_FALSE( testexp.parse( "_test_diag_(1:'2')" ) );    // kwarg key is not string
        CHECK_FALSE( testexp.parse( "_test_diag_('1':'2', 3)" ) ); // positional after kwargs
        CHECK_FALSE( testexp.parse( "_test_diag_('test_unused_kwarg': 'mustfail')" ) ); // unused kwarg
        CHECK_FALSE( testexp.parse( "_test_diag_('1':'2':)" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_('1':'2':'3')" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_(:)" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_([)" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_([" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_(2[)" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_(2+[)" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_(])" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_(2])" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_(2+])" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_(['1','':'1')" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_([1,2))" ) ); // mismatched brackets
        CHECK_FALSE( testexp.parse( "_test_diag_([1,2]*2)" ) ); // no operators support arrays (yet)
        CHECK_FALSE( testexp.parse( "_test_diag_(-[1,2])" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_([1,2]?1:2)" ) ); // array can't be condition for ternary
        CHECK_FALSE( testexp.parse( "_test_diag_(1?2:[2,3])" ) ); // no arrays in ternaries (yet)
        CHECK_FALSE( testexp.parse( "_test_diag_(1?[2,3]:4)" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_(['blorg':3])" ) ); // no kwargs in arrays
        CHECK_FALSE( testexp.parse( "_test_diag_(1?'a':1:'b':2)" ) ); // no kwargs in ternaries
        CHECK_FALSE( testexp.parse( "sin('1':'2')" ) ); // no kwargs in math functions (yet?)
        CHECK( testexp.parse( "_test_str_len_('fail')" ) );  // expected array at runtime
        bool expected_array = false;
        try {
            testexp.eval( d );
        } catch( math::runtime_error const &ex ) {
            std::string_view what( ex.what() );
            expected_array = what.find( "requested array" ) != std::string_view::npos;
        }
        CHECK( expected_array );
        CHECK_FALSE( testexp.parse( "'1':'2'" ) );
        CHECK_FALSE( testexp.parse( "2 2*2" ) ); // missing operator
        CHECK_FALSE( testexp.parse( "2+++2" ) );
        CHECK_FALSE( testexp.parse( "1=2" ) );
        CHECK_FALSE( testexp.parse( "1===2" ) );
        CHECK_FALSE( testexp.parse( "0?" ) );
        CHECK_FALSE( testexp.parse( "0?1" ) );
        CHECK_FALSE( testexp.parse( "0?1:" ) );
        CHECK_FALSE( testexp.parse( "2+3 = 10" ) );
        CHECK_FALSE( testexp.parse( "a+b = c" ) );
        CHECK_FALSE( testexp.parse( "a = b = c" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_([a=b])" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_([a=+])" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_('1':0=0?1:2)" ) );
        CHECK_FALSE( testexp.parse( "_test_diag_('1':a=2)" ) );

        CHECK_FALSE( testexp.parse( "mytripoint.(z)" ) );
        CHECK_FALSE( testexp.parse( "mytripoint.2" ) );
        CHECK_FALSE( testexp.parse( "mytripoint.w" ) );
        CHECK_FALSE( testexp.parse( "mytripoint.+x" ) );
        CHECK_FALSE( testexp.parse( "mytripoint.rand(1)" ) );
    } );
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity): false positive
TEST_CASE( "math_parser_dialogue_integration", "[math_parser]" )
{
    standard_npc dude;
    dialogue d( get_talker_for( get_avatar() ), get_talker_for( &dude ) );
    math_exp testexp;
    global_variables &globvars = get_globals();

    // reading scoped variables
    globvars.set_global_value( "x", 100 );
    CHECK( testexp.parse( "x" ) );
    CHECK( testexp.eval( d ) == Approx( 100 ) );
    get_avatar().set_value( "x", 92 );
    CHECK( testexp.parse( "u_x" ) );
    CHECK( testexp.eval( d ) == Approx( 92 ) );
    dude.set_value( "x", 21 );
    CHECK( testexp.parse( "n_x" ) );
    CHECK( testexp.eval( d ) == Approx( 21 ) );
    CHECK( testexp.parse( "x + u_x + n_x" ) );
    CHECK( testexp.eval( d ) == Approx( 213 ) );

    CHECK( testexp.parse( "_ctx" ) );
    CHECK( testexp.eval( d ) == Approx( 0 ) );

    CHECK( testexp.parse( "value_or(_ctx, 13)" ) );
    CHECK( testexp.eval( d ) == Approx( 13 ) );
    CHECK( testexp.parse( "has_var(_ctx)?19:20" ) );
    CHECK( testexp.eval( d ) == Approx( 20 ) );

    d.set_value( "ctx", 14 );

    CHECK( testexp.parse( "_ctx" ) );
    CHECK( testexp.eval( d ) == Approx( 14 ) );

    CHECK( testexp.parse( "value_or(_ctx, 13)" ) );
    CHECK( testexp.eval( d ) == Approx( 14 ) );
    CHECK( testexp.parse( "has_var(_ctx)?19:20" ) );
    CHECK( testexp.eval( d ) == Approx( 19 ) );

    // wrong arg type
    std::string dmsg = capture_debugmsg_during( [&testexp, &d]() {
        CHECK( testexp.parse( "_test_diag_( '3' )" ) ); // this function doesn't support strings
        bool expected_string = false;
        try {
            testexp.eval( d );
        } catch( math::exception const &ex ) {
            std::string_view what( ex.what() );
            expected_string = what.find( "requested double" ) != std::string_view::npos;
            CAPTURE( what );
        }
        CHECK( expected_string );
        CHECK_FALSE( testexp.parse( "val( 'stamina' )" ) ); // invalid scope for this function
        CHECK_FALSE( testexp.parse( "_test_str_len_([]) = 2" ) ); // dialogue function cannot assign
    } );
    // reading scoped values with u_val shim
    CHECK( testexp.parse( "u_val('stamina')" ) );
    CHECK( testexp.eval( d ) == get_avatar().get_stamina() );

    // units test
    CHECK( testexp.parse( "time('1 m')" ) );
    CHECK( testexp.eval( d ) == 60 );
    CHECK( testexp.parse( "time('1 m', 'unit':'minutes')" ) );
    CHECK( testexp.eval( d ) == 1 );
    CHECK( testexp.parse( "energy('25 kJ')" ) );
    CHECK( testexp.eval( d ) == 25000000 );

    // evaluating string variables in dialogue functions
    globvars.set_global_value( "someskill", "survival" );
    CHECK( testexp.parse( "_test_str_len_([someskill])" ) );
    CHECK( testexp.eval( d ) == 8 );

    // assignment to scoped variables
    CHECK( testexp.parse( "u_testvar = 159" ) );
    testexp.eval( d );
    CHECK( get_avatar().get_value( "testvar" ) == 159 );
    CHECK( testexp.parse( "testvar = 259" ) );
    testexp.eval( d );
    CHECK( globvars.get_global_value( "testvar" ) == 259 );
    CHECK( testexp.parse( "n_testvar = 359" ) );
    testexp.eval( d );
    CHECK( dude.get_value( "testvar" ) == 359 );
    CHECK( testexp.parse( "_testvar = 159" ) );
    testexp.eval( d );
    CHECK( d.get_value( "testvar" ) == 159 );
    CHECK( testexp.parse( "_testvar += 1" ) );
    testexp.eval( d );
    CHECK( d.get_value( "testvar" ) == 160 );
    CHECK( testexp.parse( "_testvar -= 1" ) );
    testexp.eval( d );
    CHECK( d.get_value( "testvar" ) == 159 );
    CHECK( testexp.parse( "_testvar *= 2" ) );
    testexp.eval( d );
    CHECK( d.get_value( "testvar" ) == 318 );
    CHECK( testexp.parse( "_testvar /= 2" ) );
    testexp.eval( d );
    CHECK( d.get_value( "testvar" ) == 159 );
    CHECK( testexp.parse( "_testvar %= 2" ) );
    testexp.eval( d );
    CHECK( d.get_value( "testvar" ) == 1 );
    CHECK( testexp.parse( "_blorg = ((((((((5+7)*7.123)-3)-((5+7)-(7.123*3)))-((5*(7-(7.123*3)))/((5*7)+(7.123+3))))-((((5+7)-(7.123*3))+((5/7)+(7.123+3)))+(((5*7)+(7.123+3))*((5/7)/(7.123-3)))))-(((((5/7)-(7.321/3))*((5-7)+(7.321+3)))*(((5-7)-(7.321+3))+((5-7)*(7.321/3))))*((((5-7)+(7.321+3))-((5*7)*(7.321+3)))-(((5-7)*(7.321/3))/(5+((7/7.321)+3)))))))" ) );
    testexp.eval( d );
    CHECK( d.get_value( "blorg" ).dbl() == Approx( 87139.724 ) );

    CHECK( testexp.parse( "_testvar++" ) );
    testexp.eval( d );
    CHECK( d.get_value( "testvar" ) == 2 );
    CHECK( testexp.parse( "_testvar --" ) );
    testexp.eval( d );
    CHECK( d.get_value( "testvar" ) == 1 );

    // assignment to scoped values with u_val shim
    CHECK( testexp.parse( "u_val('stamina') = 459" ) );
    testexp.eval( d );
    CHECK( get_avatar().get_stamina() == 459 );

}
