#include "bodypart.h"
#include "cata_catch.h"
#include "character.h"
#include "character_modifier.h"
#include "damage.h"
#include "magic_enchantment.h"
#include "mutation.h"
#include "player_helpers.h"

static const bodypart_str_id body_part_test_tail( "test_tail" );

static const character_modifier_id
character_modifier_test_add_limbscores_mod( "test_add_limbscores_mod" );
static const character_modifier_id character_modifier_test_char_cost_mod( "test_char_cost_mod" );
static const character_modifier_id
character_modifier_test_mult_limbscores_mod( "test_mult_limbscores_mod" );
static const character_modifier_id
character_modifier_test_slip_prevent_mod( "test_slip_prevent_mod" );

static const enchantment_id enchantment_ENCH_TEST_TAIL( "ENCH_TEST_TAIL" );

static const itype_id itype_test_tail_encumber( "test_tail_encumber" );

static const limb_score_id limb_score_test( "test" );

static const trait_id trait_TEST_ARMOR_MUTATION( "TEST_ARMOR_MUTATION" );

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
    REQUIRE( character_modifier_test_char_cost_mod->use_limb_scores().find( limb_score_test ) !=
             character_modifier_test_char_cost_mod->use_limb_scores().end() );

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

TEST_CASE( "Body part armor vs. damage", "[character]" )
{
    standard_npc dude( "Test NPC" );
    create_char( dude );
    const bodypart *test_bp = dude.get_part( body_part_test_tail );
    REQUIRE( test_bp != nullptr );

    GIVEN( "Body part with 5 bash / 5 acid / 0 cut" ) {
        CHECK( test_bp->get_id()->damage_resistance( damage_type::BASH ) == Approx( 5.0 ).epsilon(
                   0.001 ) );
        CHECK( test_bp->get_id()->damage_resistance( damage_type::ACID ) == Approx( 5.0 ).epsilon(
                   0.001 ) );
        CHECK( test_bp->get_id()->damage_resistance( damage_type::CUT ) == Approx( 0.0 ).epsilon( 0.001 ) );

        WHEN( "Receiving bash damage" ) {
            damage_unit du( damage_type::BASH, 10.f, 0.f );
            dude.passive_absorb_hit( body_part_test_tail, du );
            THEN( "Absorb hit" ) {
                CHECK( dude.get_armor_bash( body_part_test_tail ) == 5 );
                CHECK( du.amount == Approx( 5.0 ).epsilon( 0.001 ) );
            }
        }

        WHEN( "Receiving acid damage" ) {
            damage_unit du( damage_type::ACID, 10.f, 0.f );
            dude.passive_absorb_hit( body_part_test_tail, du );
            THEN( "Absorb hit" ) {
                CHECK( dude.get_armor_acid( body_part_test_tail ) == 5 );
                CHECK( du.amount == Approx( 5.0 ).epsilon( 0.001 ) );
            }
        }

        WHEN( "Receiving cut damage" ) {
            damage_unit du( damage_type::CUT, 10.f, 0.f );
            dude.passive_absorb_hit( body_part_test_tail, du );
            THEN( "Absorb hit" ) {
                CHECK( dude.get_armor_cut( body_part_test_tail ) == 0 );
                CHECK( du.amount == Approx( 10.0 ).epsilon( 0.001 ) );
            }
        }
    }
}

TEST_CASE( "Mutation armor vs. damage", "[character][mutation]" )
{
    standard_npc dude( "Test NPC" );
    clear_character( dude, true );
    dude.toggle_trait( trait_TEST_ARMOR_MUTATION );
    REQUIRE( dude.has_trait( trait_TEST_ARMOR_MUTATION ) );

    GIVEN( "5 bash on arms, torso and tail / 1 cut on ALL" ) {
        for( const auto &res : trait_TEST_ARMOR_MUTATION->armor ) {
            if( res.first->primary_limb_type() == body_part_type::type::arm ||
                res.first->primary_limb_type() == body_part_type::type::torso ||
                res.first->primary_limb_type() == body_part_type::type::tail ) {
                CAPTURE( res.first.c_str() );
                CHECK( res.second.type_resist( damage_type::BASH ) == Approx( 5.0 ).epsilon( 0.001 ) );
                CHECK( res.second.type_resist( damage_type::ACID ) == Approx( 0.0 ).epsilon( 0.001 ) );
                CHECK( res.second.type_resist( damage_type::CUT ) == Approx( 1.0 ).epsilon( 0.001 ) );
            } else {
                CAPTURE( res.first.c_str() );
                CHECK( res.second.type_resist( damage_type::BASH ) == Approx( 0.0 ).epsilon( 0.001 ) );
                CHECK( res.second.type_resist( damage_type::ACID ) == Approx( 0.0 ).epsilon( 0.001 ) );
                CHECK( res.second.type_resist( damage_type::CUT ) == Approx( 1.0 ).epsilon( 0.001 ) );
            }
        }

        WHEN( "Receiving bash damage" ) {
            for( const body_part_type &bp : body_part_type::get_all() ) {
                if( !dude.has_part( bp.id ) ) {
                    continue;
                }
                const bool has_res = bp.id->primary_limb_type() == body_part_type::type::arm ||
                                     bp.id->primary_limb_type() == body_part_type::type::torso ||
                                     bp.id->primary_limb_type() == body_part_type::type::tail;
                const int res_amt = has_res ? 5 : 0;
                damage_unit du( damage_type::BASH, 10.f, 0.f );
                dude.passive_absorb_hit( bp.id, du );
                THEN( "Absorb hit" ) {
                    CAPTURE( bp.id.c_str() );
                    CHECK( dude.get_armor_bash( bp.id ) == res_amt );
                    CHECK( du.amount == Approx( 10.0 - res_amt ).epsilon( 0.001 ) );
                }
            }
        }

        WHEN( "Receiving acid damage" ) {
            for( const body_part_type &bp : body_part_type::get_all() ) {
                if( !dude.has_part( bp.id ) ) {
                    continue;
                }
                damage_unit du( damage_type::ACID, 10.f, 0.f );
                dude.passive_absorb_hit( bp.id, du );
                THEN( "Absorb hit" ) {
                    CAPTURE( bp.id.c_str() );
                    CHECK( dude.get_armor_acid( bp.id ) == 0 );
                    CHECK( du.amount == Approx( 10.0 ).epsilon( 0.001 ) );
                }
            }
        }

        WHEN( "Receiving cut damage" ) {
            for( const body_part_type &bp : body_part_type::get_all() ) {
                if( !dude.has_part( bp.id ) ) {
                    continue;
                }
                damage_unit du( damage_type::CUT, 10.f, 0.f );
                dude.passive_absorb_hit( bp.id, du );
                THEN( "Absorb hit" ) {
                    CAPTURE( bp.id.c_str() );
                    CHECK( dude.get_armor_cut( bp.id ) == 1 );
                    CHECK( du.amount == Approx( 9.0 ).epsilon( 0.001 ) );
                }
            }
        }
    }

    GIVEN( "+ body part with 5 bash / 5 acid" ) {
        dude.enchantment_cache->force_add( *enchantment_ENCH_TEST_TAIL );
        dude.recalculate_bodyparts();
        REQUIRE( dude.has_part( body_part_test_tail ) );

        WHEN( "Receiving bash damage" ) {
            for( const body_part_type &bp : body_part_type::get_all() ) {
                if( !dude.has_part( bp.id ) ) {
                    continue;
                }
                const bool has_mut_res = bp.id->primary_limb_type() == body_part_type::type::arm ||
                                         bp.id->primary_limb_type() == body_part_type::type::torso ||
                                         bp.id->primary_limb_type() == body_part_type::type::tail;
                const bool has_bp_res = bp.id == body_part_test_tail;
                const int res_amt = ( has_mut_res ? 5 : 0 ) + ( has_bp_res ? 5 : 0 );
                damage_unit du( damage_type::BASH, 10.f, 0.f );
                dude.passive_absorb_hit( bp.id, du );
                THEN( "Absorb hit" ) {
                    CAPTURE( bp.id.c_str() );
                    CHECK( dude.get_armor_bash( bp.id ) == res_amt );
                    CHECK( du.amount == Approx( 10.0 - res_amt ).epsilon( 0.001 ) );
                }
            }
        }

        WHEN( "Receiving acid damage" ) {
            for( const body_part_type &bp : body_part_type::get_all() ) {
                if( !dude.has_part( bp.id ) ) {
                    continue;
                }
                const int res_amt = bp.id == body_part_test_tail ? 5 : 0;
                damage_unit du( damage_type::ACID, 10.f, 0.f );
                dude.passive_absorb_hit( bp.id, du );
                THEN( "Absorb hit" ) {
                    CAPTURE( bp.id.c_str() );
                    CHECK( dude.get_armor_acid( bp.id ) == res_amt );
                    CHECK( du.amount == Approx( 10.0 - res_amt ).epsilon( 0.001 ) );
                }
            }
        }

        WHEN( "Receiving cut damage" ) {
            for( const body_part_type &bp : body_part_type::get_all() ) {
                if( !dude.has_part( bp.id ) ) {
                    continue;
                }
                const int res_amt = bp.id == body_part_test_tail ? 6 : 1;
                damage_unit du( damage_type::CUT, 10.f, 0.f );
                dude.passive_absorb_hit( bp.id, du );
                THEN( "Absorb hit" ) {
                    CAPTURE( bp.id.c_str() );
                    CHECK( dude.get_armor_cut( bp.id ) == res_amt );
                    CHECK( du.amount == Approx( 10.0 - res_amt ).epsilon( 0.001 ) );
                }
            }
        }
    }
}

TEST_CASE( "Multi-limbscore modifiers", "[character]" )
{
    standard_npc dude( "Test NPC" );
    create_char( dude );

    WHEN( "Character is uninjured / unencumbered" ) {
        CHECK( dude.get_modifier( character_modifier_test_add_limbscores_mod ) == Approx( 2.0 ).epsilon(
                   0.001 ) );
        CHECK( dude.get_modifier( character_modifier_test_mult_limbscores_mod ) == Approx( 1.0 ).epsilon(
                   0.001 ) );
    }

    WHEN( "Character has high eye encumbrance" ) {
        item eyecover( "test_goggles_welding" );
        dude.wear_item( eyecover );
        REQUIRE( dude.encumb( dude.get_all_body_parts_of_type( body_part_type::type::sensor ).front() ) ==
                 60 );
        CHECK( dude.get_modifier( character_modifier_test_add_limbscores_mod ) == Approx( 2.0 ).epsilon(
                   0.001 ) );
        CHECK( dude.get_modifier( character_modifier_test_mult_limbscores_mod ) == Approx( 0.1 ).epsilon(
                   0.001 ) );
    }

    WHEN( "Character has broken arms" ) {
        for( const bodypart_id &bid : dude.get_all_body_parts_of_type( body_part_type::type::arm ) ) {
            dude.set_part_hp_cur( bid, 0 );
        }
        REQUIRE( dude.get_working_arm_count() == 0 );
        CHECK( dude.get_modifier( character_modifier_test_add_limbscores_mod ) == Approx( 1.4 ).epsilon(
                   0.001 ) );
        CHECK( dude.get_modifier( character_modifier_test_mult_limbscores_mod ) == Approx( 0.1 ).epsilon(
                   0.001 ) );
    }

    WHEN( "Character has high eye encumbrance and broken arms" ) {
        item eyecover( "test_goggles_welding" );
        dude.wear_item( eyecover );
        for( const bodypart_id &bid : dude.get_all_body_parts_of_type( body_part_type::type::arm ) ) {
            dude.set_part_hp_cur( bid, 0 );
        }
        REQUIRE( dude.get_working_arm_count() == 0 );
        REQUIRE( dude.encumb( dude.get_all_body_parts_of_type( body_part_type::type::sensor ).front() ) ==
                 60 );
        CHECK( dude.get_modifier( character_modifier_test_add_limbscores_mod ) == Approx( 0.4 ).epsilon(
                   0.001 ) );
        CHECK( dude.get_modifier( character_modifier_test_mult_limbscores_mod ) == Approx( 0.1 ).epsilon(
                   0.001 ) );
    }
}

TEST_CASE( "Slip prevention modifier / weighted-list multi-score modifiers", "[character]" )
{
    standard_npc dude( "Test NPC" );
    create_char( dude );

    WHEN( "Character is uninjured / unencumbered" ) {
        CHECK( dude.get_modifier( character_modifier_test_slip_prevent_mod ) == Approx( 1.0 ).epsilon(
                   0.001 ) );
    }

    WHEN( "Character has broken arms" ) {
        for( const bodypart_id &bid : dude.get_all_body_parts_of_type( body_part_type::type::arm ) ) {
            dude.set_part_hp_cur( bid, 0 );
        }
        REQUIRE( dude.get_working_arm_count() == 0 );
        CHECK( dude.get_modifier( character_modifier_test_slip_prevent_mod ) == Approx( 2.0 / 3.0 ).epsilon(
                   0.001 ) );
    }

    WHEN( "Character is heavily encumbered" ) {
        item hazmat_suit( "test_hazmat_suit" );
        dude.wear_item( hazmat_suit );
        REQUIRE( dude.encumb( dude.get_all_body_parts_of_type( body_part_type::type::foot ).front() ) ==
                 37 );
        REQUIRE( dude.encumb( dude.get_all_body_parts_of_type( body_part_type::type::hand ).front() ) ==
                 37 );
        CHECK( dude.get_modifier( character_modifier_test_slip_prevent_mod ) == Approx( 0.623 ).epsilon(
                   0.001 ) );
    }

    WHEN( "Character has broken arms and is heavily encumbered" ) {
        item hazmat_suit( "test_hazmat_suit" );
        dude.wear_item( hazmat_suit );
        for( const bodypart_id &bid : dude.get_all_body_parts_of_type( body_part_type::type::arm ) ) {
            dude.set_part_hp_cur( bid, 0 );
        }
        REQUIRE( dude.encumb( dude.get_all_body_parts_of_type( body_part_type::type::foot ).front() ) ==
                 37 );
        REQUIRE( dude.encumb( dude.get_all_body_parts_of_type( body_part_type::type::hand ).front() ) ==
                 37 );
        REQUIRE( dude.get_working_arm_count() == 0 );
        CHECK( dude.get_modifier( character_modifier_test_slip_prevent_mod ) == Approx( 0.41 ).epsilon(
                   0.001 ) );
    }
}