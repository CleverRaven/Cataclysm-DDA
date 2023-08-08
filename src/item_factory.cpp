#include "item_factory.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>

#include "addiction.h"
#include "ammo.h"
#include "assign.h"
#include "bodypart.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "damage.h"
#include "debug.h"
#include "effect_on_condition.h"
#include "enum_conversions.h"
#include "enums.h"
#include "explosion.h"
#include "flag.h"
#include "flat_set.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "init.h"
#include "item.h"
#include "item_contents.h"
#include "item_group.h"
#include "item_pocket.h"
#include "iuse_actor.h"
#include "json.h"
#include "material.h"
#include "options.h"
#include "proficiency.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "relic.h"
#include "requirements.h"
#include "ret_val.h"
#include "stomach.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translations.h"
#include "try_parse_integer.h"
#include "ui.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vitamin.h"

struct tripoint;
template <typename T> struct enum_traits;

static const ammotype ammo_NULL( "NULL" );

static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_bullet( "bullet" );

static const gun_mode_id gun_mode_DEFAULT( "DEFAULT" );
static const gun_mode_id gun_mode_MELEE( "MELEE" );

static const item_category_id item_category_ammo( "ammo" );
static const item_category_id item_category_bionics( "bionics" );
static const item_category_id item_category_books( "books" );
static const item_category_id item_category_clothing( "clothing" );
static const item_category_id item_category_drugs( "drugs" );
static const item_category_id item_category_food( "food" );
static const item_category_id item_category_guns( "guns" );
static const item_category_id item_category_magazines( "magazines" );
static const item_category_id item_category_mods( "mods" );
static const item_category_id item_category_other( "other" );
static const item_category_id item_category_tools( "tools" );
static const item_category_id item_category_veh_parts( "veh_parts" );
static const item_category_id item_category_weapons( "weapons" );

static const item_group_id Item_spawn_data_EMPTY_GROUP( "EMPTY_GROUP" );

static const material_id material_bean( "bean" );
static const material_id material_blood( "blood" );
static const material_id material_bone( "bone" );
static const material_id material_bread( "bread" );
static const material_id material_cheese( "cheese" );
static const material_id material_dried_vegetable( "dried_vegetable" );
static const material_id material_egg( "egg" );
static const material_id material_flesh( "flesh" );
static const material_id material_fruit( "fruit" );
static const material_id material_garlic( "garlic" );
static const material_id material_hblood( "hblood" );
static const material_id material_hflesh( "hflesh" );
static const material_id material_honey( "honey" );
static const material_id material_hydrocarbons( "hydrocarbons" );
static const material_id material_iflesh( "iflesh" );
static const material_id material_junk( "junk" );
static const material_id material_milk( "milk" );
static const material_id material_mushroom( "mushroom" );
static const material_id material_nut( "nut" );
static const material_id material_oil( "oil" );
static const material_id material_tomato( "tomato" );
static const material_id material_veggy( "veggy" );
static const material_id material_wheat( "wheat" );
static const material_id material_wool( "wool" );

static const quality_id qual_GUN( "GUN" );

static const skill_id skill_pistol( "pistol" );
static const skill_id skill_rifle( "rifle" );
static const skill_id skill_shotgun( "shotgun" );
static const skill_id skill_smg( "smg" );

static item_blacklist_t item_blacklist;

static DynamicDataLoader::deferred_json deferred;

std::unique_ptr<Item_factory> item_controller = std::make_unique<Item_factory>();

/** @relates string_id */
template<>
const itype &string_id<itype>::obj() const
{
    const itype *result = item_controller->find_template( *this );
    static const itype dummy{};
    return result ? *result : dummy;
}

/** @relates string_id */
template<>
bool string_id<itype>::is_valid() const
{
    return item_controller->has_template( *this );
}

/** @relates string_id */
template<>
bool string_id<Item_spawn_data>::is_valid() const
{
    return item_controller->get_group( *this ) != nullptr;
}

static item_category_id calc_category( const itype &obj );
static void hflesh_to_flesh( itype &item_template );

bool item_is_blacklisted( const itype_id &id )
{
    return item_blacklist.blacklist.count( id );
}

static void assign( const JsonObject &jo, const std::string_view name,
                    std::map<gun_mode_id, gun_modifier_data> &mods )
{
    if( !jo.has_array( name ) ) {
        return;
    }
    mods.clear();
    for( JsonArray curr : jo.get_array( name ) ) {
        translation text;
        curr.read( 1, text );
        mods.emplace( gun_mode_id( curr.get_string( 0 ) ), gun_modifier_data( text,
                      curr.get_int( 2 ), curr.size() >= 4 ? curr.get_tags( 3 ) : std::set<std::string>() ) );
    }
}

static bool assign_coverage_from_json( const JsonObject &jo, const std::string &key,
                                       body_part_set &parts )
{
    auto parse = [&parts]( const std::string & val ) {
        parts.set( bodypart_str_id( val ) );
    };

    if( jo.has_array( key ) ) {
        for( const std::string line : jo.get_array( key ) ) {
            parse( line );
        }
        return true;

    } else if( jo.has_string( key ) ) {
        parse( jo.get_string( key ) );
        return true;

    } else {
        return false;
    }
}

static bool assign_coverage_from_json( const JsonObject &jo, const std::string &key,
                                       std::optional<body_part_set> &parts )
{
    body_part_set temp;
    if( assign_coverage_from_json( jo, key, temp ) ) {
        parts = temp;
        return true;
    }

    return false;
}

static bool is_physical( const itype &type )
{
    return !type.has_flag( flag_AURA ) &&
           !type.has_flag( flag_CORPSE ) &&
           !type.has_flag( flag_IRREMOVABLE ) &&
           !type.has_flag( flag_NO_DROP ) &&
           !type.has_flag( flag_NO_UNWIELD ) &&
           !type.has_flag( flag_PERSONAL ) &&
           !type.has_flag( flag_PSEUDO ) &&
           !type.has_flag( flag_ZERO_WEIGHT );
}

void Item_factory::finalize_pre( itype &obj )
{
    // Add relic data by ID we defered
    if( obj.relic_data ) {
        obj.relic_data->finalize();
    }

    if( obj.count_by_charges() ) {
        obj.item_tags.insert( flag_NO_REPAIR );
    }

    finalize_damage_map( obj.melee );
    if( !obj.melee_proportional.empty() ) {
        finalize_damage_map( obj.melee_proportional, false, 1.f );
        for( std::pair<const damage_type_id, float> &dt : obj.melee ) {
            const auto iter = obj.melee_proportional.find( dt.first );
            if( iter != obj.melee_proportional.end() ) {
                dt.second *= iter->second;
                // For maintaining legacy behaviour (when melee damage used ints)
                dt.second = std::floor( dt.second );
            }
        }
    }
    if( !obj.melee_relative.empty() ) {
        finalize_damage_map( obj.melee_relative, false, 0.f );
        for( std::pair<const damage_type_id, float> &dt : obj.melee ) {
            const auto iter = obj.melee_relative.find( dt.first );
            if( iter != obj.melee_relative.end() ) {
                dt.second += iter->second;
                // For maintaining legacy behaviour (when melee damage used ints)
                dt.second = std::floor( dt.second );
            }
        }
    }

    if( obj.has_flag( flag_STAB ) ) {
        debugmsg( "The \"STAB\" flag used on %s is obsolete, add a \"stab\" value in the \"melee_damage\" object instead.",
                  obj.id.c_str() );
    }

    // add usage methods (with default values) based upon qualities
    // if a method was already set the specific values remain unchanged
    for( const auto &q : obj.qualities ) {
        for( const auto &u : q.first.obj().usages ) {
            if( q.second >= u.first ) {
                emplace_usage( obj.use_methods, u.second );
                // As far as I know all the actions provided by quality level do not consume ammo
                // So it is safe to set all to 0
                // To do: read the json file of this item again and get for each quality a scale number
                obj.ammo_scale.emplace( u.second, 0 );
            }
        }
    }
    for( const auto &q : obj.charged_qualities ) {
        for( const auto &u : q.first.obj().usages ) {
            if( q.second >= u.first ) {
                emplace_usage( obj.use_methods, u.second );
                // I do not know how to get the ammo scale, so hopefully it naturally comes with the item's scale?
            }
        }
    }

    if( obj.mod ) {
        std::string func = obj.gunmod ? "GUNMOD_ATTACH" : "TOOLMOD_ATTACH";
        emplace_usage( obj.use_methods, func );
        obj.ammo_scale.emplace( func, 0 );
    } else if( obj.gun ) {
        const std::string func = "detach_gunmods";
        emplace_usage( obj.use_methods, func );
        obj.ammo_scale.emplace( func, 0 );
        const std::string func2 = "modify_gunmods";
        emplace_usage( obj.use_methods, func2 );
        obj.ammo_scale.emplace( func2, 0 );
    }

    if( get_option<bool>( "NO_FAULTS" ) ) {
        obj.faults.clear();
    }

    // If no category was forced via JSON automatically calculate one now
    if( !obj.category_force.is_valid() || obj.category_force.is_empty() ) {
        obj.category_force = calc_category( obj );
    }

    // use pre-Cataclysm price as default if post-cataclysm price unspecified
    if( obj.price_post < 0_cent ) {
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
            debugmsg( obj.id.str() + " stack size is too large, reducing to 200" );
            obj.stack_size = 200;
        }
    }

    // Items always should have some volume.
    // TODO: handle possible exception software?
    if( obj.volume <= 0_ml ) {
        if( is_physical( obj ) ) {
            debugmsg( "item %s has zero volume (if zero volume is intentional "
                      "you can suppress this error with the ZERO_WEIGHT "
                      "flag)\n", obj.id.str() );
        }
        obj.volume = units::from_milliliter( 1 );
    }

    // set light_emission based on LIGHT_[X] flag
    for( const auto &f : obj.item_tags ) {
        if( string_starts_with( f.str(), "LIGHT_" ) ) {
            ret_val<int> ll = try_parse_integer<int>( f.str().substr( 6 ), false );
            if( ll.success() ) {
                if( ll.value() > 0 ) {
                    obj.light_emission = ll.value();
                } else {
                    debugmsg( "item %s specifies light emission of zero, which is redundant",
                              obj.id.str() );
                }
            } else {
                debugmsg( "error parsing integer light emission suffic for item %s: %s",
                          obj.id.str(), ll.str() );
            }
        }
    }
    // remove LIGHT_[X] flags
    erase_if( obj.item_tags, []( const flag_id & f ) {
        return string_starts_with( f.str(), "LIGHT_" );
    } );

    // for ammo not specifying loudness derive value from other properties
    if( obj.ammo ) {
        if( obj.ammo->loudness < 0 ) {
            obj.ammo->loudness = obj.ammo->range * 2;
            for( const damage_unit &du : obj.ammo->damage ) {
                obj.ammo->loudness += ( du.amount + du.res_pen ) * 2;
            }
        }

        const auto &mats = obj.materials;
        if( mats.find( material_hydrocarbons ) == mats.end() &&
            mats.find( material_oil ) == mats.end() ) {
            const auto &ammo_effects = obj.ammo->ammo_effects;
            obj.ammo->cookoff = ammo_effects.count( "INCENDIARY" ) > 0 ||
                                ammo_effects.count( "COOKOFF" ) > 0;
            static const std::set<std::string> special_cookoff_tags = {{
                    "NAPALM", "NAPALM_BIG",
                    "EXPLOSIVE_SMALL", "EXPLOSIVE", "EXPLOSIVE_BIG", "EXPLOSIVE_HUGE",
                    "TOXICGAS", "SMOKE", "SMOKE_BIG",
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
        // Special casing for shot, since the damage per pellet can be tiny.
        // Instead of handling fractional damage values, we scale the effective number
        // of projectiles based on the damage so that they end up at 1.
        if( obj.ammo->count > 1 && obj.ammo->shot_damage.total_damage() < 1.0f ) {
            // Patch to fixup shot without shot_damage until I get all the definitions consistent.
            if( obj.ammo->shot_damage.damage_units.empty() ) {
                obj.ammo->shot_damage.damage_units.emplace_back( damage_bullet, 0.1f );
            }
            obj.ammo->count = obj.ammo->count * obj.ammo->shot_damage.total_damage();
            obj.ammo->shot_damage.damage_units.front().amount = 1.0f;
        }
    }

    // Helper for ammo migration in following sections
    auto migrate_ammo_set = [this]( std::set<ammotype> &ammoset ) {
        for( auto ammo_type_it = ammoset.begin(); ammo_type_it != ammoset.end(); ) {
            const itype_id default_ammo_type = ammo_type_it->obj().default_ammotype();
            auto maybe_migrated = migrated_ammo.find( default_ammo_type );
            if( maybe_migrated != migrated_ammo.end() ) {
                ammo_type_it = ammoset.erase( ammo_type_it );
                ammoset.insert( maybe_migrated->second );
            } else {
                ++ammo_type_it;
            }
        }
    };

    auto migrate_ammo_map = [this]( std::map<ammotype, int> &ammomap ) {
        for( auto ammo_type_it = ammomap.begin(); ammo_type_it != ammomap.end(); ) {
            const itype_id default_ammo_type = ammo_type_it->first.obj().default_ammotype();
            auto maybe_migrated = migrated_ammo.find( default_ammo_type );
            if( maybe_migrated != migrated_ammo.end() ) {
                int capacity = ammo_type_it->second;
                ammo_type_it = ammomap.erase( ammo_type_it );
                ammomap.emplace( maybe_migrated->second, capacity );
            } else {
                ++ammo_type_it;
            }
        }
    };

    if( obj.magazine ) {
        // ensure default_ammo is set
        if( obj.magazine->default_ammo.is_null() ) {
            obj.magazine->default_ammo = ammotype( *obj.magazine->type.begin() )->default_ammotype();
        }

        // If the magazine has ammo types for which the default ammo has been migrated, we need to
        // replace those ammo types with that of the migrated ammo
        migrate_ammo_set( obj.magazine->type );

        // ensure default_ammo is migrated if need be
        auto maybe_migrated = migrated_ammo.find( obj.magazine->default_ammo );
        if( maybe_migrated != migrated_ammo.end() ) {
            obj.magazine->default_ammo = maybe_migrated->second.obj().default_ammotype();
        }

        for( pocket_data &magazine : obj.pockets ) {
            if( magazine.type != item_pocket::pocket_type::MAGAZINE ) {
                continue;
            }
            migrate_ammo_map( magazine.ammo_restriction );
        }

        // list all the ammo types that work
        for( const ammotype &am : obj.magazine->type ) {
            auto temp_vec = Item_factory::find( [obj, am]( const itype & e ) {
                if( !e.ammo || item_is_blacklisted( e.get_id() ) ) {
                    return false;
                }

                return e.ammo->type == am;
            } );

            for( const itype *val : temp_vec ) {
                obj.magazine->cached_ammos[am].insert( val->get_id() );
            }
        }

    }

    // Migrate compatible magazines
    for( auto kv : obj.magazines ) {
        for( auto mag_it = kv.second.begin(); mag_it != kv.second.end(); ) {
            auto maybe_migrated = migrated_magazines.find( *mag_it );
            if( maybe_migrated != migrated_magazines.end() ) {
                mag_it = kv.second.erase( mag_it );
                kv.second.insert( kv.second.begin(), maybe_migrated->second );
            } else {
                ++mag_it;
            }
        }
    }

    // Migrate default magazines
    for( auto kv : obj.magazine_default ) {
        auto maybe_migrated = migrated_magazines.find( kv.second );
        if( maybe_migrated != migrated_magazines.end() ) {
            kv.second = maybe_migrated->second;
        }
    }

    if( obj.mod ) {
        // Migrate acceptable ammo and ammo modifiers
        migrate_ammo_set( obj.mod->acceptable_ammo );
        migrate_ammo_set( obj.mod->ammo_modifier );

        for( auto kv = obj.mod->magazine_adaptor.begin(); kv != obj.mod->magazine_adaptor.end(); ) {
            auto maybe_migrated = migrated_ammo.find( kv->first.obj().default_ammotype() );
            if( maybe_migrated != migrated_ammo.end() ) {
                for( const itype_id &compatible_mag : kv->second ) {
                    obj.mod->magazine_adaptor[maybe_migrated->second].insert( compatible_mag );
                }
                kv = obj.mod->magazine_adaptor.erase( kv );
            } else {
                ++kv;
            }
        }
    }

    if( obj.gun ) {
        // If the gun has ammo types for which the default ammo has been migrated, we need to
        // replace those ammo types with that of the migrated ammo
        for( auto ammo_type_it = obj.gun->ammo.begin(); ammo_type_it != obj.gun->ammo.end(); ) {
            auto maybe_migrated = migrated_ammo.find( ammo_type_it->obj().default_ammotype() );
            if( maybe_migrated != migrated_ammo.end() ) {
                const ammotype old_ammo = *ammo_type_it;
                // Remove the old ammotype add the migrated version
                ammo_type_it = obj.gun->ammo.erase( ammo_type_it );
                const ammotype &new_ammo = maybe_migrated->second;
                obj.gun->ammo.insert( obj.gun->ammo.begin(), new_ammo );
                // Migrate the compatible magazines
                auto old_mag_it = obj.magazines.find( old_ammo );
                if( old_mag_it != obj.magazines.end() ) {
                    for( const itype_id &old_mag : old_mag_it->second ) {
                        obj.magazines[new_ammo].insert( old_mag );
                    }
                    obj.magazines.erase( old_ammo );
                }
                // And the default magazines for each magazine type
                auto old_default_mag_it = obj.magazine_default.find( old_ammo );
                if( old_default_mag_it != obj.magazine_default.end() ) {
                    const itype_id &old_default_mag = old_default_mag_it->second;
                    obj.magazine_default[new_ammo] = old_default_mag;
                    obj.magazine_default.erase( old_ammo );
                }
            } else {
                ++ammo_type_it;
            }
        }

        // generate cached map for [mag type_id] -> [set of compatible guns]
        for( const pocket_data &pocket : obj.pockets ) {
            if( pocket.type != item_pocket::pocket_type::MAGAZINE_WELL ) {
                continue;
            }
            for( const itype_id &mag_type_id : pocket.item_id_restriction ) {
                if( item_is_blacklisted( mag_type_id ) || item_is_blacklisted( obj.id ) ) {
                    continue;
                }
                islot_magazine::compatible_guns[mag_type_id].insert( obj.id );
            }
        }

        for( pocket_data &magazine : obj.pockets ) {
            if( magazine.type != item_pocket::pocket_type::MAGAZINE ) {
                continue;
            }
            migrate_ammo_map( magazine.ammo_restriction );
        }

        // TODO: add explicit action field to gun definitions
        const auto defmode_name = [&]() {
            if( obj.gun->clip == 1 ) {
                return to_translation( "manual" ); // break-type actions
            } else if( obj.gun->skill_used == skill_pistol && obj.has_flag( flag_RELOAD_ONE ) ) {
                return to_translation( "revolver" );
            } else {
                return to_translation( "semi-auto" );
            }
        };

        // if the gun doesn't have a DEFAULT mode then add one now
        obj.gun->modes.emplace( gun_mode_DEFAULT,
                                gun_modifier_data( defmode_name(), 1, std::set<std::string>() ) );

        // If a "gun" has a reach attack, give it an additional melee mode.
        if( obj.has_flag( flag_REACH_ATTACK ) ) {
            obj.gun->modes.emplace( gun_mode_MELEE,
                                    gun_modifier_data( to_translation( "melee" ), 1,
            { "MELEE" } ) );
        }

        if( obj.gun->handling < 0 ) {
            // TODO: specify in JSON via classes
            if( obj.gun->skill_used == skill_rifle ||
                obj.gun->skill_used == skill_smg ||
                obj.gun->skill_used == skill_shotgun ) {
                obj.gun->handling = 20;
            } else {
                obj.gun->handling = 10;
            }
        }

        // list all the ammo types that work
        for( const ammotype &am : obj.gun->ammo ) {
            auto temp_vec = Item_factory::find( [obj, am]( const itype & e ) {
                if( !e.ammo || item_is_blacklisted( e.get_id() ) ) {
                    return false;
                }

                return e.ammo->type == am;
            } );

            for( const itype *val : temp_vec ) {
                obj.gun->cached_ammos[am].insert( val->get_id() );
            }
        }

    }

    set_allergy_flags( obj );
    hflesh_to_flesh( obj );
    npc_implied_flags( obj );

    if( obj.comestible ) {
        std::map<vitamin_id, int> &vitamins = obj.comestible->default_nutrition.vitamins;
        if( get_option<bool>( "NO_VITAMINS" ) ) {
            for( auto &vit : vitamins ) {
                if( vit.first->type() == vitamin_type::VITAMIN ) {
                    vit.second = 0;
                }
            }
        } else if( vitamins.empty() && obj.comestible->healthy >= 0 ) {
            // Default vitamins of healthy comestibles to their edible base materials if none explicitly specified.
            int healthy = std::max( obj.comestible->healthy, 1 ) * 10;
            auto mat = obj.materials;

            // TODO: migrate inedible comestibles to appropriate alternative types.
            for( auto m = mat.begin(); m != mat.end(); ) {
                if( !m->first->edible() ) {
                    m = mat.erase( m );
                } else {
                    m++;
                }
            }

            // For comestibles composed of multiple edible materials we calculate the average.
            for( const auto &v : vitamin::all() ) {
                if( !vitamins.count( v.first ) ) {
                    for( const auto &m : mat ) {
                        double amount = m.first->vitamin( v.first ) * healthy / mat.size();
                        vitamins[v.first] += std::ceil( amount );
                    }
                }
            }
        }
    }

    // recurse into subtypes, adding self as substitution for all items in the subtypes chain
    for( itype_id it = obj.id; it->tool && it->tool->subtype.is_valid(); it = it->tool->subtype ) {
        tool_subtypes[ it->tool->subtype ].insert( obj.id );
    }

    for( auto &e : obj.use_methods ) {
        e.second.get_actor_ptr()->finalize( obj.id );
    }

    if( obj.drop_action.get_actor_ptr() != nullptr ) {
        obj.drop_action.get_actor_ptr()->finalize( obj.id );
    }

    if( obj.can_use( "MA_MANUAL" ) && obj.book && obj.book->martial_art.is_null() &&
        string_starts_with( obj.get_id().str(), "manual_" ) ) {
        // HACK: Legacy martial arts books rely on a hack whereby the name of the
        // martial art is derived from the item id
        obj.book->martial_art = matype_id( "style_" + obj.get_id().str().substr( 7 ) );
    }

    if( obj.can_use( "link_up" ) ) {
        obj.ammo_scale.emplace( "link_up", 0 );
    }

    if( obj.longest_side == -1_mm ) {
        units::volume effective_volume = obj.count_by_charges() &&
                                         obj.stack_size > 0 ? ( obj.volume / obj.stack_size ) : obj.volume;
        obj.longest_side = units::default_length_from_volume<int>( effective_volume );
    }
}

void Item_factory::register_cached_uses( const itype &obj )
{
    for( const auto &e : obj.use_methods ) {
        // can this item function as a repair tool?
        if( repair_actions.count( e.first ) ) {
            repair_tools.insert( obj.id );
        }

        // can this item be used to repair complex firearms?
        if( e.first == "GUN_REPAIR" ) {
            gun_tools.insert( obj.id );
        }
    }
}

void Item_factory::finalize_post( itype &obj )
{
    erase_if( obj.item_tags, [&]( const flag_id & f ) {
        if( !f.is_valid() ) {
            debugmsg( "itype '%s' uses undefined flag '%s'. Please add corresponding 'json_flag' entry to json.",
                      obj.id.str(), f.str() );
            return true;
        }
        return false;
    } );

    if( obj.gun && !obj.gunmod && !obj.has_flag( flag_PRIMITIVE_RANGED_WEAPON ) ) {
        const quality_id qual_gun_skill( to_upper_case( obj.gun->skill_used.str() ) );

        obj.qualities[qual_GUN] = std::max( obj.qualities[qual_GUN], 1 );
        if( qual_gun_skill.is_valid() ) {
            obj.qualities[qual_gun_skill] = std::max( obj.qualities[qual_gun_skill], 1 );
        }
    }

    // handle complex firearms as a special case
    if( obj.gun && !obj.has_flag( flag_PRIMITIVE_RANGED_WEAPON ) ) {
        std::copy( gun_tools.begin(), gun_tools.end(), std::inserter( obj.repair, obj.repair.begin() ) );
        return;
    }

    if( obj.armor ) {
        finalize_post_armor( obj );
    }

    // if we haven't set what the item can be repaired with calculate it now
    if( obj.repairs_with.empty() ) {
        for( const auto &mats : obj.materials ) {
            obj.repairs_with.insert( mats.first );
        }
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
            if( std::any_of( obj.repairs_with.begin(),
            obj.repairs_with.end(), [&opts]( const material_id & m ) {
            return opts.count( m ) > 0;
            } ) ) {
                obj.repair.insert( tool );
            }
        }
    }

    // go through each pocket and assign the name as a fallback definition
    for( pocket_data &pocket : obj.pockets ) {
        pocket.name = obj.name;
    }

    if( obj.comestible ) {
        for( const std::pair<const diseasetype_id, int> &elem : obj.comestible->contamination ) {
            const diseasetype_id dtype = elem.first;
            if( !dtype.is_valid() ) {
                debugmsg( "contamination in %s contains invalid diseasetype_id %s.",
                          obj.id.str(), dtype.str() );
            }
        }
    }
}

void Item_factory::finalize_post_armor( itype &obj )
{
    // if this armor doesn't have material info should try to populate it with base item materials
    for( armor_portion_data &data : obj.armor->sub_data ) {
        if( data.materials.empty() ) {
            // if no portion info defined skip scaling for portions
            bool skip_scale = obj.mat_portion_total == 0;
            for( const auto &m : obj.materials ) {
                float factor = skip_scale
                               ? obj.materials.size()
                               : static_cast<float>( m.second ) / static_cast<float>( obj.mat_portion_total );
                part_material pm( m.first, 100, factor * data.avg_thickness );
                // need to ignore sheet thickness since inferred thicknesses are not gonna be perfect
                pm.ignore_sheet_thickness = true;
                data.materials.push_back( pm );
            }
        }
    }

    // cache some values and entries before consolidating info per limb
    for( armor_portion_data &data : obj.armor->sub_data ) {
        // Setting max_encumber must be in finalize_post because it relies on
        // stack_size being set for all ammo, which happens in finalize_pre.
        if( data.max_encumber == -1 ) {
            units::volume total_nonrigid_volume = 0_ml;
            for( const pocket_data &pocket : obj.pockets ) {
                if( !pocket.rigid ) {
                    // include the modifier for each individual pocket
                    total_nonrigid_volume += pocket.max_contains_volume() * pocket.volume_encumber_modifier;
                }
            }
            data.max_encumber = data.encumber + total_nonrigid_volume * data.volume_encumber_modifier /
                                armor_portion_data::volume_per_encumbrance;
        }

        // Precalc average thickness per portion
        int data_count = 0;
        float thic_acc = 0.0f;
        for( part_material &m : data.materials ) {
            thic_acc += m.thickness * m.cover / 100.0f;
            data_count++;
        }
        if( data_count > 0 && thic_acc > std::numeric_limits<float>::epsilon() ) {
            data.avg_thickness = thic_acc;
        }

        // Precalc hardness and comfort for these parts of the armor. uncomfortable material supersedes softness (all rigid materials are assumed to be uncomfy)
        for( const part_material &m : data.materials ) {
            if( m.cover > islot_armor::test_threshold ) {
                if( m.id->soft() ) {
                    data.comfortable = true;
                } else {
                    data.rigid = true;
                }
                if( m.id->uncomfortable() ) {
                    data.comfortable = false;
                }
            }
        }
    }

    for( auto itt = obj.armor->sub_data.begin(); itt != obj.armor->sub_data.end(); ++itt ) {
        // using empty to signify it has already been consolidated
        if( !itt->sub_coverage.empty() ) {
            //check if any further entries should be combined with this one
            for( auto comp_itt = std::next( itt ); comp_itt != obj.armor->sub_data.end(); ++comp_itt ) {
                if( armor_portion_data::should_consolidate( *itt, *comp_itt ) ) {
                    // they are the same so add the covers and sub covers to the original and then clear them from the other
                    itt->covers->unify_set( comp_itt->covers.value() );
                    itt->sub_coverage.insert( comp_itt->sub_coverage.begin(), comp_itt->sub_coverage.end() );
                    comp_itt->covers->clear();
                    comp_itt->sub_coverage.clear();
                }
            }
        }
    }
    //remove any now empty entries
    auto remove_itt = std::remove_if( obj.armor->sub_data.begin(),
    obj.armor->sub_data.end(), [&]( const armor_portion_data & data ) {
        return data.sub_coverage.empty() && data.covers.value().none();
    } );
    obj.armor->sub_data.erase( remove_itt, obj.armor->sub_data.end() );

    // now consolidate all the loaded sub_data to one entry per body part
    for( const armor_portion_data &sub_armor : obj.armor->sub_data ) {
        // for each body part this covers we need to add to the overall data for that bp
        if( sub_armor.covers.has_value() ) {
            for( const bodypart_str_id &bp : sub_armor.covers.value() ) {
                bool found = false;
                // go through and find if the body part already exists

                for( armor_portion_data &it : obj.armor->data ) {
                    // if it contains the body part update the values with data from this
                    //body_part_set set = it.covers.value();
                    if( it.covers->test( bp ) ) {
                        found = true;
                        // modify the values with additional info

                        it.encumber += sub_armor.encumber;
                        it.max_encumber += sub_armor.max_encumber;

                        for( const encumbrance_modifier &en : sub_armor.encumber_modifiers ) {
                            it.encumber_modifiers.push_back( en );
                        }

                        // get the amount of the limb that is covered with sublocations
                        // for overall coverage we need to scale coverage by that
                        float scale = sub_armor.max_coverage( bp ) / 100.0;

                        float it_scale = it.max_coverage( bp ) / 100.0;

                        it.coverage += sub_armor.coverage * scale;
                        it.cover_melee += sub_armor.cover_melee * scale;
                        it.cover_ranged += sub_armor.cover_ranged * scale;
                        it.cover_vitals += sub_armor.cover_vitals;

                        // these values need to be averaged based on proportion covered
                        it.avg_thickness = ( sub_armor.avg_thickness * scale + it.avg_thickness * it_scale ) /
                                           ( scale + it_scale );
                        it.env_resist = ( sub_armor.env_resist * scale + it.env_resist * it_scale ) /
                                        ( scale + it_scale );
                        it.env_resist_w_filter = ( sub_armor.env_resist_w_filter * scale + it.env_resist_w_filter *
                                                   it_scale ) / ( scale + it_scale );

                        // add layers that are covered by sublimbs
                        for( const layer_level &ll : sub_armor.layers ) {
                            it.layers.insert( ll );
                        }

                        // if you are trying to add a new data entry and either the original data
                        // or the new data has an empty sublocations list then say that you are
                        // redefining a limb
                        if( it.sub_coverage.empty() || sub_armor.sub_coverage.empty() ) {
                            debugmsg( "item %s has multiple entries for %s.",
                                      obj.id.str(), bp.str() );
                        }

                        // go through the materials list and update data
                        for( const part_material &new_mat : sub_armor.materials ) {
                            bool mat_found = false;
                            for( part_material &old_mat : it.materials ) {
                                if( old_mat.id == new_mat.id ) {
                                    mat_found = true;
                                    // values should be averaged
                                    float max_coverage_new = sub_armor.max_coverage( bp );
                                    float max_coverage_mats = it.max_coverage( bp );

                                    // the percent of the coverable bits that this armor does cover
                                    float coverage_multiplier = sub_armor.coverage * max_coverage_new / 100.0f;

                                    // portion should be handled as the portion scaled by relative coverage
                                    old_mat.cover = old_mat.cover + static_cast<float>( new_mat.cover ) * coverage_multiplier / 100.0f;

                                    // with the max values we can get the weight that each should have
                                    old_mat.thickness = ( max_coverage_new * new_mat.thickness + max_coverage_mats *
                                                          old_mat.thickness ) / ( max_coverage_mats + max_coverage_new );
                                }
                            }
                            // if we didn't find an entry for this material
                            // create new entry with a scaled material coverage
                            if( !mat_found ) {
                                float max_coverage_new = sub_armor.max_coverage( bp );

                                // the percent of the coverable bits that this armor does cover
                                float coverage_multiplier = sub_armor.coverage * max_coverage_new / 100.0f;

                                part_material modified_mat = new_mat;
                                // if for example your elbow was covered in plastic but none of the rest of the arm
                                // this should be represented correctly in the UI with the covers for plastic being 5%
                                // of the arm. Similarily 50% covered in plastic covering only 30% of the arm should lead to
                                // 15% covered for the arm overall
                                modified_mat.cover = static_cast<float>( new_mat.cover ) * coverage_multiplier / 100.0f;
                                it.materials.push_back( modified_mat );
                            }
                        }

                        // add additional sub coverage locations to the original list
                        for( const sub_bodypart_str_id &sbp : sub_armor.sub_coverage ) {
                            it.sub_coverage.insert( sbp );
                        }
                    }
                }

                // if not found create a new bp entry
                if( !found ) {
                    // copy values to data but only have one limb
                    armor_portion_data new_limb = sub_armor;
                    new_limb.covers->clear();
                    new_limb.covers->set( bp );

                    // get the amount of the limb that is covered with sublocations
                    // for overall coverage we need to scale coverage by that
                    float scale = new_limb.max_coverage( bp ) / 100.0;

                    new_limb.coverage = new_limb.coverage * scale;
                    new_limb.cover_melee = new_limb.cover_melee * scale;
                    new_limb.cover_ranged = new_limb.cover_ranged * scale;

                    // need to scale each material coverage the same way since they will after this be
                    // scaled back up at the end of the amalgamation
                    for( part_material &mat : new_limb.materials ) {
                        mat.cover = static_cast<float>( mat.cover ) * new_limb.coverage / 100.0f;
                    }

                    obj.armor->data.push_back( new_limb );
                }
            }
        }

    }

    // calculate encumbrance data per limb if done by description
    for( armor_portion_data &data : obj.armor->data ) {
        if( !data.encumber_modifiers.empty() ) {
            // we know that the data entry covers a single bp
            data.encumber = data.calc_encumbrance( obj.weight, *data.covers.value().begin() );

            // need to account for varsize stuff here and double encumbrance if so
            if( obj.has_flag( flag_VARSIZE ) ) {
                data.encumber = std::min( data.encumber * 2, data.encumber + 10 );
            }

            // Recalc max encumber as well
            units::volume total_nonrigid_volume = 0_ml;
            for( const pocket_data &pocket : obj.pockets ) {
                if( !pocket.rigid ) {
                    // include the modifier for each individual pocket
                    total_nonrigid_volume += pocket.max_contains_volume() * pocket.volume_encumber_modifier;
                }
            }
            data.max_encumber = data.encumber + total_nonrigid_volume * data.volume_encumber_modifier /
                                armor_portion_data::volume_per_encumbrance;
        }
    }

    // need to scale amalgamized portion data based on total coverage.
    // 3% of 48% needs to be scaled to 6% of 100%
    for( armor_portion_data &it : obj.armor->data ) {
        for( part_material &mat : it.materials ) {
            // scale the value of portion covered based on how much total is covered
            // if you proportionally only cover 5% of the arm but overall cover 50%
            // you actually proportionally cover 10% of the armor

            // in case of 0 coverage just say the mats cover it all
            if( it.coverage == 0 ) {
                mat.cover = 100;
            } else {
                mat.cover = std::round( static_cast<float>( mat.cover ) / ( static_cast<float>
                                        ( it.coverage ) / 100.0f ) );
            }
        }
    }

    // store a shorthand var for if the item has notable sub coverage data
    for( const armor_portion_data &armor_data : obj.armor->data ) {
        if( obj.armor->has_sub_coverage ) {
            // if we already know it has subcoverage break from the loop
            break;
        }

        // if the item covers everything it doesn't have specific sub coverage
        // so don't need to display it in the UI and do tests for it
        if( !armor_data.sub_coverage.empty() ) {
            // go through and see if we are missing any sublocations
            // if we are missing some then the item does cover specific locations
            // this flag is mostly used to skip itterating and testing
            // if UI should be displayed

            // each armor data entry covers exactly 1 body part
            for( const sub_bodypart_str_id &compare_sbp : armor_data.covers->begin()->obj().sub_parts ) {
                if( compare_sbp->secondary ) {
                    // don't care about secondary locations
                    continue;
                }
                bool found = false;
                for( const sub_bodypart_str_id &sbp : armor_data.sub_coverage ) {
                    if( compare_sbp == sbp ) {
                        found = true;
                    }
                }
                // if an entry is not found we cover specific parts so this item has sub_coverage
                if( !found ) {
                    obj.armor->has_sub_coverage = true;
                }
            }
        }
    }

    // calculate worst case and best case protection %
    for( armor_portion_data &armor_data : obj.armor->data ) {
        // go through each material and contribute its values
        float tempbest = 1.0f;
        float tempworst = 1.0f;
        for( const part_material &mat : armor_data.materials ) {
            // the percent chance the material is not hit
            float cover = mat.cover * .01f;
            float cover_invert = 1.0f - cover;

            tempbest *= cover;
            // just interested in cover values that can fail.
            // multiplying by 0 would make this value pointless
            if( cover_invert > 0 ) {
                tempworst *= cover_invert;
            }
        }

        // if tempworst is 1 then the item is homogenous so it only has a single option for damage
        if( tempworst == 1 ) {
            tempworst = 0;
        }

        // if not exactly 0 it should display as at least 1
        if( tempworst > 0 ) {
            armor_data.worst_protection_chance = std::max( 1.0f, tempworst * 100.0f );
        } else {
            armor_data.worst_protection_chance = 0;
        }
        if( tempbest > 0 ) {
            armor_data.best_protection_chance = std::max( 1.0f, tempbest * 100.0f );
        } else {
            armor_data.best_protection_chance = 0;
        }
    }

    // calculate worst case and best case protection %
    for( armor_portion_data &armor_data : obj.armor->sub_data ) {
        // go through each material and contribute its values
        float tempbest = 1.0f;
        float tempworst = 1.0f;
        for( const part_material &mat : armor_data.materials ) {
            // the percent chance the material is not hit
            float cover = mat.cover * .01f;
            float cover_invert = 1.0f - cover;

            tempbest *= cover;
            // just interested in cover values that can fail.
            // multiplying by 0 would make this value pointless
            if( cover_invert > 0 ) {
                tempworst *= cover_invert;
            }
        }

        // if tempworst is 1 then the item is homogenous so it only has a single option for damage
        if( tempworst == 1 ) {
            tempworst = 0;
        }

        // if not exactly 0 it should display as at least 1
        if( tempworst > 0 ) {
            armor_data.worst_protection_chance = std::max( 1.0f, tempworst * 100.0f );
        } else {
            armor_data.worst_protection_chance = 0;
        }
        if( tempbest > 0 ) {
            armor_data.best_protection_chance = std::max( 1.0f, tempbest * 100.0f );
        } else {
            armor_data.best_protection_chance = 0;
        }
    }

    // create the vector of all layers
    if( obj.has_flag( flag_PERSONAL ) ) {
        obj.armor->all_layers.push_back( layer_level::PERSONAL );
    }
    if( obj.has_flag( flag_SKINTIGHT ) ) {
        obj.armor->all_layers.push_back( layer_level::SKINTIGHT );
    }
    if( obj.has_flag( flag_NORMAL ) ) {
        obj.armor->all_layers.push_back( layer_level::NORMAL );
    }
    if( obj.has_flag( flag_WAIST ) ) {
        obj.armor->all_layers.push_back( layer_level::WAIST );
    }
    if( obj.has_flag( flag_OUTER ) ) {
        obj.armor->all_layers.push_back( layer_level::OUTER );
    }
    if( obj.has_flag( flag_BELTED ) ) {
        obj.armor->all_layers.push_back( layer_level::BELTED );
    }
    if( obj.has_flag( flag_AURA ) ) {
        obj.armor->all_layers.push_back( layer_level::AURA );
    }
    // fallback for old way of doing items
    if( obj.armor->all_layers.empty() ) {
        obj.armor->all_layers.push_back( layer_level::NORMAL );
    }

    // generate the vector of flags that the item will default to if not override
    std::vector<layer_level> default_layers = obj.armor->all_layers;

    for( armor_portion_data &armor_data : obj.armor->data ) {
        // if an item or location has no layer data then default to the flags for the item
        if( armor_data.layers.empty() ) {
            for( const layer_level &ll : default_layers ) {
                armor_data.layers.insert( ll );
            }
        } else {
            armor_data.has_unique_layering = true;
            // add any unique layer entries to the items total layer info
            for( const layer_level &ll : armor_data.layers ) {
                if( std::count( obj.armor->all_layers.begin(), obj.armor->all_layers.end(), ll ) == 0 ) {
                    obj.armor->all_layers.push_back( ll );
                }
            }
        }
    }
    for( armor_portion_data &armor_data : obj.armor->sub_data ) {
        // if an item or location has no layer data then default to the flags for the item
        if( armor_data.layers.empty() ) {
            for( const layer_level &ll : default_layers ) {
                armor_data.layers.insert( ll );
            }
        } else {
            armor_data.has_unique_layering = true;
        }
    }

    // anything on a non traditional clothing layer can't be "rigid" as well since it's storage pouches and stuff
    for( armor_portion_data &armor_data : obj.armor->sub_data ) {
        auto clothing_layer = std::find_if( armor_data.layers.begin(),
        armor_data.layers.end(), []( const layer_level ll ) {
            return ll == layer_level::SKINTIGHT || ll == layer_level::NORMAL || ll == layer_level::OUTER;
        } );
        if( clothing_layer == armor_data.layers.end() ) {
            armor_data.rigid = false;
        }
    }

    bool all_rigid = true;
    bool all_comfortable = true;
    for( armor_portion_data &data : obj.armor->sub_data ) {
        // check if everything on this armor is still rigid / comfortable
        all_rigid = all_rigid && data.rigid;
        all_comfortable = all_comfortable && data.comfortable;
    }
    obj.armor->rigid = all_rigid;
    obj.armor->comfortable = all_comfortable;

    // go through the pockets and apply some characteristics
    for( const pocket_data &pocket : obj.pockets ) {
        if( pocket.ablative ) {
            obj.armor->ablative = true;
            break;
        }
        if( pocket.extra_encumbrance > 0 ) {
            obj.armor->additional_pocket_enc = true;
        }
        if( pocket.ripoff > 0 ) {
            obj.armor->ripoff_chance = true;
        }
        if( pocket.activity_noise.chance > 0 ) {
            obj.armor->noisy = true;
        }
    }

    // update the items materials based on the materials defined by the armor
    // this isn't perfect but neither is the usual definition so this should at
    // least give a good ballpark
    if( obj.materials.empty() ) {
        obj.mat_portion_total = 0;
        for( const armor_portion_data &armor_data : obj.armor->data ) {
            for( const part_material &mat : armor_data.materials ) {
                // if the material isn't in the map yet
                if( obj.materials.find( mat.id ) == obj.materials.end() ) {
                    obj.materials[mat.id] = mat.thickness * 100;
                } else {
                    obj.materials[mat.id] += mat.thickness * 100;
                }
                obj.mat_portion_total += mat.thickness * 100;
            }
        }
    }

    // calculate each body part breathability of the armor
    // breathability is the worst breathability of any material on that portion
    for( armor_portion_data &armor_data : obj.armor->data ) {
        // only recalculate the breathability when the value is not set in JSON
        // or when the value in JSON is invalid
        if( armor_data.breathability < 0 ) {

            std::vector<part_material> sorted_mats = armor_data.materials;
            std::sort( sorted_mats.begin(), sorted_mats.end(), []( const part_material & lhs,
            const part_material & rhs ) {
                return lhs.id->breathability() < rhs.id->breathability();
            } );

            // now that mats are sorted least breathable to most
            int coverage_counted = 0;
            int combined_breathability = 0;

            // only calcuate the breathability if the armor has no valid loaded value
            for( const part_material &mat : sorted_mats ) {
                // this isn't perfect since its impossible to know the positions of each material relatively
                // so some guessing is done
                // specifically count the worst breathability then then next best with additional coverage
                // and repeat until out of matts or fully covering.
                combined_breathability += std::max( ( mat.cover - coverage_counted ) * mat.id->breathability(), 0 );
                coverage_counted = std::max( mat.cover, coverage_counted );

                // this covers the whole piece of armor so we can stop counting better breathability
                if( coverage_counted == 100 ) {
                    break;
                }
            }
            // whatever isn't covered is as good as skin so 100%
            armor_data.breathability = ( combined_breathability / 100 ) + ( 100 - coverage_counted );
        }
    }

    for( const armor_portion_data &armor_data : obj.armor->data ) {
        for( const part_material &mat : armor_data.materials ) {
            if( mat.cover > 100 || mat.cover < 0 ) {
                debugmsg( "item %s has coverage %d for material %s.",
                          obj.id.str(), mat.cover, mat.id.str() );
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

    // for each item register all (non-obsolete) potential recipes
    for( const std::pair<const recipe_id, recipe> &p : recipe_dict ) {
        const recipe &rec = p.second;
        if( rec.obsolete || rec.will_be_blacklisted() ) {
            continue;
        }
        const itype_id &result = rec.result();
        auto it = m_templates.find( result );
        if( it != m_templates.end() ) {
            it->second.recipes.push_back( p.first );
        }
    }
}

void item_blacklist_t::clear()
{
    blacklist.clear();
    sub_blacklist.clear();
}

void Item_factory::finalize_item_blacklist()
{
    // Populate a whitelist, and a blacklist with items on whitelists and items on blacklists
    std::set<itype_id> whitelist;
    for( const std::pair<bool, std::set<itype_id>> &blacklist : item_blacklist.sub_blacklist ) {
        // True == whitelist, false == blacklist
        if( blacklist.first ) {
            whitelist.insert( blacklist.second.begin(), blacklist.second.end() );
        } else {
            item_blacklist.blacklist.insert( blacklist.second.begin(), blacklist.second.end() );
        }
    }

    bool whitelist_exists = !whitelist.empty();
    // Remove all blacklisted items on the whitelist
    std::set<itype_id> &blacklist = item_blacklist.blacklist;
    for( const itype_id &it : whitelist ) {
        if( blacklist.count( it ) ) {
            whitelist.erase( it );
        }
    }

    // Now, populate the blacklist with all the items that aren't whitelists, but only if a whitelist exists.
    if( whitelist_exists ) {
        blacklist.clear();
        for( const std::pair<const itype_id, itype> &item : m_templates ) {
            if( !whitelist.count( item.first ) ) {
                blacklist.insert( item.first );
            }
        }
    }

    // And clear the blacklists we made in-between
    item_blacklist.sub_blacklist.clear();

    for( const itype_id &blackout : item_blacklist.blacklist ) {
        std::unordered_map<itype_id, itype>::iterator candidate = m_templates.find( blackout );
        if( candidate == m_templates.end() ) {
            debugmsg( "item on blacklist %s does not exist", blackout.c_str() );
            continue;
        }

        for( std::pair<const item_group_id, std::unique_ptr<Item_spawn_data>> &g : m_template_groups ) {
            g.second->remove_item( candidate->first );
        }

        // remove any blacklisted items from requirements
        for( const std::pair<const requirement_id, requirement_data> &r : requirement_data::all() ) {
            const_cast<requirement_data &>( r.second ).blacklist_item( candidate->first );
        }

        // remove any recipes used to craft the blacklisted item
        recipe_dictionary::delete_if( [&candidate]( const recipe & r ) {
            return r.result() == candidate->first;
        } );
    }
    for( const vehicle_prototype &const_prototype : vehicles::get_all_prototypes() ) {
        vehicle_prototype &prototype = const_cast<vehicle_prototype &>( const_prototype );
        for( vehicle_item_spawn &vis : prototype.item_spawns ) {
            auto &vec = vis.item_ids;
            const auto iter = std::remove_if( vec.begin(), vec.end(), item_is_blacklisted );
            vec.erase( iter, vec.end() );
        }
    }

    // Construct a map for batch item replace
    std::unordered_map<itype_id, itype_id> replacements;
    for( const std::pair<const itype_id, std::vector<migration>> &migrate : migrations ) {
        // The valid migration entry
        const migration *valid = nullptr;
        for( const migration &cand : migrate.second ) {
            // This can only be applied to items, not itypes
            if( cand.from_variant ) {
                continue;
            }
            // There can only be one migration entry
            if( valid == nullptr ) {
                valid = &cand;
            } else {
                valid = nullptr;
                break;
            }
        }
        // Either there are no migrations, or too many migrations
        // Errors are reported below
        if( valid == nullptr ) {
            continue;
        }
        if( m_templates.count( valid->replace ) == 0 ) {
            // Errors for missing item templates will be reported below
            continue;
        }
        replacements[migrate.first] = valid->replace;
    }
    // Replace items in item template groups
    for( std::pair<const item_group_id, std::unique_ptr<Item_spawn_data>> &g : m_template_groups ) {
        g.second->replace_items( replacements );
    }
    // Replace templates in recipe/part/etc. requirements
    for( const std::pair<const requirement_id, requirement_data> &r : requirement_data::all() ) {
        const_cast<requirement_data &>( r.second ).replace_items( replacements );
    }

    for( const std::pair<const itype_id, std::vector<migration>> &migrate : migrations ) {
        const migration *parent = nullptr;
        for( const migration &migrant : migrate.second ) {
            if( m_templates.count( migrant.replace ) == 0 ) {
                debugmsg( "Replacement item (%s) for migration %s does not exist", migrant.replace.str(),
                          migrate.first.c_str() );
                continue;
            }
            // The rest of this only applies to blanket migrations
            // Not migrations looking for a particular variant
            if( migrant.from_variant ) {
                continue;
            }
            if( parent != nullptr ) {
                debugmsg( "Multiple non-variant migrations specified for %s", migrate.first.str() );
            }
            parent = &migrant;
        }
        // Only variant migrations exist, abort
        if( parent == nullptr ) {
            continue;
        }

        // remove any recipes used to craft the migrated item
        // if there's a valid recipe, it will be for the replacement
        recipe_dictionary::delete_if( [&migrate]( const recipe & r ) {
            return !r.obsolete && r.result() == migrate.first;
        } );

        // If the default ammo of an ammo_type gets migrated, we migrate all guns using that ammo
        // type to the ammo type of whatever that default ammo was migrated to.
        // To do that we need to store a map of ammo to the migration replacement thereof.
        auto maybe_ammo = m_templates.find( migrate.first );
        // If the itype_id is valid and the itype has ammo data
        if( maybe_ammo != m_templates.end() && maybe_ammo->second.ammo ) {
            auto replacement = m_templates.find( parent->replace );
            if( replacement->second.ammo ) {
                migrated_ammo.emplace( migrate.first, replacement->second.ammo->type );
            } else {
                debugmsg( "Replacement item %s for migrated ammo %s is not ammo.",
                          parent->replace.str(), migrate.first.str() );
            }
        }

        // migrate magazines as well
        auto maybe_mag = m_templates.find( migrate.first );
        if( maybe_mag != m_templates.end() && maybe_mag->second.magazine ) {
            auto replacement = m_templates.find( parent->replace );
            if( replacement->second.magazine ) {
                migrated_magazines.emplace( migrate.first, parent->replace );
            } else {
                debugmsg( "Replacement item %s for migrated magazine %s is not a magazine.",
                          parent->replace.str(), migrate.first.str() );
            }
        }
    }
    for( const vehicle_prototype &const_prototype : vehicles::get_all_prototypes() ) {
        vehicle_prototype &prototype = const_cast<vehicle_prototype &>( const_prototype );
        for( vehicle_item_spawn &vis : prototype.item_spawns ) {
            for( itype_id &type_to_spawn : vis.item_ids ) {
                std::map<itype_id, std::vector<migration>>::iterator replacement =
                        migrations.find( type_to_spawn );
                if( replacement == migrations.end() ) {
                    continue;
                }
                const migration *parent = nullptr;
                for( const migration &migrant : replacement->second ) {
                    if( m_templates.count( migrant.replace ) == 0 ) {
                        // Error reported above
                        continue;
                    }
                    // The rest of this only applies to blanket migrations
                    // Not migrations looking for a particular variant
                    if( migrant.from_variant ) {
                        continue;
                    }
                    if( parent != nullptr ) {
                        // Error reported above
                        parent = nullptr;
                        break;
                    }
                    parent = &migrant;
                }
                if( parent == nullptr ) {
                    continue;
                }
                type_to_spawn = parent->replace;
            }
        }
    }
}

void Item_factory::load_item_blacklist( const JsonObject &json )
{
    bool whitelist = json.get_bool( "whitelist" );
    std::set<itype_id> tmp_blacklist;
    json.read( "items", tmp_blacklist, true );
    item_blacklist.sub_blacklist.emplace_back( whitelist, tmp_blacklist );
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
        std::optional<int> use( Character *p, item &it, const tripoint &pos ) const override {
            return cpp_function( p, &it, pos );
        }
        std::unique_ptr<iuse_actor> clone() const override {
            return std::make_unique<iuse_function_wrapper>( *this );
        }

        void load( const JsonObject & ) override {}
};

class iuse_function_wrapper_with_info : public iuse_function_wrapper
{
    private:
        translation info_string;
    public:
        iuse_function_wrapper_with_info(
            const std::string &type, const use_function_pointer f, const translation &info )
            : iuse_function_wrapper( type, f ), info_string( info ) { }

        void info( const item &, std::vector<iteminfo> &info ) const override {
            info.emplace_back( "DESCRIPTION", info_string.translated() );
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
                             const translation &info )
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
    add_iuse( "AFS_TRANSLOCATOR", &iuse::afs_translocator );
    add_iuse( "ALCOHOL", &iuse::alcohol_medium );
    add_iuse( "ALCOHOL_STRONG", &iuse::alcohol_strong );
    add_iuse( "ALCOHOL_WEAK", &iuse::alcohol_weak );
    add_iuse( "ANTIBIOTIC", &iuse::antibiotic );
    add_iuse( "ANTICONVULSANT", &iuse::anticonvulsant );
    add_iuse( "ANTIFUNGAL", &iuse::antifungal );
    add_iuse( "ANTIPARASITIC", &iuse::antiparasitic );
    add_iuse( "BELL", &iuse::bell );
    add_iuse( "BLECH", &iuse::blech );
    add_iuse( "BLECH_BECAUSE_UNCLEAN", &iuse::blech_because_unclean );
    add_iuse( "BOLTCUTTERS", &iuse::boltcutters );
    add_iuse( "C4", &iuse::c4 );
    add_iuse( "CAMERA", &iuse::camera );
    add_iuse( "CAN_GOO", &iuse::can_goo );
    add_iuse( "COIN_FLIP", &iuse::coin_flip );
    add_iuse( "DIRECTIONAL_HOLOGRAM", &iuse::directional_hologram );
    add_iuse( "CAPTURE_MONSTER_ACT", &iuse::capture_monster_act );
    add_iuse( "CAPTURE_MONSTER_VEH", &iuse::capture_monster_veh );
    add_iuse( "CARVER_OFF", &iuse::carver_off );
    add_iuse( "CARVER_ON", &iuse::carver_on );
    add_iuse( "CHAINSAW_OFF", &iuse::chainsaw_off );
    add_iuse( "CHAINSAW_ON", &iuse::chainsaw_on );
    add_iuse( "CHEW", &iuse::chew );
    add_iuse( "RPGDIE", &iuse::rpgdie );
    add_iuse( "CHANGE_EYES", &iuse::change_eyes );
    add_iuse( "CHANGE_SKIN", &iuse::change_skin );
    add_iuse( "CHOP_TREE", &iuse::chop_tree );
    add_iuse( "CHOP_LOGS", &iuse::chop_logs );
    add_iuse( "CIRCSAW_ON", &iuse::circsaw_on );
    add_iuse( "ELECTRIC_CIRCSAW_ON", &iuse::e_circsaw_on );
    add_iuse( "CLEAR_RUBBLE", &iuse::clear_rubble );
    add_iuse( "COKE", &iuse::coke );
    add_iuse( "COMBATSAW_OFF", &iuse::combatsaw_off );
    add_iuse( "COMBATSAW_ON", &iuse::combatsaw_on );
    add_iuse( "TOOLWEAPON_DEACTIVATE", &iuse::toolweapon_deactivate );
    add_iuse( "E_COMBATSAW_OFF", &iuse::e_combatsaw_off );
    add_iuse( "E_COMBATSAW_ON", &iuse::e_combatsaw_on );
    add_iuse( "CONTACTS", &iuse::contacts );
    add_iuse( "CROWBAR", &iuse::crowbar );
    add_iuse( "CROWBAR_WEAK", &iuse::crowbar_weak );
    add_iuse( "DATURA", &iuse::datura );
    add_iuse( "DIG", &iuse::dig );
    add_iuse( "DIVE_TANK", &iuse::dive_tank );
    add_iuse( "DIVE_TANK_ACTIVATE", &iuse::dive_tank_activate );
    add_iuse( "DIRECTIONAL_ANTENNA", &iuse::directional_antenna );
    add_iuse( "DISASSEMBLE", &iuse::disassemble );
    add_iuse( "CRAFT", &iuse::craft );
    add_iuse( "DOG_WHISTLE", &iuse::dog_whistle );
    add_iuse( "DOLLCHAT", &iuse::talking_doll );
    add_iuse( "ECIG", &iuse::ecig );
    add_iuse( "EHANDCUFFS", &iuse::ehandcuffs );
    add_iuse( "EHANDCUFFS_TICK", &iuse::ehandcuffs_tick );
    add_iuse( "EPIC_MUSIC", &iuse::epic_music );
    add_iuse( "EINKTABLETPC", &iuse::einktabletpc );
    add_iuse( "ELECTRICSTORAGE", &iuse::electricstorage );
    add_iuse( "EBOOKSAVE", &iuse::ebooksave );
    add_iuse( "EBOOKREAD", &iuse::ebookread );
    add_iuse( "ELEC_CHAINSAW_OFF", &iuse::elec_chainsaw_off );
    add_iuse( "ELEC_CHAINSAW_ON", &iuse::elec_chainsaw_on );
    add_iuse( "EMF_PASSIVE_ON", &iuse::emf_passive_on );
    add_iuse( "EXTINGUISHER", &iuse::extinguisher );
    add_iuse( "EYEDROPS", &iuse::eyedrops );
    add_iuse( "FILL_PIT", &iuse::fill_pit );
    add_iuse( "FIRECRACKER", &iuse::firecracker );
    add_iuse( "FIRECRACKER_PACK", &iuse::firecracker_pack );
    add_iuse( "FIRECRACKER_PACK_ACT", &iuse::firecracker_pack_act );
    add_iuse( "FISH_ROD", &iuse::fishing_rod );
    add_iuse( "FISH_TRAP", &iuse::fish_trap );
    add_iuse( "FISH_TRAP_TICK", &iuse::fish_trap_tick );
    add_iuse( "FITNESS_CHECK", &iuse::fitness_check );
    add_iuse( "FLUMED", &iuse::flumed );
    add_iuse( "FLUSLEEP", &iuse::flusleep );
    add_iuse( "FLU_VACCINE", &iuse::flu_vaccine );
    add_iuse( "FOODPERSON", &iuse::foodperson );
    add_iuse( "FOODPERSON_VOICE", &iuse::foodperson_voice );
    add_iuse( "FUNGICIDE", &iuse::fungicide );
    add_iuse( "GASMASK_ACTIVATE", &iuse::gasmask_activate,
              to_translation( "Can be activated to <good>increase environmental "
                              "protection</good>.  Will consume charges when active, "
                              "but <info>only when environmental hazards are "
                              "present</info>."
                            ) );
    add_iuse( "GASMASK", &iuse::gasmask );
    add_iuse( "GEIGER", &iuse::geiger );
    add_iuse( "GEIGER_ACTIVE", &iuse::geiger_active );
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
    add_iuse( "HOTPLATE_ATOMIC", &iuse::hotplate_atomic );
    add_iuse( "INHALER", &iuse::inhaler );
    add_iuse( "JACKHAMMER", &iuse::jackhammer );
    add_iuse( "JET_INJECTOR", &iuse::jet_injector );
    add_iuse( "LUMBER", &iuse::lumber );
    add_iuse( "MACE", &iuse::mace );
    add_iuse( "MAGIC_8_BALL", &iuse::magic_8_ball );
    add_iuse( "PLAY_GAME", &iuse::play_game );
    add_iuse( "MAKEMOUND", &iuse::makemound );
    add_iuse( "DIG_CHANNEL", &iuse::dig_channel );
    add_iuse( "MARLOSS", &iuse::marloss );
    add_iuse( "MARLOSS_GEL", &iuse::marloss_gel );
    add_iuse( "MARLOSS_SEED", &iuse::marloss_seed );
    add_iuse( "MA_MANUAL", &iuse::ma_manual );
    add_iuse( "MANAGE_EXOSUIT", &iuse::manage_exosuit );
    add_iuse( "MEDITATE", &iuse::meditate );
    add_iuse( "METH", &iuse::meth );
    add_iuse( "MININUKE", &iuse::mininuke );
    add_iuse( "MOLOTOV_LIT", &iuse::molotov_lit );
    add_iuse( "MOP", &iuse::mop );
    add_iuse( "MP3", &iuse::mp3 );
    add_iuse( "MP3_ON", &iuse::mp3_on );
    add_iuse( "MP3_DEACTIVATE", &iuse::mp3_deactivate );
    add_iuse( "MULTICOOKER", &iuse::multicooker );
    add_iuse( "MULTICOOKER_TICK", &iuse::multicooker_tick );
    add_iuse( "MYCUS", &iuse::mycus );
    add_iuse( "NOISE_EMITTER_ON", &iuse::noise_emitter_on );
    add_iuse( "OXYGEN_BOTTLE", &iuse::oxygen_bottle );
    add_iuse( "OXYTORCH", &iuse::oxytorch );
    add_iuse( "PACK_CBM", &iuse::pack_cbm );
    add_iuse( "PACK_ITEM", &iuse::pack_item );
    add_iuse( "PETFOOD", &iuse::petfood );
    add_iuse( "PICK_LOCK", &iuse::pick_lock );
    add_iuse( "PICKAXE", &iuse::pickaxe );
    add_iuse( "PLANTBLECH", &iuse::plantblech );
    add_iuse( "POISON", &iuse::poison );
    add_iuse( "PORTABLE_GAME", &iuse::portable_game );
    add_iuse( "PORTAL", &iuse::portal );
    add_iuse( "PROZAC", &iuse::prozac );
    add_iuse( "PURIFY_SMART", &iuse::purify_smart );
    add_iuse( "RADGLOVE", &iuse::radglove );
    add_iuse( "RADIOCAR", &iuse::radiocar );
    add_iuse( "RADIOCARON", &iuse::radiocaron );
    add_iuse( "RADIOCONTROL", &iuse::radiocontrol );
    add_iuse( "RADIOCONTROL_TICK", &iuse::radiocontrol_tick );
    add_iuse( "RADIO_MOD", &iuse::radio_mod );
    add_iuse( "RADIO_OFF", &iuse::radio_off );
    add_iuse( "RADIO_ON", &iuse::radio_on );
    add_iuse( "RADIO_TICK", &iuse::radio_tick );
    add_iuse( "BINDER_ADD_RECIPE", &iuse::binder_add_recipe );
    add_iuse( "BINDER_MANAGE_RECIPE", &iuse::binder_manage_recipe );
    add_iuse( "REMOTEVEH", &iuse::remoteveh );
    add_iuse( "REMOTEVEH_TICK", &iuse::remoteveh_tick );
    add_iuse( "REMOVE_ALL_MODS", &iuse::remove_all_mods );
    add_iuse( "RM13ARMOR_OFF", &iuse::rm13armor_off );
    add_iuse( "RM13ARMOR_ON", &iuse::rm13armor_on );
    add_iuse( "ROBOTCONTROL", &iuse::robotcontrol );
    add_iuse( "SEED", &iuse::seed );
    add_iuse( "SEWAGE", &iuse::sewage );
    add_iuse( "SHAVEKIT", &iuse::shavekit );
    add_iuse( "SHOCKTONFA_OFF", &iuse::shocktonfa_off );
    add_iuse( "SHOCKTONFA_ON", &iuse::shocktonfa_on );
    add_iuse( "SIPHON", &iuse::siphon );
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
    add_iuse( "TOWEL", &iuse::towel );
    add_iuse( "TRIMMER_OFF", &iuse::trimmer_off );
    add_iuse( "TRIMMER_ON", &iuse::trimmer_on );
    add_iuse( "UNFOLD_GENERIC", &iuse::unfold_generic );
    add_iuse( "UNPACK_ITEM", &iuse::unpack_item );
    add_iuse( "CALL_OF_TINDALOS", &iuse::call_of_tindalos );
    add_iuse( "BLOOD_DRAW", &iuse::blood_draw );
    add_iuse( "VIBE", &iuse::vibe );
    add_iuse( "HAND_CRANK", &iuse::hand_crank );
    add_iuse( "VORTEX", &iuse::vortex );
    add_iuse( "WASH_SOFT_ITEMS", &iuse::wash_soft_items );
    add_iuse( "WASH_HARD_ITEMS", &iuse::wash_hard_items );
    add_iuse( "WASH_ALL_ITEMS", &iuse::wash_all_items );
    add_iuse( "WATER_PURIFIER", &iuse::water_purifier );
    add_iuse( "WEAK_ANTIBIOTIC", &iuse::weak_antibiotic );
    add_iuse( "WEATHER_TOOL", &iuse::weather_tool );
    add_iuse( "SEXTANT", &iuse::sextant );
    add_iuse( "WEED_CAKE", &iuse::weed_cake );
    add_iuse( "XANAX", &iuse::xanax );
    add_iuse( "BREAK_STICK", &iuse::break_stick );
    add_iuse( "LUX_METER", &iuse::lux_meter );
    add_iuse( "DBG_LUX_METER", &iuse::dbg_lux_meter );
    add_iuse( "CALORIES_INTAKE_TRACKER", &iuse::calories_intake_tracker );
    add_iuse( "VOLTMETER", &iuse::voltmeter );

    add_actor( std::make_unique<ammobelt_actor>() );
    add_actor( std::make_unique<consume_drug_iuse>() );
    add_actor( std::make_unique<delayed_transform_iuse>() );
    add_actor( std::make_unique<explosion_iuse>() );
    add_actor( std::make_unique<firestarter_actor>() );
    add_actor( std::make_unique<fireweapon_off_actor>() );
    add_actor( std::make_unique<fireweapon_on_actor>() );
    add_actor( std::make_unique<heal_actor>() );
    add_actor( std::make_unique<holster_actor>() );
    add_actor( std::make_unique<inscribe_actor>() );
    add_actor( std::make_unique<iuse_transform>() );
    add_actor( std::make_unique<unpack_actor>() );
    add_actor( std::make_unique<message_iuse>() );
    add_actor( std::make_unique<sound_iuse>() );
    add_actor( std::make_unique<play_instrument_iuse>() );
    add_actor( std::make_unique<manualnoise_actor>() );
    add_actor( std::make_unique<musical_instrument_actor>() );
    add_actor( std::make_unique<deploy_furn_actor>() );
    add_actor( std::make_unique<place_monster_iuse>() );
    add_actor( std::make_unique<change_scent_iuse>() );
    add_actor( std::make_unique<place_npc_iuse>() );
    add_actor( std::make_unique<reveal_map_actor>() );
    add_actor( std::make_unique<salvage_actor>() );
    add_actor( std::make_unique<place_trap_actor>() );
    add_actor( std::make_unique<emit_actor>() );
    add_actor( std::make_unique<saw_barrel_actor>() );
    add_actor( std::make_unique<saw_stock_actor>() );
    add_actor( std::make_unique<molle_attach_actor>() );
    add_actor( std::make_unique<molle_detach_actor>() );
    add_actor( std::make_unique<install_bionic_actor>() );
    add_actor( std::make_unique<detach_gunmods_actor>() );
    add_actor( std::make_unique<modify_gunmods_actor>() );
    add_actor( std::make_unique<link_up_actor>() );
    add_actor( std::make_unique<deploy_tent_actor>() );
    add_actor( std::make_unique<learn_spell_actor>() );
    add_actor( std::make_unique<cast_spell_actor>() );
    add_actor( std::make_unique<weigh_self_actor>() );
    add_actor( std::make_unique<sew_advanced_actor>() );
    add_actor( std::make_unique<effect_on_conditons_actor>() );
    // An empty dummy group, it will not spawn anything. However, it makes that item group
    // id valid, so it can be used all over the place without need to explicitly check for it.
    m_template_groups[Item_spawn_data_EMPTY_GROUP] =
        std::make_unique<Item_group>( Item_group::G_COLLECTION, 100, 0, 0, "EMPTY_GROUP" );
}

bool Item_factory::check_ammo_type( std::string &msg, const ammotype &ammo ) const
{
    if( ammo.is_null() ) {
        return false;
    }

    if( !ammo.is_valid() ) {
        msg += string_format( "ammo type %s is not known\n", ammo.c_str() );
        return false;
    }

    if( std::none_of( m_templates.begin(),
    m_templates.end(), [&ammo]( const decltype( m_templates )::value_type & e ) {
    return e.second.ammo && e.second.ammo->type == ammo;
} ) ) {
        msg += string_format( "there is no actual ammo of type %s defined\n", ammo.c_str() );
        return false;
    }
    return true;
}

void Item_factory::check_definitions() const
{
    auto is_container = []( const itype * t ) {
        bool am_container = false;
        for( const pocket_data &pocket : t->pockets ) {
            if( pocket.type == item_pocket::pocket_type::CONTAINER ) {
                am_container = true;
                // no need to look further
                break;
            }
        }
        return am_container;
    };

    for( const auto &elem : m_templates ) {
        std::string msg;
        const itype *type = &elem.second;

        if( !type->has_flag( flag_TARDIS ) ) {
            if( is_container( type ) ) {
                units::volume volume = type->volume;
                if( type->count_by_charges() ) {
                    volume /= type->charges_default();
                }
                if( item_contents( type->pockets ).bigger_on_the_inside( volume ) ) {
                    msg += "is bigger on the inside.  consider using TARDIS flag.\n";
                }
            }
        }

        if( !type->picture_id.is_empty() && !type->picture_id.is_valid() ) {
            msg +=  "item has unknown ascii_picture.";
        }

        int mag_pocket_number = 0;
        for( const pocket_data &data : type->pockets ) {
            if( data.type == item_pocket::pocket_type::MAGAZINE ||
                data.type == item_pocket::pocket_type::MAGAZINE_WELL ) {
                mag_pocket_number++;
            }
            std::string pocket_error = data.check_definition();
            if( !pocket_error.empty() ) {
                msg += "problem with pocket: " + pocket_error;
            }
        }
        if( mag_pocket_number > 1 ) {
            msg += "cannot have more than one pocket that handles ammo (MAGAZINE or MAGAZINE_WELL)\n";
        }

        if( !type->category_force.is_valid() ) {
            msg += "undefined category " + type->category_force.str() + "\n";
        }

        if( type->armor ) {
            cata::flat_set<bodypart_str_id> observed_bps;
            for( const armor_portion_data &portion : type->armor->data ) {
                if( portion.covers.has_value() ) {
                    for( const bodypart_str_id &bp : *portion.covers ) {
                        if( portion.covers->test( bp ) ) {
                            if( observed_bps.count( bp ) ) {
                                msg += string_format(
                                           "multiple portions with same body_part %s defined\n",
                                           bp.str() );
                            }
                            observed_bps.insert( bp );
                        }
                    }
                }
                if( portion.coverage == 0 && ( portion.cover_melee > 0 || portion.cover_ranged > 0 ) ) {
                    msg += "base \"coverage\" value not specified in armor portion despite using \"cover_melee\"/\"cover_ranged\"\n";
                }
            }

            // do tests for the sub bp armor data arrays
            std::vector<sub_bodypart_str_id> observed_sub_bps;
            for( const armor_portion_data &portion : type->armor->sub_data ) {
                for( const sub_bodypart_str_id &bp : portion.sub_coverage ) {
                    if( std::find( observed_sub_bps.begin(), observed_sub_bps.end(), bp ) != observed_sub_bps.end() ) {
                        msg += string_format(
                                   "multiple portions with same sub_body_part %s defined\n",
                                   bp.str() );
                    }
                    observed_sub_bps.push_back( bp );
                }
                if( portion.coverage == 0 && ( portion.cover_melee > 0 || portion.cover_ranged > 0 ) ) {
                    msg += "base \"coverage\" value not specified in armor portion despite using \"cover_melee\"/\"cover_ranged\"\n";
                }
            }

            // check the hanging location aren't being used on the non strapped layer
            if( std::find( type->armor->all_layers.begin(), type->armor->all_layers.end(),
                           layer_level::BELTED ) == type->armor->all_layers.end() ) {
                for( const armor_portion_data &portion : type->armor->sub_data ) {
                    for( const sub_bodypart_str_id &sbp : portion.sub_coverage ) {
                        if( sbp->secondary ) {
                            msg += string_format( "Secondary hanging locations should only be used on the BELTED layer: %s\n",
                                                  sbp.str() );
                        }
                    }
                }
            }

            // waist is deprecated
            if( type->has_flag( flag_WAIST ) ) {
                msg += string_format( "Waist has been deprecated as an armor layer and is now a sublocation of the torso on the BELTED layer.  If you are making new content make it BELTED specifically covering the torso_waist.  If you are loading an old mod you are probably safe to ignore this.\n" );
            }

            // check that no item has more coverage on any location than the max coverage (100)
            for( const armor_portion_data &portion : type->armor->sub_data ) {
                if( 100 < portion.coverage || 100 < portion.cover_melee ||
                    100 < portion.cover_ranged ) {
                    msg += string_format( "coverage exceeds the maximum amount for the sub locations coverage can't exceed 100, item coverage: %d\n",
                                          portion.coverage );
                }
            }
            // check that no item has more coverage on any location than the max coverage (100)
            for( const armor_portion_data &portion : type->armor->data ) {
                if( 100 < portion.coverage || 100 < portion.cover_melee ||
                    100 < portion.cover_ranged ) {
                    msg += string_format( "coverage exceeds the maximum amount for the locations coverage can't exceed 100, item coverage: %d\n",
                                          portion.coverage );
                }
            }
            // check that all materials are between 0-100 proportional coverage
            for( const armor_portion_data &portion : type->armor->sub_data ) {
                for( const part_material &mat : portion.materials ) {
                    if( mat.cover < 0 || mat.cover > 100 ) {
                        msg += string_format( "material %s has proportional coverage %d.  proportional coverage is a value between 0-100.",
                                              mat.id.str(), mat.cover );
                    }
                }
            }

            // check that at least 1 material has 100% coverage on each item
            for( const armor_portion_data &portion : type->armor->sub_data ) {
                // if the advanced materials entry is empty skip this stuff
                if( !portion.materials.empty() ) {
                    bool found = false;
                    for( const part_material &mat : portion.materials ) {
                        if( mat.cover == 100 ) {
                            found = true;
                        }
                    }

                    if( !found ) {
                        msg += string_format( "at least 1 material for any armor entry should have proportional coverage 100.  Consider dropping overall coverage to compensate." );
                    }
                }
            }
            // check each original definition had a valid material thickness
            // valid thickness is a multiple of sheet thickness
            for( const armor_portion_data &portion : type->armor->sub_data ) {
                for( const part_material &mat : portion.materials ) {
                    if( !mat.ignore_sheet_thickness && !mat.id->is_valid_thickness( mat.thickness ) ) {
                        msg += string_format( "for material %s, %f isn't a valid multiple of the thickness the underlying material comes in: %f.\n",
                                              mat.id.str(), mat.thickness, mat.id->thickness_multiple() );
                    }
                }
            }
        }

        if( !type->can_have_charges() && type->charges_default() > 0 ) {
            msg += "charges defined but can not have any\n";
        } else if( type->comestible && type->can_have_charges() && type->charges_default() <= 0 ) {
            msg += "charges expected but not defined or <= 0\n";
        }

        if( type->weight < 0_gram ) {
            msg += "negative weight\n";
        }
        if( type->weight == 0_gram && is_physical( *type ) ) {
            msg += "zero weight (if zero weight is intentional you can "
                   "suppress this error with the ZERO_WEIGHT flag)\n";
        }
        if( type->volume < 0_ml ) {
            msg += "negative volume\n";
        }
        if( type->stack_size <= 0 ) {
            if( type->count_by_charges() ) {
                msg += string_format( "invalid stack_size %d on type using charges\n", type->stack_size );
            } else if( type->phase == phase_id::LIQUID ) {
                msg += string_format( "invalid stack_size %d on liquid type\n", type->stack_size );
            }
        }
        if( type->price < 0_cent ) {
            msg += "negative price\n";
        }
        if( type->description.empty() ) {
            msg += "empty description\n";
        }

        for( const std::pair<const material_id, int> &mat_id : type->materials ) {
            if( mat_id.first.str() == "null" || !mat_id.first.is_valid() ) {
                msg += string_format( "invalid material %s\n", mat_id.first.c_str() );
            }
        }

        if( type->sym.empty() ) {
            msg += "symbol not defined\n";
        } else if( utf8_width( type->sym ) != 1 ) {
            msg += "symbol must be exactly one console cell width\n";
        }

        for( const auto &_a : type->techniques ) {
            if( !_a.is_valid() ) {
                msg += string_format( "unknown technique %s\n", _a.c_str() );
            }
        }
        if( !type->snippet_category.empty() ) {
            if( !SNIPPET.has_category( type->snippet_category ) ) {
                msg += string_format( "item %s: snippet category %s without any snippets\n", type->id.c_str(),
                                      type->snippet_category.c_str() );
            }
        }
        for( const auto &q : type->qualities ) {
            if( !q.first.is_valid() ) {
                msg += string_format( "item %s has unknown quality %s\n", type->id.c_str(),
                                      q.first.c_str() );
            }
        }
        if( type->default_container && !type->default_container->is_null() ) {
            if( has_template( *type->default_container ) ) {
                // Spawn the item in its default container to generate an error
                // if it doesn't fit.
                item( type ).in_its_container();
            } else {
                msg += string_format( "invalid container property %s\n",
                                      type->default_container->c_str() );
            }
        }

        for( const auto &e : type->emits ) {
            if( !e.is_valid() ) {
                msg += string_format( "item %s has unknown emit source %s\n", type->id.c_str(), e.c_str() );
            }
        }

        for( const auto &f : type->faults ) {
            if( !f.is_valid() ) {
                msg += string_format( "invalid item fault %s\n", f.c_str() );
            }
        }

        if( type->comestible ) {
            if( !type->comestible->tool.is_null() ) {
                const itype *req_tool = find_template( type->comestible->tool );
                if( !req_tool->tool ) {
                    msg += string_format( "invalid tool property %s\n", type->comestible->tool.c_str() );
                }
            }

            // TODO: Remove invalid, just make those items not comestibles
            static const std::set<std::string> allowed_ctypes = { "FOOD", "DRINK", "MED", "INVALID" };
            if( allowed_ctypes.count( type->comestible->comesttype ) == 0 ) {
                msg += string_format( "Invalid comestible type %s\n", type->comestible->comesttype );
            }
        }
        if( type->brewable ) {
            if( type->brewable->time < 1_turns ) {
                msg += "brewable time is less than 1 turn\n";
            }

            if( type->brewable->results.empty() ) {
                msg += "empty product list\n";
            }

            for( auto &b : type->brewable->results ) {
                if( !has_template( b ) ) {
                    msg += string_format( "invalid result id %s\n", b.c_str() );
                }
            }
        }
        if( type->seed ) {
            if( type->seed->grow < 1_turns ) {
                msg += "seed growing time is less than 1 turn\n";
            }
            if( !has_template( type->seed->fruit_id ) ) {
                msg += string_format( "invalid fruit id %s\n", type->seed->fruit_id.c_str() );
            }
            for( auto &b : type->seed->byproducts ) {
                if( !has_template( b ) ) {
                    msg += string_format( "invalid byproduct id %s\n", b.c_str() );
                }
            }
        }
        if( type->book ) {
            if( type->book->skill && !type->book->skill.is_valid() ) {
                msg += "uses invalid book skill.\n";
            }
            if( type->book->martial_art && !type->book->martial_art.is_valid() ) {
                msg += string_format( "trains invalid martial art '%s'.\n", type->book->martial_art.str() );
            }
            if( type->can_use( "MA_MANUAL" ) && !type->book->martial_art ) {
                msg += "has use_action MA_MANUAL but does not specify a martial art\n";
            }
        }
        if( type->can_use( "MA_MANUAL" ) && !type->book ) {
            msg += "has use_action MA_MANUAL but is not a book\n";
        }
        if( type->milling_data ) {
            if( !has_template( type->milling_data->into_ ) ) {
                msg += "type to mill into is invalid: " + type->milling_data->into_.str() + "\n";
            }
        }
        if( type->ammo ) {
            if( !type->ammo->type && type->ammo->type != ammo_NULL ) {
                msg += "must define at least one ammo type\n";
            }
            check_ammo_type( msg, type->ammo->type );
            if( type->ammo->casing && ( !has_template( *type->ammo->casing ) ||
                                        type->ammo->casing->is_null() ) ) {
                msg += string_format( "invalid casing property %s\n", type->ammo->casing->c_str() );
            }
            if( !type->ammo->drop.is_null() && !has_template( type->ammo->drop ) ) {
                msg += string_format( "invalid drop item %s\n", type->ammo->drop.c_str() );
            }
            if( type->ammo->shot_damage.empty() && type->ammo->count != 1 ) {
                msg += string_format( "invalid shot definition, shot count with no shot damage." );
            }
            if( !type->ammo->shot_damage.empty() && type->ammo->count == 1 ) {
                msg += string_format( "invalid shot definition, shot damage with no shot count." );
            }
        }
        if( type->battery ) {
            if( type->battery->max_capacity < 0_mJ ) {
                msg += "battery cannot have negative maximum charge\n";
            }
        }
        if( type->gun ) {
            for( const ammotype &at : type->gun->ammo ) {
                check_ammo_type( msg, at );
            }
            if( type->gun->ammo.empty() ) {
                // if gun doesn't use ammo forbid both integral or detachable magazines
                if( static_cast<bool>( type->gun->clip ) || !type->magazines.empty() ) {
                    msg += "cannot specify clip_size or magazine without ammo type\n";
                }

                if( type->has_flag( flag_RELOAD_AND_SHOOT ) ) {
                    msg += "RELOAD_AND_SHOOT requires an ammo type to be specified\n";
                }

            } else {
                if( type->has_flag( flag_RELOAD_AND_SHOOT ) && !type->magazines.empty() ) {
                    msg += "RELOAD_AND_SHOOT cannot be used with magazines\n";
                }
                for( const ammotype &at : type->gun->ammo ) {
                    if( !type->gun->clip && !type->magazines.empty() && !type->magazine_default.count( at ) ) {
                        msg += string_format( "specified magazine but none provided for ammo type %s\n", at.str() );
                    }
                }
            }
            if( type->gun->barrel_volume < 0_ml ) {
                msg += "gun barrel volume cannot be negative\n";
            }

            if( !type->gun->skill_used ) {
                msg += "uses no skill\n";
            } else if( !type->gun->skill_used.is_valid() ) {
                msg += string_format( "uses an invalid skill %s\n", type->gun->skill_used.str() );
            }
            for( const itype_id &gm : type->gun->default_mods ) {
                if( !has_template( gm ) ) {
                    msg += "invalid default mod.\n";
                }
            }
            for( const itype_id &gm : type->gun->built_in_mods ) {
                if( !has_template( gm ) ) {
                    msg += "invalid built-in mod.\n";
                }
            }
        }
        if( type->gunmod ) {
            if( type->gunmod->location.str().empty() ) {
                msg += "gunmod does not specify location\n";
            }
            if( type->gunmod->sight_dispersion >= 0 ) {
                if( type->gunmod->field_of_view <= 0 && type->gunmod->aim_speed < 0 ) {
                    msg += "gunmod must have both sight_dispersion and field_of_view set or neither of them set\n";
                } else if( type->gunmod->aim_speed > 0 ) {
                    msg += "Aim speed will be converted to FoV and aim_speed_modifier automatically, if FoV is not set.\n";
                }
            } else if( type->gunmod->sight_dispersion < 0 && type->gunmod->field_of_view > 0 ) {
                msg += "gunmod must have both sight_dispersion and field_of_view set or neither of them set\n";
            }
            if( type->gunmod->usable.empty() ) {
                msg += "gunmod does not specify mod targets\n";
            }
            if( type->gunmod->install_time < 0 ) {
                msg += "gunmod does not specify install time\n";
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
                    msg += string_format( "no magazines specified for ammo type %s\n", e.first.str() );
                }
                for( const itype_id &opt : e.second ) {
                    const itype *mag = find_template( opt );
                    if( !mag->magazine || !mag->magazine->type.count( e.first ) ) {
                        msg += string_format( "invalid magazine %s in magazine adapter\n", opt.str() );
                    }
                }
            }
        }
        if( type->magazine ) {
            for( const ammotype &at : type->magazine->type ) {
                check_ammo_type( msg, at );
            }
            if( type->magazine->type.empty() ) {
                msg += "magazine did not specify ammo type\n";
            }
            if( type->magazine->capacity < 0 ) {
                msg += string_format( "invalid capacity %i\n", type->magazine->capacity );
            }
            if( type->magazine->count < 0 || type->magazine->count > type->magazine->capacity ) {
                msg += string_format( "invalid count %i\n", type->magazine->count );
            }
            const itype_id &default_ammo = type->magazine->default_ammo;
            const itype *da = find_template( default_ammo );
            if( da->ammo && type->magazine->type.count( da->ammo->type ) ) {
                if( !migrations.count( type->id ) && !item_is_blacklisted( type->id ) ) {
                    // Verify that the default amnmo can actually be put in this
                    // item
                    item( type ).ammo_set( default_ammo, 1 );
                }
            } else {
                msg += string_format( "invalid default_ammo %s\n", type->magazine->default_ammo.str() );
            }
            if( type->magazine->reload_time < 0 ) {
                msg += string_format( "invalid reload_time %i\n", type->magazine->reload_time );
            }
            if( type->magazine->linkage && ( !has_template( *type->magazine->linkage ) ||
                                             type->magazine->linkage->is_null() ) ) {
                msg += string_format( "invalid linkage property %s\n", type->magazine->linkage->c_str() );
            }
        }

        for( const std::pair<const string_id<ammunition_type>, std::set<itype_id>> &ammo_variety :
             type->magazines ) {
            if( ammo_variety.second.empty() ) {
                msg += string_format( "no magazine specified for %s\n", ammo_variety.first.str() );
            }
            for( const itype_id &magazine : ammo_variety.second ) {
                const itype *mag_ptr = find_template( magazine );
                if( mag_ptr == nullptr ) {
                    msg += string_format( "magazine \"%s\" specified for \"%s\" does not exist\n",
                                          magazine.str(), ammo_variety.first.str() );
                } else if( !mag_ptr->magazine ) {
                    msg += string_format(
                               "magazine \"%s\" specified for \"%s\" is not a magazine\n", magazine.str(),
                               ammo_variety.first.str() );
                } else if( !mag_ptr->magazine->type.count( ammo_variety.first ) ) {
                    msg += string_format( "magazine \"%s\" does not take compatible ammo\n",
                                          magazine.str() );
                } else if( mag_ptr->has_flag( flag_SPEEDLOADER ) &&
                           mag_ptr->magazine->capacity != type->gun->clip ) {
                    msg += string_format(
                               "speedloader %s capacity (%d) does not match gun capacity (%d).\n",
                               magazine.str(), mag_ptr->magazine->capacity, type->gun->clip );
                } else {
                    // Verify that every magazine type can actually be put in
                    // this item
                    item( type ).put_in( item( magazine ), item_pocket::pocket_type::MAGAZINE_WELL );
                }
            }
        }

        if( type->tool ) {
            for( const ammotype &at : type->tool->ammo_id ) {
                check_ammo_type( msg, at );
            }
            if( !type->tool->revert_msg.empty() && !type->revert_to ) {
                msg += "cannot specify revert_msg without revert_to\n";
            }
            if( !type->tool->subtype.is_empty() && !has_template( type->tool->subtype ) ) {
                msg += string_format( "invalid tool subtype %s\n", type->tool->subtype.str() );
            }
        }
        if( type->bionic ) {
            if( !type->bionic->id.is_valid() ) {
                msg += string_format( "there is no bionic with id %s\n", type->bionic->id.c_str() );
            }
        }

        for( const auto &elem : type->use_methods ) {
            const iuse_actor *actor = elem.second.get_actor_ptr();

            cata_assert( actor );
            if( !actor->is_valid() ) {
                msg += string_format( "item action \"%s\" was not described.\n", actor->type.c_str() );
            }
        }

        if( !migrations.count( type->id ) && !item_is_blacklisted( type->id ) ) {
            // If type has a default ammo then check it can fit within
            item tmp_item( type );
            if( tmp_item.is_gun() || tmp_item.is_magazine() ) {
                if( itype_id ammo_id = tmp_item.ammo_default() ) {
                    tmp_item.ammo_set( ammo_id, 1 );
                }
            }
        }

        if( type->revert_to && ( !has_template( *type->revert_to ) ||
                                 type->revert_to->is_null() ) ) {
            msg += string_format( "invalid revert_to property %s\n does not exist", type->revert_to->c_str() );
        }

        if( msg.empty() ) {
            continue;
        }
        debugmsg( "warnings for type %s:\n%s", type->id.c_str(), msg );
    }
    for( const auto &e : migrations ) {
        for( const migration &m : e.second ) {
            if( !m_templates.count( m.replace ) ) {
                debugmsg( "Invalid migration target: %s", m.replace.c_str() );
            }
            for( const migration::content &c : m.contents ) {
                if( !m_templates.count( c.id ) ) {
                    debugmsg( "Invalid migration contents: %s", c.id.str() );
                }
            }
        }
    }
    for( const auto &elem : m_template_groups ) {
        elem.second->check_consistency();
        inp_mngr.pump_events();
    }
}

//Returns the template with the given identification tag
const itype *Item_factory::find_template( const itype_id &id ) const
{
    cata_assert( frozen );

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
        def->name = no_translation( string_format( "DEBUG: %s", id.c_str() ) );
        def->description = making_id.obj().description;
        m_runtimes[ id ].reset( def );
        return def;
    }

    debugmsg( "Missing item definition: %s", id.c_str() );

    itype *def = new itype();
    def->id = id;
    def->name = no_translation( string_format( "undefined-%s", id.c_str() ) );
    def->description = no_translation( string_format( "Missing item definition for %s.", id.c_str() ) );

    m_runtimes[ id ].reset( def );
    return def;
}

Item_spawn_data *Item_factory::get_group( const item_group_id &group_tag )
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
void Item_factory::load_slot( cata::value_ptr<SlotType> &slotptr, const JsonObject &jo,
                              const std::string &src )
{
    if( !slotptr ) {
        slotptr = cata::make_value<SlotType>();
    }
    load( *slotptr, jo, src );
}

template<typename SlotType>
void Item_factory::load_slot_optional( cata::value_ptr<SlotType> &slotptr, const JsonObject &jo,
                                       const std::string_view member, const std::string &src )
{
    if( !jo.has_member( member ) ) {
        return;
    }
    JsonObject slotjo = jo.get_object( member );
    load_slot( slotptr, slotjo, src );
}

bool Item_factory::load_definition( const JsonObject &jo, const std::string &src, itype &def )
{
    cata_assert( !frozen );

    if( !jo.has_string( "copy-from" ) ) {
        // if this is a new definition ensure we start with a clean itype
        def = itype();
        return true;
    }

    itype_id copy_from;
    jo.read( "copy-from", copy_from, true );
    auto base = m_templates.find( copy_from );
    if( base != m_templates.end() ) {
        def = base->second;
        def.looks_like = copy_from;
        def.was_loaded = true;
        return true;
    }

    auto abstract = m_abstracts.find( copy_from );
    if( abstract != m_abstracts.end() ) {
        def = abstract->second;
        if( def.looks_like.is_empty() ) {
            def.looks_like = copy_from;
        }
        def.was_loaded = true;
        return true;
    }

    deferred.emplace_back( jo, src );
    jo.allow_omitted_members();
    return false;
}

void islot_milling::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "into", into_ );
    optional( jo, was_loaded, "conversion_rate", conversion_rate_ );
}

void islot_milling::deserialize( const JsonObject &jo )
{
    load( jo );
}

static void load_memory_card_data( memory_card_info &mcd, const JsonObject &jo )
{
    mcd.data_chance = jo.get_float( "data_chance", 1.0f );
    mcd.on_read_convert_to = itype_id( jo.get_string( "on_read_convert_to" ) );

    mcd.photos_chance = jo.get_float( "photos_chance", 0.0f );
    mcd.photos_amount = jo.get_int( "photos_amount", 0 );

    mcd.songs_chance = jo.get_float( "songs_chance", 0.0f );
    mcd.songs_amount = jo.get_int( "songs_amount", 0 );

    mcd.recipes_chance = jo.get_float( "recipes_chance", 0.0f );
    mcd.recipes_amount = jo.get_int( "recipes_amount", 0 );
    mcd.recipes_level_min = jo.get_int( "recipes_level_min", 0 );
    mcd.recipes_level_max = jo.get_int( "recipes_level_max", 10 );
    mcd.recipes_categories.clear();
    if( jo.has_array( "recipes_categories" ) ) {
        for( const std::string &cat : jo.get_string_array( "recipes_categories" ) ) {
            mcd.recipes_categories.emplace( cat );
        }
    } else {
        mcd.recipes_categories = { "CC_FOOD" };
    }
}

void islot_ammo::load( const JsonObject &jo )
{
    bool strict = false;

    mandatory( jo, was_loaded, "ammo_type", type );
    optional( jo, was_loaded, "casing", casing, std::nullopt );
    optional( jo, was_loaded, "drop", drop, itype_id::NULL_ID() );
    assign( jo, "drop_chance", drop_chance, strict, 0.0f, 1.0f );
    optional( jo, was_loaded, "drop_active", drop_active, true );
    optional( jo, was_loaded, "projectile_count", count, 1 );
    optional( jo, was_loaded, "shot_spread", shot_spread, 0 );
    assign( jo, "shot_damage", shot_damage, strict );
    // Damage instance assign reader handles pierce and prop_damage
    assign( jo, "damage", damage, strict );
    assign( jo, "range", range, strict, 0 );
    assign( jo, "range_multiplier", range_multiplier, strict, 1.0f );
    assign( jo, "dispersion", dispersion, strict, 0 );
    assign( jo, "recoil", recoil, strict, 0 );
    assign( jo, "count", def_charges, strict, 1 );
    assign( jo, "loudness", loudness, strict, 0 );
    assign( jo, "effects", ammo_effects, strict );
    assign( jo, "critical_multiplier", critical_multiplier, strict, 1.0f );
    optional( jo, was_loaded, "show_stats", force_stat_display, false );
}

void islot_ammo::deserialize( const JsonObject &jo )
{
    load( jo );
}

void Item_factory::load_ammo( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        assign( jo, "stack_size", def.stack_size, src == "dda", 1 );
        if( def.was_loaded ) {
            if( def.ammo ) {
                def.ammo->was_loaded = true;
            } else {
                def.ammo = cata::make_value<islot_ammo>();
                def.ammo->was_loaded = true;
            }
        } else {
            def.ammo = cata::make_value<islot_ammo>();
        }
        def.ammo->load( jo );
        load_basic_info( jo, def, src );
    }
}

void islot_engine::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "displacement", displacement );
}

void islot_engine::deserialize( const JsonObject &jo )
{
    load( jo );
}

void Item_factory::load_engine( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        if( def.was_loaded ) {
            if( def.engine ) {
                def.engine->was_loaded = true;
            } else {
                def.engine = cata::make_value<islot_engine>();
                def.engine->was_loaded = true;
            }
        } else {
            def.engine = cata::make_value<islot_engine>();
        }
        def.engine->load( jo );
        load_basic_info( jo, def, src );
    }
}

void islot_wheel::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "diameter", diameter );
    optional( jo, was_loaded, "width", width );
}

void islot_wheel::deserialize( const JsonObject &jo )
{
    load( jo );
}

void Item_factory::load_wheel( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        if( def.was_loaded ) {
            if( def.wheel ) {
                def.wheel->was_loaded = true;
            } else {
                def.wheel = cata::make_value<islot_wheel>();
                def.wheel->was_loaded = true;
            }
        } else {
            def.wheel = cata::make_value<islot_wheel>();
        }
        def.wheel->load( jo );
        load_basic_info( jo, def, src );
    }
}

void itype_variant_data::deserialize( const JsonObject &jo )
{
    load( jo );
}

void itype_variant_data::load( const JsonObject &jo )
{
    alt_name.make_plural();
    mandatory( jo, false, "id", id );
    mandatory( jo, false, "name", alt_name );
    mandatory( jo, false, "description", alt_description );
    optional( jo, false, "symbol", alt_sym, std::nullopt );
    if( jo.has_string( "color" ) ) {
        alt_color = color_from_string( jo.get_string( "color" ) );
    }
    optional( jo, false, "ascii_picture", art );
    optional( jo, false, "weight", weight );
    optional( jo, false, "append", append );
}

void Item_factory::load( islot_gun &slot, const JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";
    assign( jo, "skill", slot.skill_used, strict );
    assign( jo, "ammo", slot.ammo, strict );
    assign( jo, "range", slot.range, strict );
    // Damage instance assign reader handles pierce
    assign( jo, "ranged_damage", slot.damage, strict,
            damage_instance( damage_type_id::NULL_ID(), -20, -20, -20, -20 ) );
    assign( jo, "dispersion", slot.dispersion, strict );
    assign( jo, "sight_dispersion", slot.sight_dispersion, strict, 0, static_cast<int>( MAX_RECOIL ) );
    assign( jo, "recoil", slot.recoil, strict, 0 );
    assign( jo, "handling", slot.handling, strict );
    assign( jo, "durability", slot.durability, strict, 0, 10 );
    assign( jo, "loudness", slot.loudness, strict );
    assign( jo, "clip_size", slot.clip, strict, 0 );
    assign( jo, "reload", slot.reload_time, strict, 0 );
    assign( jo, "reload_noise", slot.reload_noise, strict );
    assign( jo, "reload_noise_volume", slot.reload_noise_volume, strict, 0 );
    assign( jo, "barrel_volume", slot.barrel_volume, strict, 0_ml );
    assign( jo, "barrel_length", slot.barrel_length, strict, 0_mm );
    assign( jo, "built_in_mods", slot.built_in_mods, strict );
    assign( jo, "default_mods", slot.default_mods, strict );
    assign( jo, "energy_drain", slot.energy_drain, strict, 0_kJ );
    assign( jo, "blackpowder_tolerance", slot.blackpowder_tolerance, strict, 0 );
    assign( jo, "min_cycle_recoil", slot.min_cycle_recoil, strict, 0 );
    assign( jo, "ammo_effects", slot.ammo_effects, strict );
    assign( jo, "ammo_to_fire", slot.ammo_to_fire, strict, 0 );

    if( jo.has_array( "valid_mod_locations" ) ) {
        slot.valid_mod_locations.clear();
        for( JsonArray curr : jo.get_array( "valid_mod_locations" ) ) {
            slot.valid_mod_locations.emplace( gunmod_location( curr.get_string( 0 ) ),
                                              curr.get_int( 1 ) );
        }
    }

    assign( jo, "modes", slot.modes );
}

void Item_factory::load_gun( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.gun, jo, src );
        load_basic_info( jo, def, src );
    }
}

// TODO: Refactor this with load_tool_armor
void Item_factory::load_armor( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        if( def.was_loaded ) {
            if( def.armor ) {
                def.armor->was_loaded = true;
            } else {
                def.armor = cata::make_value<islot_armor>();
                def.armor->was_loaded = true;
            }
        } else {
            def.armor = cata::make_value<islot_armor>();
        }
        def.armor->load( jo );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load_pet_armor( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        if( def.was_loaded ) {
            if( def.pet_armor ) {
                def.pet_armor->was_loaded = true;
            } else {
                def.pet_armor = cata::make_value<islot_pet_armor>();
                def.pet_armor->was_loaded = true;
            }
        } else {
            def.pet_armor = cata::make_value<islot_pet_armor>();
        }
        def.pet_armor->load( jo );
        load_basic_info( jo, def, src );
    }
}

namespace io
{
template<>
std::string enum_to_string<layer_level>( layer_level data )
{
    switch( data ) {
        case layer_level::PERSONAL:
            return "PERSONAL";
        case layer_level::SKINTIGHT:
            return "SKINTIGHT";
        case layer_level::NORMAL:
            return "NORMAL";
        case layer_level::WAIST:
            return "WAIST";
        case layer_level::OUTER:
            return "OUTER";
        case layer_level::BELTED:
            return "BELTED";
        case layer_level::AURA:
            return "AURA";
        case layer_level::NUM_LAYER_LEVELS:
            break;
    }
    cata_fatal( "Invalid layer_level" );
}
} // namespace io

void part_material::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "type", id );
    optional( jo, false, "covered_by_mat", cover, 100 );
    if( cover < 1 || cover > 100 ) {
        jo.throw_error( string_format( "invalid covered_by_mat \"%d\"", cover ) );
    }
    optional( jo, false, "thickness", thickness, 0.0f );
    optional( jo, false, "ignore_sheet_thickness", ignore_sheet_thickness, false );
}

void armor_portion_data::deserialize( const JsonObject &jo )
{
    if( !assign_coverage_from_json( jo, "covers", covers ) ) {
        jo.throw_error( string_format( "need to specify covered limbs for each body part" ) );
    }
    optional( jo, false, "coverage", coverage, 0 );

    // load a breathability override
    breathability_rating temp_enum;
    optional( jo, false, "breathability", temp_enum, breathability_rating::last );
    if( temp_enum != breathability_rating::last ) {
        breathability = material_type::breathability_to_rating( temp_enum );
    }
    optional( jo, false, "specifically_covers", sub_coverage );

    // if no sub locations are specified assume it covers everything
    if( covers.has_value() && sub_coverage.empty() ) {
        for( const bodypart_str_id &bp : covers.value() ) {
            for( const sub_bodypart_str_id &sbp : bp->sub_parts ) {
                // only assume to add the non hanging locations
                if( !sbp->secondary ) {
                    sub_coverage.insert( sbp );
                }
            }
        }
    }

    optional( jo, false, "cover_melee", cover_melee, coverage );
    optional( jo, false, "cover_ranged", cover_ranged, coverage );
    optional( jo, false, "cover_vitals", cover_vitals, 0 );

    optional( jo, false, "rigid_layer_only", rigid_layer_only, false );

    if( jo.has_array( "encumbrance_modifiers" ) ) {
        // instead of reading encumbrance calculate it by weight
        optional( jo, false, "encumbrance_modifiers", encumber_modifiers );
    } else if( jo.has_array( "encumbrance" ) ) {
        encumber = jo.get_array( "encumbrance" ).get_int( 0 );
        max_encumber = jo.get_array( "encumbrance" ).get_int( 1 );
    } else {
        optional( jo, false, "encumbrance", encumber, 0 );
    }
    optional( jo, false, "material_thickness", avg_thickness, 0.0f );
    optional( jo, false, "environmental_protection", env_resist, 0 );
    optional( jo, false, "environmental_protection_with_filter", env_resist_w_filter, 0 );
    optional( jo, false, "volume_encumber_modifier", volume_encumber_modifier, 1 );

    // TODO: Make mandatory - once we remove the old loading below
    if( jo.has_member( "material" ) ) {
        if( jo.has_array( "material" ) && jo.get_array( "material" ).test_object() ) {
            mandatory( jo, false, "material", materials );
        } else {
            // Old style material definition ( ex: "material": [ "cotton", "plastic" ] )
            // TODO: Deprecate and remove
            for( const std::string &mat : jo.get_tags( "material" ) ) {
                materials.emplace_back( mat, 100, 0.0f );
            }
        }
    }

    optional( jo, false, "layers", layers );
}

template<typename T>
static void apply_optional( T &value, const std::optional<T> &applied )
{
    if( applied ) {
        value = *applied;
    }
}

// Gets around the issue that std::optional doesn't support
// the *= and += operators required for "proportional" and "relative".
template<typename T>
static void get_optional( const JsonObject &jo, bool was_loaded, const std::string &member,
                          std::optional<T> &value )
{
    T tmp;
    if( value ) {
        tmp = *value;
    }
    optional( jo, was_loaded, member, tmp );
    if( jo.has_member( member ) ) {
        value = tmp;
    }
}

template<typename T>
static void get_relative( const JsonObject &jo, const std::string_view member,
                          std::optional<T> &value,
                          T default_val )
{
    if( jo.has_member( member ) ) {
        value = static_cast<T>( value.value_or( default_val ) + jo.get_float( member ) );
    }
}

template<typename T>
static void get_proportional( const JsonObject &jo, const std::string_view member,
                              std::optional<T> &value, T default_val )
{
    if( jo.has_member( member ) ) {
        value = static_cast<T>( value.value_or( default_val ) * jo.get_float( member ) );
    }
}

void islot_armor::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "armor", sub_data );

    std::optional<body_part_set> covers;

    assign_coverage_from_json( jo, "covers", covers );
    get_optional( jo, was_loaded, "material_thickness", _material_thickness );
    get_optional( jo, was_loaded, "environmental_protection", _env_resist );
    get_optional( jo, was_loaded, "environmental_protection_with_filter", _env_resist_w_filter );

    JsonObject relative = jo.get_object( "relative" );
    relative.allow_omitted_members();
    get_relative( relative, "material_thickness", _material_thickness, 0.f );
    get_relative( relative, "environmental_protection", _env_resist, 0 );
    get_relative( relative, "environmental_protection_with_filter", _env_resist_w_filter, 0 );
    float _rel_encumb = 0.0f;
    if( relative.has_float( "encumbrance" ) ) {
        _rel_encumb = relative.get_float( "encumbrance" );
    }

    JsonObject proportional = jo.get_object( "proportional" );
    proportional.allow_omitted_members();
    get_proportional( proportional, "material_thickness", _material_thickness, 0.f );
    get_proportional( proportional, "environmental_protection", _env_resist, 0 );
    get_proportional( proportional, "environmental_protection_with_filter", _env_resist_w_filter, 0 );
    float _prop_encumb = 1.0f;
    if( proportional.has_float( "encumbrance" ) ) {
        _prop_encumb = proportional.get_float( "encumbrance" );
    }

    for( armor_portion_data &armor : sub_data ) {
        apply_optional( armor.avg_thickness, _material_thickness );
        apply_optional( armor.env_resist, _env_resist );
        apply_optional( armor.env_resist_w_filter, _env_resist_w_filter );
        if( covers ) {
            armor.covers = covers;
        }

        // pass down relative and proportional encumbrance
        if( armor.encumber > 0 ) {
            armor.encumber += _rel_encumb;
            armor.encumber *= _prop_encumb;

            armor.encumber = std::max( 0, armor.encumber );
        }

        if( armor.max_encumber > 0 ) {
            armor.max_encumber += _rel_encumb;
            armor.max_encumber *= _prop_encumb;

            armor.max_encumber = std::max( 0, armor.max_encumber );
        }

    }

    optional( jo, was_loaded, "sided", sided, false );

    optional( jo, was_loaded, "warmth", warmth, 0 );
    optional( jo, was_loaded, "non_functional", non_functional, itype_id() );
    optional( jo, was_loaded, "damage_verb", damage_verb );
    optional( jo, was_loaded, "weight_capacity_modifier", weight_capacity_modifier, 1.0 );
    optional( jo, was_loaded, "weight_capacity_bonus", weight_capacity_bonus, mass_reader{}, 0_gram );
    optional( jo, was_loaded, "power_armor", power_armor, false );
    optional( jo, was_loaded, "valid_mods", valid_mods );
}

void islot_armor::deserialize( const JsonObject &jo )
{
    load( jo );
}

void islot_pet_armor::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "material_thickness", thickness, 0 );
    optional( jo, was_loaded, "max_pet_vol", max_vol, volume_reader{}, 0_ml );
    optional( jo, was_loaded, "min_pet_vol", min_vol, volume_reader{}, 0_ml );
    optional( jo, was_loaded, "pet_bodytype", bodytype );
    optional( jo, was_loaded, "environmental_protection", env_resist, 0 );
    optional( jo, was_loaded, "environmental_protection_with_filter", env_resist_w_filter, 0 );
    optional( jo, was_loaded, "power_armor", power_armor, false );
}

void islot_pet_armor::deserialize( const JsonObject &jo )
{
    load( jo );
}

void Item_factory::load( islot_tool &slot, const JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    assign( jo, "ammo", slot.ammo_id, strict );
    assign( jo, "max_charges", slot.max_charges, strict, 0 );
    assign( jo, "initial_charges", slot.def_charges, strict, 0 );
    assign( jo, "charges_per_use", slot.charges_per_use, strict, 0 );
    assign( jo, "charge_factor", slot.charge_factor, strict, 1 );
    assign( jo, "turns_per_charge", slot.turns_per_charge, strict, 0 );
    assign( jo, "fuel_efficiency", slot.fuel_efficiency, strict, -1.0f );
    assign( jo, "power_draw", slot.power_draw, strict, 0_W );
    assign( jo, "revert_msg", slot.revert_msg, strict );
    assign( jo, "sub", slot.subtype, strict );

    if( slot.def_charges > slot.max_charges ) {
        jo.throw_error_at( "initial_charges", "initial_charges is larger than max_charges" );
    }

    if( jo.has_array( "rand_charges" ) ) {
        if( jo.has_member( "initial_charges" ) ) {
            jo.throw_error_at(
                "rand_charges",
                "You can have a fixed initial amount of charges, or randomized.  Not both." );
        }
        for( const int charge : jo.get_array( "rand_charges" ) ) {
            slot.rand_charges.push_back( charge );
        }
        if( slot.rand_charges.size() == 1 ) {
            // see item::item(...) for the use of this array
            jo.throw_error_at(
                "rand_charges",
                "a rand_charges array with only one entry will be ignored, it needs at least 2 "
                "entries!" );
        }
    }
}

void Item_factory::load_tool( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.tool, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( relic &slot, const JsonObject &jo, const std::string_view )
{
    slot.load( jo );
}

void Item_factory::load( islot_mod &slot, const JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    if( jo.has_array( "ammo_modifier" ) ) {
        for( const std::string id : jo.get_array( "ammo_modifier" ) ) {
            slot.ammo_modifier.insert( ammotype( id ) );
        }
    } else if( jo.has_string( "ammo_modifier" ) ) {
        slot.ammo_modifier.insert( ammotype( jo.get_string( "ammo_modifier" ) ) );
    }
    assign( jo, "capacity_multiplier", slot.capacity_multiplier, strict );

    if( jo.has_member( "acceptable_ammo" ) ) {
        slot.acceptable_ammo.clear();
        for( const std::string &e : jo.get_tags( "acceptable_ammo" ) ) {
            slot.acceptable_ammo.insert( ammotype( e ) );
        }
    }

    JsonArray mags = jo.get_array( "magazine_adaptor" );
    if( !mags.empty() ) {
        slot.magazine_adaptor.clear();
    }
    for( JsonArray arr : mags ) {
        ammotype ammo( arr.get_string( 0 ) ); // an ammo type (e.g. 9mm)
        // compatible magazines for this ammo type
        for( const std::string line : arr.get_array( 1 ) ) {
            slot.magazine_adaptor[ ammo ].insert( itype_id( line ) );
        }
    }

    optional( jo, false, "pocket_mods", slot.add_pockets );
}

void Item_factory::load_toolmod( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.mod, jo, src );
        load_basic_info( jo, def, src );
    }
}

// TODO: Refactor this with load_armor
// This function does load_slot( def.tool ), but otherwise they are the same
void Item_factory::load_tool_armor( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.tool, jo, src );
        if( def.was_loaded ) {
            if( def.armor ) {
                def.armor->was_loaded = true;
            } else {
                def.armor = cata::make_value<islot_armor>();
                def.armor->was_loaded = true;
            }
        } else {
            def.armor = cata::make_value<islot_armor>();
        }
        def.armor->load( jo );
        load_basic_info( jo, def, src );
    }
}

void islot_book::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "max_level", level, 0 );
    optional( jo, was_loaded, "required_level", req, 0 );
    optional( jo, was_loaded, "fun", fun, 0 );
    optional( jo, was_loaded, "intelligence", intel, 0 );

    optional( jo, was_loaded, "time", time, 0_turns );

    optional( jo, was_loaded, "skill", skill, skill_id::NULL_ID() );
    optional( jo, was_loaded, "martial_art", martial_art, matype_id::NULL_ID() );
    optional( jo, was_loaded, "chapters", chapters, 0 );
    optional( jo, was_loaded, "proficiencies", proficiencies );
    optional( jo, was_loaded, "scannable", is_scannable, true );
}

void islot_book::deserialize( const JsonObject &jo )
{
    load( jo );
}

void Item_factory::load_book( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        if( def.was_loaded ) {
            if( def.book ) {
                def.book->was_loaded = true;
            } else {
                def.book = cata::make_value<islot_book>();
                def.book->was_loaded = true;
            }
        } else {
            def.book = cata::make_value<islot_book>();
        }
        def.book->load( jo );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_comestible &slot, const JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    JsonObject relative = jo.get_object( "relative" );
    JsonObject proportional = jo.get_object( "proportional" );
    relative.allow_omitted_members();
    proportional.allow_omitted_members();

    // !slot.comesttype.empty() for was_loaded - we don't have a proper was_loaded, but
    // if it's been loaded before, this should have a value. If it's not, we'll catch it later.
    mandatory( jo, !slot.comesttype.empty(), "comestible_type", slot.comesttype );
    assign( jo, "tool", slot.tool, strict );
    assign( jo, "charges", slot.def_charges, strict, 1 );
    assign( jo, "quench", slot.quench, strict );
    assign( jo, "fun", slot.fun, strict );
    assign( jo, "stim", slot.stim, strict );
    assign( jo, "fatigue_mod", slot.fatigue_mod, strict );
    assign( jo, "healthy", slot.healthy, strict );
    assign( jo, "parasites", slot.parasites, strict, 0 );
    assign( jo, "freezing_point", slot.freeze_point, strict );
    assign( jo, "spoils_in", slot.spoils, strict, 1_hours );
    assign( jo, "cooks_like", slot.cooks_like, strict );
    assign( jo, "smoking_result", slot.smoking_result, strict );
    assign( jo, "petfood", slot.petfood, strict );

    for( const JsonObject jsobj : jo.get_array( "contamination" ) ) {
        slot.contamination.emplace( diseasetype_id( jsobj.get_string( "disease" ) ),
                                    jsobj.get_int( "probability" ) );
    }

    bool is_not_boring = false;
    if( jo.has_member( "primary_material" ) ) {
        std::string mat = jo.get_string( "primary_material" );
        slot.specific_heat_solid = material_id( mat )->specific_heat_solid();
        slot.specific_heat_liquid = material_id( mat )->specific_heat_liquid();
        slot.latent_heat = material_id( mat )->latent_heat();
        is_not_boring = is_not_boring || mat == "junk";
    } else if( jo.has_member( "material" ) ) {
        float specific_heat_solid = 0.0f;
        float specific_heat_liquid = 0.0f;
        float latent_heat = 0.0f;
        int mat_total = 0;

        auto add_spi = [&]( const material_id & m, int portion ) {
            specific_heat_solid += m->specific_heat_solid() * portion;
            specific_heat_liquid += m->specific_heat_liquid() * portion;
            latent_heat += m->latent_heat() * portion;
            mat_total += portion;
            is_not_boring = is_not_boring || m == material_junk;
        };

        if( jo.has_array( "material" ) && jo.get_array( "material" ).test_object() ) {
            for( JsonObject m : jo.get_array( "material" ) ) {
                const material_id mat_id( m.get_string( "type" ) );
                int portion = m.get_int( "portion", 1 );
                add_spi( mat_id, portion );
            }
        } else {
            for( const std::string &m : jo.get_tags( "material" ) ) {
                add_spi( material_id( m ), 1 );
            }
        }
        // Average based on number of materials.
        slot.specific_heat_liquid = specific_heat_liquid / mat_total;
        slot.specific_heat_solid = specific_heat_solid / mat_total;
        slot.latent_heat = latent_heat / mat_total;
    }

    // Junk food never gets old by default, but this can still be overridden.
    if( is_not_boring ) {
        slot.monotony_penalty = 0;
    }
    assign( jo, "monotony_penalty", slot.monotony_penalty, strict );
    assign( jo, "addiction_potential", slot.default_addict_potential, strict );
    if( jo.has_member( "addiction_type" ) ) {
        slot.addictions.clear();
        if( jo.has_string( "addiction_type" ) ) {
            slot.addictions.emplace( addiction_id( jo.get_string( "addiction_type" ) ),
                                     slot.default_addict_potential );
        } else { //"addiction_type" is an array
            for( JsonValue jval : jo.get_array( "addiction_type" ) ) {
                if( jval.test_string() ) {
                    slot.addictions.emplace( addiction_id( jval.get_string() ), slot.default_addict_potential );
                } else { //nested object with addiction + potential
                    JsonObject jobj = jval.get_object();
                    slot.addictions.emplace( addiction_id( jobj.get_string( "addiction" ) ),
                                             jobj.get_int( "potential" ) );
                }
            }
        }
    } else if( jo.has_member( "addiction_potential" ) ) {
        // copy-from'd while only changing the general potential, so assign new potential to all addictions
        for( std::pair<const addiction_id, int> &add : slot.addictions ) {
            add.second = slot.default_addict_potential;
        }
    }

    bool got_calories = false;

    if( jo.has_member( "calories" ) ) {
        // The value here is in kcal, but is stored as simply calories
        slot.default_nutrition.calories = 1000 * jo.get_int( "calories" );
        got_calories = true;

    } else if( relative.has_member( "calories" ) ) {
        // The value here is in kcal, but is stored as simply calories
        slot.default_nutrition.calories += 1000 * relative.get_int( "calories" );
        got_calories = true;

    } else if( proportional.has_member( "calories" ) ) {
        // The value here is in kcal, but is stored as simply calories
        slot.default_nutrition.calories *= proportional.get_float( "calories" );
        got_calories = true;

    } else if( jo.has_member( "nutrition" ) ) {
        // The value here is in kcal, but is stored as simply calories
        slot.default_nutrition.calories = jo.get_int( "nutrition" ) * islot_comestible::kcal_per_nutr *
                                          1000;
    }

    for( JsonValue jv : jo.get_array( "consumption_effect_on_conditions" ) ) {
        slot.consumption_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }

    if( jo.has_member( "nutrition" ) && got_calories ) {
        jo.throw_error_at( "nutrition", "cannot specify both nutrition and calories" );
    }

    // any specification of vitamins suppresses use of material defaults @see Item_factory::finalize
    if( jo.has_array( "vitamins" ) ) {
        for( JsonArray pair : jo.get_array( "vitamins" ) ) {
            vitamin_id vit( pair.get_string( 0 ) );
            slot.default_nutrition.vitamins[ vit ] = pair.get_int( 1 );
        }

    } else if( relative.has_array( "vitamins" ) ) {
        for( JsonArray pair : relative.get_array( "vitamins" ) ) {
            vitamin_id vit( pair.get_string( 0 ) );
            slot.default_nutrition.vitamins[ vit ] += pair.get_int( 1 );
        }
    }

    if( jo.has_string( "rot_spawn" ) ) {
        slot.rot_spawn = mongroup_id( jo.get_string( "rot_spawn" ) );
    }
    assign( jo, "rot_spawn_chance", slot.rot_spawn_chance, strict, 0 );

}

void islot_brewable::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "time", time, 1_turns );
    mandatory( jo, was_loaded, "results", results );
}

void islot_brewable::deserialize( const JsonObject &jo )
{
    load( jo );
}

void Item_factory::load_comestible( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        assign( jo, "stack_size", def.stack_size, src == "dda", 1 );
        load_slot( def.comestible, jo, src );
        load_basic_info( jo, def, src );
    }
}

void islot_seed::load( const JsonObject &jo )
{
    assign( jo, "grow", grow, false, 1_days );
    optional( jo, was_loaded, "fruit_div", fruit_div, 1 );
    mandatory( jo, was_loaded, "plant_name", plant_name );
    mandatory( jo, was_loaded, "fruit", fruit_id );
    optional( jo, was_loaded, "seeds", spawn_seeds, true );
    optional( jo, was_loaded, "byproducts", byproducts );
}

void islot_seed::deserialize( const JsonObject &jo )
{
    load( jo );
}

void Item_factory::load( islot_gunmod &slot, const JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    assign( jo, "damage_modifier", slot.damage, strict,
            damage_instance( damage_type_id::NULL_ID(), -20, -20, -20, -20 ) );
    assign( jo, "loudness_modifier", slot.loudness );
    assign( jo, "location", slot.location );
    assign( jo, "dispersion_modifier", slot.dispersion );
    assign( jo, "field_of_view", slot.field_of_view );
    assign( jo, "sight_dispersion", slot.sight_dispersion );
    assign( jo, "aim_speed_modifier", slot.aim_speed_modifier );
    assign( jo, "aim_speed", slot.aim_speed, strict, -1 );
    assign( jo, "handling_modifier", slot.handling, strict );
    assign( jo, "range_modifier", slot.range );
    assign( jo, "range_multiplier", slot.range_multiplier );
    assign( jo, "consume_chance", slot.consume_chance );
    assign( jo, "consume_divisor", slot.consume_divisor );
    assign( jo, "shot_spread_multiplier_modifier", slot.shot_spread_multiplier_modifier );
    assign( jo, "ammo_effects", slot.ammo_effects, strict );
    assign( jo, "energy_drain_multiplier", slot.energy_drain_multiplier );
    assign( jo, "energy_drain_modifier", slot.energy_drain_modifier );
    assign( jo, "ammo_to_fire_multiplier", slot.ammo_to_fire_multiplier );
    assign( jo, "ammo_to_fire_modifier", slot.ammo_to_fire_modifier );
    assign( jo, "weight_multiplier", slot.weight_multiplier );
    assign( jo, "overwrite_min_cycle_recoil", slot.overwrite_min_cycle_recoil );
    // convert aim_speed to FoV and aim_speed_modifier automatically, if FoV is not set
    if( slot.aim_speed >= 0 && slot.field_of_view <= 0 ) {
        if( slot.aim_speed > 6 ) {
            slot.aim_speed_modifier = 5 * ( slot.aim_speed - 6 );
            slot.field_of_view = 480;
        } else {
            slot.field_of_view = 480 - 30 * ( 6 - slot.aim_speed );
        }
    }
    if( jo.has_int( "install_time" ) ) {
        slot.install_time = jo.get_int( "install_time" );
    } else if( jo.has_string( "install_time" ) ) {
        slot.install_time = to_moves<int>( read_from_json_string<time_duration>
                                           ( jo.get_member( "install_time" ),
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
        for( JsonArray curr : jo.get_array( "add_mod" ) ) {
            slot.add_mod.emplace( gunmod_location( curr.get_string( 0 ) ), curr.get_int( 1 ) );
        }
    }
    assign( jo, "blacklist_mod", slot.blacklist_mod );
    assign( jo, "barrel_length", slot.barrel_length );
}

void Item_factory::load_gunmod( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.gunmod, jo, src );
        load_slot( def.mod, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_magazine &slot, const JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";
    assign( jo, "ammo_type", slot.type, strict );
    assign( jo, "capacity", slot.capacity, strict, 0 );
    assign( jo, "count", slot.count, strict, 0 );
    assign( jo, "default_ammo", slot.default_ammo, strict );
    assign( jo, "reload_time", slot.reload_time, strict, 0 );
    assign( jo, "linkage", slot.linkage, strict );
}

void Item_factory::load_magazine( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.magazine, jo, src );
        load_basic_info( jo, def, src );
    }
}

void islot_battery::load( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "max_capacity", max_capacity );
}

void islot_battery::deserialize( const JsonObject &jo )
{
    load( jo );
}

void Item_factory::load_battery( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        if( def.was_loaded ) {
            if( def.battery ) {
                def.battery->was_loaded = true;
            } else {
                def.battery = cata::make_value<islot_battery>();
                def.battery->was_loaded = true;
            }
        } else {
            def.battery = cata::make_value<islot_battery>();
        }
        def.battery->load( jo );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load( islot_bionic &slot, const JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    if( jo.has_member( "bionic_id" ) ) {
        assign( jo, "bionic_id", slot.id, strict );
    } else {
        assign( jo, "id", slot.id, strict );
    }

    assign( jo, "difficulty", slot.difficulty, strict, 0 );
    assign( jo, "is_upgrade", slot.is_upgrade );

    assign( jo, "installation_data", slot.installation_data );
}

void Item_factory::load_bionic( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_slot( def.bionic, jo, src );
        load_basic_info( jo, def, src );
    }
}

void Item_factory::load_generic( const JsonObject &jo, const std::string &src )
{
    itype def;
    if( load_definition( jo, src, def ) ) {
        load_basic_info( jo, def, src );
    }
}

// Adds allergy flags to items with allergenic materials
// Set for all items (not just food and clothing) to avoid edge cases
void Item_factory::set_allergy_flags( itype &item_template )
{
    static const std::array<std::pair<material_id, flag_id>, 29> all_pairs = { {
            // First allergens:
            // An item is an allergen even if it has trace amounts of allergenic material
            { material_hflesh, flag_CANNIBALISM },
            { material_hblood, flag_CANNIBALISM },
            { material_hflesh, flag_ALLERGEN_MEAT },
            { material_iflesh, flag_ALLERGEN_MEAT },
            { material_bread, flag_ALLERGEN_BREAD },
            { material_flesh, flag_ALLERGEN_MEAT },
            { material_cheese, flag_ALLERGEN_CHEESE},
            { material_blood, flag_ALLERGEN_MEAT },
            { material_hblood, flag_ALLERGEN_MEAT },
            { material_bone, flag_ALLERGEN_MEAT },
            { material_wheat, flag_ALLERGEN_WHEAT },
            { material_fruit, flag_ALLERGEN_FRUIT },
            { material_veggy, flag_ALLERGEN_VEGGY },
            { material_dried_vegetable, flag_ALLERGEN_DRIED_VEGETABLE },
            { material_bean, flag_ALLERGEN_VEGGY },
            { material_tomato, flag_ALLERGEN_VEGGY },
            { material_garlic, flag_ALLERGEN_VEGGY },
            { material_nut, flag_ALLERGEN_NUT },
            { material_mushroom, flag_ALLERGEN_VEGGY },
            { material_milk, flag_ALLERGEN_MILK },
            { material_egg, flag_ALLERGEN_EGG },
            { material_junk, flag_ALLERGEN_JUNK },
            // Not food, but we can keep it here
            { material_wool, flag_ALLERGEN_WOOL },
            // Now "made of". Those flags should not be passed
            { material_flesh, flag_CARNIVORE_OK },
            { material_hflesh, flag_CARNIVORE_OK },
            { material_iflesh, flag_CARNIVORE_OK },
            { material_blood, flag_CARNIVORE_OK },
            { material_hblood, flag_CARNIVORE_OK },
            { material_honey, flag_URSINE_HONEY }
        }
    };

    const auto &mats = item_template.materials;
    for( const auto &pr : all_pairs ) {
        if( mats.find( pr.first ) != mats.end() ) {
            item_template.item_tags.insert( pr.second );
        }
    }
}

// Migration helper: turns human flesh into generic flesh
// Don't call before making sure that the cannibalism flag is set
void hflesh_to_flesh( itype &item_template )
{
    auto &mats = item_template.materials;
    const size_t old_size = mats.size();
    int ports = 0;
    for( auto mat = mats.begin(); mat != mats.end(); ) {
        if( mat->first == material_hflesh ) {
            ports += mat->second;
            mat = mats.erase( mat );
        } else {
            mat++;
        }
    }
    // Only add "flesh" material if not already present
    if( old_size != mats.size() &&
        mats.find( material_flesh ) == mats.end() ) {
        mats.emplace( "flesh", ports );
    }
}

void Item_factory::npc_implied_flags( itype &item_template )
{
    if( item_template.use_methods.count( "explosion" ) > 0 ) {
        item_template.item_tags.insert( flag_DANGEROUS );
    }

    if( item_template.has_flag( flag_DANGEROUS ) ) {
        item_template.item_tags.insert( flag_NPC_THROW_NOW );
    }

    if( item_template.has_flag( flag_BOMB ) ) {
        item_template.item_tags.insert( flag_NPC_ACTIVATE );
    }

    if( item_template.has_flag( flag_NPC_THROW_NOW ) ) {
        item_template.item_tags.insert( flag_NPC_THROWN );
    }

    if( item_template.has_flag( flag_NPC_ACTIVATE ) ||
        item_template.has_flag( flag_NPC_THROWN ) ) {
        item_template.item_tags.insert( flag_NPC_ALT_ATTACK );
    }

    if( item_template.has_flag( flag_DANGEROUS ) ||
        item_template.has_flag( flag_PSEUDO ) || item_template.has_flag( flag_INTEGRATED ) ) {
        item_template.item_tags.insert( flag_TRADER_AVOID );
    }
}

static bool has_pocket_type( const std::vector<pocket_data> &data, item_pocket::pocket_type pk )
{
    for( const pocket_data &pocket : data ) {
        if( pocket.type == pk ) {
            return true;
        }
    }
    return false;
}

static bool has_only_special_pockets( const itype &def )
{
    // There are some places where we need to know whether any non-special
    // pockets have been added.  There might be no pockets, or there might be
    // just the special ones, depending on whether the definition is
    // copied-from another or not.
    if( def.pockets.empty() ) {
        return true;
    }

    const std::vector<item_pocket::pocket_type> special_pockets = { item_pocket::pocket_type::CORPSE, item_pocket::pocket_type::MOD, item_pocket::pocket_type::MIGRATION };

    for( const pocket_data &pocket : def.pockets ) {
        if( std::find( special_pockets.begin(), special_pockets.end(),
                       pocket.type ) == special_pockets.end() ) {
            return false;
        }
    }

    return true;
}

void Item_factory::check_and_create_magazine_pockets( itype &def )
{
    // the item we're trying to migrate must actually have data for ammo
    if( def.magazines.empty() && !( def.magazine || def.tool ) ) {
        return;
    }
    if( def.tool && def.tool->ammo_id.empty() ) {
        // If tool has no ammo type it needs no magazine.
        return;
    }
    if( def.tool && def.tool->max_charges == 0 && def.magazines.empty() ) {
        // If tool has no charges nor magazines, it needs no magazine.
        return;
    }
    if( !has_only_special_pockets( def ) ) {
        // this means pockets were defined in json already
        // we assume they're good to go, or error elsewhere
        if( def.tool && def.tool->max_charges != 0 ) {
            // warn about redundant max_charges definition
            debugmsg( "Redundant max charge value specified for %s with magazine pocket.",
                      def.get_id().str() );
        }
        return;
    }

    // Thing uses no ammo
    if( def.magazine && def.magazine->type.empty() ) {
        return;
    }
    if( def.tool && def.tool->ammo_id.empty() ) {
        return;
    }

    pocket_data mag_data;
    mag_data.holster = true;
    mag_data.volume_capacity = 200_liter;
    mag_data.max_contains_weight = 2000000_kilogram;
    mag_data.max_item_length = 2_km;
    mag_data.watertight = true;
    if( !def.magazines.empty() ) {
        mag_data.type = item_pocket::pocket_type::MAGAZINE_WELL;
        mag_data.rigid = false;
        ammotype default_ammo = ammotype::NULL_ID();
        if( def.tool ) {
            default_ammo = *def.tool->ammo_id.begin();
        }
        if( !default_ammo.is_null() ) {
            auto magazines_for_default_ammo = def.magazines.find( default_ammo );
            if( magazines_for_default_ammo == def.magazines.end() ) {
                debugmsg( "item %s defines magazines but no magazine for its default ammo type",
                          def.get_id().str() );
            } else {
                cata_assert( !magazines_for_default_ammo->second.empty() );
                mag_data.default_magazine = *magazines_for_default_ammo->second.begin();
            }
        }
        for( const std::pair<const ammotype, std::set<itype_id>> &mag_pair : def.magazines ) {
            for( const itype_id &mag_type : mag_pair.second ) {
                mag_data.item_id_restriction.insert( mag_type );
            }
        }
    } else {
        mag_data.type = item_pocket::pocket_type::MAGAZINE;
        mag_data.rigid = true;
        if( def.magazine ) {
            for( const ammotype &amtype : def.magazine->type ) {
                mag_data.ammo_restriction.emplace( amtype, def.magazine->capacity );
            }
        }
        if( def.tool ) {
            for( const ammotype &amtype : def.tool->ammo_id ) {
                mag_data.ammo_restriction.emplace( amtype, def.tool->max_charges );
            }
            def.tool->max_charges = 0;
            def.tool->def_charges = 0;
        }
    }
    def.pockets.push_back( mag_data );
    debugmsg( _( "%s needs pocket definitions" ), def.get_id().str() );
}

void Item_factory::add_special_pockets( itype &def )
{
    if( def.has_flag( flag_CORPSE ) &&
        !has_pocket_type( def.pockets, item_pocket::pocket_type::CORPSE ) ) {
        def.pockets.emplace_back( item_pocket::pocket_type::CORPSE );
    }
    if( ( def.tool || def.gun ) && !has_pocket_type( def.pockets, item_pocket::pocket_type::MOD ) ) {
        def.pockets.emplace_back( item_pocket::pocket_type::MOD );
    }
    if( !has_pocket_type( def.pockets, item_pocket::pocket_type::MIGRATION ) ) {
        def.pockets.emplace_back( item_pocket::pocket_type::MIGRATION );
    }
    if( !has_pocket_type( def.pockets, item_pocket::pocket_type::CABLE ) &&
        def.get_use( "link_up" ) != nullptr ) {
        pocket_data cable_pocket( item_pocket::pocket_type::CABLE );
        cable_pocket.rigid = false;
        def.pockets.emplace_back( cable_pocket );
    }
}

enum class grip_val : int {
    BAD = 0,
    NONE = 1,
    SOLID = 2,
    WEAPON = 3,
    LAST = 4
};
template<>
struct enum_traits<grip_val> {
    static constexpr grip_val last = grip_val::LAST;
};
enum class length_val : int {
    HAND = 0,
    SHORT = 1,
    LONG = 2,
    LAST = 3
};
template<>
struct enum_traits<length_val> {
    static constexpr length_val last = length_val::LAST;
};
enum class surface_val : int {
    POINT = 0,
    LINE = 1,
    ANY = 2,
    EVERY = 3,
    LAST = 4
};
template<>
struct enum_traits<surface_val> {
    static constexpr surface_val last = surface_val::LAST;
};
enum class balance_val : int {
    CLUMSY = 0,
    UNEVEN = 1,
    NEUTRAL = 2,
    GOOD = 3,
    LAST = 4
};
template<>
struct enum_traits<balance_val> {
    static constexpr balance_val last = balance_val::LAST;
};

namespace io
{
template<>
std::string enum_to_string<encumbrance_modifier>( encumbrance_modifier data )
{
    switch( data ) {
        case encumbrance_modifier::IMBALANCED:
            return "IMBALANCED";
        case encumbrance_modifier::RESTRICTS_NECK:
            return "RESTRICTS_NECK";
        case encumbrance_modifier::WELL_SUPPORTED:
            return "WELL_SUPPORTED";
        case encumbrance_modifier::NONE:
            return "NONE";
        case encumbrance_modifier::last:
            break;
    }
    cata_fatal( "Invalid encumbrance descriptor" );
}
} // namespace io

namespace io
{
// *INDENT-OFF*
template<>
std::string enum_to_string<link_state>( link_state data )
{
    switch( data ) {
        case link_state::no_link:
            return "no_link";
        case link_state::needs_reeling:
            return "needs_reeling";
        case link_state::vehicle_port:
            return "vehicle_port";
        case link_state::vehicle_battery:
            return "vehicle_battery";
        case link_state::vehicle_tow:
            return "vehicle_tow";
        case link_state::bio_cable:
            return "bio_cable";
        case link_state::ups:
            return "ups";
        case link_state::solarpack:
            return "solarpack";
        case link_state::last:
            break;
    }
    cata_fatal( "Invalid link_state" );
}
// *INDENT-ON*
} // namespace io

namespace io
{
// *INDENT-OFF*
template<>
std::string enum_to_string<grip_val>( grip_val val )
{
    switch( val ) {
        case grip_val::BAD: return "bad";
        case grip_val::NONE: return "none";
        case grip_val::SOLID: return "solid";
        case grip_val::WEAPON: return "weapon";
        default: break;
    }
    cata_fatal( "Invalid grip val" );
}

template<>
std::string enum_to_string<length_val>( length_val val )
{
    switch( val ) {
        case length_val::HAND: return "hand";
        case length_val::SHORT: return "short";
        case length_val::LONG: return "long";
        default: break;
    }
    cata_fatal( "Invalid length val" );
}

template<>
std::string enum_to_string<surface_val>( surface_val val )
{
    switch( val ) {
        case surface_val::POINT: return "point";
        case surface_val::LINE: return "line";
        case surface_val::ANY: return "any";
        case surface_val::EVERY: return "every";
        default: break;
    }
    cata_fatal( "Invalid surface val" );
}

template<>
std::string enum_to_string<balance_val>( balance_val val )
{
    switch( val ) {
        case balance_val::CLUMSY: return "clumsy";
        case balance_val::UNEVEN: return "uneven";
        case balance_val::NEUTRAL: return "neutral";
        case balance_val::GOOD: return "good";
        default: break;
    }
    cata_fatal( "Invalid balance val" );
}

struct acc_data {
    grip_val grip = grip_val::WEAPON;
    length_val length = length_val::HAND;
    surface_val surface = surface_val::ANY;
    balance_val balance = balance_val::NEUTRAL;

    // all items have a basic accuracy of -2, per GAME_BALANCE.md
    static constexpr int base_acc = -2;
    // grip val should go from -1 to 2 but enum_to_string wants to start at 0
    static constexpr int grip_offset = -1;
    // surface val should from from -2 to 1 but enum_to_string wants to start at 0
    static constexpr int surface_offset = -2;
    // balance val should from from -2 to 1 but enum_to_string wants to start at 0
    static constexpr int balance_offset = -2;
    // all the constant offsets and the base accuracy together
    static constexpr int acc_offset = base_acc + grip_offset + surface_offset + balance_offset;
    int sum_values() const {
        return acc_offset + static_cast<int>( grip ) + static_cast<int>( length ) +
               static_cast<int>( surface ) + static_cast<int>( balance );
    }
    void deserialize(const JsonObject& jo);
    void load( const JsonObject &jo );
};

void acc_data::deserialize( const JsonObject& jo )
{
    load( jo );
}

void acc_data::load( const JsonObject &jo )
{
    bool was_loaded = false;
    optional( jo, was_loaded, "grip", grip, grip_val::WEAPON );
    optional( jo, was_loaded, "length", length, length_val::HAND );
    optional( jo, was_loaded, "surface", surface, surface_val::ANY );
    optional( jo, was_loaded, "balance", balance, balance_val::NEUTRAL );
}
// *INDENT-ON*
} // namespace io

static void migrate_mag_from_pockets( itype &def )
{
    for( const pocket_data &pocket : def.pockets ) {
        if( pocket.type == item_pocket::pocket_type::MAGAZINE_WELL ) {
            if( def.gun ) {
                for( const ammotype &atype : def.gun->ammo ) {
                    def.magazine_default.emplace( atype, pocket.default_magazine );
                }
            }
            if( def.magazine ) {
                for( const ammotype &atype : def.magazine->type ) {
                    def.magazine_default.emplace( atype, pocket.default_magazine );
                }
            }
            if( def.tool ) {
                for( const ammotype &atype : def.tool->ammo_id ) {
                    def.magazine_default.emplace( atype, pocket.default_magazine );
                }
            }
        }
    }
}

static void replace_materials( const JsonObject &jo, itype &def )
{
    // itterate through material replacements
    for( JsonMember jv : jo ) {
        material_id to_replace = material_id( jv.name() );
        material_id replacement = material_id( jv.get_string() );

        units::mass original_weight = def.weight;
        int mat_portion = def.materials[to_replace];

        // make repairs_with act the same way
        if( !def.repairs_with.empty() ) {
            const auto iter = def.repairs_with.find( to_replace );
            if( iter != def.repairs_with.end() ) {
                def.repairs_with.erase( iter );
                def.repairs_with.insert( replacement );
            }
        }

        // material is defined
        if( mat_portion != 0 ) {
            double mat_percent = static_cast<double>( mat_portion ) / static_cast<double>
                                 ( def.mat_portion_total );

            float density_percent = replacement->density() / to_replace->density();

            // add the portion for the replaced material
            def.materials[replacement] += mat_portion;

            // delete the old data
            def.materials.erase( to_replace );

            // change the weight relative
            units::mass material_weight = original_weight * mat_percent;
            units::mass weight_dif = material_weight * density_percent - material_weight;

            def.weight += weight_dif;
        }

        // update the armor defs
        if( def.armor ) {
            for( armor_portion_data &data : def.armor->sub_data ) {
                std::vector<part_material>::iterator entry;
                do {
                    entry = std::find_if( data.materials.begin(),
                    data.materials.end(), [to_replace]( const part_material & pm ) {
                        return pm.id == to_replace;
                    } );

                    if( entry != data.materials.end() ) {
                        entry->id = replacement;
                    }

                } while( entry != data.materials.end() );
            }
        }
    }
}

void Item_factory::load_basic_info( const JsonObject &jo, itype &def, const std::string &src )
{
    bool strict = src == "dda";

    restore_on_out_of_scope<check_plural_t> restore_check_plural( check_plural );
    if( jo.has_string( "abstract" ) ) {
        check_plural = check_plural_t::none;
    }

    assign( jo, "category", def.category_force, strict );
    assign( jo, "weight", def.weight, strict, 0_gram );
    assign( jo, "integral_weight", def.integral_weight, strict, 0_gram );
    assign( jo, "volume", def.volume );
    assign( jo, "longest_side", def.longest_side );
    assign( jo, "price", def.price, false, 0_cent );
    assign( jo, "price_postapoc", def.price_post, false, 0_cent );
    assign( jo, "stackable", def.stackable_, strict );
    assign( jo, "integral_volume", def.integral_volume );
    assign( jo, "integral_longest_side", def.integral_longest_side, false, 0_mm );
    if( jo.has_object( "melee_damage" ) ) {
        def.melee = load_damage_map( jo.get_object( "melee_damage" ) );
    }
    def.melee_proportional.clear();
    if( jo.has_object( "proportional" ) ) {
        JsonObject jprop = jo.get_object( "proportional" );
        jprop.allow_omitted_members();
        if( jprop.has_object( "melee_damage" ) ) {
            def.melee_proportional = load_damage_map( jprop.get_object( "melee_damage" ) );
        }
    }
    def.melee_relative.clear();
    if( jo.has_object( "relative" ) ) {
        JsonObject jrel = jo.get_object( "relative" );
        jrel.allow_omitted_members();
        if( jrel.has_object( "melee_damage" ) ) {
            def.melee_relative = load_damage_map( jrel.get_object( "melee_damage" ) );
        }
    }
    if( jo.has_int( "to_hit" ) ) {
        assign( jo, "to_hit", def.m_to_hit, strict );
    } else if( jo.has_object( "to_hit" ) ) {
        io::acc_data temp;
        bool was_loaded = false;
        mandatory( jo, was_loaded, "to_hit", temp );
        def.m_to_hit = temp.sum_values();
    }
    optional( jo, false, "variant_type", def.variant_kind, itype_variant_kind::generic );
    optional( jo, false, "variants", def.variants );
    assign( jo, "container", def.default_container );
    assign( jo, "sealed", def.default_container_sealed );
    assign( jo, "min_strength", def.min_str );
    assign( jo, "min_dexterity", def.min_dex );
    assign( jo, "min_intelligence", def.min_int );
    assign( jo, "min_perception", def.min_per );
    assign( jo, "emits", def.emits );
    assign( jo, "explode_in_fire", def.explode_in_fire );
    assign( jo, "insulation", def.insulation_factor );
    assign( jo, "solar_efficiency", def.solar_efficiency );
    assign( jo, "ascii_picture", def.picture_id );
    assign( jo, "repairs_with", def.repairs_with );

    if( jo.has_member( "thrown_damage" ) ) {
        def.thrown_damage = load_damage_instance( jo.get_array( "thrown_damage" ) );
    } else {
        // TODO: Move to finalization
        def.thrown_damage.clear();
        def.thrown_damage.add_damage( damage_bash,
                                      def.melee[damage_bash] + def.weight / 1.0_kilogram );
    }

    if( jo.has_member( "repairs_like" ) ) {
        jo.read( "repairs_like", def.repairs_like );
    }

    optional( jo, true, "weapon_category", def.weapon_category, auto_flags_reader<weapon_category_id> {} );

    float degrade_mult = 1.0f;
    optional( jo, false, "degradation_multiplier", degrade_mult, 1.0f );
    // TODO: remove condition once degradation is ready to be applied to all items
    if( def.count_by_charges() || def.category_force != item_category_veh_parts ) {
        degrade_mult = 0.0f;
    }
    if( ( degrade_mult * itype::damage_max_ ) <= 0.5f ) {
        def.degrade_increments_ = 0;
    } else {
        float adjusted_inc = std::max( def.degrade_increments_ / degrade_mult, 1.0f );
        def.degrade_increments_ = std::isnan( adjusted_inc ) ? 0 : std::round( adjusted_inc );
    }

    // NOTE: please also change `needs_plural` in `lang/extract_json_string.py`
    // when changing this list
    static const std::set<std::string> needs_plural = {
        "AMMO",
        "ARMOR",
        "BATTERY",
        "BIONIC_ITEM",
        "BOOK",
        "COMESTIBLE",
        "CONTAINER",
        "ENGINE",
        "GENERIC",
        "GUN",
        "GUNMOD",
        "MAGAZINE",
        "PET_ARMOR",
        "TOOL",
        "TOOLMOD",
        "TOOL_ARMOR",
        "WHEEL",
    };
    if( needs_plural.find( jo.get_string( "type" ) ) != needs_plural.end() ) {
        def.name = translation( translation::plural_tag() );
    } else {
        def.name = translation();
    }
    if( !jo.read( "name", def.name ) ) {
        jo.throw_error( "name unspecified for item type" );
    }

    if( jo.has_member( "description" ) ) {
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
        def.mat_portion_total = 0;
        auto add_mat = [&def]( const material_id & m, int portion ) {
            const auto res = def.materials.emplace( m, portion );
            if( res.second ) {
                def.mat_portion_total += portion;
            }
        };
        bool first = true;
        if( jo.has_array( "material" ) && jo.get_array( "material" ).test_object() ) {
            for( JsonObject mat : jo.get_array( "material" ) ) {
                add_mat( material_id( mat.get_string( "type" ) ), mat.get_int( "portion", 1 ) );
                if( first ) {
                    def.default_mat = material_id( mat.get_string( "type" ) );
                    first = false;
                }
            }
        } else {
            for( const std::string &mat : jo.get_tags( "material" ) ) {
                add_mat( material_id( mat ), 1 );
                if( first ) {
                    def.default_mat = material_id( mat );
                    first = false;
                }
            }
        }
    }

    if( jo.has_member( "chat_topics" ) ) {
        def.chat_topics.clear();
        for( const std::string &m : jo.get_string_array( "chat_topics" ) ) {
            def.chat_topics.emplace_back( m );
        }
    }
    if( jo.has_string( "phase" ) ) {
        def.phase = jo.get_enum_value<phase_id>( "phase" );
    }

    if( jo.has_array( "magazines" ) ) {
        def.magazine_default.clear();
        def.magazines.clear();

        for( JsonArray arr : jo.get_array( "magazines" ) ) {
            ammotype ammo( arr.get_string( 0 ) ); // an ammo type (e.g. 9mm)
            JsonArray compat = arr.get_array( 1 ); // compatible magazines for this ammo type

            // the first magazine for this ammo type is the default
            def.magazine_default[ ammo ] = itype_id( compat.get_string( 0 ) );

            while( compat.has_more() ) {
                def.magazines[ ammo ].insert( itype_id( compat.next_string() ) );
            }
        }
    }

    if( jo.has_string( "nanofab_template_group" ) ) {
        def.nanofab_template_group = item_group_id( jo.get_string( "nanofab_template_group" ) );
    }

    if( jo.has_string( "template_requirements" ) ) {
        def.template_requirements = requirement_id( jo.get_string( "template_requirements" ) );
    }

    JsonArray jarr = jo.get_array( "min_skills" );
    if( !jarr.empty() ) {
        def.min_skills.clear();
    }
    for( JsonArray cur : jarr ) {
        const skill_id sk = skill_id( cur.get_string( 0 ) );
        if( !sk.is_valid() ) {
            jo.throw_error_at( "min_skills", string_format( "invalid skill: %s", sk.c_str() ) );
        }
        def.min_skills[ sk ] = cur.get_int( 1 );
    }

    if( jo.has_member( "explosion" ) ) {
        JsonObject je = jo.get_object( "explosion" );
        def.explosion = load_explosion_data( je );
    }

    if( jo.has_member( "memory_card" ) ) {
        def.memory_card_data = cata::make_value<memory_card_info>();
        load_memory_card_data( *def.memory_card_data, jo.get_object( "memory_card" ) );
    }

    assign( jo, "variables", def.item_variables );
    assign( jo, "flags", def.item_tags );
    if( jo.has_member( "source_monster" ) ) {
        assign( jo, "source_monster", def.source_monster );
    }
    assign( jo, "faults", def.faults );

    if( jo.has_member( "qualities" ) ) {
        def.qualities.clear();
        set_qualities_from_json( jo, "qualities", def );
    } else {
        if( jo.has_object( "extend" ) ) {
            JsonObject tmp = jo.get_object( "extend" );
            tmp.allow_omitted_members();
            extend_qualities_from_json( tmp, "qualities", def );
        }
        if( jo.has_object( "delete" ) ) {
            JsonObject tmp = jo.get_object( "delete" );
            tmp.allow_omitted_members();
            delete_qualities_from_json( tmp, "qualities", def );
        }
        if( jo.has_object( "relative" ) ) {
            JsonObject tmp = jo.get_object( "relative" );
            tmp.allow_omitted_members();
            relative_qualities_from_json( tmp, "qualities", def );
        }
    }

    if( jo.has_member( "charged_qualities" ) ) {
        def.charged_qualities.clear();
        set_qualities_from_json( jo, "charged_qualities", def );
    }

    if( jo.has_member( "properties" ) ) {
        set_properties_from_json( jo, "properties", def );
    }

    if( jo.has_member( "techniques" ) ) {
        def.techniques.clear();
        set_techniques_from_json( jo, "techniques", def );
    } else {
        if( jo.has_object( "extend" ) ) {
            JsonObject tmp = jo.get_object( "extend" );
            tmp.allow_omitted_members();
            extend_techniques_from_json( tmp, "techniques", def );
        }
        if( jo.has_object( "delete" ) ) {
            JsonObject tmp = jo.get_object( "delete" );
            tmp.allow_omitted_members();
            delete_techniques_from_json( tmp, "techniques", def );
        }
    }

    set_use_methods_from_json( jo, "use_action", def.use_methods, def.ammo_scale );
    set_use_methods_from_json( jo, "tick_action", def.tick_action, def.ammo_scale );

    assign( jo, "countdown_interval", def.countdown_interval );
    assign( jo, "revert_to", def.revert_to, strict );

    if( jo.has_string( "countdown_action" ) ) {
        def.countdown_action = usage_from_string( jo.get_string( "countdown_action" ) );

    } else if( jo.has_object( "countdown_action" ) ) {
        JsonObject tmp = jo.get_object( "countdown_action" );
        use_function fun = usage_from_object( tmp ).second;
        if( fun ) {
            def.countdown_action = fun;
        }
    }

    if( jo.has_string( "drop_action" ) ) {
        def.drop_action = usage_from_string( jo.get_string( "drop_action" ) );

    } else if( jo.has_object( "drop_action" ) ) {
        JsonObject tmp = jo.get_object( "drop_action" );
        use_function fun = usage_from_object( tmp ).second;
        if( fun ) {
            def.drop_action = fun;
        }
    }

    jo.read( "looks_like", def.looks_like );

    if( jo.has_member( "conditional_names" ) ) {
        def.conditional_names.clear();
        for( const JsonObject curr : jo.get_array( "conditional_names" ) ) {
            conditional_name cname;
            cname.type = curr.get_enum_value<condition_type>( "type" );
            cname.condition = curr.get_string( "condition" );
            cname.name = translation( translation::plural_tag() );
            cname.value = curr.get_string( "value", "" );
            if( !curr.read( "name", cname.name ) ) {
                curr.throw_error( "name unspecified for conditional name" );
            }
            def.conditional_names.push_back( cname );
        }
    }

    assign( jo, "armor_data", def.armor, src == "dda" );
    assign( jo, "pet_armor_data", def.pet_armor, src == "dda" );
    assign( jo, "book_data", def.book, src == "dda" );
    load_slot_optional( def.gun, jo, "gun_data", src );
    load_slot_optional( def.bionic, jo, "bionic_data", src );
    assign( jo, "ammo_data", def.ammo, src == "dda" );
    assign( jo, "seed_data", def.seed, src == "dda" );
    assign( jo, "brewable", def.brewable, src == "dda" );
    load_slot_optional( def.relic_data, jo, "relic_data", src );
    assign( jo, "milling", def.milling_data, src == "dda" );

    // optional gunmod slot may also specify mod data
    if( jo.has_member( "gunmod_data" ) ) {
        // use the same JsonObject for the two load_slot calls to avoid
        // warnings about unvisited Json members
        JsonObject jo_gunmod = jo.get_object( "gunmod_data" );
        load_slot( def.gunmod, jo_gunmod, src );
        load_slot( def.mod, jo_gunmod, src );
    }

    if( jo.has_string( "abstract" ) ) {
        jo.read( "abstract", def.id, true );
    } else {
        jo.read( "id", def.id, true );
    }

    assign( jo, "pocket_data", def.pockets );
    check_and_create_magazine_pockets( def );
    add_special_pockets( def );

    if( !def.src.empty() && def.src.back().first != def.id ) {
        def.src.clear();
    }
    def.src.emplace_back( def.id, mod_id( src ) );

    if( def.magazines.empty() ) {
        migrate_mag_from_pockets( def );
    }
    if( def.magazine && def.magazine->capacity == 0 ) {
        int largest = 0;
        for( pocket_data &pocket : def.pockets ) {
            for( const ammotype &atype : def.magazine->type ) {
                int current = pocket.ammo_restriction[atype];
                largest = largest < current ? current : largest;
            }
        }
        def.magazine->capacity = largest;
    }

    // snippet_category should be loaded after def.id is determined
    if( jo.has_array( "snippet_category" ) ) {
        // auto-create a category that is unlikely to already be used and put the
        // snippets in it.
        def.snippet_category = "auto:" + def.id.str();
        SNIPPET.add_snippets_from_json( def.snippet_category, jo.get_array( "snippet_category" ) );
    } else {
        def.snippet_category = jo.get_string( "snippet_category", "" );
    }


    // potentially replace materials and update their values
    JsonObject replace_val = jo.get_object( "replace_materials" );
    replace_val.allow_omitted_members();
    replace_materials( replace_val, def );

    if( jo.has_string( "abstract" ) ) {
        m_abstracts[ def.id ] = def;
    } else {
        if( m_templates.count( def.id ) != 0 ) {
            mod_tracker::check_duplicate_entries( m_templates[def.id], def );
        }
        m_templates[ def.id ] = def;
    }
}

void Item_factory::add_migration( const migration &m )
{
    auto it = migrations.find( m.id );
    if( it == migrations.end() ) {
        migrations[m.id] = {m};
        return;
    }

    for( migration &old : it->second ) {
        if( old.from_variant ) {
            continue;
        }
        // If we find one that isn't from variant, overwrite it
        old = m;
        return;
    }

    // Otherwise, we're specifying a new one.
    it->second.push_back( m );
}

void Item_factory::load_migration( const JsonObject &jo )
{
    migration m;
    assign( jo, "replace", m.replace );
    assign( jo, "variant", m.variant );
    assign( jo, "from_variant", m.from_variant );
    assign( jo, "flags", m.flags );
    assign( jo, "charges", m.charges );
    assign( jo, "contents", m.contents );
    assign( jo, "sealed", m.sealed );
    assign( jo, "reset_item_vars", m.reset_item_vars );

    std::vector<itype_id> ids;
    if( jo.has_string( "id" ) ) {
        ids.resize( 1 );
        jo.read( "id", ids[0], true );
    } else if( jo.has_array( "id" ) ) {
        jo.read( "id", ids, true );
    } else {
        jo.throw_error( "`id` of `MIGRATION` is neither string nor array" );
    }
    for( const itype_id &id : ids ) {
        if( m.replace && m.replace == id ) {
            jo.throw_error( string_format( "`MIGRATION` attempting to replace entity with itself: %s",
                                           id.str() ) );
        }
        m.id = id;
        if( m.from_variant ) {
            migrations[ m.id ].push_back( m );
        } else {
            add_migration( m );
        }
    }
}

bool migration::content::operator==( const content &rhs ) const
{
    return id == rhs.id && count == rhs.count;
}

void migration::content::deserialize( const JsonObject &jsobj )
{
    jsobj.get_member( "id" ).read( id );
    jsobj.get_member( "count" ).read( count );
}

itype_id Item_factory::migrate_id( const itype_id &id )
{
    auto iter = migrations.find( id );
    if( iter == migrations.end() ) {
        return id;
    }
    const migration *parent = nullptr;
    for( const migration &m : iter->second ) {
        // from-variant migrations do not apply to itypes
        if( m.from_variant ) {
            continue;
        }
        parent = &m;
        break;
    }
    return parent != nullptr ? parent->replace : id;
}

void Item_factory::migrate_item( const itype_id &id, item &obj )
{
    auto iter = migrations.find( id );
    if( iter == migrations.end() ) {
        return;
    }
    bool convert = false;
    const migration *migrant = nullptr;
    for( const migration &m : iter->second ) {
        if( m.from_variant && obj.has_itype_variant() && obj.itype_variant().id == *m.from_variant ) {
            migrant = &m;
            // This is not the variant that the item has already been convert to
            // So we'll convert it again.
            convert = true;
            break;
        }
        // When we find a migration that doesn't care about variants, keep it around
        if( !m.from_variant ) {
            migrant = &m;
        }
    }
    if( migrant == nullptr ) {
        return;
    }

    if( convert ) {
        obj.convert( migrant->replace );
    }

    if( migrant->reset_item_vars ) {
        obj.clear_vars();
        for( const auto &pair : migrant->replace.obj().item_variables ) {
            obj.set_var( pair.first, pair.second ) ;
        }
    }
    for( const std::string &f : migrant->flags ) {
        obj.set_flag( flag_id( f ) );
    }
    if( migrant->charges > 0 ) {
        obj.charges = migrant->charges;
    }

    if( migrant->from_variant ) {
        obj.clear_itype_variant();
    }
    obj.set_itype_variant( migrant->variant );

    for( const migration::content &it : migrant->contents ) {
        int count = it.count;
        item content( it.id, obj.birthday(), 1 );
        if( content.count_by_charges() ) {
            content.charges = count;
            count = 1;
        }
        for( ; count > 0; --count ) {
            if( !obj.put_in( content, item_pocket::pocket_type::CONTAINER ).success() ) {
                obj.put_in( content, item_pocket::pocket_type::MIGRATION );
            }
        }
    }

    if( !migrant->contents.empty() && migrant->sealed ) {
        obj.seal();
    }
}

void Item_factory::set_qualities_from_json( const JsonObject &jo, const std::string &member,
        itype &def )
{
    if( jo.has_array( member ) ) {
        for( JsonArray curr : jo.get_array( member ) ) {
            const auto quali = std::pair<quality_id, int>( quality_id( curr.get_string( 0 ) ),
                               curr.get_int( 1 ) );
            // Populate charged qualities or regular qualities, preventing duplicates
            if( member == "charged_qualities" ) {
                if( def.charged_qualities.count( quali.first ) > 0 ) {
                    curr.throw_error( 0, "Duplicated charged quality" );
                }
                def.charged_qualities.insert( quali );
            } else {
                if( def.qualities.count( quali.first ) > 0 ) {
                    curr.throw_error( 0, "Duplicated quality" );
                }
                def.qualities.insert( quali );
            }
        }
    } else {
        jo.throw_error_at( member, "Qualities list is not an array" );
    }
}

void Item_factory::extend_qualities_from_json( const JsonObject &jo, const std::string_view member,
        itype &def )
{
    for( JsonArray curr : jo.get_array( member ) ) {
        def.qualities[quality_id( curr.get_string( 0 ) )] = curr.get_int( 1 );
    }
}

void Item_factory::delete_qualities_from_json( const JsonObject &jo, const std::string_view member,
        itype &def )
{
    for( std::string curr : jo.get_array( member ) ) {
        const auto iter = def.qualities.find( quality_id( curr ) );
        if( iter != def.qualities.end() ) {
            def.qualities.erase( iter );
        }
    }
}

void Item_factory::relative_qualities_from_json( const JsonObject &jo,
        const std::string_view member, itype &def )
{
    for( JsonArray curr : jo.get_array( member ) ) {
        const quality_id key = quality_id( curr.get_string( 0 ) );
        if( def.qualities.find( quality_id( key ) ) != def.qualities.end() ) {
            def.qualities[ key ] += curr.get_int( 1 );
        } else {
            jo.throw_error_at( member, "Quality specified wasn't inherited" );
        }
    }
}

void Item_factory::set_properties_from_json( const JsonObject &jo, const std::string_view member,
        itype &def )
{
    if( jo.has_array( member ) ) {
        for( JsonArray curr : jo.get_array( member ) ) {
            const auto prop = std::pair<std::string, std::string>( curr.get_string( 0 ), curr.get_string( 1 ) );
            if( def.properties.count( prop.first ) > 0 ) {
                curr.throw_error( 0, "Duplicated property" );
            }
            def.properties.insert( prop );
        }
    } else {
        jo.throw_error_at( member, "Properties list is not an array" );
    }
}

void Item_factory::set_techniques_from_json( const JsonObject &jo, const std::string_view &member,
        itype &def )
{
    if( jo.has_array( member ) ) {
        for( std::string curr : jo.get_array( member ) ) {
            const matec_id tech = matec_id( curr );
            // Prevent duplicates
            if( def.techniques.count( tech ) > 0 ) {
                jo.throw_error_at( member, "Duplicated technique" );
            }
            def.techniques.insert( tech );
        }
    } else {
        jo.throw_error_at( member, "Techniques list is not an array" );
    }
}

void Item_factory::extend_techniques_from_json( const JsonObject &jo, const std::string_view member,
        itype &def )
{
    for( std::string curr : jo.get_array( member ) ) {
        def.techniques.insert( matec_id( curr ) );
    }
}

void Item_factory::delete_techniques_from_json( const JsonObject &jo, const std::string_view member,
        itype &def )
{
    for( std::string curr : jo.get_array( member ) ) {
        const matec_id tech = matec_id( curr );
        const auto iter = def.techniques.find( tech );
        if( iter != def.techniques.end() ) {
            def.techniques.erase( tech );
        }
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

    iuse_function_list.clear();

    m_templates.clear();
    m_runtimes.clear();

    item_blacklist.clear();

    tool_subtypes.clear();

    repair_tools.clear();
    gun_tools.clear();
    repair_actions.clear();

    migrated_ammo.clear();
    migrated_magazines.clear();
    migrations.clear();

    deferred.clear();

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

static Item_group *make_group_or_throw(
    const item_group_id &group_id, std::unique_ptr<Item_spawn_data> &isd, Item_group::Type t,
    int ammo_chance, int magazine_chance, const std::string &context )
{
    Item_group *ig = dynamic_cast<Item_group *>( isd.get() );
    if( ig == nullptr ) {
        isd.reset( ig = new Item_group( t, 100, ammo_chance, magazine_chance, context ) );
    } else if( ig->type != t ) {
        throw std::runtime_error(
            "item group \"" + group_id.str() + "\" already defined with type \"" +
            to_string( ig->type ) + "\"" );
    }
    return ig;
}

template<typename T>
bool load_min_max( std::pair<T, T> &pa, const JsonObject &obj, const std::string &name )
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

template<typename T>
bool load_str_arr( std::vector<T> &arr, const JsonObject &obj, const std::string_view name )
{
    if( obj.has_array( name ) ) {
        for( const std::string str : obj.get_array( name ) ) {
            arr.emplace_back( str );
        }
        return true;
    }
    return false;
}

bool Item_factory::load_sub_ref( std::unique_ptr<Item_spawn_data> &ptr, const JsonObject &obj,
                                 const std::string &name, const Item_group &parent )
{
    const std::string iname( name + "-item" );
    const std::string gname( name + "-group" );

    // pair.second is true for groups, false for items
    std::vector< std::pair<std::string, bool> > entries;
    const int prob = 100;

    auto get_array = [&obj, &name, &entries]( const std::string & arr_name, const bool isgroup ) {
        if( !obj.has_array( arr_name ) ) {
            return;
        } else if( name != "contents" ) {
            obj.throw_error( string_format( "You can't use an array for '%s'", arr_name ) );
        }
        for( const std::string line : obj.get_array( arr_name ) ) {
            entries.emplace_back( line, isgroup );
        }
    };
    get_array( iname, false );
    get_array( gname, true );

    if( obj.has_member( name ) ) {
        obj.throw_error( string_format( "This has been a TODO: since 2014.  Use '%s' and/or '%s' instead.",
                                        iname, gname ) );
    }
    if( obj.has_string( iname ) ) {
        entries.emplace_back( obj.get_string( iname ), false );
    }
    if( obj.has_string( gname ) ) {
        entries.emplace_back( obj.get_string( gname ), true );
    }

    const std::string subcontext = name + " of " + parent.context();

    if( entries.size() > 1 && name != "contents" ) {
        obj.throw_error( string_format( "You can only use one of '%s' and '%s'", iname, gname ) );
    } else if( entries.size() == 1 ) {
        const Single_item_creator::Type type = entries.front().second ?
                                               Single_item_creator::Type::S_ITEM_GROUP :
                                               Single_item_creator::Type::S_ITEM;
        Single_item_creator *result =
            new Single_item_creator( entries.front().first, type, prob, subcontext );
        result->inherit_ammo_mag_chances( parent.with_ammo, parent.with_magazine );
        ptr.reset( result );
        return true;
    } else if( entries.empty() ) {
        return false;
    }
    Item_group *result = new Item_group( Item_group::Type::G_COLLECTION, prob, parent.with_ammo,
                                         parent.with_magazine, subcontext );
    ptr.reset( result );
    for( const auto &elem : entries ) {
        if( elem.second ) {
            result->add_group_entry( item_group_id( elem.first ), prob );
        } else {
            result->add_item_entry( itype_id( elem.first ), prob );
        }
    }
    return true;
}

bool Item_factory::load_string( std::vector<std::string> &vec, const JsonObject &obj,
                                const std::string_view name )
{
    bool result = false;
    std::string temp;

    if( obj.has_array( name ) ) {
        for( const std::string line : obj.get_array( name ) ) {
            result |= true;
            vec.push_back( line );
        }
    } else if( obj.has_member( name ) ) {
        result |= obj.read( name, temp );
        vec.push_back( temp );
    }

    return result;
}

void Item_factory::add_entry( Item_group &ig, const JsonObject &obj, const std::string &context )
{
    std::unique_ptr<Item_group> gptr;
    int probability = obj.get_int( "prob", 100 );
    holiday event = obj.get_enum_value<holiday>( "event", holiday::none );
    std::string subcontext = "entry within " + context;
    JsonArray jarr;
    if( obj.has_member( "collection" ) ) {
        gptr = std::make_unique<Item_group>( Item_group::G_COLLECTION, probability, ig.with_ammo,
                                             ig.with_magazine, context, event );
        jarr = obj.get_array( "collection" );
    } else if( obj.has_member( "distribution" ) ) {
        gptr = std::make_unique<Item_group>( Item_group::G_DISTRIBUTION, probability, ig.with_ammo,
                                             ig.with_magazine, context, event );
        jarr = obj.get_array( "distribution" );
    }
    if( gptr ) {
        for( const JsonObject job2 : jarr ) {
            add_entry( *gptr, job2, subcontext );
        }
        ig.add_entry( std::move( gptr ) );
        return;
    }

    std::unique_ptr<Single_item_creator> sptr;
    if( obj.has_member( "item" ) ) {
        sptr = std::make_unique<Single_item_creator>(
                   obj.get_string( "item" ), Single_item_creator::S_ITEM, probability, context, event );
    } else if( obj.has_member( "group" ) ) {
        sptr = std::make_unique<Single_item_creator>(
                   obj.get_string( "group" ), Single_item_creator::S_ITEM_GROUP, probability,
                   context, event );
    }
    if( !sptr ) {
        return;
    }

    if( obj.has_member( "artifact" ) ) {
        sptr->artifact = cata::make_value<Item_spawn_data::relic_generator>();
        sptr->artifact->load( obj.get_object( "artifact" ) );
    }

    Item_modifier modifier;
    bool use_modifier = false;
    use_modifier |= load_min_max( modifier.damage, obj, "damage" );
    use_modifier |= load_min_max( modifier.dirt, obj, "dirt" );
    modifier.damage.first *= itype::damage_scale;
    modifier.damage.second *= itype::damage_scale;
    use_modifier |= load_min_max( modifier.charges, obj, "charges" );
    use_modifier |= load_min_max( modifier.count, obj, "count" );
    use_modifier |= load_sub_ref( modifier.ammo, obj, "ammo", ig );
    use_modifier |= load_sub_ref( modifier.container, obj, "container", ig );
    use_modifier |= load_sub_ref( modifier.contents, obj, "contents", ig );
    use_modifier |= load_str_arr( modifier.snippets, obj, "snippets" );
    if( obj.has_member( "sealed" ) ) {
        modifier.sealed = obj.get_bool( "sealed" );
        use_modifier = true;
    }
    std::vector<std::string> custom_flags;
    use_modifier |= load_string( custom_flags, obj, "custom-flags" );
    modifier.custom_flags.clear();
    for( const auto &cf : custom_flags ) {
        modifier.custom_flags.emplace_back( cf );
    }
    if( obj.has_member( "variant" ) ) {
        modifier.variant = obj.get_string( "variant" );
        use_modifier = true;
    }

    if( use_modifier ) {
        sptr->modifier.emplace( std::move( modifier ) );
    }
    ig.add_entry( std::move( sptr ) );
}

// Load an item group from JSON
void Item_factory::load_item_group( const JsonObject &jsobj )
{
    const item_group_id group_id( jsobj.get_string( "id" ) );
    const std::string subtype = jsobj.get_string( "subtype", "old" );
    load_item_group( jsobj, group_id, subtype );
}

void Item_factory::load_item_group( const JsonArray &entries, const item_group_id &group_id,
                                    const bool is_collection, const int ammo_chance,
                                    const int magazine_chance, std::string context )
{
    if( context.empty() ) {
        context = group_id.str();
    }
    const Item_group::Type type = is_collection ? Item_group::G_COLLECTION : Item_group::G_DISTRIBUTION;
    std::unique_ptr<Item_spawn_data> &isd = m_template_groups[group_id];
    Item_group *const ig =
        make_group_or_throw( group_id, isd, type, ammo_chance, magazine_chance, context );

    for( const JsonObject subobj : entries ) {
        add_entry( *ig, subobj, "entry within " + ig->context() );
    }
}

void Item_factory::load_item_group( const JsonObject &jsobj, const item_group_id &group_id,
                                    const std::string &subtype, std::string context )
{
    if( context.empty() ) {
        context = group_id.str();
    }
    std::unique_ptr<Item_spawn_data> &isd = m_template_groups[group_id];
    // If we copy-from, do copy-from
    // Otherwise, unconditionally overwrite
    if( jsobj.has_member( "copy-from" ) ) {
        // We can only copy-from a group with the same id (for now)
        if( jsobj.get_string( "copy-from" ) != group_id.str() ) {
            debugmsg( "Item group '%s' tries to copy from different group '%s'", group_id.str(),
                      jsobj.get_string( "copy-from" ) );
            return;
        }
    } else {
        isd.reset();
    }

    Item_group::Type type = Item_group::G_COLLECTION;
    if( subtype == "old" || subtype == "distribution" ) {
        type = Item_group::G_DISTRIBUTION;
    } else if( subtype != "collection" ) {
        jsobj.throw_error_at( "subtype", "unknown item group type" );
    }
    Item_group *ig = make_group_or_throw( group_id, isd, type, jsobj.get_int( "ammo", 0 ),
                                          jsobj.get_int( "magazine", 0 ), context );

    // If it extends, read from the extends block (and extend our itemgroup)
    // Otherwise, read from the object into the fresh itemgroup
    // And don't read if it has copy-from because it will not handle that correctly.
    if( jsobj.has_member( "extend" ) ) {
        load_item_group_data( jsobj.get_object( "extend" ), ig, subtype );
    } else if( !jsobj.has_member( "copy-from" ) ) {
        load_item_group_data( jsobj, ig, subtype );
    }

    if( jsobj.has_string( "container-item" ) ) {
        ig->set_container_item( itype_id( jsobj.get_string( "container-item" ) ) );
    }

    jsobj.read( "on_overflow", ig->on_overflow, false );
    if( jsobj.has_member( "sealed" ) ) {
        ig->sealed = jsobj.get_bool( "sealed" );
    }
}

void Item_factory::load_item_group_data( const JsonObject &jsobj, Item_group *ig,
        const std::string &subtype )
{
    if( subtype == "old" ) {
        for( const JsonValue entry : jsobj.get_array( "items" ) ) {
            if( entry.test_object() ) {
                JsonObject subobj = entry.get_object();
                std::string subcontext;
                if( subobj.has_string( "item" ) ) {
                    subcontext = "item " + subobj.get_string( "item" ) + " within " + ig->context();
                } else if( subobj.has_string( "group" ) ) {
                    subcontext = "group " + subobj.get_string( "group" ) + " within " + ig->context();
                } else if( subobj.has_member( "distribution" ) ) {
                    subcontext = "distribution within " + ig->context();
                } else if( subobj.has_member( "collection" ) ) {
                    subcontext = "collection within " + ig->context();
                } else {
                    debugmsg( "couldn't determine subcontext for " + subobj.str() );
                    subcontext = "item within " + ig->context();
                }
                add_entry( *ig, subobj, subcontext );
            } else {
                JsonArray pair = entry.get_array();
                ig->add_item_entry( itype_id( pair.get_string( 0 ) ), pair.get_int( 1 ) );
            }
        }
        return;
    }

    if( jsobj.has_member( "entries" ) ) {
        for( const JsonObject subobj : jsobj.get_array( "entries" ) ) {
            add_entry( *ig, subobj, "entry within " + ig->context() );
        }
    }
    if( jsobj.has_member( "items" ) ) {
        for( const JsonValue entry : jsobj.get_array( "items" ) ) {
            if( entry.test_string() ) {
                ig->add_item_entry( itype_id( entry.get_string() ), 100 );
            } else if( entry.test_array() ) {
                JsonArray subitem = entry.get_array();
                ig->add_item_entry( itype_id( subitem.get_string( 0 ) ), subitem.get_int( 1 ) );
            } else {
                JsonObject subobj = entry.get_object();
                add_entry( *ig, subobj, "item within " + ig->context() );
            }
        }
    }
    if( jsobj.has_member( "groups" ) ) {
        for( const JsonValue entry : jsobj.get_array( "groups" ) ) {
            if( entry.test_string() ) {
                ig->add_group_entry( item_group_id( entry.get_string() ), 100 );
            } else if( entry.test_array() ) {
                JsonArray subitem = entry.get_array();
                ig->add_group_entry( item_group_id( subitem.get_string( 0 ) ),
                                     subitem.get_int( 1 ) );
            } else {
                JsonObject subobj = entry.get_object();
                add_entry( *ig, subobj, "group within " + ig->context() );
            }
        }
    }
}

void Item_factory::set_use_methods_from_json( const JsonObject &jo, const std::string &member,
        std::map<std::string, use_function> &use_methods, std::map<std::string, int> &ammo_scale )
{
    if( !jo.has_member( member ) ) {
        return;
    }

    use_methods.clear();
    ammo_scale.clear();
    if( jo.has_array( member ) ) {
        for( const JsonValue entry : jo.get_array( member ) ) {
            if( entry.test_string() ) {
                std::string type = entry.get_string();
                emplace_usage( use_methods, type );
            } else if( entry.test_object() ) {
                JsonObject obj = entry.get_object();
                std::pair<std::string, use_function> fun = usage_from_object( obj );
                if( fun.second ) {
                    use_methods.insert( fun );
                    if( obj.has_int( "ammo_scale" ) ) {
                        ammo_scale.emplace( fun.first, static_cast<int>( obj.get_float( "ammo_scale" ) ) );
                    }
                }
            } else if( entry.test_array() ) {
                JsonArray curr = entry.get_array();
                std::string type = curr.get_string( 0 );
                emplace_usage( use_methods, type );
                if( curr.has_int( 1 ) ) {
                    ammo_scale.emplace( type, static_cast<int>( curr.get_float( 1 ) ) );
                }
            } else {
                entry.throw_error( "array element is neither string nor object." );
            }
        }
    } else {
        if( jo.has_string( member ) ) {
            std::string type = jo.get_string( member );
            emplace_usage( use_methods, type );
        } else if( jo.has_object( member ) ) {
            JsonObject obj = jo.get_object( member );
            std::pair<std::string, use_function> fun = usage_from_object( obj );
            if( fun.second ) {
                use_methods.insert( fun );
                if( obj.has_int( "ammo_scale" ) ) {
                    ammo_scale.emplace( fun.first, static_cast<int>( obj.get_float( "ammo_scale" ) ) );
                }
            }
        } else {
            jo.throw_error( "member 'use_action' is neither string nor object." );
        }

    }
}

// Helper to safely look up and store iuse actions.
void Item_factory::emplace_usage( std::map<std::string, use_function> &container,
                                  const std::string &iuse_id )
{
    use_function fun = usage_from_string( iuse_id );
    if( fun ) {
        container.emplace( iuse_id, fun );
    }
}

std::pair<std::string, use_function> Item_factory::usage_from_object( const JsonObject &obj )
{
    std::string type = obj.get_string( "type" );

    if( type == "repair_item" ) {
        type = obj.get_string( "item_action_type" );
        if( !has_iuse( type ) ) {
            add_actor( std::make_unique<repair_item_actor>( type ) );
            repair_actions.insert( type );
        }
    }

    use_function method = usage_from_string( type );

    if( !method.get_actor_ptr() ) {
        return std::make_pair( type, use_function() );
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
        case phase_id::PNULL: return "null";
        case phase_id::LIQUID: return "liquid";
        case phase_id::SOLID: return "solid";
        case phase_id::GAS: return "gas";
        case phase_id::PLASMA: return "plasma";
        // *INDENT-ON*
        case phase_id::num_phases:
            break;
    }
    cata_fatal( "Invalid phase" );
}
} // namespace io

item_category_id calc_category( const itype &obj )
{
    if( obj.gun && !obj.gunmod ) {
        return item_category_guns;
    }
    if( obj.magazine ) {
        return item_category_magazines;
    }
    if( obj.ammo ) {
        return item_category_ammo;
    }
    if( obj.tool ) {
        return item_category_tools;
    }
    if( obj.armor ) {
        return item_category_clothing;
    }
    if( obj.comestible ) {
        return obj.comestible->comesttype == "MED" ?
               item_category_drugs : item_category_food;
    }
    if( obj.book ) {
        return item_category_books;
    }
    if( obj.gunmod ) {
        return item_category_mods;
    }
    if( obj.bionic ) {
        return item_category_bionics;
    }

    bool weap = std::any_of( obj.melee.begin(), obj.melee.end(),
    []( const std::pair<const damage_type_id, float> &qty ) {
        return qty.second > MELEE_STAT;
    } );

    return weap ? item_category_weapons : item_category_other;
}

std::vector<item_group_id> Item_factory::get_all_group_names()
{
    std::vector<item_group_id> rval;
    for( GroupMap::value_type &group_pair : m_template_groups ) {
        rval.push_back( group_pair.first );
    }
    return rval;
}

bool Item_factory::add_item_to_group( const item_group_id &group_id, const itype_id &item_id,
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
    std::vector<item_group_id> groups = item_controller->get_all_group_names();
    uilist menu;
    menu.text = _( "Test which group?" );
    for( size_t i = 0; i < groups.size(); i++ ) {
        menu.entries.emplace_back( static_cast<int>( i ), true, -2, groups[i].str() );
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
            const ItemList items = items_from( groups[index], calendar::turn );
            for( const item &it : items ) {
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
            menu2.entries.emplace_back( static_cast<int>( menu2.entries.size() ), true, -2,
                                        string_format( _( "%d x %s" ), e.first, e.second ) );
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
    cata_assert( frozen );

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
