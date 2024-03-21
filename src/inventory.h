#pragma once
#ifndef CATA_SRC_INVENTORY_H
#define CATA_SRC_INVENTORY_H

#include <array>
#include <bitset>
#include <climits>
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "coordinates.h"
#include "item.h"
#include "magic_enchantment.h"
#include "proficiency.h"
#include "type_id.h"
#include "units_fwd.h"
#include "visitable.h"

class Character;
class JsonArray;
class JsonOut;
class JsonValue;
class item_stack;
class map;
class npc;
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
        explicit invlet_wrapper( const char *chars ) : std::string( chars ) { }

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

class inventory : public visitable
{
    public:

        invslice slice();
        const_invslice const_slice() const;
        const std::list<item> &const_stack( int i ) const;
        size_t size() const;

        std::map<char, itype_id> assigned_invlet;

        inventory();
        inventory( inventory && ) noexcept = default;
        inventory( const inventory & ) = default;
        inventory &operator=( inventory && ) = default;
        inventory &operator=( const inventory & ) = default;

        inventory &operator+= ( const inventory &rhs );
        inventory &operator+= ( const item &rhs );
        inventory &operator+= ( const std::list<item> &rhs );
        inventory &operator+= ( const item_components &rhs );
        inventory &operator+= ( const std::vector<item> &rhs );
        inventory &operator+= ( const item_stack &rhs );
        inventory  operator+ ( const inventory &rhs );
        inventory  operator+ ( const item &rhs );
        inventory  operator+ ( const std::list<item> &rhs );
        inventory  operator+ ( const item_components &rhs );

        void unsort(); // flags the inventory as unsorted
        void clear();
        void push_back( const std::list<item> &newits );
        // returns a reference to the added item
        item &add_item( item newit, bool keep_invlet = false, bool assign_invlet = true,
                        bool should_stack = true );
        void add_item_keep_invlet( const item &newit );
        void push_back( const item &newit );

        // provides pseudo tools (e.g. from terrain, furniture or vehicle parts )
        // @returns pointer to tool or nullptr if tool type_id already provided
        item *provide_pseudo_item( const item &tool );
        // provides pseudo tool of type \p tool_type constructed at current turn
        // @returns pointer to tool or nullptr if tool type_id is invalid or already provided
        item *provide_pseudo_item( const itype_id &tool_type );

        /* Check all items for proper stacking, rearranging as needed
         * game pointer is not necessary, but if supplied, will ensure no overlap with
         * the player's worn items / weapon
         */
        void restack( Character &p );
        void form_from_zone( map &m, std::unordered_set<tripoint_abs_ms> &zone_pts,
                             const Character *pl = nullptr, bool assign_invlet = true );
        void form_from_map( const tripoint &origin, int range, const Character *pl = nullptr,
                            bool assign_invlet = true,
                            bool clear_path = true );
        void form_from_map( map &m, const tripoint &origin, int range, const Character *pl = nullptr,
                            bool assign_invlet = true,
                            bool clear_path = true );
        void form_from_map( map &m, std::vector<tripoint> pts, const Character *pl,
                            bool assign_invlet = true );
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
        std::list<item> use_amount( const itype_id &it, int quantity,
                                    const std::function<bool( const item & )> &filter = return_true<item> );

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
        void dump( std::vector<const item *> &dest ) const;

        void json_load_invcache( const JsonValue &jsin );
        void json_load_items( const JsonArray &ja );

        void json_save_invcache( JsonOut &json ) const;
        void json_save_items( JsonOut &json ) const;

        // Assigns an invlet if any remain.  If none do, will assign ` if force is
        // true, empty (invlet = 0) otherwise.
        void assign_empty_invlet( item &it, const Character &p, bool force = false );
        // Assigns the item with the given invlet, and updates the favorite invlet cache. Does not check for uniqueness
        void reassign_item( item &it, char invlet, bool remove_old = true );
        // Removes invalid invlets, and assigns new ones if assign_invlet is true. Does not update the invlet cache.
        void update_invlet( item &it, bool assign_invlet = true,
                            const item *ignore_invlet_collision_with = nullptr );

        invlets_bitset allocated_invlets() const;

        /**
         * Returns visitable items binned by their itype.
         * May not contain items that wouldn't be visited by @ref visitable methods.
         */
        const itype_bin &get_binned_items() const;

        void update_cache_with_item( item &newit );

        void copy_invlet_of( const inventory &other );

        // gets a singular enchantment that is an amalgamation of all items that have active enchantments
        enchant_cache get_active_enchantment_cache( const Character &owner ) const;

        int count_item( const itype_id &item_type ) const;

        book_proficiency_bonuses get_book_proficiency_bonuses() const;

        // inherited from `visitable`
        bool has_quality( const quality_id &qual, int level = 1, int qty = 1 ) const override;
        VisitResponse visit_items( const std::function<VisitResponse( item *, item * )> &func ) const
        override;
        std::list<item> remove_items_with( const std::function<bool( const item & )> &filter,
                                           int count = INT_MAX ) override;
        int charges_of( const itype_id &what, int limit = INT_MAX,
                        const std::function<bool( const item & )> &filter = return_true<item>,
                        const std::function<void( int )> &visitor = nullptr, bool in_tools = false ) const override;
        int amount_of(
            const itype_id &what, bool pseudo = true, int limit = INT_MAX,
            const std::function<bool( const item & )> &filter = return_true<item> ) const override;

        std::pair<int, int> kcal_range(
            const itype_id &id,
            const std::function<bool( const item & )> &filter, Character &player_character ) const;

        // specifically used to for displaying non-empty liquid container color in crafting screen
        bool must_use_liq_container( const itype_id &id, int to_use ) const;
        bool must_use_hallu_poison( const itype_id &id, int to_use ) const;
        void update_liq_container_count( const itype_id &id, int count );
        void replace_liq_container_count( const std::map<itype_id, int> &newmap, bool use_max = false );

    private:
        invlet_favorites invlet_cache;
        char find_usable_cached_invlet( const itype_id &item_type );

        invstack items;

        std::map<itype_id, int> max_empty_liq_cont;

        // tracker for provide_pseudo_item to prevent duplicate tools/liquids
        std::set<itype_id> provisioned_pseudo_tools;

        mutable bool binned = false;
        /**
         * Items binned by their type.
         * That is, item_bin["carrot"] is a list of pointers to all carrots in inventory.
         * `mutable` because this is a pure cache that doesn't affect the contained items.
         */
        mutable itype_bin binned_items;

        mutable std::map<quality_query, bool> qualities_cache;
};

#endif // CATA_SRC_INVENTORY_H
