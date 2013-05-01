#include "item_factory.h"
#include "rng.h"
#include "enums.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <stdio.h>

Item_factory* item_controller = new Item_factory();

//Every item factory comes with a missing item
Item_factory::Item_factory(){
    init();
    m_missing_item = new itype();
    m_missing_item->name = "Error: Item Missing";
    m_missing_item->description = "Error: No item template of this type.";
    m_templates["MISSING_ITEM"]=m_missing_item;
    load_item_templates();
}

void Item_factory::init(){
  //Populate the iuse functions
  iuse_function_list["NONE"] = &iuse::none;
  iuse_function_list["SEWAGE"] = &iuse::sewage;

  iuse_function_list["HONEYCOMB"] = &iuse::honeycomb;
  iuse_function_list["ROYAL_JELLY"] = &iuse::royal_jelly;
  iuse_function_list["BANDAGE"] = &iuse::bandage;
  iuse_function_list["FIRSTAID"] = &iuse::firstaid;
  iuse_function_list["CAFF"] = &iuse::caff;
  iuse_function_list["ALCOHOL"] = &iuse::alcohol;
  iuse_function_list["ALCOHOL_WEAK"] = &iuse::alcohol_weak;
  iuse_function_list["PKILL_1"] = &iuse::pkill_1;
  iuse_function_list["PKILL_2"] = &iuse::pkill_2;
  iuse_function_list["PKILL_3"] = &iuse::pkill_3;
  iuse_function_list["PKILL_4"] = &iuse::pkill_4;
  iuse_function_list["PKILL_L"] = &iuse::pkill_l;
  iuse_function_list["XANAX"] = &iuse::xanax;
  iuse_function_list["CIG"] = &iuse::cig;
  iuse_function_list["ANTIBIOTIC"] = &iuse::antibiotic;
  iuse_function_list["WEED"] = &iuse::weed;
  iuse_function_list["COKE"] = &iuse::coke;
  iuse_function_list["CRACK"] = &iuse::crack;
  iuse_function_list["GRACK"] = &iuse::grack;
  iuse_function_list["METH"] = &iuse::meth;
  iuse_function_list["POISON"] = &iuse::poison;
  iuse_function_list["HALLU"] = &iuse::hallu;
  iuse_function_list["THORAZINE"] = &iuse::thorazine;
  iuse_function_list["PROZAC"] = &iuse::prozac;
  iuse_function_list["SLEEP"] = &iuse::sleep;
  iuse_function_list["IODINE"] = &iuse::iodine;
  iuse_function_list["FLUMED"] = &iuse::flumed;
  iuse_function_list["FLUSLEEP"] = &iuse::flusleep;
  iuse_function_list["INHALER"] = &iuse::inhaler;
  iuse_function_list["BLECH"] = &iuse::blech;
  iuse_function_list["MUTAGEN"] = &iuse::mutagen;
  iuse_function_list["MUTAGEN_3"] = &iuse::mutagen_3;
  iuse_function_list["PURIFIER"] = &iuse::purifier;
  iuse_function_list["MARLOSS"] = &iuse::marloss;
  iuse_function_list["DOGFOOD"] = &iuse::dogfood;

// TOOLS
  iuse_function_list["LIGHTER"] = &iuse::lighter;
  iuse_function_list["SEW"] = &iuse::sew;
  iuse_function_list["EXTRA_BATTERY"] = &iuse::extra_battery;
  iuse_function_list["SCISSORS"] = &iuse::scissors;
  iuse_function_list["EXTINGUISHER"] = &iuse::extinguisher;
  iuse_function_list["HAMMER"] = &iuse::hammer;
  iuse_function_list["LIGHT_OFF"] = &iuse::light_off;
  iuse_function_list["LIGHT_ON"] = &iuse::light_on;
  iuse_function_list["LIGHTSTRIP"] = &iuse::lightstrip;
  iuse_function_list["CAUTERIZE_ELEC"] = &iuse::cauterize_elec;
  iuse_function_list["WATER_PURIFIER"] = &iuse::water_purifier;
  iuse_function_list["TWO_WAY_RADIO"] = &iuse::two_way_radio;
  iuse_function_list["RADIO_OFF"] = &iuse::radio_off;
  iuse_function_list["RADIO_ON"] = &iuse::radio_on;
  iuse_function_list["NOISE_EMITTER_OFF"] = &iuse::noise_emitter_off;
  iuse_function_list["NOISE_EMITTER_ON"] = &iuse::noise_emitter_on;
  iuse_function_list["ROADMAP"] = &iuse::roadmap;
  //// These have special arguments and won't work here
  //iuse_function_list["ROADMAP_A_TARGET"] = &iuse::roadmap_a_target;
  //iuse_function_list["ROADMAP_TARGETS"] = &iuse::roadmap_targets;
  iuse_function_list["PICKLOCK"] = &iuse::picklock;
  iuse_function_list["CROWBAR"] = &iuse::crowbar;
  iuse_function_list["MAKEMOUND"] = &iuse::makemound;
  iuse_function_list["DIG"] = &iuse::dig;
  iuse_function_list["CHAINSAW_OFF"] = &iuse::chainsaw_off;
  iuse_function_list["CHAINSAW_ON"] = &iuse::chainsaw_on;
  iuse_function_list["JACKHAMMER"] = &iuse::jackhammer;
  iuse_function_list["JACQUESHAMMER"] = &iuse::jacqueshammer;
  iuse_function_list["PICKAXE"] = &iuse::pickaxe;
  iuse_function_list["SET_TRAP"] = &iuse::set_trap;
  iuse_function_list["GEIGER"] = &iuse::geiger;
  iuse_function_list["TELEPORT"] = &iuse::teleport;
  iuse_function_list["CAN_GOO"] = &iuse::can_goo;
  iuse_function_list["PIPEBOMB"] = &iuse::pipebomb;
  iuse_function_list["PIPEBOMB_ACT"] = &iuse::pipebomb_act;
  iuse_function_list["GRENADE"] = &iuse::grenade;
  iuse_function_list["GRENADE_ACT"] = &iuse::grenade_act;
  iuse_function_list["FLASHBANG"] = &iuse::flashbang;
  iuse_function_list["FLASHBANG_ACT"] = &iuse::flashbang_act;
  iuse_function_list["C4"] = &iuse::c4;
  iuse_function_list["C4ARMED"] = &iuse::c4armed;
  iuse_function_list["EMPBOMB"] = &iuse::EMPbomb;
  iuse_function_list["EMPBOMB_ACT"] = &iuse::EMPbomb_act;
  iuse_function_list["SCRAMBLER"] = &iuse::scrambler;
  iuse_function_list["SCRAMBLER_ACT"] = &iuse::scrambler_act;
  iuse_function_list["GASBOMB"] = &iuse::gasbomb;
  iuse_function_list["GASBOMB_ACT"] = &iuse::gasbomb_act;
  iuse_function_list["SMOKEBOMB"] = &iuse::smokebomb;
  iuse_function_list["SMOKEBOMB_ACT"] = &iuse::smokebomb_act;
  iuse_function_list["ACIDBOMB"] = &iuse::acidbomb;
  iuse_function_list["ACIDBOMB_ACT"] = &iuse::acidbomb_act;
  iuse_function_list["MOLOTOV"] = &iuse::molotov;
  iuse_function_list["MOLOTOV_LIT"] = &iuse::molotov_lit;
  iuse_function_list["DYNAMITE"] = &iuse::dynamite;
  iuse_function_list["DYNAMITE_ACT"] = &iuse::dynamite_act;
  iuse_function_list["FIRECRACKER_PACK"] = &iuse::firecracker_pack;
  iuse_function_list["FIRECRACKER_PACK_ACT"] = &iuse::firecracker_pack_act;
  iuse_function_list["FIRECRACKER"] = &iuse::firecracker;
  iuse_function_list["FIRECRACKER_ACT"] = &iuse::firecracker_act;
  iuse_function_list["MININUKE"] = &iuse::mininuke;
  iuse_function_list["MININUKE_ACT"] = &iuse::mininuke_act;
  iuse_function_list["PHEROMONE"] = &iuse::pheromone;
  iuse_function_list["PORTAL"] = &iuse::portal;
  iuse_function_list["MANHACK"] = &iuse::manhack;
  iuse_function_list["TURRET"] = &iuse::turret;
  iuse_function_list["UPS_OFF"] = &iuse::UPS_off;
  iuse_function_list["UPS_ON"] = &iuse::UPS_on;
  iuse_function_list["TAZER"] = &iuse::tazer;
  iuse_function_list["MP3"] = &iuse::mp3;
  iuse_function_list["MP3_ON"] = &iuse::mp3_on;
  iuse_function_list["VORTEX"] = &iuse::vortex;
  iuse_function_list["DOG_WHISTLE"] = &iuse::dog_whistle;
  iuse_function_list["VACUTAINER"] = &iuse::vacutainer;
  iuse_function_list["KNIFE"] = &iuse::knife;
  iuse_function_list["LUMBER"] = &iuse::lumber;
  iuse_function_list["HACKSAW"] = &iuse::hacksaw;
  iuse_function_list["TENT"] = &iuse::tent;
  iuse_function_list["SHELTER"] = &iuse::shelter;
  iuse_function_list["TORCH"] = &iuse::torch;
  iuse_function_list["TORCH_LIT"] = &iuse::torch_lit;
  iuse_function_list["CANDLE"] = &iuse::candle;
  iuse_function_list["CANDLE_LIT"] = &iuse::candle_lit;
  iuse_function_list["BULLET_PULLER"] = &iuse::bullet_puller;
  iuse_function_list["BOLTCUTTERS"] = &iuse::boltcutters;
  iuse_function_list["MOP"] = &iuse::mop;
  iuse_function_list["SPRAY_CAN"] = &iuse::spray_can;
  iuse_function_list["RAG"] = &iuse::rag;
  iuse_function_list["PDA"] = &iuse::pda;
  iuse_function_list["PDA_FLASHLIGHT"] = &iuse::pda_flashlight;
  iuse_function_list["LAW"] = &iuse::LAW;
  iuse_function_list["HEATPACK"] = &iuse::heatpack;
  iuse_function_list["DEJAR"] = &iuse::dejar;


// MACGUFFINS
  iuse_function_list["MCG_NOTE"] = &iuse::mcg_note;
// ARTIFACTS
// This function is used when an artifact is activated
// It examines the item's artifact-specific properties
// See artifact.h for a list
  iuse_function_list["ARTIFACT"] = &iuse::artifact;
}

//Will eventually be deprecated - Loads existing item format into the item factory
void Item_factory::init(game* main_game){
    // Make a copy of our items loaded from JSON
    std::map<Item_tag, itype*> new_templates = m_templates;
    //Copy the hardcoded template pointers to the factory list
    m_templates.insert(main_game->itypes.begin(), main_game->itypes.end());
    //Copy the JSON-derived items to the legacy list
    main_game->itypes.insert(new_templates.begin(), new_templates.end());
}

//Returns the template with the given identification tag
itype* Item_factory::find_template(const Item_tag id){
    std::map<Item_tag, itype*>::iterator found = m_templates.find(id);
    if(found != m_templates.end()){
        return found->second;
    }
    else{
        return m_missing_item;
    }
}

//Returns a random template from the list of all templates.
itype* Item_factory::random_template(){
    return template_from("ALL");
}

//Returns a random template from those with the given group tag
itype* Item_factory::template_from(const Item_tag group_tag){
    return find_template( id_from(group_tag) );
}

//Returns a random template name from the list of all templates.
const Item_tag Item_factory::random_id(){
    return id_from("ALL");
}

//Returns a random template name from the list of all templates.
const Item_tag Item_factory::id_from(const Item_tag group_tag){
    std::map<Item_tag, Item_group*>::iterator group_iter = m_template_groups.find(group_tag);
    //If the tag isn't found, just return a reference to missing item.
    if(group_iter != m_template_groups.end()){
        return group_iter->second->get_id();
    } else {
        return "MISSING_ITEM";
    }
}


item Item_factory::create(Item_tag id, int created_at){
    return item(find_template(id),0);
}
Item_list Item_factory::create(Item_tag id, int created_at, int quantity){
    Item_list new_items;
    item new_item_base = create(id, created_at);
    for(int ii=0;ii<quantity;++ii){
        new_items.push_back(new_item_base.clone());
    }
    return new_items;
}
item Item_factory::create_from(Item_tag group, int created_at){
    return create(id_from(group), created_at);
}
Item_list Item_factory::create_from(Item_tag group, int created_at, int quantity){
    return create(id_from(group), created_at, quantity);
}
item Item_factory::create_random(int created_at){
    return create(random_id(), created_at);
}
Item_list Item_factory::create_random(int created_at, int quantity){
    Item_list new_items;
    item new_item_base = create(random_id(), created_at);
    for(int ii=0;ii<quantity;++ii){
        new_items.push_back(new_item_base.clone());
    }
    return new_items;
}

///////////////////////
// DATA FILE READING //
///////////////////////

void Item_factory::load_item_templates(){
    load_item_templates_from("data/raw/items/instruments.json");
    load_item_templates_from("data/raw/items/melee.json");
    load_item_groups_from("data/raw/item_groups.json");
}

// Load values from this data file into m_templates
// TODO: Consider appropriate location for this code. Is this really where it belongs?
//       At the very least, it seems the json blah_from methods could be used elsewhere
void Item_factory::load_item_templates_from(const std::string file_name){
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
                Item_tag new_id = key_pair->second.get<std::string>();

                // If everything works out, add the item to the group list...
                // unless a similar item is already there
                if(m_templates.find(new_id) != m_templates.end()){
                    std::cerr << "Item definition skipped, id " << new_id << " already exists." << std::endl;
                } else {
                    itype* new_item_template;
                    if (!entry_body.has("type"))
                    {
                        new_item_template = new itype();
                    }
                    else
                    {
                        std::string type_label = string_from_json(new_id, "type", entry_body);
                        if (type_label == "MELEE")
                        {
                            new_item_template = new itype();
                        }
                        else if (type_label == "GUN")
                        {
                            it_gun* gun_template = new it_gun();
                            gun_template->ammo = ammo_from_json(new_id, "ammo", entry_body);
                            gun_template->skill_used = Skill::skill(string_from_json(new_id, "skill", entry_body));
                            gun_template->dmg_bonus = int_from_json(new_id, "ranged_damage", entry_body);
                            gun_template->accuracy = int_from_json(new_id, "accuracy", entry_body);
                            gun_template->recoil = int_from_json(new_id, "recoil", entry_body);
                            gun_template->durability = int_from_json(new_id, "durability", entry_body);
                            gun_template->burst = int_from_json(new_id, "burst", entry_body);
                            gun_template->clip = int_from_json(new_id, "clip_size", entry_body);
                            gun_template->reload_time = int_from_json(new_id, "reload_time", entry_body);

                            new_item_template = gun_template;
                        }
                        else
                        {
                            std::cerr << "Item definition for " << new_id << " skipped, unrecognized type: " <<
                                      type_label << std::endl;
                            break;
                        }
                    }
                    new_item_template->id = new_id;
                    m_templates[new_id] = new_item_template;

                    // And then proceed to assign the correct field
                    new_item_template->rarity = int_from_json(new_id, "rarity", entry_body);
                    new_item_template->name = string_from_json(new_id, "name", entry_body);
                    new_item_template->sym = char_from_json(new_id, "symbol", entry_body);
                    new_item_template->color = color_from_json(new_id, "color", entry_body);
                    new_item_template->description = string_from_json(new_id, "description", entry_body);
                    new_item_template->m1 = material_from_json(new_id, "material", entry_body, 0);
                    new_item_template->m2 = material_from_json(new_id, "material", entry_body, 1);
                    new_item_template->volume = int_from_json(new_id, "volume", entry_body);
                    new_item_template->weight = int_from_json(new_id, "weight", entry_body);
                    new_item_template->melee_dam = int_from_json(new_id, "damage", entry_body);
                    new_item_template->melee_cut = int_from_json(new_id, "cutting", entry_body);
                    new_item_template->m_to_hit = int_from_json(new_id, "to_hit", entry_body);
                    new_item_template->use = use_from_json(new_id, "use_action", entry_body);
                }
            }
        }
    }
}

// Load values from this data file into m_template_groups
void Item_factory::load_item_groups_from(const std::string file_name){
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
        std::cerr << file_name << " is not an array of item groups"<< std::endl;
        exit(2);
    }

    //Crawl through once and create an entry for every definition
    const picojson::array& all_items = input_value.get<picojson::array>();
    for (picojson::array::const_iterator entry = all_items.begin(); entry != all_items.end(); ++entry) {
        if( !(entry->is<picojson::object>()) ){
            std::cerr << "Invalid group definition, entry not a JSON object" << std::endl;
        }
        else{
            const picojson::value::object& entry_body = entry->get<picojson::object>();

            // The one element we absolutely require for an item definition is an id
            picojson::value::object::const_iterator key_pair = entry_body.find("id");
            if( key_pair == entry_body.end() || !(key_pair->second.is<std::string>()) ){
                std::cerr << "Group definition skipped, no id found or id was malformed." << std::endl;
            } else {
                Item_tag group_id = key_pair->second.get<std::string>();
                m_template_groups[group_id] = new Item_group(group_id);
            }
        }
    }
    //Once all the group definitions are set, fill them out
    for (picojson::array::const_iterator entry = all_items.begin(); entry != all_items.end(); ++entry) {
        const picojson::value::object& entry_body = entry->get<picojson::object>();

        Item_tag group_id = entry_body.find("id")->second.get<std::string>();
        Item_group current_group = *m_template_groups.find(group_id)->second;

        //Add items
        picojson::value::object::const_iterator key_pair = entry_body.find("items");
        if( key_pair != entry_body.end() ){
            if( !(key_pair->second.is<picojson::array>()) ){
                std::cerr << "Invalid item list for group definition '"+group_id+"', list of items not an array." << std::endl;
            } else {
                //We have confirmed that we have a list of SOMETHING, now let's add them one at a time.
                const picojson::array& items_to_add = key_pair->second.get<picojson::array>();
                for (picojson::array::const_iterator item_pair = items_to_add.begin(); item_pair != items_to_add.end(); ++item_pair) {
                    //Before adding, make sure this element is in the right format, namely ["TAG_NAME", frequency number]
                    if(!(item_pair->is<picojson::array>())){
                        std::cerr << "Invalid item list for group definition '"+group_id+"', element is not an array." << std::endl;
                    } else if(item_pair->get<picojson::array>().size()!=2){
                        std::cerr << "Invalid item list for group definition '"+group_id+"', element does not have 2 values." << std::endl;
                    } else {
                        picojson::array item_frequency_array = item_pair->get<picojson::array>();
                        //Finally, insure that the first value is a string, and the second is a number
                        if(!item_frequency_array[0].is<std::string>() || !item_frequency_array[1].is<double>() ){
                            std::cerr << "Invalid item list for group definition '"+group_id+"', element is not a valid tag/frequency pair." << std::endl;
                        } else {
                            current_group.add_entry(item_frequency_array[0].get<std::string>(), (int)item_frequency_array[1].get<double>());
                        }
                    }
                }
            }
        }

        //Add groups
        key_pair = entry_body.find("groups");
        if(key_pair != entry_body.end()){
            if( !(key_pair->second.is<picojson::array>()) ){
                std::cerr << "Invalid group list for group definition '"+group_id+"', list of items not an array." << std::endl;
            } else {
                //We have confirmed that we have a list of SOMETHING, now let's add them one at a time.
                const picojson::array& items_to_add = key_pair->second.get<picojson::array>();
                for (picojson::array::const_iterator item_pair = items_to_add.begin(); item_pair != items_to_add.end(); ++item_pair) {
                    //Before adding, make sure this element is in the right format, namely ["TAG_NAME", frequency number]
                    if(!(item_pair->is<picojson::array>())){
                        std::cerr << "Invalid group list for group definition '"+group_id+"', element is not an array." << std::endl;
                    } else if(item_pair->get<picojson::array>().size()!=2){
                        std::cerr << "Invalid group list for group definition '"+group_id+"', element does not have 2 values." << std::endl;
                    } else {
                        picojson::array item_frequency_array = item_pair->get<picojson::array>();
                        //Finally, insure that the first value is a string, and the second is a number
                        if(!item_frequency_array[0].is<std::string>() || !item_frequency_array[1].is<double>() ){
                            std::cerr << "Invalid group list for group definition '"+group_id+"', element is not a valid tag/frequency pair." << std::endl;
                        } else {
                            Item_group* subgroup = m_template_groups.find(item_frequency_array[0].get<std::string>())->second;
                            current_group.add_group(subgroup, (int)item_frequency_array[1].get<double>());
                        }
                    }
                }
            }
        }
    }
}

//Grab string, with appropriate error handling
Item_tag Item_factory::string_from_json(Item_tag new_id, Item_tag index, picojson::value::object value_map){
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
char Item_factory::char_from_json(Item_tag new_id, Item_tag index, picojson::value::object value_map){
    std::string symbol = string_from_json(new_id, index, value_map);
    if(symbol == ""){
        std::cerr << "Item "<< new_id << " attribute  " << "was skipped, empty string not allowed." << std::endl;
        return 'X';
    }
    return symbol[0];
}

//Grab int, with appropriate error handling
int Item_factory::int_from_json(Item_tag new_id, Item_tag index, picojson::value::object value_map){
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
nc_color Item_factory::color_from_json(Item_tag new_id, Item_tag index, picojson::value::object value_map){
    std::string new_color = string_from_json(new_id, index, value_map);
    if("red"==new_color){
        return c_red;
    } else if("blue"==new_color){
        return c_blue;
    } else if("green"==new_color){
        return c_green;
    } else if("light_gray"==new_color){
        return c_ltgray;
    } else if("light_cyan"==new_color){
        return c_ltcyan;
    } else if("brown"==new_color){
        return c_brown;
    } else if("light_red"==new_color){
        return c_red;
    } else if("white"==new_color){
        return c_white;
    } else if("light_blue"==new_color){
        return c_ltblue;
    } else if("yellow"==new_color){
        return c_yellow;
    } else if("magenta"==new_color){
        return c_magenta;
    } else if("cyan"==new_color){
        return c_cyan;
    } else if("light_gray"==new_color){
        return c_ltgray;
    } else if("dark_gray"==new_color){
        return c_dkgray;
    } else if("light_green"==new_color){
        return c_ltgreen;
    } else if("pink"==new_color){
        return c_pink;
    } else {
        std::cerr << "Item "<< new_id << " attribute color was skipped, not a color. Color is required." << std::endl;
        return c_white;
    }
}

ammotype Item_factory::ammo_from_json(Item_tag new_id, Item_tag index, picojson::value::object value_map){
    std::string new_ammo = string_from_json(new_id, index, value_map);
    if("nail"==new_ammo){
        return AT_NAIL;
    } else if ("BB" == new_ammo) {
        return AT_BB;
    } else if ("bolt" == new_ammo) {
        return AT_BOLT;
    } else if ("arrow" == new_ammo) {
        return AT_ARROW;
    } else if ("shot" == new_ammo) {
        return AT_SHOT;
    } else if (".22" == new_ammo) {
        return AT_22;
    } else if ("9mm" == new_ammo) {
        return AT_9MM;
    } else if ("7.62x25mm" == new_ammo) {
        return AT_762x25;
    } else if (".38" == new_ammo) {
        return AT_38;
    } else if (".40" == new_ammo) {
        return AT_40;
    } else if (".44" == new_ammo) {
        return AT_44;
    } else if (".45" == new_ammo) {
        return AT_45;
    } else if ("5.7mm" == new_ammo) {
        return AT_57;
    } else if ("4.6mm" == new_ammo) {
        return AT_46;
    } else if ("7.62x39mm" == new_ammo) {
        return AT_762;
    } else if (".223" == new_ammo) {
        return AT_223;
    } else if (".30-06" == new_ammo) {
        return AT_3006;
    } else if (".308" == new_ammo) {
        return AT_308;
    } else if ("40mm" == new_ammo) {
        return AT_40MM;
    } else if ("66mm" == new_ammo) {
        return AT_66MM;
    } else if ("gasoline" == new_ammo) {
        return AT_GAS;
    } else if ("thread" == new_ammo) {
        return AT_THREAD;
    } else if ("battery" == new_ammo) {
        return AT_BATT;
    } else if ("plutonium" == new_ammo) {
        return AT_PLUT;
    } else if ("muscle" == new_ammo) {
        return AT_MUSCLE;
    } else if ("fusion" == new_ammo) {
        return AT_FUSION;
    } else if ("12mm" == new_ammo) {
        return AT_12MM;
    } else if ("plasma" == new_ammo) {
        return AT_PLASMA;
    } else if ("water" == new_ammo) {
        return AT_WATER;
    } else {
        std::cerr << "Item "<< new_id << " attribute ammo was skipped, not recognized. Ammo is required for this item." << std::endl;
        return AT_NULL;
    }
}

Use_function Item_factory::use_from_json(Item_tag new_id, Item_tag index, picojson::value::object value_map){
    picojson::value::object::const_iterator value_pair = value_map.find(index);

    //If none is found, just use the standard none action
    Item_tag function_name = "NONE";
    //Otherwise, grab the right label to look for
    if(value_pair != value_map.end()){
        function_name = string_from_json(new_id, index, value_map);
    }

    std::map<Item_tag, Use_function>::iterator found_function = iuse_function_list.find(function_name);

    //Before returning, make sure sure the function actually exists
    if(found_function != iuse_function_list.end()){
        return found_function->second;
    } else {
        //Otherwise, return a hardcoded function we know exists (hopefully)
        std::cerr << "Item "<< new_id << " attribute '"<< index <<"' was skipped, no iuse function defined for the tag '" << function_name << "." << std::endl;
        return &iuse::none;
    }
}

material Item_factory::material_from_json(Item_tag new_id, Item_tag index, picojson::value::object value_map, int to_return){
    //If the value isn't found, just return a group of null materials
    material material_list[2] = {MNULL, MNULL};

    picojson::value::object::const_iterator value_pair = value_map.find(index);
    if(value_pair != value_map.end()){
        if(value_pair->second.is<std::string>()){
            material_list[0] = material_from_tag(new_id, value_pair->second.get<std::string>());
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
    return material_list[to_return];
}

material Item_factory::material_from_tag(Item_tag new_id, Item_tag name){
    // Map the valid input tags to a valid material

    // This should clearly be some sort of map, stored somewhere
    // ...unless it can get replaced entirely, which would be nice.
    // For now, though, that isn't the problem I'm solving.
    if(name == "VEGGY"){
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
