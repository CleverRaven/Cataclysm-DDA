#include "cata_utility.h"

#include "options.h"
#include "material.h"
#include "enums.h"
#include "item.h"
#include "creature.h"
#include "translations.h"

#include <algorithm>

bool isBetween( int test, int down, int up )
{
    return test > down && test < up;
}

bool lcmatch( const std::string &str, const std::string &findstr ); // ui.cpp
bool list_items_match( const item *item, std::string sPattern )
{
    size_t iPos;
    bool hasExclude = false;

    if( sPattern.find( "-" ) != std::string::npos ) {
        hasExclude = true;
    }

    do {
        iPos = sPattern.find( "," );
        std::string pat = ( iPos == std::string::npos ) ? sPattern : sPattern.substr( 0, iPos );
        bool exclude = false;
        if( pat.substr( 0, 1 ) == "-" ) {
            exclude = true;
            pat = pat.substr( 1, pat.size() - 1 );
        } else if( hasExclude ) {
            hasExclude = false; //If there are non exclusive items to filter, we flip this back to false.
        }

        std::string namepat = pat;
        std::transform( namepat.begin(), namepat.end(), namepat.begin(), tolower );
        if( lcmatch( item->tname(), namepat ) ) {
            return !exclude;
        }

        if( pat.find( "{", 0 ) != std::string::npos ) {
            std::string adv_pat_type = pat.substr( 1, pat.find( ":" ) - 1 );
            std::string adv_pat_search = pat.substr( pat.find( ":" ) + 1,
                                         ( pat.find( "}" ) - pat.find( ":" ) ) - 1 );
            std::transform( adv_pat_search.begin(), adv_pat_search.end(), adv_pat_search.begin(), tolower );
            if( adv_pat_type == "c" && lcmatch( item->get_category().name, adv_pat_search ) ) {
                return !exclude;
            } else if( adv_pat_type == "m" ) {
                for( auto material : item->made_of_types() ) {
                    if( lcmatch( material->name(), adv_pat_search ) ) {
                        return !exclude;
                    }
                }
            } else if( adv_pat_type == "dgt" && item->damage > atoi( adv_pat_search.c_str() ) ) {
                return !exclude;
            } else if( adv_pat_type == "dlt" && item->damage < atoi( adv_pat_search.c_str() ) ) {
                return !exclude;
            }
        }

        if( iPos != std::string::npos ) {
            sPattern = sPattern.substr( iPos + 1, sPattern.size() );
        }

    } while( iPos != std::string::npos );

    return hasExclude;
}

std::vector<map_item_stack> filter_item_stacks( std::vector<map_item_stack> stack,
        std::string filter )
{
    std::vector<map_item_stack> ret;

    std::string sFilterTemp = filter;

    for( auto &elem : stack ) {
        if( sFilterTemp == "" || list_items_match( elem.example, sFilterTemp ) ) {
            ret.push_back( elem );
        }
    }
    return ret;
}

//returns the first non priority items.
int list_filter_high_priority( std::vector<map_item_stack> &stack, std::string priorities )
{
    //TODO:optimize if necessary
    std::vector<map_item_stack> tempstack; // temp
    for( auto it = stack.begin(); it != stack.end(); ) {
        if( priorities == "" || !list_items_match( it->example, priorities ) ) {
            tempstack.push_back( *it );
            it = stack.erase( it );
        } else {
            it++;
        }
    }

    int id = stack.size();
    for( auto &elem : tempstack ) {
        stack.push_back( elem );
    }
    return id;
}

int list_filter_low_priority( std::vector<map_item_stack> &stack, int start,
                              std::string priorities )
{
    //TODO:optimize if necessary
    std::vector<map_item_stack> tempstack; // temp
    for( auto it = stack.begin() + start; it != stack.end(); ) {
        if( priorities != "" && list_items_match( it->example, priorities ) ) {
            tempstack.push_back( *it );
            it = stack.erase( it );
        } else {
            it++;
        }
    }

    int id = stack.size();
    for( auto &elem : tempstack ) {
        stack.push_back( elem );
    }
    return id;
}

/* used by calculate_drop_cost */
bool compare_items_by_lesser_volume( const item &a, const item &b )
{
    return a.volume() < b.volume();
}

// calculate the time (in player::moves) it takes to drop the
// items in dropped and dropped_worn.
// Items in dropped come from the main inventory (or the wielded weapon)
// Items in dropped_worn are cloth that had been worn.
// All items in dropped that fit into the removed storage space
// (freed_volume_capacity) do not take time to drop.
// Example: dropping five 2x4 (volume 5*6) and a worn backpack
// (storage 40) will take only the time for dropping the backpack
// dropping two more 2x4 takes the time for dropping the backpack and
// dropping the remaining 2x4 that does not fit into the backpack.
int calculate_drop_cost( std::vector<item> &dropped, const std::vector<item> &dropped_worn,
                         int freed_volume_capacity )
{
    // Prefer to put small items into the backpack
    std::sort( dropped.begin(), dropped.end(), compare_items_by_lesser_volume );
    int drop_item_cnt = dropped_worn.size();
    int total_volume_dropped = 0;
    for( auto &elem : dropped ) {
        total_volume_dropped += elem.volume();
        if( freed_volume_capacity == 0 || total_volume_dropped > freed_volume_capacity ) {
            drop_item_cnt++;
        }
    }
    return drop_item_cnt * 100;
}

bool compare_by_dist_attitude::operator()( Creature *a, Creature *b ) const
{
    const auto aa = u.attitude_to( *a );
    const auto ab = u.attitude_to( *b );
    if( aa != ab ) {
        return aa < ab;
    }
    return rl_dist( a->pos(), u.pos() ) < rl_dist( b->pos(), u.pos() );
}

bool pair_greater_cmp::operator()( const std::pair<int, tripoint> &a,
                                   const std::pair<int, tripoint> &b )
{
    return a.first > b.first;
}

// --- Library functions ---
// This stuff could be moved elsewhere, but there
// doesn't seem to be a good place to put it right now.

// Basic logistic function.
double logarithmic( double t )
{
    return 1 / ( 1 + exp( -t ) );
}

// Logistic curve [-6,6], flipped and scaled to
// range from 1 to 0 as pos goes from min to max.
double logarithmic_range( int min, int max, int pos )
{
    const double LOGI_CUTOFF = 4;
    const double LOGI_MIN = logarithmic( -LOGI_CUTOFF );
    const double LOGI_MAX = logarithmic( +LOGI_CUTOFF );
    const double LOGI_RANGE = LOGI_MAX - LOGI_MIN;
    // Anything beyond [min,max] gets clamped.
    if( pos < min ) {
        return 1.0;
    } else if( pos > max ) {
        return 0.0;
    }

    // Normalize the pos to [0,1]
    double range = max - min;
    double unit_pos = ( pos - min ) / range;

    // Scale and flip it to [+LOGI_CUTOFF,-LOGI_CUTOFF]
    double scaled_pos = LOGI_CUTOFF - 2 * LOGI_CUTOFF * unit_pos;

    // Get the raw logistic value.
    double raw_logistic = logarithmic( scaled_pos );

    // Scale the output to [0,1]
    return ( raw_logistic - LOGI_MIN ) / LOGI_RANGE;
}

int bound_mod_to_vals( int val, int mod, int max, int min )
{
    if( val + mod > max && max != 0 ) {
        mod = std::max( max - val, 0 );
    }
    if( val + mod < min && min != 0 ) {
        mod = std::min( min - val, 0 );
    }
    return mod;
}

const char*  velocity_units( const units_type vel_units )
{
    if( OPTIONS["USE_METRIC_SPEEDS"].getValue() == "mph" ) {
        return _( "mph" );
    } else {
        switch( vel_units ) {
        case VU_VEHICLE:
            return _( "km/h" );
        case VU_WIND:
            return _( "m/s" );
        }
    }
    return "error: unknown units!";
}

const char* weight_units()
{
    return OPTIONS["USE_METRIC_WEIGHTS"].getValue() == "lbs" ? _( "lbs" ) : _( "kg" );
}

/**
* Convert internal velocity units to units defined by user
*/
double convert_velocity( int velocity, const units_type vel_units )
{
    // internal units to mph conversion
    double ret = double( velocity ) / 100;

    if( OPTIONS["USE_METRIC_SPEEDS"] == "km/h" ) {
        switch( vel_units ) {
        case VU_VEHICLE:
            // mph to km/h conversion
            ret *= 1.609f;
            break;
        case VU_WIND:
            // mph to m/s conversion
            ret *= 0.447f;
            break;
        }
    }
    return ret;
}

/**
* Convert weight in grams to units defined by user (kg or lbs)
*/
double convert_weight( int weight )
{
    double ret;
    ret = double( weight );
    if( OPTIONS["USE_METRIC_WEIGHTS"] == "kg" ) {
        ret /= 1000;
    } else {
        ret /= 453.6;
    }
    return ret;
}
