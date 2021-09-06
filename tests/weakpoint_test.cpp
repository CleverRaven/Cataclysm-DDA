#include <string>

#include "cata_catch.h"
#include "damage.h"
#include "game_constants.h"
#include "monster.h"
#include "point.h"
#include "type_id.h"

struct weakpoint_report_item {
    std::string weakpoint;
    int totaldamage;
    int hits;
    weakpoint_report_item( std::string wp, int dam ) : weakpoint{ wp }, totaldamage{ dam }, hits{ 1 } { }
    weakpoint_report_item() : weakpoint_report_item( "", 0 ) { }
};

struct weakpoint_report {
    int hits;
    std::map<std::string, weakpoint_report_item> items;

    void Accumulate( dealt_damage_instance dd ) {
        if( items.find( dd.wp_hit ) == items.end() ) {
            items[dd.wp_hit] = weakpoint_report_item( dd.wp_hit, dd.total_damage() );
        } else {
            items[dd.wp_hit].hits += 1;
            items[dd.wp_hit].totaldamage += dd.total_damage();
        }
        hits += 1;
    }

    float PercHits( std::string wp ) {
        if( items.find( wp ) == items.end() || hits == 0 ) {
            return 0.0f;
        } else {
            return ( float )items[wp].hits / ( float )hits;
        }
    }

    float AveDam( std::string wp ) {
        if( items.find( wp ) == items.end() ) {
            return 0.0f;
        } else {
            return ( float )items[wp].totaldamage / ( float )items[wp].hits;
        }
    }

    void Reset() {
        hits = 0;
        items.clear();
    }
};

static weakpoint_report damage_monster( const std::string &target_type, const damage_instance &dam,
                                        int attacks )
{
    weakpoint_report ret{};
    for( int i = 0; i < attacks; i++ ) {
        monster target{ mtype_id( target_type ), tripoint_zero };
        ret.Accumulate( target.deal_damage( NULL, bodypart_id( "torso" ), dam ) );
    }

    return ret;
}

TEST_CASE( "monster_weakpoint", "[monster]" )
{
    // Debug Monster has two weakpoints at 25% each, one leaves 0 armor the other 25 bullet armor, down from 100 bullet armor
    weakpoint_report wr1 = damage_monster( "debug_mon", damage_instance( damage_type::BULLET, 100.0f,
                                           0.0f ), 1000 );
    CHECK( wr1.PercHits( "the knee" ) == Approx( 0.25f ).epsilon( 0.15f ) );
    CHECK( wr1.AveDam( "the knee" ) == Approx( 75.0f ).epsilon( 0.15f ) ); // 25 armor
    CHECK( wr1.PercHits( "the bugs" ) == Approx( 0.25f ).epsilon( 0.15f ) );
    CHECK( wr1.AveDam( "the bugs" ) == Approx( 100.0f ).epsilon( 0.15f ) ); // No armor
    CHECK( wr1.PercHits( "" ) == Approx( 0.50f ).epsilon( 0.15f ) );
    CHECK( wr1.AveDam( "" ) == Approx( 0.0f ).epsilon( 0.15f ) ); // Full 100 armor

    // With 25 ap, adjust no weakpoint and the knee damage
    weakpoint_report wr2 = damage_monster( "debug_mon", damage_instance( damage_type::BULLET, 100.0f,
                                           25.0f ), 1000 );
    CHECK( wr2.PercHits( "the knee" ) == Approx( 0.25f ).epsilon( 0.15f ) );
    CHECK( wr2.AveDam( "the knee" ) == Approx( 100.0f ).epsilon( 0.15f ) ); // 25 armor with 25 ap
    CHECK( wr2.PercHits( "the bugs" ) == Approx( 0.25f ).epsilon( 0.15f ) );
    CHECK( wr2.AveDam( "the bugs" ) == Approx( 100.0f ).epsilon( 0.15f ) ); // No armor with 25 ap
    CHECK( wr2.PercHits( "" ) == Approx( 0.50f ).epsilon( 0.15f ) );
    CHECK( wr2.AveDam( "" ) == Approx( 25.0f ).epsilon( 0.15f ) ); // Full 100 armor with 25 ap

    // No cut armor
    weakpoint_report wr3 = damage_monster( "debug_mon", damage_instance( damage_type::CUT, 100.0f,
                                           0.0f ), 1000 );
    CHECK( wr3.PercHits( "the knee" ) == Approx( 0.25f ).epsilon( 0.15f ) );
    CHECK( wr3.AveDam( "the knee" ) == Approx( 100.0f ).epsilon( 0.15f ) );
    CHECK( wr3.PercHits( "the bugs" ) == Approx( 0.25f ).epsilon( 0.15f ) );
    CHECK( wr3.AveDam( "the bugs" ) == Approx( 100.0f ).epsilon( 0.15f ) );
    CHECK( wr3.PercHits( "" ) == Approx( 0.50f ).epsilon( 0.15f ) );
    CHECK( wr3.AveDam( "" ) == Approx( 100.0f ).epsilon( 0.15f ) );
}
