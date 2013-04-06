#include "item_manager.h"
#include "rng.h"
#include "enums.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <stdio.h>

Item_manager* item_controller = new Item_manager();

//Every item manager comes with a missing item
Item_manager::Item_manager(){
    m_missing_item = new item_template();
    m_missing_item->name = "Error: Item Missing";
    m_missing_item->description = "Error: No item template of this type.";
    m_templates["MISSING_ITEM"]=m_missing_item;
    load_item_templates();
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

///////////////////////
// DATA FILE READING //
///////////////////////

void Item_manager::load_item_templates(){
    load_item_templates_from("data/raw/items/instruments.json");
    load_item_templates_from("data/raw/items/melee.json");
}

// Load values from this data file into m_templates
// TODO: Consider appropriate location for this code. Is this really where it belongs?
//       At the very least, it seems the json blah_from methods could be used elsewhere
void Item_manager::load_item_templates_from(const std::string file_name){
    std::ifstream data_file;
    picojson::value input_value;
    
    data_file.open(file_name.c_str());
    data_file >> input_value;
    data_file.close();

    //Handle any obvious errors on file load
    std::string err = picojson::get_last_error();
    if (! err.empty()) {
        std::cerr << "In JSON file \"" << file_name << "\"" << data_file << ":" << err << std::endl;
        exit(1);
    }
    if (! input_value.is<picojson::array>()) {
        std::cerr << file_name << " is not an array of item_templates" << std::endl;
        exit(2);
    }
    
    //Crawl through and extract the items
    const picojson::array& all_items = input_value.get<picojson::array>();
    for (picojson::array::const_iterator entry = all_items.begin(); entry != all_items.end(); ++entry) {
        bool load_success=false;
        if( !(entry->is<picojson::object>()) ){
            std::cerr << "Invalid item definition, entry not a JSON object" << std::endl;
        }
        else{
            const picojson::value::object& entry_body = entry->get<picojson::object>();

            // The one element we absolutely require for an item definition is an id
            picojson::value::object::const_iterator key_pair = entry_body.find("id");
            if( key_pair == entry_body.end() || !(key_pair->second.is<std::string>()) ){
                std::cerr << "Item definition skipped, no id found or id was malformed." << std::endl;
            } else {
                item_tag new_id = key_pair->second.get<std::string>();

                // If everything works out, add the item to the template list... 
                // unless a similar item is already there
                if(m_templates.find(new_id) != m_templates.end()){
                    std::cerr << "Item definition skipped, id " << new_id << " already exists." << std::endl;
                } else {
                    item_template* new_item_template =  new itype();
                    new_item_template->id = new_id;
                    m_templates[new_id] = new_item_template;

                    // And then proceed to assign the correct field
                    new_item_template->rarity = int_from_json(new_id, "rarity", entry_body); 
                    new_item_template->name = string_from_json(new_id, "name", entry_body); 
                    new_item_template->sym = char_from_json(new_id, "symbol", entry_body); 
                    new_item_template->color = color_from_json(new_id, "color", entry_body); 
                    new_item_template->description = string_from_json(new_id, "description", entry_body); 
                    new_item_template->m1 = materials_from_json(new_id, "material", entry_body)[0];
                    new_item_template->m2 = materials_from_json(new_id, "material", entry_body)[1];
                    new_item_template->volume = int_from_json(new_id, "volume", entry_body); 
                    new_item_template->weight = int_from_json(new_id, "weight", entry_body); 
                    new_item_template->melee_dam = int_from_json(new_id, "damage", entry_body); 
                    new_item_template->melee_cut = int_from_json(new_id, "cutting", entry_body); 
                    new_item_template->m_to_hit = int_from_json(new_id, "to_hit", entry_body); 
                }
            }
        }
    }
}

//Grab string, with appropriate error handling
item_tag Item_manager::string_from_json(item_tag new_id, item_tag index, picojson::value::object value_map){
    picojson::value::object::const_iterator value_pair = value_map.find(index);
    if(value_pair != value_map.end()){
        if(value_pair->second.is<std::string>()){
            return value_pair->second.get<std::string>();
        } else {
            std::cerr << "Item "<< new_id << " attribute " << index << "was skipped, not a string." << std::endl;
            return "Error: Unknown Value";
        }
    } else {
        //If string is not found, just return an empty string
        return "";
    }
}

//Grab character, with appropriate error handling
char Item_manager::char_from_json(item_tag new_id, item_tag index, picojson::value::object value_map){
    std::string symbol = string_from_json(new_id, index, value_map);
    if(symbol == ""){
        std::cerr << "Item "<< new_id << " attribute  " << "was skipped, empty string not allowed." << std::endl;
        return 'X';
    }
    return symbol[0];
}

//Grab int, with appropriate error handling
int Item_manager::int_from_json(item_tag new_id, item_tag index, picojson::value::object value_map){
    picojson::value::object::const_iterator value_pair = value_map.find(index);
    if(value_pair != value_map.end()){
        if(value_pair->second.is<double>()){
            return int(value_pair->second.get<double>());
        } else {
            std::cerr << "Item "<< new_id << " attribute name was skipped, not a number." << std::endl;
            return 0;
        }
    } else {
        //If the value isn't found, just return a 0
        return 0;
    }
}

//Grab color, with appropriate error handling
nc_color Item_manager::color_from_json(item_tag new_id, item_tag index, picojson::value::object value_map){
    std::string new_color = string_from_json(new_id, index, value_map);
    if("red"==new_color){
        return c_red;
    } else if("blue"==new_color){
        return c_blue;
    } else if("green"==new_color){
        return c_green;
    } else {
        std::cerr << "Item "<< new_id << " attribute name was skipped, not a color. Color is required." << std::endl;
        return c_white;
    }
}

material* Item_manager::materials_from_json(item_tag new_id, item_tag index, picojson::value::object value_map){
    //If the value isn't found, just return a group of null materials
    material material_list[2] = {MNULL, MNULL};

    picojson::value::object::const_iterator value_pair = value_map.find(index);
    if(value_pair != value_map.end()){
        bool invalid_materials = true;
        if(value_pair->second.is<std::string>()){
            material_list[0] = material_from_tag(new_id, value_pair->second.get<std::string>());
            invalid_materials = false;
        } else if (value_pair->second.is<picojson::array>()) {
            int material_count = 0;
            const picojson::array& materials_json = value_pair->second.get<picojson::array>();
            for (picojson::array::const_iterator iter = materials_json.begin(); iter != materials_json.end(); ++iter) {
                if((*iter).is<std::string>()){
                    material_list[material_count] = material_from_tag(new_id, (*iter).get<std::string>());
                } else {
                    std::cerr << "Item "<< new_id << " has a non-string material listed." << std::endl;
                }

                ++material_count;
                if(material_count > 2){
                    std::cerr << "Item "<< new_id << " has too many materials listed." << std::endl;
                }
            }
        } else {
            std::cerr << "Item "<< new_id << " material was skipped, not a string or array of strings." << std::endl;
        }
    } 
    return material_list;
}

material Item_manager::material_from_tag(item_tag new_id, item_tag name){
    // Map the valid input tags to a valid material

    // This should clearly be some sort of map, stored somewhere
    // ...unless it can get replaced entirely, which would be nice.
    // For now, though, that isn't the problem I'm solving.
    if(name == "LIQUID"){
        return LIQUID; 
    } else if(name == "VEGGY"){
        return VEGGY;
    } else if(name == "FLESH"){
        return FLESH;
    } else if(name == "POWDER"){
        return POWDER;
    } else if(name == "HFLESH"){
        return HFLESH;
    } else if(name == "COTTON"){
        return COTTON;
    } else if(name == "WOOL"){
        return WOOL;
    } else if(name == "LEATHER"){
        return LEATHER;
    } else if(name == "KEVLAR"){
        return KEVLAR;
    } else if(name == "FUR"){
        return FUR;
    } else if(name == "STONE"){
        return STONE;
    } else if(name == "PAPER"){
        return PAPER;
    } else if(name == "WOOD"){
        return WOOD;
    } else if(name == "PLASTIC"){
        return PLASTIC;
    } else if(name == "GLASS"){
        return GLASS;
    } else if(name == "IRON"){
        return IRON;
    } else if(name == "STEEL"){
        return STEEL;
    } else if(name == "SILVER"){
        return SILVER;
    } else {
        std::cerr << "Item "<< new_id << " material was skipped, not a string or array of strings." << std::endl;
        return MNULL;
    }
}
