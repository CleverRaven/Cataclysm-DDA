#include <memory>
#include <string>
#include <vector>

#include "body_part_set.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "coordinates.h"
#include "flag.h"
#include "item.h"
#include "itype.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_helpers.h"
#include "npc.h"
#include "npc_opinion.h"
#include "options_helpers.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"
#include "weather_gen.h"
#include "weather_type.h"

static const bodypart_str_id body_part_test_arm_l( "test_arm_l" );
static const bodypart_str_id body_part_test_arm_r( "test_arm_r" );
static const bodypart_str_id body_part_test_bird_foot_l( "test_bird_foot_l" );
static const bodypart_str_id body_part_test_bird_foot_r( "test_bird_foot_r" );
static const bodypart_str_id body_part_test_bird_wing_l( "test_bird_wing_l" );
static const bodypart_str_id body_part_test_bird_wing_r( "test_bird_wing_r" );
static const bodypart_str_id body_part_test_corvid_beak( "test_corvid_beak" );
static const bodypart_str_id body_part_test_lizard_tail( "test_lizard_tail" );

static const character_modifier_id character_modifier_liquid_consume_mod( "liquid_consume_mod" );
static const character_modifier_id character_modifier_solid_consume_mod( "solid_consume_mod" );

static const efftype_id effect_mending( "mending" );
static const efftype_id effect_winded_arm_r( "winded_arm_r" );

static const enchantment_id enchantment_ENCH_TEST_BIRD_PARTS( "ENCH_TEST_BIRD_PARTS" );
static const enchantment_id enchantment_ENCH_TEST_LIZARD_TAIL( "ENCH_TEST_LIZARD_TAIL" );

static const itype_id itype_test_bird_boots( "test_bird_boots" );
static const itype_id itype_test_jumpsuit_cotton( "test_jumpsuit_cotton" );
static const itype_id itype_test_liquid( "test_liquid" );
static const itype_id itype_test_pine_nuts( "test_pine_nuts" );
static const itype_id itype_test_shackles( "test_shackles" );
static const itype_id itype_test_winglets( "test_winglets" );
static const itype_id itype_test_winglets_left( "test_winglets_left" );

static const json_character_flag json_flag_WALL_CLING( "WALL_CLING" );

static const sub_bodypart_str_id
sub_body_part_sub_limb_test_bird_foot_l( "sub_limb_test_bird_foot_l" );
static const sub_bodypart_str_id
sub_body_part_sub_limb_test_bird_foot_r( "sub_limb_test_bird_foot_r" );

static const ter_str_id ter_t_grass( "t_grass" );
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
    item wing_cover_left( itype_test_winglets_left );
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
    REQUIRE( dude.get_part_encumbrance( body_part_test_bird_wing_l ) >= 15 );
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
    item wing_covers( itype_test_winglets );
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
    clear_map();
    map &here = get_map();
    standard_npc dude( "Test NPC" );
    clear_character( dude, true );

    set_time_to_day();
    const weather_manager &weather = get_weather();
    w_point &weather_point = *weather.weather_precise;
    scoped_weather_override weather_clear( WEATHER_CLEAR );
    restore_on_out_of_scope restore_temp( weather_point.temperature );
    weather_point.temperature = units::from_fahrenheit( 65 );
    restore_on_out_of_scope restore_humidity( weather_point.humidity );
    weather_point.humidity = 66.0f;

    CAPTURE( weather.weather_id.c_str() );
    CAPTURE( units::to_fahrenheit( weather_point.temperature ) );
    CAPTURE( weather_point.humidity );

    REQUIRE( here.ter( dude.pos_bub() ).id() == ter_t_grass );
    REQUIRE( here.furn( dude.pos_bub() ).id() == furn_str_id::NULL_ID() );

    REQUIRE( body_part_arm_l->drying_rate == 1.0f );
    dude.drench( 100, dude.get_drenching_body_parts(), false );
    REQUIRE( dude.get_part_wetness( body_part_arm_l ) == 200 );

    // Baseline arm dries in 450ish turns
    int base_dry = 0;
    while( dude.get_part_wetness( body_part_arm_l ) > 0 ) {
        dude.update_body_wetness( weather_point );
        base_dry++;
    }
    // 200 wetness / (1000 drench cap / ~1800 turns ) = 360
    REQUIRE( base_dry == Approx( 360 ).margin( 120 ) );

    // Birdify, clear water
    clear_character( dude, true );
    create_bird_char( dude );
    REQUIRE( here.ter( dude.pos_bub() ).id() == ter_t_grass );
    REQUIRE( here.furn( dude.pos_bub() ).id() == furn_str_id::NULL_ID() );
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
        dude.update_body_wetness( weather_point );
        if( dude.get_part_wetness( body_part_test_bird_wing_l ) > 0 ) {
            high_dry++;
        }
        low_dry++;
    }

    // A drying rate of 2 should halve the drying time
    // Higher margin for the lower rate to account for the randomness
    // 200 wetness / (1000 drench cap / ~(1800/2) turns) = 180
    CHECK( high_dry == Approx( 180 ).margin( 60 ) );
    // 200 wetness / (1000 drench cap / ~(1800*2) turns) = 720
    CHECK( low_dry == Approx( 720 ).margin( 240 ) );
}

TEST_CASE( "Limb_consumption", "[limb]" )
{
    standard_npc dude( "Test NPC" );
    const item solid( itype_test_pine_nuts );
    const item liquid( itype_test_liquid );
    clear_character( dude, true );
    // Normal chars are normal
    REQUIRE( dude.get_modifier( character_modifier_liquid_consume_mod ) == 1.0f );
    REQUIRE( dude.get_modifier( character_modifier_solid_consume_mod ) == 1.0f );
    const time_duration base_solid = dude.get_consume_time( solid );
    const time_duration base_liquid = dude.get_consume_time( liquid );
    create_bird_char( dude );
    // Testbird chars are birdy
    REQUIRE( dude.get_modifier( character_modifier_liquid_consume_mod ) == 2.0f );
    REQUIRE( dude.get_modifier( character_modifier_solid_consume_mod ) == 0.5f );
    CHECK( dude.get_consume_time( liquid ) == base_liquid * 2 );
    CHECK( dude.get_consume_time( solid ) == base_solid / 2 );
}
TEST_CASE( "Limb_armor_coverage", "[character][limb][armor]" )
{
    standard_npc dude( "Test NPC" );
    item test_shackles( itype_test_shackles );
    item test_jumpsuit_cotton( itype_test_jumpsuit_cotton );
    item wing_covers( itype_test_winglets );
    item bird_boots( itype_test_bird_boots );

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
