#include "catch/catch.hpp"
#include "map_helpers.h"
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
#include "point.h"

static const activity_id ACT_NULL( "ACT_NULL" );
static const activity_id ACT_BOLTCUTTING( "ACT_BOLTCUTTING" );
static const activity_id ACT_CRACKING( "ACT_CRACKING" );
static const activity_id ACT_OXYTORCH( "ACT_OXYTORCH" );
static const activity_id ACT_SHEARING( "ACT_SHEARING" );

static const efftype_id effect_pet( "pet" );
static const efftype_id effect_tied( "tied" );

static const furn_str_id furn_t_test_f_boltcut1( "test_f_boltcut1" );
static const furn_str_id furn_t_test_f_boltcut2( "test_f_boltcut2" );
static const furn_str_id furn_t_test_f_boltcut3( "test_f_boltcut3" );
static const furn_str_id furn_t_test_f_oxytorch1( "test_f_oxytorch1" );
static const furn_str_id furn_t_test_f_oxytorch2( "test_f_oxytorch2" );
static const furn_str_id furn_t_test_f_oxytorch3( "test_f_oxytorch3" );

static const itype_id itype_oxyacetylene( "oxyacetylene" );
static const itype_id itype_test_battery_disposable( "test_battery_disposable" );
static const itype_id itype_test_boltcutter( "test_boltcutter" );
static const itype_id itype_test_boltcutter_elec( "test_boltcutter_elec" );
static const itype_id itype_test_oxytorch( "test_oxytorch" );
static const itype_id itype_test_shears( "test_shears" );
static const itype_id itype_test_shears_off( "test_shears_off" );

static const json_character_flag json_flag_SUPER_HEARING( "SUPER_HEARING" );

static const mtype_id mon_test_shearable( "mon_test_shearable" );
static const mtype_id mon_test_non_shearable( "mon_test_non_shearable" );

static const proficiency_id proficiency_prof_safecracking( "prof_safecracking" );

static const quality_id qual_SHEAR( "SHEAR" );
static const quality_id qual_WELD( "WELD" );

static const ter_str_id ter_test_t_oxytorch1( "test_t_oxytorch1" );
static const ter_str_id ter_test_t_oxytorch2( "test_t_oxytorch2" );

static const skill_id skill_traps( "traps" );

static const ter_str_id ter_test_t_boltcut1( "test_t_boltcut1" );
static const ter_str_id ter_test_t_boltcut2( "test_t_boltcut2" );

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
            REQUIRE( dummy.get_skill_level( skill_traps ) == skill_level );
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
            mp.furn_set( safe, f_safe_l );
            REQUIRE( !dummy.has_item_with_flag( flag_SAFECRACK ) );
            REQUIRE( !dummy.has_flag( json_flag_SUPER_HEARING ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( mp.furn( safe ) == f_safe_l );

            WHEN( "player tries safecracking" ) {
                process_activity( dummy );
                THEN( "activity is canceled" ) {
                    CHECK( mp.furn( safe ) == f_safe_l );
                }
            }
        }

        GIVEN( "player has a stethoscope" ) {
            dummy.i_add( item( "stethoscope" ) );
            mp.furn_set( safe, f_safe_l );
            REQUIRE( dummy.has_item_with_flag( flag_SAFECRACK ) );
            REQUIRE( !dummy.has_flag( json_flag_SUPER_HEARING ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( mp.furn( safe ) == f_safe_l );

            WHEN( "player completes the safecracking activity" ) {
                process_activity( dummy );
                THEN( "safe is unlocked" ) {
                    CHECK( mp.furn( safe ) == f_safe_c );
                }
            }
        }

        GIVEN( "player has a stethoscope" ) {
            dummy.worn.clear();
            dummy.remove_weapon();
            dummy.add_bionic( bionic_id( "bio_ears" ) );
            mp.furn_set( safe, f_safe_l );
            REQUIRE( !dummy.has_item_with_flag( flag_SAFECRACK ) );
            REQUIRE( dummy.has_flag( json_flag_SUPER_HEARING ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( mp.furn( safe ) == f_safe_l );

            WHEN( "player completes the safecracking activity" ) {
                process_activity( dummy );
                THEN( "safe is unlocked" ) {
                    CHECK( mp.furn( safe ) == f_safe_c );
                }
            }
        }

        GIVEN( "player has a stethoscope" ) {
            dummy.clear_bionics();
            dummy.i_add( item( "stethoscope" ) );
            mp.furn_set( safe, f_safe_l );
            REQUIRE( dummy.has_item_with_flag( flag_SAFECRACK ) );
            REQUIRE( !dummy.has_flag( json_flag_SUPER_HEARING ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( mp.furn( safe ) == f_safe_l );

            WHEN( "player is safecracking" ) {
                dummy.moves += dummy.get_speed();
                for( int i = 0; i < 100; ++i ) {
                    dummy.activity.do_turn( dummy );
                }

                THEN( "player loses their stethoscope" ) {
                    dummy.worn.clear();
                    dummy.remove_weapon();
                    REQUIRE( !dummy.has_item_with_flag( flag_SAFECRACK ) );

                    process_activity( dummy );
                    THEN( "activity is canceled" ) {
                        CHECK( mp.furn( safe ) == f_safe_l );
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
            mp.furn_set( safe, f_safe_l );
            REQUIRE( dummy.has_item_with_flag( flag_SAFECRACK ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( mp.furn( safe ) == f_safe_l );

            REQUIRE( !dummy.has_proficiency( proficiency_prof_safecracking ) );

            dummy.set_focus( 100 );
            REQUIRE( dummy.get_focus() == 100 );

            const time_duration time_before = get_safecracking_time();

            process_activity( dummy );
            REQUIRE( mp.furn( safe ) == f_safe_c );
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
            elec_shears.put_in( battery, item_pocket::pocket_type::MAGAZINE_WELL );

            const use_function *use = elec_shears.type->get_use( "transform" );
            REQUIRE( use != nullptr );
            const iuse_transform *actor = dynamic_cast<const iuse_transform *>( use->get_actor_ptr() );
            actor->use( dummy, elec_shears, false, dummy.pos() );

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
            elec_shears.put_in( battery, item_pocket::pocket_type::MAGAZINE_WELL );

            const use_function *use = elec_shears.type->get_use( "transform" );
            REQUIRE( use != nullptr );
            const iuse_transform *actor = dynamic_cast<const iuse_transform *>( use->get_actor_ptr() );
            actor->use( dummy, elec_shears, false, dummy.pos() );

            dummy.i_add( elec_shears );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 3 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos(), false ) );
            dummy.activity.start_or_resume( dummy, false );
            REQUIRE( dummy.activity.id() == ACT_SHEARING );

            dummy.moves += dummy.get_speed();
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

            const itype_id test_amount( "test_rock" );
            const itype_id test_random( "test_2x4" );
            const itype_id test_mass( "test_rag" );
            const itype_id test_volume( "test_pipe" );

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

        return item_location{dummy, dummy.get_wielded_item()};
    };

    auto setup_activity = [&dummy]( const item_location & torch ) -> void {
        boltcutting_activity_actor act{tripoint_zero, torch};
        act.testing = true;
        dummy.assign_activity( player_activity( act ) );
    };

    SECTION( "boltcut start checks" ) {
        GIVEN( "a tripoint with nothing" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, t_null );
            REQUIRE( mp.ter( tripoint_zero ) == t_null );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            THEN( "boltcutting activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with invalid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, t_dirt );
            REQUIRE( mp.ter( tripoint_zero ) == t_dirt );

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

            mp.furn_set( tripoint_zero, furn_t_test_f_boltcut1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_t_test_f_boltcut1 );

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

            mp.furn_set( tripoint_zero, furn_t_test_f_boltcut1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_t_test_f_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );
            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );

            WHEN( "furniture has a duration of 5 seconds" ) {
                REQUIRE( furn_t_test_f_boltcut1->boltcut->duration() == 5_seconds );
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

            mp.furn_set( tripoint_zero, furn_t_test_f_boltcut3 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_t_test_f_boltcut3 );

            item battery( itype_test_battery_disposable );
            battery.ammo_set( battery.ammo_default(), 2 );

            item it_boltcut_elec( itype_test_boltcutter_elec );
            it_boltcut_elec.put_in( battery, item_pocket::pocket_type::MAGAZINE_WELL );

            dummy.wield( it_boltcut_elec );
            REQUIRE( dummy.get_wielded_item()->typeId() == itype_test_boltcutter_elec );

            item_location boltcutter_elec{dummy, dummy.get_wielded_item()};

            setup_activity( boltcutter_elec );
            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );

            WHEN( "player runs out of charges" ) {
                REQUIRE( dummy.activity.id() == ACT_NULL );

                THEN( "player recharges with fuel" ) {
                    boltcutter_elec->ammo_set( battery.ammo_default() );

                    AND_THEN( "player can resume the activity" ) {
                        setup_activity( boltcutter_elec );
                        dummy.moves = dummy.get_speed();
                        dummy.activity.do_turn( dummy );
                        CHECK( dummy.activity.id() == ACT_BOLTCUTTING );
                        CHECK( dummy.activity.moves_left < to_moves<int>( furn_t_test_f_boltcut3->boltcut->duration() ) );
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
                CHECK( mp.ter( tripoint_zero ) == t_dirt );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_t_test_f_boltcut1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_t_test_f_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_zero ) == f_null );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_t_test_f_boltcut2 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_t_test_f_boltcut2 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_zero ) == furn_t_test_f_boltcut1 );
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

            const itype_id test_amount( "test_rock" );
            const itype_id test_random( "test_2x4" );

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

TEST_CASE( "oxytorch", "[activity][oxytorch]" )
{
    map &mp = get_map();
    avatar &dummy = get_avatar();

    auto setup_dummy = [&dummy]() -> item_location {
        item it_welding_torch( itype_test_oxytorch );
        it_welding_torch.ammo_set( itype_oxyacetylene );

        dummy.wield( it_welding_torch );
        REQUIRE( dummy.weapon.typeId() == itype_test_oxytorch );
        REQUIRE( dummy.max_quality( qual_WELD ) == 10 );

        return item_location{dummy, &dummy.weapon};
    };

    auto setup_activity = [&dummy]( const item_location & torch ) -> void {
        oxytorch_activity_actor act{tripoint_zero, torch};
        act.testing = true;
        dummy.assign_activity( player_activity( act ) );
    };

    SECTION( "oxytorch start checks" ) {
        GIVEN( "a tripoint with nothing" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, t_null );
            REQUIRE( mp.ter( tripoint_zero ) == t_null );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            THEN( "oxytorch activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with invalid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_zero, t_dirt );
            REQUIRE( mp.ter( tripoint_zero ) == t_dirt );

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

            mp.furn_set( tripoint_zero, furn_t_test_f_oxytorch1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_t_test_f_oxytorch1 );

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

            mp.furn_set( tripoint_zero, furn_t_test_f_oxytorch1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_t_test_f_oxytorch1 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );
            REQUIRE( dummy.activity.id() == ACT_OXYTORCH );

            WHEN( "furniture has a duration of 5 seconds" ) {
                REQUIRE( furn_t_test_f_oxytorch1->oxytorch->duration() == 5_seconds );
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

            mp.furn_set( tripoint_zero, furn_t_test_f_oxytorch3 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_t_test_f_oxytorch3 );

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
                        dummy.moves = dummy.get_speed();
                        dummy.activity.do_turn( dummy );
                        CHECK( dummy.activity.id() == ACT_OXYTORCH );
                        CHECK( dummy.activity.moves_left < to_moves<int>( furn_t_test_f_oxytorch3->oxytorch->duration() ) );
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
                CHECK( mp.ter( tripoint_zero ) == t_dirt );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_t_test_f_oxytorch1 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_t_test_f_oxytorch1 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            REQUIRE( dummy.activity.id() == ACT_OXYTORCH );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_zero ) == f_null );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_zero, furn_t_test_f_oxytorch2 );
            REQUIRE( mp.furn( tripoint_zero ) == furn_t_test_f_oxytorch2 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            REQUIRE( dummy.activity.id() == ACT_OXYTORCH );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_zero ) == furn_t_test_f_oxytorch1 );
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

            const itype_id test_amount( "test_rock" );
            const itype_id test_random( "test_2x4" );

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
