#include "skill_boost.h"

#include <cmath>
#include <algorithm>
#include <set>

#include "generic_factory.h"
#include "json.h"

namespace
{
generic_factory<skill_boost> all_skill_boosts( "skill boost", "stat" );
} // namespace

const std::vector<skill_boost> &skill_boost::get_all()
{
    return all_skill_boosts.get_all();
}

cata::optional<skill_boost> skill_boost::get( const std::string &stat_str )
{
    for( const skill_boost &boost : get_all() ) {
        if( boost.stat() == stat_str ) {
            return cata::optional<skill_boost>( boost );
        }
    }
    return cata::nullopt;
}

void skill_boost::load_boost( const JsonObject &jo, const std::string &src )
{
    all_skill_boosts.load( jo, src );
}

void skill_boost::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "skills_practice", _skills_practice );
    mandatory( jo, was_loaded, "skills_knowledge", _skills_knowledge );
    mandatory( jo, was_loaded, "coefficient", _coefficient );
    mandatory( jo, was_loaded, "skill_offset", _offset );
    mandatory( jo, was_loaded, "scaling_power", _power );
    mandatory( jo, was_loaded, "max_stat", _max_stat );
}

void skill_boost::reset()
{
    all_skill_boosts.reset();
}

std::string skill_boost::stat() const
{
    return id.str();
}

const std::vector<std::string> &skill_boost::skills_practice() const
{
    return _skills_practice;
}

const std::vector<std::string> &skill_boost::skills_knowledge() const
{
    return _skills_knowledge;
}

float skill_boost::calc_bonus( int exp_total ) const
{
    if( ( _coefficient * exp_total ) + _offset <= 0 ) {
        return 0.0;
    }
    return clamp( std::floor( std::pow( ( _coefficient * exp_total ) + _offset, _power ) ), 0.0f,
                  _max_stat );
}
