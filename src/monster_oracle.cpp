#include "behavior.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "monster.h"
#include "monster_oracle.h"

struct tripoint;

namespace behavior
{

status_t monster_oracle_t::not_hallucination( const std::string & ) const
{
    return subject->is_hallucination() ? status_t::failure : status_t::running;
}

status_t monster_oracle_t::split_possible( const std::string & ) const
{
    if( ( subject->get_hp() / 2 ) > subject->get_hp_max() ) {
        return status_t::running;
    }
    return status_t::failure;
}

status_t monster_oracle_t::items_available( const std::string & ) const
{
    if( !get_map().has_flag( ter_furn_flag::TFLAG_SEALED, subject->pos() ) &&
        get_map().has_items( subject->pos() ) ) {
        std::vector<material_id> absorb_material = subject->get_absorb_material();

        if( absorb_material.empty() ) {
            return status_t::running;
        }

        for( item &it : get_map().i_at( subject->pos() ) ) {
            for( const material_type *mat_type : it.made_of_types() ) {
                if( std::find( absorb_material.begin(), absorb_material.end(),
                               mat_type->id ) != absorb_material.end() ) {
                    return status_t::running;
                }
            }
        }
    }
    return status_t::failure;
}

// TODO: Have it select a target and stash it somewhere.
status_t monster_oracle_t::adjacent_plants( const std::string & ) const
{
    for( const tripoint &p : get_map().points_in_radius( subject->pos(), 1 ) ) {
        if( get_map().has_flag( ter_furn_flag::TFLAG_PLANT, p ) ) {
            return status_t::running;
        }
    }
    return status_t::failure;
}

status_t monster_oracle_t::special_available( const std::string &special_name ) const
{
    return subject->special_available( special_name ) ? status_t::running : status_t::failure;
}

} // namespace behavior
