#include "bodypart.h"
#include "cata_catch.h"
#include "character.h"
#include "character_modifier.h"
#include "damage.h"
#include "flag.h"
#include "item.h"
#include "itype.h"
#include "magic_enchantment.h"
#include "mutation.h"
#include "player_helpers.h"

static const bodypart_str_id body_part_test_arm_l( "test_arm_l" );
static const bodypart_str_id body_part_test_arm_r( "test_arm_r" );
static const bodypart_str_id body_part_test_bird_foot_l( "test_bird_foot_l" );
static const bodypart_str_id body_part_test_bird_foot_r( "test_bird_foot_r" );
static const bodypart_str_id body_part_test_bird_wing_l( "test_bird_wing_l" );
static const bodypart_str_id body_part_test_bird_wing_r( "test_bird_wing_r" );
static const bodypart_str_id body_part_test_corvid_beak( "test_corvid_beak" );
static const bodypart_str_id body_part_test_lizard_tail( "test_lizard_tail" );

static const efftype_id effect_mending( "mending" );
static const efftype_id effect_winded_arm_r( "winded_arm_r" );

static const enchantment_id enchantment_ENCH_TEST_BIRD_PARTS( "ENCH_TEST_BIRD_PARTS" );
static const enchantment_id enchantment_ENCH_TEST_LIZARD_TAIL( "ENCH_TEST_LIZARD_TAIL" );

static const json_character_flag json_flag_WALL_CLING( "WALL_CLING" );

static const sub_bodypart_str_id
sub_body_part_sub_limb_test_bird_foot_l( "sub_limb_test_bird_foot_l" );
static const sub_bodypart_str_id
sub_body_part_sub_limb_test_bird_foot_r( "sub_limb_test_bird_foot_r" );

static const trait_id trait_DEBUG_BIG_HEAD( "DEBUG_BIG_HEAD" );
static const trait_id trait_DEBUG_ONLY_HEAD( "DEBUG_ONLY_HEAD" );
static const trait_id trait_DEBUG_TAIL( "DEBUG_TAIL" );

static void create_bird_char( Character &dude )
{
    clear_character( dude, true );

    dude.enchantment_cache->force_add( *enchantment_ENCH_TEST_BIRD_PARTS, dude );
    dude.recalculate_bodyparts();
    REQUIRE( dude.has_part( body_part_test_corvid_beak ) );
    REQUIRE( dude.has_part( body_part_test_bird_wing_l ) );
    REQUIRE( dude.has_part( body_part_test_bird_wing_r ) );
    REQUIRE( dude.has_part( body_part_test_bird_foot_l ) );
    REQUIRE( dude.has_part( body_part_test_bird_foot_r ) );
    REQUIRE( !dude.has_part( body_part_mouth ) );
    REQUIRE( !dude.has_part( body_part_hand_l ) );
    REQUIRE( !dude.has_part( body_part_hand_r ) );
    REQUIRE( !dude.has_part( body_part_foot_l ) );
    REQUIRE( !dude.has_part( body_part_foot_r ) );
}

TEST_CASE( "Gaining_losing_limbs", "[limb]" )
{
    standard_npc dude( "Test NPC" );
    clear_character( dude, true );
    static std::vector<bodypart_id> base_parts = dude.get_all_body_parts();
    REQUIRE( base_parts.size() == 12 );
    // Gain a tail and re-check
    dude.toggle_trait( trait_DEBUG_TAIL );
    REQUIRE( dude.has_trait( trait_DEBUG_TAIL ) );
    CHECK( dude.get_all_body_parts().size() == 13 );

    // Lose trait and lose the tail
    dude.toggle_trait( trait_DEBUG_TAIL );
    REQUIRE( !dude.has_trait( trait_DEBUG_TAIL ) );
    CHECK( dude.get_all_body_parts().size() == 12 );

    // Lose every limb aside from the head
    dude.toggle_trait( trait_DEBUG_ONLY_HEAD );
    REQUIRE( dude.has_trait( trait_DEBUG_ONLY_HEAD ) );
    CHECK( dude.get_all_body_parts().size() == 3 );
    REQUIRE( dude.has_part( body_part_head ) );

    // Turn our head into a big head
    dude.toggle_trait( trait_DEBUG_BIG_HEAD );
    CHECK( dude.get_all_body_parts().size() == 3 );
    REQUIRE( !dude.has_part( body_part_head ) );
}

TEST_CASE( "limb_conditional_flags", "[character][encumbrance][limb]" )
{
    standard_npc dude( "Test NPC" );
    item wing_cover_left( "test_winglets_left" );
    create_bird_char( dude );
    // Flags are recognized and counted correctly
    REQUIRE( dude.has_bodypart_with_flag( json_flag_WALL_CLING ) );
    REQUIRE( dude.count_flag( json_flag_WALL_CLING ) == 2 );

    // Damage the right wing below its health limit, disabling one instance of the flag
    dude.set_part_hp_cur( body_part_test_bird_wing_r, 10 );
    CHECK( dude.has_bodypart_with_flag( json_flag_WALL_CLING ) );
    CHECK( dude.count_flag( json_flag_WALL_CLING ) == 1 );

    // Encumber the left wing above its enc limit, disabling all flags
    dude.wear_item( wing_cover_left, false );
    REQUIRE( dude.get_part_encumbrance_data( body_part_test_bird_wing_l ).encumbrance >= 15 );
    CHECK( !dude.has_bodypart_with_flag( json_flag_WALL_CLING ) );
    CHECK( dude.count_flag( json_flag_WALL_CLING ) == 0 );

    // Reset our right wing, flag still is enabled again
    dude.set_part_hp_cur( body_part_test_bird_wing_r, 100 );
    CHECK( dude.count_flag( json_flag_WALL_CLING ) == 1 );
    // Disable it with a windage effect
    dude.add_effect( effect_winded_arm_r, 1_hours, body_part_test_bird_wing_r, false, 1, true );
    REQUIRE( dude.get_effects_with_flag( flag_EFFECT_LIMB_DISABLE_CONDITIONAL_FLAGS ).size() == 1 );
    CHECK( dude.count_flag( json_flag_WALL_CLING ) == 0 );

}

TEST_CASE( "Limb_ugliness_calculations", "[character][npc][limb]" )
{
    standard_npc dude( "Test NPC" );
    standard_npc beholder( "Beholder" );
    item wing_covers( "test_winglets" );
    // We start at +/- 3 because of being unarmed
    REQUIRE( beholder.get_opinion_values( dude ).fear == -3 );
    REQUIRE( beholder.get_opinion_values( dude ).trust == 3 );

    //Fear gets increased by half of ugliness total, trust decreased by a third
    // But there are knock-on effects from +fear leading to -trust
    create_bird_char( dude );
    CHECK( beholder.get_opinion_values( dude ).fear == 39 );
    CHECK( beholder.get_opinion_values( dude ).trust == -25 );

    // TODO: fix covered ugliness and test it...
}

TEST_CASE( "Healing/mending_bonuses", "[character][limb]" )
{
    standard_npc dude( "Test NPC" );
    clear_character( dude, true );
    dude.enchantment_cache->force_add( *enchantment_ENCH_TEST_LIZARD_TAIL, dude );
    dude.recalculate_bodyparts();
    REQUIRE( dude.has_part( body_part_test_lizard_tail ) );
    SECTION( "Heal override and heal bonus" ) {
        dude.set_part_hp_cur( body_part_test_lizard_tail, 10 );
        REQUIRE( dude.get_part_hp_cur( body_part_test_lizard_tail ) < dude.get_part_hp_max(
                     body_part_test_lizard_tail ) );
        dude.regen( to_turns<int>( 5_minutes ) );
        CHECK( dude.get_part_hp_cur( body_part_test_lizard_tail ) == 11 );
    }
    SECTION( "Mend override" ) {
        dude.set_part_hp_cur( body_part_test_lizard_tail, 0 );
        dude.set_part_hp_cur( body_part_arm_l, 0 );
        REQUIRE( dude.is_limb_broken( body_part_test_lizard_tail ) );
        REQUIRE( dude.is_limb_broken( body_part_arm_l ) );
        dude.mend( to_turns<int>( 5_minutes ) );
        CHECK( dude.has_effect( effect_mending, body_part_test_lizard_tail ) );
        CHECK( !dude.has_effect( effect_mending, body_part_arm_l ) );
    }
}

TEST_CASE( "drying_rate", "[character][limb]" )
{
    standard_npc dude( "Test NPC" );
    clear_character( dude, true );
    const weather_manager weather = get_weather();
    REQUIRE( body_part_arm_l->drying_rate == 1.0f );
    dude.drench( 100, dude.get_drenching_body_parts(), false );
    REQUIRE( dude.get_part_wetness( body_part_arm_l ) == 200 );

    // Baseline arm dries in 450ish turns
    int base_dry = 0;
    while( dude.get_part_wetness( body_part_arm_l ) > 0 ) {
        dude.update_body_wetness( *weather.weather_precise );
        base_dry++;
    }
    REQUIRE( base_dry == Approx( 450 ).margin( 125 ) );

    // Birdify, clear water
    clear_character( dude, true );
    create_bird_char( dude );
    REQUIRE( body_part_test_bird_wing_l->drying_rate == 2.0f );
    REQUIRE( body_part_test_bird_wing_r->drying_rate == 0.5f );
    REQUIRE( dude.get_part_wetness( body_part_test_bird_wing_l ) == 0 );
    REQUIRE( dude.get_part_wetness( body_part_test_bird_wing_r ) == 0 );
    dude.drench( 100, dude.get_drenching_body_parts(), false );
    REQUIRE( dude.get_part_wetness( body_part_test_bird_wing_l ) == 200 );
    REQUIRE( dude.get_part_wetness( body_part_test_bird_wing_r ) == 200 );

    int high_dry = 0;
    int low_dry = 0;
    // Filter on the slower drying limb
    while( dude.get_part_wetness( body_part_test_bird_wing_r ) > 0 ) {
        dude.update_body_wetness( *weather.weather_precise );
        if( dude.get_part_wetness( body_part_test_bird_wing_l ) > 0 ) {
            high_dry++;
        }
        low_dry++;
    }

    // A drying rate of 2 should halve the drying time
    // Higher margin for the lower rate to account for the randomness
    CHECK( high_dry == Approx( 200 ).margin( 100 ) );
    CHECK( low_dry == Approx( 900 ).margin( 300 ) );
}

TEST_CASE( "Limb_armor_coverage", "[character][limb][armor]" )
{
    standard_npc dude( "Test NPC" );
    item test_shackles( "test_shackles" );
    item test_jumpsuit_cotton( "test_jumpsuit_cotton" );
    item wing_covers( "test_winglets" );
    item bird_boots( "test_bird_boots" );

    REQUIRE( test_jumpsuit_cotton.covers( body_part_arm_l ) );
    REQUIRE( test_jumpsuit_cotton.portion_for_bodypart( body_part_arm_l )->coverage == 95 );

    clear_character( dude, true );
    create_bird_char( dude );
    REQUIRE( dude.has_part( body_part_test_bird_wing_l ) );
    REQUIRE( dude.has_part( body_part_test_bird_wing_r ) );

    REQUIRE( body_part_test_arm_l->similar_bodyparts.size() == 1 );
    REQUIRE( body_part_test_arm_r->similar_bodyparts.size() == 1 );

    // Explicitly-defined armor covers custom limbs
    CHECK( wing_covers.covers( body_part_test_bird_wing_l ) );
    CHECK( wing_covers.portion_for_bodypart( body_part_test_bird_wing_l )->coverage == 100 );

    // Armor covering a base BP covers derived bodyparts, but not non-derived ones
    CHECK( test_shackles.covers( body_part_test_bird_wing_l ) );
    CHECK( test_shackles.covers( body_part_test_bird_wing_r ) );
    CHECK( test_shackles.portion_for_bodypart( body_part_test_bird_wing_l )->coverage == 100 );
    CHECK( test_shackles.portion_for_bodypart( body_part_test_bird_wing_r )->coverage == 100 );
    CHECK( !test_jumpsuit_cotton.covers( body_part_test_bird_wing_l ) );

    // Sublimb substitution works the same
    CHECK( bird_boots.covers( sub_body_part_sub_limb_test_bird_foot_l ) );
    CHECK( bird_boots.covers( sub_body_part_sub_limb_test_bird_foot_r ) );
    CHECK( bird_boots.portion_for_bodypart( sub_body_part_sub_limb_test_bird_foot_l )->coverage ==
           100 );
    CHECK( bird_boots.portion_for_bodypart( sub_body_part_sub_limb_test_bird_foot_r )->coverage ==
           100 );
    // Backwards population of coverage works as well
    CHECK( bird_boots.covers( body_part_test_bird_foot_l ) );
    CHECK( bird_boots.covers( body_part_test_bird_foot_r ) );
}
