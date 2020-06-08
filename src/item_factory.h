#pragma once
#ifndef CATA_SRC_ITEM_FACTORY_H
#define CATA_SRC_ITEM_FACTORY_H

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "item.h"
#include "itype.h"
#include "iuse.h"
#include "type_id.h"

class Item_group;
class Item_spawn_data;
class relic;

namespace cata
{
template <typename T> class value_ptr;
}  // namespace cata

bool item_is_blacklisted( const itype_id &id );

using Group_tag = std::string;
using item_action_id = std::string;
using Item_list = std::vector<item>;

class Item_factory;
class JsonArray;
class JsonObject;

extern std::unique_ptr<Item_factory> item_controller;

class migration
{
    public:
        itype_id id;
        itype_id replace;
        std::set<std::string> flags;
        int charges = 0;
        std::set<itype_id> contents;
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
    public:
        Item_factory();
        ~Item_factory();
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
        void load_item_group( const JsonObject &jsobj, const Group_tag &group_id,
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
         *      "id": "ident",
         *      "entries": [ x, y, z ]
         * }
         * \endcode
         * Note that each entry in the array has to be a JSON object. The other function above
         * can also load data from arrays of strings, where the strings are item or group ids.
         */
        void load_item_group( const JsonArray &entries, const Group_tag &group_id, bool is_collection,
                              int ammo_chance, int magazine_chance );
        /**
         * Get the item group object. Returns null if the item group does not exists.
         */
        Item_spawn_data *get_group( const Group_tag &group_tag );
        /**
         * Returns the idents of all item groups that are known.
         */
        std::vector<Group_tag> get_all_group_names();
        /**
         * Sets the chance of the specified item in the group.
         * @param group_id Group to add item to
         * @param item_id Id of item to add to group
         * @param chance The relative weight of the item. A value of 0 removes the item from the
         * group.
         * @return false if the group doesn't exist.
         */
        bool add_item_to_group( const Group_tag &group_id, const itype_id &item_id, int chance );
        /*@}*/

        /**
         * @name Item type loading
         *
         * These function load different instances of itype objects from json.
         * The loaded item types are stored and can be accessed through @ref find_template.
         * @param jo The json object to load data from.
         * @throw JsonError if the json object contains invalid data.
         */
        /*@{*/
        void load_ammo( const JsonObject &jo, const std::string &src );
        void load_gun( const JsonObject &jo, const std::string &src );
        void load_armor( const JsonObject &jo, const std::string &src );
        void load_pet_armor( const JsonObject &jo, const std::string &src );
        void load_tool( const JsonObject &jo, const std::string &src );
        void load_toolmod( const JsonObject &jo, const std::string &src );
        void load_tool_armor( const JsonObject &jo, const std::string &src );
        void load_book( const JsonObject &jo, const std::string &src );
        void load_comestible( const JsonObject &jo, const std::string &src );
        void load_engine( const JsonObject &jo, const std::string &src );
        void load_wheel( const JsonObject &jo, const std::string &src );
        void load_fuel( const JsonObject &jo, const std::string &src );
        void load_gunmod( const JsonObject &jo, const std::string &src );
        void load_magazine( const JsonObject &jo, const std::string &src );
        void load_battery( const JsonObject &jo, const std::string &src );
        void load_generic( const JsonObject &jo, const std::string &src );
        void load_bionic( const JsonObject &jo, const std::string &src );
        /*@}*/

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

        /** Applies any migration of the item id */
        itype_id migrate_id( const itype_id &id );

        /**
         * Applies any migrations to an instance of an item
         * @param id the original id (before any replacement)
         * @param obj The instance
         * @see Item_factory::migrate_id
         */
        void migrate_item( const itype_id &id, item &obj );

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

        /**
         * Add a passed in itype to the collection of item types.
         * If the item type overrides an existing type, the existing type is deleted first.
         * @param def The new item type, must not be null.
         */
        void add_item_type( const itype &def );

        /**
         * Check if an iuse is known to the Item_factory.
         * @param type Iuse type id.
         */
        bool has_iuse( const item_action_id &type ) const {
            return iuse_function_list.find( type ) != iuse_function_list.end();
        }

        void load_item_blacklist( const JsonObject &json );

        /** Get all item templates (both static and runtime) */
        std::vector<const itype *> all() const;

        /** Get item types created at runtime. */
        std::vector<const itype *> get_runtime_types() const;

        /** Find all item templates (both static and runtime) matching UnaryPredicate function */
        static std::vector<const itype *> find( const std::function<bool( const itype & )> &func );

        /**
         * Create a new (and currently unused) item type id.
         */
        itype_id create_artifact_id() const;

        std::list<itype_id> subtype_replacement( const itype_id & ) const;

    private:
        /** Set at finalization and prevents alterations to the static item templates */
        bool frozen = false;

        std::map<itype_id, itype> m_abstracts;

        std::unordered_map<itype_id, itype> m_templates;

        mutable std::map<itype_id, std::unique_ptr<itype>> m_runtimes;

        using GroupMap = std::map<Group_tag, std::unique_ptr<Item_spawn_data>>;
        GroupMap m_template_groups;

        std::unordered_map<itype_id, ammotype> migrated_ammo;
        std::unordered_map<itype_id, itype_id> migrated_magazines;

        /** Checks that ammo is listed in ammunition_type::name().
         * At least one instance of this ammo type should be defined.
         * If any of checks fails, prints a message to the msg stream.
         * @param msg Stream in which all error messages are printed.
         * @param ammo Ammo type to check.
         */
        bool check_ammo_type( std::string &msg, const ammotype &ammo ) const;

        /**
         * Called before creating a new template and handles inheritance via copy-from
         * May defer instantiation of the template if depends on other objects not as-yet loaded
         */
        bool load_definition( const JsonObject &jo, const std::string &src, itype &def );

        /**
         * Load the data of the slot struct. It creates the slot object (of type SlotType) and
         * and calls @ref load to do the actual (type specific) loading.
         */
        template<typename SlotType>
        void load_slot( cata::value_ptr<SlotType> &slotptr, const JsonObject &jo, const std::string &src );

        /**
         * Load item the item slot if present in json.
         * Checks whether the json object has a member of the given name and if so, loads the item
         * slot from that object. If the member does not exists, nothing is done.
         */
        template<typename SlotType>
        void load_slot_optional( cata::value_ptr<SlotType> &slotptr, const JsonObject &jo,
                                 const std::string &member, const std::string &src );

        void load( islot_tool &slot, const JsonObject &jo, const std::string &src );
        void load( islot_comestible &slot, const JsonObject &jo, const std::string &src );
        void load( islot_brewable &slot, const JsonObject &jo, const std::string &src );
        void load( islot_mod &slot, const JsonObject &jo, const std::string &src );
        void load( islot_engine &slot, const JsonObject &jo, const std::string &src );
        void load( islot_wheel &slot, const JsonObject &jo, const std::string &src );
        void load( islot_fuel &slot, const JsonObject &jo, const std::string &src );
        void load( islot_gun &slot, const JsonObject &jo, const std::string &src );
        void load( islot_gunmod &slot, const JsonObject &jo, const std::string &src );
        void load( islot_magazine &slot, const JsonObject &jo, const std::string &src );
        void load( islot_battery &slot, const JsonObject &jo, const std::string &src );
        void load( islot_bionic &slot, const JsonObject &jo, const std::string &src );
        void load( islot_artifact &slot, const JsonObject &jo, const std::string &src );
        void load( relic &slot, const JsonObject &jo, const std::string &src );

        //json data handlers
        void emplace_usage( std::map<std::string, use_function> &container, const std::string &iuse_id );

        void set_use_methods_from_json( const JsonObject &jo, const std::string &member,
                                        std::map<std::string, use_function> &use_methods );

        use_function usage_from_string( const std::string &type ) const;

        std::pair<std::string, use_function> usage_from_object( const JsonObject &obj );

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
        bool load_string( std::vector<std::string> &vec, const JsonObject &obj, const std::string &name );
        void add_entry( Item_group &ig, const JsonObject &obj );

        void load_basic_info( const JsonObject &jo, itype &def, const std::string &src );
        void set_qualities_from_json( const JsonObject &jo, const std::string &member, itype &def );
        void extend_qualities_from_json( const JsonObject &jo, const std::string &member, itype &def );
        void delete_qualities_from_json( const JsonObject &jo, const std::string &member, itype &def );
        void set_properties_from_json( const JsonObject &jo, const std::string &member, itype &def );

        void clear();
        void init();

        void finalize_item_blacklist();

        /** Applies part of finalization that don't depend on other items. */
        void finalize_pre( itype &obj );
        /** Registers the item as having repair actions (if it has any). */
        void register_cached_uses( const itype &obj );
        /** Applies part of finalization that depends on other items. */
        void finalize_post( itype &obj );

        //iuse stuff
        std::map<item_action_id, use_function> iuse_function_list;

        void add_iuse( const std::string &type, use_function_pointer f );
        void add_iuse( const std::string &type, use_function_pointer f,
                       const std::string &info );
        void add_actor( std::unique_ptr<iuse_actor> );

        std::map<itype_id, migration> migrations;

        /**
         * Contains the tool subtype mappings for crafting (i.e. mess kit is a hotplate etc.).
         * This is should be obsoleted when @ref requirement_data allows AND/OR nesting.
         */
        std::map<itype_id, std::set<itype_id>> tool_subtypes;

        // tools that have at least one repair action
        std::set<itype_id> repair_tools;

        // tools that can be used to repair complex firearms
        std::set<itype_id> gun_tools;

        std::set<std::string> repair_actions;
};

#endif // CATA_SRC_ITEM_FACTORY_H
