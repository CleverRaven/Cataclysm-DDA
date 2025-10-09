#include "wound.h"

#include "bodypart.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "json.h"
#include "rng.h"

enum class bp_type;

void wound_type::load( const JsonObject &jo, const std::string_view & )
{
    name_.make_plural();
    mandatory( jo, was_loaded, "name", name_ );
    mandatory( jo, was_loaded, "description", description_ );
    optional( jo, was_loaded, "pain", pain_, std::make_pair( 0, 0 ) );
    optional( jo, was_loaded, "healing_time", healing_time_,
              std::make_pair( calendar::INDEFINITELY_LONG_DURATION, calendar::INDEFINITELY_LONG_DURATION ) );

    mandatory( jo, was_loaded, "damage_types", damage_types );
    mandatory( jo, was_loaded, "damage_required", damage_required );
    optional( jo, was_loaded, "weight", weight, 1 );

    optional( jo, was_loaded, "whitelist_bp_with_flag", whitelist_bp_with_flag );
    optional( jo, was_loaded, "whitelist_body_part_types", whitelist_body_part_types );
    optional( jo, was_loaded, "blacklist_bp_with_flag", blacklist_bp_with_flag );
    optional( jo, was_loaded, "blacklist_body_part_types", blacklist_body_part_types );
}

wound::wound( wound_type_id wd ) :
    type( wd ),
    healing_time( wd->evaluate_healing_time() ),
    pain( wd->evaluate_pain() )
{}

namespace
{
generic_factory<wound_type> wound_type_factory( "wound" );
} // namespace

void wound_type::load_wounds( const JsonObject &jo, const std::string &src )
{
    wound_type_factory.load( jo, src );
}

/** @relates string_id */
template<>
const wound_type &string_id<wound_type>::obj() const
{
    return wound_type_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<wound_type>::is_valid() const
{
    return wound_type_factory.is_valid( *this );
}

void wound_type::reset()
{
    wound_type_factory.reset();
}

void wound_type::check_consistency()
{
    wound_type_factory.check();
}

void wound_type::check() const
{

}

const std::vector<wound_type> &wound_type::get_all()
{
    return wound_type_factory.get_all();
}

void wound_type::finalize_all()
{
    wound_type_factory.finalize();
}

void wound_type::finalize()
{

}

bool wound_type::allowed_on_bodypart( bodypart_str_id bp_id ) const
{

    // doesn't have bp type we want
    for( const bp_type &bp_type : whitelist_body_part_types ) {
        if( !bp_id->has_type( bp_type ) ) {
            return false;
        }
    }

    // has no flag we want
    if( !bp_id->has_flag( whitelist_bp_with_flag ) &&
        !whitelist_bp_with_flag.is_empty() ) {
        return false;
    }

    // has type we do not want
    for( const bp_type &bp_type : blacklist_body_part_types ) {
        if( bp_id->has_type( bp_type ) ) {
            return false;
        }
    }

    // has flag we do not want
    if( bp_id->has_flag( blacklist_bp_with_flag ) ) {
        return false;
    }

    return true;
}

int wound_type::evaluate_pain() const
{
    return rng( pain_.first, pain_.second );
}

time_duration wound_type::evaluate_healing_time() const
{
    return rng( healing_time_.first, healing_time_.second );
}

std::string wound_type::get_name() const
{
    return name_.translated();
}

std::string wound_type::get_description() const
{
    return description_.translated();
}

void wound::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "type", type );
    jsout.member( "healing_time", healing_time );
    jsout.member( "healing_progress", healing_progress );
    jsout.member( "pain", pain );
    jsout.end_object();
}

void wound::deserialize( const JsonObject &jsin )
{
    jsin.read( "type", type );
    jsin.read( "healing_time", healing_time );
    jsin.read( "healing_progress", healing_progress );
    jsin.read( "pain", pain );
}

float wound::healing_percentage() const
{
    return healing_progress / healing_time;
}

int wound::get_pain() const
{
    return pain * ( 1 - healing_percentage() );
}

time_duration wound::get_healing_time() const
{
    return healing_time;
}

bool wound::update_wound( const time_duration time_passed )
{
    healing_progress += time_passed;
    return healing_progress >= healing_time;
}
