#pragma once
#ifndef CATA_SRC_INVENTORY_H
#define CATA_SRC_INVENTORY_H

#include <array>
#include <bitset>
#include <climits>
#include <cstddef>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "coords_fwd.h"
#include "item.h"
#include "proficiency.h"
#include "type_id.h"
#include "units_fwd.h"
#include "visitable.h"

class Character;
class JsonArray;
class JsonOut;
class JsonValue;
class item_components;
class item_stack;
class map;
class npc;

using invstack = std::list<std::list<item> >;
using invslice = std::vector<std::list<item> *>;
using const_invslice = std::vector<const std::list<item> *>;
using indexed_invslice = std::vector< std::pair<std::list<item>*, int> >;
using itype_bin = std::unordered_map< itype_id, std::list<const item *> >;
using invlets_bitset = std::bitset<std::numeric_limits<char>::max()>;

/**
 * Wrapper to handled a set of valid "inventory" letters. "inventory" can be any set of
 * objects that the player can access via a single character (e.g. bionics).
 * The class is (currently) derived from std::string for compatibility and because it's
 * simpler. But it may be changed to derive from `std::set<int>` or similar to get the full
 * range of possible characters.
 */
class invlet_wrapper : private std::string
{
    public:
        explicit invlet_wrapper( const char *chars ) : std::string( chars ) { }

        bool valid( int invlet ) const;

        // Get ordinal number (first, second, third, ...) of invlet.
        // Informs sorting order.
        int ordinal( int invlet ) const {
            return this->find( invlet );
        }

        std::string get_allowed_chars() const {
            return *this;
        }

        using std::string::begin;
        using std::string::end;
        using std::string::rbegin;
        using std::string::rend;
        using std::string::size;
        using std::string::length;
};

const extern invlet_wrapper inv_chars;

// For each item id, store a set of "favorite" inventory letters.
// This class maintains a bidirectional mapping between invlet letters and item ids.
// Each invlet has at most one id and each id has any number of invlets.
class invlet_favorites
{
    public:
        invlet_favorites() = default;
        explicit invlet_favorites( const std::unordered_map<itype_id, std::string> & );

        void set( char invlet, const itype_id & );
        void erase( char invlet );
        bool contains( char invlet, const itype_id & ) const;
        std::string invlets_for( const itype_id & ) const;

        // For serialization only
        const std::unordered_map<itype_id, std::string> &get_invlets_by_id() const;
    private:
        std::unordered_map<itype_id, std::string> invlets_by_id;
        std::array<itype_id, 256> ids_by_invlet;
};

struct quality_query {
    quality_id qual;
    int level;
    int count;

    bool operator==( const quality_query &other ) const {
        return qual == other.qual && level == other.level && count == other.count;
    }

    bool operator<( const quality_query &other ) const {
        return std::tie( qual, level, count ) < std::tie( other.qual, other.level, other.count );
    }
};

// Shared with reservation discovery, so a bound provider cannot outlive the condition
// that made it usable.
bool tile_has_sufficient_sunlight( const map &m, const tripoint_bub_ms &p );

class map_stack;
struct itype;

// Charges of the first item in the stack matching the type, or the first matching the
// ammotype with its id written back.
int count_charges_in_list( const itype *type, const map_stack &items );
int count_charges_in_list( const ammotype *ammotype, const map_stack &items,
                           itype_id &item_type );

#endif // CATA_SRC_INVENTORY_H
