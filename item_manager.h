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

typedef itype item_template;
typedef std::string item_tag;
typedef std::map<item_tag, itype*> item_template_container;
typedef std::set<item_tag> tag_list;
typedef std::vector<item*> item_list;


class Item_manager
{
public:
    //Setup
    Item_manager();
    void init();
    void init(game* main_game);

    //Intermediary Methods - Will be removed at final stage
    item_template* find_template(item_tag id);
    item_template* random_template();
    item_template* template_from(item_tag group_tag);
    const item_tag random_id();
    const item_tag id_from(item_tag group_tag);

    //Production methods
    item* create(item_tag id);
    item_list create(item_tag id, int quantity);
    item* create_from(item_tag group);
    item_list create_from(item_tag group, int quantity);
    item* create_random();
    item_list create_random(int quantity);

private:
    item_template_container m_templates;
    itype*  m_missing_item;
    std::map<item_tag, Item_group*> m_template_groups;

    //json data handlers
    void load_item_templates();
    void load_item_templates_from(const std::string file_name);
    void load_item_groups_from(const std::string file_name);

    item_tag string_from_json(item_tag new_id, item_tag index, picojson::value::object value_map);
    char char_from_json(item_tag new_id, item_tag index, picojson::value::object value_map);
    int int_from_json(item_tag new_id, item_tag index, picojson::value::object value_map);
    nc_color color_from_json(item_tag new_id, item_tag index, picojson::value::object value_map);
    material material_from_json(item_tag new_id, item_tag index, picojson::value::object value_map, int to_return);
    material material_from_tag(item_tag new_id, item_tag index);

};

extern Item_manager* item_controller;

#endif
