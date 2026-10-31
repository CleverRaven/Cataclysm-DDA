#include "wound.h"

#include <algorithm>
#include <memory>
#include <set>

#include "bodypart.h"
#include "character.h"
#include "damage.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "json.h"
#include "magic_enchantment.h"
#include "messages.h"
#include "options.h"
#include "pimpl.h"
#include "rng.h"

enum class bp_type;
template <typename E> struct enum_traits;

static const vitamin_id vitamin_blood( "blood" );
static const vitamin_id vitamin_redcells( "redcells" );

void wound_type::load( const JsonObject &jo, const std::string_view & )
{
    name_.make_plural();
    mandatory( jo, was_loaded, "name", name_ );
    mandatory( jo, was_loaded, "description", description_ );
    optional( jo, was_loaded, "pain", pain_, std::make_pair( 0, 0 ) );
    optional( jo, was_loaded, "healing_time", healing_time_,
              std::make_pair( calendar::INDEFINITELY_LONG_DURATION, calendar::INDEFINITELY_LONG_DURATION ) );

    optional( jo, was_loaded, "damage_types", damage_types );
    optional( jo, was_loaded, "damage_required", damage_required );
    optional( jo, was_loaded, "weight", weight, 1 );
    optional( jo, was_loaded, "limb_scores", limb_scores );
    optional( jo, was_loaded, "limit", limit, 0 );

    optional( jo, was_loaded, "mod_values", mod_values );
    optional( jo, was_loaded, "periodic_message", periodic_message );
    optional( jo, was_loaded, "wound_progression", wound_progression );
    optional( jo, was_loaded, "progress_timer", progress_timer );
    optional( jo, was_loaded, "whitelist_flags", whitelist_flags );
    optional( jo, was_loaded, "blacklist_flag", blacklist_flags );
    optional( jo, was_loaded, "whitelist_bp_with_flag", whitelist_bp_with_flag );
    optional( jo, was_loaded, "whitelist_body_part_types", whitelist_body_part_types );
    optional( jo, was_loaded, "blacklist_bp_with_flag", blacklist_bp_with_flag );
    optional( jo, was_loaded, "blacklist_body_part_types", blacklist_body_part_types );
}

wound::wound( wound_type_id wd ) :
    type( wd ),
    healing_time( wd->evaluate_healing_time() ),
    pain( wd->evaluate_pain() ),
    wound_progress_timer( wd->evaluate_progress_timer() )
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
    _is_bleeding_wound = assign_bleeding_wound();
}

namespace
{
generic_factory<wound_fix> wound_fix_factory( "wound_fix" );
} // namespace

void wound_fix::load_wound_fixes( const JsonObject &jo, const std::string &src )
{
    wound_fix_factory.load( jo, src );
}

std::string wound_fix::get_name() const
{
    return name.translated();
}

std::string wound_fix::get_description() const
{
    return description.translated();
}

/** @relates string_id */
template<>
const wound_fix &string_id<wound_fix>::obj() const
{
    return wound_fix_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<wound_fix>::is_valid() const
{
    return wound_fix_factory.is_valid( *this );
}

void wound_fix::reset()
{
    wound_fix_factory.reset();
}

void wound_fix::check_consistency()
{
    wound_fix_factory.check();
}

void wound_fix::check() const
{
    for( const auto &[skill_id, lvl] : skills ) {
        if( !skill_id.is_valid() ) {
            debugmsg( "wound_fix %s requires unknown skill '%s'", id.str(), skill_id.str() );
        }
    }
    for( const wound_type_id &wd_id : wounds_removed ) {
        if( !wd_id.is_valid() ) {
            debugmsg( "wound_fix '%s' has invalid wound id '%s' in 'wounds_removed' field",
                      id.str(), wd_id.str() );
        }
    }
    for( const wound_type_id &wd_id : wounds_added ) {
        if( !wd_id.is_valid() ) {
            debugmsg( "wound_fix '%s' has invalid wound id '%s' in 'wounds_added' field",
                      id.str(), wd_id.str() );
        }
    }

    for( const wound_proficiency &wound_prof : proficiencies ) {
        if( !wound_prof.prof.is_valid() ) {
            debugmsg( "wound_fix '%s' has invalid proficiency '%s' in 'proficiencies' field",
                      id.str(), wound_prof.prof.str() );
        }
    }
}

void wound_fix::finalize_all()
{
    wound_fix_factory.finalize();
}

template<>
struct enum_traits<wound_mod_value_type> {
    static constexpr wound_mod_value_type last = wound_mod_value_type::LAST;
};

namespace io
{
template<>
std::string enum_to_string<wound_mod_value_type>( wound_mod_value_type data )
{
    switch( data ) {
            // *INDENT-OFF*
        case wound_mod_value_type::PAIN: return "PAIN";
        case wound_mod_value_type::BLEED: return "BLEED";
        case wound_mod_value_type::VITAMIN: return "VITAMIN";

            // *INDENT-ON*
        case wound_mod_value_type::LAST:
            break;
    }
    cata_fatal( "Invalid character_stat" );
}
} // namespace io

void wound_fix::finalize()
{
    for( const std::pair<requirement_id, int> &p : pending_requirements ) {
        const requirement_id req_id = p.first;
        const int amount = p.second;
        if( !req_id.is_valid() ) {
            debugmsg( "wound_fix '%s' has invalid requirement_id '%s'", id.str(), req_id.str() );
            continue;
        }
        *requirements = *requirements + ( *req_id ) * amount;
    }
    pending_requirements.clear();
    requirements->consolidate();
    requirement_data::finalize();

    for( const wound_type_id &fid : wounds_removed ) {
        const_cast<wound_type &>( *fid ).fixes.emplace( id );
    }
}

void wound_limb_score::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "score", score );
    optional( jo, false, "value", value, 0.f );
}

void wound_progress::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "id", id );
    optional( jo, false, "chance", chance, numeric_bound_reader<int> {0, 100} );
}

void wound_proficiency::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "proficiency", prof );
    optional( jo, false, "time_save", time_save, 1.f );
    optional( jo, false, "is_mandatory", is_mandatory, false );
}

void wound_type_progress_timer::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "id", id );
    mandatory( jo, false, "time", time );
    optional( jo, false, "message", message );
}

void wound_mod_values::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "type", type );
    mandatory( jo, false, "rate", rate );
    optional( jo, false, "vitamin_id", vitamin );
    optional( jo, false, "amount", amount );
    optional( jo, false, "cap_min", cap_min, INT_MIN );
    optional( jo, false, "cap_max", cap_max, INT_MAX );
}

void wound_periodic_message::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "message", message );
    mandatory( jo, false, "rate", rate );
}

void wound_fix::load( const JsonObject &jo, const std::string_view & )
{
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "description", description );
    optional( jo, was_loaded, "success_msg", success_msg );
    optional( jo, was_loaded, "mod_hp", mod_hp, 0 );
    optional( jo, was_loaded, "time", time );
    optional( jo, was_loaded, "skills", skills );
    optional( jo, was_loaded, "wounds_removed", wounds_removed );
    optional( jo, was_loaded, "wounds_added", wounds_added );

    if( jo.has_array( "proficiencies" ) ) {
        proficiencies.clear();
        for( JsonObject job : jo.get_array( "proficiencies" ) ) {
            wound_proficiency wound_prof;
            wound_prof.deserialize( job );
            proficiencies.emplace_back( wound_prof );
        }
    }

    if( jo.has_array( "requirements" ) ) {
        pending_requirements.clear();
        for( const JsonValue &jv : jo.get_array( "requirements" ) ) {
            if( jv.test_array() ) {
                // array of 2 elements filled with [requirement_id, count]
                const JsonArray &req = static_cast<JsonArray>( jv );
                const requirement_id req_id( req.get_string( 0 ) );
                const int req_amount = req.get_int( 1 );
                pending_requirements.emplace_back( req_id, req_amount );
            } else if( jv.test_object() ) {
                // defining single requirement inline
                const JsonObject &req = static_cast<JsonObject>( jv );
                const requirement_id req_id( "wound_fix_" + id.str() + "_inline_req" );
                requirement_data::load_requirement( req, req_id );
                pending_requirements.emplace_back( req_id, 1 );
            } else {
                debugmsg( "wound_fix '%s' has has invalid requirement element", id.str() );
            }
        }
    }
}

const requirement_data &wound_fix::get_requirements() const
{
    return *requirements;
}

bool wound_type::allowed_on_bodypart( bodypart_str_id bp_id ) const
{

    // doesn't have bp type we want
    bool is_whitelisted = false;
    for( const bp_type &bp_type : whitelist_body_part_types ) {
        is_whitelisted = is_whitelisted || bp_id->has_type( bp_type );
    }
    if( !is_whitelisted && !whitelist_body_part_types.empty() ) {
        return false;
    }

    // has no flag we want
    is_whitelisted = false;
    for( const json_character_flag &bp_flag : whitelist_bp_with_flag ) {
        is_whitelisted = is_whitelisted || bp_id->has_flag( bp_flag );
    }
    if( !is_whitelisted && !whitelist_bp_with_flag.empty() ) {
        return false;
    }

    // has type we do not want
    for( const bp_type &bp_type : blacklist_body_part_types ) {
        if( bp_id->has_type( bp_type ) ) {
            return false;
        }
    }

    // has flag we do not want
    for( const json_character_flag &bp_flag : blacklist_bp_with_flag ) {
        if( bp_id->has_flag( bp_flag ) ) {
            return false;
        }
    }

    return true;
}

bool wound_type::assign_bleeding_wound() const
{
    for( const wound_mod_values &mv : mod_values ) {
        if( mv.vitamin.has_value() ) {
            const vitamin_id vit = *mv.vitamin;
            if( vit == vitamin_blood || vit == vitamin_redcells ) {
                return true;
            }
        }
    }

    return false;
}

bool wound_type::is_bleeding_wound() const
{
    return _is_bleeding_wound;
}

int wound_type::evaluate_pain() const
{
    return rng( pain_.first, pain_.second );
}

time_duration wound_type::evaluate_healing_time() const
{
    return rng( healing_time_.first, healing_time_.second );
}

std::optional<std::pair<wound_type_id, time_duration>> wound_type::evaluate_progress_timer() const
{
    if( progress_timer.has_value() ) {
        return std::make_pair( progress_timer->id, rng( progress_timer->time.first,
                               progress_timer->time.second ) );
    } else {
        return std::nullopt;
    }
}

std::string wound_type::get_progress_timer_message() const
{
    return progress_timer->message.translated();
}

int wound_type::get_limit() const
{
    return limit;
}

std::vector<wound_limb_score> wound_type::get_limb_scores() const
{
    return limb_scores;
}

std::vector<wound_mod_values> wound_type::get_modify_values() const
{
    return mod_values;
}

std::optional<wound_periodic_message> wound_type::get_periodic_message() const
{
    return periodic_message;
}

std::string wound_type::get_name() const
{
    return name_.translated();
}

std::string wound_type::get_description() const
{
    return description_.translated();
}

bool wound_type::can_be_applied( const Character *who, const bp_wounds &pt_wd,
                                 const bodypart_id &bp, const damage_instance &d ) const
{

    if( x_in_y( 1.0f - get_option<float>( "WOUND_CHANCE" ), 1.0f ) ) {
        return false;
    }

    bool damage_instance_match = false;

    for( const damage_unit &du : d.damage_units ) {
        const bool damage_within_limits = du.amount >= pt_wd.damage_required.first &&
                                          du.amount <= pt_wd.damage_required.second;

        bool damage_type_matches = std::find( pt_wd.damage_type.begin(),
                                              pt_wd.damage_type.end(), du.type ) != pt_wd.damage_type.end();

        if( damage_within_limits && damage_type_matches ) {
            damage_instance_match = true;
            break;
        }
    }

    if( !damage_instance_match ) {
        add_msg_debug( debugmode::DF_WOUNDS,
                       "%s: trying to apply wound %s, but mismatches damage amount or damage type, wound is not applied",
                       who->name, id.str() );
        return false;
    }

    const bool iwwlfb = who->get_part( bp )->is_within_wound_limit_for_bp( pt_wd.id );

    if( !iwwlfb ) {
        add_msg_debug( debugmode::DF_WOUNDS,
                       "%s: trying to apply wound %s to bodypart %s, but hits the wound limit, wound is not applied",
                       who->name, id.str(), bp.id().str() );
        return false;
    }

    for( const json_character_flag &fl_wl : whitelist_flags ) {
        if( !who->has_flag( fl_wl ) ) {
            add_msg_debug( debugmode::DF_WOUNDS,
                           "%s: wound %s require flag %s character do not have, wound is not applied",
                           who->name, id.str(), fl_wl.str() );
            return false;
        }
    }

    for( const json_character_flag &fl_bl : blacklist_flags ) {
        if( who->has_flag( fl_bl ) ) {
            add_msg_debug( debugmode::DF_WOUNDS,
                           "%s: wound %s blacklists characters with flag %s, wound is not applied",
                           who->name, id.str(), fl_bl.str() );
            return false;
        }
    }

    return true;
}


void wound::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "type", type );
    jsout.member( "healing_time", healing_time );
    jsout.member( "healing_progress", healing_progress );
    jsout.member( "wound_progress_timer", wound_progress_timer );
    jsout.member( "pain", pain );
    jsout.end_object();
}

void wound::deserialize( const JsonObject &jsin )
{
    jsin.read( "type", type );
    jsin.read( "healing_time", healing_time );
    jsin.read( "healing_progress", healing_progress );
    jsin.read( "wound_progress_timer", wound_progress_timer );
    jsin.read( "pain", pain );
}

float wound::healing_percentage() const
{
    return healing_progress / to_seconds<float>( healing_time );
}

void wound::mod_pain( const int mod )
{
    pain += mod;
}

int wound::get_pain() const
{
    return pain * ( 1 - healing_percentage() );
}

time_duration wound::get_healing_time() const
{
    return healing_time;
}

float wound::get_healing_progress() const
{
    return healing_progress;
}

wound_update_ret wound::update_wound( const Character *who, const time_duration time_passed )
{
    if( healing_time == calendar::INDEFINITELY_LONG_DURATION ) {
        return wound_update_ret::DO_NOTHING;
    }

    const float time = to_seconds<float>( time_passed );

    healing_progress += time * get_wound_healing_modifier( who );

    const time_duration heal_progress = time_duration::from_seconds( healing_progress );

    if( wound_progress_timer && wound_progress_timer->second <= heal_progress ) {
        return wound_update_ret::ADVANCE;
    } else if( heal_progress >= healing_time ) {
        return wound_update_ret::REMOVE;
    }

    return wound_update_ret::DO_NOTHING;
}

float wound::get_wound_healing_modifier( const Character *who ) const
{
    float mod = 1.f;

    // -200 health is 0.5x to healing, +200 is 1.5x
    mod = mod + ( who->get_lifestyle() / 400.f );

    mod = who->enchantment_cache->modify_value( enchant_vals::mod::REGEN_WOUND, mod );

    return mod;
}
