#pragma once
#ifndef INVENTORY_H
#define INVENTORY_H

#include <cstddef>
#include <array>
#include <list>
#include <string>
#include <unordered_map>
#include <bitset>
#include <utility>
#include <vector>
#include <limits>
#include <functional>
#include <map>

#include "cata_utility.h"
#include "item.h"
#include "item_stack.h"
#include "visitable.h"
#include "units.h"

class map;
class npc;
class Character;
class JsonIn;
class JsonOut;
class player;
struct tripoint;

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
        invlet_wrapper( const char *chars ) : std::string( chars ) { }

        bool valid( int invlet ) const;
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
        invlet_favorites( const std::unordered_map<itype_id, std::string> & );

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

class inventory : public visitable<inventory>
{
    public:
        friend visitable<inventory>;

        invslice slice();
        const_invslice const_slice() const;
        const std::list<item> &const_stack( int i ) const;
        size_t size() const;

        std::map<char, itype_id> assigned_invlet;

        inventory();
        inventory( inventory && ) = default;
        inventory( const inventory & ) = default;
        inventory &operator=( inventory && ) = default;
        inventory &operator=( const inventory & ) = default;

        inventory &operator+= ( const inventory &rhs );
        inventory &operator+= ( const item &rhs );
        inventory &operator+= ( const std::list<item> &rhs );
        inventory &operator+= ( const std::vector<item> &rhs );
        inventory &operator+= ( const item_stack &rhs );
        inventory  operator+ ( const inventory &rhs );
        inventory  operator+ ( const item &rhs );
        inventory  operator+ ( const std::list<item> &rhs );

        void unsort(); // flags the inventory as unsorted
        void clear();
        void push_back( const std::list<item> &newits );
        // returns a reference to the added item
        item &add_item( item newit, bool keep_invlet = false, bool assign_invlet = true,
                        bool should_stack = true );
        void add_item_keep_invlet( item newit );
        void push_back( item newit );

        /* Check all items for proper stacking, rearranging as needed
         * game pointer is not necessary, but if supplied, will ensure no overlap with
         * the player's worn items / weapon
         */
        void restack( player &p );
        void form_from_map( const tripoint &origin, int range, const player *pl = nullptr,
                            bool assign_invlet = true,
                            bool clear_path = true );
        void form_from_map( map &m, const tripoint &origin, int range, const player *pl = nullptr,
                            bool assign_invlet = true,
                            bool clear_path = true );
        /**
         * Remove a specific item from the inventory. The item is compared
         * by pointer. Contents of the item are removed as well.
         * @param it A pointer to the item to be removed. The item *must* exists
         * in this inventory.
         * @return A copy of the removed item.
         */
        item remove_item( const item *it );
        item remove_item( int position );
        /**
         * Randomly select items until the volume quota is filled.
         */
        std::list<item> remove_randomly_by_volume( const units::volume &volume );
        std::list<item> reduce_stack( int position, int quantity );

        const item &find_item( int position ) const;
        item &find_item( int position );

        /**
         * Returns the item position of the stack that contains the given item (compared by
         * pointers). Returns INT_MIN if the item is not found.
         * Note that this may lose some information, for example the returned position is the
         * same when the given item points to the container and when it points to the item inside
         * the container. All items that are part of the same stack have the same item position.
         */
        int position_by_item( const item *it ) const;
        int position_by_type( const itype_id &type ) const;

        /** Return the item position of the item with given invlet, return INT_MIN if
         * the inventory does not have such an item with that invlet. Don't use this on npcs inventory. */
        int invlet_to_position( char invlet ) const;

        // Below, "amount" refers to quantity
        //        "charges" refers to charges
        std::list<item> use_amount( itype_id it, int quantity,
                                    const std::function<bool( const item & )> &filter = return_true<item> );

        bool has_tools( const itype_id &it, int quantity,
                        const std::function<bool( const item & )> &filter = return_true<item> ) const;
        bool has_components( const itype_id &it, int quantity,
                             const std::function<bool( const item & )> &filter = return_true<item> ) const;
        bool has_charges( const itype_id &it, int quantity,
                          const std::function<bool( const item & )> &filter = return_true<item> ) const;

        int leak_level( const std::string &flag ) const; // level of leaked bad stuff from items

        // NPC/AI functions
        int worst_item_value( npc *p ) const;
        bool has_enough_painkiller( int pain ) const;
        item *most_appropriate_painkiller( int pain );

        void rust_iron_items();

        units::mass weight() const;
        units::mass weight_without( const std::map<const item *, int> & ) const;
        units::volume volume() const;
        units::volume volume_without( const std::map<const item *, int> & ) const;

        // dumps contents into dest (does not delete contents)
        void dump( std::vector<item *> &dest );

        // vector rather than list because it's NOT an item stack
        // returns all items that need processing
        std::vector<item *> active_items();

        void json_load_invcache( JsonIn &jsin );
        void json_load_items( JsonIn &jsin );

        void json_save_invcache( JsonOut &json ) const;
        void json_save_items( JsonOut &json ) const;

        // Assigns an invlet if any remain.  If none do, will assign ` if force is
        // true, empty (invlet = 0) otherwise.
        void assign_empty_invlet( item &it, const Character &p, bool force = false );
        // Assigns the item with the given invlet, and updates the favorite invlet cache. Does not check for uniqueness
        void reassign_item( item &it, char invlet, bool remove_old = true );
        // Removes invalid invlets, and assigns new ones if assign_invlet is true. Does not update the invlet cache.
        void update_invlet( item &it, bool assign_invlet = true );

        void set_stack_favorite( int position, bool favorite );

        invlets_bitset allocated_invlets() const;

        /**
         * Returns visitable items binned by their itype.
         * May not contain items that wouldn't be visited by @ref visitable methods.
         */
        const itype_bin &get_binned_items() const;

        void update_cache_with_item( item &newit );

        void copy_invlet_of( const inventory &other );

    private:
        invlet_favorites invlet_cache;
        char find_usable_cached_invlet( const std::string &item_type );

        invstack items;

        mutable bool binned;
        /**
         * Items binned by their type.
         * That is, item_bin["carrot"] is a list of pointers to all carrots in inventory.
         * `mutable` because this is a pure cache that doesn't affect the contained items.
         */
        mutable itype_bin binned_items;
};

#endif
