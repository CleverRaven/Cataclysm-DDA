#include "bodypart.h"

#include <map>
#include <unordered_map>
#include <set>
#include <utility>
#include <vector>

#include "anatomy.h"
#include "debug.h"
#include "generic_factory.h"
#include "translations.h"
#include "json.h"

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
    debugmsg( "Invalid side" );
    abort();
}

template<>
std::string enum_to_string<hp_part>( hp_part data )
{
    switch( data ) {
        // *INDENT-OFF*
        case hp_part::hp_head: return "head";
        case hp_part::hp_torso: return "torso";
        case hp_part::hp_arm_l: return "arm_l";
        case hp_part::hp_arm_r: return "arm_r";
        case hp_part::hp_leg_l: return "leg_l";
        case hp_part::hp_leg_r: return "leg_r";
        // *INDENT-ON*
        case hp_part::num_hp_parts:
            break;
    }
    debugmsg( "Invalid hp_part" );
    abort();
}

} // namespace io

namespace
{

generic_factory<body_part_struct> body_part_factory( "body part" );

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

template<>
bool bodypart_ids::is_valid() const
{
    return body_part_factory.is_valid( *this );
}

template<>
bool bodypart_id::is_valid() const
{
    return body_part_factory.is_valid( *this );
}

template<>
const body_part_struct &bodypart_ids::obj() const
{
    return body_part_factory.obj( *this );
}

template<>
const body_part_struct &bodypart_id::obj() const
{
    return body_part_factory.obj( *this );
}

template<>
const bodypart_ids &bodypart_id::id() const
{
    return body_part_factory.convert( *this );
}

template<>
bodypart_id bodypart_ids::id() const
{
    return body_part_factory.convert( *this, bodypart_id( 0 ) );
}

body_part get_body_part_token( const std::string &id )
{
    return legacy_id_to_enum( id );
}

const bodypart_ids &convert_bp( body_part bp )
{
    static const std::vector<bodypart_ids> body_parts = {
        bodypart_ids( "torso" ),
        bodypart_ids( "head" ),
        bodypart_ids( "eyes" ),
        bodypart_ids( "mouth" ),
        bodypart_ids( "arm_l" ),
        bodypart_ids( "arm_r" ),
        bodypart_ids( "hand_l" ),
        bodypart_ids( "hand_r" ),
        bodypart_ids( "leg_l" ),
        bodypart_ids( "leg_r" ),
        bodypart_ids( "foot_l" ),
        bodypart_ids( "foot_r" ),
        bodypart_ids( "num_bp" ),
    };
    if( bp > num_bp || bp < bp_torso ) {
        debugmsg( "Invalid body part token %d", bp );
        return body_parts[ num_bp ];
    }

    return body_parts[static_cast<size_t>( bp )];
}

static const body_part_struct &get_bp( body_part bp )
{
    return convert_bp( bp ).obj();
}

void body_part_struct::load_bp( JsonObject &jo, const std::string &src )
{
    body_part_factory.load( jo, src );
}

void body_part_struct::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );

    mandatory( jo, was_loaded, "name", name );
    optional( jo, was_loaded, "name_plural", name_multiple );
    mandatory( jo, was_loaded, "heading_singular", name_as_heading_singular );
    mandatory( jo, was_loaded, "heading_plural", name_as_heading_multiple );
    optional( jo, was_loaded, "hp_bar_ui_text", hp_bar_ui_text );
    mandatory( jo, was_loaded, "encumbrance_text", encumb_text );
    mandatory( jo, was_loaded, "hit_size", hit_size );
    mandatory( jo, was_loaded, "hit_difficulty", hit_difficulty );
    mandatory( jo, was_loaded, "hit_size_relative", hit_size_relative );

    mandatory( jo, was_loaded, "legacy_id", legacy_id );
    token = legacy_id_to_enum( legacy_id );

    mandatory( jo, was_loaded, "main_part", main_part );
    mandatory( jo, was_loaded, "opposite_part", opposite_part );

    part_side = jo.get_enum_value<side>( "side" );
}

void body_part_struct::reset()
{
    body_part_factory.reset();
}

void body_part_struct::finalize_all()
{
    body_part_factory.finalize();
}

void body_part_struct::finalize()
{
}

void body_part_struct::check_consistency()
{
    for( const body_part bp : all_body_parts ) {
        const auto &legacy_bp = convert_bp( bp );
        if( !legacy_bp.is_valid() ) {
            debugmsg( "Mandatory body part %s was not loaded", legacy_bp.c_str() );
        }
    }

    body_part_factory.check();
}

void body_part_struct::check() const
{
    const auto &under_token = get_bp( token );
    if( this != &under_token ) {
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
    }
}

// Some languages do not have plural forms, so we don't use ngettext()/npgettext()
// in the 3 following methods so the singular and plural strings can be properly
// translated for these languages.
std::string body_part_name( body_part bp, int number )
{
    const auto &bdy = get_bp( bp );
    return number > 1 ? _( bdy.name_multiple ) : _( bdy.name );
}

std::string body_part_name_accusative( body_part bp, int number )
{
    const auto &bdy = get_bp( bp );
    if( number > 1 ) {
        return pgettext( "bodypart_accusative", bdy.name_multiple.c_str() );
    } else {
        return pgettext( "bodypart_accusative", bdy.name.c_str() );
    }
}

std::string body_part_name_as_heading( body_part bp, int number )
{
    const auto &bdy = get_bp( bp );
    return number > 1 ? _( bdy.name_as_heading_multiple ) : _( bdy.name_as_heading_singular );
}

std::string body_part_hp_bar_ui_text( body_part bp )
{
    return _( get_bp( bp ).hp_bar_ui_text );
}

std::string encumb_text( body_part bp )
{
    const std::string &txt = get_bp( bp ).encumb_text;
    return !txt.empty() ? _( txt ) : txt;
}

body_part random_body_part( bool main_parts_only )
{
    const auto &part = human_anatomy->random_body_part();
    return main_parts_only ? part->main_part->token : part->token;
}

body_part mutate_to_main_part( body_part bp )
{
    return get_bp( bp ).main_part->token;
}

body_part opposite_body_part( body_part bp )
{
    return get_bp( bp ).opposite_part->token;
}

std::string get_body_part_id( body_part bp )
{
    return get_bp( bp ).legacy_id;
}
