#include <memory>
#include "character.h"
#include "effect.h"
#include "item.h"
#include "magic.h"
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

bool talker_monster_const::has_species( const species_id &species ) const
{
    add_msg_debug( debugmode::DF_TALKER, "Monster %s checked for species %s", me_mon_const->name(),
                   species.c_str() );
    return me_mon_const->in_species( species );
}

bool talker_monster_const::bodytype( const bodytype_id &bt ) const
{
    add_msg_debug( debugmode::DF_TALKER, "Monster %s checked for bodytype %s", me_mon_const->name(),
                   bt );
    return me_mon_const->type->bodytype == bt;
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

int talker_monster_const::get_size() const
{
    add_msg_debug( debugmode::DF_TALKER, "Size category of monster %s = %d", me_mon_const->name(),
                   me_mon_const->get_size() - 0 );
    return me_mon_const->get_size() - 0;
}

int talker_monster_const::get_grab_strength() const
{
    add_msg_debug( debugmode::DF_TALKER, "Grab strength of monster %s = %d", me_mon_const->name(),
                   me_mon_const->get_grab_strength() );
    return  me_mon_const->get_grab_strength();
}
void talker_monster::set_friendly( int new_val )
{
    me_mon->friendly = new_val;
}

bool talker_monster::get_is_alive() const
{
    return !me_mon->is_dead();
}

void talker_monster::die()
{
    me_mon->die( nullptr );
}

void talker_monster::set_all_parts_hp_cur( int set ) const
{
    me_mon->set_hp( set );
}

std::vector<std::string> talker_monster_const::get_topics( bool )
{
    return me_mon_const->type->chat_topics;
}

int talker_monster_const::get_cur_hp( const bodypart_id & ) const
{
    return me_mon_const->get_hp();
}

int talker_monster_const::get_hp_max( const bodypart_id & ) const
{
    return me_mon_const->get_hp_max();
}

bool talker_monster_const::will_talk_to_u( const Character &you, bool )
{
    return !you.is_dead_state();
}
