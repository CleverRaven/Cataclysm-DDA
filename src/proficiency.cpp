#include "proficiency.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>

#include "debug.h"
#include "generic_factory.h"
#include "json.h"

const float book_proficiency_bonus::default_time_factor = 0.5f;
const float book_proficiency_bonus::default_fail_factor = 0.5f;
const bool book_proficiency_bonus::default_include_prereqs = true;

namespace
{
generic_factory<proficiency> proficiency_factory( "proficiency" );
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

void proficiency::load_proficiencies( const JsonObject &jo, const std::string &src )
{
    proficiency_factory.load( jo, src );
}

void proficiency::reset()
{
    proficiency_factory.reset();
}

void proficiency::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "name", _name );
    mandatory( jo, was_loaded, "description", _description );
    mandatory( jo, was_loaded, "can_learn", _can_learn );

    optional( jo, was_loaded, "default_time_multiplier", _default_time_multiplier );
    optional( jo, was_loaded, "default_fail_multiplier", _default_fail_multiplier );
    optional( jo, was_loaded, "time_to_learn", _time_to_learn );
    optional( jo, was_loaded, "required_proficiencies", _required );
}

const std::vector<proficiency> &proficiency::get_all()
{
    return proficiency_factory.get_all();
}

bool proficiency::can_learn() const
{
    return _can_learn;
}

proficiency_id proficiency::prof_id() const
{
    return id;
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

float proficiency::default_fail_multiplier() const
{
    return _default_fail_multiplier;
}

time_duration proficiency::time_to_learn() const
{
    return _time_to_learn;
}

std::set<proficiency_id> proficiency::required_proficiencies() const
{
    return _required;
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
                                const cata::optional<time_duration> &max )
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
    for( const learning_proficiency &subject : learning ) {
        ret.push_back( subject.id );
    }
    return ret;
}

void proficiency_set::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "known", known );
    jsout.member( "learning", learning );

    jsout.end_object();
}

void proficiency_set::deserialize( JsonIn &jsin )
{
    JsonObject jsobj = jsin.get_object();

    jsobj.read( "known", known );
    jsobj.read( "learning", learning );
}

void proficiency_set::deserialize_legacy( const JsonArray &jo )
{
    for( const std::string prof : jo ) {
        known.insert( proficiency_id( prof ) );
    }
}

void learning_proficiency::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "id", id );
    jsout.member( "practiced", practiced );

    jsout.end_object();
}

void learning_proficiency::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "id", id );
    jo.read( "practiced", practiced );
}

void book_proficiency_bonus::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

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
