#include "medical.h"
#include "debug.h"

#include "creature.h"
#include "player.h"
#include "npc.h"
#include "monster.h"

#include "item.h"
#include "messages.h"
#include "bodypart.h"

#include <string>
#include <vector>
#include <map>


void Medical::show_medical_iface()
{
    if(!is_player()) {
        return;
    }
}

