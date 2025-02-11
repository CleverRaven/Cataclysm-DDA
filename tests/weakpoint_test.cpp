#include <cmath>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "damage.h"
#include "monster.h"
#include "mtype.h"
#include "player_helpers.h"
#include "type_id.h"
#include "weakpoint.h"

static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_bullet( "bullet" );
static const damage_type_id damage_cut( "cut" );

static const mtype_id mon_test_weakpoint_mon( "mon_test_weakpoint_mon" );
static const mtype_id mon_test_zombie_cop( "mon_test_zombie_cop" );
static const mtype_id mon_zombie( "mon_zombie" );
static const mtype_id pseudo_debug_mon( "pseudo_debug_mon" );

struct weakpoint_report_item {
    std::string weakpoint;
    int totaldamage;
    int hits;
    weakpoint_report_item( std::string wp, int dam ) : weakpoint{ std::move( wp ) }, totaldamage{ dam },
        hits{ 1 } { }
    weakpoint_report_item() : weakpoint_report_item( "", 0 ) { }
};

struct weakpoint_report {
    int hits;
    std::map<std::string, weakpoint_report_item> items;

    void Accumulate( const dealt_damage_instance &dd ) {
        if( items.find( dd.wp_hit ) == items.end() ) {
            items[dd.wp_hit] = weakpoint_report_item( dd.wp_hit, dd.total_damage() );
        } else {
            items[dd.wp_hit].hits += 1;
            items[dd.wp_hit].totaldamage += dd.total_damage();
        }
        hits += 1;
    }

    float PercHits( const std::string &wp ) {
        if( items.find( wp ) == items.end() || hits == 0 ) {
            return 0.0f;
        } else {
            return static_cast<float>( items[wp].hits ) / static_cast<float>( hits );
        }
    }

    float AveDam( const std::string &wp ) {
        if( items.find( wp ) == items.end() ) {
            return 0.0f;
        } else {
            return static_cast<float>( items[wp].totaldamage ) / static_cast<float>( items[wp].hits );
        }
    }

    void Reset() {
        hits = 0;
        items.clear();
    }
};

static weakpoint_report damage_monster( const mtype_id &target_type, const damage_instance &dam,
                                        int attacks )
{
    weakpoint_report ret{};
    for( int i = 0; i < attacks; i++ ) {
        monster target{ mtype_id( target_type ), tripoint_bub_ms::zero };
        ret.Accumulate( target.deal_damage( nullptr, bodypart_id( "torso" ), dam ) );
    }

    return ret;
}

static void reset_proficiencies( Character &dummy, const proficiency_id &prof )
{
    dummy.set_focus( 100 );
    dummy.add_proficiency( prof, true );
    dummy.lose_proficiency( prof, true );
}

TEST_CASE( "weakpoint_basic", "[monster][weakpoint]" )
{
    constexpr float epsilon = 0.225f;
    // Debug Monster has two weakpoints at 25% each, one leaves 0 armor the other 25 bullet armor, down from 100 bullet armor
    weakpoint_report wr1 = damage_monster( pseudo_debug_mon, damage_instance( damage_bullet, 100.0f,
                                           0.0f ), 1000 );
    CHECK( wr1.PercHits( "the knee" ) == Approx( 0.25f ).epsilon( epsilon ) );
    CHECK( wr1.AveDam( "the knee" ) == Approx( 75.0f ).epsilon( epsilon ) ); // 25 armor
    CHECK( wr1.PercHits( "the bugs" ) == Approx( 0.25f ).epsilon( epsilon ) );
    CHECK( wr1.AveDam( "the bugs" ) == Approx( 100.0f ).epsilon( epsilon ) ); // No armor
    CHECK( wr1.PercHits( "" ) == Approx( 0.50f ).epsilon( epsilon ) );
    CHECK( wr1.AveDam( "" ) == Approx( 0.0f ).epsilon( epsilon ) ); // Full 100 armor

    // With 25 ap, adjust no weakpoint and the knee damage
    weakpoint_report wr2 = damage_monster( pseudo_debug_mon, damage_instance( damage_bullet, 100.0f,
                                           25.0f ), 1000 );
    CHECK( wr2.PercHits( "the knee" ) == Approx( 0.25f ).epsilon( epsilon ) );
    CHECK( wr2.AveDam( "the knee" ) == Approx( 100.0f ).epsilon( epsilon ) ); // 25 armor with 25 ap
    CHECK( wr2.PercHits( "the bugs" ) == Approx( 0.25f ).epsilon( epsilon ) );
    CHECK( wr2.AveDam( "the bugs" ) == Approx( 100.0f ).epsilon( epsilon ) ); // No armor with 25 ap
    CHECK( wr2.PercHits( "" ) == Approx( 0.50f ).epsilon( epsilon ) );
    CHECK( wr2.AveDam( "" ) == Approx( 25.0f ).epsilon( epsilon ) ); // Full 100 armor with 25 ap

    // No cut armordebug_mon
    weakpoint_report wr3 = damage_monster( pseudo_debug_mon, damage_instance( damage_cut, 100.0f,
                                           0.0f ), 1000 );
    CHECK( wr3.PercHits( "the knee" ) == Approx( 0.25f ).epsilon( epsilon ) );
    CHECK( wr3.AveDam( "the knee" ) == Approx( 100.0f ).epsilon( epsilon ) );
    CHECK( wr3.PercHits( "the bugs" ) == Approx( 0.25f ).epsilon( epsilon ) );
    CHECK( wr3.AveDam( "the bugs" ) == Approx( 100.0f ).epsilon( epsilon ) );
    CHECK( wr3.PercHits( "" ) == Approx( 0.50f ).epsilon( epsilon ) );
    CHECK( wr3.AveDam( "" ) == Approx( 100.0f ).epsilon( epsilon ) );
}

TEST_CASE( "weakpoint_practice", "[monster][weakpoint]" )
{
    avatar &dummy = get_avatar();
    mtype_id wp_mon( "pseudo_weakpoint_mon" );
    proficiency_id prof( "prof_debug_weakpoint" );
    clear_character( dummy );

    reset_proficiencies( dummy, prof );
    wp_mon.obj().families.practice_dissect( dummy );
    CHECK( dummy.get_proficiency_practice( prof ) == Approx( 1.0f ).epsilon( 0.05f ) );

    reset_proficiencies( dummy, prof );
    wp_mon.obj().families.practice_kill( dummy );
    CHECK( dummy.get_proficiency_practice( prof ) == Approx( 0.0333f ).epsilon( 0.05f ) );

    reset_proficiencies( dummy, prof );
    wp_mon.obj().families.practice_hit( dummy );
    CHECK( dummy.get_proficiency_practice( prof )  == Approx( 0.00111f ).epsilon( 0.05f ) );
}

TEST_CASE( "Check_order_of_weakpoint_set_application", "[monster][weakpoint]" )
{
    bool has_wp_head = false;
    bool has_wp_eye = false;
    bool has_wp_neck = false;
    bool has_wp_arm = false;
    bool has_wp_leg = false;
    for( const weakpoint &wp : mon_test_zombie_cop->weakpoints.weakpoint_list ) {
        if( wp.id == "test_head" ) {
            has_wp_head = true;
            CHECK( wp.get_name() == "inline head" );
        } else if( wp.id == "test_eye" ) {
            has_wp_eye = true;
            CHECK( wp.get_name() == "humanoid eye" );
        } else if( wp.id == "test_neck" ) {
            has_wp_neck = true;
            CHECK( wp.get_name() == "special neck" );
        } else if( wp.id == "test_arm" ) {
            has_wp_arm = true;
            CHECK( wp.get_name() == "inline arm" );
        } else if( wp.id == "test_leg" ) {
            has_wp_leg = true;
            CHECK( wp.get_name() == "inline leg" );
        }
    }
    REQUIRE( has_wp_head );
    REQUIRE( has_wp_eye );
    REQUIRE( has_wp_neck );
    REQUIRE( has_wp_arm );
    REQUIRE( has_wp_leg );
}

TEST_CASE( "Check_damage_from_weakpoint_sets", "[monster][weakpoint]" )
{
    GIVEN( "100 bullet damage, 0 armor penetration" ) {
        weakpoint_report wr1 = damage_monster( mon_test_zombie_cop, damage_instance( damage_bullet,
                                               100.0f, 0.0f ), 100000 );
        THEN( "Verify hits and damage" ) {
            CHECK( wr1.PercHits( "inline head" ) == Approx( 0.25f ).epsilon( 0.20f ) );
            CHECK( wr1.AveDam( "inline head" ) == Approx( 100.0f ).epsilon( 0.020f ) );
            CHECK( wr1.PercHits( "inline arm" ) == Approx( 0.25f ).epsilon( 0.20f ) );
            CHECK( wr1.AveDam( "inline arm" ) == Approx( 97.0f ).epsilon( 0.020f ) );
            CHECK( wr1.PercHits( "inline leg" ) == Approx( 0.25f ).epsilon( 0.20f ) );
            CHECK( wr1.AveDam( "inline leg" ) == Approx( 97.0f ).epsilon( 0.020f ) );
            CHECK( wr1.PercHits( "special neck" ) == Approx( 0.1f ).epsilon( 0.20f ) );
            CHECK( wr1.AveDam( "special neck" ) == Approx( 100.0f ).epsilon( 0.020f ) );
            CHECK( wr1.PercHits( "humanoid eye" ) == Approx( 0.01f ).epsilon( 0.20f ) );
            CHECK( wr1.AveDam( "humanoid eye" ) == Approx( 100.0f ).epsilon( 0.020f ) );
            CHECK( wr1.PercHits( "" ) == Approx( 0.14f ).epsilon( 0.20f ) );
            CHECK( wr1.AveDam( "" ) == Approx( 94.0f ).epsilon( 0.020f ) );
        }
    }

    GIVEN( "100 bashing damage, 50 armor penetration" ) {
        weakpoint_report wr1 = damage_monster( mon_test_zombie_cop, damage_instance( damage_bash,
                                               100.0f, 50.0f ), 100000 );
        THEN( "Verify hits and damage" ) {
            CHECK( wr1.PercHits( "inline head" ) == Approx( 0.25f ).epsilon( 0.20f ) );
            CHECK( wr1.AveDam( "inline head" ) == Approx( 100.0f ).epsilon( 0.020f ) );
            CHECK( wr1.PercHits( "inline arm" ) == Approx( 0.25f ).epsilon( 0.20f ) );
            CHECK( wr1.AveDam( "inline arm" ) == Approx( 100.0f ).epsilon( 0.020f ) );
            CHECK( wr1.PercHits( "inline leg" ) == Approx( 0.25f ).epsilon( 0.20f ) );
            CHECK( wr1.AveDam( "inline leg" ) == Approx( 100.0f ).epsilon( 0.020f ) );
            CHECK( wr1.PercHits( "special neck" ) == Approx( 0.1f ).epsilon( 0.20f ) );
            CHECK( wr1.AveDam( "special neck" ) == Approx( 100.0f ).epsilon( 0.020f ) );
            CHECK( wr1.PercHits( "humanoid eye" ) == Approx( 0.01f ).epsilon( 0.20f ) );
            CHECK( wr1.AveDam( "humanoid eye" ) == Approx( 100.0f ).epsilon( 0.020f ) );
            CHECK( wr1.PercHits( "" ) == Approx( 0.14f ).epsilon( 0.20f ) );
            CHECK( wr1.AveDam( "" ) == Approx( 100.0f ).epsilon( 0.020f ) );
        }
    }
}

TEST_CASE( "Check_deferred_weakpoint_set_loading", "[monster][weakpoint]" )
{
    weakpoints wplist = mon_zombie->weakpoints;
    CHECK( wplist.weakpoint_list.size() == 2 );

    std::map<std::string, bool> wp_found {
        { "test_wp1", false },
        { "test_wp2", false }
    };

    std::list<std::string> unk;
    for( const weakpoint &wp : wplist.weakpoint_list ) {
        auto iter = wp_found.find( wp.id );
        if( iter != wp_found.end() ) {
            wp_found[wp.id] = true;
        } else {
            unk.emplace_back( wp.id );
        }
    }

    CHECK( unk.empty() );
    for( const auto &found : wp_found ) {
        CHECK( found.second == true );
    }
}

TEST_CASE( "Check_copy-from_inheritance_between_sets_and_inline_weakpoints",
           "[monster][weakpoint]" )
{
    weakpoints wplist = mon_test_weakpoint_mon->weakpoints;
    CHECK( wplist.weakpoint_list.size() == 2 );

    // { weakpoint_id, { found, coverage } }
    std::map<std::string, std::pair<bool, int>> wp_found {
        { "test_eye", { false, 0 } },
        { "test_arm", { false, 5 } }
    };

    std::list<std::string> unk;
    for( const weakpoint &wp : wplist.weakpoint_list ) {
        auto iter = wp_found.find( wp.id );
        if( iter != wp_found.end() &&
            iter->second.second == static_cast<int>( std::round( wp.coverage ) ) ) {
            wp_found[wp.id].first = true;
        } else {
            unk.emplace_back( wp.id );
        }
    }

    CHECK( unk.empty() );
    for( const auto &found : wp_found ) {
        CHECK( found.second.first == true );
    }
}
