#include "catch/catch.hpp"

#include "addiction.h"
#include "character.h"

// Tests for Character addiction, including:
//
// - Character::add_addiction
// - Character::rem_addiction
// - Character::has_addiction
// - Character::addiction_level
// - Character::suffer_from_addictions

// Addiction type enum is defined in pldata.h, and includes:
//
// - CIG
// - ALCOHOL
// - CAFFEINE
// - COKE
// - SPEED
// - CRACK
// - etc.

// Defined in addiction.h/cpp
// - addict_effect (applies all effects from addiction, and corresponding messages)
// Conversions:
// - enum_to_string( add_type::SPEED ) => "SPEED"
// - addiction_type( "SPEED" ) => add_type::SPEED
// - addiction_type_name( add_type::SPEED ) => "amphetamine"
//
// - addiction_name( SPEED ) => "Amphetamine Withdrawal"
// - addiction_craving( SPEED ) => MORALE_CRAVING_SPEED
// - addiction_text( SPEED ) => "Strength - 1; Intelligence - 1;\nMovement rate reduction ..."

// Addictive Personality trait (ADDICTIVE)
// - 2x strength, adds 1 hour to timer
// - 10 hour penalty in suffering

// Addiction Resistant trait (NONADDICTIVE)
// - 1/2 strength, adds 6 hours to timer
// - 3 hour penalty in suffering

// addiction_type_name returns the name of the addiction, as in "Became addicted to ______"
TEST_CASE( "addiction_type_name", "[addiction][name]" )
{
    // Just a few examples
    CHECK( addiction_type_name( add_type::CIG ) == "nicotine" );
    CHECK( addiction_type_name( add_type::SLEEP ) == "sleeping pills" );
    CHECK( addiction_type_name( add_type::MARLOSS_R ) == "Marloss berries" );
}

// Character::addiction_level returns current intensity level of an addiction, or 0 if not addicted
TEST_CASE( "addiction_level", "[character][addiction]" )
{
    Character &dude = get_player_character();

    // Ensure no initial addiction to nicotine or sleeping pills
    REQUIRE( dude.addiction_level( add_type::CIG ) == 0 );
    REQUIRE( dude.addiction_level( add_type::SLEEP ) == 0 );

    // Each addiction is added separately, with its own intensity level
    // Become addicted to sleeping pills, but not nicotine
    dude.acquire_addiction( add_type::SLEEP, 10 );
    CHECK( dude.addiction_level( add_type::SLEEP ) == 10 );
    CHECK( dude.addiction_level( add_type::CIG ) == 0 );
    // Now become addicted to nicotine, at a lower intensity level
    dude.acquire_addiction( add_type::CIG, 5 );
    CHECK( dude.addiction_level( add_type::CIG ) == 5 );
    // Sleeping pill level remains as it was before
    CHECK( dude.addiction_level( add_type::SLEEP ) == 10 );

    // Each addiction is removed without affecting other addictions
    // Lose nicotine addiction, but keep sleeping pill addiction
    dude.rem_addiction( add_type::CIG );
    CHECK( dude.addiction_level( add_type::CIG ) == 0 );
    CHECK( dude.addiction_level( add_type::SLEEP ) == 10 );
    // Now lose sleeping pill addiction
    dude.rem_addiction( add_type::SLEEP );
    CHECK( dude.addiction_level( add_type::SLEEP ) == 0 );
}

// addict_effect applies all effects from addiction, and generates corresponding messages
// Many effects are randomized, based on intensity
TEST_CASE( "addict_effect", "[character][addiction]" )
{
}

// add_addiction causes a randomized chance to become addicted, depending on strength/intensity
// Character::add_addiction
TEST_CASE( "add_addiction", "[character][addiction]" )
{
    // Addiction strength from min to max (defined in addiction.h)
    // MIN_ADDICTION_LEVEL = 3
    // MAX_ADDICTION_LEVEL = 20
}

// Character::has_addiction returns true only if intensity is minimum or greater
TEST_CASE( "has_addiction", "[character][addiction]" )
{
    Character &dude = get_player_character();

    SECTION( "at less than minimum, has_addiction is false" ) {
        dude.acquire_addiction( add_type::CIG, MIN_ADDICTION_LEVEL - 1 );
        CHECK_FALSE( dude.has_addiction( add_type::CIG ) );
    }

    SECTION( "at minimum level, has_addiction is true" ) {
        dude.acquire_addiction( add_type::CIG, MIN_ADDICTION_LEVEL );
        CHECK( dude.has_addiction( add_type::CIG ) );
    }

    SECTION( "at greater than minimum level, has_addiction is true" ) {
        dude.acquire_addiction( add_type::CIG, MIN_ADDICTION_LEVEL + 1 );
        CHECK( dude.has_addiction( add_type::CIG ) );
    }
}
