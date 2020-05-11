#include <string>

#include "behavior.h"
#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "monster.h"
#include "monster_oracle.h"

namespace behavior
{

status_t monster_oracle_t::not_hallucination( const std::string & ) const
{
    return subject->is_hallucination() ? failure : running;
}

status_t monster_oracle_t::items_available( const std::string & ) const
{
    if( !g->m.has_flag( TFLAG_SEALED, subject->pos() ) && g->m.has_items( subject->pos() ) ) {
        return running;
    }
    return failure;
}

// TODO: Have it select a target and stash it somewhere.
status_t monster_oracle_t::adjacent_plants( const std::string & ) const
{
    for( const tripoint &p : g->m.points_in_radius( subject->pos(), 1 ) ) {
        if( g->m.has_flag( "PLANT", p ) ) {
            return running;
        }
    }
    return failure;
}

status_t monster_oracle_t::special_available( const std::string &special_name ) const
{
    return subject->special_available( special_name ) ? running : failure;
}

} // namespace behavior
