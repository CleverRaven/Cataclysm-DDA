#include <memory>

#include "character_id.h"
#include "item.h"
#include "itype.h"
#include "magic.h"
#include "npc.h"
#include "pimpl.h"
#include "point.h"
#include "talker_item.h"
#include "vehicle.h"

std::string talker_item::disp_name() const
{
    return me_it->get_item()->display_name();
}

std::string talker_item::get_name() const
{
    return me_it->get_item()->type_name();
}

int talker_item::posx() const
{
    return me_it->position().x;
}

int talker_item::posy() const
{
    return me_it->position().y;
}

int talker_item::posz() const
{
    return me_it->position().z;
}

tripoint talker_item::pos() const
{
    return me_it->position();
}

tripoint_abs_ms talker_item::global_pos() const
{
    return tripoint_abs_ms( get_map().getabs( me_it->position() ) );
}

tripoint_abs_omt talker_item::global_omt_location() const
{
    return get_player_character().global_omt_location();
}

std::string talker_item::get_value( const std::string &var_name ) const
{
    return me_it->get_item()->get_var( var_name );
}

void talker_item::set_value( const std::string &var_name, const std::string &value )
{
    me_it->get_item()->set_var( var_name, value );
}

void talker_item::remove_value( const std::string &var_name )
{
    me_it->get_item()->erase_var( var_name );
}

std::vector<std::string> talker_item::get_topics( bool )
{
    return me_it->get_item()->typeId()->chat_topics;
}

bool talker_item::will_talk_to_u( const Character &you, bool )
{
    return !you.is_dead_state();
}
