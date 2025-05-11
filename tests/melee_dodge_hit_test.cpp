#include <list>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "creature.h"
#include "flag.h"
#include "item.h"
#include "item_location.h"
#include "map_helpers.h"
#include "monster.h"
#include "mtype.h"
#include "player_helpers.h"
#include "type_id.h"

static const itype_id itype_roller_shoes_on( "roller_shoes_on" );
static const itype_id itype_test_roller_blades( "test_roller_blades" );
static const itype_id itype_test_rollerskates( "test_rollerskates" );

static const mtype_id mon_zombie( "mon_zombie" );
static const mtype_id mon_zombie_smoker( "mon_zombie_smoker" );

static const skill_id skill_dodge( "dodge" );

static const trait_id trait_PROF_SKATER( "PROF_SKATER" );

// The test cases below cover polymorphic functions related to melee hit and dodge rates
// for the Character, player, and monster classes, including:
//
// - Character::get_hit_base, monster::get_hit_base
// - Character::get_dodge_base, monster::get_dodge_base
// - player::get_dodge, monster::get_dodge

// Return the avatar's `get_hit_base` with a given DEX stat.
static float hit_base_with_dex( avatar &dummy, int dexterity )
{
    clear_character( dummy );
    dummy.dex_max = dexterity;

    return dummy.get_hit_base();
}

// Return the avatar's `get_dodge_base` with the given DEX stat and dodge skill.
static float dodge_base_with_dex_and_skill( avatar &dummy, int dexterity, int dodge_skill )
{
    clear_character( dummy );
    dummy.dex_max = dexterity;
    dummy.set_skill_level( skill_dodge, dodge_skill );

    return dummy.get_dodge_base();
}

// Return the Creature's `get_dodge` with the given effect.
static float dodge_with_effect( Creature &critter, const std::string &effect_name )
{
    // Set one effect and leave other attributes alone
    critter.clear_effects();
    critter.add_effect( efftype_id( effect_name ), 1_hours );

    return critter.get_dodge();
}

// Return the avatar's `get_dodge` while wearing a single item of clothing.
static float dodge_wearing_item( avatar &dummy, item &clothing )
{
    // Get nekkid and wear just this one item

    std::list<item> temp;
    while( true ) {
        item &it = dummy.i_at( -2 );

        if( it.is_null() ) {
            break;
        }
        if( !dummy.takeoff( item_location( *dummy.as_character(), &it ), &temp ) ) {
            break;
        }
    }

    dummy.wear_item( clothing );

    return dummy.get_dodge();
}

TEST_CASE( "monster_get_hit_base", "[monster][melee][hit]" )
{
    clear_map();

    SECTION( "monster get_hit_base is equal to melee skill level" ) {
        monster zed( mon_zombie );
        CHECK( zed.get_hit_base() == zed.type->melee_skill );
    }
}

TEST_CASE( "Character_get_hit_base", "[character][melee][hit][dex]" )
{
    clear_map();

    avatar &dummy = get_avatar();
    clear_character( dummy );
    dummy.set_dodges_left( 1 );

    SECTION( "character get_hit_base increases by 1/4 for each point of DEX" ) {
        CHECK( hit_base_with_dex( dummy, 1 ) == 0.25f );
        CHECK( hit_base_with_dex( dummy, 2 ) == 0.5f );
        CHECK( hit_base_with_dex( dummy, 4 ) == 1.0f );
        CHECK( hit_base_with_dex( dummy, 6 ) == 1.5f );
        CHECK( hit_base_with_dex( dummy, 8 ) == 2.0f );
        CHECK( hit_base_with_dex( dummy, 10 ) == 2.5f );
        CHECK( hit_base_with_dex( dummy, 12 ) == 3.0f );
    }
}

TEST_CASE( "monster_get_dodge_base", "[monster][melee][dodge]" )
{
    clear_map();

    SECTION( "monster get_dodge_base is equal to dodge skill level" ) {
        monster smoker( mon_zombie_smoker );
        CHECK( smoker.get_dodge_base() == smoker.type->sk_dodge );
    }
}

TEST_CASE( "Character_get_dodge_base", "[character][melee][dodge][dex][skill]" )
{
    clear_map();

    avatar &dummy = get_avatar();
    clear_character( dummy );

    // Character::get_dodge_base is simply DEXTERITY / 2 + DODGE_SKILL
    // Even with average dexterity, you can become really good at dodging
    GIVEN( "character has 8 DEX and no dodge skill" ) {
        THEN( "base dodge is 4" ) {
            CHECK( dodge_base_with_dex_and_skill( dummy, 8, 0 ) == 4.0f );
        }

        WHEN( "their dodge skill increases to 2" ) {
            THEN( "base dodge is 6" ) {
                CHECK( dodge_base_with_dex_and_skill( dummy, 8, 2 ) == 6.0f );
            }
        }

        AND_WHEN( "their dodge skill increases to 8" ) {
            THEN( "base dodge is 12" ) {
                CHECK( dodge_base_with_dex_and_skill( dummy, 8, 8 ) == 12.0f );
            }
        }
    }

    // More generally

    SECTION( "character get_dodge_base increases by 1/2 for each point of DEX" ) {
        CHECK( dodge_base_with_dex_and_skill( dummy, 1, 0 ) == 0.5f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 2, 0 ) == 1.0f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 3, 0 ) == 1.5f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 4, 0 ) == 2.0f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 5, 0 ) == 2.5f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 6, 0 ) == 3.0f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 7, 0 ) == 3.5f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 8, 0 ) == 4.0f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 9, 0 ) == 4.5f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 10, 0 ) == 5.0f );
    }

    SECTION( "character get_dodge_base increases by 1 for each level of dodge skill" ) {
        CHECK( dodge_base_with_dex_and_skill( dummy, 8, 0 ) == 4.0f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 8, 1 ) == 5.0f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 8, 2 ) == 6.0f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 8, 3 ) == 7.0f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 8, 4 ) == 8.0f );

        CHECK( dodge_base_with_dex_and_skill( dummy, 6, 2 ) == 5.0f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 6, 4 ) == 7.0f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 6, 6 ) == 9.0f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 6, 8 ) == 11.0f );

        CHECK( dodge_base_with_dex_and_skill( dummy, 10, 7 ) == 12.0f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 10, 8 ) == 13.0f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 10, 9 ) == 14.0f );
        CHECK( dodge_base_with_dex_and_skill( dummy, 10, 10 ) == 15.0f );
    }
}

TEST_CASE( "monster_get_dodge_with_effects", "[monster][melee][dodge][effect]" )
{
    clear_map();

    monster zombie( mon_zombie_smoker );

    const float base_dodge = zombie.get_dodge_base();
    REQUIRE( base_dodge > 0 );

    // Monsters don't have all the status effects of characters and can't be grabbed,
    // but a few things may affect their ability to dodge.

    SECTION( "no effects or bonuses: base dodge" ) {
        zombie.clear_effects();
        CHECK( zombie.get_dodge() == base_dodge );
    }

    SECTION( "downed: cannot dodge" ) {
        CHECK( dodge_with_effect( zombie, "downed" ) == 0.0f );
    }

    SECTION( "trapped or tied: 1/2 dodge" ) {
        CHECK( dodge_with_effect( zombie, "beartrap" ) == base_dodge / 2 );
        CHECK( dodge_with_effect( zombie, "tied" ) == base_dodge / 2 );
    }

    SECTION( "unstable footing: 1/4 dodge" ) {
        CHECK( dodge_with_effect( zombie, "bouldering" ) == base_dodge / 4 );
    }
}

TEST_CASE( "player_get_dodge", "[player][melee][dodge]" )
{
    clear_map();

    avatar &dummy = get_avatar();
    clear_character( dummy );
    dodge_base_with_dex_and_skill( dummy, 10, 10 );
    dummy.set_dodges_left( 1 );

    const float base_dodge = dummy.get_dodge_base();

    SECTION( "speed below 100 linearly decreases dodge" ) {
        dummy.set_speed_base( 90 );
        CHECK( dummy.get_dodge() == Approx( 0.9 * base_dodge ) );
        dummy.set_speed_base( 75 );
        CHECK( dummy.get_dodge() == Approx( 0.75 * base_dodge ) );
        dummy.set_speed_base( 50 );
        CHECK( dummy.get_dodge() == Approx( 0.5 * base_dodge ) );
        dummy.set_speed_base( 25 );
        CHECK( dummy.get_dodge() == Approx( 0.25 * base_dodge ) );
    }
}

TEST_CASE( "player_get_dodge_with_effects", "[player][melee][dodge][effect]" )
{
    clear_map();

    avatar &dummy = get_avatar();
    clear_character( dummy );
    dodge_base_with_dex_and_skill( dummy, 8, 4 );

    // Compare all effects against base dodge ability
    const float base_dodge = dummy.get_dodge_base();

    SECTION( "no effects or bonuses: base dodge" ) {
        dummy.clear_effects();
        CHECK( dummy.get_dodge() == base_dodge );
    }

    SECTION( "unconscious or winded: cannot dodge" ) {
        CHECK( dodge_with_effect( dummy, "sleep" ) == 0.0f );
        CHECK( dodge_with_effect( dummy, "lying_down" ) == 0.0f );
        CHECK( dodge_with_effect( dummy, "npc_suspend" ) == 0.0f );
        CHECK( dodge_with_effect( dummy, "narcosis" ) == 0.0f );
        CHECK( dodge_with_effect( dummy, "winded" ) == 0.0f );
    }

    SECTION( "trapped: 1/2 dodge" ) {
        CHECK( dodge_with_effect( dummy, "beartrap" ) == base_dodge / 2 );
    }

    SECTION( "unstable footing: 1/4 dodge" ) {
        // FIXME: Margin is flat instead of relative %.
        CHECK( dodge_with_effect( dummy, "bouldering" ) == Approx( base_dodge / 4 ).margin( 0.1f ) );
    }

    SECTION( "skating: amateur or pro?" ) {
        item skates( itype_test_rollerskates );
        item blades( itype_test_roller_blades );
        item heelys( itype_roller_shoes_on );

        REQUIRE( skates.has_flag( flag_ROLLER_QUAD ) );
        REQUIRE( blades.has_flag( flag_ROLLER_INLINE ) );
        REQUIRE( heelys.has_flag( flag_ROLLER_ONE ) );

        SECTION( "amateur skater: 1/5 dodge" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_PROF_SKATER ) );

            CHECK( dodge_wearing_item( dummy, skates ) == base_dodge / 5 );
            CHECK( dodge_wearing_item( dummy, blades ) == base_dodge / 5 );
            CHECK( dodge_wearing_item( dummy, heelys ) == base_dodge / 5 );
        }

        SECTION( "professional skater: 1/2 dodge" ) {
            dummy.toggle_trait( trait_PROF_SKATER );
            REQUIRE( dummy.has_trait( trait_PROF_SKATER ) );

            CHECK( dodge_wearing_item( dummy, skates ) == base_dodge / 2 );
            CHECK( dodge_wearing_item( dummy, blades ) == base_dodge / 2 );
            CHECK( dodge_wearing_item( dummy, heelys ) == base_dodge / 2 );
        }
    }
}

TEST_CASE( "player_get_dodge_stamina_effects", "[player][melee][dodge][stamina]" )
{
    avatar &dummy = get_avatar();
    clear_character( dummy );
    dodge_base_with_dex_and_skill( dummy, 8, 0 );

    SECTION( "8/8/8/8, no skills, unencumbered" ) {
        const int stamina_max = dummy.get_stamina_max();

        SECTION( "100% stamina" ) {
            CHECK( dummy.get_dodge() == Approx( 4.0f ).margin( 0.001 ) );
        }

        SECTION( "75% stamina" ) {
            dummy.set_stamina( .75 * stamina_max );
            CHECK( dummy.get_dodge() == Approx( 3.8f ).margin( 0.1 ) );
        }

        SECTION( "50% stamina" ) {
            dummy.set_stamina( .5 * stamina_max );
            CHECK( dummy.get_dodge() == Approx( 2.0f ).margin( 0.1 ) );
        }

        SECTION( "40% stamina" ) {
            dummy.set_stamina( .4 * stamina_max );
            CHECK( dummy.get_dodge() == Approx( 1.0f ).margin( 0.1 ) );
        }

        SECTION( "30% stamina" ) {
            dummy.set_stamina( .3 * stamina_max );
            CHECK( dummy.get_dodge() == Approx( 0.0f ).margin( 0.1 ) );
        }

        SECTION( "20% stamina" ) {
            dummy.set_stamina( .2 * stamina_max );
            CHECK( dummy.get_dodge() == Approx( 0.0f ).margin( 0.1 ) );
        }

        SECTION( "10% stamina" ) {
            dummy.set_stamina( .1 * stamina_max );
            CHECK( dummy.get_dodge() == Approx( 0.0f ).margin( 0.1 ) );
        }

        SECTION( "0% stamina" ) {
            dummy.set_stamina( 0 );
            CHECK( dummy.get_dodge() == Approx( 0.0f ).margin( 0.1 ) );
        }
    }
}
