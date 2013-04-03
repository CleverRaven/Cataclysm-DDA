#ifndef _ITEM_MANAGER_H_
#define _ITEM_MANAGER_H_

#include <string>
#include <vector>
#include <map>
#include "game.h"
#include "itype.h"

typedef itype item_template;
typedef std::string item_tag;
typedef std::map<item_tag, itype*> item_template_container;
typedef std::set<item_tag> tag_list;


class Item_manager
{
public:
    Item_manager();
    void init();
    void init(game* main_game);

    const item_template_container* templates();
    item_template* find_template(item_tag id);
    item_template* random_template();
    item_template* random_template(item_tag group_tag);
    const item_tag random_id();
    const item_tag random_id(item_tag group_tag);
private:
    item_template_container  m_templates;
    itype*  m_missing_item;
    std::map<item_tag, tag_list> m_template_groups;
};

extern Item_manager* item_controller;

#endif
