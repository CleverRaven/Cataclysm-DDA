#include <iosfwd>
#include <map>
#include <new>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "creature.h"
#include "enums.h"
#include "generic_factory.h"
#include "json.h"
#include "magic_ter_furn_transform.h"
#include "map.h"
#include "mapdata.h"
#include "optional.h"
#include "submap.h"
#include "trap.h"
#include "type_id.h"

struct tripoint;
template <typename T> struct weighted_int_list;

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

template<class T>
void ter_furn_data<T>::load( const JsonObject &jo )
{
    load_transform_results( jo, "result", list );
    message = jo.get_string( "message", "" );
    message_good = jo.get_bool( "message_good", true );
}

void ter_furn_transform::load( const JsonObject &jo, const std::string & )
{
    std::string input;
    mandatory( jo, was_loaded, "id", input );
    id = ter_furn_transform_id( input );
    optional( jo, was_loaded, "fail_message", fail_message, "" );

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
cata::optional<ter_furn_data<T>> ter_furn_transform::find_transform( const
                              std::map<K, ter_furn_data<T>> &list, const K &key ) const
{
    const auto result_iter = list.find( key );
    if( result_iter == list.cend() ) {
        return cata::nullopt;
    }
    return result_iter->second;
}

template<class T, class K>
cata::optional<T> ter_furn_transform::next( const std::map<K, ter_furn_data<T>> &list,
        const K &key ) const
{
    const cata::optional<ter_furn_data<T>> result = find_transform( list, key );
    if( result ) {
        return result->pick();
    }
    return cata::nullopt;
}

cata::optional<ter_str_id> ter_furn_transform::next_ter( const ter_str_id &ter ) const
{
    return next( ter_transform, ter );
}

cata::optional<ter_str_id> ter_furn_transform::next_ter( const std::string &flag ) const
{
    return next( ter_flag_transform, flag );
}

cata::optional<furn_str_id> ter_furn_transform::next_furn( const furn_str_id &furn ) const
{
    return next( furn_transform, furn );
}

cata::optional<furn_str_id> ter_furn_transform::next_furn( const std::string &flag ) const
{
    return next( furn_flag_transform, flag );
}

cata::optional<field_type_id> ter_furn_transform::next_field( const field_type_id &field ) const
{
    return next( field_transform, field );
}

cata::optional<trap_str_id> ter_furn_transform::next_trap( const trap_str_id &trap ) const
{
    return next( trap_transform, trap );
}

cata::optional<trap_str_id> ter_furn_transform::next_trap( const std::string &flag ) const
{
    return next( trap_flag_transform, flag );
}

template<class T, class K>
bool ter_furn_transform::add_message( const std::map<K, ter_furn_data<T>> &list, const K &key,
                                      const Creature &critter, const tripoint &location ) const
{
    const cata::optional<ter_furn_data<T>> result = find_transform( list, key );
    if( result && !result->has_msg() ) {
        if( critter.sees( location ) ) {
            result->add_msg( critter );
        }
        return true;
    }
    return false;
}

void ter_furn_transform::add_all_messages( const Creature &critter, const tripoint &location ) const
{
    add_all_messages( get_map(), critter, location );
}

void ter_furn_transform::add_all_messages( const map &m, const Creature &critter,
        const tripoint &location ) const
{
    const ter_id ter_at_loc = m.ter( location );
    if( !add_message( ter_transform, ter_at_loc->id, critter, location ) ) {
        for( const std::pair<const std::string, ter_furn_data<ter_str_id>> &data : ter_flag_transform ) {
            if( data.second.has_msg() && ter_at_loc->has_flag( data.first ) ) {
                data.second.add_msg( critter );
                break;
            }
        }
    }

    const furn_id furn_at_loc = m.furn( location );
    if( !add_message( furn_transform, furn_at_loc->id, critter, location ) ) {
        for( const std::pair<const std::string, ter_furn_data<furn_str_id>> &data : furn_flag_transform ) {
            if( data.second.has_msg() && furn_at_loc->has_flag( data.first ) ) {
                data.second.add_msg( critter );
                break;
            }
        }
    }

    const field &field_at_loc = m.field_at( location );
    for( auto &fld : field_at_loc ) {
        add_message( field_transform, fld.first, critter, location );
    }

    const trap_str_id trap_at_loc = m.maptile_at( location ).get_trap().id();
    add_message( trap_transform, trap_at_loc, critter, location );
}

void ter_furn_transform::transform( const tripoint &location ) const
{
    transform( get_map(), location );
}

void ter_furn_transform::transform( map &m, const tripoint &location ) const
{
    const ter_id ter_at_loc = m.ter( location );
    cata::optional<ter_str_id> ter_potential = next_ter( ter_at_loc->id );
    const furn_id furn_at_loc = m.furn( location );
    cata::optional<furn_str_id> furn_potential = next_furn( furn_at_loc->id );
    const trap_str_id trap_at_loc = m.maptile_at( location ).get_trap().id();
    cata::optional<trap_str_id> trap_potential = next_trap( trap_at_loc );

    const field &field_at_loc = m.field_at( location );
    for( auto &fld : field_at_loc ) {
        cata::optional<field_type_id> field_potential = next_field( fld.first );
        if( field_potential ) {
            m.add_field( location, *field_potential, fld.second.get_field_intensity(),
                         fld.second.get_field_age(), true );
            m.remove_field( location, fld.first );
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
        m.ter_set( location, *ter_potential );
    }
    if( furn_potential ) {
        m.furn_set( location, *furn_potential );
    }
    if( trap_potential ) {
        m.trap_set( location, *trap_potential );
    }
}

template<class T>
cata::optional<T> ter_furn_data<T>::pick() const
{
    const T *picked = list.pick();
    if( picked == nullptr ) {
        return cata::nullopt;
    }
    return *picked;
}

template<class T>
bool ter_furn_data<T>::has_msg() const
{
    return !message.empty();
}

template<class T>
void ter_furn_data<T>::add_msg( const Creature &critter ) const
{
    critter.add_msg_if_player( message_good ? m_good : m_bad, message );
}
