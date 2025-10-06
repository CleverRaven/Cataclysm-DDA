#include "item.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "activity_actor_definitions.h"
#include "ammo.h"
#include "avatar.h"
#include "bionics.h"
#include "body_part_set.h"
#include "bodypart.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "character.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "city.h"
#include "color.h"
#include "coordinates.h"
#include "craft_command.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "effect.h" // for weed_msg
#include "effect_source.h"
#include "enum_bitset.h"
#include "enums.h"
#include "explosion.h"
#include "faction.h"
#include "fault.h"
#include "field_type.h"
#include "fire.h"
#include "flag.h"
#include "flat_set.h"
#include "game.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "gun_mode.h"
#include "iexamine.h"
#include "inventory.h"
#include "item_category.h"
#include "item_factory.h"
#include "item_group.h"
#include "item_tname.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "line.h"
#include "localized_comparator.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "martialarts.h"
#include "material.h"
#include "math_defines.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "pocket_type.h"
#include "point.h"
#include "proficiency.h"
#include "projectile.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "relic.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "skill.h"
#include "stomach.h"
#include "string_formatter.h"
#include "subbodypart.h"
#include "text_snippets.h"
#include "trait_group.h"
#include "translation.h"
#include "translations.h"
#include "trap.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"
#include "weather_gen.h"
#include "weather_type.h"
#include "weighted_list.h"

static const std::string GUN_MODE_VAR_NAME( "item::mode" );

static const ammotype ammo_battery( "battery" );
static const ammotype ammo_bolt( "bolt" );
static const ammotype ammo_money( "money" );
static const ammotype ammo_plutonium( "plutonium" );

static const damage_type_id damage_bash( "bash" );

static const efftype_id effect_cig( "cig" );
static const efftype_id effect_shakes( "shakes" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_weed_high( "weed_high" );

static const fault_id fault_emp_reboot( "fault_emp_reboot" );
static const fault_id fault_overheat_safety( "fault_overheat_safety" );

static const furn_str_id furn_f_metal_smoking_rack_active( "f_metal_smoking_rack_active" );
static const furn_str_id furn_f_smoking_rack_active( "f_smoking_rack_active" );
static const furn_str_id furn_f_water_mill_active( "f_water_mill_active" );
static const furn_str_id furn_f_wind_mill_active( "f_wind_mill_active" );

static const gun_mode_id gun_mode_REACH( "REACH" );

static const item_category_id item_category_container( "container" );
static const item_category_id item_category_drugs( "drugs" );
static const item_category_id item_category_food( "food" );
static const item_category_id item_category_maps( "maps" );
static const item_category_id item_category_software( "software" );
static const item_category_id item_category_spare_parts( "spare_parts" );
static const item_category_id item_category_tools( "tools" );
static const item_category_id item_category_weapons( "weapons" );

static const itype_id itype_arredondo_chute( "arredondo_chute" );
static const itype_id itype_barrel_small( "barrel_small" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_blood( "blood" );
static const itype_id itype_brass_catcher( "brass_catcher" );
static const itype_id itype_bullet_crossbow( "bullet_crossbow" );
static const itype_id itype_cash_card( "cash_card" );
static const itype_id itype_corpse( "corpse" );
static const itype_id itype_corpse_generic_human( "corpse_generic_human" );
static const itype_id itype_craft( "craft" );
static const itype_id itype_disassembly( "disassembly" );
static const itype_id itype_efile_photos( "efile_photos" );
static const itype_id itype_efile_recipes( "efile_recipes" );
static const itype_id itype_hand_crossbow( "hand_crossbow" );
static const itype_id itype_joint_lit( "joint_lit" );
static const itype_id itype_power_cord( "power_cord" );
static const itype_id itype_stock_none( "stock_none" );
static const itype_id itype_tuned_mechanism( "tuned_mechanism" );
static const itype_id itype_water( "water" );
static const itype_id itype_water_clean( "water_clean" );
static const itype_id itype_waterproof_gunmod( "waterproof_gunmod" );

static const matec_id RAPID( "RAPID" );

static const material_id material_wool( "wool" );

static const mtype_id mon_human( "mon_human" );
static const mtype_id mon_zombie_smoker( "mon_zombie_smoker" );
static const mtype_id mon_zombie_soldier_no_weakpoints( "mon_zombie_soldier_no_weakpoints" );
static const mtype_id mon_zombie_survivor_no_weakpoints( "mon_zombie_survivor_no_weakpoints" );
static const mtype_id pseudo_debug_mon( "pseudo_debug_mon" );

static const quality_id qual_BOIL( "BOIL" );

static const skill_id skill_archery( "archery" );
static const skill_id skill_survival( "survival" );
static const skill_id skill_weapon( "weapon" );

static const species_id species_ROBOT( "ROBOT" );

static const trait_id trait_JITTERY( "JITTERY" );
static const trait_id trait_LIGHTWEIGHT( "LIGHTWEIGHT" );
static const trait_id trait_TOLERANCE( "TOLERANCE" );
static const trait_id trait_WOOLALLERGY( "WOOLALLERGY" );

// vitamin flags
static const std::string flag_NO_SELL( "NO_SELL" );

// fault flags
static const std::string flag_BLACKPOWDER_FOULING_DAMAGE( "BLACKPOWDER_FOULING_DAMAGE" );

// item pricing
static const int PRICE_FILTHY_MALUS = 100;  // cents

static constexpr float MIN_LINK_EFFICIENCY = 0.001f;

class npc_class;

using npc_class_id = string_id<npc_class>;

light_emission nolight = {0, 0, 0};

// Returns the default item type, used for the null item (default constructed),
// the returned pointer is always valid, it's never cleared by the @ref Item_factory.
static const itype *nullitem()
{
    static itype nullitem_m;
    return &nullitem_m;
}

item &null_item_reference()
{
    static item result{};
    // reset it, in case a previous caller has changed it
    result = item();
    return result;
}

namespace item_internal
{
static bool goes_bad_temp_cache = false;
static const item *goes_bad_temp_cache_for = nullptr;
static bool goes_bad_cache_fetch()
{
    return goes_bad_temp_cache;
}
static void goes_bad_cache_set( const item *i )
{
    goes_bad_temp_cache = i->goes_bad();
    goes_bad_temp_cache_for = i;
}
static void goes_bad_cache_unset()
{
    goes_bad_temp_cache = false;
    goes_bad_temp_cache_for = nullptr;
}
static bool goes_bad_cache_is_for( const item *i )
{
    return goes_bad_temp_cache_for == i;
}

struct scoped_goes_bad_cache {
    explicit scoped_goes_bad_cache( item *i ) {
        goes_bad_cache_set( i );
    }
    ~scoped_goes_bad_cache() {
        goes_bad_cache_unset();
    }
};
} // namespace item_internal

const int item::INFINITE_CHARGES = INT_MAX;

item::item() : bday( calendar::start_of_cataclysm )
{
    type = nullitem();
    charges = 0;
    contents = item_contents( type->pockets );
    select_itype_variant();
}

item::item( const itype *type, time_point turn, int qty ) : type( type ), bday( turn )
{
    contents = item_contents( type->pockets );
    if( type->countdown_interval > 0_seconds ) {
        countdown_point = calendar::turn + type->countdown_interval;
    }
    item_vars = type->item_variables;

    update_prefix_suffix_flags();
    if( has_flag( flag_CORPSE ) ) {
        corpse = &type->source_monster.obj();
        if( !type->source_monster.is_null() && !type->source_monster->zombify_into.is_empty() &&
            get_option<bool>( "ZOMBIFY_INTO_ENABLED" ) ) {
            set_var( "zombie_form", type->source_monster->zombify_into.c_str() );
        }
    }

    if( qty >= 0 ) {
        if( type->can_have_charges() ) {
            charges = qty;
        } else if( qty > 1 ) {
            debugmsg( "Tried to set charges for item %s that could not have them!", tname() );
        }
    } else {
        charges = type->charges_default();
    }

    if( has_flag( flag_SPAWN_ACTIVE ) ) {
        activate();
    }

    if( has_flag( flag_ENERGY_SHIELD ) ) {
        const islot_armor *sh = find_armor_data();
        set_var( "MAX_ENERGY_SHIELD_HP", sh->max_energy_shield_hp );
        set_var( "ENERGY_SHIELD_HP", sh->max_energy_shield_hp );
    }

    if( has_flag( flag_COLLAPSE_CONTENTS ) ) {
        for( item_pocket *pocket : contents.get_all_standard_pockets() ) {
            pocket->settings.set_collapse( true );
        }
    } else {
        auto const mag_filter = []( item_pocket const & pck ) {
            return pck.is_type( pocket_type::MAGAZINE ) ||
                   pck.is_type( pocket_type::MAGAZINE_WELL );
        };
        for( item_pocket *pocket : contents.get_pockets( mag_filter ) ) {
            pocket->settings.set_collapse( true );
        }
    }

    if( has_flag( flag_NANOFAB_TEMPLATE ) ) {
        itype_id nanofab_recipe =
            item_group::item_from( type->nanofab_template_group ).typeId();
        set_var( "NANOFAB_ITEM_ID", nanofab_recipe.str() );
    }

    if( type->trait_group.is_valid() ) {
        for( const trait_and_var &tr : trait_group::traits_from( type->trait_group ) ) {
            template_traits.push_back( tr.trait );
        }
    }

    select_itype_variant();
    if( type->gun ) {
        for( const itype_id &mod : type->gun->built_in_mods ) {
            item it( mod, turn, qty );
            it.set_flag( flag_IRREMOVABLE );
            put_in( it, pocket_type::MOD );
        }
        for( const itype_id &mod : type->gun->default_mods ) {
            put_in( item( mod, turn, qty ), pocket_type::MOD );
        }

    } else if( type->magazine ) {
        if( type->magazine->count > 0 ) {
            put_in( item( type->magazine->default_ammo, calendar::turn, type->magazine->count ),
                    pocket_type::MAGAZINE );
        }

    } else if( has_temperature() ) {
        active = true;
        last_temp_check = bday;

    } else if( type->tool ) {
        if( ammo_remaining( ) && !ammo_types().empty() ) {
            ammo_set( ammo_default(), ammo_remaining( ) );
        }
    }

    if( ( type->gun || type->tool ) && !magazine_integral() ) {
        set_var( "magazine_converted", 1 );
    }

    if( !type->snippet_category.empty() ) {
        snip_id = SNIPPET.random_id_from_category( type->snippet_category );
    }

    if( type->expand_snippets ) {
        set_var( "description", SNIPPET.expand( type->description.translated() ) );
    }

    if( has_itype_variant() && itype_variant().expand_snippets ) {
        set_var( "description", SNIPPET.expand( variant_description() ) );
    }

    if( current_phase == phase_id::PNULL ) {
        current_phase = type->phase;
    }
    // item always has any relic properties from itype.
    if( type->relic_data ) {
        relic_data = type->relic_data;
    }
}

item::item( const itype_id &id, time_point turn, int qty )
    : item( find_type( id ), turn, qty ) {}

item::item( const itype *type, time_point turn, default_charges_tag )
    : item( type, turn, type->charges_default() ) {}

item::item( const itype_id &id, time_point turn, default_charges_tag tag )
    : item( find_type( id ), turn, tag ) {}

item::item( const itype *type, time_point turn, solitary_tag )
    : item( type, turn, type->count_by_charges() ? 1 : -1 ) {}

item::item( const itype_id &id, time_point turn, solitary_tag tag )
    : item( find_type( id ), turn, tag ) {}

safe_reference<item> item::get_safe_reference()
{
    return anchor->reference_to( this );
}

static const item *get_most_rotten_component( const item &craft )
{
    const item *most_rotten = nullptr;
    for( const item_components::type_vector_pair &tvp : craft.components ) {
        if( !tvp.second.front().goes_bad() ) {
            // they're all the same type, so this should be the same for all
            continue;
        }
        for( const item &it : tvp.second ) {
            if( !most_rotten || it.get_relative_rot() > most_rotten->get_relative_rot() ) {
                most_rotten = &it;
            }
        }
    }
    return most_rotten;
}

static time_duration get_shortest_lifespan_from_components( const item &craft )
{
    const item *shortest_lifespan_component = nullptr;
    time_duration shortest_lifespan = 0_turns;
    for( const item_components::type_vector_pair &tvp : craft.components ) {
        if( !tvp.second.front().goes_bad() ) {
            // they're all the same type, so this should be the same for all
            continue;
        }
        for( const item &it : tvp.second ) {
            time_duration lifespan = it.get_shelf_life() - it.get_rot();
            if( !shortest_lifespan_component || ( lifespan < shortest_lifespan ) ) {
                shortest_lifespan_component = &it;
                shortest_lifespan = shortest_lifespan_component->get_shelf_life() -
                                    shortest_lifespan_component->get_rot();
            }
        }
    }
    return shortest_lifespan;
}

static bool shelf_life_less_than_each_component( const item &craft )
{
    for( const item_components::type_vector_pair &tvp : craft.components ) {
        if( !tvp.second.front().goes_bad() || !tvp.second.front().is_comestible() ) {
            // they're all the same type, so this should be the same for all
            continue;
        }
        for( const item &it : tvp.second ) {
            if( it.get_shelf_life() < craft.get_shelf_life() ) {
                return false;
            }
        }
    }
    return true;
}

// There are two ways rot is inherited:
// 1) Inherit the remaining lifespan of the component with the lowest remaining lifespan
// 2) Inherit the relative rot of the component with the highest relative rot
//
// Method 1 is used when the result of the recipe has a lower maximum shelf life than all of
// its component's maximum shelf lives. Relative rot is not good to inherit in this case because
// it can make an extremely short resultant remaining lifespan on the product.
//
// Method 2 is used when any component has a longer maximum shelf life than the result does.
// Inheriting the lowest remaining lifespan can not be used in this case because it would break
// food preservation recipes.
static void inherit_rot_from_components( item &it )
{
    if( shelf_life_less_than_each_component( it ) ) {
        const time_duration shortest_lifespan = get_shortest_lifespan_from_components( it );
        if( shortest_lifespan > 0_turns && shortest_lifespan < it.get_shelf_life() ) {
            it.set_rot( it.get_shelf_life() - shortest_lifespan );
            return;
        }
        // Fallthrough: shortest_lifespan <= 0_turns (all components are rotten)
    }

    const item *most_rotten = get_most_rotten_component( it );
    if( most_rotten ) {
        it.set_relative_rot( most_rotten->get_relative_rot() );
    }
}

item::item( const recipe *rec, int qty, item_components items, std::vector<item_comp> selections )
    : item( itype_craft, calendar::turn )
{
    craft_data_ = cata::make_value<craft_data>();
    craft_data_->making = rec;
    craft_data_->disassembly = false;
    craft_data_->batch_size = qty;
    components = std::move( items );
    craft_data_->comps_used = std::move( selections );

    if( has_temperature() ) {
        active = true;
        last_temp_check = bday;
        if( goes_bad() ) {
            inherit_rot_from_components( *this );
        }
    }

    for( item_components::type_vector_pair &tvp : components ) {
        for( item &component : tvp.second ) {
            for( const flag_id &f : component.get_flags() ) {
                if( f->craft_inherit() ) {
                    set_flag( f );
                }
            }
            for( const flag_id &f : component.type->get_flags() ) {
                if( f->craft_inherit() ) {
                    set_flag( f );
                }
            }
        }
    }
}

item::item( const recipe *rec, int qty, item &component )
    : item( itype_disassembly, calendar::turn )
{
    craft_data_ = cata::make_value<craft_data>();
    craft_data_->making = rec;
    craft_data_->disassembly = true;
    craft_data_->batch_size = qty;
    item_components items;
    items.add( component );
    components = items;

    if( has_temperature() ) {
        active = true;
        last_temp_check = bday;
        if( goes_bad() ) {
            inherit_rot_from_components( *this );
        }
    }

    for( item_components::type_vector_pair &tvp : components ) {
        for( item &comp : tvp.second ) {
            for( const flag_id &f : comp.get_flags() ) {
                if( f->craft_inherit() ) {
                    set_flag( f );
                }
            }
            for( const flag_id &f : comp.type->get_flags() ) {
                if( f->craft_inherit() ) {
                    set_flag( f );
                }
            }
        }
    }
}

item::item( const item & ) = default;
item::item( item && ) noexcept = default;
item::~item() = default;
item &item::operator=( const item & ) = default;
item &item::operator=( item && ) noexcept = default;

item item::make_corpse( const mtype_id &mt, time_point turn, const std::string &name,
                        const int upgrade_time )
{
    if( !mt.is_valid() ) {
        debugmsg( "tried to make a corpse with an invalid mtype id" );
    }

    itype_id corpse_type = mt == mtype_id::NULL_ID() ? itype_corpse_generic_human : itype_corpse;

    item result( corpse_type, turn );
    result.corpse = &mt.obj();

    if( result.corpse->has_flag( mon_flag_REVIVES ) ) {
        if( one_in( 20 ) ) {
            result.set_flag( flag_REVIVE_SPECIAL );
        }
        result.set_var( "upgrade_time", upgrade_time );
    }

    if( !mt->zombify_into.is_empty() && get_option<bool>( "ZOMBIFY_INTO_ENABLED" ) ) {
        result.set_var( "zombie_form", mt->zombify_into.c_str() );
    }

    // This is unconditional because the const itemructor above sets result.name to
    // "human corpse".
    result.corpse_name = name;

    return result;
}

item &item::convert( const itype_id &new_type, Character *carrier )
{
    // Carry over relative rot similar to crafting
    const double rel_rot = get_relative_rot();
    type = find_type( new_type );
    set_relative_rot( rel_rot );
    requires_tags_processing = true; // new type may have "active" flags
    item temp( *this );
    temp.contents = item_contents( type->pockets );
    for( const item *it : contents.mods() ) {
        if( !temp.put_in( *it, pocket_type::MOD ).success() ) {
            debugmsg( "failed to insert mod" );
        }
    }
    temp.update_modified_pockets();
    temp.contents.combine( contents, true );
    contents = temp.contents;
    current_phase = new_type->phase;
    if( count_by_charges() != new_type->count_by_charges() ) {
        charges = new_type->charges_default();
    }

    if( temp.type->countdown_interval > 0_seconds ) {
        countdown_point = calendar::turn + type->countdown_interval;
        active = true;
    }
    if( carrier && carrier->has_item( *this ) ) {
        carrier->on_item_acquire( *this );
    }

    item_counter = 0;
    update_link_traits();
    update_prefix_suffix_flags();
    return *this;
}

item &item::deactivate( Character *ch, bool alert )
{
    if( !active ) {
        return *this; // no-op
    }

    if( type->revert_to ) {
        if( ch && alert && !type->tool->revert_msg.empty() ) {
            ch->add_msg_if_player( m_info, type->tool->revert_msg.translated(), tname() );
        }
        convert( *type->revert_to );
        active = false;

        if( ch ) {
            ch->clear_inventory_search_cache();
        }
    }
    return *this;
}

item &item::activate()
{
    if( active ) {
        return *this; // no-op
    }

    active = true;

    return *this;
}

bool item::activate_thrown( const tripoint_bub_ms &pos )
{
    return type->invoke( nullptr, *this, pos ).value_or( 0 );
}

units::energy item::mod_energy( const units::energy &qty )
{
    if( !is_vehicle_battery() ) {
        debugmsg( "Tried to set energy of non-battery item" );
        return 0_J;
    }

    units::energy val = energy_remaining( nullptr ) + qty;
    if( val < 0_J ) {
        return val;
    } else if( val > type->battery->max_capacity ) {
        energy = type->battery->max_capacity;
    } else {
        energy = val;
    }
    return 0_J;
}

item &item::ammo_set( const itype_id &ammo, int qty )
{
    if( !ammo->ammo ) {
        if( !has_flag( flag_USES_BIONIC_POWER ) ) {
            debugmsg( "can't set ammo %s in %s as it is not an ammo", ammo.c_str(), type_name() );
        }
        return *this;
    }
    const ammotype &ammo_type = ammo->ammo->type;
    if( qty < 0 ) {
        // completely fill an integral or existing magazine
        //if( magazine_current() ) then we need capacity of the magazine instead of the item maybe?
        if( magazine_integral() || magazine_current() ) {
            qty = ammo_capacity( ammo_type );

            // else try to add a magazine using default ammo count property if set
        } else if( !magazine_default( true ).is_null() ) {
            item mag( magazine_default( true ) );
            if( mag.type->magazine->count > 0 ) {
                qty = mag.type->magazine->count;
            } else {
                qty = mag.ammo_capacity( ammo_type );
            }
        }
    }

    if( qty <= 0 ) {
        ammo_unset();
        return *this;
    }

    // handle reloadable tools and guns with no specific ammo type as special case
    if( ammo.is_null() && ammo_types().empty() ) {
        if( magazine_integral() ) {
            if( is_tool() ) {
                charges = std::min( qty, ammo_capacity( ammo_type ) );
            } else if( is_gun() ) {
                const item temp_ammo( ammo_default(), calendar::turn, std::min( qty, ammo_capacity( ammo_type ) ) );
                put_in( temp_ammo, pocket_type::MAGAZINE );
            }
        }
        return *this;
    }

    // check ammo is valid for the item
    std::set<itype_id> mags = magazine_compatible();
    if( ammo_types().count( ammo_type ) == 0 && !( magazine_current() &&
            magazine_current()->ammo_types().count( ammo_type ) ) &&
    !std::any_of( mags.begin(), mags.end(), [&ammo_type]( const itype_id & mag ) {
    return mag->magazine->type.count( ammo_type );
    } ) ) {
        debugmsg( "Tried to set invalid ammo of %s for %s", ammo.c_str(), typeId().c_str() );
        return *this;
    }

    if( is_magazine() ) {
        ammo_unset();
        item set_ammo( ammo, calendar::turn, std::min( qty, ammo_capacity( ammo_type ) ) );
        if( has_flag( flag_NO_UNLOAD ) ) {
            set_ammo.set_flag( flag_NO_DROP );
            set_ammo.set_flag( flag_IRREMOVABLE );
        }
        put_in( set_ammo, pocket_type::MAGAZINE );

    } else {
        if( !magazine_current() ) {
            itype_id mag = magazine_default();
            if( !mag->magazine ) {
                debugmsg( "Tried to set ammo of %s without suitable magazine for %s",
                          ammo.c_str(), typeId().c_str() );
                return *this;
            }

            item mag_item( mag );
            // if default magazine too small fetch instead closest available match
            if( mag_item.ammo_capacity( ammo_type ) < qty ) {
                std::vector<item> opts;
                for( const itype_id &mag_type : mags ) {
                    if( mag_type->magazine->type.count( ammo_type ) ) {
                        opts.emplace_back( mag_type );
                    }
                }
                if( opts.empty() ) {
                    const std::string magazines_str = enumerate_as_string( mags,
                    []( const itype_id & mag ) {
                        const std::string ammotype_str = enumerate_as_string( mag->magazine->type,
                        []( const ammotype & a ) {
                            return a.str();
                        } );
                        return string_format( _( "%s (taking %s)" ), mag.str(), ammotype_str );
                    } );
                    debugmsg( "Cannot find magazine fitting %s with any capacity for ammo %s "
                              "(ammotype %s).  Magazines considered were %s",
                              typeId().str(), ammo.str(), ammo_type.str(), magazines_str );
                    return *this;
                }
                std::sort( opts.begin(), opts.end(), [&ammo_type]( const item & lhs, const item & rhs ) {
                    return lhs.ammo_capacity( ammo_type ) < rhs.ammo_capacity( ammo_type );
                } );
                auto iter = std::find_if( opts.begin(), opts.end(), [&qty, &ammo_type]( const item & mag ) {
                    return mag.ammo_capacity( ammo_type ) >= qty;
                } );
                if( iter != opts.end() ) {
                    mag = iter->typeId();
                } else {
                    mag = opts.back().typeId();
                }
            }
            put_in( item( mag ), pocket_type::MAGAZINE_WELL );
        }
        item *mag_cur = magazine_current();
        if( mag_cur != nullptr ) {
            mag_cur->ammo_set( ammo, qty );
        }
    }
    return *this;
}

item &item::ammo_unset()
{
    if( !is_tool() && !is_gun() && !is_magazine() ) {
        // do nothing
    } else if( is_magazine() ) {
        if( is_money() ) { // charges are set wrong on cash cards.
            charges = 0;
        }
        contents.clear_magazines();
    } else if( magazine_integral() ) {
        charges = 0;
        if( is_gun() ) {
            contents.clear_magazines();
        }
    } else if( magazine_current() ) {
        magazine_current()->ammo_unset();
    }

    return *this;
}

int item::damage() const
{
    return damage_;
}

int item::degradation() const
{
    int ret = degradation_;
    for( fault_id f : faults ) {
        ret += f.obj().degradation_mod();
    }
    return ret;
}

void item::rand_degradation()
{
    if( type->degrade_increments() == 0 ) {
        return; // likely count_by_charges
    }
    set_degradation( rng( 0, damage() ) * 50.0f / type->degrade_increments() );
}

int item::damage_level( bool precise ) const
{
    return precise ? damage_ : type->damage_level( damage_ );
}

float item::damage_adjusted_melee_weapon_damage( float value, const damage_type_id &dt ) const
{
    if( type->count_by_charges() ) {
        return value; // count by charges items don't have partial damage
    }

    for( fault_id fault : faults ) {
        for( const std::tuple<int, float, damage_type_id> &damage_mod : fault.obj().melee_damage_mod() ) {
            const damage_type_id dmg_type_of_fault = std::get<2>( damage_mod );
            if( dt == dmg_type_of_fault ) {
                const int damage_added = std::get<0>( damage_mod );
                const float damage_mult = std::get<1>( damage_mod );
                value = ( value + damage_added ) * damage_mult;
            }
        }
    }

    return std::max( 0.0f, value * ( 1.0f - 0.1f * std::max( 0, damage_level() - 1 ) ) );
}

float item::damage_adjusted_gun_damage( float value ) const
{
    if( type->count_by_charges() ) {
        return value; // count by charges items don't have partial damage
    }
    return value - 2 * std::max( 0, damage_level() - 1 );
}

float item::damage_adjusted_armor_resist( float value, const damage_type_id &dmg_type ) const
{
    if( type->count_by_charges() ) {
        return value; // count by charges items don't have partial damage
    }

    for( fault_id fault : faults ) {
        for( const std::tuple<int, float, damage_type_id> &damage_mod : fault.obj().armor_mod() ) {
            const damage_type_id dmg_type_of_fault = std::get<2>( damage_mod );
            if( dmg_type == dmg_type_of_fault ) {
                const int damage_added = std::get<0>( damage_mod );
                const float damage_mult = std::get<1>( damage_mod );
                value = ( value + damage_added ) * damage_mult;
            }
        }
    }

    return std::max( 0.0f, value * ( 1.0f - std::max( 0, damage_level() - 1 ) * 0.125f ) );
}

void item::set_damage( int qty )
{
    damage_ = std::clamp( qty, degradation_, max_damage() );
}

void item::set_degradation( int qty )
{
    if( type->degrade_increments() > 0 ) {
        degradation_ = std::clamp( qty, 0, max_damage() );
    } else {
        degradation_ = 0;
    }
    set_damage( damage_ );
}

item item::split( int qty )
{
    if( !count_by_charges() || qty <= 0 || qty >= charges ) {
        return item();
    }
    item res = *this;
    res.charges = qty;
    charges -= qty;
    return res;
}

bool item::is_null() const
{
    // Actually, type should never by null at all.
    return ( type == nullptr || type == nullitem() || typeId().is_null() );
}

bool item::is_frozen_liquid() const
{
    return made_of( phase_id::SOLID ) && made_of_from_type( phase_id::LIQUID );
}

int item::charges_per_volume( const units::volume &vol, bool suppress_warning ) const
{
    int64_t ret;
    if( count_by_charges() ) {
        if( type->volume == 0_ml ) {
            if( !suppress_warning ) {
                debugmsg( "Item '%s' with zero volume", tname() );
            }
            return INFINITE_CHARGES;
        }
        // Type cast to prevent integer overflow with large volume containers like the cargo
        // dimension
        ret = vol * static_cast<int64_t>( type->stack_size ) / type->volume;
    } else {
        units::volume my_volume = volume();
        if( my_volume == 0_ml ) {
            if( !suppress_warning ) {
                debugmsg( "Item '%s' with zero volume", tname() );
            }
            return INFINITE_CHARGES;
        }
        ret = vol / my_volume;
    }
    return std::min( ret, static_cast<int64_t>( INFINITE_CHARGES ) );
}

int item::charges_per_weight( const units::mass &m, bool suppress_warning ) const
{
    int64_t ret;
    units::mass my_weight = count_by_charges() ? type->weight : weight();
    if( my_weight == 0_gram ) {
        if( !suppress_warning ) {
            debugmsg( "Item '%s' with zero weight", tname() );
        }
        ret = INFINITE_CHARGES;
    } else {
        ret = m / my_weight;
    }
    return std::min( ret, static_cast<int64_t>( INFINITE_CHARGES ) );
}

bool item::display_stacked_with( const item &rhs, bool check_components ) const
{
    return !count_by_charges() && stacks_with( rhs, check_components );
}

bool item::can_combine( const item &rhs ) const
{
    if( !contents.empty() || !rhs.contents.empty() ) {
        return false;
    }
    if( !count_by_charges() ) {
        return false;
    }
    if( !stacks_with( rhs, true, true ) ) {
        return false;
    }
    return true;
}

bool item::combine( const item &rhs )
{
    if( !can_combine( rhs ) ) {
        return false;
    }
    if( has_temperature() ) {
        if( goes_bad() ) {
            //use maximum rot between the two
            set_relative_rot( std::max( get_relative_rot(),
                                        rhs.get_relative_rot() ) );
        }
        const float lhs_energy = get_item_thermal_energy();
        const float rhs_energy = rhs.get_item_thermal_energy();
        if( rhs_energy > 0 && lhs_energy > 0 ) {
            const float combined_specific_energy = ( lhs_energy + rhs_energy ) / ( to_gram(
                    weight() ) + to_gram( rhs.weight() ) );
            set_item_specific_energy( units::from_joule_per_gram( combined_specific_energy ) );
        }

        if( item_counter > 0 || rhs.item_counter > 0 ) {
            item_counter = ( static_cast<double>( item_counter ) * charges + static_cast<double>
                             ( rhs.item_counter ) * rhs.charges ) / ( charges + rhs.charges );
        }
    }
    charges += rhs.charges;
    if( !rhs.has_flag( flag_NO_PARASITES ) ) {
        unset_flag( flag_NO_PARASITES );
    }
    return true;
}

bool item::same_for_rle( const item &rhs ) const
{
    if( type != rhs.type ) {
        return false;
    }
    if( charges != rhs.charges ) {
        return false;
    }
    if( !contents.empty_with_no_mods() || !rhs.contents.empty_with_no_mods() ) {
        return false;
    }
    if( has_itype_variant( false ) != rhs.has_itype_variant( false ) ||
        ( has_itype_variant( false ) && rhs.has_itype_variant( false ) &&
          itype_variant().id != rhs.itype_variant().id ) ) {
        return false;
    }
    return stacks_with( rhs, true, false, false, 0, 999, true );
}

namespace
{

bool _stacks_whiteblacklist( item const &lhs, item const &rhs )
{
    bool wbl = false;
    if( lhs.get_contents().size() == rhs.get_contents().size() ) {
        std::vector<item_pocket const *> const lpkts = lhs.get_all_contained_pockets();
        std::vector<item_pocket const *> const rpkts = rhs.get_all_contained_pockets();
        if( lpkts.size() == rpkts.size() ) {
            wbl = true;
            for( std::size_t i = 0; i < lpkts.size(); i++ ) {
                wbl &= lpkts[i]->settings.get_item_whitelist() == rpkts[i]->settings.get_item_whitelist() &&
                       lpkts[i]->settings.get_item_blacklist() == rpkts[i]->settings.get_item_blacklist() &&
                       lpkts[i]->settings.get_category_whitelist() == rpkts[i]->settings.get_category_whitelist() &&
                       lpkts[i]->settings.get_category_blacklist() == rpkts[i]->settings.get_category_blacklist();
                if( !wbl ) {
                    break;
                }
            }
        }
    }
    return wbl;
}

bool _stacks_weapon_mods( item const &lhs, item const &rhs )
{
    item const *const lb = lhs.is_gun() ? lhs.gunmod_find( itype_barrel_small ) : nullptr;
    item const *const rb = rhs.is_gun() ? rhs.gunmod_find( itype_barrel_small ) : nullptr;
    return ( ( lb && rb ) || ( !lb && !rb ) ) &&
           lhs.has_flag( flag_REMOVED_STOCK ) == rhs.has_flag( flag_REMOVED_STOCK ) &&
           lhs.has_flag( flag_DIAMOND ) == rhs.has_flag( flag_DIAMOND );
}

bool _stacks_location_hint( item const &lhs, item const &rhs )
{
    static const std::string omt_loc_var = "spawn_location";
    const tripoint_abs_ms this_loc( lhs.get_var( omt_loc_var, tripoint_abs_ms::max ) );
    const tripoint_abs_ms that_loc( rhs.get_var( omt_loc_var, tripoint_abs_ms::max ) );
    if( this_loc == that_loc ) {
        return true;
    } else if( this_loc != tripoint_abs_ms::max && that_loc != tripoint_abs_ms::max ) {
        const tripoint_abs_ms player_loc( get_player_character().pos_abs() );
        const int this_dist = rl_dist( player_loc, this_loc );
        const int that_dist = rl_dist( player_loc, that_loc );
        static const auto get_bucket = []( const int dist ) {
            if( dist < 1 ) {
                return 0;
            } else if( dist < 6 ) {
                return 1;
            } else if( dist < 30 ) {
                return 2;
            } else {
                return 3;
            }
        };
        return get_bucket( this_dist ) == get_bucket( that_dist );
    }
    return false;
}

bool _stacks_rot( item const &lhs, item const &rhs, bool combine_liquid )
{
    // Stack items that fall into the same "bucket" of freshness.
    // Distant buckets are larger than near ones.
    std::pair<int, clipped_unit> my_clipped_time_to_rot =
        clipped_time( std::max( lhs.get_shelf_life() - lhs.get_rot(), 0_seconds ) );
    std::pair<int, clipped_unit> other_clipped_time_to_rot =
        clipped_time( std::max( rhs.get_shelf_life() - rhs.get_rot(), 0_seconds ) );

    return ( ( combine_liquid && lhs.made_of_from_type( phase_id::LIQUID ) &&
               rhs.made_of_from_type( phase_id::LIQUID ) ) ||
             my_clipped_time_to_rot == other_clipped_time_to_rot ) &&
           lhs.rotten() == rhs.rotten();
}

bool _stacks_mushy_dirty( item const &lhs, item const &rhs )
{
    return lhs.has_flag( flag_MUSHY ) == rhs.has_flag( flag_MUSHY ) &&
           lhs.has_own_flag( flag_DIRTY ) == rhs.has_own_flag( flag_DIRTY );
}

bool _stacks_food_status( item const &lhs, item const &rhs )
{
    return ( lhs.is_fresh() == rhs.is_fresh() || lhs.is_going_bad() == rhs.is_going_bad() ||
             lhs.rotten() == rhs.rotten() ) &&
           _stacks_mushy_dirty( lhs, rhs );
}

bool _stacks_food_traits( item const &lhs, item const &rhs )
{
    return ( !lhs.is_food() && !rhs.is_food() ) ||
           ( lhs.is_food() && rhs.is_food() &&
             lhs.has_flag( flag_HIDDEN_POISON ) == rhs.has_flag( flag_HIDDEN_POISON ) &&
             lhs.has_flag( flag_HIDDEN_HALLU ) == rhs.has_flag( flag_HIDDEN_HALLU ) );
}

bool _stacks_food_irradiated( item const &lhs, item const &rhs )
{
    return lhs.has_flag( flag_IRRADIATED ) == rhs.has_flag( flag_IRRADIATED );
}

bool _stacks_food_perishable( item const &lhs, item const &rhs, bool check_cat )
{
    return !check_cat || ( !lhs.is_food() && !rhs.is_food() ) ||
           ( lhs.is_food() && rhs.is_food() && lhs.goes_bad() == rhs.goes_bad() &&
             lhs.has_flag( flag_INEDIBLE ) == rhs.has_flag( flag_INEDIBLE ) );
}

bool _stacks_clothing_size( item const &lhs, item const &rhs )
{
    avatar &u = get_avatar();
    item::sizing const u_sizing = lhs.get_sizing( u );
    return u_sizing == rhs.get_sizing( u ) &&
           ( u_sizing == item::sizing::ignore ||
             ( lhs.has_flag( flag_FIT ) == rhs.has_flag( flag_FIT ) &&
               lhs.has_flag( flag_VARSIZE ) == rhs.has_flag( flag_VARSIZE ) ) );
}

bool _stacks_wetness( item const &lhs, item const &rhs, bool precise )
{
    return lhs.wetness == rhs.wetness || ( !precise && ( lhs.wetness > 0 ) == ( rhs.wetness > 0 ) );
}

bool _stacks_cbm_status( item const &lhs, item const &rhs )
{
    return ( !lhs.is_bionic() && !rhs.is_bionic() ) ||
           ( lhs.is_bionic() && rhs.is_bionic() &&
             lhs.has_flag( flag_NO_PACKED ) == rhs.has_flag( flag_NO_PACKED ) &&
             lhs.has_flag( flag_NO_STERILE ) == rhs.has_flag( flag_NO_STERILE ) );
}

bool _stacks_ethereal( item const &lhs, item const &rhs )
{
    static const std::string varname( "ethereal" );
    return ( !lhs.ethereal && !rhs.ethereal ) ||
           ( lhs. ethereal && rhs.ethereal && lhs.get_var( varname, 0 ) == rhs.get_var( varname, 0 ) );
}

bool _stacks_ups( item const &lhs, item const &rhs )
{
    return ( !lhs.is_tool() && !rhs.is_tool() ) ||
           ( lhs.is_tool() && rhs.is_tool() &&
             lhs.has_flag( flag_USE_UPS ) == rhs.has_flag( flag_USE_UPS ) );
}

bool _stacks_mods( item const &lhs, item const &rhs )
{
    const std::vector<const item *> this_mods = lhs.mods();
    const std::vector<const item *> that_mods = rhs.mods();
    if( lhs.is_armor() && rhs.is_armor() && lhs.has_clothing_mod() != rhs.has_clothing_mod() ) {
        return false;
    }
    if( this_mods.size() != that_mods.size() ) {
        return false;
    }
    for( const item *it1 : this_mods ) {
        bool match = false;
        const bool i1_isnull = it1 == nullptr;
        for( const item *it2 : that_mods ) {
            const bool i2_isnull = it2 == nullptr;
            if( i1_isnull != i2_isnull ) {
                continue;
            } else if( it1 == it2 || it1->typeId() == it2->typeId() ) {
                match = true;
                break;
            }
        }
        if( !match ) {
            return false;
        }
    }
    return true;
}

bool _stacks_variant( item const &lhs, item const &rhs )
{
    return ( !lhs.has_itype_variant() && !rhs.has_itype_variant() ) ||
           ( lhs.has_itype_variant() && rhs.has_itype_variant() &&
             lhs.itype_variant().id == rhs.itype_variant().id );
}

bool _stacks_components( item const &lhs, item const &rhs, bool check_components )
{
    return ( !check_components && !lhs.is_comestible() && !lhs.is_craft() ) ||
           // Only check if at least one item isn't using the default recipe or is comestible
           ( lhs.components.empty() && rhs.components.empty() ) ||
           ( lhs.get_uncraft_components() == rhs.get_uncraft_components() );
}

} // namespace

stacking_info item::stacks_with( const item &rhs, bool check_components, bool combine_liquid,
                                 bool check_cat, int depth, int maxdepth, bool precise ) const
{
    tname::segment_bitset bits;
    if( type == rhs.type ) {
        bits.set( tname::segments::TYPE );
        bits.set( tname::segments::WHEEL_DIAMETER );
        bits.set( tname::segments::WHITEBLACKLIST, _stacks_whiteblacklist( *this, rhs ) );
    }

    if( !check_cat && bits.none() ) {
        return {};
    }

    bits.set( tname::segments::CATEGORY,
              !check_cat ||
              get_category_of_contents( depth, maxdepth ).get_id() ==
              rhs.get_category_of_contents( depth, maxdepth ).get_id() );

    if( check_cat && bits.none() ) {
        return {};
    }

    bits.set( tname::segments::VARIANT, _stacks_variant( *this, rhs ) );
    bool const same_type = bits[tname::segments::TYPE] && bits[tname::segments::VARIANT];

    bits.set( tname::segments::RELIC, is_same_relic( rhs ) );

    // This function is also used to test whether items counted by charges should be merged, for that
    // check the, the charges must be ignored. In all other cases (tools/guns), the charges are important.
    bits.set( tname::segments::CHARGES,
              same_type && ( count_by_charges() || charges == rhs.charges ) );
    bits.set( tname::segments::FAVORITE_PRE, is_favorite == rhs.is_favorite );
    bits.set( tname::segments::FAVORITE_POST, is_favorite == rhs.is_favorite );
    bits.set( tname::segments::DURABILITY,
              damage_level( precise ) == rhs.damage_level( precise ) && degradation_ == rhs.degradation_ );
    bits.set( tname::segments::BURN, burnt == rhs.burnt );
    bits.set( tname::segments::ACTIVE, active == rhs.active );
    bits.set( tname::segments::ACTIVITY_OCCUPANCY, get_var( "activity_var",
              "" ) == rhs.get_var( "activity_var", "" ) );
    bits.set( tname::segments::FILTHY, is_filthy() == rhs.is_filthy() );
    bits.set( tname::segments::WETNESS, _stacks_wetness( *this, rhs, precise ) );
    bits.set( tname::segments::WEAPON_MODS, _stacks_weapon_mods( *this, rhs ) );

    if( combine_liquid && same_type && has_temperature() && made_of_from_type( phase_id::LIQUID ) ) {
        // we can combine liquids of same type and different temperatures
        bits.set( tname::segments::TAGS, equal_ignoring_elements( rhs.get_flags(), get_flags(),
        { flag_COLD, flag_FROZEN, flag_HOT, flag_NO_PARASITES, flag_FROM_FROZEN_LIQUID } ) );
        bits.set( tname::segments::TEMPERATURE );
    } else {
        bits.set( tname::segments::TEMPERATURE, is_same_temperature( rhs ) );
        bits.set( tname::segments::TAGS, item_tags == rhs.item_tags );
    }

    bits.set( tname::segments::CUSTOM_ITEM_PREFIX, bits[tname::segments::TAGS] );
    bits.set( tname::segments::CUSTOM_ITEM_SUFFIX, bits[tname::segments::TAGS] );

    bits.set( tname::segments::FAULTS, faults == rhs.faults );
    bits.set( tname::segments::FAULTS_SUFFIX, faults == rhs.faults );
    bits.set( tname::segments::TECHNIQUES, techniques == rhs.techniques );
    bits.set( tname::segments::OVERHEAT, overheat_symbol() == rhs.overheat_symbol() );
    bits.set( tname::segments::DIRT, get_var( "dirt", 0 ) == rhs.get_var( "dirt", 0 ) );
    bits.set( tname::segments::SEALED, all_pockets_sealed() == rhs.all_pockets_sealed() );
    bits.set( tname::segments::CBM_STATUS, _stacks_cbm_status( *this, rhs ) );
    bits.set( tname::segments::BROKEN, is_broken() == rhs.is_broken() );
    bits.set( tname::segments::UPS, _stacks_ups( *this, rhs ) );
    // Guns that differ only by dirt/shot_counter can still stack,
    // but other item_vars such as label/note will prevent stacking
    static const std::set<std::string> ignore_keys = { "dirt", "shot_counter", "spawn_location", "ethereal", "last_act_by_char_id", "activity_var" };
    bits.set( tname::segments::TRAITS, template_traits == rhs.template_traits );
    bits.set( tname::segments::VARS, map_equal_ignoring_keys( item_vars, rhs.item_vars, ignore_keys ) );
    bits.set( tname::segments::ETHEREAL, _stacks_ethereal( *this, rhs ) );
    bits.set( tname::segments::LOCATION_HINT, _stacks_location_hint( *this, rhs ) );

    bool const this_goes_bad = goes_bad();
    bool const that_goes_bad = rhs.goes_bad();
    if( this_goes_bad && that_goes_bad ) {
        if( same_type ) {
            bits.set( tname::segments::FOOD_STATUS,
                      _stacks_rot( *this, rhs, combine_liquid ) && _stacks_mushy_dirty( *this, rhs ) );

        } else {
            bits.set( tname::segments::FOOD_STATUS, _stacks_food_status( *this, rhs ) );
        }
        bits.set( tname::segments::FOOD_IRRADIATED, _stacks_food_irradiated( *this, rhs ) );
    } else if( !this_goes_bad && !that_goes_bad ) {
        bits.set( tname::segments::FOOD_STATUS );
        bits.set( tname::segments::FOOD_IRRADIATED );
    }

    bits.set( tname::segments::FOOD_TRAITS, _stacks_food_traits( *this, rhs ) );
    bits.set( tname::segments::CORPSE,
              ( corpse == nullptr && rhs.corpse == nullptr ) ||
              ( corpse != nullptr && rhs.corpse != nullptr && corpse->id == rhs.corpse->id &&
                corpse_name == rhs.corpse_name ) );
    bits.set( tname::segments::FOOD_PERISHABLE, _stacks_food_perishable( *this, rhs, check_cat ) );
    bits.set( tname::segments::CLOTHING_SIZE, _stacks_clothing_size( *this, rhs ) );
    bits.set( tname::segments::BROKEN, is_broken() == rhs.is_broken() );
    // In-progress crafts are always distinct items. Easier to handle for the player,
    // and there shouldn't be that many items of this type around anyway.
    bits.set( tname::segments::CRAFT, !craft_data_ && !rhs.craft_data_ );
    bits.set( tname::segments::COMPONENTS, same_type &&
              _stacks_components( *this, rhs, check_components ) );
    bits.set( tname::segments::LINK,
              link_length() == rhs.link_length() && max_link_length() == rhs.max_link_length() );
    bits.set( tname::segments::MODS, _stacks_mods( *this, rhs ) );
    //checking browsed status is not necessary, equal vars are checked earlier
    bits.set( tname::segments::EMEMORY,
              occupied_ememory() == rhs.occupied_ememory() && total_ememory() == rhs.total_ememory() );
    bits.set( tname::segments::last_segment );

    // only check contents if everything else matches
    bool const b_contents =
        ( bits | tname::tname_contents ).all() && contents.stacks_with( rhs.contents, depth, maxdepth );
    bits.set( tname::segments::CONTENTS, b_contents );
    bits.set( tname::segments::CONTENTS_FULL, b_contents );
    bits.set( tname::segments::CONTENTS_ABREV, b_contents );
    bits.set( tname::segments::CONTENTS_COUNT, b_contents );
    return { bits };
}

bool item::merge_charges( const item &rhs )
{
    if( !count_by_charges() || !stacks_with( rhs ) ) {
        return false;
    }
    // Prevent overflow when either item has "near infinite" charges.
    if( charges >= INFINITE_CHARGES / 2 || rhs.charges >= INFINITE_CHARGES / 2 ) {
        charges = INFINITE_CHARGES;
        return true;
    }
    // We'll just hope that the item counter represents the same thing for both items
    if( item_counter > 0 || rhs.item_counter > 0 ) {
        item_counter = ( static_cast<double>( item_counter ) * charges + static_cast<double>
                         ( rhs.item_counter ) * rhs.charges ) / ( charges + rhs.charges );
    }
    charges += rhs.charges;
    return true;
}

void item::set_var( const std::string &key, diag_value value )
{
    item_vars[ key ] = std::move( value );
}

double item::get_var( const std::string &key, double default_value ) const
{
    if( diag_value const *ret = maybe_get_value( key ); ret ) {
        return ret->dbl();
    }

    return default_value;
}

std::string item::get_var( const std::string &key, std::string default_value ) const
{
    if( diag_value const *ret = maybe_get_value( key ); ret ) {
        return ret->str();
    }

    return default_value;
}

tripoint_abs_ms item::get_var( const std::string &key, tripoint_abs_ms default_value ) const
{
    if( diag_value const *ret = maybe_get_value( key ); ret ) {
        return ret->tripoint();
    }

    return default_value;
}

void item::remove_var( const std::string &key )
{
    item_vars.erase( key );
}

diag_value const &item::get_value( const std::string &name ) const
{
    return global_variables::_common_get_value( name, item_vars );

}

diag_value const *item::maybe_get_value( const std::string &name ) const
{
    return global_variables::_common_maybe_get_value( name, item_vars );
}

bool item::has_var( const std::string &name ) const
{
    return item_vars.count( name ) > 0;
}

void item::erase_var( const std::string &name )
{
    item_vars.erase( name );
}

void item::clear_vars()
{
    item_vars.clear();
}

bool item::is_owned_by( const Character &c, bool available_to_take ) const
{
    // owner.is_null() implies faction_id( "no_faction" ) which shouldn't happen, or no owner at all.
    // either way, certain situations this means the thing is available to take.
    // in other scenarios we actually really want to check for id == id, even for no_faction
    if( get_owner().is_null() ) {
        return available_to_take;
    }
    if( !c.get_faction() ) {
        debugmsg( "Character %s has no faction", c.disp_name() );
        return false;
    }
    return c.get_faction()->id == get_owner();
}

bool item::is_owned_by( const monster &m, bool available_to_take ) const
{
    // owner.is_null() implies faction_id( "no_faction" ) which shouldn't happen, or no owner at all.
    // either way, certain situations this means the thing is available to take.
    // in other scenarios we actually really want to check for id == id, even for no_faction
    if( get_owner().is_null() ) {
        return available_to_take;
    }
    for( const std::pair<const faction_id, faction> &fact : g->faction_manager_ptr->all() ) {
        if( fact.second.mon_faction != m.faction.id() ) {
            continue;
        }
        return fact.first == get_owner();
    }
    return false;
}

bool item::is_old_owner( const Character &c, bool available_to_take ) const
{
    if( get_old_owner().is_null() ) {
        return available_to_take;
    }
    if( !c.get_faction() ) {
        debugmsg( "Character %s has no faction.", c.disp_name() );
        return false;
    }
    return c.get_faction()->id == get_old_owner();
}

std::string item::get_old_owner_name() const
{
    if( !g->faction_manager_ptr->get( get_old_owner() ) ) {
        debugmsg( "item::get_owner_name() item %s has no valid nor null faction id", tname() );
        return "no owner";
    }
    return g->faction_manager_ptr->get( get_old_owner() )->name;
}

std::string item::get_owner_name() const
{
    if( !g->faction_manager_ptr->get( get_owner() ) ) {
        debugmsg( "item::get_owner_name() item %s has no valid nor null faction id ", tname() );
        return "no owner";
    }
    return g->faction_manager_ptr->get( get_owner() )->name;
}

void item::set_owner( const faction_id &new_owner )
{
    owner = new_owner;
    for( item *e : contents.all_items_top() ) {
        e->set_owner( new_owner );
    }
}

void item::set_owner( const Character &c )
{
    if( !c.get_faction() ) {
        debugmsg( "item::set_owner() Character %s has no valid faction", c.disp_name() );
        return;
    }
    set_owner( c.get_faction()->id );
}

faction_id item::get_owner() const
{
    validate_ownership();
    return owner;
}

faction_id item::get_old_owner() const
{
    validate_ownership();
    return old_owner;
}

void item::validate_ownership() const
{
    if( !old_owner.is_null() && !g->faction_manager_ptr->get( old_owner, false ) ) {
        remove_old_owner();
    }
    if( !owner.is_null() && !g->faction_manager_ptr->get( owner, false ) ) {
        remove_owner();
    }
}

/*
 * 0 based lookup table of accuracy - monster defense converted into number of hits per 10000
 * attacks
 * data painstakingly looked up at http://onlinestatbook.com/2/calculators/normal_dist.html
 */
static constexpr std::array<double, 41> hits_by_accuracy = {
    0,    1,   2,   3,   7, // -20 to -16
    13,   26,  47,   82,  139, // -15 to -11
    228,   359,  548,  808, 1151, // -10 to -6
    1587, 2119, 2743, 3446, 4207, // -5 to -1
    5000,  // 0
    5793, 6554, 7257, 7881, 8413, // 1 to 5
    8849, 9192, 9452, 9641, 9772, // 6 to 10
    9861, 9918, 9953, 9974, 9987, // 11 to 15
    9993, 9997, 9998, 9999, 10000 // 16 to 20
};

double item::effective_dps( const Character &guy, Creature &mon ) const
{
    const float mon_dodge = mon.get_dodge();
    float base_hit = guy.get_dex() / 4.0f + guy.get_hit_weapon( *this );
    base_hit *= std::max( 0.25f, 1.0f - guy.avg_encumb_of_limb_type( bp_type::torso ) /
                          100.0f );
    float mon_defense = mon_dodge + mon.size_melee_penalty() / 5.0f;
    constexpr double hit_trials = 10000.0;
    const int rng_mean = std::max( std::min( static_cast<int>( base_hit - mon_defense ), 20 ),
                                   -20 ) + 20;
    double num_all_hits = hits_by_accuracy[ rng_mean ];
    /* critical hits have two chances to occur: triple critical hits happen much less frequently,
     * and double critical hits can only occur if a hit roll is more than 1.5 * monster dodge.
     * Not the hit roll used to determine the attack, another one.
     * the way the math works, some percentage of the total hits are eligible to be double
     * critical hits, and the rest are eligible to be triple critical hits, but in each case,
     * only some small percent of them actually become critical hits.
     */
    const int rng_high_mean = std::max( std::min( static_cast<int>( base_hit - 1.5 * mon_dodge ),
                                        20 ), -20 ) + 20;
    double num_high_hits = hits_by_accuracy[ rng_high_mean ] * num_all_hits / hit_trials;
    double double_crit_chance = guy.crit_chance( 4, 0, *this );
    double crit_chance = guy.crit_chance( 0, 0, *this );
    double num_low_hits = std::max( 0.0, num_all_hits - num_high_hits );

    double moves_per_attack = guy.attack_speed( *this );
    // attacks that miss do no damage but take time
    double total_moves = ( hit_trials - num_all_hits ) * moves_per_attack;
    double total_damage = 0.0;
    double num_crits = std::min( num_low_hits * crit_chance + num_high_hits * double_crit_chance,
                                 num_all_hits );
    // critical hits are counted separately
    double num_hits = num_all_hits - num_crits;
    // sum average damage past armor and return the number of moves required to achieve
    // that damage
    const auto calc_effective_damage = [ &, moves_per_attack]( const double num_strikes,
    const bool crit, const Character & guy, Creature & mon ) {
        bodypart_id bp = bodypart_id( "torso" );
        Creature *temp_mon = &mon;
        double subtotal_damage = 0;
        damage_instance base_damage;
        guy.roll_all_damage( crit, base_damage, true, *this, attack_vector_id::NULL_ID(),
                             sub_bodypart_str_id::NULL_ID(), &mon, bp );
        damage_instance dealt_damage = base_damage;
        dealt_damage = guy.modify_damage_dealt_with_enchantments( dealt_damage );
        // TODO: Modify DPS calculation to consider weakpoints.
        resistances r = resistances( *static_cast<monster *>( temp_mon ) );
        for( damage_unit &dmg_unit : dealt_damage.damage_units ) {
            dmg_unit.amount -= std::min( r.get_effective_resist( dmg_unit ), dmg_unit.amount );
        }

        dealt_damage_instance dealt_dams;
        for( const damage_unit &dmg_unit : dealt_damage.damage_units ) {
            int cur_damage = 0;
            int total_pain = 0;
            temp_mon->deal_damage_handle_type( effect_source::empty(), dmg_unit, bp,
                                               cur_damage, total_pain );
            if( cur_damage > 0 ) {
                dealt_dams.dealt_dams[dmg_unit.type] += cur_damage;
            }
        }
        double damage_per_hit = dealt_dams.total_damage();
        subtotal_damage = damage_per_hit * num_strikes;
        double subtotal_moves = moves_per_attack * num_strikes;

        if( has_technique( RAPID ) ) {
            Creature *temp_rs_mon = &mon;
            damage_instance rs_base_damage;
            guy.roll_all_damage( crit, rs_base_damage, true, *this, attack_vector_id::NULL_ID(),
                                 sub_bodypart_str_id::NULL_ID(), &mon, bp );
            damage_instance dealt_rs_damage = rs_base_damage;
            for( damage_unit &dmg_unit : dealt_rs_damage.damage_units ) {
                dmg_unit.damage_multiplier *= 0.66;
            }
            // TODO: Modify DPS calculation to consider weakpoints.
            resistances rs_r = resistances( *static_cast<monster *>( temp_rs_mon ) );
            for( damage_unit &dmg_unit : dealt_rs_damage.damage_units ) {
                dmg_unit.amount -= std::min( rs_r.get_effective_resist( dmg_unit ), dmg_unit.amount );
            }
            dealt_damage_instance rs_dealt_dams;
            for( const damage_unit &dmg_unit : dealt_rs_damage.damage_units ) {
                int cur_damage = 0;
                int total_pain = 0;
                temp_rs_mon->deal_damage_handle_type( effect_source::empty(), dmg_unit, bp,
                                                      cur_damage, total_pain );
                if( cur_damage > 0 ) {
                    rs_dealt_dams.dealt_dams[dmg_unit.type] += cur_damage;
                }
            }
            double rs_damage_per_hit = rs_dealt_dams.total_damage();
            subtotal_moves *= 0.5;
            subtotal_damage *= 0.5;
            subtotal_moves += moves_per_attack * num_strikes * 0.33;
            subtotal_damage += rs_damage_per_hit * num_strikes * 0.5;
        }
        return std::make_pair( subtotal_moves, subtotal_damage );
    };
    std::pair<double, double> crit_summary = calc_effective_damage( num_crits, true, guy, mon );
    total_moves += crit_summary.first;
    total_damage += crit_summary.second;
    std::pair<double, double> summary = calc_effective_damage( num_hits, false, guy, mon );
    total_moves += summary.first;
    total_damage += summary.second;
    return total_damage * to_moves<double>( 1_seconds ) / total_moves;
}

struct dps_comp_data {
    mtype_id mon_id;
    bool display;
    bool evaluate;
};

static const std::vector<std::pair<translation, dps_comp_data>> dps_comp_monsters = {
    { to_translation( "Best" ), { pseudo_debug_mon, true, false } },
    { to_translation( "Vs. Agile" ), { mon_zombie_smoker, true, true } },
    { to_translation( "Vs. Armored" ), { mon_zombie_soldier_no_weakpoints, true, true } },
    { to_translation( "Vs. Mixed" ), { mon_zombie_survivor_no_weakpoints, false, true } },
};

std::map<std::string, double> item::dps( const bool for_display, const bool for_calc,
        const Character &guy ) const
{
    std::map<std::string, double> results;
    for( const std::pair<translation, dps_comp_data> &comp_mon : dps_comp_monsters ) {
        if( ( comp_mon.second.display != for_display ) &&
            ( comp_mon.second.evaluate != for_calc ) ) {
            continue;
        }
        monster test_mon = monster( comp_mon.second.mon_id );
        results[ comp_mon.first.translated() ] = effective_dps( guy, test_mon );
    }
    return results;
}

std::map<std::string, double> item::dps( const bool for_display, const bool for_calc ) const
{
    return dps( for_display, for_calc, get_avatar() );
}

double item::average_dps( const Character &guy ) const
{
    double dmg_count = 0.0;
    const std::map<std::string, double> &dps_data = dps( false, true, guy );
    for( const std::pair<const std::string, double> &dps_entry : dps_data ) {
        dmg_count += dps_entry.second;
    }
    return dmg_count / dps_data.size();
}

std::map<gunmod_location, int> item::get_mod_locations() const
{
    std::map<gunmod_location, int> mod_locations = type->gun->valid_mod_locations;

    for( const item *mod : gunmods() ) {
        if( !mod->type->gunmod->add_mod.empty() ) {
            std::map<gunmod_location, int> add_locations = mod->type->gunmod->add_mod;

            for( const std::pair<const gunmod_location, int> &add_location : add_locations ) {
                mod_locations[add_location.first] += add_location.second;
            }
        }
    }

    return mod_locations;
}

int item::get_free_mod_locations( const gunmod_location &location ) const
{
    if( !is_gun() ) {
        return 0;
    }

    std::map<gunmod_location, int> mod_locations = get_mod_locations();

    const auto loc = mod_locations.find( location );
    if( loc == mod_locations.end() ) {
        return 0;
    }
    int result = loc->second;
    for( const item *elem : contents.all_items_top( pocket_type::MOD ) ) {
        const cata::value_ptr<islot_gunmod> &mod = elem->type->gunmod;
        if( mod && mod->location == location ) {
            result--;
        }
    }
    return result;
}

const std::string &item::symbol() const
{
    if( has_itype_variant() && _itype_variant->alt_sym ) {
        return *_itype_variant->alt_sym;
    }
    return type->sym;
}

nc_color item::color_in_inventory( const Character *const ch ) const
{
    const Character &player_character = ch ? *ch : get_player_character();

    // Only item not otherwise colored gets colored as favorite
    nc_color ret = is_favorite ? c_white : c_light_gray;
    if( type->can_use( "learn_spell" ) ) {
        const use_function *iuse = get_use( "learn_spell" );
        const learn_spell_actor *actor_ptr =
            static_cast<const learn_spell_actor *>( iuse->get_actor_ptr() );
        for( const std::string &spell_id_str : actor_ptr->spells ) {
            const spell_id sp_id( spell_id_str );
            std::optional<int> max_book_level = sp_id->get_max_book_level();
            if( player_character.magic->knows_spell( sp_id ) &&
                !player_character.magic->get_spell( sp_id ).is_max_level( player_character ) &&
                !( max_book_level.has_value() &&
                   player_character.magic->get_spell( sp_id ).get_level() >= max_book_level.value() ) ) {
                ret = c_yellow;
            }
            if( !player_character.magic->knows_spell( sp_id ) &&
                player_character.magic->can_learn_spell( player_character, sp_id ) ) {
                return c_light_blue;
            }
        }
    } else if( has_flag( flag_WET ) ) {
        ret = c_cyan;
    } else if( has_flag( flag_LITCIG ) ) { // NOLINT(bugprone-branch-clone)
        ret = c_red;
    } else if( is_armor() && player_character.has_trait( trait_WOOLALLERGY ) &&
               ( made_of( material_wool ) || has_own_flag( flag_wooled ) ) ) {
        ret = c_red;
    } else if( is_filthy() || has_own_flag( flag_DIRTY ) ) {
        ret = c_brown;
    } else if( ( is_relic() && !has_flag( flag_MUNDANE ) ) || has_flag( flag_RELIC_PINK ) ) {
        ret = c_pink;
    } else if( is_bionic() ) {
        if( player_character.is_installable( this, false ).success() &&
            ( !player_character.has_bionic( type->bionic->id ) || type->bionic->id->dupes_allowed ) ) {
            ret = player_character.bionic_installation_issues( type->bionic->id ).empty() ? c_green : c_red;
        } else if( !has_flag( flag_NO_STERILE ) ) {
            ret = c_dark_gray;
        }
    } else if( has_flag( flag_LEAK_DAM ) && has_flag( flag_RADIOACTIVE ) && damage() > 0 ) {
        ret = c_light_green;
    } else if( ( active && !has_temperature() ) || ( is_corpse() && can_revive() ) ) {
        // Active items show up as yellow (corpses only if reviving)
        ret = c_yellow;
    } else if( is_medication() || is_medical_tool() ) {
        ret = c_light_blue;
    } else if( is_food() ) {
        // Give color priority to allergy (allergy > inedible by freeze or other conditions)
        // TODO: refactor u.will_eat to let this section handle coloring priority without duplicating code.
        if( player_character.allergy_type( *this ) != morale_type::NULL_ID() ) {
            return c_red;
        }

        // Default: permafood, drugs
        // Brown: rotten (for non-saprophages) or non-rotten (for saprophages)
        // Dark gray: inedible
        // Red: morale penalty
        // Yellow: will rot soon
        // Cyan: edible
        // Light Cyan: will rot eventually
        const ret_val<edible_rating> rating = player_character.will_eat( *this );
        // TODO: More colors
        switch( rating.value() ) {
            case EDIBLE:
            case TOO_FULL:
                ret = c_cyan;

                // Show old items as yellow
                if( is_going_bad() ) {
                    ret = c_yellow;
                }
                // Show perishables as a separate color
                else if( goes_bad() ) {
                    ret = c_light_cyan;
                }

                break;
            case INEDIBLE:
            case INEDIBLE_MUTATION:
                ret = c_dark_gray;
                break;
            case ALLERGY:
            case ALLERGY_WEAK:
            case CANNIBALISM:
            case PARASITES:
                ret = c_red;
                break;
            case ROTTEN:
                ret = c_brown;
                break;
            case NAUSEA:
                ret = c_pink;
                break;
            case NO_TOOL:
                break;
        }
    } else if( is_gun() ) {
        // Guns are green if you are carrying ammo for them
        // ltred if you have ammo but no mags
        // Gun with integrated mag counts as both
        for( const ammotype &at : ammo_types() ) {
            // get_ammo finds uncontained ammo, find_ammo finds ammo in magazines
            bool has_ammo = !player_character.get_ammo( at ).empty() ||
                            !player_character.find_ammo( *this, false, -1 ).empty();
            bool has_mag = magazine_integral() || !player_character.find_ammo( *this, true, -1 ).empty();
            if( has_ammo && has_mag ) {
                ret = c_green;
                break;
            } else if( has_ammo || has_mag ) {
                ret = c_light_red;
                break;
            }
        }
    } else if( is_ammo() ) {
        // Likewise, ammo is green if you have guns that use it
        // ltred if you have the gun but no mags
        // Gun with integrated mag counts as both
        bool has_gun = player_character.cache_has_item_with( "is_gun", &item::is_gun,
        [this]( const item & i ) {
            return i.ammo_types().count( ammo_type() );
        } );
        bool has_mag = player_character.cache_has_item_with( "is_gun", &item::is_gun,
        [this]( const item & i ) {
            return i.magazine_integral() && i.ammo_types().count( ammo_type() );
        } );
        has_mag = has_mag ? true :
                  player_character.cache_has_item_with( "is_magazine", &item::is_magazine,
        [this]( const item & i ) {
            return i.ammo_types().count( ammo_type() );
        } );

        if( has_gun && has_mag ) {
            ret = c_green;
        } else if( has_gun || has_mag ) {
            ret = c_light_red;
        }
    } else if( is_magazine() ) {
        // Magazines are green if you have guns and ammo for them
        // ltred if you have one but not the other
        bool has_gun = player_character.cache_has_item_with( "is_gun", &item::is_gun,
        [this]( const item & it ) {
            return it.magazine_compatible().count( typeId() ) > 0;
        } );
        bool has_ammo = !player_character.find_ammo( *this, false, -1 ).empty();
        if( has_gun && has_ammo ) {
            ret = c_green;
        } else if( has_gun || has_ammo ) {
            ret = c_light_red;
        }
    } else if( is_book() ) {
        const islot_book &tmp = *type->book;
        if( player_character.has_identified( typeId() ) ) {
            if( tmp.skill && // Book can improve skill: blue
                player_character.get_skill_level_object( tmp.skill ).can_train() &&
                player_character.get_knowledge_level( tmp.skill ) >= tmp.req &&
                player_character.get_knowledge_level( tmp.skill ) < tmp.level
              ) { //NOLINT(bugprone-branch-clone)
                ret = c_light_blue;
            } else if( type->can_use( "MA_MANUAL" ) &&
                       !player_character.martial_arts_data->has_martialart( martial_art_learned_from( *type ) ) ) {
                ret = c_light_blue;
            } else if( tmp.skill && // Book can't improve skill right now, but maybe later: pink
                       player_character.get_skill_level_object( tmp.skill ).can_train() &&
                       player_character.get_knowledge_level( tmp.skill ) < tmp.level ) {
                ret = c_pink;
            } else if( !player_character.studied_all_recipes(
                           *type ) ) { // Book can't improve skill anymore, but has more recipes: yellow
                ret = c_yellow;
            }
        } else if( ( tmp.skill || type->can_use( "MA_MANUAL" ) ) ||
                   !player_character.studied_all_recipes( *type ) ) {
            // Book can teach you something and hasn't been identified yet
            ret = c_red;
        } else {
            // "just for fun" book that they haven't read yet
            ret = c_light_red;
        }
    }
    return ret;
}

int item::on_wield_cost( const Character &you ) const
{
    int mv = 0;
    // weapons with bayonet/bipod or other generic "unhandiness"
    if( has_flag( flag_SLOW_WIELD ) && !is_gunmod() ) {
        float d = 32.0f; // arbitrary linear scaling factor
        if( is_gun() ) {
            d /= std::max( you.get_skill_level( gun_skill() ), 1.0f );
        } else if( is_melee() ) {
            d /= std::max( you.get_skill_level( melee_skill() ), 1.0f );
        }

        int penalty = get_var( "volume", volume() / 250_ml ) * d;
        // arbitrary no more than 7 second of penalty
        mv += std::min( penalty, 700 );
    }

    // firearms with a folding stock or tool/melee without collapse/retract iuse
    if( has_flag( flag_NEEDS_UNFOLD ) && !is_gunmod() ) {
        int penalty = 50; // 200-300 for guns, 50-150 for melee, 50 as fallback
        if( is_gun() ) {
            penalty = std::max( 0, 300 - static_cast<int>( you.get_skill_level( gun_skill() ) * 10 ) );
        } else if( is_melee() ) {
            penalty = std::max( 0, 150 - static_cast<int>( you.get_skill_level( melee_skill() ) * 10 ) );
        }

        mv += penalty;
    }
    return mv;
}

void item::on_wield( Character &you, bool combat )
{
    int wield_cost = on_wield_cost( you );
    you.mod_moves( -wield_cost );

    std::string msg;

    msg = _( "You wield your %s." );

    // if game is loaded - don't want ownership assigned during char creation
    if( get_player_character().getID().is_valid() ) {
        handle_pickup_ownership( you );
    }
    you.add_msg_if_player( m_neutral, msg, tname() );

    if( combat && !you.martial_arts_data->selected_is_none() ) {
        you.martial_arts_data->martialart_use_message( you );
    }

    // Update encumbrance and discomfort in case we were wearing it
    you.flag_encumbrance();
    you.calc_discomfort();
    you.on_item_acquire( *this );
}

void item::handle_pickup_ownership( Character &c )
{
    const map &here = get_map();

    if( is_owned_by( c ) ) {
        return;
    }
    // Add ownership to item if unowned
    if( owner.is_null() ) {
        set_owner( c );
    } else {
        if( !is_owned_by( c ) && c.is_avatar() ) {
            const auto sees_stealing = [&c, this, &here]( const Creature & cr ) {
                const npc *const as_npc = cr.as_npc();
                const monster *const as_monster = cr.as_monster();
                bool owned_by = false;
                if( as_npc ) {
                    owned_by = is_owned_by( *as_npc );
                } else if( as_monster ) {
                    owned_by = is_owned_by( *as_monster );
                }
                return &cr != &c && owned_by && rl_dist( cr.pos_abs(), c.pos_abs() ) < MAX_VIEW_DISTANCE &&
                       cr.sees( here, c.pos_bub( here ) );
            };
            const auto sort_criteria = []( const Creature * lhs, const Creature * rhs ) {
                const npc *const lnpc = lhs->as_npc();
                const npc *const rnpc = rhs->as_npc();
                if( !lnpc && !rnpc ) {
                    const monster *const lmon = lhs->as_monster();
                    const monster *const rmon = rhs->as_monster();
                    // Sort more difficult monsters first
                    return ( lmon ? lmon->type->get_total_difficulty() : 0 ) >
                           ( rmon ? rmon->type->get_total_difficulty() : 0 );
                } else if( lnpc && !rnpc ) {
                    return true;
                } else if( !lnpc && rnpc ) {
                    return false;
                }
                return npc::theft_witness_compare( lnpc, rnpc );
            };
            std::vector<Creature *> witnesses = g->get_creatures_if( sees_stealing );
            std::sort( witnesses.begin(), witnesses.end(), sort_criteria );
            for( Creature *c : witnesses ) {
                const npc *const elem = c->as_npc();
                if( !elem ) {
                    // Sorted to have NPCs first
                    break;
                }
                elem->say( "<witnessed_thievery>", 7 );
            }
            if( !witnesses.empty() ) {
                set_old_owner( get_owner() );
                witnesses[0]->witness_thievery( this );
            }
            set_owner( c );
        }
    }
}

void item::on_pickup( Character &p )
{
    // Fake characters are used to determine pickup weight and volume
    if( p.is_fake() ) {
        return;
    }
    // if game is loaded - don't want ownership assigned during char creation
    if( get_player_character().getID().is_valid() ) {
        handle_pickup_ownership( p );
    }
    contents.on_pickup( p, this );

    p.flag_encumbrance();
    p.on_item_acquire( *this );
}

void item::update_inherited_flags()
{
    inherited_tags_cache.clear();

    auto const inehrit_flags = [this]( FlagsSetType const & Flags ) {
        for( flag_id const &f : Flags ) {
            if( f->inherit() ) {
                inherited_tags_cache.emplace( f );
            }
        }
    };

    for( const item *e : is_gun() ? gunmods() : toolmods() ) {
        // gunmods fired separately do not contribute to base gun flags
        if( !e->is_gun() ) {
            inehrit_flags( e->get_flags() );
            inehrit_flags( e->type->get_flags() );
        }
    }

    for( const item_pocket *pocket : contents.get_all_contained_pockets() ) {
        if( pocket->inherits_flags() ) {
            for( const item *e : pocket->all_items_top() ) {
                inehrit_flags( e->get_flags() );
                inehrit_flags( e->type->get_flags() );
            }
        }
    }

    // ensure efiles in device can be used in darkness if edevice can be used in darkness
    if( has_flag( flag_CAN_USE_IN_DARK ) ) {
        for( item *file : efiles() ) {
            file->set_flag( flag_CAN_USE_IN_DARK );
        }
    }

    update_prefix_suffix_flags();
}

void item::update_prefix_suffix_flags()
{
    prefix_tags_cache.clear();
    suffix_tags_cache.clear();
    auto const insert_prefix_suffix_flags = [this]( FlagsSetType const & Flags ) {
        for( flag_id const &f : Flags ) {
            update_prefix_suffix_flags( f );
        }
    };
    insert_prefix_suffix_flags( get_flags() );
    insert_prefix_suffix_flags( type->get_flags() );
    insert_prefix_suffix_flags( inherited_tags_cache );
}

void item::update_prefix_suffix_flags( const flag_id &f )
{
    if( !f->item_prefix().empty() ) {
        prefix_tags_cache.emplace( f );
    }
    if( !f->item_suffix().empty() ) {
        suffix_tags_cache.emplace( f );
    }
}

std::string item::dirt_symbol() const
{
    // these symbols are unicode square characters of different heights, representing a rough
    // estimation of fouling in a gun. This appears instead of "faulty" since most guns will
    // have some level of fouling in them, and usually it is not a big deal.
    switch( static_cast<int>( get_var( "dirt", 0 ) / 2000 ) ) {
        // *INDENT-OFF*
        case 1:  return "<color_white>\u2581</color>";
        case 2:  return "<color_light_gray>\u2583</color>";
        case 3:  return "<color_light_gray>\u2585</color>";
        case 4:  return "<color_dark_gray>\u2587</color>";
        case 5:  return "<color_brown>\u2588</color>";
        default: return "";
        // *INDENT-ON*
    }
}

std::string item::overheat_symbol() const
{

    if( !is_gun() || type->gun->overheat_threshold <= 0.0 ) {
        return "";
    }
    double modifier = 0;
    float multiplier = 1.0f;
    for( const item *mod : gunmods() ) {
        modifier += mod->type->gunmod->overheat_threshold_modifier;
        multiplier *= mod->type->gunmod->overheat_threshold_multiplier;
    }
    if( has_fault( fault_overheat_safety ) ) {
        return string_format( _( "<color_light_green>\u2588VNT </color>" ) );
    }
    switch( std::min( 5, static_cast<int>( get_var( "gun_heat",
                                           0 ) / std::max( type->gun->overheat_threshold * multiplier + modifier, 5.0 ) * 5.0 ) ) ) {
        case 1:
            return "";
        case 2:
            return string_format( _( "<color_yellow>\u2583HEAT </color>" ) );
        case 3:
            return string_format( _( "<color_light_red>\u2585HEAT </color>" ) );
        case 4:
            return string_format( _( "<color_red>\u2587HEAT! </color>" ) ); // NOLINT(cata-text-style)
        case 5:
            return string_format( _( "<color_pink>\u2588HEAT!! </color>" ) ); // NOLINT(cata-text-style)
        default:
            return "";
    }
}

std::string item::degradation_symbol() const
{
    const int inc = max_damage() / 5;
    const int dgr_lvl = degradation() / ( inc > 0 ? inc : 1 );
    std::string dgr_symbol;

    switch( dgr_lvl ) {
        case 0:
            dgr_symbol = colorize( "\u2588", c_light_green );
            break;
        case 1:
            dgr_symbol = colorize( "\u2587", c_yellow );
            break;
        case 2:
            dgr_symbol = colorize( "\u2585", c_magenta );
            break;
        case 3:
            dgr_symbol = colorize( "\u2583", c_light_red );
            break;
        default:
            dgr_symbol = colorize( "\u2581", c_red );
            break;
    }
    return type->degrade_increments() == 0 ? "" : dgr_symbol;
}

std::string item::tname( unsigned int quantity, bool with_prefix ) const
{
    return tname( quantity, with_prefix ? tname::default_tname : tname::unprefixed_tname );
}

std::string item::tname( unsigned int quantity, tname::segment_bitset const &segments ) const
{
    std::string ret;

    for( tname::segments idx : tname::get_tname_set() ) {
        if( !segments[idx] ) {
            continue;
        }
        ret += tname::print_segment( idx, *this, quantity, segments );
    }

    if( item_vars.find( "item_note" ) != item_vars.end() ) {
        //~ %s is an item name. This style is used to denote items with notes.
        return string_format( _( "*%s*" ), ret );
    }

    return ret;
}

std::string item::display_money( unsigned int quantity, unsigned int total,
                                 const std::optional<unsigned int> &selected ) const
{
    if( selected ) {
        //~ This is a string to display the selected and total amount of money in a stack of cash cards.
        //~ %1$s is the display name of cash cards.
        //~ %2$s is the total amount of money.
        //~ %3$s is the selected amount of money.
        //~ Example: "cash cards $15.35 of $20.48"
        return string_format( pgettext( "cash card and money", "%1$s %3$s of %2$s" ), tname( quantity ),
                              format_money( total ), format_money( *selected ) );
    } else {
        //~ This is a string to display the total amount of money in a stack of cash cards.
        //~ %1$s is the display name of cash cards.
        //~ %2$s is the total amount of money on the cash cards.
        //~ Example: "cash cards $20.48"
        return string_format( pgettext( "cash card and money", "%1$s %2$s" ), tname( quantity ),
                              format_money( total ) );
    }
}

std::string item::display_name( unsigned int quantity ) const
{
    std::string name = tname( quantity );
    std::string sidetxt;
    std::string amt;
    std::string cable;

    switch( get_side() ) {
        case side::BOTH:
        case side::num_sides:
            break;
        case side::LEFT:
            sidetxt = string_format( " (%s)", _( "left" ) );
            break;
        case side::RIGHT:
            sidetxt = string_format( " (%s)", _( "right" ) );
            break;
    }
    avatar &player_character = get_avatar();
    int amount = 0;
    int max_amount = 0;
    bool show_amt = false;
    // We should handle infinite charges properly in all cases.
    if( is_book() && get_chapters() > 0 ) {
        // a book which has remaining unread chapters
        amount = get_remaining_chapters( player_character );
    } else if( magazine_current() ) {
        show_amt = true;
        const item *mag = magazine_current();
        amount = ammo_remaining( );
        const itype *adata = mag->ammo_data();
        if( adata ) {
            max_amount = mag->ammo_capacity( adata->ammo->type );
        } else {
            max_amount = mag->ammo_capacity( item_controller->find_template(
                                                 mag->ammo_default() )->ammo->type );
        }
    } else if( is_tool() && has_flag( flag_USES_NEARBY_AMMO ) ) {
        show_amt = true;
        amount = ammo_remaining( player_character.as_character() );
    } else if( !ammo_types().empty() ) {
        // anything that can be reloaded including tools, magazines, guns and auxiliary gunmods
        // but excluding bows etc., which have ammo, but can't be reloaded
        amount = ammo_remaining( );
        const itype *adata = ammo_data();
        if( adata ) {
            max_amount = ammo_capacity( adata->ammo->type );
        } else {
            max_amount = ammo_capacity( item_controller->find_template( ammo_default() )->ammo->type );
        }
        show_amt = !has_flag( flag_RELOAD_AND_SHOOT );
    } else if( count_by_charges() && !has_infinite_charges() ) {
        // A chargeable item
        amount = charges;
        const itype *adata = ammo_data();
        if( adata ) {
            max_amount = ammo_capacity( adata->ammo->type );
        } else if( !ammo_default().is_null() ) {
            max_amount = ammo_capacity( item_controller->find_template( ammo_default() )->ammo->type );
        }
    } else if( is_vehicle_battery() ) {
        show_amt = true;
        amount = to_joule( energy_remaining( nullptr ) );
        max_amount = to_joule( type->battery->max_capacity );
    }

    std::string ammotext;
    if( !is_ammo() && ( ( is_gun() && ammo_required() ) || is_magazine() ) &&
        get_option<bool>( "AMMO_IN_NAMES" ) ) {
        if( !ammo_current().is_null() ) {
            // Loaded with ammo
            ammotext = ammo_current()->nname( 1 );
        } else if( !ammo_types().empty() ) {
            // Is not loaded but can be loaded
            ammotext = ammotype( *ammo_types().begin() )->name();
        } else if( magazine_current() ) {
            // Is not loaded but has magazine that can be loaded
            ammotext = magazine_current()->ammo_default()->ammo->type->name();
        } else if( !magazine_default().is_null() ) {
            // Is not loaded and doesn't have magazine but can use magazines that could be loaded
            item tmp_mag( magazine_default() );
            ammotext = tmp_mag.ammo_default()->ammo->type->name();
        }
    }

    if( ( amount || show_amt ) && !has_flag( flag_PSEUDO ) ) {
        if( is_money() ) {
            amt = " " + format_money( amount );
        } else {
            if( !ammotext.empty() ) {
                ammotext = " " + ammotext;
            }

            if( max_amount != 0 ) {
                const double ratio = static_cast<double>( amount ) / static_cast<double>( max_amount );
                nc_color charges_color;
                if( amount == 0 ) {
                    charges_color = c_light_red;
                } else if( amount == max_amount ) {
                    charges_color = c_white;
                } else if( ratio < 1.0 / 3.0 ) {
                    charges_color = c_red;
                } else if( ratio < 2.0 / 3.0 ) {
                    charges_color = c_yellow;
                } else {
                    charges_color = c_light_green;
                }
                amt = string_format( " (%s%s)", colorize( string_format( "%i/%i", amount, max_amount ),
                                     charges_color ),
                                     ammotext );
            } else {
                amt = string_format( " (%i%s)", amount, ammotext );
            }
        }
    } else if( !ammotext.empty() ) {
        amt = " (" + ammotext + ")";
    }

    if( has_link_data() ) {
        std::string extensions = cables().empty() ? "" : string_format( "+%d", cables().size() );
        const int link_len = link_length();
        const int link_max_len = max_link_length();
        if( link_len <= -2 ) {
            cable = string_format( _( " (-/%1$d cable%2$s)" ), link_max_len, extensions );
        } else if( link_len == -1 ) {
            cable = string_format( _( " (%1$s cable%2$s)" ),
                                   colorize( string_format( "×/%d", link_max_len ), c_light_red ), extensions );
        } else {
            nc_color cable_color;
            const double ratio = static_cast<double>( link_len ) / static_cast<double>( link_max_len );
            if( ratio < 1.0 / 3.0 ) {
                cable_color = c_light_green;
            } else if( ratio < 2.0 / 3.0 ) {
                cable_color = c_yellow;
            } else {
                cable_color = c_red;
            }
            cable = string_format( " (%s)", colorize( string_format( _( "%d/%d cable%s" ),
                                   link_max_len - link_len, link_max_len, extensions ), cable_color ) );
        }
    }
    // HACK: This is a hack to prevent possible crashing when displaying maps as items during character creation
    if( is_map() && calendar::turn != calendar::turn_zero ) {
        tripoint_abs_omt map_pos_omt =
            project_to<coords::omt>( get_var( "reveal_map_center", player_character.pos_abs() ) );
        tripoint_abs_sm map_pos =
            project_to<coords::sm>( map_pos_omt );
        const city *c = overmap_buffer.closest_city( map_pos ).city;
        if( c != nullptr ) {
            name = string_format( "%s %s", c->name, name );
        }
    }

    return string_format( "%s%s%s%s", name, sidetxt, amt, cable );
}

nc_color item::color() const
{
    if( is_null() ) {
        return c_black;
    }
    if( is_corpse() ) {
        return corpse->color;
    }
    if( has_itype_variant() && _itype_variant->alt_color ) {
        return *_itype_variant->alt_color;
    }
    return type->color;
}

int item::price( bool practical ) const
{
    int res = 0;

    visit_items( [&res, practical]( const item * e, item * ) {
        res += e->price_no_contents( practical );
        return VisitResponse::NEXT;
    } );

    return res;
}

int item::price_no_contents( bool practical, std::optional<int> price_override ) const
{
    if( rotten() ) {
        return 0;
    }
    int price = price_override ? *price_override
                : units::to_cent( practical ? type->price_post : type->price );
    if( damage() > 0 ) {
        // maximal damage level is 5, maximal reduction is 80% of the value.
        price -= price * static_cast<double>( std::max( 0, damage_level() - 1 ) ) / 5;
    }

    if( count_by_charges() || made_of( phase_id::LIQUID ) ) {
        // price from json data is for default-sized stack
        price *= charges / static_cast< double >( type->stack_size );

    } else if( is_tool() && type->tool->max_charges != 0 ) {
        // if tool has no ammo (e.g. spray can) reduce price proportional to remaining charges
        price *= ammo_remaining( ) / static_cast< double >( std::max( type->charges_default(), 1 ) );

    }

    // Nominal price for perfectly fresh food, decreasing linearly as it gets closer to expiry
    if( is_food() ) {
        price *= ( 1.0 - get_relative_rot() );
    }

    if( is_filthy() ) {
        // Filthy items receieve a fixed price malus. This means common clothing ends up
        // with no value (it's *everywhere*), but valuable items retain most of their value.
        // https://github.com/CleverRaven/Cataclysm-DDA/issues/49469
        price = std::max( price - PRICE_FILTHY_MALUS, 0 );
    }

    for( fault_id fault : faults ) {
        price *= fault->price_mod();
    }

    if( is_food() && get_comestible() ) {
        const nutrients &nutrients_value = default_character_compute_effective_nutrients( *this );
        for( const std::pair<const vitamin_id, int> &vit_pair : nutrients_value.vitamins() ) {
            if( vit_pair.second > 0 && vit_pair.first->has_flag( flag_NO_SELL ) ) {
                price = 0.0;
            }
        }
    }

    return price;
}

// TODO: MATERIALS add a density field to materials.json
units::mass item::weight( bool include_contents, bool integral ) const
{
    if( is_null() ) {
        return 0_gram;
    }

    // Items that don't drop aren't really there, they're items just for ease of implementation
    if( has_flag( flag_NO_DROP ) ) {
        return 0_gram;
    }

    if( is_craft() ) {
        if( !craft_data_->cached_weight ) {
            units::mass ret = 0_gram;
            for( const item_components::type_vector_pair &tvp : components ) {
                for( const item &it : tvp.second ) {
                    ret += it.weight();
                }
            }
            craft_data_->cached_weight = ret;
        }
        return *craft_data_->cached_weight;
    }

    units::mass ret;
    double ret_mul = 1.0;
    diag_value const &local_mass = get_value( integral ? "integral_weight" : "weight" );
    if( local_mass.is_empty() ) {
        ret = integral ? type->integral_weight : type->weight;
    } else {
        ret = units::from_milligram( local_mass.dbl() );
    }

    if( has_flag( flag_REDUCED_WEIGHT ) ) {
        ret_mul *= 0.75;
    }

    // if this is a gun apply all of its gunmods' weight multipliers
    if( type->gun ) {
        for( const item *mod : gunmods() ) {
            ret_mul *= mod->type->gunmod->weight_multiplier;
        }
    }

    if( count_by_charges() ) {
        ret_mul *= charges;

    } else if( is_corpse() ) {
        cata_assert( corpse ); // To appease static analysis
        ret = corpse->weight;
        ret_mul = 1.0;
        if( has_flag( flag_FIELD_DRESS ) || has_flag( flag_FIELD_DRESS_FAILED ) ) {
            ret_mul *= 0.75;
        }
        if( has_flag( flag_GIBBED ) ) {
            ret_mul *= 0.85;
        }
        if( has_flag( flag_SKINNED ) ) {
            ret_mul *= 0.85;
        }
        if( has_flag( flag_QUARTERED ) ) {
            ret_mul *= 0.25;
        }

    }

    // prevent units::mass::max() * 1.0, which results in -units::mass::max()
    if( ret_mul != 1 ) {
        ret *= ret_mul;
    }

    // if it has additional pockets include the mass of those
    if( contents.has_additional_pockets() ) {
        ret += contents.get_additional_weight();
    }

    if( include_contents ) {
        ret += contents.item_weight_modifier();
    }

    // if this is an ammo belt add the weight of any implicitly contained linkages
    if( type->magazine && type->magazine->linkage ) {
        item links( *type->magazine->linkage );
        links.charges = ammo_remaining( );
        ret += links.weight();
    }

    // reduce weight for sawn-off barrel capped to the apportioned weight of the barrel
    if( gunmod_find( itype_barrel_small ) ) {
        const units::volume b = type->gun->barrel_volume;
        const units::mass max_barrel_weight = units::from_gram( to_milliliter( b ) );
        const units::mass barrel_weight = units::from_gram( b.value() * type->weight.value() /
                                          type->volume.value() );
        ret -= std::min( max_barrel_weight, barrel_weight );
    }

    // reduce weight for sawn-off stock
    if( gunmod_find( itype_stock_none ) ) {
        // Length taken from item::length(), height and width are "average" values
        const float stock_dimensions = 0.26f * 0.11f * 0.04f; // length * height * width = 0.00114 m3.
        // density of 'wood' material
        const int density = 850;
        ret -= units::from_kilogram( stock_dimensions * density );
    }

    return ret;
}

static units::length sawn_off_reduction( const itype *type )
{
    int barrel_percentage = type->gun->barrel_volume / ( type->volume / 100 );
    return type->longest_side * barrel_percentage / 100;
}

units::length item::length() const
{
    if( made_of( phase_id::LIQUID ) || ( is_soft() && is_container_empty() ) ) {
        return 0_mm;
    }

    if( is_corpse() ) {
        return units::default_length_from_volume<int>( corpse->volume );
    }

    if( is_gun() ) {
        units::length length_adjusted = type->longest_side;

        //TODO: Unhardcode this now that integral_longest_side can be used
        if( gunmod_find( itype_barrel_small ) ) {
            length_adjusted = type->longest_side - sawn_off_reduction( type );
        }

        std::vector<const item *> mods = gunmods();
        // only the longest thing sticking off the gun matters for length adjustment
        // TODO: Differentiate from back end mods like stocks vs front end like bayonets and muzzle devices
        units::length max_length = 0_mm;
        units::length stock_length = 0_mm;
        units::length stock_accessory_length = 0_mm;
        for( const item *mod : mods ) {
            // stocks and accessories for stocks are added with whatever the longest thing on the front is
            // stock mount is added here for support for sawing off stocks
            if( mod->type->gunmod->location.str() == "stock mount" ) {
                if( has_flag( flag_REMOVED_STOCK ) ) {
                    // stock has been removed so slightly more length removed about 26cm
                    stock_length -= 26_cm;
                }
            } else if( mod->type->gunmod->location.str() == "stock" ) {
                stock_length = mod->type->integral_longest_side;
                if( has_flag( flag_COLLAPSED_STOCK ) || has_flag( flag_FOLDED_STOCK ) ) {
                    // stock is folded so need to reduce length

                    // folded stock length is estimated at about 20 cm, LOP for guns should be about
                    // 13.5 inches and a folding stock folds past the trigger and isn't perfectly efficient
                    stock_length -= 20_cm;
                }
            } else if( mod->type->gunmod->location.str() == "stock accessory" ) {
                stock_accessory_length = mod->type->integral_longest_side;
            } else {
                units::length l = mod->type->integral_longest_side;
                if( l > max_length ) {
                    max_length = l;
                }
            }
        }
        return length_adjusted + max_length + stock_length + stock_accessory_length;
    }

    units::length max = is_soft() ? 0_mm : type->longest_side;
    max = std::max( contents.item_length_modifier(), max );
    return max;
}

units::length item::barrel_length() const
{
    if( is_gun() ) {
        units::length l = type->gun->barrel_length;
        for( const item *mod : mods() ) {
            // if a gun has a barrel length specifying mod then use that length for sure
            if( mod->type->gunmod->barrel_length > 0_mm ) {
                l = mod->type->gunmod->barrel_length;
                // if we find an explicit barrel mod then use that and quit the loop
                break;
            }
        }
        // if we've sawn off the barrel reduce the damage
        if( gunmod_find( itype_barrel_small ) ) {
            l = l - sawn_off_reduction( type );
        }

        return l;
    } else {
        return 0_mm;
    }
}

units::volume item::collapsed_volume_delta() const
{
    units::volume delta_volume = 0_ml;

    // COLLAPSIBLE_STOCK is the legacy version of this just here for mod support
    // TODO Remove at some point
    if( is_gun() && ( has_flag( flag_COLLAPSIBLE_STOCK ) || has_flag( flag_COLLAPSED_STOCK ) ||
                      has_flag( flag_REMOVED_STOCK ) ) ) {
        // consider only the base size of the gun (without mods)
        int tmpvol = get_var( "volume",
                              ( type->volume - type->gun->barrel_volume ) / 250_ml );
        if( tmpvol <= 3 ) {
            // intentional NOP
        } else if( tmpvol <= 5 ) {
            delta_volume = 250_ml;
        } else if( tmpvol <= 6 ) {
            delta_volume = 500_ml;
        } else if( tmpvol <= 9 ) {
            delta_volume = 750_ml;
        } else if( tmpvol <= 12 ) {
            delta_volume = 1000_ml;
        } else if( tmpvol <= 15 ) {
            delta_volume = 1250_ml;
        } else {
            delta_volume = 1500_ml;
        }
    }

    return delta_volume;
}

units::volume item::corpse_volume( const mtype *corpse ) const
{
    units::volume corpse_volume = corpse->volume;
    if( has_flag( flag_QUARTERED ) ) {
        corpse_volume *= 0.25;
    }
    if( has_flag( flag_FIELD_DRESS ) || has_flag( flag_FIELD_DRESS_FAILED ) ) {
        corpse_volume *= 0.75;
    }
    if( has_flag( flag_GIBBED ) ) {
        corpse_volume *= 0.85;
    }
    if( has_flag( flag_SKINNED ) ) {
        corpse_volume *= 0.85;
    }
    if( corpse_volume > MAX_ITEM_VOLUME ) {
        // Silently set volume so the corpse can still spawn but a mtype can have a volume > MAX_ITEM_VOLUME
        corpse_volume = MAX_ITEM_VOLUME;
    }
    if( corpse_volume > 0_ml ) {
        return corpse_volume;
    }
    debugmsg( "invalid monster volume for corpse of %s", corpse->id.str() );
    return 0_ml;
}

units::volume item::base_volume() const
{
    if( is_null() ) {
        return 0_ml;
    }
    if( is_corpse() ) {
        return corpse_volume( corpse );
    }

    if( is_craft() ) {
        units::volume ret = 0_ml;
        for( const item_components::type_vector_pair &tvp : components ) {
            for( const item &it : tvp.second ) {
                ret += it.base_volume();
            }
        }
        return ret;
    }

    if( count_by_charges() && type->stack_size > 0 ) {
        if( type->volume % type->stack_size == 0_ml ) {
            return type->volume / type->stack_size;
        } else {
            return type->volume / type->stack_size + 1_ml;
        }
    }

    return type->volume;
}

units::volume item::volume( bool integral, bool ignore_contents, int charges_in_vol ) const
{
    charges_in_vol = charges_in_vol < 0 || charges_in_vol > charges ? charges : charges_in_vol;
    if( is_null() ) {
        return 0_ml;
    }

    if( is_corpse() ) {
        return corpse_volume( corpse );
    }

    if( is_craft() ) {
        if( !craft_data_->cached_volume ) {
            units::volume ret = 0_ml;
            for( const item_components::type_vector_pair &tvp : components ) {
                for( const item &it : tvp.second ) {
                    ret += it.volume();
                }
            }
            // 1 mL minimum craft volume to avoid 0 volume errors from practices or hammerspace
            craft_data_->cached_volume = std::max( ret, 1_ml );
        }
        return *craft_data_->cached_volume;
    }

    const int local_volume = get_var( "volume", -1 );
    units::volume ret;
    if( local_volume >= 0 ) {
        ret = local_volume * 250_ml;
    } else if( integral ) {
        ret = type->integral_volume;
    } else {
        ret = type->volume;
    }

    if( count_by_charges() || made_of( phase_id::LIQUID ) ) {
        units::quantity<int64_t, units::volume_in_milliliter_tag> num = ret * static_cast<int64_t>
                ( charges_in_vol );
        if( type->stack_size <= 0 ) {
            if( type->charges_default() <= 0 ) {
                debugmsg( "Item type %s has invalid default charges %d", typeId().str(), type->charges_default() );
            } else {
                debugmsg( "Item type %s has invalid stack_size %d", typeId().str(), type->stack_size );
            }
            ret = num;
        } else {
            ret = num / type->stack_size;
            if( num % type->stack_size != 0_ml ) {
                ret += 1_ml;
            }
        }
    }

    if( !ignore_contents ) {
        ret += contents.item_size_modifier();
    }

    // if it has additional pockets include the volume of those
    if( contents.has_additional_pockets() ) {
        ret += contents.get_additional_volume();
    }

    // TODO: do a check if the item is collapsed or not
    ret -= collapsed_volume_delta();

    if( is_gun() ) {
        for( const item *elem : gunmods() ) {
            ret += elem->volume( true );
        }

        if( gunmod_find( itype_barrel_small ) ) {
            ret -= type->gun->barrel_volume;
        }
    }

    return ret;
}

int item::lift_strength() const
{
    const int mass = units::to_gram( weight() );
    return std::max( mass / 10000, 1 );
}

int item::attack_time( const Character &you ) const
{
    int ret = 65 + ( volume() / 62.5_ml + weight() / 60_gram ) / count();
    ret = calculate_by_enchantment_wield( you, ret, enchant_vals::mod::ITEM_ATTACK_SPEED,
                                          true );
    return ret;
}

int item::damage_melee( const damage_type_id &dt ) const
{
    if( is_null() ) {
        return 0;
    }

    // effectiveness is reduced by 10% per damage level
    int res = 0;
    if( type->melee.damage_map.count( dt ) > 0 ) {
        res = type->melee.damage_map.at( dt );
    }
    res = damage_adjusted_melee_weapon_damage( res, dt );

    // apply type specific flags
    // FIXME: Hardcoded damage types
    if( dt == damage_bash && has_flag( flag_REDUCED_BASHING ) ) {
        res *= 0.5;
    }
    if( dt->edged && has_flag( flag_DIAMOND ) ) {
        res *= 1.3;
    }

    // consider any attached bayonets
    if( is_gun() ) {
        std::vector<int> opts = { res };
        for( const item *mod : gunmods() ) {
            if( mod->type->gunmod->is_bayonet ) {
                opts.push_back( mod->damage_melee( dt ) );
            }
        }
        return *std::max_element( opts.begin(), opts.end() );

    }

    return std::max( res, 0 );
}

damage_instance item::base_damage_melee() const
{
    // TODO: Caching
    damage_instance ret;
    for( const damage_type &dt : damage_type::get_all() ) {
        int dam = damage_melee( dt.id );
        if( dam > 0 ) {
            ret.add_damage( dt.id, dam );
        }
    }

    return ret;
}

damage_instance item::base_damage_thrown() const
{
    // TODO: Create a separate cache for individual items (for modifiers like diamond etc.)
    return type->thrown_damage;
}

int item::reach_range( const Character &guy ) const
{
    int res = 1;
    int reach_attack_add = has_flag( flag_REACH_ATTACK ) ? has_flag( flag_REACH3 ) ? 2 : 1 : 0;

    res += reach_attack_add;

    if( !is_gun() ) {
        res = std::max( 1, static_cast<int>( guy.calculate_by_enchantment( res,
                                             enchant_vals::mod::MELEE_RANGE_MODIFIER ) ) );
    }

    // for guns consider any attached gunmods
    if( is_gun() && !is_gunmod() ) {
        for( const std::pair<const gun_mode_id, gun_mode> &m : gun_all_modes() ) {
            if( guy.is_npc() && m.second.flags.count( "NPC_AVOID" ) ) {
                continue;
            }
            if( m.second.melee() ) {
                res = std::max( res, std::max( 1,
                                               static_cast<int>( guy.calculate_by_enchantment( m.second.qty + reach_attack_add,
                                                       enchant_vals::mod::MELEE_RANGE_MODIFIER ) ) ) );
            }
        }
    }

    return res;
}

int item::current_reach_range( const Character &guy ) const
{
    int res = 1;

    if( has_flag( flag_REACH_ATTACK ) ) {
        res = has_flag( flag_REACH3 ) ? 3 : 2;
    } else if( is_gun() && !is_gunmod() && gun_current_mode().melee() ) {
        res = gun_current_mode().target->gun_range();
    }

    if( !is_gun() || ( is_gun() && !is_gunmod() && gun_current_mode().melee() ) ) {
        res = std::max( 1, static_cast<int>( guy.calculate_by_enchantment( res,
                                             enchant_vals::mod::MELEE_RANGE_MODIFIER ) ) );
    }

    if( is_gun() && !is_gunmod() ) {
        gun_mode gun = gun_current_mode();
        if( !( guy.is_npc() && gun.flags.count( "NPC_AVOID" ) ) && gun.melee() ) {
            res = std::max( res, gun.qty );
        }
    }

    return res;
}

void item::unset_flags()
{
    item_tags.clear();
    requires_tags_processing = true;
}

bool item::has_fault( const fault_id &fault ) const
{
    return faults.count( fault );
}

bool item::has_fault_of_type( const std::string &fault_type ) const
{
    for( const fault_id &f : faults ) {
        if( f.obj().type() == fault_type ) {
            return true;
        }
    }
    return false;
}

bool item::has_fault_flag( const std::string &searched_flag ) const
{
    for( const fault_id &fault : faults ) {
        if( fault->has_flag( searched_flag ) ) {
            return true;
        }
    }
    return false;
}

bool item::has_own_flag( const flag_id &f ) const
{
    return item_tags.find( f ) != item_tags.end();
}

bool item::has_flag( const flag_id &f ) const
{
    bool ret = false;
    if( !f.is_valid() ) {
        debugmsg( "Attempted to check invalid flag_id %s", f.str() );
        return false;
    }

    ret = inherited_tags_cache.find( f ) != inherited_tags_cache.end();
    if( ret ) {
        return ret;
    }

    // other item type flags
    ret = type->has_flag( f );
    if( ret ) {
        return ret;
    }

    // now check for item specific flags
    ret = has_own_flag( f );
    return ret;
}

item &item::set_flag( const flag_id &flag )
{
    if( flag.is_valid() ) {
        item_tags.insert( flag );
        update_prefix_suffix_flags( flag );
        requires_tags_processing = true;
    } else {
        debugmsg( "Attempted to set invalid flag_id %s", flag.str() );
    }
    return *this;
}

bool item::has_vitamin( const vitamin_id &v ) const
{
    if( !this->is_comestible() ) {
        return false;
    }
    const nutrients food_item = default_character_compute_effective_nutrients( *this );
    for( auto const& [vit_id, amount] : food_item.vitamins() ) {
        if( vit_id == v ) {
            if( amount > 0 ) {
                return true;
            } else {
                break;
            }
        }
    }
    return false;
}

std::string item::get_fault_description( const fault_id &f_id ) const
{
    return string_format( f_id.obj().description(), type->nname( 1 ) );
}

bool item::can_have_fault( const fault_id &f_id )
{
    // f_id fault is not defined in itype
    if( type->faults.get_specific_weight( f_id ) == 0 ) {
        return false;
    }

    // f_id is blocked by some another fault
    for( const fault_id &faults_of_item : faults ) {
        for( const fault_id &blocked_fault : faults_of_item.obj().get_block_faults() ) {
            if( f_id == blocked_fault ) {
                return false;
            }
        }
    }

    return true;
}

bool item::set_fault( const fault_id &f_id, bool force, bool message )
{
    if( !force && !can_have_fault( f_id ) ) {
        return false;
    }

    // if f_id fault blocks fault A, we should remove fault A before applying fault f_id
    // can't have chipped blade if the blade is gone
    for( const fault_id &f_blocked : f_id.obj().get_block_faults() ) {
        remove_fault( f_blocked );
    }

    if( message ) {
        add_msg( m_bad, f_id.obj().message() );
    }

    faults.insert( f_id );
    return true;
}

void item::set_random_fault_of_type( const std::string &fault_type, bool force, bool message )
{
    if( force ) {
        set_fault( random_entry( faults::all_of_type( fault_type ) ), true, message );
        return;
    }

    weighted_int_list<fault_id> faults_by_type;
    for( const std::pair<fault_id, int> &f : type->faults ) {
        if( f.first.obj().type() == fault_type && can_have_fault( f.first ) ) {
            faults_by_type.add( f.first, f.second );
        }

    }

    if( !faults_by_type.empty() ) {
        set_fault( *faults_by_type.pick(), force, message );
    }

}

void item::remove_fault( const fault_id &fault_id )
{
    faults.erase( fault_id );
}

void item::remove_single_fault_of_type( const std::string &fault_type )
{
    for( const fault_id f : faults ) {
        if( f.obj().type() == fault_type ) {
            faults.erase( f );
            return;
        }
    }
}

item &item::unset_flag( const flag_id &flag )
{
    item_tags.erase( flag );
    update_prefix_suffix_flags();
    requires_tags_processing = true;
    return *this;
}

item &item::set_flag_recursive( const flag_id &flag )
{
    set_flag( flag );
    for( item_components::type_vector_pair &tvp : components ) {
        for( item &it : tvp.second ) {
            it.set_flag_recursive( flag );
        }
    }
    return *this;
}

const item::FlagsSetType &item::get_flags() const
{
    return item_tags;
}

const item::FlagsSetType &item::get_prefix_flags() const
{
    return prefix_tags_cache;
}

const item::FlagsSetType &item::get_suffix_flags() const
{
    return suffix_tags_cache;
}

bool item::has_property( const std::string &prop ) const
{
    return type->properties.find( prop ) != type->properties.end();
}

std::string item::get_property_string( const std::string &prop, const std::string &def ) const
{
    const auto it = type->properties.find( prop );
    return it != type->properties.end() ? it->second : def;
}

int64_t item::get_property_int64_t( const std::string &prop, int64_t def ) const
{
    const auto it = type->properties.find( prop );
    if( it != type->properties.end() ) {
        char *e = nullptr;
        int64_t r = std::strtoll( it->second.c_str(), &e, 10 );
        if( !it->second.empty() && *e == '\0' ) {
            return r;
        }
        debugmsg( "invalid property '%s' for item '%s'", prop.c_str(), tname() );
    }
    return def;
}

bool item::has_quality_nonrecursive( const quality_id &qual, int level ) const
{
    return get_quality_nonrecursive( qual ) >= level;
}

int item::get_quality_nonrecursive( const quality_id &id, const bool strict_boiling ) const
{
    /**
     * EXCEPTION: Items with quality BOIL only count as such if they are empty.
     */
    if( strict_boiling && id == qual_BOIL && !contents.empty_container() ) {
        return INT_MIN;
    }

    int return_quality = INT_MIN;

    // Check for inherent item quality
    for( const std::pair<const quality_id, int> &quality : type->qualities ) {
        if( quality.first == id ) {
            return_quality = quality.second;
        }
    }

    // If tool has charged qualities and enough charge to use at least once
    // (using ammo_remaining() with player character to include bionic/UPS power)
    if( !type->charged_qualities.empty() && ammo_sufficient( &get_player_character() ) ) {
        // see if any charged qualities are better than the current one
        for( const std::pair<const quality_id, int> &quality : type->charged_qualities ) {
            if( quality.first == id ) {
                return_quality = std::max( return_quality, quality.second );
            }
        }
    }

    return return_quality;
}

int item::get_quality( const quality_id &id, const bool strict_boiling ) const
{
    /**
     * EXCEPTION: Items with quality BOIL only count as such if they are empty.
     */
    if( strict_boiling && id == qual_BOIL && !contents.empty_container() ) {
        return INT_MIN;
    }

    // First check the item itself
    int return_quality = get_quality_nonrecursive( id, false );

    // If any contained item has a better quality, use that instead
    return_quality = std::max( return_quality, contents.best_quality( id ) );

    return return_quality;
}

bool item::has_technique( const matec_id &tech ) const
{
    return type->techniques.count( tech ) > 0 || techniques.count( tech ) > 0;
}

void item::add_technique( const matec_id &tech )
{
    techniques.insert( tech );
}

std::vector<item *> item::toolmods()
{
    std::vector<item *> res;
    if( is_tool() ) {
        for( item *e : contents.all_items_top( pocket_type::MOD ) ) {
            if( e->is_toolmod() ) {
                res.push_back( e );
            }
        }
    }
    return res;
}

std::vector<const item *> item::toolmods() const
{
    std::vector<const item *> res;
    if( is_tool() ) {
        for( const item *e : contents.all_items_top( pocket_type::MOD ) ) {
            if( e->is_toolmod() ) {
                res.push_back( e );
            }
        }
    }
    return res;
}

std::set<matec_id> item::get_techniques() const
{
    std::set<matec_id> result = type->techniques;
    result.insert( techniques.begin(), techniques.end() );
    return result;
}

int item::get_comestible_fun() const
{
    if( !is_comestible() ) {
        return 0;
    }
    int fun = get_comestible()->fun;
    for( const flag_id &flag : item_tags ) {
        fun += flag->taste_mod();
    }
    for( const flag_id &flag : type->get_flags() ) {
        fun += flag->taste_mod();
    }

    if( has_flag( flag_MUSHY ) ) {
        return std::min( -5, fun ); // defrosted MUSHY food is practically tasteless or tastes off
    }

    return fun;
}

bool item::goes_bad() const
{
    if( item_internal::goes_bad_cache_is_for( this ) ) {
        return item_internal::goes_bad_cache_fetch();
    }
    if( has_flag( flag_PROCESSING ) ) {
        return false;
    }
    if( is_corpse() ) {
        // Corpses rot only if they are made of rotting materials
        // They also only rot if they are not dormant
        if( corpse->has_flag( mon_flag_DORMANT ) ) {
            return false;
        }
        return made_of_any( materials::get_rotting() );
    }
    return is_comestible() && get_comestible()->spoils != 0_turns;
}

time_duration item::get_shelf_life() const
{
    if( goes_bad() ) {
        if( is_comestible() ) {
            return get_comestible()->spoils;
        } else if( is_corpse() ) {
            return 24_hours;
        }
    }
    return 0_turns;
}

double item::get_relative_rot() const
{
    if( goes_bad() ) {
        return rot / get_shelf_life();
    }
    return 0;
}

void item::set_relative_rot( double val )
{
    if( goes_bad() ) {
        rot = get_shelf_life() * val;
        // calc_rot uses last_temp_check (when it's not turn_zero) instead of bday.
        // this makes sure the rotting starts from now, not from bday.
        // if this item is the result of smoking or milling don't do this, we want to start from bday.
        if( !has_flag( flag_PROCESSING_RESULT ) ) {
            last_temp_check = calendar::turn;
        }
    }
}

void item::set_rot( time_duration val )
{
    rot = val;
}

void item::randomize_rot()
{
    if( is_comestible() && get_comestible()->spoils > 0_turns ) {
        const double x_input = rng_float( 0.0, 1.0 );
        const double k_rot = ( 1.0 - x_input ) / ( 1.0 + 2.0 * x_input );
        set_rot( get_shelf_life() * k_rot );
    }

    for( item_pocket *pocket : contents.get_all_contained_pockets() ) {
        if( pocket->spoil_multiplier() > 0.0f ) {
            for( item *subitem : pocket->all_items_top() ) {
                subitem->randomize_rot();
            }
        }
    }
}

int item::spoilage_sort_order() const
{
    int bottom = std::numeric_limits<int>::max();

    bool any_goes_bad = false;
    time_duration min_spoil_time = calendar::INDEFINITELY_LONG_DURATION;
    visit_items( [&]( item * const node, item * const parent ) {
        if( node && node->goes_bad() ) {
            float spoil_multiplier = 1.0f;
            if( parent ) {
                const item_pocket *const parent_pocket = parent->contained_where( *node );
                if( parent_pocket ) {
                    spoil_multiplier = parent_pocket->spoil_multiplier();
                }
            }
            if( spoil_multiplier > 0.0f ) {
                time_duration remaining_shelf_life = node->get_shelf_life() - node->rot;
                if( !any_goes_bad || min_spoil_time * spoil_multiplier > remaining_shelf_life ) {
                    any_goes_bad = true;
                    min_spoil_time = remaining_shelf_life / spoil_multiplier;
                }
            }
        }
        return VisitResponse::NEXT;
    } );
    if( any_goes_bad ) {
        return to_turns<int>( min_spoil_time );
    }

    if( get_comestible() ) {
        if( get_category_shallow().get_id() == item_category_food ) {
            return bottom - 3;
        } else if( get_category_shallow().get_id() == item_category_drugs ) {
            return bottom - 2;
        } else {
            return bottom - 1;
        }
    }
    return bottom;
}

/**
 * Food decay calculation.
 * Calculate how much food rots per hour, based on 3600 rot/hour at 65 F (18.3 C).
 * Rate of rot doubles every 16 F (~8.8888 C) increase in temperature
 * Rate of rot halves every 16 F decrease in temperature
 * Rot maxes out at 105 F
 * Rot stops below 32 F (0C) and above 145 F (63 C)
 */
float item::calc_hourly_rotpoints_at_temp( const units::temperature &temp ) const
{
    const units::temperature dropoff = units::from_fahrenheit( 38 ); // F, ~3 C
    const float max_rot_temp = 105; // F, ~41 C, Maximum rotting rate is at this temperature

    if( temp <= temperatures::freezing ) {
        return 0.f;
    } else if( temp < dropoff ) {
        // ditch our fancy equation and do a linear approach to 0 rot from 38 F (3 C) -> 32 F (0 C)
        return 600.f * std::exp2( -27.f / 16.f ) * ( units::to_fahrenheit( temp ) - units::to_fahrenheit(
                    temperatures::freezing ) );
    } else if( temp < units::from_fahrenheit( max_rot_temp ) ) {
        // Exponential progress from 38 F (3 C) to 105 F (41 C)
        return 3600.f * std::exp2( ( units::to_fahrenheit( temp ) - 65.f ) / 16.f );
    } else {
        // Constant rot from 105 F (41 C) upwards
        // This is approximately 20364.67 rot/hour
        return 3600.f * std::exp2( ( max_rot_temp - 65.f ) / 16.f );
    }
}

void item::calc_rot( units::temperature temp, const float spoil_modifier,
                     const time_duration &time_delta )
{
    // Avoid needlessly calculating already rotten things.  Corpses should
    // always rot away and food rots away at twice the shelf life.  If the food
    // is in a sealed container they won't rot away, this avoids needlessly
    // calculating their rot in that case.
    if( !is_corpse() && get_relative_rot() > 2.0 ) {
        return;
    }

    if( has_own_flag( flag_FROZEN ) ) {
        return;
    }

    // rot modifier
    float factor = spoil_modifier;
    if( is_corpse() && has_flag( flag_FIELD_DRESS ) ) {
        factor *= 0.75;
    }
    if( has_own_flag( flag_MUSHY ) ) {
        factor *= 3.0;
    }
    // Food irradiation can quadruple the shelf life.
    // "Shelf life of ground beef patties treated by gamma radiation":
    // > Beef patties treated at 1.0 and 3.0 kGy [spoiled] by day 14 and 21, ...
    // > patties treated at 5.0 kGy did not spoil until 42 days.
    // > The nonirradiated control samples for both batches of ground beef spoiled within 7 days
    // We get 0.5, 0.33, and 0.167. 0.25 seems reasonable for irradiation
    if( has_own_flag( flag_IRRADIATED ) ) {
        factor *= 0.25;
    }

    if( has_own_flag( flag_COLD ) ) {
        temp = std::min( temperatures::fridge, temp );
    }

    rot += factor * time_delta / 1_seconds * calc_hourly_rotpoints_at_temp( temp ) * 1_turns /
           ( 1_hours / 1_seconds );
}

void item::calc_rot_while_processing( time_duration processing_duration )
{
    if( !has_own_flag( flag_PROCESSING ) ) {
        debugmsg( "calc_rot_while_processing called on non smoking item: %s", tname() );
        return;
    }

    // Apply no rot or temperature while smoking
    last_temp_check += processing_duration;
}

bool item::process_decay_in_air( map &here, Character *carrier, const tripoint_bub_ms &pos,
                                 int max_air_exposure_hours,
                                 time_duration time_delta )
{
    if( !has_own_flag( flag_FROZEN ) ) {
        double environment_multiplier = here.is_outside( pos ) ? 2.0 : 1.0;
        time_duration new_air_exposure = time_duration::from_seconds( item_counter ) + time_delta *
                                         rng_normal( 0.9, 1.1 ) * environment_multiplier;
        if( new_air_exposure >= time_duration::from_hours( max_air_exposure_hours ) ) {
            convert( *type->revert_to, carrier );
            return true;
        }
        item_counter = to_seconds<int>( new_air_exposure );
    }
    return false;
}

int item::get_env_resist( int override_base_resist ) const
{
    const islot_armor *t = find_armor_data();
    if( t == nullptr ) {
        return is_pet_armor() ? type->pet_armor->env_resist : 0;
    }
    // modify if item is a gas mask and has filter
    int resist_base = t->avg_env_resist();
    int resist_filter = get_var( "overwrite_env_resist", 0 );
    int resist = std::max( { resist_base, resist_filter, override_base_resist } );

    return std::lround( resist * get_relative_health() );
}

int item::get_base_env_resist_w_filter() const
{
    const islot_armor *t = find_armor_data();
    if( t == nullptr ) {
        return is_pet_armor() ? type->pet_armor->env_resist_w_filter : 0;
    }
    return t->avg_env_resist_w_filter();
}

time_duration item::brewing_time() const
{
    return is_brewable() ? type->brewable->time * calendar::season_from_default_ratio() : 0_turns;
}

const std::map<itype_id, int> &item::brewing_results() const
{
    static const std::map<itype_id, int> nulresult{};
    return is_brewable() ? type->brewable->results : nulresult;
}

time_duration item::composting_time() const
{
    return is_compostable() ? type->compostable->time * calendar::season_from_default_ratio() : 0_turns;
}

const std::map<itype_id, int> &item::composting_results() const
{
    static const std::map<itype_id, int> nulresult{};
    return is_compostable() ? type->compostable->results : nulresult;
}

bool item::can_revive() const
{
    return is_corpse() && ( corpse->has_flag( mon_flag_REVIVES ) || has_var( "zombie_form" ) ) &&
           damage() < max_damage() &&
           !( has_flag( flag_FIELD_DRESS ) || has_flag( flag_FIELD_DRESS_FAILED ) ||
              has_flag( flag_QUARTERED ) ||
              has_flag( flag_SKINNED ) || has_flag( flag_PULPED ) );
}

bool item::ready_to_revive( map &here, const tripoint_bub_ms &pos ) const
{
    if( !can_revive() ) {
        return false;
    }
    if( here.veh_at( pos ) ) {
        return false;
    }
    if( !calendar::once_every( 1_seconds ) ) {
        return false;
    }
    int age_in_hours = to_hours<int>( age() );
    age_in_hours -= static_cast<int>( static_cast<float>( burnt ) / ( volume() / 250_ml ) );
    if( damage_level() > 0 ) {
        age_in_hours /= ( damage_level() + 1 );
    }
    int rez_factor = 48 - age_in_hours;
    if( age_in_hours > 6 && ( rez_factor <= 0 || one_in( rez_factor ) ) ) {
        // If we're a special revival zombie, wait to get up until the player is nearby.
        const bool isReviveSpecial = has_flag( flag_REVIVE_SPECIAL );
        if( isReviveSpecial ) {
            const int distance = rl_dist( pos, get_player_character().pos_bub() );
            if( distance > 3 ) {
                return false;
            }
            if( !one_in( distance + 1 ) ) {
                return false;
            }
        }

        return true;
    }
    return false;
}

bool item::is_money() const
{
    return is_money( ammo_types() );
}

bool item::is_money( const std::set<ammotype> &ammo ) const
{
    return ammo.count( ammo_money );
}

bool item::is_cash_card() const
{
    return typeId() == itype_cash_card;
}

bool item::is_estorage() const
{
    return contents.has_pocket_type( pocket_type::E_FILE_STORAGE );
}

bool item::is_estorage_usable( const Character &who ) const
{
    return is_estorage() && is_browsed() && efile_activity_actor::edevice_has_use( this ) &&
           ammo_sufficient( &who );
}

bool item::is_estorable() const
{
    return has_flag( flag_E_STORABLE ) || has_flag( flag_E_STORABLE_EXCLUSIVE ) || is_book();
}

bool item::is_estorable_exclusive() const
{
    return has_flag( flag_E_STORABLE_EXCLUSIVE );
}

bool item::is_browsed() const
{
    return get_var( "browsed", "false" ) == "true";
}

void item::set_browsed( bool browsed )
{
    if( browsed ) {
        set_var( "browsed", "true" );
    } else {
        set_var( "browsed", "false" );
    }
}

bool item::is_ecopiable() const
{
    return has_flag( flag_E_COPIABLE ) || ( is_book() && !type->book->generic );
}

bool item::efiles_all_browsed() const
{
    if( is_browsed() ) {
        return true;
    }
    std::vector<const item *> all_efiles = efiles();
    if( !all_efiles.empty() ) {
        for( const item *i : all_efiles ) {
            if( !i->is_browsed() ) {
                return false;
            }
        }
    }
    return true;
}

units::ememory item::ememory_size() const
{
    units::ememory ememory_return = type->ememory_size;
    if( typeId() == itype_efile_recipes ) {
        ememory_return *= get_saved_recipes().size();
    } else if( typeId() == itype_efile_photos ) {
        ememory_return *= total_photos();
    }
    return ememory_return;
}

units::ememory item::occupied_ememory() const
{
    std::vector<const item *> all_efiles = efiles();
    units::ememory total = 0_KB;
    for( const item *i : all_efiles ) {
        total += i->ememory_size();
    }
    return total;
}

units::ememory item::total_ememory() const
{
    std::vector<const item_pocket *> pockets = contents.get_pockets( []( item_pocket const & pocket ) {
        return pocket.is_type( pocket_type::E_FILE_STORAGE );
    } );
    if( pockets.size() > 1 ) {
        debugmsg( "no more than one E_FILE_STORAGE pocket is allowed" );
    }
    if( pockets.empty() ) {
        return 0_KB;
    }
    return pockets.back()->get_pocket_data()->ememory_max;
}

units::ememory item::remaining_ememory() const
{
    return total_ememory() - occupied_ememory();
}

item *item::get_recipe_catalog()
{
    const std::function<bool( const item &i )> filter = []( const item & i ) {
        cata::value_ptr<memory_card_info> recipe_gen = i.typeId()->memory_card_data;
        if( recipe_gen ) {
            return true;
        }
        return false;
    };
    if( filter( *this ) ) {
        return this;
    }
    return get_contents().get_item_with( filter );
}

const item *item::get_recipe_catalog() const
{
    const std::function<bool( const item &i )> filter = []( const item & i ) {
        cata::value_ptr<memory_card_info> recipe_gen = i.typeId()->memory_card_data;
        if( recipe_gen ) {
            return true;
        }
        return false;
    };
    if( filter( *this ) ) {
        return this;
    }
    return get_contents().get_item_with( filter );
}

item *item::get_photo_gallery()
{
    const std::function<bool( const item &i )> filter = []( const item & i ) {
        return i.typeId() == itype_efile_photos;
    };
    return get_contents().get_item_with( filter );
}

const item *item::get_photo_gallery() const
{
    const std::function<bool( const item &i )> filter = []( const item & i ) {
        return i.typeId() == itype_efile_photos;
    };
    return get_contents().get_item_with( filter );
}

int item::total_photos() const
{
    std::vector<item::extended_photo_def> extended_photos;
    read_extended_photos( extended_photos, "CAMERA_EXTENDED_PHOTOS", true );
    read_extended_photos( extended_photos, "CAMERA_MONSTER_PHOTOS", true );
    return extended_photos.size();
}

bool item::is_software() const
{
    return type->category_force == item_category_software;
}

bool item::is_maybe_melee_weapon() const
{
    item_category_id my_cat_id = get_category_shallow().id;
    return my_cat_id == item_category_weapons || my_cat_id == item_category_spare_parts ||
           my_cat_id == item_category_tools;
}

bool item::made_of_any_food_components( bool deep_search ) const
{
    if( components.empty() ) {
        return false;
    }

    for( const std::pair<itype_id, std::vector<item>> pair : components ) {
        for( const item &it : pair.second ) {
            if( it.is_food() || ( deep_search && it.made_of_any_food_components( deep_search ) ) ) {
                return true;
            }
        }
    }
    return false;
}

bool item::has_edged_damage() const
{
    for( const damage_type &dt : damage_type::get_all() ) {
        if( dt.edged && is_melee( dt.id ) ) {
            return true;
        }
    }
    return false;
}

bool item::count_by_charges() const
{
    return type->count_by_charges();
}

void item::compress_charges_or_liquid( int &compcount )
{
    if( count_by_charges() || made_of( phase_id::LIQUID ) ) {
        charges = compcount;
        compcount = 1;
    } else if( !craft_has_charges() && charges > 0 ) {
        // tools that can be unloaded should be created unloaded,
        // tools that can't be unloaded will keep their default charges.
        charges = 0;
    }
}

int item::count() const
{
    return count_by_charges() ? charges : 1;
}

bool item::craft_has_charges() const
{
    return count_by_charges() || ammo_types().empty();
}

#if defined(_MSC_VER)
// Deal with MSVC compiler bug (#17791, #17958)
#pragma optimize( "", off )
#endif

bool item::damage_type_can_damage_items( const damage_type_id &dmg_type ) const
{
    return dmg_type->physical;
}

template<typename bodypart_target>
float item::resist( const damage_type_id &dmg_type, const bool to_self,
                    const bodypart_target &bp,
                    const int resist_value ) const
{
    if( is_null() ) {
        return 0.0f;
    }

    if( dmg_type.is_null() ) {
        return 0.0f;
    }

    if( !dmg_type.is_valid() ) {
        debugmsg( "Invalid damage type: %d", dmg_type.c_str() );
        return 0.0f;
    }

    // Implicit item damage immunity for damage types listed prior to bash
    // Acid/fire immunity would be handled in _environmental_resist, but there are more
    // dmg types that do not affect items such as PURE/COLD etc..
    if( to_self && !damage_type_can_damage_items( dmg_type ) ) {
        return std::numeric_limits<float>::max();
    }

    const bool bp_null = bp == bodypart_target();
    const std::vector<const part_material *> &armor_mats = armor_made_of( bp );
    const float avg_thickness = bp_null ? get_thickness() : get_thickness( bp );
    return _resist( dmg_type, to_self, resist_value, bp_null, armor_mats, avg_thickness );
}
template float item::resist<bodypart_id>( const damage_type_id &dmg_type,
        const bool to_self, const bodypart_id &bp, const int resist_value ) const;
template float item::resist<sub_bodypart_id>( const damage_type_id &dmg_type,
        const bool to_self, const sub_bodypart_id &bp, const int resist_value ) const;

float item::_resist( const damage_type_id &dmg_type, bool to_self, int resist_value,
                     const bool bp_null,
                     const std::vector<const part_material *> &armor_mats,
                     const float avg_thickness ) const
{
    if( dmg_type->env ) {
        return _environmental_resist( dmg_type, to_self, resist_value, bp_null, armor_mats );
    }

    std::optional<std::pair<damage_type_id, float>> derived;
    if( !dmg_type->derived_from.first.is_null() ) {
        derived = dmg_type->derived_from;
    }

    float resist = 0.0f;
    float mod = get_clothing_mod_val_for_damage_type( dmg_type );

    const float damage_scale = damage_adjusted_armor_resist( 1.0f, dmg_type );

    if( !bp_null ) {
        // If we have armour portion materials for this body part, use that instead
        if( !armor_mats.empty() ) {
            for( const part_material *m : armor_mats ) {
                // only count the material if it's hit

                // if roll is -1 each material is rolled at this point individually
                int internal_roll;
                resist_value < 0 ? internal_roll = rng( 0, 99 ) : internal_roll = resist_value;
                if( internal_roll < m->cover ) {
                    float tmp_add = 0.f;
                    if( derived.has_value() && !m->id->has_dedicated_resist( dmg_type ) ) {
                        tmp_add = m->id->resist( derived->first ) * m->thickness * derived->second;
                    } else {
                        tmp_add = m->id->resist( dmg_type ) * m->thickness;
                    }
                    resist += tmp_add;
                }
            }
            return ( resist + mod ) * damage_scale;
        }
    }

    // base resistance
    const std::map<material_id, int> mats = made_of();
    if( !mats.empty() ) {
        const int total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;
        for( const auto &m : mats ) {
            float tmp_add = 0.f;
            if( derived.has_value() && !m.first->has_dedicated_resist( dmg_type ) ) {
                tmp_add = m.first->resist( derived->first ) * m.second * derived->second;
            } else {
                tmp_add = m.first->resist( dmg_type ) * m.second;
            }
            resist += tmp_add;
        }
        // Average based portion of materials
        resist /= total;
    }

    return ( resist * avg_thickness + mod ) * damage_scale;
}

float item::_environmental_resist( const damage_type_id &dmg_type, const bool to_self,
                                   int base_env_resist,
                                   const bool bp_null,
                                   const std::vector<const part_material *> &armor_mats ) const
{
    if( to_self ) {
        // Currently no items are damaged by acid, and fire is handled elsewhere
        return std::numeric_limits<float>::max();
    }

    std::optional<std::pair<damage_type_id, float>> derived;
    if( !dmg_type->derived_from.first.is_null() ) {
        derived = dmg_type->derived_from;
    }

    float resist = 0.0f;
    float mod = get_clothing_mod_val_for_damage_type( dmg_type );

    if( !bp_null ) {
        // If we have armour portion materials for this body part, use that instead
        if( !armor_mats.empty() ) {
            for( const part_material *m : armor_mats ) {
                float tmp_add = 0.f;
                if( derived.has_value() && !m->id->has_dedicated_resist( dmg_type ) ) {
                    tmp_add = m->id->resist( derived->first ) * m->cover * 0.01f * derived->second;
                } else {
                    tmp_add = m->id->resist( dmg_type ) * m->cover * 0.01f;
                }
                resist += tmp_add;
            }
            const int env = get_env_resist( base_env_resist );
            if( env < 10 ) {
                resist *= env / 10.0f;
            }
        }
        return resist + mod;
    }

    const std::map<material_id, int> mats = made_of();
    if( !mats.empty() ) {
        const int total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;
        // Not sure why cut and bash get an armor thickness bonus but acid/fire doesn't,
        // but such is the way of the code.
        for( const auto &m : mats ) {
            float tmp_add = 0.f;
            if( derived.has_value() && !m.first->has_dedicated_resist( dmg_type ) ) {
                tmp_add = m.first->resist( derived->first ) * m.second * derived->second;
            } else {
                tmp_add = m.first->resist( dmg_type ) * m.second;
            }
            resist += tmp_add;
        }
        // Average based portion of materials
        resist /= total;
    }

    const int env = get_env_resist( base_env_resist );
    if( env < 10 ) {
        // Low env protection means it doesn't prevent acid seeping in.
        resist *= env / 10.0f;
    }

    return resist + mod;
}

#if defined(_MSC_VER)
#pragma optimize( "", on )
#endif

int item::max_damage() const
{
    return type->damage_max();
}

float item::get_relative_health() const
{
    const int max_dmg = max_damage();
    if( max_dmg == 0 ) { // count_by_charges items
        return 1.0f;
    }
    return 1.0f - static_cast<float>( damage() ) / max_dmg;
}

static int get_degrade_amount( const item &it, int dmgNow_, int dmgPrev )
{
    const int max_dmg = it.max_damage();
    const int degrade_increments = it.type->degrade_increments();
    if( max_dmg == 0 || degrade_increments == 0 ) {
        return 0; // count by charges
    }
    const int facNow_ = dmgNow_ == 0 ? -1 : ( dmgNow_ - 1 ) * 5 / max_dmg;
    const int facPrev = dmgPrev == 0 ? -1 : ( dmgPrev - 1 ) * 5 / max_dmg;

    return std::max( facNow_ - facPrev, 0 ) * max_dmg / degrade_increments;
}

bool item::mod_damage( int qty )
{
    if( has_flag( flag_UNBREAKABLE ) ) {
        return false;
    }
    if( count_by_charges() ) {
        charges -= std::min( type->stack_size * qty / itype::damage_scale, charges );
        return charges == 0; // return destroy = true if no charges
    } else {
        const int dmg_before = damage_;
        const bool destroy = ( damage_ + qty ) > max_damage();
        set_damage( damage_ + qty );

        if( qty > 0 && !destroy ) { // apply automatic degradation
            set_degradation( degradation_ + get_degrade_amount( *this, damage_, dmg_before ) );
        }

        // TODO: think about better way to telling the game what faults should be applied when
        if( qty > 0 ) {
            set_random_fault_of_type( "mechanical_damage" );
        }

        return destroy;
    }
}

bool item::inc_damage()
{
    return mod_damage( itype::damage_scale );
}

int item::repairable_levels() const
{
    const int levels = type->damage_level( damage_ ) - type->damage_level( degradation_ );

    return levels > 0
           ? levels                    // full integer levels of damage
           : damage() > degradation(); // partial level of damage can still be repaired
}

item::armor_status item::damage_armor_durability( damage_unit &du, damage_unit &premitigated,
        const bodypart_id &bp,
        double enchant_multiplier )
{
    //Energy shields aren't damaged by attacks but do get their health variable reduced.  They are also only
    //damaged by the damage types they actually protect against.
    if( has_var( "ENERGY_SHIELD_HP" ) && resist( du.type, false, bp ) > 0.0f ) {
        double shield_hp = get_var( "ENERGY_SHIELD_HP", 0.0 );
        shield_hp -= premitigated.amount;
        set_var( "ENERGY_SHIELD_HP", shield_hp );
        if( shield_hp > 0 ) {
            return armor_status::UNDAMAGED;
        } else {
            //Shields deliberately ignore the enchantment multiplier, as the health mechanic wouldn't make sense otherwise.
            mod_damage( itype::damage_scale * 6 );
            return armor_status::DESTROYED;
        }
    }

    if( has_flag( flag_UNBREAKABLE ) || enchant_multiplier <= 0.0f ) {
        return armor_status::UNDAMAGED;
    }

    // We want armor's own resistance to this type, not the resistance it grants
    double armors_own_resist = resist( du.type, true, bp );
    if( armors_own_resist > 1000.0 ) {
        // This is some weird type that doesn't damage armors
        return armor_status::UNDAMAGED;
    }
    /*
    * Armor damage chance is calculated using the logistics function.
    *  No matter the damage dealt, an armor piece has at at most 80% chance of being damaged.
    *  Chance of damage is 40% when the premitigation damage is equal to armor*1.333
    *  Sturdy items will not take damage if premitigation damage isn't higher than armor*1.333.
    *
    *  Non fragile items are considered to always have at least 10 points of armor, this is to prevent
    *  regular clothes from exploding into ribbons whenever you get punched.
    */
    armors_own_resist = has_flag( flag_FRAGILE ) ? armors_own_resist * 0.6666 : std::max( 10.0,
                        armors_own_resist * 1.3333 );
    double damage_chance = 0.8 / ( 1.0 + std::exp( -( premitigated.amount - armors_own_resist ) ) ) *
                           enchant_multiplier;

    // Scale chance of article taking damage based on the number of parts it covers.
    // This represents large articles being able to take more punishment
    // before becoming ineffective or being destroyed
    damage_chance = damage_chance / get_covered_body_parts().count();
    if( has_flag( flag_STURDY ) && premitigated.amount < armors_own_resist ) {
        return armor_status::UNDAMAGED;
    } else if( x_in_y( damage_chance, 1.0 ) ) {
        return mod_damage( itype::damage_scale * enchant_multiplier ) ? armor_status::DESTROYED :
               armor_status::DAMAGED;
    }
    return armor_status::UNDAMAGED;
}


item::armor_status item::damage_armor_transforms( damage_unit &du, double enchant_multiplier ) const
{
    if( enchant_multiplier <= 0.0f ) {
        return armor_status::UNDAMAGED;
    }

    // We want armor's own resistance to this type, not the resistance it grants
    const float armors_own_resist = resist( du.type, true );

    // to avoid dumb chip damage plates shouldn't ever transform if they take less than
    // 20% of their protection in damage
    if( du.amount / armors_own_resist < 0.2f ) {
        return armor_status::UNDAMAGED;
    }

    // plates are rated to survive 3 shots at the caliber they protect
    // linearly scale off the scale value to find the chance it breaks
    float break_chance = 33.3f * ( du.amount / armors_own_resist ) * enchant_multiplier;

    float roll_to_break = rng_float( 0.0, 100.0 );

    if( roll_to_break < break_chance ) {
        //the plate is broken
        return armor_status::TRANSFORMED;
    }
    return armor_status::UNDAMAGED;
}

std::string item::damage_indicator() const
{
    return cata::options::damage_indicators[damage_level()];
}

std::string item::durability_indicator( bool include_intact ) const
{
    const std::string item_health_option = get_option<std::string>( "ITEM_HEALTH" );
    const bool show_bars_only = item_health_option == "bars";
    const bool show_description_only = item_health_option == "descriptions";
    const bool show_both = item_health_option == "both";
    const bool show_bars = show_both || show_bars_only;
    const bool show_description = show_both || show_description_only;
    std::string bars;
    std::string description;
    if( show_bars ) {
        bars = damage_indicator() + degradation_symbol() + "\u00A0";
    }
    if( damage() < 0 ) {
        if( show_description ) {
            if( is_gun() ) {
                description = pgettext( "damage adjective", "accurized " );
            } else {
                description = pgettext( "damage adjective", "reinforced " );
            }
        }
        if( show_bars_only ) {
            return bars;
        }
        if( show_description_only ) {
            return description;
        }
        return bars + description;
    }
    if( has_flag( flag_CORPSE ) ) {
        if( damage() > 0 ) {
            switch( damage_level() ) {
                case 1:
                    return pgettext( "damage adjective", "bruised " );
                case 2:
                    return pgettext( "damage adjective", "damaged " );
                case 3:
                    return pgettext( "damage adjective", "mangled " );
                default:
                    return pgettext( "damage adjective", "pulped " );
            }
        }
    }
    if( show_bars_only ) {
        return bars;
    }
    description = string_format( "%s ", get_base_material().dmg_adj( damage_level() ) );
    if( description == " " ) {
        if( include_intact ) {
            description = _( "fully intact " );
        } else {
            description = "";
        }
    }
    if( show_description_only ) {
        return description;
    }
    return bars + description;
}

const std::set<itype_id> &item::repaired_with() const
{
    static std::set<itype_id> no_repair;
    static std::set<itype_id> tools;
    tools.clear();

    std::copy_if( type->repair.begin(), type->repair.end(), std::inserter( tools, tools.begin() ),
    []( const itype_id & i ) {
        return !item_is_blacklisted( i );
    } );

    return has_flag( flag_NO_REPAIR ) ? no_repair : tools;
}

bool item::is_two_handed( const Character &guy ) const
{
    if( has_flag( flag_ALWAYS_TWOHAND ) ) {
        return true;
    }
    ///\EFFECT_STR determines which weapons can be wielded with one hand
    return ( ( weight() / 113_gram ) > guy.get_arm_str() * 4 );
}

const std::map<material_id, int> &item::made_of() const
{
    if( is_corpse() ) {
        return corpse->mat;
    }
    return type->materials;
}

std::vector<const material_type *> item::made_of_types() const
{
    std::vector<const material_type *> material_types_composed_of;
    if( is_corpse() ) {
        for( const auto &mat_id : made_of() ) {
            material_types_composed_of.push_back( &mat_id.first.obj() );
        }
    } else {
        for( const auto &mat_id : type->materials ) {
            material_types_composed_of.push_back( &mat_id.first.obj() );
        }
    }
    return material_types_composed_of;
}

bool item::made_of_any( const std::set<material_id> &mat_idents ) const
{
    const std::map<material_id, int> &mats = made_of();
    if( mats.empty() ) {
        return false;
    }

    return std::any_of( mats.begin(),
    mats.end(), [&mat_idents]( const std::pair<material_id, int> &e ) {
        return mat_idents.count( e.first );
    } );
}

bool item::only_made_of( const std::set<material_id> &mat_idents ) const
{
    const std::map<material_id, int> &mats = made_of();
    if( mats.empty() ) {
        return false;
    }

    return std::all_of( mats.begin(),
    mats.end(), [&mat_idents]( const std::pair<material_id, int> &e ) {
        return mat_idents.count( e.first );
    } );
}

int item::made_of( const material_id &mat_ident ) const
{
    const std::map<material_id, int> &materials = made_of();
    auto mat = materials.find( mat_ident );
    if( mat == materials.end() ) {
        return 0;
    }
    return mat->second;
}

bool item::can_repair_with( const material_id &mat_ident ) const
{
    auto mat = type->repairs_with.find( mat_ident );
    return mat != type->repairs_with.end();
}

bool item::made_of( phase_id phase ) const
{
    if( is_null() ) {
        return false;
    }
    return current_phase == phase;
}

bool item::made_of_from_type( phase_id phase ) const
{
    if( is_null() ) {
        return false;
    }
    return type->phase == phase;
}

bool item::conductive() const
{
    if( is_null() ) {
        return false;
    }

    if( has_flag( flag_CONDUCTIVE ) ) {
        return true;
    }

    if( has_flag( flag_NONCONDUCTIVE ) ) {
        return false;
    }

    // If any material has electricity resistance equal to or lower than flesh (1) we are conductive.
    const std::vector<const material_type *> &mats = made_of_types();
    return std::any_of( mats.begin(), mats.end(), []( const material_type * mt ) {
        return mt->is_conductive();
    } );
}

bool item::is_gun() const
{
    return !!type->gun;
}

void item::select_itype_variant()
{
    weighted_int_list<std::string> variants;
    for( const itype_variant_data &iv : type->variants ) {
        variants.add( iv.id, iv.weight );
    }

    const std::string *selected = variants.pick();
    if( !selected ) {
        // No variants exist
        return;
    }

    set_itype_variant( *selected );
}

bool item::can_have_itype_variant() const
{
    return !type->variants.empty();
}

bool item::possible_itype_variant( const std::string &test ) const
{
    if( !can_have_itype_variant() ) {
        return false;
    }

    const auto variant_looking_for = [&test]( const itype_variant_data & variant ) {
        return variant.id == test;
    };

    return std::find_if( type->variants.begin(), type->variants.end(),
                         variant_looking_for ) != type->variants.end();
}

bool item::has_itype_variant( bool check_option ) const
{
    if( _itype_variant == nullptr ) {
        return false;
    } else if( !check_option ) {
        return true;
    }

    switch( type->variant_kind ) {
        case itype_variant_kind::gun:
            return get_option<bool>( "SHOW_GUN_VARIANTS" );
        case itype_variant_kind::drug:
            return get_option<bool>( "SHOW_DRUG_VARIANTS" );
        default:
            return true;
    }
    return false;
}

const itype_variant_data &item::itype_variant() const
{
    return *_itype_variant;
}

void item::set_itype_variant( const std::string &variant )
{
    if( variant.empty() || type->variants.empty() ) {
        return;
    }

    if( variant == "<any>" ) {
        select_itype_variant();
        return;
    }

    for( const itype_variant_data &option : type->variants ) {
        if( option.id == variant ) {
            _itype_variant = &option;
            if( option.expand_snippets ) {
                set_var( "description", SNIPPET.expand( variant_description() ) );
            }
            return;
        }
    }

    debugmsg( "item '%s' has no variant '%s'!", typeId().str(), variant );
}

std::string item::variant_description() const
{
    if( !has_itype_variant() ) {
        return "";
    }

    // append the description instead of fully overwriting it
    if( itype_variant().append ) {
        return string_format( pgettext( "variant description", "%s  %s" ), type->description.translated(),
                              itype_variant().alt_description.translated() );
    } else {
        return itype_variant().alt_description.translated();
    }
}

void item::clear_itype_variant()
{
    _itype_variant = nullptr;
}

bool item::is_firearm() const
{
    return is_gun() && !has_flag( flag_PRIMITIVE_RANGED_WEAPON );
}

int item::get_reload_time() const
{
    if( !is_gun() && !is_magazine() ) {
        return 0;
    }
    int reload_time = 0;
    if( is_gun() ) {
        reload_time = type->gun->reload_time;
    } else if( type->magazine ) {
        reload_time = type->magazine->reload_time;
    } else {
        reload_time = INVENTORY_HANDLING_PENALTY;
    }
    for( const item *mod : gunmods() ) {
        reload_time = static_cast<int>( reload_time * ( 100 + mod->type->gunmod->reload_modifier ) / 100 );
    }

    return reload_time;
}

bool item::is_silent() const
{
    return gun_noise().volume < 5;
}

bool item::is_gunmod() const
{
    return !!type->gunmod;
}

bool item::is_bionic() const
{
    return !!type->bionic;
}

bool item::is_magazine() const
{
    return !!type->magazine || contents.has_pocket_type( pocket_type::MAGAZINE );
}

bool item::is_battery() const
{
    return is_magazine() && ammo_capacity( ammo_battery );
}

bool item::is_vehicle_battery() const
{
    return !!type->battery;
}

bool item::is_ammo_belt() const
{
    return is_magazine() && has_flag( flag_MAG_BELT );
}

bool item::is_ammo() const
{
    return !!type->ammo;
}

bool item::is_comestible() const
{
    return !!get_comestible();
}

bool item::is_food() const
{
    if( !is_comestible() ) {
        return false;
    }
    const std::string &comest_type = get_comestible()->comesttype;
    return comest_type == "FOOD" || comest_type == "DRINK";
}

bool item::is_medication() const
{
    if( !is_comestible() ) {
        return false;
    }
    return get_comestible()->comesttype == "MED";
}

bool item::is_medical_tool() const
{
    return type->get_use( "heal" ) != nullptr;
}

bool item::is_brewable() const
{
    return !!type->brewable;
}

bool item::is_compostable() const
{
    return !!type->compostable;
}

bool item::is_food_container() const
{
    return ( !is_food() && has_item_with( []( const item & food ) {
        return food.is_food();
    } ) ) ||
    ( is_craft() && !craft_data_->disassembly &&
      craft_data_->making->create_results().front().is_food_container() );
}

bool item::has_temperature() const
{
    return ( is_comestible() && !has_flag( flag_NO_TEMP ) )  || is_corpse();
}

bool item::is_corpse() const
{
    return corpse != nullptr && has_flag( flag_CORPSE );
}

const mtype *item::get_mtype() const
{
    return corpse;
}

float item::get_specific_heat_liquid() const
{
    if( is_comestible() ) {
        return get_comestible()->specific_heat_liquid;
    }
    return made_of_types()[0]->specific_heat_liquid();
}

float item::get_specific_heat_solid() const
{
    if( is_comestible() ) {
        return get_comestible()->specific_heat_solid;
    }
    return made_of_types()[0]->specific_heat_solid();
}

float item::get_latent_heat() const
{
    if( is_comestible() ) {
        return get_comestible()->latent_heat;
    }
    return made_of_types()[0]->latent_heat();
}

units::temperature item::get_freeze_point() const
{
    if( is_comestible() ) {
        return units::from_celsius( get_comestible()->freeze_point );
    }
    return units::from_celsius( made_of_types()[0]->freeze_point() );
}

void item::set_mtype( const mtype *const m )
{
    // This is potentially dangerous, e.g. for corpse items, which *must* have a valid mtype pointer.
    if( m == nullptr ) {
        debugmsg( "setting item::corpse of %s to NULL", tname() );
        return;
    }
    corpse = m;
}

bool item::is_ammo_container() const
{
    return contents.has_any_with(
    []( const item & it ) {
        return it.is_ammo();
    }, pocket_type::CONTAINER );
}

bool item::is_melee() const
{
    for( const damage_type &dt : damage_type::get_all() ) {
        if( is_melee( dt.id ) ) {
            return true;
        }
    }
    return false;
}

bool item::is_melee( const damage_type_id &dt ) const
{
    return damage_melee( dt ) > MELEE_STAT;
}

bool item::is_book() const
{
    return !!type->book;
}

std::string item::get_book_skill() const
{
    if( is_book() ) {
        if( type->book->skill->ident() != skill_id::NULL_ID() ) {
            return type->book->skill->name();
        }
    }
    return "";
}

bool item::is_map() const
{
    return get_category_shallow().get_id() == item_category_maps ||
           type->use_methods.count( "reveal_map" );
}

bool item::is_engine() const
{
    return !!type->engine;
}

bool item::is_wheel() const
{
    return !!type->wheel;
}

bool item::is_fuel() const
{
    // A fuel is made of only one material
    if( type->materials.size() != 1 ) {
        return false;
    }
    // and this material has to produce energy
    if( get_base_material().get_fuel_data().energy <= 0_J ) {
        return false;
    }
    // and it needs to be have consumable charges
    return count_by_charges();
}

bool item::is_toolmod() const
{
    return !is_gunmod() && type->mod;
}

bool item::is_irremovable() const
{
    return has_flag( flag_IRREMOVABLE );
}

bool item::is_identifiable() const
{
    return is_book();
}

bool item::is_broken() const
{
    return has_flag( flag_ITEM_BROKEN ) || has_fault_flag( std::string( "ITEM_BROKEN" ) );
}

bool item::is_broken_on_active() const
{
    return has_flag( flag_ITEM_BROKEN ) ||
           has_fault_flag( std::string( "ITEM_BROKEN" ) ) ||
           ( wetness && has_flag( flag_WATER_BREAK_ACTIVE ) );
}

std::set<fault_id> item::faults_potential() const
{
    std::set<fault_id> res;
    for( const std::pair<fault_id, int> &fault_pair : type->faults ) {
        res.insert( fault_pair.first );
    }
    return res;
}

std::set<fault_id> item::faults_potential_of_type( const std::string &fault_type ) const
{
    std::set<fault_id> res;
    for( const std::pair<fault_id, int> &some_fault : type->faults ) {
        if( some_fault.first->type() == fault_type ) {
            res.emplace( some_fault.first );
        }
    }
    return res;
}

int item::wheel_area() const
{
    return is_wheel() ? type->wheel->diameter * type->wheel->width : 0;
}

units::energy item::fuel_energy() const
{
    // The odd units and division are to avoid integer rounding errors.
    return get_base_material().get_fuel_data().energy * units::to_milliliter( base_volume() ) / 1000;
}

std::string item::fuel_pump_terrain() const
{
    return get_base_material().get_fuel_data().pump_terrain;
}

bool item::has_explosion_data() const
{
    return !get_base_material().get_fuel_data().explosion_data.is_empty();
}

struct fuel_explosion_data item::get_explosion_data() const {
    return get_base_material().get_fuel_data().explosion_data;
}

bool item::is_magazine_full() const
{
    return contents.is_magazine_full();
}

bool item::allows_speedloader( const itype_id &speedloader_id ) const
{
    return contents.allows_speedloader( speedloader_id );
}

bool item::can_reload_with( const item &ammo, bool now ) const
{
    if( has_flag( flag_NO_RELOAD ) && !has_flag( flag_VEHICLE ) ) {
        return false; // turrets ignore NO_RELOAD flag
    }

    if( now && ammo.is_magazine() && !ammo.empty() ) {
        if( is_tool() ) {
            // Dirty hack because "ammo" on tools is actually completely separate thing from "ammo" on guns and "ammo_types()" works only for guns
            if( !type->tool->ammo_id.count( ammo.contents.first_ammo().ammo_type() ) ) {
                return false;
            }
        } else {
            if( !ammo_types().count( ammo.contents.first_ammo().ammo_type() ) ) {
                return false;
            }
        }
    }

    // Check if the item is in general compatible with any reloadable pocket.
    return contents.can_reload_with( ammo, now );
}

bool item::is_salvageable() const
{
    if( is_null() ) {
        return false;
    }
    // None of the materials are salvageable or they turn into the original item
    const std::map<material_id, int> &mats = made_of();
    if( std::none_of( mats.begin(), mats.end(), [this]( const std::pair<material_id, int> &m ) {
    return m.first->salvaged_into().has_value() && m.first->salvaged_into().value() != type->get_id();
    } ) ) {
        return false;
    }
    return !has_flag( flag_NO_SALVAGE );
}

bool item::is_disassemblable() const
{
    return !ethereal && ( recipe_dictionary::get_uncraft( typeId() ) || typeId() == itype_disassembly );
}

bool item::is_craft() const
{
    return craft_data_ != nullptr;
}

bool item::is_emissive() const
{
    if( light.luminance > 0 || type->light_emission > 0 ) {
        return true;
    }

    for( const item_pocket *pkt : get_all_contained_and_mod_pockets() ) {
        if( pkt->transparent() ) {
            for( const item *it : pkt->all_items_top() ) {
                if( it->is_emissive() ) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool item::is_deployable() const
{
    return type->can_use( "deploy_furn" ) || type->can_use( "deploy_appliance" );
}

bool item::is_tool() const
{
    return !!type->tool;
}

bool item::is_transformable() const
{
    return type->use_methods.find( "transform" ) != type->use_methods.end();
}

bool item::is_relic() const
{
    return !!relic_data;
}

bool item::is_same_relic( item const &rhs ) const
{
    return ( !is_relic() && !rhs.is_relic() ) ||
           ( is_relic() && rhs.is_relic() && *relic_data == *rhs.relic_data );
}

bool item::is_same_temperature( item const &rhs ) const
{
    return ( !has_temperature() && !rhs.has_temperature() ) ||
           ( has_temperature() && rhs.has_temperature() &&
             has_flag( flag_HOT ) == rhs.has_flag( flag_HOT ) &&
             has_flag( flag_COLD ) == rhs.has_flag( flag_COLD ) &&
             has_flag( flag_FROZEN ) == rhs.has_flag( flag_FROZEN ) &&
             has_flag( flag_MELTS ) == rhs.has_flag( flag_MELTS ) );
}

bool item::has_relic_recharge() const
{
    return is_relic() && relic_data->has_recharge();
}

bool item::has_relic_activation() const
{
    return is_relic() && relic_data->has_activation();
}

std::vector<enchant_cache> item::get_proc_enchantments() const
{
    if( !is_relic() ) {
        return std::vector<enchant_cache> {};
    }
    return relic_data->get_proc_enchantments();
}

std::vector<enchantment> item::get_defined_enchantments() const
{
    if( !is_relic() || !type->relic_data ) {
        return std::vector<enchantment> {};
    }

    return type->relic_data->get_defined_enchantments();
}

double item::calculate_by_enchantment_wield( const Character &owner, double modify,
        enchant_vals::mod value,
        bool round_value ) const
{
    double add_value = 0.0;
    double mult_value = 1.0;
    for( const enchant_cache &ench : get_proc_enchantments() ) {
        if( ench.active_wield() ) {
            add_value += ench.get_value_add( value );
            mult_value += ench.get_value_multiply( value );
        }
    }
    if( type->relic_data ) {
        for( const enchantment &ench : type->relic_data->get_defined_enchantments() ) {
            if( ench.active_wield() ) {
                add_value += ench.get_value_add( value, owner );
                mult_value += ench.get_value_multiply( value, owner );
            }
        }
    }
    modify += add_value;
    modify *= mult_value;
    if( round_value ) {
        modify = std::round( modify );
    }
    return modify;
}

book_proficiency_bonuses item::get_book_proficiency_bonuses() const
{
    book_proficiency_bonuses ret;

    if( is_estorage() ) {
        for( const item *book : ebooks() ) {
            ret += book->get_book_proficiency_bonuses();
        }
    }

    if( !type->book ) {
        return ret;
    }
    for( const book_proficiency_bonus &bonus : type->book->proficiencies ) {
        ret.add( bonus );
    }
    return ret;
}

int item::pages_in_book( const itype &type )
{
    if( !type.book ) {
        return 0;
    }
    // an A4 sheet weights roughly 5 grams
    return std::max( 1, static_cast<int>( units::to_gram( type.weight ) / 5 ) );
}

int item::get_chapters() const
{
    if( !type->book ) {
        return 0;
    }
    return type->book->chapters;
}

int item::get_remaining_chapters( const Character &u ) const
{
    const itype_id &type = typeId();
    if( is_book() && type->book->generic ) {
        // NOLINTNEXTLINE(cata-translate-string-literal)
        const std::string var = string_format( "remaining-chapters-%d", u.getID().get_value() );
        return get_var( var, get_chapters() );
    }
    const std::map<itype_id, int> &book_chapters = u.book_chapters;
    auto find_chapters = book_chapters.find( type );
    if( find_chapters == book_chapters.end() ) {
        return get_chapters();
    }
    return find_chapters->second;
}

void item::mark_chapter_as_read( Character &u )
{
    // books without chapters will always have remaining chapters == 0, so we don't need to store them
    if( is_book() && get_chapters() == 0 ) {
        return;
    }
    if( is_book() && type->book->generic ) {
        // NOLINTNEXTLINE(cata-translate-string-literal)
        const std::string var = string_format( "remaining-chapters-%d", u.getID().get_value() );
        const int remain = std::max( 0, get_remaining_chapters( u ) - 1 );
        set_var( var, remain );
        return;
    }
    u.book_chapters[typeId()] = std::max( 0, get_remaining_chapters( u ) - 1 );
}

std::set<recipe_id> item::get_saved_recipes() const
{
    if( is_broken_on_active() ) {
        return {};
    }
    if( is_estorage() && efile_activity_actor::edevice_has_use( this ) ) {
        const item *recipe_catalog = get_recipe_catalog();
        if( recipe_catalog != nullptr ) {
            return recipe_catalog->get_saved_recipes();
        }
    }
    std::set<recipe_id> result;
    for( const std::string &rid_str : string_split( get_var( "EIPC_RECIPES" ), ',' ) ) {
        const recipe_id rid( rid_str );
        if( rid.is_empty() ) {
            continue;
        }
        if( !rid.is_valid() ) {
            DebugLog( D_WARNING, D_GAME ) << type->get_id().str() <<
                                          " contained invalid recipe id '" << rid.str() << "'";
            continue;
        }
        result.emplace( rid );
    }
    return result;
}

void item::set_saved_recipes( const std::set<recipe_id> &recipes )
{
    set_var( "EIPC_RECIPES", string_join( recipes, "," ) );
}

void item::generate_recipes()
{
    const memory_card_info *mmd = type->memory_card_data ? &*type->memory_card_data : nullptr;
    std::set<recipe_id> saved_recipes = get_saved_recipes();
    if( mmd != nullptr ) {
        std::set<recipe_id> candidates;
        for( const auto& [rid, r] : recipe_dict ) {
            if( r.never_learn || r.obsolete || mmd->recipes_categories.count( r.category ) == 0 ||
                saved_recipes.count( rid ) ||
                r.difficulty > mmd->recipes_level_max || r.difficulty < mmd->recipes_level_min ||
                ( r.has_flag( "SECRET" ) && !mmd->secret_recipes ) ) {
                continue;
            }
            candidates.emplace( rid );
        }
        const int recipe_num = rng( 1, mmd->recipes_amount );
        for( int i = 0; i < recipe_num && !candidates.empty(); i++ ) {
            const recipe_id rid = random_entry_removed( candidates );
            saved_recipes.emplace( rid );
        }
        set_saved_recipes( saved_recipes );
    }
}
std::vector<std::pair<const recipe *, int>> item::get_available_recipes(
            const Character &u ) const
{
    std::vector<std::pair<const recipe *, int>> recipe_entries;
    if( is_book() ) {
        if( !u.has_identified( typeId() ) ) {
            return {};
        }

        for( const islot_book::recipe_with_description_t &elem : type->book->recipes ) {
            if( u.get_knowledge_level( elem.recipe->skill_used ) >= elem.skill_level ) {
                recipe_entries.emplace_back( elem.recipe, elem.skill_level );
            }
        }
    } else {
        for( const recipe_id &rid : get_saved_recipes() ) {
            const recipe *r = &rid.obj();
            if( u.get_knowledge_level( r->skill_used ) >= r->difficulty ) {
                recipe_entries.emplace_back( r, r->difficulty );
            }
        }
    }
    return recipe_entries;
}

const material_type &item::get_random_material() const
{
    std::vector<material_id> matlist;
    const std::map<material_id, int> &mats = made_of();
    matlist.reserve( mats.size() );
    for( const std::pair<const material_id, int> &mat : mats ) {
        matlist.emplace_back( mat.first );
    }
    return *random_entry( matlist, material_id::NULL_ID() );
}

const material_type &item::get_base_material() const
{
    const std::map<material_id, int> &mats = made_of();
    const material_type *m = &material_id::NULL_ID().obj();
    int portion = 0;
    for( const std::pair<const material_id, int> &mat : mats ) {
        if( mat.second > portion ) {
            portion = mat.second;
            m = &mat.first.obj();
        }
    }
    // Material portions all equal / not specified. Select first material.
    if( portion == 1 ) {
        if( is_corpse() ) {
            return corpse->mat.begin()->first.obj();
        }
        return *type->default_mat;
    }
    return *m;
}

bool item::operator<( const item &other ) const
{
    const item_category &cat_a = get_category_of_contents();
    const item_category &cat_b = other.get_category_of_contents();
    if( cat_a != cat_b ) {
        return cat_a < cat_b;
    } else {

        if( this->typeId() == other.typeId() ) {
            if( this->is_money() ) {
                return this->charges > other.charges;
            }
            return this->charges < other.charges;
        } else {
            std::string n1 = this->type->nname( 1 );
            std::string n2 = other.type->nname( 1 );
            return localized_compare( n1, n2 );
        }
    }
}

skill_id item::gun_skill() const
{
    if( !is_gun() ) {
        return skill_id::NULL_ID();
    }
    return type->gun->skill_used;
}

gun_type_type item::gun_type() const
{
    static skill_id skill_archery( "archery" );

    if( !is_gun() ) {
        return gun_type_type( std::string() );
    }
    // TODO: move to JSON and remove extraction of this from "GUN" (via skill id)
    //and from "GUNMOD" (via "mod_targets") in lang/extract_json_strings.py
    if( gun_skill() == skill_archery ) {
        if( ammo_types().count( ammo_bolt ) || typeId() == itype_bullet_crossbow ) {
            return gun_type_type( translate_marker_context( "gun_type_type", "crossbow" ) );
        } else {
            return gun_type_type( translate_marker_context( "gun_type_type", "bow" ) );
        }
    }
    return gun_type_type( gun_skill().str() );
}

skill_id item::melee_skill() const
{
    if( !is_melee() ) {
        return skill_id::NULL_ID();
    }

    int hi = 0;
    skill_id res = skill_id::NULL_ID();

    for( const damage_type &dt : damage_type::get_all() ) {
        const int val = damage_melee( dt.id );
        if( val > hi && !dt.skill.is_null() ) {
            hi = val;
            res = dt.skill;
        }
    }

    return res;
}

int item::min_cycle_recoil() const
{
    if( !is_gun() ) {
        return 0;
    }
    int to_cycle = type->gun->min_cycle_recoil;
    // This should only be used for one mod or it'll mess things up
    // TODO: maybe generalize this so you can have mods for hot loads or whatever
    for( const item *mod : gunmods() ) {
        // this value defaults to -1
        if( mod->type->gunmod->overwrite_min_cycle_recoil > 0 ) {
            to_cycle = mod->type->gunmod->overwrite_min_cycle_recoil;
        }
    }
    return to_cycle;
}

int item::gun_dispersion( bool with_ammo, bool with_scaling ) const
{
    if( !is_gun() ) {
        return 0;
    }
    int dispersion_sum = type->gun->dispersion;
    for( const item *mod : gunmods() ) {
        dispersion_sum += mod->type->gunmod->dispersion;
    }
    int dispPerDamage = get_option< int >( "DISPERSION_PER_GUN_DAMAGE" );
    dispersion_sum += damage_level() * dispPerDamage;
    dispersion_sum = std::max( dispersion_sum, 0 );
    if( with_ammo && has_ammo() ) {
        dispersion_sum += ammo_data()->ammo->dispersion_considering_length( barrel_length() );
    }
    if( !with_scaling ) {
        return dispersion_sum;
    }

    // Dividing dispersion by 15 temporarily as a gross adjustment,
    // will bake that adjustment into individual gun definitions in the future.
    // Absolute minimum gun dispersion is 1.
    double divider = get_option< float >( "GUN_DISPERSION_DIVIDER" );
    dispersion_sum = std::max( static_cast<int>( std::round( dispersion_sum / divider ) ), 1 );

    return dispersion_sum;
}

std::pair<int, int> item::sight_dispersion( const Character &character ) const
{
    if( !is_gun() ) {
        return std::make_pair( 0, 0 );
    }
    int act_disp = has_flag( flag_DISABLE_SIGHTS ) ? 300 : type->gun->sight_dispersion;
    int eff_disp = character.effective_dispersion( act_disp );

    for( const item *e : gunmods() ) {
        const islot_gunmod &mod = *e->type->gunmod;
        int e_act_disp = mod.sight_dispersion;
        if( mod.sight_dispersion < 0 || mod.field_of_view <= 0 ) {
            continue;
        }
        int e_eff_disp = character.effective_dispersion( e_act_disp, e->has_flag( flag_ZOOM ) );
        if( eff_disp > e_eff_disp ) {
            eff_disp = e_eff_disp;
            act_disp = e_act_disp;

        }
    }
    return std::make_pair( act_disp, eff_disp );
}

damage_instance item::gun_damage( bool with_ammo, bool shot ) const
{
    if( !is_gun() ) {
        return damage_instance();
    }

    units::length bl = barrel_length();
    damage_instance ret = type->gun->damage.di_considering_length( bl );

    for( const item *mod : gunmods() ) {
        ret.add( mod->type->gunmod->damage.di_considering_length( bl ) );
    }

    if( with_ammo && has_ammo() ) {
        if( shot ) {
            ret.add( ammo_data()->ammo->shot_damage.di_considering_length( bl ) );
        } else {
            ret.add( ammo_data()->ammo->damage.di_considering_length( bl ) );
        }
    }

    if( damage() > 0 ) {
        // TODO: This isn't a good solution for multi-damage guns/ammos
        for( damage_unit &du : ret ) {
            if( du.amount <= 1.0 ) {
                continue;
            }
            du.amount = std::max<float>( 1.0f, damage_adjusted_gun_damage( du.amount ) );
        }
    }

    return ret;
}

damage_instance item::gun_damage( itype_id ammo ) const
{
    if( !is_gun() ) {
        return damage_instance();
    }

    units::length bl = barrel_length();
    damage_instance ret = type->gun->damage.di_considering_length( bl );

    for( const item *mod : gunmods() ) {
        ret.add( mod->type->gunmod->damage.di_considering_length( bl ) );
    }

    ret.add( ammo->ammo->damage.di_considering_length( bl ) );

    if( damage() > 0 ) {
        // TODO: This isn't a good solution for multi-damage guns/ammos
        for( damage_unit &du : ret ) {
            if( du.amount <= 1.0 ) {
                continue;
            }
            du.amount = std::max<float>( 1.0f, damage_adjusted_gun_damage( du.amount ) );
        }
    }

    return ret;
}
units::mass item::gun_base_weight() const
{
    units::mass base_weight = type->weight;
    for( const item *mod : gunmods() ) {
        if( !mod->type->mod->ammo_modifier.empty() ) {
            base_weight += mod->type->integral_weight;
        }
    }
    return base_weight;

}
int item::gun_recoil( const Character &p, bool bipod, bool ideal_strength ) const
{
    if( !is_gun() || ( ammo_required() && !ammo_remaining( ) ) ) {
        return 0;
    }

    ///\ARM_STR improves the handling of heavier weapons
    // we consider only base weight to avoid exploits
    // now we need to add weight of receiver
    double wt = ideal_strength ? gun_base_weight() / 333.0_gram : std::min( gun_base_weight(),
                p.get_arm_str() * 333_gram ) / 333.0_gram;

    double handling = type->gun->handling;
    for( const item *mod : gunmods() ) {
        if( bipod || !mod->has_flag( flag_BIPOD ) ) {
            handling += mod->type->gunmod->handling;
        }
    }

    // rescale from JSON units which are intentionally specified as integral values
    handling /= 10;

    // algorithm is biased so heavier weapons benefit more from improved handling
    handling = std::pow( wt, 0.8 ) * std::pow( handling, 1.2 );

    int qty = type->gun->recoil;
    if( has_ammo() ) {
        qty += ammo_data()->ammo->recoil;
    }

    // handling could be either a bonus or penalty dependent upon installed mods
    if( handling > 1.0 ) {
        return qty / handling;
    } else {
        return qty * ( 1.0 + std::abs( handling ) );
    }
}

float item::gun_shot_spread_multiplier() const
{
    if( !is_gun() ) {
        return 0;
    }
    float ret = 1.0f;
    for( const item *mod : gunmods() ) {
        ret += mod->type->gunmod->shot_spread_multiplier_modifier;
    }
    return std::max( ret, 0.0f );
}

int item::gun_range( bool with_ammo ) const
{
    if( !is_gun() ) {
        return 0;
    }
    int ret = type->gun->range;
    float range_multiplier = 1.0;
    for( const item *mod : gunmods() ) {
        ret += mod->type->gunmod->range;
        range_multiplier *= mod->type->gunmod->range_multiplier;
    }
    if( with_ammo && has_ammo() ) {
        const itype *ammo_info = ammo_data();
        ret += ammo_info->ammo->range;
        range_multiplier *= ammo_info->ammo->range_multiplier;
    }
    ret *= range_multiplier;
    return std::min( std::max( 0, ret ), RANGE_HARD_CAP );
}

int item::gun_range( const Character *p ) const
{
    int ret = gun_range( true );
    if( p == nullptr ) {
        return ret;
    }
    if( !p->meets_requirements( *this ) ) {
        return 0;
    }

    // Reduce bow range until player has twice minimum required strength
    if( has_flag( flag_STR_DRAW ) ) {
        ret += std::max( 0.0, ( p->get_str() - get_min_str() ) * 0.5 );
    }

    ret = p->enchantment_cache->modify_value( enchant_vals::mod::RANGE, ret );

    return std::max( 0, ret );
}

int item::shots_remaining( const map &here, const Character *carrier ) const
{
    int ret = 1000; // Arbitrary large number for things that do not require ammo.
    if( ammo_required() ) {
        ret = std::min( ammo_remaining_linked( here,  carrier ) / ammo_required(), ret );
    }
    if( get_gun_energy_drain() > 0_kJ ) {
        ret = std::min( static_cast<int>( energy_remaining( carrier ) / get_gun_energy_drain() ), ret );
    }
    return ret;
}

int item::ammo_remaining( const map &here, const std::set<ammotype> &ammo, const Character *carrier,
                          const bool include_linked ) const
{
    const bool is_tool_with_carrier = carrier != nullptr && is_tool();

    if( is_tool_with_carrier && has_flag( flag_USES_NEARBY_AMMO ) && !ammo.empty() ) {
        const inventory &crafting_inventory = carrier->crafting_inventory();
        const ammotype &a = *ammo.begin();
        return crafting_inventory.charges_of( a->default_ammotype(), INT_MAX );
    }

    int ret = 0;

    // Magazine in the item
    const item *mag = magazine_current();
    if( mag ) {
        ret += mag->ammo_remaining( );
    }

    // Cable connections
    if( include_linked && link_length() >= 0 && link().efficiency >= MIN_LINK_EFFICIENCY ) {
        if( link().t_veh ) {
            ret += link().t_veh->connected_battery_power_level( here ).first;
        } else {
            const optional_vpart_position vp = here.veh_at( link().t_abs_pos );
            if( vp ) {
                ret += vp->vehicle().connected_battery_power_level( here ).first;
            }
        }
    }

    // Non ammo using item that uses charges
    if( ammo.empty() ) {
        ret += charges;
    }

    // Magazines and integral magazines on their own
    if( is_magazine() ) {
        for( const item *e : contents.all_items_top( pocket_type::MAGAZINE ) ) {
            if( e->is_ammo() ) {
                ret += e->charges;
            }
        }
    }

    // Handle non-magazines with ammo_restriction in a CONTAINER type pocket (like quivers)
    if( !( mag || is_magazine() || ammo.empty() ) ) {
        for( const item *e : contents.all_items_top( pocket_type::CONTAINER ) ) {
            if( e->is_ammo() && ammo.find( e->ammo_type() ) != ammo.end() ) {
                ret += e->charges;
            }
        }
    }

    // UPS/bionic power can replace ammo requirement.
    // Only for tools. Guns should always use energy_drain for electricity use.
    if( is_tool_with_carrier ) {
        if( has_flag( flag_USES_BIONIC_POWER ) ) {
            ret += units::to_kilojoule( carrier->get_power_level() );
        }
        if( has_flag( flag_USE_UPS ) ) {
            ret += units::to_kilojoule( carrier->available_ups() );
        }
    }

    return ret;
}

int item::ammo_remaining_linked( const map &here, const Character *carrier ) const
{
    std::set<ammotype> ammo = ammo_types();
    return ammo_remaining( here, ammo, carrier, true );
}
int item::ammo_remaining( const Character *carrier ) const
{
    std::set<ammotype> ammo = ammo_types();
    return ammo_remaining( get_map(), ammo, carrier, false ); // get_map() is a dummy parameter here.
}

int item::ammo_remaining_linked( const map &here ) const
{
    return ammo_remaining_linked( here, nullptr );
}

int item::ammo_remaining( ) const
{
    return ammo_remaining( nullptr );
}

bool item::uses_energy() const
{
    if( is_vehicle_battery() ) {
        return true;
    }
    const item *mag = magazine_current();
    if( mag && mag->uses_energy() ) {
        return true;
    }
    return has_flag( flag_USES_BIONIC_POWER ) ||
           has_flag( flag_USE_UPS ) ||
           ( is_magazine() && ammo_capacity( ammo_battery ) > 0 );
}

bool item::is_chargeable() const
{
    if( !uses_energy() ) {
        return false;
    }
    // bionic power using items have ammo_capacity = player bionic power storage.  Since the items themselves aren't chargeable, auto fail unless they also have a magazine.
    if( has_flag( flag_USES_BIONIC_POWER ) && !magazine_current() ) {
        return false;
    }
    if( ammo_remaining() < ammo_capacity( ammo_battery ) ) {
        return true;
    } else {
        return false;
    }
}

units::energy item::energy_remaining( const Character *carrier ) const
{
    return energy_remaining( carrier, false );
}

units::energy item::energy_remaining( const Character *carrier, bool ignoreExternalSources ) const
{
    units::energy ret = 0_kJ;

    // Magazine in the item
    const item *mag = magazine_current();
    if( mag ) {
        ret += mag->energy_remaining( carrier );
    }

    if( !ignoreExternalSources ) {

        // Future energy based batteries
        if( is_vehicle_battery() ) {
            ret += energy;
        }

        // Power from bionic
        if( carrier != nullptr && has_flag( flag_USES_BIONIC_POWER ) ) {
            ret += carrier->get_power_level();
        }

        // Extra power from UPS
        if( carrier != nullptr && has_flag( flag_USE_UPS ) ) {
            ret += carrier->available_ups();
        }
    };

    // Battery(ammo) contained within
    if( is_magazine() ) {
        ret += energy;
        for( const item *e : contents.all_items_top( pocket_type::MAGAZINE ) ) {
            if( e->ammo_type() == ammo_battery ) {
                ret += units::from_kilojoule( static_cast<std::int64_t>( e->charges ) );
            }
        }
    }

    return ret;
}

int item::remaining_ammo_capacity() const
{
    if( ammo_types().empty() ) {
        return 0;
    }

    const itype *loaded_ammo = ammo_data();
    if( loaded_ammo == nullptr ) {
        return ammo_capacity( item::find_type( ammo_default() )->ammo->type ) - ammo_remaining( );
    } else {
        return ammo_capacity( loaded_ammo->ammo->type ) - ammo_remaining( );
    }
}

int item::ammo_capacity( const ammotype &ammo, bool include_linked ) const
{
    map &here = get_map();

    const item *mag = magazine_current();
    if( include_linked && has_link_data() ) {
        return link().t_veh ? link().t_veh->connected_battery_power_level( here ).second : 0;
    } else if( mag ) {
        return mag->ammo_capacity( ammo );
    } else if( has_flag( flag_USES_BIONIC_POWER ) ) {
        return units::to_kilojoule( get_player_character().get_max_power_level() );
    }

    if( contents.has_pocket_type( pocket_type::MAGAZINE ) ) {
        return contents.ammo_capacity( ammo );
    }
    if( is_magazine() ) {
        return type->magazine->capacity;
    }
    return 0;
}

int item::ammo_required() const
{
    if( is_gun() ) {
        if( type->gun->ammo.empty() ) {
            return 0;
        } else {
            int modifier = 0;
            float multiplier = 1.0f;
            for( const item *mod : gunmods() ) {
                modifier += mod->type->gunmod->ammo_to_fire_modifier;
                multiplier *= mod->type->gunmod->ammo_to_fire_multiplier;
            }
            return ( type->gun->ammo_to_fire * multiplier ) + modifier;
        }
    }

    return type->charges_to_use();
}

item &item::first_ammo()
{
    return contents.first_ammo();
}

const item &item::first_ammo() const
{
    return contents.first_ammo();
}

bool item::ammo_sufficient( const Character *carrier, int qty ) const
{
    map &here = get_map();

    if( count_by_charges() ) {
        return ammo_remaining_linked( here, carrier ) >= qty;
    }

    if( is_comestible() ) {
        return true;
    }

    return shots_remaining( here, carrier ) >= qty;
}

bool item::ammo_sufficient( const Character *carrier, const std::string &method, int qty ) const
{
    auto iter = type->ammo_scale.find( method );
    if( iter != type->ammo_scale.end() ) {
        qty *= iter->second;
    }

    return ammo_sufficient( carrier, qty );
}

int item::ammo_consume( int qty, const tripoint_bub_ms &pos, Character *carrier )
{
    return item::ammo_consume( qty, get_map(), pos, carrier );
}

int item::ammo_consume( int qty, map &here, const tripoint_bub_ms &pos, Character *carrier )
{
    if( qty < 0 ) {
        debugmsg( "Cannot consume negative quantity of ammo for %s", tname() );
        return 0;
    }
    const int wanted_qty = qty;
    const bool is_tool_with_carrier = carrier != nullptr && is_tool();

    if( is_tool_with_carrier && has_flag( flag_USES_NEARBY_AMMO ) ) {
        const ammotype ammo = ammo_type();
        if( !ammo.is_null() ) {
            const inventory &carrier_inventory = carrier->crafting_inventory( &here );
            itype_id ammo_type = ammo->default_ammotype();
            const int charges_avalable = carrier_inventory.charges_of( ammo_type, INT_MAX );

            qty = std::min( wanted_qty, charges_avalable );

            std::vector<item_comp> components;
            components.emplace_back( ammo_type, qty );
            carrier->consume_items( components, 1 );
            return wanted_qty - qty;
        }
    }

    // Consume power from appliances/vehicles connected with cables
    if( has_link_data() ) {
        if( link().t_veh && link().efficiency >= MIN_LINK_EFFICIENCY ) {
            qty = link().t_veh->discharge_battery( here, qty, true );
        } else {
            const optional_vpart_position vp = here.veh_at( link().t_abs_pos );
            if( vp ) {
                qty = vp->vehicle().discharge_battery( here, qty, true );
            }
        }
    }

    // Consume charges loaded in the item or its magazines
    if( is_magazine() || uses_magazine() ) {
        qty -= contents.ammo_consume( qty, &here, pos );
        if( ammo_capacity( ammo_battery ) == 0 && carrier != nullptr ) {
            carrier->invalidate_weight_carried_cache();
        }
    }

    // Dirty fix: activating a container of meds leads here, and used to use up all of the charges.
    if( is_medication() && charges > 1 ) {
        int charg_used = std::min( charges, qty );
        charges -= charg_used;
        qty -= charg_used;
    }

    // Some weird internal non-item charges (used by grenades)
    if( is_tool() && type->tool->ammo_id.empty() ) {
        int charg_used = std::min( charges, qty );
        charges -= charg_used;
        qty -= charg_used;
    }

    bool is_off_grid_powered = has_flag( flag_USE_UPS ) || has_flag( flag_USES_BIONIC_POWER );

    // Modded tools can consume UPS/bionic energy instead of ammo.
    // Guns handle energy in energy_consume()
    if( is_tool_with_carrier && is_off_grid_powered ) {
        units::energy wanted_energy = units::from_kilojoule( static_cast<std::int64_t>( qty ) );

        if( has_flag( flag_USE_UPS ) ) {
            wanted_energy -= carrier->consume_ups( wanted_energy );
        }

        if( has_flag( flag_USES_BIONIC_POWER ) ) {
            units::energy bio_used = std::min( carrier->get_power_level(), wanted_energy );
            carrier->mod_power_level( -bio_used );
            wanted_energy -= bio_used;
        }

        // It is possible for this to cause rounding error due to different precision of energy
        // But that can happen only if there was not enough ammo and you shouldn't be able to use without sufficient ammo
        qty = units::to_kilojoule( wanted_energy );
    }
    return wanted_qty - qty;
}

units::energy item::energy_consume( units::energy qty, const tripoint_bub_ms &pos,
                                    Character *carrier,
                                    float fuel_efficiency )
{
    return item::energy_consume( qty, &get_map(), pos, carrier, fuel_efficiency );
}

units::energy item::energy_consume( units::energy qty, map *here, const tripoint_bub_ms &pos,
                                    Character *carrier,
                                    float fuel_efficiency )
{
    if( qty < 0_kJ ) {
        debugmsg( "Cannot consume negative quantity of energy for %s", tname() );
        return 0_kJ;
    }

    const units::energy wanted_energy = qty;

    // Consume battery(ammo) and other fuel (if allowed)
    if( is_battery() || fuel_efficiency >= 0 ) {
        int consumed_kj = contents.ammo_consume( units::to_kilojoule( qty ), here, pos, fuel_efficiency );
        qty -= units::from_kilojoule( static_cast<std::int64_t>( consumed_kj ) );
        // Either we're out of juice or truncating the value above means we didn't drain quite enough.
        // In the latter case at least this will bump up energy enough to satisfy the remainder,
        // if not it will drain the item all the way.
        // TODO: reconsider what happens with fuel burning, right now this stashes
        // the remainder of energy from burning the fuel in the item in question,
        // which potentially allows it to burn less fuel next time.
        // Do we want an implicit 1kJ battery in the generator to smooth things out?
        if( qty > energy ) {
            int64_t residual_drain = contents.ammo_consume( 1, here, pos, fuel_efficiency );
            energy += units::from_kilojoule( residual_drain );
        }
        if( qty > energy ) {
            qty -= energy;
            energy = 0_J;
        } else {
            energy -= qty;
            qty = 0_J;
        }
    }

    // Consume energy from contained magazine
    if( magazine_current() ) {
        qty -= magazine_current()->energy_consume( qty, here, pos, carrier );
    }

    // Consume UPS energy from various sources
    if( carrier != nullptr && has_flag( flag_USE_UPS ) ) {
        qty -= carrier->consume_ups( qty );
    }

    // Consume bio energy
    if( carrier != nullptr && has_flag( flag_USES_BIONIC_POWER ) ) {
        units::energy bio_used = std::min( carrier->get_power_level(), qty );
        carrier->mod_power_level( -bio_used );
        qty -= bio_used;
    }

    return wanted_energy - qty;
}

int item::activation_consume( int qty, const tripoint_bub_ms &pos, Character *carrier )
{
    return ammo_consume( qty * ammo_required(), pos, carrier );
}

bool item::has_ammo() const
{
    const item *mag = magazine_current();
    if( mag ) {
        return mag->has_ammo();
    }

    if( is_ammo() ) {
        return true;
    }

    if( is_magazine() ) {
        return !contents.empty() && contents.first_ammo().has_ammo();
    }

    auto mods = is_gun() ? gunmods() : toolmods();
    for( const item *e : mods ) {
        if( !e->type->mod->ammo_modifier.empty() && e->has_ammo() ) {
            return true;
        }
    }

    return false;
}

bool item::has_ammo_data() const
{
    const item *mag = magazine_current();
    if( mag ) {
        return mag->has_ammo_data();
    }

    if( is_ammo() ) {
        return true;
    }

    if( is_magazine() ) {
        return !contents.empty() && contents.first_ammo().has_ammo_data();
    }

    auto mods = is_gun() ? gunmods() : toolmods();
    for( const item *e : mods ) {
        if( !e->type->mod->ammo_modifier.empty() && e->has_ammo_data() ) {
            return true;
        }
    }

    return false;
}

const itype *item::ammo_data() const
{
    const item *mag = magazine_current();
    if( mag ) {
        return mag->ammo_data();
    }

    if( is_ammo() ) {
        return type;
    }

    if( is_magazine() ) {
        return !contents.empty() ? contents.first_ammo().ammo_data() : nullptr;
    }

    auto mods = is_gun() ? gunmods() : toolmods();
    for( const item *e : mods ) {
        if( !e->type->mod->ammo_modifier.empty() && !e->ammo_current().is_null() &&
            item_controller->has_template( e->ammo_current() ) ) {
            return item_controller->find_template( e->ammo_current() );
        }
    }

    return nullptr;
}

itype_id item::ammo_current() const
{
    const itype *ammo = ammo_data();
    if( ammo ) {
        return ammo->get_id();
    }
    if( is_tool() && has_flag( flag_USES_NEARBY_AMMO ) ) {
        return ammo_default();
    }

    return itype_id::NULL_ID();
}

const item &item::loaded_ammo() const
{
    const item *mag = magazine_current();
    if( mag ) {
        return mag->loaded_ammo();
    }

    if( is_magazine() ) {
        return !contents.empty() ? contents.first_ammo() : null_item_reference();
    }

    auto mods = is_gun() ? gunmods() : toolmods();
    for( const item *e : mods ) {
        const item &mod_ammo = e->loaded_ammo();
        if( !mod_ammo.is_null() ) {
            return mod_ammo;
        }
    }

    if( is_gun() && ammo_remaining( ) != 0 ) {
        return contents.first_ammo();
    }
    return null_item_reference();
}

std::set<ammotype> item::ammo_types( bool conversion ) const
{
    if( conversion ) {
        const std::vector<const item *> &mods = is_gun() ? gunmods() : toolmods();
        for( const item *e : mods ) {
            if( !e->type->mod->ammo_modifier.empty() ) {
                return e->type->mod->ammo_modifier;
            }
        }
    }

    if( is_gun() ) {
        return type->gun->ammo;
    }

    if( is_tool() && has_flag( flag_USES_NEARBY_AMMO ) ) {
        return type->tool->ammo_id;
    }

    return contents.ammo_types();
}

ammotype item::ammo_type() const
{
    if( is_ammo() ) {
        return type->ammo->type;
    }

    if( is_tool() && has_flag( flag_USES_NEARBY_AMMO ) ) {
        const std::set<ammotype> ammo_type_choices = ammo_types();
        if( !ammo_type_choices.empty() ) {
            return *ammo_type_choices.begin();
        }
    }

    return ammotype::NULL_ID();
}

itype_id item::ammo_default( bool conversion ) const
{
    if( !ammo_types( conversion ).empty() ) {
        itype_id res = ammotype( *ammo_types( conversion ).begin() )->default_ammotype();
        if( !res.is_empty() ) {
            return res;
        }
    }

    return itype_id::NULL_ID();
}

std::string item::print_ammo( ammotype at, const item *gun ) const
{
    const item *mag = magazine_current();
    if( mag ) {
        return mag->print_ammo( at, this );
    } else if( has_flag( flag_USES_BIONIC_POWER ) ) {
        return _( "energy" );
    }

    // for magazines if we have a gun in mind display damage as well
    if( type->magazine ) {
        if( gun ) {
            return enumerate_as_string( type->magazine->cached_ammos[at],
            [gun]( const itype_id & id ) {
                return string_format( "<info>%s</info>(%d)", id->nname( 1 ),
                                      static_cast<int>( gun->gun_damage( id ).total_damage() ) );
            } );
        } else {
            return enumerate_as_string( type->magazine->cached_ammos[at],
            []( const itype_id & id ) {
                return string_format( "<info>%s</info>", id->nname( 1 ) );
            } );
        }
    }

    if( type->gun ) {
        return enumerate_as_string( type->gun->cached_ammos[at],
        [this]( const itype_id & id ) {
            return string_format( "<info>%s(%d)</info>", id->nname( 1 ),
                                  static_cast<int>( gun_damage( id ).total_damage() ) );
        } );
    }

    return "";
}

itype_id item::common_ammo_default( bool conversion ) const
{
    for( const ammotype &at : ammo_types( conversion ) ) {
        const item *mag = magazine_current();
        if( mag && mag->type->magazine->type.count( at ) ) {
            itype_id res = at->default_ammotype();
            if( !res.is_empty() ) {
                return res;
            }
        }
    }
    return itype_id::NULL_ID();
}

std::set<ammo_effect_str_id> item::ammo_effects( bool with_ammo ) const
{
    if( !is_gun() ) {
        return std::set<ammo_effect_str_id>();
    }

    std::set<ammo_effect_str_id> res = type->gun->ammo_effects;
    if( with_ammo && has_ammo() ) {
        res.insert( ammo_data()->ammo->ammo_effects.begin(), ammo_data()->ammo->ammo_effects.end() );
    }

    for( const item *mod : gunmods() ) {
        res.insert( mod->type->gunmod->ammo_effects.begin(), mod->type->gunmod->ammo_effects.end() );
    }

    return res;
}

std::string item::ammo_sort_name() const
{
    if( is_magazine() || is_gun() || is_tool() ) {
        const std::set<ammotype> &types = ammo_types();
        if( !types.empty() ) {
            return ammotype( *types.begin() )->name();
        }
    }
    if( is_ammo() ) {
        return ammo_type()->name();
    }
    return "";
}

bool item::magazine_integral() const
{
    return contents.has_pocket_type( pocket_type::MAGAZINE );
}

bool item::uses_magazine() const
{
    return contents.has_pocket_type( pocket_type::MAGAZINE_WELL );
}

itype_id item::magazine_default( bool conversion ) const
{
    // consider modded ammo types
    itype_id ammo;
    if( conversion && ( ammo = ammo_default(), !ammo.is_null() ) ) {
        for( const itype_id &mag : contents.magazine_compatible() ) {
            auto mag_types = mag->magazine->type;
            if( mag_types.find( ammo->ammo->type ) != mag_types.end() ) {
                return mag;
            }
        }
    }

    // otherwise return the default
    return contents.magazine_default();
}

std::set<itype_id> item::magazine_compatible() const
{
    return contents.magazine_compatible();
}

item *item::magazine_current()
{
    return contents.magazine_current();
}

const item *item::magazine_current() const
{
    return const_cast<item *>( this )->magazine_current();
}

std::vector<item *> item::gunmods()
{
    return contents.gunmods();
}

std::vector<const item *> item::gunmods() const
{
    return contents.gunmods();
}

std::vector<const item *> item::mods() const
{
    return contents.mods();
}

std::vector<const item *> item::softwares() const
{
    return contents.softwares();
}

std::vector<const item *> item::ebooks() const
{
    return contents.ebooks();
}

std::vector< item *> item::efiles()
{
    return contents.efiles();
}

std::vector<const item *> item::efiles() const
{
    return contents.efiles();
}

std::vector<const item *> item::cables() const
{
    return contents.cables();
}

item *item::gunmod_find( const itype_id &mod )
{
    std::vector<item *> mods = gunmods();
    auto it = std::find_if( mods.begin(), mods.end(), [&mod]( item * e ) {
        return e->typeId() == mod;
    } );
    return it != mods.end() ? *it : nullptr;
}

const item *item::gunmod_find( const itype_id &mod ) const
{
    return const_cast<item *>( this )->gunmod_find( mod );
}

item *item::gunmod_find_by_flag( const flag_id &flag )
{
    std::vector<item *> mods = gunmods();
    auto it = std::find_if( mods.begin(), mods.end(), [&flag]( item * e ) {
        return e->has_flag( flag );
    } );
    return it != mods.end() ? *it : nullptr;
}

ret_val<void> item::is_gunmod_compatible( const item &mod ) const
{
    if( !mod.is_gunmod() ) {
        debugmsg( "Tried checking compatibility of non-gunmod" );
        return ret_val<void>::make_failure();
    }
    static const gun_type_type pistol_gun_type( translate_marker_context( "gun_type_type", "pistol" ) );

    if( !is_gun() ) {
        return ret_val<void>::make_failure( _( "isn't a weapon" ) );

    } else if( is_gunmod() ) {
        return ret_val<void>::make_failure( _( "is a gunmod and cannot be modded" ) );

    } else if( gunmod_find( mod.typeId() ) ) {
        return ret_val<void>::make_failure( _( "already has a %s" ), mod.tname( 1 ) );

    } else if( !get_mod_locations().count( mod.type->gunmod->location ) ) {
        return ret_val<void>::make_failure( _( "doesn't have a slot for this mod" ) );

    } else if( get_free_mod_locations( mod.type->gunmod->location ) <= 0 ) {
        return ret_val<void>::make_failure( _( "doesn't have enough room for another %s mod" ),
                                            mod.type->gunmod->location.name() );

    } else if( !mod.type->gunmod->usable.count( gun_type() ) &&
               !mod.type->gunmod->usable.count( gun_type_type( typeId().str() ) ) ) {
        return ret_val<void>::make_failure( _( "cannot have a %s" ), mod.tname() );

    } else if( typeId() == itype_hand_crossbow &&
               !mod.type->gunmod->usable.count( pistol_gun_type ) ) {
        return ret_val<void>::make_failure( _( "isn't big enough to use that mod" ) );

    } else if( mod.type->gunmod->location.str() == "underbarrel" &&
               !mod.has_flag( flag_PUMP_RAIL_COMPATIBLE ) && has_flag( flag_PUMP_ACTION ) ) {
        return ret_val<void>::make_failure( _( "can only accept small mods on that slot" ) );

    } else if( mod.typeId() == itype_waterproof_gunmod && has_flag( flag_WATERPROOF_GUN ) ) {
        return ret_val<void>::make_failure( _( "is already waterproof" ) );

    } else if( mod.typeId() == itype_tuned_mechanism && has_flag( flag_NEVER_JAMS ) ) {
        return ret_val<void>::make_failure( _( "is already eminently reliable" ) );

    } else if( mod.typeId() == itype_brass_catcher && has_flag( flag_RELOAD_EJECT ) ) {
        return ret_val<void>::make_failure( _( "cannot have a brass catcher" ) );

    } else if( ( mod.type->gunmod->location.name() == "magazine" ||
                 mod.type->gunmod->location.name() == "mechanism" ||
                 mod.type->gunmod->location.name() == "loading port" ||
                 mod.type->gunmod->location.name() == "bore" ) &&
               ( ammo_remaining( ) > 0 || magazine_current() ) ) {
        return ret_val<void>::make_failure( _( "must be unloaded before installing this mod" ) );

    } else if( gunmod_find( itype_stock_none ) &&
               mod.type->gunmod->location.name() == "stock accessory" ) {
        return ret_val<void>::make_failure( _( "doesn't have a stock to attach this mod" ) );

    } else if( mod.typeId() == itype_arredondo_chute ) {
        return ret_val<void>::make_failure( _( "chute needs modification before attaching" ) );

        // Acceptable_ammo check is kinda weird now, if it is passed, checks after it will be ignored.
        // Moved it here as a workaround.
    } else if( !mod.type->mod->acceptable_ammo.empty() ) {
        bool compat_ammo = false;
        for( const ammotype &at : mod.type->mod->acceptable_ammo ) {
            if( ammo_types( false ).count( at ) ) {
                compat_ammo = true;
            }
        }
        if( !compat_ammo ) {
            return ret_val<void>::make_failure(
                       _( "%1$s cannot be used on item with no compatible ammo types" ), mod.tname( 1 ) );
        }
    }

    for( const gunmod_location &slot : mod.type->gunmod->blacklist_slot ) {
        if( get_mod_locations().count( slot ) ) {
            return ret_val<void>::make_failure( _( "cannot be installed on a weapon with a \"%s\"" ),
                                                slot.name() );
        }
    }

    for( const itype_id &testmod : mod.type->gunmod->blacklist_mod ) {
        if( gunmod_find( testmod ) ) {
            return ret_val<void>::make_failure( _( "cannot be installed on a weapon with a \"%s\"" ),
                                                ( testmod->nname( 1 ) ) );
        }
    }

    return ret_val<void>::make_success();
}

std::map<gun_mode_id, gun_mode> item::gun_all_modes() const
{
    std::map<gun_mode_id, gun_mode> res;

    if( !is_gun() && !is_gunmod() ) {
        return res;
    }

    std::vector<const item *> opts = gunmods();
    opts.push_back( this );

    for( const item *e : opts ) {

        // handle base item plus any auxiliary gunmods
        if( e->is_gun() ) {
            for( const std::pair<const gun_mode_id, gun_modifier_data> &m : e->type->gun->modes ) {
                // prefix attached gunmods, e.g. M203_DEFAULT to avoid index key collisions
                std::string prefix = e->is_gunmod() ? ( std::string( e->typeId() ) += "_" ) : "";
                std::transform( prefix.begin(), prefix.end(), prefix.begin(),
                                static_cast<int( * )( int )>( toupper ) );

                const int qty = m.second.qty();

                res.emplace( gun_mode_id( prefix + m.first.str() ), gun_mode( m.second.name(),
                             const_cast<item *>( e ),
                             qty, m.second.flags() ) );
            }

            // non-auxiliary gunmods may provide additional modes for the base item
        } else if( e->is_gunmod() ) {
            for( const std::pair<const gun_mode_id, gun_modifier_data> &m : e->type->gunmod->mode_modifier ) {
                //checks for melee gunmod, points to gunmod
                if( m.first == gun_mode_REACH ) {
                    res.emplace( m.first, gun_mode { m.second.name(), const_cast<item *>( e ),
                                                     m.second.qty(), m.second.flags() } );
                    //otherwise points to the parent gun, not the gunmod
                } else {
                    res.emplace( m.first, gun_mode { m.second.name(), const_cast<item *>( this ),
                                                     m.second.qty(), m.second.flags() } );
                }
            }
        }
    }

    return res;
}

gun_mode item::gun_get_mode( const gun_mode_id &mode ) const
{
    if( is_gun() ) {
        for( const std::pair<const gun_mode_id, gun_mode> &e : gun_all_modes() ) {
            if( e.first == mode ) {
                return e.second;
            }
        }
    }
    return gun_mode();
}

gun_mode item::gun_current_mode() const
{
    return gun_get_mode( gun_get_mode_id() );
}

gun_mode_id item::gun_get_mode_id() const
{
    if( !is_gun() || is_gunmod() ) {
        return gun_mode_id();
    }
    return gun_mode_id( get_var( GUN_MODE_VAR_NAME, "DEFAULT" ) );
}

bool item::gun_set_mode( const gun_mode_id &mode )
{
    if( !is_gun() || is_gunmod() || !gun_all_modes().count( mode ) ) {
        return false;
    }
    set_var( GUN_MODE_VAR_NAME, mode.str() );
    return true;
}

void item::gun_cycle_mode()
{
    if( !is_gun() || is_gunmod() ) {
        return;
    }

    const gun_mode_id cur = gun_get_mode_id();
    const std::map<gun_mode_id, gun_mode> modes = gun_all_modes();

    for( auto iter = modes.begin(); iter != modes.end(); ++iter ) {
        if( iter->first == cur ) {
            if( std::next( iter ) == modes.end() ) {
                break;
            }
            gun_set_mode( std::next( iter )->first );
            return;
        }
    }
    gun_set_mode( modes.begin()->first );
}

const use_function *item::get_use( const std::string &use_name ) const
{
    const use_function *fun = nullptr;
    visit_items(
    [&fun, &use_name]( const item * it, auto ) {
        if( it == nullptr ) {
            return VisitResponse::SKIP;
        }
        fun = it->get_use_internal( use_name );
        if( fun != nullptr ) {
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } );

    return fun;
}

const use_function *item::get_use_internal( const std::string &use_name ) const
{
    if( type != nullptr ) {
        return type->get_use( use_name );
    }
    return nullptr;
}

template<typename Item>
Item *item::get_usable_item_helper( Item &self, const std::string &use_name )
{
    Item *ret = nullptr;
    self.visit_items(
    [&ret, &use_name]( item * it, auto ) {
        if( it == nullptr ) {
            return VisitResponse::SKIP;
        }
        if( it->get_use_internal( use_name ) ) {
            ret = it;
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } );

    return ret;
}

const item *item::get_usable_item( const std::string &use_name ) const
{
    return get_usable_item_helper( *this, use_name );
}

item *item::get_usable_item( const std::string &use_name )
{
    return get_usable_item_helper( *this, use_name );
}

item::reload_option::reload_option( const reload_option & ) = default;

item::reload_option &item::reload_option::operator=( const reload_option & ) = default;

item::reload_option::reload_option( const Character *who, const item_location &target,
                                    const item_location &ammo ) :
    who( who ), target( target ), ammo( ammo )
{
    if( this->target->is_ammo_belt() && this->target->type->magazine->linkage ) {
        max_qty = this->who->charges_of( * this->target->type->magazine->linkage );
    }
    qty( max_qty );
}

int item::reload_option::moves() const
{
    int mv = ammo.obtain_cost( *who, qty() ) + who->item_reload_cost( *target, *ammo, qty() );
    if( target.has_parent() ) {
        item_location parent = target.parent_item();
        if( parent->is_gun() && !target->is_gunmod() ) {
            mv += parent->get_reload_time() * 1.5;
        } else if( parent->is_tool() ) {
            mv += 100;
        }
    }
    return mv;
}

void item::reload_option::qty( int val )
{
    bool ammo_in_container = ammo->is_ammo_container();
    bool ammo_in_liquid_container = ammo->is_watertight_container();
    item &ammo_obj = ( ammo_in_container || ammo_in_liquid_container ) ?
                     // this is probably not the right way to do this. there is no guarantee whatsoever that ammo_obj will be an ammo
                     *ammo->contents.all_items_top( pocket_type::CONTAINER ).front() : *ammo;

    if( ( ammo_in_container && !ammo_obj.is_ammo() ) ||
        ( ammo_in_liquid_container && !ammo_obj.made_of( phase_id::LIQUID ) ) ) {
        debugmsg( "Invalid reload option: %s", ammo_obj.tname() );
        return;
    }

    // Checking ammo capacity implicitly limits guns with removable magazines to capacity 0.
    // This gets rounded up to 1 later.
    int remaining_capacity = target->is_watertight_container() ?
                             target->get_remaining_capacity_for_liquid( ammo_obj, true ) :
                             target->remaining_ammo_capacity();
    if( target->has_flag( flag_RELOAD_ONE ) &&
        !( ammo->has_flag( flag_SPEEDLOADER ) || ammo->has_flag( flag_SPEEDLOADER_CLIP ) ) ) {
        remaining_capacity = 1;
    }
    if( ammo_obj.type->ammo ) {
        if( ammo_obj.ammo_type() == ammo_plutonium ) {
            remaining_capacity = remaining_capacity / PLUTONIUM_CHARGES +
                                 ( remaining_capacity % PLUTONIUM_CHARGES != 0 );
        }
    }

    bool ammo_by_charges = ammo_obj.is_ammo() || ammo_in_liquid_container;
    int available_ammo;
    if( ammo->has_flag( flag_SPEEDLOADER ) || ammo->has_flag( flag_SPEEDLOADER_CLIP ) ) {
        available_ammo = ammo_obj.ammo_remaining( );
    } else {
        available_ammo = ammo_by_charges ? ammo_obj.charges : ammo_obj.count();
    }
    // constrain by available ammo, target capacity and other external factors (max_qty)
    // @ref max_qty is currently set when reloading ammo belts and limits to available linkages
    qty_ = std::min( { val, available_ammo, remaining_capacity, max_qty } );

    // always expect to reload at least one charge
    qty_ = std::max( qty_, 1 );

}

int item::casings_count() const
{
    int res = 0;

    const_cast<item *>( this )->casings_handle( [&res]( item & ) {
        ++res;
        return false;
    } );

    return res;
}

void item::casings_handle( const std::function<bool( item & )> &func )
{
    if( !is_gun() && !is_tool() ) {
        return;
    }
    contents.casings_handle( func );
}

bool item::reload( Character &u, item_location ammo, int qty )
{
    if( qty <= 0 ) {
        debugmsg( "Tried to reload zero or less charges" );
        return false;
    }

    if( !ammo ) {
        debugmsg( "Tried to reload using non-existent ammo" );
        return false;
    }

    if( !can_reload_with( *ammo.get_item(), true ) ) {
        return false;
    }

    bool ammo_from_map = !ammo.held_by( u );
    if( ammo->has_flag( flag_SPEEDLOADER ) || ammo->has_flag( flag_SPEEDLOADER_CLIP ) ) {
        // if the thing passed in is a speed loader, we want the ammo
        ammo = item_location( ammo, &ammo->first_ammo() );
    }

    // limit quantity of ammo loaded to remaining capacity
    int limit = 0;
    if( is_watertight_container() && ammo->made_of_from_type( phase_id::LIQUID ) ) {
        limit = get_remaining_capacity_for_liquid( *ammo );
    } else if( ammo->is_ammo() ) {
        limit = ammo_capacity( ammo->ammo_type() ) - ammo_remaining( );
    }

    if( ammo->ammo_type() == ammo_plutonium ) {
        limit = limit / PLUTONIUM_CHARGES + ( limit % PLUTONIUM_CHARGES != 0 );
    }

    qty = std::min( qty, limit );

    casings_handle( [&u]( item & e ) {
        return u.i_add_or_drop( e );
    } );

    if( is_magazine() ) {
        qty = std::min( qty, ammo->charges );

        if( is_ammo_belt() && type->magazine->linkage ) {
            if( !u.use_charges_if_avail( *type->magazine->linkage, qty ) ) {
                debugmsg( "insufficient linkages available when reloading ammo belt" );
            }
        }

        item item_copy( *ammo );
        ammo->charges -= qty;

        if( ammo->ammo_type() == ammo_plutonium ) {
            // any excess is wasted rather than overfilling the item
            item_copy.charges = std::min( qty * PLUTONIUM_CHARGES, ammo_capacity( ammo_plutonium ) );
        } else {
            item_copy.charges = qty;
        }

        put_in( item_copy, pocket_type::MAGAZINE );

    } else if( is_watertight_container() && ammo->made_of_from_type( phase_id::LIQUID ) ) {
        item contents( *ammo );
        fill_with( contents, qty );
        if( ammo.has_parent() ) {
            ammo.parent_item()->contained_where( *ammo.get_item() )->on_contents_changed();
        }
        ammo->charges -= qty;
    } else {
        item magazine_removed;
        bool allow_wield = false;
        // if we already have a magazine loaded prompt to eject it
        if( magazine_current() ) {
            // we don't want to replace wielded `ammo` with ejected magazine
            // as it will invalidate `ammo`. Also keep wielding the item being reloaded.
            allow_wield = ( !u.is_wielding( *ammo ) && !u.is_wielding( *this ) );
            // Defer placing the magazine into inventory until new magazine is installed.
            magazine_removed = *magazine_current();
            remove_item( *magazine_current() );
        }

        put_in( *ammo, pocket_type::MAGAZINE_WELL );
        ammo.remove_item();
        if( ammo_from_map ) {
            u.invalidate_weight_carried_cache();
        }
        if( magazine_removed.type != nullitem() ) {
            u.i_add( magazine_removed, true, nullptr, nullptr, true, allow_wield );
        }
        return true;
    }

    if( ammo->charges == 0 ) {
        ammo.remove_item();
    }
    if( ammo_from_map ) {
        u.invalidate_weight_carried_cache();
    }
    return true;
}

float item::simulate_burn( fire_data &frd ) const
{
    const std::map<material_id, int> &mats = made_of();
    float smoke_added = 0.0f;
    float time_added = 0.0f;
    float burn_added = 0.0f;
    const units::volume vol = base_volume();
    const int effective_intensity = frd.contained ? 3 : frd.fire_intensity;
    for( const auto &m : mats ) {
        const mat_burn_data &bd = m.first->burn_data( effective_intensity );
        if( bd.immune ) {
            // Made to protect from fire
            return 0.0f;
        }

        // If fire is contained, burn rate is independent of volume
        if( frd.contained || bd.volume_per_turn == 0_ml ) {
            time_added += bd.fuel * m.second;
            smoke_added += bd.smoke * m.second;
            burn_added += bd.burn * m.second;
        } else {
            double volume_burn_rate = to_liter( bd.volume_per_turn ) / to_liter( vol );
            time_added += bd.fuel * volume_burn_rate * m.second;
            smoke_added += bd.smoke * volume_burn_rate * m.second;
            burn_added += bd.burn * volume_burn_rate * m.second;
        }
    }
    const int mat_total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;

    // Liquids that don't burn well smother fire well instead
    if( made_of( phase_id::LIQUID ) && time_added < 200 ) {
        time_added -= rng( 400.0 * to_liter( vol ), 1200.0 * to_liter( vol ) );
    } else if( mats.size() > 1 ) {
        // Average the materials
        time_added /= mat_total;
        smoke_added /= mat_total;
        burn_added /= mat_total;
    } else if( mats.empty() ) {
        // Non-liquid items with no specified materials will burn at moderate speed
        burn_added = 1;
    }

    // Fire will depend on amount of fuel if stackable
    if( count_by_charges() ) {
        int stack_burnt = std::max( 1, count() / type->stack_size );
        time_added *= stack_burnt;
        smoke_added *= stack_burnt;
        burn_added *= stack_burnt;
    }

    frd.fuel_produced += time_added;
    frd.smoke_produced += smoke_added;
    return burn_added;
}

bool item::burn( fire_data &frd )
{
    if( has_flag( flag_UNBREAKABLE ) ) {
        return false;
    }
    float burn_added = simulate_burn( frd );

    if( burn_added <= 0 ) {
        return false;
    }

    if( count_by_charges() ) {
        if( type->volume == 0_ml ) {
            charges = 0;
        } else {
            charges -= roll_remainder( burn_added * 250_ml * type->stack_size /
                                       ( 3.0 * type->volume ) );
        }

        return charges <= 0;
    }

    if( is_corpse() ) {
        const mtype *mt = get_mtype();
        if( active && mt != nullptr && burnt + burn_added > mt->hp &&
            !mt->burn_into.is_null() && mt->burn_into.is_valid() ) {
            corpse = &get_mtype()->burn_into.obj();
            // Delay rezing
            set_age( 0_turns );
            burnt = 0;
            return false;
        }
    } else if( has_temperature() ) {
        heat_up();
    }

    contents.heat_up();

    burnt += roll_remainder( burn_added );

    const int vol = base_volume() / 250_ml;
    return burnt >= vol * 3;
}

bool item::flammable( int threshold ) const
{
    const std::map<material_id, int> &mats = made_of();
    if( mats.empty() ) {
        // Don't know how to burn down something made of nothing.
        return false;
    }

    int flammability = 0;
    units::volume volume_per_turn = 0_ml;
    for( const std::pair<const material_id, int> &m : mats ) {
        const mat_burn_data &bd = m.first->burn_data( 1 );
        if( bd.immune ) {
            // Made to protect from fire
            return false;
        }

        flammability += bd.fuel * m.second;
        volume_per_turn += bd.volume_per_turn * m.second;
    }
    const int total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;
    flammability /= total;
    volume_per_turn /= total;

    if( threshold == 0 || flammability <= 0 ) {
        return flammability > 0;
    }

    units::volume vol = base_volume();
    if( volume_per_turn > 0_ml && volume_per_turn < vol ) {
        flammability = flammability * volume_per_turn / vol;
    } else {
        // If it burns well, it provides a bonus here
        flammability = flammability * vol / 250_ml;
    }

    return flammability > threshold;
}

itype_id item::typeId() const
{
    return type ? type->get_id() : itype_id::NULL_ID();
}

bool item::affects_fall() const
{
    return type ? type->fall_damage_reduction != 0 : false;
}
int item::fall_damage_reduction() const
{
    return type->fall_damage_reduction;
}
bool item::getlight( float &luminance, units::angle &width, units::angle &direction ) const
{
    luminance = 0;
    width = 0_degrees;
    direction = 0_degrees;
    if( light.luminance > 0 ) {
        luminance = static_cast<float>( light.luminance );
        if( light.width > 0 ) {  // width > 0 is a light arc
            width = units::from_degrees( light.width );
            direction = units::from_degrees( light.direction );
        }
        return true;
    } else {
        const int lumint = getlight_emit();
        if( lumint > 0 ) {
            luminance = static_cast<float>( lumint );
            return true;
        }
    }
    return false;
}

int item::getlight_emit() const
{
    const map &here = get_map();

    float lumint = type->light_emission;

    if( lumint == 0 ) {
        // gunmods can create light, but cache_visit_items_with() do not check items inside mod pockets
        // we will check it here
        if( is_gun() && !gunmods().empty() ) {
            for( const item *maybe_flashlight : gunmods() ) {
                if( maybe_flashlight->type->light_emission != 0 ) {
                    return maybe_flashlight->getlight_emit();
                }
            }
            return 0;
        }
    }

    if( ammo_required() == 0 || ( has_flag( flag_USE_UPS ) && ammo_capacity( ammo_battery ) == 0 ) ||
        has_flag( flag_USES_BIONIC_POWER ) ) {
        return lumint;
    }
    if( ammo_remaining_linked( here ) == 0 ) {
        return 0;
    }
    if( has_flag( flag_CHARGEDIM ) && is_tool() && !has_flag( flag_USE_UPS ) ) {
        // check if there's a link and it has a charge rate or charge interval
        // if so, use its power supply
        if( has_link_data() && ( link().charge_rate > 0 || link().charge_interval > 0 ) ) {
            return lumint;
        }

        // If we somehow got this far without a battery/power,
        // return an illumination value of 0
        if( !has_ammo() ) {
            return 0;
        }

        // Falloff starts at 1/5 total charge and scales linearly from there to 0.
        const ammotype &loaded_ammo = ammo_data()->ammo->type;
        if( ammo_capacity( loaded_ammo ) &&
            ammo_remaining_linked( here ) < ( ammo_capacity( loaded_ammo ) / 5 ) ) {
            lumint *= ammo_remaining_linked( here ) * 5.0 / ammo_capacity( loaded_ammo );
        }
    }
    return lumint;
}

bool item::use_amount( const itype_id &it, int &quantity, std::list<item> &used,
                       const std::function<bool( const item & )> &filter )
{
    if( is_null() ) {
        return false;
    }
    // Remember quantity so that we can unseal self
    int old_quantity = quantity;
    std::vector<item *> removed_items;
    const std::list<item *> temp_contained_list = all_items_ptr( pocket_type::CONTAINER );
    std::list<item *> contained_list;
    // Reverse the list, as it's created from the top down, but we have to remove items
    // from the bottom up in order for the references to remain valid until used.
    for( item *contained : temp_contained_list ) {
        contained_list.emplace_front( contained );
    }
    for( item *contained : contained_list ) {
        if( contained->use_amount_internal( it, quantity, used, filter ) ) {
            removed_items.push_back( contained );
        }
    }

    for( item *removed : removed_items ) {
        // Handle cases where items are removed but the pocket isn't emptied
        item *parent = this->find_parent( *removed );
        for( item_pocket *pocket : parent->get_all_standard_pockets() ) {
            if( pocket->has_item( *removed ) ) {
                pocket->unseal();
            }
        }
        this->remove_item( *removed );
    }

    if( quantity != old_quantity ) {
        on_contents_changed();
    }
    return use_amount_internal( it, quantity, used, filter );
}

// NOLINTNEXTLINE(readability-make-member-function-const)
bool item::use_amount_internal( const itype_id &it, int &quantity, std::list<item> &used,
                                const std::function<bool( const item & )> &filter )
{
    if( typeId().is_null() ) {
        return false;
    }
    if( typeId() == it && quantity > 0 && filter( *this ) ) {
        used.push_back( *this );
        quantity--;
        return true;
    } else {
        return false;
    }
}

bool item::allow_crafting_component() const
{
    if( has_flag( flag_PSEUDO ) ) {
        return false;
    }

    // vehicle batteries are implemented as magazines of charge
    if( is_magazine() && ammo_types().count( ammo_battery ) ) {
        return true;
    }

    // fixes #18886 - turret installation may require items with irremovable mods
    if( is_gun() ) {
        bool valid = true;
        visit_items( [&]( const item * it, item * ) {
            if( this == it ) {
                return VisitResponse::NEXT;
            }
            if( !( it->is_magazine() || ( it->is_gunmod() && it->is_irremovable() ) ) ) {
                valid = false;
                return VisitResponse::ABORT;
            }
            return VisitResponse::NEXT;
        } );
        return valid;
    }

    return true;
}

void item::set_item_specific_energy( const units::specific_energy &new_specific_energy )
{
    const float float_new_specific_energy = units::to_joule_per_gram( new_specific_energy );
    const float specific_heat_liquid = get_specific_heat_liquid(); // J/g K
    const float specific_heat_solid = get_specific_heat_solid(); // J/g K
    const float latent_heat = get_latent_heat(); // J/g
    const float freezing_temperature = units::to_kelvin( get_freeze_point() );
    // Energy that the item would have if it was completely solid at freezing temperature
    const float completely_frozen_specific_energy = specific_heat_solid * freezing_temperature; // J/g
    // Energy that the item would have if it was completely liquid at freezing temperature
    const float completely_liquid_specific_energy = completely_frozen_specific_energy + latent_heat;
    float new_item_temperature = 0.0f; // K
    float freeze_percentage = 1.0f;

    if( float_new_specific_energy > completely_liquid_specific_energy ) {
        // Item is liquid
        new_item_temperature = freezing_temperature + ( float_new_specific_energy -
                               completely_liquid_specific_energy ) /
                               specific_heat_liquid;
        freeze_percentage = 0.0f;
    } else if( float_new_specific_energy < completely_frozen_specific_energy ) {
        // Item is solid
        new_item_temperature = float_new_specific_energy / specific_heat_solid;
    } else {
        // Item is partially solid
        new_item_temperature = freezing_temperature;
        freeze_percentage = ( completely_liquid_specific_energy - float_new_specific_energy ) /
                            ( completely_liquid_specific_energy - completely_frozen_specific_energy );
    }

    temperature = units::from_kelvin( new_item_temperature );
    specific_energy = new_specific_energy;
    set_temp_flags( temperature, freeze_percentage );
    reset_temp_check();
}

units::specific_energy item::get_specific_energy_from_temperature( const units::temperature
        &new_temperature ) const
{
    const float specific_heat_liquid = get_specific_heat_liquid(); // J/g K
    const float specific_heat_solid = get_specific_heat_solid(); // J/g K
    const float latent_heat = get_latent_heat(); // J/g
    const float freezing_temperature = units::to_kelvin( get_freeze_point() );
    // Energy that the item would have if it was completely solid at freezing temperature
    const float completely_frozen_energy = specific_heat_solid * freezing_temperature;
    // Energy that the item would have if it was completely liquid at freezing temperature
    const float completely_liquid_energy = completely_frozen_energy + latent_heat;
    float new_specific_energy;

    if( units::to_kelvin( new_temperature ) <= freezing_temperature ) {
        new_specific_energy = specific_heat_solid * units::to_kelvin( new_temperature );
    } else {
        new_specific_energy = completely_liquid_energy + specific_heat_liquid *
                              ( units::to_kelvin( new_temperature ) - freezing_temperature );
    }
    return units::from_joule_per_gram( new_specific_energy );
}

void item::set_item_temperature( units::temperature new_temperature )
{
    const float freezing_temperature = units::to_kelvin( get_freeze_point() );
    const float specific_heat_solid = get_specific_heat_solid(); // J/g K
    const float latent_heat = get_latent_heat(); // J/g

    units::specific_energy new_specific_energy = get_specific_energy_from_temperature(
                new_temperature );
    float freeze_percentage = 0.0f;

    temperature = new_temperature;
    specific_energy = new_specific_energy ;

    // Energy that the item would have if it was completely solid at freezing temperature
    const float completely_frozen_specific_energy = specific_heat_solid * freezing_temperature;
    // Energy that the item would have if it was completely liquid at freezing temperature
    const float completely_liquid_specific_energy = completely_frozen_specific_energy + latent_heat;

    if( units::to_joule_per_gram( new_specific_energy ) < completely_frozen_specific_energy ) {
        // Item is solid
        freeze_percentage = 1;
    } else {
        // Item is partially solid
        freeze_percentage = ( completely_liquid_specific_energy - units::to_joule_per_gram(
                                  new_specific_energy ) ) /
                            ( completely_liquid_specific_energy - completely_frozen_specific_energy );
    }
    set_temp_flags( new_temperature, freeze_percentage );
    reset_temp_check();
}

void item::set_countdown( int num_turns )
{
    if( num_turns < 0 ) {
        debugmsg( "Tried to set a negative countdown value %d.", num_turns );
        return;
    }
    if( !ammo_types().empty() ) {
        debugmsg( "Tried to set countdown on an item with ammo." );
        return;
    }
    charges = num_turns;
}

bool item::use_charges( const itype_id &what, int &qty, std::list<item> &used,
                        const tripoint_bub_ms &pos, const std::function<bool( const item & )> &filter,
                        Character *carrier, bool in_tools )
{
    std::vector<item *> del;

    visit_items(
    [&what, &qty, &used, &pos, &del, &filter, &carrier, &in_tools]( item * e, item * parent ) {
        if( qty == 0 ) {
            // found sufficient charges
            return VisitResponse::ABORT;
        }

        if( !filter( *e ) ) {
            return VisitResponse::NEXT;
        }

        if( e->is_tool() || e->is_gun() ) {
            if( e->typeId() == what || ( in_tools && e->ammo_current() == what ) ) {
                int n;
                if( carrier ) {
                    n = e->ammo_consume( qty, pos, carrier );
                } else {
                    n = e->ammo_consume( qty, pos, nullptr );
                }

                if( n > 0 ) {
                    item temp( *e );
                    used.push_back( temp );
                    qty -= n;
                    return VisitResponse::SKIP;
                }
            }
            return VisitResponse::NEXT;

        } else if( e->count_by_charges() ) {
            if( e->typeId() == what ) {
                // if can supply excess charges split required off leaving remainder in-situ
                item obj = e->split( qty );
                if( parent ) {
                    parent->contained_where( *e )->on_contents_changed();
                    parent->on_contents_changed();
                }
                if( !obj.is_null() ) {
                    used.push_back( obj );
                    qty = 0;
                    return VisitResponse::ABORT;
                }

                qty -= e->charges;
                used.push_back( *e );
                del.push_back( e );
            }
            // items counted by charges are not themselves expected to be containers
            return VisitResponse::SKIP;
        }

        // recurse through any nested containers
        return VisitResponse::NEXT;
    } );

    bool destroy = false;
    for( item *e : del ) {
        if( e == this ) {
            destroy = true; // cannot remove ourselves...
        } else {
            remove_item( *e );
        }
    }

    return destroy;
}

void item::set_snippet( const snippet_id &id )
{
    if( is_null() ) {
        return;
    }
    if( !id.is_valid() ) {
        debugmsg( "there's no snippet with id %s", id.str() );
        return;
    }
    snip_id = id;
}

const item_category &item::get_category_shallow() const
{
    static item_category null_category;
    return type->category_force.is_valid() ? type->category_force.obj() : null_category;
}

const item_category &item::get_category_of_contents( int depth, int maxdepth ) const
{
    if( depth < maxdepth && type->category_force == item_category_container ) {
        if( cached_category.timestamp == calendar::turn ) {
            return cached_category.id.obj();
        }
        ++depth;
        if( item::aggregate_t const aggi = aggregated_contents( depth, maxdepth ); aggi ) {
            item_category const &cat = aggi.header->get_category_of_contents( depth, maxdepth );
            cached_category = { cat.get_id(), calendar::turn };
            return cat;
        }
    }
    return this->get_category_shallow();
}

bool item::will_explode_in_fire() const
{
    if( type->explode_in_fire ) {
        return true;
    }

    if( type->ammo && ( type->ammo->special_cookoff || type->ammo->cookoff ) ) {
        return true;
    }

    return false;
}

bool item::detonate( const tripoint_bub_ms &p, std::vector<item> &drops )
{
    const Creature *source = get_player_character().get_faction()->id == owner
                             ? &get_player_character()
                             : nullptr;
    if( type->explosion.power >= 0 ) {
        explosion_handler::explosion( source, p, type->explosion );
        return true;
    } else if( type->ammo && ( type->ammo->special_cookoff || type->ammo->cookoff ) ) {
        int charges_remaining = charges;
        const int rounds_exploded = rng( 1, charges_remaining / 2 );
        if( type->ammo->special_cookoff ) {
            // If it has a special effect just trigger it.
            apply_ammo_effects( nullptr, p, type->ammo->ammo_effects, 1 );
        }
        if( type->ammo->cookoff ) {
            // If ammo type can burn, then create an explosion proportional to quantity.
            float power = 3.0f * std::pow( rounds_exploded / 25.0f, 0.25f );
            explosion_handler::explosion( nullptr, p, power, 0.0f, false, 0 );
        }
        charges_remaining -= rounds_exploded;
        if( charges_remaining > 0 ) {
            item temp_item = *this;
            temp_item.charges = charges_remaining;
            drops.push_back( temp_item );
        }

        return true;
    }

    return false;
}
bool item::has_rotten_away() const
{
    if( is_corpse() && !can_revive() ) {
        return get_rot() > 10_days;
    } else {
        return get_relative_rot() > 2.0;
    }
}

bool item_ptr_compare_by_charges( const item *left, const item *right )
{
    if( left->empty() ) {
        return false;
    } else if( right->empty() ) {
        return true;
    } else {
        return right->only_item().charges < left->only_item().charges;
    }
}

bool item_compare_by_charges( const item &left, const item &right )
{
    return item_ptr_compare_by_charges( &left, &right );
}

static const std::string USED_BY_IDS( "USED_BY_IDS" );
bool item::already_used_by_player( const Character &p ) const
{
    const auto it = item_vars.find( USED_BY_IDS );
    if( it == item_vars.end() ) {
        return false;
    }
    // USED_BY_IDS always starts *and* ends with a ';', the search string
    // ';<id>;' matches at most one part of USED_BY_IDS, and only when exactly that
    // id has been added.
    const std::string needle = string_format( ";%d;", p.getID().get_value() );
    return it->second.str().find( needle ) != std::string::npos;
}

void item::mark_as_used_by_player( const Character &p )
{
    std::string used_by_ids = get_value( USED_BY_IDS ).str();
    if( used_by_ids.empty() ) {
        // *always* start with a ';'
        used_by_ids = ";";
    }
    // and always end with a ';'
    used_by_ids += string_format( "%d;", p.getID().get_value() );
    set_var( USED_BY_IDS, used_by_ids );
}

std::string item::components_to_string() const
{
    using t_count_map = std::map<std::string, int>;
    t_count_map counts;
    for( const item_components::type_vector_pair &tvp : components ) {
        for( const item &it : tvp.second ) {
            if( !it.has_flag( flag_BYPRODUCT ) ) {
                const std::string name = it.display_name();
                counts[name]++;
            }
        }
    }
    return enumerate_as_string( counts,
    []( const std::pair<std::string, int> &entry ) {
        return entry.second != 1
               ? string_format( pgettext( "components count", "%d x %s" ), entry.second, entry.first )
               : entry.first;
    }, enumeration_conjunction::none );
}

uint64_t item::make_component_hash() const
{
    std::string concatenated_ids;
    for( const item_components::type_vector_pair &tvp : components ) {
        for( size_t i = 0; i < tvp.second.size(); ++i ) {
            concatenated_ids += tvp.first.str();
        }
    }

    std::hash<std::string> hasher;
    return hasher( concatenated_ids );
}

bool item::needs_processing() const
{
    return processing_speed() != NO_PROCESSING;
}

int item::processing_speed() const
{
    if( is_corpse() || is_comestible() ) {
        return to_turns<int>( 10_minutes );
    }

    if( active || ethereal || wetness || has_link_data() ||
        has_flag( flag_RADIO_ACTIVATION ) || has_relic_recharge() ||
        has_fault_flag( flag_BLACKPOWDER_FOULING_DAMAGE ) || get_var( "gun_heat", 0 ) > 0 ||
        has_fault( fault_emp_reboot ) ) {
        // Unless otherwise indicated, update every turn.
        return 1;
    }

    return item::NO_PROCESSING;
}

void item::apply_freezerburn()
{
    if( !has_flag( flag_FREEZERBURN ) ) {
        return;
    }
    set_flag( flag_MUSHY );
}

bool item::process_temperature_rot( float insulation, const tripoint_bub_ms &pos, map &here,
                                    Character *carrier, const temperature_flag flag, float spoil_modifier, bool watertight_container )
{
    const time_point now = calendar::turn;

    // if player debug menu'd the time backward it breaks stuff, just reset the
    // last_temp_check in this case
    if( now - last_temp_check < 0_turns ) {
        reset_temp_check();
        return false;
    }

    // process temperature and rot at most once every 100_turns (10 min)
    // note we're also gated by item::processing_speed
    time_duration smallest_interval = 10_minutes;
    if( now - last_temp_check < smallest_interval && units::to_joule_per_gram( specific_energy ) > 0 ) {
        return false;
    }

    units::temperature temp = get_weather().get_temperature( pos );

    switch( flag ) {
        case temperature_flag::NORMAL:
            // Just use the temperature normally
            break;
        case temperature_flag::FRIDGE:
            temp = std::min( temp, temperatures::fridge );
            break;
        case temperature_flag::FREEZER:
            temp = std::min( temp, temperatures::freezer );
            break;
        case temperature_flag::HEATER:
            temp = std::max( temp, temperatures::normal );
            break;
        case temperature_flag::ROOT_CELLAR:
            temp = units::from_celsius( get_weather().get_cur_weather_gen().base_temperature );
            break;
        default:
            debugmsg( "Temperature flag enum not valid.  Using current temperature." );
    }

    bool carried = carrier != nullptr;
    // body heat increases inventory temperature by 5 F (2.77 K) and insulation by 50%
    if( carried ) {
        insulation *= 1.5;
        temp += units::from_fahrenheit_delta( 5 );
    }

    time_point time = last_temp_check;
    item_internal::scoped_goes_bad_cache _cache( this );
    const bool process_rot = goes_bad() && spoil_modifier != 0;
    const bool decays_in_air = !watertight_container && has_flag( flag_DECAYS_IN_AIR ) &&
                               type->revert_to;
    int64_t max_air_exposure_hours = decays_in_air ? get_property_int64_t( "max_air_exposure_hours" ) :
                                     0;

    if( now - time > 1_hours ) {
        // This code is for items that were left out of reality bubble for long time

        const weather_generator &wgen = get_weather().get_cur_weather_gen();
        const unsigned int seed = g->get_seed();

        units::temperature_delta temp_mod;
        // Toilets and vending machines will try to get the heat radiation and convection during mapgen and segfault.
        if( !g->new_game && !g->swapping_dimensions ) {
            temp_mod = get_heat_radiation( pos );
            temp_mod += get_convection_temperature( pos );
            temp_mod += here.get_temperature_mod( pos );
        } else {
            temp_mod = units::from_kelvin_delta( 0 );
        }

        if( carried ) {
            temp_mod += units::from_fahrenheit_delta( 5 ); // body heat increases inventory temperature
        }

        // Process the past of this item in 1h chunks until there is less than 1h left.
        time_duration time_delta = 1_hours;

        while( now - time > 1_hours ) {
            time += time_delta;

            // Get the environment temperature
            // Use weather if above ground, use map temp if below
            units::temperature env_temperature;
            if( pos.z() >= 0 && flag != temperature_flag::ROOT_CELLAR ) {
                env_temperature = wgen.get_weather_temperature( get_map().get_abs( pos ), time, seed );
            } else {
                env_temperature = units::from_celsius( get_weather().get_cur_weather_gen().base_temperature );
            }
            env_temperature += temp_mod;

            switch( flag ) {
                case temperature_flag::NORMAL:
                    // Just use the temperature normally
                    break;
                case temperature_flag::FRIDGE:
                    env_temperature = std::min( env_temperature, temperatures::fridge );
                    break;
                case temperature_flag::FREEZER:
                    env_temperature = std::min( env_temperature, temperatures::freezer );
                    break;
                case temperature_flag::HEATER:
                    env_temperature = std::max( env_temperature, temperatures::normal );
                    break;
                case temperature_flag::ROOT_CELLAR:
                    env_temperature =  units::from_celsius( get_weather().get_cur_weather_gen().base_temperature );
                    break;
                default:
                    debugmsg( "Temperature flag enum not valid.  Using normal temperature." );
            }

            // Calculate item temperature from environment temperature
            // If the time was more than 2 d ago we do not care about item temperature.
            if( now - time < 2_days ) {
                calc_temp( env_temperature, insulation, time_delta );
            }
            last_temp_check = time;

            if( decays_in_air &&
                process_decay_in_air( here, carrier, pos, max_air_exposure_hours, time_delta ) ) {
                return false;
            }

            // Calculate item rot
            if( process_rot ) {
                calc_rot( env_temperature, spoil_modifier, time_delta );

                if( has_rotten_away() && carrier == nullptr ) {
                    // No need to track item that will be gone
                    return true;
                }
            }
        }
    }

    // Remaining <1 h from above
    // and items that are held near the player
    if( now - time > smallest_interval ) {
        calc_temp( temp, insulation, now - time );
        last_temp_check = now;

        if( decays_in_air &&
            process_decay_in_air( here, carrier, pos, max_air_exposure_hours, now - time ) ) {
            return false;
        }

        if( process_rot ) {
            calc_rot( temp, spoil_modifier, now - time );
            return has_rotten_away() && carrier == nullptr;
        }
    }

    // Just now created items will get here.
    if( units::to_joule_per_gram( specific_energy ) < 0 ) {
        set_item_temperature( temp );
    }
    return false;
}

void item::calc_temp( const units::temperature &temp, const float insulation,
                      const time_duration &time_delta )
{
    const float env_temperature = units::to_kelvin( temp ); // K
    const float old_temperature = units::to_kelvin( temperature ); // K
    const float temperature_difference = env_temperature - old_temperature;

    // If no or only small temperature difference then no need to do math.
    if( std::abs( temperature_difference ) < 0.4 ) {
        return;
    }
    const float mass = to_gram( weight() ); // g

    // If item has negative energy set to environment temperature (it not been processed ever)
    if( units::to_joule_per_gram( specific_energy ) < 0 ) {
        set_item_temperature( temp );
        return;
    }

    // specific_energy = item thermal energy (J/g). Stored in the item
    // temperature = item temperature (K). Stored in the item
    const float conductivity_term = 0.0076 * std::pow( to_milliliter( volume() ),
                                    2.0 / 3.0 ) / insulation;
    const float specific_heat_liquid = get_specific_heat_liquid();
    const float specific_heat_solid = get_specific_heat_solid();
    const float latent_heat = get_latent_heat();
    const float freezing_temperature = units::to_kelvin( get_freeze_point() );
    // Energy that the item would have if it was completely solid at freezing temperature
    const units::specific_energy completely_frozen_specific_energy = units::from_joule_per_gram(
                specific_heat_solid * freezing_temperature );
    // Energy that the item would have if it was completely liquid at freezing temperature
    const units::specific_energy completely_liquid_specific_energy = completely_frozen_specific_energy +
            units::from_joule_per_gram( latent_heat );

    float new_specific_energy;
    float new_item_temperature;
    float freeze_percentage = 0.0f;
    int extra_time;

    // Temperature calculations based on Newton's law of cooling.
    // Calculations are done assuming that the item stays in its phase.
    // This assumption can cause over heating when transitioning from melting to liquid.
    // In other transitions the item may cool/heat too little but that won't be a problem.
    if( specific_energy < completely_frozen_specific_energy ) {
        // Was solid.
        new_item_temperature = ( - temperature_difference
                                 * std::exp( - to_turns<int>( time_delta ) * conductivity_term / ( mass * specific_heat_solid ) )
                                 + env_temperature );
        new_specific_energy = new_item_temperature * specific_heat_solid;
        if( new_item_temperature > freezing_temperature + 0.5 ) {
            // Started melting before temp was calculated.
            // 0.5 is here because we don't care if it heats up a bit too high
            // Calculate how long the item was solid
            // and apply rest of the time as melting
            extra_time = to_turns<int>( time_delta )
                         - std::log( - temperature_difference / ( freezing_temperature - env_temperature ) )
                         * ( mass * specific_heat_solid / conductivity_term );
            new_specific_energy = units::to_joule_per_gram( completely_frozen_specific_energy )
                                  + conductivity_term
                                  * ( env_temperature - freezing_temperature ) * extra_time / mass;
            new_item_temperature = freezing_temperature;
            if( units::from_joule_per_gram( new_specific_energy ) > completely_liquid_specific_energy ) {
                // The item then also finished melting.
                // This may happen rarely with very small items
                // Just set the item to environment temperature
                set_item_temperature( units::from_kelvin( env_temperature ) );
                return;
            }
        }
    } else if( specific_energy > completely_liquid_specific_energy ) {
        // Was liquid.
        new_item_temperature = ( - temperature_difference
                                 * std::exp( - to_turns<int>( time_delta ) * conductivity_term / ( mass *
                                             specific_heat_liquid ) )
                                 + env_temperature );
        new_specific_energy = ( new_item_temperature - freezing_temperature ) * specific_heat_liquid +
                              units::to_joule_per_gram( completely_liquid_specific_energy );
        if( new_item_temperature < freezing_temperature - 0.5 ) {
            // Started freezing before temp was calculated.
            // 0.5 is here because we don't care if it cools down a bit too low
            // Calculate how long the item was liquid
            // and apply rest of the time as freezing
            extra_time = to_turns<int>( time_delta )
                         - std::log( - temperature_difference / ( freezing_temperature - env_temperature ) )
                         * ( mass * specific_heat_liquid / conductivity_term );
            new_specific_energy = units::to_joule_per_gram( completely_liquid_specific_energy )
                                  + conductivity_term
                                  * ( env_temperature - freezing_temperature ) * extra_time / mass;
            new_item_temperature = freezing_temperature;
            if( units::from_joule_per_gram( new_specific_energy ) < completely_frozen_specific_energy ) {
                // The item then also finished freezing.
                // This may happen rarely with very small items
                // Just set the item to environment temperature
                set_item_temperature( units::from_kelvin( env_temperature ) );
                return;
            }
        }
    } else {
        // Was melting or freezing
        new_specific_energy = units::to_joule_per_gram( specific_energy ) + conductivity_term *
                              temperature_difference * to_turns<int>( time_delta ) / mass;
        new_item_temperature = freezing_temperature;
        if( units::from_joule_per_gram( new_specific_energy ) > completely_liquid_specific_energy ) {
            // Item melted before temp was calculated
            // Calculate how long the item was melting
            // and apply rest of the time as liquid
            extra_time = to_turns<int>( time_delta ) - ( mass / ( conductivity_term * temperature_difference ) )
                         *
                         units::to_joule_per_gram( completely_liquid_specific_energy - specific_energy );
            new_item_temperature = ( ( freezing_temperature - env_temperature )
                                     * std::exp( - extra_time * conductivity_term / ( mass *
                                                 specific_heat_liquid ) )
                                     + env_temperature );
            new_specific_energy = ( new_item_temperature - freezing_temperature ) * specific_heat_liquid +
                                  units::to_joule_per_gram( completely_liquid_specific_energy );
        } else if( units::from_joule_per_gram( new_specific_energy ) < completely_frozen_specific_energy ) {
            // Item froze before temp was calculated
            // Calculate how long the item was freezing
            // and apply rest of the time as solid
            extra_time = to_turns<int>( time_delta ) - ( mass / ( conductivity_term * temperature_difference ) )
                         *
                         units::to_joule_per_gram( completely_frozen_specific_energy - specific_energy );
            new_item_temperature = ( ( freezing_temperature - env_temperature )
                                     * std::exp( - extra_time * conductivity_term / ( mass *
                                                 specific_heat_solid ) )
                                     + env_temperature );
            new_specific_energy = new_item_temperature * specific_heat_solid;
        }
    }
    // Check freeze status now based on energies.
    if( units::from_joule_per_gram( new_specific_energy ) > completely_liquid_specific_energy ) {
        freeze_percentage = 0;
    } else if( units::from_joule_per_gram( new_specific_energy ) < completely_frozen_specific_energy ) {
        // Item is solid
        freeze_percentage = 1;
    } else {
        // Item is partially solid
        freeze_percentage = ( units::to_joule_per_gram( completely_liquid_specific_energy ) -
                              new_specific_energy ) /
                            units::to_joule_per_gram( completely_liquid_specific_energy - completely_frozen_specific_energy );
    }

    temperature = units::from_kelvin( new_item_temperature );
    specific_energy = units::from_joule_per_gram( new_specific_energy );
    set_temp_flags( temperature, freeze_percentage );
}

void item::set_temp_flags( units::temperature new_temperature, float freeze_percentage )
{
    units::temperature freezing_temperature = get_freeze_point();
    // Apply temperature tags tags
    // Hot = over temperatures::hot
    // Warm = over temperatures::warm
    // Cold = below temperatures::cold
    // Frozen = Over 50% frozen
    if( has_own_flag( flag_FROZEN ) ) {
        unset_flag( flag_FROZEN );
        if( freeze_percentage < 0.5 ) {
            // Item melts and becomes mushy
            current_phase = type->phase;
            apply_freezerburn();
            unset_flag( flag_SHREDDED );
            if( made_of( phase_id::LIQUID ) ) {
                set_flag( flag_FROM_FROZEN_LIQUID );
            }
        }
    } else if( has_own_flag( flag_COLD ) ) {
        unset_flag( flag_COLD );
    } else if( has_own_flag( flag_HOT ) ) {
        unset_flag( flag_HOT );
    }
    if( new_temperature > temperatures::hot ) {
        set_flag( flag_HOT );
    } else if( freeze_percentage > 0.5 ) {
        set_flag( flag_FROZEN );
        current_phase = phase_id::SOLID;
        // If below freezing temp AND the food may have parasites AND food does not have "NO_PARASITES" tag then add the "NO_PARASITES" tag.
        if( is_food() && new_temperature < freezing_temperature && get_comestible()->parasites > 0 ) {
            set_flag( flag_NO_PARASITES );
        }
    } else if( new_temperature < temperatures::cold ) {
        set_flag( flag_COLD );
    }

    // Convert water into clean water if it starts boiling
    if( typeId() == itype_water && new_temperature > temperatures::boiling ) {
        convert( itype_water_clean ).poison = 0;
    }
}

float item::get_item_thermal_energy() const
{
    const float mass = to_gram( weight() ); // g
    return units::to_joule_per_gram( specific_energy ) * mass;
}

void item::heat_up()
{
    unset_flag( flag_COLD );
    unset_flag( flag_FROZEN );
    unset_flag( flag_SHREDDED );
    set_flag( flag_HOT );
    current_phase = type->phase;
    // Set item temperature to 60 C (333.15 K, 122 F)
    // Also set the energy to match
    temperature = units::from_celsius( 60 );
    specific_energy = get_specific_energy_from_temperature( units::from_celsius( 60 ) );

    reset_temp_check();
}

void item::cold_up()
{
    unset_flag( flag_HOT );
    unset_flag( flag_FROZEN );
    unset_flag( flag_SHREDDED );
    set_flag( flag_COLD );
    current_phase = type->phase;
    // Set item temperature to 3 C (276.15 K, 37.4 F)
    // Also set the energy to match
    temperature = units::from_celsius( 3 );
    specific_energy = get_specific_energy_from_temperature( units::from_celsius( 3 ) );

    reset_temp_check();
}

void item::reset_temp_check()
{
    last_temp_check = calendar::turn;
}

void item::overwrite_relic( const relic &nrelic )
{
    this->relic_data = cata::make_value<relic>( nrelic );
}

bool item::use_relic( Character &guy, const tripoint_bub_ms &pos )
{
    return relic_data->activate( guy, pos );
}

void item::process_relic( Character *carrier, const tripoint_bub_ms &pos )
{
    if( !is_relic() ) {
        return;
    }

    relic_data->try_recharge( *this, carrier, pos );
}

bool item::process_corpse( map &here, Character *carrier, const tripoint_bub_ms &pos )
{
    // some corpses rez over time
    if( corpse == nullptr || damage() >= max_damage() ) {
        return false;
    }

    if( corpse->has_flag( mon_flag_DORMANT ) ) {
        //if dormant, ensure trap still exists.
        const trap *trap_here = &here.tr_at( pos );
        if( trap_here->is_null() ) {
            // if there isn't a trap, we need to add one again.
            here.trap_set( pos, trap_id( "tr_dormant_corpse" ) );
        } else if( trap_here->loadid != trap_id( "tr_dormant_corpse" ) ) {
            // if there is a trap, but it isn't the right one, we need to revive the zombie manually.
            return g->revive_corpse( pos, *this, 3 );
        }
        return false;
    }

    // handle human corpses rising as zombies
    if( corpse->id == mtype_id::NULL_ID() && !has_var( "zombie_form" ) &&
        get_option<bool>( "ZOMBIFY_INTO_ENABLED" ) &&
        !mon_human->zombify_into.is_empty() ) {
        set_var( "zombie_form", mon_human->zombify_into.c_str() );
    }

    if( !ready_to_revive( here, pos ) ) {
        return false;
    }
    if( rng( 0, volume() / 250_ml ) > burnt &&
        g->revive_corpse( pos, *this ) ) {
        if( carrier == nullptr ) {
            if( corpse->in_species( species_ROBOT ) ) {
                add_msg_if_player_sees( pos, m_warning, _( "A nearby robot has repaired itself and stands up!" ) );
            } else {
                add_msg_if_player_sees( pos, m_warning, _( "A nearby corpse rises and moves towards you!" ) );
            }
        } else {
            if( corpse->in_species( species_ROBOT ) ) {
                carrier->add_msg_if_player( m_warning,
                                            _( "Oh dear god, a robot you're carrying has started moving!" ) );
            } else {
                carrier->add_msg_if_player( m_warning,
                                            _( "Oh dear god, a corpse you're carrying has started moving!" ) );
            }
        }
        // Destroy this corpse item
        return true;
    }

    return false;
}

bool item::process_fake_mill( map &here, Character * /*carrier*/, const tripoint_bub_ms &pos )
{
    const furn_id &f = here.furn( pos );
    if( f != furn_f_wind_mill_active &&
        f != furn_f_water_mill_active ) {
        item_counter = 0;
        return true; //destroy fake mill
    }
    if( age() >= 6_hours || item_counter == 0 ) {
        iexamine::mill_finalize( get_avatar(), pos ); //activate effects when timers goes to zero
        return true; //destroy fake mill item
    }

    return false;
}

bool item::process_fake_smoke( map &here, Character * /*carrier*/, const tripoint_bub_ms &pos )
{
    const furn_id &f = here.furn( pos );
    if( f != furn_f_smoking_rack_active &&
        f != furn_f_metal_smoking_rack_active ) {
        item_counter = 0;
        return true; //destroy fake smoke
    }

    if( age() >= 6_hours || item_counter == 0 ) {
        iexamine::on_smoke_out( pos, birthday() ); //activate effects when timers goes to zero
        return true; //destroy fake smoke when it 'burns out'
    }

    return false;
}

bool item::process_litcig( map &here, Character *carrier, const tripoint_bub_ms &pos )
{
    // cig dies out
    if( item_counter == 0 ) {
        if( carrier != nullptr ) {
            carrier->add_msg_if_player( m_neutral, _( "You finish your %s." ), type_name() );
        }
        if( type->revert_to ) {
            convert( *type->revert_to, carrier );
        } else {
            type->invoke( carrier, *this, pos, "transform" );
        }
        if( typeId() == itype_joint_lit && carrier != nullptr ) {
            carrier->add_effect( effect_weed_high, 1_minutes ); // one last puff
            here.add_field( pos + point( rng( -1, 1 ), rng( -1, 1 ) ), field_type_id( "fd_weedsmoke" ), 2 );
            weed_msg( *carrier );
        }
        active = false;
        return false;
    }

    if( carrier != nullptr ) {
        // No lit cigs in inventory, only in hands or in mouth
        // So if we're taking cig off or unwielding it, extinguish it first
        if( !carrier->is_worn( *this ) && !carrier->is_wielding( *this ) ) {
            if( type->revert_to ) {
                carrier->add_msg_if_player( m_neutral, _( "You extinguish your %s and put it away." ),
                                            type_name() );
                convert( *type->revert_to, carrier );
            } else {
                type->invoke( carrier, *this, pos, "transform" );
            }
            active = false;
            return false;
        }
    }

    if( !one_in( 10 ) ) {
        return false;
    }
    process_extinguish( here, carrier, pos );
    // process_extinguish might have extinguished the item already
    if( !active ) {
        return false;
    }
    // if carried by someone:
    if( carrier != nullptr ) {
        time_duration duration = 15_seconds;
        if( carrier->has_trait( trait_TOLERANCE ) ) {
            duration = 7_seconds;
        } else if( carrier->has_trait( trait_LIGHTWEIGHT ) ) {
            duration = 30_seconds;
        }
        carrier->add_msg_if_player( m_neutral, _( "You take a puff of your %s." ), type_name() );
        if( has_flag( flag_TOBACCO ) ) {
            carrier->add_effect( effect_cig, duration );
        } else {
            carrier->add_effect( effect_weed_high, duration / 2 );
        }
        carrier->mod_moves( -to_moves<int>( 1_seconds ) * 0.15 );

        if( ( carrier->has_effect( effect_shakes ) && one_in( 10 ) ) ||
            ( carrier->has_trait( trait_JITTERY ) && one_in( 200 ) ) ) {
            carrier->add_msg_if_player( m_bad, _( "Your shaking hand causes you to drop your %s." ),
                                        type_name() );
            here.add_item_or_charges( pos + point( rng( -1, 1 ), rng( -1, 1 ) ), *this );
            return true; // removes the item that has just been added to the map
        }

        if( carrier->has_effect( effect_sleep ) ) {
            carrier->add_msg_if_player( m_bad, _( "You fall asleep and drop your %s." ),
                                        type_name() );
            here.add_item_or_charges( pos + point( rng( -1, 1 ), rng( -1, 1 ) ), *this );
            return true; // removes the item that has just been added to the map
        }
    } else {
        // If not carried by someone, but laying on the ground:
        if( item_counter % 5 == 0 ) {
            // lit cigarette can start fires
            for( const item &i : here.i_at( pos ) ) {
                if( i.typeId() == typeId() ) {
                    if( here.has_flag( ter_furn_flag::TFLAG_FLAMMABLE, pos ) ||
                        here.has_flag( ter_furn_flag::TFLAG_FLAMMABLE_ASH, pos ) ) {
                        here.add_field( pos, fd_fire, 1 );
                        break;
                    }
                } else if( i.flammable( 0 ) ) {
                    here.add_field( pos, fd_fire, 1 );
                    break;
                }
            }
        }
    }

    // Item remains
    return false;
}

bool item::process_extinguish( map &here, Character *carrier, const tripoint_bub_ms &pos )
{
    // checks for water
    bool extinguish = false;
    bool in_inv = carrier != nullptr && carrier->has_item( *this );
    bool submerged = false;
    bool precipitation = false;
    bool windtoostrong = false;
    bool in_veh = carrier != nullptr && carrier->in_vehicle;
    w_point weatherPoint = *get_weather().weather_precise;
    int windpower = get_weather().windspeed;
    switch( get_weather().weather_id->precip ) {
        case precip_class::very_light:
            precipitation = one_in( 100 );
            break;
        case precip_class::light:
            precipitation = one_in( 50 );
            break;
        case precip_class::heavy:
            precipitation = one_in( 10 );
            break;
        default:
            break;
    }
    if( in_inv && !in_veh && here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, pos ) ) {
        extinguish = true;
        submerged = true;
    }
    if( ( !in_inv && here.has_flag( ter_furn_flag::TFLAG_LIQUID, pos ) && !here.veh_at( pos ) ) ||
        ( precipitation && !g->is_sheltered( pos ) ) ) {
        extinguish = true;
    }
    if( in_inv && windpower > 5 && !g->is_sheltered( pos ) &&
        this->has_flag( flag_WIND_EXTINGUISH ) ) {
        windtoostrong = true;
        extinguish = true;
    }
    if( !extinguish ||
        ( in_inv && precipitation && carrier->get_wielded_item() &&
          carrier->get_wielded_item()->has_flag( flag_RAIN_PROTECT ) ) ) {
        return false; //nothing happens
    }
    if( carrier != nullptr ) {
        if( submerged ) {
            carrier->add_msg_if_player( m_neutral, _( "Your %s is quenched by water." ), tname() );
        } else if( precipitation ) {
            carrier->add_msg_if_player( m_neutral, _( "Your %s is quenched by precipitation." ),
                                        tname() );
        } else if( windtoostrong ) {
            carrier->add_msg_if_player( m_neutral, _( "Your %s is blown out by the wind." ),
                                        tname() );
        }
    }

    if( type->revert_to ) {
        convert( *type->revert_to, carrier );
    } else {
        type->invoke( carrier, *this, pos, "transform" );
    }
    active = false;
    // Item remains
    return false;
}

item::link_data &item::link()
{
    if( link_ ) {
        return *link_;
    }
    if( !can_link_up() ) {
        debugmsg( "%s is creating link_data even though it doesn't have a link_up action!  can_link_up() should be checked before using link().",
                  tname() );
    }
    return *( link_ = cata::make_value<item::link_data>() );
}

const item::link_data &item::link() const
{
    if( link_ ) {
        return *link_;
    }
    if( !can_link_up() ) {
        debugmsg( "%s is creating link_data even though it doesn't have a link_up action!  can_link_up() should be checked before using link().",
                  tname() );
    }
    return *( link_ = cata::make_value<item::link_data>() );
}

bool item::has_link_data() const
{
    return !!link_;
}

bool item::can_link_up() const
{
    return has_link_data() || type->can_use( "link_up" );
}

bool item::link_has_state( link_state state ) const
{
    return !has_link_data() ? state == link_state::no_link :
           link_->source == state || link_->target == state;
}

bool item::link_has_states( link_state source, link_state target ) const
{
    return !has_link_data() ? source == link_state::no_link && target == link_state::no_link :
           ( link_->source == source || link_->source == link_state::automatic ) &&
           ( link_->target == target || link_->target == link_state::automatic );
}

bool item::has_no_links() const
{
    return !has_link_data() ? true :
           link_->source == link_state::no_link && link_->target == link_state::no_link;
}

std::string item::link_name() const
{
    return has_flag( flag_CABLE_SPOOL ) ? label( 1 ) : string_format( _( " %s's cable" ), label( 1 ) );
}

ret_val<void> item::link_to( const optional_vpart_position &linked_vp, link_state link_type )
{
    return linked_vp ? link_to( linked_vp->vehicle(), linked_vp->mount_pos(), link_type ) :
           ret_val<void>::make_failure();
}

ret_val<void> item::link_to( const optional_vpart_position &first_linked_vp,
                             const optional_vpart_position &second_linked_vp, link_state link_type )
{
    if( !first_linked_vp || !second_linked_vp ) {
        return ret_val<void>::make_failure();
    }
    // Link up the second vehicle first so, if it's a tow cable, the first vehicle will tow the second.
    ret_val<void> result = link_to( second_linked_vp->vehicle(), second_linked_vp->mount_pos(),
                                    link_type );
    if( !result.success() ) {
        return result;
    }
    return link_to( first_linked_vp->vehicle(), first_linked_vp->mount_pos(), link_type );
}

ret_val<void> item::link_to( vehicle &veh, const point_rel_ms &mount, link_state link_type )
{
    map &here = get_map();

    if( !can_link_up() ) {
        return ret_val<void>::make_failure( _( "The %s doesn't have a cable!" ), type_name() );
    }

    // First, check if the desired link is actually possible.

    if( link_type == link_state::vehicle_tow ) {
        if( veh.has_tow_attached() || veh.is_towed() || veh.is_towing() ) {
            return ret_val<void>::make_failure( _( "That vehicle already has a tow-line attached." ) );
        } else if( !veh.is_external_part( mount ) ) {
            return ret_val<void>::make_failure( _( "You can't attach a tow-line to an internal part." ) );
        } else {
            const int part_at = veh.part_at( veh.coord_translate( mount ) );
            if( part_at != -1 && !veh.part( part_at ).carried_stack.empty() ) {
                return ret_val<void>::make_failure( _( "You can't attach a tow-line to a racked part." ) );
            }
        }
    } else {
        const link_up_actor *it_actor = static_cast<const link_up_actor *>
                                        ( get_use( "link_up" )->get_actor_ptr() );
        bool can_link_port = ( link_type == link_state::vehicle_port ||
                               link_type == link_state::automatic ) &&
                             it_actor->targets.find( link_state::vehicle_port ) != it_actor->targets.end() &&
                             veh.avail_linkable_part( mount, true ) != -1;

        bool can_link_battery = ( link_type == link_state::vehicle_battery ||
                                  link_type == link_state::automatic ) &&
                                it_actor->targets.find( link_state::vehicle_battery ) != it_actor->targets.end() &&
                                veh.avail_linkable_part( mount, false ) != -1;

        if( !can_link_port && !can_link_battery ) {
            return ret_val<void>::make_failure( _( "The %s can't connect to that." ), type_name() );
        } else if( link_type == link_state::automatic ) {
            link_type = can_link_port ? link_state::vehicle_port : link_state::vehicle_battery;
        }
    }

    bool bio_link = link_has_states( link_state::no_link, link_state::bio_cable );
    if( bio_link || has_no_links() ) {

        // No link exists, so start a new link to a vehicle/appliance.

        link().source = bio_link ? link_state::bio_cable : link_state::no_link;
        link().target = link_type;
        link().t_veh = veh.get_safe_reference();
        link().t_abs_pos = link().t_veh->pos_abs();
        link().t_mount = mount;
        link().s_bub_pos = tripoint_bub_ms::min; // Forces the item to check the length during process_link.

        update_link_traits();
        return ret_val<void>::make_success();
    }

    // There's already a link, so connect the previous vehicle/appliance to the new one.

    if( !link_has_state( link_state::no_link ) ) {
        debugmsg( "Failed to connect the %s, both ends are already connected!", tname() );
        return ret_val<void>::make_failure();
    }
    if( !link().t_veh ) {
        vehicle *found_veh = vehicle::find_vehicle( here, link().t_abs_pos );
        if( found_veh ) {
            link().t_veh = found_veh->get_safe_reference();
        } else {
            debugmsg( "Failed to connect the %s, it lost its vehicle pointer!", tname() );
            return ret_val<void>::make_failure();
        }
    }
    vehicle *prev_veh = link().t_veh.get();
    if( prev_veh == &veh ) {
        return ret_val<void>::make_failure( _( "You cannot connect the %s to itself." ), prev_veh->name );
    }

    // Prepare target tripoints for the cable parts that'll be added to the selected/previous vehicles
    const std::pair<tripoint_abs_ms, tripoint_abs_ms> prev_part_target = std::make_pair(
                veh.pos_abs() + veh.coord_translate( mount ),
                veh.pos_abs() );
    const std::pair<tripoint_abs_ms, tripoint_abs_ms> sel_part_target = std::make_pair(
                link().t_abs_pos + prev_veh->coord_translate( link().t_mount ),
                link().t_abs_pos );

    for( const vpart_reference &vpr : prev_veh->get_any_parts( VPFLAG_POWER_TRANSFER ) ) {
        if( vpr.part().target.first == prev_part_target.first &&
            vpr.part().target.second == prev_part_target.second ) {
            return ret_val<void>::make_failure( _( "The %1$s and %2$s are already connected." ),
                                                veh.name, prev_veh->name );
        }
    }

    if( rl_dist( prev_part_target.first, sel_part_target.first ) > link().max_length ) {
        return ret_val<void>::make_failure( _( "The %s can't stretch that far!" ), type_name() );
    }

    const itype_id item_id = typeId();
    vpart_id vpid = vpart_id::NULL_ID();
    for( const vpart_info &e : vehicles::parts::get_all() ) {
        if( e.base_item == item_id ) {
            vpid = e.id;
            break;
        }
    }

    if( vpid.is_null() ) {
        debugmsg( "item %s is not base item of any vehicle part!", item_id.c_str() );
        return ret_val<void>::make_failure();
    }

    const point_rel_ms prev_mount = link().t_mount;

    const ret_val<void> can_mount1 = prev_veh->can_mount( prev_mount, *vpid );
    if( !can_mount1.success() ) {
        //~ %1$s - cable name, %2$s - the reason why it failed
        return ret_val<void>::make_failure( _( "You can't attach the %1$s: %2$s" ),
                                            type_name(), can_mount1.str() );
    }
    const ret_val<void> can_mount2 = veh.can_mount( mount, *vpid );
    if( !can_mount2.success() ) {
        //~ %1$s - cable name, %2$s - the reason why it failed
        return ret_val<void>::make_failure( _( "You can't attach the %1$s: %2$s" ),
                                            type_name(), can_mount2.str() );
    }

    vehicle_part prev_veh_part( vpid, item( *this ) );
    prev_veh_part.target.first = prev_part_target.first;
    prev_veh_part.target.second = prev_part_target.second;
    prev_veh->install_part( here, prev_mount, std::move( prev_veh_part ) );
    prev_veh->precalc_mounts( 1, prev_veh->pivot_rotation[1], prev_veh->pivot_anchor[1] );

    vehicle_part sel_veh_part( vpid, item( *this ) );
    sel_veh_part.target.first = sel_part_target.first;
    sel_veh_part.target.second = sel_part_target.second;
    veh.install_part( here, mount, std::move( sel_veh_part ) );
    veh.precalc_mounts( 1, veh.pivot_rotation[1], veh.pivot_anchor[1] );

    if( link_type == link_state::vehicle_tow ) {
        if( link().source == link_state::vehicle_tow ) {
            veh.tow_data.set_towing( prev_veh, &veh ); // Previous vehicle is towing.
        } else {
            veh.tow_data.set_towing( &veh, prev_veh ); // Previous vehicle is being towed.
        }
    }

    if( typeId() != itype_power_cord ) {
        // Remove linked_flag from attached parts - the just-added cable vehicle parts do the same thing.
        reset_link();
    }
    //~ %1$s - first vehicle name, %2$s - second vehicle name - %3$s - cable name,
    return ret_val<void>::make_success( _( "You connect the %1$s and %2$s with the %3$s." ),
                                        prev_veh->disp_name(), veh.disp_name(), type_name() );
}

void item::update_link_traits()
{
    if( !can_link_up() ) {
        return;
    }

    const link_up_actor *it_actor = static_cast<const link_up_actor *>
                                    ( get_use( "link_up" )->get_actor_ptr() );
    link().max_length = it_actor->cable_length == -1 ? type->maximum_charges() : it_actor->cable_length;
    link().efficiency = it_actor->efficiency < MIN_LINK_EFFICIENCY ? 0.0f : it_actor->efficiency;
    // Reset s_bub_pos to force the item to check the length during process_link.
    link().s_bub_pos = tripoint_bub_ms::min;
    link().last_processed = calendar::turn;

    for( const item *cable : cables() ) {
        if( !cable->can_link_up() ) {
            continue;
        }
        const link_up_actor *actor = static_cast<const link_up_actor *>
                                     ( cable->get_use( "link_up" )->get_actor_ptr() );
        link().max_length += actor->cable_length == -1 ? cable->type->maximum_charges() :
                             actor->cable_length;
        link().efficiency = link().efficiency < MIN_LINK_EFFICIENCY ? 0.0f :
                            link().efficiency * actor->efficiency;
    }

    if( link().efficiency >= MIN_LINK_EFFICIENCY ) {
        link().charge_rate = it_actor->charge_rate.value();
        // Convert charge_rate to how long it takes to charge 1 kW, the unit batteries use.
        // 0 means batteries won't be charged, but it can still provide power to devices unless efficiency is also 0.
        if( it_actor->charge_rate != 0_W ) {
            double charge_rate = std::floor( 1000000.0 / abs( it_actor->charge_rate.value() ) + 0.5 );
            link().charge_interval = std::max( 1, static_cast<int>( charge_rate ) );
        }
    }
}

int item::link_length() const
{
    return has_no_links() ? -2 :
           link_has_state( link_state::needs_reeling ) ? -1 : link().length;
}

int item::max_link_length() const
{
    return !has_link_data() ? -2 :
           link().max_length != -1 ? link().max_length : type->maximum_charges();
}

int item::link_sort_key() const
{
    const int length = link_length();
    int key = length >= 0 ? -1000000000 : length == -1 ? 0 : 1000000000;
    key += max_link_length() * 100000;
    return key - length;
}

bool item::process_link( map &here, Character *carrier, const tripoint_bub_ms &pos )
{
    if( link_length() < 0 ) {
        return false;
    }

    const bool is_cable_item = has_flag( flag_CABLE_SPOOL );

    // Handle links to items in the inventory.
    if( link().source == link_state::solarpack ) {
        if( carrier == nullptr || !carrier->worn_with_flag( flag_SOLARPACK_ON ) ) {
            add_msg_if_player_sees( pos, m_bad, _( "The %s has come loose from the solar pack." ),
                                    link_name() );
            reset_link( true, carrier );
            return false;
        }
    }
    const item_filter used_ups = [&]( const item & itm ) {
        return itm.get_var( "cable" ) == "plugged_in";
    };
    if( link().source == link_state::ups ) {
        if( carrier == nullptr || !carrier->cache_has_item_with( flag_IS_UPS, used_ups ) ) {
            add_msg_if_player_sees( pos, m_bad, _( "The %s has come loose from the UPS." ), link_name() );
            reset_link( true, carrier );
            return false;
        }
    }
    // Certain cable states should skip processing and also become inactive if dropped.
    if( ( link().target == link_state::no_link && link().source != link_state::vehicle_tow ) ||
        link().target == link_state::bio_cable ) {
        if( carrier == nullptr ) {
            return reset_link( true, nullptr, -1, true, pos );
        }
        return false;
    }

    const bool last_t_abs_pos_is_oob = !here.inbounds( link().t_abs_pos );

    // Lambda function for checking if a cable has been stretched too long, resetting it if so.
    // @return True if the cable is disconnected.
    const auto check_length = [this, carrier, pos, last_t_abs_pos_is_oob, is_cable_item]() {
        if( debug_mode ) {
            add_msg_debug( debugmode::DF_IUSE, "%s linked to %s%s, length %d/%d", type_name(),
                           link().t_abs_pos.to_string_writable(), last_t_abs_pos_is_oob ? " (OoB)" : "",
                           link().length, link().max_length );
        }
        if( link().length > link().max_length ) {
            std::string cable_name = is_cable_item ? string_format( _( "over-extended %s" ), label( 1 ) ) :
                                     string_format( _( "%s's over-extended cable" ), label( 1 ) );
            if( carrier != nullptr ) {
                carrier->add_msg_if_player( m_bad, _( "Your %s breaks loose!" ), cable_name );
            } else {
                add_msg_if_player_sees( pos, m_bad, _( "Your %s breaks loose!" ), cable_name );
            }
            return true;
        } else if( link().length + M_SQRT2 >= link().max_length + 1 && carrier != nullptr ) {
            carrier->add_msg_if_player( m_warning, _( "Your %s is stretched to its limit!" ), link_name() );
        }
        return false;
    };

    // Check if the item has moved positions this turn.
    bool length_check_needed = false;
    if( link().s_bub_pos != pos ) {
        link().s_bub_pos = pos;
        length_check_needed = true;
    }

    // Re-establish vehicle pointer if it got lost or if this item just got loaded.
    if( !link().t_veh ) {
        vehicle *found_veh = vehicle::find_vehicle( here, link().t_abs_pos );
        if( !found_veh ) {
            return reset_link( true, carrier, -2, true, pos );
        }
        if( debug_mode ) {
            add_msg_debug( debugmode::DF_IUSE, "Re-established link of %s to %s.", type_name(),
                           found_veh->disp_name() );
        }
        link().t_veh = found_veh->get_safe_reference();
    }

    // Regular pointers are faster, so make one now that we know the reference is valid.
    vehicle *t_veh = link().t_veh.get();

    // We should skip processing if the last saved target point is out of bounds, since vehicles give innacurate absolute coordinates when out of bounds.
    // However, if the linked vehicle is moving fast enough, we should always do processing to avoid erroneously skipping linked items riding inside of it.
    if( last_t_abs_pos_is_oob && t_veh->velocity < HALF_MAPSIZE_X * 400 ) {
        if( !length_check_needed ) {
            return false;
        }
        link().length = rl_dist( here.get_abs( pos ), link().t_abs_pos ) +
                        link().t_mount.abs().x() + link().t_mount.abs().y();
        if( check_length() ) {
            return reset_link( true, carrier );
        }
        return false;
    }

    // Find the vp_part index the cable is linked to.
    int link_vp_index = -1;
    if( link().target == link_state::vehicle_port ) {
        for( int idx : t_veh->cable_ports ) {
            if( t_veh->part( idx ).mount == link().t_mount ) {
                link_vp_index = idx;
                break;
            }
        }
    } else if( link().target == link_state::vehicle_battery ) {
        for( int idx : t_veh->batteries ) {
            if( t_veh->part( idx ).mount == link().t_mount ) {
                link_vp_index = idx;
                break;
            }
        }
        if( link_vp_index == -1 ) {
            // Check cable_ports, since that includes appliances
            for( int idx : t_veh->cable_ports ) {
                if( t_veh->part( idx ).mount == link().t_mount ) {
                    link_vp_index = idx;
                    break;
                }
            }
        }
    } else if( link().target == link_state::vehicle_tow || link().source == link_state::vehicle_tow ) {
        link_vp_index = t_veh->part_at( t_veh->coord_translate( link().t_mount ) );
    }
    if( link_vp_index == -1 ) {
        // The part with cable ports was lost, so disconnect the cable.
        return reset_link( true, carrier, -2, true, pos );
    }

    if( link().last_processed <= t_veh->part( link_vp_index ).last_disconnected ) {
        add_msg_if_player_sees( pos, m_warning, string_format( _( "You detached the %s." ),
                                type_name() ) );
        return reset_link( true, carrier, -2 );
    }
    t_veh->part( link_vp_index ).set_flag( vp_flag::linked_flag );

    int turns_elapsed = to_turns<int>( calendar::turn - link().last_processed );
    link().last_processed = calendar::turn;

    // Set the new absolute position to the vehicle's origin.
    tripoint_abs_ms new_t_abs_pos = t_veh->pos_abs();;
    tripoint_bub_ms t_veh_bub_pos = here.get_bub( new_t_abs_pos );;
    if( link().t_abs_pos != new_t_abs_pos ) {
        link().t_abs_pos = new_t_abs_pos;
        length_check_needed = true;
    }

    // If either of the link's connected sides moved, check the cable's length.
    if( length_check_needed ) {
        link().length = rl_dist( pos, t_veh_bub_pos + t_veh->part( link_vp_index ).precalc[0].raw() );
        if( check_length() ) {
            return reset_link( true, carrier, link_vp_index );
        }
    }

    // Extra behaviors for the cabled item.
    if( !is_cable_item ) {
        int power_draw = 0;

        if( link().efficiency < MIN_LINK_EFFICIENCY ) {
            return false;
        }
        // Recharge or charge linked batteries
        power_draw -= charge_linked_batteries( *t_veh, turns_elapsed );

        // Tool power draw display
        if( active && type->tool && type->tool->power_draw > 0_W ) {
            power_draw -= type->tool->power_draw.value();
        }

        // Send total power draw to the vehicle so it can be displayed.
        if( power_draw != 0 ) {
            t_veh->linked_item_epower_this_turn += units::from_milliwatt( power_draw );
        }
    } else if( has_flag( flag_NO_DROP ) ) {
        debugmsg( "%s shouldn't exist outside of an item or vehicle part.", tname() );
        return true;
    }

    return false;
}

int item::charge_linked_batteries( vehicle &linked_veh, int turns_elapsed )
{
    map &here = get_map();

    if( !has_link_data() || link().charge_rate == 0 || link().charge_interval < 1 ) {
        return 0;
    }

    if( !is_battery() ) {
        const item *parent_mag = magazine_current();
        if( !parent_mag || !parent_mag->has_flag( flag_RECHARGE ) ) {
            return 0;
        }
    }

    const bool power_in = link().charge_rate > 0;
    if( power_in ? ammo_remaining( ) >= ammo_capacity( ammo_battery ) :
        ammo_remaining( ) <= 0 ) {
        return 0;
    }

    // Normally efficiency is the chance to get a charge every charge_interval, but if we're catching up from
    // time spent ouside the reality bubble it should be applied as a percentage of the total instead.
    bool short_time_passed = turns_elapsed <= link().charge_interval;

    if( turns_elapsed < 1 || ( short_time_passed &&
                               !calendar::once_every( time_duration::from_turns( link().charge_interval ) ) ) ) {
        return link().charge_rate;
    }

    // If a long time passed, multiply the total by the efficiency rather than cancelling a charge.
    int transfer_total = short_time_passed ? 1 :
                         ( turns_elapsed * 1.0f / link().charge_interval ) * link().charge_interval;

    if( power_in ) {
        const int battery_deficit = linked_veh.discharge_battery( here, transfer_total, true );
        // Around 85% efficient by default; a few of the discharges don't actually recharge
        if( battery_deficit == 0 && ( !short_time_passed || rng_float( 0.0, 1.0 ) <= link().efficiency ) ) {
            ammo_set( itype_battery, ammo_remaining( ) + transfer_total );
        }
    } else {
        // Around 85% efficient by default; a few of the discharges don't actually charge
        if( !short_time_passed || rng_float( 0.0, 1.0 ) <= link().efficiency ) {
            const int battery_surplus = linked_veh.charge_battery( here, transfer_total, true );
            if( battery_surplus == 0 ) {
                ammo_set( itype_battery, ammo_remaining( ) - transfer_total );
            }
        } else {
            const std::pair<int, int> linked_levels = linked_veh.connected_battery_power_level( here );
            if( linked_levels.first < linked_levels.second ) {
                ammo_set( itype_battery, ammo_remaining( ) - transfer_total );
            }
        }
    }
    return link().charge_rate;
}

bool item::reset_link( bool unspool_if_too_long, Character *p, int vpart_index,
                       const bool loose_message, const tripoint_bub_ms cable_position )
{
    if( !has_link_data() ) {
        return has_flag( flag_NO_DROP );
    }
    if( unspool_if_too_long && link_has_state( link_state::needs_reeling ) ) {
        // Cables that need reeling should be reset with a reel_cable_activity_actor instead.
        return false;
    }

    if( vpart_index != -2 && link().t_veh ) {
        if( vpart_index == -1 ) {
            vehicle *t_veh = link().t_veh.get();
            // Find the vp_part index the cable is linked to.
            if( link().target == link_state::vehicle_port ) {
                for( int idx : t_veh->cable_ports ) {
                    if( t_veh->part( idx ).mount == link().t_mount ) {
                        vpart_index = idx;
                        break;
                    }
                }
            } else if( link().target == link_state::vehicle_battery ) {
                for( int idx : t_veh->batteries ) {
                    if( t_veh->part( idx ).mount == link().t_mount ) {
                        vpart_index = idx;
                        break;
                    }
                }
            } else if( link().target == link_state::vehicle_tow || link().source == link_state::vehicle_tow ) {
                vpart_index = t_veh->part_at( t_veh->coord_translate( link().t_mount ) );
            }
        }
        if( vpart_index != -1 ) {
            link().t_veh->part( vpart_index ).remove_flag( vp_flag::linked_flag );
        }
    }

    if( loose_message ) {
        if( p != nullptr ) {
            p->add_msg_if_player( m_warning, _( "Your %s has come loose." ), link_name() );
        } else {
            add_msg_if_player_sees( cable_position, m_warning, _( "The %s has come loose." ),
                                    link_name() );
        }
    }

    if( unspool_if_too_long && link().length > LINK_RESPOOL_THRESHOLD ) {
        // Cables that are too long need to be manually rewound before reuse.
        link().source = link_state::needs_reeling;
        return false;
    }

    if( loose_message && p ) {
        p->add_msg_if_player( m_info, _( "You reel in the %s and wind it up." ), link_name() );
    }

    link_.reset();
    if( !cables().empty() ) {
        // If there are extensions, keep link active to maintain max_length.
        update_link_traits();
    }
    return has_flag( flag_NO_DROP );
}

bool item::process_linked_item( Character *carrier, const tripoint_bub_ms & /*pos*/,
                                const link_state required_state )
{
    if( carrier == nullptr ) {
        erase_var( "cable" );
        active = false;
        return false;
    }
    bool has_connected_cable = carrier->cache_has_item_with( "can_link_up", &item::can_link_up,
    [&required_state]( const item & it ) {
        return it.link_has_state( required_state );
    } );
    if( !has_connected_cable ) {
        erase_var( "cable" );
        active = false;
    }
    return false;
}

bool item::process_wet( Character *carrier, const tripoint_bub_ms & /*pos*/ )
{
    if( item_counter == 0 ) {
        if( type->revert_to ) {
            convert( *type->revert_to, carrier );
        }
        unset_flag( flag_WET );
        active = false;
    }
    // Always return true so our caller will bail out instead of processing us as a tool.
    return true;
}

units::energy item::energy_per_second() const
{
    units::energy energy_to_burn;
    if( type->tool->turns_per_charge > 0 ) {
        energy_to_burn += units::from_kilojoule( std::max<int64_t>( ammo_required(),
                          1 ) ) / type->tool->turns_per_charge;
    } else if( type->tool->power_draw > 0_mW ) {
        energy_to_burn += type->tool->power_draw * 1_seconds;
    }
    return energy_to_burn;
}

bool item::process_tool( Character *carrier, const tripoint_bub_ms &pos )
{
    map &here = get_map();

    // FIXME: remove this once power armors don't need to be TOOL_ARMOR anymore
    if( is_power_armor() && carrier && carrier->can_interface_armor() && carrier->has_power() ) {
        return false;
    }

    // if insufficient available charges shutdown the tool
    if( ( type->tool->power_draw > 0_W || type->tool->turns_per_charge > 0 ) &&
        ( ( uses_energy() && energy_remaining( carrier ) < energy_per_second() ) ||
          ( !uses_energy() && ammo_remaining_linked( here, carrier ) == 0 ) ) ) {
        if( carrier && has_flag( flag_USE_UPS ) ) {
            carrier->add_msg_if_player( m_info, _( "You need an UPS to run the %s!" ), tname() );
        }
        if( carrier ) {
            carrier->add_msg_if_player( m_info, _( "The %s ran out of energy!" ), tname() );
        }
        if( type->revert_to.has_value() ) {
            deactivate( carrier );
            return false;
        } else {
            return true;
        }
    }

    if( energy_remaining( carrier ) > 0_J ) {
        energy_consume( energy_per_second(), pos, carrier );
    } else {
        // Non-electrical charge consumption.
        int charges_to_use = 0;
        if( type->tool->turns_per_charge > 0 &&
            to_turn<int>( calendar::turn ) % type->tool->turns_per_charge == 0 ) {
            charges_to_use = std::max( ammo_required(), 1 );
        }

        if( charges_to_use > 0 ) {
            ammo_consume( charges_to_use, pos, carrier );
        }
    }

    // FIXME: some iuse functions return 1+ expecting to be destroyed (molotovs), others
    // to use charges, and others just because?
    // allow some items to opt into requesting destruction
    const int charges_used = type->tick( carrier, *this, pos );
    const bool destroy = has_flag( flag_DESTROY_ON_CHARGE_USE );
    if( !destroy && charges_used > 0 ) {
        debugmsg( "Item %s consumes charges via tick_action, but should not", tname() );
    }
    return destroy && charges_used > 0;
}

bool item::process_blackpowder_fouling( Character *carrier )
{
    // Rust is deterministic. At a total modifier of 1 (the max): 12 hours for first rust, then 24 (36 total), then 36 (72 total) and finally 48 (120 hours to go to XX)
    // this speeds up by the amount the gun is dirty, 2-6x as fast depending on dirt level. At minimum dirt, the modifier is 0.3x the speed of the above mentioned figures.
    set_var( "rust_timer", get_var( "rust_timer", 0 ) + std::min( 0.3 + get_var( "dirt", 0 ) / 200,
             1.0 ) );
    double time_mult = 1.0 + ( 4.0 * static_cast<double>( damage() ) ) / static_cast<double>
                       ( max_damage() );
    if( damage() < max_damage() && get_var( "rust_timer", 0 ) > 43200.0 * time_mult ) {
        inc_damage();
        set_var( "rust_timer", 0 );
        if( carrier ) {
            carrier->add_msg_if_player( m_bad, _( "Your %s rusts due to corrosive powder fouling." ), tname() );
        }
    }
    return false;
}

bool item::process_gun_cooling( Character *carrier )
{
    double heat = get_var( "gun_heat", 0 );
    double overheat_modifier = 0;
    float overheat_multiplier = 1.0f;
    double cooling_modifier = 0;
    float cooling_multiplier = 1.0f;
    for( const item *mod : gunmods() ) {
        overheat_modifier += mod->type->gunmod->overheat_threshold_modifier;
        overheat_multiplier *= mod->type->gunmod->overheat_threshold_multiplier;
        cooling_modifier += mod->type->gunmod->cooling_value_modifier;
        cooling_multiplier *= mod->type->gunmod->cooling_value_multiplier;
    }
    double threshold = std::max( ( type->gun->overheat_threshold * overheat_multiplier ) +
                                 overheat_modifier, 5.0 );
    heat -= std::max( ( type->gun->cooling_value * cooling_multiplier ) + cooling_modifier, 0.5 );
    set_var( "gun_heat", std::max( 0.0, heat ) );
    if( has_fault( fault_overheat_safety ) && heat < threshold * 0.2 ) {
        remove_fault( fault_overheat_safety );
        if( carrier ) {
            carrier->add_msg_if_player( m_good, _( "Your %s beeps as its cooling cycle concludes." ), tname() );
        }
    }
    if( carrier ) {
        if( heat > threshold ) {
            carrier->add_msg_if_player( m_bad,
                                        _( "You can feel the struggling systems of your %s beneath the blare of the overheat alarm." ),
                                        tname() );
        } else if( heat > threshold * 0.8 ) {
            carrier->add_msg_if_player( m_bad, _( "Your %s frantically sounds an overheat alarm." ), tname() );
        } else if( heat > threshold * 0.6 ) {
            carrier->add_msg_if_player( m_warning, _( "Your %s sounds a heat warning chime!" ), tname() );
        }
    }
    return false;
}

bool item::process( map &here, Character *carrier, const tripoint_bub_ms &pos, float insulation,
                    temperature_flag flag, float spoil_multiplier_parent, bool watertight_container, bool recursive )
{
    process_relic( carrier, pos );
    if( recursive ) {
        contents.process( here, carrier, pos, type->insulation_factor * insulation, flag,
                          spoil_multiplier_parent, watertight_container );
    }
    return process_internal( here, carrier, pos, insulation, flag, spoil_multiplier_parent,
                             watertight_container );
}

void item::set_last_temp_check( const time_point &pt )
{
    last_temp_check = pt;
}

bool item::process_internal( map &here, Character *carrier, const tripoint_bub_ms &pos,
                             float insulation, const temperature_flag flag, float spoil_modifier, bool watertight_container )
{
    if( ethereal ) {
        diag_value const &eth = get_value( "ethereal" );
        if( eth.is_empty() ) {
            return true;
        }
        double const set = eth.dbl() - 1;
        set_var( "ethereal", set );
        const bool processed = set <= 0;
        if( processed && carrier != nullptr ) {
            carrier->add_msg_if_player( _( "Your %s disappears!" ), tname() );
        }
        return processed;
    }

    // How this works: it checks what kind of processing has to be done
    // (e.g. for food, for drying towels, lit cigars), and if that matches,
    // call the processing function. If that function returns true, the item
    // has been destroyed by the processing, so no further processing has to be
    // done.
    // Otherwise processing continues. This allows items that are processed as
    // food and as litcig and as ...

    if( wetness > 0 ) {
        wetness -= 1;
    }

    if( has_link_data() ) {
        process_link( here, carrier, pos );
    }

    // Remaining stuff is only done for active items.
    if( active ) {

        if( wetness && has_flag( flag_WATER_BREAK ) ) {
            deactivate();
            set_random_fault_of_type( "wet", true );
            if( has_flag( flag_ELECTRONIC ) ) {
                set_random_fault_of_type( "shorted", true );
            }
        }

        if( !is_food() && item_counter > 0 ) {
            item_counter--;
        }

        if( calendar::turn >= countdown_point ) {
            active = false;
            if( type->countdown_action ) {
                type->countdown_action.call( carrier, *this, pos );
            }
            countdown_point = calendar::turn_max;
            if( type->revert_to ) {
                convert( *type->revert_to, carrier );

                active = needs_processing();
            } else {
                return true;
            }
        }

        for( const emit_id &e : type->emits ) {
            here.emit_field( pos, e );
        }

        if( requires_tags_processing ) {
            // `mark` becomes true if any of the flags that require processing are present
            bool mark = false;
            const auto mark_flag = [&mark]() {
                mark = true;
                return true;
            };

            if( has_flag( flag_FAKE_SMOKE ) && mark_flag() && process_fake_smoke( here, carrier, pos ) ) {
                return true;
            }
            if( has_flag( flag_FAKE_MILL ) && mark_flag() && process_fake_mill( here, carrier, pos ) ) {
                return true;
            }
            if( is_corpse() && mark_flag() && process_corpse( here, carrier, pos ) ) {
                return true;
            }
            if( has_flag( flag_WET ) && mark_flag() && process_wet( carrier, pos ) ) {
                // Drying items are never destroyed, but we want to exit so they don't get processed as tools.
                return false;
            }
            if( has_flag( flag_LITCIG ) && mark_flag() && process_litcig( here, carrier, pos ) ) {
                return true;
            }
            if( ( has_flag( flag_WATER_EXTINGUISH ) || has_flag( flag_WIND_EXTINGUISH ) ) &&
                mark_flag() && process_extinguish( here, carrier, pos ) ) {
                return false;
            }
            if( has_flag( flag_CABLE_SPOOL ) && mark_flag() ) {
                // DO NOT process this as a tool! It really isn't!
                // Cables have been reworked and shouldn't be active items anymore.
                active = false;
                return false;
            }
            if( has_flag( flag_IS_UPS ) && mark_flag() ) {
                // DO NOT process this as a tool! It really isn't!
                return process_linked_item( carrier, pos, link_state::ups );
            }
            if( ( has_flag( flag_SOLARPACK ) || has_flag( flag_SOLARPACK_ON ) ) && mark_flag() ) {
                // DO NOT process this as a tool! It really isn't!
                return process_linked_item( carrier, pos, link_state::solarpack );
            }

            if( !mark ) {
                // no flag checks above were successful and no additional processing logic
                // that could've changed flags was executed
                requires_tags_processing = false;
            }
        }

        if( is_tool() ) {
            return process_tool( carrier, pos );
        }
        // All foods that go bad have temperature
        if( has_temperature() &&
            process_temperature_rot( insulation, pos, here, carrier, flag, spoil_modifier,
                                     watertight_container ) ) {
            if( is_comestible() ) {
                here.rotten_item_spawn( *this, pos );
            }
            if( is_corpse() ) {
                here.handle_decayed_corpse( *this, here.get_abs( pos ) );
            }
            return true;
        }
    } else {
        // guns are never active so we only need to tick this on inactive items. For performance reasons.
        if( has_fault_flag( flag_BLACKPOWDER_FOULING_DAMAGE ) ) {
            return process_blackpowder_fouling( carrier );
        }
        if( get_var( "gun_heat", 0 ) > 0 ) {
            return process_gun_cooling( carrier );
        }
        if( has_fault( fault_emp_reboot ) ) {
            if( one_in( 60 ) ) {
                if( !one_in( 20 ) ) {
                    remove_fault( fault_emp_reboot );
                    if( carrier ) {
                        carrier->add_msg_if_player( m_good, _( "Your %s reboots successfully." ), tname() );
                    }
                } else {
                    remove_fault( fault_emp_reboot );
                    set_random_fault_of_type( "shorted", true );
                    if( carrier ) {
                        carrier->add_msg_if_player( m_bad, _( "Your %s fails to reboot properly." ), tname() );
                    }
                }
            }
            return false;
        }
    }
    return false;
}

void item::mod_charges( int mod )
{
    if( has_infinite_charges() ) {
        return;
    }

    if( !count_by_charges() ) {
        debugmsg( "Tried to remove %s by charges, but item is not counted by charges.", tname() );
    } else if( mod < 0 && charges + mod < 0 ) {
        debugmsg( "Tried to remove charges that do not exist, removing maximum available charges instead." );
        charges = 0;
    } else if( mod > 0 && charges >= INFINITE_CHARGES - mod ) {
        charges = INFINITE_CHARGES - 1; // Highly unlikely, but finite charges should not become infinite.
    } else {
        charges += mod;
    }
}

bool item::is_seed() const
{
    return !!type->seed;
}

std::string item::get_plant_name() const
{
    if( !type->seed ) {
        return std::string{};
    }
    return type->seed->plant_name.translated();
}

bool item::is_dangerous() const
{
    if( has_flag( flag_DANGEROUS ) ) {
        return true;
    }

    // Note: Item should be dangerous regardless of what type of a container is it
    // Visitable interface would skip some options
    for( const item *it : contents.all_items_top() ) {
        if( it->is_dangerous() ) {
            return true;
        }
    }
    return false;
}

bool item::is_tainted() const
{
    return corpse && corpse->has_flag( mon_flag_POISON );
}

bool item::is_reloadable() const
{
    if( ( has_flag( flag_NO_RELOAD ) && !has_flag( flag_VEHICLE ) ) ||
        ( is_gun() && !ammo_default() ) ) {
        // turrets ignore NO_RELOAD flag
        // don't show guns without default ammo defined in reload ui
        return false;
    }

    for( const item_pocket *pocket : contents.get_all_reloadable_pockets() ) {
        if( pocket->is_type( pocket_type::MAGAZINE_WELL ) ) {
            if( pocket->empty() || !pocket->front().is_magazine_full() ) {
                return true;
            }
        } else if( pocket->is_type( pocket_type::MAGAZINE ) ) {
            if( remaining_ammo_capacity() > 0 ) {
                return true;
            }
        } else if( pocket->is_type( pocket_type::CONTAINER ) ) {
            // Container pockets are reloadable only if they are watertight, not full and do not contain non-liquid item
            if( pocket->full( false ) || !pocket->watertight() ) {
                continue;
            }
            if( pocket->empty() || pocket->front().made_of( phase_id::LIQUID ) ) {
                return true;
            }
        }
    }

    for( const item *gunmod : gunmods() ) {
        if( gunmod->is_reloadable() ) {
            return true;
        }
    }

    return false;
}

std::string item::type_name( unsigned int quantity, bool use_variant, bool use_cond_name,
                             bool use_corpse ) const
{
    const auto iter = item_vars.find( "name" );
    std::string ret_name;
    if( typeId() == itype_blood ) {
        if( corpse == nullptr || corpse->id.is_null() ) {
            return npgettext( "item name", "human blood", "human blood", quantity );
        } else {
            return string_format( npgettext( "item name", "%s blood",
                                             "%s blood",  quantity ),
                                  corpse->nname() );
        }
    } else if( iter != item_vars.end() ) {
        return iter->second.str();
    } else if( use_variant && has_itype_variant() ) {
        ret_name = itype_variant().alt_name.translated( quantity );
    } else {
        ret_name = type->nname( quantity );
    }

    // Apply conditional names, in order.
    std::vector<conditional_name> const &cond_names =
        use_cond_name ? type->conditional_names : std::vector<conditional_name> {};
    for( const conditional_name &cname : cond_names ) {
        // Lambda for searching for a item ID among all components.
        std::function<bool( const item_components & )> component_id_equals =
        [&]( const item_components & components ) {
            for( const item_components::type_vector_pair &tvp : components ) {
                for( const item &component : tvp.second ) {
                    if( component.typeId().str() == cname.condition ) {
                        return true;
                    }
                }
            }
            return false;
        };
        // Lambda for recursively searching for a item ID substring among all components.
        std::function<bool ( const item_components & )> component_id_contains =
        [&]( const item_components & components ) {
            for( const item_components::type_vector_pair &tvp : components ) {
                for( const item &component : tvp.second ) {
                    if( component.typeId().str().find( cname.condition ) != std::string::npos ||
                        component_id_contains( component.components ) ) {
                        return true;
                    }
                }
            }
            return false;
        };
        switch( cname.type ) {
            case condition_type::FLAG:
                if( has_flag( flag_id( cname.condition ) ) ) {
                    ret_name = string_format( cname.name.translated( quantity ), ret_name );
                }
                break;
            case condition_type::VITAMIN:
                if( has_vitamin( vitamin_id( cname.condition ) ) ) {
                    ret_name = string_format( cname.name.translated( quantity ), ret_name );
                }
                break;
            case condition_type::COMPONENT_ID:
                if( component_id_equals( components ) ) {
                    ret_name = string_format( cname.name.translated( quantity ), ret_name );
                }
                break;
            case condition_type::COMPONENT_ID_SUBSTRING:
                if( component_id_contains( components ) ) {
                    ret_name = string_format( cname.name.translated( quantity ), ret_name );
                }
                break;
            case condition_type::VAR:
                if( has_var( cname.condition ) && get_var( cname.condition ) == cname.value ) {
                    ret_name = string_format( cname.name.translated( quantity ), ret_name );
                }
                break;
            case condition_type::SNIPPET_ID:
                if( has_var( cname.condition + "_snippet_id" ) &&
                    get_var( cname.condition + "_snippet_id" ) == cname.value ) {
                    ret_name = string_format( cname.name.translated( quantity ), ret_name );
                }
                break;
            case condition_type::num_condition_types:
                break;
        }
    }

    // Identify who this corpse belonged to, if applicable.
    if( corpse != nullptr && use_corpse && has_flag( flag_CORPSE ) ) {
        if( corpse_name.empty() ) {
            //~ %1$s: name of corpse with modifiers;  %2$s: species name
            ret_name = string_format( pgettext( "corpse ownership qualifier", "%1$s of a %2$s" ),
                                      ret_name, corpse->nname() );
        } else {
            //~ %1$s: name of corpse with modifiers;  %2$s: proper name;  %3$s: species name
            ret_name = string_format( pgettext( "corpse ownership qualifier", "%1$s of %2$s, %3$s" ),
                                      ret_name, corpse_name, corpse->nname() );
        }
    }

    return ret_name;
}

const mtype *item::get_corpse_mon() const
{
    return corpse;
}

std::string item::get_corpse_name() const
{
    return corpse_name;
}

std::string item::nname( const itype_id &id, unsigned int quantity )
{
    const itype *t = find_type( id );
    return t->nname( quantity );
}

std::string item::tname( const itype_id &id, unsigned int quantity,
                         const tname::segment_bitset &segments )
{
    item item_temp( id );
    return item_temp.tname( quantity, segments );
}

bool item::count_by_charges( const itype_id &id )
{
    const itype *t = find_type( id );
    return t->count_by_charges();
}

bool item::type_is_defined( const itype_id &id )
{
    return item_controller->has_template( id );
}

const itype *item::find_type( const itype_id &type )
{
    return item_controller->find_template( type );
}

units::energy item::get_gun_energy_drain() const
{
    units::energy draincount = 0_kJ;
    if( type->gun ) {
        units::energy modifier = 0_kJ;
        float multiplier = 1.0f;
        for( const item *mod : gunmods() ) {
            modifier += mod->type->gunmod->energy_drain_modifier;
            multiplier *= mod->type->gunmod->energy_drain_multiplier;
        }
        draincount = ( type->gun->energy_drain * multiplier ) + modifier;
    }
    return draincount;
}

units::energy item::get_gun_ups_drain() const
{
    if( has_flag( flag_USE_UPS ) ) {
        return get_gun_energy_drain();
    }
    return 0_kJ;
}

units::energy item::get_gun_bionic_drain() const
{
    if( has_flag( flag_USES_BIONIC_POWER ) ) {
        return get_gun_energy_drain();
    }
    return 0_kJ;
}

bool item::has_label() const
{
    return has_var( "item_label" );
}

std::string item::label( unsigned int quantity, bool use_variant,
                         bool use_cond_name, bool use_corpse ) const
{
    if( has_label() ) {
        return get_var( "item_label" );
    }

    return type_name( quantity, use_variant, use_cond_name, use_corpse );
}

bool item::has_infinite_charges() const
{
    return charges == INFINITE_CHARGES;
}

skill_id item::contextualize_skill( const skill_id &id ) const
{
    if( id->is_contextual_skill() ) {
        if( id == skill_weapon ) {
            if( is_gun() ) {
                return gun_skill();
            } else if( is_melee() ) {
                return melee_skill();
            }
        }
    }

    return id;
}

int item::get_to_hit() const
{
    int to_hit = type->m_to_hit;
    for( const item *mod : gunmods() ) {
        to_hit += mod->type->gunmod->to_hit_mod;
    }

    return to_hit;
}

bool item::is_filthy() const
{
    return has_flag( flag_FILTHY );
}

bool item::on_drop( const tripoint_bub_ms &pos )
{
    return on_drop( pos, get_map() );
}

bool item::on_drop( const tripoint_bub_ms &pos, map &m )
{
    // dropping liquids, even currently frozen ones, on the ground makes them
    // dirty
    if( made_of_from_type( phase_id::LIQUID ) && !m.has_flag( ter_furn_flag::TFLAG_LIQUIDCONT, pos ) &&
        !has_own_flag( flag_DIRTY ) ) {
        set_flag( flag_DIRTY );
    }

    avatar &player_character = get_avatar();

    return type->drop_action && type->drop_action.call( &player_character, *this, &m, pos );
}

time_duration item::age() const
{
    return calendar::turn - birthday();
}

void item::set_age( const time_duration &age )
{
    set_birthday( time_point( calendar::turn ) - age );
}

time_point item::birthday() const
{
    return bday;
}

void item::set_birthday( const time_point &bday )
{
    this->bday = std::max( calendar::turn_zero, bday );
}

bool item::is_upgrade() const
{
    if( !type->bionic ) {
        return false;
    }
    return type->bionic->is_upgrade;
}

int item::get_min_str() const
{
    const Character &p = get_player_character();
    if( type->gun ) {
        int min_str = type->min_str;
        // we really need some better check for bows than its skill
        if( type->gun->skill_used == skill_archery ) {
            min_str -= p.get_proficiency_bonus( "archery", proficiency_bonus_type::strength );
        }
        for( const item *mod : gunmods() ) {
            min_str += mod->type->gunmod->min_str_required_mod;
        }
        if( p.is_prone() ) {
            for( const item *mod : gunmods() ) {
                min_str += mod->type->gunmod->min_str_required_mod_if_prone;
            }
        }
        return min_str > 0 ? min_str : 0;
    } else {
        return type->min_str;
    }
}

std::vector<item_comp> item::get_uncraft_components() const
{
    std::vector<item_comp> ret;
    if( components.empty() ) {
        //If item wasn't crafted with specific components use default recipe
        std::vector<std::vector<item_comp>> recipe = recipe_dictionary::get_uncraft(
                                             typeId() ).disassembly_requirements().get_components();
        for( std::vector<item_comp> &component : recipe ) {
            ret.push_back( component.front() );
        }
    } else {
        //Make a new vector of components from the registered components
        for( const item_components::type_vector_pair &tvp : components ) {
            for( const item &component : tvp.second ) {
                if( component.has_flag( flag_UNRECOVERABLE ) ) {
                    continue;
                }
                auto iter = std::find_if( ret.begin(), ret.end(), [&component]( item_comp & obj ) {
                    return obj.type == component.typeId();
                } );

                if( iter != ret.end() ) {
                    iter->count += component.count();
                } else {
                    ret.emplace_back( component.typeId(), component.count() );
                }
            }
        }
    }
    return ret;
}

void item::set_favorite( const bool favorite )
{
    is_favorite = favorite;
}

const recipe &item::get_making() const
{
    if( !craft_data_ ) {
        debugmsg( "'%s' is not a craft or has a null recipe", tname() );
        static const recipe dummy{};
        return dummy;
    }
    cata_assert( craft_data_->making );
    return *craft_data_->making;
}

int item::get_making_batch_size() const
{
    if( !craft_data_ ) {
        debugmsg( "'%s' is not a craft or has a null recipe", tname() );
        return 0;
    }
    cata_assert( craft_data_->batch_size );
    return craft_data_->batch_size;
}

void item::set_tools_to_continue( bool value )
{
    cata_assert( craft_data_ );
    craft_data_->tools_to_continue = value;
}

bool item::has_tools_to_continue() const
{
    cata_assert( craft_data_ );
    return craft_data_->tools_to_continue;
}

void item::set_cached_tool_selections( const std::vector<comp_selection<tool_comp>> &selections )
{
    cata_assert( craft_data_ );
    craft_data_->cached_tool_selections = selections;
}

const std::vector<comp_selection<tool_comp>> &item::get_cached_tool_selections() const
{
    cata_assert( craft_data_ );
    return craft_data_->cached_tool_selections;
}

const cata::value_ptr<islot_comestible> &item::get_comestible() const
{
    if( is_craft() && !craft_data_->disassembly ) {
        return find_type( craft_data_->making->result() )->comestible;
    } else {
        return type->comestible;
    }
}

units::volume item::get_selected_stack_volume( const std::map<const item *, int> &without ) const
{
    auto stack = without.find( this );
    if( stack != without.end() ) {
        int selected = stack->second;
        item copy = *this;
        copy.charges = selected;
        return copy.volume();
    }

    return 0_ml;
}

int item::get_recursive_disassemble_moves( const Character &guy ) const
{
    int moves = recipe_dictionary::get_uncraft( type->get_id() ).time_to_craft_moves( guy,
                recipe_time_flag::ignore_proficiencies );
    std::vector<item_comp> to_be_disassembled = get_uncraft_components();
    while( !to_be_disassembled.empty() ) {
        item_comp current_comp = to_be_disassembled.back();
        to_be_disassembled.pop_back();
        const recipe &r = recipe_dictionary::get_uncraft( current_comp.type->get_id() );
        if( r.ident() != recipe_id::NULL_ID() ) {
            moves += r.time_to_craft_moves( guy ) * current_comp.count;
            std::vector<item_comp> components = item( current_comp.type->get_id() ).get_uncraft_components();
            for( item_comp &component : components ) {
                component.count *= current_comp.count;
                to_be_disassembled.push_back( component );
            }
        }
    }
    return moves;
}

size_t item::num_item_stacks() const
{
    return contents.num_item_stacks();
}

bool is_preferred_component( const item &component )
{
    const float survival = get_player_character().get_greater_skill_or_knowledge_level(
                               skill_survival );
    return component.is_container_empty() &&
           ( survival < 3 || !component.has_flag( flag_HIDDEN_POISON ) ) &&
           ( survival < 5 || !component.has_flag( flag_HIDDEN_HALLU ) );
}

disp_mod_by_barrel::disp_mod_by_barrel() = default;
void disp_mod_by_barrel::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "barrel_length", barrel_length );
    mandatory( jo, false, "dispersion", dispersion_modifier );
}

void rot_spawn_data::deserialize( const JsonObject &jo )
{
    optional( jo, false, "monster", rot_spawn_monster, mtype_id::NULL_ID() );
    optional( jo, false, "group", rot_spawn_group, mongroup_id::NULL_ID() );
    optional( jo, false, "chance", rot_spawn_chance );
    optional( jo, false, "amount", rot_spawn_monster_amount, {1, 1} );
}
