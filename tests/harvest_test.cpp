#include "activity_handlers.h"
#include "cata_catch.h"
#include "harvest.h"
#include "item_group.h"
#include "itype.h"
#include "map.h"
#include "monster.h"
#include "mtype.h"
#include "player_helpers.h"

static const activity_id ACT_BUTCHER_FULL( "ACT_BUTCHER_FULL" );
static const activity_id ACT_DISSECT( "ACT_DISSECT" );

static const furn_str_id furn_butcher_rack( "f_butcher_rack" );

static const item_group_id Item_spawn_data_bovine_sample( "bovine_sample" );

static const itype_id itype_test_butchering_kit( "test_butchering_kit" );
static const itype_id itype_test_hacksaw_butcher( "test_hacksaw_butcher" );
static const itype_id itype_test_rope( "test_rope" );
static const itype_id itype_test_scalpel( "test_scalpel" );
static const itype_id itype_test_scissors( "test_scissors" );

static const mtype_id mon_test_CBM( "mon_test_CBM" );
static const mtype_id mon_test_bovine( "mon_test_bovine" );

static const quality_id qual_BUTCHER( "BUTCHER" );
static const quality_id qual_CUT_FINE( "CUT_FINE" );

static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_survival( "survival" );

static constexpr tripoint mon_pos( HALF_MAPSIZE_X - 1, HALF_MAPSIZE_Y, 0 );
static constexpr tripoint rack_pos( HALF_MAPSIZE_X - 4, HALF_MAPSIZE_Y, 0 );

static void butcher_mon( int iterations, const skill_id &sk, int sk_lvl, item &tool,
                         const mtype_id &monid, const activity_id &actid, int *cbm_count, int *sample_count,
                         int *yield_count, int *refuse_count )
{
    item rope( itype_test_rope );
    item hacksaw( itype_test_hacksaw_butcher );
    Character &u = get_player_character();
    map &here = get_map();
    here.furn_set( rack_pos, furn_butcher_rack );
    const tripoint_abs_ms orig_pos = u.get_location();
    for( int i = 0; i < iterations; i++ ) {
        clear_character( u, true );
        u.set_skill_level( sk, sk_lvl );
        u.wield( tool );
        monster cow( monid, mon_pos );
        const tripoint cow_loc = cow.pos();
        cow.die( nullptr );
        u.move_to( cow.get_location() );
        if( actid != ACT_DISSECT ) {
            u.i_add_or_drop( rope );
            u.i_add_or_drop( hacksaw );
        }
        player_activity act( actid, 0, true );
        act.targets.emplace_back( map_cursor( u.pos() ), &*here.i_at( cow_loc ).begin() );
        while( !act.is_null() ) {
            activity_handlers::butcher_finish( &act, &u );
        }
        for( const tripoint &p : here.points_in_radius( cow_loc, 2 ) ) {
            for( const item &it : here.i_at( p ) ) {
                if( it.is_bionic() ) {
                    ( *cbm_count ) += it.count_by_charges() ? it.charges : 1;
                } else if( item_group::group_contains_item( Item_spawn_data_bovine_sample, it.typeId() ) ) {
                    ( *sample_count ) += it.count_by_charges() ? it.charges : 1;
                } else if( it.typeId() == cow.type->harvest->leftovers ) {
                    ( *refuse_count ) += it.count_by_charges() ? it.charges : 1;
                } else {
                    ( *yield_count ) += it.count_by_charges() ? it.charges : 1;
                }
            }
            here.i_clear( p );
        }
        u.move_to( orig_pos );
    }
}

TEST_CASE( "Harvest drops from dissecting corpse", "[harvest]" )
{
    static const int max_iters = 1000;

    SECTION( "Mutagen samples" ) {
        int sample_count = 0;
        int cbm_count = 0;
        int yield_count = 0;
        int refuse_count = 0;
        item scalpel( itype_test_scalpel );
        REQUIRE( scalpel.get_quality( qual_CUT_FINE ) == 3 );
        butcher_mon( max_iters, skill_firstaid, 10, scalpel, mon_test_bovine, ACT_DISSECT, &cbm_count,
                     &sample_count, &yield_count, &refuse_count );
        CHECK( refuse_count > 0 );
        CHECK( cbm_count == 0 );
        CHECK( sample_count > 0 );
    }

    SECTION( "CBMs" ) {
        int sample_count = 0;
        int cbm_count = 0;
        int yield_count = 0;
        int refuse_count = 0;
        item scalpel( itype_test_scalpel );
        REQUIRE( scalpel.get_quality( qual_CUT_FINE ) == 3 );
        butcher_mon( max_iters, skill_firstaid, 10, scalpel, mon_test_CBM, ACT_DISSECT, &cbm_count,
                     &sample_count, &yield_count, &refuse_count );
        CHECK( refuse_count > 0 );
        CHECK( cbm_count > 0 );
        CHECK( sample_count == 0 );
    }
}

TEST_CASE( "Harvest drops from quick butchery", "[harvest]" )
{
    static const int max_iters = 1000;
    // Values based on maximum possible yields:refuse ratio
    static const double max_yields = 324.0 * max_iters;
    static const double max_refuse = 36.0 * max_iters;

    SECTION( "0 survival, 3 butchering quality" ) {
        int sample_count = 0;
        int cbm_count = 0;
        int yield_count = 0;
        int refuse_count = 0;
        item scissors( itype_test_scissors );
        REQUIRE( scissors.get_quality( qual_BUTCHER ) == 3 );
        butcher_mon( max_iters, skill_survival, 0, scissors, mon_test_bovine, ACT_BUTCHER_FULL, &cbm_count,
                     &sample_count, &yield_count, &refuse_count );
        CHECK( yield_count / max_yields == Approx( 0.2 ).epsilon( 0.1 ) );
        CHECK( refuse_count / max_refuse == Approx( 6.0 ).epsilon( 0.5 ) );
        CHECK( cbm_count == 0 );
        CHECK( sample_count == 0 );
    }

    SECTION( "5 survival, 3 butchering quality" ) {
        int sample_count = 0;
        int cbm_count = 0;
        int yield_count = 0;
        int refuse_count = 0;
        item scissors( itype_test_scissors );
        REQUIRE( scissors.get_quality( qual_BUTCHER ) == 3 );
        butcher_mon( max_iters, skill_survival, 5, scissors, mon_test_bovine, ACT_BUTCHER_FULL, &cbm_count,
                     &sample_count, &yield_count, &refuse_count );
        CHECK( yield_count / max_yields == Approx( 0.75 ).epsilon( 0.1 ) );
        CHECK( refuse_count / max_refuse == Approx( 2.2 ).epsilon( 0.5 ) );
        CHECK( cbm_count == 0 );
        CHECK( sample_count == 0 );
    }

    SECTION( "10 survival, 3 butchering quality" ) {
        int sample_count = 0;
        int cbm_count = 0;
        int yield_count = 0;
        int refuse_count = 0;
        item scissors( itype_test_scissors );
        REQUIRE( scissors.get_quality( qual_BUTCHER ) == 3 );
        butcher_mon( max_iters, skill_survival, 10, scissors, mon_test_bovine, ACT_BUTCHER_FULL, &cbm_count,
                     &sample_count, &yield_count, &refuse_count );
        CHECK( yield_count / max_yields == Approx( 1.0 ).epsilon( 0.2 ) );
        CHECK( refuse_count / max_refuse == Approx( 1.0 ).epsilon( 0.2 ) );
        CHECK( cbm_count == 0 );
        CHECK( sample_count == 0 );
    }

    SECTION( "0 survival, 37 butchering quality" ) {
        int sample_count = 0;
        int cbm_count = 0;
        int yield_count = 0;
        int refuse_count = 0;
        item butkit( itype_test_butchering_kit );
        REQUIRE( butkit.get_quality( qual_BUTCHER ) == 37 );
        butcher_mon( max_iters, skill_survival, 0, butkit, mon_test_bovine, ACT_BUTCHER_FULL, &cbm_count,
                     &sample_count, &yield_count, &refuse_count );
        CHECK( yield_count / max_yields == Approx( 0.4 ).epsilon( 0.1 ) );
        CHECK( refuse_count / max_refuse == Approx( 4.5 ).epsilon( 0.5 ) );
        CHECK( cbm_count == 0 );
        CHECK( sample_count == 0 );
    }

    SECTION( "5 survival, 37 butchering quality" ) {
        int sample_count = 0;
        int cbm_count = 0;
        int yield_count = 0;
        int refuse_count = 0;
        item butkit( itype_test_butchering_kit );
        REQUIRE( butkit.get_quality( qual_BUTCHER ) == 37 );
        butcher_mon( max_iters, skill_survival, 5, butkit, mon_test_bovine, ACT_BUTCHER_FULL, &cbm_count,
                     &sample_count, &yield_count, &refuse_count );
        CHECK( yield_count / max_yields == Approx( 0.8 ).epsilon( 0.1 ) );
        CHECK( refuse_count / max_refuse == Approx( 2.0 ).epsilon( 0.2 ) );
        CHECK( cbm_count == 0 );
        CHECK( sample_count == 0 );
    }

    SECTION( "10 survival, 37 butchering quality" ) {
        int sample_count = 0;
        int cbm_count = 0;
        int yield_count = 0;
        int refuse_count = 0;
        item butkit( itype_test_butchering_kit );
        REQUIRE( butkit.get_quality( qual_BUTCHER ) == 37 );
        butcher_mon( max_iters, skill_survival, 10, butkit, mon_test_bovine, ACT_BUTCHER_FULL, &cbm_count,
                     &sample_count, &yield_count, &refuse_count );
        CHECK( yield_count / max_yields == Approx( 1.0 ).epsilon( 0.2 ) );
        CHECK( refuse_count / max_refuse == Approx( 1.0 ).epsilon( 0.2 ) );
        CHECK( cbm_count == 0 );
        CHECK( sample_count == 0 );
    }
}