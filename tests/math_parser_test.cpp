#include "cata_catch.h"

#include <cmath>

#include "avatar.h"
#include "dialogue.h"
#include "global_vars.h"
#include "math_parser.h"
#include "math_parser_func.h"

static const spell_id spell_test_spell_pew( "test_spell_pew" );

// NOLINTNEXTLINE(readability-function-cognitive-complexity): false positive
TEST_CASE( "math_parser_parsing", "[math_parser]" )
{
    dialogue const d( std::make_unique<talker>(), std::make_unique<talker>() );
    math_exp testexp;

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

    // nan and inf
    CHECK( testexp.parse( "1 / 0" ) );
    CHECK( std::isinf( testexp.eval( d ) ) );
    CHECK( testexp.parse( "-1 ^ 0.5" ) );
    CHECK( std::isnan( testexp.eval( d ) ) );

    // failed validation
    // NOLINTNEXTLINE(readability-function-cognitive-complexity): false positive
    std::string dmsg = capture_debugmsg_during( [&testexp, &d]() {
        CHECK_FALSE( testexp.parse( "2+" ) );
        CHECK_FALSE( testexp.parse( ")" ) );
        CHECK_FALSE( testexp.parse( "(" ) );
        CHECK_FALSE( testexp.parse( "()" ) );
        CHECK_FALSE( testexp.parse( "(2+2))" ) );
        CHECK_FALSE( testexp.parse( "(2+2^)" ) );
        CHECK_FALSE( testexp.parse( "((2+2)" ) );
        CHECK_FALSE( testexp.parse( "(2+2,3)" ) );
        CHECK_FALSE( testexp.parse( "sin(1,2)" ) ); // too many arguments
        CHECK_FALSE( testexp.parse( "rng(1)" ) );   // not enough arguments
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
        CHECK_FALSE( testexp.parse( "u_val('wr'ong')" ) ); // stray ' inside string
        CHECK_FALSE( testexp.parse( "2 2*2" ) ); // stray space inside variable name
        CHECK_FALSE( testexp.parse( "2+++2" ) );
        CHECK( testexp.parse( "2+3" ) );
        testexp.assign( d, 10 ); // assignment called on eval tree should not crash
    } );
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity): false positive
TEST_CASE( "math_parser_dialogue_integration", "[math_parser]" )
{
    standard_npc dude;
    dialogue const d( get_talker_for( get_avatar() ), get_talker_for( &dude ) );
    math_exp testexp;
    global_variables &globvars = get_globals();

    // reading scoped variables
    globvars.set_global_value( "npctalk_var_x", "100" );
    CHECK( testexp.parse( "x" ) );
    CHECK( testexp.eval( d ) == Approx( 100 ) );
    get_avatar().set_value( "npctalk_var_x", "92" );
    CHECK( testexp.parse( "u_x" ) );
    CHECK( testexp.eval( d ) == Approx( 92 ) );
    dude.set_value( "npctalk_var_x", "21" );
    CHECK( testexp.parse( "n_x" ) );
    CHECK( testexp.eval( d ) == Approx( 21 ) );
    CHECK( testexp.parse( "x + u_x + n_x" ) );
    CHECK( testexp.eval( d ) == Approx( 213 ) );

    // reading scoped values with u_val shim
    std::string dmsg = capture_debugmsg_during( [&testexp]() {
        CHECK_FALSE( testexp.parse( "u_val( 3 )" ) ); // only quoted strings accepted as parameters
        CHECK_FALSE( testexp.parse( "u_val( stamina )" ) );
        CHECK_FALSE( testexp.parse( "val( 'stamina' )" ) ); // invalid scope for this function
    } );
    CHECK( testexp.parse( "u_val('stamina')" ) );
    CHECK( testexp.eval( d ) == get_avatar().get_stamina() );
    CHECK( testexp.parse( "u_val('spell_level', 'spell: test_spell_pew')" ) );
    get_avatar().magic->learn_spell( spell_test_spell_pew, get_avatar(), true );
    get_avatar().magic->set_spell_level( spell_test_spell_pew, 4, &get_avatar() );
    REQUIRE( d.actor( false )->get_spell_level( spell_test_spell_pew ) != 0 );
    CHECK( testexp.eval( d ) == d.actor( false )->get_spell_level( spell_test_spell_pew ) );
    CHECK( testexp.parse( "u_val('time: 1 m')" ) ); // test get_member() in shim
    CHECK( testexp.eval( d ) == 60 );

    // assignment to scoped variables
    CHECK( testexp.parse( "u_testvar", true ) );
    testexp.assign( d, 159 );
    CHECK( std::stoi( get_avatar().get_value( "npctalk_var_testvar" ) ) == 159 );
    CHECK( testexp.parse( "testvar", true ) );
    testexp.assign( d, 259 );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_testvar" ) ) == 259 );
    CHECK( testexp.parse( "n_testvar", true ) );
    testexp.assign( d, 359 );
    CHECK( std::stoi( dude.get_value( "npctalk_var_testvar" ) ) == 359 );

    // assignment to scoped values with u_val shim
    CHECK( testexp.parse( "u_val('stamina')", true ) );
    testexp.assign( d, 459 );
    CHECK( get_avatar().get_stamina() == 459 );
    CHECK( testexp.parse( "n_val('stored_kcal')", true ) );
    testexp.assign( d, 559 );
    CHECK( dude.get_stored_kcal() == 559 );
    std::string morelogs = capture_debugmsg_during( [&testexp, &d]() {
        CHECK( testexp.eval( d ) == Approx( 0 ) ); // eval called on assignment tree should not crash
        CHECK_FALSE( testexp.parse( "val( 'stamina' ) * 3", true ) ); // eval expression in assignment tree
    } );
}
