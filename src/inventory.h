#ifndef INVENTORY_H
#define INVENTORY_H

#include "item.h"
#include "enums.h"

#include <list>
#include <string>
#include <utility>
#include <vector>

class map;

const extern std::string inv_chars;

typedef std::list< std::list<item> > invstack;
typedef std::vector< std::list<item>* > invslice;
typedef std::vector< const std::list<item>* > const_invslice;
typedef std::vector< std::pair<std::list<item>*, int> > indexed_invslice;

class inventory
{
    public:
        invslice slice();
        const_invslice const_slice() const;
        const std::list<item> &const_stack(int i) const;
        size_t size() const;
        int num_items() const;
        bool is_sorted() const;

        inventory();
        inventory(inventory &&) = default;
        inventory(const inventory &) = default;
        inventory &operator=(inventory &&) = default;
        inventory &operator=(const inventory &) = default;

        inventory &operator+= (const inventory &rhs);
        inventory &operator+= (const item &rhs);
        inventory &operator+= (const std::list<item> &rhs);
        inventory &operator+= (const std::vector<item> &rhs);
        inventory  operator+  (const inventory &rhs);
        inventory  operator+  (const item &rhs);
        inventory  operator+  (const std::list<item> &rhs);

        static bool has_activation(const item &it, const player &u);
        static bool has_category(const item &it, item_cat cat, const player &u);
        static bool has_capacity_for_liquid(const item &it, const item &liquid);

        indexed_invslice slice_filter();  // unfiltered, but useful for a consistent interface.
        indexed_invslice slice_filter_by_activation(const player &u);
        indexed_invslice slice_filter_by_category(item_cat cat, const player &u);
        indexed_invslice slice_filter_by_capacity_for_liquid(const item &liquid);
        indexed_invslice slice_filter_by_flag(const std::string flag);
        indexed_invslice slice_filter_by_salvageability();

        void unsort(); // flags the inventory as unsorted
        void sort();
        void clear();
        void add_stack(std::list<item> newits);
        void clone_stack(const std::list<item> &rhs);
        void push_back(std::list<item> newits);
        item &add_item (item newit, bool keep_invlet = false,
                        bool assign_invlet = true); //returns a ref to the added item
        void add_item_keep_invlet(item newit);
        void push_back(item newit);

        /* Check all items for proper stacking, rearranging as needed
         * game pointer is not necessary, but if supplied, will ensure no overlap with
         * the player's worn items / weapon
         */
        void restack(player *p = NULL);

        void form_from_map(point origin, int distance, bool assign_invlet = true);

        /**
         * Remove a specific item from the inventory. The item is compared
         * by pointer. Contents of the item are removed as well.
         * @param it A pointer to the item to be removed. The item *must* exists
         * in this inventory.
         * @return A copy of the removed item.
         */
        item remove_item(const item *it);
        item remove_item(int position);
        std::list<item> reduce_stack(int position, int quantity);
        std::list<item> reduce_stack(const itype_id &type, int quantity);

        // amount of -1 removes the entire stack.
        template<typename Locator> std::list<item> reduce_stack(const Locator &type, int amount);

        item &find_item(int position);
        item &item_by_type(itype_id type);
        item &item_or_container(itype_id type); // returns an item, or a container of it

        int position_by_item(const item *it);  // looks up an item (via pointer comparison)
        int position_by_type(itype_id type);
        /** Return the item position of the item with given invlet, return INT_MIN if
         * the inventory does not have such an item with that invlet. Don't use this on npcs inventory. */
        int invlet_to_position(char invlet) const;

        std::vector<std::pair<item *, int> > all_items_by_type(itype_id type);
        std::vector<item *> all_ammo(const ammotype &type);

        // Below, "amount" refers to quantity
        //        "charges" refers to charges
        int  amount_of (itype_id it) const;
        int  amount_of (itype_id it, bool used_as_tool) const;
        long charges_of(itype_id it) const;

        std::list<item> use_amount (itype_id it, int quantity, bool use_container = false);
        std::list<item> use_charges(itype_id it, long quantity);

        bool has_amount (itype_id it, int quantity) const;
        bool has_amount (itype_id it, int quantity, bool used_as_tool) const;
        bool has_tools (itype_id it, int quantity) const;
        bool has_components (itype_id it, int quantity) const;
        bool has_charges(itype_id it, long quantity) const;
        /**
         * Check whether a specific item is in this inventory.
         * The item is compared by pointer.
         * @param it A pointer to the item to be looked for.
         */
        bool has_item(const item *it) const;
        bool has_items_with_quality(std::string id, int level, int amount) const;

        static int num_items_at_position( int position );

        int leak_level(std::string flag) const; // level of leaked bad stuff from items

        int butcher_factor() const;

        // NPC/AI functions
        int worst_item_value(npc *p) const;
        bool has_enough_painkiller(int pain) const;
        item *most_appropriate_painkiller(int pain);
        item *best_for_melee(player *p);
        item *most_loaded_gun();

        void rust_iron_items();

        int weight() const;
        int volume() const;

        void dump(std::vector<item *> &dest); // dumps contents into dest (does not delete contents)

        // vector rather than list because it's NOT an item stack
        std::vector<item *> active_items();

        void load_invlet_cache( std::ifstream &fin ); // see savegame_legacy.cpp

        void json_load_invcache(JsonIn &jsin);
        void json_load_items(JsonIn &jsin);

        void json_save_invcache(JsonOut &jsout) const;
        void json_save_items(JsonOut &jsout) const;

        item nullitem;
        std::list<item> nullstack;

        // Assigns an invlet if any remain.  If none do, will assign ` if force is
        // true, empty (invlet = 0) otherwise.
        void assign_empty_invlet(item &it, bool force = false);

        std::set<char> allocated_invlets() const;

        template<typename T>
        static void items_with_recursive( std::vector<const item *> &vec, const item &it, T filter )
        {
            if( filter( it ) ) {
                vec.push_back( &it );
            }
            for( auto &c : it.contents ) {
                items_with_recursive( vec, c, filter );
            }
        }
        template<typename T>
        static bool has_item_with_recursive( const item &it, T filter )
        {
            if( filter( it ) ) {
                return true;
            }
            for( auto &c : it.contents ) {
                if( has_item_with_recursive( c, filter ) ) {
                    return true;
                }
            }
            return false;
        }
        template<typename T>
        bool has_item_with(T filter) const
        {
            for( auto &stack : items ) {
                for( auto &it : stack ) {
                    if( has_item_with_recursive( it, filter ) ) {
                        return true;
                    }
                }
            }
            return false;
        }
        template<typename T>
        std::vector<const item *> items_with(T filter) const
        {
            std::vector<const item *> result;
            for( auto &stack : items ) {
                for( auto &it : stack ) {
                    inventory::items_with_recursive( result, it, filter );
                }
            }
            return result;
        }
        template<typename T>
        std::list<item> remove_items_with( T filter )
        {
            std::list<item> result;
            for( auto items_it = items.begin(); items_it != items.end(); ) {
                auto &stack = *items_it;
                for( auto stack_it = stack.begin(); stack_it != stack.end(); ) {
                    if( filter( *stack_it ) ) {
                        result.push_back( std::move( *stack_it ) );
                        stack_it = stack.erase( stack_it );
                        if( stack_it == stack.begin() && !stack.empty() ) {
                            // preserve the invlet when removing the first item of a stack
                            stack_it->invlet = result.back().invlet;
                        }
                    } else {
                        result.splice( result.begin(), stack_it->remove_items_with( filter ) );
                        ++stack_it;
                    }
                }
                if( stack.empty() ) {
                    items_it = items.erase( items_it );
                } else {
                    ++items_it;
                }
            }
            return result;
        }
    private:
        // For each item ID, store a set of "favorite" inventory letters.
        std::map<std::string, std::vector<char> > invlet_cache;
        void update_cache_with_item(item &newit);
        char find_usable_cached_invlet(const std::string &item_type);

        // Often items can be located using typeid, position, or invlet.  To reduce code duplication,
        // we back those functions with a single internal function templated on the type of Locator.
        template<typename Locator> item remove_item_internal(const Locator &locator);
        template<typename Locator> std::list<item> reduce_stack_internal(const Locator &type, int amount);

        invstack items;
        bool sorted;
};

#endif
