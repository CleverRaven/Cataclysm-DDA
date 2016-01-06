#include "item_factory.h"
#include "enums.h"
#include "json.h"
#include "addiction.h"
#include "translations.h"
#include "item_group.h"
#include "crafting.h"
#include "recipe_dictionary.h"
#include "iuse_actor.h"
#include "item.h"
#include "mapdata.h"
#include "debug.h"
#include "construction.h"
#include "text_snippets.h"
#include "ui.h"
#include "skill.h"
#include "bionics.h"
#include "material.h"
#include "artifact.h"
#include "veh_type.h"

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

typedef std::set<std::string> t_string_set;
static t_string_set item_blacklist;
static t_string_set item_whitelist;

std::unique_ptr<Item_factory> item_controller( new Item_factory() );

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
        recipe_dict.delete_if( [&]( recipe &r ) {
            return r.result == itm || r.requirements.remove_item( itm );
        } );

        remove_construction_if([&](construction &c) {
            return c.requirements.remove_item(itm);
        });
    }

    for( auto &vid : vehicle_prototype::get_all() ) {
        vehicle_prototype &prototype = const_cast<vehicle_prototype&>( vid.obj() );
        for( vehicle_item_spawn &vis : prototype.item_spawns ) {
            auto &vec = vis.item_ids;
            const auto iter = std::remove_if( vec.begin(), vec.end(), item_is_blacklisted );
            vec.erase( iter, vec.end() );
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
    iuse_function_list["CAFF"] = &iuse::caff;
    iuse_function_list["ATOMIC_CAFF"] = &iuse::atomic_caff;
    iuse_function_list["ALCOHOL"] = &iuse::alcohol_medium;
    iuse_function_list["ALCOHOL_WEAK"] = &iuse::alcohol_weak;
    iuse_function_list["ALCOHOL_STRONG"] = &iuse::alcohol_strong;
    iuse_function_list["XANAX"] = &iuse::xanax;
    iuse_function_list["SMOKING"] = &iuse::smoking;
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
    iuse_function_list["FLU_VACCINE"] = &iuse::flu_vaccine;
    iuse_function_list["POISON"] = &iuse::poison;
    iuse_function_list["FUN_HALLU"] = &iuse::fun_hallu;
    iuse_function_list["MEDITATE"] = &iuse::meditate;
    iuse_function_list["THORAZINE"] = &iuse::thorazine;
    iuse_function_list["PROZAC"] = &iuse::prozac;
    iuse_function_list["SLEEP"] = &iuse::sleep;
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
    iuse_function_list["CAPTURE_MONSTER_ACT"] = &iuse::capture_monster_act;
    // TOOLS
    iuse_function_list["SEW_ADVANCED"] = &iuse::sew_advanced;
    iuse_function_list["EXTRA_BATTERY"] = &iuse::extra_battery;
    iuse_function_list["DOUBLE_REACTOR"] = &iuse::double_reactor;
    iuse_function_list["RECHARGEABLE_BATTERY"] = &iuse::rechargeable_battery;
    iuse_function_list["EXTINGUISHER"] = &iuse::extinguisher;
    iuse_function_list["HAMMER"] = &iuse::hammer;
    iuse_function_list["DIRECTIONAL_ANTENNA"] = &iuse::directional_antenna;
    iuse_function_list["WATER_PURIFIER"] = &iuse::water_purifier;
    iuse_function_list["TWO_WAY_RADIO"] = &iuse::two_way_radio;
    iuse_function_list["RADIO_OFF"] = &iuse::radio_off;
    iuse_function_list["RADIO_ON"] = &iuse::radio_on;
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
    iuse_function_list["JACKHAMMER"] = &iuse::jackhammer;
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
    iuse_function_list["LAW"] = &iuse::LAW;
    iuse_function_list["HEATPACK"] = &iuse::heatpack;
    iuse_function_list["QUIVER"] = &iuse::quiver;
    iuse_function_list["TOWEL"] = &iuse::towel;
    iuse_function_list["UNFOLD_GENERIC"] = &iuse::unfold_generic;
    iuse_function_list["ADRENALINE_INJECTOR"] = &iuse::adrenaline_injector;
    iuse_function_list["JET_INJECTOR"] = &iuse::jet_injector;
    iuse_function_list["STIMPACK"] = &iuse::stimpack;
    iuse_function_list["CONTACTS"] = &iuse::contacts;
    iuse_function_list["HOTPLATE"] = &iuse::hotplate;
    iuse_function_list["DOLLCHAT"] = &iuse::talking_doll;
    iuse_function_list["BELL"] = &iuse::bell;
    iuse_function_list["SEED"] = &iuse::seed;
    iuse_function_list["OXYGEN_BOTTLE"] = &iuse::oxygen_bottle;
    iuse_function_list["ATOMIC_BATTERY"] = &iuse::atomic_battery;
    iuse_function_list["UPS_BATTERY"] = &iuse::ups_battery;
    iuse_function_list["RADIO_MOD"] = &iuse::radio_mod;
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
    iuse_function_list["SHAVEKIT"]  = &iuse::shavekit;
    iuse_function_list["HAIRKIT"]  = &iuse::hairkit;
    iuse_function_list["WEATHER_TOOL"] = &iuse::weather_tool;
    iuse_function_list["REMOVE_ALL_MODS"] = &iuse::remove_all_mods;
    iuse_function_list["LADDER"] = &iuse::ladder;

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

    // An empty dummy group, it will not spawn anything. However, it makes that item group
    // id valid, so it can be used all over the place without need to explicitly check for it.
    m_template_groups["EMPTY_GROUP"] = new Item_group( Item_group::G_COLLECTION, 100 );
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
    add_category(category_id_guns, -21, _("GUNS"));
    add_category(category_id_ammo, -20, _("AMMO"));
    add_category(category_id_weapons, -19, _("WEAPONS"));
    add_category(category_id_tools, -18, _("TOOLS"));
    add_category(category_id_clothing, -17, _("CLOTHING"));
    add_category(category_id_food, -16, _("FOOD"));
    add_category(category_id_drugs, -15, _("DRUGS"));
    add_category(category_id_books, -14, _("BOOKS"));
    add_category(category_id_mods, -13, _("MODS"));
    add_category(category_id_cbm, -12, _("BIONICS"));
    add_category(category_id_mutagen, -11, _("MUTAGEN"));
    add_category(category_id_other, -10, _("OTHER"));
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

/**
 * Checks that ammo type is fake type or not.
 * @param ammo type for check.
 * @return true if ammo type is a fake, false otherwise.
 */
static bool fake_ammo_type(const std::string &ammo)
{
    if (  ammo == "NULL" || ammo == "generic_no_ammo" ||
          ammo == "pointer_fake_ammo" ) {
        return true;
    }
    return false;
}

void Item_factory::check_ammo_type(std::ostream &msg, const std::string &ammo) const
{
    // Skip fake types
    if ( fake_ammo_type(ammo) ) {
        return;
    }

    // Should be skipped too.
    if ( ammo == "UPS" ) {
        return;
    }

    // Check for valid ammo type name.
    if (ammo_name(ammo) == "none") {
        msg << string_format("ammo type %s not listed in ammo_name() function", ammo.c_str()) << "\n";
    }

    // Search ammo type.
    for( const auto &elem : m_templates ) {
        const auto ammot = elem.second;
        if( !ammot->ammo ) {
            continue;
        }
        if( ammot->ammo->type == ammo ) {
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
            if( !_a.is_valid() ) {
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
        if( type->spawn ) {
            if( type->spawn->default_container != "null" && !has_template( type->spawn->default_container ) ) {
                msg << string_format( "invalid container property %s", type->spawn->default_container.c_str() ) << "\n";
            }
        }
        const it_comest *comest = dynamic_cast<const it_comest *>(type);
        if (comest != 0) {
            if (comest->tool != "null" && !has_template(comest->tool)) {
                msg << string_format("invalid tool property %s", comest->tool.c_str()) << "\n";
            }
        }
        if( type->seed ) {
            if( !has_template( type->seed->fruit_id ) ) {
                msg << string_format( "invalid fruit id %s", type->seed->fruit_id.c_str() ) << "\n";
            }
            for( auto & b : type->seed->byproducts ) {
                if( !has_template( b ) ) {
                    msg << string_format( "invalid byproduct id %s", b.c_str() ) << "\n";
                }
            }
        }
        if( type->book ) {
            if( type->book->skill && !type->book->skill.is_valid() ) {
                msg << string_format("uses invalid book skill.") << "\n";
            }
        }
        if( type->ammo ) {
            check_ammo_type( msg, type->ammo->type );
            if( type->ammo->casing != "NULL" && !has_template( type->ammo->casing ) ) {
                msg << string_format( "invalid casing property %s", type->ammo->casing.c_str() ) << "\n";
            }
        }
        if( type->gun ) {
            check_ammo_type( msg, type->gun->ammo );
            if( !type->gun->skill_used ) {
                msg << string_format("uses no skill") << "\n";
            } else if( !type->gun->skill_used.is_valid() ) {
                msg << "uses an invalid skill " << type->gun->skill_used.str() << "\n";
            }
            if( type->item_tags.count( "BURST_ONLY" ) > 0 && type->item_tags.count( "MODE_BURST" ) < 1 ) {
                msg << string_format("has BURST_ONLY but no MODE_BURST") << "\n";
            }
            for( auto &gm : type->gun->default_mods ){
                if( !has_template( gm ) ){
                    msg << string_format("invalid default mod.") << "\n";
                }
            }
            for( auto &gm : type->gun->built_in_mods ){
                if( !has_template( gm ) ){
                    msg << string_format("invalid built-in mod.") << "\n";
                }
            }
        }
        if( type->gunmod ) {
            check_ammo_type( msg, type->gunmod->newtype );
            if( type->gunmod->skill_used && !type->gunmod->skill_used.is_valid() ) {
                msg << string_format("uses invalid gunmod skill.") << "\n";
            }
        }
        const it_tool *tool = dynamic_cast<const it_tool *>(type);
        if (tool != 0) {
            check_ammo_type(msg, tool->ammo_id);
            if (tool->revert_to != "null" && !has_template(tool->revert_to)) {
                msg << string_format("invalid revert_to property %s", tool->revert_to.c_str()) << "\n";
            }
        }
        if( type->bionic ) {
            if (!is_valid_bionic(type->bionic->bionic_id)) {
                msg << string_format("there is no bionic with id %s", type->bionic->bionic_id.c_str()) << "\n";
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
    if( !slotptr ) {
        slotptr.reset( new SlotType() );
    }
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

template<typename E>
void load_optional_enum_array( std::vector<E> &vec, JsonObject &jo, const std::string &member )
{

    if( !jo.has_member( member ) ) {
        return;
    } else if( !jo.has_array( member ) ) {
        jo.throw_error( "expected array", member );
    }

    JsonIn &stream = *jo.get_raw( member );
    stream.start_array();
    while( !stream.end_array() ) {
        vec.push_back( stream.get_enum_value<E>() );
    }
}

void Item_factory::load( islot_artifact &slot, JsonObject &jo )
{
    slot.charge_type = jo.get_enum_value( "charge_type", ARTC_NULL );
    load_optional_enum_array( slot.effects_wielded, jo, "effects_wielded" );
    load_optional_enum_array( slot.effects_activated, jo, "effects_activated" );
    load_optional_enum_array( slot.effects_carried, jo, "effects_carried" );
    load_optional_enum_array( slot.effects_worn, jo, "effects_worn" );
}

void Item_factory::load( islot_software &slot, JsonObject &jo )
{
    slot.type = jo.get_string( "type" );
    slot.power = jo.get_int( "power" );
}

void Item_factory::load( islot_ammo &slot, JsonObject &jo )
{
    slot.type = jo.get_string( "ammo_type" );
    slot.casing = jo.get_string( "casing", "NULL" );
    slot.damage = jo.get_int( "damage" );
    slot.pierce = jo.get_int( "pierce", 0 );
    slot.range = jo.get_int( "range" );
    slot.dispersion = jo.get_int( "dispersion" );
    slot.recoil = jo.get_int( "recoil" );
    slot.def_charges = jo.get_long( "count" );
    slot.ammo_effects = jo.get_tags( "effects" );
}

void Item_factory::load_ammo(JsonObject &jo)
{
    itype *new_item_template = new itype();
    load_slot( new_item_template->ammo, jo );
    new_item_template->stack_size = jo.get_int( "stack_size", new_item_template->ammo->def_charges );
    load_basic_info( jo, new_item_template );
    load_slot( new_item_template->spawn, jo );
}

void Item_factory::load( islot_gun &slot, JsonObject &jo )
{
    slot.ammo = jo.get_string( "ammo" );
    slot.skill_used = skill_id( jo.get_string( "skill" ) );
    slot.loudness = jo.get_int( "loudness", 0 );
    slot.damage = jo.get_int( "ranged_damage", 0 );
    slot.range = jo.get_int( "range", 0 );
    slot.dispersion = jo.get_int( "dispersion" );
    slot.sight_dispersion = jo.get_int("sight_dispersion");
    slot.aim_speed = jo.get_int("aim_speed");
    slot.recoil = jo.get_int( "recoil" );
    slot.durability = jo.get_int( "durability" );
    slot.burst = jo.get_int( "burst", 0 );
    slot.clip = jo.get_int( "clip_size" );
    slot.reload_time = jo.get_int( "reload" );
    slot.reload_noise = jo.get_string( "reload_noise", _ ("click.") );
    slot.reload_noise_volume = jo.get_int( "reload_noise_volume", -1 );
    slot.pierce = jo.get_int( "pierce", 0 );
    slot.ammo_effects = jo.get_tags( "ammo_effects" );
    slot.ups_charges = jo.get_int( "ups_charges", 0 );

    if( jo.has_array( "valid_mod_locations" ) ) {
        JsonArray jarr = jo.get_array( "valid_mod_locations" );
        while( jarr.has_more() ) {
            JsonArray curr = jarr.next_array();
            slot.valid_mod_locations.insert( std::pair<std::string, int>( curr.get_string( 0 ),
                                             curr.get_int( 1 ) ) );
        }
    }

    //Add any built-in mods.
    if( jo.has_array( "built_in_mods" ) ) {
    JsonArray jarr = jo.get_array( "built_in_mods" );
        while( jarr.has_more() ) {
            std::string temp = jarr.next_string();
            slot.built_in_mods.push_back( temp );
        }
    }

    //Add default
    if( jo.has_array( "default_mods" ) ) {
    JsonArray jarr = jo.get_array( "default_mods" );
        while( jarr.has_more() ) {
            std::string temp = jarr.next_string();
            slot.default_mods.push_back( temp );
        }
    }

}

void Item_factory::load( islot_spawn &slot, JsonObject &jo )
{
    if( jo.has_array( "rand_charges" ) ) {
        JsonArray jarr = jo.get_array( "rand_charges" );
        while( jarr.has_more() ) {
            slot.rand_charges.push_back( jarr.next_long() );
        }
        if( slot.rand_charges.size() == 1 ) {
            // see item::item(...) for the use of this array
            jarr.throw_error( "a rand_charges array with only one entry will be ignored, it needs at least 2 entries!" );
        }
    }
    slot.default_container = jo.get_string( "container", slot.default_container );
}

void Item_factory::load_gun(JsonObject &jo)
{
    itype* new_item_template = new itype();
    load_slot( new_item_template->gun, jo );
    load_basic_info( jo, new_item_template );
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
    slot.env_resist = jo.get_int( "environmental_protection", 0 );
    slot.warmth = jo.get_int( "warmth", 0 );
    slot.storage = jo.get_int( "storage", 0 );
    slot.power_armor = jo.get_bool( "power_armor", false );
    slot.covers = jo.has_member( "covers" ) ? flags_from_json( jo, "covers", "bodyparts" ) : 0;

    auto ja = jo.get_array("covers");
    while (ja.has_more()) {
        if (ja.next_string().find("_EITHER") != std::string::npos) {
            slot.sided = true;
            break;
        }
    }
}

void Item_factory::load_tool(JsonObject &jo)
{
    it_tool *tool_template = new it_tool();
    tool_template->ammo_id = jo.get_string("ammo");
    tool_template->max_charges = jo.get_long("max_charges");
    tool_template->def_charges = jo.get_long("initial_charges");
    tool_template->charges_per_use = jo.get_int("charges_per_use");
    tool_template->turns_per_charge = jo.get_int("turns_per_charge");
    tool_template->revert_to = jo.get_string("revert_to");
    tool_template->subtype = jo.get_string("sub", "");

    itype *new_item_template = tool_template;
    load_basic_info(jo, new_item_template);
    load_slot( new_item_template->spawn, jo );
}

void Item_factory::load_tool_armor(JsonObject &jo)
{
    it_tool *tool_template = new it_tool();

    tool_template->ammo_id = jo.get_string("ammo");
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
    slot.skill = skill_id( jo.get_string( "skill" ) );
    slot.chapters = jo.get_int( "chapters", -1 );
    set_use_methods_from_json( jo, "use_action", slot.use_methods );
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
    comest_template->quench = jo.get_int("quench", 0);
    comest_template->nutr = jo.get_int("nutrition", 0);
    comest_template->spoils = jo.get_int("spoils_in", 0);
    // In json it's in hours, here it shall be in turns, as item::rot is also in turns.
    comest_template->spoils *= 600;
    comest_template->brewtime = jo.get_int("brew_time", 0);
    comest_template->addict = jo.get_int("addiction_potential", 0);
    comest_template->def_charges = jo.get_long("charges", 0);
    if (jo.has_member("stack_size")) {
        comest_template->stack_size = jo.get_long("stack_size");
    } else {
        comest_template->stack_size = comest_template->def_charges;
    }
    comest_template->stim = jo.get_int("stim", 0);
    comest_template->healthy = jo.get_int("healthy", 0);
    comest_template->fun = jo.get_int("fun", 0);

    comest_template->add = addiction_type(jo.get_string("addiction_type"));

    itype *new_item_template = comest_template;
    load_basic_info(jo, new_item_template);
    load_slot( new_item_template->spawn, jo );
}

void Item_factory::load_container(JsonObject &jo)
{
    itype *new_item_template = new itype();
    load_slot( new_item_template->container, jo );
    load_basic_info( jo, new_item_template );
}

void Item_factory::load( islot_seed &slot, JsonObject &jo )
{
    slot.grow = jo.get_int( "grow" );
    slot.fruit_div = jo.get_int( "fruit_div", 1 );
    slot.plant_name = _( jo.get_string( "plant_name" ).c_str() );
    slot.fruit_id = jo.get_string( "fruit" );
    slot.spawn_seeds = jo.get_bool( "seeds", true );
    slot.byproducts = jo.get_string_array( "byproducts" );
}

void Item_factory::load( islot_container &slot, JsonObject &jo )
{
    slot.contains = jo.get_int( "contains" );
    slot.seals = jo.get_bool( "seals", false );
    slot.watertight = jo.get_bool( "watertight", false );
    slot.preserves = jo.get_bool( "preserves", false );
    slot.rigid = jo.get_bool( "rigid", false );
}

void Item_factory::load( islot_gunmod &slot, JsonObject &jo )
{
    slot.damage = jo.get_int( "damage_modifier", 0 );
    slot.loudness = jo.get_int( "loudness_modifier", 0 );
    slot.newtype = jo.get_string( "ammo_modifier", "NULL" );
    slot.location = jo.get_string( "location" );
    // TODO: implement loading this from json (think of a proper name)
    // slot.pierce = jo.get_string( "mod_pierce", 0 );
    slot.used_on_pistol = is_mod_target( jo, "mod_targets", "pistol" );
    slot.used_on_shotgun = is_mod_target( jo, "mod_targets", "shotgun" );
    slot.used_on_smg = is_mod_target( jo, "mod_targets", "smg" );
    slot.used_on_rifle = is_mod_target( jo, "mod_targets", "rifle" );
    slot.used_on_bow = is_mod_target( jo, "mod_targets", "bow" );
    slot.used_on_crossbow = is_mod_target( jo, "mod_targets", "crossbow" );
    slot.used_on_launcher = is_mod_target( jo, "mod_targets", "launcher" );
    slot.dispersion = jo.get_int( "dispersion_modifier", 0 );
    slot.sight_dispersion = jo.get_int( "sight_dispersion", -1 );
    slot.aim_speed = jo.get_int( "aim_speed", -1 );
    slot.recoil = jo.get_int( "recoil_modifier", 0 );
    slot.burst = jo.get_int( "burst_modifier", 0 );
    slot.range = jo.get_int( "range", 0 );
    slot.clip = jo.get_int( "clip_size_modifier", 0 );
    slot.acceptable_ammo_types = jo.get_tags( "acceptable_ammo" );
    slot.skill_used = skill_id( jo.get_string( "skill", "gun" ) );
    slot.req_skill = jo.get_int( "skill_required", 0 );
    slot.ups_charges = jo.get_int( "ups_charges", 0 );
}

void Item_factory::load_gunmod(JsonObject &jo)
{
    itype *new_item_template = new itype();
    load_slot( new_item_template->gunmod, jo );
    load_basic_info( jo, new_item_template );
}

void Item_factory::load( islot_bionic &slot, JsonObject &jo )
{
    slot.difficulty = jo.get_int( "difficulty" );
    // TODO: must be the same as the item type id, for compatibility
    slot.bionic_id = jo.get_string( "id" );
}

void Item_factory::load_bionic( JsonObject &jo )
{
    itype *new_item_template = new itype();
    load_slot( new_item_template->bionic, jo );
    load_basic_info( jo, new_item_template );
}

void Item_factory::load( islot_variable_bigness &slot, JsonObject &jo )
{
    slot.min_bigness = jo.get_int( "min-bigness" );
    slot.max_bigness = jo.get_int( "max-bigness" );
    const std::string big_aspect = jo.get_string( "bigness-aspect" );
    if( big_aspect == "WHEEL_DIAMETER" ) {
        slot.bigness_aspect = BIGNESS_WHEEL_DIAMETER;
    } else if( big_aspect == "ENGINE_DISPLACEMENT" ) {
        slot.bigness_aspect = BIGNESS_ENGINE_DISPLACEMENT;
    } else {
        jo.throw_error( "invalid bigness-aspect", "bigness-aspect" );
    }
}

void Item_factory::load_veh_part(JsonObject &jo)
{
    itype *new_item_template = new itype();
    load_slot( new_item_template->variable_bigness, jo );
    load_basic_info( jo, new_item_template );
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
    new_item_template->price = jo.get_int( "price" );
    new_item_template->name = jo.get_string( "name" ).c_str();
    if( jo.has_member( "name_plural" ) ) {
        new_item_template->name_plural = jo.get_string( "name_plural" ).c_str();
    } else {
        // default behavior: Assume the regular plural form (appending an “s”)
        new_item_template->name_plural = ( jo.get_string( "name" ) + "s" ).c_str();
    }
    new_item_template->sym = jo.get_string( "symbol" )[0];
    new_item_template->color = color_from_string( jo.get_string( "color" ) );
    std::string temp_desc;
    temp_desc = jo.get_string( "description" );
    if( !temp_desc.empty() ) {
        new_item_template->description = _( jo.get_string( "description" ).c_str() );
    } else {
        new_item_template->description = "";
    }
    if( jo.has_member( "material" ) ) {
        set_material_from_json( jo, "material", new_item_template );
    } else {
        new_item_template->materials.push_back( "null" );
    }
    new_item_template->phase = jo.get_enum_value( "phase", SOLID );
    new_item_template->volume = jo.get_int( "volume" );
    new_item_template->weight = jo.get_int( "weight" );
    new_item_template->melee_dam = jo.get_int( "bashing", 0 );
    new_item_template->melee_cut = jo.get_int( "cutting", 0 );
    new_item_template->m_to_hit = jo.get_int( "to_hit" );

    new_item_template->min_str = jo.get_int( "min_strength",     0 );
    new_item_template->min_dex = jo.get_int( "min_dexterity",    0 );
    new_item_template->min_int = jo.get_int( "min_intelligence", 0 );
    new_item_template->min_per = jo.get_int( "min_perception",   0 );

    JsonArray jarr = jo.get_array( "min_skills" );
    while( jarr.has_more() ) {
        JsonArray cur = jarr.next_array();
        new_item_template->min_skills[skill_id( cur.get_string( 0 ) )] = cur.get_int( 1 );
    }

    if (jo.has_member("explode_in_fire")) {
        JsonObject je = jo.get_object("explode_in_fire");
        je.read("power", new_item_template->explosion_on_fire_data.power);
        je.read("distance_factor", new_item_template->explosion_on_fire_data.distance_factor);
        je.read("shrapnel", new_item_template->explosion_on_fire_data.shrapnel);
        je.read("fire", new_item_template->explosion_on_fire_data.fire);
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
    FIT - Reduces encumbrance by ten
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

    if (jo.has_member("properties")) {
        set_properties_from_json(jo, "properties", new_item_template);
    }

    for( auto & s : jo.get_tags( "techniques" ) ) {
        new_item_template->techniques.insert( matec_id( s ) );
    }

    set_use_methods_from_json( jo, "use_action", new_item_template->use_methods );

    if (jo.has_member("category")) {
        new_item_template->category = get_category(jo.get_string("category"));
    } else {
        new_item_template->category = get_category(calc_category(new_item_template));
    }

    load_slot_optional( new_item_template->container, jo, "container_data" );
    load_slot_optional( new_item_template->armor, jo, "armor_data" );
    load_slot_optional( new_item_template->book, jo, "book_data" );
    load_slot_optional( new_item_template->gun, jo, "gun_data" );
    load_slot_optional( new_item_template->gunmod, jo, "gunmod_data" );
    load_slot_optional( new_item_template->bionic, jo, "bionic_data" );
    load_slot_optional( new_item_template->spawn, jo, "spawn_data" );
    load_slot_optional( new_item_template->ammo, jo, "ammo_data" );
    load_slot_optional( new_item_template->seed, jo, "seed_data" );
    load_slot_optional( new_item_template->software, jo, "software_data" );
    load_slot_optional( new_item_template->artifact, jo, "artifact_data" );
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

void Item_factory::set_properties_from_json(JsonObject &jo, std::string member,
        itype *new_item_template)
{
    if (jo.has_array(member)) {
        JsonArray jarr = jo.get_array(member);
        while (jarr.has_more()) {
            JsonArray curr = jarr.next_array();
            const auto prop = std::pair<std::string, std::string>(curr.get_string(0), curr.get_string(1));
            if( new_item_template->properties.count( prop.first ) > 0 ) {
                curr.throw_error( "Duplicated property", 0 );
            }
            new_item_template->properties.insert( prop );
        }
    } else {
        jo.throw_error( "Properties list is not an array", member );
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
        throw std::runtime_error("item group already defined with different type");
    }
    return ig;
}

template<typename T>
bool load_min_max(std::pair<T, T> &pa, JsonObject &obj, const std::string &name)
{
    bool result = false;
    if( obj.has_array( name ) ) {
        // An array means first is min, second entry is max. Both are mandatory.
        JsonArray arr = obj.get_array( name );
        result |= arr.read_next( pa.first );
        result |= arr.read_next( pa.second );
    } else {
        // Not an array, should be a single numeric value, which is set as min and max.
        result |= obj.read( name, pa.first );
        result |= obj.read( name, pa.second );
    }
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

void Item_factory::load_item_group_entries( Item_group& ig, JsonArray& entries )
{
    while( entries.has_more() ) {
        JsonObject subobj = entries.next_object();
        add_entry( &ig, subobj );
    }
}

void Item_factory::load_item_group( JsonArray &entries, const Group_tag &group_id,
                                    const bool is_collection )
{
    const auto type = is_collection ? Item_group::G_COLLECTION : Item_group::G_DISTRIBUTION;
    Item_spawn_data *&isd = m_template_groups[group_id];
    Item_group* const ig = make_group_or_throw( isd, type );

    load_item_group_entries( *ig, entries );
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
        load_item_group_entries( *ig, items );
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

void Item_factory::set_use_methods_from_json( JsonObject &jo, std::string member,
        std::vector<use_function> &use_methods )
{
    if( !jo.has_member( member ) ) {
        return;
    }
    if( jo.has_array( member ) ) {
        JsonArray jarr = jo.get_array( member );
        while( jarr.has_more() ) {
            if( jarr.test_string() ) {
                use_methods.push_back( use_from_string( jarr.next_string() ) );
            } else if( jarr.test_object() ) {
                set_uses_from_object( jarr.next_object(), use_methods );
            } else {
                jarr.throw_error( "array element is neither string nor object." );
            }

        }
    } else {
        if( jo.has_string( member ) ) {
            use_methods.push_back( use_from_string( jo.get_string( member ) ) );
        } else if( jo.has_object( member ) ) {
            set_uses_from_object( jo.get_object( member ), use_methods );
        } else {
            jo.throw_error( "member 'use_action' is neither string nor object." );
        }

    }
}

template<typename IuseActorType>
use_function load_actor( JsonObject obj )
{
    std::unique_ptr<IuseActorType> actor( new IuseActorType() );
    actor->type = obj.get_string("type");
    actor->load( obj );
    return use_function( actor.release() );
}

template<typename IuseActorType>
use_function load_actor( JsonObject obj, const std::string &type )
{
    std::unique_ptr<IuseActorType> actor( new IuseActorType() );
    actor->type = type;
    actor->load( obj );
    return use_function( actor.release() );
}

void Item_factory::set_uses_from_object(JsonObject obj, std::vector<use_function> &use_methods)
{
    const std::string type = obj.get_string("type");
    use_function newfun;
    if (type == "transform") {
        newfun = load_actor<iuse_transform>( obj );
    } else if (type == "auto_transform") {
        newfun = load_actor<auto_iuse_transform>( obj );
    } else if (type == "delayed_transform") {
        newfun = load_actor<delayed_transform_iuse>( obj );
    } else if (type == "explosion") {
        newfun = load_actor<explosion_iuse>( obj );
    } else if (type == "unfold_vehicle") {
        newfun = load_actor<unfold_vehicle_iuse>( obj );
    } else if (type == "picklock") {
        newfun = load_actor<pick_lock_actor>( obj );
    } else if (type == "consume_drug") {
        newfun = load_actor<consume_drug_iuse>( obj );
    } else if( type == "place_monster" ) {
        newfun = load_actor<place_monster_iuse>( obj );
    } else if( type == "ups_based_armor" ) {
        newfun = load_actor<ups_based_armor_actor>( obj );
    } else if( type == "reveal_map" ) {
        newfun = load_actor<reveal_map_actor>( obj );
    } else if( type == "firestarter" ) {
        newfun = load_actor<firestarter_actor>( obj );
    } else if( type == "extended_firestarter" ) {
        newfun = load_actor<extended_firestarter_actor>( obj );
    } else if( type == "salvage" ) {
        newfun = load_actor<salvage_actor>( obj );
    } else if( type == "inscribe" ) {
        newfun = load_actor<inscribe_actor>( obj );
    } else if( type == "cauterize" ) {
        newfun = load_actor<cauterize_actor>( obj );
    } else if( type == "enzlave" ) {
        newfun = load_actor<enzlave_actor>( obj );
    } else if( type == "fireweapon_off" ) {
        newfun = load_actor<fireweapon_off_actor>( obj );
    } else if( type == "fireweapon_on" ) {
        newfun = load_actor<fireweapon_on_actor>( obj );
    } else if( type == "manualnoise" ) {
        newfun = load_actor<manualnoise_actor>( obj );
    } else if( type == "musical_instrument" ) {
        newfun = load_actor<musical_instrument_actor>( obj );
    } else if( type == "holster" ) {
        newfun = load_actor<holster_actor>( obj );
    } else if( type == "repair_item" ) {
        newfun = load_actor<repair_item_actor>( obj );
    } else if( type == "heal" ) {
        newfun = load_actor<heal_actor>( obj );
    } else if( type == "knife" ) {
        use_methods.push_back( load_actor<salvage_actor>( obj, "salvage" ) );
        use_methods.push_back( load_actor<inscribe_actor>( obj, "inscribe" ) );
        use_methods.push_back( load_actor<cauterize_actor>( obj, "cauterize" ) );
        use_methods.push_back( load_actor<enzlave_actor>( obj, "enzlave" ) );
        return;
    } else {
        obj.throw_error( "unknown use_action", "type" );
    }

    use_methods.push_back( newfun );
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
        if (new_flag == "ARMS" || new_flag == "ARM_EITHER") {
            cur_flags.set( bp_arm_l );
            cur_flags.set( bp_arm_r );
        } else if (new_flag == "HANDS" || new_flag == "HAND_EITHER") {
            cur_flags.set( bp_hand_l );
            cur_flags.set( bp_hand_r );
        } else if (new_flag == "LEGS" || new_flag == "LEG_EITHER") {
            cur_flags.set( bp_leg_l );
            cur_flags.set( bp_leg_r );
        } else if (new_flag == "FEET" || new_flag == "FOOT_EITHER") {
            cur_flags.set( bp_foot_l );
            cur_flags.set( bp_foot_r );
        } else {
            cur_flags.set( get_body_part_token( new_flag ) );
        }
    }
}

namespace io {
static const std::unordered_map<std::string, phase_id> phase_id_values = { {
    { "liquid", LIQUID },
    { "solid", SOLID },
    { "gas", GAS },
    { "plasma", PLASMA },
} };
template<>
phase_id string_to_enum<phase_id>( const std::string &data )
{
    return string_to_enum_look_up( phase_id_values, data );
}
} // namespace io

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
    const auto &iter = iuse_function_list.find( id );
    if( iter != iuse_function_list.end() ) {
        return &iter->second;
    }

    return nullptr;
}

const std::string &Item_factory::inverse_get_iuse( const use_function *fun )
{
    // Ugly and slow - compares values to get the key
    // Don't use when performance matters
    for( const auto &pr : iuse_function_list ) {
        if( pr.second == *fun ) {
            return pr.first;
        }
    }

    debugmsg( "Tried to get id of a function not in iuse_function_list" );
    const static std::string errstr("ERROR");
    return errstr;
}

const std::string &Item_factory::calc_category( const itype *it )
{
    if( it->gun && !it->gunmod ) {
        return category_id_guns;
    }
    if( it->ammo ) {
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
    if( it->gunmod ) {
        return category_id_mods;
    }
    if( it->bionic ) {
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
