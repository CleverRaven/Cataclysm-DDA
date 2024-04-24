#include "scenario.h"

#include <algorithm>
#include <cstdlib>

#include "achievement.h"
#include "debug.h"
#include "generic_factory.h"
#include "json.h"
#include "map_extras.h"
#include "mission.h"
#include "mutation.h"
#include "options.h"
#include "past_games_info.h"
#include "past_achievements_info.h"
#include "profession.h"
#include "rng.h"
#include "start_location.h"
#include "string_id.h"
#include "translations.h"

static const achievement_id achievement_achievement_arcade_mode( "achievement_arcade_mode" );

namespace
{
generic_factory<scenario> all_scenarios( "scenario" );
} // namespace

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

static scen_blacklist sc_blacklist;

scenario::scenario()
// NOLINTNEXTLINE(cata-static-string_id-constants)
    : id( "" ), _name_male( no_translation( "null" ) ),
      _name_female( no_translation( "null" ) ),
      _description_male( no_translation( "null" ) ),
      _description_female( no_translation( "null" ) )
{
}

void scenario::load_scenario( const JsonObject &jo, const std::string &src )
{
    all_scenarios.load( jo, src );
}

void scenario::load( const JsonObject &jo, const std::string_view )
{
    // TODO: pretty much the same as in profession::load, but different contexts for pgettext.
    // TODO: maybe combine somehow?
    if( !was_loaded || jo.has_string( "name" ) ) {
        // These may differ depending on the language settings!
        const std::string name = jo.get_string( "name" );
        _name_female = to_translation( "scenario_female", name );
        _name_male = to_translation( "scenario_male", name );
    }

    if( !was_loaded || jo.has_string( "description" ) ) {
        // These also may differ depending on the language settings!
        std::string desc;
        mandatory( jo, false, "description", desc, text_style_check_reader() );
        _description_male = to_translation( "scen_desc_male", desc );
        _description_female = to_translation( "scen_desc_female", desc );
    }

    if( !was_loaded || jo.has_string( "start_name" ) ) {
        // Specifying translation context here and above to avoid adding unnecessary json code for every scenario
        // NOLINTNEXTLINE(cata-json-translation-input)
        _start_name = to_translation( "start_name", jo.get_string( "start_name" ) );
    }

    mandatory( jo, was_loaded, "points", _point_cost );

    optional( jo, was_loaded, "blacklist_professions", blacklist );
    optional( jo, was_loaded, "add_professions", extra_professions );
    optional( jo, was_loaded, "professions", professions, string_id_reader<::profession> {} );

    optional( jo, was_loaded, "hobbies", hobby_exclusion );
    optional( jo, was_loaded, "whitelist_hobbies", hobbies_whitelist, true );

    optional( jo, was_loaded, "traits", _allowed_traits, string_id_reader<::mutation_branch> {} );
    optional( jo, was_loaded, "forced_traits", _forced_traits, string_id_reader<::mutation_branch> {} );
    optional( jo, was_loaded, "forbidden_traits", _forbidden_traits,
              string_id_reader<::mutation_branch> {} );
    optional( jo, was_loaded, "allowed_locs", _allowed_locs, string_id_reader<::start_location> {} );
    if( _allowed_locs.empty() ) {
        jo.throw_error( "at least one starting location (member \"allowed_locs\") must be defined" );
    }
    optional( jo, was_loaded, "flags", flags, auto_flags_reader<> {} );
    optional( jo, was_loaded, "map_extra", _map_extra, map_extra_id::NULL_ID() );
    optional( jo, was_loaded, "missions", _missions, string_id_reader<::mission_type> {} );

    optional( jo, was_loaded, "requirement", _requirement );

    optional( jo, was_loaded, "reveal_locale", reveal_locale, true );

    optional( jo, was_loaded, "eoc", _eoc, auto_flags_reader<effect_on_condition_id> {} );

    if( !was_loaded ) {

        int _start_of_cataclysm_hour = 0;
        int _start_of_cataclysm_day = 61;
        season_type _start_of_cataclysm_season = SPRING;
        int _start_of_cataclysm_year = 1;
        if( jo.has_member( "start_of_cataclysm" ) ) {
            JsonObject jocid = jo.get_member( "start_of_cataclysm" );
            optional( jocid, was_loaded, "hour", _start_of_cataclysm_hour );
            optional( jocid, was_loaded, "day", _start_of_cataclysm_day );
            optional( jocid, was_loaded, "season", _start_of_cataclysm_season );
            optional( jocid, was_loaded, "year", _start_of_cataclysm_year );
        }
        _default_start_of_cataclysm = calendar::turn_zero +
                                      1_hours * _start_of_cataclysm_hour +
                                      1_days * ( _start_of_cataclysm_day - 1 ) +
                                      1_days * get_option<int>( "SEASON_LENGTH" ) * _start_of_cataclysm_season +
                                      calendar::year_length() * ( _start_of_cataclysm_year - 1 )
                                      ;

        int _start_of_game_hour = 8;
        int _start_of_game_day = 61;
        season_type _start_of_game_season = SPRING;
        int _start_of_game_year = 1;
        if( jo.has_member( "start_of_game" ) ) {
            JsonObject jocid = jo.get_member( "start_of_game" );
            optional( jocid, was_loaded, "hour", _start_of_game_hour );
            optional( jocid, was_loaded, "day", _start_of_game_day );
            optional( jocid, was_loaded, "season", _start_of_game_season );
            optional( jocid, was_loaded, "year", _start_of_game_year );
        }
        _default_start_of_game = calendar::turn_zero +
                                 1_hours * _start_of_game_hour +
                                 1_days * ( _start_of_game_day - 1 ) +
                                 1_days * get_option<int>( "SEASON_LENGTH" ) * _start_of_game_season +
                                 calendar::year_length() * ( _start_of_game_year - 1 )
                                 ;

        reset_calendar();
    }

    if( jo.has_string( "vehicle" ) ) {
        _starting_vehicle = vproto_id( jo.get_string( "vehicle" ) );
    }

    for( JsonArray ja : jo.get_array( "surround_groups" ) ) {
        _surround_groups.emplace_back( mongroup_id( ja.get_string( 0 ) ),
                                       static_cast<float>( ja.get_float( 1 ) ) );
    }
}

const scenario *scenario::generic()
{
    static const string_id<scenario> generic_scenario_id(
        get_option<std::string>( "GENERIC_SCENARIO_ID" ) );

    std::vector<const scenario *> all;
    for( const scenario &scen : scenario::get_all() ) {
        if( scen.scen_is_blacklisted() ) {
            continue;
        }
        all.push_back( &scen );
    }
    if( find_if( all.begin(), all.end(), []( const scenario * s ) {
    return s->ident() == generic_scenario_id;
    } ) != all.end() ) {
        // if the default scenario exists return it
        return &generic_scenario_id.obj();
    }

    // if generic doesn't exist just return to the first scenario
    return *all.begin();

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
        const scenario &scen = random_entry_ref( list );

        if( x_in_y( 2, std::abs( scen.point_cost() ) + 2 ) ) {
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

void scenario::finalize()
{
    for( const scenario &scen : all_scenarios.get_all() ) {
        scen.check_definition();
    }
    sc_blacklist.finalize();
}

static void check_traits( const std::set<trait_id> &traits, const string_id<scenario> &ident )
{
    for( const auto &t : traits ) {
        if( !t.is_valid() ) {
            debugmsg( "trait %s for scenario %s does not exist", t.c_str(), ident.c_str() );
        }
    }
}

void scenario::check_definition() const
{
    for( const auto &p : professions ) {
        if( !p.is_valid() ) {
            debugmsg( "profession %s for scenario %s does not exist", p.c_str(), id.c_str() );
        }
    }

    for( const string_id<profession> &hobby : hobby_exclusion ) {
        if( !hobby.is_valid() ) {
            debugmsg( "hobby %s for scenario %s does not exist", hobby.str(), id.str() );
        } else if( !hobby->is_hobby() ) {
            debugmsg( "hobby %s for scenario %s is a profession", hobby.str(), id.str() );
        }
    }

    std::unordered_set<string_id<profession>> professions_set;
    for( const auto &p : professions ) {
        if( professions_set.count( p ) == 1 ) {
            debugmsg( "Duplicate profession %s in scenario %s.", p.c_str(), this->id.c_str() );
        } else {
            professions_set.insert( p );
        }
    }

    for( const start_location_id &l : _allowed_locs ) {
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

    if( !_map_extra.is_valid() )  {
        debugmsg( "there is no map extra with id %s", _map_extra.str() );
    }

    for( const auto &e : eoc() ) {
        if( !e.is_valid() ) {
            debugmsg( "effect on condition %s for scenario %s does not exist", e.c_str(), id.c_str() );
        }
    }

    for( const auto &m : _missions ) {
        if( !m.is_valid() ) {
            debugmsg( "starting mission %s for scenario %s does not exist", m.c_str(), id.c_str() );
        }

        if( std::find( m->origins.begin(), m->origins.end(), ORIGIN_GAME_START ) == m->origins.end() ) {
            debugmsg( "starting mission %s for scenario %s must include an origin of ORIGIN_GAME_START",
                      m.c_str(), id.c_str() );
        }
    }

    if( _starting_vehicle && !_starting_vehicle.is_valid() ) {
        debugmsg( "vehicle prototype %s for profession %s does not exist", _starting_vehicle.c_str(),
                  id.c_str() );
    }
}

const string_id<scenario> &scenario::ident() const
{
    return id;
}

std::string scenario::gender_appropriate_name( bool male ) const
{
    if( male ) {
        return _name_male.translated();
    } else {
        return _name_female.translated();
    }
}

std::string scenario::description( bool male ) const
{
    if( male ) {
        return _description_male.translated();
    } else {
        return _description_female.translated();
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

bool scenario::scen_is_blacklisted() const
{
    return sc_blacklist.scenarios.count( id ) != 0;
}

void scen_blacklist::load_scen_blacklist( const JsonObject &jo, const std::string_view src )
{
    sc_blacklist.load( jo, src );
}

void scen_blacklist::load( const JsonObject &jo, const std::string_view )
{
    if( !scenarios.empty() ) {
        DebugLog( D_INFO, DC_ALL ) <<
                                   "Attempted to load scenario blacklist with an existing scenario blacklist, resetting blacklist info";
        reset_scenarios_blacklist();
    }

    const std::string bl_stype = jo.get_string( "subtype" );

    if( bl_stype == "whitelist" ) {
        whitelist = true;
    } else if( bl_stype == "blacklist" ) {
        whitelist = false;
    } else {
        jo.throw_error( "Blacklist subtype is not a valid subtype." );
    }

    for( const std::string line : jo.get_array( "scenarios" ) ) {
        scenarios.emplace( line );
    }
}

void scen_blacklist::finalize()
{
    std::vector<string_id<scenario>> all_scens;
    for( const scenario &scen : scenario::get_all() ) {
        all_scens.emplace_back( scen.ident() );
    }
    for( const string_id<scenario> &sc : sc_blacklist.scenarios ) {
        if( std::find( all_scens.begin(), all_scens.end(), sc ) == all_scens.end() ) {
            debugmsg( "Scenario blacklist contains invalid scenario" );
        }
    }

    if( sc_blacklist.whitelist ) {
        std::set<string_id<scenario>> listed_scenarios = sc_blacklist.scenarios;
        sc_blacklist.scenarios.clear();
        for( const scenario &scen : scenario::get_all() ) {
            sc_blacklist.scenarios.insert( scen.ident() );
        }
        for( auto i = sc_blacklist.scenarios.begin(); i != sc_blacklist.scenarios.end(); ) {
            if( listed_scenarios.count( *i ) != 0 ) {
                i = sc_blacklist.scenarios.erase( i );
            } else {
                ++i;
            }
        }
    }
}

void reset_scenarios_blacklist()
{
    sc_blacklist.scenarios.clear();
    sc_blacklist.whitelist = false;
}

std::vector<string_id<profession>> scenario::permitted_professions() const
{
    if( !cached_permitted_professions.empty() ) {
        return cached_permitted_professions;
    }

    const std::vector<profession> &all = profession::get_all();
    std::vector<string_id<profession>> &res = cached_permitted_professions;
    for( const profession &p : all ) {
        if( p.is_hobby() || p.is_blacklisted() ) {
            continue;
        }
        const bool present = std::find( professions.begin(), professions.end(),
                                        p.ident() ) != professions.end();

        bool conflicting_traits = scenario_traits_conflict_with_profession_traits( p );

        if( blacklist || professions.empty() ) {
            if( !present && !p.has_flag( "SCEN_ONLY" ) && !conflicting_traits ) {
                res.push_back( p.ident() );
            }
        } else if( present ) {
            if( !conflicting_traits ) {
                res.push_back( p.ident() );
            } else {
                debugmsg( "Scenario %s and profession %s have conflicting trait requirements",
                          id.c_str(), p.ident().c_str() );
            }
        } else if( extra_professions ) {
            if( !p.has_flag( "SCEN_ONLY" ) && !conflicting_traits ) {
                res.push_back( p.ident() );
            }
        }
    }

    if( res.empty() ) {
        debugmsg( "Why would you blacklist every profession?" );
        res.push_back( profession::generic()->ident() );
    }
    return res;
}

std::vector<string_id<profession>> scenario::permitted_hobbies() const
{
    if( !cached_permitted_hobbies.empty() ) {
        return cached_permitted_hobbies;
    }

    std::vector<string_id<profession>> all = profession::get_all_hobbies();
    std::vector<string_id<profession>> &res = cached_permitted_hobbies;
    for( const string_id<profession> &hobby : all ) {
        if( hobby->is_blacklisted() ) {
            continue;
        }
        if( scenario_traits_conflict_with_profession_traits( *hobby ) ) {
            continue;
        }
        if( !hobbies_whitelist && hobby_exclusion.count( hobby ) != 0 ) {
            continue;
        }
        if( hobbies_whitelist && !hobby_exclusion.empty() && hobby_exclusion.count( hobby ) == 0 ) {
            continue;
        }

        res.push_back( hobby );
    }

    if( res.empty() ) {
        debugmsg( "Why would you blacklist every hobby?" );
        res.insert( res.end(), all.begin(), all.end() );
    }

    return res;
}

bool scenario::scenario_traits_conflict_with_profession_traits( const profession &p ) const
{
    for( const auto &pt : p.get_forbidden_traits() ) {
        if( is_locked_trait( pt ) ) {
            return true;
        }
    }

    for( trait_and_var &pt : p.get_locked_traits() ) {
        if( is_forbidden_trait( pt.trait ) ) {
            return true;
        }
    }

    //  check if:
    //  locked traits for scenario prevent taking locked traits for professions
    //  locked traits for professions prevent taking locked traits for scenario
    for( const trait_id &st : get_locked_traits() ) {
        for( trait_and_var &pt : p.get_locked_traits() ) {
            if( are_conflicting_traits( st, pt.trait ) || are_conflicting_traits( pt.trait, st ) ) {
                return true;
            }
        }
    }
    return false;
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
        if( candidate->can_pick().success() && x_in_y( 2, 2 + std::abs( candidate->point_cost() ) ) ) {
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
    return _start_name.translated();
}

int scenario::start_location_count() const
{
    return _allowed_locs.size();
}

int scenario::start_location_targets_count() const
{
    int cnt = 0;
    for( const auto &sloc : _allowed_locs ) {
        cnt += sloc.obj().targets_count();
    }
    return cnt;
}

std::optional<achievement_id> scenario::get_requirement() const
{
    return _requirement;
}

bool scenario::get_reveal_locale() const
{
    return reveal_locale;
}

void scenario::normalize_calendar() const
{
    scenario *hack = const_cast<scenario *>( this );
    // We don't currently allow to start game before cataclysm
    if( hack->_default_start_of_game < hack->_default_start_of_cataclysm ) {
        hack->_default_start_of_game = hack->_default_start_of_cataclysm;
    }
    if( hack->_start_of_game < hack->_start_of_cataclysm ) {
        hack->_start_of_game = hack->_start_of_cataclysm;
    }
}

void scenario::reset_calendar() const
{
    scenario *hack = const_cast<scenario *>( this );
    hack->_start_of_cataclysm = _default_start_of_cataclysm;
    hack->_start_of_game = _default_start_of_game;
    hack->normalize_calendar();
}

void scenario::change_start_of_cataclysm( const time_point &t ) const
{
    scenario *hack = const_cast<scenario *>( this );
    hack->_start_of_cataclysm = t;
    hack->normalize_calendar();
}

void scenario::change_start_of_game( const time_point &t ) const
{
    scenario *hack = const_cast<scenario *>( this );
    hack->_start_of_game = t;
    hack->normalize_calendar();
}

time_point scenario::start_of_cataclysm() const
{
    return _start_of_cataclysm;
}

time_point scenario::start_of_game() const
{
    return _start_of_game;
}

vproto_id scenario::vehicle() const
{
    return _starting_vehicle;
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

bool scenario::has_flag( const std::string &flag ) const
{
    return flags.count( flag ) != 0;
}

bool scenario::allowed_start( const start_location_id &loc ) const
{
    const auto &vec = _allowed_locs;
    return std::find( vec.begin(), vec.end(), loc ) != vec.end();
}

ret_val<void> scenario::can_afford( const scenario &current_scenario, const int points ) const
{
    if( point_cost() - current_scenario.point_cost() <= points ) {
        return ret_val<void>::make_success();

    }

    return ret_val<void>::make_failure( _( "You don't have enough points" ) );
}

ret_val<void> scenario::can_pick() const
{
    // if meta progression is disabled then skip this
    if( get_past_achievements().is_completed( achievement_achievement_arcade_mode ) ||
        !get_option<bool>( "META_PROGRESS" ) ) {
        return ret_val<void>::make_success();
    }

    if( _requirement ) {
        const bool has_req = get_past_achievements().is_completed(
                                 _requirement.value()->id );
        if( !has_req ) {
            return ret_val<void>::make_failure(
                       _( "You must complete the achievement \"%s\" to unlock this scenario." ),
                       _requirement.value()->name() );
        }
    }

    return ret_val<void>::make_success();
}

bool scenario::has_map_extra() const
{
    return !_map_extra.is_null();
}
const map_extra_id &scenario::get_map_extra() const
{
    return _map_extra;
}
const std::vector<mission_type_id> &scenario::missions() const
{
    return _missions;
}
const std::vector<effect_on_condition_id> &scenario::eoc() const
{
    return _eoc;
}
const std::vector<std::pair<mongroup_id, float>> &scenario::surround_groups() const
{
    return _surround_groups;
}
// vim:ts=4:sw=4:et:tw=0:fdm=marker:
