#include "behavior.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "monster.h"
#include "monster_oracle.h"

struct tripoint;

namespace behavior
{

status_t monster_oracle_t::not_hallucination( const std::string_view ) const
{
    return subject->is_hallucination() ? status_t::failure : status_t::running;
}

status_t monster_oracle_t::split_possible( const std::string_view ) const
{
    // check if subject has split to support inverting this predicate for absorb monsters without split
    if( subject->has_special( "SPLIT" ) && ( subject->get_hp() / 2 ) > subject->get_hp_max() ) {
        return status_t::running;
    }
    return status_t::failure;
}

status_t monster_oracle_t::items_available( const std::string_view ) const
{
    if( !get_map().has_flag( ter_furn_flag::TFLAG_SEALED, subject->pos() ) &&
        get_map().has_items( subject->pos() ) ) {
        std::vector<material_id> absorb_material = subject->get_absorb_material();
        std::vector<material_id> no_absorb_material = subject->get_no_absorb_material();

        // base case: there are no specified conditions for what it can eat
        if( absorb_material.empty() && no_absorb_material.empty() ) {
            return status_t::running;
        }
        // case 2: no whitelist specified (it is approved for everything) but a blacklist specified
        if( absorb_material.empty() && !no_absorb_material.empty() ) {
            bool found = false;
            for( item &it : get_map().i_at( subject->pos() ) ) {
                for( const material_type *mat_type : it.made_of_types() ) {
                    if( !( std::find( no_absorb_material.begin(), no_absorb_material.end(),
                                      mat_type->id ) != no_absorb_material.end() ) ) {
                        //we found something that wasn't on the blacklist
                        found = true;
                    }
                }
            }
            if( found ) {
                return status_t::running;
            } else {
                return status_t::failure;
            }
        }
        // Case 3: there is a whitelist but no blacklist, so only allow the whitelisted ones
        if( !absorb_material.empty() && no_absorb_material.empty() ) {
            for( item &it : get_map().i_at( subject->pos() ) ) {
                for( const material_type *mat_type : it.made_of_types() ) {
                    if( std::find( absorb_material.begin(), absorb_material.end(),
                                   mat_type->id ) != absorb_material.end() ) {
                        return status_t::running;
                    }
                }
            }
        }
        // case 4: no whitelist specified (it is approved for everything) but a blacklist specified
        if( !absorb_material.empty() && !no_absorb_material.empty() ) {
            for( item &it : get_map().i_at( subject->pos() ) ) {
                for( const material_type *mat_type : it.made_of_types() ) {
                    if( !( std::find( no_absorb_material.begin(), no_absorb_material.end(),
                                      mat_type->id ) != no_absorb_material.end() ) ) {
                        //we found something that wasn't on the blacklist, so now check the whitelist
                        if( std::find( absorb_material.begin(), absorb_material.end(),
                                       mat_type->id ) != absorb_material.end() ) {
                            return status_t::running;
                        }
                    }
                }
            }
        }
    }
    return status_t::failure;
}

// TODO: Have it select a target and stash it somewhere.
status_t monster_oracle_t::adjacent_plants( const std::string_view ) const
{
    for( const tripoint &p : get_map().points_in_radius( subject->pos(), 1 ) ) {
        if( get_map().has_flag( ter_furn_flag::TFLAG_PLANT, p ) ) {
            return status_t::running;
        }
    }
    return status_t::failure;
}

status_t monster_oracle_t::special_available( const std::string_view special_name ) const
{
    bool only_if_present = false;
    std::string mspecial_name;

    if( special_name.rfind( '!', 0 ) != std::string::npos ) {
        only_if_present = true;
        mspecial_name = special_name.substr( 1 );
    } else {
        mspecial_name = special_name;
    }

    bool has_special = only_if_present ? subject->has_special( mspecial_name ) : true;

    return ( !has_special && only_if_present ) || ( has_special &&
            subject->special_available( mspecial_name ) ) ? status_t::running : status_t::failure;
}

} // namespace behavior
