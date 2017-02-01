#include "bodypart.h"
#include "translations.h"
#include "rng.h"
#include "debug.h"
#include "generic_factory.h"
#include <map>
#include <unordered_map>

side opposite_side( side s )
{
    switch( s ) {
        case side::BOTH:
            return side::BOTH;
        case side::LEFT:
            return side::RIGHT;
        case side::RIGHT:
            return side::LEFT;
    }

    return s;
}

namespace io {

static const std::map<std::string, side> side_map = {{
    { "left", side::LEFT },
    { "right", side::RIGHT },
    { "both", side::BOTH }
}};

template<>
side string_to_enum<side>( const std::string &data )
{
    return string_to_enum_look_up( side_map, data );
}

}

static std::array<body_part_struct, num_bp + 1> human_anatomy;
static std::unordered_map<std::string, body_part> body_parts_map;

namespace
{
const std::unordered_map<std::string, body_part> &get_body_parts_map()
{
    return body_parts_map;
}
} // namespace

body_part get_body_part_token( const std::string &id )
{
    auto &map = get_body_parts_map();
    const auto iter = map.find( id );
    if( iter == map.end() ) {
        debugmsg( "invalid body part id %s", id.c_str() );
        return num_bp;
    }
    return iter->second;
}

body_part hardcoded_bp_by_token( const std::string &token )
{
    static const std::unordered_map<std::string, body_part> body_parts = {
        { "torso", bp_torso },
        { "head", bp_head },
        { "eyes", bp_eyes },
        { "mouth", bp_mouth },
        { "arm_l", bp_arm_l },
        { "arm_r", bp_arm_r },
        { "hand_l", bp_hand_l },
        { "hand_r", bp_hand_r },
        { "leg_l", bp_leg_l },
        { "leg_r", bp_leg_r },
        { "foot_l", bp_foot_l },
        { "foot_r", bp_foot_r },
        { "num_bp", num_bp },
    };
    const auto &iter = body_parts.find( token );
    if( iter == body_parts.end() ) {
        debugmsg( "Invalid body part id %s", token.c_str() );
        return num_bp;
    }

    return iter->second;
}

void body_part_struct::load( JsonObject &jo, const std::string &src )
{
    body_part_struct new_part;
    new_part.load_this( jo, src );
    human_anatomy[ new_part.id ] = new_part;
    body_parts_map[ new_part.token ] = new_part.id;
}

void body_part_struct::load_this( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "token", token );
    id = hardcoded_bp_by_token( token );
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "name_as_heading_singular", name_as_heading_singular );
    mandatory( jo, was_loaded, "name_as_heading_multiple", name_as_heading_multiple );
    mandatory( jo, was_loaded, "encumb_text", encumb_text );
    mandatory( jo, was_loaded, "hit_size", hit_size );

    mandatory( jo, was_loaded, "main_part", main_part_string );
    mandatory( jo, was_loaded, "opposite_part", opposite_part_string );

    part_side = jo.get_enum_value<side>( "part_side" );
}

void body_part_struct::reset()
{
    for( auto &part : human_anatomy ) {
        // This will cause warnings if data isn't loaded before next check_consistency()
        part.id = num_bp;
    }

    body_parts_map.clear();
}

void body_part_struct::finalize()
{
    size_sum = 0;
    for( auto &part : human_anatomy ) {
        size_sum += part.hit_size;
        part.main_part = get_body_part_token( part.main_part_string );
        part.opposite_part = get_body_part_token( part.opposite_part_string );
    }
}

// @todo get_function_with_better_name
const body_part_struct &body_part_struct::get_part_with_cumulative_hit_size( int size )
{
    for( auto &part : human_anatomy ) {
        size -= part.hit_size;
        if( size <= 0 ) {
            return part;
        }
    }

    return human_anatomy[ num_bp ];
}

void body_part_struct::check_consistency()
{
    for( int i = 0; i < num_bp; i++ ) {
        body_part bp = static_cast<body_part>( i );
        const auto &part = human_anatomy[ i ];
        if( part.id != bp ) {
            debugmsg( "Body part %s has invalid id %d (should be %d)", part.token.c_str(), part.id, i );
        }

        const auto iter = body_parts_map.find( part.token );
        if( iter == body_parts_map.end() ) {
            debugmsg( "Body part %s is not in the map", part.token.c_str() );
        } else if( iter->second != part.id ) {
            debugmsg( "Body part map has mapped %s to id %d (should be %d)", part.token.c_str(), *iter, part.id );
        }

        if( part.id != num_bp && part.main_part == num_bp ) {
            debugmsg( "Body part %s has unset main part", part.token.c_str() );
        }

        if( part.id != num_bp && part.opposite_part == num_bp ) {
            debugmsg( "Body part %s has unset opposite part", part.token.c_str() );
        }

        // @todo Get rid of bp_aiOther
        if( static_cast<body_part>( bp_aiOther[ i ] ) != part.opposite_part ) {
            debugmsg( "Body part %s has invalid opposite_part %d (should be %d)", part.token.c_str(), part.opposite_part, bp_aiOther[ i ] );
        }
    }

    if( get_part_with_cumulative_hit_size( size_sum ).id == num_bp ) {
        debugmsg( "Invalid size_sum calculation" );
    }
}

body_part_struct get_bp( body_part bp )
{
    return human_anatomy[ bp ];
}

std::string body_part_name( body_part bp )
{
    return _( get_bp( bp ).name.c_str() );
}

std::string body_part_name_accusative( body_part bp )
{
    return pgettext( "bodypart_accusative", get_bp( bp ).name.c_str() );
}

std::string body_part_name_as_heading( body_part bp, int number )
{
    const auto &bdy = get_bp( bp );
    return ngettext( bdy.name_as_heading_singular.c_str(), bdy.name_as_heading_multiple.c_str(), number );
}

std::string encumb_text( body_part bp )
{
    const std::string &txt = get_bp( bp ).encumb_text;
    return !txt.empty() ? _( txt.c_str() ) : txt;
}

const body_part_struct &body_part_struct::random_part()
{
    return get_part_with_cumulative_hit_size( rng( 1, size_sum ) );
}

body_part random_body_part( bool main_parts_only )
{
    const auto &part = body_part_struct::random_part();
    return main_parts_only ? part.main_part : part.id;
}

body_part mutate_to_main_part( body_part bp )
{
    return get_bp( bp ).main_part;
}

body_part opposite_body_part( body_part bp )
{
    return get_bp( bp ).opposite_part;
}

std::string get_body_part_id( body_part bp )
{
    return get_bp( bp ).token;
}
