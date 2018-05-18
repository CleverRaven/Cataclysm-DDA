#include "catch/catch.hpp"

#include "enums.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "line.h"
#include "map.h"
#include "monster.h"

#include "map_helpers.h"
#include "test_statistics.h"

void check_lethality( std::string explosive_id, int range, float lethality )
{
    int num_survivors = 0;
    int num_subjects = 0;
    int num_wounded = 0;
    statistics lethality_ratios;
    std::stringstream survivor_stats;
    int total_hp = 0;
    int average_hp = 0;
    float error = 0.0;
    float lethality_ratio = 0.0;
    do {
        // Clear map
        clear_map();
        // Spawn some monsters in a circle.
        tripoint origin( 30, 30, 0 );
        for( const tripoint monster_position : closest_tripoints_first( range, origin ) ) {
            if( rl_dist( monster_position, origin ) != range ) {
                continue;
            }
            num_subjects++;
            spawn_test_monster( "mon_zombie", monster_position );
        }
        // Set off an explosion
        item grenade( explosive_id );
        grenade.charges = 0;
        grenade.type->invoke( g->u, grenade, origin );
        // see how many monsters survive
        std::vector<Creature *> survivors = g->get_creatures_if( []( const Creature & critter ) {
            return critter.is_monster();
        } );
        num_survivors += survivors.size();
        for( Creature *survivor : survivors ) {
            survivor_stats << survivor->pos() << " " << survivor->get_hp() << ", ";
            num_wounded += ( survivor->get_hp() < survivor->get_hp_max() ) ? 1 : 0;
            total_hp += survivor->get_hp();
        }
        if( num_survivors > 0 ) {
            survivor_stats << std::endl;
            average_hp = total_hp / num_survivors;
        }
        float survivor_ratio = ( float )num_survivors / num_subjects;
        lethality_ratio = 1.0 - survivor_ratio;
        lethality_ratios.add( lethality_ratio );
        error = lethality_ratios.adj_wald_error();
    } while( lethality_ratios.n() < 5 ||
             ( lethality_ratios.avg() + error > lethality &&
               lethality_ratios.avg() - error < lethality ) );
    INFO( "samples " << lethality_ratios.n() );
    INFO( explosive_id );
    INFO( "range " << range );
    INFO( num_survivors << " survivors out of " << num_subjects << " targets." );
    INFO( survivor_stats.str() );
    INFO( "Wounded survivors: " << num_wounded );
    INFO( "average hp of survivors: " << average_hp );
    CHECK( lethality_ratio == Approx( lethality ).epsilon( 0.05 ) );
}

TEST_CASE( "grenade_lethal_at_5m", "[grenade],[explosion],[balance]" )
{
    check_lethality( "grenade_act", 5, 0.95 );
    check_lethality( "grenade_act", 15, 0.5 );
}
