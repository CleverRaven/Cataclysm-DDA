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
#include "init.h"
#include "game.h"

#include <algorithm>
#include <sstream>

extern class game *g;

static const std::string category_id_guns("guns");
static const std::string category_id_ammo("ammo");
static const std::string category_id_weapons("weapons");
static const std::string category_id_tools("tools");
static const std::string category_id_clothing("clothing");
static const std::string category_id_drugs("drugs");
static const std::string category_id_food("food");
static const std::string category_id_books("books");
static const std::string category_id_mods("mods");
static const std::string category_id_magazines("magazines");
static const std::string category_id_cbm("bionics");
static const std::string category_id_mutagen("mutagen");
static const std::string category_id_veh_parts("veh_parts");
static const std::string category_id_other("other");

typedef std::set<std::string> t_string_set;
static t_string_set item_blacklist;
static t_string_set item_whitelist;
static bool item_whitelist_is_exclusive = false;

std::unique_ptr<Item_factory> item_controller( new Item_factory() );

static void set_allergy_flags( itype &item_template );
static void hflesh_to_flesh( itype &item_template );

bool item_is_blacklisted(const std::string &id)
{
    if (item_whitelist.count(id) > 0) {
        return false;
    } else if (item_blacklist.count(id) > 0) {
        return true;
    }
    // Return true if the whitelist mode is exclusive and the whitelist is populated.
    return item_whitelist_is_exclusive && !item_whitelist.empty();
}

void Item_factory::finalize() {

    auto& dyn = DynamicDataLoader::get_instance();

    while( !deferred.empty() ) {
        auto n = deferred.size();
        auto it = deferred.begin();
        for( decltype(deferred)::size_type idx = 0; idx != n; ++idx ) {
            try {
                std::istringstream str( *it );
                JsonIn jsin( str );
                JsonObject jo = jsin.get_object();
                dyn.load_object( jo );
            } catch( const std::exception &err ) {
                debugmsg( "Error loading data from json: %s", err.what() );
            }
            ++it;
        }
        deferred.erase( deferred.begin(), it );
        if( deferred.size() == n ) {
            debugmsg( "JSON contains circular dependency: discarded %i templates", n );
            break;
        }
    }

    finalize_item_blacklist();

    for( auto& e : m_templates ) {
        itype& obj = *e.second;

        if( !obj.category ) {
            obj.category = get_category( calc_category( &obj ) );
        }

        // use pre-cataclysm price as default if post-cataclysm price unspecified
        if( obj.price_post < 0 ) {
            obj.price_post = obj.price;
        }
        // use base volume if integral volume unspecified
        if( obj.integral_volume < 0 ) {
            obj.integral_volume = obj.volume;
        }
        // for ammo and comestibles stack size defaults to count of initial charges
        if( obj.stack_size == 0 && ( obj.ammo || obj.comestible ) ) {
            obj.stack_size = obj.charges_default();
        }
        for( const auto &tag : obj.item_tags ) {
            if( tag.size() > 6 && tag.substr( 0, 6 ) == "LIGHT_" ) {
                obj.light_emission = std::max( atoi( tag.substr( 6 ).c_str() ), 0 );
            }
        }
        // for ammo not specifying loudness (or an explicit zero) derive value from other properties
        if( obj.ammo && obj.ammo->loudness < 0 ) {
            obj.ammo->loudness = std::max( std::max( { obj.ammo->damage, obj.ammo->pierce, obj.ammo->range } ) * 3,
                                           obj.ammo->recoil / 3 );
        }
        if( obj.gun ) {
            obj.gun->reload_noise = _( obj.gun->reload_noise.c_str() );
        }

        set_allergy_flags( *e.second );
        hflesh_to_flesh( *e.second );

        if( obj.comestible ) {
            if( g->has_option( "no_vitamins" ) ) {
                obj.comestible->vitamins.clear();

            } else if( obj.comestible->vitamins.empty() && obj.comestible->healthy >= 0 ) {
                // default vitamins of healthy comestibles to their edible base materials if none explicitly specified
                auto healthy = std::max( obj.comestible->healthy, 1 ) * 10;

                auto mat = obj.materials;
                mat.erase( std::remove_if( mat.begin(), mat.end(), []( const string_id<material_type> &m ) {
                    return !m.obj().edible(); // @todo migrate inedible comestibles to appropriate alternative types
                } ), mat.end() );

                // for comestibles composed of multiple edible materials we calculate the average
                for( const auto &v : vitamin::all() ) {
                    if( obj.comestible->vitamins.find( v.first ) == obj.comestible->vitamins.end() ) {
                        for( const auto &m : mat ) {
                            obj.comestible->vitamins[ v.first ] += ceil( m.obj().vitamin( v.first ) * healthy / mat.size() );
                        }
                    }
                }
            }
        }
    }
}

void Item_factory::finalize_item_blacklist()
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

    // Can't be part of the blacklist loop because the magazines might be
    // deleted before the guns are processed.
    const bool magazines_blacklisted = g->has_option( "blacklist_magazines" );

    if( magazines_blacklisted ) {
        for( auto& e : m_templates ) {
            if( !e.second->gun || e.second->magazines.empty() ) {
                continue;
            }

            // check_definitions() guarantees that defmag both exists and is a magazine
            itype *defmag = m_templates[ e.second->magazine_default[ e.second->gun->ammo ] ].get();
            e.second->volume += defmag->volume;
            e.second->weight += defmag->weight;
            e.second->magazines.clear();
            e.second->magazine_default.clear();
            e.second->magazine_well = 0;

            if( e.second->gun ) {
                e.second->gun->clip = defmag->magazine->capacity;
                e.second->gun->reload_time = defmag->magazine->capacity * defmag->magazine->reload_time;
            }
        }
    }

    for( auto &e : m_templates ) {
        if( !( item_is_blacklisted( e.first ) || ( magazines_blacklisted && e.second->magazine ) ) ) {
            continue;
        }
        for( auto &g : m_template_groups ) {
            g.second->remove_item( e.first );
        }
        recipe_dict.delete_if( [&]( recipe &r ) {
            return r.result == e.first || r.requirements.remove_item( e.first );
        } );

        remove_construction_if([&](construction &c) {
            return c.requirements.remove_item( e.first );
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

void add_to_set( t_string_set &s, JsonObject &json, const std::string &name )
{
    JsonArray jarr = json.get_array( name );
    while( jarr.has_more() ) {
        s.insert( jarr.next_string() );
    }
}

void Item_factory::load_item_blacklist( JsonObject &json )
{
    add_to_set( item_blacklist, json, "items" );
}

void Item_factory::load_item_whitelist( JsonObject &json )
{
    if( json.has_string( "mode" ) && json.get_string( "mode" ) == "EXCLUSIVE" ) {
        item_whitelist_is_exclusive = true;
    }
    add_to_set( item_whitelist, json, "items" );
}

Item_factory::~Item_factory()
{
    clear();
}

Item_factory::Item_factory()
{
    init();
}

class iuse_function_wrapper : public iuse_actor
{
    private:
        use_function_pointer cpp_function;
    public:
        iuse_function_wrapper( const use_function_pointer f ) : cpp_function( f ) { }
        ~iuse_function_wrapper() = default;
        long use( player *p, item *it, bool a, const tripoint &pos ) const override {
            iuse tmp;
            return ( tmp.*cpp_function )( p, it, a, pos );
        }
        iuse_actor *clone() const override {
            return new iuse_function_wrapper( *this );
        }
};

use_function::use_function( const use_function_pointer f )
    : use_function( new iuse_function_wrapper( f ) )
{
}

void Item_factory::init()
{
    //Populate the iuse functions
    iuse_function_list["NONE"] = use_function();
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
    iuse_function_list["BOLTCUTTERS"] = &iuse::boltcutters;
    iuse_function_list["MOP"] = &iuse::mop;
    iuse_function_list["SPRAY_CAN"] = &iuse::spray_can;
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
    iuse_function_list["SAW_BARREL"] = &iuse::saw_barrel;

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

    // The above creates iuse_actor instances (from the function pointers) that have
    // no `type` set. This loops sets the type to the same as the key in the map.
    for( auto &e : iuse_function_list ) {
        iuse_actor * const actor = e.second.get_actor_ptr();
        if( actor ) {
            actor->type = e.first;
        }
    }

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
    add_category(category_id_guns, -22, _("GUNS"));
    add_category(category_id_magazines, -21, _("MAGAZINES"));
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
    add_category(category_id_veh_parts, -10, _("VEHICLE PARTS"));
    add_category(category_id_other, -9, _("OTHER"));
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

bool Item_factory::check_ammo_type( std::ostream &msg, const ammotype& ammo ) const
{
    if ( ammo == "NULL" || ammo == "pointer_fake_ammo" ) {
        return false; // skip fake types
    }

    if (ammo_name(ammo) == "none") {
        msg << string_format("ammo type %s not listed in ammo_name() function", ammo.c_str()) << "\n";
        return false;
    }

    if( std::none_of( m_templates.begin(), m_templates.end(), [&ammo]( const decltype(m_templates)::value_type& e ) {
        return e.second->ammo && e.second->ammo->type == ammo;
    } ) ) {
        msg << string_format("there is no actual ammo of type %s defined", ammo.c_str()) << "\n";
        return false;
    }
    return true;
}

void Item_factory::check_definitions() const
{
    std::ostringstream main_stream;
    std::set<itype_id> magazines_used;
    std::set<itype_id> magazines_defined;
    for( const auto &elem : m_templates ) {
        std::ostringstream msg;
        const itype *type = elem.second.get();

        if( type->weight < 0 ) {
            msg << "negative weight" << "\n";
        }
        if( type->volume < 0 ) {
            msg << "negative volume" << "\n";
        }
        if( type->price < 0 ) {
            msg << "negative price" << "\n";
        }

        for( auto mat_id : type->materials ) {
            if( mat_id.str() == "null" || !mat_id.is_valid() ) {
                msg << string_format("invalid material %s", mat_id.c_str()) << "\n";
            }
        }

        if( type->sym == 0 ) {
            msg << "symbol not defined" << "\n";
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
        for( auto &q : type->qualities ) {
            if( !q.first.is_valid() ) {
                msg << string_format("item %s has unknown quality %s", type->id.c_str(), q.first.c_str()) << "\n";
            }
        }
        if( type->default_container != "null" && !has_template( type->default_container ) ) {
            msg << string_format( "invalid container property %s", type->default_container.c_str() ) << "\n";
        }

        if( type->engine ) {
            for( const auto& f : type->engine->faults ) {
                if( !f.is_valid() ) {
                    msg << string_format( "invalid item fault %s", f.c_str() ) << "\n";
                }
            }
        }

        if( type->comestible ) {
            if( type->comestible->tool != "null" ) {
                auto req_tool = find_template( type->comestible->tool );
                if( !req_tool->tool ) {
                    msg << string_format( "invalid tool property %s", type->comestible->tool.c_str() ) << "\n";
                }
            }
        }
        if( type->brewable != nullptr ) {
            if( type->brewable->results.empty() ) {
                msg << string_format( "empty product list" ) << "\n";
            }

            for( auto & b : type->brewable->results ) {
                if( !has_template( b ) ) {
                    msg << string_format( "invalid result id %s", b.c_str() ) << "\n";
                }
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
            if( type->ammo->type.empty() ) {
                msg << "ammo does not specify a type" << "\n";
            } else {
                check_ammo_type( msg, type->ammo->type );
            }
            if( type->ammo->casing != "null" && !has_template( type->ammo->casing ) ) {
                msg << string_format( "invalid casing property %s", type->ammo->casing.c_str() ) << "\n";
            }
        }
        if( type->gun ) {
            check_ammo_type( msg, type->gun->ammo );

            if( type->gun->ammo == "NULL" ) {
                // if gun doesn't use ammo forbid both integral or detachable magazines
                if( bool( type->gun->clip ) || !type->magazines.empty() ) {
                    msg << "cannot specify clip_size or magazine without ammo type" << "\n";
                }

                if( type->item_tags.count( "RELOAD_AND_SHOOT" ) ) {
                    msg << "RELOAD_AND_SHOOT requires an ammo type to be specified" << "\n";
                }

            } else {
                // whereas if it does use ammo enforce specifying either (but not both)
                if( bool( type->gun->clip ) == !type->magazines.empty() ) {
                    msg << "missing or duplicte clip_size or magazine" << "\n";
                }

                if( type->item_tags.count( "RELOAD_AND_SHOOT" ) && !type->magazines.empty() ) {
                    msg << "RELOAD_AND_SHOOT cannot be used with magazines" << "\n";
                }

                if( type->item_tags.count( "BIO_WEAPON" ) ) {
                    msg << "BIO_WEAPON must not be specified with an ammo type" << "\n";
                }

                if( !type->magazines.empty() && !type->magazine_default.count( type->gun->ammo ) ) {
                    msg << "specified magazine but none provided for default ammo type" << "\n";
                }
            }

            if( type->gun->barrel_length < 0 ) {
                msg << "gun barrel length cannot be negative" << "\n";
            }

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
            check_ammo_type( msg, type->gunmod->ammo_modifier );
        }
        if( type->magazine ) {
            magazines_defined.insert( type->id );
            check_ammo_type( msg, type->magazine->type );
            if( type->magazine->type == "NULL" ) {
                msg << "magazine did not specify ammo type" << "\n";
            }
            if( type->magazine->capacity < 0 ) {
                msg << string_format("invalid capacity %i", type->magazine->capacity) << "\n";
            }
            if( type->magazine->count < 0 || type->magazine->count > type->magazine->capacity ) {
                msg << string_format("invalid count %i", type->magazine->count) << "\n";
            }
            if( type->magazine->reliability < 0 || type->magazine->reliability > 100) {
                msg << string_format("invalid reliability %i", type->magazine->reliability) << "\n";
            }
            if( type->magazine->reload_time < 0 ) {
                msg << string_format("invalid reload_time %i", type->magazine->reload_time) << "\n";
            }
            if( type->magazine->linkage != "NULL" && !has_template( type->magazine->linkage ) ) {
                msg << string_format( "invalid linkage property %s", type->magazine->linkage.c_str() ) << "\n";
            }
        }

        for( const auto& typ : type->magazines ) {
            for( const auto& mag : typ.second ) {
                if( !has_template( mag ) || !find_template( mag )->magazine ) {
                    msg << string_format("invalid magazine.") << "\n";
                }
                magazines_used.insert( mag );
            }
        }

        if( type->tool ) {
            check_ammo_type( msg, type->tool->ammo_id );
            if( type->tool->revert_to != "null" && !has_template( type->tool->revert_to ) ) {
                msg << string_format( "invalid revert_to property %s", type->tool->revert_to.c_str() ) << "\n";
            }
            if( !type->tool->revert_msg.empty() && type->tool->revert_to == "null" ) {
                msg << _( "cannot specify revert_msg without revert_to" ) << "\n";
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
    if( !g->has_option( "blacklist_magazines" ) ) {
        for( auto &mag : magazines_defined ) {
            if( magazines_used.count( mag ) == 0 ) {
                main_stream << "Magazine " << mag << " defined but not used.\n";
            }
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

//Returns the template with the given identification tag
const itype * Item_factory::find_template( const itype_id& id ) const
{
    auto& found = m_templates[ id ];
    if( !found ) {
        debugmsg( "Missing item definition: %s", id.c_str() );
        found.reset( new itype() );
        found->id = id;
        found->name = string_format( "undefined-%ss", id.c_str() );
        found->name_plural = string_format( "undefined-%s", id.c_str() );
        found->description = string_format( "Missing item definition for %s.", id.c_str() );
    }

    return found.get();
}

void Item_factory::add_item_type(itype *new_type)
{
    if( new_type == nullptr ) {
        debugmsg( "called Item_factory::add_item_type with nullptr" );
        return;
    }
    m_templates[ new_type->id ].reset( new_type );
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

itype * Item_factory::load_definition( JsonObject& jo ) {
    if( !jo.has_string( "copy-from" ) ) {
        return new itype();
    }

    auto base = m_templates.find( jo.get_string( "copy-from" ) );
    if( base != m_templates.end() ) {
        return new itype( *base->second );
    }

    auto abstract = m_abstracts.find( jo.get_string( "copy-from" ) );
    if( abstract != m_abstracts.end() ) {
        return new itype( *abstract->second );
    }

    deferred.emplace_back( jo.str() );
    return nullptr;
}

void Item_factory::load( islot_artifact &slot, JsonObject &jo )
{
    slot.charge_type = jo.get_enum_value( "charge_type", ARTC_NULL );
    load_optional_enum_array( slot.effects_wielded, jo, "effects_wielded" );
    load_optional_enum_array( slot.effects_activated, jo, "effects_activated" );
    load_optional_enum_array( slot.effects_carried, jo, "effects_carried" );
    load_optional_enum_array( slot.effects_worn, jo, "effects_worn" );
}

/** Helpers to handle json entity inheritance logic in a generic way. **/
template <typename T>
typename std::enable_if<std::is_integral<T>::value, bool>::type assign(
    JsonObject &jo, const std::string& name, T& val ) {
    T tmp;
    if( jo.get_object( "relative" ).read( name, tmp ) ) {
        val += tmp;
        return true;
    }

    double scalar;
    if( jo.get_object( "proportional" ).read( name, scalar ) && scalar > 0.0 ) {
        val *= scalar;
        return true;
    }

    return jo.read( name, val );
}

template <typename T>
typename std::enable_if<std::is_constructible<T, std::string>::value, bool>::type assign(
    JsonObject &jo, const std::string& name, T& val ) {
    return jo.read( name, val );
}

template <typename T>
typename std::enable_if<std::is_constructible<T, std::string>::value, bool>::type assign(
    JsonObject &jo, const std::string& name, std::set<T>& val ) {

    if( jo.has_string( name ) || jo.has_array( name ) ) {
        val = jo.get_tags<T>( name );
        return true;
    }

    bool res = false;

    auto add = jo.get_object( "extend" );
    if( add.has_string( name ) || add.has_array( name ) ) {
        auto tags = add.get_tags<T>( name );
        val.insert( tags.begin(), tags.end() );
        res = true;
    }

    auto del = jo.get_object( "delete" );
    if( del.has_string( name ) || del.has_array( name ) ) {
        for( const auto& e : del.get_tags<T>( name ) ) {
            val.erase( e );
        }
        res = true;
    }

    return res;
}

void Item_factory::load( islot_ammo &slot, JsonObject &jo )
{
    assign( jo, "ammo_type", slot.type );
    assign( jo, "casing", slot.casing );
    assign( jo, "damage", slot.damage );
    assign( jo, "pierce", slot.pierce );
    assign( jo, "range", slot.range );
    assign( jo, "dispersion", slot.dispersion );
    assign( jo, "recoil", slot.recoil );
    assign( jo, "count", slot.def_charges );
    assign( jo, "loudness", slot.loudness );
    assign( jo, "effects", slot.ammo_effects );
}

void Item_factory::load_ammo(JsonObject &jo)
{
    auto def = load_definition( jo );
    if( def) {
        load_slot( def->ammo, jo );
        load_basic_info( jo, def );
    }
}

void Item_factory::load( islot_engine &slot, JsonObject &jo )
{
    assign( jo, "displacement", slot.displacement );
    assign( jo, "faults", slot.faults );
}

void Item_factory::load_engine( JsonObject &jo )
{
    auto def = load_definition( jo );
    if( def) {
        load_slot( def->engine, jo );
        load_basic_info( jo, def );
    }
}

void Item_factory::load( islot_gun &slot, JsonObject &jo )
{
    assign( jo, "skill", slot.skill_used );
    assign( jo, "ammo", slot.ammo );
    assign( jo, "range", slot.range );
    assign( jo, "ranged_damage", slot.damage );
    assign( jo, "pierce", slot.pierce );
    assign( jo, "dispersion", slot.dispersion );
    assign( jo, "sight_dispersion", slot.sight_dispersion );
    assign( jo, "aim_speed", slot.aim_speed );
    assign( jo, "recoil", slot.recoil );
    assign( jo, "durability", slot.durability );
    assign( jo, "burst", slot.burst );
    assign( jo, "loudness", slot.loudness );
    assign( jo, "clip_size", slot.clip );
    assign( jo, "reload", slot.reload_time );
    assign( jo, "reload_noise", slot.reload_noise );
    assign( jo, "reload_noise_volume", slot.reload_noise_volume );
    assign( jo, "barrel_length", slot.barrel_length );
    assign( jo, "built_in_mods", slot.built_in_mods );
    assign( jo, "default_mods", slot.default_mods );
    assign( jo, "ups_charges", slot.ups_charges );
    assign( jo, "ammo_effects", slot.ammo_effects );

    if( jo.has_array( "valid_mod_locations" ) ) {
        slot.valid_mod_locations.clear();
        JsonArray jarr = jo.get_array( "valid_mod_locations" );
        while( jarr.has_more() ) {
            JsonArray curr = jarr.next_array();
            slot.valid_mod_locations.emplace( curr.get_string( 0 ), curr.get_int( 1 ) );
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
}

void Item_factory::load_gun(JsonObject &jo)
{
    auto def = load_definition( jo );
    if( def) {
        load_slot( def->gun, jo );
        load_basic_info( jo, def );
    }
}

void Item_factory::load_armor(JsonObject &jo)
{
    itype* new_item_template = new itype();
    load_slot( new_item_template->armor, jo );
    load_basic_info( jo, new_item_template );
}

void Item_factory::load( islot_armor &slot, JsonObject &jo )
{
    slot.encumber = jo.get_int( "encumbrance", 0 );
    slot.coverage = jo.get_int( "coverage", 0 );
    slot.thickness = jo.get_int( "material_thickness", 0 );
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

void Item_factory::load( islot_tool &slot, JsonObject &jo )
{
    assign( jo, "ammo", slot.ammo_id );
    assign( jo, "max_charges", slot.max_charges );
    assign( jo, "initial_charges", slot.def_charges );
    assign( jo, "charges_per_use", slot.charges_per_use );
    assign( jo, "turns_per_charge", slot.turns_per_charge );
    assign( jo, "revert_to", slot.revert_to );
    assign( jo, "revert_msg", slot.revert_msg );
    assign( jo, "sub", slot.subtype );
}

void Item_factory::load_tool(JsonObject &jo)
{
    auto def = load_definition( jo );
    if( def ) {
        load_slot( def->tool, jo );
        load_basic_info( jo, def );
        load_slot( def->spawn, jo ); // @todo deprecate
    }
}

void Item_factory::load_tool_armor(JsonObject &jo)
{
    auto def = new itype();
    load_slot( def->tool, jo );
    load_slot( def->armor, jo );
    load_basic_info( jo, def );
}

void Item_factory::load( islot_book &slot, JsonObject &jo )
{
    assign( jo, "max_level", slot.level );
    assign( jo, "required_level", slot.req );
    assign( jo, "fun", slot.fun );
    assign( jo, "intelligence", slot.intel );
    assign( jo, "time", slot.time );
    assign( jo, "skill", slot.skill );
    assign( jo, "chapters", slot.chapters );

    set_use_methods_from_json( jo, "use_action", slot.use_methods );
}

void Item_factory::load_book( JsonObject &jo )
{
    auto def = load_definition( jo );
    if( def) {
        load_slot( def->book, jo );
        load_basic_info( jo, def );
    }
}

void Item_factory::load( islot_comestible &slot, JsonObject &jo )
{
    assign( jo, "comestible_type", slot.comesttype );
    assign( jo, "tool", slot.tool );
    assign( jo, "charges", slot.def_charges );
    assign( jo, "quench", slot.quench );
    assign( jo, "fun", slot.fun );
    assign( jo, "stim", slot.stim );
    assign( jo, "healthy", slot.healthy );
    assign( jo, "parasites", slot.parasites );

    if( jo.read( "spoils_in", slot.spoils ) ) {
        slot.spoils *= 600; // JSON specifies hours so convert to turns
    }

    if( jo.has_string( "addiction_type" ) ) {
        slot.add = addiction_type( jo.get_string( "addiction_type" ) );
    }

    bool got_calories = false;

    if( jo.has_int( "calories" ) ) {
        slot.nutr = jo.get_int( "calories" ) / islot_comestible::kcal_per_nutr;
        got_calories = true;

    } else if( jo.get_object( "relative" ).has_int( "calories" ) ) {
        slot.nutr += jo.get_object( "relative" ).get_int( "calories" ) / islot_comestible::kcal_per_nutr;
        got_calories = true;

    } else if( jo.get_object( "proportional" ).has_float( "calories" ) ) {
        slot.nutr *= jo.get_object( "proportional" ).get_float( "calories" );
        got_calories = true;

    } else {
        jo.read( "nutrition", slot.nutr );
    }

    if( jo.has_member( "nutrition" ) && got_calories ) {
        jo.throw_error( "cannot specify both nutrition and calories", "nutrition" );
    }

    // any specification of vitamins suppresses use of material defaults @see Item_factory::finalize
    if( jo.has_array( "vitamins" ) ) {
        auto vits = jo.get_array( "vitamins" );
        if( vits.empty() ) {
            for( auto &v : vitamin::all() ) {
                slot.vitamins[ v.first ] = 0;
            }
        } else {
            while( vits.has_more() ) {
                auto pair = vits.next_array();
                slot.vitamins[ vitamin_id( pair.get_string( 0 ) ) ] = pair.get_int( 1 );
            }
        }

    } else if( jo.has_object( "relative" ) ) {
        auto rel = jo.get_object( "relative" );
        if( rel.has_int( "vitamins" ) ) {
            // allows easy specification of 'fortified' comestibles
            for( auto &v : vitamin::all() ) {
                slot.vitamins[ v.first ] += rel.get_int( "vitamins" );
            }
        } else if( rel.has_array( "vitamins" ) ) {
            auto vits = rel.get_array( "vitamins" );
            while( vits.has_more() ) {
                auto pair = vits.next_array();
                slot.vitamins[ vitamin_id( pair.get_string( 0 ) ) ] += pair.get_int( 1 );
            }
        }
    }
}

void Item_factory::load( islot_brewable &slot, JsonObject &jo )
{
    slot.time = jo.get_int( "time" );
    slot.results = jo.get_string_array( "results" );
}

void Item_factory::load_comestible(JsonObject &jo)
{
    auto def = load_definition( jo );
    if( def) {
        load_slot( def->comestible, jo );
        load_basic_info( jo, def );
    }
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
}

void Item_factory::load( islot_gunmod &slot, JsonObject &jo )
{
    slot.damage = jo.get_int( "damage_modifier", 0 );
    slot.loudness = jo.get_int( "loudness_modifier", 0 );
    slot.ammo_modifier = jo.get_string( "ammo_modifier", slot.ammo_modifier );
    slot.location = jo.get_string( "location" );
    // TODO: implement loading this from json (think of a proper name)
    // slot.pierce = jo.get_string( "mod_pierce", 0 );
    slot.usable = jo.get_tags( "mod_targets");
    slot.dispersion = jo.get_int( "dispersion_modifier", 0 );
    slot.sight_dispersion = jo.get_int( "sight_dispersion", -1 );
    slot.aim_speed = jo.get_int( "aim_speed", -1 );
    slot.recoil = jo.get_int( "recoil_modifier", 0 );
    slot.burst = jo.get_int( "burst_modifier", 0 );
    slot.range = jo.get_int( "range_modifier", 0 );
    slot.acceptable_ammo = jo.get_tags( "acceptable_ammo" );
    slot.ups_charges = jo.get_int( "ups_charges", slot.ups_charges );
    slot.install_time = jo.get_int( "install_time", slot.install_time );

    JsonArray mags = jo.get_array( "magazine_adaptor" );
    while( mags.has_more() ) {
        JsonArray arr = mags.next_array();

        ammotype ammo = arr.get_string( 0 ); // an ammo type (eg. 9mm)
        JsonArray compat = arr.get_array( 1 ); // compatible magazines for this ammo type

        while( compat.has_more() ) {
            slot.magazine_adaptor[ ammo ].insert( compat.next_string() );
        }
    }
}

void Item_factory::load_gunmod(JsonObject &jo)
{
    itype *new_item_template = new itype();
    load_slot( new_item_template->gunmod, jo );
    load_basic_info( jo, new_item_template );
}

void Item_factory::load( islot_magazine &slot, JsonObject &jo )
{
    assign( jo, "ammo_type", slot.type );
    assign( jo, "capacity", slot.capacity );
    assign( jo, "count", slot.count );
    assign( jo, "reliability", slot.reliability );
    assign( jo, "reload_time", slot.reload_time );
    assign( jo, "linkage", slot.linkage );
}

void Item_factory::load_magazine(JsonObject &jo)
{
    auto def = load_definition( jo );
    if( def) {
        load_slot( def->magazine, jo );
        load_basic_info( jo, def );
    }
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
    auto def = load_definition( jo );
    if( def ) {
        load_basic_info( jo, def );
    }
}

// Adds allergy flags to items with allergenic materials
// Set for all items (not just food and clothing) to avoid edge cases
static void set_allergy_flags( itype &item_template )
{
    using material_allergy_pair = std::pair<material_id, std::string>;
    static const std::vector<material_allergy_pair> all_pairs = {{
        // First allergens:
        // An item is an allergen even if it has trace amounts of allergenic material
        std::make_pair( material_id( "hflesh" ), "CANNIBALISM" ),

        std::make_pair( material_id( "hflesh" ), "ALLERGEN_MEAT" ),
        std::make_pair( material_id( "iflesh" ), "ALLERGEN_MEAT" ),
        std::make_pair( material_id( "flesh" ), "ALLERGEN_MEAT" ),
        std::make_pair( material_id( "wheat" ), "ALLERGEN_WHEAT" ),
        std::make_pair( material_id( "fruit" ), "ALLERGEN_FRUIT" ),
        std::make_pair( material_id( "veggy" ), "ALLERGEN_VEGGY" ),
        std::make_pair( material_id( "milk" ), "ALLERGEN_MILK" ),
        std::make_pair( material_id( "egg" ), "ALLERGEN_EGG" ),
        std::make_pair( material_id( "junk" ), "ALLERGEN_JUNK" ),
        // Not food, but we can keep it here
        std::make_pair( material_id( "wool" ), "ALLERGEN_WOOL" ),
        // Now "made of". Those flags should not be passed
        std::make_pair( material_id( "flesh" ), "CARNIVORE_OK" ),
        std::make_pair( material_id( "hflesh" ), "CARNIVORE_OK" ),
        std::make_pair( material_id( "iflesh" ), "CARNIVORE_OK" ),
        std::make_pair( material_id( "milk" ), "CARNIVORE_OK" ),
        std::make_pair( material_id( "egg" ), "CARNIVORE_OK" ),
        std::make_pair( material_id( "honey" ), "URSINE_HONEY" ),
    }};

    const auto &mats = item_template.materials;
    for( const auto &pr : all_pairs ) {
        if( std::find( mats.begin(), mats.end(), std::get<0>( pr ) ) != mats.end() ) {
            item_template.item_tags.insert( std::get<1>( pr ) );
        }
    }
}

// Migration helper: turns human flesh into generic flesh
// Don't call before making sure that the cannibalism flag is set
void hflesh_to_flesh( itype &item_template )
{
    auto &mats = item_template.materials;
    const auto old_size = mats.size();
    mats.erase( std::remove( mats.begin(), mats.end(), material_id( "hflesh" ) ), mats.end() );
    // Only add "flesh" material if not already present
    if( old_size != mats.size() &&
        std::find( mats.begin(), mats.end(), material_id( "flesh" ) ) == mats.end() ) {
        mats.push_back( material_id( "flesh" ) );
    }
}

void Item_factory::load_basic_info(JsonObject &jo, itype *new_item_template)
{
    if( jo.has_string( "abstract" ) ) {
        new_item_template->id = jo.get_string( "abstract" );
        m_abstracts[ new_item_template->id ].reset( new_item_template );
    } else {
        new_item_template->id = jo.get_string( "id" );
        m_templates[ new_item_template->id ].reset( new_item_template );
    }

    assign( jo, "weight", new_item_template->weight );
    assign( jo, "volume", new_item_template->volume );
    assign( jo, "price", new_item_template->price );
    assign( jo, "price_postapoc", new_item_template->price_post );
    assign( jo, "stack_size", new_item_template->stack_size );
    assign( jo, "integral_volume", new_item_template->integral_volume );
    assign( jo, "bashing", new_item_template->melee_dam );
    assign( jo, "cutting", new_item_template->melee_cut );
    assign( jo, "to_hit", new_item_template->m_to_hit );
    assign( jo, "container", new_item_template->default_container );
    assign( jo, "rigid", new_item_template->rigid );
    assign( jo, "min_strength", new_item_template->min_str );
    assign( jo, "min_dexterity", new_item_template->min_dex );
    assign( jo, "min_intelligence", new_item_template->min_int );
    assign( jo, "min_perception", new_item_template->min_per );
    assign( jo, "magazine_well", new_item_template->magazine_well );
    assign( jo, "explode_in_fire", new_item_template->explode_in_fire );

    new_item_template->name = jo.get_string( "name" );
    if( jo.has_member( "name_plural" ) ) {
        new_item_template->name_plural = jo.get_string( "name_plural" );
    } else {
        new_item_template->name_plural = jo.get_string( "name" ) += "s";
    }

    if( jo.has_string( "description" ) ) {
        new_item_template->description = _( jo.get_string( "description" ).c_str() );
    }

    if( jo.has_string( "symbol" ) ) {
        new_item_template->sym = jo.get_string( "symbol" )[0];
    }

    if( jo.has_string( "color" ) ) {
        new_item_template->color = color_from_string( jo.get_string( "color" ) );
    }

    if( jo.has_member( "material" ) ) {
        new_item_template->materials.clear();
        for( auto &m : jo.get_tags( "material" ) ) {
            new_item_template->materials.emplace_back( m );
        }
    }

    if( jo.has_string( "phase" ) ) {
        new_item_template->phase = jo.get_enum_value<phase_id>( "phase" );
    }

    if( jo.has_array( "magazines" ) ) {
        new_item_template->magazine_default.clear();
        new_item_template->magazines.clear();
    }
    JsonArray mags = jo.get_array( "magazines" );
    while( mags.has_more() ) {
        JsonArray arr = mags.next_array();

        ammotype ammo = arr.get_string( 0 ); // an ammo type (eg. 9mm)
        JsonArray compat = arr.get_array( 1 ); // compatible magazines for this ammo type

        // the first magazine for this ammo type is the default;
        new_item_template->magazine_default[ ammo ] = compat.get_string( 0 );

        while( compat.has_more() ) {
            new_item_template->magazines[ ammo ].insert( compat.next_string() );
        }
    }

    JsonArray jarr = jo.get_array( "min_skills" );
    while( jarr.has_more() ) {
        JsonArray cur = jarr.next_array();
        new_item_template->min_skills[skill_id( cur.get_string( 0 ) )] = cur.get_int( 1 );
    }

    if( jo.has_member("explosion" ) ) {
        JsonObject je = jo.get_object( "explosion" );
        new_item_template->explosion = load_explosion_data( je );
    }

    if( jo.has_array( "snippet_category" ) ) {
        // auto-create a category that is unlikely to already be used and put the
        // snippets in it.
        new_item_template->snippet_category = std::string( "auto:" ) + new_item_template->id;
        JsonArray jarr = jo.get_array( "snippet_category" );
        SNIPPET.add_snippets_from_json( new_item_template->snippet_category, jarr );
    } else {
        new_item_template->snippet_category = jo.get_string( "snippet_category", "" );
    }

    assign( jo, "flags", new_item_template->item_tags );

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

    if( jo.has_member( "category" ) ) {
        new_item_template->category = get_category( jo.get_string( "category" ) );
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
    load_slot_optional( new_item_template->artifact, jo, "artifact_data" );
    load_slot_optional( new_item_template->brewable, jo, "brewable" );
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
            const auto quali = std::pair<quality_id, int>(quality_id(curr.get_string(0)), curr.get_int(1));
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

    m_templates.clear();
    item_blacklist.clear();
    item_whitelist.clear();
    item_whitelist_is_exclusive = false;
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
    } else if (subtype == "collection") {
        ig = make_group_or_throw(isd, Item_group::G_COLLECTION);
    } else if (subtype == "distribution") {
        ig = make_group_or_throw(isd, Item_group::G_DISTRIBUTION);
    } else {
        jsobj.throw_error("unknown item group type", "subtype");
    }

    assign( jsobj, "ammo", ig->with_ammo );
    assign( jsobj, "magazine", ig->with_magazine );

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

    use_methods.clear();
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
    } else if( type == "bandolier" ) {
        newfun = load_actor<bandolier_actor>( obj );
    } else if( type == "ammobelt" ) {
        newfun = load_actor<ammobelt_actor>( obj );
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

const std::string &Item_factory::calc_category( const itype *it )
{
    if( it->gun && !it->gunmod ) {
        return category_id_guns;
    }
    if( it->magazine ) {
        return category_id_magazines;
    }
    if( it->ammo ) {
        return category_id_ammo;
    }
    if( it->tool ) {
        return category_id_tools;
    }
    if( it->armor ) {
        return category_id_clothing;
    }
    if (it->comestible) {
        return it->comestible->comesttype == "MED" ? category_id_drugs : category_id_food;
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
