#pragma once
#ifndef CATA_SRC_MOD_TRACKER_H
#define CATA_SRC_MOD_TRACKER_H

#include <string>

#include "cata_void.h"
#include "type_id.h"

/**
 * Mod Tracking:
 *
 * These functions live here to provide various JSON-loaded entities
 * with the ids of the mods that they are from.
 *
 * To subscribe, a JSON-loaded entity simply needs to have a
 * 'std::vector<std::pair<(string or int)_id<T>, mod_id> src;' member.
 * (As well as storing their id in an 'id' member of either string or int id type)
 *
 * If the entity is loaded with generic_factory, no further changes are needed.
 * If the entity is not loaded with generic_factory, 'assign_src()' must be called sometime
 * after the  'id' member has been assigned.
 */

/** Template magic to determine if the conditions above are satisfied */
template<typename T, typename = cata::void_t<>>
struct has_src_member : std::false_type {};

template<typename T>
struct has_src_member<T, cata::void_t<decltype( std::declval<T &>().src.emplace_back( std::declval<T &>().id, mod_id() ) )>> :
std::true_type {};

/** Dummy function, for if those conditions are not satisfied */
template < typename T, typename std::enable_if_t < !has_src_member<T>::value > * = nullptr >
void assign_src( T &, const std::string & )
{
}

/** If those conditions are satisfied, keep track of where this item has been modified */
template<typename T, typename std::enable_if_t<has_src_member<T>::value > * = nullptr>
void assign_src( T &def, const std::string &src )
{
    // We need to make sure we're keeping where this entity has been loaded
    // If the id this was last loaded with is not this one, discard the history and start again
    if( !def.src.empty() && def.src.back().first != def.id ) {
        def.src.clear();
    }
    def.src.emplace_back( def.id, mod_id( src ) );
}

#endif // CATA_SRC_MOD_TRACKER_H
