#include "mission.h"
#include "game.h"

mission mission_type::create(game *g)
{
 mission ret;
 ret.type = this;
 if (deadline_low != 0 || deadline_high != 0)
  ret.deadline = g->turn + rng(deadline_low, deadline_high);
 else
  ret.deadline = 0;

 mission_start m_s;
 (m_s.*start)(g, &ret);

 return ret;
}

std::string mission::name()
{
 if (type == NULL)
  return "NULL";
 return type->name;
}
