#include "item_manager.h"
#include "rng.h"
#include <algorithm>

Item_manager* item_controller = new Item_manager();

//Every item manager comes with a missing item
Item_manager::Item_manager(){
    m_missing_item = new item_template();
    m_missing_item->name = "Error: Item Missing";
    m_missing_item->description = "Error: No item template of this type.";
    m_templates["MISSING_ITEM"]=m_missing_item;
}

void Item_manager::init(){
}

//Will eventually be deprecated - Loads existing item format into the item manager
void Item_manager::init(game* main_game){
    //Copy over the template pointers
    m_templates.insert(main_game->itypes.begin(), main_game->itypes.end());
    //Load up the group lists with the default 'all' label
    for(item_template_container::iterator iter = m_templates.begin(); iter != m_templates.end(); ++iter){
      item_tag id = iter->first;
      bool standard=true;
      m_template_groups["ALL"].insert(id);
      if(std::find(unreal_itype_ids.begin(),unreal_itype_ids.end(), id) != unreal_itype_ids.end()){
          m_template_groups["UNREAL"].insert(id);
          standard=false;
      }
      if(std::find(martial_arts_itype_ids.begin(),martial_arts_itype_ids.end(),id) != martial_arts_itype_ids.end()){
          m_template_groups["STYLE"].insert(id);
          standard=false;
      }
      if(std::find(artifact_itype_ids.begin(),artifact_itype_ids.end(),id) != artifact_itype_ids.end()){
          m_template_groups["ARTIFACT"].insert(id);
          standard=false;
      }
      if(std::find(unreal_itype_ids.begin(),unreal_itype_ids.end(),id) != unreal_itype_ids.end()){
          m_template_groups["PSEUDO"].insert(id);
          standard=false;
      }
      if(standard){
          m_template_groups["STANDARD"].insert(id);
      }
      // This is temporary, and only for testing the tag access algorithms work.
      // It replaces a similar list located in the mapgen file
      // Once reading from files are implemented, this will just check for a "CAN" tag, obviously.
      tag_list can_list;
      can_list.insert("can_beans"); can_list.insert("can_corn"); can_list.insert("can_spam"); 
      can_list.insert("can_pineapple"); can_list.insert("can_coconut"); can_list.insert("can_sardine");
      can_list.insert("can_tuna");
      if(can_list.find(id)!=can_list.end()){
        m_template_groups["CAN"].insert(id);
      }
    }
    init();
}

//Returns the full template container
const item_template_container* Item_manager::templates(){
    return &m_templates;
}

//Returns the template with the given identification tag
item_template* Item_manager::find_template(const item_tag id){
    item_template_container::iterator found = m_templates.find(id);
    if(found != m_templates.end()){
        return found->second;
    }
    else{
        return m_missing_item; 
    }
}

//Returns a random template from the list of all templates.
item_template* Item_manager::random_template(){
    return random_template("ALL");
}

//Returns a random template from those with the given group tag
item_template* Item_manager::random_template(const item_tag group_tag){ 
    return find_template( random_id(group_tag) );
}

//Returns a random template name from the list of all templates.
const item_tag Item_manager::random_id(){
    return random_id("ALL");
}

//Returns a random template name from the list of all templates.
const item_tag Item_manager::random_id(const item_tag group_tag){
    std::map<item_tag, tag_list>::iterator group_iter = m_template_groups.find(group_tag);
    if(group_iter != m_template_groups.end() && group_iter->second.begin() != group_iter->second.end()){
        tag_list group = group_iter->second;
        tag_list::iterator random_element = group.begin();
        std::advance(random_element, rng(0,group.size()-1));
        item_tag item_id = *random_element;
        return item_id;
    } else {
        return "MISSING_ITEM";
    }
}
