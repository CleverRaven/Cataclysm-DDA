#include "item_factory.h"
#include "rng.h"
#include "enums.h"
#include "catajson.h"
#include "addiction.h"
#include "translations.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <stdio.h>

// mfb(n) converts a flag to its appropriate position in covers's bitfield
#ifndef mfb
#define mfb(n) static_cast <unsigned long> (1 << (n))
#endif

Item_factory* item_controller = new Item_factory();

//Every item factory comes with a missing item
Item_factory::Item_factory(){
    init();
    m_missing_item = new itype();
    m_missing_item->name = _("Error: Item Missing.");
    m_missing_item->description = _("There is only the space where an object should be, but isn't. No item template of this type exists.");
    m_templates["MISSING_ITEM"]=m_missing_item;
}

void Item_factory::init(){
    //Populate the iuse functions
    iuse_function_list["NONE"] = &iuse::none;
    iuse_function_list["SEWAGE"] = &iuse::sewage;

    iuse_function_list["HONEYCOMB"] = &iuse::honeycomb;
    iuse_function_list["ROYAL_JELLY"] = &iuse::royal_jelly;
    iuse_function_list["BANDAGE"] = &iuse::bandage;
    iuse_function_list["FIRSTAID"] = &iuse::firstaid;
    iuse_function_list["DISINFECTANT"] = &iuse::disinfectant;
    iuse_function_list["CAFF"] = &iuse::caff;
    iuse_function_list["ALCOHOL"] = &iuse::alcohol;
    iuse_function_list["ALCOHOL_WEAK"] = &iuse::alcohol_weak;
    iuse_function_list["PKILL"] = &iuse::pkill;
    iuse_function_list["XANAX"] = &iuse::xanax;
    iuse_function_list["CIG"] = &iuse::cig;
    iuse_function_list["ANTIBIOTIC"] = &iuse::antibiotic;
    iuse_function_list["WEED"] = &iuse::weed;
    iuse_function_list["COKE"] = &iuse::coke;
    iuse_function_list["CRACK"] = &iuse::crack;
    iuse_function_list["GRACK"] = &iuse::grack;
    iuse_function_list["METH"] = &iuse::meth;
    iuse_function_list["VITAMINS"] = &iuse::vitamins;
    iuse_function_list["VACCINE"] = &iuse::vaccine;
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
    iuse_function_list["PURIFIER"] = &iuse::purifier;
    iuse_function_list["MARLOSS"] = &iuse::marloss;
    iuse_function_list["DOGFOOD"] = &iuse::dogfood;

  // TOOLS
    iuse_function_list["LIGHTER"] = &iuse::lighter;
    iuse_function_list["PRIMITIVE_FIRE"] = &iuse::primitive_fire;
    iuse_function_list["SEW"] = &iuse::sew;
    iuse_function_list["EXTRA_BATTERY"] = &iuse::extra_battery;
    iuse_function_list["SCISSORS"] = &iuse::scissors;
    iuse_function_list["EXTINGUISHER"] = &iuse::extinguisher;
    iuse_function_list["HAMMER"] = &iuse::hammer;
    iuse_function_list["LIGHT_OFF"] = &iuse::light_off;
    iuse_function_list["LIGHT_ON"] = &iuse::light_on;
    iuse_function_list["GASOLINE_LANTERN_OFF"] = &iuse::gasoline_lantern_off;
    iuse_function_list["GASOLINE_LANTERN_ON"] = &iuse::gasoline_lantern_on;
    iuse_function_list["LIGHTSTRIP"] = &iuse::lightstrip;
    iuse_function_list["LIGHTSTRIP_ACTIVE"] = &iuse::lightstrip_active;
    iuse_function_list["GLOWSTICK"] = &iuse::glowstick;
    iuse_function_list["GLOWSTICK_ACTIVE"] = &iuse::glowstick_active;
    iuse_function_list["DIRECTIONAL_ANTENNA"] = &iuse::directional_antenna;
    iuse_function_list["CAUTERIZE_ELEC"] = &iuse::cauterize_elec;
    iuse_function_list["SOLDER_WELD"] = &iuse::solder_weld;
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
    iuse_function_list["SIPHON"] = &iuse::siphon;
    iuse_function_list["CHAINSAW_OFF"] = &iuse::chainsaw_off;
    iuse_function_list["CHAINSAW_ON"] = &iuse::chainsaw_on;
    iuse_function_list["SHISHKEBAB_OFF"] = &iuse::shishkebab_off;
    iuse_function_list["SHISHKEBAB_ON"] = &iuse::shishkebab_on;
    iuse_function_list["FIREMACHETE_OFF"] = &iuse::firemachete_off;
    iuse_function_list["FIREMACHETE_ON"] = &iuse::firemachete_on;
    iuse_function_list["BROADFIRE_OFF"] = &iuse::broadfire_off;
    iuse_function_list["BROADFIRE_ON"] = &iuse::broadfire_on;
    iuse_function_list["FIREKATANA_OFF"] = &iuse::firekatana_off;
    iuse_function_list["FIREKATANA_ON"] = &iuse::firekatana_on;
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
    iuse_function_list["ARROW_FLAMABLE"] = &iuse::arrow_flamable;
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
    iuse_function_list["adv_UPS_OFF"] = &iuse::adv_UPS_off;
    iuse_function_list["adv_UPS_ON"] = &iuse::adv_UPS_on;
    iuse_function_list["TAZER"] = &iuse::tazer;
    iuse_function_list["MP3"] = &iuse::mp3;
    iuse_function_list["MP3_ON"] = &iuse::mp3_on;
    iuse_function_list["PORTABLE_GAME"] = &iuse::portable_game;
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
    iuse_function_list["RAD_BADGE"] = &iuse::rad_badge;
    iuse_function_list["BOOTS"] = &iuse::boots;
    iuse_function_list["ABSORBENT"] = &iuse::towel;
    iuse_function_list["UNFOLD_BICYCLE"] = &iuse::unfold_bicycle;
    iuse_function_list["ADRENALINE_INJECTOR"] = &iuse::adrenaline_injector;
    // MACGUFFINS
    iuse_function_list["MCG_NOTE"] = &iuse::mcg_note;
    // ARTIFACTS
    // This function is used when an artifact is activated
    // It examines the item's artifact-specific properties
    // See artifact.h for a list
    iuse_function_list["ARTIFACT"] = &iuse::artifact;

    // Offensive Techniques
    techniques_list["SWEEP"] = mfb(TEC_SWEEP);
    techniques_list["PRECISE"] = mfb(TEC_PRECISE);
    techniques_list["BRUTAL"] = mfb(TEC_BRUTAL);
    techniques_list["GRAB"] = mfb(TEC_GRAB);
    techniques_list["WIDE"] = mfb(TEC_WIDE);
    techniques_list["RAPID"] = mfb(TEC_RAPID);
    techniques_list["FEINT"] = mfb(TEC_FEINT);
    techniques_list["THROW"] = mfb(TEC_THROW);
    techniques_list["DISARM"] = mfb(TEC_DISARM);
    techniques_list["FLAMING"] = mfb(TEC_FLAMING);
    // Defensive Techniques
    techniques_list["BLOCK"] = mfb(TEC_BLOCK);
    techniques_list["BLOCK_LEGS"] = mfb(TEC_BLOCK_LEGS);
    techniques_list["WBLOCK_1"] = mfb(TEC_WBLOCK_1);
    techniques_list["WBLOCK_2"] = mfb(TEC_WBLOCK_2);
    techniques_list["WBLOCK_3"] = mfb(TEC_WBLOCK_3);
    techniques_list["COUNTER"] = mfb(TEC_COUNTER);
    techniques_list["BREAK"] = mfb(TEC_BREAK);
    techniques_list["DEF_THROW"] = mfb(TEC_DEF_THROW);
    techniques_list["DEF_DISARM"] = mfb(TEC_DEF_DISARM);

    bodyparts_list["TORSO"] = mfb(bp_torso);
    bodyparts_list["HEAD"] = mfb(bp_head);
    bodyparts_list["EYES"] = mfb(bp_eyes);
    bodyparts_list["MOUTH"] = mfb(bp_mouth);
    bodyparts_list["ARMS"] = mfb(bp_arms);
    bodyparts_list["HANDS"] = mfb(bp_hands);
    bodyparts_list["LEGS"] = mfb(bp_legs);
    bodyparts_list["FEET"] = mfb(bp_feet);
}

//Will eventually be deprecated - Loads existing item format into the item factory, and vice versa
void Item_factory::init(game* main_game) throw(std::string){
    try {
        load_item_templates(); // this one HAS to be called after game is created
    }
    catch (std::string &error_message) {
        throw;
    }
    // Make a copy of our items loaded from JSON
    std::map<Item_tag, itype*> new_templates = m_templates;
    //Copy the hardcoded template pointers to the factory list
    m_templates.insert(main_game->itypes.begin(), main_game->itypes.end());
    //Copy the JSON-derived items to the legacy list
    main_game->itypes.insert(new_templates.begin(), new_templates.end());
    //And add them to the various item lists, as needed.
    for(std::map<Item_tag, itype*>::iterator iter = new_templates.begin(); iter != new_templates.end(); ++iter) {
      standard_itype_ids.push_back(iter->first);
    }
    load_item_groups_from(main_game, "data/raw/item_groups.json");

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
    return item(find_template(id), created_at);
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

bool Item_factory::group_contains_item(Item_tag group_tag, Item_tag item) {
	Item_group *current_group = m_template_groups.find(group_tag)->second;
	if(current_group) {
		return current_group->has_item(item);
	} else {
		return 0;
	}
}

///////////////////////
// DATA FILE READING //
///////////////////////

void Item_factory::load_item_templates() throw(std::string){
    try {
    load_item_templates_from("data/raw/items/melee.json");
    load_item_templates_from("data/raw/items/ranged.json");
    load_item_templates_from("data/raw/items/ammo.json");
    load_item_templates_from("data/raw/items/mods.json");
    load_item_templates_from("data/raw/items/tools.json");
    load_item_templates_from("data/raw/items/containers.json");
    load_item_templates_from("data/raw/items/comestibles.json");
    load_item_templates_from("data/raw/items/armor.json");
    load_item_templates_from("data/raw/items/books.json");
    load_item_templates_from("data/raw/items/archery.json");
    }
    catch (std::string &error_message) {
        throw;
    }
}

// Load values from this data file into m_templates
void Item_factory::load_item_templates_from(const std::string file_name) throw (std::string){
    catajson all_items(file_name);

    if(! json_good())
    	throw (std::string)"Could not open " + file_name;

    if (! all_items.is_array())
        throw file_name + (std::string)"is not an array of item_templates";

    //Crawl through and extract the items
    for (all_items.set_begin(); all_items.has_curr(); all_items.next()) {
        catajson entry = all_items.curr();
        // The one element we absolutely require for an item definition is an id
        if(!entry.has("id") || !entry.get("id").is_string())
        {
            debugmsg("Item definition skipped, no id found or id was malformed.");
        }
        else
        {
            Item_tag new_id = entry.get("id").as_string();

            // If everything works out, add the item to the group list...
            // unless a similar item is already there
            if(m_templates.find(new_id) != m_templates.end())
            {
                debugmsg("Item definition skipped, id %s already exists.", new_id.c_str());
            }
            else
            {
                itype* new_item_template;
                if (!entry.has("type"))
                {
                    new_item_template = new itype();
                }
                else
                {
                    std::string type_label = entry.get("type").as_string();
                    if (type_label == "GUNMOD")
                    {
                        it_gunmod* gunmod_template = new it_gunmod();
                        gunmod_template->damage = entry.get("damage_modifier").as_int();;
                        gunmod_template->loudness = entry.get("loudness_modifier").as_int();
                        gunmod_template->newtype = entry.get("ammo_modifier").as_string();
                        gunmod_template->used_on_pistol = is_mod_target(entry.get("mod_targets"), "pistol");
                        gunmod_template->used_on_shotgun = is_mod_target(entry.get("mod_targets"), "shotgun");
                        gunmod_template->used_on_smg = is_mod_target(entry.get("mod_targets"), "smg");
                        gunmod_template->used_on_rifle = is_mod_target(entry.get("mod_targets"), "rifle");
                        gunmod_template->dispersion = entry.get("dispersion_modifier").as_int();
                        gunmod_template->recoil = entry.get("recoil_modifier").as_int();
                        gunmod_template->burst = entry.get("burst_modifier").as_int();
                        gunmod_template->clip = entry.get("clip_size_modifier").as_int();
                        if( entry.has("acceptable_ammo") ) {
                            tags_from_json( entry.get("acceptable_ammo"),
                                            gunmod_template->acceptible_ammo_types );
                        }
                        new_item_template = gunmod_template;
                    }
                    else if (type_label == "COMESTIBLE")
                    {
                        it_comest* comest_template = new it_comest();
                        comest_template->comesttype = entry.get("comestible_type").as_string();
                        comest_template->tool = entry.get("tool").as_string();
                        comest_template->container = entry.get("container").as_string();
                        comest_template->quench = entry.get("quench").as_int();
                        comest_template->nutr = entry.get("nutrition").as_int();
                        comest_template->spoils = entry.get("spoils_in").as_int();
                        comest_template->addict = entry.get("addiction_potential").as_int();
                        comest_template->charges = entry.get("charges").as_int();
                        comest_template->stim = entry.get("stim").as_int();
                        comest_template->healthy = entry.get("heal").as_int();
                        comest_template->fun = entry.get("fun").as_int();
                        comest_template->add = addiction_type(entry.get("addiction_type").as_string());
                        new_item_template = comest_template;
                    }
                    else if (type_label == "GUN")
                    {
                        it_gun* gun_template = new it_gun();
                        gun_template->ammo = entry.get("ammo").as_string();
                        gun_template->skill_used = Skill::skill(entry.get("skill").as_string());
                        gun_template->dmg_bonus = entry.get("ranged_damage").as_int();
                        gun_template->range = entry.get("range").as_int();
                        gun_template->dispersion = entry.get("dispersion").as_int();
                        gun_template->recoil = entry.get("recoil").as_int();
                        gun_template->durability = entry.get("durability").as_int();
                        gun_template->burst = entry.get("burst").as_int();
                        gun_template->clip = entry.get("clip_size").as_int();
                        gun_template->reload_time = entry.get("reload").as_int();

                        new_item_template = gun_template;
                    }
                    else if (type_label == "TOOL")
                    {
                        it_tool* tool_template = new it_tool();
                        tool_template->ammo = entry.get("ammo").as_string();
                        tool_template->max_charges = entry.get("max_charges").as_int();
                        tool_template->def_charges = entry.get("initial_charges").as_int();
                        tool_template->charges_per_use = entry.get("charges_per_use").as_int();
                        tool_template->turns_per_charge = entry.get("turns_per_charge").as_int();
                        tool_template->revert_to = entry.get("revert_to").as_string();

                        new_item_template = tool_template;
                    }
                    else if (type_label == "AMMO")
                    {
                        it_ammo* ammo_template = new it_ammo();
                        ammo_template->type = entry.get("ammo_type").as_string();
                        if(entry.has("casing")) {
                            ammo_template->casing = entry.get("casing").as_string();
                        }
                        ammo_template->damage = entry.get("damage").as_int();
                        ammo_template->pierce = entry.get("pierce").as_int();
                        ammo_template->range = entry.get("range").as_int();
                        ammo_template->dispersion =
                            entry.get("dispersion").as_int();
                        ammo_template->recoil = entry.get("recoil").as_int();
                        ammo_template->count = entry.get("count").as_int();
                        if( entry.has("effects") ) {
                            tags_from_json(entry.get("effects"), ammo_template->ammo_effects);
                        }

                        new_item_template = ammo_template;
                    }
                    else if (type_label == "ARMOR")
                    {
                        it_armor* armor_template = new it_armor();

                        armor_template->encumber = entry.get("encumberance").as_int();
                        armor_template->coverage = entry.get("coverage").as_int();
                        armor_template->thickness = entry.get("material_thickness").as_int();
                        armor_template->env_resist = entry.get("enviromental_protection").as_int();
                        armor_template->warmth = entry.get("warmth").as_int();
                        armor_template->storage = entry.get("storage").as_int();
                        armor_template->power_armor = entry.has("power_armor") ? entry.get("power_armor").as_bool() : false;
                        armor_template->covers = entry.has("covers") ?
                          flags_from_json(entry.get("covers"),"bodyparts") : 0;

                        new_item_template = armor_template;
                    }
                    else if (type_label == "BOOK")
                    {
                        it_book* book_template = new it_book();

                        book_template->level = entry.get("max_level").as_int();
                        book_template->req = entry.get("required_level").as_int();
                        book_template->fun = entry.get("fun").as_int();
                        book_template->intel = entry.get("intelligence").as_int();
                        book_template->time = entry.get("time").as_int();
                        book_template->type = Skill::skill(entry.get("skill").as_string());

                        new_item_template = book_template;
                    }
                    else if (type_label == "CONTAINER")
                    {
                        it_container* container_template = new it_container();

                        container_template->contains = entry.get("contains").as_int();

                        new_item_template = container_template;
                    }
                    else
                    {
                        debugmsg("Item definition for %s skipped, unrecognized type: %s", new_id.c_str(),
                                 type_label.c_str());
                        break;
                    }
                }
                new_item_template->id = new_id;
                m_templates[new_id] = new_item_template;

                // And then proceed to assign the correct field
                new_item_template->price = entry.get("price").as_int();
                new_item_template->name = _(entry.get("name").as_string().c_str());
                new_item_template->sym = entry.get("symbol").as_char();
                new_item_template->color = color_from_string(entry.get("color").as_string());
                new_item_template->description = _(entry.get("description").as_string().c_str());
                if(entry.has("material")){
                  set_material_from_json(new_id, entry.get("material"));
                } else {
                  new_item_template->m1 = "null";
                  new_item_template->m2 = "null";
                }
                Item_tag new_phase = "solid";
                if(entry.has("phase")){
                    new_phase = entry.get("phase").as_string();
                }
                new_item_template->phase = phase_from_tag(new_phase);
                new_item_template->volume = entry.get("volume").as_int();
                new_item_template->weight = entry.get("weight").as_int();
                new_item_template->melee_dam = entry.get("bashing").as_int();
                new_item_template->melee_cut = entry.get("cutting").as_int();
                new_item_template->m_to_hit = entry.get("to_hit").as_int();

                if( entry.has("flags") )
                {
                    new_item_template->item_tags = entry.get("flags").as_tags();
                    /*
                    List of current flags
                    FIT - Reduces encumberance by one
                    VARSIZE - Can be made to fit via tailoring
                    OVERSIZE - Can always be worn no matter encumberance/mutations/bionics/etc
                    HOOD - Will increase warmth for head if head is cold and player is not wearing a helmet (headwear of material that is not wool or cotton)
                    POCKETS - Will increase warmth for hands if hands are cold and the player is wielding nothing
                    WATCH - Shows the current time, instead of sun/moon position
                    ALARMCLOCK - Has an alarmclock feature
                    MALE_TYPICAL - Typically only worn by men.
                    FEMALE_TYPICAL - Typically only worn by women.
                    USE_EAT_VERB - Use the eat verb, even if it's a liquid(soup, jam etc.)

                    Container-only flags:
                    SEALS
                    RIGID
                    WATERTIGHT
                    */
                }

                new_item_template->techniques = (!entry.has("techniques") ? 0 :
                                                 flags_from_json(entry.get("techniques"), "techniques"));
                new_item_template->use = (!entry.has("use_action") ? &iuse::none :
                                          use_from_string(entry.get("use_action").as_string()));
            }
        }
    }
    if(!json_good())
        throw "There was an error reading " + file_name;
}

// Load values from this data file into m_template_groups
void Item_factory::load_item_groups_from( game *g, const std::string file_name ) throw ( std::string ) {
    std::ifstream data_file;
    picojson::value input_value;

    data_file.open(file_name.c_str());

    if(! data_file.good()) {
        throw "Could not read " + file_name;
    }

    data_file >> input_value;
    data_file.close();

    if(! json_good()) {
        throw "The data in " + file_name + " is not an array";
    }
    if (! input_value.is<picojson::array>()) {
        throw file_name + "is not an array of item groups";
    }

    //Crawl through once and create an entry for every definition
    const picojson::array& all_items = input_value.get<picojson::array>();
    for (picojson::array::const_iterator entry = all_items.begin();
         entry != all_items.end(); ++entry) {
        // TODO: Make sure we have at least an item or group child, as otherwise
        //       later things will bug out.

        if( !(entry->is<picojson::object>()) ){
            debugmsg("Invalid group definition, entry not a JSON object");
            continue;
        }
        const picojson::value::object& entry_body = entry->get<picojson::object>();

        // The one element we absolutely require for an item definition is an id
        picojson::value::object::const_iterator key_pair = entry_body.find( "id" );
        if( key_pair == entry_body.end() || !(key_pair->second.is<std::string>()) ) {
            debugmsg("Group definition skipped, no id found or id was malformed.");
            continue;
        }

        Item_tag group_id = key_pair->second.get<std::string>();
        m_template_groups[group_id] = new Item_group(group_id);
    }
    //Once all the group definitions are set, fill them out
    for (picojson::array::const_iterator entry = all_items.begin();
         entry != all_items.end(); ++entry) {
        const picojson::value::object& entry_body = entry->get<picojson::object>();

        Item_tag group_id = entry_body.find("id")->second.get<std::string>();
        Item_group *current_group = m_template_groups.find(group_id)->second;

        //Add items
        picojson::value::object::const_iterator key_pair = entry_body.find("items");
        if( key_pair != entry_body.end() ) {
            if( !(key_pair->second.is<picojson::array>()) ) {
                debugmsg("Invalid item list for group definition '%s', list of items not an array.",
                         group_id.c_str());
                // We matched the find above, so we're NOT a group.
                continue;
            }
            //We have confirmed that we have a list of SOMETHING, now let's add them one at a time.
            const picojson::array& items_to_add = key_pair->second.get<picojson::array>();
            for (picojson::array::const_iterator item_pair = items_to_add.begin();
                 item_pair != items_to_add.end(); ++item_pair) {
                // Before adding, make sure this element is in the right format,
                // namely ["TAG_NAME", frequency number]
                if( !(item_pair->is<picojson::array>()) ) {
                    debugmsg("Invalid item list for group definition '%s', element is not an array.",
                             group_id.c_str());
                    continue;
                }
                if( item_pair->get<picojson::array>().size() != 2 ) {
                    debugmsg( "Invalid item list for group definition '%s', element does not have 2 values.",
                              group_id.c_str() );
                    continue;
                }
                picojson::array item_frequency_array = item_pair->get<picojson::array>();
                // Insure that the first value is a string, and the second is a number
                if( !item_frequency_array[0].is<std::string>() ||
                    !item_frequency_array[1].is<double>() ) {
                    debugmsg("Invalid item list for group definition '%s', element is not a valid tag/frequency pair.",
                             group_id.c_str());
                    continue;
                }
                if( m_missing_item == find_template( item_frequency_array[0].get<std::string>() ) &&
                    0 == g->itypes.count( item_frequency_array[0].get<std::string>() ) ) {
                    debugmsg( "Item '%s' listed in group '%s' does not exist.",
                              item_frequency_array[0].get<std::string>().c_str(), group_id.c_str() );
                    continue;
                }
                current_group->add_entry(item_frequency_array[0].get<std::string>(),
                                         (int)item_frequency_array[1].get<double>());
            }
        }

        //Add groups
        key_pair = entry_body.find("groups");
        if(key_pair != entry_body.end()){
            if( !(key_pair->second.is<picojson::array>()) ){
                debugmsg("Invalid group list for group definition '%s', list of items not an array.",
                         group_id.c_str());
                continue;
            }
            //We have confirmed that we have a list of SOMETHING, now let's add them one at a time.
            const picojson::array& items_to_add = key_pair->second.get<picojson::array>();
            for ( picojson::array::const_iterator item_pair = items_to_add.begin();
                  item_pair != items_to_add.end(); ++item_pair ) {
                //Before adding, make sure this element is in the right format, namely ["TAG_NAME", frequency number]
                if( !(item_pair->is<picojson::array>()) ) {
                    debugmsg("Invalid group list for group definition '%s', element is not an array.",
                             group_id.c_str());
                    continue;
                }
                if( item_pair->get<picojson::array>().size() != 2 ) {
                    debugmsg("Invalid group list for group definition '%s', element does not have 2 values.",
                             group_id.c_str());
                    continue;
                }
                picojson::array item_frequency_array = item_pair->get<picojson::array>();
                //Finally, insure that the first value is a string, and the second is a number
                if(!item_frequency_array[0].is<std::string>() ||
                   !item_frequency_array[1].is<double>() ){
                    debugmsg("Invalid group list for group definition '%s', element is not a valid tag/frequency pair.",
                             group_id.c_str());
                    continue;
                }
                Item_group* subgroup = m_template_groups.find(item_frequency_array[0].get<std::string>())->second;
                current_group->add_group( subgroup, (int)item_frequency_array[1].get<double>() );
            }
        }
    }
    if( !json_good() ) {
        throw "There was an error reading " + file_name;
    }
}

//Grab color, with appropriate error handling
nc_color Item_factory::color_from_string(std::string new_color){
    if("red"==new_color){
        return c_red;
    } else if("blue"==new_color){
        return c_blue;
    } else if("green"==new_color){
        return c_green;
    } else if("light_cyan"==new_color){
        return c_ltcyan;
    } else if("brown"==new_color){
        return c_brown;
    } else if("light_red"==new_color){
        return c_ltred;
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
        debugmsg("Received invalid color property %s. Color is required.", new_color.c_str());
        return c_white;
    }
}

Use_function Item_factory::use_from_string(std::string function_name){
    std::map<Item_tag, Use_function>::iterator found_function = iuse_function_list.find(function_name);

    //Before returning, make sure sure the function actually exists
    if(found_function != iuse_function_list.end()){
        return found_function->second;
    } else {
        //Otherwise, return a hardcoded function we know exists (hopefully)
        debugmsg("Received unrecognized iuse function %s, using iuse::none instead", function_name.c_str());
        return &iuse::none;
    }
}

void Item_factory::set_flag_by_string(unsigned& cur_flags, std::string new_flag, std::string flag_type)
{
    std::map<Item_tag, unsigned> flag_map;
    if(flag_type=="techniques"){
      flag_map = techniques_list;
    } else if(flag_type=="bodyparts"){
        flag_map = bodyparts_list;
    }

    set_bitmask_by_string(flag_map, cur_flags, new_flag);
}

void Item_factory::set_bitmask_by_string(std::map<Item_tag, unsigned> flag_map, unsigned& cur_bitmask, std::string new_flag)
{
    std::map<Item_tag, unsigned>::const_iterator found_flag_iter = flag_map.find(new_flag);
    if(found_flag_iter != flag_map.end())
    {
        cur_bitmask = cur_bitmask | found_flag_iter->second;
    }
    else
    {
        debugmsg("Invalid item flag (etc.): %s", new_flag.c_str());
    }
}

void Item_factory::tags_from_json(catajson tag_list, std::set<std::string> &tags)
{
    if (tag_list.is_array())
    {
        for (tag_list.set_begin(); tag_list.has_curr(); tag_list.next())
        {
            tags.insert( tag_list.curr().as_string() );
        }
    }
    else
    {
        //we should have gotten a string, if not an array, and catajson will do error checking
        tags.insert( tag_list.as_string() );
    }
}

unsigned Item_factory::flags_from_json(catajson flag_list, std::string flag_type){
    //If none is found, just use the standard none action
    unsigned flag = 0;
    //Otherwise, grab the right label to look for
    if (flag_list.is_array())
    {
        for (flag_list.set_begin(); flag_list.has_curr(); flag_list.next())
        {
            set_flag_by_string(flag, flag_list.curr().as_string(), flag_type);
        }
    }
    else
    {
        //we should have gotten a string, if not an array, and catajson will do error checking
        set_flag_by_string(flag, flag_list.as_string(), flag_type);
    }
    return flag;
}

bool Item_factory::is_mod_target(catajson targets, std::string weapon){
    //If none is found, just use the standard none action
    unsigned is_included = false;
    //Otherwise, grab the right label to look for
    if (targets.is_array())
    {
        for (targets.set_begin(); targets.has_curr() && !is_included; targets.next())
        {
            if(targets.curr().as_string() == weapon){
              is_included=true;
            }
        }
    }
    else
    {
        if(targets.as_string() == weapon){
          is_included=true;
        }
    }
    return is_included;
}

void Item_factory::set_material_from_json(Item_tag new_id, catajson mat_list){
    //If the value isn't found, just return a group of null materials
    std::string material_list[2] = {"null", "null"};

    if (mat_list.is_array())
    {
        if (mat_list.has(2))
        {
            debugmsg("Too many materials provided for item %s", new_id.c_str());
        }
        material_list[0] = mat_list.get(0).as_string();
        material_list[1] = mat_list.get(1).as_string();
    }
    else
    {
        material_list[0] = mat_list.as_string();
    }

    itype* temp = find_template(new_id);
    temp->m1 = material_list[0];
    temp->m2 = material_list[1];
}


phase_id Item_factory::phase_from_tag(Item_tag name){
    if(name == "liquid"){
        return LIQUID;
    } else if(name == "solid"){
        return SOLID;
    } else if(name == "gas"){
        return GAS;
    } else if(name == "plasma"){
        return PLASMA;
    } else {
        return PNULL;
    }
};
