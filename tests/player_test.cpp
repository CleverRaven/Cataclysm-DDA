#include <array>
#include <functional>
#include <iosfwd>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "item.h"
#include "player_helpers.h"
#include "type_id.h"
#include "weather.h"

// Set the stage for a particular ambient and target temperature and run update_bodytemp() until
// core body temperature settles.
static void temperature_check( Character *p, const int ambient_temp,
                               const units::temperature target_temp )
{
    p->set_body();
    get_weather().temperature = units::from_fahrenheit( ambient_temp );
    p->set_all_parts_temp_cur( BODYTEMP_NORM );
    p->set_all_parts_temp_conv( BODYTEMP_NORM );

    units::temperature prev_temp = 0_K;
    units::temperature_delta prev_diff = 0_C_delta;
    for( int i = 0; i < 10000; i++ ) {
        const units::temperature torso_temp_cur = p->get_part_temp_cur( bodypart_id( "torso" ) );
        if( prev_diff != prev_temp - torso_temp_cur ) {
            prev_diff = prev_temp - torso_temp_cur;
        } else if( prev_temp == torso_temp_cur ) {
            break;
        }
        prev_temp = torso_temp_cur;
        p->update_bodytemp();
    }
    const units::temperature high = target_temp + 0.2_C_delta;
    const units::temperature low = target_temp - 0.2_C_delta;
    CHECK( low < p->get_part_temp_cur( bodypart_id( "torso" ) ) );
    CHECK( high > p->get_part_temp_cur( bodypart_id( "torso" ) ) );
}

// Set the stage for a particular ambient and target temperature and run update_bodytemp() until
// core body temperature settles.
static void temperature_and_sweat_check( Character *p, const int ambient_temp,
        const units::temperature target_temp )
{

    weather_manager &weather = get_weather();

    weather.temperature = units::from_fahrenheit( ambient_temp );

    for( int i = 0; i < 1000; i++ ) {
        p->process_effects();
        p->update_bodytemp();
        p->update_body_wetness( *weather.weather_precise );
    }
    const units::temperature high = target_temp + 0.2_C_delta;
    const units::temperature low = target_temp - 0.2_C_delta;
    CHECK( low < p->get_part_temp_cur( bodypart_id( "torso" ) ) );
    CHECK( high > p->get_part_temp_cur( bodypart_id( "torso" ) ) );
}

static void equip_clothing( Character *p, const std::string &clothing )
{
    const item article( clothing, calendar::turn_zero );
    p->wear_item( article );
}

// Run the tests for each of the temperature setpoints.
// ambient_temps MUST have 7 values or we'll segfault.
static void test_temperature_spread( Character *p, const std::array<int, 7> &ambient_temps )
{
    temperature_check( p, ambient_temps[0], BODYTEMP_FREEZING );
    temperature_check( p, ambient_temps[1], BODYTEMP_VERY_COLD );
    temperature_check( p, ambient_temps[2], BODYTEMP_COLD );
    temperature_check( p, ambient_temps[3], BODYTEMP_NORM );
    temperature_check( p, ambient_temps[4], BODYTEMP_HOT );
    temperature_check( p, ambient_temps[5], BODYTEMP_VERY_HOT );
    temperature_check( p, ambient_temps[6], BODYTEMP_SCORCHING );
}

TEST_CASE( "player_body_temperatures_converge_on_expected_values", "[.bodytemp]" )
{

    Character &dummy = get_player_character();

    // Strip off any potentially encumbering clothing.
    dummy.remove_worn_items_with( []( item & ) {
        return true;
    } );

    // See http://personal.cityu.edu.hk/~bsapplec/heat.htm for temperature basis.
    // As we aren't modeling metabolic rate, assume 2 METS when not sleeping.
    // Obviously though 0.7 METS when sleeping is called for.

    // I'm not sure how to apply +1 METS as a temperature offset,
    // treating it as a 12C/54F boost across the board

    // The commented out tests are the ideals calculated from the above site.
    // The enabled tests are the current status quo just to check for regressions.

    SECTION( "Nude target temperatures." ) {
        //test_temperature_spread( &dummy, { 19, 34, 49, 64, 79, 94, 109 } );
        test_temperature_spread( &dummy, {{ -12, 15, 40, 64, 78, 90, 101 }} );
    }

    SECTION( "Lightly clothed target temperatures" ) {
        equip_clothing( &dummy, "hat_ball" );
        equip_clothing( &dummy, "bandana" );
        equip_clothing( &dummy, "tshirt" );
        equip_clothing( &dummy, "gloves_fingerless" );
        equip_clothing( &dummy, "jeans" );
        equip_clothing( &dummy, "socks" );
        equip_clothing( &dummy, "sneakers" );

        //test_temperature_spread( &dummy, { -3, 12, 27, 42, 57, 72, 87 } );
        test_temperature_spread( &dummy, {{ -17, 10, 35, 62, 77, 90, 100 }} );
    }

    SECTION( "Heavily clothed target temperatures" ) {
        equip_clothing( &dummy, "hat_knit" );
        equip_clothing( &dummy, "tshirt" );
        equip_clothing( &dummy, "vest" );
        equip_clothing( &dummy, "trenchcoat" );
        equip_clothing( &dummy, "gloves_wool" );
        equip_clothing( &dummy, "long_underpants" );
        equip_clothing( &dummy, "pants_army" );
        equip_clothing( &dummy, "socks_wool" );
        equip_clothing( &dummy, "boots" );

        //test_temperature_spread( &dummy, { -25, -10, 5, 20, 35, 50, 65 } );
        test_temperature_spread( &dummy, {{ -39, -14, 17, 46, 70, 84, 96 }} );
    }

    SECTION( "Arctic gear target temperatures" ) {
        equip_clothing( &dummy, "balclava" );
        equip_clothing( &dummy, "goggles_ski" );
        equip_clothing( &dummy, "hat_hunting" );
        equip_clothing( &dummy, "under_armor" );
        equip_clothing( &dummy, "vest" );
        equip_clothing( &dummy, "coat_winter" );
        equip_clothing( &dummy, "gloves_liner" );
        equip_clothing( &dummy, "gloves_winter" );
        equip_clothing( &dummy, "long_underpants" );
        equip_clothing( &dummy, "pants_fur" );
        equip_clothing( &dummy, "socks_wool" );
        equip_clothing( &dummy, "boots_winter" );

        //test_temperature_spread( &dummy, { -47, -32, -17, -2, 13, 28, 43 } );
        test_temperature_spread( &dummy, {{ -115, -87, -54, -6, 36, 64, 80 }} );
    }
}

TEST_CASE( "sweating", "[char][suffer][.bodytemp]" )
{
    avatar &dummy = get_avatar();
    clear_character( dummy );

    // three different materials of breathability, same warmth
    item fur_jumper( "test_jumpsuit_fur" );
    item lycra_jumper( "test_jumpsuit_lycra" );
    item cotton_jumper( "test_jumpsuit_cotton" );

    GIVEN( "avatar wears outfit and sweats for an hour" ) {
        WHEN( "wearing fur" ) {
            dummy.clear_worn();
            dummy.wear_item( fur_jumper, false );

            temperature_and_sweat_check( &dummy, 100, 43.2_C );
        }
        WHEN( "wearing cotton" ) {
            dummy.clear_worn();
            dummy.wear_item( cotton_jumper, false );

            temperature_and_sweat_check( &dummy, 100, 42.8_C );
        }
        WHEN( "wearing lycra" ) {
            dummy.clear_worn();
            dummy.wear_item( lycra_jumper, false );

            temperature_and_sweat_check( &dummy, 100, 41_C );
        }

    }
}
