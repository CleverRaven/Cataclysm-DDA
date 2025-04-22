#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "morale.h"
#include "npc.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "point.h"
#include "subbodypart.h"
#include "type_id.h"

static const efftype_id effect_cold( "cold" );
static const efftype_id effect_hot( "hot" );
static const efftype_id effect_took_prozac( "took_prozac" );

static const itype_id itype_boots( "boots" );
static const itype_id itype_legpouch( "legpouch" );
static const itype_id itype_shotgun_s( "shotgun_s" );
static const itype_id itype_tinfoil_hat( "tinfoil_hat" );

static const morale_type morale_book( "morale_book" );
static const morale_type morale_food_bad( "morale_food_bad" );
static const morale_type morale_food_good( "morale_food_good" );
static const morale_type morale_killed_innocent( "morale_killed_innocent" );
static const morale_type morale_perm_badtemper( "morale_perm_badtemper" );
static const morale_type morale_perm_constrained( "morale_perm_constrained" );
static const morale_type morale_perm_masochist( "morale_perm_masochist" );
static const morale_type morale_perm_optimist( "morale_perm_optimist" );
static const morale_type morale_wet( "morale_wet" );

static const trait_id trait_BADTEMPER( "BADTEMPER" );
static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_DEBUG_CLAIRVOYANCE( "DEBUG_CLAIRVOYANCE" );
static const trait_id trait_FLOWERS( "FLOWERS" );
static const trait_id trait_MASOCHIST( "MASOCHIST" );
static const trait_id trait_OPTIMISTIC( "OPTIMISTIC" );
static const trait_id trait_PLANT( "PLANT" );
static const trait_id trait_ROOTS1( "ROOTS1" );

TEST_CASE( "player_morale_empty", "[player_morale]" )
{
    player_morale m;

    GIVEN( "an empty morale" ) {
        CHECK( m.get_level() == 0 );
    }
}

TEST_CASE( "player_morale_decay", "[player_morale]" )
{
    player_morale m;

    GIVEN( "temporary morale (food)" ) {
        m.add( morale_food_good, 20, 40, 20_turns, 10_turns );
        m.add( morale_food_bad, -10, -20, 20_turns, 10_turns );

        CHECK( m.has( morale_food_good ) == 20 );
        CHECK( m.has( morale_food_bad ) == -10 );
        CHECK( m.get_level() == 10 );

        WHEN( "it decays" ) {
            AND_WHEN( "it's just started" ) {
                m.decay( 10_turns );
                CHECK( m.has( morale_food_good ) == 20 );
                CHECK( m.has( morale_food_bad ) == -10 );
                CHECK( m.get_level() == 10 );
            }
            AND_WHEN( "it's halfway there" ) {
                m.decay( 15_turns );
                CHECK( m.has( morale_food_good ) == 10 );
                CHECK( m.has( morale_food_bad ) == -5 );
                CHECK( m.get_level() == 5 );
            }
            AND_WHEN( "it's finished" ) {
                m.decay( 20_turns );
                CHECK( m.has( morale_food_good ) == 0 );
                CHECK( m.has( morale_food_bad ) == 0 );
                CHECK( m.get_level() == 0 );
            }
        }

        WHEN( "it gets deleted" ) {
            AND_WHEN( "good one gets deleted" ) {
                m.remove( morale_food_good );
                CHECK( m.get_level() == -10 );
            }
            AND_WHEN( "bad one gets deleted" ) {
                m.remove( morale_food_bad );
                CHECK( m.get_level() == 20 );
            }
            AND_WHEN( "both get deleted" ) {
                m.remove( morale_food_bad );
                m.remove( morale_food_good );
                CHECK( m.get_level() == 0 );
            }
        }

        WHEN( "it gets cleared" ) {
            m.clear();
            CHECK( m.get_level() == 0 );
        }

        WHEN( "it's added/subtracted (no cap)" ) {
            m.add( morale_food_good, 10, 40, 20_turns, 10_turns, false );
            m.add( morale_food_bad, -10, -20, 20_turns, 10_turns, false );

            CHECK( m.has( morale_food_good ) == 22 );
            CHECK( m.has( morale_food_bad ) == -14 );
            CHECK( m.get_level() == 8 );

        }

        WHEN( "it's added/subtracted (with a cap)" ) {
            m.add( morale_food_good, 5, 10, 20_turns, 10_turns, true );
            m.add( morale_food_bad, -5, -10, 20_turns, 10_turns, true );

            CHECK( m.has( morale_food_good ) == 10 );
            CHECK( m.has( morale_food_bad ) == -10 );
            CHECK( m.get_level() == 0 );
        }
    }
}

TEST_CASE( "player_morale_persistent", "[player_morale]" )
{
    player_morale m;

    GIVEN( "persistent morale" ) {
        m.set_permanent( morale_perm_masochist, 5 );

        CHECK( m.has( morale_perm_masochist ) == 5 );

        WHEN( "it decays" ) {
            m.decay( 100_turns );
            THEN( "nothing happens" ) {
                CHECK( m.has( morale_perm_masochist ) == 5 );
                CHECK( m.get_level() == 5 );
            }
        }
    }
}

TEST_CASE( "player_morale_optimist", "[player_morale]" )
{
    player_morale m;

    GIVEN( "OPTIMISTIC trait" ) {
        m.on_mutation_gain( trait_OPTIMISTIC );
        CHECK( m.has( morale_perm_optimist ) == 9 );
        CHECK( m.get_level() == 10 );

        WHEN( "lost the trait" ) {
            m.on_mutation_loss( trait_OPTIMISTIC );
            CHECK( m.has( morale_perm_optimist ) == 0 );
            CHECK( m.get_level() == 0 );
        }
    }
}

TEST_CASE( "player_morale_bad_temper", "[player_morale]" )
{
    player_morale m;

    GIVEN( "BADTEMPER trait" ) {
        m.on_mutation_gain( trait_BADTEMPER );
        CHECK( m.has( morale_perm_badtemper ) == -9 );
        CHECK( m.get_level() == -10 );

        WHEN( "lost the trait" ) {
            m.on_mutation_loss( trait_BADTEMPER );
            CHECK( m.has( morale_perm_badtemper ) == 0 );
            CHECK( m.get_level() == 0 );
        }
    }
}

TEST_CASE( "player_morale_killed_innocent_affected_by_prozac", "[player_morale]" )
{
    player_morale m;

    GIVEN( "killed an innocent" ) {
        m.add( morale_killed_innocent, -100 );

        WHEN( "took prozac" ) {
            m.on_effect_int_change( effect_took_prozac, 1 );

            THEN( "it's not so bad" ) {
                CHECK( m.get_level() == -25 );

                AND_WHEN( "the effect ends" ) {
                    m.on_effect_int_change( effect_took_prozac, 0 );

                    THEN( "guilt returns" ) {
                        CHECK( m.get_level() == -100 );
                    }
                }
            }
        }
    }
}

TEST_CASE( "player_morale_murdered_innocent", "[player_morale]" )
{
    map &here = get_map();
    clear_avatar();
    Character &player = get_player_character();
    player_morale &m = *player.morale;
    tripoint_bub_ms next_to = player.adjacent_tile();
    standard_npc innocent( "Lapin", next_to, {}, 0, 8, 8, 8, 7 );
    // Innocent as could be.
    faction_id lapin( "lapin" );
    innocent.set_fac( lapin );
    innocent.setpos( here, next_to );
    innocent.set_all_parts_hp_cur( 1 );
    CHECK( m.get_total_positive_value() == 0 );
    CHECK( m.get_total_negative_value() == 0 );
    CHECK( innocent.hit_by_player == false );
    while( !innocent.is_dead_state() ) {
        player.mod_moves( 1000 );
        player.melee_attack( innocent, true );
    }
    // Death is just a data state, we can still do useful checks even after they're dead.
    CHECK( innocent.hit_by_player == true );
    REQUIRE( m.get_total_negative_value() == 90 );
}

TEST_CASE( "player_morale_kills_hostile_bandit", "[player_morale]" )
{
    map &here = get_map();
    clear_avatar();
    Character &player = get_player_character();
    player_morale &m = *player.morale;
    tripoint_bub_ms next_to = player.adjacent_tile();
    standard_npc badguy( "Raider", next_to, {}, 0, 8, 8, 8, 7 );
    // Always-hostile
    faction_id hells_raiders( "hells_raiders" );
    badguy.set_fac( hells_raiders );
    badguy.setpos( here, next_to );
    badguy.set_all_parts_hp_cur( 1 );
    CHECK( m.get_total_positive_value() == 0 );
    CHECK( m.get_total_negative_value() == 0 );
    CHECK( badguy.guaranteed_hostile() == true );
    while( !badguy.is_dead_state() ) {
        player.mod_moves( 1000 );
        player.melee_attack( badguy, true );
    }
    REQUIRE( m.get_total_negative_value() == 0 );
}

TEST_CASE( "player_morale_ranged_kill_of_unaware_hostile_bandit", "[player_morale]" )
{
    map &here = get_map();

    clear_avatar();
    avatar &player = get_avatar();
    // Set the time to midnight to ensure the bandit doesn't notice the player.
    set_time( calendar::turn_zero + 0_hours );
    // Give us a clairvoyance to clearly see the target in the night.
    player.set_mutation( trait_DEBUG_CLAIRVOYANCE );
    player_morale &m = *player.morale;
    const tripoint_bub_ms bandit_pos = player.pos_bub() + ( point::east * 2 );
    npc &badguy = spawn_npc( bandit_pos.xy(), "thug" );
    badguy.set_all_parts_hp_cur( 1 );
    CHECK( m.get_total_positive_value() == 0 );
    CHECK( m.get_total_negative_value() == 0 );
    CHECK( badguy.guaranteed_hostile() == true );
    CHECK( badguy.sees( here,  player.pos_bub( here ) ) == false );
    for( size_t loop = 0; loop < 1000; loop++ ) {
        player.set_body();
        arm_shooter( player, itype_shotgun_s );
        player.recoil = 0;
        player.fire_gun( here, bandit_pos, 1, *player.get_wielded_item() );
        if( badguy.is_dead_state() ) {
            break;
        }
    }
    CHECK( badguy.is_dead_state() == true );
    REQUIRE( m.get_total_negative_value() == 0 );
}

TEST_CASE( "player_morale_masochist", "[player_morale]" )
{
    player_morale m;

    GIVEN( "masochist trait" ) {
        m.on_mutation_gain( trait_MASOCHIST );

        CHECK( m.has( morale_perm_masochist ) == 0 );

        WHEN( "in minimal pain" ) {
            m.on_stat_change( "perceived_pain", 10 );
            CHECK( m.has( morale_perm_masochist ) == 10 );
        }

        WHEN( "in mind pain" ) {
            m.on_stat_change( "perceived_pain", 20 );
            CHECK( m.has( morale_perm_masochist ) == 20 );
        }

        WHEN( "in an insufferable pain" ) {
            m.on_stat_change( "perceived_pain", 120 );
            THEN( "there's a limit" ) {
                CHECK( m.has( morale_perm_masochist ) == 20 );
            }
        }
    }

    GIVEN( "masochist morale table" ) {
        m.on_mutation_gain( trait_MASOCHIST );

        CHECK( m.has( morale_perm_masochist ) == 0 );

        WHEN( "in minimal pain" ) {
            m.on_stat_change( "perceived_pain", 10 );
            CHECK( m.has( morale_perm_masochist ) - ( m.get_perceived_pain() > 20 ? m.get_perceived_pain() -
                    20 : 0 ) == 10 );
        }

        WHEN( "in mind pain" ) {
            m.on_stat_change( "perceived_pain", 20 );
            CHECK( m.has( morale_perm_masochist ) - ( m.get_perceived_pain() > 20 ? m.get_perceived_pain() -
                    20 : 0 ) == 20 );
        }

        WHEN( "in moderate pain" ) {
            m.on_stat_change( "perceived_pain", 30 );
            CHECK( m.has( morale_perm_masochist ) - ( m.get_perceived_pain() > 20 ? m.get_perceived_pain() -
                    20 : 0 ) == 10 );
        }

        WHEN( "in distracting pain" ) {
            m.on_stat_change( "perceived_pain", 40 );
            CHECK( m.has( morale_perm_masochist ) - ( m.get_perceived_pain() > 20 ? m.get_perceived_pain() -
                    20 : 0 ) == 0 );
        }

        WHEN( "in distressing pain" ) {
            m.on_stat_change( "perceived_pain", 50 );
            CHECK( m.has( morale_perm_masochist ) - ( m.get_perceived_pain() > 20 ? m.get_perceived_pain() -
                    20 : 0 ) == -10 );
        }

        WHEN( "in unmanagable pain" ) {
            m.on_stat_change( "perceived_pain", 60 );
            CHECK( m.has( morale_perm_masochist ) - ( m.get_perceived_pain() > 20 ? m.get_perceived_pain() -
                    20 : 0 ) == -20 );
        }

        WHEN( "in intense pain" ) {
            m.on_stat_change( "perceived_pain", 70 );
            CHECK( m.has( morale_perm_masochist ) - ( m.get_perceived_pain() > 20 ? m.get_perceived_pain() -
                    20 : 0 ) == -30 );
        }

        WHEN( "in severe pain" ) {
            m.on_stat_change( "perceived_pain", 80 );
            CHECK( m.has( morale_perm_masochist ) - ( m.get_perceived_pain() > 20 ? m.get_perceived_pain() -
                    20 : 0 ) == -40 );
        }
    }
}

TEST_CASE( "player_morale_cenobite", "[player_morale]" )
{
    player_morale m;

    GIVEN( "cenobite trait" ) {
        m.on_mutation_gain( trait_CENOBITE );

        CHECK( m.has( morale_perm_masochist ) == 0 );

        WHEN( "in an insufferable pain" ) {
            m.on_stat_change( "perceived_pain", 120 );

            THEN( "there's no limit" ) {
                CHECK( m.has( morale_perm_masochist ) == 120 );
            }

            AND_WHEN( "took prozac" ) {
                m.on_effect_int_change( effect_took_prozac, 1 );
                THEN( "it spoils all fun" ) {
                    CHECK( m.has( morale_perm_masochist ) == 60 );
                }
            }
        }
    }
}

TEST_CASE( "player_morale_plant", "[player_morale]" )
{
    player_morale m;

    GIVEN( "a humanoid plant" ) {
        m.on_mutation_gain( trait_PLANT );
        m.on_mutation_gain( trait_FLOWERS );
        m.on_mutation_gain( trait_ROOTS1 );

        CHECK( m.has( morale_perm_constrained ) == 0 );

        WHEN( "wearing a hat" ) {
            const item hat( itype_tinfoil_hat, calendar::turn_zero );

            m.on_item_wear( hat );
            THEN( "the flowers need sunlight" ) {
                CHECK( m.has( morale_perm_constrained ) == -10 );

                AND_WHEN( "taking it off again" ) {
                    m.on_item_takeoff( hat );
                    CHECK( m.has( morale_perm_constrained ) == 0 );
                }
            }
        }

        WHEN( "wearing a legpouch" ) {
            item legpouch( itype_legpouch, calendar::turn_zero );
            legpouch.set_side( side::LEFT );

            m.on_item_wear( legpouch );
            THEN( "half of the roots are suffering" ) {
                CHECK( m.has( morale_perm_constrained ) == -5 );
            }
        }

        WHEN( "wearing a pair of boots" ) {
            const item boots( itype_boots, calendar::turn_zero );

            m.on_item_wear( boots );
            THEN( "all of the roots are suffering" ) {
                CHECK( m.has( morale_perm_constrained ) == -10 );
            }

            AND_WHEN( "even more constrains" ) {
                const item hat( itype_tinfoil_hat, calendar::turn_zero );

                m.on_item_wear( hat );
                THEN( "it can't be worse" ) {
                    CHECK( m.has( morale_perm_constrained ) == -10 );
                }
            }
        }
    }
}

TEST_CASE( "player_morale_tough_temperature", "[player_morale]" )
{
    player_morale m;

    GIVEN( "tough temperature conditions" ) {
        WHEN( "chilly" ) {
            m.on_effect_int_change( effect_cold, 1, bodypart_id( "torso" ) );
            m.on_effect_int_change( effect_cold, 1, bodypart_id( "head" ) );
            m.on_effect_int_change( effect_cold, 1, bodypart_id( "eyes" ) );
            m.on_effect_int_change( effect_cold, 1, bodypart_id( "mouth" ) );
            m.on_effect_int_change( effect_cold, 1, bodypart_id( "arm_l" ) );
            m.on_effect_int_change( effect_cold, 1, bodypart_id( "arm_r" ) );
            m.on_effect_int_change( effect_cold, 1, bodypart_id( "leg_l" ) );
            m.on_effect_int_change( effect_cold, 1, bodypart_id( "leg_r" ) );
            m.on_effect_int_change( effect_cold, 1, bodypart_id( "hand_l" ) );
            m.on_effect_int_change( effect_cold, 1, bodypart_id( "hand_r" ) );
            m.on_effect_int_change( effect_cold, 1, bodypart_id( "foot_l" ) );
            m.on_effect_int_change( effect_cold, 1, bodypart_id( "foot_r" ) );

            AND_WHEN( "no time has passed" ) {
                CHECK( m.get_level() == 0 );
            }
            AND_WHEN( "1 turn has passed" ) {
                m.decay( 1_turns );
                CHECK( m.get_level() == -2 );
            }
            AND_WHEN( "2 turns have passed" ) {
                m.decay( 2_turns );
                CHECK( m.get_level() == -4 );
            }
            AND_WHEN( "3 turns have passed" ) {
                m.decay( 3_turns );
                CHECK( m.get_level() == -6 );
            }
            AND_WHEN( "6 minutes have passed" ) {
                m.decay( 6_minutes );
                CHECK( m.get_level() == -10 );
            }
        }

        WHEN( "cold" ) {
            m.on_effect_int_change( effect_cold, 2, bodypart_id( "torso" ) );
            m.on_effect_int_change( effect_cold, 2, bodypart_id( "head" ) );
            m.on_effect_int_change( effect_cold, 2, bodypart_id( "eyes" ) );
            m.on_effect_int_change( effect_cold, 2, bodypart_id( "mouth" ) );
            m.on_effect_int_change( effect_cold, 2, bodypart_id( "arm_l" ) );
            m.on_effect_int_change( effect_cold, 2, bodypart_id( "arm_r" ) );
            m.on_effect_int_change( effect_cold, 2, bodypart_id( "leg_l" ) );
            m.on_effect_int_change( effect_cold, 2, bodypart_id( "leg_r" ) );
            m.on_effect_int_change( effect_cold, 2, bodypart_id( "hand_l" ) );
            m.on_effect_int_change( effect_cold, 2, bodypart_id( "hand_r" ) );
            m.on_effect_int_change( effect_cold, 2, bodypart_id( "foot_l" ) );
            m.on_effect_int_change( effect_cold, 2, bodypart_id( "foot_r" ) );

            AND_WHEN( "no time has passed" ) {
                CHECK( m.get_level() == 0 );
            }
            AND_WHEN( "1 turn has passed" ) {
                m.decay( 1_turns );
                CHECK( m.get_level() == -2 );
            }
            AND_WHEN( "9 turns have passed" ) {
                m.decay( 9_turns );
                CHECK( m.get_level() == -18 );
            }
            AND_WHEN( "1 minute has passed" ) {
                m.decay( 1_minutes );
                CHECK( m.get_level() == -20 );
            }
            AND_WHEN( "6 minutes have passed" ) {
                m.decay( 6_minutes );
                CHECK( m.get_level() == -20 );
            }
            AND_WHEN( "warmed up afterwards" ) {
                m.on_effect_int_change( effect_cold, 0, bodypart_id( "torso" ) );
                m.on_effect_int_change( effect_cold, 0, bodypart_id( "head" ) );
                m.on_effect_int_change( effect_cold, 0, bodypart_id( "eyes" ) );
                m.on_effect_int_change( effect_cold, 0, bodypart_id( "mouth" ) );
                m.on_effect_int_change( effect_cold, 0, bodypart_id( "arm_l" ) );
                m.on_effect_int_change( effect_cold, 0, bodypart_id( "arm_r" ) );
                m.on_effect_int_change( effect_cold, 0, bodypart_id( "leg_l" ) );
                m.on_effect_int_change( effect_cold, 0, bodypart_id( "leg_r" ) );
                m.on_effect_int_change( effect_cold, 0, bodypart_id( "hand_l" ) );
                m.on_effect_int_change( effect_cold, 0, bodypart_id( "hand_r" ) );
                m.on_effect_int_change( effect_cold, 0, bodypart_id( "foot_l" ) );
                m.on_effect_int_change( effect_cold, 0, bodypart_id( "foot_r" ) );

                m.decay( 1_minutes );
                CHECK( m.get_level() == 0 );
            }
        }

        WHEN( "warm" ) {
            m.on_effect_int_change( effect_hot, 1, bodypart_id( "torso" ) );
            m.on_effect_int_change( effect_hot, 1, bodypart_id( "head" ) );
            m.on_effect_int_change( effect_hot, 1, bodypart_id( "eyes" ) );
            m.on_effect_int_change( effect_hot, 1, bodypart_id( "mouth" ) );
            m.on_effect_int_change( effect_hot, 1, bodypart_id( "arm_l" ) );
            m.on_effect_int_change( effect_hot, 1, bodypart_id( "arm_r" ) );
            m.on_effect_int_change( effect_hot, 1, bodypart_id( "leg_l" ) );
            m.on_effect_int_change( effect_hot, 1, bodypart_id( "leg_r" ) );
            m.on_effect_int_change( effect_hot, 1, bodypart_id( "hand_l" ) );
            m.on_effect_int_change( effect_hot, 1, bodypart_id( "hand_r" ) );
            m.on_effect_int_change( effect_hot, 1, bodypart_id( "foot_l" ) );
            m.on_effect_int_change( effect_hot, 1, bodypart_id( "foot_r" ) );

            AND_WHEN( "no time has passed" ) {
                CHECK( m.get_level() == 0 );
            }
            AND_WHEN( "1 turn has passed" ) {
                m.decay( 1_turns );
                CHECK( m.get_level() == -2 );
            }
            AND_WHEN( "2 turns have passed" ) {
                m.decay( 2_turns );
                CHECK( m.get_level() == -4 );
            }
            AND_WHEN( "3 turns have passed" ) {
                m.decay( 3_turns );
                CHECK( m.get_level() == -6 );
            }
            AND_WHEN( "6 minutes have passed" ) {
                m.decay( 6_minutes );
                CHECK( m.get_level() == -10 );
            }
        }

        WHEN( "hot" ) {
            m.on_effect_int_change( effect_hot, 2, bodypart_id( "torso" ) );
            m.on_effect_int_change( effect_hot, 2, bodypart_id( "head" ) );
            m.on_effect_int_change( effect_hot, 2, bodypart_id( "eyes" ) );
            m.on_effect_int_change( effect_hot, 2, bodypart_id( "mouth" ) );
            m.on_effect_int_change( effect_hot, 2, bodypart_id( "arm_l" ) );
            m.on_effect_int_change( effect_hot, 2, bodypart_id( "arm_r" ) );
            m.on_effect_int_change( effect_hot, 2, bodypart_id( "leg_l" ) );
            m.on_effect_int_change( effect_hot, 2, bodypart_id( "leg_r" ) );
            m.on_effect_int_change( effect_hot, 2, bodypart_id( "hand_l" ) );
            m.on_effect_int_change( effect_hot, 2, bodypart_id( "hand_r" ) );
            m.on_effect_int_change( effect_hot, 2, bodypart_id( "foot_l" ) );
            m.on_effect_int_change( effect_hot, 2, bodypart_id( "foot_r" ) );

            AND_WHEN( "no time has passed" ) {
                CHECK( m.get_level() == 0 );
            }
            AND_WHEN( "1 turn has passed" ) {
                m.decay( 1_turns );
                CHECK( m.get_level() == -2 );
            }
            AND_WHEN( "9 turns have passed" ) {
                m.decay( 9_turns );
                CHECK( m.get_level() == -18 );
            }
            AND_WHEN( "1 minute has passed" ) {
                m.decay( 1_minutes );
                CHECK( m.get_level() == -20 );
            }
            AND_WHEN( "6 minutes have passed" ) {
                m.decay( 6_minutes );
                CHECK( m.get_level() == -20 );
            }
            AND_WHEN( "cooled afterwards" ) {
                m.on_effect_int_change( effect_hot, 0, bodypart_id( "torso" ) );
                m.on_effect_int_change( effect_hot, 0, bodypart_id( "head" ) );
                m.on_effect_int_change( effect_hot, 0, bodypart_id( "eyes" ) );
                m.on_effect_int_change( effect_hot, 0, bodypart_id( "mouth" ) );
                m.on_effect_int_change( effect_hot, 0, bodypart_id( "arm_l" ) );
                m.on_effect_int_change( effect_hot, 0, bodypart_id( "arm_r" ) );
                m.on_effect_int_change( effect_hot, 0, bodypart_id( "leg_l" ) );
                m.on_effect_int_change( effect_hot, 0, bodypart_id( "leg_r" ) );
                m.on_effect_int_change( effect_hot, 0, bodypart_id( "hand_l" ) );
                m.on_effect_int_change( effect_hot, 0, bodypart_id( "hand_r" ) );
                m.on_effect_int_change( effect_hot, 0, bodypart_id( "foot_l" ) );
                m.on_effect_int_change( effect_hot, 0, bodypart_id( "foot_r" ) );

                m.decay( 1_minutes );
                CHECK( m.get_level() == 0 );
            }
        }
    }
}

TEST_CASE( "player_morale_stacking", "[player_morale]" )
{
    player_morale m;

    GIVEN( "stacking of bonuses" ) {
        m.add( morale_food_good, 10, 40, 20_turns, 10_turns );
        m.add( morale_book, 10, 40, 20_turns, 10_turns );

        CHECK( m.has( morale_food_good ) == 10 );
        CHECK( m.has( morale_book ) == 10 );
        CHECK( m.get_level() == 14 );

        WHEN( "a bonus is added" ) {
            m.set_permanent( morale_perm_masochist, 50 );

            CHECK( m.has( morale_food_good ) == 10 );
            CHECK( m.has( morale_book ) == 10 );
            CHECK( m.has( morale_perm_masochist ) == 50 );

            CHECK( m.get_level() == 51 );
        }

        WHEN( "a negative bonus is added" ) {
            m.add( morale_wet, -10, -40, 20_turns, 10_turns );

            CHECK( m.has( morale_food_good ) == 10 );
            CHECK( m.has( morale_book ) == 10 );
            CHECK( m.has( morale_wet ) == -10 );

            CHECK( m.get_level() == 4 );
        }

        WHEN( "a bonus is lost" ) {
            m.remove( morale_book );

            CHECK( m.has( morale_food_good ) == 10 );
            CHECK( m.has( morale_book ) == 0 );

            CHECK( m.get_level() == 10 );
        }
    }
}
