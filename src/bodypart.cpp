#include "bodypart.h"

#include <algorithm>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "assign.h"
#include "body_part_set.h"
#include "debug.h"
#include "enum_conversions.h"
#include "flexbuffer_json-inl.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "init.h"
#include "json.h"
#include "json_error.h"
#include "localized_comparator.h"
#include "rng.h"
#include "subbodypart.h"

const bodypart_str_id body_part_arm_l( "arm_l" );
const bodypart_str_id body_part_arm_r( "arm_r" );
const bodypart_str_id body_part_bp_null( "bp_null" );
const bodypart_str_id body_part_eyes( "eyes" );
const bodypart_str_id body_part_foot_l( "foot_l" );
const bodypart_str_id body_part_foot_r( "foot_r" );
const bodypart_str_id body_part_hand_l( "hand_l" );
const bodypart_str_id body_part_hand_r( "hand_r" );
const bodypart_str_id body_part_head( "head" );
const bodypart_str_id body_part_leg_l( "leg_l" );
const bodypart_str_id body_part_leg_r( "leg_r" );
const bodypart_str_id body_part_mouth( "mouth" );
const bodypart_str_id body_part_torso( "torso" );

const sub_bodypart_str_id sub_body_part_sub_limb_debug( "sub_limb_debug" );

side opposite_side( side s )
{
    switch( s ) {
        case side::BOTH:
            return side::BOTH;
        case side::LEFT:
            return side::RIGHT;
        case side::RIGHT:
            return side::LEFT;
        case side::num_sides:
            debugmsg( "invalid side %d", static_cast<int>( s ) );
            break;
    }

    return s;
}

namespace io
{

template<>
std::string enum_to_string<side>( side data )
{
    switch( data ) {
        // *INDENT-OFF*
        case side::LEFT: return "left";
        case side::RIGHT: return "right";
        case side::BOTH: return "both";
        // *INDENT-ON*
        case side::num_sides:
            break;
    }
    cata_fatal( "Invalid side" );
}

template<>
std::string enum_to_string<body_part_type::type>( body_part_type::type data )
{
    switch( data ) {
        // *INDENT-OFF*
        case body_part_type::type::arm: return "arm";
        case body_part_type::type::other: return "other";
        case body_part_type::type::foot: return "foot";
        case body_part_type::type::hand: return "hand";
        case body_part_type::type::head: return "head";
        case body_part_type::type::leg: return "leg";
        case body_part_type::type::mouth: return "mouth";
        case body_part_type::type::sensor: return "sensor";
        case body_part_type::type::tail: return "tail";
        case body_part_type::type::torso: return "torso";
        case body_part_type::type::wing: return "wing";
            // *INDENT-ON*
        case body_part_type::type::num_types:
            break;
    }
    cata_fatal( "Invalid body part type." );
}

} // namespace io

namespace
{

generic_factory<body_part_type> body_part_factory( "body part" );
generic_factory<limb_score> limb_score_factory( "limb score" );

} // namespace

static body_part legacy_id_to_enum( const std::string &legacy_id )
{
    static const std::unordered_map<std::string, body_part> body_parts = {
        { "TORSO", bp_torso },
        { "HEAD", bp_head },
        { "EYES", bp_eyes },
        { "MOUTH", bp_mouth },
        { "ARM_L", bp_arm_l },
        { "ARM_R", bp_arm_r },
        { "HAND_L", bp_hand_l },
        { "HAND_R", bp_hand_r },
        { "LEG_L", bp_leg_l },
        { "LEG_R", bp_leg_r },
        { "FOOT_L", bp_foot_l },
        { "FOOT_R", bp_foot_r },
        { "NUM_BP", num_bp },
    };
    const auto &iter = body_parts.find( legacy_id );
    if( iter == body_parts.end() ) {
        debugmsg( "Invalid body part legacy id %s", legacy_id.c_str() );
        return num_bp;
    }

    return iter->second;
}

/** @relates string_id */
template<>
const limb_score &string_id<limb_score>::obj() const
{
    return limb_score_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<limb_score>::is_valid() const
{
    return limb_score_factory.is_valid( *this );
}

void limb_score::load_limb_scores( const JsonObject &jo, const std::string &src )
{
    limb_score_factory.load( jo, src );
}

void limb_score::reset()
{
    limb_score_factory.reset();
}

const std::vector<limb_score> &limb_score::get_all()
{
    return limb_score_factory.get_all();
}

void limb_score::load( const JsonObject &jo, const std::string_view )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "name", _name );
    optional( jo, was_loaded, "affected_by_wounds", wound_affect, true );
    optional( jo, was_loaded, "affected_by_encumb", encumb_affect, true );
}

/**@relates string_id*/
template<>
bool string_id<body_part_type>::is_valid() const
{
    return body_part_factory.is_valid( *this );
}

/** @relates int_id */
template<>
bool int_id<body_part_type>::is_valid() const
{
    return body_part_factory.is_valid( *this );
}

/**@relates string_id*/
template<>
const body_part_type &string_id<body_part_type>::obj() const
{
    return body_part_factory.obj( *this );
}

/** @relates int_id */
template<>
const body_part_type &int_id<body_part_type>::obj() const
{
    return body_part_factory.obj( *this );
}

/** @relates int_id */
template<>
const bodypart_str_id &int_id<body_part_type>::id() const
{
    return body_part_factory.convert( *this );
}

/**@relates string_id*/
template<>
bodypart_id string_id<body_part_type>::id() const
{
    return body_part_factory.convert( *this, bodypart_id( 0 ) );
}

/** @relates int_id */
template<>
int_id<body_part_type>::int_id( const string_id<body_part_type> &id ) : _id( id.id() ) {}

const bodypart_str_id &convert_bp( body_part bp )
{
    static const std::vector<bodypart_str_id> body_parts = {
        body_part_torso,
        body_part_head,
        body_part_eyes,
        body_part_mouth,
        body_part_arm_l,
        body_part_arm_r,
        body_part_hand_l,
        body_part_hand_r,
        body_part_leg_l,
        body_part_leg_r,
        body_part_foot_l,
        body_part_foot_r,
        bodypart_str_id::NULL_ID(),
    };
    if( bp > num_bp || bp < bp_torso ) {
        debugmsg( "Invalid body part token %d", bp );
        return body_parts[ num_bp ];
    }

    return body_parts[static_cast<size_t>( bp )];
}

void body_part_type::load_bp( const JsonObject &jo, const std::string &src )
{
    body_part_factory.load( jo, src );
}

body_part_type::type body_part_type::primary_limb_type() const
{
    return _primary_limb_type;
}

bool body_part_type::has_type( const body_part_type::type &type ) const
{
    return limbtypes.count( type ) > 0;
}

bool body_part_type::has_flag( const json_character_flag &flag ) const
{
    return flags.count( flag ) > 0;
}

sub_bodypart_id body_part_type::random_sub_part( bool secondary ) const
{
    int total_weight = 0;
    for( const sub_bodypart_str_id &bp : sub_parts ) {
        // filter for secondary sub locations
        if( secondary == bp->secondary ) {
            total_weight += bp->max_coverage;
        }
    }
    int roll = rng( 1, total_weight );
    for( const sub_bodypart_str_id &bp : sub_parts ) {
        // filter for secondary sub locations
        if( secondary == bp->secondary ) {
            if( roll <= bp->max_coverage ) {
                return bp.id();
            }
            roll = roll - bp->max_coverage;
        }
    }
    // should never get here
    return sub_bodypart_id();
}

const std::vector<body_part_type> &body_part_type::get_all()
{
    return body_part_factory.get_all();
}

void body_part_type::load( const JsonObject &jo, const std::string_view )
{
    mandatory( jo, was_loaded, "id", id );

    mandatory( jo, was_loaded, "name", name );
    // This is NOT the plural of `name`; it's a name referring to the pair of
    // bodyparts which this bodypart belongs to, and thus should not be implemented
    // using "n_gettext" or "translation::make_plural". Otherwise, in languages
    // without plural forms, translation of this string would indicate it
    // to be a left or right part, while it is not.
    optional( jo, was_loaded, "name_multiple", name_multiple );

    mandatory( jo, was_loaded, "accusative", accusative );
    // same as the above comment
    optional( jo, was_loaded, "accusative_multiple", accusative_multiple );

    mandatory( jo, was_loaded, "heading", name_as_heading );
    // Same as the above comment
    mandatory( jo, was_loaded, "heading_multiple", name_as_heading_multiple );
    optional( jo, was_loaded, "hp_bar_ui_text", hp_bar_ui_text );
    mandatory( jo, was_loaded, "encumbrance_text", encumb_text );
    mandatory( jo, was_loaded, "hit_size", hit_size );
    mandatory( jo, was_loaded, "hit_difficulty", hit_difficulty );

    mandatory( jo, was_loaded, "base_hp", base_hp );
    optional( jo, was_loaded, "stat_hp_mods", hp_mods );
    optional( jo, was_loaded, "heal_bonus", heal_bonus, 0 );
    optional( jo, was_loaded, "mend_rate", mend_rate, 1.0f );

    mandatory( jo, was_loaded, "drench_capacity", drench_max );
    optional( jo, was_loaded, "drench_increment", drench_increment, 2 );
    optional( jo, was_loaded, "drying_rate", drying_rate, 1.0f );

    optional( jo, was_loaded, "wet_morale", wet_morale, 0 );

    optional( jo, was_loaded, "ugliness", ugliness, 0 );
    optional( jo, was_loaded, "ugliness_mandatory", ugliness_mandatory, 0 );

    optional( jo, was_loaded, "is_limb", is_limb, false );
    optional( jo, was_loaded, "is_vital", is_vital, false );
    if( jo.has_array( "limb_types" ) ) {
        limbtypes.clear();
        body_part_type::type first_type = body_part_type::type::num_types;
        bool set_first_type = true;
        for( JsonValue jval : jo.get_array( "limb_types" ) ) {
            float weight = 1.0f;
            body_part_type::type limb_type;
            if( jval.test_array() ) {
                JsonArray jarr = jval.get_array();
                limb_type = io::string_to_enum<body_part_type::type>( jarr.get_string( 0 ) );
                weight = jarr.get_float( 1 );
                set_first_type = false;
            } else {
                limb_type = io::string_to_enum<body_part_type::type>( jval.get_string() );
            }
            limbtypes.emplace( limb_type, weight );
            if( first_type == body_part_type::type::num_types ) {
                first_type = limb_type;
            }
        }
        // set cached primary type if no weights specified
        if( set_first_type ) {
            _primary_limb_type = first_type;
        }
    } else {
        limbtypes.clear();
        body_part_type::type limb_type = {};
        mandatory( jo, was_loaded, "limb_type", limb_type );
        limbtypes.emplace( limb_type, 1.0f );
    }

    if( _primary_limb_type == body_part_type::type::num_types ) {
        float high = 0.f;
        for( auto &bp_type : limbtypes ) {
            if( high < bp_type.second ) {
                high = bp_type.second;
                _primary_limb_type = bp_type.first;
            }
        }
    }

    // tokens are actually legacy code that should be on their way out.
    if( !was_loaded ) {
        optional( jo, was_loaded, "legacy_id", legacy_id, "BP_NULL" );
        if( legacy_id != "BP_NULL" ) {
            token = legacy_id_to_enum( legacy_id );
        }
    } else {
        // we need to clear this because any bodypart using copy-from will not be a legacy part.
        legacy_id = "BP_NULL";
        token = body_part::num_bp;
    }

    optional( jo, was_loaded, "env_protection", env_protection, 0 );

    int legacy_fire_warmth_bonus = units::to_legacy_bodypart_temp_delta( fire_warmth_bonus );
    optional( jo, was_loaded, "fire_warmth_bonus", legacy_fire_warmth_bonus, 0 );
    fire_warmth_bonus = units::from_legacy_bodypart_temp_delta( legacy_fire_warmth_bonus );

    mandatory( jo, was_loaded, "main_part", main_part );
    if( main_part == id ) {
        mandatory( jo, was_loaded, "connected_to", connected_to );
    } else {
        connected_to = main_part;
    }
    mandatory( jo, was_loaded, "opposite_part", opposite_part );

    optional( jo, was_loaded, "windage_effect", windage_effect, efftype_id::NULL_ID() );
    optional( jo, was_loaded, "no_power_effect", no_power_effect, efftype_id::NULL_ID() );
    optional( jo, was_loaded, "smash_message", smash_message );
    optional( jo, was_loaded, "smash_efficiency", smash_efficiency, 0.5f );

    optional( jo, was_loaded, "hot_morale_mod", hot_morale_mod, 0.0 );
    optional( jo, was_loaded, "cold_morale_mod", cold_morale_mod, 0.0 );

    optional( jo, was_loaded, "feels_discomfort", feels_discomfort, true );

    optional( jo, was_loaded, "stylish_bonus", stylish_bonus, 0 );
    optional( jo, was_loaded, "squeamish_penalty", squeamish_penalty, 0 );

    optional( jo, was_loaded, "bionic_slots", bionic_slots_, 0 );

    optional( jo, was_loaded, "flags", flags );
    optional( jo, was_loaded, "conditional_flags", conditional_flags );

    optional( jo, was_loaded, "encumbrance_threshold", encumbrance_threshold, 0 );
    optional( jo, was_loaded, "encumbrance_limit", encumbrance_limit, 100 );
    optional( jo, was_loaded, "health_limit", health_limit, 1 );
    optional( jo, was_loaded, "techniques", techniques );
    optional( jo, was_loaded, "technique_encumbrance_limit", technique_enc_limit, 50 );
    optional( jo, was_loaded, "bmi_encumbrance_threshold", bmi_encumbrance_threshold, 999 );
    optional( jo, was_loaded, "bmi_encumbrance_scalar", bmi_encumbrance_scalar, 0 );

    optional( jo, was_loaded, "power_efficiency", power_efficiency, 0 );

    optional( jo, was_loaded, "similar_bodyparts", similar_bodyparts );

    if( jo.has_member( "limb_scores" ) ) {
        limb_scores.clear();
        const JsonArray &jarr = jo.get_array( "limb_scores" );
        for( const JsonValue jval : jarr ) {
            bp_limb_score bpls;
            const limb_score_id id = limb_score_id( jval.get_array().get_string( 0 ) );
            bpls.score = jval.get_array().get_float( 1 );
            bpls.max = jval.get_array().size() > 2 ? jval.get_array().get_float( 2 ) : bpls.score;
            limb_scores[id] = bpls;
        }
    }

    if( jo.has_array( "effects_on_hit" ) ) {
        effects_on_hit.clear();
        for( const JsonObject effect_jo : jo.get_array( "effects_on_hit" ) ) {
            bp_onhit_effect eff;
            eff.load( effect_jo );
            effects_on_hit.push_back( eff );
        }
    }

    if( jo.has_array( "temp_mod" ) ) {
        JsonArray temp_array = jo.get_array( "temp_mod" );
        temp_min = units::from_legacy_bodypart_temp_delta( temp_array.get_int( 0 ) );
        temp_max = units::from_legacy_bodypart_temp_delta( temp_array.get_int( 1 ) );
    }

    if( jo.has_array( "unarmed_damage" ) ) {
        unarmed_bonus = true;
        damage = damage_instance();
        damage = load_damage_instance( jo.get_array( "unarmed_damage" ) );
    }

    if( jo.has_object( "armor" ) ) {
        armor = resistances();
        armor = load_resistances_instance( jo.get_object( "armor" ) );
    }

    mandatory( jo, was_loaded, "side", part_side );

    mandatory( jo, was_loaded, "sub_parts", sub_parts );

    if( jo.has_array( "encumbrance_per_weight" ) ) {
        const JsonArray &jarr = jo.get_array( "encumbrance_per_weight" );
        for( const JsonObject jval : jarr ) {
            units::mass weight = 0_gram;
            int encumbrance = 0;

            assign( jval, "weight", weight, true );
            mandatory( jval, was_loaded, "encumbrance", encumbrance );

            encumbrance_per_weight.insert( std::pair<units::mass, int>( weight, encumbrance ) );
        }
    }
}

void bp_onhit_effect::load( const JsonObject &jo )
{
    mandatory( jo, false, "id", id );
    optional( jo, false, "global", global );
    optional( jo, false, "dmg_type", dtype, damage_type_id::NULL_ID() );
    optional( jo, false, "dmg_threshold", dmg_threshold, 1 );
    optional( jo, false, "dmg_scale_increment", scale_increment, 1.0f );
    optional( jo, false, "chance", chance, 100 );
    optional( jo, false, "chance_dmg_scaling", chance_dmg_scaling, 0.0f );
    optional( jo, false, "intensity", intensity, 1 );
    optional( jo, false, "intensity_dmg_scaling", intensity_dmg_scaling, 0.0f );
    optional( jo, false, "max_intensity", max_intensity, INT_MAX );
    optional( jo, false, "duration", duration, 1 );
    optional( jo, false, "duration_dmg_scaling", duration_dmg_scaling, 0.0f );
    optional( jo, false, "max_duration", max_duration, INT_MAX );
}

void body_part_type::reset()
{
    body_part_factory.reset();
}

void body_part_type::finalize_all()
{
    body_part_factory.finalize();
}

void body_part_type::finalize()
{
    finalize_damage_map( armor.resist_vals );
}

void body_part_type::check_consistency()
{
    for( const body_part bp : all_body_parts ) {
        const bodypart_str_id &legacy_bp = convert_bp( bp );
        if( !legacy_bp.is_valid() ) {
            debugmsg( "Mandatory body part %s was not loaded", legacy_bp.c_str() );
        }
    }

    body_part_factory.check();
}

void body_part_type::check() const
{
    const body_part_type &under_token = convert_bp( token ).obj();
    if( this != &under_token && token != body_part::num_bp ) {
        debugmsg( "Body part %s has duplicate token %d, mapped to %s", id.c_str(), token,
                  under_token.id.c_str() );
    }

    if( !id.is_null() && main_part.is_null() ) {
        debugmsg( "Body part %s has unset main part", id.c_str() );
    }

    if( !id.is_null() && opposite_part.is_null() ) {
        debugmsg( "Body part %s has unset opposite part", id.c_str() );
    }

    if( !main_part.is_valid() ) {
        debugmsg( "Body part %s has invalid main part %s.", id.c_str(), main_part.c_str() );
    }

    if( !opposite_part.is_valid() ) {
        debugmsg( "Body part %s has invalid opposite part %s.", id.c_str(), opposite_part.c_str() );
    } else if( opposite_part->opposite_part != id ) {
        debugmsg( "Bodypart %s has inconsistent opposite part!", id.str() );
    }

    for( const std::pair<const limb_score_id, bp_limb_score> &bpls : limb_scores ) {
        if( !bpls.first.is_valid() ) {
            debugmsg( "Body part %s has invalid limb score %s.", id.str(), bpls.first.str() );
        }
        if( bpls.second.score > bpls.second.max ) {
            debugmsg( "Body part %s has higher %s score than max.", id.str(), bpls.first.str() );
        }
    }

    for( const std::pair<const damage_type_id, float> &dt : armor.resist_vals ) {
        if( !dt.first.is_valid() ) {
            debugmsg( "Invalid armor type \"%s\" for body part %s", dt.first.c_str(), id.c_str() );
        }
    }

    for( const damage_unit &dt : damage.damage_units ) {
        if( !dt.type.is_valid() ) {
            debugmsg( "Invalid unarmed_damage type \"%s\" for body part %s", dt.type.c_str(), id.c_str() );
        }
    }

    // Check that connected_to leads eventually to the root bodypart (currently always head),
    // without any loops
    std::unordered_set<bodypart_str_id> visited = { id };
    bodypart_str_id next = connected_to;
    while( !visited.count( next ) ) {
        visited.insert( next );
        next = next->connected_to;
    }

    if( next != next->connected_to ) {
        debugmsg( "Loop in body part connectedness starting from %s", id.str() );
    }
}

float body_part_type::get_limb_score( const limb_score_id &id ) const
{
    const auto it = limb_scores.find( id );
    return it == limb_scores.end() ? 0.0f : it->second.score;
}

float body_part_type::get_limb_score_max( const limb_score_id &id ) const
{
    const auto it = limb_scores.find( id );
    return it == limb_scores.end() ? 0.0f : it->second.max;
}

bool body_part_type::has_limb_score( const limb_score_id &id ) const
{
    return limb_scores.count( id );
}

float body_part_type::unarmed_damage( const damage_type_id &dt ) const
{
    return damage.type_damage( dt );
}

float body_part_type::unarmed_arpen( const damage_type_id &dt ) const
{
    return damage.type_arpen( dt );
}

float body_part_type::damage_resistance( const damage_type_id &dt ) const
{
    return armor.type_resist( dt );
}

float body_part_type::damage_resistance( const damage_unit &du ) const
{
    return armor.get_effective_resist( du );
}

std::set<translation, localized_comparator> body_part_type::consolidate(
    std::vector<sub_bodypart_id> &covered )
{
    std::set<translation, localized_comparator> to_return;
    std::vector<bodypart_id> full_bps;

    //first try to compress sets of sub body parts together into a full limb
    for( size_t i = 0; i < covered.size(); i++ ) {
        const sub_bodypart_id &sbp = covered[i];
        if( sbp == sub_body_part_sub_limb_debug ) {
            // if we have already covered this continue
            continue;
        }

        // try to find all matching sublimbs from the parent
        bool found_all = true;
        for( const sub_bodypart_str_id &searching : sbp->parent->sub_parts ) {
            //skip secondary locations for this
            if( !searching->secondary ) {
                found_all = std::find( covered.begin(), covered.end(), searching.id() ) != covered.end() &&
                            found_all;
            }
        }

        // if found all consolidate all the limb bits together
        // TODO: shouldn't need so many loops to do this
        if( found_all ) {
            full_bps.emplace_back( sbp->parent );
            // set the initial to a skipped value
            for( const sub_bodypart_str_id &searching : sbp->parent->sub_parts ) {
                if( !searching->secondary ) {
                    auto sbp_it = std::find( covered.begin(), covered.end(), searching.id() );
                    *sbp_it = sub_body_part_sub_limb_debug;
                }
            }
        }
    }

    // now try and compress together matching limbs
    for( size_t i = 0; i < full_bps.size(); i++ ) {
        const bodypart_id &bp = full_bps[i];

        if( bp == bodypart_str_id::NULL_ID().id() ) {
            //its already been covered
            continue;
        }
        // bodyparts with no opposite are their own opposite
        if( bp->opposite_part.id() == bp ) {
            // if no opposite don't look for one
            to_return.insert( bp->name_as_heading );
            continue;
        }
        auto bp_itt = std::find( full_bps.begin(), full_bps.end(), bp->opposite_part );
        if( bp_itt == full_bps.end() ) {
            // if we didn't find the match just add the limb we were just looking at
            to_return.insert( bp->name_as_heading );
            continue;
        }

        *bp_itt = bodypart_str_id::NULL_ID().id();
        to_return.insert( bp->name_as_heading_multiple );

    }

    for( size_t i = 0; i < covered.size(); i++ ) {
        const sub_bodypart_id &sbp = covered[i];
        if( sbp == sub_body_part_sub_limb_debug ) {
            // if we have already covered this value as a pair continue
            continue;
        }
        sub_bodypart_id temp;
        // if our sub part has an opposite
        if( sbp->opposite != sub_body_part_sub_limb_debug ) {
            temp = sbp->opposite;
        } else {
            // if it doesn't have an opposite add it to the return vector alone and continue
            to_return.insert( sbp->name );
            continue;
        }

        bool found = false;
        for( sub_bodypart_id &sbp_it : covered ) {
            // go through each body part and test if its partner is there as well
            if( temp == sbp_it ) {
                // add the multiple name not the single
                to_return.insert( sbp->name_multiple );
                found = true;
                // set the found part to a null value
                sbp_it = sub_body_part_sub_limb_debug;
                break;
            }
        }
        // if we didn't find its pair print it normally
        if( !found ) {
            to_return.insert( sbp->name );
        }
    }

    return to_return;
}

std::set<translation, localized_comparator> body_part_type::consolidate(
    std::vector<bodypart_id> &covered )
{
    std::set<translation, localized_comparator> to_return;

    // now try and compress together matching limbs
    for( size_t i = 0; i < covered.size(); i++ ) {
        const bodypart_id &bp = covered[i];

        if( bp == bodypart_str_id::NULL_ID().id() ) {
            //its already been covered
            continue;
        }
        // bodyparts with no opposite are their own opposite
        if( bp->opposite_part.id() == bp ) {
            // if no opposite don't look for one
            to_return.insert( bp->name_as_heading );
            continue;
        }
        auto bp_itt = std::find( covered.begin(), covered.end(), bp->opposite_part );
        if( bp_itt == covered.end() ) {
            // if we didn't find the match just add the limb we were just looking at
            to_return.insert( bp->name_as_heading );
            continue;
        }

        *bp_itt = bodypart_str_id::NULL_ID().id();
        to_return.insert( bp->name_as_heading_multiple );
    }

    return to_return;
}

bool encumbrance_data::add_sub_location(
    const layer_level level, const sub_bodypart_id sbp )
{
    for( const sub_bodypart_id &layer_sbp : layer_penalty_details[static_cast<size_t>
            ( level )].covered_sub_parts ) {
        // if we find a location return true since we should add penalty
        if( sbp == layer_sbp ) {
            return true;
        }
    }
    // otherwise we should add it to the list
    layer_penalty_details[static_cast<size_t>( level )].covered_sub_parts.push_back( sbp );

    return false;
}

bool encumbrance_data::add_sub_location(
    const layer_level level, const sub_bodypart_str_id sbp )
{
    for( const sub_bodypart_id &layer_sbp : layer_penalty_details[static_cast<size_t>
            ( level )].covered_sub_parts ) {
        // if we find a location return true since we should add penalty
        if( sbp.id() == layer_sbp ) {
            return true;
        }
    }
    // otherwise we should add it to the list
    layer_penalty_details[static_cast<size_t>( level )].covered_sub_parts.push_back( sbp.id() );

    return false;
}

std::string body_part_name( const bodypart_id &bp, int number )
{
    // See comments in `body_part_type::load` about why these two strings are
    // not a single translation object with plural enabled.
    return number > 1 ? bp->name_multiple.translated() : bp->name.translated();
}

std::string body_part_name_accusative( const bodypart_id &bp, int number )
{
    // See comments in `body_part_type::load` about why these two strings are
    // not a single translation object with plural enabled.
    return number > 1 ? bp->accusative_multiple.translated() : bp->accusative.translated();
}

std::string body_part_name_as_heading( const bodypart_id &bp, int number )
{
    // See comments in `body_part_type::load` about why these two strings are
    // not a single translation object with plural enabled.
    return number > 1 ? bp->name_as_heading_multiple.translated() : bp->name_as_heading.translated();
}

std::string body_part_hp_bar_ui_text( const bodypart_id &bp )
{
    return bp->hp_bar_ui_text.translated();
}

std::string encumb_text( const bodypart_id &bp )
{
    return bp->encumb_text.translated();
}

body_part_set body_part_set::unify_set( const body_part_set &rhs )
{
    for( const bodypart_str_id &i : rhs ) {
        if( !test( i ) ) {
            set( i );
        }
    }
    return *this;
}

body_part_set body_part_set::intersect_set( const body_part_set &rhs )
{
    body_part_set temp;
    for( const bodypart_str_id &j : rhs ) {
        if( test( j ) ) {
            temp.set( j );
        }
    }
    clear();
    unify_set( temp );
    return *this;
}

body_part_set body_part_set::substract_set( const body_part_set &rhs )
{
    for( const bodypart_str_id &j : rhs ) {
        if( test( j ) ) {
            reset( j );
        }
    }
    return *this;
}

body_part_set body_part_set::make_intersection( const body_part_set &rhs ) const
{
    body_part_set new_intersection;
    new_intersection.unify_set( *this );
    return new_intersection.intersect_set( rhs );
}

void body_part_set::fill( const std::vector<bodypart_id> &bps )
{
    for( const bodypart_id &bp : bps ) {
        parts.insert( bp.id() );
    }
}

void body_part_set::serialize( JsonOut &s ) const
{
    s.write( parts );
}

void body_part_set::deserialize( const JsonValue &s )
{
    s.read( parts );
}

bodypart_id bodypart::get_id() const
{
    return id;
}

void bodypart::set_hp_to_max()
{
    hp_cur = hp_max;
}

bool bodypart::is_at_max_hp() const
{
    return hp_cur == hp_max;
}

float bodypart::get_wetness_percentage() const
{
    if( id->drench_max == 0 ) {
        return 0.0f;
    } else {
        return static_cast<float>( wetness ) / id->drench_max;
    }
}

efftype_id bodypart::get_windage_effect() const
{
    return id->windage_effect;
}

int bodypart::get_encumbrance_threshold() const
{
    return id->encumbrance_threshold;
}

bool bodypart::is_limb_overencumbered() const
{
    return get_encumbrance_data().encumbrance >= id->encumbrance_limit;
}

bool bodypart::has_conditional_flag( const json_character_flag &flag ) const
{
    return id->conditional_flags.count( flag ) > 0 && hp_cur > id->health_limit &&
           !is_limb_overencumbered();
}

std::set<matec_id> bodypart::get_limb_techs() const
{
    std::set<matec_id> result;
    if( !x_in_y( get_encumbrance_data().encumbrance, id->technique_enc_limit ) &&
        hp_cur > id->health_limit ) {
        result.insert( id->techniques.begin(), id->techniques.end() );
    }
    return result;
}

std::vector<bp_onhit_effect> bodypart::get_onhit_effects( damage_type_id dtype ) const
{
    std::vector<bp_onhit_effect> result;
    for( const bp_onhit_effect &effect : id->effects_on_hit ) {
        if( effect.dtype == dtype || effect.dtype == damage_type_id::NULL_ID() ) {
            result.push_back( effect );
        }
    }
    return result;
}

float bodypart::wound_adjusted_limb_value( const float val ) const
{
    double percent = static_cast<double>( get_hp_cur() ) /
                     static_cast<double>( get_hp_max() );
    if( percent > 0.75 ) {
        return val;
    }
    percent /= 0.75;
    return val * static_cast<float>( percent );
}

float bodypart::encumb_adjusted_limb_value( const float val ) const
{
    int enc = get_encumbrance_data().encumbrance;
    // Check if we're over our encumbrance limit, return 0 if so
    if( is_limb_overencumbered() ) {
        return 0;
    }
    // Reduce encumbrance by the limb's encumbrance threshold, limiting to 0
    enc = std::max( 0, ( enc - get_encumbrance_threshold() ) );
    // This is designed to get a 5% adjustment for an increase of 3 encumbrance, with further
    // adjustments decreasing to avoid a multiplier of 0 (or infinity if reciprocal).
    return val * 19.0f / ( 19.0f + enc / 3.0f );
}

float bodypart::skill_adjusted_limb_value( float val, int skill ) const
{
    const float mitigated_score = val * ( 1.0f + ( skill / 10.0f ) );
    return std::min( val, mitigated_score );
}

float bodypart::get_limb_score( const limb_score_id &score, int skill, int override_encumb,
                                int override_wounds ) const
{
    bool process_wounds = override_wounds == 1 || ( override_wounds == -1 &&
                          score->affected_by_wounds() );
    bool process_encumb = override_encumb == 1 || ( override_encumb == -1 &&
                          score->affected_by_encumb() );

    float sc = id->get_limb_score( score );
    if( process_wounds ) {
        sc = wound_adjusted_limb_value( sc );
    }
    if( process_encumb ) {
        sc = encumb_adjusted_limb_value( sc );
    }
    if( skill >= 0 ) {
        sc = skill_adjusted_limb_value( sc, skill );
    }
    return sc;
}

float bodypart::get_limb_score_max( const limb_score_id &score ) const
{
    return id->get_limb_score_max( score );
}

int bodypart::get_hp_cur() const
{
    return hp_cur;
}

int bodypart::get_hp_max() const
{
    return hp_max;
}

int bodypart::get_healed_total() const
{
    return healed_total;
}

int bodypart::get_damage_bandaged() const
{
    return damage_bandaged;
}

int bodypart::get_damage_disinfected() const
{
    return damage_disinfected;
}

const encumbrance_data &bodypart::get_encumbrance_data() const
{
    return encumb_data;
}

int bodypart::get_drench_capacity() const
{
    return id->drench_max;
}

int bodypart::get_wetness() const
{
    return wetness;
}

int bodypart::get_frostbite_timer() const
{
    return frostbite_timer;
}

units::temperature bodypart::get_temp_cur() const
{
    return temp_cur;
}

units::temperature bodypart::get_temp_conv() const
{
    return temp_conv;
}

int bodypart::get_bmi_encumbrance_threshold() const
{
    return id->bmi_encumbrance_threshold;
}

float bodypart::get_bmi_encumbrance_scalar() const
{
    return id->bmi_encumbrance_scalar;
}

std::array<int, NUM_WATER_TOLERANCE> bodypart::get_mut_drench() const
{
    return mut_drench;
}

float bodypart::get_hit_size() const
{
    return id->hit_size;
}

int bodypart::get_power_efficiency() const
{
    return id->power_efficiency;
}

void bodypart::set_hp_cur( int set )
{
    hp_cur = set;
}

void bodypart::set_hp_max( int set )
{
    hp_max = set;
}

void bodypart::set_healed_total( int set )
{
    healed_total = set;
}

void bodypart::set_damage_bandaged( int set )
{
    damage_bandaged = set;
}

void bodypart::set_damage_disinfected( int set )
{
    damage_disinfected = set;
}

void bodypart::set_encumbrance_data( const encumbrance_data &set )
{
    encumb_data = set;
}

void bodypart::set_mut_drench( const std::pair<water_tolerance, int> &set )
{
    if( set.first < WT_IGNORED || set.first > NUM_WATER_TOLERANCE ) {
        debugmsg( "Tried to use invalid water tolerance enum in set_mut_drench()." );
    }
    mut_drench[set.first] = set.second;
}

void bodypart::set_wetness( int set )
{
    wetness = set;
}

void bodypart::set_temp_cur( units::temperature set )
{
    temp_cur = set;
}

void bodypart::set_temp_conv( units::temperature set )
{
    temp_conv = set;
}

void bodypart::set_frostbite_timer( int set )
{
    frostbite_timer = set;
}

void bodypart::mod_hp_cur( int mod )
{
    hp_cur += mod;
}

void bodypart::mod_hp_max( int mod )
{
    hp_max += mod;
}

void bodypart::mod_healed_total( int mod )
{
    healed_total += mod;
}

void bodypart::mod_damage_bandaged( int mod )
{
    damage_bandaged += mod;
}

void bodypart::mod_damage_disinfected( int mod )
{
    damage_disinfected += mod;
}

void bodypart::mod_wetness( int mod )
{
    wetness += mod;
}

void bodypart::mod_temp_cur( units::temperature_delta mod )
{
    temp_cur += mod;
}

void bodypart::mod_temp_conv( units::temperature_delta mod )
{
    temp_conv += mod;
}

void bodypart::mod_frostbite_timer( int mod )
{
    frostbite_timer += mod;
}

void bodypart::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "id", id );
    json.member( "hp_cur", hp_cur );
    json.member( "hp_max", hp_max );
    json.member( "healed_total", healed_total );
    json.member( "damage_bandaged", damage_bandaged );
    json.member( "damage_disinfected", damage_disinfected );

    json.member( "wetness", wetness );
    json.member( "temp_cur", units::to_legacy_bodypart_temp( temp_cur ) );
    json.member( "temp_conv", units::to_legacy_bodypart_temp( temp_conv ) );
    json.member( "frostbite_timer", frostbite_timer );

    json.end_object();
}

void bodypart::deserialize( const JsonObject &jo )
{
    jo.read( "id", id, true );
    jo.read( "hp_cur", hp_cur, true );
    jo.read( "hp_max", hp_max, true );
    jo.read( "healed_total", healed_total );
    jo.read( "damage_bandaged", damage_bandaged, true );
    jo.read( "damage_disinfected", damage_disinfected, true );

    jo.read( "wetness", wetness, true );
    if( int legacy_temp_cur; jo.read( "temp_cur", legacy_temp_cur, true ) ) {
        temp_cur = units::from_legacy_bodypart_temp( legacy_temp_cur );
    }
    if( int legacy_temp_conv; jo.read( "temp_conv", legacy_temp_conv, true ) ) {
        temp_conv = units::from_legacy_bodypart_temp( legacy_temp_conv );
    }
    jo.read( "frostbite_timer", frostbite_timer, true );

}

void stat_hp_mods::load( const JsonObject &jsobj )
{
    optional( jsobj, was_loaded, "str_mod", str_mod, 3.0f );
    optional( jsobj, was_loaded, "dex_mod", dex_mod, 0.0f );
    optional( jsobj, was_loaded, "int_mod", int_mod, 0.0f );
    optional( jsobj, was_loaded, "per_mod", per_mod, 0.0f );

    optional( jsobj, was_loaded, "health_mod", health_mod, 0.0f );
}

void stat_hp_mods::deserialize( const JsonObject &jo )
{
    load( jo );
}
