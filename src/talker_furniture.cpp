#include "talker_furniture.h"

#include "character.h"
#include "computer.h"
#include "item.h"
#include "itype.h"
#include "magic.h"
#include "point.h"
#include "vehicle.h"

std::string talker_furniture_const::disp_name() const
{
    return me_comp->name;
}

int talker_furniture_const::posx() const
{
    return me_comp->loc.x();
}

int talker_furniture_const::posy() const
{
    return me_comp->loc.y();
}

int talker_furniture_const::posz() const
{
    return me_comp->loc.z();
}

tripoint talker_furniture_const::pos() const
{
    return me_comp->loc.raw();
}

tripoint_bub_ms talker_furniture_const::pos_bub() const
{
    return me_comp->loc;
}

tripoint_abs_ms talker_furniture_const::global_pos() const
{
    return get_map().getglobal( me_comp->loc );
}

tripoint_abs_omt talker_furniture_const::global_omt_location() const
{
    return get_player_character().global_omt_location();
}

std::optional<std::string> talker_furniture_const::maybe_get_value( const std::string &var_name )
const
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

std::vector<std::string> talker_furniture_const::get_topics( bool ) const
{
    return me_comp->chat_topics;
}

bool talker_furniture_const::will_talk_to_u( const Character &you, bool ) const
{
    return !you.is_dead_state();
}
