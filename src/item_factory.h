#ifndef ITEM_FACTORY_H
#define ITEM_FACTORY_H

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <bitset>
#include <memory>
#include <list>

#include "json.h"
#include "itype.h"

bool item_is_blacklisted( const std::string &id );

typedef std::string Item_tag;
typedef std::string Group_tag;
typedef std::vector<item> Item_list;

class Item_spawn_data;
class Item_group;
class item;
class item_category;

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
         * Registers a LUA based iuse function.
         * @param name The name that is used in the json data to refer to the LUA function.
         * It is stored in @ref iuse_function_list, the iuse function can be requested with
         * @ref get_iuse.
         * @param lua_function The LUA id of the LUA function.
         */
        void register_iuse_lua( const std::string &name, int lua_function );


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
         * The entries are loaded via @ref load_item_group_entries.
         * Assuming the input array looks like `[ x, y, z ]`, this function loads it like the
         * above would load this object:
         * \code
         * {
         *      "subtype": "depends on is_collection parameter",
         *      "id": "ident",
         *      "entries": [ x, y, z ]
         * }
         * \endcode
         * Note that each entrie in the array has to be a JSON object. The other function above
         * can also load data from arrays of strings, where the strings are item or group ids.
         */
        void load_item_group( JsonArray &entries, const Group_tag &ident, bool is_collection );
        /**
         * Get the item group object. Returns null if the item group does not exists.
         */
        Item_spawn_data *get_group( const Group_tag &id );
        /**
         * Returns the idents of all item groups that are known.
         * This is meant to be accessed at startup by lua to do mod-related modifications of groups.
         */
        std::vector<Group_tag> get_all_group_names();
        /**
         * Sets the chance of the specified item in the group.
         * This is meant to be accessed at startup by lua to do mod-related modifications of groups.
         * @param weight The relative weight of the item. A value of 0 removes the item from the
         * group.
         * @return false if the group doesn't exist.
         */
        bool add_item_to_group( const Group_tag group_id, const Item_tag item_id, int weight );
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
        void load_tool( JsonObject &jo, const std::string &src );
        void load_toolmod( JsonObject &jo, const std::string &src );
        void load_tool_armor( JsonObject &jo, const std::string &src );
        void load_book( JsonObject &jo, const std::string &src );
        void load_comestible( JsonObject &jo, const std::string &src );
        void load_container( JsonObject &jo, const std::string &src );
        void load_engine( JsonObject &jo, const std::string &src );
        void load_wheel( JsonObject &jo, const std::string &src );
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
         * @see Item_factory::migrate_id
         */
        void migrate_item( const itype_id &id, item &obj );

        /**
         * Check if an item type is known to the Item_factory.
         * @param id Item type id (@ref itype::id).
         */
        bool has_template( const itype_id &id ) const {
            return m_templates.count( id );
        }

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
         * @param new_type The new item type, must not be null.
         */
        void add_item_type( itype *new_type );

        void load_item_blacklist( JsonObject &jo );

        /**
         * A list of *all* known item type ids. Each is suitable as input to
         * @ref find_template or as parameter to @ref item::item.
         */
        std::vector<Item_tag> get_all_itype_ids() const;
        /**
         * The map of all known item type instances.
         * Key is the item type id (@ref itype::id, the parameter to
         * @ref find_template).
         * Value is the itype instance (result of @ref find_template).
         */
        const std::map<const itype_id, std::unique_ptr<itype>> &get_all_itypes() const {
            return m_templates;
        }
        /**
         * Create a new (and currently unused) item type id.
         */
        Item_tag create_artifact_id() const;

    private:
        std::map<std::string, std::unique_ptr<itype>> m_abstracts;

        mutable std::map<const itype_id, std::unique_ptr<itype>> m_templates;

        typedef std::map<Group_tag, Item_spawn_data *> GroupMap;
        GroupMap m_template_groups;

        /** Checks that ammo is listed in ammo_name().
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
        itype *load_definition( JsonObject &jo, const std::string &src );

        /**
         * Load the data of the slot struct. It creates the slot object (of type SlotType) and
         * and calls @ref load to do the actual (type specific) loading.
         */
        template<typename SlotType>
        void load_slot( std::unique_ptr<SlotType> &slotptr, JsonObject &jo, const std::string &src );
        /**
         * Load item the item slot if present in json.
         * Checks whether the json object has a member of the given name and if so, loads the item
         * slot from that object. If the member does not exists, nothing is done.
         */
        template<typename SlotType>
        void load_slot_optional( std::unique_ptr<SlotType> &slotptr, JsonObject &jo,
                                 const std::string &member, const std::string &src );

        void load( islot_tool &slot, JsonObject &jo, const std::string &src );
        void load( islot_container &slot, JsonObject &jo, const std::string &src );
        void load( islot_comestible &slot, JsonObject &jo, const std::string &src );
        void load( islot_brewable &slot, JsonObject &jo, const std::string &src );
        void load( islot_armor &slot, JsonObject &jo, const std::string &src );
        void load( islot_book &slot, JsonObject &jo, const std::string &src );
        void load( islot_mod &slot, JsonObject &jo, const std::string &src );
        void load( islot_engine &slot, JsonObject &jo, const std::string &src );
        void load( islot_wheel &slot, JsonObject &jo, const std::string &src );
        void load( islot_gun &slot, JsonObject &jo, const std::string &src );
        void load( islot_gunmod &slot, JsonObject &jo, const std::string &src );
        void load( islot_magazine &slot, JsonObject &jo, const std::string &src );
        void load( islot_bionic &slot, JsonObject &jo, const std::string &src );
        void load( islot_spawn &slot, JsonObject &jo, const std::string &src );
        void load( islot_ammo &slot, JsonObject &jo, const std::string &src );
        void load( islot_seed &slot, JsonObject &jo, const std::string &src );
        void load( islot_artifact &slot, JsonObject &jo, const std::string &src );

        //json data handlers
        void set_use_methods_from_json( JsonObject &jo, std::string member,
                                        std::map<std::string, use_function> &use_methods );

        use_function usage_from_string( const std::string &type ) const;

        std::pair<std::string, use_function> usage_from_object( JsonObject &obj ) const;

        void add_entry( Item_group *sg, JsonObject &obj );
        void load_item_group_entries( Item_group &ig, JsonArray &entries );

        void load_basic_info( JsonObject &jo, itype *new_item, const std::string &src );
        void tags_from_json( JsonObject &jo, std::string member, std::set<std::string> &tags );
        void set_qualities_from_json( JsonObject &jo, std::string member, itype *new_item );
        void set_properties_from_json( JsonObject &jo, std::string member, itype *new_item );

        void clear();
        void init();

        void finalize_item_blacklist();

        //iuse stuff
        std::map<Item_tag, use_function> iuse_function_list;

        void add_iuse( const std::string &type, const use_function_pointer f );
        void add_actor( iuse_actor *ptr );

        std::map<itype_id, migration> migrations;
};

extern std::unique_ptr<Item_factory> item_controller;

#endif
