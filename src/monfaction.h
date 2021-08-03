#pragma once
#ifndef CATA_SRC_MONFACTION_H
#define CATA_SRC_MONFACTION_H

#include <cstdint>
#include <iosfwd>
#include <limits>
#include <map>
#include <set>
#include <vector>

#include "optional.h"
#include "type_id.h"

class JsonObject;
class monfaction;
template <typename E> struct enum_traits;
template <typename T> class generic_factory;

enum mf_attitude {
    MFA_BY_MOOD = 0,    // Hostile if angry
    MFA_NEUTRAL,        // Neutral even when angry
    MFA_FRIENDLY,       // Friendly
    MFA_HATE,           // Attacks on sight

    MFA_SIZE            // reserved, last element
};

template<>
struct enum_traits<mf_attitude> {
    static constexpr mf_attitude last = MFA_SIZE;
};

using mfaction_att_map = std::map<mfaction_str_id, mf_attitude>;
using mfaction_att_vec = std::vector<uint8_t>;
static_assert( std::numeric_limits<mfaction_att_vec::value_type>::max() >
               enum_traits<mf_attitude>::last,
               "mfaction_att_vec element size is insufficient to represent all values of mf_attitude" );

namespace monfactions
{
const std::vector<monfaction> &get_all();
void reset();
void finalize();
void load_monster_faction( const JsonObject &jo, const std::string &src );
} // namespace monfactions

class monfaction
{
    public:
        // returns attitude towards the other faction
        // @see attitude_rec how attitude calculation works in regards to base_faction
        mf_attitude attitude( const mfaction_id &other ) const;

        // is the root of the hierarchy formed by `base_faction` relation
        // basically same as id == base_faction
        bool is_root() const;

        mfaction_str_id id = mfaction_str_id::NULL_ID();
        mfaction_str_id base_faction = mfaction_str_id::NULL_ID();

    private:
        // used by generic_factory
        bool was_loaded = false;

        // temporary attitude cache, used by `attitude_rec`
        mutable mfaction_att_map attitude_map;
        // final attitude cache,
        // compact vector of attitudes towards other factions,
        // where index is other faction's `int_id`
        // and value is this faction -> other faction attitude
        mutable mfaction_att_vec attitude_vec;

        // attitude calculation logic
        // used internally and results is stored in attitude_vec
        cata::optional<mf_attitude> attitude_rec( const mfaction_str_id &other ) const;
        bool detect_base_faction_cycle( ) const;

        // recursively inherit `attitude_map` elements from all children
        void inherit_parent_attitude_rec(
            std::set<mfaction_str_id> &processed,
            std::map<mfaction_str_id, mf_attitude> &child_attitudes
        ) const;

        // populates attitude_vec using `attitude_rec` function
        void populate_attitude_vec() const;

        /** Load from JSON */
        void load( const JsonObject &jo, const std::string &src );

        friend void monfactions::finalize();
        friend class generic_factory<monfaction>;
};

#endif // CATA_SRC_MONFACTION_H
