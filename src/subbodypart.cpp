#include "subbodypart.h"

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

const sub_bodypart_str_id sub_bodypart_sub_limb_debug( "sub_limb_debug" );

namespace
{

generic_factory<sub_body_part_type> sub_body_part_factory( "sub body part" );

} // namespace

/**@relates string_id*/
template<>
bool string_id<sub_body_part_type>::is_valid() const
{
    return sub_body_part_factory.is_valid( *this );
}

/** @relates int_id */
template<>
bool int_id<sub_body_part_type>::is_valid() const
{
    return sub_body_part_factory.is_valid( *this );
}

/**@relates string_id*/
template<>
const sub_body_part_type &string_id<sub_body_part_type>::obj() const
{
    return sub_body_part_factory.obj( *this );
}

/** @relates int_id */
template<>
const sub_body_part_type &int_id<sub_body_part_type>::obj() const
{
    return sub_body_part_factory.obj( *this );
}

/** @relates int_id */
template<>
const sub_bodypart_str_id &int_id<sub_body_part_type>::id() const
{
    return sub_body_part_factory.convert( *this );
}

/**@relates string_id*/
template<>
sub_bodypart_id string_id<sub_body_part_type>::id() const
{
    return sub_body_part_factory.convert( *this, sub_bodypart_id( 0 ) );
}

/** @relates int_id */
template<>
int_id<sub_body_part_type>::int_id( const string_id<sub_body_part_type> &id ) : _id( id.id() ) {}

void sub_body_part_type::load_bp( const JsonObject &jo, const std::string &src )
{
    sub_body_part_factory.load( jo, src );
}

void sub_body_part_type::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "parent", parent );
    optional( jo, was_loaded, "secondary", secondary );
    optional( jo, was_loaded, "max_coverage", max_coverage );
    optional( jo, was_loaded, "side", part_side );
    optional( jo, was_loaded, "name_multiple", name_multiple );
    optional( jo, was_loaded, "opposite", opposite );
}

std::vector<translation> sub_body_part_type::consolidate( std::vector<sub_bodypart_id> &covered )
{
    std::vector<translation> to_return;
    for( size_t i = 0; i < covered.size(); i++ ) {
        const sub_bodypart_id &sbp = covered[i];
        if( sbp == sub_bodypart_sub_limb_debug ) {
            // if we have already covered this value as a pair continue
            continue;
        }
        sub_bodypart_id temp;
        // if our sub part has an opposite
        if( sbp->opposite != sub_bodypart_sub_limb_debug ) {
            temp = sbp->opposite;
        } else {
            // if it doesn't have an opposite add it to the return vector alone and continue
            to_return.push_back( sbp->name );
            continue;
        }

        bool found = false;
        for( std::vector<sub_bodypart_id>::iterator sbp_it = covered.begin(); sbp_it != covered.end();
             ++sbp_it ) {
            // go through each body part and test if its partner is there as well
            if( temp == *sbp_it ) {
                // add the multiple name not the single
                to_return.push_back( sbp->name_multiple );
                found = true;
                // set the found part to a null value
                *sbp_it = sub_bodypart_sub_limb_debug;
                break;
            }
        }
        // if we didn't find its pair print it normally
        if( !found ) {
            to_return.push_back( sbp->name );
        }
    }

    return to_return;
}

void sub_body_part_type::reset()
{
    sub_body_part_factory.reset();
}

void sub_body_part_type::finalize_all()
{
    sub_body_part_factory.finalize();
}

void sub_body_part_type::finalize()
{

}
