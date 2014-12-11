#include "item_factory.h"
#include "enums.h"
#include "json.h"
#include "addiction.h"
#include "translations.h"
#include "item_group.h"
#include "bodypart.h"
#include "crafting.h"
#include "iuse_actor.h"
#include "item.h"
#include "mapdata.h"
#include "debug.h"
#include "construction.h"
#include "text_snippets.h"
#include <algorithm>
#include <sstream>

static const std::string category_id_guns("guns");
static const std::string category_id_ammo("ammo");
static const std::string category_id_weapons("weapons");
static const std::string category_id_tools("tools");
static const std::string category_id_clothing("clothing");
static const std::string category_id_drugs("drugs");
static const std::string category_id_food("food");
static const std::string category_id_books("books");
static const std::string category_id_mods("mods");
static const std::string category_id_cbm("bionics");
static const std::string category_id_mutagen("mutagen");
static const std::string category_id_other("other");

std::unique_ptr<Item_factory> item_controller( new Item_factory() );

typedef std::set<std::string> t_string_set;
static t_string_set item_blacklist;
static t_string_set item_whitelist;

void remove_item(const std::string &itm, std::vector<map_bash_item_drop> &vec)
{
    for (size_t i = 0; i < vec.size(); i++) {
        if (vec[i].itemtype == itm) {
            vec.erase(vec.begin() + i);
            i--;
        }
    }
}

bool item_is_blacklisted(const std::string &id)
{
    if (item_whitelist.count(id) > 0) {
        return false;
    } else if (item_blacklist.count(id) > 0) {
        return true;
    }
    // Empty whitelist: default to enable all,
    // Non-empty whitelist: default to disable all.
    return !item_whitelist.empty();
}

void Item_factory::finialize_item_blacklist()
{
    for (t_string_set::const_iterator a = item_whitelist.begin(); a != item_whitelist.end(); ++a) {
        if (!has_template(*a)) {
            debugmsg("item on whitelist %s does not exist", a->c_str());
        }
    }
    for (t_string_set::const_iterator a = item_blacklist.begin(); a != item_blacklist.end(); ++a) {
        if (!has_template(*a)) {
            debugmsg("item on blacklist %s does not exist", a->c_str());
        }
    }
    for (std::map<std::string, itype *>::const_iterator a = m_templates.begin(); a != m_templates.end();
         ++a) {
        const std::string &itm = a->first;
        if (!item_is_blacklisted(itm)) {
            continue;
        }
        for( auto &elem : m_template_groups ) {
            elem.second->remove_item( itm );
        }
        for( auto &recipes_b : recipes ) {
            for( size_t c = 0; c < recipes_b.second.size(); c++ ) {
                recipe *r = recipes_b.second[c];
                if( r->result == itm || r->requirements.remove_item(itm) ) {
                    delete r;
                    recipes_b.second.erase( recipes_b.second.begin() + c );
                    c--;
                    continue;
                }
            }
        }
        for (size_t i = 0; i < constructions.size(); i++) {
            construction *c = constructions[i];
            if( c->requirements.remove_item( itm ) ) {
                delete c;
                constructions.erase(constructions.begin() + i);
                i--;
            }
        }
        for( auto &elem : terlist ) {
            remove_item( itm, elem.bash.items );
        }
        for( auto &elem : furnlist ) {
            remove_item( itm, elem.bash.items );
        }
    }
}

void add_to_set(t_string_set &s, JsonObject &json, const std::string &name)
{
    JsonArray jarr = json.get_array(name);
    while (jarr.has_more()) {
        s.insert(jarr.next_string());
    }
}

void Item_factory::load_item_blacklist(JsonObject &json)
{
    add_to_set(item_blacklist, json, "items");
}

void Item_factory::load_item_whitelist(JsonObject &json)
{
    add_to_set(item_whitelist, json, "items");
}

Item_factory::~Item_factory()
{
    clear();
}

Item_factory::Item_factory()
{
    init();
}

void Item_factory::init()
{
    //Populate the iuse functions
    iuse_function_list["NONE"] = use_function();
    iuse_function_list["RAW_MEAT"] = &iuse::raw_meat;
    iuse_function_list["RAW_FAT"] = &iuse::raw_fat;
    iuse_function_list["RAW_BONE"] = &iuse::raw_bone;
    iuse_function_list["RAW_FISH"] = &iuse::raw_fish;
    iuse_function_list["RAW_WILDVEG"] = &iuse::raw_wildveg;
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
    iuse_function_list["ALCOHOL_STRONG"] = &iuse::alcohol_strong;
    iuse_function_list["XANAX"] = &iuse::xanax;
    iuse_function_list["SMOKING"] = &iuse::smoking;
    iuse_function_list["SMOKING_PIPE"] = &iuse::smoking_pipe;
    iuse_function_list["ECIG"] = &iuse::ecig;
    iuse_function_list["ANTIBIOTIC"] = &iuse::antibiotic;
    iuse_function_list["EYEDROPS"] = &iuse::eyedrops;
    iuse_function_list["FUNGICIDE"] = &iuse::fungicide;
    iuse_function_list["ANTIFUNGAL"] = &iuse::antifungal;
    iuse_function_list["ANTIPARASITIC"] = &iuse::antiparasitic;
    iuse_function_list["ANTICONVULSANT"] = &iuse::anticonvulsant;
    iuse_function_list["WEED_BROWNIE"] = &iuse::weed_brownie;
    iuse_function_list["COKE"] = &iuse::coke;
    iuse_function_list["GRACK"] = &iuse::grack;
    iuse_function_list["METH"] = &iuse::meth;
    iuse_function_list["VITAMINS"] = &iuse::vitamins;
    iuse_function_list["VACCINE"] = &iuse::vaccine;
    iuse_function_list["POISON"] = &iuse::poison;
    iuse_function_list["FUN_HALLU"] = &iuse::fun_hallu;
    iuse_function_list["THORAZINE"] = &iuse::thorazine;
    iuse_function_list["PROZAC"] = &iuse::prozac;
    iuse_function_list["SLEEP"] = &iuse::sleep;
    iuse_function_list["IODINE"] = &iuse::iodine;
    iuse_function_list["DATURA"] = &iuse::datura;
    iuse_function_list["FLUMED"] = &iuse::flumed;
    iuse_function_list["FLUSLEEP"] = &iuse::flusleep;
    iuse_function_list["INHALER"] = &iuse::inhaler;
    iuse_function_list["BLECH"] = &iuse::blech;
    iuse_function_list["PLANTBLECH"] = &iuse::plantblech;
    iuse_function_list["CHEW"] = &iuse::chew;
    iuse_function_list["MUTAGEN"] = &iuse::mutagen;
    iuse_function_list["MUT_IV"] = &iuse::mut_iv;
    iuse_function_list["PURIFIER"] = &iuse::purifier;
    iuse_function_list["MUT_IV"] = &iuse::mut_iv;
    iuse_function_list["PURIFY_IV"] = &iuse::purify_iv;
    iuse_function_list["MARLOSS"] = &iuse::marloss;
    iuse_function_list["MARLOSS_SEED"] = &iuse::marloss_seed;
    iuse_function_list["MARLOSS_GEL"] = &iuse::marloss_gel;
    iuse_function_list["MYCUS"] = &iuse::mycus;
    iuse_function_list["DOGFOOD"] = &iuse::dogfood;
    iuse_function_list["CATFOOD"] = &iuse::catfood;

    // TOOLS
    iuse_function_list["FIRESTARTER"] = &iuse::firestarter;
    iuse_function_list["SEW"] = &iuse::sew;
    iuse_function_list["EXTRA_BATTERY"] = &iuse::extra_battery;
    iuse_function_list["RECHARGEABLE_BATTERY"] = &iuse::rechargeable_battery;
    iuse_function_list["EXTINGUISHER"] = &iuse::extinguisher;
    iuse_function_list["HAMMER"] = &iuse::hammer;
    iuse_function_list["DIRECTIONAL_ANTENNA"] = &iuse::directional_antenna;
    iuse_function_list["SOLDER_WELD"] = &iuse::solder_weld;
    iuse_function_list["WATER_PURIFIER"] = &iuse::water_purifier;
    iuse_function_list["TWO_WAY_RADIO"] = &iuse::two_way_radio;
    iuse_function_list["RADIO_OFF"] = &iuse::radio_off;
    iuse_function_list["RADIO_ON"] = &iuse::radio_on;
    iuse_function_list["HORN_BICYCLE"] = &iuse::horn_bicycle;
    iuse_function_list["NOISE_EMITTER_OFF"] = &iuse::noise_emitter_off;
    iuse_function_list["NOISE_EMITTER_ON"] = &iuse::noise_emitter_on;
    iuse_function_list["MA_MANUAL"] = &iuse::ma_manual;
    iuse_function_list["CROWBAR"] = &iuse::crowbar;
    iuse_function_list["MAKEMOUND"] = &iuse::makemound;
    iuse_function_list["DIG"] = &iuse::dig;
    iuse_function_list["SIPHON"] = &iuse::siphon;
    iuse_function_list["CHAINSAW_OFF"] = &iuse::chainsaw_off;
    iuse_function_list["CHAINSAW_ON"] = &iuse::chainsaw_on;
    iuse_function_list["ELEC_CHAINSAW_OFF"] = &iuse::elec_chainsaw_off;
    iuse_function_list["ELEC_CHAINSAW_ON"] = &iuse::elec_chainsaw_on;
    iuse_function_list["CS_LAJATANG_OFF"] = &iuse::cs_lajatang_off;
    iuse_function_list["CS_LAJATANG_ON"] = &iuse::cs_lajatang_on;
    iuse_function_list["CARVER_OFF"] = &iuse::carver_off;
    iuse_function_list["CARVER_ON"] = &iuse::carver_on;
    iuse_function_list["TRIMMER_OFF"] = &iuse::trimmer_off;
    iuse_function_list["TRIMMER_ON"] = &iuse::trimmer_on;
    iuse_function_list["CIRCSAW_ON"] = &iuse::circsaw_on;
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
    iuse_function_list["THROWABLE_EXTINGUISHER_ACT"] = &iuse::throwable_extinguisher_act;
    iuse_function_list["PIPEBOMB_ACT"] = &iuse::pipebomb_act;
    iuse_function_list["GRANADE"] = &iuse::granade;
    iuse_function_list["GRANADE_ACT"] = &iuse::granade_act;
    iuse_function_list["C4"] = &iuse::c4;
    iuse_function_list["ACIDBOMB_ACT"] = &iuse::acidbomb_act;
    iuse_function_list["GRENADE_INC_ACT"] = &iuse::grenade_inc_act;
    iuse_function_list["ARROW_FLAMABLE"] = &iuse::arrow_flamable;
    iuse_function_list["MOLOTOV"] = &iuse::molotov;
    iuse_function_list["MOLOTOV_LIT"] = &iuse::molotov_lit;
    iuse_function_list["FIRECRACKER_PACK"] = &iuse::firecracker_pack;
    iuse_function_list["FIRECRACKER_PACK_ACT"] = &iuse::firecracker_pack_act;
    iuse_function_list["FIRECRACKER"] = &iuse::firecracker;
    iuse_function_list["FIRECRACKER_ACT"] = &iuse::firecracker_act;
    iuse_function_list["MININUKE"] = &iuse::mininuke;
    iuse_function_list["PHEROMONE"] = &iuse::pheromone;
    iuse_function_list["PORTAL"] = &iuse::portal;
    iuse_function_list["TAZER"] = &iuse::tazer;
    iuse_function_list["TAZER2"] = &iuse::tazer2;
    iuse_function_list["SHOCKTONFA_OFF"] = &iuse::shocktonfa_off;
    iuse_function_list["SHOCKTONFA_ON"] = &iuse::shocktonfa_on;
    iuse_function_list["MP3"] = &iuse::mp3;
    iuse_function_list["MP3_ON"] = &iuse::mp3_on;
    iuse_function_list["PORTABLE_GAME"] = &iuse::portable_game;
    iuse_function_list["VIBE"] = &iuse::vibe;
    iuse_function_list["VORTEX"] = &iuse::vortex;
    iuse_function_list["DOG_WHISTLE"] = &iuse::dog_whistle;
    iuse_function_list["VACUTAINER"] = &iuse::vacutainer;
    iuse_function_list["KNIFE"] = &iuse::knife;
    iuse_function_list["LUMBER"] = &iuse::lumber;
    iuse_function_list["OXYTORCH"] = &iuse::oxytorch;
    iuse_function_list["HACKSAW"] = &iuse::hacksaw;
    iuse_function_list["PORTABLE_STRUCTURE"] = &iuse::portable_structure;
    iuse_function_list["TORCH_LIT"] = &iuse::torch_lit;
    iuse_function_list["BATTLETORCH_LIT"] = &iuse::battletorch_lit;
    iuse_function_list["BULLET_PULLER"] = &iuse::bullet_puller;
    iuse_function_list["BOLTCUTTERS"] = &iuse::boltcutters;
    iuse_function_list["MOP"] = &iuse::mop;
    iuse_function_list["SPRAY_CAN"] = &iuse::spray_can;
    iuse_function_list["RAG"] = &iuse::rag;
    iuse_function_list["LAW"] = &iuse::LAW;
    iuse_function_list["HEATPACK"] = &iuse::heatpack;
    iuse_function_list["BOOTS"] = &iuse::boots;
    iuse_function_list["QUIVER"] = &iuse::quiver;
    iuse_function_list["SHEATH_SWORD"] = &iuse::sheath_sword;
    iuse_function_list["SHEATH_KNIFE"] = &iuse::sheath_knife;
    iuse_function_list["HOLSTER_PISTOL"] = &iuse::holster_pistol;
    iuse_function_list["HOLSTER_ANKLE"] = &iuse::holster_ankle;
    iuse_function_list["TOWEL"] = &iuse::towel;
    iuse_function_list["UNFOLD_GENERIC"] = &iuse::unfold_generic;
    iuse_function_list["ADRENALINE_INJECTOR"] = &iuse::adrenaline_injector;
    iuse_function_list["JET_INJECTOR"] = &iuse::jet_injector;
    iuse_function_list["CONTACTS"] = &iuse::contacts;
    iuse_function_list["AIRHORN"] = &iuse::airhorn;
    iuse_function_list["HOTPLATE"] = &iuse::hotplate;
    iuse_function_list["DOLLCHAT"] = &iuse::talking_doll;
    iuse_function_list["BELL"] = &iuse::bell;
    iuse_function_list["SEED"] = &iuse::seed;
    iuse_function_list["OXYGEN_BOTTLE"] = &iuse::oxygen_bottle;
    iuse_function_list["ATOMIC_BATTERY"] = &iuse::atomic_battery;
    iuse_function_list["UPS_BATTERY"] = &iuse::ups_battery;
    iuse_function_list["FISH_ROD"] = &iuse::fishing_rod;
    iuse_function_list["FISH_TRAP"] = &iuse::fish_trap;
    iuse_function_list["GUN_REPAIR"] = &iuse::gun_repair;
    iuse_function_list["MISC_REPAIR"] = &iuse::misc_repair;
    iuse_function_list["RM13ARMOR_OFF"] = &iuse::rm13armor_off;
    iuse_function_list["RM13ARMOR_ON"] = &iuse::rm13armor_on;
    iuse_function_list["UNPACK_ITEM"] = &iuse::unpack_item;
    iuse_function_list["PACK_ITEM"] = &iuse::pack_item;
    iuse_function_list["RADGLOVE"] = &iuse::radglove;
    iuse_function_list["ROBOTCONTROL"] = &iuse::robotcontrol;
    iuse_function_list["EINKTABLETPC"] = &iuse::einktabletpc;
    iuse_function_list["CAMERA"] = &iuse::camera;
    iuse_function_list["EHANDCUFFS"] = &iuse::ehandcuffs;
    iuse_function_list["CABLE_ATTACH"]  = &iuse::cable_attach;
    iuse_function_list["SURVIVOR_BELT"]  = &iuse::survivor_belt;
    iuse_function_list["WEATHER_TOOL"] = &iuse::weather_tool;
    iuse_function_list["REMOVE_ALL_MODS"] = &iuse::remove_all_mods;

    // MACGUFFINS
    iuse_function_list["MCG_NOTE"] = &iuse::mcg_note;

    // ARTIFACTS
    // This function is used when an artifact is activated
    // It examines the item's artifact-specific properties
    // See artifact.h for a list
    iuse_function_list["ARTIFACT"] = &iuse::artifact;

    iuse_function_list["RADIOCAR"] = &iuse::radiocar;
    iuse_function_list["RADIOCARON"] = &iuse::radiocaron;
    iuse_function_list["RADIOCONTROL"] = &iuse::radiocontrol;

    iuse_function_list["MULTICOOKER"] = &iuse::multicooker;

    iuse_function_list["REMOTEVEH"] = &iuse::remoteveh;

    create_inital_categories();

    init_old();
}

void Item_factory::create_inital_categories()
{
    // Load default categories with their default sort_rank
    // Negative rank so the default categories come before all
    // the explicit defined categories from json
    // (assuming the category definitions in json use positive sort_ranks).
    // Note that json data might override these settings
    // (simply define a category in json with the id
    // taken from category_id_* and that definition will get
    // used - see load_item_category)
    add_category(category_id_guns, -21, _("guns"));
    add_category(category_id_ammo, -20, _("ammo"));
    add_category(category_id_weapons, -19, _("weapons"));
    add_category(category_id_tools, -18, _("tools"));
    add_category(category_id_clothing, -17, _("clothing"));
    add_category(category_id_food, -16, _("food"));
    add_category(category_id_drugs, -15, _("drugs"));
    add_category(category_id_books, -14, _("books"));
    add_category(category_id_mods, -13, _("mods"));
    add_category(category_id_cbm, -12, _("bionics"));
    add_category(category_id_mutagen, -11, _("mutagen"));
    add_category(category_id_other, -10, _("other"));
}

void Item_factory::add_category(const std::string &id, int sort_rank, const std::string &name)
{
    // unconditionally override any existing definition
    // as there should be none.
    item_category &cat = m_categories[id];
    cat.id = id;
    cat.sort_rank = sort_rank;
    cat.name = name;
}

inline int ammo_type_defined(const std::string &ammo)
{
    if (ammo == "NULL" || ammo == "generic_no_ammo") {
        return 1; // Known ammo type
    }
    if (ammo_name(ammo) != "XXX") {
        return 1; // Known ammo type
    }
    if (!item_controller->has_template(ammo)) {
        return 0; // Unknown ammo type
    }
    return 2; // Unknown from ammo_name, but defined as itype
}

void Item_factory::check_ammo_type(std::ostream &msg, const std::string &ammo) const
{
    if (ammo == "NULL" || ammo == "generic_no_ammo") {
        return;
    }
    if (ammo_name(ammo) == "XXX") {
        msg << string_format("ammo type %s not listed in ammo_name() function", ammo.c_str()) << "\n";
    }
    if (ammo == "UPS") {
        return;
    }
    for( const auto &elem : m_templates ) {
        const it_ammo *ammot = dynamic_cast<const it_ammo *>( elem.second );
        if (ammot == 0) {
            continue;
        }
        if (ammot->type == ammo) {
            return;
        }
    }
    msg << string_format("there is no actual ammo of type %s defined", ammo.c_str()) << "\n";
}

void Item_factory::check_definitions() const
{
    std::ostringstream main_stream;
    for( const auto &elem : m_templates ) {
        std::ostringstream msg;
        const itype *type = elem.second;
        for( auto mat_id : type->materials ) {
            if( mat_id != "null" && !material_type::has_material(mat_id) ) {
                msg << string_format("invalid material %s", mat_id.c_str()) << "\n";
            }
        }
        for( const auto &_a : type->techniques ) {
            if( ma_techniques.count( _a ) == 0 ) {
                msg << string_format( "unknown technique %s", _a.c_str() ) << "\n";
            }
        }
        if( !type->snippet_category.empty() ) {
            if( !SNIPPET.has_category( type->snippet_category ) ) {
                msg << string_format("snippet category %s without any snippets", type->id.c_str(), type->snippet_category.c_str()) << "\n";
            }
        }
        for (std::map<std::string, int>::const_iterator a = type->qualities.begin();
             a != type->qualities.end(); ++a) {
            if( !quality::has( a->first ) ) {
                msg << string_format("item %s has unknown quality %s", type->id.c_str(), a->first.c_str()) << "\n";
            }
        }
        const it_comest *comest = dynamic_cast<const it_comest *>(type);
        if (comest != 0) {
            if (comest->default_container != "null" && !has_template(comest->default_container)) {
                msg << string_format("invalid container property %s", comest->default_container.c_str()) << "\n";
            }
            if (comest->tool != "null" && !has_template(comest->tool)) {
                msg << string_format("invalid tool property %s", comest->tool.c_str()) << "\n";
            }
        }
        const it_ammo *ammo = dynamic_cast<const it_ammo *>(type);
        if (ammo != 0) {
            check_ammo_type(msg, ammo->type);
            if (ammo->casing != "NULL" && !has_template(ammo->casing)) {
                msg << string_format("invalid casing property %s", ammo->casing.c_str()) << "\n";
            }
            if (ammo->default_container != "null" && !has_template(ammo->default_container)) {
                msg << string_format("invalid container property %s", ammo->default_container.c_str()) << "\n";
            }
        }
        const it_gun *gun = dynamic_cast<const it_gun *>(type);
        if (gun != 0) {
            check_ammo_type(msg, gun->ammo);
            if (gun->skill_used == 0) {
                msg << string_format("uses no skill") << "\n";
            }
        }
        const it_gunmod *gunmod = dynamic_cast<const it_gunmod *>(type);
        if (gunmod != 0) {
            check_ammo_type(msg, gunmod->newtype);
        }
        const it_tool *tool = dynamic_cast<const it_tool *>(type);
        if (tool != 0) {
            check_ammo_type(msg, tool->ammo);
            if (tool->revert_to != "null" && !has_template(tool->revert_to)) {
                msg << string_format("invalid revert_to property %s", tool->revert_to.c_str()) << "\n";
            }
        }
        const it_bionic *bionic = dynamic_cast<const it_bionic *>(type);
        if (bionic != 0) {
            if (bionics.count(bionic->id) == 0) {
                msg << string_format("there is no bionic with id %s", bionic->id.c_str()) << "\n";
            }
        }
        if (msg.str().empty()) {
            continue;
        }
        main_stream << "warnings for type " << type->id << ":\n" << msg.str() << "\n";
        const std::string &buffer = main_stream.str();
        const size_t lines = std::count(buffer.begin(), buffer.end(), '\n');
        if (lines > 10) {
            fold_and_print(stdscr, 0, 0, getmaxx(stdscr), c_red, "%s\n  Press any key...", buffer.c_str());
            getch();
            werase(stdscr);
            main_stream.str(std::string());
        }
    }
    const std::string &buffer = main_stream.str();
    if (!buffer.empty()) {
        fold_and_print(stdscr, 0, 0, getmaxx(stdscr), c_red, "%s\n  Press any key...", buffer.c_str());
        getch();
        werase(stdscr);
    }
    for( const auto &elem : m_template_groups ) {
        elem.second->check_consistency();
    }
}

bool Item_factory::has_template(const Item_tag &id) const
{
    return m_templates.count(id) > 0;
}

//Returns the template with the given identification tag
itype *Item_factory::find_template(Item_tag id)
{
    std::map<Item_tag, itype *>::iterator found = m_templates.find(id);
    if (found != m_templates.end()) {
        return found->second;
    }

    debugmsg("Missing item (check item_groups.json): %s", id.c_str());
    it_artifact_tool *bad_itype = new it_artifact_tool();
    bad_itype->id = id.c_str();
    bad_itype->name = string_format("undefined-%ss", id.c_str());
    bad_itype->name_plural = string_format("undefined-%ss", id.c_str());
    bad_itype->description = string_format("A strange shimmering... nothing."
                                           "  You think it wants to be a %s.", id.c_str());
    bad_itype->sym = '.';
    bad_itype->color = c_white;
    m_templates[id] = bad_itype;
    return bad_itype;
}

void Item_factory::add_item_type(itype *new_type)
{
    if( new_type == nullptr ) {
        debugmsg( "called Item_factory::add_item_type with nullptr" );
        return;
    }
    auto &entry = m_templates[new_type->id];
    delete entry;
    entry = new_type;
}

Item_spawn_data *Item_factory::get_group(const Item_tag &group_tag)
{
    GroupMap::iterator group_iter = m_template_groups.find(group_tag);
    if (group_iter != m_template_groups.end()) {
        return group_iter->second;
    }
    return NULL;
}

///////////////////////
// DATA FILE READING //
///////////////////////

template<typename SlotType>
void Item_factory::load_slot( std::unique_ptr<SlotType> &slotptr, JsonObject &jo )
{
    slotptr.reset( new SlotType() );
    load( *slotptr, jo );
}

template<typename SlotType>
void Item_factory::load_slot_optional( std::unique_ptr<SlotType> &slotptr, JsonObject &jo, const std::string &member )
{
    if( !jo.has_member( member ) ) {
        return;
    }
    JsonObject slotjo = jo.get_object( member );
    load_slot( slotptr, slotjo );
}

void Item_factory::load_ammo(JsonObject &jo)
{
    it_ammo *ammo_template = new it_ammo();
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
    ammo_template->default_container = jo.get_string("container", "null");

    itype *new_item_template = ammo_template;
    load_basic_info(jo, new_item_template);
}

void Item_factory::load_gun(JsonObject &jo)
{
    it_gun *gun_template = new it_gun();
    gun_template->ammo = jo.get_string("ammo");
    gun_template->skill_used = Skill::skill(jo.get_string("skill"));
    gun_template->dmg_bonus = jo.get_int("ranged_damage");
    gun_template->range = jo.get_int("range");
    gun_template->dispersion = jo.get_int("dispersion");
    gun_template->sight_dispersion = jo.get_int("sight_dispersion");
    gun_template->aim_speed = jo.get_int("aim_speed");
    gun_template->recoil = jo.get_int("recoil");
    gun_template->durability = jo.get_int("durability");
    gun_template->burst = jo.get_int("burst");
    gun_template->clip = jo.get_int("clip_size");
    gun_template->reload_time = jo.get_int("reload");
    gun_template->pierce = jo.get_int("pierce", 0);
    gun_template->ammo_effects = jo.get_tags("ammo_effects");
    gun_template->ups_charges = jo.get_int( "ups_charges", 0 );

    if (jo.has_array("valid_mod_locations")) {
        JsonArray jarr = jo.get_array("valid_mod_locations");
        while (jarr.has_more()) {
            JsonArray curr = jarr.next_array();
            gun_template->valid_mod_locations.insert(std::pair<std::string, int>(curr.get_string(0),
                    curr.get_int(1)));
        }
    }

    itype *new_item_template = gun_template;
    load_basic_info(jo, new_item_template);
}

void Item_factory::load_armor(JsonObject &jo)
{
    itype* new_item_template = new itype();
    load_slot( new_item_template->armor, jo );
    load_basic_info( jo, new_item_template );
}

void Item_factory::load( islot_armor &slot, JsonObject &jo )
{
    slot.encumber = jo.get_int( "encumbrance" );
    slot.coverage = jo.get_int( "coverage" );
    slot.thickness = jo.get_int( "material_thickness" );
    // TODO (as of may 2014): sometimes in the future: remove this if clause and accept
    // only "environmental_protection" and not "enviromental_protection".
    if( jo.has_member( "enviromental_protection" ) ) {
        debugmsg( "the item property \"enviromental_protection\" has been renamed to \"environmental_protection\"\n"
                  "please change the json data for item %d", jo.get_string( "id", "" ).c_str() );
        slot.env_resist = jo.get_int( "enviromental_protection" );
    } else {
        slot.env_resist = jo.get_int( "environmental_protection" );
    }
    slot.warmth = jo.get_int( "warmth" );
    slot.storage = jo.get_int( "storage" );
    slot.power_armor = jo.get_bool( "power_armor", false );
    slot.covers = jo.has_member( "covers" ) ? flags_from_json( jo, "covers", "bodyparts" ) : 0;
    slot.sided = jo.has_member( "covers" ) ? flags_from_json( jo, "covers", "sided" ) : 0;
}

void Item_factory::load_tool(JsonObject &jo)
{
    it_tool *tool_template = new it_tool();
    tool_template->ammo = jo.get_string("ammo");
    tool_template->max_charges = jo.get_long("max_charges");
    tool_template->def_charges = jo.get_long("initial_charges");

    if (jo.has_array("rand_charges")) {
        JsonArray jarr = jo.get_array("rand_charges");
        while (jarr.has_more()) {
            tool_template->rand_charges.push_back(jarr.next_long());
        }
    } else {
        tool_template->rand_charges.push_back(tool_template->def_charges);
    }

    tool_template->charges_per_use = jo.get_int("charges_per_use");
    tool_template->turns_per_charge = jo.get_int("turns_per_charge");
    tool_template->revert_to = jo.get_string("revert_to");
    tool_template->subtype = jo.get_string("sub", "");

    itype *new_item_template = tool_template;
    load_basic_info(jo, new_item_template);
}

void Item_factory::load_tool_armor(JsonObject &jo)
{
    it_tool *tool_template = new it_tool();

    tool_template->ammo = jo.get_string("ammo");
    tool_template->max_charges = jo.get_int("max_charges");
    tool_template->def_charges = jo.get_int("initial_charges");
    tool_template->charges_per_use = jo.get_int("charges_per_use");
    tool_template->turns_per_charge = jo.get_int("turns_per_charge");
    tool_template->revert_to = jo.get_string("revert_to");

    load_slot( tool_template->armor , jo );

    load_basic_info(jo, tool_template);
}

void Item_factory::load( islot_book &slot, JsonObject &jo )
{
    slot.level = jo.get_int( "max_level" );
    slot.req = jo.get_int( "required_level" );
    slot.fun = jo.get_int( "fun" );
    slot.intel = jo.get_int( "intelligence" );
    slot.time = jo.get_int( "time" );
    slot.type = Skill::skill( jo.get_string( "skill" ) );
    slot.chapters = jo.get_int( "chapters", -1 );
}

void Item_factory::load_book( JsonObject &jo )
{
    itype *new_item_template = new itype();
    load_slot( new_item_template->book, jo );
    load_basic_info( jo, new_item_template );
}

void Item_factory::load_comestible(JsonObject &jo)
{
    it_comest *comest_template = new it_comest();
    comest_template->comesttype = jo.get_string("comestible_type");
    comest_template->tool = jo.get_string("tool", "null");
    comest_template->default_container = jo.get_string("container", "null");
    comest_template->quench = jo.get_int("quench", 0);
    comest_template->nutr = jo.get_int("nutrition", 0);
    comest_template->spoils = jo.get_int("spoils_in", 0);
    // In json it's in hours, here it shall be in turns, as item::rot is also in turns.
    comest_template->spoils *= 600;
    comest_template->brewtime = jo.get_int("brew_time", 0);
    comest_template->addict = jo.get_int("addiction_potential", 0);
    comest_template->charges = jo.get_long("charges", 0);
    if (jo.has_member("stack_size")) {
        comest_template->stack_size = jo.get_long("stack_size");
    } else {
        comest_template->stack_size = comest_template->charges;
    }
    comest_template->stim = jo.get_int("stim", 0);
    // TODO: sometimes in the future: remove this if clause and accept
    // only "healthy" and not "heal".
    if (jo.has_member("heal")) {
        debugmsg("the item property \"heal\" has been renamed to \"healthy\"\n"
                 "please change the json data for item %d", comest_template->id.c_str());
        comest_template->healthy = jo.get_int("heal");
    } else {
        comest_template->healthy = jo.get_int("healthy", 0);
    }
    comest_template->fun = jo.get_int("fun", 0);
    comest_template->add = addiction_type(jo.get_string("addiction_type"));

    if (jo.has_array("rand_charges")) {
        JsonArray jarr = jo.get_array("rand_charges");
        while (jarr.has_more()) {
            comest_template->rand_charges.push_back(jarr.next_long());
        }
    } else {
        comest_template->rand_charges.push_back(comest_template->charges);
    }

    itype *new_item_template = comest_template;
    load_basic_info(jo, new_item_template);
}

void Item_factory::load_container(JsonObject &jo)
{
    itype *new_item_template = new itype();
    load_slot( new_item_template->container, jo );
    load_basic_info( jo, new_item_template );
}

void Item_factory::load( islot_container &slot, JsonObject &jo )
{
    slot.contains = jo.get_int( "contains" );
    slot.seals = jo.get_bool( "seals", false );
    slot.watertight = jo.get_bool( "watertight", false );
    slot.preserves = jo.get_bool( "preserves", false );
    slot.rigid = jo.get_bool( "rigid", false );
}

void Item_factory::load_gunmod(JsonObject &jo)
{
    it_gunmod *gunmod_template = new it_gunmod();
    gunmod_template->damage = jo.get_int("damage_modifier", 0);
    gunmod_template->loudness = jo.get_int("loudness_modifier", 0);
    gunmod_template->newtype = jo.get_string("ammo_modifier");
    gunmod_template->location = jo.get_string("location");
    gunmod_template->used_on_pistol = is_mod_target(jo, "mod_targets", "pistol");
    gunmod_template->used_on_shotgun = is_mod_target(jo, "mod_targets", "shotgun");
    gunmod_template->used_on_smg = is_mod_target(jo, "mod_targets", "smg");
    gunmod_template->used_on_rifle = is_mod_target(jo, "mod_targets", "rifle");
    gunmod_template->used_on_bow = is_mod_target(jo, "mod_targets", "bow");
    gunmod_template->used_on_crossbow = is_mod_target(jo, "mod_targets", "crossbow");
    gunmod_template->used_on_launcher = is_mod_target(jo, "mod_targets", "launcher");
    gunmod_template->dispersion = jo.get_int("dispersion_modifier", 0);
    gunmod_template->mod_dispersion = jo.get_int("dispersion", 0);
    gunmod_template->sight_dispersion = jo.get_int("sight_dispersion", -1);
    gunmod_template->aim_speed = jo.get_int("aim_speed", -1);
    gunmod_template->recoil = jo.get_int("recoil_modifier", 0);
    gunmod_template->burst = jo.get_int("burst_modifier", 0);
    gunmod_template->range = jo.get_int("range", 0);
    gunmod_template->clip = jo.get_int("clip_size_modifier", 0);
    gunmod_template->acceptible_ammo_types = jo.get_tags("acceptable_ammo");
    gunmod_template->skill_used = Skill::skill(jo.get_string("skill", "gun"));
    gunmod_template->req_skill = jo.get_int("skill_required", 0);

    itype *new_item_template = gunmod_template;
    load_basic_info(jo, new_item_template);
}

void Item_factory::load_bionic(JsonObject &jo)
{
    it_bionic *bionic_template = new it_bionic();
    bionic_template->difficulty = jo.get_int("difficulty");
    load_basic_info(jo, bionic_template);
}

void Item_factory::load_veh_part(JsonObject &jo)
{
    it_var_veh_part *veh_par_template = new it_var_veh_part();
    veh_par_template->min_bigness = jo.get_int("min-bigness");
    veh_par_template->max_bigness = jo.get_int("max-bigness");
    const std::string big_aspect = jo.get_string("bigness-aspect");
    if (big_aspect == "WHEEL_DIAMETER") {
        veh_par_template->bigness_aspect = BIGNESS_WHEEL_DIAMETER;
        veh_par_template->engine = false;
    } else if (big_aspect == "ENGINE_DISPLACEMENT") {
        veh_par_template->bigness_aspect = BIGNESS_ENGINE_DISPLACEMENT;
        veh_par_template->engine = true;
    } else {
        throw std::string("invalid bigness-aspect: ") + big_aspect;
    }
    load_basic_info(jo, veh_par_template);
}

void Item_factory::load_generic(JsonObject &jo)
{
    itype *new_item_template = new itype();
    load_basic_info(jo, new_item_template);
}

void Item_factory::load_basic_info(JsonObject &jo, itype *new_item_template)
{
    std::string new_id = jo.get_string("id");
    new_item_template->id = new_id;
    if (m_templates.count(new_id) > 0) {
        // New item already exists. Because mods are loaded after
        // core data, we override it. This allows mods to change
        // item from core data.
        delete m_templates[new_id];
    }
    m_templates[new_id] = new_item_template;

    // And then proceed to assign the correct field
    new_item_template->price = jo.get_int("price");
    new_item_template->name = jo.get_string("name").c_str();
    if (jo.has_member("name_plural")) {
        new_item_template->name_plural = jo.get_string("name_plural").c_str();
    } else {
        // default behaviour: Assume the regular plural form (appending an “s”)
        new_item_template->name_plural = (jo.get_string("name") + "s").c_str();
    }
    new_item_template->sym = jo.get_string("symbol")[0];
    new_item_template->color = color_from_string(jo.get_string("color"));
    new_item_template->description = _(jo.get_string("description").c_str());
    if( jo.has_member("material") ){
        set_material_from_json( jo, "material", new_item_template );
    } else {
        new_item_template->materials.push_back("null");
    }
    Item_tag new_phase = "solid";
    if (jo.has_member("phase")) {
        new_phase = jo.get_string("phase");
    }
    new_item_template->phase = phase_from_tag(new_phase);
    new_item_template->volume = jo.get_int("volume");
    new_item_template->weight = jo.get_int("weight");
    new_item_template->melee_dam = jo.get_int("bashing");
    new_item_template->melee_cut = jo.get_int("cutting");
    new_item_template->m_to_hit = jo.get_int("to_hit");

    if (jo.has_member("explode_in_fire")) {
        JsonObject je = jo.get_object("explode_in_fire");
        je.read("power", new_item_template->explosion_on_fire_data.power);
        je.read("shrapnel", new_item_template->explosion_on_fire_data.shrapnel);
        je.read("fire", new_item_template->explosion_on_fire_data.fire);
        je.read("blast", new_item_template->explosion_on_fire_data.blast);
    }

    new_item_template->light_emission = 0;

    if( jo.has_array( "snippet_category" ) ) {
        // auto-create a category that is unlikely to already be used and put the
        // snippets in it.
        new_item_template->snippet_category = std::string( "auto:" ) + new_item_template->id;
        JsonArray jarr = jo.get_array( "snippet_category" );
        SNIPPET.add_snippets_from_json( new_item_template->snippet_category, jarr );
    } else {
        new_item_template->snippet_category = jo.get_string( "snippet_category", "" );
    }

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
    SWIM_GOGGLES - Allows you to see much further under water.
    REBREATHER - Works to supply you with oxygen while underwater. Requires external limiter like battery power.
    UNRECOVERABLE - Prevents the item from being recovered when deconstructing another item that uses this one.
    GNV_EFFECT - Green night vision effect. Requires external limiter like battery power.
    IR_EFFECT - Infrared vision effect. Requires external limiter like battery power.
    SUN_GLASSES - Protects from sunlight's 'glare' effect.
    RAD_RESIST - Partially protects from ambient radiation.
    RAD_PROOF- Fully protects from ambient radiation.
    ELECTRIC_IMMUNE- Fully protects from electricity.
    THERMOMETER - Shows current air temperature. If an item has Thermo, Hygro and/or Baro, more information is shown, such as windchill and wind speed.
    HYGROMETER - Shows current relative humidity. If an item has Thermo, Hygro and/or Baro, more information is shown, such as windchill and wind speed.
    BAROMETER - Shows current pressure. If an item has Thermo, Hygro and/or Baro, more information is shown, such as windchill and wind speed.
    */
    new_item_template->item_tags = jo.get_tags("flags");
    if (!new_item_template->item_tags.empty()) {
        for (std::set<std::string>::const_iterator it = new_item_template->item_tags.begin();
             it != new_item_template->item_tags.end(); ++it) {
            set_intvar(std::string(*it), new_item_template->light_emission, 1, 10000);
        }
    }

    if (jo.has_member("qualities")) {
        set_qualities_from_json(jo, "qualities", new_item_template);
    }

    new_item_template->techniques = jo.get_tags("techniques");

    if (jo.has_member("use_action")) {
        set_use_methods_from_json(jo, "use_action", new_item_template);
    }

    if (jo.has_member("category")) {
        new_item_template->category = get_category(jo.get_string("category"));
    } else {
        new_item_template->category = get_category(calc_category(new_item_template));
    }

    load_slot_optional( new_item_template->container, jo, "container_data" );
    load_slot_optional( new_item_template->armor, jo, "armor_data" );
    load_slot_optional( new_item_template->book, jo, "book_data" );
}

void Item_factory::load_item_category(JsonObject &jo)
{
    const std::string id = jo.get_string("id");
    // reuse an existing definition,
    // override the name and the sort_rank if
    // these are present in the json
    item_category &cat = m_categories[id];
    cat.id = id;
    if (jo.has_member("name")) {
        cat.name = _(jo.get_string("name").c_str());
    }
    if (jo.has_member("sort_rank")) {
        cat.sort_rank = jo.get_int("sort_rank");
    }
}

void Item_factory::set_qualities_from_json(JsonObject &jo, std::string member,
        itype *new_item_template)
{
    if (jo.has_array(member)) {
        JsonArray jarr = jo.get_array(member);
        while (jarr.has_more()) {
            JsonArray curr = jarr.next_array();
            const auto quali = std::pair<std::string, int>(curr.get_string(0), curr.get_int(1));
            if( new_item_template->qualities.count( quali.first ) > 0 ) {
                curr.throw_error( "Duplicated quality", 0 );
            }
            new_item_template->qualities.insert( quali );
        }
    } else {
        jo.throw_error( "Qualities list is not an array", member );
    }
}

std::bitset<num_bp> Item_factory::flags_from_json(JsonObject &jo, const std::string &member,
        std::string flag_type)
{
    //If none is found, just use the standard none action
    std::bitset<num_bp> flag = 0;
    //Otherwise, grab the right label to look for
    if (jo.has_array(member)) {
        JsonArray jarr = jo.get_array(member);
        while (jarr.has_more()) {
            set_flag_by_string(flag, jarr.next_string(), flag_type);
        }
    } else if (jo.has_string(member)) {
        //we should have gotten a string, if not an array
        set_flag_by_string(flag, jo.get_string(member), flag_type);
    }

    return flag;
}

void Item_factory::set_material_from_json( JsonObject& jo, std::string member,
                                           itype *new_item_template )
{
    // All materials need a type, even if it is "null", which seems to be the base type.
    if( jo.has_array(member) ) {
        JsonArray jarr = jo.get_array(member);
        for( int i = 0; i < (int)jarr.size(); ++i ) {
            std::string material_id = jarr.get_string(i);
            if( material_id == "null" ) {
                continue;
            }
            new_item_template->materials.push_back( material_id );
        }
    } else if( jo.has_string(member) ) {
        new_item_template->materials.push_back( jo.get_string(member) );
    } else {
        // Default material.
        new_item_template->materials.push_back("null");
    }
}

bool Item_factory::is_mod_target(JsonObject &jo, std::string member, std::string weapon)
{
    //If none is found, just use the standard none action
    unsigned is_included = false;
    //Otherwise, grab the right label to look for
    if (jo.has_array(member)) {
        JsonArray jarr = jo.get_array(member);
        while (jarr.has_more() && is_included == false) {
            if (jarr.next_string() == weapon) {
                is_included = true;
            }
        }
    } else {
        if (jo.get_string(member) == weapon) {
            is_included = true;
        }
    }
    return is_included;
}

void Item_factory::reset()
{
    clear();
    init();
}

void Item_factory::clear()
{
    // clear groups
    for( auto &elem : m_template_groups ) {
        delete elem.second;
    }
    m_template_groups.clear();

    m_categories.clear();
    create_inital_categories();
    // Also clear functions refering to lua
    iuse_function_list.clear();

    for( auto &elem : m_templates ) {
        delete elem.second;
    }
    m_templates.clear();
    item_blacklist.clear();
    item_whitelist.clear();
}

Item_group *make_group_or_throw(Item_spawn_data *&isd, Item_group::Type t)
{

    Item_group *ig = dynamic_cast<Item_group *>(isd);
    if (ig == NULL) {
        isd = ig = new Item_group(t, 100);
    } else if (ig->type != t) {
        throw std::string("item group already definded with different type");
    }
    return ig;
}

template<typename T>
bool load_min_max(std::pair<T, T> &pa, JsonObject &obj, const std::string &name)
{
    bool result = false;
    result |= obj.read(name, pa.first);
    result |= obj.read(name, pa.second);
    result |= obj.read(name + "-min", pa.first);
    result |= obj.read(name + "-max", pa.second);
    return result;
}

bool load_sub_ref(std::unique_ptr<Item_spawn_data> &ptr, JsonObject &obj, const std::string &name)
{
    if (obj.has_member(name)) {
        // TODO!
    } else if (obj.has_member(name + "-item")) {
        ptr.reset(new Single_item_creator(obj.get_string(name + "-item"), Single_item_creator::S_ITEM,
                                          100));
        return true;
    } else if (obj.has_member(name + "-group")) {
        ptr.reset(new Single_item_creator(obj.get_string(name + "-group"),
                                          Single_item_creator::S_ITEM_GROUP, 100));
        return true;
    }
    return false;
}

void Item_factory::add_entry(Item_group *ig, JsonObject &obj)
{
    std::unique_ptr<Item_spawn_data> ptr;
    int probability = obj.get_int("prob", 100);
    JsonArray jarr;
    if (obj.has_member("collection")) {
        ptr.reset(new Item_group(Item_group::G_COLLECTION, probability));
        jarr = obj.get_array("collection");
    } else if (obj.has_member("distribution")) {
        ptr.reset(new Item_group(Item_group::G_DISTRIBUTION, probability));
        jarr = obj.get_array("distribution");
    }
    if (ptr.get() != NULL) {
        Item_group *ig2 = dynamic_cast<Item_group *>(ptr.get());
        while (jarr.has_more()) {
            JsonObject job2 = jarr.next_object();
            add_entry(ig2, job2);
        }
        ig->add_entry(ptr);
        return;
    }

    if (obj.has_member("item")) {
        ptr.reset(new Single_item_creator(obj.get_string("item"), Single_item_creator::S_ITEM,
                                          probability));
    } else if (obj.has_member("group")) {
        ptr.reset(new Single_item_creator(obj.get_string("group"), Single_item_creator::S_ITEM_GROUP,
                                          probability));
    }
    if (ptr.get() == NULL) {
        return;
    }
    std::unique_ptr<Item_modifier> modifier(new Item_modifier());
    bool use_modifier = false;
    use_modifier |= load_min_max(modifier->damage, obj, "damage");
    use_modifier |= load_min_max(modifier->charges, obj, "charges");
    use_modifier |= load_min_max(modifier->count, obj, "count");
    use_modifier |= load_sub_ref(modifier->ammo, obj, "ammo");
    use_modifier |= load_sub_ref(modifier->container, obj, "container");
    use_modifier |= load_sub_ref(modifier->contents, obj, "contents");
    if (use_modifier) {
        dynamic_cast<Single_item_creator *>(ptr.get())->modifier = std::move(modifier);
    }
    ig->add_entry(ptr);
}

// Load an item group from JSON
void Item_factory::load_item_group(JsonObject &jsobj)
{
    const Item_tag group_id = jsobj.get_string("id");
    const std::string subtype = jsobj.get_string("subtype", "old");
    load_item_group(jsobj, group_id, subtype);
}

void Item_factory::load_item_group(JsonObject &jsobj, const Group_tag &group_id,
                                   const std::string &subtype)
{
    Item_spawn_data *&isd = m_template_groups[group_id];
    Item_group *ig = dynamic_cast<Item_group *>(isd);
    if (subtype == "old") {
        ig = make_group_or_throw(isd, Item_group::G_DISTRIBUTION);
        ig->with_ammo = jsobj.get_bool("guns_with_ammo", ig->with_ammo);
    } else if (subtype == "collection") {
        ig = make_group_or_throw(isd, Item_group::G_COLLECTION);
    } else if (subtype == "distribution") {
        ig = make_group_or_throw(isd, Item_group::G_DISTRIBUTION);
    } else {
        jsobj.throw_error("unknown item group type", "subtype");
    }

    if (subtype == "old") {
        JsonArray items = jsobj.get_array("items");
        while (items.has_more()) {
            if( items.test_object() ) {
                JsonObject subobj = items.next_object();
                add_entry( ig, subobj );
            } else {
                JsonArray pair = items.next_array();
                ig->add_item_entry(pair.get_string(0), pair.get_int(1));
            }
        }
        return;
    }

    if (jsobj.has_member("entries")) {
        JsonArray items = jsobj.get_array("entries");
        while (items.has_more()) {
            JsonObject subobj = items.next_object();
            add_entry(ig, subobj);
        }
    }
    if (jsobj.has_member("items")) {
        JsonArray items = jsobj.get_array("items");
        while (items.has_more()) {
            if (items.test_string()) {
                ig->add_item_entry(items.next_string(), 100);
            } else if (items.test_array()) {
                JsonArray subitem = items.next_array();
                ig->add_item_entry(subitem.get_string(0), subitem.get_int(1));
            } else {
                JsonObject subobj = items.next_object();
                add_entry(ig, subobj);
            }
        }
    }
    if (jsobj.has_member("groups")) {
        JsonArray items = jsobj.get_array("groups");
        while (items.has_more()) {
            if (items.test_string()) {
                ig->add_group_entry(items.next_string(), 100);
            } else if (items.test_array()) {
                JsonArray subitem = items.next_array();
                ig->add_group_entry(subitem.get_string(0), subitem.get_int(1));
            } else {
                JsonObject subobj = items.next_object();
                add_entry(ig, subobj);
            }
        }
    }
}

void Item_factory::set_use_methods_from_json(JsonObject &jo, std::string member,
        itype *new_item_template)
{
    if (jo.has_array(member)) {
        JsonArray jarr = jo.get_array(member);
        while (jarr.has_more()) {
            use_function new_function;
            if (jarr.test_string()) {
                new_function = use_from_string(jarr.next_string());
            } else if (jarr.test_object()) {
                new_function = use_from_object(jarr.next_object());
            } else {
                debugmsg("use_action array element for item %s is neither string nor object.",
                         new_item_template->id.c_str());
            }
            new_item_template->use_methods.push_back(new_function);
        }
    } else {
        use_function new_function;
        if (jo.has_string("use_action")) {
            new_function = use_from_string(jo.get_string("use_action"));
        } else if (jo.has_object("use_action")) {
            new_function = use_from_object(jo.get_object("use_action"));
        } else {
            debugmsg("use_action member for item %s is neither string nor object.",
                     new_item_template->id.c_str());
        }
        new_item_template->use_methods.push_back(new_function);
    }
}

template<typename IuseActorType>
use_function load_actor( JsonObject obj )
{
    std::unique_ptr<IuseActorType> actor( new IuseActorType() );
    actor->load( obj );
    return use_function( actor.release() );
}

use_function Item_factory::use_from_object(JsonObject obj)
{
    const std::string type = obj.get_string("type");
    if (type == "transform") {
        return load_actor<iuse_transform>( obj );
    } else if (type == "auto_transform") {
        return load_actor<auto_iuse_transform>( obj );
    } else if (type == "delayed_transform") {
        return load_actor<delayed_transform_iuse>( obj );
    } else if (type == "explosion") {
        return load_actor<explosion_iuse>( obj );
    } else if (type == "unfold_vehicle") {
        return load_actor<unfold_vehicle_iuse>( obj );
    } else if (type == "picklock") {
        return load_actor<pick_lock_actor>( obj );
    } else if (type == "consume_drug") {
        return load_actor<consume_drug_iuse>( obj );
    } else if( type == "place_monster" ) {
        return load_actor<place_monster_iuse>( obj );
    } else if( type == "ups_based_armor" ) {
        return load_actor<ups_based_armor_actor>( obj );
    } else if( type == "reveal_map" ) {
        return load_actor<reveal_map_actor>( obj );
    } else {
        obj.throw_error( "unknown use_action", "type" );
        return use_function(); // line above throws, but the compiler does not know \-:
    }
}

use_function Item_factory::use_from_string(std::string function_name)
{
    std::map<Item_tag, use_function>::iterator found_function = iuse_function_list.find(function_name);

    //Before returning, make sure sure the function actually exists
    if (found_function != iuse_function_list.end()) {
        return found_function->second;
    } else {
        //Otherwise, return a hardcoded function we know exists (hopefully)
        debugmsg("Received unrecognized iuse function %s, using iuse::none instead", function_name.c_str());
        return use_function();
    }
}

void Item_factory::set_flag_by_string(std::bitset<num_bp> &cur_flags, const std::string &new_flag,
                                      const std::string &flag_type)
{
    if (flag_type == "bodyparts") {
        // global defined in bodypart.h
        if (new_flag == "ARM" || new_flag == "HAND" || new_flag == "LEG" || new_flag == "FOOT") {
            return;
        } else if (new_flag == "ARMS" || new_flag == "HANDS" || new_flag == "LEGS" || new_flag == "FEET") {
            std::vector<std::string> parts;
            if (new_flag == "ARMS") {
                parts.push_back("ARM_L");
                parts.push_back("ARM_R");
            } else if (new_flag == "HANDS") {
                parts.push_back("HAND_L");
                parts.push_back("HAND_R");
            } else if (new_flag == "LEGS") {
                parts.push_back("LEG_L");
                parts.push_back("LEG_R");
            } else if (new_flag == "FEET") {
                parts.push_back("FOOT_L");
                parts.push_back("FOOT_R");
            }
            for( auto &part : parts ) {
                std::map<std::string, body_part>::const_iterator found_flag_iter =
                    body_parts.find( part );
                if (found_flag_iter != body_parts.end()) {
                    cur_flags.set(found_flag_iter->second);
                } else {
                    debugmsg("Invalid item bodyparts flag: %s", new_flag.c_str());
                }
            }
        } else {
            std::map<std::string, body_part>::const_iterator found_flag_iter = body_parts.find(new_flag);
            if (found_flag_iter != body_parts.end()) {
                cur_flags.set(found_flag_iter->second);
            } else {
                debugmsg("Invalid item bodyparts flag: %s", new_flag.c_str());
            }
        }
    } else if (flag_type == "sided") {
        // global defined in bodypart.h
        if (new_flag == "ARM" || new_flag == "HAND" || new_flag == "LEG" || new_flag == "FOOT") {
            std::vector<std::string> parts;
            if (new_flag == "ARM") {
                parts.push_back("ARM_L");
                parts.push_back("ARM_R");
            } else if (new_flag == "HAND") {
                parts.push_back("HAND_L");
                parts.push_back("HAND_R");
            } else if (new_flag == "LEG") {
                parts.push_back("LEG_L");
                parts.push_back("LEG_R");
            } else if (new_flag == "FOOT") {
                parts.push_back("FOOT_L");
                parts.push_back("FOOT_R");
            }
            for( auto &part : parts ) {
                std::map<std::string, body_part>::const_iterator found_flag_iter =
                    body_parts.find( part );
                if (found_flag_iter != body_parts.end()) {
                    cur_flags.set(found_flag_iter->second);
                } else {
                    debugmsg("Invalid item bodyparts flag: %s", new_flag.c_str());
                }
            }
        }
    }

}

phase_id Item_factory::phase_from_tag(Item_tag name)
{
    if (name == "liquid") {
        return LIQUID;
    } else if (name == "solid") {
        return SOLID;
    } else if (name == "gas") {
        return GAS;
    } else if (name == "plasma") {
        return PLASMA;
    } else {
        return PNULL;
    }
};

void Item_factory::set_intvar(std::string tag, unsigned int &var, int min, int max)
{
    if (tag.size() > 6 && tag.substr(0, 6) == "LIGHT_") {
        int candidate = atoi(tag.substr(6).c_str());
        if (candidate >= min && candidate <= max) {
            var = candidate;
        }
    }
}

const item_category *Item_factory::get_category(const std::string &id)
{
    const CategoryMap::const_iterator a = m_categories.find(id);
    if (a != m_categories.end()) {
        return &a->second;
    }
    // Unknown / invalid category id, improvise and make this
    // a new category with at least a name.
    item_category &cat = m_categories[id];
    cat.id = id;
    cat.name = id;
    return &cat;
}

const use_function *Item_factory::get_iuse(const std::string &id)
{
    return &iuse_function_list.at(id);
}

const std::string &Item_factory::calc_category( const itype *it )
{
    if (it->is_gun()) {
        return category_id_guns;
    }
    if (it->is_ammo()) {
        return category_id_ammo;
    }
    if (it->is_tool()) {
        return category_id_tools;
    }
    if( it->armor ) {
        return category_id_clothing;
    }
    if (it->is_food()) {
        const it_comest *comest = dynamic_cast<const it_comest *>( it );
        return (comest->comesttype == "MED" ? category_id_drugs : category_id_food);
    }
    if( it->book ) {
        return category_id_books;
    }
    if (it->is_gunmod()) {
        return category_id_mods;
    }
    if (it->is_bionic()) {
        return category_id_cbm;
    }
    if (it->melee_dam > 7 || it->melee_cut > 5) {
        return category_id_weapons;
    }
    return category_id_other;
}

std::vector<Group_tag> Item_factory::get_all_group_names()
{
    std::vector<std::string> rval;
    GroupMap::iterator it;
    for (it = m_template_groups.begin(); it != m_template_groups.end(); it++) {
        rval.push_back(it->first);
    }
    return rval;
}

bool Item_factory::add_item_to_group(const Group_tag group_id, const Item_tag item_id,
                                     int chance)
{
    if (m_template_groups.find(group_id) == m_template_groups.end()) {
        return false;
    }
    Item_spawn_data *group_to_access = m_template_groups[group_id];
    if (group_to_access->has_item(item_id)) {
        group_to_access->remove_item(item_id);
    }

    Item_group *ig = dynamic_cast<Item_group *>(group_to_access);
    if (chance != 0 && ig != NULL) {
        // Only re-add if chance != 0
        ig->add_item_entry(item_id, chance);
    }

    return true;
}

void item_group::debug_spawn()
{
    std::vector<std::string> groups = item_controller->get_all_group_names();
    uimenu menu;
    menu.text = _("Test which group?");
    for (size_t i = 0; i < groups.size(); i++) {
        menu.entries.push_back(uimenu_entry(i, true, -2, groups[i]));
    }
    //~ Spawn group menu: Menu entry to exit menu
    menu.entries.push_back(uimenu_entry(menu.entries.size(), true, -2, _("cancel")));
    while (true) {
        menu.query();
        const size_t index = menu.ret;
        if (index >= groups.size()) {
            break;
        }
        // Spawn items from the group 100 times
        std::map<std::string, int> itemnames;
        for (size_t a = 0; a < 100; a++) {
            const auto items = items_from( groups[index], calendar::turn );
            for( auto &it : items ) {
                itemnames[it.display_name()]++;
            }
        }
        // Invert the map to get sorting!
        std::multimap<int, std::string> itemnames2;
        for (const auto &e : itemnames) {
            itemnames2.insert(std::pair<int, std::string>(e.second, e.first));
        }
        uimenu menu2;
        menu2.text = _("Result of 100 spawns:");
        for (const auto &e : itemnames2) {
            std::ostringstream buffer;
            buffer << e.first << " x " << e.second << "\n";
            menu2.entries.push_back(uimenu_entry(menu2.entries.size(), true, -2, buffer.str()));
        }
        menu2.query();
    }
}

std::vector<Item_tag> Item_factory::get_all_itype_ids() const
{
    std::vector<Item_tag> result;
    result.reserve( m_templates.size() );
    for( auto & p : m_templates ) {
        result.push_back( p.first );
    }
    return result;
}

const std::map<Item_tag, itype *> &Item_factory::get_all_itypes() const
{
    return m_templates;
}

Item_tag Item_factory::create_artifact_id() const
{
    Item_tag id;
    int i = m_templates.size();
    do {
        id = string_format( "artifact_%d", i );
        i++;
    } while( has_template( id ) );
    return id;
}
