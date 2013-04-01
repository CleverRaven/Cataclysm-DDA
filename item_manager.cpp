#include "item_manager.h"

Item_manager* item_controller = new Item_manager();

Item_manager::Item_manager(){
    m_missing_item = new itype();
    m_missing_item->name = "Error: Item Missing";
    m_missing_item->description = "Error: No item template of this type.";
    m_templates["MISSING_ITEM"] = m_missing_item;
}

void Item_manager::init(){
}

void Item_manager::init(game* main_game){
     m_templates.insert(main_game->itypes.begin(), main_game->itypes.end());
     init();
}

item_template_container* Item_manager::templates(){
    return &m_templates;
}

itype* Item_manager::find_template(std::string name){
    item_template_container::iterator found = m_templates.find(name);
    if(found != m_templates.end()){
        return found->second;
    }
    else{
        return m_missing_item; 
    }
}
