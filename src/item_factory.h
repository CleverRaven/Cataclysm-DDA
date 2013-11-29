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

class Item_factory
{
public:
    //Setup
    Item_factory();
    void init();
    void init_old();
    void register_iuse_lua(const char* name, int lua_function);

    void load_item_group(JsonObject &jsobj);

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

private:
    std::map<Item_tag, itype*> m_templates;
    itype*  m_missing_item;
    std::map<Item_tag, Item_group*> m_template_groups;

    //json data handlers
    use_function use_from_string(std::string name);
    phase_id phase_from_tag(Item_tag name);

    void load_basic_info(JsonObject &jo, itype *new_item);
    void tags_from_json(JsonObject &jo, std::string member, std::set<std::string> &tags);
    void set_qualities_from_json(JsonObject &jo, std::string member, itype *new_item);
    unsigned flags_from_json(JsonObject &jo, std::string member, std::string flag_type="");
    void set_material_from_json(JsonObject &jo, std::string member, itype *new_item);
    bool is_mod_target(JsonObject &jo, std::string member, std::string weapon);

    void set_intvar(std::string tag, unsigned int & var, int min, int max);

    //two convenience functions that just call into set_bitmask_by_string
    void set_flag_by_string(unsigned& cur_flags, std::string new_flag, std::string flag_type);
    //sets a bitmask (cur_bitmask) based on the values of flag_map and new_flag
    void set_bitmask_by_string(std::map<Item_tag, unsigned> flag_map,
                               unsigned& cur_bitmask, std::string new_flag);

    //iuse stuff
    std::map<Item_tag, use_function> iuse_function_list;
    //techniques stuff
    std::map<Item_tag, matec_id> techniques_list;
    //bodyparts
    std::map<Item_tag, unsigned> bodyparts_list;
};

extern Item_factory* item_controller;

#endif
