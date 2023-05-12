#include <memory>
#include "character.h"
#include "effect.h"
#include "item.h"
#include "magic.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "pimpl.h"
#include "point.h"
#include "talker_monster.h"
#include "vehicle.h"

class time_duration;


talker_monster::talker_monster( monster *new_me )
{
    me_mon = new_me;
    me_mon_const = new_me;
}

std::string talker_monster_const::disp_name() const
{
    return me_mon_const->disp_name();
}

int talker_monster_const::posx() const
{
    return me_mon_const->posx();
}

int talker_monster_const::posy() const
{
    return me_mon_const->posy();
}

int talker_monster_const::posz() const
{
    return me_mon_const->posz();
}

tripoint talker_monster_const::pos() const
{
    return me_mon_const->pos();
}

tripoint_abs_ms talker_monster_const::global_pos() const
{
    return me_mon_const->get_location();
}

tripoint_abs_omt talker_monster_const::global_omt_location() const
{
    return me_mon_const->global_omt_location();
}

int talker_monster_const::pain_cur() const
{
    return me_mon_const->get_pain();
}

bool talker_monster_const::has_effect( const efftype_id &effect_id, const bodypart_id &bp ) const
{
    return me_mon_const->has_effect( effect_id, bp );
}

effect talker_monster_const::get_effect( const efftype_id &effect_id, const bodypart_id &bp ) const
{
    return me_mon_const->get_effect( effect_id, bp );
}

void talker_monster::add_effect( const efftype_id &new_effect, const time_duration &dur,
                                 const std::string &bp, bool permanent, bool force,
                                 int intensity )
{
    me_mon->add_effect( new_effect, dur, bodypart_str_id( bp ), permanent, intensity, force );
}

void talker_monster::remove_effect( const efftype_id &old_effect )
{
    me_mon->remove_effect( old_effect );
}

void talker_monster::mod_pain( int amount )
{
    me_mon->mod_pain( amount );
}

std::string talker_monster_const:: get_value( const std::string &var_name ) const
{
    return me_mon_const->get_value( var_name );
}

bool talker_monster_const::has_flag( const flag_id &f ) const
{
    add_msg_debug( debugmode::DF_TALKER, "Monster %s checked for flag %s", me_mon_const->name(),
                   f.c_str() );
    return me_mon_const->has_flag( f );
}

void talker_monster::set_value( const std::string &var_name, const std::string &value )
{
    me_mon->set_value( var_name, value );
}

void talker_monster::remove_value( const std::string &var_name )
{
    me_mon->remove_value( var_name );
}

std::string talker_monster_const::short_description() const
{
    return me_mon_const->type->get_description();
}

int talker_monster_const::get_anger() const
{
    return me_mon_const->anger;
}

void talker_monster::set_anger( int new_val )
{
    me_mon->anger = new_val;
}

int talker_monster_const::morale_cur() const
{
    return me_mon_const->morale;
}

void talker_monster::set_morale( int new_val )
{
    me_mon->morale = new_val;
}

int talker_monster_const::get_friendly() const
{
    return me_mon_const->friendly;
}

void talker_monster::set_friendly( int new_val )
{
    me_mon->friendly = new_val;
}

std::vector<std::string> talker_monster_const::get_topics( bool )
{
    return me_mon_const->type->chat_topics;
}

int talker_monster_const::get_cur_hp( const bodypart_id & ) const
{
    return me_mon_const->get_hp();
}

bool talker_monster_const::will_talk_to_u( const Character &you, bool )
{
    return !you.is_dead_state();
}
