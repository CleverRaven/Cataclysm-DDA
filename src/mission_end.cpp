#include "mission.h"
#include "game.h"
#include "translations.h"

void mission_end::heal_infection(mission *miss)
{
    bool found_npc = false;
    for (int i = 0; !found_npc && i < g->cur_om->npcs.size(); i++) {
        if (g->cur_om->npcs[i]->getID() == miss->npc_id) {
            g->cur_om->npcs[i]->rem_disease("infection");
            found_npc = true;
        }
    }
}

void mission_end::leave(mission *miss)
{
    npc *p = g->find_npc(miss->npc_id);
    p->attitude = NPCATT_NULL;
}

void mission_end::deposit_box(mission *miss)
{
    npc *p = g->find_npc(miss->npc_id);
    p->attitude = NPCATT_NULL;//npc leaves your party
    std::string itemName = "deagle_44";
    if (one_in(4)) {
        itemName = "katana";
    } else if (one_in(3)) {
        itemName = "m4a1";
    }
    g->u.i_add( item(itypes[itemName], 0) );
    g->add_msg(_("%s gave you an item from the deposit box."), p->name.c_str());
}
