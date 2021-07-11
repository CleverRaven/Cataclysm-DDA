#include "catch/catch.hpp"
#include "map_helpers.h"
#include "player_helpers.h"
#include "activity_scheduling_helper.h"

#include "map.h"
#include "character.h"
#include "monster.h"
#include "avatar.h"
#include "calendar.h"
#include "iuse_actor.h"
#include "game.h"
#include "flag.h"
#include "point.h"
#include "activity_actor_definitions.h"

static const activity_id ACT_NULL( "ACT_NULL" );
static const activity_id ACT_SHEARING( "ACT_SHEARING" );

static const efftype_id effect_pet( "pet" );
static const efftype_id effect_tied( "tied" );

static const itype_id itype_test_battery_disposable( "test_battery_disposable" );
static const itype_id itype_test_shears( "test_shears" );
static const itype_id itype_test_shears_off( "test_shears_off" );

static const mtype_id mon_test_shearable( "mon_test_shearable" );
static const mtype_id mon_test_non_shearable( "mon_test_non_shearable" );

static const quality_id qual_SHEAR( "SHEAR" );

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

                CHECK( dummy.weapon.ammo_remaining() == 0 );
                REQUIRE( dummy.weapon.typeId().str() == itype_test_shears_off.str() );

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
