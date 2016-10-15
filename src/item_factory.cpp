#include "item_factory.h"

#include "addiction.h"
#include "artifact.h"
#include "bionics.h"
#include "catacharset.h"
#include "construction.h"
#include "crafting.h"
#include "debug.h"
#include "enums.h"
#include "generic_factory.h"
#include "init.h"
#include "item.h"
#include "item_group.h"
#include "iuse_actor.h"
#include "json.h"
#include "mapdata.h"
#include "material.h"
#include "options.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "skill.h"
#include "translations.h"
#include "text_snippets.h"
#include "ui.h"
#include "veh_type.h"
#include "field.h"

#include <algorithm>
#include <sstream>

typedef std::set<std::string> t_string_set;
static t_string_set item_blacklist;

static DynamicDataLoader::deferred_json deferred;

std::unique_ptr<Item_factory> item_controller( new Item_factory() );

static const std::string calc_category( const itype &obj );
static void set_allergy_flags( itype &item_template );
static void hflesh_to_flesh( itype &item_template );
static void npc_implied_flags( itype &item_template );

extern const double MIN_RECOIL;

bool item_is_blacklisted(const std::string &id)
{
    return item_blacklist.count( id );
}


static bool assign_coverage_from_json( JsonObject &jo, const std::string &key,
                                       std::bitset<num_bp> &parts, bool &sided )
{
    auto parse = [&parts,&sided]( const std::string &val ) {
        if( val == "ARMS" || val == "ARM_EITHER" ) {
            parts.set( bp_arm_l );
            parts.set( bp_arm_r );
        } else if( val == "HANDS" || val == "HAND_EITHER" ) {
            parts.set( bp_hand_l );
            parts.set( bp_hand_r );
        } else if( val == "LEGS" || val == "LEG_EITHER" ) {
            parts.set( bp_leg_l );
            parts.set( bp_leg_r );
        } else if( val == "FEET" || val == "FOOT_EITHER" ) {
            parts.set( bp_foot_l );
            parts.set( bp_foot_r );
        } else {
            parts.set( get_body_part_token( val ) );
        }
        sided += ( val == "ARM_EITHER" || val == "HAND_EITHER" ||
                   val == "LEG_EITHER" || val == "FOOT_EITHER" );
    };

    if( jo.has_array( key ) ) {
        JsonArray arr = jo.get_array( key );
        while( arr.has_more() ) {
            parse( arr.next_string() );
        }
        return true;

    } else if( jo.has_string( key ) ) {
        parse( jo.get_string( key ) );
        return true;

    } else {
        return false;
    }
}

void Item_factory::finalize() {
    if( !DynamicDataLoader::get_instance().load_deferred( deferred ) ) {
        debugmsg( "JSON contains circular dependency: discarded %i templates", deferred.size() );
    }

    finalize_item_blacklist();

    for( auto& e : m_templates ) {
        itype& obj = *e.second;

        if( obj.item_tags.count( "STAB" ) || obj.item_tags.count( "SPEAR" ) ) {
            std::swap(obj.melee[DT_CUT], obj.melee[DT_STAB]);
        }

        // add usage methods (with default values) based upon qualities
        // if a method was already set the specific values remain unchanged
        for( const auto &q : obj.qualities ) {
            for( const auto &u : q.first.obj().usages ) {
                if( q.second >= u.first ) {
                    obj.use_methods.emplace( u.second, usage_from_string( u.second ) );
                }
            }
        }

        if( obj.mod ) {
            std::string func = obj.gunmod ? "GUNMOD_ATTACH" : "TOOLMOD_ATTACH";
            obj.use_methods.emplace( func, usage_from_string( func ) );
        }

        if( obj.engine && get_world_option<bool>( "NO_FAULTS" ) ) {
            obj.engine->faults.clear();
        }

        // If no category was forced via JSON automatically calculate one now
        if( obj.category_force.empty() ) {
            obj.category_force = calc_category( obj );
        }

        // If category exists assign now otherwise throw error later in @see check_definitions()
        auto cat = categories.find( obj.category_force );
        if( cat != categories.end() ) {
            obj.category = &cat->second;
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
        // JSON contains volume per complete stack, convert it to volume per single item
        if( obj.count_by_charges() ) {
            obj.volume = obj.volume / obj.stack_size;
            obj.integral_volume = obj.integral_volume / obj.stack_size;
        }
        // Items always should have some volume.
        // TODO: handle possible exception software?
        // TODO: make items with 0 volume an error during loading?
        if( obj.volume <= 0 ) {
            obj.volume = units::from_milliliter( 1 );
        }
        for( const auto &tag : obj.item_tags ) {
            if( tag.size() > 6 && tag.substr( 0, 6 ) == "LIGHT_" ) {
                obj.light_emission = std::max( atoi( tag.substr( 6 ).c_str() ), 0 );
            }
        }
        // for ammo not specifying loudness (or an explicit zero) derive value from other properties
        if( obj.ammo ) {
            if( obj.ammo->loudness < 0 ) {
                obj.ammo->loudness = std::max( std::max( { obj.ammo->damage, obj.ammo->pierce, obj.ammo->range } ) * 3,
                                               obj.ammo->recoil / 3 );
            }

            const auto &mats = obj.materials;
            if( std::find( mats.begin(), mats.end(), material_id( "hydrocarbons" ) ) == mats.end() &&
                std::find( mats.begin(), mats.end(), material_id( "oil" ) ) == mats.end() ) {
                const auto &ammo_effects = obj.ammo->ammo_effects;
                obj.ammo->cookoff = ammo_effects.count( "INCENDIARY" ) > 0 ||
                                    ammo_effects.count( "COOKOFF" ) > 0;
                static const std::set<std::string> special_cookoff_tags = {{
                    "NAPALM", "NAPALM_BIG",
                    "EXPLOSIVE_SMALL", "EXPLOSIVE", "EXPLOSIVE_BIG", "EXPLOSIVE_HUGE",
                    "TOXICGAS", "TEARGAS", "SMOKE", "SMOKE_BIG",
                    "FRAG", "FLASHBANG"
                }};
                obj.ammo->special_cookoff = std::any_of( ammo_effects.begin(), ammo_effects.end(),
                    []( const std::string &s ) {
                        return special_cookoff_tags.count( s ) > 0;
                    } );
            } else {
                obj.ammo->cookoff = false;
                obj.ammo->special_cookoff = false;
            }
        }
        // for magazines ensure default_ammo is set
        if( obj.magazine && obj.magazine->default_ammo == "NULL" ) {
               obj.magazine->default_ammo = default_ammo( obj.magazine->type );
        }
        if( obj.gun ) {
            // @todo add explicit action field to gun definitions
            std::string defmode = "semi-auto";
            if( obj.gun->clip == 1 ) {
                defmode = "manual"; // break-type actions
            } else if( obj.gun->skill_used == skill_id( "pistol" ) && obj.item_tags.count( "RELOAD_ONE" ) ) {
                defmode = "revolver";
            }

            // if the gun doesn't have a DEFAULT mode then add one now
            obj.gun->modes.emplace( "DEFAULT", std::tuple<std::string, int, std::set<std::string>>( defmode, 1,
                                    std::set<std::string>() ) );

            if( obj.gun->burst > 1 ) {
                // handle legacy JSON format
                obj.gun->modes.emplace( "AUTO", std::tuple<std::string, int, std::set<std::string>>( "auto", obj.gun->burst,
                                        std::set<std::string>() ) );
            }

            if( obj.gun->handling < 0 ) {
                // @todo specify in JSON via classes
                if( obj.gun->skill_used == skill_id( "rifle" ) ||
                    obj.gun->skill_used == skill_id( "smg" ) ||
                    obj.gun->skill_used == skill_id( "shotgun" ) ){
                    obj.gun->handling = 20;
                } else {
                    obj.gun->handling = 10;
                }
            }

            obj.gun->reload_noise = _( obj.gun->reload_noise.c_str() );

            // @todo Move to jsons?
            if( obj.gun->skill_used == skill_id( "archery" ) ||
                obj.gun->skill_used == skill_id( "throw" ) ) {
                obj.item_tags.insert( "WATERPROOF_GUN" );
                obj.item_tags.insert( "NEVER_JAMS" );
                obj.gun->ammo_effects.insert( "NEVER_MISFIRES" );
            }
        }

        set_allergy_flags( *e.second );
        hflesh_to_flesh( *e.second );
        npc_implied_flags( *e.second );

        if( obj.comestible ) {
            obj.comestible->spoils *= HOURS( 1 ); // JSON specifies hours so convert to turns

            if( get_world_option<bool>( "NO_VITAMINS" ) ) {
                obj.comestible->vitamins.clear();
            } else if( obj.comestible->vitamins.empty() && obj.comestible->healthy >= 0 ) {
                // Default vitamins of healthy comestibles to their edible base materials if none explicitly specified.
                auto healthy = std::max( obj.comestible->healthy, 1 ) * 10;
                auto mat = obj.materials;

                // @todo migrate inedible comestibles to appropriate alternative types.
                mat.erase( std::remove_if( mat.begin(), mat.end(), []( const string_id<material_type> &m ) {
                            return !m.obj().edible();
                        } ), mat.end() );

                // For comestibles composed of multiple edible materials we calculate the average.
                for( const auto &v : vitamin::all() ) {
                    if( obj.comestible->vitamins.find( v.first ) == obj.comestible->vitamins.end() ) {
                        for( const auto &m : mat ) {
                            obj.comestible->vitamins[ v.first ] += ceil( m.obj().vitamin( v.first ) * healthy / mat.size() );
                        }
                    }
                }
            }
        }

        for( auto &e : obj.use_methods ) {
            e.second.get_actor_ptr()->finalize( obj.id );
        }
    }
}

void Item_factory::finalize_item_blacklist()
{
    for (t_string_set::const_iterator a = item_blacklist.begin(); a != item_blacklist.end(); ++a) {
        if (!has_template(*a)) {
            debugmsg("item on blacklist %s does not exist", a->c_str());
        }

    }

    for( auto &e : m_templates ) {
        if( !item_is_blacklisted( e.first ) ) {
            continue;
        }
        for( auto &g : m_template_groups ) {
            g.second->remove_item( e.first );
        }

        // remove any blacklisted items from requirements
        for( auto &r : requirement_data::all() ) {
            const_cast<requirement_data &>( r.second ).blacklist_item( e.first );
        }

        // remove any recipes used to craft the blacklisted item
        recipe_dictionary::delete_if( [&]( const recipe &r ) {
            return r.result == e.first;
        } );
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
        iuse_function_wrapper( const std::string &type, const use_function_pointer f )
            : iuse_actor( type ), cpp_function( f ) { }

        ~iuse_function_wrapper() override = default;
        long use( player *p, item *it, bool a, const tripoint &pos ) const override {
            iuse tmp;
            return ( tmp.*cpp_function )( p, it, a, pos );
        }
        iuse_actor *clone() const override {
            return new iuse_function_wrapper( *this );
        }

        void load( JsonObject & ) override {}
};

use_function::use_function( const std::string &type, const use_function_pointer f )
    : use_function( new iuse_function_wrapper( type, f ) ) {}


void Item_factory::add_iuse( const std::string &type, const use_function_pointer f ) {
    iuse_function_list[ type ] = use_function( type, f );
}

void Item_factory::add_actor( iuse_actor *ptr ) {
    iuse_function_list[ ptr->type ] = use_function( ptr );
}

void Item_factory::init()
{
    add_iuse( "ACIDBOMB_ACT", &iuse::acidbomb_act );
    add_iuse( "ADRENALINE_INJECTOR", &iuse::adrenaline_injector );
    add_iuse( "ALCOHOL", &iuse::alcohol_medium );
    add_iuse( "ALCOHOL_STRONG", &iuse::alcohol_strong );
    add_iuse( "ALCOHOL_WEAK", &iuse::alcohol_weak );
    add_iuse( "ANTIBIOTIC", &iuse::antibiotic );
    add_iuse( "ANTICONVULSANT", &iuse::anticonvulsant );
    add_iuse( "ANTIFUNGAL", &iuse::antifungal );
    add_iuse( "ANTIPARASITIC", &iuse::antiparasitic );
    add_iuse( "ARROW_FLAMABLE", &iuse::arrow_flamable );
    add_iuse( "ARTIFACT", &iuse::artifact );
    add_iuse( "ATOMIC_CAFF", &iuse::atomic_caff );
    add_iuse( "BATTLETORCH_LIT", &iuse::battletorch_lit );
    add_iuse( "BELL", &iuse::bell );
    add_iuse( "BLECH", &iuse::blech );
    add_iuse( "BOLTCUTTERS", &iuse::boltcutters );
    add_iuse( "C4", &iuse::c4 );
    add_iuse( "CABLE_ATTACH", &iuse::cable_attach );
    add_iuse( "CAFF", &iuse::caff );
    add_iuse( "CAMERA", &iuse::camera );
    add_iuse( "CAN_GOO", &iuse::can_goo );
    add_iuse( "CAPTURE_MONSTER_ACT", &iuse::capture_monster_act );
    add_iuse( "CARVER_OFF", &iuse::carver_off );
    add_iuse( "CARVER_ON", &iuse::carver_on );
    add_iuse( "CATFOOD", &iuse::catfood );
    add_iuse( "CHAINSAW_OFF", &iuse::chainsaw_off );
    add_iuse( "CHAINSAW_ON", &iuse::chainsaw_on );
    add_iuse( "CHEW", &iuse::chew );
    add_iuse( "CIRCSAW_ON", &iuse::circsaw_on );
    add_iuse( "COKE", &iuse::coke );
    add_iuse( "COMBATSAW_OFF", &iuse::combatsaw_off );
    add_iuse( "COMBATSAW_ON", &iuse::combatsaw_on );
    add_iuse( "CONTACTS", &iuse::contacts );
    add_iuse( "CROWBAR", &iuse::crowbar );
    add_iuse( "CS_LAJATANG_OFF", &iuse::cs_lajatang_off );
    add_iuse( "CS_LAJATANG_ON", &iuse::cs_lajatang_on );
    add_iuse( "DATURA", &iuse::datura );
    add_iuse( "DIG", &iuse::dig );
    add_iuse( "DIRECTIONAL_ANTENNA", &iuse::directional_antenna );
    add_iuse( "DOGFOOD", &iuse::dogfood );
    add_iuse( "DOG_WHISTLE", &iuse::dog_whistle );
    add_iuse( "DOLLCHAT", &iuse::talking_doll );
    add_iuse( "ECIG", &iuse::ecig );
    add_iuse( "EHANDCUFFS", &iuse::ehandcuffs );
    add_iuse( "EINKTABLETPC", &iuse::einktabletpc );
    add_iuse( "ELEC_CHAINSAW_OFF", &iuse::elec_chainsaw_off );
    add_iuse( "ELEC_CHAINSAW_ON", &iuse::elec_chainsaw_on );
    add_iuse( "EXTINGUISHER", &iuse::extinguisher );
    add_iuse( "EYEDROPS", &iuse::eyedrops );
    add_iuse( "FIRECRACKER", &iuse::firecracker );
    add_iuse( "FIRECRACKER_ACT", &iuse::firecracker_act );
    add_iuse( "FIRECRACKER_PACK", &iuse::firecracker_pack );
    add_iuse( "FIRECRACKER_PACK_ACT", &iuse::firecracker_pack_act );
    add_iuse( "FISH_ROD", &iuse::fishing_rod );
    add_iuse( "FISH_TRAP", &iuse::fish_trap );
    add_iuse( "FLUMED", &iuse::flumed );
    add_iuse( "FLUSLEEP", &iuse::flusleep );
    add_iuse( "FLU_VACCINE", &iuse::flu_vaccine );
    add_iuse( "FUNGICIDE", &iuse::fungicide );
    add_iuse( "FUN_HALLU", &iuse::fun_hallu );
    add_iuse( "GEIGER", &iuse::geiger );
    add_iuse( "GRANADE", &iuse::granade );
    add_iuse( "GRANADE_ACT", &iuse::granade_act );
    add_iuse( "GRENADE_INC_ACT", &iuse::grenade_inc_act );
    add_iuse( "GUN_REPAIR", &iuse::gun_repair );
    add_iuse( "GUNMOD_ATTACH", &iuse::gunmod_attach );
    add_iuse( "TOOLMOD_ATTACH", &iuse::toolmod_attach );
    add_iuse( "HACKSAW", &iuse::hacksaw );
    add_iuse( "HAIRKIT", &iuse::hairkit );
    add_iuse( "HAMMER", &iuse::hammer );
    add_iuse( "HEATPACK", &iuse::heatpack );
    add_iuse( "HEAT_FOOD", &iuse::heat_food );
    add_iuse( "HONEYCOMB", &iuse::honeycomb );
    add_iuse( "HOTPLATE", &iuse::hotplate );
    add_iuse( "INHALER", &iuse::inhaler );
    add_iuse( "JACKHAMMER", &iuse::jackhammer );
    add_iuse( "JET_INJECTOR", &iuse::jet_injector );
    add_iuse( "LADDER", &iuse::ladder );
    add_iuse( "LUMBER", &iuse::lumber );
    add_iuse( "MAKEMOUND", &iuse::makemound );
    add_iuse( "MARLOSS", &iuse::marloss );
    add_iuse( "MARLOSS_GEL", &iuse::marloss_gel );
    add_iuse( "MARLOSS_SEED", &iuse::marloss_seed );
    add_iuse( "MA_MANUAL", &iuse::ma_manual );
    add_iuse( "MCG_NOTE", &iuse::mcg_note );
    add_iuse( "MEDITATE", &iuse::meditate );
    add_iuse( "METH", &iuse::meth );
    add_iuse( "MININUKE", &iuse::mininuke );
    add_iuse( "MISC_REPAIR", &iuse::misc_repair );
    add_iuse( "MOLOTOV_LIT", &iuse::molotov_lit );
    add_iuse( "MOP", &iuse::mop );
    add_iuse( "MP3", &iuse::mp3 );
    add_iuse( "MP3_ON", &iuse::mp3_on );
    add_iuse( "MULTICOOKER", &iuse::multicooker );
    add_iuse( "MUTAGEN", &iuse::mutagen );
    add_iuse( "MUT_IV", &iuse::mut_iv );
    add_iuse( "MUT_IV", &iuse::mut_iv );
    add_iuse( "MYCUS", &iuse::mycus );
    add_iuse( "NOISE_EMITTER_OFF", &iuse::noise_emitter_off );
    add_iuse( "NOISE_EMITTER_ON", &iuse::noise_emitter_on );
    add_iuse( "OXYGEN_BOTTLE", &iuse::oxygen_bottle );
    add_iuse( "OXYTORCH", &iuse::oxytorch );
    add_iuse( "PACK_ITEM", &iuse::pack_item );
    add_iuse( "PHEROMONE", &iuse::pheromone );
    add_iuse( "PICKAXE", &iuse::pickaxe );
    add_iuse( "PIPEBOMB_ACT", &iuse::pipebomb_act );
    add_iuse( "PLANTBLECH", &iuse::plantblech );
    add_iuse( "POISON", &iuse::poison );
    add_iuse( "PORTABLE_GAME", &iuse::portable_game );
    add_iuse( "PORTABLE_STRUCTURE", &iuse::portable_structure );
    add_iuse( "PORTAL", &iuse::portal );
    add_iuse( "PROZAC", &iuse::prozac );
    add_iuse( "PURIFIER", &iuse::purifier );
    add_iuse( "PURIFY_IV", &iuse::purify_iv );
    add_iuse( "RADGLOVE", &iuse::radglove );
    add_iuse( "RADIOCAR", &iuse::radiocar );
    add_iuse( "RADIOCARON", &iuse::radiocaron );
    add_iuse( "RADIOCONTROL", &iuse::radiocontrol );
    add_iuse( "RADIO_MOD", &iuse::radio_mod );
    add_iuse( "RADIO_OFF", &iuse::radio_off );
    add_iuse( "RADIO_ON", &iuse::radio_on );
    add_iuse( "REMOTEVEH", &iuse::remoteveh );
    add_iuse( "REMOVE_ALL_MODS", &iuse::remove_all_mods );
    add_iuse( "RM13ARMOR_OFF", &iuse::rm13armor_off );
    add_iuse( "RM13ARMOR_ON", &iuse::rm13armor_on );
    add_iuse( "ROBOTCONTROL", &iuse::robotcontrol );
    add_iuse( "ROYAL_JELLY", &iuse::royal_jelly );
    add_iuse( "SAW_BARREL", &iuse::saw_barrel );
    add_iuse( "SEED", &iuse::seed );
    add_iuse( "SEWAGE", &iuse::sewage );
    add_iuse( "SEW_ADVANCED", &iuse::sew_advanced );
    add_iuse( "SHAVEKIT", &iuse::shavekit );
    add_iuse( "SHOCKTONFA_OFF", &iuse::shocktonfa_off );
    add_iuse( "SHOCKTONFA_ON", &iuse::shocktonfa_on );
    add_iuse( "SIPHON", &iuse::siphon );
    add_iuse( "SLEEP", &iuse::sleep );
    add_iuse( "SMOKING", &iuse::smoking );
    add_iuse( "SPRAY_CAN", &iuse::spray_can );
    add_iuse( "STIMPACK", &iuse::stimpack );
    add_iuse( "TAZER", &iuse::tazer );
    add_iuse( "TAZER2", &iuse::tazer2 );
    add_iuse( "TELEPORT", &iuse::teleport );
    add_iuse( "THORAZINE", &iuse::thorazine );
    add_iuse( "THROWABLE_EXTINGUISHER_ACT", &iuse::throwable_extinguisher_act );
    add_iuse( "TORCH_LIT", &iuse::torch_lit );
    add_iuse( "TOWEL", &iuse::towel );
    add_iuse( "TRIMMER_OFF", &iuse::trimmer_off );
    add_iuse( "TRIMMER_ON", &iuse::trimmer_on );
    add_iuse( "UNFOLD_GENERIC", &iuse::unfold_generic );
    add_iuse( "UNPACK_ITEM", &iuse::unpack_item );
    add_iuse( "VACCINE", &iuse::vaccine );
    add_iuse( "VACUTAINER", &iuse::vacutainer );
    add_iuse( "VIBE", &iuse::vibe );
    add_iuse( "VORTEX", &iuse::vortex );
    add_iuse( "WASHCLOTHES", &iuse::washclothes );
    add_iuse( "WATER_PURIFIER", &iuse::water_purifier );
    add_iuse( "WEATHER_TOOL", &iuse::weather_tool );
    add_iuse( "WEED_BROWNIE", &iuse::weed_brownie );
    add_iuse( "XANAX", &iuse::xanax );

    add_actor( new ammobelt_actor() );
    add_actor( new bandolier_actor() );
    add_actor( new cauterize_actor() );
    add_actor( new consume_drug_iuse() );
    add_actor( new delayed_transform_iuse() );
    add_actor( new enzlave_actor() );
    add_actor( new explosion_iuse() );
    add_actor( new firestarter_actor() );
    add_actor( new fireweapon_off_actor() );
    add_actor( new fireweapon_on_actor() );
    add_actor( new heal_actor() );
    add_actor( new holster_actor() );
    add_actor( new inscribe_actor() );
    add_actor( new iuse_transform() );
    add_actor( new countdown_actor() );
    add_actor( new manualnoise_actor() );
    add_actor( new musical_instrument_actor() );
    add_actor( new pick_lock_actor() );
    add_actor( new place_monster_iuse() );
    add_actor( new reveal_map_actor() );
    add_actor( new salvage_actor() );
    add_actor( new unfold_vehicle_iuse() );
    add_actor( new ups_based_armor_actor() );
    add_actor( new place_trap_actor() );

    // An empty dummy group, it will not spawn anything. However, it makes that item group
    // id valid, so it can be used all over the place without need to explicitly check for it.
    m_template_groups["EMPTY_GROUP"] = new Item_group( Item_group::G_COLLECTION, 100 );
}

bool Item_factory::check_ammo_type( std::ostream &msg, const ammotype& ammo ) const
{
    if ( ammo.is_null() ) {
        return false;
    }

    if( !ammo.is_valid() ) {
        msg << string_format("ammo type %s is not known", ammo.c_str()) << "\n";
        return false;
    }

    if( std::none_of( m_templates.begin(), m_templates.end(), [&ammo]( const decltype(m_templates)::value_type& e ) {
        return e.second->ammo && e.second->ammo->type.count( ammo );
    } ) ) {
        msg << string_format("there is no actual ammo of type %s defined", ammo.c_str()) << "\n";
        return false;
    }
    return true;
}

void Item_factory::check_definitions() const
{
    for( const auto &elem : m_templates ) {
        std::ostringstream msg;
        const itype *type = elem.second.get();

        if( !type->category ) {
            msg << "undefined category " << type->category_force << "\n";
        }

        if( type->weight < 0 ) {
            msg << "negative weight" << "\n";
        }
        if( type->volume < 0 ) {
            msg << "negative volume" << "\n";
        }
        if( type->price < 0 ) {
            msg << "negative price" << "\n";
        }
        if( type->description.empty() ) {
            msg << "empty description" << "\n";
        }

        for( auto mat_id : type->materials ) {
            if( mat_id.str() == "null" || !mat_id.is_valid() ) {
                msg << string_format("invalid material %s", mat_id.c_str()) << "\n";
            }
        }

        if( type->sym.empty() ) {
            msg << "symbol not defined" << "\n";
        } else if( utf8_width( type->sym ) != 1 ) {
            msg << "symbol must be exactly one console cell width" << "\n";
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

        for( const auto& e : type->emits ) {
            if( !e.is_valid() ) {
                msg << string_format( "item %s has unknown emit source %s", type->id.c_str(), e.c_str() ) << "\n";
            }
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
                msg << "must define at least one ammo type" << "\n";
            }
            for( const auto &e : type->ammo->type ) {
                check_ammo_type( msg, e );
            }
            if( type->ammo->casing != "null" && !has_template( type->ammo->casing ) ) {
                msg << string_format( "invalid casing property %s", type->ammo->casing.c_str() ) << "\n";
            }
            if( type->ammo->drop != "null" && !has_template( type->ammo->drop ) ) {
                msg << string_format( "invalid drop item %s", type->ammo->drop.c_str() ) << "\n";
            }
        }
        if( type->gun ) {
            check_ammo_type( msg, type->gun->ammo );

            if( !type->gun->ammo ) {
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
                    msg << "missing or duplicate clip_size or magazine" << "\n";
                }

                if( type->item_tags.count( "RELOAD_AND_SHOOT" ) && !type->magazines.empty() ) {
                    msg << "RELOAD_AND_SHOOT cannot be used with magazines" << "\n";
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
            if( type->gunmod->location.empty() ) {
                    msg << "gunmod does not specify location" << "\n";
            }

        }
        if( type->mod ) {
            check_ammo_type( msg, type->mod->ammo_modifier );

            for( const auto &e : type->mod->acceptable_ammo ) {
                check_ammo_type( msg, e );
            }

            for( const auto &e : type->mod->magazine_adaptor ) {
                check_ammo_type( msg, e.first );
                for( const itype_id &opt : e.second ) {
                    const itype *mag = find_template( opt );
                    if( !mag->magazine || mag->magazine->type != e.first ) {
                        msg << "invalid magazine " << opt << " in magazine adaptor\n";
                    }
                }
            }
        }
        if( type->magazine ) {
            check_ammo_type( msg, type->magazine->type );
            if( !type->magazine->type ) {
                msg << "magazine did not specify ammo type" << "\n";
            }
            if( type->magazine->capacity < 0 ) {
                msg << string_format("invalid capacity %i", type->magazine->capacity) << "\n";
            }
            if( type->magazine->count < 0 || type->magazine->count > type->magazine->capacity ) {
                msg << string_format("invalid count %i", type->magazine->count) << "\n";
            }
            const itype *da = find_template( type->magazine->default_ammo );
            if( !( da->ammo && da->ammo->type.count( type->magazine->type ) ) ) {
                msg << string_format( "invalid default_ammo %s", type->magazine->default_ammo.c_str() ) << "\n";
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
            for( const auto& opt : typ.second ) {
                const itype *mag = find_template( opt );
                if( !mag->magazine || mag->magazine->type != typ.first ) {
                    msg << "invalid magazine " << opt << "\n";
                }
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

        if( type->container != nullptr ) {
            if( type->container->seals && type->container->unseals_into != "null" ) {
                msg << string_format("Resealable container unseals_into %s", type->container->unseals_into.c_str() ) << "\n";
            }
            if( type->container->contains <= 0 ) {
                msg << string_format("\"contains\" (%d) must be >0", type->container->contains ) << "\n";
            }
            if( !has_template( type->container->unseals_into ) ) {
                msg << string_format("unseals_into invalid id", type->container->unseals_into.c_str() ) << "\n";
            }
        }
        if (msg.str().empty()) {
            continue;
        }
        debugmsg( "warnings for type %s:\n%s", type->id.c_str(), msg.str().c_str() );
    }
    for( const auto& e : migrations ) {
        if( !m_templates.count( e.second.replace ) ) {
            debugmsg( "Invalid migration target: %s", e.second.replace.c_str() );
        }
        for( const auto& c : e.second.contents ) {
            if( !m_templates.count( c ) ) {
                debugmsg( "Invalid migration contents: %s", c.c_str() );
            }
        }
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
void Item_factory::load_slot( std::unique_ptr<SlotType> &slotptr, JsonObject &jo, const std::string &src )
{
    if( !slotptr ) {
        slotptr.reset( new SlotType() );
    }
    load( *slotptr, jo, src );
}

template<typename SlotType>
void Item_factory::load_slot_optional( std::unique_ptr<SlotType> &slotptr, JsonObject &jo,
                                       const std::string &member, const std::string &src )
{
    if( !jo.has_member( member ) ) {
        return;
    }
    JsonObject slotjo = jo.get_object( member );
    load_slot( slotptr, slotjo, src );
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

itype * Item_factory::load_definition( JsonObject& jo, const std::string &src ) {
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

    deferred.emplace_back( jo.str(), src );
    return nullptr;
}

void Item_factory::load( islot_artifact &slot, JsonObject &jo, const std::string & )
{
    slot.charge_type = jo.get_enum_value( "charge_type", ARTC_NULL );
    load_optional_enum_array( slot.effects_wielded, jo, "effects_wielded" );
    load_optional_enum_array( slot.effects_activated, jo, "effects_activated" );
    load_optional_enum_array( slot.effects_carried, jo, "effects_carried" );
    load_optional_enum_array( slot.effects_worn, jo, "effects_worn" );
}

void Item_factory::load( islot_ammo &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "core";

    assign( jo, "ammo_type", slot.type, strict );
    assign( jo, "casing", slot.casing, strict );
    assign( jo, "drop", slot.drop, strict );
    assign( jo, "drop_chance", slot.drop_chance, strict, 0.0f, 1.0f );
    assign( jo, "drop_active", slot.drop_active, strict );
    assign( jo, "damage", slot.damage, strict, 0 );
    assign( jo, "pierce", slot.pierce, strict, 0 );
    assign( jo, "range", slot.range, strict, 0 );
    assign( jo, "dispersion", slot.dispersion, strict, 0 );
    assign( jo, "recoil", slot.recoil, strict, 0 );
    assign( jo, "count", slot.def_charges, strict, 1L );
    assign( jo, "loudness", slot.loudness, strict, 0 );
    assign( jo, "effects", slot.ammo_effects, strict );
}

void Item_factory::load_ammo( JsonObject &jo, const std::string &src )
{
    auto def = load_definition( jo, src );
    if( def) {
        assign( jo, "stack_size", def->stack_size, src == "core", 1 );
        load_slot( def->ammo, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_engine &slot, JsonObject &jo, const std::string & )
{
    assign( jo, "displacement", slot.displacement );
    assign( jo, "faults", slot.faults );
}

void Item_factory::load_engine( JsonObject &jo, const std::string &src )
{
    auto def = load_definition( jo, src );
    if( def) {
        load_slot( def->engine, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_wheel &slot, JsonObject &jo, const std::string & )
{
    assign( jo, "diameter", slot.diameter );
    assign( jo, "width", slot.width );
}

void Item_factory::load_wheel( JsonObject &jo, const std::string &src )
{
    auto def = load_definition( jo, src );
    if( def) {
        load_slot( def->wheel, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_gun &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "core";

    if( jo.has_member( "burst" ) && jo.has_member( "modes" ) ) {
        jo.throw_error( "cannot specify both burst and modes", "burst" );
    }

    assign( jo, "skill", slot.skill_used, strict );
    assign( jo, "ammo", slot.ammo, strict );
    assign( jo, "range", slot.range, strict );
    assign( jo, "ranged_damage", slot.damage, strict );
    assign( jo, "pierce", slot.pierce, strict );
    assign( jo, "dispersion", slot.dispersion, strict );
    assign( jo, "sight_dispersion", slot.sight_dispersion, strict, 0, int( MIN_RECOIL ) );
    assign( jo, "recoil", slot.recoil, strict, 0 );
    assign( jo, "handling", slot.handling, strict );
    assign( jo, "durability", slot.durability, strict, 0, 10 );
    assign( jo, "burst", slot.burst, strict, 1 );
    assign( jo, "loudness", slot.loudness, strict );
    assign( jo, "clip_size", slot.clip, strict, 0 );
    assign( jo, "reload", slot.reload_time, strict, 0 );
    assign( jo, "reload_noise", slot.reload_noise, strict );
    assign( jo, "reload_noise_volume", slot.reload_noise_volume, strict, 0 );
    assign( jo, "barrel_length", slot.barrel_length, strict, 0 );
    assign( jo, "built_in_mods", slot.built_in_mods, strict );
    assign( jo, "default_mods", slot.default_mods, strict );
    assign( jo, "ups_charges", slot.ups_charges, strict, 0 );
    assign( jo, "ammo_effects", slot.ammo_effects, strict );

    if( jo.has_array( "valid_mod_locations" ) ) {
        slot.valid_mod_locations.clear();
        JsonArray jarr = jo.get_array( "valid_mod_locations" );
        while( jarr.has_more() ) {
            JsonArray curr = jarr.next_array();
            slot.valid_mod_locations.emplace( curr.get_string( 0 ), curr.get_int( 1 ) );
        }
    }

    if( jo.has_array( "modes" ) ) {
        slot.modes.clear();
        JsonArray jarr = jo.get_array( "modes" );
        while( jarr.has_more() ) {
            JsonArray curr = jarr.next_array();

            std::tuple<std::string, int, std::set<std::string>> mode(
                curr.get_string( 1 ),
                curr.get_int( 2 ),
                curr.size() >= 4 ? curr.get_tags( 3 ) : std::set<std::string>()
            );
            slot.modes.emplace( curr.get_string( 0 ), mode );
        }
    }
}

void Item_factory::load( islot_spawn &slot, JsonObject &jo, const std::string & )
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

void Item_factory::load_gun( JsonObject &jo, const std::string &src )
{
    auto def = load_definition( jo, src );
    if( def) {
        load_slot( def->gun, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load_armor( JsonObject &jo, const std::string &src )
{
    auto def = load_definition( jo,src );
    if( def) {
        load_slot( def->armor, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_armor &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "core";

    assign( jo, "encumbrance", slot.encumber, strict, 0 );
    assign( jo, "coverage", slot.coverage, strict, 0, 100 );
    assign( jo, "material_thickness", slot.thickness, strict, 0 );
    assign( jo, "environmental_protection", slot.env_resist, strict, 0 );
    assign( jo, "warmth", slot.warmth, strict, 0 );
    assign( jo, "storage", slot.storage, strict, 0 );
    assign( jo, "power_armor", slot.power_armor, strict );

    assign_coverage_from_json( jo, "covers", slot.covers, slot.sided );
}

void Item_factory::load( islot_tool &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "core";

    // @todo update tool slot to use signed integers (int) throughout
    assign( jo, "ammo", slot.ammo_id, strict );
    assign( jo, "max_charges", slot.max_charges, strict, 0L );
    assign( jo, "initial_charges", slot.def_charges, strict, 0L );
    assign( jo, "charges_per_use", slot.charges_per_use, strict, static_cast<decltype( slot.charges_per_use )>( 0 ) );
    assign( jo, "turns_per_charge", slot.turns_per_charge, strict, static_cast<decltype( slot.turns_per_charge )>( 0 ) );
    assign( jo, "revert_to", slot.revert_to, strict );
    assign( jo, "revert_msg", slot.revert_msg, strict );
    assign( jo, "sub", slot.subtype, strict );
}

void Item_factory::load_tool( JsonObject &jo, const std::string &src )
{
    auto def = load_definition( jo, src );
    if( def ) {
        load_slot( def->tool, jo, src );
        load_basic_info( jo, def, src );
        load_slot( def->spawn, jo, src ); // @todo deprecate
    }
}

void Item_factory::load( islot_mod &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "core";

    assign( jo, "ammo_modifier", slot.ammo_modifier, strict );
    assign( jo, "capacity_multiplier", slot.capacity_multiplier, strict );

    if( jo.has_member( "acceptable_ammo" ) ) {
        slot.acceptable_ammo.clear();
        for( auto &e : jo.get_tags( "acceptable_ammo" ) ) {
            slot.acceptable_ammo.insert( ammotype( e ) );
        }
    }

    JsonArray mags = jo.get_array( "magazine_adaptor" );
    if( !mags.empty() ) {
        slot.magazine_adaptor.clear();
    }
    while( mags.has_more() ) {
        JsonArray arr = mags.next_array();

        ammotype ammo( arr.get_string( 0 ) ); // an ammo type (eg. 9mm)
        JsonArray compat = arr.get_array( 1 ); // compatible magazines for this ammo type

        while( compat.has_more() ) {
            slot.magazine_adaptor[ ammo ].insert( compat.next_string() );
        }
    }
}

void Item_factory::load_toolmod( JsonObject &jo, const std::string &src )
{
    auto def = load_definition( jo, src );
    if( def ) {
        load_slot( def->mod, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load_tool_armor( JsonObject &jo, const std::string &src )
{
    auto def = load_definition( jo, src );
    if( def ) {
        load_slot( def->tool, jo, src );
        load_slot( def->armor, jo, src );
        load_basic_info( jo, def, src );
        load_slot( def->spawn, jo, src ); // @todo deprecate
    }
}

void Item_factory::load( islot_book &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "core";

    assign( jo, "max_level", slot.level, strict, 0, MAX_SKILL );
    assign( jo, "required_level", slot.req, strict, 0, MAX_SKILL );
    assign( jo, "fun", slot.fun, strict );
    assign( jo, "intelligence", slot.intel, strict, 0 );
    assign( jo, "time", slot.time, strict, 0 );
    assign( jo, "skill", slot.skill, strict );
    assign( jo, "chapters", slot.chapters, strict, 0 );
}

void Item_factory::load_book( JsonObject &jo, const std::string &src )
{
    auto def = load_definition( jo, src );
    if( def ) {
        load_slot( def->book, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_comestible &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "core";

    assign( jo, "comestible_type", slot.comesttype, strict );
    assign( jo, "tool", slot.tool, strict );
    assign( jo, "charges", slot.def_charges, strict, 1L );
    assign( jo, "quench", slot.quench, strict );
    assign( jo, "fun", slot.fun, strict );
    assign( jo, "stim", slot.stim, strict );
    assign( jo, "healthy", slot.healthy, strict );
    assign( jo, "parasites", slot.parasites, strict, 0 );
    assign( jo, "spoils_in", slot.spoils, strict, 0 );

    if( jo.has_string( "addiction_type" ) ) {
        slot.add = addiction_type( jo.get_string( "addiction_type" ) );
    }

    assign( jo, "addiction_potential", slot.addict, strict );

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

void Item_factory::load( islot_brewable &slot, JsonObject &jo, const std::string & )
{
    slot.time = jo.get_int( "time" );
    slot.results = jo.get_string_array( "results" );
}

void Item_factory::load_comestible( JsonObject &jo, const std::string &src )
{
    auto def = load_definition( jo, src );
    if( def ) {
        assign( jo, "stack_size", def->stack_size, src == "core", 1 );
        load_slot( def->comestible, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load_container( JsonObject &jo, const std::string &src )
{
    auto def = load_definition( jo, src );
    if( def ) {
        load_slot( def->container, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_seed &slot, JsonObject &jo, const std::string & )
{
    slot.grow = jo.get_int( "grow" );
    slot.fruit_div = jo.get_int( "fruit_div", 1 );
    slot.plant_name = _( jo.get_string( "plant_name" ).c_str() );
    slot.fruit_id = jo.get_string( "fruit" );
    slot.spawn_seeds = jo.get_bool( "seeds", true );
    slot.byproducts = jo.get_string_array( "byproducts" );
}

void Item_factory::load( islot_container &slot, JsonObject &jo, const std::string & )
{
    assign( jo, "contains", slot.contains );
    assign( jo, "seals", slot.seals );
    assign( jo, "watertight", slot.watertight );
    assign( jo, "preserves", slot.preserves );
    assign( jo, "unseals_into", slot.unseals_into );
}

void Item_factory::load( islot_gunmod &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "core";

    assign( jo, "damage_modifier", slot.damage );
    assign( jo, "loudness_modifier", slot.loudness );
    assign( jo, "location", slot.location );
    assign( jo, "dispersion_modifier", slot.dispersion );
    assign( jo, "sight_dispersion", slot.sight_dispersion );
    assign( jo, "aim_cost", slot.aim_cost, strict, 0 );
    assign( jo, "handling_modifier", slot.handling, strict );
    assign( jo, "range_modifier", slot.range );
    assign( jo, "ups_charges", slot.ups_charges );
    assign( jo, "install_time", slot.install_time );

    if( jo.has_member( "mod_targets" ) ) {
        slot.usable = jo.get_tags( "mod_targets");
    }

    if( jo.has_array( "mode_modifier" ) ) {
        slot.mode_modifier.clear();
        JsonArray jarr = jo.get_array( "mode_modifier" );
        while( jarr.has_more() ) {
            JsonArray curr = jarr.next_array();

            std::tuple<std::string, int, std::set<std::string>> mode(
                curr.get_string( 1 ),
                curr.get_int( 2 ),
                curr.size() >= 4 ? curr.get_tags( 3 ) : std::set<std::string>()
            );
            slot.mode_modifier.emplace( curr.get_string( 0 ), mode );
        }
    }
}

void Item_factory::load_gunmod( JsonObject &jo, const std::string &src )
{
    auto def = load_definition( jo, src );
    if( def ) {
        load_slot( def->gunmod, jo, src );
        load_slot( def->mod, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_magazine &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "core";

    assign( jo, "ammo_type", slot.type, strict );
    assign( jo, "capacity", slot.capacity, strict, 0 );
    assign( jo, "count", slot.count, strict, 0 );
    assign( jo, "default_ammo", slot.default_ammo, strict );
    assign( jo, "reliability", slot.reliability, strict, 0, 10 );
    assign( jo, "reload_time", slot.reload_time, strict, 0 );
    assign( jo, "linkage", slot.linkage, strict );
}

void Item_factory::load_magazine( JsonObject &jo, const std::string &src )
{
    auto def = load_definition( jo, src );
    if( def) {
        load_slot( def->magazine, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_bionic &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "core";

    assign( jo, "difficulty", slot.difficulty, strict, 0 );
    // TODO: must be the same as the item type id, for compatibility
    assign( jo, "id", slot.bionic_id, strict );
}

void Item_factory::load_bionic( JsonObject &jo, const std::string &src )
{
    auto def = load_definition( jo, src );
    if( def ) {
        load_slot( def->bionic, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load_generic( JsonObject &jo, const std::string &src )
{
    auto def = load_definition( jo, src );
    if( def ) {
        load_basic_info( jo, def, src );
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

void npc_implied_flags( itype &item_template )
{
    if( item_template.use_methods.count( "explosion" ) > 0 ) {
        item_template.item_tags.insert( "DANGEROUS" );
    }

    if( item_template.item_tags.count( "DANGEROUS" ) > 0 ) {
        item_template.item_tags.insert( "NPC_THROW_NOW" );
    }

    if( item_template.item_tags.count( "BOMB" ) > 0 ) {
        item_template.item_tags.insert( "NPC_ACTIVATE" );
    }

    if( item_template.item_tags.count( "NPC_THROW_NOW" ) > 0 ) {
        item_template.item_tags.insert( "NPC_THROWN" );
    }

    if( item_template.item_tags.count( "NPC_ACTIVATE" ) > 0 ||
        item_template.item_tags.count( "NPC_THROWN" ) > 0 ) {
        item_template.item_tags.insert( "NPC_ALT_ATTACK" );
    }

    if( item_template.item_tags.count( "DANGEROUS" ) > 0 ||
        item_template.item_tags.count( "PSEUDO" ) > 0 ) {
        item_template.item_tags.insert( "TRADER_AVOID" );
    }
}

void Item_factory::load_basic_info( JsonObject &jo, itype *new_item_template, const std::string &src )
{
    bool strict = src == "core";

    if( jo.has_string( "abstract" ) ) {
        new_item_template->id = jo.get_string( "abstract" );
        m_abstracts[ new_item_template->id ].reset( new_item_template );
    } else {
        new_item_template->id = jo.get_string( "id" );
        m_templates[ new_item_template->id ].reset( new_item_template );
    }

    assign( jo, "category", new_item_template->category_force, strict );
    assign( jo, "weight", new_item_template->weight, strict, 0 );
    assign( jo, "volume", new_item_template->volume );
    assign( jo, "price", new_item_template->price );
    assign( jo, "price_postapoc", new_item_template->price_post );
    assign( jo, "integral_volume", new_item_template->integral_volume );
    assign( jo, "bashing", new_item_template->melee[DT_BASH], strict, 0 );
    assign( jo, "cutting", new_item_template->melee[DT_CUT], strict, 0 );
    assign( jo, "to_hit", new_item_template->m_to_hit, strict );
    assign( jo, "container", new_item_template->default_container );
    assign( jo, "rigid", new_item_template->rigid );
    assign( jo, "min_strength", new_item_template->min_str );
    assign( jo, "min_dexterity", new_item_template->min_dex );
    assign( jo, "min_intelligence", new_item_template->min_int );
    assign( jo, "min_perception", new_item_template->min_per );
    assign( jo, "emits", new_item_template->emits );
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
        new_item_template->sym = jo.get_string( "symbol" );
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

        ammotype ammo( arr.get_string( 0 ) ); // an ammo type (eg. 9mm)
        JsonArray compat = arr.get_array( 1 ); // compatible magazines for this ammo type

        // the first magazine for this ammo type is the default;
        new_item_template->magazine_default[ ammo ] = compat.get_string( 0 );

        while( compat.has_more() ) {
            new_item_template->magazines[ ammo ].insert( compat.next_string() );
        }
    }

    JsonArray jarr = jo.get_array( "min_skills" );
    if( !jarr.empty() ) {
        new_item_template->min_skills.clear();
    }
    while( jarr.has_more() ) {
        JsonArray cur = jarr.next_array();
        const auto sk = skill_id( cur.get_string( 0 ) );
        if( !sk.is_valid() ) {
            jo.throw_error( string_format( "invalid skill: %s", sk.c_str() ), "min_skills" );
        }
        new_item_template->min_skills[ sk ] = cur.get_int( 1 );
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

    assign( jo, "countdown_interval", new_item_template->countdown_interval );
    assign( jo, "countdown_destroy", new_item_template->countdown_destroy );

    if( jo.has_string( "countdown_action" ) ) {
        new_item_template->countdown_action = usage_from_string( jo.get_string( "countdown_action" ) );

    } else if( jo.has_object( "countdown_action" ) ) {
        auto tmp = jo.get_object( "countdown_action" );
        new_item_template->countdown_action = usage_from_object( tmp ).second;
    }

    load_slot_optional( new_item_template->container, jo, "container_data", src );
    load_slot_optional( new_item_template->armor, jo, "armor_data", src );
    load_slot_optional( new_item_template->book, jo, "book_data", src );
    load_slot_optional( new_item_template->gun, jo, "gun_data", src );
    load_slot_optional( new_item_template->bionic, jo, "bionic_data", src );
    load_slot_optional( new_item_template->spawn, jo, "spawn_data", src );
    load_slot_optional( new_item_template->ammo, jo, "ammo_data", src );
    load_slot_optional( new_item_template->seed, jo, "seed_data", src );
    load_slot_optional( new_item_template->artifact, jo, "artifact_data", src );
    load_slot_optional( new_item_template->brewable, jo, "brewable", src );

    // optional gunmod slot may also specify mod data
    load_slot_optional( new_item_template->gunmod, jo, "gunmod_data", src );
    load_slot_optional( new_item_template->mod, jo, "gunmod_data", src );
}

void Item_factory::load_item_category(JsonObject &jo)
{
    const std::string id = jo.get_string("id");
    // reuse an existing definition,
    // override the name and the sort_rank if
    // these are present in the json
    item_category &cat = categories[id];
    cat.id = id;
    if (jo.has_member("name")) {
        cat.name = _(jo.get_string("name").c_str());
    }
    if (jo.has_member("sort_rank")) {
        cat.sort_rank = jo.get_int("sort_rank");
    }
}

void Item_factory::load_migration( JsonObject &jo )
{
    migration m;
    m.id = jo.get_string( "id" );
    assign( jo, "replace", m.replace );
    assign( jo, "flags", m.flags );
    assign( jo, "charges", m.charges );
    assign( jo, "contents", m.contents );

    migrations[ jo.get_string( "id" ) ] = m;
}

itype_id Item_factory::migrate_id( const itype_id& id )
{
    auto iter = migrations.find( id );
    return iter != migrations.end() ? iter->second.replace : id;
}

void Item_factory::migrate_item( const itype_id& id, item& obj )
{
    auto iter = migrations.find( id );
    if( iter != migrations.end() ) {
        std::copy( iter->second.flags.begin(), iter->second.flags.end(), std::inserter( obj.item_tags, obj.item_tags.begin() ) );
        obj.charges = iter->second.charges;

        for( const auto& c: iter->second.contents ) {
            if( std::none_of( obj.contents.begin(), obj.contents.end(), [&]( const item& e ) { return e.typeId() == c; } ) ) {
                obj.emplace_back( c, obj.bday );
            }
        }

        // check contents of migrated containers do not exceed capacity
        if( obj.is_container() && !obj.contents.empty() ) {
            item &child = obj.contents.back();
            const long capacity = child.charges_per_volume( obj.get_container_capacity() );
            child.charges = std::min( child.charges, capacity );
        }
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

    categories.clear();

    // Also clear functions refering to lua
    iuse_function_list.clear();

    m_templates.clear();
    item_blacklist.clear();
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
                                              std::map<std::string, use_function> &use_methods )
{
    if( !jo.has_member( member ) ) {
        return;
    }

    use_methods.clear();
    if( jo.has_array( member ) ) {
        JsonArray jarr = jo.get_array( member );
        while( jarr.has_more() ) {
            if( jarr.test_string() ) {
                std::string type = jarr.next_string();
                use_methods.emplace( type, usage_from_string( type ) );
            } else if( jarr.test_object() ) {
                auto obj = jarr.next_object();
                use_methods.insert( usage_from_object( obj ) );
            } else {
                jarr.throw_error( "array element is neither string nor object." );
            }

        }
    } else {
        if( jo.has_string( member ) ) {
            std::string type = jo.get_string( member );
            use_methods.emplace( type, usage_from_string( type ) );
        } else if( jo.has_object( member ) ) {
            auto obj = jo.get_object( member );
            use_methods.insert( usage_from_object( obj ) );
        } else {
            jo.throw_error( "member 'use_action' is neither string nor object." );
        }

    }
}

std::pair<std::string, use_function> Item_factory::usage_from_object( JsonObject &obj ) const
{
    auto type = obj.get_string( "type" );

    use_function method;
    if( type == "repair_item" ) {
        type = obj.get_string( "item_action_type" );
        method = use_function( new repair_item_actor( type ) );
    } else {
        method = usage_from_string( type );
    }

    if( !method.get_actor_ptr() ) {
        obj.throw_error( "unknown use_action", "type" );
    }

    method.get_actor_ptr()->load( obj );
    return std::make_pair( type, method );
}

use_function Item_factory::usage_from_string( const std::string &type ) const
{
    auto func = iuse_function_list.find( type );
    if( func != iuse_function_list.end() ) {
        return func->second;
    }

    // Otherwise, return a hardcoded function we know exists (hopefully)
    debugmsg( "Received unrecognized iuse function %s, using iuse::none instead", type.c_str() );
    return use_function();
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

const std::string calc_category( const itype &obj )
{
    if( obj.gun && !obj.gunmod ) {
        return "guns";
    }
    if( obj.magazine ) {
        return "magazines";
    }
    if( obj.ammo ) {
        return "ammo";
    }
    if( obj.tool ) {
        return "tools";
    }
    if( obj.armor ) {
        return "clothing";
    }
    if (obj.comestible) {
        return obj.comestible->comesttype == "MED" ? "drugs" : "food";
    }
    if( obj.book ) {
        return "books";
    }
    if( obj.gunmod ) {
        return "mods";
    }
    if( obj.bionic ) {
        return "bionics";
    }

    bool weap = std::any_of( obj.melee.begin(), obj.melee.end(), []( int qty ) {
        return qty > MELEE_STAT;
    } );

    return weap ? "weapons" : "other";
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
    menu.return_invalid = true;
    menu.text = _("Test which group?");
    for (size_t i = 0; i < groups.size(); i++) {
        menu.entries.push_back(uimenu_entry(i, true, -2, groups[i]));
    }
    //~ Spawn group menu: Menu entry to exit menu
    menu.entries.push_back(uimenu_entry(menu.entries.size(), true, -2, _("cancel")));
    while (true) {
        menu.query();
        const int index = menu.ret;
        if ( index >= (int)groups.size() || index < 0 ) {
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
        menu2.return_invalid = true;
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
