#include "proficiency.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>

#include "debug.h"
#include "generic_factory.h"
#include "json.h"
#include "localized_comparator.h"
#include "enums.h"
#include "options.h"

const float book_proficiency_bonus::default_time_factor = 0.5f;
const float book_proficiency_bonus::default_fail_factor = 0.5f;
const bool book_proficiency_bonus::default_include_prereqs = true;

namespace
{
generic_factory<proficiency> proficiency_factory( "proficiency" );
generic_factory<proficiency_category> proficiency_category_factory( "proficiency category" );
} // namespace

template<>
const proficiency &proficiency_id::obj() const
{
    return proficiency_factory.obj( *this );
}

template<>
bool proficiency_id::is_valid() const
{
    return proficiency_factory.is_valid( *this );
}

template<>
const proficiency_category &proficiency_category_id::obj() const
{
    return proficiency_category_factory.obj( *this );
}

template<>
bool proficiency_category_id::is_valid() const
{
    return proficiency_category_factory.is_valid( *this );
}

namespace io
{
template<>
std::string enum_to_string<proficiency_bonus_type>( const proficiency_bonus_type data )
{
    switch( data ) {
        // *INDENT-OFF*
        case proficiency_bonus_type::strength: return "strength";
        case proficiency_bonus_type::dexterity: return "dexterity";
        case proficiency_bonus_type::intelligence: return "intelligence";
        case proficiency_bonus_type::perception: return "perception";
        case proficiency_bonus_type::stamina: return "stamina";
        case proficiency_bonus_type::last: break;
        // *INDENT-ON*
    }

    debugmsg( "Invalid proficiency bonus type" );
    return "";
}
} // namespace io

void proficiency::load_proficiencies( const JsonObject &jo, const std::string &src )
{
    proficiency_factory.load( jo, src );
}

void proficiency_category::load_proficiency_categories( const JsonObject &jo,
        const std::string &src )
{
    proficiency_category_factory.load( jo, src );
}

void proficiency_bonus::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "type", type );
    mandatory( jo, false, "value", value );
}

void proficiency::reset()
{
    proficiency_factory.reset();
}

void proficiency_category::reset()
{
    proficiency_category_factory.reset();
}

void proficiency::load( const JsonObject &jo, const std::string_view )
{
    mandatory( jo, was_loaded, "name", _name );
    mandatory( jo, was_loaded, "description", _description );
    mandatory( jo, was_loaded, "can_learn", _can_learn );
    mandatory( jo, was_loaded, "category", _category );

    optional( jo, was_loaded, "default_time_multiplier", _default_time_multiplier );
    optional( jo, was_loaded, "default_skill_penalty", _default_skill_penalty );
    optional( jo, was_loaded, "default_weakpoint_bonus", _default_weakpoint_bonus );
    optional( jo, was_loaded, "default_weakpoint_penalty", _default_weakpoint_penalty );
    optional( jo, was_loaded, "time_to_learn", _time_to_learn );
    optional( jo, was_loaded, "required_proficiencies", _required );
    optional( jo, was_loaded, "ignore_focus", _ignore_focus );
    optional( jo, was_loaded, "teachable", _teachable, true );

    optional( jo, was_loaded, "bonuses", _bonuses );

    // TODO: Remove at some point
    if( jo.has_float( "default_fail_multiplier" ) ) {
        debugmsg( "Proficiency %s uses 'default_fail_multiplier' instead of 'default_skill_penalty'!",
                  id.c_str() );
        _default_skill_penalty = jo.get_float( "default_fail_multiplier" ) - 1.f;
    }
}

void proficiency_category::load( const JsonObject &jo, const std::string_view )
{
    mandatory( jo, was_loaded, "name", _name );
    mandatory( jo, was_loaded, "description", _description );
}

const std::vector<proficiency> &proficiency::get_all()
{
    return proficiency_factory.get_all();
}

const std::vector<proficiency_category> &proficiency_category::get_all()
{
    return proficiency_category_factory.get_all();
}

bool proficiency::can_learn() const
{
    if( _can_learn ) {
        const double scaling = get_option<float>( "PROFICIENCY_TRAINING_SPEED" );
        return scaling != 0.0;
    } else {
        return false;
    }
}

bool proficiency::ignore_focus() const
{
    return _ignore_focus;
}

bool proficiency::is_teachable() const
{
    return _teachable;
}

proficiency_id proficiency::prof_id() const
{
    return id;
}

proficiency_category_id proficiency::prof_category() const
{
    return _category;
}

std::string proficiency::name() const
{
    return _name.translated();
}

std::string proficiency::description() const
{
    return _description.translated();
}

float proficiency::default_time_multiplier() const
{
    return _default_time_multiplier;
}

float proficiency::default_skill_penalty() const
{
    return _default_skill_penalty;
}

float proficiency::default_weakpoint_bonus() const
{
    return _default_weakpoint_bonus;
}

float proficiency::default_weakpoint_penalty() const
{
    return _default_weakpoint_penalty;
}

time_duration proficiency::time_to_learn() const
{
    const double scaling = get_option<float>( "PROFICIENCY_TRAINING_SPEED" );
    if( scaling != 1.0 && scaling != 0.0 ) {
        return _time_to_learn / scaling;
    } else {
        return _time_to_learn;
    }
}

std::set<proficiency_id> proficiency::required_proficiencies() const
{
    return _required;
}

std::vector<proficiency_bonus> proficiency::get_bonuses( const std::string &category ) const
{
    auto bonus_it = _bonuses.find( category );
    if( bonus_it == _bonuses.end() ) {
        return std::vector<proficiency_bonus>();
    }
    return bonus_it->second;
}

std::optional<float> proficiency::bonus_for( const std::string &category,
        proficiency_bonus_type type ) const
{
    auto bonus_it = _bonuses.find( category );
    if( bonus_it == _bonuses.end() ) {
        return std::nullopt;
    }
    for( const proficiency_bonus &b : bonus_it->second ) {
        if( b.type == type ) {
            return b.value;
        }
    }

    return std::nullopt;
}

learning_proficiency &proficiency_set::fetch_learning( const proficiency_id &target )
{
    for( learning_proficiency &cursor : learning ) {
        if( cursor.id == target ) {
            return cursor;
        }
    }

    // This should _never_ happen
    debugmsg( "Uh-oh!  Requested proficiency that character does not know"
              " - expect crash or undefined behavior" );
    return learning[0];
}

std::vector<display_proficiency> proficiency_set::display() const
{
    // The proficiencies are sorted by whether or not you know them
    // and then alphabetically
    std::vector<std::pair<std::string, proficiency_id>> sorted_known;
    std::vector<std::pair<std::string, proficiency_id>> sorted_learning;

    for( const proficiency_id &cur : known ) {
        sorted_known.emplace_back( cur->name(), cur );
    }

    sorted_learning.reserve( learning.size() );
    for( const learning_proficiency &cur : learning ) {
        sorted_learning.emplace_back( cur.id->name(), cur.id );
    }

    std::sort( sorted_known.begin(), sorted_known.end(), localized_compare );
    std::sort( sorted_learning.begin(), sorted_learning.end(), localized_compare );

    // Our display_proficiencies, sorted in the order above
    std::vector<display_proficiency> ret;

    for( const std::pair<std::string, proficiency_id> &cur : sorted_known ) {
        display_proficiency disp;
        disp.id = cur.second;
        disp.color = c_white;
        disp.practice = 1.0f;
        disp.spent = cur.second->time_to_learn();
        disp.known = true;
        ret.insert( ret.end(), disp );
    }

    for( const std::pair<std::string, proficiency_id> &cur : sorted_learning ) {
        display_proficiency disp;
        disp.id = cur.second;
        disp.color = c_light_gray;
        time_duration practiced;
        for( const learning_proficiency &cursor : learning ) {
            if( cursor.id == cur.second ) {
                practiced = cursor.practiced;
                break;
            }
        }
        disp.spent = practiced;
        disp.practice = practiced / cur.second->time_to_learn();
        disp.known = false;
        ret.insert( ret.end(), disp );
    }

    return ret;
}

bool proficiency_set::practice( const proficiency_id &practicing, const time_duration &amount,
                                float remainder, const std::optional<time_duration> &max )
{
    if( has_learned( practicing ) || !practicing->can_learn() || !has_prereqs( practicing ) ) {
        return false;
    }
    if( !has_practiced( practicing ) ) {
        learning.emplace_back( practicing, 0_seconds );
    }

    learning_proficiency &current = fetch_learning( practicing );

    if( max && current.practiced > *max ) {
        return false;
    }

    current.practiced += amount;
    current.remainder += remainder;
    if( current.remainder > 1.f ) {
        current.practiced += 1_seconds;
        current.remainder -= 1.f;
    }

    if( current.practiced >= practicing->time_to_learn() ) {
        for( std::vector<learning_proficiency>::iterator it = learning.begin(); it != learning.end(); ) {
            if( it->id == practicing ) {
                it = learning.erase( it );
            } else {
                ++it;
            }
        }
        learn( practicing );

        return true;
    }

    return false;
}

void proficiency_set::set_time_practiced( const proficiency_id &practicing,
        const time_duration &amount )
{
    if( amount >= practicing->time_to_learn() ) {
        for( std::vector<learning_proficiency>::iterator it = learning.begin(); it != learning.end(); ) {
            if( it->id == practicing ) {
                it = learning.erase( it );
            } else {
                ++it;
            }
        }
        learn( practicing );
        return;
    } else if( known.count( practicing ) ) {
        remove( practicing );
    }
    if( !has_practiced( practicing ) ) {
        learning.emplace_back( practicing, 0_seconds );
    }
    learning_proficiency &current = fetch_learning( practicing );
    current.practiced = amount;
}

void proficiency_set::learn( const proficiency_id &learned )
{
    for( const proficiency_id &req : learned->required_proficiencies() ) {
        if( !has_learned( req ) ) {
            return;
        }
    }
    known.insert( learned );
}

// All the proficiencies in subjects that require one of the proficiencies in requirements
static std::set<proficiency_id> proficiencies_requiring(
    const std::set<proficiency_id> &requirements,
    const cata::flat_set<proficiency_id> &subjects )
{
    std::set<proficiency_id> ret;
    for( const proficiency_id &candidate : subjects ) {
        for( const proficiency_id &selector : requirements ) {
            if( candidate->required_proficiencies().count( selector ) ) {
                ret.insert( candidate );
                break;
            }
        }
    }
    return ret;
}

void proficiency_set::remove( const proficiency_id &lost )
{
    // Removes from proficiencies you are learning
    for( std::vector<learning_proficiency>::iterator it = learning.begin(); it != learning.end(); ) {
        if( it->id == lost ) {
            it = learning.erase( it );
        } else {
            ++it;
        }
    }

    // No unintended side effects
    if( !known.count( lost ) ) {
        return;
    }
    // Loop over all the proficiencies requiring lost, to remove them too
    // Then the ones requiring it, etc.
    std::set<proficiency_id> to_remove;
    to_remove.insert( lost );
    size_t cached_size = 0;
    // While we have new proficiencies we need to check for anything requiring them
    while( to_remove.size() != cached_size ) {
        cached_size = to_remove.size();
        std::set<proficiency_id> additional = proficiencies_requiring( to_remove, known );
        to_remove.insert( additional.begin(), additional.end() );
    }

    for( const proficiency_id &gone : to_remove ) {
        known.erase( gone );
    }
}

void proficiency_set::direct_learn( const proficiency_id &learned )
{
    // Player might be learning proficiency
    for( std::vector<learning_proficiency>::iterator it = learning.begin(); it != learning.end(); ) {
        if( it->id == learned ) {
            it = learning.erase( it );
        } else {
            ++it;
        }
    }

    known.insert( learned );
}

void proficiency_set::direct_remove( const proficiency_id &lost )
{
    known.erase( lost );
}

bool proficiency_set::has_learned( const proficiency_id &query ) const
{
    return known.count( query );
}

bool proficiency_set::has_practiced( const proficiency_id &query ) const
{
    for( const learning_proficiency &cursor : learning ) {
        if( cursor.id == query ) {
            return true;
        }
    }
    return false;
}

bool proficiency_set::has_prereqs( const proficiency_id &query ) const
{
    for( const proficiency_id &req : query->required_proficiencies() ) {
        if( !has_learned( req ) ) {
            return false;
        }
    }
    return true;
}

float proficiency_set::pct_practiced( const proficiency_id &query ) const
{
    for( const learning_proficiency &prof : learning ) {
        if( prof.id == query ) {
            return prof.practiced / query->time_to_learn();
        }
    }
    if( has_learned( query ) ) {
        return 1.0f;
    }
    return 0.0f;
}

time_duration proficiency_set::pct_practiced_time( const proficiency_id &query ) const
{
    for( const learning_proficiency &prof : learning ) {
        if( prof.id == query ) {
            return prof.practiced;
        }
    }
    if( has_learned( query ) ) {
        return query->time_to_learn();
    }
    return 0_seconds;
}

time_duration proficiency_set::training_time_needed( const proficiency_id &query ) const
{
    for( const learning_proficiency &prof : learning ) {
        if( prof.id == query ) {
            return query->time_to_learn() - prof.practiced;
        }
    }
    return query->time_to_learn();
}

std::vector<proficiency_id> proficiency_set::known_profs() const
{
    std::vector<proficiency_id> ret;
    for( const proficiency_id &knows : known ) {
        ret.push_back( knows );
    }
    return ret;
}

std::vector<proficiency_id> proficiency_set::learning_profs() const
{
    std::vector<proficiency_id> ret;
    ret.reserve( learning.size() );
    for( const learning_proficiency &subject : learning ) {
        ret.push_back( subject.id );
    }
    return ret;
}

float proficiency_set::get_proficiency_bonus( const std::string &category,
        proficiency_bonus_type prof_bonus ) const
{
    float stat_bonus = 0;

    for( const proficiency_id &knows : known ) {
        const std::vector<proficiency_bonus> &prof_bonuses = knows->get_bonuses( category );
        for( const proficiency_bonus &bonus : prof_bonuses ) {
            if( bonus.type == prof_bonus ) {
                stat_bonus += bonus.value;
            }
        }
    }
    return stat_bonus;
}

void proficiency_set::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "known", known );
    jsout.member( "learning", learning );

    jsout.end_object();
}

void proficiency_set::deserialize( const JsonObject &jsobj )
{
    jsobj.read( "known", known );
    jsobj.read( "learning", learning );
}

void learning_proficiency::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "id", id );
    jsout.member( "practiced", practiced );
    jsout.member( "remainder", remainder );

    jsout.end_object();
}

void learning_proficiency::deserialize( const JsonObject &jo )
{
    jo.read( "id", id );
    jo.read( "practiced", practiced );
    jo.read( "remainder", remainder, 0.f );
}

void book_proficiency_bonus::deserialize( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "proficiency", id );
    optional( jo, was_loaded, "fail_factor", fail_factor, default_fail_factor );
    optional( jo, was_loaded, "time_factor", time_factor, default_time_factor );
    optional( jo, was_loaded, "include_prereqs", include_prereqs, default_include_prereqs );
    if( fail_factor < 0 || fail_factor >= 1 ) {
        jo.throw_error( "fail_factor must be in range [0,1)" );
    }
    if( time_factor < 0 || time_factor >= 1 ) {
        jo.throw_error( "time_factor must be in range [0,1)" );
    }
}

void book_proficiency_bonuses::add( const book_proficiency_bonus &bonus )
{
    std::set<proficiency_id> ret;
    add( bonus, ret );
}

void book_proficiency_bonuses::add( const book_proficiency_bonus &bonus,
                                    std::set<proficiency_id> &already_included )
{
    bonuses.push_back( bonus );
    if( bonus.include_prereqs ) {
        for( const proficiency_id &prereqs : bonus.id->required_proficiencies() ) {
            if( !already_included.count( prereqs ) ) {
                already_included.emplace( prereqs );
                book_proficiency_bonus inherited_bonus = bonus;
                inherited_bonus.id = prereqs;
                add( inherited_bonus );
            }
        }
    }
}

book_proficiency_bonuses &book_proficiency_bonuses::operator+=( const book_proficiency_bonuses
        &rhs )
{
    for( const book_proficiency_bonus &bonus : rhs.bonuses ) {
        add( bonus );
    }
    return *this;
}

float book_proficiency_bonuses::fail_factor( const proficiency_id &id ) const
{
    double sum = 0;

    for( const book_proficiency_bonus &bonus : bonuses ) {
        if( id != bonus.id ) {
            continue;
        }
        sum += std::pow( std::log( 1.0 - bonus.fail_factor ), 2 );
    }

    return static_cast<float>( 1.0 - std::exp( -std::sqrt( sum ) ) );
}

float book_proficiency_bonuses::time_factor( const proficiency_id &id ) const
{
    double sum = 0;

    for( const book_proficiency_bonus &bonus : bonuses ) {
        if( id != bonus.id ) {
            continue;
        }
        sum += std::pow( std::log( 1.0 - bonus.time_factor ), 2 );
    }

    return static_cast<float>( 1.0 - std::exp( -std::sqrt( sum ) ) );
}
