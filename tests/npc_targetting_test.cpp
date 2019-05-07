#include <stddef.h>
#include <string>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "catch/catch.hpp"
#include "common_types.h"
#include "faction.h"
#include "field.h"
#include "game.h"
#include "map.h"
#include "npc.h"
#include "npc_class.h"
#include "overmapbuffer.h"
#include "player.h"
#include "text_snippets.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_reference.h" // IWYU pragma: keep
#include "calendar.h"
#include "creature.h"
#include "enums.h"
#include "game_constants.h"
#include "line.h"
#include "mapdata.h"
#include "optional.h"
#include "pimpl.h"
#include "string_id.h"
#include "mtype.h"

npc create_model()
{
    standard_npc dude("TestCharacter", {}, 3, 8, 8, 8, 8);
    dude.weapon = item("2x4");
    return dude;
}
