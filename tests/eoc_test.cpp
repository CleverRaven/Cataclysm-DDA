#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "common_types.h"
#include "creature_tracker.h"
#include "effect_on_condition.h"
#include "faction.h"
#include "field.h"
#include "field_type.h"
#include "game.h"
#include "line.h"
#include "map.h"
#include "map_helpers.h"
#include "memory_fast.h"
#include "npc.h"
#include "npc_class.h"
#include "optional.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "point.h"
#include "text_snippets.h"
#include "type_id.h"
#include "units.h"

static const effect_on_condition_id eoc_EOC_teleport_test( "EOC_teleport_test" );


TEST_CASE( "EOC_teleport", "[eoc]" )
{
    tripoint_abs_ms before = get_avatar().get_location();
    dialogue newDialog( get_talker_for( get_avatar() ), nullptr );
    eoc_EOC_teleport_test->activate( newDialog );
    tripoint_abs_ms after = get_avatar().get_location();

    CHECK( before + tripoint_south_east == after );
}
