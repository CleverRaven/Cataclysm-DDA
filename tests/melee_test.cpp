#include <cstddef>
#include <sstream>
#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "creature.h"
#include "game_constants.h"
#include "item.h"
#include "monattack.h"
#include "monster.h"
#include "npc.h"
#include "point.h"
#include "type_id.h"

static const efftype_id effect_sleep( "sleep" );

static const mtype_id mon_manhack( "mon_manhack" );
static const mtype_id mon_zombie( "mon_zombie" );
static const mtype_id mon_zombie_hulk( "mon_zombie_hulk" );

static float brute_probability( Creature &attacker, Creature &target, const size_t iters )
{
    // Note: not using deal_melee_attack because it trains dodge, which causes problems here
    size_t hits = 0;
    for( size_t i = 0; i < iters; i++ ) {
        const int spread = attacker.hit_roll() - target.dodge_roll();
        if( spread > 0 ) {
            hits++;
        }
    }

    return static_cast<float>( hits ) / iters;
}

static float brute_special_probability( monster &attacker, Creature &target, const size_t iters )
{
    size_t hits = 0;
    for( size_t i = 0; i < iters; i++ ) {
        if( !mattack::dodge_check( &attacker, &target ) ) {
            hits++;
        }
    }

    return static_cast<float>( hits ) / iters;
}

static std::string full_attack_details( const Character &dude )
{
    std::stringstream ss;
    ss << "Details for " << dude.disp_name() << std::endl;
    ss << "get_hit() == " << dude.get_hit() << std::endl;
    ss << "get_melee_hit_base() == " << dude.get_melee_hit_base() << std::endl;
    ss << "get_hit_weapon() == " << dude.get_hit_weapon( dude.get_wielded_item() ) << std::endl;
    return ss.str();
}

static std::string percent_string( const float f )
{
    // Using stringstream for prettier precision printing
    std::stringstream ss;
    ss << 100.0f * f << "%";
    return ss.str();
}

static void check_near( float prob, const float expected, const float tolerance )
{
    const float low = expected - tolerance;
    const float high = expected + tolerance;
    THEN( "The chance to hit is between " + percent_string( low ) +
          " and " + percent_string( high ) ) {
        REQUIRE( prob > low );
        REQUIRE( prob < high );
    }
}

static const int num_iters = 10000;

static constexpr tripoint dude_pos( HALF_MAPSIZE_X, HALF_MAPSIZE_Y, 0 );

TEST_CASE( "Character attacking a zombie", "[.melee]" )
{
    monster zed( mon_zombie );
    INFO( "Zombie has get_dodge() == " + std::to_string( zed.get_dodge() ) );

    SECTION( "8/8/8/8, no skills, unarmed" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        const float prob = brute_probability( dude, zed, num_iters );
        INFO( full_attack_details( dude ) );
        check_near( prob, 0.6f, 0.1f );
    }

    SECTION( "8/8/8/8, 3 all skills, plank" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 3, 8, 8, 8, 8 );
        dude.set_wielded_item( item( "2x4" ) );
        const float prob = brute_probability( dude, zed, num_iters );
        INFO( full_attack_details( dude ) );
        check_near( prob, 0.8f, 0.05f );
    }

    SECTION( "10/10/10/10, 8 all skills, katana" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 8, 10, 10, 10, 10 );
        dude.set_wielded_item( item( "katana" ) );
        const float prob = brute_probability( dude, zed, num_iters );
        INFO( full_attack_details( dude ) );
        check_near( prob, 0.975f, 0.025f );
    }
}

TEST_CASE( "Character attacking a manhack", "[.melee]" )
{
    monster manhack( mon_manhack );
    INFO( "Manhack has get_dodge() == " + std::to_string( manhack.get_dodge() ) );

    SECTION( "8/8/8/8, no skills, unarmed" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        const float prob = brute_probability( dude, manhack, num_iters );
        INFO( full_attack_details( dude ) );
        check_near( prob, 0.2f, 0.05f );
    }

    SECTION( "8/8/8/8, 3 all skills, plank" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 3, 8, 8, 8, 8 );
        dude.set_wielded_item( item( "2x4" ) );
        const float prob = brute_probability( dude, manhack, num_iters );
        INFO( full_attack_details( dude ) );
        check_near( prob, 0.4f, 0.05f );
    }

    SECTION( "10/10/10/10, 8 all skills, katana" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 8, 10, 10, 10, 10 );
        dude.set_wielded_item( item( "katana" ) );
        const float prob = brute_probability( dude, manhack, num_iters );
        INFO( full_attack_details( dude ) );
        check_near( prob, 0.7f, 0.05f );
    }
}

TEST_CASE( "Zombie attacking a character", "[.melee]" )
{
    monster zed( mon_zombie );
    INFO( "Zombie has get_hit() == " + std::to_string( zed.get_hit() ) );

    SECTION( "8/8/8/8, no skills, unencumbered" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        const float prob = brute_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        THEN( "Character has no significant dodge bonus or penalty" ) {
            REQUIRE( dude.get_dodge_bonus() < 0.5f );
            REQUIRE( dude.get_dodge_bonus() > -0.5f );
        }

        THEN( "Character's dodge skill is roughly equal to zombie's attack skill" ) {
            REQUIRE( dude.get_dodge() < zed.get_hit() + 0.5f );
            REQUIRE( dude.get_dodge() > zed.get_hit() - 0.5f );
        }

        check_near( prob, 0.5f, 0.05f );
    }

    SECTION( "10/10/10/10, 3 all skills, good cotton armor" ) {
        standard_npc dude( "TestCharacter", dude_pos,
        { "hoodie", "jeans", "long_underpants", "long_undertop", "longshirt" },
        3, 10, 10, 10, 10 );
        const float prob = brute_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        check_near( prob, 0.2f, 0.05f );
    }

    SECTION( "10/10/10/10, 8 all skills, survivor suit" ) {
        standard_npc dude( "TestCharacter", dude_pos, { "survivor_suit" }, 8, 10, 10, 10, 10 );
        const float prob = brute_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        check_near( prob, 0.025f, 0.0125f );
    }
}

TEST_CASE( "Manhack attacking a character", "[.melee]" )
{
    monster manhack( mon_manhack );
    INFO( "Manhack has get_hit() == " + std::to_string( manhack.get_hit() ) );

    SECTION( "8/8/8/8, no skills, unencumbered" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        const float prob = brute_probability( manhack, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        THEN( "Character has no significant dodge bonus or penalty" ) {
            REQUIRE( dude.get_dodge_bonus() < 0.5f );
            REQUIRE( dude.get_dodge_bonus() > -0.5f );
        }

        check_near( prob, 0.9f, 0.05f );
    }

    SECTION( "10/10/10/10, 3 all skills, good cotton armor" ) {
        standard_npc dude( "TestCharacter", dude_pos,
        { "hoodie", "jeans", "long_underpants", "long_undertop", "longshirt" },
        3, 10, 10, 10, 10 );
        const float prob = brute_probability( manhack, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        check_near( prob, 0.6f, 0.05f );
    }

    SECTION( "10/10/10/10, 8 all skills, survivor suit" ) {
        standard_npc dude( "TestCharacter", dude_pos, { "survivor_suit" }, 8, 10, 10, 10, 10 );
        const float prob = brute_probability( manhack, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        check_near( prob, 0.25f, 0.05f );
    }
}

TEST_CASE( "Hulk smashing a character", "[.], [melee], [monattack]" )
{
    monster zed( mon_zombie_hulk );
    INFO( "Hulk has get_hit() == " + std::to_string( zed.get_hit() ) );

    SECTION( "8/8/8/8, no skills, unencumbered" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        const float prob = brute_special_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        THEN( "Character has no significant dodge bonus or penalty" ) {
            REQUIRE( dude.get_dodge_bonus() < 0.5f );
            REQUIRE( dude.get_dodge_bonus() > -0.5f );
        }

        check_near( prob, 0.95f, 0.05f );
    }

    SECTION( "10/10/10/10, 3 all skills, good cotton armor" ) {
        standard_npc dude( "TestCharacter", dude_pos,
        { "hoodie", "jeans", "long_underpants", "long_undertop", "longshirt" },
        3, 10, 10, 10, 10 );
        const float prob = brute_special_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        check_near( prob, 0.75f, 0.05f );
    }

    SECTION( "10/10/10/10, 8 all skills, survivor suit" ) {
        standard_npc dude( "TestCharacter", dude_pos, { "survivor_suit" }, 8, 10, 10, 10, 10 );
        const float prob = brute_special_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        check_near( prob, 0.2f, 0.05f );
    }
}

TEST_CASE( "Charcter can dodge" )
{
    standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
    monster zed( mon_zombie );

    dude.clear_effects();
    REQUIRE( dude.get_dodge() > 0.0 );

    const int dodges_left = dude.dodges_left;
    for( int i = 0; i < 10000; ++i ) {
        dude.deal_melee_attack( &zed, 1 );
        if( dodges_left < dude.dodges_left ) {
            CHECK( dodges_left < dude.dodges_left );
            break;
        }
    }
}

TEST_CASE( "Incapacited character can't dodge" )
{
    standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
    monster zed( mon_zombie );

    dude.clear_effects();
    dude.add_effect( effect_sleep, 1_hours );
    REQUIRE( dude.get_dodge() == 0.0 );

    const int dodges_left = dude.dodges_left;
    for( int i = 0; i < 10000; ++i ) {
        dude.deal_melee_attack( &zed, 1 );
        CHECK( dodges_left == dude.dodges_left );
    }
}
