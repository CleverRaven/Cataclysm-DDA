#include <memory>

#include "behavior.h"
#include "game.h"
#include "map.h"
#include "mapdata.h"
#include "monster.h"
#include "monster_oracle.h"

namespace behavior
{

status_t monster_oracle_t::has_special() const
{
    if( subject->shortest_special_cooldown() == 0 ) {
        return running;
    }
    return failure;
}

status_t monster_oracle_t::not_hallucination() const
{
    return subject->is_hallucination() ? failure : running;
}

status_t monster_oracle_t::items_available() const
{
    if( !g->m.has_flag( TFLAG_SEALED, subject->pos() ) && g->m.has_items( subject->pos() ) ) {
        return running;
    }
    return failure;
}

} // namespace behavior
