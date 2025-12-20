#pragma once
#ifndef CATA_SRC_ITEM_FACTORY_H
#define CATA_SRC_ITEM_FACTORY_H

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "generic_factory.h"
#include "item.h"
#include "itype.h"
#include "iuse.h"
#include "type_id.h"
#include "units_fwd.h"

class Item_group;
class Item_spawn_data;
class relic;
class translation;

namespace cata
{
template <typename T> class value_ptr;
}  // namespace cata

/**
 * Blacklists are an old way to remove items from the game.
 * It doesn't change at runtime.
 */
bool item_is_blacklisted( const itype_id &id );

using item_action_id = std::string;
using Item_list = std::vector<item>;

class Item_factory;
class JsonArray;
class JsonObject;

extern std::unique_ptr<Item_factory> item_controller;

/**
* See OBSOLETION_AND_MIGRATION.md
*/
class migration
{
    public:
        itype_id id;
        std::string variant;
        std::optional<std::string> from_variant;
        itype_id replace;
        std::set<std::string> flags;
        int charges = 0;

        // if set to true then reset item_vars std::map to the value of itype's item_variables
        bool reset_item_vars = false;

        class content
        {
            public:
                itype_id id;
                int count = 0;

                bool operator==( const content & ) const;
                void deserialize( const JsonObject &jsobj );
        };
        std::vector<content> contents;
        bool sealed = true;
};

struct item_blacklist_t {
    std::set<itype_id> blacklist;

    std::vector<std::pair<bool, std::set<itype_id>>> sub_blacklist;

    void clear();
};

/**
 * Central item type management class.
 * It contains a map of all item types, accessible via @ref find_template. Those item types are
 * loaded from json via the load_* functions.
 *
 * It also contains all item groups that are used to spawn items.
 *
 * You usually use the single global instance @ref item_controller.
 */
class Item_factory
{
        friend class generic_factory<itype>;
        friend struct itype;
    public:
        generic_factory<itype> item_factory = generic_factory<itype>( "ITEM" );

        Item_factory();
        ~Item_factory();

        generic_factory<itype> &get_generic_factory() {
            return item_factory;
        }

        //initialize use_functions
        void init();
        /**
         * Reset the item factory. All item type definitions and item groups are erased.
         */
        void reset();
        /**
         * Check consistency of itype and item group definitions, for example
         * valid material, valid tool, etc.
         * This should be called once after all json data has been loaded.
         */
        void check_definitions() const;

        /**
         * @name Item groups
         *
         * Item groups are used to spawn random items (in random amounts).
         * You usually only need the @ref item_group::items_from function to create items
         * from a group.
         */
        /*@{*/
        /**
         * Callback for the init system (@ref DynamicDataLoader), loads an item group definitions.
         * @param jsobj The json object to load from.
         * @throw JsonError if the json object contains invalid data.
         */
        void load_item_group( const JsonObject &jsobj );
        /**
         * Load a item group from json. It differs from the other load_item_group function as it
         * uses the ident and subtype given as parameter and does not look up those things in
         * the json data (the json object here does not need to have them).
         * This is intended for inline definitions of item groups, e.g. in monster death drops:
         * the item group there is embedded into the monster type definition.
         * @param jsobj The json object to load from.
         * @param group_id The ident of the item that is to be loaded.
         * @param subtype The type of the item group, either "collection", "distribution" or "old"
         * ("old" is a distribution, too).
         * @throw JsonError if the json object contains invalid data.
         */
        void load_item_group( const JsonObject &jsobj, const item_group_id &group_id,
                              const std::string &subtype, std::string context = {} );

        /**
        * Actually the load item group data into the group. Matches the above function, which merely
        * organises which group to load into and the type.
        * @param jsobj The json object to load from.
        * @param group_id The ident of the item that is to be loaded.
        * @param subtype The type of the item group, either "collection", "distribution" or "old"
        * ("old" is a distribution, too).
        */
        void load_item_group_data( const JsonObject &jsobj, Item_group *ig,
                                   const std::string &subtype );
        /**
         * Like above, but the above loads data from several members of the object, this function
         * assume the given array is the "entries" member of the item group.
         *
         * For each element in the array, @ref Item_factory::add_entry is called.
         *
         * Assuming the input array looks like `[ x, y, z ]`, this function loads it like the
         * above would load this object:
         * \code
         * {
         *      "subtype": "depends on is_collection parameter",
         *      "id": "identifier",
         *      "entries": [ x, y, z ]
         * }
         * \endcode
         * Note that each entry in the array has to be a JSON object. The other function above
         * can also load data from arrays of strings, where the strings are item or group ids.
         */
        void load_item_group( const JsonArray &entries, const item_group_id &group_id,
                              bool is_collection, int ammo_chance, int magazine_chance,
                              std::string context = {} );
        /**
         * Get the item group object. Returns null if the item group does not exists.
         */
        Item_spawn_data *get_group( const item_group_id & );
        /**
         * Returns the idents of all item groups that are known.
         */
        std::vector<item_group_id> get_all_group_names();

        /**
          *  a temporary function to aid in nested container migration of magazine and gun json
          *  - creates a magazine pocket if none is specified and the islot is loaded
          */
        void check_and_create_magazine_pockets( itype &def );
        /**
         * adds the pockets that are not encoded in json - CORPSE, MOD, etc.
         */
        void add_special_pockets( itype &def );

        /** called after all JSON has been read and performs any necessary cleanup tasks */
        void finalize();

        /** Migrations transform items loaded from legacy saves */
        void load_migration( const JsonObject &jo );

        // Add or overwrite a migration
        void add_migration( const migration &m );

        /** Applies any migration of the item id */
        itype_id migrate_id( const itype_id &id );

        /**
         * Applies any migrations to an instance of an item
         * @param id the original id (before any replacement)
         * @param obj The instance
         * @see Item_factory::migrate_id
         */
        void migrate_item( const itype_id &id, item &obj );

        /** applies a migration to the item if one exists with the given from_variant */
        void migrate_item_from_variant( item &obj, const std::string &from_variant );

        /**
         * Add itype_id to m_runtimes.
         *
         * If the itype overrides an existing itype, the existing itype is deleted first.
         * Return the newly created itype.
         */
        const itype *add_runtime( const itype_id &id, translation name, translation description ) const;

        /**
         * Check if an item type is known to the Item_factory.
         * @param id Item type id (@ref itype::id).
         */
        bool has_template( const itype_id &id ) const;

        /**
         * Returns the itype with the given id.
         * This function never returns null, if the item type is unknown, a new item type is
         * generated, stored and returned.
         * @param id Item type id (@ref itype::id).
         */
        const itype *find_template( const itype_id &id ) const;

        static inline std::map<item_action_id, use_function> iuse_function_list;
        static std::set<std::string> repair_actions;

        static use_function usage_from_string( const std::string &type );

        static use_function read_use_function( const JsonObject &jo, std::map<std::string, int> &ammo_scale,
                                               std::string &type );

        //iuse stuff
        static void add_iuse( const std::string &type, use_function_pointer f );
        static void add_iuse( const std::string &type, use_function_pointer f,
                              const translation &info );
        static void add_actor( std::unique_ptr<iuse_actor> ptr );

        std::map<itype_id, std::vector<migration>> migrations;
        /**
         * Check if an iuse is known to the Item_factory.
         * @param type Iuse type id.
         */
        static bool has_iuse( const item_action_id &type );

        void load_item_blacklist( const JsonObject &json );

        /** Get all item templates (both static and runtime) */
        const std::vector<const itype *> &all() const;

        /** Find all item templates (both static and runtime) matching UnaryPredicate function */
        static std::vector<const itype *> find( const std::function<bool( const itype & )> &func );
        /**
         * All armor that can be used as a container. Much faster than iterating all items.
         *
         * Return begin and end iterators.
         */
        std::pair<std::vector<item>::const_iterator, std::vector<item>::const_iterator>
        get_armor_containers( units::volume min_volume ) const;

        std::list<itype_id> subtype_replacement( const itype_id & ) const;

    private:
        /** Set at finalization and prevents alterations to the static item templates */
        bool frozen = false;

        mutable std::map<itype_id, std::unique_ptr<itype>> m_runtimes;
        /** Runtimes rarely change. Used for cache templates_all_cache for the all() method. */
        mutable bool m_runtimes_dirty = true;
        mutable std::vector<const itype *> templates_all_cache;

        using GroupMap = std::map<item_group_id, std::unique_ptr<Item_spawn_data>>;
        GroupMap m_template_groups;

        std::unordered_map<itype_id, ammotype> migrated_ammo;
        std::unordered_map<itype_id, itype_id> migrated_magazines;

        /**
         * Cache for armor_containers.
         *
         * If `!armor_containers.empty()` then cache is valid. When valid they have the same size
         * and `armor_containers[i]` has the biggest pocket volume equal to `volumes[i]`.
         */
        mutable std::vector<item> armor_containers;
        mutable std::vector<units::volume> volumes;

        /** Checks that ammo is listed in ammunition_type::name().
         * At least one instance of this ammo type should be defined.
         * If any of checks fails, prints a message to the msg stream.
         * @param msg Stream in which all error messages are printed.
         * @param ammo Ammo type to check.
         */
        bool check_ammo_type( std::string &msg, const ammotype &ammo ) const;

        template<typename SlotType>
        static void load_slot( const JsonObject &jo, bool was_loaded,
                               cata::value_ptr<SlotType> &slotptr );

        void load( relic &slot, const JsonObject &jo, std::string_view src );

        //json data handlers
        void emplace_usage( std::map<std::string, use_function> &container,
                            const std::string &iuse_id );

        /**
         * Helper function for Item_group loading
         *
         * If obj contains an array or string titled name + "-item" or name + "-group",
         * this resets ptr and adds the item(s) or group(s) to it.
         *
         * @param ptr Data we operate on, results are stored here
         * @param obj Json object being searched
         * @param name Name of item or group we are searching for
         * @param parent The item group that obj is in. Used so that the result's magazine and ammo
         * probabilities can be inherited.
         * @returns Whether anything was loaded.
         */
        bool load_sub_ref( std::unique_ptr<Item_spawn_data> &ptr, const JsonObject &obj,
                           const std::string &name, const Item_group &parent );
        bool load_string( std::vector<std::string> &vec, const JsonObject &obj, std::string_view name );
        void add_entry( Item_group &ig, const JsonObject &obj, const std::string &context );

        // declared here to have friendship status with itype
        static void npc_implied_flags( itype &item_template );
        static void set_allergy_flags( itype &item_template );

        void clear();

        void finalize_item_blacklist();

        /** Applies part of finalization that don't depend on other items. */
        void finalize_pre( itype &obj );
        /** Registers the item as having repair actions (if it has any). */
        void register_cached_uses( const itype &obj );
        /** Applies part of finalization that depends on other items. */
        void finalize_post( itype &obj );

        void finalize_post_armor( itype &obj );

        /**
         * Contains the tool subtype mappings for crafting (i.e. mess kit is a hotplate etc.).
         * This is should be obsoleted when @ref requirement_data allows AND/OR nesting.
         */
        std::map<itype_id, std::set<itype_id>> tool_subtypes;

        // tools that have at least one repair action
        std::set<itype_id> repair_tools;

        // tools that can be used to repair complex firearms
        std::set<itype_id> gun_tools;
};

namespace items
{
/** Load all items */
void load( const JsonObject &jo, const std::string &src );
/** Finalize all loaded items */
void finalize_all();
/** Clear all loaded items (invalidating any pointers) */
void reset();
/** Checks all loaded items are valid */
void check_consistency();
}  // namespace items
#endif // CATA_SRC_ITEM_FACTORY_H
