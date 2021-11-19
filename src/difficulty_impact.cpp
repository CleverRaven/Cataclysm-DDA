#include <map>
#include <string>

#include "difficulty_impact.h"
#include "generic_factory.h"

namespace
{
generic_factory<difficulty_opt> difficulty_opt_factory( "difficulty option" );
generic_factory<difficulty_impact> difficulty_impact_factory( "difficulty impact" );
} // namespace

/*****************************************************************************
 * Difficulty options
 *****************************************************************************/

template<>
const difficulty_opt &difficulty_opt_id::obj() const
{
    return difficulty_opt_factory.obj( *this );
}

/** @relates string_id */
template<>
bool difficulty_opt_id::is_valid() const
{
    return difficulty_opt_factory.is_valid( *this );
}

void difficulty_opt::load_difficulty_opts( const JsonObject &jo, const std::string &src )
{
    difficulty_opt_factory.load( jo, src );
}

void difficulty_opt::reset()
{
    difficulty_opt_factory.reset();
}

const std::vector<difficulty_opt> &difficulty_opt::get_all()
{
    return difficulty_opt_factory.get_all();
}

void difficulty_opt::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "name", name_ );
    mandatory( jo, was_loaded, "value", value_ );
    optional( jo, was_loaded, "color", color_, "light_gray" );
}

int difficulty_opt::avg_value_ = 0;

void difficulty_opt::finalize()
{
    int val_total = 0;
    int val_count = 0;
    for( const difficulty_opt &opt : get_all() ) {
        val_total += opt.value();
        val_count++;
    }
    val_count = val_count > 0 ? val_count : 1;
    difficulty_opt::avg_value_ = std::round( val_total / static_cast<float>( val_count ) );
}

int difficulty_opt::avg_value()
{
    return difficulty_opt::avg_value_;
}

int difficulty_opt::value( const difficulty_opt_id &id )
{
    for( const difficulty_opt &opt : get_all() ) {
        if( opt.getId() == id ) {
            return opt.value();
        }
    }
    return 0;
}

difficulty_opt_id difficulty_opt::getId( int value )
{
    for( const difficulty_opt &opt : get_all() ) {
        if( opt.value() == value ) {
            return opt.getId();
        }
    }
    return difficulty_opt_id::NULL_ID();
}

/*****************************************************************************
 * Difficulty impacts
 *****************************************************************************/

template<>
const difficulty_impact &difficulty_impact_id::obj() const
{
    return difficulty_impact_factory.obj( *this );
}

/** @relates string_id */
template<>
bool difficulty_impact_id::is_valid() const
{
    return difficulty_impact_factory.is_valid( *this );
}

void difficulty_impact::load_difficulty_impacts( const JsonObject &jo, const std::string &src )
{
    difficulty_impact_factory.load( jo, src );
}

void difficulty_impact::reset()
{
    difficulty_impact_factory.reset();
}

const std::vector<difficulty_impact> &difficulty_impact::get_all()
{
    return difficulty_impact_factory.get_all();
}

void difficulty_impact::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "name", name_ );
    if( !jo.has_object( "weight" ) ) {
        jo.throw_error( "missing mandatory field \"weight\"" );
    } else {
        float readr;
        JsonObject jobj = jo.get_object( "weight" );
        mandatory( jobj, was_loaded, "scenario", readr );
        weight_.emplace( SCENARIO, readr );
        mandatory( jobj, was_loaded, "profession", readr );
        weight_.emplace( PROFFESION, readr );
        mandatory( jobj, was_loaded, "hobby", readr );
        weight_.emplace( HOBBY, readr );
        mandatory( jobj, was_loaded, "mutation", readr );
        weight_.emplace( MUTATION, readr );
    }
}

float difficulty_impact::weight( difficulty_source src ) const
{
    auto w = weight_.find( src );
    return w != weight_.end() ? w->second : 0.0f;
}