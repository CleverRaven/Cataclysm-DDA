#include "settlement.h"
#include "game.h"
#include "output.h"
#include "line.h"
#include "rng.h"

settlement::settlement()
{
 fact.randomize();
 posx = 0;
 posy = 0;
 size = 0;
 pop = 0;
 for (int i = 0; i < ot_wall - ot_set_house + 1; i++)
  buildings[i] = 0;
}

settlement::settlement(int mapx, int mapy)
{
 fact.randomize();
 posx = mapx;
 posy = mapy;
 size = 0;
 pop = 0;
 for (int i = 0; i < ot_wall - ot_set_house + 1; i++)
  buildings[i] = 0;
}

void settlement::set_population()
{
 pop = rng(4, 40);
 pop += rng(0, (fact.power - 20) / 6);
 if (pop < 4)
  pop = 4;
}

int settlement::num(oter_id ter)
{
 if (ter < ot_set_house || ter > ot_wall) {
  debugmsg("settlement::num requested bad oter_id (%s)",
           oterlist[ter].name.c_str());
  return -1;
 }
 return buildings[ter - ot_set_house];
}

void settlement::add_building(oter_id ter)
{
 if (ter < ot_set_house || ter > ot_wall)
  return;
 buildings[ter - ot_set_house]++;
}
