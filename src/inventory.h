#pragma once
#ifndef INVENTORY_H
#define INVENTORY_H

#include "visitable.h"
#include "item.h"
#include "enums.h"

#include <list>
#include <string>
#include <utility>
#include <vector>
#include <functional>
#include <unordered_map>

class map;
class npc;

typedef std::list< std::list<item> > invstack;
typedef std::vector< std::list<item>* > invslice;
typedef std::vector< const std::list<item>* > const_invslice;
typedef std::vector< std::pair<std::list<item>*, int> > indexed_invslice;
typedef std::unordered_map< itype_id, std::list<const item *> > itype_bin;

class salvage_actor;

/**
 * Wrapper to handled a set of valid "inventory" letters. "inventory" can be any set of
 * objects that the player can access via a single character (e.g. bionics).
 * The class is (currently) derived from std::string for compatibility and because it's
 * simpler. But it may be changed to derive from `std::set<long>` or similar to get the full
 * range of possible characters.
 */
class invlet_wrapper : private std::string
{
    private:

    public:
        invlet_wrapper( const char *chars ) : std::string( chars ) { }

        bool valid( long invlet ) const;
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
        inventory  operator+ ( const inventory &rhs );
        inventory  operator+ ( const item &rhs );
        inventory  operator+ ( const std::list<item> &rhs );

        void unsort(); // flags the inventory as unsorted
        void clear();
        void push_back( std::list<item> newits );
        // returns a reference to the added item
        item &add_item( item newit, bool keep_invlet = false, bool assign_invlet = true );
        void add_item_keep_invlet( item newit );
        void push_back( item newit );

        /* Check all items for proper stacking, rearranging as needed
         * game pointer is not necessary, but if supplied, will ensure no overlap with
         * the player's worn items / weapon
         */
        void restack( player &p );

        void form_from_map( const tripoint &origin, int distance, bool assign_invlet = true );

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
        int position_by_type( itype_id type );
        /** Return the item position of the item with given invlet, return INT_MIN if
         * the inventory does not have such an item with that invlet. Don't use this on npcs inventory. */
        int invlet_to_position( char invlet ) const;

        // Below, "amount" refers to quantity
        //        "charges" refers to charges
        std::list<item> use_amount( itype_id it, int quantity );

        bool has_tools( itype_id it, int quantity ) const;
        bool has_components( itype_id it, int quantity ) const;
        bool has_charges( itype_id it, long quantity ) const;

        int leak_level( std::string flag ) const; // level of leaked bad stuff from items

        // NPC/AI functions
        int worst_item_value( npc *p ) const;
        bool has_enough_painkiller( int pain ) const;
        item *most_appropriate_painkiller( int pain );
        item *best_for_melee( player &p, double &best );
        item *most_loaded_gun();

        void rust_iron_items();

        units::mass weight() const;
        units::volume volume() const;

        // dumps contents into dest (does not delete contents)
        void dump( std::vector<item *> &dest );

        // vector rather than list because it's NOT an item stack
        std::vector<item *> active_items();

        void json_load_invcache( JsonIn &jsin );
        void json_load_items( JsonIn &jsin );

        void json_save_invcache( JsonOut &jsout ) const;
        void json_save_items( JsonOut &jsout ) const;

        // Assigns an invlet if any remain.  If none do, will assign ` if force is
        // true, empty (invlet = 0) otherwise.
        void assign_empty_invlet( item &it, const Character &p, bool force = false );
        // Assigns the item with the given invlet, and updates the favorite invlet cache. Does not check for uniqueness
        void reassign_item( item &it, char invlet, bool remove_old = true );
        // Removes invalid invlets, and assigns new ones if assign_invlet is true. Does not update the invlet cache.
        void update_invlet( item &it, bool assign_invlet = true );

        std::set<char> allocated_invlets() const;

        /**
         * Returns visitable items binned by their itype.
         * May not contain items that wouldn't be visited by @ref visitable methods.
         */
        const itype_bin &get_binned_items() const;

        void update_cache_with_item( item &newit );

    private:
        // For each item ID, store a set of "favorite" inventory letters.
        std::map<std::string, std::vector<char> > invlet_cache;
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
