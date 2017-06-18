#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

#include "scenario.h"

#include "debug.h"
#include "json.h"
#include "player.h"
#include "bionics.h"
#include "game.h"
#include "map.h"
#include "translations.h"
#include "pldata.h"
#include "addiction.h"
#include "profession.h"
#include "mutation.h"
#include "mapgen.h"
#include "generic_factory.h"

namespace
{
generic_factory<scenario> all_scenarios( "scenario", "ident" );
const string_id<scenario> generic_scenario_id( "evacuee" );
}

/** @relates string_id */
template<>
const scenario &string_id<scenario>::obj() const
{
    return all_scenarios.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<scenario>::is_valid() const
{
    return all_scenarios.is_valid( *this );
}

scenario::scenario()
    : id( "" ), _name_male( "null" ), _name_female( "null" ),
      _description_male( "null" ), _description_female( "null" )
{
}

void scenario::load_scenario( JsonObject &jo, const std::string &src )
{
    all_scenarios.load( jo, src );
}

void scenario::load( JsonObject &jo, const std::string & )
{
    // TODO: pretty much the same as in profession::load, but different contexts for pgettext.
    // TODO: maybe combine somehow?
    if( !was_loaded || jo.has_string( "name" ) ) {
        // These may differ depending on the language settings!
        const std::string name = jo.get_string( "name" );
        _name_female = pgettext( "scenario_female", name.c_str() );
        _name_male = pgettext( "scenario_male", name.c_str() );
    }

    if( !was_loaded || jo.has_string( "description" ) ) {
        // These also may differ depending on the language settings!
        const std::string desc = jo.get_string( "description" );
        _description_male = pgettext( "scen_desc_male", desc.c_str() );
        _description_female = pgettext( "scen_desc_female", desc.c_str() );
    }

    if( !was_loaded || jo.has_string( "start_name" ) ) {
        _start_name = pgettext( "start_name", jo.get_string( "start_name" ).c_str() );
    }

    mandatory( jo, was_loaded, "points", _point_cost );

    optional( jo, was_loaded, "blacklist_professions", blacklist );
    optional( jo, was_loaded, "professions", professions,
              auto_flags_reader<string_id<profession>> {} );

    optional( jo, was_loaded, "traits", _allowed_traits, auto_flags_reader<trait_id> {} );
    optional( jo, was_loaded, "forced_traits", _forced_traits, auto_flags_reader<trait_id> {} );
    optional( jo, was_loaded, "forbidden_traits", _forbidden_traits, auto_flags_reader<trait_id> {} );
    optional( jo, was_loaded, "allowed_locs", _allowed_locs, auto_flags_reader<start_location_id> {} );
    if( _allowed_locs.empty() ) {
        jo.throw_error( "at least one starting location (member \"allowed_locs\") must be defined" );
    }
    optional( jo, was_loaded, "flags", flags, auto_flags_reader<> {} );
    optional( jo, was_loaded, "map_special", _map_special, "mx_null" );
}

const scenario *scenario::generic()
{
    return &generic_scenario_id.obj();
}

// Strategy: a third of the time, return the generic scenario.  Otherwise, return a scenario,
// weighting 0 cost scenario more likely--the weight of a scenario with cost n is 2/(|n|+2),
// e.g., cost 1 is 2/3rds as likely, cost -2 is 1/2 as likely.
const scenario *scenario::weighted_random()
{
    if( one_in( 3 ) ) {
        return generic();
    }

    const auto &list = all_scenarios.get_all();
    while( true ) {
        auto iter = list.begin();
        std::advance( iter, rng( 0, list.size() - 1 ) );
        const scenario &scen = *iter;

        if( x_in_y( 2, abs( scen.point_cost() ) + 2 ) ) {
            return &scen;
        }
        // else reroll in the while loop.
    }
}

const std::vector<scenario> &scenario::get_all()
{
    return all_scenarios.get_all();
}

void scenario::reset()
{
    all_scenarios.reset();
}

void scenario::check_definitions()
{
    for( const auto &scen : all_scenarios.get_all() ) {
        scen.check_definition();
    }
}

void check_traits( const std::set<trait_id> &traits, const string_id<scenario> &ident )
{
    for( auto &t : traits ) {
        if( !t.is_valid() ) {
            debugmsg( "trait %s for scenario %s does not exist", t.c_str(), ident.c_str() );
        }
    }
}

void scenario::check_definition() const
{
    for( auto &p : professions ) {
        if( !p.is_valid() ) {
            debugmsg( "profession %s for scenario %s does not exist", p.c_str(), id.c_str() );
        }
    }
    if( std::any_of( professions.begin(), professions.end(), [&]( const string_id<profession> &p ) {
    return std::count( professions.begin(), professions.end(), p ) > 1;
    } ) ) {
        debugmsg( "Duplicate entries in the professions array." );
    }

    for( auto &l : _allowed_locs ) {
        if( !l.is_valid() ) {
            debugmsg( "starting location %s for scenario %s does not exist", l.c_str(), id.c_str() );
        }
    }

    if( blacklist ) {
        if( professions.empty() ) {
            debugmsg( "Scenario %s: Use an empty whitelist to whitelist everything.", id.c_str() );
        } else {
            permitted_professions(); // Debug msg if every profession is blacklisted
        }
    }

    check_traits( _allowed_traits, id );
    check_traits( _forced_traits, id );
    check_traits( _forbidden_traits, id );
    MapExtras::get_function( _map_special ); // triggers a debug message upon invalid input
}

const string_id<scenario> &scenario::ident() const
{
    return id;
}

std::string scenario::gender_appropriate_name( bool male ) const
{
    if( male ) {
        return _name_male;
    } else {
        return _name_female;
    }
}

std::string scenario::description( bool male ) const
{
    if( male ) {
        return _description_male;
    } else {
        return _description_female;
    }
}

signed int scenario::point_cost() const
{
    return _point_cost;
}

start_location_id scenario::start_location() const
{
    return _allowed_locs.front();
}
start_location_id scenario::random_start_location() const
{
    return random_entry( _allowed_locs );
}

std::vector<string_id<profession>> scenario::permitted_professions() const
{
    if( !cached_permitted_professions.empty() ) {
        return cached_permitted_professions;
    }

    const auto all = profession::get_all();
    std::vector<string_id<profession>> &res = cached_permitted_professions;
    for( const profession &p : all ) {
        const bool present = std::find( professions.begin(), professions.end(),
                                        p.ident() ) != professions.end();
        if( blacklist || professions.empty() ) {
            if( !present && !p.has_flag( "SCEN_ONLY" ) ) {
                res.push_back( p.ident() );
            }
        } else if( present ) {
            res.push_back( p.ident() );
        }
    }

    if( res.empty() ) {
        debugmsg( "Why would you blacklist every profession?" );
        res.push_back( profession::generic()->ident() );
    }
    return res;
}

const profession *scenario::weighted_random_profession() const
{
    // Strategy: 1/3 of the time, return the generic profession (if it's permitted).
    // Otherwise, the weight of each permitted profession is 2 / ( |point cost| + 2 )
    const auto choices = permitted_professions();
    if( one_in( 3 ) && choices.front() == profession::generic()->ident() ) {
        return profession::generic();
    }

    while( true ) {
        const string_id<profession> &candidate = random_entry_ref( choices );
        if( x_in_y( 2, 2 + std::abs( candidate->point_cost() ) ) ) {
            return &candidate.obj();
        }
    }
    return profession::generic(); // Suppress warnings
}

std::string scenario::prof_count_str() const
{
    if( professions.empty() ) {
        return _( "All" );
    }
    return blacklist ? _( "Almost all" ) : _( "Limited" );
}

std::string scenario::start_name() const
{
    return _start_name;
}

bool scenario::traitquery( const trait_id &trait ) const
{
    return _allowed_traits.count( trait ) != 0 || is_locked_trait( trait ) ||
           ( !is_forbidden_trait( trait ) && trait->startingtrait );
}

std::set<trait_id> scenario::get_locked_traits() const
{
    return _forced_traits;
}

bool scenario::is_locked_trait( const trait_id &trait ) const
{
    return _forced_traits.count( trait ) != 0;
}

bool scenario::is_forbidden_trait( const trait_id &trait ) const
{
    return _forbidden_traits.count( trait ) != 0;
}

bool scenario::has_flag( std::string flag ) const
{
    return flags.count( flag ) != 0;
}

bool scenario::allowed_start( const start_location_id &loc ) const
{
    auto &vec = _allowed_locs;
    return std::find( vec.begin(), vec.end(), loc ) != vec.end();
}

bool scenario::can_pick( int points ) const
{
    if( point_cost() - g->scen->point_cost() > points ) {
        return false;
    }

    return true;
}
bool scenario::has_map_special() const
{
    return _map_special != "mx_null";
}
const std::string &scenario::get_map_special() const
{
    return _map_special;
}
// vim:ts=4:sw=4:et:tw=0:fdm=marker:
