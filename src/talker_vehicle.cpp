#include <memory>

#include "avatar.h"
#include "character.h"
#include "talker_vehicle.h"
#include "vehicle.h"

std::string talker_vehicle_const::disp_name() const
{
    return me_veh_const->disp_name();
}

std::string talker_vehicle_const::get_name() const
{
    return me_veh_const->name;
}

int talker_vehicle_const::posx() const
{
    return pos_bub().x();
}

int talker_vehicle_const::posy() const
{
    return pos_bub().y();
}

int talker_vehicle_const::posz() const
{
    return pos_bub().z();
}

tripoint_bub_ms talker_vehicle_const::pos_bub() const
{
    return me_veh_const->pos_bub( get_map() );
}

tripoint_abs_ms talker_vehicle_const::pos_abs() const
{
    return me_veh_const->pos_abs();
}

tripoint_abs_omt talker_vehicle_const::pos_abs_omt() const
{
    return me_veh_const->pos_abs_omt();
}

std::optional<std::string> talker_vehicle_const::maybe_get_value( const std::string &var_name )
const
{
    return me_veh_const->maybe_get_value( var_name );
}

void talker_vehicle::set_value( const std::string &var_name, const std::string &value )
{
    me_veh->set_value( var_name, value );
}

void talker_vehicle::remove_value( const std::string &var_name )
{
    me_veh->remove_value( var_name );
}

std::vector<std::string> talker_vehicle_const::get_topics( bool ) const
{
    return me_veh_const->chat_topics;
}

bool talker_vehicle_const::will_talk_to_u( const Character &you, bool ) const
{
    return !you.is_dead_state();
}

int talker_vehicle_const::get_weight() const
{
    return units::to_milligram( me_veh_const->total_mass( get_map() ) );
}

int talker_vehicle_const::unloaded_weight() const
{
    return units::to_milligram( me_veh_const->unloaded_mass() );
}

bool talker_vehicle_const::is_driven() const
{
    //When/if npcs can drive this will need updating
    return me_veh_const->player_in_control( get_map(), get_avatar() );
}

int talker_vehicle_const::vehicle_facing() const
{
    return std::lround( units::to_degrees( me_veh_const->face.dir() + 90_degrees ) ) % 360;
}

bool talker_vehicle_const::can_fly() const
{
    return me_veh_const->is_flyable();
}

bool talker_vehicle_const::is_flying() const
{
    return me_veh_const->is_flying_in_air();
}

bool talker_vehicle_const::can_float() const
{
    return me_veh_const->can_float( get_map() );
}

bool talker_vehicle_const::is_floating() const
{
    return me_veh_const->is_watercraft() && me_veh_const->can_float( get_map() );
}

int talker_vehicle_const::current_speed() const
{
    return me_veh_const->velocity;
}

int talker_vehicle_const::friendly_passenger_count() const
{
    return me_veh_const->get_passenger_count( false );
}

int talker_vehicle_const::hostile_passenger_count() const
{
    return me_veh_const->get_passenger_count( true );
}

bool talker_vehicle_const::has_part_flag( const std::string &flag, bool enabled ) const
{
    return me_veh_const->has_part( flag, enabled );
}

bool talker_vehicle_const::is_falling() const
{
    return me_veh_const->is_falling;
}

bool talker_vehicle_const::is_skidding() const
{
    return me_veh_const->skidding;
}

bool talker_vehicle_const::is_sinking() const
{
    return me_veh_const->is_in_water( true ) && !me_veh_const->can_float( get_map() );
}

bool talker_vehicle_const::is_on_rails() const
{
    return me_veh_const->can_use_rails( get_map() );
}

bool talker_vehicle_const::is_remote_controlled() const
{
    return me_veh_const->remote_controlled( get_avatar() );
}

bool talker_vehicle_const::is_passenger( Character &you ) const
{
    return me_veh_const->is_passenger( you );
}
