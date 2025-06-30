#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "enums.h"
#include "flag.h"
#include "handle_liquid.h"
#include "inventory.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "map.h"
#include "map_helpers.h"
#include "mapdata.h"
#include "monster.h"
#include "monster_helpers.h"
#include "options_helpers.h"
#include "pimpl.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "proficiency.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "weather_type.h"

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

static const item_group_id Item_spawn_data_test_edevices_compat( "test_edevices_compat" );
static const item_group_id Item_spawn_data_test_edevices_incompat( "test_edevices_incompat" );
static const item_group_id Item_spawn_data_test_edevices_power( "test_edevices_power" );
static const item_group_id Item_spawn_data_test_edevices_recipes( "test_edevices_recipes" );
static const item_group_id Item_spawn_data_test_edevices_standard( "test_edevices_standard" );

static const itype_id itype_book_binder( "book_binder" );
static const itype_id itype_glass_shard( "glass_shard" );
static const itype_id itype_memory_card( "memory_card" );
static const itype_id itype_oxyacetylene( "oxyacetylene" );
static const itype_id itype_stethoscope( "stethoscope" );
static const itype_id itype_tent_kit( "tent_kit" );
static const itype_id itype_test_2x4( "test_2x4" );
static const itype_id itype_test_backpack( "test_backpack" );
static const itype_id itype_test_battery_disposable( "test_battery_disposable" );
static const itype_id itype_test_boltcutter( "test_boltcutter" );
static const itype_id itype_test_boltcutter_elec( "test_boltcutter_elec" );
static const itype_id itype_test_efile_copiable( "test_efile_copiable" );
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

static const skill_id skill_computer( "computer" );
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
        map &here = get_map();
        clear_avatar();
        clear_map();

        tripoint_bub_ms safe;
        dummy.setpos( here, safe + tripoint::east );

        dummy.activity = player_activity( safecracking_activity_actor( safe ) );
        dummy.activity.start_or_resume( dummy, false );

        GIVEN( "player without the required tools" ) {
            here.furn_set( safe, furn_f_safe_l );
            REQUIRE( !dummy.cache_has_item_with( flag_SAFECRACK ) );
            REQUIRE( !dummy.has_flag( json_flag_SAFECRACK_NO_TOOL ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( here.furn( safe ) == furn_f_safe_l );

            WHEN( "player tries safecracking" ) {
                process_activity( dummy );
                THEN( "activity is canceled" ) {
                    CHECK( here.furn( safe ) == furn_f_safe_l );
                }
            }
        }

        GIVEN( "player has a stethoscope" ) {
            dummy.i_add( item( itype_stethoscope ) );
            here.furn_set( safe, furn_f_safe_l );
            REQUIRE( dummy.cache_has_item_with( flag_SAFECRACK ) );
            REQUIRE( !dummy.has_flag( json_flag_SAFECRACK_NO_TOOL ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( here.furn( safe ) == furn_f_safe_l );

            WHEN( "player completes the safecracking activity" ) {
                process_activity( dummy );
                THEN( "safe is unlocked" ) {
                    CHECK( here.furn( safe ) == furn_f_safe_c );
                }
            }
        }

        GIVEN( "player has a stethoscope" ) {
            dummy.clear_worn();
            dummy.remove_weapon();
            dummy.add_bionic( bio_ears );
            here.furn_set( safe, furn_f_safe_l );
            REQUIRE( !dummy.cache_has_item_with( flag_SAFECRACK ) );
            REQUIRE( dummy.has_flag( json_flag_SAFECRACK_NO_TOOL ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( here.furn( safe ) == furn_f_safe_l );

            WHEN( "player completes the safecracking activity" ) {
                process_activity( dummy );
                THEN( "safe is unlocked" ) {
                    CHECK( here.furn( safe ) == furn_f_safe_c );
                }
            }
        }

        GIVEN( "player has a stethoscope" ) {
            dummy.clear_bionics();
            dummy.i_add( item( itype_stethoscope ) );
            here.furn_set( safe, furn_f_safe_l );
            REQUIRE( dummy.cache_has_item_with( flag_SAFECRACK ) );
            REQUIRE( !dummy.has_flag( json_flag_SAFECRACK_NO_TOOL ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( here.furn( safe ) == furn_f_safe_l );

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
                        CHECK( here.furn( safe ) == furn_f_safe_l );
                    }
                }
            }
        }
    }

    SECTION( "safecracking proficiency test" ) {
        map &here = get_map();

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

        tripoint_bub_ms safe;
        dummy.setpos( here, safe + tripoint::east );

        dummy.activity = player_activity( safecracking_activity_actor( safe ) );
        dummy.activity.start_or_resume( dummy, false );

        GIVEN( "player cracks one safe" ) {
            dummy.i_add( item( itype_stethoscope ) );
            here.furn_set( safe, furn_f_safe_l );
            REQUIRE( dummy.cache_has_item_with( flag_SAFECRACK ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( here.furn( safe ) == furn_f_safe_l );

            REQUIRE( !dummy.has_proficiency( proficiency_prof_safecracking ) );

            dummy.set_focus( 100 );
            REQUIRE( dummy.get_focus() == 100 );

            const time_duration time_before = get_safecracking_time();

            process_activity( dummy );
            REQUIRE( here.furn( safe ) == furn_f_safe_c );
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
            mon = &spawn_test_monster( mon_test_shearable.str(), dummy.pos_bub() + tripoint::north );
        } else
        {
            mon = &spawn_test_monster( mon_test_non_shearable.str(), dummy.pos_bub() + tripoint::north );
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
            dummy.activity = player_activity( shearing_activity_actor( mon.pos_bub(), false ) );
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

            dummy.activity = player_activity( shearing_activity_actor( mon.pos_bub(), false ) );
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
            actor->use( &dummy, elec_shears, dummy.pos_bub() );

            dummy.i_add( elec_shears );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 3 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos_bub(), false ) );
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

            dummy.activity = player_activity( shearing_activity_actor( mon.pos_bub(), false ) );
            dummy.activity.start_or_resume( dummy, false );

            THEN( "shearing can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }
    }

    SECTION( "shearing losing tool" ) {
        GIVEN( "an electric tool with shearing quality three" ) {
            map &here = get_map();

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
            actor->use( &dummy, elec_shears, dummy.pos_bub() );

            dummy.i_add( elec_shears );
            REQUIRE( dummy.max_quality( qual_SHEAR ) == 3 );

            dummy.activity = player_activity( shearing_activity_actor( mon.pos_bub(), false ) );
            dummy.activity.start_or_resume( dummy, false );
            REQUIRE( dummy.activity.id() == ACT_SHEARING );

            dummy.mod_moves( dummy.get_speed() );
            for( int i = 0; i < 100; ++i ) {
                dummy.activity.do_turn( dummy );
            }

            WHEN( "tool runs out of charges mid activity" ) {
                for( int i = 0; i < 10000; ++i ) {
                    dummy.process_items( &here );
                }

                CHECK( dummy.get_wielded_item()->ammo_remaining( ) == 0 );
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

            dummy.activity = player_activity( shearing_activity_actor( mon.pos_bub(), false ) );
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
                    const map_stack items = get_map().i_at( dummy.pos_bub() );
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

            dummy.activity = player_activity( shearing_activity_actor( mon.pos_bub(), false ) );
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

            dummy.activity = player_activity( shearing_activity_actor( mon.pos_bub(), true ) );
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
        boltcutting_activity_actor act{tripoint_bub_ms::zero, torch};
        act.testing = true;
        dummy.assign_activity( act );
    };

    SECTION( "boltcut start checks" ) {
        GIVEN( "a tripoint with nothing" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_str_id::NULL_ID() );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_str_id::NULL_ID() );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            THEN( "boltcutting activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with invalid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_t_dirt );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_t_dirt );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            THEN( "boltcutting activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_boltcut1 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            THEN( "boltcutting activity can start" ) {
                CHECK( dummy.activity.id() == ACT_BOLTCUTTING );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_boltcut1 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            THEN( "boltcutting activity can start" ) {
                CHECK( dummy.activity.id() == ACT_BOLTCUTTING );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_boltcut1 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_boltcut1 );

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

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_boltcut1 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_boltcut1 );

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

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_boltcut3 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_boltcut3 );

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

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_boltcut1 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "terrain gets converted to new terrain type" ) {
                CHECK( mp.ter( tripoint_bub_ms::zero ) == ter_t_dirt );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_boltcut1 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_boltcut1 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_bub_ms::zero ) == furn_str_id::NULL_ID() );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_boltcut2 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_boltcut2 );

            item_location boltcutter = setup_dummy();
            setup_activity( boltcutter );

            REQUIRE( dummy.activity.id() == ACT_BOLTCUTTING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_boltcut1 );
            }
        }

        GIVEN( "a tripoint with a valid furniture with byproducts" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_boltcut2 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_boltcut2 );

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
                    const map_stack items = get_map().i_at( tripoint_bub_ms::zero );
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
        hacksaw_activity_actor act{tripoint_bub_ms::zero, torch};
        act.testing = true;
        dummy.assign_activity( act );
    };

    SECTION( "hacksaw start checks" ) {
        GIVEN( "a tripoint with nothing" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_str_id::NULL_ID() );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_str_id::NULL_ID() );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );

            THEN( "hacksaw activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with invalid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_t_dirt );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_t_dirt );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );

            THEN( "hacksaw activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_hacksaw1 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_hacksaw1 );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );

            THEN( "hacksaw activity can start" ) {
                CHECK( dummy.activity.id() == ACT_HACKSAW );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_hacksaw1 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_hacksaw1 );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );

            THEN( "hacksaw activity can start" ) {
                CHECK( dummy.activity.id() == ACT_HACKSAW );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_hacksaw1 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_hacksaw1 );

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

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_hacksaw1 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_hacksaw1 );

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

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_hacksaw3 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_hacksaw3 );

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

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_hacksaw1 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_hacksaw1 );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );

            REQUIRE( dummy.activity.id() == ACT_HACKSAW );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "terrain gets converted to new terrain type" ) {
                CHECK( mp.ter( tripoint_bub_ms::zero ) == ter_t_dirt );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_hacksaw1 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_hacksaw1 );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );

            REQUIRE( dummy.activity.id() == ACT_HACKSAW );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_bub_ms::zero ) == furn_str_id::NULL_ID() );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_hacksaw2 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_hacksaw2 );

            item_location hacksaw = setup_dummy();
            setup_activity( hacksaw );

            REQUIRE( dummy.activity.id() == ACT_HACKSAW );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_hacksaw1 );
            }
        }

        GIVEN( "a tripoint with a valid furniture with byproducts" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_hacksaw2 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_hacksaw2 );

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
                    const map_stack items = get_map().i_at( tripoint_bub_ms::zero );
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
        oxytorch_activity_actor act{tripoint_bub_ms::zero, torch};
        act.testing = true;
        dummy.assign_activity( act );
    };

    SECTION( "oxytorch start checks" ) {
        GIVEN( "a tripoint with nothing" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_str_id::NULL_ID() );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_str_id::NULL_ID() );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            THEN( "oxytorch activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with invalid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_t_dirt );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_t_dirt );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            THEN( "oxytorch activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_oxytorch1 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_oxytorch1 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            THEN( "oxytorch activity can start" ) {
                CHECK( dummy.activity.id() == ACT_OXYTORCH );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_oxytorch1 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_oxytorch1 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            THEN( "oxytorch activity can start" ) {
                CHECK( dummy.activity.id() == ACT_OXYTORCH );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_oxytorch1 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_oxytorch1 );

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

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_oxytorch1 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_oxytorch1 );

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

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_oxytorch3 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_oxytorch3 );

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

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_oxytorch1 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_oxytorch1 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            REQUIRE( dummy.activity.id() == ACT_OXYTORCH );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "terrain gets converted to new terrain type" ) {
                CHECK( mp.ter( tripoint_bub_ms::zero ) == ter_t_dirt );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_oxytorch1 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_oxytorch1 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            REQUIRE( dummy.activity.id() == ACT_OXYTORCH );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_bub_ms::zero ) == furn_str_id::NULL_ID() );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_oxytorch2 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_oxytorch2 );

            item_location welding_torch = setup_dummy();
            setup_activity( welding_torch );

            REQUIRE( dummy.activity.id() == ACT_OXYTORCH );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_oxytorch1 );
            }
        }

        GIVEN( "a tripoint with a valid furniture with byproducts" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_oxytorch2 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_oxytorch2 );

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
                    const map_stack items = get_map().i_at( tripoint_bub_ms::zero );
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
    const tripoint_bub_ms &target = tripoint_bub_ms::zero ) -> void {
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

            THEN( "prying time is 30 seconds" ) {
                CHECK( prying_time == 30_seconds );
            }
        }
    }

    SECTION( "prying start checks" ) {
        GIVEN( "a tripoint with nothing" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_str_id::NULL_ID() );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_str_id::NULL_ID() );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );

            THEN( "prying activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with invalid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_t_dirt );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_t_dirt );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );

            THEN( "prying activity can't start" ) {
                CHECK( dummy.activity.id() == ACT_NULL );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_prying1 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_prying1 );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );

            THEN( "prying activity can start" ) {
                CHECK( dummy.activity.id() == ACT_PRYING );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_prying1 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_prying1 );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );

            THEN( "oxytorch activity can start" ) {
                CHECK( dummy.activity.id() == ACT_PRYING );
            }
        }

        GIVEN( "a tripoint with valid terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_prying1 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_prying1 );

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

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_prying1 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_prying1 );

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

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_prying1 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_prying1 );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );

            REQUIRE( dummy.activity.id() == ACT_PRYING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "terrain gets converted to new terrain type" ) {
                CHECK( mp.ter( tripoint_bub_ms::zero ) == ter_t_dirt );
            }
        }

        GIVEN( "a tripoint with valid furniture" ) {
            clear_map();
            clear_avatar();

            mp.furn_set( tripoint_bub_ms::zero, furn_test_f_prying1 );
            REQUIRE( mp.furn( tripoint_bub_ms::zero ) == furn_test_f_prying1 );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );

            REQUIRE( dummy.activity.id() == ACT_PRYING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "furniture gets converted to new furniture type" ) {
                CHECK( mp.furn( tripoint_bub_ms::zero ) == furn_str_id::NULL_ID() );
            }
        }
    }

    SECTION( "prying finish checks without prying_nails" ) {
        GIVEN( "a tripoint with valid impossible to pry open terrain" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_prying2 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_prying2 );

            item_location prying_tool = setup_dummy( true );
            setup_activity( prying_tool );

            REQUIRE( dummy.activity.id() == ACT_PRYING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "activity fails" ) {
                CHECK( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_prying2 );
            }
        }

        GIVEN( "a tripoint with valid terrain with a tool that always opens it" ) {
            clear_map();
            clear_avatar();

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_prying2 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_prying2 );

            item_location prying_tool = setup_dummy( false );
            setup_activity( prying_tool );

            REQUIRE( dummy.activity.id() == ACT_PRYING );
            process_activity( dummy );
            REQUIRE( dummy.activity.id() == ACT_NULL );

            THEN( "terrain gets converted to new type" ) {
                CHECK( mp.ter( tripoint_bub_ms::zero ) == ter_t_dirt );
            }
        }

        GIVEN( "a tripoint with valid terrain that will break" ) {
            clear_map();
            clear_avatar();

            const tripoint_bub_ms terrain_pos = dummy.pos_bub() + tripoint::north;

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

            mp.ter_set( tripoint_bub_ms::zero, ter_test_t_prying1 );
            REQUIRE( mp.ter( tripoint_bub_ms::zero ) == ter_test_t_prying1 );

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
                    const map_stack items = get_map().i_at( tripoint_bub_ms::zero );
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

static void update_efiles( std::vector<item_location> &edevice_locs,
                           std::vector<item_location> &efile_locs,
                           std::vector<item_location> &copiable_efile_locs )
{
    //update files
    efile_locs.clear();
    copiable_efile_locs.clear();
    for( item_location &edevice : edevice_locs ) {
        for( item *efile : edevice->efiles() ) {
            efile_locs.emplace_back( edevice, efile );
        }
    }
    for( item_location &loc : efile_locs ) {
        if( loc->is_ecopiable() ) {
            copiable_efile_locs.emplace_back( loc );
        }
    }
}

TEST_CASE( "edevice", "[activity][edevice]" )
{
    avatar dummy;
    dummy.set_skill_level( skill_computer, 1 );
    clear_map();
    std::vector<item_location> edevice_locs;
    std::vector<item_location> efile_locs;
    std::vector<item_location> copiable_efile_locs;

    item_location laptop_with_files;
    item_location edevice_without_files;
    std::vector<item_location> vector_laptop_with_files;
    std::vector<item_location> vector_edevice_without_files;

    const std::function<bool( const item &i )> copy_efile_filter = []( const item & i ) {
        return i.typeId() == itype_test_efile_copiable;
    };
    auto do_activity = [&dummy]( item_location & used_edevice,
                                 std::vector<item_location> &target_edevices,
    std::vector<item_location> &selected_efiles, efile_action action, bool pass_time = true ) {
        efile_activity_actor act( used_edevice, target_edevices, selected_efiles,
                                  action, efile_combo::COMBO_NONE );
        dummy.assign_activity( act );
        process_activity( dummy, pass_time );
    };

    auto add_edevices = [&]( const item_group_id & igroup, bool add_etd = true ) {
        dummy.clear_worn();
        dummy.wear_item( item( itype_test_backpack ) );
        edevice_locs.clear();
        item_group::ItemList items = item_group::items_from( igroup );
        for( item &i : items ) {
            REQUIRE( !i.is_browsed() );
            edevice_locs.emplace_back( dummy.i_add( i ) );
        }
        //every test item group is only two devices
        laptop_with_files = edevice_locs.front();
        vector_laptop_with_files.clear();
        vector_laptop_with_files.emplace_back( laptop_with_files );

        edevice_without_files = edevice_locs.back();
        vector_edevice_without_files.clear();
        vector_edevice_without_files.emplace_back( edevice_without_files );
        if( add_etd ) {
            item etd( itype_memory_card );
            etd.set_browsed( true );
            dummy.i_add( etd );
        }
        REQUIRE( !!laptop_with_files );
        REQUIRE( !!edevice_without_files );

        update_efiles( edevice_locs, efile_locs, copiable_efile_locs );
        //browse required before any further efile operations
        do_activity( laptop_with_files, edevice_locs, efile_locs, EF_BROWSE );
        for( item_location &i : edevice_locs ) {
            REQUIRE( i->is_browsed() );
        }
    };

    SECTION( "move to / move from" ) {
        add_edevices( Item_spawn_data_test_edevices_standard );
        REQUIRE( laptop_with_files->efiles().size() == 3 );
        REQUIRE( edevice_without_files->efiles().empty() );
        do_activity( edevice_without_files, vector_laptop_with_files, efile_locs, EF_MOVE_ONTO_THIS );
        REQUIRE( edevice_without_files->efiles().size() == 3 );
        REQUIRE( laptop_with_files->efiles().empty() );
        update_efiles( edevice_locs, efile_locs, copiable_efile_locs );
        REQUIRE( efile_locs.size() == 3 );
        REQUIRE( efile_locs.front() );
        do_activity( edevice_without_files, vector_laptop_with_files, efile_locs, EF_MOVE_FROM_THIS );
        REQUIRE( laptop_with_files->efiles().size() == 3 );
        REQUIRE( edevice_without_files->efiles().empty() );
    }
    SECTION( "copy onto" ) {
        add_edevices( Item_spawn_data_test_edevices_standard );
        do_activity( edevice_without_files, vector_laptop_with_files, copiable_efile_locs,
                     EF_COPY_ONTO_THIS );
        REQUIRE( laptop_with_files->efiles().size() == 3 );
        REQUIRE( edevice_without_files->efiles().size() == 1 );
        REQUIRE( edevice_without_files->efiles().front()->typeId() == itype_test_efile_copiable );
        item *copied_efile = laptop_with_files->get_item_with( copy_efile_filter );
        REQUIRE( copied_efile != nullptr );
        REQUIRE( copied_efile->typeId() == itype_test_efile_copiable );
    }
    SECTION( "copy from" ) {
        add_edevices( Item_spawn_data_test_edevices_standard );
        do_activity( laptop_with_files, vector_edevice_without_files, copiable_efile_locs,
                     EF_COPY_FROM_THIS );
        REQUIRE( laptop_with_files->efiles().size() == 3 );
        REQUIRE( edevice_without_files->efiles().size() == 1 );
        REQUIRE( edevice_without_files->efiles().front()->typeId() == itype_test_efile_copiable );
        item *copied_efile = laptop_with_files->get_item_with( copy_efile_filter );
        REQUIRE( copied_efile != nullptr );
        REQUIRE( copied_efile->typeId() == itype_test_efile_copiable );
    }
    SECTION( "wipe" ) {
        add_edevices( Item_spawn_data_test_edevices_standard );
        do_activity( edevice_without_files, vector_laptop_with_files, efile_locs, EF_WIPE );
        REQUIRE( edevice_without_files->efiles().empty() );
        REQUIRE( laptop_with_files->efiles().empty() );
    }
    SECTION( "empty parameters test" ) {
        efile_locs.clear();
        item_location empty_loc;
        std::vector<item_location> vector_empty_loc;
        efile_activity_actor act( empty_loc, vector_empty_loc, efile_locs, EF_MOVE_ONTO_THIS, COMBO_NONE );
        dummy.assign_activity( act );
        dummy.activity.do_turn( dummy );
        REQUIRE( !dummy.activity );
    }
    //power tests
    SECTION( "move to used edevice runs out of power" ) {
        add_edevices( Item_spawn_data_test_edevices_power );
        do_activity( edevice_without_files, vector_laptop_with_files, efile_locs, EF_MOVE_ONTO_THIS );
        //the operation failed and the file has not been moved
        REQUIRE( edevice_without_files->efiles().empty() );
        REQUIRE( laptop_with_files->efiles().size() == 1 );
    }
    SECTION( "move from target edevice runs out of power" ) {
        add_edevices( Item_spawn_data_test_edevices_power );
        REQUIRE( edevice_without_files );
        do_activity( laptop_with_files, vector_edevice_without_files, efile_locs, EF_MOVE_FROM_THIS );
        //the operation fails and the file has not been moved
        REQUIRE( edevice_without_files->efiles().empty() );
        REQUIRE( laptop_with_files->efiles().size() == 1 );
    }
    SECTION( "move to used edevice disappears" ) {
        add_edevices( Item_spawn_data_test_edevices_power );
        REQUIRE( edevice_without_files );
        efile_activity_actor act( edevice_without_files, vector_laptop_with_files,
                                  efile_locs, EF_MOVE_ONTO_THIS, COMBO_NONE );
        dummy.assign_activity( act );
        for( int i = 0; i < 20; i++ ) {
            calendar::turn += 1_seconds;
            dummy.activity.do_turn( dummy );
        }
        dummy.remove_item( *edevice_without_files );
        dummy.activity.do_turn( dummy );
        //the operation immediately fails
        REQUIRE( !dummy.activity );
        //the operation failed and the file has not been moved
        REQUIRE( laptop_with_files->efiles().size() == 1 );
    }
    SECTION( "move from target edevice disappears" ) {
        add_edevices( Item_spawn_data_test_edevices_power );
        efile_activity_actor act( laptop_with_files, vector_edevice_without_files, efile_locs,
                                  EF_MOVE_FROM_THIS, COMBO_NONE );
        dummy.assign_activity( act );
        for( int i = 0; i < 20; i++ ) {
            calendar::turn += 1_seconds;
            dummy.activity.do_turn( dummy );
        }
        dummy.remove_item( *edevice_without_files );
        process_activity( dummy );
        //the operation fails and the file has not been moved
        REQUIRE( laptop_with_files->efiles().size() == 1 );
    }
    SECTION( "fast move between compatible devices" ) {
        add_edevices( Item_spawn_data_test_edevices_compat );
        time_point before = calendar::turn;
        do_activity( laptop_with_files, vector_edevice_without_files, efile_locs, EF_MOVE_FROM_THIS );
        REQUIRE( calendar::turn - ( before + 1_seconds ) ==
                 810_seconds ); // 24GB / 30MB per sec + 10 second boot
    }
    SECTION( "slow move between incompatible devices" ) {
        add_edevices( Item_spawn_data_test_edevices_incompat, false );
        time_point before = calendar::turn;
        int battery_start = laptop_with_files->ammo_remaining( );
        do_activity( laptop_with_files, vector_edevice_without_files, efile_locs, EF_MOVE_FROM_THIS );
        REQUIRE( battery_start - laptop_with_files->ammo_remaining( ) ==
                 100 ); //400 minutes / 1 charge per 4 min
        REQUIRE( calendar::turn - ( before + 1_seconds ) == 24010_seconds ); // 24GB / 1MB per sec
    }
    SECTION( "fast move between incompatible devices with ETD" ) {
        add_edevices( Item_spawn_data_test_edevices_incompat );
        time_point before = calendar::turn;
        do_activity( laptop_with_files, vector_edevice_without_files, efile_locs, EF_MOVE_FROM_THIS );
        REQUIRE( calendar::turn - ( before + 1_seconds ) == 4010_seconds ); // 24GB / (12 / 2)MB per sec
    }
    SECTION( "recipe combination" ) {
        add_edevices( Item_spawn_data_test_edevices_recipes );
        std::set<recipe_id> recipes1 = laptop_with_files->get_saved_recipes();
        std::set<recipe_id> recipes2 = edevice_without_files->get_saved_recipes();
        std::set<recipe_id> combined = recipes1;
        for( const recipe_id &recipe : recipes2 ) {
            combined.insert( recipe );
        }
        REQUIRE( !recipes1.empty() );
        REQUIRE( !recipes2.empty() );
        std::vector<item_location> lwf_locs;
        for( item *efile : laptop_with_files->efiles() ) {
            lwf_locs.emplace_back( laptop_with_files, efile );
        }
        do_activity( laptop_with_files, vector_edevice_without_files, lwf_locs, EF_MOVE_FROM_THIS );
        REQUIRE( laptop_with_files->get_saved_recipes().empty() );
        REQUIRE( combined.size() == edevice_without_files->get_saved_recipes().size() );
    }
}

static liquid_dest_opt dest_opt; // Defaults to LD_NULL, which is good enough when not used.

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
    [] { return player_activity( hotwire_car_activity_actor( 1, get_avatar().pos_abs() ) ); },
    //player_activity( insert_item_activity_actor() ),
    [] { return player_activity( lockpick_activity_actor::use_item( 1, item_location(), get_avatar().pos_abs() ) ); },
    //player_activity( longsalvage_activity_actor() ),
    [] { return player_activity( meditate_activity_actor() ); },
    [] { return player_activity( migration_cancel_activity_actor() ); },
    [] { return player_activity( milk_activity_actor( 1, 3000, get_avatar().pos_abs(), dest_opt, false ) ); },
    [] { return player_activity( mop_activity_actor( 1 ) ); },
    //player_activity( move_furniture_activity_actor( p, false ) ),
    [] { return player_activity( move_items_activity_actor( {}, {}, false, tripoint_rel_ms::north ) ); },
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
    [] { return player_activity( safecracking_activity_actor( get_avatar().pos_bub() + tripoint_rel_ms::north ) ); },
    [] { return player_activity( shave_activity_actor() ); },
    //player_activity( shearing_activity_actor( north ) ),
    [] { return player_activity( stash_activity_actor() ); },
    [] { return player_activity( tent_deconstruct_activity_actor( 1, 1, get_avatar().pos_bub(), itype_tent_kit ) ); },
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

        tripoint_bub_ms zombie_pos_near = dummy.pos_bub() + tripoint( 2, 0, 0 );
        tripoint_bub_ms zombie_pos_far = dummy.pos_bub() + tripoint( 10, 0, 0 );

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

            REQUIRE( dummy.sees( m, zombie ) );

            std::map<distraction_type, std::string> dists = dummy.activity.get_distractions();

            CHECK( dists.empty() );

            THEN( "interruption by zombie moving towards dummy" ) {
                zombie.set_dest( get_map().get_abs( dummy.pos_bub() ) );
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

            m.ter_set( dummy.pos_bub() + tripoint::east, ter_t_wall );
            monster &zombie = spawn_test_monster( mon_zombie.str(), zombie_pos_near );
            update_cache( m );

            REQUIRE( !dummy.sees( m, zombie ) );

            std::map<distraction_type, std::string> dists = dummy.activity.get_distractions();

            CHECK( dists.empty() );

            THEN( "interruption by zombie moving towards dummy" ) {
                zombie.set_dest( get_map().get_abs( dummy.pos_bub() ) );
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

            m.add_field( dummy.pos_bub(), field_fd_smoke );

            std::map<distraction_type, std::string> dists = dummy.activity.get_distractions();

            CHECK( dists.size() == 1 );
            CHECK( dists.find( distraction_type::dangerous_field ) != dists.end() );
        }

        SECTION( act + " interruption by multiple sources" ) {
            cleanup( dummy );

            spawn_test_monster( mon_zombie.str(), zombie_pos_near );
            m.add_field( dummy.pos_bub(), field_fd_smoke );
            std::map<distraction_type, std::string> dists = dummy.activity.get_distractions();

            CHECK( dists.size() == 2 );
            CHECK( dists.find( distraction_type::hostile_spotted_near ) != dists.end() );
            CHECK( dists.find( distraction_type::dangerous_field ) != dists.end() );
        }
    }
}
