#include <string>
#include <vector>

#include "activity_handlers.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "map.h"
#include "map_scale_constants.h"
#include "map_selector.h"
#include "monster.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const activity_id ACT_DISSECT( "ACT_DISSECT" );

static const item_group_id
Item_spawn_data_cattle_sample_single( "cattle_sample_single" );

static const itype_id itype_scalpel( "scalpel" );

static const mtype_id mon_test_CBM( "mon_test_CBM" );
static const mtype_id mon_test_bovine( "mon_test_bovine" );

static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_survival( "survival" );

static const int max_iters = 1000;
static constexpr tripoint_bub_ms mon_pos( HALF_MAPSIZE_X - 1, HALF_MAPSIZE_Y, 0 );

static void butcher_mon( const mtype_id &monid, const activity_id &actid, int *cbm_count,
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
        player_activity act( actid, 0, true );
        act.targets.emplace_back( map_cursor( u.pos_abs() ), &*here.i_at( cow_loc ).begin() );
        while( !act.is_null() ) {
            activity_handlers::butcher_finish( &act, &u );
        }
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
        butcher_mon( mon_test_bovine, ACT_DISSECT, &cbm_count, &sample_count, &other_count );
        CHECK( other_count > 0 );
        CHECK( cbm_count == 0 );
        CHECK( sample_count > 0 );
    }

    SECTION( "CBMs" ) {
        int sample_count = 0;
        int cbm_count = 0;
        int other_count = 0;
        butcher_mon( mon_test_CBM, ACT_DISSECT, &cbm_count, &sample_count, &other_count );
        CHECK( other_count > 0 );
        CHECK( cbm_count > 0 );
        CHECK( sample_count == 0 );
    }
}
