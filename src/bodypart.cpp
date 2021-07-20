#include "bodypart.h"

#include <cstdlib>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "debug.h"
#include "enum_conversions.h"
#include "generic_factory.h"
#include "json.h"
#include "type_id.h"

const bodypart_str_id body_part_head( "head" );
const bodypart_str_id body_part_eyes( "eyes" );
const bodypart_str_id body_part_mouth( "mouth" );
const bodypart_str_id body_part_torso( "torso" );
const bodypart_str_id body_part_arm_l( "arm_l" );
const bodypart_str_id body_part_arm_r( "arm_r" );
const bodypart_str_id body_part_hand_l( "hand_l" );
const bodypart_str_id body_part_hand_r( "hand_r" );
const bodypart_str_id body_part_leg_l( "leg_l" );
const bodypart_str_id body_part_foot_l( "foot_l" );
const bodypart_str_id body_part_leg_r( "leg_r" );
const bodypart_str_id body_part_foot_r( "foot_r" );

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

} // namespace io

namespace
{

generic_factory<body_part_type> body_part_factory( "body part" );

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

bool body_part_type::has_flag( const std::string &flag ) const
{
    return flags.count( flag ) > 0;
}

const std::vector<body_part_type> &body_part_type::get_all()
{
    return body_part_factory.get_all();
}

void body_part_type::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );

    mandatory( jo, was_loaded, "name", name );
    // This is NOT the plural of `name`; it's a name referring to the pair of
    // bodyparts which this bodypart belongs to, and thus should not be implemented
    // using "ngettext" or "translation::make_plural". Otherwise, in languages
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
    mandatory( jo, was_loaded, "hit_size_relative", hit_size_relative );

    mandatory( jo, was_loaded, "base_hp", base_hp );
    optional( jo, was_loaded, "stat_hp_mods", hp_mods );

    mandatory( jo, was_loaded, "drench_capacity", drench_max );

    optional( jo, was_loaded, "is_limb", is_limb, false );
    mandatory( jo, was_loaded, "drench_capacity", drench_max );

    mandatory( jo, was_loaded, "legacy_id", legacy_id );
    token = legacy_id_to_enum( legacy_id );

    optional( jo, was_loaded, "fire_warmth_bonus", fire_warmth_bonus, 0 );

    mandatory( jo, was_loaded, "main_part", main_part );
    if( main_part == id ) {
        mandatory( jo, was_loaded, "connected_to", connected_to );
    } else {
        connected_to = main_part;
    }
    mandatory( jo, was_loaded, "opposite_part", opposite_part );

    optional( jo, was_loaded, "smash_message", smash_message );
    optional( jo, was_loaded, "smash_efficiency", smash_efficiency, 0.5f );

    optional( jo, was_loaded, "hot_morale_mod", hot_morale_mod, 0.0 );
    optional( jo, was_loaded, "cold_morale_mod", cold_morale_mod, 0.0 );

    optional( jo, was_loaded, "stylish_bonus", stylish_bonus, 0 );
    optional( jo, was_loaded, "squeamish_penalty", squeamish_penalty, 0 );

    optional( jo, was_loaded, "bionic_slots", bionic_slots_, 0 );

    optional( jo, was_loaded, "flags", flags );

    part_side = jo.get_enum_value<side>( "side" );
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
}

void body_part_type::check_consistency()
{
    for( const body_part bp : all_body_parts ) {
        const auto &legacy_bp = convert_bp( bp );
        if( !legacy_bp.is_valid() ) {
            debugmsg( "Mandatory body part %s was not loaded", legacy_bp.c_str() );
        }
    }

    body_part_factory.check();
}

void body_part_type::check() const
{
    const body_part_type &under_token = convert_bp( token ).obj();
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
    } else if( opposite_part->opposite_part != id ) {
        debugmsg( "Bodypart %s has inconsistent opposite part!", id.str() );
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
    return static_cast<float>( wetness ) / id->drench_max;
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

int bodypart::get_temp_cur() const
{
    return temp_cur;
}

int bodypart::get_temp_conv() const
{
    return temp_conv;
}

std::array<int, NUM_WATER_TOLERANCE> bodypart::get_mut_drench() const
{
    return mut_drench;
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

void bodypart::set_temp_cur( int set )
{
    temp_cur = set;
}

void bodypart::set_temp_conv( int set )
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

void bodypart::mod_temp_cur( int mod )
{
    temp_cur += mod;
}

void bodypart::mod_temp_conv( int mod )
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
    json.member( "damage_bandaged", damage_bandaged );
    json.member( "damage_disinfected", damage_disinfected );

    json.member( "wetness", wetness );
    json.member( "temp_cur", temp_cur );
    json.member( "temp_conv", temp_conv );
    json.member( "frostbite_timer", frostbite_timer );

    json.end_object();
}

void bodypart::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    jo.read( "id", id, true );
    jo.read( "hp_cur", hp_cur, true );
    jo.read( "hp_max", hp_max, true );
    jo.read( "damage_bandaged", damage_bandaged, true );
    jo.read( "damage_disinfected", damage_disinfected, true );

    jo.read( "wetness", wetness, true );
    jo.read( "temp_cur", temp_cur, true );
    jo.read( "temp_conv", temp_conv, true );
    jo.read( "frostbite_timer", frostbite_timer, true );

}

void stat_hp_mods::load( const JsonObject &jsobj )
{
    optional( jsobj, was_loaded, "str_mod", str_mod, 3.0f );
    optional( jsobj, was_loaded, "dex_mod", dex_mod, 0.0f );
    optional( jsobj, was_loaded, "int_mod", int_mod, 0.0f );
    optional( jsobj, was_loaded, "per_mod", str_mod, 0.0f );

    optional( jsobj, was_loaded, "health_mod", health_mod, 0.0f );
}

void stat_hp_mods::deserialize( JsonIn &jsin )
{
    const JsonObject &jo = jsin.get_object();
    load( jo );
}
