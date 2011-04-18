#include "mission.h"
#include "game.h"

mission mission_type::create(game *g)
{
 mission ret;
 ret.goal = goal;
 if (deadline_low != 0 || deadline_high != 0)
  ret.deadline = g->turn + rng(deadline_low, deadline_high);

 mission_start m_s;
 (m_s.*start)(g, &ret);
}
