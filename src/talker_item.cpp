#include "talker_item.h"

#include "character.h"
#include "item.h"
#include "itype.h"
#include "magic.h"
#include "point.h"
#include "vehicle.h"

static const ammotype ammo_battery( "battery" );

static const itype_id itype_battery( "battery" );

std::string talker_item_const::disp_name() const
{
    return me_it_const->get_item()->display_name();
}

std::string talker_item_const::get_name() const
{
    return me_it_const->get_item()->type_name();
}

int talker_item_const::posx() const
{
    return me_it_const->position().x;
}

int talker_item_const::posy() const
{
    return me_it_const->position().y;
}

int talker_item_const::posz() const
{
    return me_it_const->position().z;
}

tripoint talker_item_const::pos() const
{
    return me_it_const->position();
}

tripoint_bub_ms talker_item_const::pos_bub() const
{
    return me_it_const->pos_bub();
}

tripoint_abs_ms talker_item_const::global_pos() const
{
    return get_map().getglobal( me_it_const->pos_bub() );
}

tripoint_abs_omt talker_item_const::global_omt_location() const
{
    return get_player_character().global_omt_location();
}

std::optional<std::string> talker_item_const::maybe_get_value( const std::string &var_name ) const
{
    return me_it_const->get_item()->maybe_get_var( var_name );
}

bool talker_item_const::has_flag( const flag_id &f ) const
{
    add_msg_debug( debugmode::DF_TALKER, "Item %s checked for flag %s",
                   me_it_const->get_item()->tname(),
                   f.c_str() );
    return me_it_const->get_item()->has_flag( f );
}

std::vector<std::string> talker_item_const::get_topics( bool ) const
{
    return me_it_const->get_item()->typeId()->chat_topics;
}

bool talker_item_const::will_talk_to_u( const Character &you, bool ) const
{
    return !you.is_dead_state();
}

int talker_item_const::get_cur_hp( const bodypart_id & ) const
{
    return me_it_const->get_item()->max_damage() - me_it_const->get_item()->damage();
}

int talker_item_const::get_degradation() const
{
    return me_it_const->get_item()->degradation();
}

int talker_item_const::get_hp_max( const bodypart_id & ) const
{
    return me_it_const->get_item()->max_damage();
}

units::energy talker_item_const::power_cur() const
{
    return 1_mJ * me_it_const->get_item()->ammo_remaining();
}

units::energy talker_item_const::power_max() const
{
    return 1_mJ * me_it_const->get_item()->ammo_capacity( ammo_battery );
}

int talker_item_const::get_count() const
{
    return me_it_const->get_item()->count();
}

int talker_item_const::coverage_at( bodypart_id &id ) const
{
    return me_it_const->get_item()->get_coverage( id );
}

int talker_item_const::encumbrance_at( bodypart_id &id ) const
{
    return me_it_const->get_item()->get_encumber( get_player_character(), id );
}

int talker_item_const::get_volume() const
{

    return units::to_milliliter( me_it_const->get_item()->volume() );
}

int talker_item_const::get_weight() const
{

    return units::to_milligram( me_it_const->get_item()->weight() );
}

void talker_item::set_value( const std::string &var_name, const std::string &value )
{
    me_it->get_item()->set_var( var_name, value );
}

void talker_item::remove_value( const std::string &var_name )
{
    me_it->get_item()->erase_var( var_name );
}

void talker_item::set_power_cur( units::energy value )
{
    me_it->get_item()->ammo_set( itype_battery, clamp( static_cast<int>( value.value() ), 0,
                                 me_it->get_item()->ammo_capacity( ammo_battery ) ) );
}

void talker_item::set_all_parts_hp_cur( int set )
{
    me_it->get_item()->set_damage( me_it->get_item()->max_damage() - set );
}

void talker_item::set_degradation( int set )
{
    me_it->get_item()->set_degradation( set );
}

void talker_item::die()
{
    me_it->remove_item();
}
