#include "bodypart.h"
#include "cata_catch.h"
#include "character.h"
#include "character_modifier.h"
#include "damage.h"
#include "magic_enchantment.h"
#include "mutation.h"
#include "player_helpers.h"

static const bodypart_str_id body_part_test_bird_foot_l( "test_bird_foot_l" );
static const bodypart_str_id body_part_test_bird_foot_r( "test_bird_foot_r" );
static const bodypart_str_id body_part_test_bird_wing_l( "test_bird_wing_l" );
static const bodypart_str_id body_part_test_bird_wing_r( "test_bird_wing_r" );
static const bodypart_str_id body_part_test_corvid_beak( "test_corvid_beak" );
static const bodypart_str_id body_part_test_lizard_tail( "test_lizard_tail" );

static const efftype_id effect_mending( "mending" );

static const enchantment_id enchantment_ENCH_TEST_BIRD_PARTS( "ENCH_TEST_BIRD_PARTS" );
static const enchantment_id enchantment_ENCH_TEST_LIZARD_TAIL( "ENCH_TEST_LIZARD_TAIL" );

static const json_character_flag json_flag_WALL_CLING( "WALL_CLING" );

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
