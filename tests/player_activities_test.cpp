#include "catch/catch.hpp"
#include "map_helpers.h"
#include "monster_helpers.h"
#include "player_helpers.h"
#include "activity_scheduling_helper.h"

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "flag.h"
#include "game.h"
#include "itype.h"
#include "iuse_actor.h"
#include "map.h"
#include "monster.h"
#include "options_helpers.h"
#include "point.h"

static const activity_id ACT_AIM( "ACT_AIM" );
static const activity_id ACT_BOLTCUTTING( "ACT_BOLTCUTTING" );
static const activity_id ACT_CRACKING( "ACT_CRACKING" );
static const activity_id ACT_HACKSAW( "ACT_HACKSAW" );
static const activity_id ACT_NULL( "ACT_NULL" );
static const activity_id ACT_OXYTORCH( "ACT_OXYTORCH" );
static const activity_id ACT_PRYING( "ACT_PRYING" );
static const activity_id ACT_SHEARING( "ACT_SHEARING" );

static const bionic_id bio_ears( "bio_ears" );

static const efftype_id effect_pet( "pet" );
static const efftype_id effect_tied( "tied" );

static const field_type_str_id field_fd_smoke( "fd_smoke" );

static const furn_str_id furn_f_safe_c( "f_safe_c" );
static const furn_str_id furn_f_safe_l( "f_safe_l" );
static const furn_str_id furn_test_f_boltcut1( "test_f_boltcut1" );
static const furn_str_id furn_test_f_boltcut2( "test_f_boltcut2" );
static const furn_str_id furn_test_f_boltcut3( "test_f_boltcut3" );
static const furn_str_id furn_test_f_hacksaw1( "test_f_hacksaw1" );
static const furn_str_id furn_test_f_hacksaw2( "test_f_hacksaw2" );
static const furn_str_id furn_test_f_hacksaw3( "test_f_hacksaw3" );
static const furn_str_id furn_test_f_oxytorch1( "test_f_oxytorch1" );
static const furn_str_id furn_test_f_oxytorch2( "test_f_oxytorch2" );
static const furn_str_id furn_test_f_oxytorch3( "test_f_oxytorch3" );
static const furn_str_id furn_test_f_prying1( "test_f_prying1" );

static const itype_id itype_book_binder( "book_binder" );
static const itype_id itype_glass_shard( "glass_shard" );
static const itype_id itype_oxyacetylene( "oxyacetylene" );
static const itype_id itype_tent_kit( "tent_kit" );
static const itype_id itype_test_2x4( "test_2x4" );
static const itype_id itype_test_backpack( "test_backpack" );
static const itype_id itype_test_battery_disposable( "test_battery_disposable" );
static const itype_id itype_test_boltcutter( "test_boltcutter" );
static const itype_id itype_test_boltcutter_elec( "test_boltcutter_elec" );
static const itype_id itype_test_hacksaw( "test_hacksaw" );
static const itype_id itype_test_hacksaw_elec( "test_hacksaw_elec" );
static const itype_id itype_test_halligan( "test_halligan" );
static const itype_id itype_test_halligan_no_nails( "test_halligan_no_nails" );
static const itype_id itype_test_oxytorch( "test_oxytorch" );
static const itype_id itype_test_pipe( "test_pipe" );
static const itype_id itype_test_rag( "test_rag" );
static const itype_id itype_test_rock( "test_rock" );
static const itype_id itype_test_shears( "test_shears" );
static const itype_id itype_test_shears_off( "test_shears_off" );
static const itype_id itype_test_weldtank( "test_weldtank" );
static const itype_id itype_water_clean( "water_clean" );

static const json_character_flag json_flag_SAFECRACK_NO_TOOL( "SAFECRACK_NO_TOOL" );

static const mtype_id mon_test_non_shearable( "mon_test_non_shearable" );
static const mtype_id mon_test_shearable( "mon_test_shearable" );
static const mtype_id mon_zombie( "mon_zombie" );

static const proficiency_id proficiency_prof_safecracking( "prof_safecracking" );

static const quality_id qual_PRY( "PRY" );
static const quality_id qual_PRYING_NAIL( "PRYING_NAIL" );
static const quality_id qual_SAW_M( "SAW_M" );
static const quality_id qual_SHEAR( "SHEAR" );
static const quality_id qual_WELD( "WELD" );

static const recipe_id recipe_water_clean( "water_clean" );

static const skill_id skill_traps( "traps" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_wall( "t_wall" );
static const ter_str_id ter_test_t_boltcut1( "test_t_boltcut1" );
static const ter_str_id ter_test_t_boltcut2( "test_t_boltcut2" );
static const ter_str_id ter_test_t_hacksaw1( "test_t_hacksaw1" );
static const ter_str_id ter_test_t_hacksaw2( "test_t_hacksaw2" );
static const ter_str_id ter_test_t_oxytorch1( "test_t_oxytorch1" );
static const ter_str_id ter_test_t_oxytorch2( "test_t_oxytorch2" );
static const ter_str_id ter_test_t_prying1( "test_t_prying1" );
static const ter_str_id ter_test_t_prying2( "test_t_prying2" );
static const ter_str_id ter_test_t_prying3( "test_t_prying3" );
static const ter_str_id ter_test_t_prying4( "test_t_prying4" );

TEST_CASE( "safecracking", "[activity][safecracking]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();

    SECTION( "safecracking testing time" ) {

        auto safecracking_setup = [&dummy]( int perception,
        int skill_level, bool has_proficiency ) -> void {
            dummy.per_max = perception;
            dummy.set_skill_level( skill_traps, skill_level );

            REQUIRE( dummy.get_per() == perception );
            REQUIRE( static_cast<int>( dummy.get_skill_level( skill_traps ) ) == skill_level );
            if( has_proficiency )
            {
                dummy.add_proficiency( proficiency_prof_safecracking );
                REQUIRE( dummy.has_proficiency( proficiency_prof_safecracking ) );
            } else
            {
                dummy.lose_proficiency( proficiency_prof_safecracking );
                REQUIRE( !dummy.has_proficiency( proficiency_prof_safecracking ) );
            }
        };

        GIVEN( "player with default perception, 0 devices skill and no safecracking proficiency" ) {
            safecracking_setup( 8, 0, false );
            THEN( "cracking time is 10 hours and 30 minutes" ) {
                const time_duration time = safecracking_activity_actor::safecracking_time( dummy );
                CHECK( time == 630_minutes );
            }
        }

        GIVEN( "player with 10 perception, 4 devices skill and no safecracking proficiency" ) {
            safecracking_setup( 10, 4, false );
            THEN( "cracking time is 5 hours and 30 minutes" ) {
                const time_duration time = safecracking_activity_actor::safecracking_time( dummy );
                CHECK( time == 330_minutes );
            }
        }

        GIVEN( "player with 10 perception, 4 devices skill and safecracking proficiency" ) {
            safecracking_setup( 10, 4, true );
            THEN( "cracking time is 1 hour and 50 minutes" ) {
                const time_duration time = safecracking_activity_actor::safecracking_time( dummy );
                CHECK( time == 110_minutes );
            }
        }

        GIVEN( "player with 8 perception, 10 devices skill and safecracking proficiency" ) {
            safecracking_setup( 8, 10, true );
            THEN( "cracking time is 30 minutes" ) {
                const time_duration time = safecracking_activity_actor::safecracking_time( dummy );
                CHECK( time == 30_minutes );
            }
        }

        GIVEN( "player with 8 perception, 10 devices skill and no safecracking proficiency" ) {
            safecracking_setup( 8, 10, false );
            THEN( "cracking time is 90 minutes" ) {
                const time_duration time = safecracking_activity_actor::safecracking_time( dummy );
                CHECK( time == 90_minutes );
            }
        }

        GIVEN( "player with 1 perception, 0 devices skill and no safecracking proficiency" ) {
            safecracking_setup( 1, 0, false );
            THEN( "cracking time is 14 hours" ) {
                const time_duration time = safecracking_activity_actor::safecracking_time( dummy );
                CHECK( time == 14_hours );
            }
        }
    }

    SECTION( "safecracking tools test" ) {
        clear_avatar();
        clear_map();

        tripoint safe;
        dummy.setpos( safe + tripoint_east );

        map &mp = get_map();
        dummy.activity = player_activity( safecracking_activity_actor( safe ) );
        dummy.activity.start_or_resume( dummy, false );

        GIVEN( "player without the required tools" ) {
            mp.furn_set( safe, furn_f_safe_l );
            REQUIRE( !dummy.cache_has_item_with( flag_SAFECRACK ) );
            REQUIRE( !dummy.has_flag( json_flag_SAFECRACK_NO_TOOL ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( mp.furn( safe ) == furn_f_safe_l );

            WHEN( "player tries safecracking" ) {
                process_activity( dummy );
                THEN( "activity is canceled" ) {
                    CHECK( mp.furn( safe ) == furn_f_safe_l );
                }
            }
        }

        GIVEN( "player has a stethoscope" ) {
            dummy.i_add( item( "stethoscope" ) );
            mp.furn_set( safe, furn_f_safe_l );
            REQUIRE( dummy.cache_has_item_with( flag_SAFECRACK ) );
            REQUIRE( !dummy.has_flag( json_flag_SAFECRACK_NO_TOOL ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( mp.furn( safe ) == furn_f_safe_l );

            WHEN( "player completes the safecracking activity" ) {
                process_activity( dummy );
                THEN( "safe is unlocked" ) {
                    CHECK( mp.furn( safe ) == furn_f_safe_c );
                }
            }
        }

        GIVEN( "player has a stethoscope" ) {
            dummy.clear_worn();
            dummy.remove_weapon();
            dummy.add_bionic( bio_ears );
            mp.furn_set( safe, furn_f_safe_l );
            REQUIRE( !dummy.cache_has_item_with( flag_SAFECRACK ) );
            REQUIRE( dummy.has_flag( json_flag_SAFECRACK_NO_TOOL ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( mp.furn( safe ) == furn_f_safe_l );

            WHEN( "player completes the safecracking activity" ) {
                process_activity( dummy );
                THEN( "safe is unlocked" ) {
                    CHECK( mp.furn( safe ) == furn_f_safe_c );
                }
            }
        }

        GIVEN( "player has a stethoscope" ) {
            dummy.clear_bionics();
            dummy.i_add( item( "stethoscope" ) );
            mp.furn_set( safe, furn_f_safe_l );
            REQUIRE( dummy.cache_has_item_with( flag_SAFECRACK ) );
            REQUIRE( !dummy.has_flag( json_flag_SAFECRACK_NO_TOOL ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( mp.furn( safe ) == furn_f_safe_l );

            WHEN( "player is safecracking" ) {
                dummy.mod_moves( dummy.get_speed() );
                for( int i = 0; i < 100; ++i ) {
                    dummy.activity.do_turn( dummy );
                }

                THEN( "player loses their stethoscope" ) {
                    dummy.clear_worn();
                    dummy.remove_weapon();
                    REQUIRE( !dummy.cache_has_item_with( flag_SAFECRACK ) );

                    process_activity( dummy );
                    THEN( "activity is canceled" ) {
                        CHECK( mp.furn( safe ) == furn_f_safe_l );
                    }
                }
            }
        }
    }

    SECTION( "safecracking proficiency test" ) {

        auto get_safecracking_time = [&dummy]() -> time_duration {
            const std::vector<display_proficiency> profs = dummy.display_proficiencies();
            const auto iter = std::find_if( profs.begin(), profs.end(),
            []( const display_proficiency & p ) -> bool {
                return p.id == proficiency_prof_safecracking;
            } );
            if( iter == profs.end() )
            {
                return time_duration();
            }
            return iter->spent;
        };

        clear_avatar();
        clear_map();

        tripoint safe;
        dummy.setpos( safe + tripoint_east );

        map &mp = get_map();
        dummy.activity = player_activity( safecracking_activity_actor( safe ) );
        dummy.activity.start_or_resume( dummy, false );

        GIVEN( "player cracks one safe" ) {
            dummy.i_add( item( "stethoscope" ) );
            mp.furn_set( safe, furn_f_safe_l );
            REQUIRE( dummy.cache_has_item_with( flag_SAFECRACK ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( mp.furn( safe ) == furn_f_safe_l );

            REQUIRE( !dummy.has_proficiency( proficiency_prof_safecracking ) );

            dummy.set_focus( 100 );
            REQUIRE( dummy.get_focus() == 100 );

            const time_duration time_before = get_safecracking_time();

            process_activity( dummy );
            REQUIRE( mp.furn( safe ) == furn_f_safe_c );
            THEN( "proficiency given is less than 90 minutes" ) {
                const time_duration time_after = get_safecracking_time();
                REQUIRE( time_after > 0_seconds );

                const time_duration time_delta = time_after - time_before;
                CHECK( time_delta > 5_minutes );
                CHECK( time_delta < 90_minutes );
            }
        }
    }
}

TEST_CASE( "shearing", "[activity][shearing][animals]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();
    clear_map();

    auto test_monster = [&dummy]( bool mon_shearable ) -> monster & {
        monster *mon;
        if( mon_shearable )
        {
            mon = &spawn_test_monster( mon_test_shearable.str(), dummy.pos() + tripoint_north );
        } else
        {
            mon = &spawn_test_monster( mon_test_non_shearable.str(), dummy.pos() + tripoint_north );
        }

        mon->friendly = -1;
        mon->add_effect( effect_pet, 1_turns, true );
        mon->add_effect( effect_tied, 1_turns, true );
        return *mon;
    };

    SECTION( "shearing testing time" ) {

        GIVEN( "player without shearing quality" ) {
            clear_avatar();
            clear_map();
            monster &mon = test_monster( true );

            REQUIRE( dummy.max_quality( qual_SHEAR ) <= 0 );
            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), false ) );
            dummy.activity.start_or_resume( dummy, false );

            THEN( "shearing can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tool with shearing quality one" ) {
            clear_avatar();
            clear_map();
            monster &mon = test_monster( true );

            dummy.i_add( item( itype_test_shears ) );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 1 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), false ) );
            dummy.activity.start_or_resume( dummy, false );
            REQUIRE( dummy.activity.id() == ACT_SHEARING );

            THEN( "shearing time is 30 minutes" ) {
                CHECK( dummy.activity.moves_total == to_moves<int>( 30_minutes ) );
            }
        }

        GIVEN( "an electric tool with shearing quality three" ) {
            clear_avatar();
            clear_map();
            monster &mon = test_monster( true );

            item battery( itype_test_battery_disposable );
            battery.ammo_set( battery.ammo_default(), 300 );

            item elec_shears( itype_test_shears_off );
            elec_shears.put_in( battery, pocket_type::MAGAZINE_WELL );

            const use_function *use = elec_shears.type->get_use( "transform" );
            REQUIRE( use != nullptr );
            const iuse_transform *actor = dynamic_cast<const iuse_transform *>( use->get_actor_ptr() );
            actor->use( &dummy, elec_shears, dummy.pos() );

            dummy.i_add( elec_shears );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 3 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), false ) );
            dummy.activity.start_or_resume( dummy, false );
            REQUIRE( dummy.activity.id() == ACT_SHEARING );

            THEN( "shearing time is 10 minutes" ) {
                CHECK( dummy.activity.moves_total == to_moves<int>( 10_minutes ) );
            }
        }

        GIVEN( "a non shearable animal" ) {
            clear_avatar();
            clear_map();
            monster &mon = test_monster( false );

            dummy.i_add( item( itype_test_shears ) );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 1 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), false ) );
            dummy.activity.start_or_resume( dummy, false );

            THEN( "shearing can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }
    }

    SECTION( "shearing losing tool" ) {
        GIVEN( "an electric tool with shearing quality three" ) {
            clear_avatar();
            clear_map();
            monster &mon = test_monster( true );

            item battery( itype_test_battery_disposable );
            battery.ammo_set( battery.ammo_default(), 5 );

            item elec_shears( itype_test_shears_off );
            elec_shears.put_in( battery, pocket_type::MAGAZINE_WELL );

            const use_function *use = elec_shears.type->get_use( "transform" );
            REQUIRE( use != nullptr );
            const iuse_transform *actor = dynamic_cast<const iuse_transform *>( use->get_actor_ptr() );
            actor->use( &dummy, elec_shears, dummy.pos() );

            dummy.i_add( elec_shears );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 3 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), false ) );
            dummy.activity.start_or_resume( dummy, false );
            REQUIRE( dummy.activity.id() == ACT_SHEARING );

            dummy.mod_moves( dummy.get_speed() );
            for( int i = 0; i < 100; ++i ) {
                dummy.activity.do_turn( dummy );
            }

            WHEN( "tool runs out of charges mid activity" ) {
                for( int i = 0; i < 10000; ++i ) {
                    dummy.process_items();
                }

                CHECK( dummy.get_wielded_item()->ammo_remaining() == 0 );
                REQUIRE( dummy.get_wielded_item()->typeId().str() == itype_test_shears_off.str() );

                CHECK( dummy.max_quality( qual_SHEAR ) <= 0 );

                THEN( "activity stops without completing" ) {
                    CHECK( dummy.activity.id() == ACT_SHEARING );
                    CHECK( dummy.activity.moves_left > 0 );
                    dummy.activity.do_turn( dummy );
                    CHECK( dummy.activity.id() == ACT_NULL );
                }
            }
        }
    }

    SECTION( "shearing testing result" ) {

        GIVEN( "a shearable monster" ) {

            clear_avatar();
            clear_map();
            monster &mon = test_monster( true );

            dummy.i_add( item( itype_test_shears ) );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 1 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), false ) );
            dummy.activity.start_or_resume( dummy, false );
            REQUIRE( dummy.activity.id() == ACT_SHEARING );

            const itype_id test_amount = itype_test_rock;
            const itype_id test_random = itype_test_2x4;
            const itype_id test_mass = itype_test_rag;
            const itype_id test_volume = itype_test_pipe;

            process_activity( dummy );
            WHEN( "shearing finishes" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
                THEN( "player receives the items" ) {
                    const map_stack items = get_map().i_at( dummy.pos() );
                    int count_amount = 0;
                    int count_random = 0;
                    int count_mass   = 0;
                    int count_volume = 0;
                    for( const item &it : items ) {
                        // can't use switch here
                        const itype_id it_id = it.typeId();
                        if( it_id == test_amount ) {
                            count_amount += it.charges;
                        } else if( it_id == test_random ) {
                            count_random += 1;
                        } else if( it_id == test_mass ) {
                            count_mass   += 1;
                        } else if( it_id == test_volume ) {
                            count_volume += 1;
                        }
                    }

                    CHECK( count_amount == 30 );
                    CHECK( ( 5 <= count_random && count_random <= 10 ) );

                    float weight = units::to_kilogram( mon.get_weight() );
                    CHECK( count_mass == static_cast<int>( 0.5f * weight ) );

                    float volume = units::to_liter( mon.get_volume() );
                    CHECK( count_volume == static_cast<int>( 0.3f * volume ) );
                }
            }
        }

        GIVEN( "an previously tied shearable monster" ) {
            clear_avatar();
            clear_map();
            monster &mon = test_monster( true );

            dummy.i_add( item( itype_test_shears ) );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 1 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), false ) );
            dummy.activity.start_or_resume( dummy, false );
            REQUIRE( dummy.activity.id() == ACT_SHEARING );

            process_activity( dummy );
            WHEN( "player is done shearing" ) {
                REQUIRE( dummy.activity.id() == ACT_NULL );
                THEN( "monster is tied" ) {
                    CHECK( mon.has_effect( effect_tied ) );
                }
            }
        }

        GIVEN( "a previously untied shearable monster" ) {
            clear_avatar();
            clear_map();
            monster &mon = test_monster( true );

            dummy.i_add( item( itype_test_shears ) );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 1 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), true ) );
            dummy.activity.start_or_resume( dummy, false );
            REQUIRE( dummy.activity.id() == ACT_SHEARING );

            process_activity( dummy );
            WHEN( "player is done shearing" ) {
                REQUIRE( dummy.activity.id() == ACT_NULL );
                THEN( "monster is untied" ) {
                    CHECK_FALSE( mon.has_effect( effect_tied ) );
                }
            }
        }
    }
}

TEST_CASE( "boltcut", "[activity][boltcut]" )
{
    map &mp = get_map();
    avatar &dummy = get_avatar();

    auto setup_dummy = [&dummy]() -> item_location {
        item it_boltcut( itype_test_boltcutter );

        dummy.wield( it_boltcut );
        REQUIRE( dummy.get_wielded_item()->typeId() == itype_test_boltcutter );

        return dummy.get_wielded_item();
    };

    auto setup_activity = [&dummy]( const item_location & torch ) -> void {
        boltcutting_activity_actor act{tripoint_zero, torch};
        act.testing = true;
        dummy.assign_activity( act );
    };

    SECTION( "boltcut start checks" ) {
        GIVEN( "a tripoint with nothing" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_str_id::NULL_ID() );
            REQUIRE( mp.ter( tripoint_zero ) == ter_str_id::NULL_ID() );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            THEN( "boltcutting activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with invalid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_t_dirt );
            REQUIRE( mp.ter( tripoint_zero ) == ter_t_dirt );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            THEN( "boltcutting activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_boltcut1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            THEN( "boltcutting activity can start" ) {
                CHECK( dummy.activity.id() == ACT_BOLTCUTTING );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_boltcut1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            THEN( "boltcutting activity can start" ) {
                CHECK( dummy.activity.id() == ACT_BOLTCUTTING );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_boltcut1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );
            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );

            WHEN( "terrain has a duration of 10 seconds" ) {
                REQUIRE( ter_test_t_boltcut1->boltcut->duration() == 10_seconds );
                THEN( "moves_left is equal to 10 seconds" ) {
                    CHECK( dummy.activity.moves_left == to_moves<int>( 10_seconds ) );
                }
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_boltcut1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );
            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );

            WHEN( "furniture has a duration of 5 seconds" ) {
                REQUIRE( furn_test_f_boltcut1->boltcut->duration() == 5_seconds );
                THEN( "moves_left is equal to 5 seconds" ) {
                    CHECK( dummy.activity.moves_left == to_moves<int>( 5_seconds ) );
                }
            }
        }
    }

    SECTION( "boltcut turn checks" ) {
        GIVEN( "player is in mid activity" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_boltcut3 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_boltcut3 );

            item battery( itype_test_battery_disposable );
            battery.ammo_set( battery.ammo_default(), 2 );

            item it_boltcut_elec( itype_test_boltcutter_elec );
            it_boltcut_elec.put_in( battery, pocket_type::MAGAZINE_WELL );

            dummy.wield( it_boltcut_elec );
            REQUIRE( dummy.get_wielded_item()->typeId() == itype_test_boltcutter_elec );

            item_location boltcutter_elec = dummy.get_wielded_item();

            setup_activity( boltcutter_elec );
            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );

            WHEN( "player runs out of charges" ) {
                REQUIRE( dummy.activity.id() == ACT_NULL );

                THEN( "player recharges with fuel" ) {
                    boltcutter_elec->ammo_set( battery.ammo_default() );

                    AND_THEN( "player can resume the activity" ) {
                        setup_activity( boltcutter_elec );
                        dummy.mod_moves( dummy.get_speed() );
                        dummy.activity.do_turn( dummy );
                        CHECK( dummy.activity.id() == ACT_BOLTCUTTING );
                        CHECK( dummy.activity.moves_left < to_moves<int>( furn_test_f_boltcut3->boltcut->duration() ) );
                    }
                }
            }
        }
    }

    SECTION( "boltcut finish checks" ) {
        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_boltcut1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "terrain gets converted to new terrain type" ) {
                CHECK( mp.ter( tripoint_zero ) == ter_t_dirt );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_boltcut1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_zero ) == furn_str_id::NULL_ID() );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_boltcut2 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_boltcut2 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_zero ) == furn_test_f_boltcut1 );
            }
        }

        GIVEN( "a tripoint with a valid furniture with byproducts" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_boltcut2 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_boltcut2 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            REQUIRE( ter_test_t_boltcut2->boltcut->byproducts().size() == 2 );

            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            const itype_id test_amount = itype_test_rock;
            const itype_id test_random = itype_test_2x4;

            WHEN( "boltcut acitivy finishes" ) {
                CHECK( dummy.activity.id() == ACT_NULL );

                THEN( "player receives the items" ) {
                    const map_stack items = get_map().i_at( tripoint_zero );
                    int count_amount = 0;
                    int count_random = 0;
                    for( const item &it : items ) {
                        // can't use switch here
                        const itype_id it_id = it.typeId();
                        if( it_id == test_amount ) {
                            count_amount += it.charges;
                        } else if( it_id == test_random ) {
                            count_random += 1;
                        }
                    }

                    CHECK( count_amount == 3 );
                    CHECK( ( 7 <= count_random && count_random <= 9 ) );
                }
            }
        }
    }
}

TEST_CASE( "hacksaw", "[activity][hacksaw]" )
{
    map &mp = get_map();
    avatar &dummy = get_avatar();

    auto setup_dummy = [&dummy]() -> item_location {
        item it_hacksaw( itype_test_hacksaw );

        dummy.wield( it_hacksaw );
        REQUIRE( dummy.get_wielded_item()->typeId() == itype_test_hacksaw );
        REQUIRE( dummy.max_quality( qual_SAW_M ) == 2 );

        return dummy.get_wielded_item();
    };

    auto setup_activity = [&dummy]( const item_location & torch ) -> void {
        hacksaw_activity_actor act{tripoint_zero, torch};
        act.testing = true;
        dummy.assign_activity( act );
    };

    SECTION( "hacksaw start checks" ) {
        GIVEN( "a tripoint with nothing" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_str_id::NULL_ID() );
            REQUIRE( mp.ter( tripoint_zero ) == ter_str_id::NULL_ID() );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );

            THEN( "hacksaw activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with invalid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_t_dirt );
            REQUIRE( mp.ter( tripoint_zero ) == ter_t_dirt );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );

            THEN( "hacksaw activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_hacksaw1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_hacksaw1 );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );

            THEN( "hacksaw activity can start" ) {
                CHECK( dummy.activity.id() == ACT_HACKSAW );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_hacksaw1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_hacksaw1 );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );

            THEN( "hacksaw activity can start" ) {
                CHECK( dummy.activity.id() == ACT_HACKSAW );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_hacksaw1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_hacksaw1 );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );
            REQUIRE( dummy.activity.id() == ACT_HACKSAW );

            WHEN( "terrain has a duration of 10 minutes" ) {
                REQUIRE( ter_test_t_hacksaw1->hacksaw->duration() == 10_minutes );
                THEN( "moves_left is equal to 10 minutes" ) {
                    CHECK( dummy.activity.moves_left == to_moves<int>( 10_minutes ) );
                }
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_hacksaw1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_hacksaw1 );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );
            REQUIRE( dummy.activity.id() == ACT_HACKSAW );

            WHEN( "furniture has a duration of 5 minutes" ) {
                REQUIRE( furn_test_f_hacksaw1->hacksaw->duration() == 5_minutes );
                THEN( "moves_left is equal to 5 minutes" ) {
                    CHECK( dummy.activity.moves_left == to_moves<int>( 5_minutes ) );
                }
            }
        }
    }

    SECTION( "hacksaw turn checks" ) {
        GIVEN( "player is in mid activity" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_hacksaw3 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_hacksaw3 );

            item battery( itype_test_battery_disposable );
            battery.ammo_set( battery.ammo_default() );

            item it_hacksaw_elec( itype_test_hacksaw_elec );
            it_hacksaw_elec.put_in( battery, pocket_type::MAGAZINE_WELL );

            dummy.wield( it_hacksaw_elec );
            REQUIRE( dummy.get_wielded_item()->typeId() == itype_test_hacksaw_elec );
            REQUIRE( dummy.max_quality( qual_SAW_M ) == 2 );

            item_location hacksaw_elec = dummy.get_wielded_item();

            setup_activity( hacksaw_elec );
            REQUIRE( dummy.activity.id() == ACT_HACKSAW );
            process_activity( dummy );

            WHEN( "player runs out of charges" ) {
                REQUIRE( dummy.activity.id() == ACT_NULL );

                THEN( "player recharges with fuel" ) {
                    hacksaw_elec->ammo_set( battery.ammo_default() );

                    AND_THEN( "player can resume the activity" ) {
                        setup_activity( hacksaw_elec );
                        dummy.mod_moves( dummy.get_speed() );
                        dummy.activity.do_turn( dummy );
                        CHECK( dummy.activity.id() == ACT_HACKSAW );
                        CHECK( dummy.activity.moves_left < to_moves<int>( furn_test_f_hacksaw3->hacksaw->duration() ) );
                    }
                }
            }
        }
    }

    SECTION( "hacksaw finish checks" ) {
        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_hacksaw1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_hacksaw1 );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );

            REQUIRE( dummy.activity.id() == ACT_HACKSAW );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "terrain gets converted to new terrain type" ) {
                CHECK( mp.ter( tripoint_zero ) == ter_t_dirt );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_hacksaw1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_hacksaw1 );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );

            REQUIRE( dummy.activity.id() == ACT_HACKSAW );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_zero ) == furn_str_id::NULL_ID() );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_hacksaw2 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_hacksaw2 );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );

            REQUIRE( dummy.activity.id() == ACT_HACKSAW );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_zero ) == furn_test_f_hacksaw1 );
            }
        }

        GIVEN( "a tripoint with a valid furniture with byproducts" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_hacksaw2 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_hacksaw2 );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );

            REQUIRE( ter_test_t_hacksaw2->hacksaw->byproducts().size() == 2 );

            REQUIRE( dummy.activity.id() == ACT_HACKSAW );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            const itype_id test_amount = itype_test_rock;
            const itype_id test_random = itype_test_2x4;

            WHEN( "hacksaw acitivy finishes" ) {
                CHECK( dummy.activity.id() == ACT_NULL );

                THEN( "player receives the items" ) {
                    const map_stack items = get_map().i_at( tripoint_zero );
                    int count_amount = 0;
                    int count_random = 0;
                    for( const item &it : items ) {
                        // can't use switch here
                        const itype_id it_id = it.typeId();
                        if( it_id == test_amount ) {
                            count_amount += it.charges;
                        } else if( it_id == test_random ) {
                            count_random += 1;
                        }
                    }

                    CHECK( count_amount == 3 );
                    CHECK( ( 7 <= count_random && count_random <= 9 ) );
                }
            }
        }
    }
}

TEST_CASE( "oxytorch", "[activity][oxytorch]" )
{
    map &mp = get_map();
    avatar &dummy = get_avatar();

    auto setup_dummy = [&dummy]() -> item_location {
        item it_welding_torch( itype_test_oxytorch );
        it_welding_torch.ammo_set( itype_oxyacetylene );

        dummy.wield( it_welding_torch );
        REQUIRE( dummy.get_wielded_item()->typeId() == itype_test_oxytorch );
        REQUIRE( dummy.max_quality( qual_WELD ) == 10 );

        return dummy.get_wielded_item();
    };

    auto setup_activity = [&dummy]( const item_location & torch ) -> void {
        oxytorch_activity_actor act{tripoint_zero, torch};
        act.testing = true;
        dummy.assign_activity( act );
    };

    SECTION( "oxytorch start checks" ) {
        GIVEN( "a tripoint with nothing" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_str_id::NULL_ID() );
            REQUIRE( mp.ter( tripoint_zero ) == ter_str_id::NULL_ID() );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            THEN( "oxytorch activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with invalid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_t_dirt );
            REQUIRE( mp.ter( tripoint_zero ) == ter_t_dirt );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            THEN( "oxytorch activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_oxytorch1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_oxytorch1 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            THEN( "oxytorch activity can start" ) {
                CHECK( dummy.activity.id() == ACT_OXYTORCH );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_oxytorch1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_oxytorch1 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            THEN( "oxytorch activity can start" ) {
                CHECK( dummy.activity.id() == ACT_OXYTORCH );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_oxytorch1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_oxytorch1 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );
            REQUIRE( dummy.activity.id() == ACT_OXYTORCH );

            WHEN( "terrain has a duration of 10 seconds" ) {
                REQUIRE( ter_test_t_oxytorch1->oxytorch->duration() == 10_seconds );
                THEN( "moves_left is equal to 10 seconds" ) {
                    CHECK( dummy.activity.moves_left == to_moves<int>( 10_seconds ) );
                }
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_oxytorch1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_oxytorch1 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );
            REQUIRE( dummy.activity.id() == ACT_OXYTORCH );

            WHEN( "furniture has a duration of 5 seconds" ) {
                REQUIRE( furn_test_f_oxytorch1->oxytorch->duration() == 5_seconds );
                THEN( "moves_left is equal to 5 seconds" ) {
                    CHECK( dummy.activity.moves_left == to_moves<int>( 5_seconds ) );
                }
            }
        }
    }

    SECTION( "oxytorch turn checks" ) {
        GIVEN( "player is in mid activity" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_oxytorch3 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_oxytorch3 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );
            REQUIRE( dummy.activity.id() == ACT_OXYTORCH );
            process_activity( dummy );

            WHEN( "player runs out of fuel" ) {
                REQUIRE( dummy.activity.id() == ACT_NULL );

                THEN( "player recharges with fuel" ) {
                    welding_torch->ammo_set( itype_oxyacetylene );

                    AND_THEN( "player can resume the activity" ) {
                        setup_activity( welding_torch );
                        dummy.mod_moves( dummy.get_speed() );
                        dummy.activity.do_turn( dummy );
                        CHECK( dummy.activity.id() == ACT_OXYTORCH );
                        CHECK( dummy.activity.moves_left < to_moves<int>( furn_test_f_oxytorch3->oxytorch->duration() ) );
                    }
                }
            }
        }
    }

    SECTION( "oxytorch finish checks" ) {
        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_oxytorch1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_oxytorch1 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            REQUIRE( dummy.activity.id() == ACT_OXYTORCH );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "terrain gets converted to new terrain type" ) {
                CHECK( mp.ter( tripoint_zero ) == ter_t_dirt );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_oxytorch1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_oxytorch1 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            REQUIRE( dummy.activity.id() == ACT_OXYTORCH );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_zero ) == furn_str_id::NULL_ID() );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_oxytorch2 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_oxytorch2 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            REQUIRE( dummy.activity.id() == ACT_OXYTORCH );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_zero ) == furn_test_f_oxytorch1 );
            }
        }

        GIVEN( "a tripoint with a valid furniture with byproducts" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_oxytorch2 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_oxytorch2 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            REQUIRE( ter_test_t_oxytorch2->oxytorch->byproducts().size() == 2 );

            REQUIRE( dummy.activity.id() == ACT_OXYTORCH );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            const itype_id test_amount = itype_test_rock;
            const itype_id test_random = itype_test_2x4;

            WHEN( "oxytorch acitivy finishes" ) {
                CHECK( dummy.activity.id() == ACT_NULL );

                THEN( "player receives the items" ) {
                    const map_stack items = get_map().i_at( tripoint_zero );
                    int count_amount = 0;
                    int count_random = 0;
                    for( const item &it : items ) {
                        // can't use switch here
                        const itype_id it_id = it.typeId();
                        if( it_id == test_amount ) {
                            count_amount += it.charges;
                        } else if( it_id == test_random ) {
                            count_random += 1;
                        }
                    }

                    CHECK( count_amount == 3 );
                    CHECK( ( 7 <= count_random && count_random <= 9 ) );
                }
            }
        }
    }
}

TEST_CASE( "prying", "[activity][prying]" )
{
    map &mp = get_map();
    avatar &dummy = get_avatar();

    auto setup_dummy = [&dummy]( bool need_nails ) -> item_location {
        itype_id prying_tool_id( need_nails ? itype_test_halligan : itype_test_halligan_no_nails );
        item it_prying_tool( prying_tool_id );

        dummy.wield( it_prying_tool );
        REQUIRE( dummy.has_quality( qual_PRY ) );
        if( need_nails )
        {
            REQUIRE( dummy.get_wielded_item()->typeId() == itype_test_halligan );
            REQUIRE( dummy.has_quality( qual_PRYING_NAIL ) );
        } else
        {
            REQUIRE( dummy.get_wielded_item()->typeId() == itype_test_halligan_no_nails );
            REQUIRE( dummy.max_quality( qual_PRY ) == 999999 );
        }

        return dummy.get_wielded_item();
    };

    auto setup_activity = [&dummy]( const item_location & tool,
    const tripoint &target = tripoint_zero ) -> void {
        prying_activity_actor act{target, tool};
        act.testing = true;
        dummy.assign_activity( act );
    };

    SECTION( "prying time tests" ) {
        GIVEN( "a furniture with prying_nails and duration set to 17 seconds " ) {
            clear_map();
            clear_avatar();

            item_location prying_tool = setup_dummy( true );

            const time_duration prying_time =
                prying_activity_actor::prying_time(
                    *furn_test_f_prying1->prying, prying_tool, dummy );

            THEN( "prying_nails time is 17 seconds" ) {
                CHECK( prying_time == 17_seconds );
            }
        }

        GIVEN( "a terrain without prying_nails" ) {
            clear_map();
            clear_avatar();

            item_location prying_tool = setup_dummy( true );

            REQUIRE( dummy.get_str() == 8 );

            const time_duration prying_time = prying_activity_actor::prying_time(
                                                  *ter_test_t_prying3->prying, prying_tool, dummy );

            THEN( "prying time is 32 seconds" ) {
                CHECK( prying_time == 32_seconds );
            }
        }
    }

    SECTION( "prying start checks" ) {
        GIVEN( "a tripoint with nothing" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_str_id::NULL_ID() );
            REQUIRE( mp.ter( tripoint_zero ) == ter_str_id::NULL_ID() );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );

            THEN( "prying activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with invalid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_t_dirt );
            REQUIRE( mp.ter( tripoint_zero ) == ter_t_dirt );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );

            THEN( "prying activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_prying1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_prying1 );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );

            THEN( "prying activity can start" ) {
                CHECK( dummy.activity.id() == ACT_PRYING );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_prying1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_prying1 );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );

            THEN( "oxytorch activity can start" ) {
                CHECK( dummy.activity.id() == ACT_PRYING );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_prying1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_prying1 );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );
            REQUIRE( dummy.activity.id() == ACT_PRYING );

            WHEN( "terrain has a duration of 30 seconds" ) {
                REQUIRE( ter_test_t_prying1->prying->duration() == 30_seconds );
                THEN( "moves_left is equal to 30 seconds" ) {
                    CHECK( dummy.activity.moves_left == to_moves<int>( 30_seconds ) );
                }
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_prying1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_prying1 );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );
            REQUIRE( dummy.activity.id() == ACT_PRYING );

            WHEN( "furniture has a duration of 17 seconds" ) {
                REQUIRE( furn_test_f_prying1->prying->duration() == 17_seconds );
                THEN( "moves_left is equal to 17 seconds" ) {
                    CHECK( dummy.activity.moves_left == to_moves<int>( 17_seconds ) );
                }
            }
        }
    }

    SECTION( "prying finish checks with prying_nails" ) {
        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_prying1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_prying1 );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );

            REQUIRE( dummy.activity.id() == ACT_PRYING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "terrain gets converted to new terrain type" ) {
                CHECK( mp.ter( tripoint_zero ) == ter_t_dirt );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_test_f_prying1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_test_f_prying1 );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );

            REQUIRE( dummy.activity.id() == ACT_PRYING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_zero ) == furn_str_id::NULL_ID() );
            }
        }
    }

    SECTION( "prying finish checks without prying_nails" ) {
        GIVEN( "a tripoint with valid impossible to pry open terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_prying2 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_prying2 );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );

            REQUIRE( dummy.activity.id() == ACT_PRYING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "activity fails" ) {
                CHECK( mp.ter( tripoint_zero ) == ter_test_t_prying2 );
            }
        }

        GIVEN( "a tripoint with valid terrain with a tool that always opens it" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_prying2 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_prying2 );

            item_location prying_tool = setup_dummy( false );
            setup_activity( prying_tool );

            REQUIRE( dummy.activity.id() == ACT_PRYING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "terrain gets converted to new type" ) {
                CHECK( mp.ter( tripoint_zero ) == ter_t_dirt );
            }
        }

        GIVEN( "a tripoint with valid terrain that will break" ) {
            clear_map();
            clear_avatar();

            const tripoint terrain_pos = dummy.pos() + tripoint_north;

            mp.ter_set( terrain_pos, ter_test_t_prying4 );
            REQUIRE( mp.ter( terrain_pos ) == ter_test_t_prying4 );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool, terrain_pos );

            REQUIRE( dummy.activity.id() == ACT_PRYING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            const itype_id test_shards = itype_glass_shard;

            WHEN( "activity fails" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
                CHECK( mp.ter( terrain_pos ) == ter_t_dirt );
                const map_stack items = get_map().i_at( terrain_pos );
                int count_shards = 0;
                for( const item &it : items ) {
                    if( it.typeId() == test_shards ) {
                        count_shards += 1;
                    }
                }

                THEN( "number of shards is between 21 and 29" ) {
                    CHECK( 21 <= count_shards );
                    CHECK( count_shards <= 29 );
                }
            }
        }

        GIVEN( "a tripoint with a valid terrain with byproducts" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, ter_test_t_prying1 );
            REQUIRE( mp.ter( tripoint_zero ) == ter_test_t_prying1 );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );

            REQUIRE( ter_test_t_prying1->prying->byproducts().size() == 2 );

            REQUIRE( dummy.activity.id() == ACT_PRYING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            const itype_id test_amount = itype_test_rock;
            const itype_id test_random = itype_test_2x4;

            WHEN( "prying acitivy finishes" ) {
                CHECK( dummy.activity.id() == ACT_NULL );

                THEN( "player receives the items" ) {
                    const map_stack items = get_map().i_at( tripoint_zero );
                    int count_amount = 0;
                    int count_random = 0;
                    for( const item &it : items ) {
                        // can't use switch here
                        const itype_id it_id = it.typeId();
                        if( it_id == test_amount ) {
                            count_amount += it.charges;
                        } else if( it_id == test_random ) {
                            count_random += 1;
                        }
                    }

                    CHECK( count_amount == 3 );
                    CHECK( ( 7 <= count_random && count_random <= 9 ) );
                }
            }
        }
    }
}

/**
* Helper method to create activity stubs that aren't meant to be processed.
* The activities here still need to be able to pass activity_actor::start and
* set activity::moves_left to a value greater than 0.
* Some require a more complex setup for this, which is why they're commented out for now.
* EDIT: They can now be done, but need someone to write the necessary terrain/items/vehicles/etc spawns.
* Activity actors that use ui functionality in their start methods cannot work at all,
* only affects workout_activity_actor right now
*/
static const std::vector<std::function<player_activity()>> test_activities {
    //player_activity( autodrive_activity_actor() ),
    //player_activity( bikerack_racking_activity_actor() ),
    //player_activity( boltcutting_activity_actor( north, item_location() ) ),
    [] {
        Character *dummy = get_avatar().as_character();
        dummy->wear_item( item( itype_test_backpack, calendar::turn ), false );
        item_location bookbinder = dummy->i_add( item( itype_book_binder, calendar::turn ) );
        return player_activity( bookbinder_copy_activity_actor( bookbinder, recipe_water_clean ) );
    },
    [] { return player_activity( consume_activity_actor( item( itype_water_clean ) ) ); },
    //player_activity( craft_activity_actor() ),
    //player_activity( disable_activity_actor() ),
    //player_activity( disassemble_activity_actor( 1 ) ),
    [] { return player_activity( drop_activity_actor() ); },
    //player_activity( ebooksave_activity_actor( loc, loc ) ),
    [] { return player_activity( firstaid_activity_actor( 1, std::string(), get_avatar().getID() ) ); },
    [] { return player_activity( forage_activity_actor( 1 ) ); },
    [] { return player_activity( gunmod_remove_activity_actor( 1, item_location(), 0 ) ); },
    [] { return player_activity( hacking_activity_actor( item_location() ) ); },
    //player_activity( hacksaw_activity_actor( p, loc ) ),
    [] { return player_activity( haircut_activity_actor() ); },
    //player_activity( harvest_activity_actor( p ) ),
    [] { return player_activity( hotwire_car_activity_actor( 1, get_avatar().pos() ) ); },
    //player_activity( insert_item_activity_actor() ),
    [] { return player_activity( lockpick_activity_actor::use_item( 1, item_location(), get_avatar().pos() ) ); },
    //player_activity( longsalvage_activity_actor() ),
    [] { return player_activity( meditate_activity_actor() ); },
    [] { return player_activity( migration_cancel_activity_actor() ); },
    [] { return player_activity( milk_activity_actor( 1, {get_avatar().pos()}, {std::string()} ) ); },
    [] { return player_activity( mop_activity_actor( 1 ) ); },
    //player_activity( move_furniture_activity_actor( p, false ) ),
    [] { return player_activity( move_items_activity_actor( {}, {}, false, get_avatar().pos() + tripoint_north ) ); },
    [] { return player_activity( open_gate_activity_actor( 1, get_avatar().pos_bub() ) ); },
    //player_activity( oxytorch_activity_actor( p, loc ) ),
    [] { return player_activity( pickup_activity_actor( {}, {}, std::nullopt, false ) ); },
    [] { return player_activity( play_with_pet_activity_actor() ); },
    //player_activity( prying_activity_actor( p, loc ) ),
    //player_activity( read_activity_actor() ),
    [] {
        Character *dummy = get_avatar().as_character();
        dummy->wear_item( item( itype_test_backpack, calendar::turn ), false );
        item_location target = dummy->i_add( item( itype_test_oxytorch, calendar::turn ) );
        item_location ammo = dummy->i_add( item( itype_test_weldtank, calendar::turn ) );
        item::reload_option opt( dummy, target, ammo );
        return player_activity( reload_activity_actor( std::move( opt ) ) );
    },
    [] { return player_activity( safecracking_activity_actor( get_avatar().pos() + tripoint_north ) ); },
    [] { return player_activity( shave_activity_actor() ); },
    //player_activity( shearing_activity_actor( north ) ),
    [] { return player_activity( stash_activity_actor() ); },
    [] { return player_activity( tent_deconstruct_activity_actor( 1, 1, get_avatar().pos(), itype_tent_kit ) ); },
    //player_activity( tent_placement_activity_actor() ),
    [] { return player_activity( try_sleep_activity_actor( time_duration::from_hours( 1 ) ) ); },
    [] { return player_activity( unload_activity_actor( 1, item_location() ) ); },
    [] { return player_activity( wear_activity_actor( {}, {} ) ); },
    [] { return player_activity( wield_activity_actor( item_location(), 1 ) ); },
    //player_activity( workout_activity_actor( p ) )
};

static void cleanup( avatar &dummy )
{
    dummy.inv->clear();
    clear_map();

    REQUIRE( dummy.activity.get_distractions().empty() );
    REQUIRE( !dummy.activity.is_distraction_ignored( distraction_type::hostile_spotted_near ) );
    REQUIRE( !dummy.activity.is_distraction_ignored( distraction_type::dangerous_field ) );
}

static void update_cache( map &m )
{
    // Why twice? See vision_test.cpp
    m.invalidate_visibility_cache();
    m.update_visibility_cache( 0 );
    m.invalidate_map_cache( 0 );
    m.build_map_cache( 0 );
    m.invalidate_visibility_cache();
    m.update_visibility_cache( 0 );
    m.invalidate_map_cache( 0 );
    m.build_map_cache( 0 );
}

TEST_CASE( "activity_interruption_by_distractions", "[activity][interruption]" )
{
    clear_avatar();
    clear_map();
    set_time_to_day();
    scoped_weather_override clear_weather( WEATHER_CLEAR );
    avatar &dummy = get_avatar();
    map &m = get_map();

    for( const std::function<player_activity()> &setup_activity : test_activities ) {
        player_activity activity = setup_activity();
        CAPTURE( activity.id() );
        dummy.activity = activity;
        dummy.activity.start_or_resume( dummy, false );

        //this must be larger than 0 for interruption to happen
        REQUIRE( dummy.activity.moves_left > 0 );
        //some activities are set to null during start if they don't get valid data
        REQUIRE( dummy.activity.id() != ACT_NULL );
        //aiming is excluded from this kind of interruption
        REQUIRE( dummy.activity.id() != ACT_AIM );

        tripoint zombie_pos_near = dummy.pos() + tripoint( 2, 0, 0 );
        tripoint zombie_pos_far = dummy.pos() + tripoint( 10, 0, 0 );

        //to make section names unique
        std::string act = activity.id().str();

        SECTION( act + " interruption by nearby enemy" ) {
            cleanup( dummy );

            spawn_test_monster( mon_zombie.str(), zombie_pos_near );
            update_cache( m );

            std::map<distraction_type, std::string> dists = dummy.activity.get_distractions();

            CHECK( dists.size() == 1 );
            CHECK( dists.find( distraction_type::hostile_spotted_near ) != dists.end() );
        }

        SECTION( act + " enemy too far away to interrupt" ) {
            cleanup( dummy );

            monster &zombie = spawn_test_monster( mon_zombie.str(), zombie_pos_far );
            update_cache( m );

            REQUIRE( dummy.sees( zombie ) );

            std::map<distraction_type, std::string> dists = dummy.activity.get_distractions();

            CHECK( dists.empty() );

            THEN( "interruption by zombie moving towards dummy" ) {
                zombie.set_dest( get_map().getglobal( dummy.pos() ) );
                int turns = 0;
                do {
                    move_monster_turn( zombie );
                    dists = dummy.activity.get_distractions();
                    turns++;
                } while( turns < 10 && dists.empty() );

                CHECK( dists.size() == 1 );
                CHECK( dists.find( distraction_type::hostile_spotted_near ) != dists.end() );
            }
        }

        SECTION( act + " enemy nearby, but no line of sight" ) {
            cleanup( dummy );

            m.ter_set( dummy.pos() + tripoint_east, ter_t_wall );
            monster &zombie = spawn_test_monster( mon_zombie.str(), zombie_pos_near );
            update_cache( m );

            REQUIRE( !dummy.sees( zombie ) );

            std::map<distraction_type, std::string> dists = dummy.activity.get_distractions();

            CHECK( dists.empty() );

            THEN( "interruption by zombie moving towards dummy" ) {
                zombie.set_dest( get_map().getglobal( dummy.pos() ) );
                int turns = 0;
                do {
                    move_monster_turn( zombie );
                    dists = dummy.activity.get_distractions();
                    turns++;
                } while( turns < 5 && dists.empty() );

                CHECK( dists.size() == 1 );
                CHECK( dists.find( distraction_type::hostile_spotted_near ) != dists.end() );
            }
        }

        SECTION( act + " interruption by dangerous field" ) {
            cleanup( dummy );

            m.add_field( dummy.pos(), field_fd_smoke );

            std::map<distraction_type, std::string> dists = dummy.activity.get_distractions();

            CHECK( dists.size() == 1 );
            CHECK( dists.find( distraction_type::dangerous_field ) != dists.end() );
        }

        SECTION( act + " interruption by multiple sources" ) {
            cleanup( dummy );

            spawn_test_monster( mon_zombie.str(), zombie_pos_near );
            m.add_field( dummy.pos(), field_fd_smoke );
            std::map<distraction_type, std::string> dists = dummy.activity.get_distractions();

            CHECK( dists.size() == 2 );
            CHECK( dists.find( distraction_type::hostile_spotted_near ) != dists.end() );
            CHECK( dists.find( distraction_type::dangerous_field ) != dists.end() );
        }
    }
}
