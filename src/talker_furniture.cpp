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

faction *talker_furniture::get_faction() const
{
    return me_comp->get_faction();
}

std::string talker_furniture::get_value( const std::string &var_name ) const
{
    return me_comp->get_value( var_name );
}

void talker_furniture::set_value( const std::string &var_name, const std::string &value )
{
    me_comp->set_value( var_name, value );
}

void talker_furniture::remove_value( const std::string &var_name )
{
    me_comp->remove_value( var_name );
}

ret_val<void> talker_furniture::wants_to_sell( const item_location &it, int at_price ) const
{
    // We can only get here if an EOC or dialog option explicitly called out trade and our inventory only exists for trade so they are willing to sell.
    return ret_val<void>::make_success();
}

ret_val<void> talker_furniture::wants_to_buy( const item &it, int at_price ) const
{
    return me_comp->wants_to_buy( it, at_price );
}

faction_price_rule const *talker_furniture::get_price_rules( item const &it ) const
{
    return me_comp->get_price_rules( it );
}

std::vector<std::string> talker_furniture::get_topics( bool )
{
    return me_comp->chat_topics;
}

bool talker_furniture::will_talk_to_u( const Character &you, bool )
{
    return !you.is_dead_state();
}
