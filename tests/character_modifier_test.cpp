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
static const bodypart_str_id body_part_test_tail( "test_tail" );

static const character_modifier_id
character_modifier_test_add_limbscores_mod( "test_add_limbscores_mod" );
static const character_modifier_id character_modifier_test_char_cost_mod( "test_char_cost_mod" );
static const character_modifier_id
character_modifier_test_mult_limbscores_mod( "test_mult_limbscores_mod" );
static const character_modifier_id
character_modifier_test_ranged_dispersion_manip_mod( "test_ranged_dispersion_manip_mod" );
static const character_modifier_id
character_modifier_test_slip_prevent_mod( "test_slip_prevent_mod" );
static const character_modifier_id character_modifier_test_thrown_dex_mod( "test_thrown_dex_mod" );
static const character_modifier_id
character_modifier_test_thrown_dex_mod_hand( "test_thrown_dex_mod_hand" );

static const damage_type_id damage_acid( "acid" );
static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_cut( "cut" );

static const enchantment_id enchantment_ENCH_TEST_BIRD_PARTS( "ENCH_TEST_BIRD_PARTS" );
static const enchantment_id enchantment_ENCH_TEST_TAIL( "ENCH_TEST_TAIL" );

static const itype_id itype_test_tail_encumber( "test_tail_encumber" );

static const limb_score_id limb_score_test( "test" );

static const trait_id trait_TEST_ARMOR_MUTATION( "TEST_ARMOR_MUTATION" );

static void create_char( Character &dude )
{
    clear_character( dude, true );

    dude.enchantment_cache->force_add( *enchantment_ENCH_TEST_TAIL, dude );
    dude.recalculate_bodyparts();
    REQUIRE( dude.has_part( body_part_test_tail ) );
}

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

TEST_CASE( "Basic_limb_score_test", "[character][encumbrance][limb]" )
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

TEST_CASE( "Basic_character_modifier_test", "[character][encumbrance][limb]" )
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

TEST_CASE( "Body_part_armor_vs_damage", "[character][limb]" )
{
    standard_npc dude( "Test NPC" );
    create_char( dude );
    const bodypart *test_bp = dude.get_part( body_part_test_tail );
    REQUIRE( test_bp != nullptr );

    GIVEN( "Body part with 5 bash / 5 acid / 0 cut" ) {
        CHECK( test_bp->get_id()->damage_resistance( damage_bash ) == Approx( 5.0 ).epsilon(
                   0.001 ) );
        CHECK( test_bp->get_id()->damage_resistance( damage_acid ) == Approx( 5.0 ).epsilon(
                   0.001 ) );
        CHECK( test_bp->get_id()->damage_resistance( damage_cut ) == Approx( 0.0 ).epsilon( 0.001 ) );

        WHEN( "Receiving bash damage" ) {
            damage_unit du( damage_bash, 10.f, 0.f );
            dude.passive_absorb_hit( body_part_test_tail, du );
            THEN( "Absorb hit" ) {
                CHECK( dude.get_armor_type( du.type, body_part_test_tail ) == 5 );
                CHECK( du.amount == Approx( 5.0 ).epsilon( 0.001 ) );
            }
        }

        WHEN( "Receiving acid damage" ) {
            damage_unit du( damage_acid, 10.f, 0.f );
            dude.passive_absorb_hit( body_part_test_tail, du );
            THEN( "Absorb hit" ) {
                CHECK( dude.get_armor_type( du.type, body_part_test_tail ) == 5 );
                CHECK( du.amount == Approx( 5.0 ).epsilon( 0.001 ) );
            }
        }

        WHEN( "Receiving cut damage" ) {
            damage_unit du( damage_cut, 10.f, 0.f );
            dude.passive_absorb_hit( body_part_test_tail, du );
            THEN( "Absorb hit" ) {
                CHECK( dude.get_armor_type( du.type, body_part_test_tail ) == 0 );
                CHECK( du.amount == Approx( 10.0 ).epsilon( 0.001 ) );
            }
        }
    }
}

static double get_limbtype_modval( const double &val, const bodypart_id &id,
                                   const std::set<body_part_type::type> &bp_types )
{
    double ret = 0.0;
    for( const body_part_type::type &bp_type : bp_types ) {
        if( id->has_type( bp_type ) ) {
            ret += val * id->limbtypes.at( bp_type );
        }
    }
    return ret;
}

TEST_CASE( "Mutation_armor_vs_damage", "[character][mutation][limb]" )
{
    standard_npc dude( "Test NPC" );
    clear_character( dude, true );
    dude.toggle_trait( trait_TEST_ARMOR_MUTATION );
    REQUIRE( dude.has_trait( trait_TEST_ARMOR_MUTATION ) );

    static const std::set<body_part_type::type> affected_bptypes = {
        body_part_type::type::arm,
        body_part_type::type::torso,
        body_part_type::type::tail
    };

    GIVEN( "5 bash on arms, torso and tail / 1 cut on ALL" ) {
        for( const auto &res : trait_TEST_ARMOR_MUTATION->armor ) {
            if( res.first->has_type( body_part_type::type::arm ) ||
                res.first->has_type( body_part_type::type::torso ) ||
                res.first->has_type( body_part_type::type::tail ) ) {
                CAPTURE( res.first.c_str() );
                const double bash_res = get_limbtype_modval( 5.0, res.first, affected_bptypes );
                CHECK( res.second.type_resist( damage_bash ) == Approx( bash_res ).epsilon( 0.001 ) );
                CHECK( res.second.type_resist( damage_acid ) == Approx( 0.0 ).epsilon( 0.001 ) );
                CHECK( res.second.type_resist( damage_cut ) == Approx( 1.0 ).epsilon( 0.001 ) );
            } else {
                CAPTURE( res.first.c_str() );
                CHECK( res.second.type_resist( damage_bash ) == Approx( 0.0 ).epsilon( 0.001 ) );
                CHECK( res.second.type_resist( damage_acid ) == Approx( 0.0 ).epsilon( 0.001 ) );
                CHECK( res.second.type_resist( damage_cut ) == Approx( 1.0 ).epsilon( 0.001 ) );
            }
        }

        WHEN( "Receiving bash damage" ) {
            for( const body_part_type &bp : body_part_type::get_all() ) {
                if( !dude.has_part( bp.id ) ) {
                    continue;
                }
                const bool has_res = bp.id->has_type( body_part_type::type::arm ) ||
                                     bp.id->has_type( body_part_type::type::torso ) ||
                                     bp.id->has_type( body_part_type::type::tail );
                double res_val = get_limbtype_modval( 5.0, bp.id, affected_bptypes );
                const int res_amt = has_res ? static_cast<int>( res_val ) : 0;
                damage_unit du( damage_bash, 10.f, 0.f );
                dude.passive_absorb_hit( bp.id, du );
                THEN( "Absorb hit" ) {
                    CAPTURE( bp.id.c_str() );
                    CHECK( dude.get_armor_type( du.type, bp.id ) == res_amt );
                    CHECK( du.amount == Approx( 10.0 - res_amt ).epsilon( 0.001 ) );
                }
            }
        }

        WHEN( "Receiving acid damage" ) {
            for( const body_part_type &bp : body_part_type::get_all() ) {
                if( !dude.has_part( bp.id ) ) {
                    continue;
                }
                damage_unit du( damage_acid, 10.f, 0.f );
                dude.passive_absorb_hit( bp.id, du );
                THEN( "Absorb hit" ) {
                    CAPTURE( bp.id.c_str() );
                    CHECK( dude.get_armor_type( du.type, bp.id ) == 0 );
                    CHECK( du.amount == Approx( 10.0 ).epsilon( 0.001 ) );
                }
            }
        }

        WHEN( "Receiving cut damage" ) {
            for( const body_part_type &bp : body_part_type::get_all() ) {
                if( !dude.has_part( bp.id ) ) {
                    continue;
                }
                damage_unit du( damage_cut, 10.f, 0.f );
                dude.passive_absorb_hit( bp.id, du );
                THEN( "Absorb hit" ) {
                    CAPTURE( bp.id.c_str() );
                    CHECK( dude.get_armor_type( du.type, bp.id ) == 1 );
                    CHECK( du.amount == Approx( 9.0 ).epsilon( 0.001 ) );
                }
            }
        }
    }

    GIVEN( "+ body part with 5 bash / 5 acid" ) {
        dude.enchantment_cache->force_add( *enchantment_ENCH_TEST_TAIL, dude );
        dude.recalculate_bodyparts();
        REQUIRE( dude.has_part( body_part_test_tail ) );

        WHEN( "Receiving bash damage" ) {
            for( const body_part_type &bp : body_part_type::get_all() ) {
                if( !dude.has_part( bp.id ) ) {
                    continue;
                }
                const bool has_mut_res = bp.id->has_type( body_part_type::type::arm ) ||
                                         bp.id->has_type( body_part_type::type::torso ) ||
                                         bp.id->has_type( body_part_type::type::tail );
                const bool has_bp_res = bp.id == body_part_test_tail;
                double res_val = get_limbtype_modval( 5.0, bp.id, affected_bptypes );
                const int res_amt = ( has_mut_res ? static_cast<int>( res_val ) : 0 ) + ( has_bp_res ? 5 : 0 );
                damage_unit du( damage_bash, 10.f, 0.f );
                dude.passive_absorb_hit( bp.id, du );
                THEN( "Absorb hit" ) {
                    CAPTURE( bp.id.c_str() );
                    CHECK( dude.get_armor_type( du.type, bp.id ) == res_amt );
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
                damage_unit du( damage_acid, 10.f, 0.f );
                dude.passive_absorb_hit( bp.id, du );
                THEN( "Absorb hit" ) {
                    CAPTURE( bp.id.c_str() );
                    CHECK( dude.get_armor_type( du.type, bp.id ) == res_amt );
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
                damage_unit du( damage_cut, 10.f, 0.f );
                dude.passive_absorb_hit( bp.id, du );
                THEN( "Absorb hit" ) {
                    CAPTURE( bp.id.c_str() );
                    CHECK( dude.get_armor_type( du.type, bp.id ) == res_amt );
                    CHECK( du.amount == Approx( 10.0 - res_amt ).epsilon( 0.001 ) );
                }
            }
        }
    }
}

TEST_CASE( "Multi-limbscore_modifiers", "[character][limb]" )
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
        REQUIRE( dude.encumb( dude.get_all_body_parts_of_type( body_part_type::type::sensor,
                              get_body_part_flags::primary_type ).front() ) == 60 );
        CHECK( dude.get_modifier( character_modifier_test_add_limbscores_mod ) == Approx( 2.0 ).epsilon(
                   0.001 ) );
        CHECK( dude.get_modifier( character_modifier_test_mult_limbscores_mod ) == Approx( 0.1 ).epsilon(
                   0.001 ) );
    }

    WHEN( "Character has broken arms" ) {
        for( const bodypart_id &bid : dude.get_all_body_parts_of_type( body_part_type::type::arm,
                get_body_part_flags::primary_type ) ) {
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
        for( const bodypart_id &bid : dude.get_all_body_parts_of_type( body_part_type::type::arm,
                get_body_part_flags::primary_type ) ) {
            dude.set_part_hp_cur( bid, 0 );
        }
        REQUIRE( dude.get_working_arm_count() == 0 );
        REQUIRE( dude.encumb( dude.get_all_body_parts_of_type( body_part_type::type::sensor,
                              get_body_part_flags::primary_type ).front() ) == 60 );
        CHECK( dude.get_modifier( character_modifier_test_add_limbscores_mod ) == Approx( 0.4 ).epsilon(
                   0.001 ) );
        CHECK( dude.get_modifier( character_modifier_test_mult_limbscores_mod ) == Approx( 0.1 ).epsilon(
                   0.001 ) );
    }
}

TEST_CASE( "Slip_prevention_modifier_/_weighted-list_multi-score_modifiers", "[character][limb]" )
{
    standard_npc dude( "Test NPC" );
    create_char( dude );

    WHEN( "Character is uninjured / unencumbered" ) {
        CHECK( dude.get_modifier( character_modifier_test_slip_prevent_mod ) == Approx( 1.0 ).epsilon(
                   0.001 ) );
    }

    WHEN( "Character has broken arms" ) {
        for( const bodypart_id &bid : dude.get_all_body_parts_of_type( body_part_type::type::arm,
                get_body_part_flags::primary_type ) ) {
            dude.set_part_hp_cur( bid, 0 );
        }
        REQUIRE( dude.get_working_arm_count() == 0 );
        CHECK( dude.get_modifier( character_modifier_test_slip_prevent_mod ) == Approx( 2.0 / 3.0 ).epsilon(
                   0.001 ) );
    }

    WHEN( "Character is heavily encumbered" ) {
        item hazmat_suit( "test_hazmat_suit" );
        dude.wear_item( hazmat_suit );
        REQUIRE( dude.encumb( dude.get_all_body_parts_of_type( body_part_type::type::foot,
                              get_body_part_flags::primary_type ).front() ) == 37 );
        REQUIRE( dude.encumb( dude.get_all_body_parts_of_type( body_part_type::type::hand,
                              get_body_part_flags::primary_type ).front() ) == 37 );
        CHECK( dude.get_modifier( character_modifier_test_slip_prevent_mod ) == Approx( 0.623 ).epsilon(
                   0.001 ) );
    }

    WHEN( "Character has broken arms and is heavily encumbered" ) {
        item hazmat_suit( "test_hazmat_suit" );
        dude.wear_item( hazmat_suit );
        for( const bodypart_id &bid : dude.get_all_body_parts_of_type( body_part_type::type::arm,
                get_body_part_flags::primary_type ) ) {
            dude.set_part_hp_cur( bid, 0 );
        }
        REQUIRE( dude.encumb( dude.get_all_body_parts_of_type( body_part_type::type::foot,
                              get_body_part_flags::primary_type ).front() ) == 37 );
        REQUIRE( dude.encumb( dude.get_all_body_parts_of_type( body_part_type::type::hand,
                              get_body_part_flags::primary_type ).front() ) == 37 );
        REQUIRE( dude.get_working_arm_count() == 0 );
        CHECK( dude.get_modifier( character_modifier_test_slip_prevent_mod ) == Approx( 0.41 ).epsilon(
                   0.001 ) );
    }
}

TEST_CASE( "Weighted_limb_types", "[character][limb]" )
{
    item boxing_gloves( "test_boxing_gloves" );
    item wing_covers( "test_winglets" );
    standard_npc dude( "Test NPC" );
    clear_character( dude, true );

    GIVEN( "Character has normal limbs" ) {
        REQUIRE( dude.get_all_body_parts().size() == 12 );
        REQUIRE( dude.get_all_body_parts_of_type( body_part_type::type::hand ).size() == 2 );
        WHEN( "Uninjured / unencumbered" ) {
            REQUIRE( dude.get_hp() == dude.get_hp_max() );
            REQUIRE( dude.get_part_hp_cur( body_part_hand_l ) == dude.get_part_hp_max( body_part_hand_l ) );
            REQUIRE( dude.get_part_hp_cur( body_part_hand_r ) == dude.get_part_hp_max( body_part_hand_r ) );
            REQUIRE( dude.avg_encumb_of_limb_type( body_part_type::type::hand ) == 0 );
            CHECK( dude.get_modifier( character_modifier_test_ranged_dispersion_manip_mod ) == Approx(
                       0.0 ).epsilon( 0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod ) == Approx( 1.0 ).epsilon(
                       0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod_hand ) == Approx( 1.0 ).epsilon(
                       0.01 ) );
        }
        WHEN( "Damaged hands / unencumbered" ) {
            dude.set_part_hp_cur( body_part_hand_l, dude.get_part_hp_max( body_part_hand_l ) / 2 );
            dude.set_part_hp_cur( body_part_hand_r, dude.get_part_hp_max( body_part_hand_l ) / 2 );
            REQUIRE( dude.get_part_hp_cur( body_part_hand_l ) == dude.get_part_hp_max( body_part_hand_l ) / 2 );
            REQUIRE( dude.get_part_hp_cur( body_part_hand_r ) == dude.get_part_hp_max( body_part_hand_l ) / 2 );
            REQUIRE( dude.avg_encumb_of_limb_type( body_part_type::type::hand ) == 0 );
            CHECK( dude.get_modifier( character_modifier_test_ranged_dispersion_manip_mod ) == Approx(
                       11.8 ).epsilon( 0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod ) == Approx( 0.658 ).epsilon(
                       0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod_hand ) == Approx( 0.658 ).epsilon(
                       0.01 ) );
        }
        WHEN( "Uninjured / hands encumbered" ) {
            dude.wear_item( boxing_gloves, false );
            REQUIRE( dude.get_hp() == dude.get_hp_max() );
            REQUIRE( dude.get_part_hp_cur( body_part_hand_l ) == dude.get_part_hp_max( body_part_hand_l ) );
            REQUIRE( dude.get_part_hp_cur( body_part_hand_r ) == dude.get_part_hp_max( body_part_hand_r ) );
            REQUIRE( dude.avg_encumb_of_limb_type( body_part_type::type::hand ) == 70 );
            CHECK( dude.get_modifier( character_modifier_test_ranged_dispersion_manip_mod ) == Approx(
                       28.0 ).epsilon( 0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod ) == Approx( 0.449 ).epsilon(
                       0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod_hand ) == Approx( 0.449 ).epsilon(
                       0.01 ) );
        }
        WHEN( "Damaged hands / hands encumbered" ) {
            dude.wear_item( boxing_gloves, false );
            dude.set_part_hp_cur( body_part_hand_l, dude.get_part_hp_max( body_part_hand_l ) / 2 );
            dude.set_part_hp_cur( body_part_hand_r, dude.get_part_hp_max( body_part_hand_l ) / 2 );
            REQUIRE( dude.get_part_hp_cur( body_part_hand_l ) == dude.get_part_hp_max( body_part_hand_l ) / 2 );
            REQUIRE( dude.get_part_hp_cur( body_part_hand_r ) == dude.get_part_hp_max( body_part_hand_l ) / 2 );
            REQUIRE( dude.avg_encumb_of_limb_type( body_part_type::type::hand ) == 70 );
            CHECK( dude.get_modifier( character_modifier_test_ranged_dispersion_manip_mod ) == Approx(
                       54.3 ).epsilon( 0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod ) == Approx( 0.295 ).epsilon(
                       0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod_hand ) == Approx( 0.295 ).epsilon(
                       0.01 ) );
        }
    }

    GIVEN( "Character has mutated limbs" ) {
        create_bird_char( dude );
        REQUIRE( dude.get_all_body_parts().size() == 12 );
        REQUIRE( dude.get_all_body_parts_of_type( body_part_type::type::hand ).size() == 5 );
        WHEN( "Uninjured / unencumbered" ) {
            REQUIRE( dude.get_hp() == dude.get_hp_max() );
            REQUIRE( dude.get_part_hp_cur( body_part_test_bird_wing_l ) == dude.get_part_hp_max(
                         body_part_test_bird_wing_l ) );
            REQUIRE( dude.get_part_hp_cur( body_part_test_bird_wing_r ) == dude.get_part_hp_max(
                         body_part_test_bird_wing_r ) );
            REQUIRE( dude.avg_encumb_of_limb_type( body_part_type::type::wing ) == 0 );
            CHECK( dude.get_modifier( character_modifier_test_ranged_dispersion_manip_mod ) == Approx(
                       129.2 ).epsilon( 0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod ) == Approx( 0.8 ).epsilon(
                       0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod_hand ) == Approx( 0.15 ).epsilon(
                       0.01 ) );
        }
        WHEN( "Damaged wings / unencumbered" ) {
            dude.set_part_hp_cur( body_part_test_bird_wing_l,
                                  dude.get_part_hp_max( body_part_test_bird_wing_l ) / 2 );
            dude.set_part_hp_cur( body_part_test_bird_wing_r,
                                  dude.get_part_hp_max( body_part_test_bird_wing_r ) / 2 );
            REQUIRE( dude.get_part_hp_cur( body_part_test_bird_wing_l ) == dude.get_part_hp_max(
                         body_part_test_bird_wing_l ) / 2 );
            REQUIRE( dude.get_part_hp_cur( body_part_test_bird_wing_r ) == dude.get_part_hp_max(
                         body_part_test_bird_wing_r ) / 2 );
            REQUIRE( dude.avg_encumb_of_limb_type( body_part_type::type::wing ) == 0 );
            CHECK( dude.get_modifier( character_modifier_test_ranged_dispersion_manip_mod ) == Approx(
                       140.057 ).epsilon( 0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod ) == Approx( 0.533 ).epsilon(
                       0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod_hand ) == Approx( 0.14 ).epsilon(
                       0.01 ) );
        }
        WHEN( "Uninjured / wings encumbered" ) {
            dude.wear_item( wing_covers, false );
            REQUIRE( dude.get_hp() == dude.get_hp_max() );
            REQUIRE( dude.get_part_hp_cur( body_part_test_bird_wing_l ) == dude.get_part_hp_max(
                         body_part_test_bird_wing_l ) );
            REQUIRE( dude.get_part_hp_cur( body_part_test_bird_wing_r ) == dude.get_part_hp_max(
                         body_part_test_bird_wing_r ) );
            REQUIRE( dude.avg_encumb_of_limb_type( body_part_type::type::wing ) == 70 );
            CHECK( dude.get_modifier( character_modifier_test_ranged_dispersion_manip_mod ) == Approx(
                       173.641 ).epsilon( 0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod ) == Approx( 0.4 ).epsilon(
                       0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod_hand ) == Approx( 0.116 ).epsilon(
                       0.01 ) );
        }
        WHEN( "Damaged wings / wings encumbered" ) {
            dude.wear_item( wing_covers, false );
            dude.set_part_hp_cur( body_part_test_bird_wing_l,
                                  dude.get_part_hp_max( body_part_test_bird_wing_l ) / 2 );
            dude.set_part_hp_cur( body_part_test_bird_wing_r,
                                  dude.get_part_hp_max( body_part_test_bird_wing_r ) / 2 );
            REQUIRE( dude.get_part_hp_cur( body_part_test_bird_wing_l ) == dude.get_part_hp_max(
                         body_part_test_bird_wing_l ) / 2 );
            REQUIRE( dude.get_part_hp_cur( body_part_test_bird_wing_r ) == dude.get_part_hp_max(
                         body_part_test_bird_wing_r ) / 2 );
            REQUIRE( dude.avg_encumb_of_limb_type( body_part_type::type::wing ) == 70 );
            CHECK( dude.get_modifier( character_modifier_test_ranged_dispersion_manip_mod ) == Approx(
                       211.341 ).epsilon( 0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod ) == Approx( 0.4 ).epsilon(
                       0.01 ) );
            CHECK( dude.get_modifier( character_modifier_test_thrown_dex_mod_hand ) == Approx( 0.097 ).epsilon(
                       0.01 ) );
        }
    }
}
