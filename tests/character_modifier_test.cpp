#include "bodypart.h"
#include "cata_catch.h"
#include "character_modifier.h"
#include "magic_enchantment.h"
#include "player_helpers.h"

static const bodypart_str_id body_part_test_tail( "test_tail" );
static const character_modifier_id character_modifier_test_char_cost_mod( "test_char_cost_mod" );
static const enchantment_id enchantment_ENCH_TEST_TAIL( "ENCH_TEST_TAIL" );
static const itype_id itype_test_tail_encumber( "test_tail_encumber" );
static const limb_score_id limb_score_test( "test" );

static void create_char( Character &dude )
{
    clear_character( dude, true );

    dude.enchantment_cache->force_add( *enchantment_ENCH_TEST_TAIL );
    dude.recalculate_bodyparts();
    REQUIRE( dude.has_part( body_part_test_tail ) );
}

TEST_CASE( "Basic limb score test", "[character][encumbrance]" )
{
    standard_npc dude( "Test NPC" );
    create_char( dude );
    const bodypart *test_bp = dude.get_part( body_part_test_tail );
    REQUIRE( test_bp != nullptr );

    GIVEN( "limb is not encumbered" ) {
        WHEN( "limb is not wounded" ) {
            THEN( "modified limb score equals unmodified limb score" ) {
                CHECK( test_bp->get_id()->get_limb_score( limb_score_test ) == Approx( 0.8 ).epsilon( 0.001 ) );
                CHECK( dude.get_limb_score( limb_score_test ) == Approx( 0.8 ).epsilon( 0.001 ) );
            }
        }
        WHEN( "limb is wounded" ) {
            dude.apply_damage( nullptr, test_bp->get_id(), test_bp->get_hp_max() / 2, true );
            THEN( "modified limb score is less than unmodified limb score" ) {
                CHECK( test_bp->get_id()->get_limb_score( limb_score_test ) == Approx( 0.8 ).epsilon( 0.001 ) );
                CHECK( dude.get_limb_score( limb_score_test ) == Approx( 0.53333 ).epsilon( 0.001 ) );
            }
        }
    }

    GIVEN( "limb is encumbered" ) {
        item it( itype_test_tail_encumber );
        dude.wear_item( it, false );
        WHEN( "limb is not wounded" ) {
            THEN( "modified limb score is less than unmodified limb score" ) {
                CHECK( test_bp->get_id()->get_limb_score( limb_score_test ) == Approx( 0.8 ).epsilon( 0.001 ) );
                CHECK( dude.get_limb_score( limb_score_test ) == Approx( 0.4701 ).epsilon( 0.001 ) );
            }
        }
        WHEN( "limb is wounded" ) {
            dude.apply_damage( nullptr, test_bp->get_id(), test_bp->get_hp_max() / 2, true );
            THEN( "modified limb score is less than unmodified limb score" ) {
                CHECK( test_bp->get_id()->get_limb_score( limb_score_test ) == Approx( 0.8 ).epsilon( 0.001 ) );
                CHECK( dude.get_limb_score( limb_score_test ) == Approx( 0.3134 ).epsilon( 0.001 ) );
            }
        }
    }
}

TEST_CASE( "Basic character modifier test", "[character][encumbrance]" )
{
    standard_npc dude( "Test NPC" );
    create_char( dude );
    const bodypart *test_bp = dude.get_part( body_part_test_tail );
    REQUIRE( test_bp != nullptr );
    REQUIRE( character_modifier_test_char_cost_mod->use_limb_score() == limb_score_test );

    GIVEN( "limb is not encumbered" ) {
        WHEN( "limb is not wounded" ) {
            THEN( "modified limb score equals unmodified limb score" ) {
                CHECK( dude.get_modifier( character_modifier_test_char_cost_mod ) == Approx( 7.5 ).epsilon(
                           0.001 ) );
            }
        }
        WHEN( "limb is wounded" ) {
            dude.apply_damage( nullptr, test_bp->get_id(), test_bp->get_hp_max() / 2, true );
            THEN( "modified limb score is less than unmodified limb score" ) {
                CHECK( dude.get_modifier( character_modifier_test_char_cost_mod ) == Approx( 26.25 ).epsilon(
                           0.001 ) );
            }
        }
    }

    GIVEN( "limb is encumbered" ) {
        item it( itype_test_tail_encumber );
        dude.wear_item( it, false );
        WHEN( "limb is not wounded" ) {
            THEN( "modified limb score is less than unmodified limb score" ) {
                CHECK( dude.get_modifier( character_modifier_test_char_cost_mod ) == Approx( 33.81579 ).epsilon(
                           0.001 ) );
            }
        }
        WHEN( "limb is wounded" ) {
            dude.apply_damage( nullptr, test_bp->get_id(), test_bp->get_hp_max() / 2, true );
            THEN( "modified limb score is less than unmodified limb score" ) {
                CHECK( dude.get_modifier( character_modifier_test_char_cost_mod ) == Approx( 65.72368 ).epsilon(
                           0.001 ) );
            }
        }
    }
}