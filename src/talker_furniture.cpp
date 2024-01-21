#include <memory>

#include "character_id.h"
#include "computer.h"
#include "item.h"
#include "itype.h"
#include "magic.h"
#include "npc.h"
#include "pimpl.h"
#include "point.h"
#include "talker_furniture.h"
#include "vehicle.h"

std::string talker_furniture::disp_name() const
{
    return me_comp->name;
}

int talker_furniture::posx() const
{
    return me_comp->loc.x;
}

int talker_furniture::posy() const
{
    return me_comp->loc.y;
}

int talker_furniture::posz() const
{
    return me_comp->loc.z;
}

tripoint talker_furniture::pos() const
{
    return me_comp->loc;
}

tripoint_abs_ms talker_furniture::global_pos() const
{
    return tripoint_abs_ms( get_map().getabs( me_comp->loc ) );
}

tripoint_abs_omt talker_furniture::global_omt_location() const
{
    return get_player_character().global_omt_location();
}

std::optional<std::string> talker_furniture::maybe_get_value( const std::string &var_name ) const
{
    return me_comp->maybe_get_value( var_name );
}

void talker_furniture::set_value( const std::string &var_name, const std::string &value )
{
    me_comp->set_value( var_name, value );
}

void talker_furniture::remove_value( const std::string &var_name )
{
    me_comp->remove_value( var_name );
}

std::vector<std::string> talker_furniture::get_topics( bool )
{
    return me_comp->chat_topics;
}

bool talker_furniture::will_talk_to_u( const Character &you, bool )
{
    return !you.is_dead_state();
}
