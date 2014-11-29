#include "medical.h"
#include "debug.h"

#include "creature.h"
#include "player.h"
#include "npc.h"
#include "monster.h"

#include "item.h"
#include "messages.h"

#include <string>
#include <vector>
#include <map>


Medical::Medical(Creature *c)
{
    init(c);
}

Medical::~Medical()
{
}

void Medical::init(Creature *c)
{
    us = c;
}

void Medical::show_interface()
{
    if(!us->is_player()) {
        return;
    }
}

