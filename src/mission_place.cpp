#include "mission.h"
#include "game.h"

bool mission_place::near_town(int posx, int posy)
{
 return (g->cur_om->dist_from_city( point(posx, posy) ) <= 40);
}
