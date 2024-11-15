#include "field.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <utility>

#include "calendar.h"
#include "make_static.h"
#include "rng.h"

std::string field_entry::symbol() const
{
    return get_field_type()->get_symbol( get_field_intensity() - 1 );
}

nc_color field_entry::color() const
{
    return get_intensity_level().color;
}

const field_intensity_level &field_entry::get_intensity_level() const
{
    return get_field_type()->get_intensity_level( intensity - 1 );
}

bool field_entry::is_dangerous() const
{
    return get_intensity_level().dangerous;
}

bool field_entry::is_mopsafe() const
{
    return !get_intensity_level().dangerous || get_field_type()->mopsafe;
}

field_type_id field_entry::get_field_type() const
{
    return type;
}

field_type_id field_entry::set_field_type( const field_type_id &new_type )
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

void field_entry::mod_field_intensity( int mod )
{
    set_field_intensity( get_field_intensity() + mod );
}

time_duration field_entry::get_field_age() const
{
    return age;
}

time_duration field_entry::set_field_age( const time_duration &new_age )
{
    decay_time = time_point();
    return age = new_age;
}

void field_entry::initialize_decay()
{
    std::exponential_distribution<> d( 1.0f / ( M_LOG2E * to_turns<float>
                                       ( type.obj().half_life ) ) );
    const time_duration decay_delay = time_duration::from_turns( d( rng_get_engine() ) );
    decay_time = calendar::turn - age + decay_delay;
}

void field_entry::do_decay()
{
    // Bypass set_field_age() so we don't reset decay_time;
    age += 1_turns;
    if( type.obj().half_life > 0_turns && get_field_age() > 0_turns ) {
        // Legacy handling for fire because it's weird and complicated.
        if( type == STATIC( field_type_str_id( "fd_fire" ) ) ) {
            if( to_turns<int>( type->half_life ) < dice( 2, to_turns<int>( age ) ) ) {
                set_field_age( 0_turns );
                set_field_intensity( get_field_intensity() - 1 );
            }
            return;
        }
        if( decay_time == calendar::turn_zero ) {
            initialize_decay();
        }
        if( decay_time <= calendar::turn ) {
            set_field_age( 0_turns );
            set_field_intensity( get_field_intensity() - 1 );
        }
    }
}

field::field()
    : _displayed_field_type( fd_null.id_or( INVALID_FIELD_TYPE_ID ) )
{
}

/*
Function: find_field
Returns a field entry corresponding to the field_type_id parameter passed in. If no fields are found then returns NULL.
Good for checking for existence of a field: if(myfield.find_field(fd_fire)) would tell you if the field is on fire.
*/
field_entry *field::find_field( const field_type_id &field_type_to_find, const bool alive_only )
{
    if( !_displayed_field_type ) {
        return nullptr;
    }
    const auto it = _field_type_list->find( field_type_to_find );
    if( it != _field_type_list->end() && ( !alive_only || it->second.is_field_alive() ) ) {
        return &it->second;
    }
    return nullptr;
}

const field_entry *field::find_field( const field_type_id &field_type_to_find,
                                      const bool alive_only ) const
{
    if( !_displayed_field_type ) {
        return nullptr;
    }
    const auto it = _field_type_list->find( field_type_to_find );
    if( it != _field_type_list->end() && ( !alive_only || it->second.is_field_alive() ) ) {
        return &it->second;
    }
    return nullptr;
}

/*
Function: add_field
Inserts the given field_type_id into the field list for a given tile if it does not already exist.
Returns false if the field_type_id already exists, true otherwise.
If the field already exists, it will return false BUT it will add the intensity/age to the current values for upkeep.
If you wish to modify an already existing field use find_field and modify the result.
Intensity defaults to 1, and age to 0 (permanent) if not specified.
*/
bool field::add_field( const field_type_id &field_type_to_add, const int new_intensity,
                       const time_duration &new_age )
{
    // sanity check, we don't want to store fd_null
    if( !field_type_to_add ) {
        return false;
    }
    auto it = _field_type_list->find( field_type_to_add );
    if( it != _field_type_list->end() ) {
        //Already exists, but lets update it. This is tentative.
        int prev_intensity = it->second.get_field_intensity();
        if( !it->second.is_field_alive() ) {
            it->second.set_field_age( new_age );
            prev_intensity = 0;
        }
        it->second.set_field_intensity( prev_intensity + new_intensity );
        return false;
    }
    if( !_displayed_field_type ||
        field_type_to_add.obj().priority >= _displayed_field_type.obj().priority ) {
        _displayed_field_type = field_type_to_add;
    }
    _field_type_list[field_type_to_add] = field_entry( field_type_to_add, new_intensity, new_age );
    return true;
}

bool field::remove_field( const field_type_id &field_to_remove )
{
    const auto it = _field_type_list->find( field_to_remove );
    if( it == _field_type_list->end() ) {
        return false;
    }
    remove_field( it );
    return true;
}

void field::remove_field( std::map<field_type_id, field_entry>::iterator const it )
{
    _field_type_list->erase( it );
    _displayed_field_type = fd_null;
    for( auto &fld : *_field_type_list ) {
        if( !_displayed_field_type || fld.first.obj().priority >= _displayed_field_type.obj().priority ) {
            _displayed_field_type = fld.first;
        }
    }
}

void field::clear()
{
    _field_type_list->clear();
    _displayed_field_type = fd_null;
}

/*
Function: field_count
Returns the number of fields existing on the current tile.
*/
unsigned int field::field_count() const
{
    return _field_type_list->size();
}

std::map<field_type_id, field_entry>::iterator field::begin()
{
    return _field_type_list->begin();
}

std::map<field_type_id, field_entry>::const_iterator field::begin() const
{
    return _field_type_list->begin();
}

std::map<field_type_id, field_entry>::iterator field::end()
{
    return _field_type_list->end();
}

std::map<field_type_id, field_entry>::const_iterator field::end() const
{
    return _field_type_list->end();
}

/*
Function: displayed_field_type
Returns the last added field type from the tile for drawing purposes.
*/
field_type_id field::displayed_field_type() const
{
    return _displayed_field_type;
}

description_affix field::displayed_description_affix() const
{
    return _displayed_field_type.obj().desc_affix;
}

int field::displayed_intensity() const
{
    auto it = _field_type_list->find( _displayed_field_type );
    return it->second.get_field_intensity();
}

int field::total_move_cost() const
{
    int current_cost = 0;
    for( const auto &fld : *_field_type_list ) {
        current_cost += fld.second.get_intensity_level().move_cost;
    }
    return current_cost;
}

std::vector<field_effect> field_entry::field_effects() const
{
    return type->get_intensity_level( intensity - 1 ).field_effects;
}
