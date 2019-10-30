#include <string>

#include "avatar.h"
#include "cata_utility.h"
#include "json.h"
#include "player.h"
#include "stomach.h"
#include "units.h"
#include "game.h"
#include "itype.h"

stomach_contents::stomach_contents() = default;

stomach_contents::stomach_contents( units::volume max_vol, bool is_stomach )
{
    max_volume = max_vol;
    stomach = is_stomach;
    last_ate = calendar::before_time_starts;
}

static std::string ml_to_string( units::volume vol )
{
    return to_string( units::to_milliliter<int>( vol ) ) + "_ml";
}

void stomach_contents::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "vitamins", vitamins );
    json.member( "calories", calories );
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

void stomach_contents::deserialize( JsonIn &json )
{
    JsonObject jo = json.get_object();
    jo.read( "vitamins", vitamins );
    jo.read( "calories", calories );
    std::string str;
    jo.read( "water", str );
    water = string_to_ml( str );
    jo.read( "max_volume", str );
    max_volume = string_to_ml( str );
    jo.read( "contents", str );
    contents = string_to_ml( str );
    jo.read( "last_ate", last_ate );
}

units::volume stomach_contents::capacity() const
{
    float max_mod = 1;
    if( g->u.has_trait( trait_id( "GIZZARD" ) ) ) {
        max_mod *= 0.9;
    }
    if( g->u.has_active_mutation( trait_id( "HIBERNATE" ) ) ) {
        max_mod *= 3;
    }
    if( g->u.has_active_mutation( trait_id( "GOURMAND" ) ) ) {
        max_mod *= 2;
    }
    if( g->u.has_trait( trait_id( "SLIMESPAWNER" ) ) ) {
        max_mod *= 3;
    }
    return max_volume * max_mod;
}

units::volume stomach_contents::stomach_remaining() const
{
    return capacity() - contents - water;
}

units::volume stomach_contents::contains() const
{
    return contents + water;
}

void stomach_contents::ingest( player &p, item &food, int charges = 1 )
{
    item comest;
    if( food.is_container() ) {
        comest = food.get_contained();
    } else {
        comest = food;
    }
    if( charges == 0 ) {
        // if charges is 0, it means the item is not count_by_charges
        charges = 1;
    }
    // maybe move tapeworm to digestion
    for( const auto &v : p.vitamins_from( comest ) ) {
        vitamins[v.first] += p.has_effect( efftype_id( "tapeworm" ) ) ? v.second / 2 : v.second;
    }
    auto &comest_t = comest.type->comestible;
    const units::volume add_water = std::max( 0_ml, comest_t->quench * 5_ml );
    // if this number is negative, we decrease the quench/increase the thirst of the player
    if( comest_t->quench < 0 ) {
        p.mod_thirst( -comest_t->quench );
    } else {
        mod_quench( comest_t->quench );
    }
    // @TODO: Move quench values to mL and remove the magic number here
    mod_contents( comest.base_volume() * charges - add_water );

    ate();

    mod_calories( p.kcal_for( comest ) );
}

void stomach_contents::ingest( const nutrients &ingested )
{
    contents += ingested.solids;
    water += ingested.water;
    calories += ingested.kcal;
    for( const std::pair<const vitamin_id, int> &vit : ingested.vitamins ) {
        vitamins[vit.first] += vit.second;
    }
}

nutrients stomach_contents::digest( const needs_rates &metabolic_rates,
                                    int five_mins, int half_hours )
{
    nutrients digested;
    stomach_digest_rates rates = get_digest_rates( metabolic_rates );

    // Digest water, but no more than in stomach.
    digested.water = std::min( water, rates.water * five_mins );
    water -= digested.water;

    // If no half-hour intervals have passed, we only process water, so bail out early.
    if( half_hours == 0 ) {
        // We need to initialize these to zero first, though.
        digested.kcal = 0;
        digested.solids = 0_ml;
        for( const std::pair<const vitamin_id, int> &vit : digested.vitamins ) {
            digested.vitamins[vit.first] = 0;
        }
        return digested;
    }

    // Digest solids, but no more than in stomach.
    digested.solids = std::min( contents, rates.solids * half_hours );
    contents -= digested.solids;

    // Digest kCal -- use min_kcal by default, but no more than what's in stomach,
    // and no less than percentage_kcal of what's in stomach.
    digested.kcal = half_hours * clamp( rates.min_kcal,
                                        static_cast<int>( round( calories * rates.percent_kcal ) ), calories );
    calories -= digested.kcal;

    // Digest vitamins just like we did kCal, but we need to do one at a time.
    for( const std::pair<const vitamin_id, int> &vit : vitamins ) {
        digested.vitamins[vit.first] = half_hours * clamp( rates.min_vitamin,
                                       static_cast<int>( round( vit.second * rates.percent_vitamin ) ), vit.second );
        vitamins[vit.first] -= digested.vitamins[vit.first];
    }

    return digested;
}

void stomach_contents::empty()
{
    calories = 0;
    water = 0_ml;
    contents = 0_ml;
    vitamins.clear();
}

stomach_digest_rates stomach_contents::get_digest_rates( const needs_rates &metabolic_rates )
{
    stomach_digest_rates rates;
    if( stomach ) {
        // The stomach is focused on passing material on to the guts.
        // 3 hours to empty in 30 minute increments
        rates.solids = capacity() / 6;
        rates.water = 250_ml; // Water is special, passes very quickly, in 5 minute intervals
        rates.min_vitamin = 1;
        rates.percent_vitamin = 1.0f / 6.0f;
        rates.min_kcal = 5;
        rates.percent_kcal = 1.0f / 6.0f;
    } else {
        // The guts are focused on absorption into the body, we don't care about passing rates.
        // Solids rate doesn't do anything impactful here so just make it big enough to avoid overflow.
        rates.solids = 250_ml;
        rates.water = 250_ml;
        rates.min_kcal = roll_remainder( metabolic_rates.kcal / 24.0 * metabolic_rates.hunger );
        rates.percent_kcal = 0.05f * metabolic_rates.hunger;
        rates.min_vitamin = round( 100.0 / 24.0 * metabolic_rates.hunger );
        rates.percent_vitamin = 0.05f * metabolic_rates.hunger;
    }
    return rates;
}

void stomach_contents::mod_calories( int cal )
{
    if( -cal >= calories ) {
        calories = 0;
        return;
    }
    calories += cal;
}

void stomach_contents::mod_nutr( int nutr )
{
    // nutr is legacy type code, this function simply converts old nutrition to new kcal
    mod_calories( -1 * round( nutr * 2500.0f / ( 12 * 24 ) ) );
}

void stomach_contents::mod_water( units::volume h2o )
{
    if( h2o > 0_ml ) {
        water += h2o;
    }
}

void stomach_contents::mod_quench( int quench )
{
    mod_water( 5_ml * quench );
}

void stomach_contents::mod_contents( units::volume vol )
{
    if( -vol >= contents ) {
        contents = 0_ml;
        return;
    }
    contents += vol;
}

int stomach_contents::get_calories() const
{
    return calories;
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
    guts = stomach_contents( 24000_ml, false );
    guts.mod_calories( 300 );
    stomach.mod_calories( 800 );
    stomach.mod_contents( 475_ml );
}
