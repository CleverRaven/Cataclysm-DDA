#include "anatomy.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <set>
#include <string>
#include <unordered_set>

#include "cata_utility.h"
#include "character.h"
#include "debug.h"
#include "generic_factory.h"
#include "flag.h"
#include "json.h"
#include "messages.h"
#include "output.h"
#include "rng.h"
#include "type_id.h"
#include "weighted_list.h"

static const anatomy_id anatomy_human_anatomy( "human_anatomy" );

static const json_character_flag json_flag_ALWAYS_BLOCK( "ALWAYS_BLOCK" );
static const json_character_flag json_flag_LIMB_LOWER( "LIMB_LOWER" );
static const json_character_flag json_flag_LIMB_UPPER( "LIMB_UPPER" );

static const limb_score_id limb_score_block( "block" );

namespace
{

generic_factory<anatomy> anatomy_factory( "anatomy" );

} // namespace

template<>
bool anatomy_id::is_valid() const
{
    return anatomy_factory.is_valid( *this );
}

template<>
const anatomy &anatomy_id::obj() const
{
    return anatomy_factory.obj( *this );
}

void anatomy::load_anatomy( const JsonObject &jo, const std::string &src )
{
    anatomy_factory.load( jo, src );
}

void anatomy::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "parts", unloaded_bps );
}

void anatomy::reset()
{
    anatomy_factory.reset();
}

void anatomy::finalize_all()
{
    // For some weird reason, generic_factory::finalize doesn't call finalize
    anatomy_factory.finalize();
    for( const anatomy &an : anatomy_factory.get_all() ) {
        const_cast<anatomy &>( an ).finalize();
    }
}

void anatomy::finalize()
{
    size_sum = 0.0f;

    cached_bps.clear();
    for( const bodypart_str_id &id : unloaded_bps ) {
        if( id.is_valid() ) {
            add_body_part( id );
        } else {
            debugmsg( "Anatomy %s has invalid body part %s", id.c_str(), id.c_str() );
        }
    }
}

void anatomy::check_consistency()
{
    anatomy_factory.check();
    if( !anatomy_human_anatomy.is_valid() ) {
        debugmsg( "Could not load human anatomy, expect crash" );
    }
}

void anatomy::check() const
{
    if( !get_part_with_cumulative_hit_size( size_sum ).is_valid() ) {
        debugmsg( "Invalid size_sum calculation for anatomy %s", id.c_str() );
    }

    std::unordered_set<bodypart_str_id> all_parts( unloaded_bps.begin(), unloaded_bps.end() );
    std::unordered_set<bodypart_str_id> root_parts;

    for( const bodypart_str_id &bp : unloaded_bps ) {
        if( !id.is_valid() ) {
            // Error already reported in finalize
            continue;
        }
        if( !all_parts.count( bp->main_part ) ) {
            debugmsg( "Anatomy %s contains part %s whose main_part %s is not part of the anatomy",
                      id.str(), bp.str(), bp->main_part.str() );
        } else if( !all_parts.count( bp->connected_to ) ) {
            debugmsg( "Anatomy %s contains part %s with connected_to part %s which is not part of "
                      "the anatomy", id.str(), bp.str(), bp->main_part.str() );
        }

        if( bp->connected_to == bp ) {
            root_parts.insert( bp );
        }
    }

    if( root_parts.size() > 1 ) {
        debugmsg( "Anatomy %s has multiple root parts: %s", id.str(),
        enumerate_as_string( root_parts.begin(), root_parts.end(), []( const bodypart_str_id & p ) {
            return p.str();
        } ) );
    }
}

std::vector<bodypart_id> anatomy::get_bodyparts() const
{
    return cached_bps;
}

anatomy::anatomy( const std::vector<bodypart_id> &parts )
{
    for( const bodypart_id &part : parts ) {
        add_body_part( part.id() );
        unloaded_bps.push_back( part.id() );
    }
}

void anatomy::add_body_part( const bodypart_str_id &new_bp )
{
    cached_bps.emplace_back( new_bp.id() );
    const body_part_type &bp_struct = new_bp.obj();
    size_sum += bp_struct.hit_size;
}

// TODO: get_function_with_better_name
bodypart_str_id anatomy::get_part_with_cumulative_hit_size( float size ) const
{
    for( const bodypart_id &part : cached_bps ) {
        size -= part->hit_size;
        if( size <= 0.0f ) {
            return part.id();
        }
    }

    return bodypart_str_id::NULL_ID();
}

bodypart_id anatomy::random_body_part() const
{
    return get_part_with_cumulative_hit_size( rng_float( 0.0f, size_sum ) ).id();
}

bodypart_id anatomy::select_body_part( int min_hit, int max_hit, bool can_attack_high,
                                       int hit_roll ) const
{

    weighted_float_list<bodypart_id> hit_weights;
    for( const bodypart_id &bp : cached_bps ) {
        float weight = bp->hit_size;
        //Filter out too-large or too-small bodyparts
        if( weight < min_hit || ( max_hit > -1 && weight > max_hit ) ) {
            add_msg_debug( debugmode::DF_ANATOMY_BP, "BP %s discarded - hitsize %.1f( min %d max %d )",
                           body_part_name( bp ), weight, min_hit, max_hit );
            continue;
        }

        if( !can_attack_high ) {
            if( bp->has_flag( json_flag_LIMB_UPPER ) ) {
                add_msg_debug( debugmode::DF_ANATOMY_BP, "limb %s discarded, we can't attack upper limbs",
                               body_part_name( bp ) );
                continue;
            }
            if( bp->has_flag( json_flag_LIMB_LOWER ) ) {
                add_msg_debug( debugmode::DF_ANATOMY_BP,
                               "limb %s's weight tripled for short attackers",
                               body_part_name( bp ) );
                weight *= 3;
            }
        }

        if( hit_roll > 0 ) {
            weight *= std::pow( static_cast<float>( hit_roll ), bp->hit_difficulty );
        }

        hit_weights.add( bp, weight );
    }

    // Debug for seeing weights.
    for( const weighted_object<double, bodypart_id> &pr : hit_weights ) {
        add_msg_debug( debugmode::DF_ANATOMY_BP, "%s = %.3f", pr.obj.obj().name, pr.weight );
    }

    const bodypart_id *ret = hit_weights.pick();
    if( ret == nullptr ) {
        debugmsg( "Attempted to select body part from empty anatomy %s", id.c_str() );
        return bodypart_str_id::NULL_ID().id();
    }

    add_msg_debug( debugmode::DF_ANATOMY_BP, "selected part: %s", ret->id().obj().name );
    return *ret;
}


bodypart_id anatomy::select_blocking_part( const Creature *blocker, bool arm, bool leg,
        bool nonstandard ) const
{
    weighted_float_list<bodypart_id> block_scores;
    for( const bodypart_id &bp : cached_bps ) {
        float block_score = bp->get_limb_score( limb_score_block );
        if( const Character *u = dynamic_cast<const Character *>( blocker ) ) {
            block_score = u->get_part( bp )->get_limb_score( limb_score_block );
            // Weigh shielded bodyparts higher
            block_score *= u->worn_with_flag( flag_BLOCK_WHILE_WORN, bp ) ? 5 : 1;
        }
        body_part_type::type limb_type = bp->limb_type;

        if( block_score == 0 ) {
            add_msg_debug( debugmode::DF_MELEE, "BP %s discarded, no blocking score",
                           body_part_name( bp ) );
            continue;
        }

        // Filter out arm and leg types TODO consolidate into one if
        if( limb_type == body_part_type::type::arm && !arm &&
            !bp->has_flag( json_flag_ALWAYS_BLOCK ) )  {
            add_msg_debug( debugmode::DF_MELEE, "BP %s discarded, no arm blocks allowed",
                           body_part_name( bp ) );
            continue;
        }

        if( limb_type == body_part_type::type::leg && !leg &&
            !bp->has_flag( json_flag_ALWAYS_BLOCK ) ) {
            add_msg_debug( debugmode::DF_MELEE, "BP %s discarded, no leg blocks allowed",
                           body_part_name( bp ) );
            continue;
        }

        if( limb_type != body_part_type::type::arm && limb_type != body_part_type::type::leg &&
            !nonstandard ) {
            add_msg_debug( debugmode::DF_MELEE, "BP %s discarded, no nonstandard blocks allowed",
                           body_part_name( bp ) );
            continue;
        }

        block_scores.add( bp, block_score );
    }

    // Debug for seeing weights.
    for( const weighted_object<double, bodypart_id> &pr : block_scores ) {
        add_msg_debug( debugmode::DF_MELEE, "%s = %.3f", pr.obj.obj().name, pr.weight );
    }

    const bodypart_id *ret = block_scores.pick();
    if( ret == nullptr ) {
        debugmsg( "Attempted to select body part from empty anatomy %s", id.c_str() );
        return bodypart_str_id::NULL_ID().id();
    }

    add_msg_debug( debugmode::DF_MELEE, "selected part: %s", ret->id().obj().name );
    return *ret;
}
