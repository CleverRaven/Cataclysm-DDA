#include <string>

#include "activity_actor_definitions.h"
#include "butchery.h"
#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "coordinates.h"
#include "flag.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "map.h"
#include "map_scale_constants.h"
#include "map_selector.h"
#include "monster.h"
#include "mtype.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const item_group_id
Item_spawn_data_cattle_sample_single( "cattle_sample_single" );

static const itype_id itype_debug_backpack( "debug_backpack" );
static const itype_id itype_fake_lift_light( "fake_lift_light" );
static const itype_id itype_hacksaw( "hacksaw" );
static const itype_id itype_knife_combat( "knife_combat" );
static const itype_id itype_plastic_sheet( "plastic_sheet" );
static const itype_id itype_rope_30( "rope_30" );
static const itype_id itype_scalpel( "scalpel" );

static const mtype_id mon_deer( "mon_deer" );
static const mtype_id mon_test_CBM( "mon_test_CBM" );
static const mtype_id mon_test_bovine( "mon_test_bovine" );

static const proficiency_id proficiency_prof_butchering_adv( "prof_butchering_adv" );
static const proficiency_id proficiency_prof_butchering_basic( "prof_butchering_basic" );
static const proficiency_id proficiency_prof_skinning_adv( "prof_skinning_adv" );
static const proficiency_id proficiency_prof_skinning_basic( "prof_skinning_basic" );

static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_survival( "survival" );

static const int max_iters = 1000;
static constexpr tripoint_bub_ms mon_pos( HALF_MAPSIZE_X - 1, HALF_MAPSIZE_Y, 0 );

static void butcher_mon( const mtype_id &monid, butcher_type butchery_type, int *cbm_count,
                         int *sample_count, int *other_count )
{
    item scalpel( itype_scalpel );
    Character &u = get_player_character();
    map &here = get_map();
    const tripoint_abs_ms orig_pos = u.pos_abs();
    for( int i = 0; i < max_iters; i++ ) {
        clear_character( u, true );
        u.set_skill_level( skill_firstaid, 10 );
        u.set_skill_level( skill_survival, 10 );
        u.wield( scalpel );
        monster cow( monid, mon_pos );
        const tripoint_bub_ms cow_loc = cow.pos_bub();
        cow.die( &here, nullptr );
        u.move_to( cow.pos_abs() );

        item_location loc = item_location( map_cursor( u.pos_abs() ), &*here.i_at( cow_loc ).begin() );
        butchery_data bd( loc, butchery_type );
        butchery_activity_actor act( bd );
        act.calculate_butchery_data( u, bd );
        destroy_the_carcass( bd, u );
        for( const item &it : here.i_at( cow_loc ) ) {
            if( it.is_bionic() ) {
                ( *cbm_count )++;
            } else if( item_group::group_contains_item( Item_spawn_data_cattle_sample_single,
                       it.typeId() ) ) {
                ( *sample_count )++;
            } else {
                ( *other_count )++;
            }
        }
        here.i_clear( cow_loc );
        u.move_to( orig_pos );
    }
}

TEST_CASE( "Harvest_drops_from_dissecting_corpse", "[harvest]" )
{
    SECTION( "Mutagen samples" ) {
        int sample_count = 0;
        int cbm_count = 0;
        int other_count = 0;
        butcher_mon( mon_test_bovine, butcher_type::DISSECT, &cbm_count, &sample_count, &other_count );
        CHECK( other_count > 0 );
        CHECK( cbm_count == 0 );
        CHECK( sample_count > 0 );
    }

    SECTION( "CBMs" ) {
        int sample_count = 0;
        int cbm_count = 0;
        int other_count = 0;
        butcher_mon( mon_test_CBM, butcher_type::DISSECT, &cbm_count, &sample_count, &other_count );
        CHECK( other_count > 0 );
        CHECK( cbm_count > 0 );
        CHECK( sample_count == 0 );
    }
}

static void do_butchery_timing( time_duration expected_time, butcher_type butchery_type,
                                int skill, mtype_id mon_type )
{
    Character &u = get_player_character();
    map &here = get_map();
    const tripoint_abs_ms orig_pos = u.pos_abs();
    clear_character( u, true );
    u.set_skill_level( skill_firstaid, skill );
    u.set_skill_level( skill_survival, skill );
    u.worn.wear_item( u, item( itype_debug_backpack ), false, false );
    u.i_add( item( itype_fake_lift_light ) );
    u.i_add( item( itype_hacksaw ) );
    u.i_add( item( itype_knife_combat ) );
    u.i_add( item( itype_plastic_sheet ) );
    u.i_add( item( itype_rope_30 ) );
    u.add_proficiency( proficiency_prof_butchering_basic, true, true );
    u.add_proficiency( proficiency_prof_butchering_adv, true, true );
    u.add_proficiency( proficiency_prof_skinning_adv, true, true );
    u.add_proficiency( proficiency_prof_skinning_basic, true, true );

    monster butcherable_animal( mon_type, mon_pos );
    // Just in case they have well fed or starving effects...
    butcherable_animal.clear_effects();
    butcherable_animal.mod_amount_eaten( butcherable_animal.type->stomach_size );
    const tripoint_bub_ms animal_loc = butcherable_animal.pos_bub();
    const tripoint_abs_ms animal_loc_abs = butcherable_animal.pos_abs();
    here.i_clear( animal_loc );
    butcherable_animal.die( &here, nullptr );
    const map_stack &corpse_items = here.i_at( animal_loc );
    CAPTURE( corpse_items );
    REQUIRE( corpse_items.size() == 1 );
    REQUIRE( !corpse_items.begin()->has_flag( flag_UNDERFED ) );

    u.move_to( animal_loc_abs );
    REQUIRE( u.pos_abs() == animal_loc_abs );
    item_location loc = item_location( map_cursor( u.pos_abs() ), &*here.i_at( animal_loc ).begin() );
    butchery_data bd( loc, butchery_type );
    butchery_activity_actor act( bd );
    player_activity p_act{ act };
    int turns_taken = 0;
    while( p_act.moves_left > 0 ) {
        p_act.do_turn( u );
        turns_taken++;
    }
    CHECK( p_act.moves_left <= 0 );
    const time_duration time_taken = time_duration::from_turns( turns_taken );
    CAPTURE( to_string( time_taken ) );
    CHECK( time_taken > ( expected_time * 0.9 ) );
    CHECK( time_taken < ( expected_time * 1.1 ) );
    here.i_clear( animal_loc );
    u.move_to( orig_pos );
}

TEST_CASE( "butchery_speed", "[harvest]" )
{
    do_butchery_timing( 8_hours, butcher_type::FULL, 5, mon_deer );
}
