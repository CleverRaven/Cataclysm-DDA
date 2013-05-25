#include "item_factory.h"
#include "rng.h"
#include "enums.h"
#include "catajson.h"
#include "addiction.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <stdio.h>

// mfb(n) converts a flag to its appropriate position in covers's bitfield
#ifndef mfb
#define mfb(n) long(1 << (n))
#endif

Item_factory* item_controller = new Item_factory();

//Every item factory comes with a missing item
Item_factory::Item_factory(){
    init();
    m_missing_item = new itype();
    m_missing_item->name = "Error: Item Missing.";
    m_missing_item->description = "There is only the space where an object should be, but isn't. No item template of this type exists.";
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
    iuse_function_list["DEVAC"] = &iuse::devac;
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
    iuse_function_list["SIPHON"] = &iuse::siphon;
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

    //Ammo lists
    ammo_flags_list["NULL"] = mfb(AT_NULL);
    ammo_flags_list["THREAD"] = mfb(AT_THREAD);
    ammo_flags_list["BATT"] = mfb(AT_BATT);
    ammo_flags_list["PLUT"] = mfb(AT_PLUT);
    ammo_flags_list["NAIL"] = mfb(AT_NAIL);
    ammo_flags_list["BB"] = mfb(AT_BB);
    ammo_flags_list["BOLT"] = mfb(AT_BOLT);
    ammo_flags_list["ARROW"] = mfb(AT_ARROW);
    ammo_flags_list["SHOT"] = mfb(AT_SHOT);
    ammo_flags_list["22"] = mfb(AT_22);
    ammo_flags_list["9MM"] = mfb(AT_9MM);
    ammo_flags_list["762x25"] = mfb(AT_762x25);
    ammo_flags_list["38"] = mfb(AT_38);
    ammo_flags_list["40"] = mfb(AT_40);
    ammo_flags_list["44"] = mfb(AT_44);
    ammo_flags_list["45"] = mfb(AT_45);
    ammo_flags_list["57"] = mfb(AT_57);
    ammo_flags_list["46"] = mfb(AT_46);
    ammo_flags_list["762"] = mfb(AT_762);
    ammo_flags_list["223"] = mfb(AT_223);
    ammo_flags_list["3006"] = mfb(AT_3006);
    ammo_flags_list["308"] = mfb(AT_308);
    ammo_flags_list["40MM"] = mfb(AT_40MM);
    ammo_flags_list["66MM"] = mfb(AT_66MM);
    ammo_flags_list["GAS"] = mfb(AT_GAS);
    ammo_flags_list["FUSION"] = mfb(AT_FUSION);
    ammo_flags_list["MUSCLE"] = mfb(AT_MUSCLE);
    ammo_flags_list["12MM"] = mfb(AT_12MM);
    ammo_flags_list["PLASMA"] = mfb(AT_PLASMA);
    ammo_flags_list["WATER"] = mfb(AT_WATER);
    ammo_flags_list["PEBBLE"] = mfb(AT_PEBBLE);

    ammo_effects_list["FLAME"] = mfb(AMMO_FLAME);
    ammo_effects_list["INCENDIARY"] = mfb(AMMO_INCENDIARY);
    ammo_effects_list["EXPLOSIVE"] = mfb(AMMO_EXPLOSIVE);
    ammo_effects_list["FRAG"] = mfb(AMMO_FRAG);
    ammo_effects_list["NAPALM"] = mfb(AMMO_NAPALM);
    ammo_effects_list["ACIDBOMB"] = mfb(AMMO_ACIDBOMB);
    ammo_effects_list["EXPLOSIVE_BIG"] = mfb(AMMO_EXPLOSIVE_BIG);
    ammo_effects_list["TEARGAS"] = mfb(AMMO_TEARGAS);
    ammo_effects_list["SMOKE"] = mfb(AMMO_SMOKE);
    ammo_effects_list["TRAIL"] = mfb(AMMO_TRAIL);
    ammo_effects_list["FLASHBANG"] = mfb(AMMO_FLASHBANG);
    ammo_effects_list["STREAM"] = mfb(AMMO_STREAM);
    ammo_effects_list["COOKOFF"] = mfb(AMMO_COOKOFF);
    ammo_effects_list["LASER"] = mfb(AMMO_LASER);

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
void Item_factory::init(game* main_game){
    load_item_templates(); // this one HAS to be called after game is created
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
    load_item_templates_from("data/raw/items/melee.json");
    load_item_templates_from("data/raw/items/ranged.json");
    load_item_templates_from("data/raw/items/ammo.json");
    load_item_templates_from("data/raw/items/mods.json");
    load_item_templates_from("data/raw/items/tools.json");
    load_item_templates_from("data/raw/items/comestibles.json");
    load_item_templates_from("data/raw/items/armor.json");
    load_item_templates_from("data/raw/items/books.json");
    load_item_groups_from("data/raw/item_groups.json");
}

// Load values from this data file into m_templates
void Item_factory::load_item_templates_from(const std::string file_name){
    catajson all_items(file_name);

    if (! all_items.is_array()) {
        debugmsg("%s is not an array of item_templates", file_name.c_str());
        exit(2);
    }

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
                        gunmod_template->newtype = ammo_from_string(entry.get("ammo_modifier").as_string());
                        gunmod_template->used_on_pistol = is_mod_target(entry.get("mod_targets"), "pistol");
                        gunmod_template->used_on_shotgun = is_mod_target(entry.get("mod_targets"), "shotgun");
                        gunmod_template->used_on_smg = is_mod_target(entry.get("mod_targets"), "smg");
                        gunmod_template->used_on_rifle = is_mod_target(entry.get("mod_targets"), "rifle");
                        gunmod_template->accuracy = entry.get("accuracy_modifier").as_int();
                        gunmod_template->recoil = entry.get("recoil_modifier").as_int();
                        gunmod_template->burst = entry.get("burst_modifier").as_int();
                        gunmod_template->clip = entry.get("clip_size_modifier").as_int();
                        gunmod_template->acceptible_ammo_types = (!entry.has("acceptable_ammo") ? 0 : flags_from_json(entry.get("acceptable_ammo"), "ammo"));
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
                        gun_template->ammo = ammo_from_string(entry.get("ammo").as_string());
                        gun_template->skill_used = Skill::skill(entry.get("skill").as_string());
                        gun_template->dmg_bonus = entry.get("ranged_damage").as_int();
                        gun_template->range = entry.get("range").as_int();
                        gun_template->accuracy = entry.get("accuracy").as_int();
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
                        tool_template->ammo = ammo_from_string(entry.get("ammo").as_string());
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
                        ammo_template->type =
                            ammo_from_string(
                                entry.get("ammo_type").as_string());
                        ammo_template->damage = entry.get("damage").as_int();
                        ammo_template->pierce = entry.get("pierce").as_int();
                        ammo_template->range = entry.get("range").as_int();
                        ammo_template->accuracy =
                            entry.get("accuracy").as_int();
                        ammo_template->recoil = entry.get("recoil").as_int();
                        ammo_template->count = entry.get("count").as_int();
                        ammo_template->ammo_effects =
                            (!entry.has("effects") ? 0 :
                             flags_from_json(entry.get("effects"),
                                             "ammo_effects"));

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
                new_item_template->rarity = entry.get("rarity").as_int();
                new_item_template->price = entry.get("price").as_int();
                new_item_template->name = entry.get("name").as_string();
                new_item_template->sym = entry.get("symbol").as_char();
                new_item_template->color = color_from_string(entry.get("color").as_string());
                new_item_template->description = entry.get("description").as_string();
                if(entry.has("material")){
                  set_material_from_json(new_id, entry.get("material"));
                } else {
                  new_item_template->m1 = MNULL;
                  new_item_template->m2 = MNULL;
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
                }

                new_item_template->techniques = (!entry.has("techniques") ? 0 :
                                                 flags_from_json(entry.get("techniques"), "techniques"));
                new_item_template->use = (!entry.has("use_action") ? &iuse::none :
                                          use_from_string(entry.get("use_action").as_string()));
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

ammotype Item_factory::ammo_from_string(std::string new_ammo){
    if("nail"==new_ammo){
        return AT_NAIL;
    } else if ("BB" == new_ammo) {
        return AT_BB;
    } else if ("pebble" == new_ammo) {
        return AT_PEBBLE;
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
    } else if ("none" == new_ammo) {
        return AT_NULL; // NX17 and other special weapons
    } else {
        debugmsg("Read invalid ammo %s.", new_ammo.c_str());
        return AT_NULL;
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
    if(flag_type=="ammo"){
      flag_map = ammo_flags_list;
    } else if(flag_type=="techniques"){
      flag_map = techniques_list;
    } else if(flag_type=="ammo_effects"){
        flag_map = ammo_effects_list;
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
    material material_list[2] = {MNULL, MNULL};

    if (mat_list.is_array())
    {
        if (mat_list.has(2))
        {
            debugmsg("Too many materials provided for item %s", new_id.c_str());
        }
        material_list[0] = material_from_tag(new_id, mat_list.get(0).as_string());
        material_list[1] = material_from_tag(new_id, mat_list.get(1).as_string());
    }
    else
    {
        material_list[0] = material_from_tag(new_id, mat_list.as_string());
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

//  TODO: depreciate once materials rewrite is up and running

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
    } else if(name == "CHITIN"){
        return CHITIN;        
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
    } else if(name == "NULL"){
        return MNULL;
    } else {
        return MNULL;
    }
}
