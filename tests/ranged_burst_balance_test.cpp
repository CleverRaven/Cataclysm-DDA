#include <vector>

#include "catch/catch.hpp"
#include "npc.h"
#include "item.h"
#include "ranged.h"

static constexpr tripoint shooter_pos( 60, 60, 0 );

static void check_burst_penalty( const Character &shooter, item gun, int min, int max )
{
    if( gun.ammo_required() && !gun.ammo_sufficient() && gun.ammo_default() != "NULL" ) {
        gun.ammo_set( gun.ammo_default(), -1 );
    }
    int penalty = ranged::burst_penalty( shooter, gun, gun.gun_recoil() );
    CAPTURE( gun.tname() );
    CHECK( penalty >= min );
    CHECK( penalty <= max );
}

static void check_burst_penalty( const Character &shooter, const itype_id &gun_type,
                                 std::vector<itype_id> mods, int min, int max )
{
    item gun( gun_type );
    for( const itype_id &mod_type : mods ) {
        item mod( mod_type );
        CAPTURE( gun_type );
        CAPTURE( mod_type );
        REQUIRE( gun.is_gunmod_compatible( mod ).success() );
        gun.put_in( mod );
    }
    check_burst_penalty( shooter, gun, min, max );
}

TEST_CASE( "unskilled_burst", "[ranged] [balance]" )
{
    const std::vector<itype_id> modset = {};
    standard_npc shooter( "Shooter", shooter_pos, {}, 0, 8, 8, 8, 8 );
    // .22 SMG - the lightest burst (from a firearm) expected to be in the game
    check_burst_penalty( shooter, "american_180", modset, 0, 50 );
    // 9mm SMG - should be manageable
    check_burst_penalty( shooter, "calico", modset, 0, 150 );
    // .223 machine gun - should have lower penalty than a rifle of the same caliber
    check_burst_penalty( shooter, "m249", modset, 100, 300 );
    // .223 rifle
    check_burst_penalty( shooter, "m4a1", modset, 300, 400 );
    // 7.62 rifle
    check_burst_penalty( shooter, "ak47", modset, 400, 500 );
    // .50 machine gun - heaviest expected burst fire
    check_burst_penalty( shooter, "m2browning", modset, 500, 1500 );
}

TEST_CASE( "average_burst", "[ranged] [balance]" )
{
    const std::vector<itype_id> modset = {"adjustable_stock"};
    standard_npc shooter( "Shooter", shooter_pos, {}, 5, 10, 8, 8, 8 );
    check_burst_penalty( shooter, "american_180", modset, 0, 50 );
    check_burst_penalty( shooter, "calico", modset, 0, 150 );
    check_burst_penalty( shooter, "m249", modset, 100, 300 );
    check_burst_penalty( shooter, "m4a1", modset, 300, 400 );
    check_burst_penalty( shooter, "ak47", modset, 400, 500 );
    check_burst_penalty( shooter, "m2browning", {}, 500, 1500 );
}

TEST_CASE( "great_burst", "[ranged] [balance]" )
{
    const std::vector<itype_id> modset = {"recoil_stock", "suppressor"};
    standard_npc shooter( "Shooter", shooter_pos, {}, 10, 14, 8, 8, 8 );
    check_burst_penalty( shooter, "american_180", modset, 0, 50 );
    check_burst_penalty( shooter, "calico", modset, 0, 150 );
    check_burst_penalty( shooter, "m249", modset, 100, 300 );
    check_burst_penalty( shooter, "m4a1", modset, 300, 400 );
    check_burst_penalty( shooter, "ak47", modset, 400, 500 );
    check_burst_penalty( shooter, "m2browning", {"suppressor"}, 500, 1500 );
}

// Stats as if good start + mutations + bionics (hydraulics on)
TEST_CASE( "god_burst", "[ranged] [balance]" )
{
    const std::vector<itype_id> modset = {"recoil_stock", "muzzle_brake_mod", "stabilizer"};
    standard_npc shooter( "Shooter", shooter_pos, {}, 10, 40, 14, 14, 14 );
    check_burst_penalty( shooter, "american_180", modset, 0, 50 );
    check_burst_penalty( shooter, "calico", modset, 0, 150 );
    check_burst_penalty( shooter, "m249", modset, 100, 300 );
    check_burst_penalty( shooter, "m4a1", modset, 300, 400 );
    check_burst_penalty( shooter, "ak47", {"recoil_stock", "muzzle_brake_mod"}, 400, 500 );
    check_burst_penalty( shooter, "m2browning", {"muzzle_brake_mod"}, 500, 1500 );
}
