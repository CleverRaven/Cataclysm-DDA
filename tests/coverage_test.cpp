#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ballistics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "damage.h"
#include "dispersion.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "monster.h"
#include "npc.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "projectile.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "subbodypart.h"
#include "type_id.h"
#include "weakpoint.h"

static const damage_type_id damage_bullet( "bullet" );

static const efftype_id effect_bite( "bite" );
static const efftype_id effect_sleep( "sleep" );

static const flag_id json_flag_FILTHY( "FILTHY" );

static const itype_id itype_ballistic_vest_esapi( "ballistic_vest_esapi" );
static const itype_id itype_face_shield( "face_shield" );
static const itype_id itype_hat_hard( "hat_hard" );
static const itype_id itype_test_ghost_vest( "test_ghost_vest" );
static const itype_id itype_test_hazmat_suit( "test_hazmat_suit" );
static const itype_id itype_test_hazmat_suit_nomelee( "test_hazmat_suit_nomelee" );
static const itype_id itype_test_hazmat_suit_noranged( "test_hazmat_suit_noranged" );
static const itype_id
itype_test_multi_portion_segmented_armor( "test_multi_portion_segmented_armor" );
static const itype_id itype_test_plate( "test_plate" );
static const itype_id itype_test_plate_skirt_super( "test_plate_skirt_super" );
static const itype_id itype_test_portion_segmented_armor( "test_portion_segmented_armor" );
static const itype_id itype_test_swat_mostly_cotton( "test_swat_mostly_cotton" );
static const itype_id itype_test_swat_mostly_steel( "test_swat_mostly_steel" );
static const itype_id itype_test_zentai( "test_zentai" );
static const itype_id itype_test_zentai_nomelee( "test_zentai_nomelee" );

static const mtype_id mon_manhack( "mon_manhack" );

static const sub_bodypart_str_id sub_body_part_eyes_right( "eyes_right" );

static const int num_iters = 10000;
static constexpr tripoint_bub_ms dude_pos( HALF_MAPSIZE_X + 4, HALF_MAPSIZE_Y, 0 );
static constexpr tripoint_bub_ms mon_pos( HALF_MAPSIZE_X + 3, HALF_MAPSIZE_Y, 0 );
static constexpr tripoint_bub_ms badguy_pos( HALF_MAPSIZE_X + 1, HALF_MAPSIZE_Y, 0 );

static void check_near( const std::string &subject, float actual, const float expected,
                        const float tolerance )
{
    THEN( string_format( "%s is about %.1f (+/- %.2f) with val %.1f", subject, expected, tolerance,
                         actual ) ) {
        CHECK( actual == Approx( expected ).margin( tolerance ) );
    }
}

static void check_not_near( const std::string &subject, float actual, const float undesired,
                            const float tolerance )
{
    THEN( string_format( "%s is not about %.1f (+/- %.1f)  with val %.1f", subject, undesired,
                         tolerance, actual ) ) {
        CHECK_FALSE( actual == Approx( undesired ).margin( tolerance ) );
    }
}

static float get_avg_melee_dmg( const itype_id &clothing_id, bool infect_risk = false )
{
    map &here = get_map();

    monster zed( mon_manhack, mon_pos );
    standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
    item cloth( clothing_id );
    if( infect_risk ) {
        cloth.set_flag( json_flag_FILTHY );
    }
    int dam_acc = 0;
    int num_hits = 0;
    for( int i = 0; i < num_iters; i++ ) {
        clear_character( dude, true );
        dude.setpos( here, dude_pos );
        dude.wear_item( cloth, false );
        dude.add_effect( effect_sleep, 1_hours );
        if( zed.melee_attack( dude, 10000.0f ) ) {
            num_hits++;
        }
        cloth.set_damage( 0 );
        if( !infect_risk ) {
            dam_acc += dude.get_hp_max() - dude.get_hp();
        } else if( dude.has_effect( effect_bite ) ) {
            dam_acc++;
        }
        if( dude.is_dead() ) {
            break;
        }
    }
    CAPTURE( dude.is_dead() );
    const std::string ret_type = infect_risk ? "infections" : "damage total";
    INFO( string_format( "%s landed %d hits on character, causing %d %s.", zed.get_name(), num_hits,
                         dam_acc, ret_type ) );
    num_hits = num_hits ? num_hits : 1;
    return static_cast<float>( dam_acc ) / num_hits;
}

static float get_avg_melee_dmg( item cloth, bool infect_risk = false )
{
    map &here = get_map();

    monster zed( mon_manhack, mon_pos );
    standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
    if( infect_risk ) {
        cloth.set_flag( json_flag_FILTHY );
    }
    int dam_acc = 0;
    int num_hits = 0;
    for( int i = 0; i < num_iters; i++ ) {
        clear_character( dude, true );
        dude.setpos( here, dude_pos );
        dude.wear_item( cloth, false );
        dude.add_effect( effect_sleep, 1_hours );
        if( zed.melee_attack( dude, 10000.0f ) ) {
            num_hits++;
        }
        cloth.set_damage( 0 );
        if( !infect_risk ) {
            dam_acc += dude.get_hp_max() - dude.get_hp();
        } else if( dude.has_effect( effect_bite ) ) {
            dam_acc++;
        }
        if( dude.is_dead() ) {
            break;
        }
    }
    CAPTURE( dude.is_dead() );
    const std::string ret_type = infect_risk ? "infections" : "damage total";
    INFO( string_format( "%s landed %d hits on character, causing %d %s.", zed.get_name(), num_hits,
                         dam_acc, ret_type ) );
    num_hits = num_hits ? num_hits : 1;
    return static_cast<float>( dam_acc ) / num_hits;
}

static float get_avg_bullet_dmg( const itype_id &clothing_id )
{
    map &here = get_map();
    clear_map();
    std::unique_ptr<standard_npc> badguy = std::make_unique<standard_npc>( "TestBaddie",
                                           badguy_pos, std::vector<itype_id>(), 0, 8, 8, 8, 8 );
    std::unique_ptr<standard_npc> dude = std::make_unique<standard_npc>( "TestCharacter",
                                         dude_pos, std::vector<itype_id>(), 0, 8, 8, 8, 8 );
    item cloth( clothing_id );
    projectile proj;
    proj.speed = 1000;
    proj.impact = damage_instance( damage_bullet, 20 );
    proj.range = 30;
    proj.proj_effects = std::set<ammo_effect_str_id>();
    proj.critical_multiplier = 1;

    int dam_acc = 0;
    int num_hits = 0;
    for( int i = 0; i < num_iters; i++ ) {
        clear_character( *dude, true );
        dude->setpos( here, dude_pos );
        dude->wear_item( cloth, false );
        dude->add_effect( effect_sleep, 1_hours );
        dealt_projectile_attack atk;
        projectile_attack( atk, proj, badguy_pos, dude_pos, dispersion_sources(),
                           &*badguy );
        dude->deal_projectile_attack( &here,  & *badguy, atk, atk.missed_by, false );
        if( atk.missed_by < 1.0 ) {
            num_hits++;
        }
        cloth.set_damage( 0 );
        dam_acc += dude->get_hp_max() - dude->get_hp();
        if( dude->is_dead() ) {
            break;
        }
    }
    CAPTURE( dude->is_dead() );
    INFO( string_format( "%s landed %d hits on character, causing %d damage total.",
                         badguy->disp_name( false, true ), num_hits, dam_acc ) );
    num_hits = num_hits ? num_hits : 1;
    return static_cast<float>( dam_acc ) / num_hits;
}

TEST_CASE( "Infections_from_filthy_clothing", "[coverage]" )
{
    SECTION( "Full melee and ranged coverage vs. melee attack" ) {
        const float chance = get_avg_melee_dmg( itype_test_zentai, true );
        check_near( "Infection chance", chance, 0.48f, 0.05f );
    }

    SECTION( "No melee coverage vs. melee attack" ) {
        const float chance = get_avg_melee_dmg( itype_test_zentai_nomelee, true );
        check_near( "Infection chance", chance, 0.0f, 0.0001f );
    }
}

TEST_CASE( "Melee_coverage_vs_melee_damage", "[coverage] [melee] [damage]" )
{
    SECTION( "Full melee and ranged coverage vs. melee attack" ) {
        const float dmg = get_avg_melee_dmg( itype_test_hazmat_suit );
        check_near( "Average damage", dmg, 9.2f, 0.2f );
    }

    SECTION( "No melee coverage vs. melee attack" ) {
        const float dmg = get_avg_melee_dmg( itype_test_hazmat_suit_nomelee );
        check_near( "Average damage", dmg, 17.0f, 0.2f );
    }
}

TEST_CASE( "Ranged_coverage_vs_bullet", "[coverage] [ranged]" )
{
    SECTION( "Full melee and ranged coverage vs. ranged attack" ) {
        const float dmg = get_avg_bullet_dmg( itype_test_hazmat_suit );
        check_near( "Average damage", dmg, 16.4f, 0.2f );
    }

    SECTION( "No ranged coverage vs. ranged attack" ) {
        const float dmg = get_avg_bullet_dmg( itype_test_hazmat_suit_noranged );
        check_near( "Average damage", dmg, 18.4f, 0.2f );
    }
}

TEST_CASE( "Proportional_armor_material_resistances", "[material]" )
{
    SECTION( "Mostly steel armor vs. melee" ) {
        const float dmg = get_avg_melee_dmg( itype_test_swat_mostly_steel );
        check_near( "Average damage", dmg, 4.0f, 0.2f );
    }

    SECTION( "Mostly cotton armor vs. melee" ) {
        const float dmg = get_avg_melee_dmg( itype_test_swat_mostly_cotton );
        // more variance on this test since it has a 5% chance of blocking with
        // high protection steel
        check_near( "Average damage", dmg, 14.4f, 0.4f );
    }

    SECTION( "Multi material segmented armor vs. melee" ) {
        const float dmg = get_avg_melee_dmg( itype_test_multi_portion_segmented_armor );
        const float base_line = get_avg_melee_dmg( itype_test_portion_segmented_armor );
        // our armor should NOT be near 1 mm cloth + 80% of 1mm of steel
        // and should be higher (so lower damage) since they can overlap
        check_not_near( "Average damage", dmg, base_line, 0.05f );
    }
}

TEST_CASE( "Ghost_ablative_vest", "[coverage]" )
{
    SECTION( "Ablative not covered same limb" ) {
        item full = item( itype_test_ghost_vest );
        full.force_insert_item( item( itype_test_plate ), pocket_type::CONTAINER );
        full.force_insert_item( item( itype_test_plate ), pocket_type::CONTAINER );
        item empty = item( itype_test_ghost_vest );

        // make sure vest only covers torso_upper when it has armor in it
        REQUIRE( full.covers( sub_bodypart_id( "torso_upper" ) ) );
        REQUIRE( !empty.covers( sub_bodypart_id( "torso_upper" ) ) );
        const float dmg_full = get_avg_melee_dmg( full );
        const float dmg_empty = get_avg_melee_dmg( empty );
        // make sure the armor is counting even if the base vest doesn't do anything
        check_not_near( "Average damage", dmg_full, dmg_empty, 0.5f );
    }
}

TEST_CASE( "helmet_with_face_shield_coverage", "[coverage]" )
{
    Character &dummy = get_player_character();
    clear_avatar();

    item hat_hard( itype_hat_hard );
    CHECK( hat_hard.get_coverage( body_part_eyes ) == 0 );

    WHEN( "wearing helmet with face shield should cover eyes and mouth" ) {
        item face_shield( itype_face_shield );
        REQUIRE( hat_hard.put_in( face_shield, pocket_type::CONTAINER ).success() );
        dummy.wear_item( hat_hard );

        CHECK( hat_hard.get_coverage( body_part_eyes ) == 100 );
        CHECK( hat_hard.get_coverage( body_part_mouth ) == 100 );
        CHECK( hat_hard.get_coverage( sub_body_part_eyes_right ) == 100 );
    }

}

TEST_CASE( "vest_with_plate_coverage", "[coverage]" )
{
    //Vest covers torso_upper and torso_lower
    item vest = item( itype_ballistic_vest_esapi );
    //100 (torso_upper coverage) * 0.6 (torso_upper max_coverage) + 80 (torso_lower coverage) * 0.4
    CHECK( vest.get_coverage( body_part_torso ) == 92 );

    WHEN( "inserting 2 plates" ) {
        //Each plate covers torso_upper with coverage 45
        CHECK( vest.put_in( item( itype_test_plate ), pocket_type::CONTAINER ).success() );
        CHECK( vest.put_in( item( itype_test_plate ), pocket_type::CONTAINER ).success() );

        THEN( "vest with plates should retain the same coverage" ) {
            CHECK( vest.get_coverage( body_part_torso ) == 92 );
        }
    }

}

TEST_CASE( "Off_Limb_Ghost_ablative_vest", "[coverage]" )
{
    SECTION( "Ablative not covered seperate limb" ) {
        item full = item( itype_test_ghost_vest );
        full.force_insert_item( item( itype_test_plate_skirt_super ), pocket_type::CONTAINER );

        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        dude.wear_item( full, false );
        damage_instance du_full = damage_instance( damage_bullet, 100.0f );
        dude.absorb_hit( weakpoint_attack(), bodypart_id( "leg_l" ), du_full );
        check_near( "Damage Protected", du_full.total_damage(), 0.0f, 0.1f );
    }
}

