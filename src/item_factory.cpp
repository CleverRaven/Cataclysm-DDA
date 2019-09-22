#include "item_factory.h"

#include <cstdlib>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>
#include <sstream>
#include <array>
#include <iterator>
#include <stdexcept>
#include <type_traits>

#include "addiction.h"
#include "ammo.h"
#include "artifact.h"
#include "assign.h"
#include "catacharset.h"
#include "debug.h"
#include "enums.h"
#include "init.h"
#include "item.h"
#include "item_category.h"
#include "item_group.h"
#include "iuse_actor.h"
#include "json.h"
#include "material.h"
#include "options.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translations.h"
#include "ui.h"
#include "veh_type.h"
#include "vitamin.h"
#include "bodypart.h"
#include "calendar.h"
#include "color.h"
#include "damage.h"
#include "explosion.h"
#include "game_constants.h"
#include "optional.h"
#include "recipe.h"
#include "string_id.h"
#include "units.h"
#include "cata_utility.h"
#include "flat_set.h"

class player;
struct tripoint;

using t_string_set = std::set<std::string>;
static t_string_set item_blacklist;

static DynamicDataLoader::deferred_json deferred;

std::unique_ptr<Item_factory> item_controller = std::make_unique<Item_factory>();

static std::string calc_category( const itype &obj );
static void set_allergy_flags( itype &item_template );
static void hflesh_to_flesh( itype &item_template );
static void npc_implied_flags( itype &item_template );

extern const double MAX_RECOIL;

bool item_is_blacklisted( const std::string &id )
{
    return item_blacklist.count( id );
}

static void assign( JsonObject &jo, const std::string &name,
                    std::map<gun_mode_id, gun_modifier_data> &mods )
{
    if( !jo.has_array( name ) ) {
        return;
    }
    mods.clear();
    JsonArray jarr = jo.get_array( name );
    while( jarr.has_more() ) {
        JsonArray curr = jarr.next_array();
        mods.emplace( gun_mode_id( curr.get_string( 0 ) ), gun_modifier_data( curr.get_string( 1 ),
                      curr.get_int( 2 ), curr.size() >= 4 ? curr.get_tags( 3 ) : std::set<std::string>() ) );
    }
}

static bool assign_coverage_from_json( JsonObject &jo, const std::string &key,
                                       body_part_set &parts, bool &sided )
{
    auto parse = [&parts, &sided]( const std::string & val ) {
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
        sided |= val == "ARM_EITHER" || val == "HAND_EITHER" ||
                 val == "LEG_EITHER" || val == "FOOT_EITHER";
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

void Item_factory::finalize_pre( itype &obj )
{
    // TODO: separate repairing from reinforcing/enhancement
    if( obj.damage_max() == obj.damage_min() ) {
        obj.item_tags.insert( "NO_REPAIR" );
    }

    if( obj.item_tags.count( "STAB" ) || obj.item_tags.count( "SPEAR" ) ) {
        std::swap( obj.melee[DT_CUT], obj.melee[DT_STAB] );
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

    static const auto handle_legacy_ranged = []( common_ranged_data & ranged ) {
        if( ranged.legacy_damage != 0 ) {
            ranged.damage.add( damage_instance::physical( 0, 0, ranged.legacy_damage, ranged.legacy_pierce ) );
            ranged.legacy_damage = 0;
            ranged.legacy_pierce = 0;
        }
    };

    if( obj.gunmod ) {
        handle_legacy_ranged( *obj.gunmod );
    }

    if( obj.mod ) {
        std::string func = obj.gunmod ? "GUNMOD_ATTACH" : "TOOLMOD_ATTACH";
        obj.use_methods.emplace( func, usage_from_string( func ) );
    } else if( obj.gun ) {
        const std::string func = "detach_gunmods";
        obj.use_methods.emplace( func, usage_from_string( func ) );
    }

    if( get_option<bool>( "NO_FAULTS" ) ) {
        obj.faults.clear();
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
    if( obj.integral_volume < 0_ml ) {
        obj.integral_volume = obj.volume;
    }
    // use base weight if integral weight unspecified
    if( obj.integral_weight < 0_gram ) {
        obj.integral_weight = obj.weight;
    }
    // for ammo and comestibles stack size defaults to count of initial charges
    // Set max stack size to 200 to prevent integer overflow
    if( obj.count_by_charges() ) {
        if( obj.stack_size == 0 ) {
            obj.stack_size = obj.charges_default();
        } else if( obj.stack_size > 200 ) {
            debugmsg( obj.id + " stack size is too large, reducing to 200" );
            obj.stack_size = 200;
        }
    }

    // Items always should have some volume.
    // TODO: handle possible exception software?
    // TODO: make items with 0 volume an error during loading?
    if( obj.volume <= 0_ml ) {
        obj.volume = units::from_milliliter( 1 );
    }
    for( const auto &tag : obj.item_tags ) {
        if( tag.size() > 6 && tag.substr( 0, 6 ) == "LIGHT_" ) {
            obj.light_emission = std::max( atoi( tag.substr( 6 ).c_str() ), 0 );
        }
    }

    // Set max volume for containers to prevent integer overflow
    if( obj.container && obj.container->contains > 10000000_ml ) {
        debugmsg( obj.id + " storage volume is too large, reducing to 10000000" );
        obj.container->contains = 10000000_ml;
    }

    // for ammo not specifying loudness (or an explicit zero) derive value from other properties
    if( obj.ammo ) {
        handle_legacy_ranged( *obj.ammo );

        if( obj.ammo->loudness < 0 ) {
            obj.ammo->loudness = obj.ammo->range * 2;
            for( const damage_unit &du : obj.ammo->damage ) {
                obj.ammo->loudness += ( du.amount + du.res_pen ) * 2;
            }
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
                }
            };
            obj.ammo->special_cookoff = std::any_of( ammo_effects.begin(), ammo_effects.end(),
            []( const std::string & s ) {
                return special_cookoff_tags.count( s ) > 0;
            } );
        } else {
            obj.ammo->cookoff = false;
            obj.ammo->special_cookoff = false;
        }
    }
    // for magazines ensure default_ammo is set
    if( obj.magazine && obj.magazine->default_ammo == "NULL" ) {
        obj.magazine->default_ammo = ammotype( *obj.magazine->type.begin() )->default_ammotype();
    }
    if( obj.gun ) {
        handle_legacy_ranged( *obj.gun );
        // TODO: add explicit action field to gun definitions
        const auto defmode_name = [&]() {
            if( obj.gun->clip == 1 ) {
                return translate_marker( "manual" ); // break-type actions
            } else if( obj.gun->skill_used == skill_id( "pistol" ) && obj.item_tags.count( "RELOAD_ONE" ) ) {
                return translate_marker( "revolver" );
            } else {
                return translate_marker( "semi-auto" );
            }
        };

        // if the gun doesn't have a DEFAULT mode then add one now
        obj.gun->modes.emplace( gun_mode_id( "DEFAULT" ),
                                gun_modifier_data( defmode_name(), 1, std::set<std::string>() ) );

        // If a "gun" has a reach attack, give it an additional melee mode.
        if( obj.item_tags.count( "REACH_ATTACK" ) ) {
            obj.gun->modes.emplace( gun_mode_id( "MELEE" ),
                                    gun_modifier_data( translate_marker( "melee" ), 1,
            { "MELEE" } ) );
        }
        if( obj.gun->burst > 1 ) {
            // handle legacy JSON format
            obj.gun->modes.emplace( gun_mode_id( "AUTO" ),
                                    gun_modifier_data( translate_marker( "auto" ), obj.gun->burst,
                                            std::set<std::string>() ) );
        }

        if( obj.gun->handling < 0 ) {
            // TODO: specify in JSON via classes
            if( obj.gun->skill_used == skill_id( "rifle" ) ||
                obj.gun->skill_used == skill_id( "smg" ) ||
                obj.gun->skill_used == skill_id( "shotgun" ) ) {
                obj.gun->handling = 20;
            } else {
                obj.gun->handling = 10;
            }
        }

        obj.gun->reload_noise = _( obj.gun->reload_noise );

        // TODO: Move to jsons?
        if( obj.gun->skill_used == skill_id( "archery" ) ||
            obj.gun->skill_used == skill_id( "throw" ) ) {
            obj.item_tags.insert( "WATERPROOF_GUN" );
            obj.item_tags.insert( "NEVER_JAMS" );
            obj.gun->ammo_effects.insert( "NEVER_MISFIRES" );
        }
    }

    set_allergy_flags( obj );
    hflesh_to_flesh( obj );
    npc_implied_flags( obj );

    if( obj.comestible ) {
        if( get_option<bool>( "NO_VITAMINS" ) ) {
            obj.comestible->vitamins.clear();
        } else if( obj.comestible->vitamins.empty() && obj.comestible->healthy >= 0 ) {
            // Default vitamins of healthy comestibles to their edible base materials if none explicitly specified.
            auto healthy = std::max( obj.comestible->healthy, 1 ) * 10;
            auto mat = obj.materials;

            // TODO: migrate inedible comestibles to appropriate alternative types.
            mat.erase( std::remove_if( mat.begin(), mat.end(), []( const string_id<material_type> &m ) {
                return !m.obj().edible();
            } ), mat.end() );

            // For comestibles composed of multiple edible materials we calculate the average.
            for( const auto &v : vitamin::all() ) {
                if( obj.comestible->vitamins.find( v.first ) == obj.comestible->vitamins.end() ) {
                    for( const auto &m : mat ) {
                        obj.comestible->vitamins[ v.first ] += std::ceil( m.obj().vitamin( v.first ) * healthy /
                                                               mat.size() );
                    }
                }
            }
        }
    }

    if( obj.tool ) {
        if( !obj.tool->subtype.empty() && has_template( obj.tool->subtype ) ) {
            tool_subtypes[ obj.tool->subtype ].insert( obj.id );
        }
    }

    for( auto &e : obj.use_methods ) {
        e.second.get_actor_ptr()->finalize( obj.id );
    }

    if( obj.drop_action.get_actor_ptr() != nullptr ) {
        obj.drop_action.get_actor_ptr()->finalize( obj.id );
    }

    if( obj.item_tags.count( "PERSONAL" ) ) {
        obj.layer = PERSONAL_LAYER;
    } else if( obj.item_tags.count( "SKINTIGHT" ) ) {
        obj.layer = UNDERWEAR_LAYER;
    } else if( obj.item_tags.count( "WAIST" ) ) {
        obj.layer = WAIST_LAYER;
    } else if( obj.item_tags.count( "OUTER" ) ) {
        obj.layer = OUTER_LAYER;
    } else if( obj.item_tags.count( "BELTED" ) ) {
        obj.layer = BELTED_LAYER;
    } else if( obj.item_tags.count( "AURA" ) ) {
        obj.layer = AURA_LAYER;
    } else {
        obj.layer = REGULAR_LAYER;
    }

    if( obj.can_use( "MA_MANUAL" ) && obj.book && obj.book->martial_art.is_null() &&
        string_starts_with( obj.get_id(), "manual_" ) ) {
        // Legacy martial arts books rely on a hack whereby the name of the
        // martial art is derived from the item id
        obj.book->martial_art = matype_id( "style_" + obj.get_id().substr( 7 ) );
    }
}

void Item_factory::register_cached_uses( const itype &obj )
{
    for( auto &e : obj.use_methods ) {
        // can this item function as a repair tool?
        if( repair_actions.count( e.first ) ) {
            repair_tools.insert( obj.id );
        }

        // can this item be used to repair complex firearms?
        if( e.first == "GUN_REPAIR" ) {
            gun_tools.insert( obj.id );
        }

        // can this item be used to repair wood/paper/bone/chitin items?
        if( e.first == "MISC_REPAIR" ) {
            misc_tools.insert( obj.id );
        }
    }
}

void Item_factory::finalize_post( itype &obj )
{
    // handle complex firearms as a special case
    if( obj.gun && !obj.item_tags.count( "PRIMITIVE_RANGED_WEAPON" ) ) {
        std::copy( gun_tools.begin(), gun_tools.end(), std::inserter( obj.repair, obj.repair.begin() ) );
        return;
    }

    // handle wood/paper/bone/chitin items as a special case
    if( !obj.gun && !obj.count_by_charges() && std::any_of( obj.materials.begin(), obj.materials.end(),
    []( const material_id & m ) {
    return m == material_id( "wood" ) || m == material_id( "paper" ) ||
               m == material_id( "bone" ) || m == material_id( "chitin" ) || m == material_id( "acidchitin" );
    } ) ) {
        std::copy( misc_tools.begin(), misc_tools.end(), std::inserter( obj.repair, obj.repair.begin() ) );
        return;
    }

    // for each item iterate through potential repair tools
    for( const auto &tool : repair_tools ) {

        // check if item can be repaired with any of the actions?
        for( const auto &act : repair_actions ) {
            const use_function *func = m_templates[tool].get_use( act );
            if( func == nullptr ) {
                continue;
            }

            // tool has a possible repair action, check if the materials are compatible
            const auto &opts = dynamic_cast<const repair_item_actor *>( func->get_actor_ptr() )->materials;
            if( std::any_of( obj.materials.begin(), obj.materials.end(), [&opts]( const material_id & m ) {
            return opts.count( m ) > 0;
            } ) ) {
                obj.repair.insert( tool );
            }
        }
    }
}

void Item_factory::finalize()
{
    DynamicDataLoader::get_instance().load_deferred( deferred );

    finalize_item_blacklist();

    // we can no longer add or adjust static item templates
    frozen = true;

    for( auto &e : m_templates ) {
        finalize_pre( e.second );
        register_cached_uses( e.second );
    }

    for( auto &e : m_templates ) {
        finalize_post( e.second );
    }

    // We may actually have some runtimes here - ones loaded from saved game
    // TODO: support for runtimes that repair
    for( auto &e : m_runtimes ) {
        finalize_pre( *e.second );
        finalize_post( *e.second );
    }
}

void Item_factory::finalize_item_blacklist()
{
    for( t_string_set::const_iterator a = item_blacklist.begin(); a != item_blacklist.end(); ++a ) {
        if( !has_template( *a ) ) {
            debugmsg( "item on blacklist %s does not exist", a->c_str() );
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
        recipe_dictionary::delete_if( [&]( const recipe & r ) {
            return r.result() == e.first;
        } );
    }

    for( auto &vid : vehicle_prototype::get_all() ) {
        vehicle_prototype &prototype = const_cast<vehicle_prototype &>( vid.obj() );
        for( vehicle_item_spawn &vis : prototype.item_spawns ) {
            auto &vec = vis.item_ids;
            const auto iter = std::remove_if( vec.begin(), vec.end(), item_is_blacklisted );
            vec.erase( iter, vec.end() );
        }
    }
}

void Item_factory::load_item_blacklist( JsonObject &json )
{
    add_array_to_set( item_blacklist, json, "items" );
}

Item_factory::~Item_factory() = default;

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
        int use( player &p, item &it, bool a, const tripoint &pos ) const override {
            iuse tmp;
            return ( tmp.*cpp_function )( &p, &it, a, pos );
        }
        std::unique_ptr<iuse_actor> clone() const override {
            return std::make_unique<iuse_function_wrapper>( *this );
        }

        void load( JsonObject & ) override {}
};

class iuse_function_wrapper_with_info : public iuse_function_wrapper
{
    private:
        std::string info_string; // Untranslated
    public:
        iuse_function_wrapper_with_info(
            const std::string &type, const use_function_pointer f, const std::string &info )
            : iuse_function_wrapper( type, f ), info_string( info ) { }

        void info( const item &, std::vector<iteminfo> &info ) const override {
            info.emplace_back( "DESCRIPTION", _( info_string ) );
        }
        std::unique_ptr<iuse_actor> clone() const override {
            return std::make_unique<iuse_function_wrapper_with_info>( *this );
        }
};

use_function::use_function( const std::string &type, const use_function_pointer f )
    : use_function( std::make_unique<iuse_function_wrapper>( type, f ) ) {}

void Item_factory::add_iuse( const std::string &type, const use_function_pointer f )
{
    iuse_function_list[ type ] = use_function( type, f );
}

void Item_factory::add_iuse( const std::string &type, const use_function_pointer f,
                             const std::string &info )
{
    iuse_function_list[ type ] =
        use_function( std::make_unique<iuse_function_wrapper_with_info>( type, f, info ) );
}

void Item_factory::add_actor( std::unique_ptr<iuse_actor> ptr )
{
    std::string type = ptr->type;
    iuse_function_list[ type ] = use_function( std::move( ptr ) );
}

void Item_factory::add_item_type( const itype &def )
{
    if( m_runtimes.count( def.id ) > 0 ) {
        // Do NOT allow overwriting it, it's undefined behavior
        debugmsg( "Tried to add runtime type %s, but it exists already", def.id.c_str() );
        return;
    }

    auto &new_item_ptr = m_runtimes[ def.id ];
    new_item_ptr = std::make_unique<itype>( def );
    if( frozen ) {
        finalize_pre( *new_item_ptr );
        finalize_post( *new_item_ptr );
    }
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
    add_iuse( "ARROW_FLAMABLE", &iuse::arrow_flammable );
    add_iuse( "ARTIFACT", &iuse::artifact );
    add_iuse( "ATOMIC_CAFF", &iuse::atomic_caff );
    add_iuse( "AUTOCLAVE", &iuse::autoclave );
    add_iuse( "BELL", &iuse::bell );
    add_iuse( "BLECH", &iuse::blech );
    add_iuse( "BOLTCUTTERS", &iuse::boltcutters );
    add_iuse( "C4", &iuse::c4 );
    add_iuse( "CABLE_ATTACH", &iuse::cable_attach );
    add_iuse( "CAFF", &iuse::caff );
    add_iuse( "CAMERA", &iuse::camera );
    add_iuse( "CAN_GOO", &iuse::can_goo );
    add_iuse( "COIN_FLIP", &iuse::coin_flip );
    add_iuse( "DIRECTIONAL_HOLOGRAM", &iuse::directional_hologram );
    add_iuse( "CAPTURE_MONSTER_ACT", &iuse::capture_monster_act );
    add_iuse( "CAPTURE_MONSTER_VEH", &iuse::capture_monster_veh );
    add_iuse( "CARVER_OFF", &iuse::carver_off );
    add_iuse( "CARVER_ON", &iuse::carver_on );
    add_iuse( "CATFOOD", &iuse::catfood );
    add_iuse( "CATTLEFODDER", &iuse::feedcattle );
    add_iuse( "CHAINSAW_OFF", &iuse::chainsaw_off );
    add_iuse( "CHAINSAW_ON", &iuse::chainsaw_on );
    add_iuse( "CHEW", &iuse::chew );
    add_iuse( "RPGDIE", &iuse::rpgdie );
    add_iuse( "BIRDFOOD", &iuse::feedbird );
    add_iuse( "BURROW", &iuse::burrow );
    add_iuse( "CHOP_TREE", &iuse::chop_tree );
    add_iuse( "CHOP_LOGS", &iuse::chop_logs );
    add_iuse( "CIRCSAW_ON", &iuse::circsaw_on );
    add_iuse( "CLEAR_RUBBLE", &iuse::clear_rubble );
    add_iuse( "COKE", &iuse::coke );
    add_iuse( "COMBATSAW_OFF", &iuse::combatsaw_off );
    add_iuse( "COMBATSAW_ON", &iuse::combatsaw_on );
    add_iuse( "E_COMBATSAW_OFF", &iuse::e_combatsaw_off );
    add_iuse( "E_COMBATSAW_ON", &iuse::e_combatsaw_on );
    add_iuse( "CONTACTS", &iuse::contacts );
    add_iuse( "CROWBAR", &iuse::crowbar );
    add_iuse( "CS_LAJATANG_OFF", &iuse::cs_lajatang_off );
    add_iuse( "CS_LAJATANG_ON", &iuse::cs_lajatang_on );
    add_iuse( "ECS_LAJATANG_OFF", &iuse::ecs_lajatang_off );
    add_iuse( "ECS_LAJATANG_ON", &iuse::ecs_lajatang_on );
    add_iuse( "DATURA", &iuse::datura );
    add_iuse( "DIG", &iuse::dig );
    add_iuse( "DIVE_TANK", &iuse::dive_tank );
    add_iuse( "DIRECTIONAL_ANTENNA", &iuse::directional_antenna );
    add_iuse( "DISASSEMBLE", &iuse::disassemble );
    add_iuse( "CRAFT", &iuse::craft );
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
    add_iuse( "FILL_PIT", &iuse::fill_pit );
    add_iuse( "FIRECRACKER", &iuse::firecracker );
    add_iuse( "FIRECRACKER_ACT", &iuse::firecracker_act );
    add_iuse( "FIRECRACKER_PACK", &iuse::firecracker_pack );
    add_iuse( "FIRECRACKER_PACK_ACT", &iuse::firecracker_pack_act );
    add_iuse( "FISH_ROD", &iuse::fishing_rod );
    add_iuse( "FISH_TRAP", &iuse::fish_trap );
    add_iuse( "FLUMED", &iuse::flumed );
    add_iuse( "FLUSLEEP", &iuse::flusleep );
    add_iuse( "FLU_VACCINE", &iuse::flu_vaccine );
    add_iuse( "FOODPERSON", &iuse::foodperson );
    add_iuse( "FUNGICIDE", &iuse::fungicide );
    add_iuse( "GASMASK", &iuse::gasmask,
              translate_marker( "Can be activated to <good>increase environmental "
                                "protection</good>.  Will consume charges when active, "
                                "but <info>only when environmental hazards are "
                                "present</info>."
                              ) );
    add_iuse( "GEIGER", &iuse::geiger );
    add_iuse( "GOBAG_NORMAL", &iuse::gobag_normal );
    add_iuse( "GOBAG_PERSONAL", &iuse::gobag_personal );
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
    add_iuse( "MAGIC_8_BALL", &iuse::magic_8_ball );
    add_iuse( "MAKEMOUND", &iuse::makemound );
    add_iuse( "DIG_CHANNEL", &iuse::dig_channel );
    add_iuse( "MARLOSS", &iuse::marloss );
    add_iuse( "MARLOSS_GEL", &iuse::marloss_gel );
    add_iuse( "MARLOSS_SEED", &iuse::marloss_seed );
    add_iuse( "MA_MANUAL", &iuse::ma_manual );
    add_iuse( "MEDITATE", &iuse::meditate );
    add_iuse( "METH", &iuse::meth );
    add_iuse( "MININUKE", &iuse::mininuke );
    add_iuse( "MISC_REPAIR", &iuse::misc_repair );
    add_iuse( "MOLOTOV_LIT", &iuse::molotov_lit );
    add_iuse( "MOP", &iuse::mop );
    add_iuse( "MP3", &iuse::mp3 );
    add_iuse( "MP3_ON", &iuse::mp3_on );
    add_iuse( "MULTICOOKER", &iuse::multicooker );
    add_iuse( "MYCUS", &iuse::mycus );
    add_iuse( "NOISE_EMITTER_OFF", &iuse::noise_emitter_off );
    add_iuse( "NOISE_EMITTER_ON", &iuse::noise_emitter_on );
    add_iuse( "OXYGEN_BOTTLE", &iuse::oxygen_bottle );
    add_iuse( "OXYTORCH", &iuse::oxytorch );
    add_iuse( "PACK_CBM", &iuse::pack_cbm );
    add_iuse( "PACK_ITEM", &iuse::pack_item );
    add_iuse( "PANACEA", &iuse::panacea );
    add_iuse( "PHEROMONE", &iuse::pheromone );
    add_iuse( "PICKAXE", &iuse::pickaxe );
    add_iuse( "PLANTBLECH", &iuse::plantblech );
    add_iuse( "POISON", &iuse::poison );
    add_iuse( "PORTABLE_GAME", &iuse::portable_game );
    add_iuse( "PORTAL", &iuse::portal );
    add_iuse( "PROZAC", &iuse::prozac );
    add_iuse( "PURIFIER", &iuse::purifier );
    add_iuse( "PURIFY_IV", &iuse::purify_iv );
    add_iuse( "PURIFY_SMART", &iuse::purify_smart );
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
    add_iuse( "SEED", &iuse::seed );
    add_iuse( "SEWAGE", &iuse::sewage );
    add_iuse( "SHAVEKIT", &iuse::shavekit );
    add_iuse( "SHOCKTONFA_OFF", &iuse::shocktonfa_off );
    add_iuse( "SHOCKTONFA_ON", &iuse::shocktonfa_on );
    add_iuse( "SIPHON", &iuse::siphon );
    add_iuse( "SLEEP", &iuse::sleep );
    add_iuse( "SMOKING", &iuse::smoking );
    add_iuse( "SOLARPACK", &iuse::solarpack );
    add_iuse( "SOLARPACK_OFF", &iuse::solarpack_off );
    add_iuse( "SPRAY_CAN", &iuse::spray_can );
    add_iuse( "STIMPACK", &iuse::stimpack );
    add_iuse( "STRONG_ANTIBIOTIC", &iuse::strong_antibiotic );
    add_iuse( "TAZER", &iuse::tazer );
    add_iuse( "TAZER2", &iuse::tazer2 );
    add_iuse( "TELEPORT", &iuse::teleport );
    add_iuse( "THORAZINE", &iuse::thorazine );
    add_iuse( "THROWABLE_EXTINGUISHER_ACT", &iuse::throwable_extinguisher_act );
    add_iuse( "TOWEL", &iuse::towel );
    add_iuse( "TRIMMER_OFF", &iuse::trimmer_off );
    add_iuse( "TRIMMER_ON", &iuse::trimmer_on );
    add_iuse( "UNFOLD_GENERIC", &iuse::unfold_generic );
    add_iuse( "UNPACK_ITEM", &iuse::unpack_item );
    add_iuse( "VACCINE", &iuse::vaccine );
    add_iuse( "CALL_OF_TINDALOS", &iuse::call_of_tindalos );
    add_iuse( "BLOOD_DRAW", &iuse::blood_draw );
    add_iuse( "MIND_SPLICER", &iuse::mind_splicer );
    add_iuse( "VIBE", &iuse::vibe );
    add_iuse( "HAND_CRANK", &iuse::hand_crank );
    add_iuse( "VORTEX", &iuse::vortex );
    add_iuse( "WASH_SOFT_ITEMS", &iuse::wash_soft_items );
    add_iuse( "WASH_HARD_ITEMS", &iuse::wash_hard_items );
    add_iuse( "WASH_ALL_ITEMS", &iuse::wash_all_items );
    add_iuse( "WATER_PURIFIER", &iuse::water_purifier );
    add_iuse( "WEAK_ANTIBIOTIC", &iuse::weak_antibiotic );
    add_iuse( "WEATHER_TOOL", &iuse::weather_tool );
    add_iuse( "WEED_CAKE", &iuse::weed_cake );
    add_iuse( "XANAX", &iuse::xanax );
    add_iuse( "BREAK_STICK", &iuse::break_stick );
    add_iuse( "MAGNESIUM_TABLET", &iuse::magnesium_tablet );

    add_actor( std::make_unique<ammobelt_actor>() );
    add_actor( std::make_unique<bandolier_actor>() );
    add_actor( std::make_unique<cauterize_actor>() );
    add_actor( std::make_unique<consume_drug_iuse>() );
    add_actor( std::make_unique<delayed_transform_iuse>() );
    add_actor( std::make_unique<enzlave_actor>() );
    add_actor( std::make_unique<explosion_iuse>() );
    add_actor( std::make_unique<firestarter_actor>() );
    add_actor( std::make_unique<fireweapon_off_actor>() );
    add_actor( std::make_unique<fireweapon_on_actor>() );
    add_actor( std::make_unique<heal_actor>() );
    add_actor( std::make_unique<holster_actor>() );
    add_actor( std::make_unique<inscribe_actor>() );
    add_actor( std::make_unique<iuse_transform>() );
    add_actor( std::make_unique<countdown_actor>() );
    add_actor( std::make_unique<manualnoise_actor>() );
    add_actor( std::make_unique<musical_instrument_actor>() );
    add_actor( std::make_unique<pick_lock_actor>() );
    add_actor( std::make_unique<deploy_furn_actor>() );
    add_actor( std::make_unique<place_monster_iuse>() );
    add_actor( std::make_unique<reveal_map_actor>() );
    add_actor( std::make_unique<salvage_actor>() );
    add_actor( std::make_unique<unfold_vehicle_iuse>() );
    add_actor( std::make_unique<ups_based_armor_actor>() );
    add_actor( std::make_unique<place_trap_actor>() );
    add_actor( std::make_unique<emit_actor>() );
    add_actor( std::make_unique<saw_barrel_actor>() );
    add_actor( std::make_unique<install_bionic_actor>() );
    add_actor( std::make_unique<detach_gunmods_actor>() );
    add_actor( std::make_unique<mutagen_actor>() );
    add_actor( std::make_unique<mutagen_iv_actor>() );
    add_actor( std::make_unique<deploy_tent_actor>() );
    add_actor( std::make_unique<learn_spell_actor>() );
    add_actor( std::make_unique<cast_spell_actor>() );
    add_actor( std::make_unique<weigh_self_actor>() );
    add_actor( std::make_unique<sew_advanced_actor>() );
    // An empty dummy group, it will not spawn anything. However, it makes that item group
    // id valid, so it can be used all over the place without need to explicitly check for it.
    m_template_groups["EMPTY_GROUP"] = std::make_unique<Item_group>( Item_group::G_COLLECTION, 100, 0,
                                       0 );
}

bool Item_factory::check_ammo_type( std::ostream &msg, const ammotype &ammo ) const
{
    if( ammo.is_null() ) {
        return false;
    }

    if( !ammo.is_valid() ) {
        msg << string_format( "ammo type %s is not known", ammo.c_str() ) << "\n";
        return false;
    }

    if( std::none_of( m_templates.begin(),
    m_templates.end(), [&ammo]( const decltype( m_templates )::value_type & e ) {
    return e.second.ammo && e.second.ammo->type == ammo;
} ) ) {
        msg << string_format( "there is no actual ammo of type %s defined", ammo.c_str() ) << "\n";
        return false;
    }
    return true;
}

void Item_factory::check_definitions() const
{
    for( const auto &elem : m_templates ) {
        std::ostringstream msg;
        const itype *type = &elem.second;

        if( !type->category ) {
            msg << "undefined category " << type->category_force << "\n";
        }

        if( type->weight < 0_gram ) {
            msg << "negative weight" << "\n";
        }
        if( type->volume < 0_ml ) {
            msg << "negative volume" << "\n";
        }
        if( type->count_by_charges() || type->phase == LIQUID ) {
            if( type->stack_size <= 0 ) {
                msg << "invalid stack_size " << type->stack_size << " on type using charges\n";
            }
        }
        if( type->price < 0 ) {
            msg << "negative price" << "\n";
        }
        if( type->damage_min() > 0 || type->damage_max() < 0 || type->damage_min() > type->damage_max() ) {
            msg << "invalid damage range" << "\n";
        }
        if( type->description.empty() ) {
            msg << "empty description" << "\n";
        }

        for( const material_id &mat_id : type->materials ) {
            if( mat_id.str() == "null" || !mat_id.is_valid() ) {
                msg << string_format( "invalid material %s", mat_id.c_str() ) << "\n";
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
                msg << string_format( "item %s: snippet category %s without any snippets", type->id.c_str(),
                                      type->snippet_category.c_str() ) << "\n";
            }
        }
        for( auto &q : type->qualities ) {
            if( !q.first.is_valid() ) {
                msg << string_format( "item %s has unknown quality %s", type->id.c_str(), q.first.c_str() ) << "\n";
            }
        }
        if( type->default_container && ( !has_template( *type->default_container ) ||
                                         *type->default_container == "null" ) ) {
            msg << string_format( "invalid container property %s", type->default_container->c_str() ) << "\n";
        }

        for( const auto &e : type->emits ) {
            if( !e.is_valid() ) {
                msg << string_format( "item %s has unknown emit source %s", type->id.c_str(), e.c_str() ) << "\n";
            }
        }

        for( const auto &f : type->faults ) {
            if( !f.is_valid() ) {
                msg << string_format( "invalid item fault %s", f.c_str() ) << "\n";
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
        if( type->brewable ) {
            if( type->brewable->time < 1_turns ) {
                msg << "brewable time is less than 1 turn\n";
            }

            if( type->brewable->results.empty() ) {
                msg << "empty product list" << "\n";
            }

            for( auto &b : type->brewable->results ) {
                if( !has_template( b ) ) {
                    msg << string_format( "invalid result id %s", b.c_str() ) << "\n";
                }
            }
        }
        if( type->seed ) {
            if( type->seed->grow < 1_turns ) {
                msg << "seed growing time is less than 1 turn\n";
            }
            if( !has_template( type->seed->fruit_id ) ) {
                msg << string_format( "invalid fruit id %s", type->seed->fruit_id.c_str() ) << "\n";
            }
            for( auto &b : type->seed->byproducts ) {
                if( !has_template( b ) ) {
                    msg << string_format( "invalid byproduct id %s", b.c_str() ) << "\n";
                }
            }
        }
        if( type->book ) {
            if( type->book->skill && !type->book->skill.is_valid() ) {
                msg << "uses invalid book skill." << "\n";
            }
            if( type->book->martial_art && !type->book->martial_art.is_valid() ) {
                msg << string_format( "trains invalid martial art '%s'.",
                                      type->book->martial_art.str() ) << "\n";
            }
            if( type->can_use( "MA_MANUAL" ) && !type->book->martial_art ) {
                msg << "has use_action MA_MANUAL but does not specify a martial art\n";
            }
        }
        if( type->can_use( "MA_MANUAL" ) && !type->book ) {
            msg << "has use_action MA_MANUAL but is not a book\n";
        }
        if( type->ammo ) {
            if( !type->ammo->type && type->ammo->type != ammotype( "NULL" ) ) {
                msg << "must define at least one ammo type" << "\n";
            }
            check_ammo_type( msg, type->ammo->type );
            if( type->ammo->casing && ( !has_template( *type->ammo->casing ) ||
                                        *type->ammo->casing == "null" ) ) {
                msg << string_format( "invalid casing property %s", type->ammo->casing->c_str() ) << "\n";
            }
            if( type->ammo->drop != "null" && !has_template( type->ammo->drop ) ) {
                msg << string_format( "invalid drop item %s", type->ammo->drop.c_str() ) << "\n";
            }
        }
        if( type->battery ) {
            if( type->battery->max_capacity < 0_mJ ) {
                msg << "battery cannot have negative maximum charge\n";
            }
        }
        if( type->gun ) {
            for( const ammotype &at : type->gun->ammo ) {
                check_ammo_type( msg, at );
            }
            if( type->gun->ammo.empty() ) {
                // if gun doesn't use ammo forbid both integral or detachable magazines
                if( static_cast<bool>( type->gun->clip ) || !type->magazines.empty() ) {
                    msg << "cannot specify clip_size or magazine without ammo type" << "\n";
                }

                if( type->item_tags.count( "RELOAD_AND_SHOOT" ) ) {
                    msg << "RELOAD_AND_SHOOT requires an ammo type to be specified" << "\n";
                }

            } else {
                if( type->item_tags.count( "RELOAD_AND_SHOOT" ) && !type->magazines.empty() ) {
                    msg << "RELOAD_AND_SHOOT cannot be used with magazines" << "\n";
                }
                for( const ammotype &at : type->gun->ammo ) {
                    if( !type->gun->clip && !type->magazines.empty() && !type->magazine_default.count( at ) ) {
                        msg << "specified magazine but none provided for ammo type " << at.str() << "\n";
                    }
                }
            }
            if( type->gun->barrel_length < 0_ml ) {
                msg << "gun barrel length cannot be negative" << "\n";
            }

            if( !type->gun->skill_used ) {
                msg << "uses no skill" << "\n";
            } else if( !type->gun->skill_used.is_valid() ) {
                msg << "uses an invalid skill " << type->gun->skill_used.str() << "\n";
            }
            for( auto &gm : type->gun->default_mods ) {
                if( !has_template( gm ) ) {
                    msg << "invalid default mod." << "\n";
                }
            }
            for( auto &gm : type->gun->built_in_mods ) {
                if( !has_template( gm ) ) {
                    msg << "invalid built-in mod." << "\n";
                }
            }
        }
        if( type->gunmod ) {
            if( type->gunmod->location.str().empty() ) {
                msg << "gunmod does not specify location" << "\n";
            }
            if( ( type->gunmod->sight_dispersion < 0 ) != ( type->gunmod->aim_speed < 0 ) ) {
                msg << "gunmod must have both sight_dispersion and aim_speed set or neither of them set" << "\n";
            }
        }
        if( type->mod ) {
            for( const ammotype &at : type->mod->ammo_modifier ) {
                check_ammo_type( msg, at );
            }

            for( const auto &e : type->mod->acceptable_ammo ) {
                check_ammo_type( msg, e );
            }

            for( const auto &e : type->mod->magazine_adaptor ) {
                check_ammo_type( msg, e.first );
                if( e.second.empty() ) {
                    msg << "No magazines specified for ammo type " << e.first.str() << "\n";
                }
                for( const itype_id &opt : e.second ) {
                    const itype *mag = find_template( opt );
                    if( !mag->magazine || !mag->magazine->type.count( e.first ) ) {
                        msg << "invalid magazine " << opt << " in magazine adapter\n";
                    }
                }
            }
        }
        if( type->magazine ) {
            for( const ammotype &at : type->magazine->type ) {
                check_ammo_type( msg, at );
            }
            if( type->magazine->type.empty() ) {
                msg << "magazine did not specify ammo type" << "\n";
            }
            if( type->magazine->capacity < 0 ) {
                msg << string_format( "invalid capacity %i", type->magazine->capacity ) << "\n";
            }
            if( type->magazine->count < 0 || type->magazine->count > type->magazine->capacity ) {
                msg << string_format( "invalid count %i", type->magazine->count ) << "\n";
            }
            const itype *da = find_template( type->magazine->default_ammo );
            if( !( da->ammo && type->magazine->type.count( da->ammo->type ) ) ) {
                msg << string_format( "invalid default_ammo %s", type->magazine->default_ammo.c_str() ) << "\n";
            }
            if( type->magazine->reliability < 0 || type->magazine->reliability > 100 ) {
                msg << string_format( "invalid reliability %i", type->magazine->reliability ) << "\n";
            }
            if( type->magazine->reload_time < 0 ) {
                msg << string_format( "invalid reload_time %i", type->magazine->reload_time ) << "\n";
            }
            if( type->magazine->linkage && ( !has_template( *type->magazine->linkage ) ||
                                             *type->magazine->linkage == "null" ) ) {
                msg << string_format( "invalid linkage property %s", type->magazine->linkage->c_str() ) << "\n";
            }
        }

        for( const std::pair<const string_id<ammunition_type>, std::set<std::string>> &ammo_variety :
             type->magazines ) {
            if( ammo_variety.second.empty() ) {
                msg << "No magazine specified for " << ammo_variety.first.str() << "\n";
            }
            for( const std::string &magazine : ammo_variety.second ) {
                const itype *mag_ptr = find_template( magazine );
                if( mag_ptr == nullptr ) {
                    msg << "Magazine \"" << magazine << "\" specified for \""
                        << ammo_variety.first.str() << "\" does not exist\n";
                } else if( !mag_ptr->magazine ) {
                    msg << "Magazine \"" << magazine << "\" specified for \""
                        << ammo_variety.first.str() << "\" is not a magazine\n";
                } else if( !mag_ptr->magazine->type.count( ammo_variety.first ) ) {
                    msg << "magazine \"" << magazine << "\" does not take compatible ammo \n";
                } else if( mag_ptr->item_tags.count( "SPEEDLOADER" ) &&
                           mag_ptr->magazine->capacity != type->gun->clip ) {
                    msg << "Speedloader " << magazine << " capacity ("
                        << mag_ptr->magazine->capacity << ") does not match gun capacity ("
                        << type->gun->clip << ").\n";
                }
            }
        }

        if( type->tool ) {
            for( const ammotype &at : type->tool->ammo_id ) {
                check_ammo_type( msg, at );
            }
            if( type->tool->revert_to && ( !has_template( *type->tool->revert_to ) ||
                                           *type->tool->revert_to == "null" ) ) {
                msg << string_format( "invalid revert_to property %s", type->tool->revert_to->c_str() ) << "\n";
            }
            if( !type->tool->revert_msg.empty() && !type->tool->revert_to ) {
                msg << _( "cannot specify revert_msg without revert_to" ) << "\n";
            }
            if( !type->tool->subtype.empty() && !has_template( type->tool->subtype ) ) {
                msg << _( "Invalid tool subtype" ) << type->tool->subtype << "\n";
            }
        }
        if( type->bionic ) {
            if( !type->bionic->id.is_valid() ) {
                msg << string_format( "there is no bionic with id %s", type->bionic->id.c_str() ) << "\n";
            }
        }

        if( type->container ) {
            if( type->container->seals && type->container->unseals_into != "null" ) {
                msg << string_format( "Resealable container unseals_into %s",
                                      type->container->unseals_into.c_str() ) << "\n";
            }
            if( type->container->contains <= 0_ml ) {
                msg << string_format( "\"contains\" (%d) must be >0", type->container->contains.value() ) << "\n";
            }
            if( !has_template( type->container->unseals_into ) ) {
                msg << string_format( "unseals_into invalid id %s", type->container->unseals_into.c_str() ) << "\n";
            }
        }

        for( const auto &elem : type->use_methods ) {
            const iuse_actor *actor = elem.second.get_actor_ptr();

            assert( actor );
            if( !actor->is_valid() ) {
                msg << string_format( "item action \"%s\" was not described.", actor->type.c_str() ) << "\n";
            }
        }

        if( type->fuel && !type->count_by_charges() ) {
            msg << "fuel value set, but item isn't count_by_charges.\n";
        }

        if( msg.str().empty() ) {
            continue;
        }
        debugmsg( "warnings for type %s:\n%s", type->id.c_str(), msg.str() );
    }
    for( const auto &e : migrations ) {
        if( !m_templates.count( e.second.replace ) ) {
            debugmsg( "Invalid migration target: %s", e.second.replace.c_str() );
        }
        for( const auto &c : e.second.contents ) {
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
const itype *Item_factory::find_template( const itype_id &id ) const
{
    assert( frozen );

    auto found = m_templates.find( id );
    if( found != m_templates.end() ) {
        return &found->second;
    }

    auto rt = m_runtimes.find( id );
    if( rt != m_runtimes.end() ) {
        return rt->second.get();
    }

    //If we didn't find the item maybe it is a building instead!
    const recipe_id &making_id = recipe_id( id.c_str() );
    if( oter_str_id( id.c_str() ).is_valid() ||
        ( making_id.is_valid() && making_id.obj().is_blueprint() ) ) {
        itype *def = new itype();
        def->id = id;
        def->name = def->name_plural = string_format( "DEBUG: %s", id.c_str() );
        def->description = making_id.obj().description;
        m_runtimes[ id ].reset( def );
        return def;
    }

    debugmsg( "Missing item definition: %s", id.c_str() );

    itype *def = new itype();
    def->id = id;
    def->name = string_format( "undefined-%ss", id.c_str() );
    def->name_plural = string_format( "undefined-%s", id.c_str() );
    def->description = no_translation( string_format( "Missing item definition for %s.", id.c_str() ) );

    m_runtimes[ id ].reset( def );
    return def;
}

Item_spawn_data *Item_factory::get_group( const Item_tag &group_tag )
{
    GroupMap::iterator group_iter = m_template_groups.find( group_tag );
    if( group_iter != m_template_groups.end() ) {
        return group_iter->second.get();
    }
    return nullptr;
}

///////////////////////
// DATA FILE READING //
///////////////////////

template<typename SlotType>
void Item_factory::load_slot( cata::optional<SlotType> &slotptr, JsonObject &jo,
                              const std::string &src )
{
    if( !slotptr ) {
        slotptr.emplace();
    }
    load( *slotptr, jo, src );
}

template<typename SlotType>
void Item_factory::load_slot_optional( cata::optional<SlotType> &slotptr, JsonObject &jo,
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

bool Item_factory::load_definition( JsonObject &jo, const std::string &src, itype &def )
{
    assert( !frozen );

    if( !jo.has_string( "copy-from" ) ) {
        // if this is a new definition ensure we start with a clean itype
        def = itype();
        return true;
    }

    auto base = m_templates.find( jo.get_string( "copy-from" ) );
    if( base != m_templates.end() ) {
        def = base->second;
        def.looks_like = jo.get_string( "copy-from" );
        return true;
    }

    auto abstract = m_abstracts.find( jo.get_string( "copy-from" ) );
    if( abstract != m_abstracts.end() ) {
        def = abstract->second;
        if( def.looks_like.empty() ) {
            def.looks_like = jo.get_string( "copy-from" );
        }
        return true;
    }

    deferred.emplace_back( jo.str(), src );
    return false;
}

void Item_factory::load( islot_artifact &slot, JsonObject &jo, const std::string & )
{
    slot.charge_type = jo.get_enum_value( "charge_type", ARTC_NULL );
    slot.charge_req  = jo.get_enum_value( "charge_req",  ACR_NULL );
    // No dreams unless specified for artifacts embedded in items.
    // If specifying dreams, message should be set too,
    // since the array with the defaults isn't accessible from here.
    slot.dream_freq_unmet = jo.get_int( "dream_freq_unmet", 0 );
    slot.dream_freq_met   = jo.get_int( "dream_freq_met",   0 );
    slot.dream_msg_unmet  =
        jo.get_string_array( "dream_unmet" ); // TODO: Make sure it doesn't cause problems if this is empty
    slot.dream_msg_met    = jo.get_string_array( "dream_met" );
    load_optional_enum_array( slot.effects_wielded, jo, "effects_wielded" );
    load_optional_enum_array( slot.effects_activated, jo, "effects_activated" );
    load_optional_enum_array( slot.effects_carried, jo, "effects_carried" );
    load_optional_enum_array( slot.effects_worn, jo, "effects_worn" );
}

void Item_factory::load( islot_ammo &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    assign( jo, "ammo_type", slot.type, strict );
    assign( jo, "casing", slot.casing, strict );
    assign( jo, "drop", slot.drop, strict );
    assign( jo, "drop_chance", slot.drop_chance, strict, 0.0f, 1.0f );
    assign( jo, "drop_active", slot.drop_active, strict );
    if( jo.has_object( "damage" ) ) {
        assign( jo, "damage", slot.damage, strict );
    } else {
        assign( jo, "damage", slot.legacy_damage, strict, 0 );
    }

    assign( jo, "pierce", slot.legacy_pierce, strict, 0 );
    assign( jo, "range", slot.range, strict, 0 );
    assign( jo, "dispersion", slot.dispersion, strict, 0 );
    assign( jo, "recoil", slot.recoil, strict, 0 );
    assign( jo, "count", slot.def_charges, strict, 1 );
    assign( jo, "loudness", slot.loudness, strict, 0 );
    assign( jo, "effects", slot.ammo_effects, strict );
    assign( jo, "prop_damage", slot.prop_damage, strict );
    assign( jo, "show_stats", slot.force_stat_display, strict );
}

void Item_factory::load_ammo( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        assign( jo, "stack_size", def.stack_size, src == "dda", 1 );
        load_slot( def.ammo, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_engine &slot, JsonObject &jo, const std::string & )
{
    assign( jo, "displacement", slot.displacement );
}

void Item_factory::load_engine( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.engine, jo, src );
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
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.wheel, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_fuel &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    assign( jo, "energy", slot.energy, strict, 0.001f );
    if( jo.has_member( "pump_terrain" ) ) {
        slot.pump_terrain = jo.get_string( "pump_terrain" );
    }
    if( jo.has_member( "explosion_data" ) ) {
        slot.has_explode_data = true;
        JsonObject jo_ed = jo.get_object( "explosion_data" );
        slot.explosion_data.explosion_chance_hot = jo_ed.get_int( "chance_hot" );
        slot.explosion_data.explosion_chance_cold = jo_ed.get_int( "chance_cold" );
        slot.explosion_data.explosion_factor = jo_ed.get_float( "factor" );
        slot.explosion_data.fiery_explosion = jo_ed.get_bool( "fiery" );
        slot.explosion_data.fuel_size_factor = jo_ed.get_float( "size_factor" );
    }
}

void Item_factory::load_fuel( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.fuel, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_gun &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    if( jo.has_member( "burst" ) && jo.has_member( "modes" ) ) {
        jo.throw_error( "cannot specify both burst and modes", "burst" );
    }

    assign( jo, "skill", slot.skill_used, strict );
    if( jo.has_array( "ammo" ) ) {
        slot.ammo.clear();
        JsonArray atypes = jo.get_array( "ammo" );
        for( size_t i = 0; i < atypes.size(); ++i ) {
            slot.ammo.insert( ammotype( atypes.get_string( i ) ) );
        }
    } else if( jo.has_string( "ammo" ) ) {
        slot.ammo.clear();
        slot.ammo.insert( ammotype( jo.get_string( "ammo" ) ) );
    }
    assign( jo, "range", slot.range, strict );
    if( jo.has_object( "ranged_damage" ) || jo.has_array( "ranged_damage" ) ) {
        assign( jo, "ranged_damage", slot.damage, strict );
    } else {
        assign( jo, "ranged_damage", slot.legacy_damage, strict );
    }

    assign( jo, "pierce", slot.legacy_pierce, strict, 0 );
    assign( jo, "dispersion", slot.dispersion, strict );
    assign( jo, "sight_dispersion", slot.sight_dispersion, strict, 0, static_cast<int>( MAX_RECOIL ) );
    assign( jo, "recoil", slot.recoil, strict, 0 );
    assign( jo, "handling", slot.handling, strict );
    assign( jo, "durability", slot.durability, strict, 0, 10 );
    assign( jo, "burst", slot.burst, strict, 1 );
    assign( jo, "loudness", slot.loudness, strict );
    assign( jo, "clip_size", slot.clip, strict, 0 );
    assign( jo, "reload", slot.reload_time, strict, 0 );
    assign( jo, "reload_noise", slot.reload_noise, strict );
    assign( jo, "reload_noise_volume", slot.reload_noise_volume, strict, 0 );
    assign( jo, "barrel_length", slot.barrel_length, strict, 0_ml );
    assign( jo, "built_in_mods", slot.built_in_mods, strict );
    assign( jo, "default_mods", slot.default_mods, strict );
    assign( jo, "ups_charges", slot.ups_charges, strict, 0 );
    assign( jo, "blackpowder_tolerance", slot.blackpowder_tolerance, strict, 0 );
    assign( jo, "min_cycle_recoil", slot.min_cycle_recoil, strict, 0 );
    assign( jo, "ammo_effects", slot.ammo_effects, strict );

    if( jo.has_array( "valid_mod_locations" ) ) {
        slot.valid_mod_locations.clear();
        JsonArray jarr = jo.get_array( "valid_mod_locations" );
        while( jarr.has_more() ) {
            JsonArray curr = jarr.next_array();
            slot.valid_mod_locations.emplace( curr.get_string( 0 ), curr.get_int( 1 ) );
        }
    }

    assign( jo, "modes", slot.modes );
}

void Item_factory::load_gun( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.gun, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load_armor( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.armor, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load_pet_armor( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.pet_armor, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_armor &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    assign( jo, "encumbrance", slot.encumber, strict, 0 );
    assign( jo, "max_encumbrance", slot.max_encumber, strict, slot.encumber );
    assign( jo, "coverage", slot.coverage, strict, 0, 100 );
    assign( jo, "material_thickness", slot.thickness, strict, 0 );
    assign( jo, "environmental_protection", slot.env_resist, strict, 0 );
    assign( jo, "environmental_protection_with_filter", slot.env_resist_w_filter, strict, 0 );
    assign( jo, "warmth", slot.warmth, strict, 0 );
    assign( jo, "storage", slot.storage, strict, 0_ml );
    assign( jo, "weight_capacity_modifier", slot.weight_capacity_modifier );
    assign( jo, "weight_capacity_bonus", slot.weight_capacity_bonus, strict, 0_gram );
    assign( jo, "power_armor", slot.power_armor, strict );

    assign_coverage_from_json( jo, "covers", slot.covers, slot.sided );
}

void Item_factory::load( islot_pet_armor &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    assign( jo, "material_thickness", slot.thickness, strict, 0 );
    assign( jo, "max_pet_vol", slot.max_vol, strict, 0_ml );
    assign( jo, "min_pet_vol", slot.min_vol, strict, 0_ml );
    assign( jo, "pet_bodytype", slot.bodytype, strict );
    assign( jo, "environmental_protection", slot.env_resist, strict, 0 );
    assign( jo, "environmental_protection_with_filter", slot.env_resist_w_filter, strict, 0 );
    assign( jo, "storage", slot.storage, strict, 0_ml );
    assign( jo, "power_armor", slot.power_armor, strict );
}

void Item_factory::load( islot_tool &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    if( jo.has_array( "ammo" ) ) {
        JsonArray atypes = jo.get_array( "ammo" );
        for( size_t i = 0; i < atypes.size(); ++i ) {
            slot.ammo_id.insert( ammotype( atypes.get_string( i ) ) );
        }
    } else if( jo.has_string( "ammo" ) ) {
        slot.ammo_id.insert( ammotype( jo.get_string( "ammo" ) ) );
    }
    assign( jo, "max_charges", slot.max_charges, strict, 0 );
    assign( jo, "initial_charges", slot.def_charges, strict, 0 );
    assign( jo, "charges_per_use", slot.charges_per_use, strict, 0 );
    assign( jo, "charge_factor", slot.charge_factor, strict, 1 );
    assign( jo, "turns_per_charge", slot.turns_per_charge, strict, 0 );
    assign( jo, "power_draw", slot.power_draw, strict, 0 );
    assign( jo, "revert_to", slot.revert_to, strict );
    assign( jo, "revert_msg", slot.revert_msg, strict );
    assign( jo, "sub", slot.subtype, strict );

    if( jo.has_array( "rand_charges" ) ) {
        JsonArray jarr = jo.get_array( "rand_charges" );
        if( jo.has_member( "initial_charges" ) ) {
            jarr.throw_error( "You can have a fixed initial amount of charges, or randomized. Not both." );
        }
        while( jarr.has_more() ) {
            slot.rand_charges.push_back( jarr.next_int() );
        }
        if( slot.rand_charges.size() == 1 ) {
            // see item::item(...) for the use of this array
            jarr.throw_error( "a rand_charges array with only one entry will be ignored, it needs at least 2 entries!" );
        }
    }
}

void Item_factory::load_tool( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.tool, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( relic &slot, JsonObject &jo, const std::string & )
{
    slot.load( jo );
}

void Item_factory::load( islot_mod &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    if( jo.has_array( "ammo_modifier" ) ) {
        JsonArray atypes = jo.get_array( "ammo_modifier" );
        for( size_t i = 0; i < atypes.size(); ++i ) {
            slot.ammo_modifier.insert( ammotype( atypes.get_string( i ) ) );
        }
    } else if( jo.has_string( "ammo_modifier" ) ) {
        slot.ammo_modifier.insert( ammotype( jo.get_string( "ammo_modifier" ) ) );
    }
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

        ammotype ammo( arr.get_string( 0 ) ); // an ammo type (e.g. 9mm)
        JsonArray compat = arr.get_array( 1 ); // compatible magazines for this ammo type

        while( compat.has_more() ) {
            slot.magazine_adaptor[ ammo ].insert( compat.next_string() );
        }
    }
}

void Item_factory::load_toolmod( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.mod, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load_tool_armor( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.tool, jo, src );
        load_slot( def.armor, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_book &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    assign( jo, "max_level", slot.level, strict, 0, MAX_SKILL );
    assign( jo, "required_level", slot.req, strict, 0, MAX_SKILL );
    assign( jo, "fun", slot.fun, strict );
    assign( jo, "intelligence", slot.intel, strict, 0 );
    if( jo.has_int( "time" ) ) {
        slot.time = jo.get_int( "time" );
    } else if( jo.has_string( "time" ) ) {
        slot.time = to_minutes<int>( read_from_json_string<time_duration>( *jo.get_raw( "time" ),
                                     time_duration::units ) );
    }
    assign( jo, "skill", slot.skill, strict );
    assign( jo, "martial_art", slot.martial_art, strict );
    assign( jo, "chapters", slot.chapters, strict, 0 );
}

void Item_factory::load_book( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.book, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_comestible &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    assign( jo, "comestible_type", slot.comesttype, strict );
    assign( jo, "tool", slot.tool, strict );
    assign( jo, "charges", slot.def_charges, strict, 1 );
    assign( jo, "quench", slot.quench, strict );
    assign( jo, "fun", slot.fun, strict );
    assign( jo, "stim", slot.stim, strict );
    assign( jo, "healthy", slot.healthy, strict );
    assign( jo, "parasites", slot.parasites, strict, 0 );
    assign( jo, "freezing_point", slot.freeze_point, strict );
    assign( jo, "spoils_in", slot.spoils, strict, 1_hours );
    assign( jo, "cooks_like", slot.cooks_like, strict );
    assign( jo, "smoking_result", slot.smoking_result, strict );

    if( jo.has_member( "primary_material" ) ) {
        slot.specific_heat_solid = material_id(
                                       jo.get_string( "primary_material" ) )->specific_heat_solid();
        slot.specific_heat_liquid = material_id(
                                        jo.get_string( "primary_material" ) )->specific_heat_liquid();
        slot.latent_heat = material_id( jo.get_string( "primary_material" ) )->latent_heat();
    } else if( jo.has_member( "material" ) ) {
        float specific_heat_solid = 0;
        float specific_heat_liquid = 0;
        float latent_heat = 0;

        for( auto &m : jo.get_tags( "material" ) ) {
            specific_heat_solid += material_id( m )->specific_heat_solid();
            specific_heat_liquid += material_id( m )->specific_heat_liquid();
            latent_heat += material_id( m )->latent_heat();
        }
        // Average based on number of materials.
        slot.specific_heat_liquid = specific_heat_liquid / jo.get_tags( "material" ).size();
        slot.specific_heat_solid = specific_heat_solid / jo.get_tags( "material" ).size();
        slot.latent_heat = latent_heat / jo.get_tags( "material" ).size();
    }

    if( jo.has_string( "addiction_type" ) ) {
        slot.add = addiction_type( jo.get_string( "addiction_type" ) );
    }

    assign( jo, "addiction_potential", slot.addict, strict );

    bool got_calories = false;

    if( jo.has_int( "calories" ) ) {
        slot.kcal = jo.get_int( "calories" );
        got_calories = true;

    } else if( jo.get_object( "relative" ).has_int( "calories" ) ) {
        slot.kcal += jo.get_object( "relative" ).get_int( "calories" );
        got_calories = true;

    } else if( jo.get_object( "proportional" ).has_float( "calories" ) ) {
        slot.kcal *= jo.get_object( "proportional" ).get_float( "calories" );
        got_calories = true;

    } else if( jo.has_int( "nutrition" ) ) {
        slot.kcal = jo.get_int( "nutrition" ) * islot_comestible::kcal_per_nutr;
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

    if( jo.has_string( "rot_spawn" ) ) {
        slot.rot_spawn = mongroup_id( jo.get_string( "rot_spawn" ) );
    }
    assign( jo, "rot_spawn_chance", slot.rot_spawn_chance, strict, 0 );

}

void Item_factory::load( islot_brewable &slot, JsonObject &jo, const std::string & )
{
    assign( jo, "time", slot.time, false, 1_turns );
    slot.results = jo.get_string_array( "results" );
}

void Item_factory::load_comestible( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        assign( jo, "stack_size", def.stack_size, src == "dda", 1 );
        load_slot( def.comestible, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load_container( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.container, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_seed &slot, JsonObject &jo, const std::string & )
{
    assign( jo, "grow", slot.grow, false, 1_days );
    slot.fruit_div = jo.get_int( "fruit_div", 1 );
    jo.read( "plant_name", slot.plant_name );
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
    bool strict = src == "dda";

    if( jo.has_object( "damage_modifier" ) ) {
        assign( jo, "damage_modifier", slot.damage );
    } else {
        assign( jo, "damage_modifier", slot.legacy_damage );
    }
    assign( jo, "loudness_modifier", slot.loudness );
    assign( jo, "location", slot.location );
    assign( jo, "dispersion_modifier", slot.dispersion );
    assign( jo, "sight_dispersion", slot.sight_dispersion );
    assign( jo, "aim_speed", slot.aim_speed, strict, -1 );
    assign( jo, "handling_modifier", slot.handling, strict );
    assign( jo, "range_modifier", slot.range );
    assign( jo, "consume_chance", slot.consume_chance );
    assign( jo, "consume_divisor", slot.consume_divisor );
    assign( jo, "ammo_effects", slot.ammo_effects, strict );
    assign( jo, "ups_charges", slot.ups_charges );
    if( jo.has_int( "time" ) ) {
        slot.install_time = jo.get_int( "time" );
    } else if( jo.has_string( "time" ) ) {
        slot.install_time = to_moves<int>( read_from_json_string<time_duration>( *jo.get_raw( "time" ),
                                           time_duration::units ) );
    }

    if( jo.has_member( "mod_targets" ) ) {
        slot.usable.clear();
        for( const auto &t : jo.get_tags( "mod_targets" ) ) {
            slot.usable.insert( gun_type_type( t ) );
        }
    }

    assign( jo, "mode_modifier", slot.mode_modifier );
    assign( jo, "reload_modifier", slot.reload_modifier );
    assign( jo, "min_str_required_mod", slot.min_str_required_mod );
    if( jo.has_array( "add_mod" ) ) {
        slot.add_mod.clear();
        JsonArray jarr = jo.get_array( "add_mod" );
        while( jarr.has_more() ) {
            JsonArray curr = jarr.next_array();
            slot.add_mod.emplace( curr.get_string( 0 ), curr.get_int( 1 ) );
        }
    }
    assign( jo, "blacklist_mod", slot.blacklist_mod );
}

void Item_factory::load_gunmod( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.gunmod, jo, src );
        load_slot( def.mod, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_magazine &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";
    if( jo.has_array( "ammo_type" ) ) {
        JsonArray atypes = jo.get_array( "ammo_type" );
        for( size_t i = 0; i < atypes.size(); ++i ) {
            slot.type.insert( ammotype( atypes.get_string( i ) ) );
        }
    } else if( jo.has_string( "ammo_type" ) ) {
        slot.type.insert( ammotype( jo.get_string( "ammo_type" ) ) );
    }
    assign( jo, "capacity", slot.capacity, strict, 0 );
    assign( jo, "count", slot.count, strict, 0 );
    assign( jo, "default_ammo", slot.default_ammo, strict );
    assign( jo, "reliability", slot.reliability, strict, 0, 10 );
    assign( jo, "reload_time", slot.reload_time, strict, 0 );
    assign( jo, "linkage", slot.linkage, strict );
}

void Item_factory::load_magazine( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.magazine, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_battery &slot, JsonObject &jo, const std::string & )
{
    slot.max_capacity = read_from_json_string<units::energy>( *jo.get_raw( "max_capacity" ),
                        units::energy_units );
}

void Item_factory::load_battery( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.battery, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_bionic &slot, JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    if( jo.has_member( "bionic_id" ) ) {
        assign( jo, "bionic_id", slot.id, strict );
    } else {
        assign( jo, "id", slot.id, strict );
    }

    assign( jo, "difficulty", slot.difficulty, strict, 0 );
    assign( jo, "is_upgrade", slot.is_upgrade );
}

void Item_factory::load_bionic( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.bionic, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load_generic( JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
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
            std::make_pair( material_id( "bean" ), "ALLERGEN_VEGGY" ),
            std::make_pair( material_id( "tomato" ), "ALLERGEN_VEGGY" ),
            std::make_pair( material_id( "garlic" ), "ALLERGEN_VEGGY" ),
            std::make_pair( material_id( "nut" ), "ALLERGEN_VEGGY" ),
            std::make_pair( material_id( "mushroom" ), "ALLERGEN_VEGGY" ),
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
        }
    };

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

void Item_factory::load_basic_info( JsonObject &jo, itype &def, const std::string &src )
{
    bool strict = src == "dda";

    assign( jo, "category", def.category_force, strict );
    assign( jo, "weight", def.weight, strict, 0_gram );
    assign( jo, "integral_weight", def.integral_weight, strict, 0_gram );
    assign( jo, "volume", def.volume );
    assign( jo, "price", def.price );
    assign( jo, "price_postapoc", def.price_post );
    assign( jo, "stackable", def.stackable_, strict );
    assign( jo, "integral_volume", def.integral_volume );
    assign( jo, "bashing", def.melee[DT_BASH], strict, 0 );
    assign( jo, "cutting", def.melee[DT_CUT], strict, 0 );
    assign( jo, "to_hit", def.m_to_hit, strict );
    assign( jo, "container", def.default_container );
    assign( jo, "rigid", def.rigid );
    assign( jo, "min_strength", def.min_str );
    assign( jo, "min_dexterity", def.min_dex );
    assign( jo, "min_intelligence", def.min_int );
    assign( jo, "min_perception", def.min_per );
    assign( jo, "emits", def.emits );
    assign( jo, "magazine_well", def.magazine_well );
    assign( jo, "explode_in_fire", def.explode_in_fire );
    assign( jo, "insulation", def.insulation_factor );

    if( jo.has_member( "thrown_damage" ) ) {
        JsonArray jarr = jo.get_array( "thrown_damage" );
        def.thrown_damage = load_damage_instance( jarr );
    } else {
        // TODO: Move to finalization
        def.thrown_damage.clear();
        def.thrown_damage.add_damage( DT_BASH, def.melee[DT_BASH] + def.weight / 1.0_kilogram );
    }

    if( jo.has_member( "damage_states" ) ) {
        auto arr = jo.get_array( "damage_states" );
        def.damage_min_ = arr.get_int( 0 ) * itype::damage_scale;
        def.damage_max_ = arr.get_int( 1 ) * itype::damage_scale;
    }

    def.name = jo.get_string( "name" );
    if( jo.has_member( "name_plural" ) ) {
        def.name_plural = jo.get_string( "name_plural" );
    } else {
        def.name_plural = jo.get_string( "name" ) += "s";
    }

    if( jo.has_string( "description" ) ) {
        jo.read( "description", def.description );
    }

    if( jo.has_string( "symbol" ) ) {
        def.sym = jo.get_string( "symbol" );
    }

    if( jo.has_string( "color" ) ) {
        def.color = color_from_string( jo.get_string( "color" ) );
    }

    if( jo.has_member( "material" ) ) {
        def.materials.clear();
        for( auto &m : jo.get_tags( "material" ) ) {
            def.materials.emplace_back( m );
        }
    }

    if( jo.has_string( "phase" ) ) {
        def.phase = jo.get_enum_value<phase_id>( "phase" );
    }

    if( jo.has_array( "magazines" ) ) {
        def.magazine_default.clear();
        def.magazines.clear();

        JsonArray mags = jo.get_array( "magazines" );
        while( mags.has_more() ) {
            JsonArray arr = mags.next_array();

            ammotype ammo( arr.get_string( 0 ) ); // an ammo type (e.g. 9mm)
            JsonArray compat = arr.get_array( 1 ); // compatible magazines for this ammo type

            // the first magazine for this ammo type is the default
            def.magazine_default[ ammo ] = compat.get_string( 0 );

            while( compat.has_more() ) {
                def.magazines[ ammo ].insert( compat.next_string() );
            }
        }
    }

    JsonArray jarr = jo.get_array( "min_skills" );
    if( !jarr.empty() ) {
        def.min_skills.clear();
    }
    while( jarr.has_more() ) {
        JsonArray cur = jarr.next_array();
        const auto sk = skill_id( cur.get_string( 0 ) );
        if( !sk.is_valid() ) {
            jo.throw_error( string_format( "invalid skill: %s", sk.c_str() ), "min_skills" );
        }
        def.min_skills[ sk ] = cur.get_int( 1 );
    }

    if( jo.has_member( "explosion" ) ) {
        JsonObject je = jo.get_object( "explosion" );
        def.explosion = load_explosion_data( je );
    }

    assign( jo, "flags", def.item_tags );
    assign( jo, "faults", def.faults );

    if( jo.has_member( "qualities" ) ) {
        set_qualities_from_json( jo, "qualities", def );
    }

    if( jo.has_member( "properties" ) ) {
        set_properties_from_json( jo, "properties", def );
    }

    for( auto &s : jo.get_tags( "techniques" ) ) {
        def.techniques.insert( matec_id( s ) );
    }

    set_use_methods_from_json( jo, "use_action", def.use_methods );

    assign( jo, "countdown_interval", def.countdown_interval );
    assign( jo, "countdown_destroy", def.countdown_destroy );

    if( jo.has_string( "countdown_action" ) ) {
        def.countdown_action = usage_from_string( jo.get_string( "countdown_action" ) );

    } else if( jo.has_object( "countdown_action" ) ) {
        auto tmp = jo.get_object( "countdown_action" );
        def.countdown_action = usage_from_object( tmp ).second;
    }

    if( jo.has_string( "drop_action" ) ) {
        def.drop_action = usage_from_string( jo.get_string( "drop_action" ) );

    } else if( jo.has_object( "drop_action" ) ) {
        auto tmp = jo.get_object( "drop_action" );
        def.drop_action = usage_from_object( tmp ).second;
    }

    if( jo.has_string( "looks_like" ) ) {
        def.looks_like = jo.get_string( "looks_like" );
    }

    load_slot_optional( def.container, jo, "container_data", src );
    load_slot_optional( def.armor, jo, "armor_data", src );
    load_slot_optional( def.pet_armor, jo, "pet_armor_data", src );
    load_slot_optional( def.book, jo, "book_data", src );
    load_slot_optional( def.gun, jo, "gun_data", src );
    load_slot_optional( def.bionic, jo, "bionic_data", src );
    load_slot_optional( def.ammo, jo, "ammo_data", src );
    load_slot_optional( def.seed, jo, "seed_data", src );
    load_slot_optional( def.artifact, jo, "artifact_data", src );
    load_slot_optional( def.brewable, jo, "brewable", src );
    load_slot_optional( def.fuel, jo, "fuel", src );
    load_slot_optional( def.relic_data, jo, "relic_data", src );

    // optional gunmod slot may also specify mod data
    load_slot_optional( def.gunmod, jo, "gunmod_data", src );
    load_slot_optional( def.mod, jo, "gunmod_data", src );

    if( jo.has_string( "abstract" ) ) {
        def.id = jo.get_string( "abstract" );
    } else {
        def.id = jo.get_string( "id" );
    }

    // snippet_category should be loaded after def.id is determined
    if( jo.has_array( "snippet_category" ) ) {
        // auto-create a category that is unlikely to already be used and put the
        // snippets in it.
        def.snippet_category = std::string( "auto:" ) + def.id;
        JsonArray jarr = jo.get_array( "snippet_category" );
        SNIPPET.add_snippets_from_json( def.snippet_category, jarr );
    } else {
        def.snippet_category = jo.get_string( "snippet_category", "" );
    }

    if( jo.has_string( "abstract" ) ) {
        m_abstracts[ def.id ] = def;
    } else {
        m_templates[ def.id ] = def;
    }
}

void Item_factory::load_item_category( JsonObject &jo )
{
    const std::string id = jo.get_string( "id" );
    const auto iter = categories.find( id );
    if( iter != categories.end() ) {
        debugmsg( "Item category %s already exists", id );
        return;
    }
    translation name;
    jo.read( "name", name );
    categories.emplace( id, item_category( id, name, jo.get_int( "sort_rank" ) ) );
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

itype_id Item_factory::migrate_id( const itype_id &id )
{
    auto iter = migrations.find( id );
    return iter != migrations.end() ? iter->second.replace : id;
}

void Item_factory::migrate_item( const itype_id &id, item &obj )
{
    auto iter = migrations.find( id );
    if( iter != migrations.end() ) {
        std::copy( iter->second.flags.begin(), iter->second.flags.end(), std::inserter( obj.item_tags,
                   obj.item_tags.begin() ) );
        obj.charges = iter->second.charges;

        for( const auto &c : iter->second.contents ) {
            if( std::none_of( obj.contents.begin(), obj.contents.end(), [&]( const item & e ) {
            return e.typeId() == c;
            } ) ) {
                obj.emplace_back( c, obj.birthday() );
            }
        }

        // check contents of migrated containers do not exceed capacity
        if( obj.is_container() && !obj.contents.empty() ) {
            item &child = obj.contents.back();
            const int capacity = child.charges_per_volume( obj.get_container_capacity() );
            child.charges = std::min( child.charges, capacity );
        }
    }
}

void Item_factory::set_qualities_from_json( JsonObject &jo, const std::string &member, itype &def )
{
    if( jo.has_array( member ) ) {
        JsonArray jarr = jo.get_array( member );
        while( jarr.has_more() ) {
            JsonArray curr = jarr.next_array();
            const auto quali = std::pair<quality_id, int>( quality_id( curr.get_string( 0 ) ),
                               curr.get_int( 1 ) );
            if( def.qualities.count( quali.first ) > 0 ) {
                curr.throw_error( "Duplicated quality", 0 );
            }
            def.qualities.insert( quali );
        }
    } else {
        jo.throw_error( "Qualities list is not an array", member );
    }
}

void Item_factory::set_properties_from_json( JsonObject &jo, const std::string &member, itype &def )
{
    if( jo.has_array( member ) ) {
        JsonArray jarr = jo.get_array( member );
        while( jarr.has_more() ) {
            JsonArray curr = jarr.next_array();
            const auto prop = std::pair<std::string, std::string>( curr.get_string( 0 ), curr.get_string( 1 ) );
            if( def.properties.count( prop.first ) > 0 ) {
                curr.throw_error( "Duplicated property", 0 );
            }
            def.properties.insert( prop );
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
    m_template_groups.clear();

    categories.clear();

    iuse_function_list.clear();

    m_templates.clear();
    m_runtimes.clear();

    item_blacklist.clear();

    tool_subtypes.clear();

    repair_tools.clear();
    gun_tools.clear();
    misc_tools.clear();
    repair_actions.clear();

    frozen = false;
}

static std::string to_string( Item_group::Type t )
{
    switch( t ) {
        case Item_group::Type::G_COLLECTION:
            return "collection";
        case Item_group::Type::G_DISTRIBUTION:
            return "distribution";
    }

    return "BUGGED";
}

static Item_group *make_group_or_throw( const Group_tag &group_id,
                                        std::unique_ptr<Item_spawn_data> &isd,
                                        Item_group::Type t, int ammo_chance, int magazine_chance )
{
    Item_group *ig = dynamic_cast<Item_group *>( isd.get() );
    if( ig == nullptr ) {
        isd.reset( ig = new Item_group( t, 100, ammo_chance, magazine_chance ) );
    } else if( ig->type != t ) {
        throw std::runtime_error( "item group \"" + group_id + "\" already defined with type \"" +
                                  to_string( ig->type ) + "\"" );
    }
    return ig;
}

template<typename T>
bool load_min_max( std::pair<T, T> &pa, JsonObject &obj, const std::string &name )
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
    result |= obj.read( name + "-min", pa.first );
    result |= obj.read( name + "-max", pa.second );
    return result;
}

bool Item_factory::load_sub_ref( std::unique_ptr<Item_spawn_data> &ptr, JsonObject &obj,
                                 const std::string &name, const Item_group &parent )
{
    const std::string iname( name + "-item" );
    const std::string gname( name + "-group" );
    std::vector< std::pair<const std::string, bool> >
    entries; // pair.second is true for groups, false for items
    const int prob = 100;

    auto get_array = [&obj, &name, &entries]( const std::string & arr_name, const bool isgroup ) {
        if( !obj.has_array( arr_name ) ) {
            return;
        } else if( name != "contents" ) {
            obj.throw_error( string_format( "You can't use an array for '%s'", arr_name ) );
        }
        JsonArray arr = obj.get_array( arr_name );
        while( arr.has_more() ) {
            entries.push_back( std::make_pair( arr.next_string(), isgroup ) );
        }
    };
    get_array( iname, false );
    get_array( gname, true );

    if( obj.has_member( name ) ) {
        obj.throw_error( string_format( "This has been a TODO: since 2014. Use '%s' and/or '%s' instead.",
                                        iname, gname ) );
        return false; // TODO: !
    }
    if( obj.has_string( iname ) ) {
        entries.push_back( std::make_pair( obj.get_string( iname ), false ) );
    }
    if( obj.has_string( gname ) ) {
        entries.push_back( std::make_pair( obj.get_string( gname ), true ) );
    }

    if( entries.size() > 1 && name != "contents" ) {
        obj.throw_error( string_format( "You can only use one of '%s' and '%s'", iname, gname ) );
        return false;
    } else if( entries.size() == 1 ) {
        const auto type = entries.front().second ? Single_item_creator::Type::S_ITEM_GROUP :
                          Single_item_creator::Type::S_ITEM;
        Single_item_creator *result = new Single_item_creator( entries.front().first, type, prob );
        result->inherit_ammo_mag_chances( parent.with_ammo, parent.with_magazine );
        ptr.reset( result );
        return true;
    } else if( entries.empty() ) {
        return false;
    }
    Item_group *result = new Item_group( Item_group::Type::G_COLLECTION, prob, parent.with_ammo,
                                         parent.with_magazine );
    ptr.reset( result );
    for( const auto &elem : entries ) {
        if( elem.second ) {
            result->add_group_entry( elem.first, prob );
        } else {
            result->add_item_entry( elem.first, prob );
        }
    }
    return true;
}

bool Item_factory::load_string( std::vector<std::string> &vec, JsonObject &obj,
                                const std::string &name )
{
    bool result = false;
    std::string temp;

    if( obj.has_array( name ) ) {
        JsonArray arr = obj.get_array( name );
        while( arr.has_more() ) {
            result |= arr.read_next( temp );
            vec.push_back( temp );
        }
    } else if( obj.has_member( name ) ) {
        result |= obj.read( name, temp );
        vec.push_back( temp );
    }

    return result;
}

void Item_factory::add_entry( Item_group &ig, JsonObject &obj )
{
    std::unique_ptr<Item_group> gptr;
    int probability = obj.get_int( "prob", 100 );
    JsonArray jarr;
    if( obj.has_member( "collection" ) ) {
        gptr = std::make_unique<Item_group>( Item_group::G_COLLECTION, probability, ig.with_ammo,
                                             ig.with_magazine );
        jarr = obj.get_array( "collection" );
    } else if( obj.has_member( "distribution" ) ) {
        gptr = std::make_unique<Item_group>( Item_group::G_DISTRIBUTION, probability, ig.with_ammo,
                                             ig.with_magazine );
        jarr = obj.get_array( "distribution" );
    }
    if( gptr ) {
        while( jarr.has_more() ) {
            JsonObject job2 = jarr.next_object();
            add_entry( *gptr, job2 );
        }
        ig.add_entry( std::move( gptr ) );
        return;
    }

    std::unique_ptr<Single_item_creator> sptr;
    if( obj.has_member( "item" ) ) {
        sptr = std::make_unique<Single_item_creator>(
                   obj.get_string( "item" ), Single_item_creator::S_ITEM, probability );
    } else if( obj.has_member( "group" ) ) {
        sptr = std::make_unique<Single_item_creator>(
                   obj.get_string( "group" ), Single_item_creator::S_ITEM_GROUP, probability );
    }
    if( !sptr ) {
        return;
    }

    Item_modifier modifier;
    bool use_modifier = false;
    use_modifier |= load_min_max( modifier.damage, obj, "damage" );
    modifier.damage.first *= itype::damage_scale;
    modifier.damage.second *= itype::damage_scale;
    use_modifier |= load_min_max( modifier.charges, obj, "charges" );
    use_modifier |= load_min_max( modifier.count, obj, "count" );
    use_modifier |= load_sub_ref( modifier.ammo, obj, "ammo", ig );
    use_modifier |= load_sub_ref( modifier.container, obj, "container", ig );
    use_modifier |= load_sub_ref( modifier.contents, obj, "contents", ig );
    use_modifier |= load_string( modifier.custom_flags, obj, "custom-flags" );
    if( use_modifier ) {
        sptr->modifier.emplace( std::move( modifier ) );
    }
    ig.add_entry( std::move( sptr ) );
}

// Load an item group from JSON
void Item_factory::load_item_group( JsonObject &jsobj )
{
    const Item_tag group_id = jsobj.get_string( "id" );
    const std::string subtype = jsobj.get_string( "subtype", "old" );
    load_item_group( jsobj, group_id, subtype );
}

void Item_factory::load_item_group( JsonArray &entries, const Group_tag &group_id,
                                    const bool is_collection, int ammo_chance,
                                    int magazine_chance )
{
    const auto type = is_collection ? Item_group::G_COLLECTION : Item_group::G_DISTRIBUTION;
    std::unique_ptr<Item_spawn_data> &isd = m_template_groups[group_id];
    Item_group *const ig = make_group_or_throw( group_id, isd, type, ammo_chance, magazine_chance );

    while( entries.has_more() ) {
        JsonObject subobj = entries.next_object();
        add_entry( *ig, subobj );
    }
}

void Item_factory::load_item_group( JsonObject &jsobj, const Group_tag &group_id,
                                    const std::string &subtype )
{
    std::unique_ptr<Item_spawn_data> &isd = m_template_groups[group_id];

    Item_group::Type type = Item_group::G_COLLECTION;
    if( subtype == "old" || subtype == "distribution" ) {
        type = Item_group::G_DISTRIBUTION;
    } else if( subtype != "collection" ) {
        jsobj.throw_error( "unknown item group type", "subtype" );
    }
    Item_group *ig = make_group_or_throw( group_id, isd, type, jsobj.get_int( "ammo", 0 ),
                                          jsobj.get_int( "magazine", 0 ) );

    if( subtype == "old" ) {
        JsonArray items = jsobj.get_array( "items" );
        while( items.has_more() ) {
            if( items.test_object() ) {
                JsonObject subobj = items.next_object();
                add_entry( *ig, subobj );
            } else {
                JsonArray pair = items.next_array();
                ig->add_item_entry( pair.get_string( 0 ), pair.get_int( 1 ) );
            }
        }
        return;
    }

    if( jsobj.has_member( "entries" ) ) {
        JsonArray items = jsobj.get_array( "entries" );
        while( items.has_more() ) {
            JsonObject subobj = items.next_object();
            add_entry( *ig, subobj );
        }
    }
    if( jsobj.has_member( "items" ) ) {
        JsonArray items = jsobj.get_array( "items" );
        while( items.has_more() ) {
            if( items.test_string() ) {
                ig->add_item_entry( items.next_string(), 100 );
            } else if( items.test_array() ) {
                JsonArray subitem = items.next_array();
                ig->add_item_entry( subitem.get_string( 0 ), subitem.get_int( 1 ) );
            } else {
                JsonObject subobj = items.next_object();
                add_entry( *ig, subobj );
            }
        }
    }
    if( jsobj.has_member( "groups" ) ) {
        JsonArray items = jsobj.get_array( "groups" );
        while( items.has_more() ) {
            if( items.test_string() ) {
                ig->add_group_entry( items.next_string(), 100 );
            } else if( items.test_array() ) {
                JsonArray subitem = items.next_array();
                ig->add_group_entry( subitem.get_string( 0 ), subitem.get_int( 1 ) );
            } else {
                JsonObject subobj = items.next_object();
                add_entry( *ig, subobj );
            }
        }
    }
}

void Item_factory::set_use_methods_from_json( JsonObject &jo, const std::string &member,
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

std::pair<std::string, use_function> Item_factory::usage_from_object( JsonObject &obj )
{
    auto type = obj.get_string( "type" );

    if( type == "repair_item" ) {
        type = obj.get_string( "item_action_type" );
        if( !has_iuse( type ) ) {
            add_actor( std::make_unique<repair_item_actor>( type ) );
            repair_actions.insert( type );
        }
    }

    use_function method = usage_from_string( type );

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

namespace io
{
template<>
std::string enum_to_string<phase_id>( phase_id data )
{
    switch( data ) {
        // *INDENT-OFF*
        case PNULL: return "null";
        case LIQUID: return "liquid";
        case SOLID: return "solid";
        case GAS: return "gas";
        case PLASMA: return "plasma";
        // *INDENT-ON*
        case num_phases:
            break;
    }
    debugmsg( "Invalid phase" );
    abort();
}
} // namespace io

std::string calc_category( const itype &obj )
{
    if( obj.artifact ) {
        return "artifacts";
    }
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
    if( obj.comestible ) {
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
    for( GroupMap::value_type &group_pair : m_template_groups ) {
        rval.push_back( group_pair.first );
    }
    return rval;
}

bool Item_factory::add_item_to_group( const Group_tag &group_id, const Item_tag &item_id,
                                      int chance )
{
    if( m_template_groups.find( group_id ) == m_template_groups.end() ) {
        return false;
    }
    Item_spawn_data &group_to_access = *m_template_groups[group_id];
    if( group_to_access.has_item( item_id ) ) {
        group_to_access.remove_item( item_id );
    }

    Item_group *ig = dynamic_cast<Item_group *>( &group_to_access );
    if( chance != 0 && ig != nullptr ) {
        // Only re-add if chance != 0
        ig->add_item_entry( item_id, chance );
    }

    return true;
}

void item_group::debug_spawn()
{
    std::vector<std::string> groups = item_controller->get_all_group_names();
    uilist menu;
    menu.text = _( "Test which group?" );
    for( size_t i = 0; i < groups.size(); i++ ) {
        menu.entries.emplace_back( static_cast<int>( i ), true, -2, groups[i] );
    }
    while( true ) {
        menu.query();
        const int index = menu.ret;
        if( index >= static_cast<int>( groups.size() ) || index < 0 ) {
            break;
        }
        // Spawn items from the group 100 times
        std::map<std::string, int> itemnames;
        for( size_t a = 0; a < 100; a++ ) {
            const auto items = items_from( groups[index], calendar::turn );
            for( auto &it : items ) {
                itemnames[it.display_name()]++;
            }
        }
        // Invert the map to get sorting!
        std::multimap<int, std::string> itemnames2;
        for( const auto &e : itemnames ) {
            itemnames2.insert( std::pair<int, std::string>( e.second, e.first ) );
        }
        uilist menu2;
        menu2.text = _( "Result of 100 spawns:" );
        for( const auto &e : itemnames2 ) {
            std::ostringstream buffer;
            buffer << e.first << " x " << e.second << "\n";
            menu2.entries.emplace_back( static_cast<int>( menu2.entries.size() ), true, -2, buffer.str() );
        }
        menu2.query();
    }
}

bool Item_factory::has_template( const itype_id &id ) const
{
    return m_templates.count( id ) || m_runtimes.count( id );
}

std::vector<const itype *> Item_factory::all() const
{
    assert( frozen );

    std::vector<const itype *> res;
    res.reserve( m_templates.size() + m_runtimes.size() );

    for( const auto &e : m_templates ) {
        res.push_back( &e.second );
    }
    for( const auto &e : m_runtimes ) {
        res.push_back( e.second.get() );
    }

    return res;
}

std::vector<const itype *> Item_factory::get_runtime_types() const
{
    std::vector<const itype *> res;
    res.reserve( m_runtimes.size() );
    for( const auto &e : m_runtimes ) {
        res.push_back( e.second.get() );
    }

    return res;
}

/** Find all templates matching the UnaryPredicate function */
std::vector<const itype *> Item_factory::find( const std::function<bool( const itype & )> &func )
{
    std::vector<const itype *> res;

    std::vector<const itype *> opts = item_controller->all();

    std::copy_if( opts.begin(), opts.end(), std::back_inserter( res ),
    [&func]( const itype * e ) {
        return func( *e );
    } );

    return res;
}

Item_tag Item_factory::create_artifact_id() const
{
    Item_tag id;
    int i = m_runtimes.size();
    do {
        id = string_format( "artifact_%d", i );
        i++;
    } while( has_template( id ) );
    return id;
}

std::list<itype_id> Item_factory::subtype_replacement( const itype_id &base ) const
{
    std::list<itype_id> ret;
    ret.push_back( base );
    const auto replacements = tool_subtypes.find( base );
    if( replacements != tool_subtypes.end() ) {
        ret.insert( ret.end(), replacements->second.begin(), replacements->second.end() );
    }

    return ret;
}
