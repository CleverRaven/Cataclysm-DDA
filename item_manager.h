#ifndef _ITEM_MANAGER_H_
#define _ITEM_MANAGER_H_

#include <string>
#include <vector>
#include <map>
#include "game.h"
#include "itype.h"

typedef std::map<std::string, itype*> item_template_container;
typedef std::vector<std::string> access_key_list;

class Item_manager
{
public:
    Item_manager();
    void init();
    void init(game* main_game);

    item_template_container* templates();
    access_key_list* template_keys();
    itype* find_template(std::string name);
private:
    item_template_container  m_templates;
    itype*  m_missing_item;
};

extern Item_manager* item_controller;

#endif
