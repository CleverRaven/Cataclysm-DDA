#include "catch/catch.hpp"
#include "game.h"
#include "player.h"
#include "activity_handlers.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "overmapbuffer.h"
#include "iexamine.h"
#include "test_statistics.h"

struct forage_test_data {
    statistics<int> calories;
    statistics<bool> forests;
};

// returns the calories a player would harvest from an underbrush
int forage_calories( player &p, items_location loc )
{
    int calories = 0;
    const std::vector<item *> drops = find_forage_items( &p, loc );
    for( const auto &item : drops ) {
        if( item->is_comestible() ) {
            calories += item->type->comestible->get_calories() * item->charges;
        }
    }
    i_clear_adjacent( p.pos() );
    return calories;
}

// gets all harvest information from an overmap tile
std::map<harvest_id, int> get_map_terrain_harvest( const tripoint &p )
{
    std::map<harvest_id, int> harvests;
    tripoint t = p;
    for( int x = 0; x < SEEX * 2; ++x ) {
        for( int y = 0; y < SEEY * 2; ++y ) {
            t.x = p.x + x;
            t.y = p.y + y;
            harvests[ g->m.get_harvest( t ) ]++;
        }
    }
    return harvests;
}

// gives a vector of tripoints with harvestable terrain or furniture
std::vector<tripoint> get_harvest_pos( const tripoint &p )
{
    std::vector<tripoint> harvests;
    tripoint t = p;
    for( int x = 0; x < SEEX * 2; ++x ) {
        for( int y = 0; y < SEEY * 2; ++y ) {
            t.x = p.x + x;
            t.y = p.y + y;
            if( g->m.get_harvest( t ) != harvest_id( "null" ) ) {
                harvests.push_back( t );
            }
        }
    }
    return harvests;
}

/**
* gives a vector of tripoints for underbrush.
* since underbrush is not a "harvestable terrain or furniture" it needs a seperate function
* @TODO: Calculate time spent using these tripoints
*/
std::vector<tripoint> get_underbrush_pos( const tripoint &p )
{
    std::vector<tripoint> underbrush;
    tripoint t = p;
    for( int x = 0; x < SEEX * 2; ++x ) {
        for( int y = 0; y < SEEY * 2; ++y ) {
            t.x = p.x + x;
            t.y = p.y + y;
            if( g->m.tername( t ) == "underbrush" ) {
                underbrush.push_back( t );
            }
        }
    }
    return underbrush;
}

// gets calories from a single harvest action
int harvest_calories( const player &p, const tripoint &hv_p )
{
    std::vector<item> harvested = harvest_terrain( p, hv_p );
    int calories = 0;
    for( const item &it : harvested ) {
        if( it.is_comestible() ) {
            calories += it.type->comestible->get_calories();
        } else {
            std::string hv_it = it.typeId().c_str();
            /**
             * The numbers below are hardcoded because currently
             * the items they represent are not edible comestibles
             * directly, rather they need to be cooked over a fire.
             */
            if( hv_it == "pinecone" ) {
                calories += 202;
            } else if( hv_it == "chestnut" ) {
                calories += 288;
            } else if( hv_it == "hazelnut" ) {
                calories += 193;
            } else if( hv_it == "hickory_nut" ) {
                calories += 193;
            } else if( hv_it == "walnut" ) {
                calories += 193;
            }
        }
    }
    return calories;
}

// gets calories for harvest actions from a vector of points
int harvest_calories( const player &p, const std::vector<tripoint> &hv_p )
{
    int calories = 0;
    for( const tripoint tp : hv_p ) {
        calories += harvest_calories( p, tp );
    }
    return calories;
}

// returns total calories you can find in one overmap tile forest_thick
int calories_in_forest( player &p, items_location loc, bool print = false )
{
    generate_forest_OMT( p.pos() );
    const int underbrush = get_underbrush_pos( p.pos() ).size();
    int calories = 0;
    for( int i = 0; i < underbrush; i++ ) {
        calories += forage_calories( p, loc );
    }
    calories += harvest_calories( p, get_harvest_pos( p.pos() ) );
    return calories;
}

// returns number of forests required to reach RDAKCAL
bool run_forage_test( const int min_forests, const int max_forests, const int season = 0,
                      const int survival = 0, const int perception = 8, bool print = false )
{
    forage_test_data data;
    calendar::turn += to_turns<int>( calendar::season_length() ) * season;
    player &dummy = g->u;
    items_location loc;
    switch( season ) {
        case 0:
            loc = "forage_spring";
            break;
        case 1:
            loc = "forage_summer";
            break;
        case 2:
            loc = "forage_autumn";
            break;
        case 3:
            loc = "forage_winter";
            break;
    }
    dummy.set_skill_level( skill_id( "skill_survival" ), survival );
    dummy.per_cur = perception;
    int calories_total = 0;
    int count = 0;
    // 2500 kCal per day in CDDA
    while( calories_total < 2500 && count <= max_forests ) {
        const int temp = calories_in_forest( dummy, loc );
        data.calories.add( temp );
        calories_total += temp;
        count++;
    }
    if( print ) {
        printf( "\n" );
        printf( "%s\n", calendar::name_season( season_of_year( calendar::turn ) ) );
        printf( "Survival: %i, Perception: %i\n", dummy.get_skill_level( skill_id( "skill_survival" ) ),
                dummy.per_cur );
        printf( "Average Calories in %i Forests: %f\n", data.calories.n(), data.calories.avg() );
        printf( "Min Calories: %i, Max Calories: %i, Std Deviation: %f\n", data.calories.min(),
                data.calories.max(),
                data.calories.stddev() );
        printf( "\n" );
    }
    if( count >= min_forests && count <= max_forests ) {
        return true;
    } else {
        return false;
    }
}
