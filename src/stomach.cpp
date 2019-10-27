#include <algorithm>
#include <memory>
#include <string>
#include <cmath>

#include "avatar.h"
#include "calendar.h"
#include "cata_utility.h"
#include "json.h"
#include "player.h"
#include "stomach.h"
#include "units.h"
#include "compatibility.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "optional.h"
#include "rng.h"
#include "character.h"
#include "options.h"

stomach_contents::stomach_contents() = default;

stomach_contents::stomach_contents( units::volume max_vol )
{
    max_volume = max_vol;
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
    json.member( "vitamins_absorbed", vitamins_absorbed );
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
    jo.read( "vitamins_absorbed", vitamins_absorbed );
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

bool stomach_contents::store_absorbed( player &p )
{
    if( p.is_npc() && get_option<bool>( "NO_NPC_FOOD" ) ) {
        return false;
    }
    bool absorbed = false;
    if( calories_absorbed != 0 ) {
        p.mod_stored_kcal( calories_absorbed );
        absorbed = true;
    }
    p.vitamins_mod( vitamins_absorbed, false );
    vitamins_absorbed.clear();
    return absorbed;
}

void stomach_contents::bowel_movement( const stomach_pass_rates &rates )
{
    int cal_rate = static_cast<int>( round( calories * rates.percent_kcal ) );
    cal_rate = clamp( cal_rate, std::min( rates.min_kcal, calories - calories_absorbed ),
                      calories - calories_absorbed );
    if( cal_rate < 0 ) {
        cal_rate = 0;
    }
    mod_calories( -cal_rate );
    calories_absorbed = 0;
    units::volume contents_rate = clamp( units::from_milliliter<int>( round(
            units::to_milliliter<float>
            ( contents ) * rates.percent_vol ) ), rates.min_vol, contents );
    mod_contents( -contents_rate );
    // water is a special case.
    water = 0_ml;

    for( auto &vit : vitamins ) {
        const int vit_absorbed = vitamins_absorbed[vit.first];
        // need to take absorbed vitamins into account for % moved
        int percent = static_cast<int>( ( vit.second + vit_absorbed ) * rates.percent_vit );
        // minimum vitamins that will move
        int min = rates.min_vit;
        int calc = std::max( percent, min );

        if( vit_absorbed >= calc ) {
            // vitamins are absorbed instead of moved
            // assumed absorption is called from another function
            calc = 0;
        } else if( vit_absorbed != 0 ) {
            calc -= vit_absorbed;
        }

        if( calc > vitamins[vit.first] ) {
            calc = vitamins[vit.first];
        }

        vitamins[vit.first] -= calc;
    }
}

void stomach_contents::bowel_movement()
{
    stomach_pass_rates rates;

    /**
     * The min numbers aren't technically needed since the percentage will
     * take care of the entirety, but some values needed to be assigned
     */
    rates.percent_kcal = 1;
    rates.min_kcal = 250;
    rates.percent_vol = 1;
    rates.min_vol = 25000_ml;
    rates.percent_vit = 1;
    rates.min_vit = 100;

    bowel_movement( rates );
}

void stomach_contents::bowel_movement( const stomach_pass_rates &rates, stomach_contents &move_to )
{
    int cal_rate = static_cast<int>( round( calories * rates.percent_kcal ) );
    cal_rate = clamp( cal_rate, std::min( rates.min_kcal, calories - calories_absorbed ),
                      calories - calories_absorbed );
    move_to.mod_calories( cal_rate );
    units::volume contents_rate = clamp( units::from_milliliter<int>( round(
            units::to_milliliter<float>
            ( contents ) * rates.percent_vol ) ), rates.min_vol, contents );
    move_to.mod_contents( -contents_rate );
    move_to.water += water;

    for( auto &vit : vitamins ) {
        const int vit_absorbed = vitamins_absorbed[vit.first];
        // need to take absorbed vitamins into account for % moved
        int percent = static_cast<int>( ( vit.second + vit_absorbed ) * rates.percent_vit );
        // minimum vitamins that will move
        int min = rates.min_vit;
        int calc = std::max( percent, min );

        if( vit_absorbed >= calc ) {
            // vitamins are absorbed instead of moved
            // assumed absorption is called from another function
            calc = 0;
        } else if( vit_absorbed != 0 ) {
            calc -= vit_absorbed;
        }

        if( calc > vitamins[vit.first] ) {
            calc = vitamins[vit.first];
        }

        move_to.vitamins[vit.first] += calc;
    }
    bowel_movement( rates );
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

void stomach_contents::absorb_water( player &p, units::volume amount )
{
    if( water < amount ) {
        amount = water;
        water = 0_ml;
    } else {
        water -= amount;
    }
    if( p.get_thirst() < -100 ) {
        return;
    } else if( p.get_thirst() - units::to_milliliter( amount / 5 ) < -100 ) {
        p.set_thirst( -100 );
    }
    p.mod_thirst( -units::to_milliliter( amount ) / 5 );
}

stomach_pass_rates stomach_contents::get_pass_rates( bool stomach )
{
    stomach_pass_rates rates;
    // a human will digest food in about 53 hours
    // most of the excrement is completely indigestible
    // ie almost all of the calories ingested are absorbed.
    // 3 hours will be accounted here as stomach
    // the rest will be guts
    if( stomach ) {
        rates.min_vol = capacity() / 6;
        // 3 hours to empty in 30 minute increments
        rates.percent_vol = 1.0f / 6.0f;
        rates.min_vit = 1;
        rates.percent_vit = 1.0f / 6.0f;
        rates.min_kcal = 5;
        rates.percent_kcal = 1.0f / 6.0f;
    } else {
        rates.min_vol = std::max( capacity() / 100, 1_ml );
        // 50 hours to empty in 30 minute increments
        rates.percent_vol = 0.01f;
        rates.min_vit = 1;
        rates.percent_vit = 0.01f;
        rates.min_kcal = 5;
        rates.percent_kcal = 0.01f;
    }
    return rates;
}

stomach_absorb_rates stomach_contents::get_absorb_rates( bool stomach,
        const needs_rates &metabolic_rates )
{
    stomach_absorb_rates rates;
    if( !stomach ) {
        rates.min_kcal = roll_remainder( metabolic_rates.kcal / 24.0 * metabolic_rates.hunger );
        rates.percent_kcal = 0.05f * metabolic_rates.hunger;
        rates.min_vitamin_default = round( 100.0 / 24.0 * metabolic_rates.hunger );
        rates.percent_vitamin_default = 0.05f * metabolic_rates.hunger;
    } else {
        rates.min_kcal = 0;
        rates.percent_kcal = 0.0f;
        rates.min_vitamin_default = 0;
        rates.percent_vitamin_default = 0.0f;
    }
    return rates;
}

void stomach_contents::calculate_absorbed( stomach_absorb_rates rates )
{
    for( const auto vit : vitamins ) {
        int min_v;
        float percent_v;
        if( rates.min_vitamin.find( vit.first ) != rates.min_vitamin.end() ) {
            min_v = rates.min_vitamin[vit.first];
        } else {
            min_v = rates.min_vitamin_default;
        }
        if( rates.percent_vitamin.find( vit.first ) != rates.percent_vitamin.end() ) {
            percent_v = rates.percent_vitamin[vit.first];
        } else {
            percent_v = rates.percent_vitamin_default;
        }
        int vit_absorbed = std::max( min_v, static_cast<int>( percent_v * vit.second ) );
        // make sure the vitamins don't overflow
        vit_absorbed = std::min( vit_absorbed, vit.second );
        vitamins_absorbed[vit.first] += vit_absorbed;
        vitamins[vit.first] -= vit_absorbed;
    }
    int cal = clamp( static_cast<int>( round( calories * rates.percent_kcal ) ),
                     std::min( rates.min_kcal, calories ), calories );
    calories_absorbed += cal;
    calories -= cal;
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

int stomach_contents::get_calories_absorbed() const
{
    return calories_absorbed;
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
    stomach = stomach_contents( 2500_ml );
    guts = stomach_contents( 24000_ml );
    guts.mod_calories( 300 );
    stomach.mod_calories( 800 );
    stomach.mod_contents( 475_ml );
}
