#ifndef _ITEM_FACTORY_H_
#define _ITEM_FACTORY_H_

#include "game.h"
#include "itype.h"
#include "item.h"
#include "color.h"
#include "json.h"
#include "item_group.h"
#include "iuse.h"
#include "martialarts.h"
#include <string>
#include <vector>
#include <map>

typedef std::string Item_tag;
typedef std::vector<item> Item_list;

//For the iuse arguments
class game;
class player;

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
    void init();
    void clear_items_and_groups();
    void init_old();
    void register_iuse_lua(const char* name, int lua_function);

    void load_item_group(JsonObject &jsobj);

    bool has_template(Item_tag id) const;

    //Intermediary Methods - Will probably be removed at final stage
    itype* find_template(Item_tag id);
    itype* random_template();
    itype* template_from(Item_tag group_tag);
    const Item_tag random_id();
    const Item_tag id_from(Item_tag group_tag);
    const Item_tag id_from(Item_tag group_tag, bool & with_ammo);
    bool group_contains_item(Item_tag group_tag, Item_tag item);

    //Production methods
    item create(Item_tag id, int created_at);
    Item_list create(Item_tag id, int created_at, int quantity);
    item create_from(Item_tag group, int created_at);
    Item_list create_from(Item_tag group, int created_at, int quantity);
    item create_random(int created_at);
    Item_list create_random(int created_at, int quantity);

    void load_ammo      (JsonObject &jo);
    void load_gun       (JsonObject &jo);
    void load_armor     (JsonObject &jo);
    void load_tool      (JsonObject &jo);
    void load_book      (JsonObject &jo);
    void load_comestible(JsonObject &jo);
    void load_container (JsonObject &jo);
    void load_gunmod    (JsonObject &jo);
    void load_generic   (JsonObject &jo);
    void load_bionic    (JsonObject &jo);
    void load_veh_part  (JsonObject &jo);

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

private:
    std::map<Item_tag, itype*> m_templates;
    itype*  m_missing_item;
    std::map<Item_tag, Item_group*> m_template_groups;

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
    use_function use_from_string(std::string name);
    phase_id phase_from_tag(Item_tag name);

    void load_basic_info(JsonObject &jo, itype *new_item);
    void tags_from_json(JsonObject &jo, std::string member, std::set<std::string> &tags);
    void set_qualities_from_json(JsonObject &jo, std::string member, itype *new_item);
    unsigned flags_from_json(JsonObject &jo, const std::string & member, std::string flag_type="");
    void set_material_from_json(JsonObject &jo, std::string member, itype *new_item);
    bool is_mod_target(JsonObject &jo, std::string member, std::string weapon);

    void set_intvar(std::string tag, unsigned int & var, int min, int max);

    //two convenience functions that just call into set_bitmask_by_string
    void set_flag_by_string(unsigned& cur_flags, const std::string & new_flag, const std::string & flag_type);

    //iuse stuff
    std::map<Item_tag, use_function> iuse_function_list;
    //techniques stuff
    std::map<Item_tag, matec_id> techniques_list;
};

extern Item_factory* item_controller;

#endif
