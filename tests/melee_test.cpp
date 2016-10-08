#include "catch/catch.hpp"

#include "npc.h"
#include "monster.h"
#include "monattack.h"
#include "game.h"

#include <string>
#include <sstream>

static float brute_probability( Creature& attacker, Creature &target, size_t iters )
{
    // Note: not using deal_melee_attack because it trains dodge, which causes problems here
    size_t hits = 0;
    for( size_t i = 0; i < iters; i++ ) {
        int spread = attacker.hit_roll() - target.dodge_roll();
        if( spread > 0 ) {
            hits++;
        }
    }

    return (float)hits / iters;
}

static float brute_special_probability( monster& attacker, Creature &target, size_t iters )
{
    size_t hits = 0;
    for( size_t i = 0; i < iters; i++ ) {
        if( !mattack::dodge_check( &attacker, &target ) ) {
            hits++;
        }
    }

    return (float)hits / iters;
}

static std::string full_attack_details( const player &dude )
{
    std::stringstream ss;
    ss << "Details for " << dude.disp_name() << std::endl;
    ss << "get_hit() == " << dude.get_hit() << std::endl;
    ss << "get_hit_base() == " << dude.get_hit_base() << std::endl;
    ss << "get_hit_weapon() == " << dude.get_hit_weapon( dude.weapon ) << std::endl;
    return ss.str();
}

const int num_iters = 1000;

TEST_CASE("Character attacking a zombie", "[melee]") {
    monster zed( mtype_id( "mon_zombie" ) );
    INFO( "Zombie has get_dodge() == " + std::to_string( zed.get_dodge() ) );

    SECTION("8/8/8/8, no skills, unarmed") {
        standard_npc dude( "TestCharacter", {}, 0, 8, 8, 8, 8 );
        float prob = brute_probability( dude, zed, num_iters );
        INFO( full_attack_details( dude ) );
        THEN( "The chance to hit is near 60%" ) {
            REQUIRE( prob > 0.5f );
            REQUIRE( prob < 0.7f );
        }
    }

    SECTION("8/8/8/8, 3 all skills, two-by-four") {
        standard_npc dude( "TestCharacter", {}, 3, 8, 8, 8, 8 );
        dude.weapon = item( "2x4" );
        float prob = brute_probability( dude, zed, num_iters );
        INFO( full_attack_details( dude ) );
        THEN( "The chance to hit is near 70%" ) {
            REQUIRE( prob > 0.6f );
            REQUIRE( prob < 0.8f );
        }
    }

    SECTION("10/10/10/10, 8 all skills, katana") {
        standard_npc dude( "TestCharacter", {}, 8, 10, 10, 10, 10 );
        dude.weapon = item( "katana" );
        float prob = brute_probability( dude, zed, num_iters );
        INFO( full_attack_details( dude ) );
        THEN( "The chance to hit is near 95%" ) {
            REQUIRE( prob > 0.9f );
        }
    }
}

TEST_CASE("Character attacking a manhack", "[melee]") {
    monster manhack( mtype_id( "mon_manhack" ) );
    INFO( "Manhack has get_dodge() == " + std::to_string( manhack.get_dodge() ) );

    SECTION("8/8/8/8, no skills, unarmed") {
        standard_npc dude( "TestCharacter", {}, 0, 8, 8, 8, 8 );
        float prob = brute_probability( dude, manhack, num_iters );
        INFO( "Has get_hit() == " + std::to_string( dude.get_hit() ) );
        THEN( "The chance to hit is near 15%" ) {
            REQUIRE( prob > 0.1f );
            REQUIRE( prob < 0.2f );
        }
    }

    SECTION("8/8/8/8, 3 all skills, two-by-four") {
        standard_npc dude( "TestCharacter", {}, 3, 8, 8, 8, 8 );
        dude.weapon = item( "2x4" );
        float prob = brute_probability( dude, manhack, num_iters );
        INFO( "Has get_hit() == " + std::to_string( dude.get_hit() ) );
        THEN( "The chance to hit is near 30%" ) {
            REQUIRE( prob > 0.3f );
            REQUIRE( prob < 0.5f );
        }
    }

    SECTION("10/10/10/10, 8 all skills, katana") {
        standard_npc dude( "TestCharacter", {}, 8, 10, 10, 10, 10 );
        dude.weapon = item( "katana" );
        float prob = brute_probability( dude, manhack, num_iters );
        INFO( "Has get_hit() == " + std::to_string( dude.get_hit() ) );
        THEN( "The chance to hit is near 70%" ) {
            REQUIRE( prob > 0.6f );
            REQUIRE( prob < 0.8f );
        }
    }
}

TEST_CASE("Zombie attacking a character", "[melee]") {
    monster zed( mtype_id( "mon_zombie" ) );
    INFO( "Zombie has get_hit() == " + std::to_string( zed.get_hit() ) );

    SECTION("8/8/8/8, no skills, unencumbered") {
        standard_npc dude( "TestCharacter", {}, 0, 8, 8, 8, 8 );
        float prob = brute_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        THEN( "Character has no significant dodge bonus or penalty" ) {
            REQUIRE( dude.get_dodge_bonus() < 0.5f );
            REQUIRE( dude.get_dodge_bonus() > -0.5f );
        }

        THEN( "Character's dodge skill is roughly equal to zombie's attack skill" ) {
            REQUIRE( dude.get_dodge() < zed.get_hit() + 0.5f );
            REQUIRE( dude.get_dodge() > zed.get_hit() - 0.5f );
        }

        THEN( "The chance to hit is near 50%" ) {
            REQUIRE( prob > 0.4f );
            REQUIRE( prob < 0.6f );
        }
    }

    SECTION("10/10/10/10, 3 all skills, good cotton armor") {
        standard_npc dude( "TestCharacter", { "hoodie", "jeans", "long_underpants", "long_undertop", "longshirt" }, 3, 10, 10, 10, 10 );
        float prob = brute_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        THEN( "The chance to hit is near 20%" ) {
            REQUIRE( prob > 0.1f );
            REQUIRE( prob < 0.3f );
        }
    }

    SECTION("10/10/10/10, 8 all skills, survivor suit") {
        standard_npc dude( "TestCharacter", { "survivor_suit" }, 8, 10, 10, 10, 10 );
        float prob = brute_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        THEN( "The chance to hit is near 5%" ) {
            REQUIRE( prob > 0.025f );
            REQUIRE( prob < 0.1f );
        }
    }
}

TEST_CASE("Manhack attacking a character", "[melee]") {
    monster manhack( mtype_id( "mon_manhack" ) );
    INFO( "Manhack has get_hit() == " + std::to_string( manhack.get_hit() ) );

    SECTION("8/8/8/8, no skills, unencumbered") {
        standard_npc dude( "TestCharacter", {}, 0, 8, 8, 8, 8 );
        float prob = brute_probability( manhack, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        THEN( "Character has no significant dodge bonus or penalty" ) {
            REQUIRE( dude.get_dodge_bonus() < 0.5f );
            REQUIRE( dude.get_dodge_bonus() > -0.5f );
        }

        THEN( "The chance to hit is near 80%" ) {
            REQUIRE( prob > 0.7f );
            REQUIRE( prob < 0.9f );
        }
    }

    SECTION("10/10/10/10, 3 all skills, good cotton armor") {
        standard_npc dude( "TestCharacter", { "hoodie", "jeans", "long_underpants", "long_undertop", "longshirt" }, 3, 10, 10, 10, 10 );
        float prob = brute_probability( manhack, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        THEN( "The chance to hit is near 60%" ) {
            REQUIRE( prob > 0.5f );
            REQUIRE( prob < 0.7f );
        }
    }

    SECTION("10/10/10/10, 8 all skills, survivor suit") {
        standard_npc dude( "TestCharacter", { "survivor_suit" }, 8, 10, 10, 10, 10 );
        float prob = brute_probability( manhack, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        THEN( "The chance to hit is near 30%" ) {
            REQUIRE( prob > 0.2f );
            REQUIRE( prob < 0.4f );
        }
    }
}

TEST_CASE("Hulk smashing a character", "[melee], [monattack]") {
    monster zed( mtype_id( "mon_zombie_hulk" ) );
    INFO( "Hulk has get_hit() == " + std::to_string( zed.get_hit() ) );

    SECTION("8/8/8/8, no skills, unencumbered") {
        standard_npc dude( "TestCharacter", {}, 0, 8, 8, 8, 8 );
        float prob = brute_special_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        THEN( "Character has no significant dodge bonus or penalty" ) {
            REQUIRE( dude.get_dodge_bonus() < 0.5f );
            REQUIRE( dude.get_dodge_bonus() > -0.5f );
        }

        THEN( "The chance to hit is near 95%" ) {
            REQUIRE( prob > 0.9f );
        }
    }

    SECTION("10/10/10/10, 3 all skills, good cotton armor") {
        standard_npc dude( "TestCharacter", { "hoodie", "jeans", "long_underpants", "long_undertop", "longshirt" }, 3, 10, 10, 10, 10 );
        float prob = brute_special_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        THEN( "The chance to hit is near 70%" ) {
            REQUIRE( prob > 0.6f );
            REQUIRE( prob < 0.8f );
        }
    }

    SECTION("10/10/10/10, 8 all skills, survivor suit") {
        standard_npc dude( "TestCharacter", { "survivor_suit" }, 8, 10, 10, 10, 10 );
        float prob = brute_special_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        THEN( "The chance to hit is near 20%" ) {
            REQUIRE( prob > 0.1f );
            REQUIRE( prob < 0.3f );
        }
    }
}
