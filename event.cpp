#include "event.h"
#include "npc.h"
#include "game.h"
#include "rng.h"

void event::actualize(game *g)
{
 npc tmp;
 switch (type) {
  case EVENT_HELP: {
   int num = 1;
   if (relevant_faction)
    num = rng(1, 6);
   for (int i = 0; i < num; i++) {
    if (relevant_faction)
     tmp.randomize_from_faction(g, relevant_faction);
    else
     tmp.randomize(g);
    tmp.attitude = NPCATT_DEFEND;
    tmp.posx = g->u.posx - SEEX * 2 + rng(-5, 5);
    tmp.posy = g->u.posy - SEEY * 2 + rng(-5, 5);
    g->active_npc.push_back(tmp);
   }
  } break;
 }
}
