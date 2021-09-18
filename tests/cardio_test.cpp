#include "avatar.h"
#include "cata_catch.h"
#include "options.h"
#include "player_helpers.h"

/*
Functions:
- Character::get_stamina
- Character::set_stamina
- Character::get_stamina_max
- Character::get_cardiofit
- Character::get_cardio_acc
- Character::set_cardio_acc
- avatar::update_cardio_acc <-- Critical
*/

TEST_CASE( "cardio influences max stamina", "[cardio][stamina]" )
{
    // With baseline cardio, max stamina is X
    // With low cardio, max stamina is X--
    // With high cardio, max stamina is X++

    const avatar &they = get_avatar();
    clear_avatar();

    // Ensure configured game balance options are what the test assumes
    const int max_stamina_base = get_option<int>( "PLAYER_MAX_STAMINA_BASE" );
    const int cardiofit_stamina_scaling = get_option<int>( "PLAYER_CARDIOFIT_STAMINA_SCALING" );
    REQUIRE( max_stamina_base == 3500 );
    REQUIRE( cardiofit_stamina_scaling == 3 );

    // Default cleared avatar needs this starting base_bmr, or no later assertions will work
    const int base_bmr = they.base_bmr();
    REQUIRE( base_bmr == 1738 );

    // Accumulator should start at half base BMR
    CHECK( they.get_cardio_acc() == base_bmr / 2 );

    // Max should be 3500 + 3 * get_cardiofit
    CHECK( they.get_cardiofit() == 1739 );
    CHECK( they.get_stamina_max() == 8717 );
}

TEST_CASE( "cardio influences stamina regeneration", "[cardio][stamina]" )
{
    // With baseline cardio, stamina regen is X
    // With low cardio, stamina regen is X--
    // With high cardio, stamina regen is X++
}

TEST_CASE( "cardio is affected by activity level each day", "[cardio]" )
{
    // When activity increases, cardio goes up
    // When activity decreases, cardio goes down
    // When activity stays the same, cardio stays the same
}

TEST_CASE( "cardio fitness level", "[cardio][fitness]" )
{
    // get_cardiofit - WHAT IS IT?
    // yes get_cardiofit - WHAT IS IT?
}

