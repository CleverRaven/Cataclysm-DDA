#ifndef _ITEM_MANAGER_H_
#define _ITEM_MANAGER_H_

#include <string>
#include <vector>
#include <map>
#include "game.h"
#include "itype.h"
#include "item.h"
#include "color.h"
#include "picojson.h"
#include "item_group.h"

typedef std::string Item_tag;
typedef std::vector<item*> Item_list;

class Item_factory
{
public:
    //Setup
    Item_factory();
    void init();
    void init(game* main_game);

    //Intermediary Methods - Will probably be removed at final stage
    itype* find_template(Item_tag id);
    itype* random_template();
    itype* template_from(Item_tag group_tag);
    const Item_tag random_id();
    const Item_tag id_from(Item_tag group_tag);

    //Production methods
    item* create(Item_tag id, int created_at);
    Item_list create(Item_tag id, int created_at, int quantity);
    item* create_from(Item_tag group, int created_at);
    Item_list create_from(Item_tag group, int created_at, int quantity);
    item* create_random(int created_at);
    Item_list create_random(int created_at, int quantity);

private:
    std::map<Item_tag, itype*> m_templates;
    itype*  m_missing_item;
    std::map<Item_tag, Item_group*> m_template_groups;

    //json data handlers
    void load_item_templates();
    void load_item_templates_from(const std::string file_name);
    void load_item_groups_from(const std::string file_name);

    Item_tag string_from_json(Item_tag new_id, Item_tag index, picojson::value::object value_map);
    char char_from_json(Item_tag new_id, Item_tag index, picojson::value::object value_map);
    int int_from_json(Item_tag new_id, Item_tag index, picojson::value::object value_map);
    nc_color color_from_json(Item_tag new_id, Item_tag index, picojson::value::object value_map);
    material material_from_json(Item_tag new_id, Item_tag index, picojson::value::object value_map, int to_return);
    material material_from_tag(Item_tag new_id, Item_tag index);

};

extern Item_factory* item_controller;

#endif
