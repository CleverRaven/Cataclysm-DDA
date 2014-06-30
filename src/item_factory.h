#ifndef _ITEM_FACTORY_H_
#define _ITEM_FACTORY_H_

#include "color.h"
#include "json.h"
#include "iuse.h"
#include "martialarts.h"
#include <string>
#include <vector>
#include <map>

typedef std::string Item_tag;
typedef std::string Group_tag;
typedef std::vector<item> Item_list;

//For the iuse arguments
class game;
class player;
class Item_spawn_data;
class Item_group;
class item;
struct itype;

class item_category {
public:
    // id (like itype::id) - used when loading from json
    std::string id;
    // display name (localized)
    std::string name;
    // categories are sorted by this value,
    // lower values means the category is shown first
    int sort_rank;

    item_category() : id(), name(), sort_rank(0) { }
    item_category(const std::string &id_, const std::string &name_, int sort_rank_) : id(id_), name(name_), sort_rank(sort_rank_) { }

    // Comparators operato on the sort_rank, name, id
    // (in that order).
    bool operator<(const item_category &rhs) const;
    bool operator==(const item_category &rhs) const;
    bool operator!=(const item_category &rhs) const;
};

class Item_factory
{
public:
    //Setup
    Item_factory();
    ~Item_factory();
    void init();
    void clear_items_and_groups();
    void init_old();
    void register_iuse_lua(const char* name, int lua_function);

    void load_item_group(JsonObject &jsobj);
    // Same as other load_item_group, but takes the ident, subtype
    // from parameters instead of looking into the json object.
    void load_item_group(JsonObject &jsobj, const std::string &ident, const std::string &subtype);
    /**
     * Check if an item type is known to the Item_factory,
     * (or if it can create it).
     * If this function returns true, @ref find_template
     * will never return the MISSING_ITEM or null-item type.
     */
    bool has_template(const Item_tag& id) const;

    bool has_group(const Group_tag& id) const;
    Item_spawn_data *get_group(const Group_tag& id);

    //Intermediary Methods - Will probably be removed at final stage
    /**
     * Returns the itype with the given id.
     * Never return NULL.
     */
    itype* find_template( Item_tag id );

    /**
     * Add a passed in itype to the collection of item types.
     */
    void add_item_type( itype *new_type );

    /**
     * Return a random item type from the given item group.
     */
    const Item_tag id_from(Item_tag group_tag);
    bool group_contains_item(Item_tag group_tag, Item_tag item);

    /**
     * Create items from the given group. It creates as many items as the
     * group definition requests.
     * For example if the group is a distribution that only contains
     * item ids it will create single item.
     * If the group is a collection with several entries it can contain
     * more than one item (or none at all!).
     * This function also creates ammo for guns, if this is requested
     * in the item group.
     */
    Item_list create_from_group(Group_tag group, int created_at);

    void debug_spawn();

    void load_ammo      (JsonObject &jo);
    void load_gun       (JsonObject &jo);
    void load_armor     (JsonObject &jo);
    void load_tool      (JsonObject &jo);
    void load_tool_armor(JsonObject &jo);
    void load_book      (JsonObject &jo);
    void load_comestible(JsonObject &jo);
    void load_container (JsonObject &jo);
    void load_gunmod    (JsonObject &jo);
    void load_generic   (JsonObject &jo);
    void load_bionic    (JsonObject &jo);
    void load_veh_part  (JsonObject &jo);
    void load_stationary(JsonObject &jo);

    void load_item_blacklist(JsonObject &jo);
    void load_item_whitelist(JsonObject &jo);
    void finialize_item_blacklist();

    // Check that all items referenced in the groups
    // do actually exist (are defined)
    void check_items_of_groups_exist() const;
    // Check consistency in itype definitions
    // like: valid material, valid tool
    void check_itype_definitions() const;

    void load_item_category(JsonObject &jo);

    // Determine and return the category id of the given type
    const std::string &calc_category(itype *ity);
    // Get the category from the category id.
    // This will never return 0.
    // The returned value stays valid as long as this Item_factory
    // stays valid.
    const item_category *get_category(const std::string &id);

    const use_function *get_iuse( const std::string &id );

    // The below functions are meant to be accessed at startup by lua to
    // do mod-related modifications of groups.
    std::vector<std::string> get_all_group_names();

    // Sets the chance of the specified item in the group. weight 0 will allow you to remove
    // the item from the group. Returns false if the group doesn't exist.
    bool add_item_to_group(const std::string group_id, const std::string item_id, int weight);

private:
    std::map<Item_tag, itype*> m_templates;
    itype*  m_missing_item;
    typedef std::map<Group_tag, Item_spawn_data*> GroupMap;
    GroupMap m_template_groups;

    // Checks that ammo is listed in ammo_name(),
    // That there is at least on instance (it_ammo) of
    // this ammo type defined.
    // If any of this fails, prints a message to the msg
    // stream.
    void check_ammo_type(std::ostream& msg, const std::string &ammo) const;

    typedef std::map<std::string, item_category> CategoryMap;
    // Map with all the defined item categories,
    // get_category returns a value from this map. This map
    // should only grow, categories should never be removed from
    // it as itype::category contains a pointer to the values
    // of this map (which has been returned by get_category).
    // The key is the id of the item_category.
    CategoryMap m_categories;

    void create_inital_categories();

    // used to add the default categories
    void add_category(const std::string &id, int sort_rank, const std::string &name);

    //json data handlers
    void set_use_methods_from_json( JsonObject& jo, std::string member, itype *new_item_template );
    use_function use_from_string(std::string name);
    use_function use_from_object(JsonObject obj);
    phase_id phase_from_tag(Item_tag name);

    void add_entry(Item_group* sg, JsonObject &obj);

    void load_basic_info(JsonObject &jo, itype *new_item);
    void tags_from_json(JsonObject &jo, std::string member, std::set<std::string> &tags);
    void set_qualities_from_json(JsonObject &jo, std::string member, itype *new_item);
    unsigned flags_from_json(JsonObject &jo, const std::string & member, std::string flag_type="");
    void set_material_from_json(JsonObject &jo, std::string member, itype *new_item);
    bool is_mod_target(JsonObject &jo, std::string member, std::string weapon);

    void set_intvar(std::string tag, unsigned int & var, int min, int max);

    //two convenience functions that just call into set_bitmask_by_string
    void set_flag_by_string(unsigned& cur_flags, const std::string & new_flag,
                            const std::string & flag_type);

    //iuse stuff
    std::map<Item_tag, use_function> iuse_function_list;
};

extern Item_factory* item_controller;

#endif
