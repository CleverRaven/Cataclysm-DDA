#include "field.h"

#include <algorithm>
#include <utility>

#include "calendar.h"

int field_entry::move_cost() const
{
    return type.obj().get_move_cost( intensity - 1 );
}

int field_entry::extra_radiation_min() const
{
    return type.obj().get_extra_radiation_min( intensity - 1 );
}

int field_entry::extra_radiation_max() const
{
    return type.obj().get_extra_radiation_max( intensity - 1 );
}

int field_entry::radiation_hurt_damage_min() const
{
    return type.obj().get_radiation_hurt_damage_min( intensity - 1 );
}

int field_entry::radiation_hurt_damage_max() const
{
    return type.obj().get_radiation_hurt_damage_max( intensity - 1 );
}

std::string field_entry::radiation_hurt_message() const
{
    return type.obj().get_radiation_hurt_message( intensity - 1 );
}

int field_entry::intensity_upgrade_chance() const
{
    return type.obj().get_intensity_upgrade_chance( intensity - 1 );
}

time_duration field_entry::intensity_upgrade_duration() const
{
    return type.obj().get_intensity_upgrade_duration( intensity - 1 );
}
float field_entry::light_emitted() const
{
    return type.obj().get_light_emitted( intensity - 1 );
}

float field_entry::translucency() const
{
    return type.obj().get_translucency( intensity - 1 );
}

bool field_entry::is_transparent() const
{
    return type.obj().get_transparent( intensity - 1 );
}

int field_entry::convection_temperature_mod() const
{
    return type.obj().get_convection_temperature_mod( intensity - 1 );
}

nc_color field_entry::color() const
{
    return type.obj().get_color( intensity - 1 );
}

std::string field_entry::symbol() const
{
    return type.obj().get_symbol( intensity - 1 );

}

field_type_id field_entry::get_field_type() const
{
    return type;
}

field_type_id field_entry::set_field_type( const field_type_id new_type )
{
    type = new_type;
    return type;
}

int field_entry::get_max_field_intensity() const
{
    return type.obj().get_max_intensity();
}

int field_entry::get_field_intensity() const
{
    return intensity;
}

int field_entry::set_field_intensity( int new_intensity )
{
    is_alive = new_intensity > 0;
    return intensity = std::max( std::min( new_intensity, get_max_field_intensity() ), 1 );

}

time_duration field_entry::get_field_age() const
{
    return age;
}

time_duration field_entry::set_field_age( const time_duration &new_age )
{
    return age = new_age;
}

field::field()
    : _displayed_field_type( fd_null )
{
}

/*
Function: find_field
Returns a field entry corresponding to the field_type_id parameter passed in. If no fields are found then returns NULL.
Good for checking for existence of a field: if(myfield.find_field(fd_fire)) would tell you if the field is on fire.
*/
field_entry *field::find_field( const field_type_id field_type_to_find )
{
    const auto it = _field_type_list.find( field_type_to_find );
    if( it != _field_type_list.end() ) {
        return &it->second;
    }
    return nullptr;
}

const field_entry *field::find_field_c( const field_type_id field_type_to_find ) const
{
    const auto it = _field_type_list.find( field_type_to_find );
    if( it != _field_type_list.end() ) {
        return &it->second;
    }
    return nullptr;
}

const field_entry *field::find_field( const field_type_id field_type_to_find ) const
{
    return find_field_c( field_type_to_find );
}

/*
Function: add_field
Inserts the given field_type_id into the field list for a given tile if it does not already exist.
Returns false if the field_type_id already exists, true otherwise.
If the field already exists, it will return false BUT it will add the intensity/age to the current values for upkeep.
If you wish to modify an already existing field use find_field and modify the result.
Intensity defaults to 1, and age to 0 (permanent) if not specified.
*/
bool field::add_field( const field_type_id field_type_to_add, const int new_intensity,
                       const time_duration &new_age )
{
    auto it = _field_type_list.find( field_type_to_add );
    if( field_type_to_add.obj().priority >= _displayed_field_type.obj().priority ) {
        _displayed_field_type = field_type_to_add;
    }
    if( it != _field_type_list.end() ) {
        //Already exists, but lets update it. This is tentative.
        it->second.set_field_intensity( it->second.get_field_intensity() + new_intensity );
        return false;
    }
    _field_type_list[field_type_to_add] = field_entry( field_type_to_add, new_intensity, new_age );
    return true;
}

bool field::remove_field( field_type_id const field_to_remove )
{
    const auto it = _field_type_list.find( field_to_remove );
    if( it == _field_type_list.end() ) {
        return false;
    }
    remove_field( it );
    return true;
}

void field::remove_field( std::map<field_type_id, field_entry>::iterator const it )
{
    _field_type_list.erase( it );
    if( _field_type_list.empty() ) {
        _displayed_field_type = fd_null;
    } else {
        _displayed_field_type = fd_null;
        for( auto &fld : _field_type_list ) {
            if( fld.first.obj().priority >= _displayed_field_type.obj().priority ) {
                _displayed_field_type = fld.first;
            }
        }
    }
}

/*
Function: field_count
Returns the number of fields existing on the current tile.
*/
unsigned int field::field_count() const
{
    return _field_type_list.size();
}

std::map<field_type_id, field_entry>::iterator field::begin()
{
    return _field_type_list.begin();
}

std::map<field_type_id, field_entry>::const_iterator field::begin() const
{
    return _field_type_list.begin();
}

std::map<field_type_id, field_entry>::iterator field::end()
{
    return _field_type_list.end();
}

std::map<field_type_id, field_entry>::const_iterator field::end() const
{
    return _field_type_list.end();
}

/*
Function: displayed_field_type
Returns the last added field type from the tile for drawing purposes.
*/
field_type_id field::displayed_field_type() const
{
    return _displayed_field_type;
}

int field::total_move_cost() const
{
    int current_cost = 0;
    for( auto &fld : _field_type_list ) {
        current_cost += fld.second.move_cost();
    }
    return current_cost;
}
