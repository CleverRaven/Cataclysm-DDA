#include "item_factory.h"
#include "rng.h"
#include "enums.h"
#include "json.h"
#include "addiction.h"
#include "translations.h"
#include "bodypart.h"
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
    // intentionally left untranslated
    // because using _() at global scope is problematic,
    // and if this appears it's a bug anyway.
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
    iuse_function_list["DISINFECTANT"] = &iuse::disinfectant;
    iuse_function_list["CAFF"] = &iuse::caff;
    iuse_function_list["ATOMIC_CAFF"] = &iuse::atomic_caff;
    iuse_function_list["ALCOHOL"] = &iuse::alcohol;
    iuse_function_list["ALCOHOL_WEAK"] = &iuse::alcohol_weak;
    iuse_function_list["PKILL"] = &iuse::pkill;
    iuse_function_list["XANAX"] = &iuse::xanax;
    iuse_function_list["CIG"] = &iuse::cig;
    iuse_function_list["ANTIBIOTIC"] = &iuse::antibiotic;
    iuse_function_list["FUNGICIDE"] = &iuse::fungicide;
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
    iuse_function_list["CHEW"] = &iuse::chew;
    iuse_function_list["MUTAGEN"] = &iuse::mutagen;
    iuse_function_list["PURIFIER"] = &iuse::purifier;
    iuse_function_list["MARLOSS"] = &iuse::marloss;
    iuse_function_list["DOGFOOD"] = &iuse::dogfood;
    iuse_function_list["CATFOOD"] = &iuse::catfood;

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
    iuse_function_list["SOLDER_WELD"] = &iuse::solder_weld;
    iuse_function_list["WATER_PURIFIER"] = &iuse::water_purifier;
    iuse_function_list["TWO_WAY_RADIO"] = &iuse::two_way_radio;
    iuse_function_list["RADIO_OFF"] = &iuse::radio_off;
    iuse_function_list["RADIO_ON"] = &iuse::radio_on;
    iuse_function_list["HORN_BICYCLE"] = &iuse::horn_bicycle;
    iuse_function_list["NOISE_EMITTER_OFF"] = &iuse::noise_emitter_off;
    iuse_function_list["NOISE_EMITTER_ON"] = &iuse::noise_emitter_on;
    iuse_function_list["ROADMAP"] = &iuse::roadmap;
    iuse_function_list["SURVIVORMAP"] = &iuse::survivormap;
    iuse_function_list["MILITARYMAP"] = &iuse::militarymap;
    iuse_function_list["RESTAURANTMAP"] = &iuse::restaurantmap;
    iuse_function_list["TOURISTMAP"] = &iuse::touristmap;
    iuse_function_list["PICKLOCK"] = &iuse::picklock;
    iuse_function_list["CROWBAR"] = &iuse::crowbar;
    iuse_function_list["MAKEMOUND"] = &iuse::makemound;
    iuse_function_list["DIG"] = &iuse::dig;
    iuse_function_list["SIPHON"] = &iuse::siphon;
    iuse_function_list["CHAINSAW_OFF"] = &iuse::chainsaw_off;
    iuse_function_list["CHAINSAW_ON"] = &iuse::chainsaw_on;
    iuse_function_list["CARVER_OFF"] = &iuse::carver_off;
    iuse_function_list["CARVER_ON"] = &iuse::carver_on;
    iuse_function_list["COMBATSAW_OFF"] = &iuse::combatsaw_off;
    iuse_function_list["COMBATSAW_ON"] = &iuse::combatsaw_on;
    iuse_function_list["SHISHKEBAB_OFF"] = &iuse::shishkebab_off;
    iuse_function_list["SHISHKEBAB_ON"] = &iuse::shishkebab_on;
    iuse_function_list["FIREMACHETE_OFF"] = &iuse::firemachete_off;
    iuse_function_list["FIREMACHETE_ON"] = &iuse::firemachete_on;
    iuse_function_list["BROADFIRE_OFF"] = &iuse::broadfire_off;
    iuse_function_list["BROADFIRE_ON"] = &iuse::broadfire_on;
    iuse_function_list["FIREKATANA_OFF"] = &iuse::firekatana_off;
    iuse_function_list["FIREKATANA_ON"] = &iuse::firekatana_on;
    iuse_function_list["ZWEIFIRE_OFF"] = &iuse::zweifire_off;
    iuse_function_list["ZWEIFIRE_ON"] = &iuse::zweifire_on;
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
    iuse_function_list["GRANADE"] = &iuse::granade;
    iuse_function_list["GRANADE_ACT"] = &iuse::granade_act;
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
    iuse_function_list["MATCHBOMB"] = &iuse::matchbomb;
    iuse_function_list["MATCHBOMB_ACT"] = &iuse::matchbomb_act;
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
    iuse_function_list["TURRET_LASER"] = &iuse::turret_laser;
    iuse_function_list["UPS_OFF"] = &iuse::UPS_off;
    iuse_function_list["UPS_ON"] = &iuse::UPS_on;
    iuse_function_list["adv_UPS_OFF"] = &iuse::adv_UPS_off;
    iuse_function_list["adv_UPS_ON"] = &iuse::adv_UPS_on;
    iuse_function_list["TAZER"] = &iuse::tazer;
    iuse_function_list["TAZER2"] = &iuse::tazer2;
    iuse_function_list["SHOCKTONFA_OFF"] = &iuse::shocktonfa_off;
    iuse_function_list["SHOCKTONFA_ON"] = &iuse::shocktonfa_on;
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
    iuse_function_list["HANDFLARE"] = &iuse::handflare;
    iuse_function_list["HANDFLARE_LIT"] = &iuse::handflare_lit;
    iuse_function_list["BATTLETORCH"] = &iuse::battletorch;
    iuse_function_list["BATTLETORCH_LIT"] = &iuse::battletorch_lit;
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
    iuse_function_list["JET_INJECTOR"] = &iuse::jet_injector;
    iuse_function_list["CONTACTS"] = &iuse::contacts;
    iuse_function_list["AIRHORN"] = &iuse::airhorn;
    iuse_function_list["HOTPLATE"] = &iuse::hotplate;
    iuse_function_list["DOLLCHAT"] = &iuse::talking_doll;
    // MACGUFFINS
    iuse_function_list["MCG_NOTE"] = &iuse::mcg_note;
    // ARTIFACTS
    // This function is used when an artifact is activated
    // It examines the item's artifact-specific properties
    // See artifact.h for a list
    iuse_function_list["ARTIFACT"] = &iuse::artifact;

    // Offensive Techniques
    techniques_list["PRECISE"] = "tec_precise";
    techniques_list["RAPID"] = "tec_rapid";
}

//Will eventually be deprecated - Loads existing item format into the item factory, and vice versa
void Item_factory::init_old() {
    // Make a copy of our items loaded from JSON
    std::map<Item_tag, itype*> new_templates = m_templates;
    //Copy the hardcoded template pointers to the factory list
    m_templates.insert(itypes.begin(), itypes.end());
    //Copy the JSON-derived items to the legacy list
    itypes.insert(new_templates.begin(), new_templates.end());
    //And add them to the various item lists, as needed.
    for(std::map<Item_tag, itype*>::iterator iter = new_templates.begin(); iter != new_templates.end(); ++iter) {
      standard_itype_ids.push_back(iter->first);
    }
}

inline int ammo_type_defined(const std::string &ammo) {
    if(ammo == "NULL" || ammo == "generic_no_ammo") {
        return 1; // Known ammo type
    }
    if(ammo_name(ammo) != "XXX") {
        return 1; // Known ammo type
    }
    if(!item_controller->has_template(ammo)) {
        return 0; // Unknown ammo type
    }
    return 2; // Unknown from ammo_name, but defined as itype
}

void Item_factory::check_ammo_type(std::ostream& msg, const std::string &ammo) const {
    if(ammo == "NULL" || ammo == "generic_no_ammo") {
        return;
    }
    if(ammo_name(ammo) == "XXX") {
        msg << string_format("ammo type %s not listed in ammo_name() function", ammo.c_str()) << "\n";
    }
    if(ammo == "UPS") {
        return;
    }
    for(std::map<Item_tag, itype*>::const_iterator it = m_templates.begin(); it != m_templates.end(); ++it) {
        const it_ammo* ammot = dynamic_cast<const it_ammo*>(it->second);
        if(ammot == 0) {
            continue;
        }
        if(ammot->type == ammo) {
            return;
        }
    }
    msg << string_format("there is no actuall ammo of type %s defined", ammo.c_str()) << "\n";
}

void Item_factory::check_itype_definitions() const {
    std::ostringstream main_stream;
    for(std::map<Item_tag, itype*>::const_iterator it = m_templates.begin(); it != m_templates.end(); ++it) {
        std::ostringstream msg;
        const itype* type = it->second;
        if(type->m1 != "null" && !material_type::has_material(type->m1)) {
            msg << string_format("invalid material %s", type->m1.c_str()) << "\n";
        }
        if(type->m2 != "null" && !material_type::has_material(type->m2)) {
            msg << string_format("invalid material %s", type->m2.c_str()) << "\n";
        }
        const it_comest* comest = dynamic_cast<const it_comest*>(type);
        if(comest != 0) {
            if(comest->container != "null" && !has_template(comest->container)) {
                msg << string_format("invalid container property %s", comest->container.c_str()) << "\n";
            }
            if(comest->container != "null" && !has_template(comest->tool)) {
                msg << string_format("invalid tool property %s", comest->tool.c_str()) << "\n";
            }
        }
        const it_ammo* ammo = dynamic_cast<const it_ammo*>(type);
        if(ammo != 0) {
            check_ammo_type(msg, ammo->type);
            if(ammo->casing != "NULL" && !has_template(ammo->casing)) {
                msg << string_format("invalid casing property %s", ammo->casing.c_str()) << "\n";
            }
        }
        const it_gun* gun = dynamic_cast<const it_gun*>(type);
        if(gun != 0) {
            check_ammo_type(msg, gun->ammo);
            if(gun->skill_used == 0) {
                msg << string_format("uses no skill") << "\n";
            }
        }
        const it_gunmod* gunmod = dynamic_cast<const it_gunmod*>(type);
        if(gunmod != 0) {
            check_ammo_type(msg, gunmod->newtype);
        }
        const it_tool* tool = dynamic_cast<const it_tool*>(type);
        if(tool != 0) {
            if(tool->max_charges != 0 || tool->def_charges != 0) {
                check_ammo_type(msg, tool->ammo);
            }
            if(tool->revert_to != "null" && !has_template(tool->revert_to)) {
                msg << string_format("invalid revert_to property %s", tool->revert_to.c_str()) << "\n";
            }
        }
        if(msg.str().empty()) {
            continue;
        }
        main_stream << "warnings for type " << type->id << ":\n" << msg.str() << "\n";
        const std::string &buffer = main_stream.str();
        const size_t lines = std::count(buffer.begin(), buffer.end(), '\n');
        if(lines > 10) {
            fold_and_print(stdscr, 0, 0, getmaxx(stdscr), c_red, "%s\n  Press any key...", buffer.c_str());
            getch();
            werase(stdscr);
            main_stream.str(std::string());
        }
    }
    const std::string &buffer = main_stream.str();
    if(!buffer.empty()) {
        fold_and_print(stdscr, 0, 0, getmaxx(stdscr), c_red, "%s\n  Press any key...", buffer.c_str());
        getch();
        werase(stdscr);
    }
}

void Item_factory::check_items_of_groups_exist() const {
    for(std::map<Item_tag, Item_group*>::const_iterator a = m_template_groups.begin(); a != m_template_groups.end(); a++) {
        a->second->check_items_exist();
    }
}

bool Item_factory::has_template(Item_tag id) const {
    return m_templates.count(id) > 0;
}

//Returns the template with the given identification tag
itype* Item_factory::find_template(const Item_tag id){
    std::map<Item_tag, itype*>::iterator found = m_templates.find(id);
    if(found != m_templates.end()){
        return found->second;
    }
    else{
        debugmsg("Missing item (check item_groups.json): %s", id.c_str());
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

void Item_factory::load_ammo(JsonObject& jo)
{
    it_ammo* ammo_template = new it_ammo();
    ammo_template->type = jo.get_string("ammo_type");
    ammo_template->casing = jo.get_string("casing", "NULL");
    ammo_template->damage = jo.get_int("damage");
    ammo_template->pierce = jo.get_int("pierce");
    ammo_template->range = jo.get_int("range");
    ammo_template->dispersion = jo.get_int("dispersion");
    ammo_template->recoil = jo.get_int("recoil");
    ammo_template->count = jo.get_int("count");
    ammo_template->stack_size = jo.get_int("stack_size", ammo_template->count);
    ammo_template->ammo_effects = jo.get_tags("effects");

    itype *new_item_template = ammo_template;
    load_basic_info(jo, new_item_template);
}

void Item_factory::load_gun(JsonObject& jo)
{
    it_gun* gun_template = new it_gun();
    gun_template->ammo = jo.get_string("ammo");
    gun_template->skill_used = Skill::skill(jo.get_string("skill"));
    gun_template->dmg_bonus = jo.get_int("ranged_damage");
    gun_template->range = jo.get_int("range");
    gun_template->dispersion = jo.get_int("dispersion");
    gun_template->recoil = jo.get_int("recoil");
    gun_template->durability = jo.get_int("durability");
    gun_template->burst = jo.get_int("burst");
    gun_template->clip = jo.get_int("clip_size");
    gun_template->reload_time = jo.get_int("reload");
    gun_template->pierce = jo.get_int("pierce", 0);
    gun_template->ammo_effects = jo.get_tags("ammo_effects");

    itype *new_item_template = gun_template;
    load_basic_info(jo, new_item_template);
}

void Item_factory::load_armor(JsonObject& jo)
{
    it_armor* armor_template = new it_armor();

    armor_template->encumber = jo.get_int("encumbrance");
    armor_template->coverage = jo.get_int("coverage");
    armor_template->thickness = jo.get_int("material_thickness");
    armor_template->env_resist = jo.get_int("enviromental_protection");
    armor_template->warmth = jo.get_int("warmth");
    armor_template->storage = jo.get_int("storage");
    armor_template->power_armor = jo.get_bool("power_armor", false);
    armor_template->covers = jo.has_member("covers") ?
        flags_from_json(jo, "covers", "bodyparts") : 0;

    itype *new_item_template = armor_template;
    load_basic_info(jo, new_item_template);
}

void Item_factory::load_tool(JsonObject& jo)
{
    it_tool* tool_template = new it_tool();
    tool_template->ammo = jo.get_string("ammo");
    tool_template->max_charges = jo.get_int("max_charges");
    tool_template->def_charges = jo.get_int("initial_charges");
    tool_template->charges_per_use = jo.get_int("charges_per_use");
    tool_template->turns_per_charge = jo.get_int("turns_per_charge");
    tool_template->revert_to = jo.get_string("revert_to");

    itype *new_item_template = tool_template;
    load_basic_info(jo, new_item_template);
}

void Item_factory::load_book(JsonObject& jo)
{
    it_book* book_template = new it_book();

    book_template->level = jo.get_int("max_level");
    book_template->req = jo.get_int("required_level");
    book_template->fun = jo.get_int("fun");
    book_template->intel = jo.get_int("intelligence");
    book_template->time = jo.get_int("time");
    book_template->type = Skill::skill(jo.get_string("skill"));

    book_template->chapters = jo.get_int("chapters", -1);

    itype *new_item_template = book_template;
    load_basic_info(jo, new_item_template);
}

void Item_factory::load_comestible(JsonObject& jo)
{
    it_comest* comest_template = new it_comest();
    comest_template->comesttype = jo.get_string("comestible_type");
    comest_template->tool = jo.get_string("tool", "null");
    comest_template->container = jo.get_string("container", "null");
    comest_template->quench = jo.get_int("quench", 0);
    comest_template->nutr = jo.get_int("nutrition", 0);
    comest_template->spoils = jo.get_int("spoils_in", 0);
    comest_template->addict = jo.get_int("addiction_potential", 0);
    comest_template->charges = jo.get_int("charges", 0);
    if(jo.has_member("stack_size")) {
      comest_template->stack_size = jo.get_int("stack_size");
    } else {
      comest_template->stack_size = comest_template->charges;
    }
    comest_template->stim = jo.get_int("stim", 0);
    comest_template->healthy = jo.get_int("heal", 0);
    comest_template->fun = jo.get_int("fun", 0);
    comest_template->add = addiction_type(jo.get_string("addiction_type"));

    itype *new_item_template = comest_template;
    load_basic_info(jo, new_item_template);
}

void Item_factory::load_container(JsonObject &jo)
{
    it_container* container_template = new it_container();

    container_template->contains = jo.get_int("contains");

    itype *new_item_template = container_template;
    load_basic_info(jo, new_item_template);
}

void Item_factory::load_gunmod(JsonObject& jo)
{
    it_gunmod* gunmod_template = new it_gunmod();
    gunmod_template->damage = jo.get_int("damage_modifier", 0);
    gunmod_template->loudness = jo.get_int("loudness_modifier", 0);
    gunmod_template->newtype = jo.get_string("ammo_modifier");
    gunmod_template->used_on_pistol = is_mod_target(jo, "mod_targets", "pistol");
    gunmod_template->used_on_shotgun = is_mod_target(jo, "mod_targets", "shotgun");
    gunmod_template->used_on_smg = is_mod_target(jo, "mod_targets", "smg");
    gunmod_template->used_on_rifle = is_mod_target(jo, "mod_targets", "rifle");
    gunmod_template->dispersion = jo.get_int("dispersion_modifier", 0);
    gunmod_template->recoil = jo.get_int("recoil_modifier", 0);
    gunmod_template->burst = jo.get_int("burst_modifier", 0);
    gunmod_template->clip = jo.get_int("clip_size_modifier", 0);
    gunmod_template->acceptible_ammo_types = jo.get_tags("acceptable_ammo");

    itype *new_item_template = gunmod_template;
    load_basic_info(jo, new_item_template);
}

void Item_factory::load_generic(JsonObject& jo)
{
    itype *new_item_template = new itype();
    load_basic_info(jo, new_item_template);
}

void Item_factory::load_basic_info(JsonObject& jo, itype* new_item_template)
{
    std::string new_id = jo.get_string("id");
    new_item_template->id = new_id;
    m_templates[new_id] = new_item_template;

    // And then proceed to assign the correct field
    new_item_template->price = jo.get_int("price");
    new_item_template->name = _(jo.get_string("name").c_str());
    new_item_template->sym = jo.get_string("symbol")[0];
    new_item_template->color = color_from_string(jo.get_string("color"));
    new_item_template->description = _(jo.get_string("description").c_str());
    if(jo.has_member("material")){
      set_material_from_json(jo, "material", new_item_template);
    } else {
      new_item_template->m1 = "null";
      new_item_template->m2 = "null";
    }
    Item_tag new_phase = "solid";
    if(jo.has_member("phase")){
        new_phase = jo.get_string("phase");
    }
    new_item_template->phase = phase_from_tag(new_phase);
    new_item_template->volume = jo.get_int("volume");
    new_item_template->weight = jo.get_int("weight");
    new_item_template->melee_dam = jo.get_int("bashing");
    new_item_template->melee_cut = jo.get_int("cutting");
    new_item_template->m_to_hit = jo.get_int("to_hit");

    new_item_template->light_emission = 0;

    /*
    List of current flags
    FIT - Reduces encumbrance by one
    SKINTIGHT - Reduces layer penalty
    VARSIZE - Can be made to fit via tailoring
    OVERSIZE - Can always be worn no matter encumbrance/mutations/bionics/etc
    POCKETS - Will increase warmth for hands if hands are cold and the player is wielding nothing
    HOOD - Will increase warmth for head if head is cold and player's head isn't encumbered
    RAINPROOF - Works like a raincoat to protect from rain effects
    WATCH - Shows the current time, instead of sun/moon position
    ALARMCLOCK - Has an alarmclock feature
    FANCY - Less than practical clothing meant primarily to convey a certain image.
    SUPER_FANCY - Clothing suitable for the most posh of events.
    LIGHT_* - light emission, sets cached int light_emission
    USE_EAT_VERB - Use the eat verb, even if it's a liquid(soup, jam etc.)
    STURDY - Clothing is made to be armor. Prevents damage to armor unless it is penetrated.

    Container-only flags:
    SEALS
    RIGID
    WATERTIGHT
    */
    new_item_template->item_tags = jo.get_tags("flags");
    if ( new_item_template->item_tags.size() > 0 ) {
        for( std::set<std::string>::const_iterator it = new_item_template->item_tags.begin();
        it != new_item_template->item_tags.end(); ++it ) {
            set_intvar(std::string(*it), new_item_template->light_emission, 1, 10000);
        }
    }

    if( jo.has_member("qualities") ){
        set_qualities_from_json(jo, "qualities", new_item_template);
    }

    new_item_template->techniques = jo.get_tags("techniques");

    new_item_template->use = (!jo.has_member("use_action") ? &iuse::none :
                              use_from_string(jo.get_string("use_action")));
}

void Item_factory::set_qualities_from_json(JsonObject& jo, std::string member, itype* new_item_template)
{

    if ( jo.has_array(member) ) {
        JsonArray jarr = jo.get_array(member);
        while (jarr.has_more()){
            JsonArray curr = jarr.next_array();
            new_item_template->qualities.insert(std::pair<std::string, int>(curr.get_string(0), curr.get_int(1)));
        }
    } else {
        debugmsg("Qualities list for item %s not an array", new_item_template->id.c_str());
    }
}

unsigned Item_factory::flags_from_json(JsonObject& jo, std::string member, std::string flag_type)
{
    //If none is found, just use the standard none action
    unsigned flag = 0;
    //Otherwise, grab the right label to look for
    if ( jo.has_array(member) ) {
        JsonArray jarr = jo.get_array(member);
        while (jarr.has_more()){
            set_flag_by_string(flag, jarr.next_string(), flag_type);
        }
    } else if ( jo.has_string(member) ) {
        //we should have gotten a string, if not an array
        set_flag_by_string(flag, jo.get_string(member), flag_type);
    }

    return flag;
}

void Item_factory::set_material_from_json(JsonObject& jo, std::string member, itype* new_item_template)
{
    //If the value isn't found, just return a group of null materials
    std::string material_list[2] = {"null", "null"};
    if( jo.has_array(member) ) {
        JsonArray jarr = jo.get_array(member);
        if (jarr.size() > 2) {
            debugmsg("Too many materials provided for item %s", new_item_template->id.c_str());
        }
        material_list[0] = jarr.get_string(0);
        material_list[1] = jarr.get_string(1);
    } else if ( jo.has_string(member) ) {
        material_list[0] = jo.get_string(member);
    }
    new_item_template->m1 = material_list[0];
    new_item_template->m2 = material_list[1];
}

bool Item_factory::is_mod_target(JsonObject& jo, std::string member, std::string weapon)
{
    //If none is found, just use the standard none action
    unsigned is_included = false;
    //Otherwise, grab the right label to look for
    if ( jo.has_array(member) ) {
        JsonArray jarr = jo.get_array(member);
        while (jarr.has_more() && is_included == false){
            if (jarr.next_string() == weapon){
                is_included = true;
            }
        }
    } else {
        if (jo.get_string(member) == weapon){
            is_included = true;
        }
    }
    return is_included;
}


// Load an item group from JSON
void Item_factory::load_item_group(JsonObject &jsobj)
{
    Item_tag group_id = jsobj.get_string("id");
    Item_group *current_group = new Item_group(group_id);
    m_template_groups[group_id] = current_group;

    JsonArray items = jsobj.get_array("items");
    while (items.has_more()) {
        JsonArray pair = items.next_array();
        current_group->add_entry(pair.get_string(0), pair.get_int(1));
    }

    JsonArray groups = jsobj.get_array("groups");
    while (groups.has_more()) {
        JsonArray pair = groups.next_array();
        std::string name = pair.get_string(0);
        int frequency = pair.get_int(1);
        // we had better have loaded it already!
        if (m_template_groups.find(name) == m_template_groups.end()) {
            throw jsobj.line_number() + ": unrecognized group name: " + name;
        }
        current_group->add_group(m_template_groups[name], frequency);
    }
}

use_function Item_factory::use_from_string(std::string function_name){
    std::map<Item_tag, use_function>::iterator found_function = iuse_function_list.find(function_name);

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
    std::map<Item_tag, unsigned> str2int_map;
    if(flag_type=="bodyparts"){
      str2int_map = body_parts;
      set_bitmask_by_string(str2int_map, cur_flags, new_flag);
    }

}

void Item_factory::set_bitmask_by_string(std::map<Item_tag, unsigned> flag_map, unsigned& cur_bitmask, std::string new_flag)
{
    std::map<Item_tag, unsigned>::const_iterator found_flag_iter = flag_map.find(new_flag);
    if(found_flag_iter != flag_map.end())
    {
        cur_bitmask = cur_bitmask | mfb(found_flag_iter->second);
    }
    else
    {
        debugmsg("Invalid item flag (etc.): %s", new_flag.c_str());
    }
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

void Item_factory::set_intvar(std::string tag, unsigned int & var, int min, int max) {
    if( tag.size() > 6 && tag.substr(0,6) == "LIGHT_" ) {
        int candidate=atoi(tag.substr(6).c_str());
        if ( candidate >= min && candidate <= max ) {
            var=candidate;
        }
    }
}
