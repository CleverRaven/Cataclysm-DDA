#include "anatomy.h"
#include "generic_factory.h"
#include "rng.h"

anatomy_id human_anatomy( "human_anatomy" );

// @todo Better home
/**
 * A template for weighted random rolls out of discrete set.
 * Chance to pick an element is size/sum(all_sizes)
 */
template <typename T>
class roulette {
    private:
        std::vector<std::pair<float, T>> elems;
    public:
        /** Adds new element to the roulette. Can be duplicate. */
        add( float size, const T &new_elem );
        /** Rolls once and returns a reference to the result. */
        const T &get() const;
};

namespace
{

generic_factory<anatomy> anatomy_factory( "anatomy" );

}

template<>
const anatomy_id anatomy_id::NULL_ID( "null_anatomy" );

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
    anatomy_factory.finalize();
}

void anatomy::finalize()
{
    size_sum = 0.0f;
    size_relative_sum.fill( 0.0f );

    cached_bps.clear();
    for( const auto &id : unloaded_bps ) {
        const auto &new_bp = bodypart_ids( id );
        if( new_bp.is_valid() ) {
            add_body_part( new_bp );
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
        debugmsg( "Invalid size_sum calculation" );
    }
}

void anatomy::add_body_part( const bodypart_ids &new_bp )
{
    cached_bps.emplace_back( new_bp.id() );
    const auto &bp_struct = new_bp.obj();
    size_sum += bp_struct.hit_size;
    for( size_t i = 0; i < size_relative_sum.size(); i++ ) {
        size_relative_sum[i] += bp_struct.hit_size_relative[i];
    }
}

// @todo get_function_with_better_name
bodypart_ids anatomy::get_part_with_cumulative_hit_size( float size ) const
{
    for( auto &part : cached_bps ) {
        size -= part->hit_size;
        if( size <= 0.0f ) {
            return part.id();
        }
    }

    return NULL_ID;
}

bodypart_id anatomy::random_body_part() const
{
    return get_part_with_cumulative_hit_size( rng_float( 0.0f, size_sum ) ).id();
}

bodypart_id anatomy::select_body_part( int size_diff, int hit_roll ) const
{
    (void)size_diff;
    (void)hit_roll;
    return bodypart_ids(NULL_ID).id();
}