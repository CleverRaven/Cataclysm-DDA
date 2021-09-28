#include "catch/catch.hpp"

#include "creature_tracker.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "npc.h"
#include "npc_attack.h"
#include "npc_class.h"
#include "options_helpers.h"
#include "player_helpers.h"

static const mtype_id mon_zombie( "mon_zombie" );

static constexpr point main_npc_start{ 50, 50 };
static constexpr tripoint main_npc_start_tripoint{ main_npc_start, 0 };

namespace npc_attack_setup
{
static npc &spawn_main_npc()
{
    creature_tracker &creatures = get_creature_tracker();
    get_player_character().setpos( { main_npc_start, -1 } );
    const string_id<npc_template> blank_template( "test_talker" );
    REQUIRE( creatures.creature_at<Creature>( main_npc_start_tripoint ) == nullptr );
    const character_id model_id = get_map().place_npc( main_npc_start, blank_template );

    npc &model_npc = *g->find_npc( model_id );
    clear_character( model_npc );
    model_npc.setpos( main_npc_start_tripoint );

    g->load_npcs();

    REQUIRE( creatures.creature_at<npc>( main_npc_start_tripoint ) != nullptr );

    return model_npc;
}

static npc &respawn_main_npc()
{
    npc *guy = get_creature_tracker().creature_at<npc>( main_npc_start_tripoint );
    if( guy ) {
        guy->die( nullptr );
    }
    return spawn_main_npc();
}

static monster *spawn_zombie_at_range( const int range )
{
    return g->place_critter_at( mon_zombie, main_npc_start_tripoint + tripoint( range, 0, 0 ) );
}
} // namespace npc_attack_setup

TEST_CASE( "NPC faces zombies", "[npc_attack]" )
{
    clear_map_and_put_player_underground();
    clear_vehicles();
    scoped_weather_override sunny_weather( weather_type_id( "sunny" ) );
    npc &main_npc = npc_attack_setup::respawn_main_npc();
    // Allied NPC for behavior testing purposes
    main_npc.set_fac( faction_id( "your_followers" ) );

    GIVEN( "There is a zombie 1 tile away" ) {
        monster *zombie = npc_attack_setup::spawn_zombie_at_range( 1 );

        WHEN( "NPC only has a chef knife" ) {
            item weapon( "knife_chef" );
            main_npc.set_wielded_item( weapon );
            REQUIRE( main_npc.get_wielded_item().typeId() == itype_id( "knife_chef" ) );

            THEN( "NPC attempts to melee the enemy target" ) {
                main_npc.evaluate_best_weapon( zombie );
                const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                npc_attack_melee *melee_attack = dynamic_cast<npc_attack_melee *>( attack.get() );
                CHECK( melee_attack );
                const npc_attack_rating &rating = main_npc.get_current_attack_evaluation();
                CHECK( rating.value() );
                CHECK( *rating.value() > 0 );
                CHECK( rating.target() == zombie->pos() );
            }
        }
        WHEN( "NPC only has an m16a4" ) {
            arm_shooter( main_npc, "m16a4" );

            WHEN( "NPC is allowed to use loud ranged weapons" ) {
                main_npc.rules.set_flag( ally_rule::use_guns );
                REQUIRE( main_npc.rules.has_flag( ally_rule::use_guns ) );
                main_npc.rules.clear_flag( ally_rule::use_silent );
                REQUIRE( !main_npc.rules.has_flag( ally_rule::use_silent ) );

                THEN( "NPC tries to shoot the enemy target" ) {
                    main_npc.evaluate_best_weapon( zombie );
                    const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                    npc_attack_gun *ranged_attack = dynamic_cast<npc_attack_gun *>( attack.get() );
                    CHECK( ranged_attack );
                    const npc_attack_rating &rating = main_npc.get_current_attack_evaluation();
                    CHECK( rating.value() );
                    CHECK( *rating.value() > 0 );
                    CHECK( rating.target() == zombie->pos() );
                }
            }
            WHEN( "NPC isn't allowed to use loud weapons" ) {
                main_npc.rules.set_flag( ally_rule::use_silent );
                REQUIRE( main_npc.rules.has_flag( ally_rule::use_silent ) );

                THEN( "NPC can't fire his weapon" ) {
                    main_npc.evaluate_best_weapon( zombie );
                    const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                    npc_attack_gun *ranged_attack = dynamic_cast<npc_attack_gun *>( attack.get() );
                    CHECK( !ranged_attack );
                }
            }
            WHEN( "NPC isn't allowed to use ranged weapons" ) {
                main_npc.rules.clear_flag( ally_rule::use_guns );
                REQUIRE( !main_npc.rules.has_flag( ally_rule::use_guns ) );

                THEN( "NPC can't fire his weapon" ) {
                    main_npc.evaluate_best_weapon( zombie );
                    const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                    npc_attack_gun *ranged_attack = dynamic_cast<npc_attack_gun *>( attack.get() );
                    CHECK( !ranged_attack );
                }
            }
        }
        WHEN( "NPC only has a bunch of rocks" ) {
            item weapon( "rock" );
            main_npc.set_wielded_item( weapon );
            REQUIRE( main_npc.get_wielded_item().typeId() == itype_id( "rock" ) );

            THEN( "NPC doesn't bother throwing the rocks so close" ) {
                main_npc.evaluate_best_weapon( zombie );
                const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                npc_attack_throw *throw_attack = dynamic_cast<npc_attack_throw *>( attack.get() );
                CHECK( !throw_attack );
            }
        }
    }
    GIVEN( "There is a zombie 5 tiles away" ) {
        monster *zombie = npc_attack_setup::spawn_zombie_at_range( 5 );

        WHEN( "NPC only has a chef knife" ) {
            item weapon( "knife_chef" );
            main_npc.set_wielded_item( weapon );
            REQUIRE( main_npc.get_wielded_item().typeId() == itype_id( "knife_chef" ) );

            THEN( "NPC attempts to melee the enemy target" ) {
                main_npc.evaluate_best_weapon( zombie );
                const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                npc_attack_melee *melee_attack = dynamic_cast<npc_attack_melee *>( attack.get() );
                CHECK( melee_attack );
                const npc_attack_rating &rating = main_npc.get_current_attack_evaluation();
                CHECK( rating.value() );
                CHECK( *rating.value() > 0 );
                CHECK( rating.target() == zombie->pos() );
            }
        }

        WHEN( "NPC only has a bunch of rocks" ) {
            item weapon( "rock" );
            main_npc.set_wielded_item( weapon );
            REQUIRE( main_npc.get_wielded_item().typeId() == itype_id( "rock" ) );

            THEN( "NPC throws rocks at the zombie" ) {
                main_npc.evaluate_best_weapon( zombie );
                const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                npc_attack_throw *throw_attack = dynamic_cast<npc_attack_throw *>( attack.get() );
                CHECK( throw_attack );
            }
        }
    }
    GIVEN( "There is a zombie 1 tile away and another 8 tiles away" ) {
        monster *zombie = npc_attack_setup::spawn_zombie_at_range( 1 );
        monster *zombie_far = npc_attack_setup::spawn_zombie_at_range( 8 );

        WHEN( "NPC only has a chef knife" ) {
            item weapon( "knife_chef" );
            main_npc.set_wielded_item( weapon );
            REQUIRE( main_npc.get_wielded_item().typeId() == itype_id( "knife_chef" ) );

            WHEN( "NPC is targetting closest zombie" ) {
                main_npc.evaluate_best_weapon( zombie );

                THEN( "NPC tries to attack closest zombie" ) {
                    const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                    npc_attack_melee *melee_attack = dynamic_cast<npc_attack_melee *>( attack.get() );
                    CHECK( melee_attack );
                    const npc_attack_rating &rating = main_npc.get_current_attack_evaluation();
                    CHECK( rating.value() );
                    CHECK( *rating.value() > 0 );
                    CHECK( rating.target() == zombie->pos() );
                }
            }
            WHEN( "NPC is targetting farthest zombie" ) {
                WHEN( "Furthest zombie is at full HP" ) {
                    main_npc.evaluate_best_weapon( zombie_far );

                    THEN( "NPC tries to attack closest zombie" ) {
                        const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                        npc_attack_melee *melee_attack = dynamic_cast<npc_attack_melee *>( attack.get() );
                        CHECK( melee_attack );
                        const npc_attack_rating &rating = main_npc.get_current_attack_evaluation();
                        CHECK( rating.value() );
                        CHECK( *rating.value() > 0 );
                        CHECK( rating.target() == zombie->pos() );
                    }
                }
                WHEN( "Furthest zombie is at low HP" ) {
                    zombie_far->set_hp( 1 );
                    main_npc.evaluate_best_weapon( zombie_far );

                    THEN( "NPC tries to attack furthest zombie" ) {
                        const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                        npc_attack_melee *melee_attack = dynamic_cast<npc_attack_melee *>( attack.get() );
                        CHECK( melee_attack );
                        const npc_attack_rating &rating = main_npc.get_current_attack_evaluation();
                        CHECK( rating.value() );
                        CHECK( *rating.value() > 0 );
                        CHECK( rating.target() == zombie_far->pos() );
                    }
                }
            }
        }
    }
}

// TODO: Add scenarios for:
// - NPCs carrying a mix of weapons
// - NPCs trying to shoot through allies
