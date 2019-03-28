#pragma once
#ifndef ITEM_FACTORY_H
#define ITEM_FACTORY_H

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "itype.h"

bool item_is_blacklisted( const std::string &id );

typedef std::string Item_tag;
typedef std::string Group_tag;
typedef std::vector<item> Item_list;

class Item_spawn_data;
class Item_group;
class item;
class item_category;
class Item_factory;
class JsonObject;
class JsonArray;

extern std::unique_ptr<Item_factory> item_controller;

class migration
{
    public:
        std::string id;
        std::string replace;
        std::set<std::string> flags;
        int charges = 0;
        std::set<std::string> contents;
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
         * @throw std::string if the json object contains invalid data.
         */
        void load_item_group( JsonObject &jsobj );
        /**
         * Load a item group from json. It differs from the other load_item_group function as it
         * uses the ident and subtype given as parameter and does not look up those things in
         * the json data (the json object here does not need to have them).
         * This is intended for inline definitions of item groups, e.g. in monster death drops:
         * the item group there is embedded into the monster type definition.
         * @param jsobj The json object to load from.
         * @param ident The ident of the item that is to be loaded.
         * @param subtype The type of the item group, either "collection", "distribution" or "old"
         * ("old" is a distribution, too).
         * @throw std::string if the json object contains invalid data.
         */
        void load_item_group( JsonObject &jsobj, const Group_tag &ident, const std::string &subtype );
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
        void load_item_group( JsonArray &entries, const Group_tag &ident, bool is_collection,
                              int ammo_chance, int magazine_chance );
        /**
         * Get the item group object. Returns null if the item group does not exists.
         */
        Item_spawn_data *get_group( const Group_tag &id );
        /**
         * Returns the idents of all item groups that are known.
         */
        std::vector<Group_tag> get_all_group_names();
        /**
         * Sets the chance of the specified item in the group.
         * @param group_id Group to add item to
         * @param item_id Id of item to add to group
         * @param weight The relative weight of the item. A value of 0 removes the item from the
         * group.
         * @return false if the group doesn't exist.
         */
        bool add_item_to_group( const Group_tag &group_id, const Item_tag &item_id, int weight );
        /*@}*/

        /**
         * @name Item type loading
         *
         * These function load different instances of itype objects from json.
         * The loaded item types are stored and can be accessed through @ref find_template.
         * @param jo The json object to load data from.
         * @throw std::string if the json object contains invalid data.
         */
        /*@{*/
        void load_ammo( JsonObject &jo, const std::string &src );
        void load_gun( JsonObject &jo, const std::string &src );
        void load_armor( JsonObject &jo, const std::string &src );
        void load_pet_armor( JsonObject &jo, const std::string &src );
        void load_tool( JsonObject &jo, const std::string &src );
        void load_toolmod( JsonObject &jo, const std::string &src );
        void load_tool_armor( JsonObject &jo, const std::string &src );
        void load_book( JsonObject &jo, const std::string &src );
        void load_comestible( JsonObject &jo, const std::string &src );
        void load_container( JsonObject &jo, const std::string &src );
        void load_engine( JsonObject &jo, const std::string &src );
        void load_wheel( JsonObject &jo, const std::string &src );
        void load_fuel( JsonObject &jo, const std::string &src );
        void load_gunmod( JsonObject &jo, const std::string &src );
        void load_magazine( JsonObject &jo, const std::string &src );
        void load_generic( JsonObject &jo, const std::string &src );
        void load_bionic( JsonObject &jo, const std::string &src );
        /*@}*/

        /** called after all JSON has been read and performs any necessary cleanup tasks */
        void finalize();

        /**
         * Load item category definition from json
         * @param jo The json object to load data from.
         * @throw std::string if the json object contains invalid data.
         */
        void load_item_category( JsonObject &jo );

        /** Migrations transform items loaded from legacy saves */
        void load_migration( JsonObject &jo );

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
        bool has_iuse( const std::string &type ) const {
            return iuse_function_list.find( type ) != iuse_function_list.end();
        }

        void load_item_blacklist( JsonObject &jo );

        /** Get all item templates (both static and runtime) */
        std::vector<const itype *> all() const;

        /** Get item types created at runtime. */
        std::vector<const itype *> get_runtime_types() const;

        /** Find all item templates (both static and runtime) matching UnaryPredicate function */
        static std::vector<const itype *> find( const std::function<bool( const itype & )> &func );

        /**
         * Create a new (and currently unused) item type id.
         */
        Item_tag create_artifact_id() const;

        std::list<itype_id> subtype_replacement( const itype_id & ) const;

    private:
        /** Set at finalization and prevents alterations to the static item templates */
        bool frozen = false;

        std::map<const std::string, itype> m_abstracts;

        std::unordered_map<itype_id, itype> m_templates;

        mutable std::map<itype_id, std::unique_ptr<itype>> m_runtimes;

        typedef std::map<Group_tag, std::unique_ptr<Item_spawn_data>> GroupMap;
        GroupMap m_template_groups;

        /** Checks that ammo is listed in ammunition_type::name().
         * At least one instance of this ammo type should be defined.
         * If any of checks fails, prints a message to the msg stream.
         * @param msg Stream in which all error messages are printed.
         * @param ammo Ammo type to check.
         */
        bool check_ammo_type( std::ostream &msg, const ammotype &ammo ) const;

        // Map with all the defined item categories,
        // This map should only grow, categories should never be removed from
        // it as itype::category contains a pointer to the values of this map
        // The key is the id of the item_category.
        std::map<std::string, item_category> categories;

        /**
         * Called before creating a new template and handles inheritance via copy-from
         * May defer instantiation of the template if depends on other objects not as-yet loaded
         */
        bool load_definition( JsonObject &jo, const std::string &src, itype &def );

        /**
         * Load the data of the slot struct. It creates the slot object (of type SlotType) and
         * and calls @ref load to do the actual (type specific) loading.
         */
        template<typename SlotType>
        void load_slot( cata::optional<SlotType> &slotptr, JsonObject &jo, const std::string &src );

        /**
         * Load item the item slot if present in json.
         * Checks whether the json object has a member of the given name and if so, loads the item
         * slot from that object. If the member does not exists, nothing is done.
         */
        template<typename SlotType>
        void load_slot_optional( cata::optional<SlotType> &slotptr, JsonObject &jo,
                                 const std::string &member, const std::string &src );

        void load( islot_tool &slot, JsonObject &jo, const std::string &src );
        void load( islot_container &slot, JsonObject &jo, const std::string &src );
        void load( islot_comestible &slot, JsonObject &jo, const std::string &src );
        void load( islot_brewable &slot, JsonObject &jo, const std::string &src );
        void load( islot_armor &slot, JsonObject &jo, const std::string &src );
        void load( islot_pet_armor &slot, JsonObject &jo, const std::string &src );
        void load( islot_book &slot, JsonObject &jo, const std::string &src );
        void load( islot_mod &slot, JsonObject &jo, const std::string &src );
        void load( islot_engine &slot, JsonObject &jo, const std::string &src );
        void load( islot_wheel &slot, JsonObject &jo, const std::string &src );
        void load( islot_fuel &slot, JsonObject &jo, const std::string &src );
        void load( islot_gun &slot, JsonObject &jo, const std::string &src );
        void load( islot_gunmod &slot, JsonObject &jo, const std::string &src );
        void load( islot_magazine &slot, JsonObject &jo, const std::string &src );
        void load( islot_bionic &slot, JsonObject &jo, const std::string &src );
        void load( islot_ammo &slot, JsonObject &jo, const std::string &src );
        void load( islot_seed &slot, JsonObject &jo, const std::string &src );
        void load( islot_artifact &slot, JsonObject &jo, const std::string &src );

        //json data handlers
        void set_use_methods_from_json( JsonObject &jo, const std::string &member,
                                        std::map<std::string, use_function> &use_methods );

        use_function usage_from_string( const std::string &type ) const;

        std::pair<std::string, use_function> usage_from_object( JsonObject &obj );

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
        bool load_sub_ref( std::unique_ptr<Item_spawn_data> &ptr, JsonObject &obj,
                           const std::string &name, const Item_group &parent );
        bool load_string( std::vector<std::string> &vec, JsonObject &obj, const std::string &name );
        void add_entry( Item_group &sg, JsonObject &obj );

        void load_basic_info( JsonObject &jo, itype &def, const std::string &src );
        void set_qualities_from_json( JsonObject &jo, const std::string &member, itype &def );
        void set_properties_from_json( JsonObject &jo, const std::string &member, itype &def );

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
        std::map<Item_tag, use_function> iuse_function_list;

        void add_iuse( const std::string &type, const use_function_pointer f );
        void add_iuse( const std::string &type, const use_function_pointer f,
                       const std::string &info );
        void add_actor( iuse_actor *ptr );

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

        // tools that can be used to repair wood/paper/bone/chitin items
        std::set<itype_id> misc_tools;
};

#endif
