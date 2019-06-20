#include "anatomy.h"

#include <cstddef>
#include <array>
#include <cmath>
#include <numeric>
#include <set>

#include "cata_utility.h"
#include "generic_factory.h"
#include "messages.h"
#include "rng.h"
#include "weighted_list.h"
#include "debug.h"
#include "enums.h"
#include "int_id.h"
#include "json.h"

anatomy_id human_anatomy( "human_anatomy" );

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

void anatomy::load_anatomy( JsonObject &jo, const std::string &src )
{
    anatomy_factory.load( jo, src );
}

void anatomy::load( JsonObject &jo, const std::string & )
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
    for( const auto &id : unloaded_bps ) {
        if( id.is_valid() ) {
            add_body_part( id );
        } else {
            debugmsg( "Anatomy %s has invalid body part %s", id.c_str(), id.c_str() );
        }
    }

    unloaded_bps.clear();
}

void anatomy::check_consistency()
{
    anatomy_factory.check();
    if( !human_anatomy.is_valid() ) {
        debugmsg( "Could not load human anatomy, expect crash" );
    }
}

void anatomy::check() const
{
    if( !get_part_with_cumulative_hit_size( size_sum ).is_valid() ) {
        debugmsg( "Invalid size_sum calculation for anatomy %s", id.c_str() );
    }

    for( size_t i = 0; i < 3; i++ ) {
        const float size_all = std::accumulate( cached_bps.begin(), cached_bps.end(), 0.0f, [i]( float acc,
        const bodypart_id & bp ) {
            return acc + bp->hit_size_relative[i];
        } );
        if( size_all <= 0.0f ) {
            debugmsg( "Anatomy %s has no part hittable when size difference is %d", id.c_str(),
                      static_cast<int>( i ) - 1 );
        }
    }
}

void anatomy::add_body_part( const bodypart_ids &new_bp )
{
    cached_bps.emplace_back( new_bp.id() );
    const auto &bp_struct = new_bp.obj();
    size_sum += bp_struct.hit_size;
}

// TODO: get_function_with_better_name
bodypart_ids anatomy::get_part_with_cumulative_hit_size( float size ) const
{
    for( auto &part : cached_bps ) {
        size -= part->hit_size;
        if( size <= 0.0f ) {
            return part.id();
        }
    }

    return bodypart_ids::NULL_ID();
}

bodypart_id anatomy::random_body_part() const
{
    return get_part_with_cumulative_hit_size( rng_float( 0.0f, size_sum ) ).id();
}

bodypart_id anatomy::select_body_part( int size_diff, int hit_roll ) const
{
    const size_t size_diff_index = static_cast<size_t>( 1 + clamp( size_diff, -1, 1 ) );
    weighted_float_list<bodypart_id> hit_weights;
    for( const auto &bp : cached_bps ) {
        float weight = bp->hit_size_relative[size_diff_index];
        if( weight <= 0.0f ) {
            continue;
        }

        if( hit_roll != 0 ) {
            weight *= std::pow( hit_roll, bp->hit_difficulty );
        }

        hit_weights.add( bp, weight );
    }

    // Debug for seeing weights.
    for( const auto &pr : hit_weights ) {
        add_msg( m_debug, "%s = %.3f", pr.obj.obj().name, pr.weight );
    }

    const bodypart_id *ret = hit_weights.pick();
    if( ret == nullptr ) {
        debugmsg( "Attempted to select body part from empty anatomy %s", id.c_str() );
        return bodypart_ids::NULL_ID().id();
    }

    add_msg( m_debug, "selected part: %s", ret->id().obj().name );
    return *ret;
}
