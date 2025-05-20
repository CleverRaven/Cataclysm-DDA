#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "avatar.h"
#include "coordinates.h"
#include "creature.h"
#include "enums.h"
#include "field.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "magic_ter_furn_transform.h"
#include "map.h"
#include "mapdata.h"
#include "point.h"
#include "submap.h"
#include "translation.h"
#include "trap.h"
#include "type_id.h"
#include "weighted_list.h"

namespace
{
generic_factory<ter_furn_transform> ter_furn_transform_factory( "ter_furn_transform" );
} // namespace

template<>
const ter_furn_transform &string_id<ter_furn_transform>::obj() const
{
    return ter_furn_transform_factory.obj( *this );
}

template<>
bool string_id<ter_furn_transform>::is_valid() const
{
    return ter_furn_transform_factory.is_valid( *this );
}

void ter_furn_transform::load_transform( const JsonObject &jo, const std::string &src )
{
    ter_furn_transform_factory.load( jo, src );
}

void ter_furn_transform::reset_all()
{
    ter_furn_transform_factory.reset();
}

bool ter_furn_transform::is_valid() const
{
    return ter_furn_transform_factory.is_valid( this->id );
}

const std::vector<ter_furn_transform> &ter_furn_transform::get_all()
{
    return ter_furn_transform_factory.get_all();
}

template<class T>
static void load_transform_results( const JsonObject &jsi, const std::string &json_key,
                                    weighted_int_list<T> &list )
{
    if( jsi.has_string( json_key ) ) {
        list.add( T( jsi.get_string( json_key ) ), 1 );
        return;
    }
    load_weighted_list( jsi.get_member( json_key ), list, 1 );
}

void ter_furn_transform::reset()
{
    ter_furn_transform_factory.reset();
}

template<class T>
void ter_furn_data<T>::load( const JsonObject &jo )
{
    load_transform_results( jo, "result", list );
    jo.read( "message", message, false );
    message_good = jo.get_bool( "message_good", true );
}

void ter_furn_transform::load( const JsonObject &jo, std::string_view )
{
    std::string input;
    mandatory( jo, was_loaded, "id", input );
    id = ter_furn_transform_id( input );

    if( jo.has_member( "terrain" ) ) {
        for( JsonObject ter_obj : jo.get_array( "terrain" ) ) {
            ter_furn_data<ter_str_id> cur_results = ter_furn_data<ter_str_id>();
            cur_results.load( ter_obj );

            for( const std::string valid_terrain : ter_obj.get_array( "valid_terrain" ) ) {
                ter_transform.emplace( ter_str_id( valid_terrain ), cur_results );
            }

            for( const std::string valid_terrain : ter_obj.get_array( "valid_flags" ) ) {
                ter_flag_transform.emplace( valid_terrain, cur_results );
            }
        }
    }

    if( jo.has_member( "furniture" ) ) {
        for( JsonObject furn_obj : jo.get_array( "furniture" ) ) {
            ter_furn_data<furn_str_id> cur_results = ter_furn_data<furn_str_id>();
            cur_results.load( furn_obj );

            for( const std::string valid_furn : furn_obj.get_array( "valid_furniture" ) ) {
                furn_transform.emplace( furn_str_id( valid_furn ), cur_results );
            }

            for( const std::string valid_terrain : furn_obj.get_array( "valid_flags" ) ) {
                furn_flag_transform.emplace( valid_terrain, cur_results );
            }
        }
    }

    if( jo.has_member( "field" ) ) {
        for( JsonObject field_obj : jo.get_array( "field" ) ) {
            ter_furn_data<field_type_id> cur_results = ter_furn_data<field_type_id>();
            cur_results.load( field_obj );

            for( const std::string valid_field : field_obj.get_array( "valid_field" ) ) {
                field_transform.emplace( field_type_id( valid_field ), cur_results );
            }
        }
    }

    if( jo.has_member( "trap" ) ) {
        for( JsonObject trap_obj : jo.get_array( "trap" ) ) {
            ter_furn_data<trap_str_id> cur_results = ter_furn_data<trap_str_id>();
            cur_results.load( trap_obj );

            for( const std::string valid_trap : trap_obj.get_array( "valid_trap" ) ) {
                trap_transform.emplace( trap_str_id( valid_trap ), cur_results );
            }

            for( const std::string valid_trap : trap_obj.get_array( "valid_flags" ) ) {
                trap_flag_transform.emplace( valid_trap, cur_results );
            }
        }
    }
}

template<class T, class K>
std::optional<ter_furn_data<T>> ter_furn_transform::find_transform( const
                             std::map<K, ter_furn_data<T>> &list, const K &key ) const
{
    const auto result_iter = list.find( key );
    if( result_iter == list.cend() ) {
        return std::nullopt;
    }
    return result_iter->second;
}

template<class T, class K>
std::optional<std::pair<T, std::pair<translation, bool>>> ter_furn_transform::next( const
        std::map<K, ter_furn_data<T>> &list,
        const K &key ) const
{
    const std::optional<ter_furn_data<T>> result = find_transform( list, key );
    if( result.has_value() ) {
        return std::make_pair( result.value().pick().value(), std::make_pair( result->message,
                               result->message_good ) );
    }
    return std::nullopt;
}

std::optional<std::pair<ter_str_id, std::pair<translation, bool>>> ter_furn_transform::next_ter(
    const ter_str_id &ter ) const
{
    return next( ter_transform, ter );
}

std::optional<std::pair<ter_str_id, std::pair<translation, bool>>> ter_furn_transform::next_ter(
    const std::string &flag ) const
{
    return next( ter_flag_transform, flag );
}

std::optional<std::pair<furn_str_id, std::pair<translation, bool>>> ter_furn_transform::next_furn(
    const furn_str_id &furn ) const
{
    return next( furn_transform, furn );
}

std::optional<std::pair<furn_str_id, std::pair<translation, bool>>> ter_furn_transform::next_furn(
    const std::string &flag ) const
{
    return next( furn_flag_transform, flag );
}

std::optional<std::pair<field_type_id, std::pair<translation, bool>>>
ter_furn_transform::next_field(
    const field_type_id &field ) const
{
    return next( field_transform, field );
}

std::optional<std::pair<trap_str_id, std::pair<translation, bool>>> ter_furn_transform::next_trap(
    const trap_str_id &trap ) const
{
    return next( trap_transform, trap );
}

std::optional<std::pair<trap_str_id, std::pair<translation, bool>>> ter_furn_transform::next_trap(
    const std::string &flag ) const
{
    return next( trap_flag_transform, flag );
}

void ter_furn_transform::transform( map &m, const tripoint_bub_ms &location ) const
{
    avatar &you = get_avatar();
    const ter_id &ter_at_loc = m.ter( location );
    std::optional<std::pair<ter_str_id, std::pair<translation, bool>>> ter_potential = next_ter(
                ter_at_loc->id );
    const furn_id &furn_at_loc = m.furn( location );
    std::optional<std::pair<furn_str_id, std::pair<translation, bool>>> furn_potential = next_furn(
                furn_at_loc->id );
    const trap_str_id trap_at_loc = m.maptile_at( location ).get_trap().id();
    std::optional<std::pair<trap_str_id, std::pair<translation, bool>>> trap_potential = next_trap(
                trap_at_loc );

    const field &field_at_loc = m.field_at( location );
    for( const auto &fld : field_at_loc ) {
        std::optional<std::pair<field_type_id, std::pair<translation, bool>>> field_potential = next_field(
                    fld.first );
        if( field_potential ) {
            m.add_field( location, field_potential->first, fld.second.get_field_intensity(),
                         fld.second.get_field_age(), true );
            m.remove_field( location, fld.first );
            if( you.sees( m, location ) && !field_potential->second.first.empty() ) {
                you.add_msg_if_player( field_potential->second.first.translated(),
                                       field_potential->second.second ? m_good : m_bad );
            }
        }
    }

    if( !ter_potential ) {
        for( const std::pair<const std::string, ter_furn_data<ter_str_id>> &flag_result :
             ter_flag_transform )             {
            if( ter_at_loc->has_flag( flag_result.first ) ) {
                ter_potential = next_ter( flag_result.first );
                if( ter_potential ) {
                    break;
                }
            }
        }
    }

    if( !furn_potential ) {
        for( const std::pair<const std::string, ter_furn_data<furn_str_id>> &flag_result :
             furn_flag_transform ) {
            if( furn_at_loc->has_flag( flag_result.first ) ) {
                furn_potential = next_furn( flag_result.first );
                if( furn_potential ) {
                    break;
                }
            }
        }
    }

    if( !trap_potential ) {
        for( const std::pair<const std::string, ter_furn_data<trap_str_id>> &flag_result :
             trap_flag_transform ) {
            if( trap_at_loc->has_flag( flag_id( flag_result.first ) ) ) {
                trap_potential = next_trap( flag_result.first );
                if( trap_potential ) {
                    break;
                }
            }
        }
    }

    if( ter_potential ) {
        m.ter_set( location, ter_potential->first );
        if( you.sees( m, location ) && !ter_potential->second.first.empty() ) {
            you.add_msg_if_player( ter_potential->second.first.translated(),
                                   ter_potential->second.second ? m_good : m_bad );
        }
    }
    if( furn_potential ) {
        m.furn_set( location, furn_potential->first );
        if( you.sees( m, location ) && !furn_potential->second.first.empty() ) {
            you.add_msg_if_player( furn_potential->second.first.translated(),
                                   furn_potential->second.second ? m_good : m_bad );
        }
    }
    if( trap_potential ) {
        m.trap_set( location, trap_potential->first );
        if( you.sees( m, location ) && !trap_potential->second.first.empty() ) {
            you.add_msg_if_player( trap_potential->second.first.translated(),
                                   trap_potential->second.second ? m_good : m_bad );
        }
    }
}

template<class T>
std::optional<T> ter_furn_data<T>::pick() const
{
    const T *picked = list.pick();
    if( picked == nullptr ) {
        return std::nullopt;
    }
    return *picked;
}

template<class T>
bool ter_furn_data<T>::has_msg() const
{
    return !message.empty();
}
