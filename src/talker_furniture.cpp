#include "talker_furniture.h"

#include <vector>

#include "character.h"
#include "computer.h"
#include "coordinates.h"
#include "map.h"
#include "math_parser_diag_value.h"
#include "point.h"

std::string talker_furniture_const::disp_name() const
{
    return me_comp->name;
}

int talker_furniture_const::posx( const map &here ) const
{
    return here.get_bub( me_comp->loc ).x();
}

int talker_furniture_const::posy( const map &here ) const
{
    return here.get_bub( me_comp->loc ).y();
}

int talker_furniture_const::posz() const
{
    return me_comp->loc.z();
}

tripoint_bub_ms talker_furniture_const::pos_bub( const map &here ) const
{
    return here.get_bub( me_comp->loc );
}

tripoint_abs_ms talker_furniture_const::pos_abs() const
{
    return me_comp->loc;
}

tripoint_abs_omt talker_furniture_const::pos_abs_omt() const
{
    return get_player_character().pos_abs_omt();
}

diag_value const *talker_furniture_const::maybe_get_value( const std::string &var_name ) const
{
    return me_comp->maybe_get_value( var_name );
}

void talker_furniture::set_value( const std::string &var_name, diag_value const &value )
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
