#include <algorithm>
#include <cmath>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include "cata_assert.h"
#include "cata_utility.h"
#include "character.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "itype.h"
#include "json.h"
#include "magic_enchantment.h"
#include "pimpl.h"
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
            vitamins_.erase( vit );
        } else {
            auto our_vit = vitamins_.find( vit );
            if( our_vit != vitamins_.end() ) {
                // We must be finalized because we're calling vitamin::all()
                our_vit->second = std::min( std::get<int>( our_vit->second ), other );
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
            std::variant<int, vitamin_units::mass> &val = vitamins_[vit];
            // We must be finalized because we're calling vitamin::all()
            val = std::max( std::get<int>( val ), other );
        }
    }
}

void nutrients::clear_vitamins()
{
    vitamins_.clear();
}

std::map<vitamin_id, int> nutrients::vitamins() const
{
    if( !finalized ) {
        debugmsg( "Called nutrients::vitamins() before they were finalized!" );
        return std::map<vitamin_id, int>();
    }

    std::map<vitamin_id, int> ret;
    for( const std::pair<const vitamin_id, std::variant<int, vitamin_units::mass>> &vit : vitamins_ ) {
        ret.emplace( vit.first, std::get<int>( vit.second ) );
    }
    return ret;
}

void nutrients::set_vitamin( const vitamin_id &vit, vitamin_units::mass mass )
{
    if( finalized ) {
        set_vitamin( vit, vit->units_from_mass( mass ) );
        return;
    }
    vitamins_[vit] = mass;
}

void nutrients::add_vitamin( const vitamin_id &vit, vitamin_units::mass mass )
{
    if( finalized ) {
        add_vitamin( vit, vit->units_from_mass( mass ) );
        return;
    }
    auto iter = vitamins_.emplace( vit, vitamin_units::mass( 0, {} ) ).first;
    if( !std::holds_alternative<vitamin_units::mass>( iter->second ) ) {
        debugmsg( "Tried to add mass vitamin to units vitamin before vitamins were finalized!" );
        return;
    }
    iter->second = std::get<vitamin_units::mass>( iter->second ) + mass;
}

void nutrients::set_vitamin( const vitamin_id &vit, int units )
{
    auto iter = vitamins_.emplace( vit, 0 ).first;
    iter->second = units;
}

void nutrients::add_vitamin( const vitamin_id &vit, int units )
{
    auto iter = vitamins_.emplace( vit, 0 ).first;
    if( std::holds_alternative<vitamin_units::mass>( iter->second ) ) {
        debugmsg( "Tried to add mass vitamin to units vitamin before vitamins were finalized!" );
        return;
    }
    iter->second = std::get<int>( iter->second ) + units;
}

void nutrients::remove_vitamin( const vitamin_id &vit )
{
    vitamins_.erase( vit );
}

int nutrients::get_vitamin( const vitamin_id &vit ) const
{
    auto it = vitamins_.find( vit );
    if( it == vitamins_.end() ) {
        return 0;
    }
    if( !finalized && std::holds_alternative<vitamin_units::mass>( it->second ) ) {
        debugmsg( "Called get_vitamin on a mass vitamin before vitamins were finalized!" );
        return 0;
    }
    return std::get<int>( it->second );
}

int nutrients::kcal() const
{
    return calories / 1000;
}

void nutrients::finalize_vitamins()
{
    for( std::pair<const vitamin_id, std::variant<int, vitamin_units::mass> > &vit : vitamins_ ) {
        if( std::holds_alternative<vitamin_units::mass>( vit.second ) ) {
            vit.second = vit.first->units_from_mass( std::get<vitamin_units::mass>( vit.second ) );
        }
        if( std::holds_alternative<vitamin_units::mass>( vit.second ) ) {
            debugmsg( "Error occured during vitamin finalization!" );
        }
    }

    finalized = true;
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

nutrients nutrients::operator-()
{
    nutrients negative_copy = *this;
    negative_copy.calories *= -1;
    for( const std::pair<const vitamin_id, std::variant<int, vitamin_units::mass>> &vit :
         negative_copy.vitamins_ ) {
        std::variant<int, vitamin_units::mass> &here = vitamins_[vit.first];
        here = std::get<int>( here ) * -1;
    }
    return negative_copy;
}

nutrients &nutrients::operator+=( const nutrients &r )
{
    if( !finalized || !r.finalized ) {
        debugmsg( "Nutrients not finalized when += called!" );
    }
    calories += r.calories;
    for( const std::pair<const vitamin_id, std::variant<int, vitamin_units::mass>> &vit :
         r.vitamins_ ) {
        std::variant<int, vitamin_units::mass> &here = vitamins_[vit.first];
        here = std::get<int>( here ) + std::get<int>( vit.second );
    }
    return *this;
}

nutrients &nutrients::operator-=( const nutrients &r )
{
    if( !finalized || !r.finalized ) {
        debugmsg( "Nutrients not finalized when -= called!" );
    }
    calories -= r.calories;
    for( const std::pair<const vitamin_id, std::variant<int, vitamin_units::mass>> &vit :
         r.vitamins_ ) {
        std::variant<int, vitamin_units::mass> &here = vitamins_[vit.first];
        here = std::get<int>( here ) - std::get<int>( vit.second );
    }
    return *this;
}

nutrients &nutrients::operator*=( int r )
{
    if( !finalized ) {
        debugmsg( "Nutrients not finalized when *= called!" );
    }
    calories *= r;
    for( const std::pair<const vitamin_id, std::variant<int, vitamin_units::mass>> &vit : vitamins_ ) {
        std::variant<int, vitamin_units::mass> &here = vitamins_[vit.first];
        here = std::get<int>( here ) * r;
    }
    return *this;
}

nutrients &nutrients::operator*=( double r )
{
    if( !finalized ) {
        debugmsg( "Nutrients not finalized when *= called!" );
    }
    calories *= r;
    for( const std::pair<const vitamin_id, std::variant<int, vitamin_units::mass>> &vit : vitamins_ ) {
        std::variant<int, vitamin_units::mass> &here = vitamins_[vit.first];
        cata_assert( std::get<int>( here ) >= 0 );
        if( std::get<int>( here ) == 0 ) {
            continue;
        }
        // truncates, but always keep at least 1 (for e.g. allergies)
        here = std::max( static_cast<int>( std::get<int>( here ) * r ), 1 );
    }
    return *this;
}

nutrients &nutrients::operator/=( int r )
{
    if( !finalized ) {
        debugmsg( "Nutrients not finalized when -= called!" );
    }
    calories = divide_round_up<int64_t>( calories, r );
    for( const std::pair<const vitamin_id, std::variant<int, vitamin_units::mass>> &vit : vitamins_ ) {
        std::variant<int, vitamin_units::mass> &here = vitamins_[vit.first];
        here = divide_round_up( std::get<int>( here ), r );
    }
    return *this;
}

void nutrients::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "calories", calories );
    jsout.member( "vitamins", vitamins() );
    jsout.end_object();
}

void nutrients::deserialize( const JsonObject &jo )
{
    jo.read( "calories", calories );
    std::map<vitamin_id, int> vit_map;
    jo.read( "vitamins", vit_map );
    for( auto &vit : vit_map ) {
        //rebuild vitamins_
        set_vitamin( vit.first, vit.second );
    }
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
    json.member( "vitamins", nutr.vitamins() );
    json.member( "calories", nutr.calories );
    json.member( "water", ml_to_string( water ) );
    json.member( "max_volume", ml_to_string( max_volume ) );
    json.member( "contents", ml_to_string( contents ) );
    json.member( "last_ate", last_ate );
    json.end_object();
}

static units::volume string_to_ml( std::string_view str )
{
    return units::from_milliliter( svto<int>( str.substr( 0, str.size() - 3 ) ) );
}

void stomach_contents::deserialize( const JsonObject &jo )
{
    std::map<vitamin_id, int> vitamins;
    jo.read( "vitamins", vitamins );
    for( const std::pair<const vitamin_id, int> &vit : vitamins ) {
        nutr.set_vitamin( vit.first, vit.second );
    }
    jo.read( "calories", nutr.calories );
    std::string str;
    jo.read( "water", str );
    water = string_to_ml( str );
    jo.read( "max_volume", str );
    max_volume = string_to_ml( str );
    jo.read( "contents", str );
    contents = string_to_ml( str );
    jo.read( "last_ate", last_ate );

    // the next chunk deletes obsoleted vitamins
    const auto predicate = []( const std::pair<vitamin_id, int> &pair ) {
        if( !pair.first.is_valid() ) {
            DebugLog( D_WARNING, DC_ALL )
                    << "deleted '" << pair.first.str() << "' from stomach_contents::nutrients::vitamins";
            return true;
        }
        return false;
    };
    std::map<vitamin_id, int> vit = nutr.vitamins();
    for( const std::pair<const vitamin_id, int> &pair : vit ) {
        if( predicate( pair ) ) {
            nutr.remove_vitamin( pair.first );
        }
    }
}

units::volume stomach_contents::capacity( const Character &owner ) const
{
    return std::max( 250_ml, owner.enchantment_cache->modify_value(
                         enchant_vals::mod::STOMACH_SIZE_MULTIPLIER, max_volume ) );
}

units::volume stomach_contents::stomach_remaining( const Character &owner ) const
{
    return capacity( owner ) - contents - water;
}

bool stomach_contents::would_be_engorged_with( const Character &owner, units::volume intake,
        bool calorie_deficit ) const
{
    const double fullness_ratio = ( contains() + intake ) / capacity( owner );
    return ( calorie_deficit && fullness_ratio >= 1.0 ) || ( fullness_ratio >= 5.0 / 6.0 );
}

bool stomach_contents::would_be_full_with( const Character &owner, units::volume intake,
        bool calorie_deficit ) const
{
    const double fullness_ratio = ( contains() + intake ) / capacity( owner );
    return ( calorie_deficit && fullness_ratio >= 11.0 / 20.0 ) || ( fullness_ratio >= 3.0 / 4.0 );
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
    digested.nutr.calories = half_hours * clamp<int64_t>( rates.min_calories, kcal_fraction * 1000,
                             nutr.calories );

    // Digest vitamins just like we did kCal, but we need to do one at a time.
    for( const std::pair<const vitamin_id, int> &vit : nutr.vitamins() ) {
        if( vit.first->type() != vitamin_type::DRUG ) {
            int vit_fraction = std::lround( vit.second * rates.percent_vitamin );
            digested.nutr.set_vitamin( vit.first, half_hours * clamp( rates.min_vitamin, vit_fraction,
                                       vit.second ) );
        }
        // drug vitamins are absorbed to the blood instantly after the first stomach step.
        // this makes the drug vitamins easier to balance (no need to account for slow trickle-ing in of the drug)
        else if( vit.first->type() == vitamin_type::DRUG && stomach ) {
            digested.nutr.set_vitamin( vit.first, vit.second );
        }
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
    mod_calories( -1 * std::round( nutr * base_metabolic_rate / ( 12 * 24 ) ) );
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
