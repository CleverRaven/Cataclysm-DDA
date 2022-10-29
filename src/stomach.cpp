#include <algorithm>
#include <cmath>
#include <iosfwd>
#include <string>
#include <utility>

#include "cata_utility.h"
#include "character.h"
#include "json.h"
#include "stomach.h"
#include "units.h"
#include "vitamin.h"

void nutrients::min_in_place( const nutrients &r )
{
    calories = std::min( calories, r.calories );
    for( const std::pair<const vitamin_id, vitamin> &vit_pair : vitamin::all() ) {
        const vitamin_id &vit = vit_pair.first;
        int other = r.get_vitamin( vit );
        if( other == 0 ) {
            vitamins.erase( vit );
        } else {
            auto our_vit = vitamins.find( vit );
            if( our_vit != vitamins.end() ) {
                our_vit->second = std::min( our_vit->second, other );
            }
        }
    }
}

void nutrients::max_in_place( const nutrients &r )
{
    calories = std::max( calories, r.calories );
    for( const std::pair<const vitamin_id, vitamin> &vit_pair : vitamin::all() ) {
        const vitamin_id &vit = vit_pair.first;
        int other = r.get_vitamin( vit );
        if( other != 0 ) {
            int &val = vitamins[vit];
            val = std::max( val, other );
        }
    }
}

int nutrients::get_vitamin( const vitamin_id &vit ) const
{
    auto it = vitamins.find( vit );
    if( it == vitamins.end() ) {
        return 0;
    }
    return it->second;
}

int nutrients::kcal() const
{
    return calories / 1000;
}

bool nutrients::operator==( const nutrients &r ) const
{
    if( kcal() != r.kcal() ) {
        return false;
    }
    // Can't just use vitamins == r.vitamins, because there might be zero
    // entries in the map, which need to compare equal to missing entries.
    for( const std::pair<const vitamin_id, vitamin> &vit_pair : vitamin::all() ) {
        const vitamin_id &vit = vit_pair.first;
        if( get_vitamin( vit ) != r.get_vitamin( vit ) ) {
            return false;
        }
    }
    return true;
}

nutrients &nutrients::operator+=( const nutrients &r )
{
    calories += r.calories;
    for( const std::pair<const vitamin_id, int> &vit : r.vitamins ) {
        vitamins[vit.first] += vit.second;
    }
    return *this;
}

nutrients &nutrients::operator-=( const nutrients &r )
{
    calories -= r.calories;
    for( const std::pair<const vitamin_id, int> &vit : r.vitamins ) {
        vitamins[vit.first] -= vit.second;
    }
    return *this;
}

nutrients &nutrients::operator*=( int r )
{
    calories *= r;
    for( std::pair<const vitamin_id, int> &vit : vitamins ) {
        vit.second *= r;
    }
    return *this;
}

nutrients &nutrients::operator/=( int r )
{
    calories = divide_round_up( calories, r );
    for( std::pair<const vitamin_id, int> &vit : vitamins ) {
        vit.second = divide_round_up( vit.second, r );
    }
    return *this;
}

stomach_contents::stomach_contents() = default;

stomach_contents::stomach_contents( units::volume max_vol, bool is_stomach )
{
    max_volume = max_vol;
    stomach = is_stomach;
    last_ate = calendar::before_time_starts;
}

static std::string ml_to_string( const units::volume &vol )
{
    return std::to_string( units::to_milliliter<int>( vol ) ) + "_ml";
}

void stomach_contents::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "vitamins", nutr.vitamins );
    json.member( "calories", nutr.calories );
    json.member( "water", ml_to_string( water ) );
    json.member( "max_volume", ml_to_string( max_volume ) );
    json.member( "contents", ml_to_string( contents ) );
    json.member( "last_ate", last_ate );
    json.end_object();
}

static units::volume string_to_ml( const std::string &str )
{
    return units::from_milliliter( std::stoi( str.substr( 0, str.size() - 3 ) ) );
}

void stomach_contents::deserialize( const JsonObject &jo )
{
    jo.read( "vitamins", nutr.vitamins );
    jo.read( "calories", nutr.calories );
    std::string str;
    jo.read( "water", str );
    water = string_to_ml( str );
    jo.read( "max_volume", str );
    max_volume = string_to_ml( str );
    jo.read( "contents", str );
    contents = string_to_ml( str );
    jo.read( "last_ate", last_ate );
}

units::volume stomach_contents::capacity( const Character &owner ) const
{
    return max_volume * owner.mutation_value( "stomach_size_multiplier" );
}

units::volume stomach_contents::stomach_remaining( const Character &owner ) const
{
    return capacity( owner ) - contents - water;
}

units::volume stomach_contents::contains() const
{
    return contents + water;
}

void stomach_contents::ingest( const food_summary &ingested )
{
    contents += ingested.solids;
    water += ingested.water;
    nutr += ingested.nutr;
    ate();
}

food_summary stomach_contents::digest( const Character &owner, const needs_rates &metabolic_rates,
                                       int five_mins, int half_hours )
{
    food_summary digested;
    stomach_digest_rates rates = get_digest_rates( metabolic_rates, owner );

    // Digest water, but no more than in stomach.
    digested.water = std::min( water, rates.water * five_mins );
    water -= digested.water;

    // If no half-hour intervals have passed, we only process water, so bail out early.
    if( half_hours == 0 ) {
        return digested;
    }

    // Digest solids, but no more than in stomach.
    digested.solids = std::min( contents, rates.solids * half_hours );
    contents -= digested.solids;

    // Digest kCal -- use min_kcal by default, but no more than what's in stomach,
    // and no less than percentage_kcal of what's in stomach.
    int kcal_fraction = std::lround( nutr.kcal() * rates.percent_kcal );
    digested.nutr.calories = half_hours * clamp( rates.min_calories, kcal_fraction * 1000,
                             nutr.calories );

    // Digest vitamins just like we did kCal, but we need to do one at a time.
    for( const std::pair<const vitamin_id, int> &vit : nutr.vitamins ) {
        int vit_fraction = std::lround( vit.second * rates.percent_vitamin );
        digested.nutr.vitamins[vit.first] =
            half_hours * clamp( rates.min_vitamin, vit_fraction, vit.second );
    }

    nutr -= digested.nutr;
    return digested;
}

void stomach_contents::empty()
{
    nutr = nutrients{};
    water = 0_ml;
    contents = 0_ml;
}

stomach_digest_rates stomach_contents::get_digest_rates( const needs_rates &metabolic_rates,
        const Character &owner ) const
{
    stomach_digest_rates rates;
    if( stomach ) {
        // The stomach is focused on passing material on to the guts.
        // 3 hours to empty in 30 minute increments
        rates.solids = capacity( owner ) / 6;
        rates.water = 250_ml; // Water is special, passes very quickly, in 5 minute intervals
        rates.min_vitamin = 1;
        rates.percent_vitamin = 1.0f / 6.0f;
        rates.min_calories = 5000;
        rates.percent_kcal = 1.0f / 6.0f;
    } else {
        // The guts are focused on absorption into the body, we don't care about passing rates.
        // Solids rate doesn't do anything impactful here so just make it big enough to avoid overflow.
        rates.solids = 250_ml;
        rates.water = 250_ml;
        // Explicitly floor it, because casting it to an int will do so anyways
        rates.min_calories = std::floor( metabolic_rates.kcal / 24.0 * metabolic_rates.hunger * 1000 );
        rates.percent_kcal = 0.05f * metabolic_rates.hunger;
        rates.min_vitamin = std::round( 100.0 / 24.0 * metabolic_rates.hunger );
        rates.percent_vitamin = 0.05f * metabolic_rates.hunger;
    }
    return rates;
}

void stomach_contents::mod_calories( int kcal )
{
    if( -kcal >= nutr.kcal() ) {
        nutr.calories = 0;
        return;
    }
    nutr.calories += kcal * 1000;
}

void stomach_contents::mod_nutr( int nutr )
{
    // nutr is legacy type code, this function simply converts old nutrition to new kcal
    mod_calories( -1 * std::round( nutr * 2500.0f / ( 12 * 24 ) ) );
}

void stomach_contents::mod_water( const units::volume &h2o )
{
    if( h2o > 0_ml ) {
        water += h2o;
    }
}

void stomach_contents::mod_quench( int quench )
{
    mod_water( 5_ml * quench );
}

void stomach_contents::mod_contents( const units::volume &vol )
{
    if( -vol >= contents ) {
        contents = 0_ml;
        return;
    }
    contents += vol;
}

int stomach_contents::get_calories() const
{
    return nutr.kcal();
}

units::volume stomach_contents::get_water() const
{
    return water;
}

void stomach_contents::ate()
{
    last_ate = calendar::turn;
}

time_duration stomach_contents::time_since_ate() const
{
    return calendar::turn - last_ate;
}

// sets default stomach contents when starting the game
void Character::initialize_stomach_contents()
{
    stomach = stomach_contents( 2500_ml, true );
    guts = stomach_contents( 24_liter, false );
    guts.mod_calories( 300 );
    stomach.mod_calories( 800 );
    stomach.mod_contents( 475_ml );
}
