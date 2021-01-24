#include <cstddef>
#include <sstream>
#include <string>

#include "catch/catch.hpp"
#include "creature.h"
#include "game_constants.h"
#include "item.h"
#include "item_factory.h"
#include "monattack.h"
#include "monster.h"
#include "npc.h"
#include "player.h"
#include "point.h"
#include "string_formatter.h"
#include "type_id.h"

static constexpr tripoint dude_pos( HALF_MAPSIZE_X, HALF_MAPSIZE_Y, 0 );

static std::vector<const itype *> find_weapons()
{
    std::vector<const itype *> result;
    for( const itype *it : item_controller->all() ) {
        if( it->melee[DT_BASH] + it->melee[DT_CUT] + it->melee[DT_STAB] >= 10 ) {
            result.push_back( it );
        }
    }

    return result;
}

static void print_stats( const player &p, const std::vector<const itype *> &weapons,
                         const monster &m )
{
    std::vector<std::pair<std::string, double>> weapon_stats;
    for( const itype *w : weapons ) {
        item wp( w );
        weapon_stats.emplace_back( wp.tname( 1, false ), wp.effective_dps( p, m ) );
    }
    std::sort( weapon_stats.begin(), weapon_stats.end(), []( const auto & l, const auto & r ) {
        return l.second < r.second;
    } );
    for( const auto &pr : weapon_stats ) {
        cata_printf( "%-30s : %.1f\n", pr.first.c_str(), pr.second );
    }
}

TEST_CASE( "Weak character using melee weapons against a brute", "[.][melee][slow]" )
{
    monster zed( mtype_id( "mon_zombie_brute" ) );
    auto weapons = find_weapons();

    SECTION( "8/8/8/8, no skills" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        print_stats( dude, weapons, zed );
    }
}

TEST_CASE( "Average character using melee weapons against a hulk", "[.][melee][slow]" )
{
    monster zed( mtype_id( "mon_zombie_hulk" ) );
    auto weapons = find_weapons();

    SECTION( "12/10/8/8, 3 in all skills" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 3, 12, 10, 8, 8 );
        print_stats( dude, weapons, zed );
    }
}

TEST_CASE( "Strong character using melee weapons against a kevlar zombie", "[.][melee][slow]" )
{
    monster zed( mtype_id( "mon_zombie_kevlar_1" ) );
    auto weapons = find_weapons();

    SECTION( "12/10/8/8, 3 in all skills" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 3, 12, 10, 8, 8 );
        print_stats( dude, weapons, zed );
    }
}
