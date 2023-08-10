#include "item.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>

#include "ammo.h"
#include "ascii_art.h"
#include "avatar.h"
#include "bionics.h"
#include "bodygraph.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "city.h"
#include "clothing_mod.h"
#include "clzones.h"
#include "color.h"
#include "coordinates.h"
#include "craft_command.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "dispersion.h"
#include "display.h"
#include "effect.h" // for weed_msg
#include "effect_source.h"
#include "enum_traits.h"
#include "enums.h"
#include "explosion.h"
#include "faction.h"
#include "fault.h"
#include "field_type.h"
#include "fire.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "gun_mode.h"
#include "iexamine.h"
#include "inventory.h"
#include "item_category.h"
#include "item_factory.h"
#include "item_group.h"
#include "item_pocket.h"
#include "iteminfo_query.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "line.h"
#include "localized_comparator.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "make_static.h"
#include "map.h"
#include "martialarts.h"
#include "material.h"
#include "messages.h"
#include "mod_manager.h"
#include "monster.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "point.h"
#include "proficiency.h"
#include "projectile.h"
#include "ranged.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "relic.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "skill.h"
#include "stomach.h"
#include "string_formatter.h"
#include "string_id.h"
#include "string_id_utils.h"
#include "text_snippets.h"
#include "translations.h"
#include "try_parse_integer.h"
#include "units.h"
#include "units_fwd.h"
#include "units_utility.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "vitamin.h"
#include "veh_type.h"
#include "vpart_position.h"
#include "weather.h"
#include "weather_gen.h"
#include "weather_type.h"

static const std::string GUN_MODE_VAR_NAME( "item::mode" );
static const std::string CLOTHING_MOD_VAR_PREFIX( "clothing_mod_" );

static const ammotype ammo_battery( "battery" );
static const ammotype ammo_bolt( "bolt" );
static const ammotype ammo_money( "money" );
static const ammotype ammo_plutonium( "plutonium" );

static const bionic_id bio_digestion( "bio_digestion" );

static const bodygraph_id bodygraph_full_body_iteminfo( "full_body_iteminfo" );

static const damage_type_id damage_acid( "acid" );
static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_bullet( "bullet" );
static const damage_type_id damage_cut( "cut" );
static const damage_type_id damage_heat( "heat" );

static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_cig( "cig" );
static const efftype_id effect_shakes( "shakes" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_weed_high( "weed_high" );

static const furn_str_id furn_f_metal_smoking_rack_active( "f_metal_smoking_rack_active" );
static const furn_str_id furn_f_smoking_rack_active( "f_smoking_rack_active" );
static const furn_str_id furn_f_water_mill_active( "f_water_mill_active" );
static const furn_str_id furn_f_wind_mill_active( "f_wind_mill_active" );

static const gun_mode_id gun_mode_REACH( "REACH" );

static const item_category_id item_category_container( "container" );
static const item_category_id item_category_drugs( "drugs" );
static const item_category_id item_category_food( "food" );
static const item_category_id item_category_maps( "maps" );
static const item_category_id item_category_spare_parts( "spare_parts" );
static const item_category_id item_category_tools( "tools" );
static const item_category_id item_category_weapons( "weapons" );

static const itype_id itype_barrel_small( "barrel_small" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_blood( "blood" );
static const itype_id itype_brass_catcher( "brass_catcher" );
static const itype_id itype_bullet_crossbow( "bullet_crossbow" );
static const itype_id itype_cig_butt( "cig_butt" );
static const itype_id itype_cig_lit( "cig_lit" );
static const itype_id itype_cigar_butt( "cigar_butt" );
static const itype_id itype_cigar_lit( "cigar_lit" );
static const itype_id itype_disassembly( "disassembly" );
static const itype_id itype_hand_crossbow( "hand_crossbow" );
static const itype_id itype_joint_roach( "joint_roach" );
static const itype_id itype_null( "null" );
static const itype_id itype_rad_badge( "rad_badge" );
static const itype_id itype_rm13_armor( "rm13_armor" );
static const itype_id itype_stock_none( "stock_none" );
static const itype_id itype_tuned_mechanism( "tuned_mechanism" );
static const itype_id itype_water( "water" );
static const itype_id itype_water_clean( "water_clean" );
static const itype_id itype_waterproof_gunmod( "waterproof_gunmod" );

static const json_character_flag json_flag_CANNIBAL( "CANNIBAL" );
static const json_character_flag json_flag_IMMUNE_SPOIL( "IMMUNE_SPOIL" );

static const matec_id RAPID( "RAPID" );

static const material_id material_wool( "wool" );

static const mon_flag_str_id mon_flag_POISON( "POISON" );
static const mon_flag_str_id mon_flag_REVIVES( "REVIVES" );

static const morale_type morale_null( "morale_null" );

static const mtype_id debug_mon( "debug_mon" );
static const mtype_id mon_human( "mon_human" );
static const mtype_id mon_zombie_smoker( "mon_zombie_smoker" );
static const mtype_id mon_zombie_soldier( "mon_zombie_soldier" );
static const mtype_id mon_zombie_survivor( "mon_zombie_survivor" );

static const npc_class_id NC_BOUNTY_HUNTER( "NC_BOUNTY_HUNTER" );

static const quality_id qual_BOIL( "BOIL" );
static const quality_id qual_JACK( "JACK" );
static const quality_id qual_LIFT( "LIFT" );

static const skill_id skill_cooking( "cooking" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_survival( "survival" );
static const skill_id skill_weapon( "weapon" );

static const species_id species_ROBOT( "ROBOT" );

static const sub_bodypart_str_id sub_body_part_torso_lower( "torso_lower" );
static const sub_bodypart_str_id sub_body_part_torso_upper( "torso_upper" );

static const trait_id trait_CARNIVORE( "CARNIVORE" );
static const trait_id trait_JITTERY( "JITTERY" );
static const trait_id trait_LIGHTWEIGHT( "LIGHTWEIGHT" );
static const trait_id trait_TOLERANCE( "TOLERANCE" );
static const trait_id trait_WOOLALLERGY( "WOOLALLERGY" );

// vitamin flags
static const std::string flag_NO_DISPLAY( "NO_DISPLAY" );

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

    if( has_flag( flag_CORPSE ) ) {
        corpse = &type->source_monster.obj();
        if( !type->source_monster.is_null() && !type->source_monster->zombify_into.is_empty() ) {
            set_var( "zombie_form", type->source_monster->zombify_into.c_str() );
        }
    }

    if( qty >= 0 ) {
        charges = qty;
    } else {
        if( type->tool && type->tool->rand_charges.size() > 1 ) {
            const int charge_roll = rng( 1, type->tool->rand_charges.size() - 1 );
            charges = rng( type->tool->rand_charges[charge_roll - 1], type->tool->rand_charges[charge_roll] );
        } else {
            charges = type->charges_default();
        }
    }

    if( has_flag( flag_SPAWN_ACTIVE ) ) {
        activate();
    }

    if( has_flag( flag_COLLAPSE_CONTENTS ) ) {
        for( item_pocket *pocket : contents.get_all_standard_pockets() ) {
            pocket->settings.set_collapse( true );
        }
    } else {
        auto const mag_filter = []( item_pocket const & pck ) {
            return pck.is_type( item_pocket::pocket_type::MAGAZINE ) ||
                   pck.is_type( item_pocket::pocket_type::MAGAZINE_WELL );
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

    select_itype_variant();
    if( type->gun ) {
        for( const itype_id &mod : type->gun->built_in_mods ) {
            item it( mod, turn, qty );
            it.set_flag( flag_IRREMOVABLE );
            put_in( it, item_pocket::pocket_type::MOD );
        }
        for( const itype_id &mod : type->gun->default_mods ) {
            put_in( item( mod, turn, qty ), item_pocket::pocket_type::MOD );
        }

    } else if( type->magazine ) {
        if( type->magazine->count > 0 ) {
            put_in( item( type->magazine->default_ammo, calendar::turn, type->magazine->count ),
                    item_pocket::pocket_type::MAGAZINE );
        }

    } else if( has_temperature() ) {
        active = true;
        last_temp_check = bday;

    } else if( type->tool ) {
        if( ammo_remaining() && !ammo_types().empty() ) {
            ammo_set( ammo_default(), ammo_remaining() );
        }
    }

    if( ( type->gun || type->tool ) && !magazine_integral() ) {
        set_var( "magazine_converted", 1 );
    }

    if( !type->snippet_category.empty() ) {
        snip_id = SNIPPET.random_id_from_category( type->snippet_category );
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
    return anchor.reference_to( this );
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

// sorts with localized_compare, and enumerates entries, if more than \p max entries
// the rest are abbreviated into " and %d more"
static std::string enumerate_lcsorted_with_limit( const std::vector<std::string> &v,
        const int max = 5 )
{
    if( max < 1 ) {
        debugmsg( "Expected max to be 1 or more" );
        return std::string();
    }
    std::vector<std::string> c( std::min<size_t>( max, v.size() ) );
    std::partial_sort_copy( v.begin(), v.end(), c.begin(), c.end(), localized_compare );
    const int more = static_cast<int>( v.size() - c.size() );
    if( more == 0 ) {
        return enumerate_as_string( c );
    }
    const std::string res = enumerate_as_string( c, enumeration_conjunction::none );
    return string_format( max == 1 ? _( "%s and %d more" ) : _( "%s, and %d more" ), res, more );
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
        }
    } else {
        const item *most_rotten = get_most_rotten_component( it );
        if( most_rotten ) {
            it.set_relative_rot( most_rotten->get_relative_rot() );
        }
    }
}

item::item( const recipe *rec, int qty, item_components items, std::vector<item_comp> selections )
    : item( "craft", calendar::turn )
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
    : item( "disassembly", calendar::turn )
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
item::item( item && ) noexcept( map_is_noexcept ) = default;
item::~item() = default;
item &item::operator=( const item & ) = default;
item &item::operator=( item && ) noexcept( list_is_noexcept ) = default;

item item::make_corpse( const mtype_id &mt, time_point turn, const std::string &name,
                        const int upgrade_time )
{
    if( !mt.is_valid() ) {
        debugmsg( "tried to make a corpse with an invalid mtype id" );
    }

    std::string corpse_type = mt == mtype_id::NULL_ID() ? "corpse_generic_human" : "corpse";

    item result( corpse_type, turn );
    result.corpse = &mt.obj();

    if( result.corpse->has_flag( mon_flag_REVIVES ) ) {
        if( one_in( 20 ) ) {
            result.set_flag( flag_REVIVE_SPECIAL );
        }
        result.set_var( "upgrade_time", std::to_string( upgrade_time ) );
    }

    if( !mt->zombify_into.is_empty() ) {
        result.set_var( "zombie_form", mt->zombify_into.c_str() );
    }

    // This is unconditional because the const itemructor above sets result.name to
    // "human corpse".
    result.corpse_name = name;

    return result;
}

item &item::convert( const itype_id &new_type )
{
    // Carry over relative rot similar to crafting
    const double rel_rot = get_relative_rot();
    type = find_type( new_type );
    set_relative_rot( rel_rot );
    requires_tags_processing = true; // new type may have "active" flags
    item temp( *this );
    temp.contents = item_contents( type->pockets );
    for( const item *it : contents.mods() ) {
        if( !temp.put_in( *it, item_pocket::pocket_type::MOD ).success() ) {
            debugmsg( "failed to insert mod" );
        }
    }
    temp.update_modified_pockets();
    temp.contents.combine( contents, true );
    contents = temp.contents;

    if( temp.type->countdown_interval > 0_seconds ) {
        countdown_point = calendar::turn + type->countdown_interval;
        active = true;
    }

    return *this;
}

item &item::deactivate( const Character *ch, bool alert )
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

bool item::activate_thrown( const tripoint &pos )
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
                put_in( temp_ammo, item_pocket::pocket_type::MAGAZINE );
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
        put_in( set_ammo, item_pocket::pocket_type::MAGAZINE );

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
            put_in( item( mag ), item_pocket::pocket_type::MAGAZINE_WELL );
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
    return degradation_;
}

void item::rand_degradation()
{
    if( type->degrade_increments() == 0 ) {
        return; // likely count_by_charges
    }
    set_degradation( rng( 0, damage() ) * 50.0f / type->degrade_increments() );
}

int item::damage_level() const
{
    return type->damage_level( damage_ );
}

float item::damage_adjusted_melee_weapon_damage( float value ) const
{
    if( type->count_by_charges() ) {
        return value; // count by charges items don't have partial damage
    }
    return value * ( 1.0f - 0.1f * std::max( 0, damage_level() - 1 ) );
}

float item::damage_adjusted_gun_damage( float value ) const
{
    if( type->count_by_charges() ) {
        return value; // count by charges items don't have partial damage
    }
    return value - 2 * std::max( 0, damage_level() - 1 );
}

float item::damage_adjusted_armor_resist( float value ) const
{
    if( type->count_by_charges() ) {
        return value; // count by charges items don't have partial damage
    }
    return value * ( 1.0f - std::max( 0, damage_level() - 1 ) * 0.125f );
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

bool item::covers( const sub_bodypart_id &bp ) const
{
    // if the item has no armor data it doesn't cover that part
    const islot_armor *armor = find_armor_data();
    if( armor == nullptr ) {
        return false;
    }

    bool has_sub_data = false;

    // If the item has no sub location info and covers the part's parent,
    // then assume that the item covers that sub body part.
    for( const armor_portion_data &data : armor->sub_data ) {
        if( !data.sub_coverage.empty() ) {
            has_sub_data = true;
        }
    }

    if( !has_sub_data ) {
        return covers( bp->parent );
    }

    bool does_cover = false;
    bool subpart_cover = false;

    iterate_covered_sub_body_parts_internal( get_side(), [&]( const sub_bodypart_str_id & covered ) {
        does_cover = does_cover || bp == covered;
    } );

    return does_cover || subpart_cover;
}

bool item::covers( const bodypart_id &bp ) const
{
    bool does_cover = false;
    bool subpart_cover = false;
    iterate_covered_body_parts_internal( get_side(), [&]( const bodypart_str_id & covered ) {
        does_cover = does_cover || bp == covered;
    } );

    return does_cover || subpart_cover;
}

std::optional<side> item::covers_overlaps( const item &rhs ) const
{
    if( get_layer() != rhs.get_layer() ) {
        return std::nullopt;
    }
    const islot_armor *armor = find_armor_data();
    if( armor == nullptr ) {
        return std::nullopt;
    }
    const islot_armor *rhs_armor = rhs.find_armor_data();
    if( rhs_armor == nullptr ) {
        return std::nullopt;
    }
    body_part_set this_covers;
    for( const armor_portion_data &data : armor->data ) {
        if( data.covers.has_value() ) {
            this_covers.unify_set( *data.covers );
        }
    }
    body_part_set rhs_covers;
    for( const armor_portion_data &data : rhs_armor->data ) {
        if( data.covers.has_value() ) {
            rhs_covers.unify_set( *data.covers );
        }
    }
    if( this_covers.intersect_set( rhs_covers ).any() ) {
        return rhs.get_side();
    } else {
        return std::nullopt;
    }
}

std::vector<sub_bodypart_id> item::get_covered_sub_body_parts() const
{
    return get_covered_sub_body_parts( get_side() );
}

std::vector<sub_bodypart_id> item::get_covered_sub_body_parts( const side s ) const
{
    std::vector<sub_bodypart_id> res;
    iterate_covered_sub_body_parts_internal( s, [&]( const sub_bodypart_id & bp ) {
        res.push_back( bp );
    } );
    return res;
}

body_part_set item::get_covered_body_parts() const
{
    return get_covered_body_parts( get_side() );
}

body_part_set item::get_covered_body_parts( const side s ) const
{
    body_part_set res;
    iterate_covered_body_parts_internal( s, [&]( const bodypart_str_id & bp ) {
        res.set( bp );
    } );
    return res;
}

namespace
{

const std::array<bodypart_str_id, 4> &left_side_parts()
{
    static const std::array<bodypart_str_id, 4> result{ {
            body_part_arm_l,
            body_part_hand_l,
            body_part_leg_l,
            body_part_foot_l
        } };
    return result;
}

const std::array<bodypart_str_id, 4> &right_side_parts()
{
    static const std::array<bodypart_str_id, 4> result{ {
            body_part_arm_r,
            body_part_hand_r,
            body_part_leg_r,
            body_part_foot_r
        } };
    return result;
}

} // namespace

static void iterate_helper_sbp( const item *i, const side s,
                                const std::function<void( const sub_bodypart_str_id & )> &cb )
{
    const islot_armor *armor = i->find_armor_data();
    if( armor == nullptr ) {
        return;
    }

    for( const armor_portion_data &data : armor->sub_data ) {
        if( !data.sub_coverage.empty() ) {
            if( !armor->sided || s == side::BOTH || s == side::num_sides ) {
                for( const sub_bodypart_str_id &bpid : data.sub_coverage ) {
                    cb( bpid );
                }
                continue;
            }
            for( const sub_bodypart_str_id &bpid : data.sub_coverage ) {
                if( bpid->part_side == s || bpid->part_side == side::BOTH ) {
                    cb( bpid );
                }
            }
        }
    }
}

void item::iterate_covered_sub_body_parts_internal( const side s,
        const std::function<void( const sub_bodypart_str_id & )> &cb ) const
{
    iterate_helper_sbp( this, s, cb );

    //check for ablative armor too
    if( is_ablative() ) {
        for( const item_pocket *pocket : get_all_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();

                iterate_helper_sbp( &ablative_armor, s, cb );
            }
        }
    }
}

static void iterate_helper( const item *i, const side s,
                            const std::function<void( const bodypart_str_id & )> &cb )
{
    const islot_armor *armor = i->find_armor_data();
    if( armor == nullptr ) {
        return;
    }

    // If we are querying for only one side of the body, we ignore coverage
    // for parts on the opposite side of the body.
    const auto &opposite_side_parts = s == side::LEFT ? right_side_parts() : left_side_parts();

    for( const armor_portion_data &data : armor->data ) {
        if( data.covers.has_value() ) {
            if( !armor->sided || s == side::BOTH || s == side::num_sides ) {
                for( const bodypart_str_id &bpid : data.covers.value() ) {
                    cb( bpid );
                }
                continue;
            }
            for( const bodypart_str_id &bpid : data.covers.value() ) {
                if( std::find( opposite_side_parts.begin(), opposite_side_parts.end(),
                               bpid ) == opposite_side_parts.end() ) {
                    cb( bpid );
                }
            }
        }
    }
}

void item::iterate_covered_body_parts_internal( const side s,
        const std::function<void( const bodypart_str_id & )> &cb ) const
{
    iterate_helper( this, s, cb );

    //check for ablative armor too
    if( is_ablative() ) {
        for( const item_pocket *pocket : get_all_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();

                iterate_helper( &ablative_armor, s, cb );
            }
        }
    }
}

bool item::is_sided() const
{
    const islot_armor *t = find_armor_data();
    return t ? t->sided : false;
}

side item::get_side() const
{
    // MSVC complains if directly cast double to enum
    return static_cast<side>( static_cast<int>( get_var( STATIC( std::string{ "lateral" } ),
                              static_cast<int>( side::BOTH ) ) ) );
}

bool item::set_side( side s )
{
    if( !is_sided() ) {
        return false;
    }

    if( s == side::BOTH ) {
        erase_var( "lateral" );
    } else {
        set_var( "lateral", static_cast<int>( s ) );
    }

    return true;
}

bool item::swap_side()
{
    return set_side( opposite_side( get_side() ) );
}

bool item::is_ablative() const
{
    const islot_armor *t = find_armor_data();
    return t ? t->ablative : false;
}

bool item::has_additional_encumbrance() const
{
    const islot_armor *t = find_armor_data();
    return t ? t->additional_pocket_enc : false;
}

bool item::has_ripoff_pockets() const
{
    const islot_armor *t = find_armor_data();
    return t ? t->ripoff_chance : false;
}

bool item::has_noisy_pockets() const
{
    const islot_armor *t = find_armor_data();
    return t ? t->noisy : false;
}

bool item::is_worn_only_with( const item &it ) const
{
    return is_power_armor() && it.is_power_armor() && it.covers( bodypart_id( "torso" ) );
}
bool item::is_worn_by_player() const
{
    return get_player_character().is_worn( *this );
}

item item::in_its_container( int qty ) const
{
    return in_container( type->default_container.value_or( itype_null ), qty,
                         type->default_container_sealed );
}

item item::in_container( const itype_id &cont, int qty, const bool sealed ) const
{
    if( cont.is_null() ) {
        return *this;
    }

    if( qty <= 0 ) {
        qty = count();
    }
    item container( cont, birthday() );
    if( container.is_container() ) {
        container.fill_with( *this, qty );
        container.invlet = invlet;
        if( sealed ) {
            container.seal();
        }
        if( !container.has_item_with( [&cont]( const item & it ) {
        return it.typeId() == cont;
        } ) ) {
            debugmsg( "ERROR: failed to put %s in its container %s", typeId().c_str(), cont.c_str() );
            return *this;
        }
        container.add_automatic_whitelist();
        return container;
    } else if( is_software() && container.is_software_storage() ) {
        container.put_in( *this, item_pocket::pocket_type::SOFTWARE );
        container.invlet = invlet;
        return container;
    }
    return *this;
}

void item::add_automatic_whitelist()
{
    std::vector<item_pocket *> pkts = get_all_contained_pockets();
    if( pkts.size() == 1 && contents_only_one_type() ) {
        pkts.front()->settings.whitelist_item( contents.first_item().typeId() );
    }
}

void item::clear_automatic_whitelist()
{
    std::vector<item_pocket *> pkts = get_all_contained_pockets();
    if( pkts.size() == 1 && !pkts.front()->settings.was_edited() ) {
        cata::flat_set<itype_id> const &wl = pkts.front()->settings.get_item_whitelist();
        if( !wl.empty() ) {
            pkts.front()->settings.clear_item( *wl.begin() );
        }
    }
}

void item::update_modified_pockets()
{
    std::optional<const pocket_data *> mag_or_mag_well;
    std::vector<const pocket_data *> container_pockets;

    for( const pocket_data &pocket : type->pockets ) {
        if( pocket.type == item_pocket::pocket_type::CONTAINER ) {
            container_pockets.push_back( &pocket );
        } else if( pocket.type == item_pocket::pocket_type::MAGAZINE ||
                   pocket.type == item_pocket::pocket_type::MAGAZINE_WELL ) {
            mag_or_mag_well = &pocket;
        }
    }

    for( const item *mod : mods() ) {
        if( mod->type->mod ) {
            for( const pocket_data &pocket : mod->type->mod->add_pockets ) {
                if( pocket.type == item_pocket::pocket_type::CONTAINER ) {
                    container_pockets.push_back( &pocket );
                } else if( pocket.type == item_pocket::pocket_type::MAGAZINE ||
                           pocket.type == item_pocket::pocket_type::MAGAZINE_WELL ) {
                    mag_or_mag_well = &pocket;
                }
            }
        }
    }

    contents.update_modified_pockets( mag_or_mag_well, container_pockets );
}

int item::charges_per_volume( const units::volume &vol ) const
{
    if( count_by_charges() ) {
        if( type->volume == 0_ml ) {
            debugmsg( "Item '%s' with zero volume", tname() );
            return INFINITE_CHARGES;
        }
        // Type cast to prevent integer overflow with large volume containers like the cargo
        // dimension
        return vol * static_cast<int64_t>( type->stack_size ) / type->volume;
    } else {
        units::volume my_volume = volume();
        if( my_volume == 0_ml ) {
            debugmsg( "Item '%s' with zero volume", tname() );
            return INFINITE_CHARGES;
        }
        return vol / my_volume;
    }
}

int item::charges_per_weight( const units::mass &m ) const
{
    if( count_by_charges() ) {
        if( type->weight == 0_gram ) {
            debugmsg( "Item '%s' with zero weight", tname() );
            return INFINITE_CHARGES;
        }
        return m / type->weight;
    } else {
        units::mass my_weight = weight();
        if( my_weight == 0_gram ) {
            debugmsg( "Item '%s' with zero weight", tname() );
            return INFINITE_CHARGES;
        }
        return m / my_weight;
    }
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
    return stacks_with( rhs, true, false, 0, 9 );
}

bool item::stacks_with( const item &rhs, bool check_components, bool combine_liquid, int depth,
                        int maxdepth ) const
{
    if( type != rhs.type ) {
        return false;
    }
    if( is_relic() && rhs.is_relic() && !( *relic_data == *rhs.relic_data ) ) {
        return false;
    }
    if( has_itype_variant() != rhs.has_itype_variant() ||
        ( has_itype_variant() && rhs.has_itype_variant() &&
          itype_variant().id != rhs.itype_variant().id ) ) {
        return false;
    }
    if( ammo_remaining() != 0 && rhs.ammo_remaining() != 0 && is_money() ) {
        // Dealing with nonempty cash cards
        // TODO: Fix cash cards not showing total value. Until that is fixed do not stack cash cards.
        // When that is fixed just change this to true.
        return false;
    }
    // This function is also used to test whether items counted by charges should be merged, for that
    // check the, the charges must be ignored. In all other cases (tools/guns), the charges are important.
    if( !count_by_charges() && charges != rhs.charges ) {
        return false;
    }
    if( is_favorite != rhs.is_favorite ) {
        return false;
    }
    if( damage_ != rhs.damage_ ) {
        return false;
    }
    if( degradation_ != rhs.degradation_ ) {
        return false;
    }
    if( burnt != rhs.burnt ) {
        return false;
    }
    if( active != rhs.active ) {
        return false;
    }
    if( combine_liquid && has_temperature() && made_of_from_type( phase_id::LIQUID ) ) {

        // we can combine liquids of same type and different temperatures
        if( !equal_ignoring_elements( rhs.get_flags(), get_flags(),
        { flag_COLD, flag_FROZEN, flag_HOT, flag_NO_PARASITES, flag_FROM_FROZEN_LIQUID } ) ) {
            return false;
        }
    } else if( item_tags != rhs.item_tags ) {
        return false;
    }
    // Items with different faults do not stack (such as new vs. used guns)
    if( faults != rhs.faults ) {
        return false;
    }
    if( techniques != rhs.techniques ) {
        return false;
    }
    // Guns with enough fouling to change the indicator symbol don't stack
    if( dirt_symbol() != rhs.dirt_symbol() ) {
        return false;
    }
    // Guns that differ only by dirt/shot_counter can still stack,
    // but other item_vars such as label/note will prevent stacking
    const std::vector<std::string> ignore_keys = { "dirt", "shot_counter", "spawn_location_omt" };
    if( map_without_keys( item_vars, ignore_keys ) != map_without_keys( rhs.item_vars, ignore_keys ) ) {
        return false;
    }
    const std::string omt_loc_var = "spawn_location_omt";
    const bool this_has_location = has_var( omt_loc_var );
    const bool that_has_location = has_var( omt_loc_var );
    if( this_has_location != that_has_location ) {
        return false;
    }
    if( this_has_location && that_has_location ) {
        const tripoint_abs_omt this_loc( get_var( "spawn_location_omt", tripoint_zero ) );
        const tripoint_abs_omt that_loc( rhs.get_var( "spawn_location_omt", tripoint_zero ) );
        const tripoint_abs_omt player_loc( ms_to_omt_copy( get_map().getabs(
                                               get_player_character().pos() ) ) );
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
        if( get_bucket( this_dist ) != get_bucket( that_dist ) ) {
            return false;
        }
    }
    if( goes_bad() && rhs.goes_bad() ) {
        // Stack items that fall into the same "bucket" of freshness.
        // Distant buckets are larger than near ones.
        std::pair<int, clipped_unit> my_clipped_time_to_rot =
            clipped_time( get_shelf_life() - rot );
        std::pair<int, clipped_unit> other_clipped_time_to_rot =
            clipped_time( rhs.get_shelf_life() - rhs.rot );
        if( ( !combine_liquid || !made_of_from_type( phase_id::LIQUID ) ) &&
            my_clipped_time_to_rot != other_clipped_time_to_rot ) {
            return false;
        }
        if( rotten() != rhs.rotten() ) {
            // just to be safe that rotten and unrotten food is *never* stacked.
            return false;
        }
    }
    if( ( corpse == nullptr && rhs.corpse != nullptr ) ||
        ( corpse != nullptr && rhs.corpse == nullptr ) ) {
        return false;
    }
    if( corpse != nullptr && rhs.corpse != nullptr &&
        ( corpse->id != rhs.corpse->id || corpse_name != rhs.corpse_name ) ) {
        return false;
    }
    if( craft_data_ || rhs.craft_data_ ) {
        // In-progress crafts are always distinct items. Easier to handle for the player,
        // and there shouldn't be that many items of this type around anyway.
        return false;
    }
    if( check_components || is_comestible() || is_craft() ) {
        //Only check if at least one item isn't using the default recipe or is comestible
        if( !components.empty() || !rhs.components.empty() ) {
            if( get_uncraft_components() != rhs.get_uncraft_components() ) {
                return false;
            }
        }
    }
    if( link_length() != rhs.link_length() ) {
        return false;
    }
    if( max_link_length() != rhs.max_link_length() ) {
        return false;
    }
    const std::vector<const item *> this_mods = mods();
    const std::vector<const item *> that_mods = rhs.mods();
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
    return contents.stacks_with( rhs.contents, depth, maxdepth );
}

bool item::same_contents( const item &rhs ) const
{
    return get_contents().same_contents( rhs.get_contents() );
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

int item::obtain_cost( const item &it ) const
{
    return contents.obtain_cost( it );
}

int item::insert_cost( const item &it ) const
{
    return contents.insert_cost( it );
}

ret_val<void> item::put_in( const item &payload, item_pocket::pocket_type pk_type,
                            const bool unseal_pockets )
{
    ret_val<item_pocket *> result = contents.insert_item( payload, pk_type );
    if( !result.success() ) {
        debugmsg( "tried to put an item (%s) count (%d) in a container (%s) that cannot contain it: %s",
                  payload.typeId().str(), payload.count(), typeId().str(), result.str() );
    }
    if( pk_type == item_pocket::pocket_type::MOD ) {
        update_modified_pockets();
    }
    if( unseal_pockets && result.success() ) {
        result.value()->unseal();
    }
    on_contents_changed();
    if( result.success() ) {
        return ret_val<void>::make_success( result.str() );
    } else {
        return ret_val<void>::make_failure( result.str() );
    }
}

void item::force_insert_item( const item &it, item_pocket::pocket_type pk_type )
{
    contents.force_insert_item( it, pk_type );
    update_inherited_flags();
}

void item::set_var( const std::string &name, const int value )
{
    std::ostringstream tmpstream;
    tmpstream.imbue( std::locale::classic() );
    tmpstream << value;
    item_vars[name] = tmpstream.str();
}

void item::set_var( const std::string &name, const long long value )
{
    std::ostringstream tmpstream;
    tmpstream.imbue( std::locale::classic() );
    tmpstream << value;
    item_vars[name] = tmpstream.str();
}

// NOLINTNEXTLINE(cata-no-long)
void item::set_var( const std::string &name, const long value )
{
    std::ostringstream tmpstream;
    tmpstream.imbue( std::locale::classic() );
    tmpstream << value;
    item_vars[name] = tmpstream.str();
}

void item::set_var( const std::string &name, const double value )
{
    item_vars[name] = string_format( "%f", value );
}

double item::get_var( const std::string &name, const double default_value ) const
{
    const auto it = item_vars.find( name );
    if( it == item_vars.end() ) {
        return default_value;
    }
    const std::string &val = it->second;
    char *end;
    errno = 0;
    double result = strtod( val.data(), &end );
    if( errno != 0 ) {
        debugmsg( "Error parsing floating point value from %s in item::get_var: %s",
                  val, strerror( errno ) );
        return default_value;
    }
    if( end != val.data() + val.size() ) {
        if( *end == ',' ) {
            // likely legacy format with localized ',' for fraction separator instead of '.'
            std::string converted_val = val;
            converted_val[end - val.data()] = '.';
            errno = 0;
            double result = strtod( converted_val.data(), &end );
            if( errno != 0 ) {
                debugmsg( "Error parsing floating point value from %s in item::get_var: %s",
                          val, strerror( errno ) );
                return default_value;
            }
            if( end != converted_val.data() + converted_val.size() ) {
                debugmsg( "Stray characters at end of floating point value %s in item::get_var", val );
            }
            return result;
        }
        debugmsg( "Stray characters at end of floating point value %s in item::get_var", val );
    }
    return result;
}

void item::set_var( const std::string &name, const tripoint &value )
{
    item_vars[name] = string_format( "%d,%d,%d", value.x, value.y, value.z );
}

tripoint item::get_var( const std::string &name, const tripoint &default_value ) const
{
    const auto it = item_vars.find( name );
    if( it == item_vars.end() ) {
        return default_value;
    }
    std::vector<std::string> values = string_split( it->second, ',' );
    cata_assert( values.size() == 3 );
    auto convert_or_error = []( const std::string_view s ) {
        ret_val<int> result = try_parse_integer<int>( s, false );
        if( result.success() ) {
            return result.value();
        } else {
            debugmsg( "Error parsing tripoint coordinate in item::get_var: %s", result.str() );
            return 0;
        }
    };
    return tripoint( convert_or_error( values[0] ),
                     convert_or_error( values[1] ),
                     convert_or_error( values[2] ) );
}

void item::set_var( const std::string &name, const std::string &value )
{
    item_vars[name] = value;
}

std::string item::get_var( const std::string &name, const std::string &default_value ) const
{
    const auto it = item_vars.find( name );
    if( it == item_vars.end() ) {
        return default_value;
    }
    return it->second;
}

std::string item::get_var( const std::string &name ) const
{
    return get_var( name, "" );
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

// TODO: Get rid of, handle multiple types gracefully
static int get_ranged_pierce( const common_ranged_data &ranged )
{
    if( ranged.damage.empty() ) {
        return 0;
    }

    return ranged.damage.damage_units.front().res_pen;
}

std::string item::info( bool showtext ) const
{
    std::vector<iteminfo> dummy;
    return info( showtext, dummy );
}

std::string item::info( bool showtext, std::vector<iteminfo> &iteminfo ) const
{
    return info( showtext, iteminfo, 1 );
}

std::string item::info( bool showtext, std::vector<iteminfo> &iteminfo, int batch ) const
{
    return info( iteminfo, showtext ? &iteminfo_query::all : &iteminfo_query::notext, batch );
}

// Generates a long-form description of the freshness of the given rottable food item.
// NB: Doesn't check for non-rottable!
static std::string get_freshness_description( const item &food_item )
{
    // So, skilled characters looking at food that is neither super-fresh nor about to rot
    // can guess its age as one of {quite fresh,midlife,past midlife,old soon}, and also
    // guess about how long until it spoils.
    const double rot_progress = food_item.get_relative_rot();
    const time_duration shelf_life = food_item.get_shelf_life();
    time_duration time_left = shelf_life - ( shelf_life * rot_progress );

    // Correct for an estimate that exceeds shelf life -- this happens especially with
    // fresh items.
    if( time_left > shelf_life ) {
        time_left = shelf_life;
    }

    Character &player_character = get_player_character();
    if( food_item.is_fresh() ) {
        // Fresh food is assumed to be obviously so regardless of skill.
        if( player_character.can_estimate_rot() ) {
            return string_format( _( "* This food looks as <good>fresh</good> as it can be.  "
                                     "It still has <info>%s</info> until it spoils." ),
                                  to_string_approx( time_left ) );
        } else {
            return _( "* This food looks as <good>fresh</good> as it can be." );
        }
    } else if( food_item.is_going_bad() ) {
        // Old food likewise is assumed to be fairly obvious.
        if( player_character.can_estimate_rot() ) {
            return string_format( _( "* This food looks <bad>old</bad>.  "
                                     "It's just <info>%s</info> from becoming inedible." ),
                                  to_string_approx( time_left ) );
        } else {
            return _( "* This food looks <bad>old</bad>.  "
                      "It's on the brink of becoming inedible." );
        }
    }

    if( !player_character.can_estimate_rot() ) {
        // Unskilled characters only get a hint that more information exists...
        return _( "* This food looks <info>fine</info>.  If you were more skilled in "
                  "cooking or survival, you might be able to make a better estimation." );
    }

    // Otherwise, a skilled character can determine the below options:
    if( rot_progress < 0.3 ) {
        //~ here, %s is an approximate time span, e.g., "over 2 weeks" or "about 1 season"
        return string_format( _( "* This food looks <good>quite fresh</good>.  "
                                 "It has <info>%s</info> until it spoils." ),
                              to_string_approx( time_left ) );
    } else if( rot_progress < 0.5 ) {
        //~ here, %s is an approximate time span, e.g., "over 2 weeks" or "about 1 season"
        return string_format( _( "* This food looks like it is reaching its <neutral>midlife</neutral>.  "
                                 "There's <info>%s</info> before it spoils." ),
                              to_string_approx( time_left ) );
    } else if( rot_progress < 0.7 ) {
        //~ here, %s is an approximate time span, e.g., "over 2 weeks" or "about 1 season"
        return string_format( _( "* This food looks like it has <neutral>passed its midlife</neutral>.  "
                                 "Edible, but will go bad in <info>%s</info>." ),
                              to_string_approx( time_left ) );
    } else {
        //~ here, %s is an approximate time span, e.g., "over 2 weeks" or "about 1 season"
        return string_format( _( "* This food looks like it <bad>will be old soon</bad>.  "
                                 "It has <info>%s</info>, so if you plan to use it, it's now or never." ),
                              to_string_approx( time_left ) );
    }
}

item::sizing item::get_sizing( const Character &p ) const
{
    const islot_armor *armor_data = find_armor_data();
    if( !armor_data ) {
        return sizing::ignore;
    }
    bool to_ignore = true;
    for( const armor_portion_data &piece : armor_data->data ) {
        if( piece.encumber != 0 ) {
            to_ignore = false;
        }
    }
    if( to_ignore ) {
        return sizing::ignore;
    } else {
        const bool small = p.get_size() == creature_size::tiny;

        const bool big = p.get_size() == creature_size::huge;

        if( has_flag( flag_INTEGRATED ) ) {
            if( big ) {
                return sizing::big_sized_big_char;
            } else if( small ) {
                return sizing::small_sized_small_char;
            } else {
                return sizing::human_sized_human_char;
            }
        }

        // due to the iterative nature of these features, something can fit and be undersized/oversized
        // but that is fine because we have separate logic to adjust encumbrance per each. One day we
        // may want to have fit be a flag that only applies if a piece of clothing is sized for you as there
        // is a bit of cognitive dissonance when something 'fits' and is 'oversized' and the same time
        const bool undersize = has_flag( flag_UNDERSIZE );
        const bool oversize = has_flag( flag_OVERSIZE );

        if( undersize ) {
            if( small ) {
                return sizing::small_sized_small_char;
            } else if( big ) {
                return sizing::small_sized_big_char;
            } else {
                return sizing::small_sized_human_char;
            }
        } else if( oversize ) {
            if( big ) {
                return sizing::big_sized_big_char;
            } else if( small ) {
                return sizing::big_sized_small_char;
            } else {
                return sizing::big_sized_human_char;
            }
        } else {
            if( big ) {
                return sizing::human_sized_big_char;
            } else if( small ) {
                return sizing::human_sized_small_char;
            } else {
                return sizing::human_sized_human_char;
            }
        }
    }
}

static int get_base_env_resist( const item &it )
{
    const islot_armor *t = it.find_armor_data();
    if( t == nullptr ) {
        if( it.is_pet_armor() ) {
            return it.type->pet_armor->env_resist * it.get_relative_health();
        } else {
            return 0;
        }
    }

    return t->avg_env_resist() * it.get_relative_health();
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

static void insert_separation_line( std::vector<iteminfo> &info )
{
    if( info.empty() || info.back().sName != "--" ) {
        info.emplace_back( "DESCRIPTION", "--" );
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
    base_hit *= std::max( 0.25f, 1.0f - guy.avg_encumb_of_limb_type( body_part_type::type::torso ) /
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
        guy.roll_all_damage( crit, base_damage, true, *this, "WEAPON", &mon, bp );
        damage_instance dealt_damage = base_damage;
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
            guy.roll_all_damage( crit, rs_base_damage, true, *this, "WEAPON", &mon, bp );
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
    { to_translation( "Best" ), { debug_mon, true, false } },
    { to_translation( "Vs. Agile" ), { mon_zombie_smoker, true, true } },
    { to_translation( "Vs. Armored" ), { mon_zombie_soldier, true, true } },
    { to_translation( "Vs. Mixed" ), { mon_zombie_survivor, false, true } },
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

void item::basic_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                       bool /* debug */ ) const
{
    if( parts->test( iteminfo_parts::BASE_MOD_SRC ) ) {
        info.emplace_back( "BASE", string_format( _( "Origin: %s" ), enumerate_as_string( type->src,
        []( const std::pair<itype_id, mod_id> &source ) {
            return string_format( "'%s'", source.second->name() );
        }, enumeration_conjunction::arrow ) ) );
        insert_separation_line( info );
    }

    const std::string space = "  ";
    if( parts->test( iteminfo_parts::BASE_MATERIAL ) ) {
        const std::vector<const material_type *> mat_types = made_of_types();
        if( !mat_types.empty() ) {
            const std::string material_list = enumerate_as_string( mat_types,
            []( const material_type * material ) {
                return string_format( "<stat>%s</stat>", material->name() );
            }, enumeration_conjunction::none );
            info.emplace_back( "BASE", string_format( _( "Material: %s" ), material_list ) );
        }
    }
    if( parts->test( iteminfo_parts::BASE_VOLUME ) ) {
        info.push_back( vol_to_info( "BASE", _( "Volume: " ), volume() * batch, 3 ) );
    }
    if( parts->test( iteminfo_parts::BASE_WEIGHT ) ) {
        info.push_back( weight_to_info( "BASE", space + _( "Weight: " ), weight() * batch ) );
        info.back().bNewLine = true;
    }
    if( parts->test( iteminfo_parts::BASE_LENGTH ) && length() > 0_mm ) {
        info.emplace_back( "BASE", _( "Length: " ),
                           string_format( "<num> %s", length_units( length() ) ),
                           iteminfo::lower_is_better,
                           convert_length( length() ), length().value() );
    }
    if( parts->test( iteminfo_parts::BASE_OWNER ) && !owner.is_null() ) {
        info.emplace_back( "BASE", string_format( _( "Owner: %s" ),
                           _( get_owner_name() ) ) );
    }
    if( parts->test( iteminfo_parts::BASE_CATEGORY ) ) {
        info.emplace_back( "BASE", _( "Category: " ),
                           "<header>" + get_category_shallow().name() + "</header>" );
    }

    if( parts->test( iteminfo_parts::DESCRIPTION ) ) {
        insert_separation_line( info );
        const std::map<std::string, std::string>::const_iterator idescription =
            item_vars.find( "description" );
        const std::optional<translation> snippet = SNIPPET.get_snippet_by_id( snip_id );
        if( snippet.has_value() ) {
            // Just use the dynamic description
            info.emplace_back( "DESCRIPTION", snippet.value().translated() );

            // only ever do the effect for a snippet the first time you see it
            if( !get_avatar().has_seen_snippet( snip_id ) ) {
                // Have looked at the item so call the on examine EOC for the snippet
                const std::optional<talk_effect_t> examine_effect = SNIPPET.get_EOC_by_id( snip_id );
                if( examine_effect.has_value() ) {
                    // activate the effect
                    dialogue d( get_talker_for( get_avatar() ), nullptr );
                    examine_effect.value().apply( d );
                }

                //note that you have seen the snippet
                get_avatar().add_snippet( snip_id );
            }
        } else if( idescription != item_vars.end() ) {
            info.emplace_back( "DESCRIPTION", idescription->second );
        } else if( has_itype_variant() ) {
            // append the description instead of fully overwriting it
            if( itype_variant().append ) {
                info.emplace_back( "DESCRIPTION", _( string_format( "%s  %s", type->description.translated(),
                                                     itype_variant().alt_description.translated() ) ) );
            } else {
                info.emplace_back( "DESCRIPTION", itype_variant().alt_description.translated() );
            }
        } else {
            if( has_flag( flag_MAGIC_FOCUS ) ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "This item is a <info>magical focus</info>.  "
                                      "You can cast spells with it in your hand." ) );
            }
            if( is_craft() ) {
                const std::string desc = ( typeId() == itype_disassembly ) ?
                                         _( "This is an in progress disassembly of %s.  "
                                            "It is %d percent complete." ) : _( "This is an in progress %s.  "
                                                    "It is %d percent complete." );
                const int percent_progress = item_counter / 100000;
                info.emplace_back( "DESCRIPTION", string_format( desc,
                                   craft_data_->making->result_name(),
                                   percent_progress ) );
            } else {
                info.emplace_back( "DESCRIPTION", type->description.translated() );
            }
        }
        insert_separation_line( info );
    }

    insert_separation_line( info );

    if( parts->test( iteminfo_parts::BASE_REQUIREMENTS ) ) {
        // Display any minimal stat or skill requirements for the item
        std::vector<std::string> req;
        if( get_min_str() > 0 ) {
            req.push_back( string_format( "%s %d", _( "strength" ), get_min_str() ) );
        }
        if( type->min_dex > 0 ) {
            req.push_back( string_format( "%s %d", _( "dexterity" ), type->min_dex ) );
        }
        if( type->min_int > 0 ) {
            req.push_back( string_format( "%s %d", _( "intelligence" ), type->min_int ) );
        }
        if( type->min_per > 0 ) {
            req.push_back( string_format( "%s %d", _( "perception" ), type->min_per ) );
        }
        for( const std::pair<skill_id, int> &sk : sorted_lex( type->min_skills ) ) {
            req.push_back( string_format( "%s %d", sk.first->name(), sk.second ) );
        }
        if( !req.empty() ) {
            info.emplace_back( "BASE", _( "<bold>Minimum requirements</bold>:" ) );
            info.emplace_back( "BASE", enumerate_as_string( req ) );
            insert_separation_line( info );
        }
    }

    if( has_var( "contained_name" ) && parts->test( iteminfo_parts::BASE_CONTENTS ) ) {
        info.emplace_back( "BASE", string_format( _( "Contains: %s" ),
                           get_var( "contained_name" ) ) );
    }
    if( count_by_charges() && !is_food() && !is_medication() &&
        parts->test( iteminfo_parts::BASE_AMOUNT ) ) {
        info.emplace_back( "BASE", _( "Amount: " ), "<num>", iteminfo::no_flags,
                           charges * batch );
    }
}

void item::debug_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /* batch */,
                       bool debug ) const
{
    if( debug && parts->test( iteminfo_parts::BASE_DEBUG ) ) {
        if( g != nullptr ) {
            info.emplace_back( "BASE", string_format( "itype_id: %s",
                               typeId().str() ) );
            if( !old_owner.is_null() ) {
                info.emplace_back( "BASE", string_format( _( "Old owner: %s" ),
                                   _( get_old_owner_name() ) ) );
            }
            info.emplace_back( "BASE", _( "age (hours): " ), "", iteminfo::lower_is_better,
                               to_hours<int>( age() ) );
            info.emplace_back( "BASE", _( "charges: " ), "", iteminfo::lower_is_better,
                               charges );
            info.emplace_back( "BASE", _( "damage: " ), "", iteminfo::lower_is_better,
                               damage_ );
            info.emplace_back( "BASE", _( "degradation: " ), "", iteminfo::lower_is_better,
                               degradation_ );
            info.emplace_back( "BASE", _( "active: " ), "", iteminfo::lower_is_better,
                               active );
            info.emplace_back( "BASE", _( "burn: " ), "", iteminfo::lower_is_better,
                               burnt );
            if( countdown_point != calendar::turn_max ) {
                info.emplace_back( "BASE", _( "countdown: " ), "", iteminfo::lower_is_better,
                                   to_seconds<int>( countdown_point - calendar::turn ) );
            }

            const std::string tags_listed = enumerate_as_string( item_tags,
            []( const flag_id & f ) {
                return f.str();
            }, enumeration_conjunction::none );
            info.emplace_back( "BASE", string_format( _( "tags: %s" ), tags_listed ) );

            const std::string flags_listed = enumerate_as_string( type->get_flags(),
            []( const flag_id & f ) {
                return f.str();
            }, enumeration_conjunction::none );

            info.emplace_back( "BASE", string_format( _( "flags: %s" ), flags_listed ) );
            for( auto const &imap : item_vars ) {
                info.emplace_back( "BASE",
                                   string_format( _( "item var: %s, %s" ), imap.first,
                                                  imap.second ) );
            }

            info.emplace_back( "BASE", _( "wetness: " ),
                               "", iteminfo::lower_is_better,
                               wetness );

            const std::string space = "  ";
            if( goes_bad() ) {
                info.emplace_back( "BASE", _( "age (turns): " ),
                                   "", iteminfo::lower_is_better,
                                   to_turns<int>( age() ) );
                info.emplace_back( "BASE", _( "rot (turns): " ),
                                   "", iteminfo::lower_is_better,
                                   to_turns<int>( rot ) );
                info.emplace_back( "BASE", space + _( "max rot (turns): " ),
                                   "", iteminfo::lower_is_better,
                                   to_turns<int>( get_shelf_life() ) );
            }
            if( has_temperature() ) {
                info.emplace_back( "BASE", _( "last temp: " ),
                                   "", iteminfo::lower_is_better,
                                   to_turn<int>( last_temp_check ) );
                info.emplace_back( "BASE", _( "Temp: " ), "", iteminfo::lower_is_better | iteminfo::is_decimal,
                                   units::to_kelvin( temperature ) );
                info.emplace_back( "BASE", _( "Spec ener: " ), "",
                                   iteminfo::lower_is_better | iteminfo::is_decimal,
                                   units::to_joule_per_gram( specific_energy ) );
                info.emplace_back( "BASE", _( "Spec heat lq: " ), "",
                                   iteminfo::lower_is_better | iteminfo::is_decimal,
                                   get_specific_heat_liquid() );
                info.emplace_back( "BASE", _( "Spec heat sld: " ), "",
                                   iteminfo::lower_is_better | iteminfo::is_decimal,
                                   get_specific_heat_solid() );
                info.emplace_back( "BASE", _( "latent heat: " ), "",
                                   iteminfo::lower_is_better,
                                   get_latent_heat() );
                info.emplace_back( "BASE", _( "Freeze point: " ), "",
                                   iteminfo::lower_is_better | iteminfo::is_decimal,
                                   units::to_kelvin( get_freeze_point() ) );
            }
        }
    }
}

void item::med_info( const item *med_item, std::vector<iteminfo> &info, const iteminfo_query *parts,
                     int batch, bool debug ) const
{
    const cata::value_ptr<islot_comestible> &med_com = med_item->get_comestible();
    if( med_com->quench != 0 && parts->test( iteminfo_parts::MED_QUENCH ) ) {
        info.emplace_back( "MED", _( "Quench: " ), med_com->quench );
    }

    Character &player_character = get_player_character();
    if( med_item->get_comestible_fun() != 0 && parts->test( iteminfo_parts::MED_JOY ) ) {
        info.emplace_back( "MED", _( "Enjoyability: " ),
                           player_character.fun_for( *med_item ).first );
    }

    if( parts->test( iteminfo_parts::FOOD_HEALTH ) && med_com->healthy != 0 ) {
        info.emplace_back( "MED", _( "Health: " ), healthy_bar( med_com->healthy ) );
    }

    if( med_com->stim != 0 && parts->test( iteminfo_parts::MED_STIMULATION ) ) {
        std::string name = string_format( "%s<stat>%s</stat>", _( "Stimulation: " ),
                                          med_com->stim > 0 ? _( "Upper" ) : _( "Downer" ) );
        info.emplace_back( "MED", name );
    }

    if( parts->test( iteminfo_parts::MED_PORTIONS ) ) {
        info.emplace_back( "MED", _( "Portions: " ),
                           std::abs( static_cast<int>( med_item->count() ) * batch ) );
    }

    if( parts->test( iteminfo_parts::MED_CONSUME_TIME ) ) {
        info.emplace_back( "MED", _( "Consume time: " ),
                           to_string( player_character.get_consume_time( *med_item ) ) );
    }

    bool addictive = false;
    for( const std::pair<const addiction_id, int> &add : med_com->addictions ) {
        if( add.second > 0 ) {
            addictive = true;
            break;
        }
    }
    if( addictive && parts->test( iteminfo_parts::DESCRIPTION_MED_ADDICTING ) ) {
        info.emplace_back( "DESCRIPTION", _( "* Consuming this item is <bad>addicting</bad>." ) );
    }

    rot_info( med_item, info, parts, batch, debug );
}

void item::food_info( const item *food_item, std::vector<iteminfo> &info,
                      const iteminfo_query *parts, int batch, bool debug ) const
{
    nutrients min_nutr;
    nutrients max_nutr;

    Character &player_character = get_player_character();
    std::string recipe_exemplar = get_var( "recipe_exemplar", "" );
    if( recipe_exemplar.empty() ) {
        min_nutr = max_nutr = player_character.compute_effective_nutrients( *food_item );
    } else {
        std::map<recipe_id, std::pair<nutrients, nutrients>> rec_cache;
        std::tie( min_nutr, max_nutr ) =
            player_character.compute_nutrient_range( *food_item, recipe_id( recipe_exemplar ), rec_cache );
    }

    bool show_nutr = parts->test( iteminfo_parts::FOOD_NUTRITION ) ||
                     parts->test( iteminfo_parts::FOOD_VITAMINS );
    if( min_nutr != max_nutr && show_nutr ) {
        info.emplace_back(
            "FOOD", _( "Nutrition will <color_cyan>vary with chosen ingredients</color>." ) );
        if( recipe_dict.is_item_on_loop( food_item->typeId() ) ) {
            info.emplace_back(
                "FOOD", _( "Nutrition range cannot be calculated accurately due to "
                           "<color_red>recipe loops</color>." ) );
        }
    }

    if( max_nutr.kcal() != 0 || food_item->get_comestible()->quench != 0 ) {
        if( parts->test( iteminfo_parts::FOOD_NUTRITION ) ) {
            info.emplace_back( "FOOD", _( "<bold>Calories (kcal)</bold>: " ),
                               "", iteminfo::no_newline, min_nutr.kcal() );
            if( max_nutr.kcal() != min_nutr.kcal() ) {
                info.emplace_back( "FOOD", _( "-" ),
                                   "", iteminfo::no_newline, max_nutr.kcal() );
            }
        }
        if( parts->test( iteminfo_parts::FOOD_QUENCH ) ) {
            const std::string space = "  ";
            info.emplace_back( "FOOD", space + _( "Quench: " ),
                               food_item->get_comestible()->quench );
        }
        if( parts->test( iteminfo_parts::FOOD_SATIATION ) ) {

            if( max_nutr.kcal() == min_nutr.kcal() ) {
                info.emplace_back( "FOOD", _( "<bold>Satiety: </bold>" ),
                                   satiety_bar( player_character.compute_calories_per_effective_volume( *food_item ) ) );
            } else {
                info.emplace_back( "FOOD", _( "<bold>Satiety: </bold>" ),
                                   satiety_bar( player_character.compute_calories_per_effective_volume( *food_item, &min_nutr ) ),
                                   iteminfo::no_newline
                                 );
                info.emplace_back( "FOOD", _( " - " ),
                                   satiety_bar( player_character.compute_calories_per_effective_volume( *food_item, &max_nutr ) ) );
            }
        }
    }

    const std::pair<int, int> fun_for_food_item = player_character.fun_for( *food_item );
    if( fun_for_food_item.first != 0 && parts->test( iteminfo_parts::FOOD_JOY ) ) {
        info.emplace_back( "FOOD", _( "Enjoyability: " ), fun_for_food_item.first );
    }

    if( parts->test( iteminfo_parts::FOOD_HEALTH ) && food_item->get_comestible()->healthy != 0 ) {
        info.emplace_back( "MED", _( "Health: " ),
                           healthy_bar( food_item->get_comestible()->healthy ) );
    }

    if( parts->test( iteminfo_parts::FOOD_PORTIONS ) ) {
        info.emplace_back( "FOOD", _( "Portions: " ),
                           std::abs( static_cast<int>( food_item->count() ) * batch ) );
    }
    if( food_item->corpse != nullptr && parts->test( iteminfo_parts::FOOD_SMELL ) &&
        ( debug || ( g != nullptr && player_character.has_trait( trait_CARNIVORE ) ) ) ) {
        info.emplace_back( "FOOD", _( "Smells like: " ) + food_item->corpse->nname() );
    }

    if( parts->test( iteminfo_parts::FOOD_CONSUME_TIME ) ) {
        info.emplace_back( "FOOD", _( "Consume time: " ),
                           to_string( player_character.get_consume_time( *food_item ) ) );
    }

    auto format_vitamin = [&]( const std::pair<vitamin_id, int> &v, bool display_vitamins ) {
        const bool is_vitamin = v.first->type() == vitamin_type::VITAMIN;
        // only display vitamins that we actually require
        if( player_character.vitamin_rate( v.first ) == 0_turns || v.second == 0 ||
            display_vitamins != is_vitamin || v.first->has_flag( flag_NO_DISPLAY ) ) {
            return std::string();
        }
        const int min_value = min_nutr.get_vitamin( v.first );
        const int max_value = v.second;
        const int min_rda = player_character.vitamin_RDA( v.first, min_value );
        const int max_rda = player_character.vitamin_RDA( v.first, max_value );
        std::string format;
        if( is_vitamin ) {
            format = min_value == max_value ? "%s (%i%%)" : "%s (%i-%i%%)";
            return string_format( format, v.first->name(), min_rda, max_rda );
        }
        format = min_value == max_value ? "%s (%i U)" : "%s (%i-%i U)";
        return string_format( format, v.first->name(), min_value, max_value );
    };

    const auto max_nutr_vitamins = sorted_lex( max_nutr.vitamins );
    const std::string required_vits = enumerate_as_string( max_nutr_vitamins,
    [&]( const std::pair<vitamin_id, int> &v ) {
        return format_vitamin( v, true );
    } );

    const std::string effect_vits = enumerate_as_string( max_nutr_vitamins,
    [&]( const std::pair<vitamin_id, int> &v ) {
        return format_vitamin( v, false );
    } );

    if( !required_vits.empty() && parts->test( iteminfo_parts::FOOD_VITAMINS ) ) {
        info.emplace_back( "FOOD", _( "Vitamins (RDA): " ), required_vits );
    }

    if( !effect_vits.empty() && parts->test( iteminfo_parts::FOOD_VIT_EFFECTS ) ) {
        info.emplace_back( "FOOD", _( "Other contents: " ), effect_vits );
    }

    insert_separation_line( info );

    if( parts->test( iteminfo_parts::FOOD_ALLERGEN )
        && player_character.allergy_type( *food_item ) != morale_null ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This food will cause an <bad>allergic reaction</bad>." ) );
    }

    if( food_item->has_flag( flag_CANNIBALISM ) &&
        parts->test( iteminfo_parts::FOOD_CANNIBALISM ) ) {
        if( !player_character.has_flag( json_flag_CANNIBAL ) ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* This food contains <bad>human flesh</bad>." ) );
        } else {
            info.emplace_back( "DESCRIPTION",
                               _( "* This food contains <good>human flesh</good>." ) );
        }
    }

    if( food_item->is_tainted() && parts->test( iteminfo_parts::FOOD_CANNIBALISM ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This food is <bad>tainted</bad> and will poison you." ) );
    }

    ///\EFFECT_SURVIVAL >=3 allows detection of poisonous food
    if( food_item->has_flag( flag_HIDDEN_POISON ) &&
        player_character.get_skill_level( skill_survival ) >= 3 &&
        parts->test( iteminfo_parts::FOOD_POISON ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* On closer inspection, this appears to be "
                              "<bad>poisonous</bad>." ) );
    }

    ///\EFFECT_SURVIVAL >=5 allows detection of hallucinogenic food
    if( food_item->has_flag( flag_HIDDEN_HALLU ) &&
        player_character.get_skill_level( skill_survival ) >= 5 &&
        parts->test( iteminfo_parts::FOOD_HALLUCINOGENIC ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* On closer inspection, this appears to be "
                              "<neutral>hallucinogenic</neutral>." ) );
    }

    rot_info( food_item, info, parts, batch, debug );
}

void item::rot_info( const item *const food_item, std::vector<iteminfo> &info,
                     const iteminfo_query *const parts, const int /*batch*/, const bool /*debug*/ ) const
{
    Character &player_character = get_player_character();

    if( food_item->goes_bad() && parts->test( iteminfo_parts::FOOD_ROT ) ) {
        const std::string rot_time = to_string_clipped( food_item->get_shelf_life() );
        info.emplace_back( "DESCRIPTION",
                           string_format( _( "* This item is <neutral>perishable</neutral>, "
                                             "and at room temperature has an estimated nominal "
                                             "shelf life of <info>%s</info>." ), rot_time ) );

        if( !food_item->rotten() ) {
            info.emplace_back( "DESCRIPTION", get_freshness_description( *food_item ) );
        }

        if( food_item->has_flag( flag_FREEZERBURN ) && !food_item->rotten() &&
            !food_item->has_flag( flag_MUSHY ) ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* Quality of this food suffers when it's frozen, and it "
                                  "<neutral>will become mushy after thawing out</neutral>." ) );
        }
        if( food_item->has_flag( flag_MUSHY ) && !food_item->rotten() ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* It was frozen once and after thawing became <bad>mushy and "
                                  "tasteless</bad>.  It will rot quickly if thawed again." ) );
        }
        if( food_item->has_flag( flag_NO_PARASITES ) &&
            player_character.get_skill_level( skill_cooking ) >= 3 ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* It seems that deep freezing <good>killed all "
                                  "parasites</good>." ) );
        }
        if( food_item->rotten() ) {
            if( player_character.has_bionic( bio_digestion ) ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "This item has started to <neutral>rot</neutral>, "
                                      "but <info>your bionic digestion can tolerate "
                                      "it</info>." ) );
            } else if( player_character.has_flag( json_flag_IMMUNE_SPOIL ) ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "This item has started to <neutral>rot</neutral>, "
                                      "but <info>you can tolerate it</info>." ) );
            } else {
                info.emplace_back( "DESCRIPTION",
                                   _( "This item has started to <bad>rot</bad>. "
                                      "<info>Eating</info> it would be a <bad>very bad "
                                      "idea</bad>." ) );
            }
        }
    }
}

void item::magazine_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /*batch*/,
                          bool /*debug*/ ) const
{
    if( !is_magazine() || has_flag( flag_NO_RELOAD ) || is_tool() ) {
        return;
    }

    if( parts->test( iteminfo_parts::MAGAZINE_COMPATIBLE_GUNS ) &&
        islot_magazine::compatible_guns.find( this->typeId() ) != islot_magazine::compatible_guns.end() ) {
        const auto compare = []( const auto & lhs, const auto & rhs ) {
            return localized_compare( lhs, rhs );
        };
        std::set<std::string, decltype( compare )> compatible_guns( compare );

        for( const itype_id &gun_type_id : islot_magazine::compatible_guns[this->typeId()] ) {
            const itype &gun_type = gun_type_id.obj();
            if( get_option<bool>( "SHOW_GUN_VARIANTS" ) ) {
                if( gun_type.variants.empty() ) {
                    compatible_guns.insert( gun_type.nname( 1 ) );
                }
                for( const itype_variant_data &variant_type : gun_type.variants ) {
                    compatible_guns.insert( variant_type.alt_name.translated() );
                }
            } else {
                compatible_guns.insert( gun_type.nname( 1 ) );
            }
        }

        if( !compatible_guns.empty() ) {
            const std::string mag_names = enumerate_as_string( compatible_guns );
            info.emplace_back( "MAGAZINE", _( "<bold>Compatible guns</bold>: " ) + mag_names );
            insert_separation_line( info );
        }
    }
    if( parts->test( iteminfo_parts::MAGAZINE_CAPACITY ) ) {
        for( const ammotype &at : ammo_types() ) {
            const std::string fmt = string_format( n_gettext( "<num> round of %s",
                                                   "<num> rounds of %s", ammo_capacity( at ) ),
                                                   at->name() );
            info.emplace_back( "MAGAZINE", _( "Capacity: " ), fmt, iteminfo::no_flags,
                               ammo_capacity( at ) );
        }
    }
    if( parts->test( iteminfo_parts::MAGAZINE_COMPATABILITY ) ) {
        for( const ammotype &at : ammo_types() ) {
            std::string ammo_details = print_ammo( at );
            if( !ammo_details.empty() ) {
                info.emplace_back( "MAGAZINE", _( "Variants: " ), ammo_details, iteminfo::no_flags );
            }

        }
    }
    if( parts->test( iteminfo_parts::MAGAZINE_RELOAD ) && type->magazine ) {
        info.emplace_back( "MAGAZINE", _( "Reload time: " ), _( "<num> moves per round" ),
                           iteminfo::lower_is_better, type->magazine->reload_time );
    }
    insert_separation_line( info );
}

void item::ammo_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /* batch */,
                      bool /* debug */ ) const
{
    // Skip this section for guns and items without ammo data
    if( is_gun() || !ammo_data() ||
        !parts->test( iteminfo_parts::AMMO_REMAINING_OR_TYPES ) ) {
        return;
    }

    const islot_ammo &ammo = *ammo_data()->ammo;
    if( !ammo.damage.empty() || ammo.force_stat_display ) {
        if( is_ammo() ) {
            info.emplace_back( "AMMO", _( "<bold>Ammunition type</bold>: " ), ammo_type()->name() );
        } else if( ammo_remaining() > 0 ) {
            info.emplace_back( "AMMO", _( "<bold>Ammunition</bold>: " ),
                               ammo_data()->nname( ammo_remaining() ) );
        }

        const std::string space = "  ";
        if( !ammo.damage.empty() && ammo.damage.damage_units.front().amount > 0 ) {
            if( parts->test( iteminfo_parts::AMMO_DAMAGE_VALUE ) ) {
                info.emplace_back( "AMMO", _( "Damage: " ), "",
                                   iteminfo::no_newline, ammo.damage.total_damage() );
            }
        } else if( parts->test( iteminfo_parts::AMMO_DAMAGE_PROPORTIONAL ) ) {
            const float multiplier = ammo.damage.empty() ? 1.0f
                                     : ammo.damage.damage_units.front().unconditional_damage_mult;
            info.emplace_back( "AMMO", _( "Damage multiplier: " ), "",
                               iteminfo::no_newline | iteminfo::is_decimal,
                               multiplier );
        }
        if( parts->test( iteminfo_parts::AMMO_DAMAGE_AP ) ) {
            info.emplace_back( "AMMO", space + _( "Armor-pierce: " ), get_ranged_pierce( ammo ) );
        }
        if( parts->test( iteminfo_parts::AMMO_DAMAGE_RANGE ) ) {
            info.emplace_back( "AMMO", _( "Range: " ), "<num>" + space,
                               iteminfo::no_newline, ammo.range );
        }
        if( ammo.range_multiplier != 1.0f && parts->test( iteminfo_parts::AMMO_DAMAGE_RANGE_MULTIPLIER ) ) {
            info.emplace_back( "AMMO", _( "Range Multiplier: " ), "<num>",
                               iteminfo::is_decimal, ammo.range_multiplier );
        }
        if( parts->test( iteminfo_parts::AMMO_DAMAGE_DISPERSION ) ) {
            info.emplace_back( "AMMO", _( "Dispersion: " ), "",
                               iteminfo::lower_is_better, ammo.dispersion );
        }
        if( parts->test( iteminfo_parts::AMMO_DAMAGE_RECOIL ) ) {
            info.emplace_back( "AMMO", _( "Recoil: " ), "",
                               iteminfo::lower_is_better | iteminfo::no_newline, ammo.recoil );
        }
        if( parts->test( iteminfo_parts::AMMO_DAMAGE_CRIT_MULTIPLIER ) ) {
            info.emplace_back( "AMMO", space + _( "Critical multiplier: " ), ammo.critical_multiplier );
        }
        if( parts->test( iteminfo_parts::AMMO_BARREL_DETAILS ) ) {
            std::string barrel_details;
            const units::length small = 150_mm;
            const units::length medium = 400_mm;
            const units::length large = 600_mm;
            const int small_damage = static_cast<int>( ammo.damage.di_considering_length(
                                         small ).total_damage() );
            const int medium_damage = static_cast<int>( ammo.damage.di_considering_length(
                                          medium ).total_damage() );
            const int large_damage = static_cast<int>( ammo.damage.di_considering_length(
                                         large ).total_damage() );
            if( small_damage != medium_damage || medium_damage != large_damage ) {
                info.emplace_back( "AMMO", _( "Damage by barrel length: " ) );
                const std::string small_string = string_format( " <info>%d %s</info>: ",
                                                 convert_length( small ),
                                                 length_units( small ) );
                info.emplace_back( "AMMO", small_string, small_damage );
                const std::string medium_string = string_format( " <info>%d %s</info>: ",
                                                  convert_length( medium ),
                                                  length_units( medium ) );
                info.emplace_back( "AMMO", medium_string, medium_damage );
                const std::string large_string = string_format( " <info>%d %s</info>: ",
                                                 convert_length( large ),
                                                 length_units( large ) );
                info.emplace_back( "AMMO", large_string, large_damage );
            }
        }
    }

    std::vector<std::string> fx;
    if( ammo.ammo_effects.count( "RECYCLED" ) &&
        parts->test( iteminfo_parts::AMMO_FX_RECYCLED ) ) {
        fx.emplace_back( _( "This ammo has been <bad>hand-loaded</bad>." ) );
    }
    if( ammo.ammo_effects.count( "MATCHHEAD" ) &&
        parts->test( iteminfo_parts::AMMO_FX_BLACKPOWDER ) ) {
        fx.emplace_back(
            _( "This ammo has been loaded with <bad>matchhead powder</bad>, and will quickly "
               "clog and rust most guns like blackpowder, but will also rarely cause the gun damage from pressure spikes." ) );
    } else if( ammo.ammo_effects.count( "BLACKPOWDER" ) &&
               parts->test( iteminfo_parts::AMMO_FX_BLACKPOWDER ) ) {
        fx.emplace_back(
            _( "This ammo has been loaded with <bad>blackpowder</bad>, and will quickly "
               "clog up most guns, and cause rust if the gun is not cleaned." ) );
    }
    if( ammo.ammo_effects.count( "NEVER_MISFIRES" ) &&
        parts->test( iteminfo_parts::AMMO_FX_CANTMISSFIRE ) ) {
        fx.emplace_back( _( "This ammo <good>never misfires</good>." ) );
    }
    if( parts->test( iteminfo_parts::AMMO_FX_RECOVER ) ) {
        for( const std::string &effect : ammo.ammo_effects ) {
            if( string_starts_with( effect, "RECOVER_" ) ) {
                ret_val<int> try_recover_chance =
                    try_parse_integer<int>( effect.substr( 8 ), false );
                if( !try_recover_chance.success() ) {
                    debugmsg( "Error parsing ammo RECOVER_ denominator: %s",
                              try_recover_chance.str() );
                    break;
                }
                int recover_chance = try_recover_chance.value();
                if( recover_chance <= 5 ) {
                    fx.emplace_back( _( "Stands a <bad>very low</bad> chance of remaining intact once fired." ) );
                } else if( recover_chance <= 10 ) {
                    fx.emplace_back( _( "Stands a <bad>low</bad> chance of remaining intact once fired." ) );
                } else if( recover_chance <= 20 ) {
                    fx.emplace_back( _( "Stands a somewhat low chance of remaining intact once fired." ) );
                } else if( recover_chance <= 30 ) {
                    fx.emplace_back( _( "Stands a <good>decent</good> chance of remaining intact once fired." ) );
                } else {
                    fx.emplace_back( _( "Stands a <good>good</good> chance of remaining intact once fired." ) );
                }
                break;
            }
        }
    }
    if( ammo.ammo_effects.count( "INCENDIARY" ) &&
        parts->test( iteminfo_parts::AMMO_FX_INCENDIARY ) ) {
        fx.emplace_back( _( "This ammo <neutral>starts fires</neutral>." ) );
    }
    if( !fx.empty() ) {
        insert_separation_line( info );
        for( const std::string &e : fx ) {
            info.emplace_back( "AMMO", e );
        }
    }
}

void item::gun_info( const item *mod, std::vector<iteminfo> &info, const iteminfo_query *parts,
                     int /* batch */, bool /* debug */ ) const
{
    const std::string space = "  ";
    const islot_gun &gun = *mod->type->gun;
    const Skill &skill = *mod->gun_skill();

    // many statistics are dependent upon loaded ammo
    // if item is unloaded (or is RELOAD_AND_SHOOT) shows approximate stats using default ammo
    const item *loaded_mod = mod;
    item tmp;
    const itype *curammo = nullptr;
    if( mod->ammo_required() && !mod->ammo_remaining() ) {
        tmp = *mod;
        itype_id default_ammo = mod->magazine_current() ? tmp.common_ammo_default() : tmp.ammo_default();
        if( !default_ammo.is_null() ) {
            tmp.ammo_set( default_ammo );
        } else if( !tmp.magazine_default().is_null() ) {
            // clear out empty magazines so put_in below doesn't error
            for( item *i : tmp.contents.all_items_top() ) {
                if( i->is_magazine() ) {
                    tmp.remove_item( *i );
                    tmp.on_contents_changed();
                }
            }

            item tmp_mag( tmp.magazine_default() );
            tmp_mag.ammo_set( tmp_mag.ammo_default() );
            tmp.put_in( tmp_mag, item_pocket::pocket_type::MAGAZINE_WELL );
        }
        loaded_mod = &tmp;
        curammo = loaded_mod->ammo_data();
        // TODO: Should this be .is_null(), rather than comparing to "none"?
        if( loaded_mod->typeId().str() == "none" || loaded_mod == nullptr ||
            curammo == nullptr ) {
            if( magazine_current() ) {
                const itype_id mag_default = magazine_current()->ammo_default();
                if( mag_default.is_null() ) {
                    debugmsg( "gun %s has magazine %s with no default ammo",
                              typeId().c_str(), magazine_current()->typeId().c_str() );
                    return;
                }
                curammo = &mag_default.obj();
            } else {
                debugmsg( "loaded a nun or ammo_data() is nullptr" );
                return;
            }
        }
        if( parts->test( iteminfo_parts::GUN_DEFAULT_AMMO ) ) {
            insert_separation_line( info );
            info.emplace_back( "GUN",
                               _( "Weapon is <bad>not loaded</bad>, so stats below assume the default ammo: " ),
                               string_format( "<stat>%s</stat>",
                                              curammo->nname( 1 ) ) );
        }
    } else {
        curammo = loaded_mod->ammo_data();
    }

    if( parts->test( iteminfo_parts::GUN_DAMAGE ) ) {
        insert_separation_line( info );
        info.emplace_back( "GUN", _( "<bold>Ranged damage</bold>: " ), "", iteminfo::no_newline,
                           mod->gun_damage( false ).total_damage() );
    }

    if( mod->ammo_required() ) {
        // ammo_damage, sum_of_damage, and ammo_mult not shown so don't need to translate.
        float dmg_mult = 1.0f;
        for( const damage_unit &dmg : curammo->ammo->damage.damage_units ) {
            dmg_mult *= dmg.unconditional_damage_mult;
        }
        if( dmg_mult != 1.0f ) {
            if( parts->test( iteminfo_parts::GUN_DAMAGE_AMMOPROP ) ) {
                info.emplace_back( "GUN", "ammo_mult", "*",
                                   iteminfo::no_newline | iteminfo::no_name | iteminfo::is_decimal, dmg_mult );
            }
        } else {
            if( parts->test( iteminfo_parts::GUN_DAMAGE_LOADEDAMMO ) ) {
                damage_instance ammo_dam = curammo->ammo->damage;
                int bullet_damage = ammo_dam.total_damage();
                if( mod->barrel_length().value() > 0 ) {
                    bullet_damage = ammo_dam.di_considering_length( mod->barrel_length() ).total_damage();
                }
                info.emplace_back( "GUN", "ammo_damage", "",
                                   iteminfo::no_newline | iteminfo::no_name |
                                   iteminfo::show_plus, bullet_damage );
            }
        }

        const int gun_damage = loaded_mod->gun_damage( true ).total_damage();
        if( damage() > 0 ) {
            item intact_mod( *loaded_mod );
            intact_mod.set_degradation( 0 );
            intact_mod.set_damage( 0 );
            const int intact_damage = intact_mod.gun_damage( true ).total_damage();
            if( intact_damage != gun_damage ) {
                const int dmg_penalty = gun_damage - intact_damage;
                info.emplace_back( "GUN", "damaged_weapon_penalty", "",
                                   iteminfo::no_newline | iteminfo::no_name, dmg_penalty );
            }
        }

        if( parts->test( iteminfo_parts::GUN_DAMAGE_TOTAL ) ) {
            info.emplace_back( "GUN", "sum_of_damage", _( " = <num>" ),
                               iteminfo::no_newline | iteminfo::no_name, gun_damage );
        }
    }
    info.back().bNewLine = true;

    if( mod->barrel_length().value() > 0 ) {
        if( parts->test( iteminfo_parts::GUN_BARRELLENGTH ) ) {
            info.emplace_back( "GUN", string_format( _( "Barrel Length: %d %s" ),
                               convert_length( mod->barrel_length() ), length_units( mod->barrel_length() ) ),
                               iteminfo::no_flags );
        }
    }

    if( mod->ammo_required() && curammo->ammo->critical_multiplier != 1.0 ) {
        if( parts->test( iteminfo_parts::AMMO_DAMAGE_CRIT_MULTIPLIER ) ) {
            info.emplace_back( "GUN", _( "Critical multiplier: " ), "<num>",
                               iteminfo::no_flags, curammo->ammo->critical_multiplier );
        }
    }

    avatar &player_character = get_avatar();
    int max_gun_range = loaded_mod->gun_range( &player_character );
    if( max_gun_range > 0 && parts->test( iteminfo_parts::GUN_MAX_RANGE ) ) {
        info.emplace_back( "GUN", _( "Maximum range: " ), "<num>", iteminfo::no_flags,
                           max_gun_range );
    }

    // TODO: This doesn't cover multiple damage types

    if( parts->test( iteminfo_parts::GUN_ARMORPIERCE ) ) {
        info.emplace_back( "GUN", _( "Armor-pierce: " ), "",
                           iteminfo::no_newline, get_ranged_pierce( gun ) );
    }
    if( mod->ammo_required() ) {
        int ammo_pierce = get_ranged_pierce( *curammo->ammo );
        // ammo_armor_pierce and sum_of_armor_pierce don't need to translate.
        if( parts->test( iteminfo_parts::GUN_ARMORPIERCE_LOADEDAMMO ) ) {
            info.emplace_back( "GUN", "ammo_armor_pierce", "",
                               iteminfo::no_newline | iteminfo::no_name |
                               iteminfo::show_plus, ammo_pierce );
        }
        if( parts->test( iteminfo_parts::GUN_ARMORPIERCE_TOTAL ) ) {
            info.emplace_back( "GUN", "sum_of_armor_pierce", _( " = <num>" ),
                               iteminfo::no_name,
                               get_ranged_pierce( gun ) + ammo_pierce );
        }
    }
    info.back().bNewLine = true;

    if( parts->test( iteminfo_parts::GUN_DISPERSION ) ) {
        info.emplace_back( "GUN", _( "Dispersion: " ), "",
                           iteminfo::no_newline | iteminfo::lower_is_better,
                           mod->gun_dispersion( false, false ) );
    }
    if( mod->ammo_required() ) {
        int ammo_dispersion = curammo->ammo->dispersion;
        // ammo_dispersion and sum_of_dispersion don't need to translate.
        if( parts->test( iteminfo_parts::GUN_DISPERSION_LOADEDAMMO ) ) {
            info.emplace_back( "GUN", "ammo_dispersion", "",
                               iteminfo::no_newline | iteminfo::lower_is_better |
                               iteminfo::no_name | iteminfo::show_plus,
                               ammo_dispersion );
        }
        if( parts->test( iteminfo_parts::GUN_DISPERSION_TOTAL ) ) {
            info.emplace_back( "GUN", "sum_of_dispersion", _( " = <num>" ),
                               iteminfo::lower_is_better | iteminfo::no_name,
                               loaded_mod->gun_dispersion( true, false ) );
        }
    }
    info.back().bNewLine = true;

    // if effective sight dispersion differs from actual sight dispersion display both
    std::pair<int, int> disp = mod->sight_dispersion( player_character );
    int act_disp = disp.first;
    int eff_disp = disp.second;
    int adj_disp = eff_disp - act_disp;
    int point_shooting_limit = player_character.point_shooting_limit( *mod );

    if( parts->test( iteminfo_parts::GUN_DISPERSION_SIGHT ) ) {
        if( point_shooting_limit <= eff_disp ) {
            info.emplace_back( "GUN", _( "Sight dispersion (point shooting): " ), "",
                               iteminfo::no_newline | iteminfo::lower_is_better,
                               point_shooting_limit );
        } else {
            info.emplace_back( "GUN", _( "Sight dispersion: " ), "",
                               iteminfo::no_newline | iteminfo::lower_is_better,
                               act_disp );

            if( adj_disp ) {
                info.emplace_back( "GUN", "sight_adj_disp", "",
                                   iteminfo::no_newline | iteminfo::lower_is_better |
                                   iteminfo::no_name | iteminfo::show_plus, adj_disp );
                info.emplace_back( "GUN", "sight_eff_disp", _( " = <num>" ),
                                   iteminfo::lower_is_better | iteminfo::no_name,
                                   eff_disp );
            }
        }
    }

    bool bipod = mod->has_flag( flag_BIPOD );

    if( loaded_mod->gun_recoil( player_character ) ) {
        if( parts->test( iteminfo_parts::GUN_RECOIL ) ) {
            info.emplace_back( "GUN", _( "Effective recoil: " ), "",
                               iteminfo::no_newline | iteminfo::lower_is_better,
                               loaded_mod->gun_recoil( player_character ) );
        }
        if( bipod && parts->test( iteminfo_parts::GUN_RECOIL_BIPOD ) ) {
            info.emplace_back( "GUN", "bipod_recoil", _( " (with bipod <num>)" ),
                               iteminfo::lower_is_better | iteminfo::no_name,
                               loaded_mod->gun_recoil( player_character, true ) );
        }
    }
    info.back().bNewLine = true;

    std::map<gun_mode_id, gun_mode> fire_modes = mod->gun_all_modes();
    if( std::any_of( fire_modes.begin(), fire_modes.end(),
    []( const std::pair<gun_mode_id, gun_mode> &e ) {
    return e.second.qty > 1 && !e.second.melee();
    } ) ) {
        info.emplace_back( "GUN", _( "Recommended strength (burst): " ), "",
                           iteminfo::lower_is_better, std::ceil( mod->type->weight / 333.0_gram ) );
    }

    if( parts->test( iteminfo_parts::GUN_RELOAD_TIME ) ) {
        info.emplace_back( "GUN", _( "Reload time: " ),
                           has_flag( flag_RELOAD_ONE ) ? _( "<num> moves per round" ) :
                           _( "<num> moves" ),
                           iteminfo::lower_is_better,  mod->get_reload_time() );
    }

    if( parts->test( iteminfo_parts::GUN_CURRENT_LOUDNESS ) ) {
        const bool is_default_fire_mode = loaded_mod->gun_current_mode().tname() == "DEFAULT";
        //if empty, use the temporary gun loaded with default ammo
        const item::sound_data data = ( mod->ammo_required() &&
                                        !mod->ammo_remaining() ) ? tmp.gun_noise( is_default_fire_mode ) : loaded_mod->gun_noise(
                                          is_default_fire_mode );
        const int loudness = data.volume;
        info.emplace_back( "GUN", _( "Loudness with current fire mode: " ), "", iteminfo::lower_is_better,
                           loudness );
    }

    if( parts->test( iteminfo_parts::GUN_USEDSKILL ) ) {
        info.emplace_back( "GUN", _( "Skill used: " ),
                           "<info>" + skill.name() + "</info>" );
    }

    if( mod->magazine_integral() || mod->magazine_current() ) {
        if( mod->magazine_current() && parts->test( iteminfo_parts::GUN_MAGAZINE ) ) {
            info.emplace_back( "GUN", _( "Magazine: " ),
                               string_format( "<stat>%s</stat>",
                                              mod->magazine_current()->tname() ) );
        }
        if( !mod->ammo_types().empty() && parts->test( iteminfo_parts::GUN_CAPACITY ) ) {
            for( const ammotype &at : mod->ammo_types() ) {
                const std::string fmt = string_format( n_gettext( "<num> round of %s",
                                                       "<num> rounds of %s",
                                                       mod->ammo_capacity( at ) ), at->name() );
                info.emplace_back( "GUN", _( "Capacity: " ), fmt, iteminfo::no_flags,
                                   mod->ammo_capacity( at ) );
                std::string ammo_details = print_ammo( at );
                if( !ammo_details.empty() && !magazine_integral() ) {
                    info.emplace_back( "GUN", _( "Variants: " ), ammo_details, iteminfo::no_flags );
                }
            }
        }
    } else if( parts->test( iteminfo_parts::GUN_TYPE ) ) {
        const std::set<ammotype> types_of_ammo = mod->ammo_types();
        if( !types_of_ammo.empty() ) {
            info.emplace_back( "GUN", _( "Type: " ), enumerate_as_string( types_of_ammo,
            []( const ammotype & at ) {
                return at->name();
            }, enumeration_conjunction::none ) );
        }
    }

    if( mod->ammo_data() && parts->test( iteminfo_parts::AMMO_REMAINING ) ) {
        info.emplace_back( "AMMO", _( "Ammunition: " ), string_format( "<stat>%s</stat>",
                           mod->ammo_data()->nname( mod->ammo_remaining() ) ) );
    }

    if( mod->ammo_required() > 1 && parts->test( iteminfo_parts::AMMO_TO_FIRE ) ) {
        info.emplace_back( "AMMO",  _( "Ammunition consumed per shot: " ), "",
                           iteminfo::lower_is_better, mod->ammo_required() );
    }

    if( mod->get_gun_energy_drain() > 0_kJ && parts->test( iteminfo_parts::AMMO_UPSCOST ) ) {
        info.emplace_back( "AMMO", _( "Energy per shot: " ), string_format( "<stat>%s</stat>",
                           units::display( mod->get_gun_energy_drain() ) ) );
    }

    if( parts->test( iteminfo_parts::GUN_AIMING_STATS ) ) {
        insert_separation_line( info );
        info.emplace_back( "GUN", _( "<bold>Base aim speed</bold>: " ), "<num>", iteminfo::no_flags,
                           player_character.aim_per_move( *mod, MAX_RECOIL ) );
        for( const aim_type &type : player_character.get_aim_types( *mod ) ) {
            // Nameless and immediate aim levels don't get an entry.
            if( type.name.empty() || type.threshold == static_cast<int>( MAX_RECOIL ) ) {
                continue;
            }
            // For item comparison to work correctly each info object needs a
            // distinct tag per aim type.
            const std::string tag = "GUN_" + type.name;
            info.emplace_back( tag, string_format( "<info>%s</info>", type.name ) );
            int max_dispersion = player_character.get_weapon_dispersion( *loaded_mod ).max();
            int range = range_with_even_chance_of_good_hit( max_dispersion + type.threshold );
            info.emplace_back( tag, _( "Even chance of good hit at range: " ),
                               _( "<num>" ), iteminfo::no_flags, range );
            int aim_mv = player_character.gun_engagement_moves( *mod, type.threshold );
            info.emplace_back( tag, _( "Time to reach aim level: " ), _( "<num> moves" ),
                               iteminfo::lower_is_better, aim_mv );
        }
    }

    if( parts->test( iteminfo_parts::GUN_FIRE_MODES ) ) {
        std::vector<std::string> fm;
        for( const std::pair<const gun_mode_id, gun_mode> &e : fire_modes ) {
            if( e.second.target == this && !e.second.melee() ) {
                fm.emplace_back( string_format( "%s (%i)", e.second.tname(), e.second.qty ) );
            }
        }
        if( !fm.empty() ) {
            insert_separation_line( info );
            info.emplace_back( "GUN", _( "<bold>Fire modes</bold>: " ) + enumerate_as_string( fm ) );
        }
    }

    if( !magazine_integral() && parts->test( iteminfo_parts::GUN_ALLOWED_MAGAZINES ) ) {
        insert_separation_line( info );
        if( uses_magazine() ) {
            // Keep this identical with tool magazines in item::tool_info
            const std::vector<itype_id> compat_sorted = sorted_lex( magazine_compatible() );
            const std::string mag_names = enumerate_as_string( compat_sorted,
            []( const itype_id & id ) {
                return item::nname( id );
            } );

            const std::set<flag_id> flag_restrictions = contents.magazine_flag_restrictions();
            const std::string flag_names = enumerate_as_string( flag_restrictions,
            []( const flag_id & e ) {
                return e->name();
            } );

            std::string display = _( "<bold>Compatible magazines</bold>:" );
            if( !compat_sorted.empty() ) {
                display += _( "\n<bold>Types</bold>: " ) + mag_names;
            }
            if( !flag_restrictions.empty() ) {
                display += _( "\n<bold>Form factors</bold>: " ) + flag_names;
            }

            info.emplace_back( "DESCRIPTION", display );
        }
    }

    if( !gun.valid_mod_locations.empty() && parts->test( iteminfo_parts::DESCRIPTION_GUN_MODS ) ) {
        insert_separation_line( info );

        std::string mod_str = _( "<bold>Mods</bold>:" );

        std::map<gunmod_location, int> mod_locations = get_mod_locations();

        int iternum = 0;
        mod_str += "\n";
        for( std::pair<const gunmod_location, int> &elem : mod_locations ) {
            if( iternum != 0 ) {
                mod_str += "\n";
            }
            const int free_slots = elem.second - get_free_mod_locations( elem.first );
            mod_str += string_format( "<color_cyan># </color>%s:", elem.first.name() );

            for( const item *gmod : gunmods() ) {
                if( gmod->type->gunmod->location == elem.first ) { // if mod for this location
                    mod_str +=
                        string_format( "\n    <stat>[</stat><color_light_green>● </color><stat>%s]</stat>",
                                       gmod->tname( 1, false ) );
                }
            }
            for( int i = free_slots + 1 ; i <= elem.second ; i++ ) {
                if( i == free_slots + 1 && i <= elem.second ) {
                    mod_str += string_format( "\n   " );
                }
                mod_str += string_format( " <color_dark_gray>[-%s-]</color>", _( "empty" ) );
            }
            iternum++;
        }
        info.emplace_back( "DESCRIPTION", mod_str );
    }

    if( mod->casings_count() && parts->test( iteminfo_parts::DESCRIPTION_GUN_CASINGS ) ) {
        insert_separation_line( info );
        std::string tmp = n_gettext( "Contains <stat>%i</stat> casing",
                                     "Contains <stat>%i</stat> casings", mod->casings_count() );
        info.emplace_back( "DESCRIPTION", string_format( tmp, mod->casings_count() ) );
    }

    if( is_gun() && has_flag( flag_FIRE_TWOHAND ) &&
        parts->test( iteminfo_parts::DESCRIPTION_TWOHANDED ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This weapon needs <info>two free hands</info> to fire." ) );
    }
}

void item::gunmod_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /* batch */,
                        bool /* debug */ ) const
{
    if( !is_gunmod() ) {
        return;
    }
    const islot_gunmod &mod = *type->gunmod;

    if( is_gun() && parts->test( iteminfo_parts::DESCRIPTION_GUNMOD ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This mod <info>must be attached to a gun</info>, "
                              "it can not be fired separately." ) );
    }
    if( has_flag( flag_REACH_ATTACK ) && parts->test( iteminfo_parts::DESCRIPTION_GUNMOD_REACH ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "When attached to a gun, <good>allows</good> making "
                              "<info>reach melee attacks</info> with it." ) );
    }

    if( is_gunmod() && has_flag( flag_DISABLE_SIGHTS ) &&
        parts->test( iteminfo_parts::DESCRIPTION_GUNMOD_DISABLESSIGHTS ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This mod <bad>obscures sights</bad> of the "
                              "base weapon." ) );
    }

    if( is_gunmod() && has_flag( flag_CONSUMABLE ) &&
        parts->test( iteminfo_parts::DESCRIPTION_GUNMOD_CONSUMABLE ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This mod might <bad>suffer wear</bad> when firing "
                              "the base weapon." ) );
    }

    if( mod.dispersion != 0 && parts->test( iteminfo_parts::GUNMOD_DISPERSION ) ) {
        info.emplace_back( "GUNMOD", _( "Dispersion modifier: " ), "",
                           iteminfo::lower_is_better | iteminfo::show_plus,
                           mod.dispersion );
    }
    if( mod.sight_dispersion != -1 && parts->test( iteminfo_parts::GUNMOD_DISPERSION_SIGHT ) ) {
        info.emplace_back( "GUNMOD", _( "Sight dispersion: " ), "",
                           iteminfo::lower_is_better, mod.sight_dispersion );
    }
    if( mod.field_of_view > 0 && parts->test( iteminfo_parts::GUNMOD_FIELD_OF_VIEW ) ) {
        if( mod.field_of_view >= MAX_RECOIL ) {
            info.emplace_back( "GUNMOD", _( "Field of view: <good>No limit</good>" ) );
        } else {
            info.emplace_back( "GUNMOD", _( "Field of view: " ), "",
                               iteminfo::no_flags, mod.field_of_view );
        }
    }
    if( mod.field_of_view > 0 && parts->test( iteminfo_parts::GUNMOD_AIM_SPEED_MODIFIER ) ) {
        info.emplace_back( "GUNMOD", _( "Aim speed modifier: " ), "",
                           iteminfo::no_flags, mod.aim_speed_modifier );
    }
    int total_damage = static_cast<int>( mod.damage.total_damage() );
    if( total_damage != 0 && parts->test( iteminfo_parts::GUNMOD_DAMAGE ) ) {
        info.emplace_back( "GUNMOD", _( "Damage: " ), "", iteminfo::show_plus,
                           total_damage );
    }
    int pierce = get_ranged_pierce( mod );
    if( get_ranged_pierce( mod ) != 0 && parts->test( iteminfo_parts::GUNMOD_ARMORPIERCE ) ) {
        info.emplace_back( "GUNMOD", _( "Armor-pierce: " ), "", iteminfo::show_plus,
                           pierce );
    }
    if( mod.range != 0 && parts->test( iteminfo_parts::GUNMOD_RANGE ) ) {
        info.emplace_back( "GUNMOD", _( "Range: " ), "",
                           iteminfo::show_plus | iteminfo::no_newline, mod.range );
    }
    if( mod.range_multiplier != 1.0f && parts->test( iteminfo_parts::GUNMOD_RANGE_MULTIPLIER ) ) {
        info.emplace_back( "GUNMOD", _( "Range Multiplier: " ), "",
                           iteminfo::is_decimal, mod.range_multiplier );
    }
    if( mod.handling != 0 && parts->test( iteminfo_parts::GUNMOD_HANDLING ) ) {
        info.emplace_back( "GUNMOD", _( "Handling modifier: " ), "",
                           iteminfo::show_plus, mod.handling );
    }
    if( mod.loudness != 0 && parts->test( iteminfo_parts::GUNMOD_LOUDNESS_MODIFIER ) ) {
        info.emplace_back( "GUNMOD", _( "Loudness modifier: " ), "",
                           iteminfo::lower_is_better | iteminfo::show_plus,
                           mod.loudness );
    }
    if( !type->mod->ammo_modifier.empty() && parts->test( iteminfo_parts::GUNMOD_AMMO ) ) {
        for( const ammotype &at : type->mod->ammo_modifier ) {
            info.emplace_back( "GUNMOD", string_format( _( "Ammo: <stat>%s</stat>" ),
                               at->name() ) );
        }
    }
    if( mod.reload_modifier != 0 && parts->test( iteminfo_parts::GUNMOD_RELOAD ) ) {
        info.emplace_back( "GUNMOD", _( "Reload modifier: " ), _( "<num>%" ),
                           iteminfo::lower_is_better, mod.reload_modifier );
    }
    if( mod.min_str_required_mod > 0 && parts->test( iteminfo_parts::GUNMOD_STRENGTH ) ) {
        info.emplace_back( "GUNMOD", _( "Minimum strength required modifier: " ),
                           mod.min_str_required_mod );
    }
    if( !mod.add_mod.empty() && parts->test( iteminfo_parts::GUNMOD_ADD_MOD ) ) {
        insert_separation_line( info );

        std::string mod_loc_str = _( "<bold>Adds mod locations: </bold> " );

        std::map<gunmod_location, int> mod_locations = mod.add_mod;

        int iternum = 0;
        for( std::pair<const gunmod_location, int> &elem : mod_locations ) {
            if( iternum != 0 ) {
                mod_loc_str += "; ";
            }
            mod_loc_str += string_format( "<bold>%s</bold> %s", elem.second, elem.first.name() );
            iternum++;
        }
        mod_loc_str += ".";
        info.emplace_back( "GUNMOD", mod_loc_str );
    }

    insert_separation_line( info );

    if( parts->test( iteminfo_parts::GUNMOD_USEDON ) ) {
        std::set<std::string> used_on_translated; // uses set to prune duplicates
        for( const gun_type_type &gtt : mod.usable ) {
            used_on_translated.emplace( string_format( "<info>%s</info>", gtt.name() ) );
        }
        info.emplace_back( "GUNMOD", _( "Used on: " ) + enumerate_as_string( used_on_translated ) );
    }

    if( parts->test( iteminfo_parts::GUNMOD_LOCATION ) ) {
        info.emplace_back( "GUNMOD", string_format( _( "Location: %s" ), mod.location.name() ) );
    }

    if( !mod.blacklist_mod.empty() && parts->test( iteminfo_parts::GUNMOD_BLACKLIST_MOD ) ) {
        std::string mod_black_str = _( "<bold>Incompatible with mod location: </bold> " );

        int iternum = 0;
        for( const gunmod_location &black : mod.blacklist_mod ) {
            if( iternum != 0 ) {
                mod_black_str += ", ";
            }
            mod_black_str += string_format( "%s", black.name() );
            iternum++;
        }
        mod_black_str += ".";
        info.emplace_back( "GUNMOD", mod_black_str );
    }
}

static bool armor_encumb_header_info( const item &it, std::vector<iteminfo> &info )
{
    std::string format;
    Character &player_character = get_player_character();
    const bool sizing_matters = it.get_sizing( player_character ) != item::sizing::ignore;

    if( it.has_flag( flag_FIT ) ) {
        format = _( " <info>(fits)</info>" );
    } else if( it.has_flag( flag_VARSIZE ) && sizing_matters ) {
        format = _( " <bad>(poor fit)</bad>" );
    }
    if( sizing_matters ) {
        const item::sizing sizing_level = it.get_sizing( player_character );
        //If we have the wrong size, we do not fit so alert the player
        if( sizing_level == item::sizing::human_sized_small_char ) {
            format = _( " <bad>(too big)</bad>" );
        } else if( sizing_level == item::sizing::big_sized_small_char ) {
            format = _( " <bad>(huge!)</bad>" );
        } else if( sizing_level == item::sizing::small_sized_human_char ||
                   sizing_level == item::sizing::human_sized_big_char ) {
            format = _( " <bad>(too small)</bad>" );
        } else if( sizing_level == item::sizing::small_sized_big_char ) {
            format = _( " <bad>(tiny!)</bad>" );
        }
    }
    if( format.empty() ) {
        return false;
    }

    //~ The size or fit of an article of clothing (fits / poor fit)
    info.emplace_back( "ARMOR", _( "<bold>Size/Fit</bold>:" ) + format );
    return true;
}

bool item::armor_full_protection_info( std::vector<iteminfo> &info,
                                       const iteminfo_query *parts ) const
{
    bool divider_needed = false;
    const std::string space = "  ";

    bool ret = false;
    if( const islot_armor *t = find_armor_data() ) {
        if( t->data.empty() ) {
            return ret;
        }

        for( const armor_portion_data &p : t->sub_data ) {
            if( divider_needed ) {
                insert_separation_line( info );
            }

            std::vector<sub_bodypart_id> covered( p.sub_coverage.size() );
            std::transform( p.sub_coverage.begin(), p.sub_coverage.end(),
            covered.begin(), []( sub_bodypart_str_id a ) {
                return a.id();
            } );
            std::set<translation, localized_comparator> to_print = body_part_type::consolidate( covered );
            std::string coverage = _( "<bold>Protection for</bold>:" );
            for( const translation &entry : to_print ) {
                coverage += string_format( _( " The <info>%s</info>." ), entry );
            }
            info.emplace_back( "ARMOR", coverage );

            // the following function need one representative sub limb from which to query data
            armor_material_info( info, parts, 0, false, *p.sub_coverage.begin() );
            armor_attribute_info( info, parts, 0, false, *p.sub_coverage.begin() );
            armor_protection_info( info, parts, 0, false, *p.sub_coverage.begin() );
            ret = true;
            divider_needed = true;
        }
    } else if( is_gun() && has_flag( flag_IS_ARMOR ) ) {
        if( divider_needed ) {
            insert_separation_line( info );
        }
        //right now all eligible gunmods (shoulder_strap, belt_clip) have the is_armor flag and use the torso
        info.emplace_back( "ARMOR", _( "Torso:" ) + space, "",
                           iteminfo::no_flags | iteminfo::lower_is_better, get_avg_encumber( get_avatar() ) );

        //~ Limb-specific coverage (for guns strapped to torso)
        info.emplace_back( "DESCRIPTION", _( "<bold>Torso coverage</bold>:" ) );
        //~ Regular/Default coverage
        info.emplace_back( "ARMOR", space + _( "Default:" ) + space, "",
                           iteminfo::no_flags, get_coverage( body_part_torso.id() ) );
        //~ Melee coverage
        info.emplace_back( "ARMOR", space + _( "Melee:" ) + space, "",
                           iteminfo::no_flags, get_coverage( body_part_torso.id(), cover_type::COVER_MELEE ) );
        //~ Ranged coverage
        info.emplace_back( "ARMOR", space + _( "Ranged:" ) + space, "",
                           iteminfo::no_flags, get_coverage( body_part_torso.id(), cover_type::COVER_RANGED ) );
        //~ Vitals coverage
        info.emplace_back( "ARMOR", space + _( "Vitals:" ) + space, "",
                           iteminfo::no_flags, get_coverage( body_part_torso.id(), cover_type::COVER_VITALS ) );
    }

    return ret;
}

static void armor_protect_dmg_info( int dmg, std::vector<iteminfo> &info )
{
    if( dmg > 0 ) {
        info.emplace_back( "ARMOR", _( "Protection values are <bad>reduced by damage</bad> and "
                                       "you may be able to <info>improve them by repairing this "
                                       "item</info>." ) );
    }
}

void item::armor_protection_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                                  int /*batch*/, bool /*debug*/, const sub_bodypart_id &sbp ) const
{
    if( !is_armor() && !is_pet_armor() ) {
        return;
    }

    if( parts->test( iteminfo_parts::ARMOR_PROTECTION ) ) {
        const std::string space = "  ";
        // NOLINTNEXTLINE(cata-translate-string-literal)
        std::string bp_cat = string_format( "ARMOR" );
        std::string layering;

        // get the layers this bit of the armor covers if its unique compared to the rest of the armor
        for( const layer_level &ll : get_layer( sbp ) ) {
            switch( ll ) {
                case layer_level::PERSONAL:
                    layering += _( " <stat>Personal aura</stat>." );
                    break;
                case layer_level::SKINTIGHT:
                    layering += _( " <stat>Close to skin</stat>." );
                    break;
                case layer_level::NORMAL:
                    layering += _( " <stat>Normal</stat>." );
                    break;
                case layer_level::WAIST:
                    layering += _( " <stat>Waist</stat>." );
                    break;
                case layer_level::OUTER:
                    layering += _( " <stat>Outer</stat>." );
                    break;
                case layer_level::BELTED:
                    layering += _( " <stat>Strapped</stat>." );
                    break;
                case layer_level::AURA:
                    layering += _( " <stat>Outer aura</stat>." );
                    break;
                default:
                    layering += _( " Should never see this." );
            }
        }
        //~ Limb-specific coverage (%s = name of limb)
        info.emplace_back( "DESCRIPTION", string_format( _( "<bold>Coverage</bold>:%s" ), layering ) );
        //~ Regular/Default coverage
        info.emplace_back( bp_cat, string_format( "%s%s%s", space, _( "Default:" ), space ), "",
                           iteminfo::no_flags, get_coverage( sbp ) );
        if( get_coverage( sbp ) != get_coverage( sbp, item::cover_type::COVER_MELEE ) ) {
            //~ Melee coverage
            info.emplace_back( bp_cat, string_format( "%s%s%s", space, _( "Melee:" ), space ), "",
                               iteminfo::no_flags, get_coverage( sbp, item::cover_type::COVER_MELEE ) );
        }
        if( get_coverage( sbp ) != get_coverage( sbp, item::cover_type::COVER_RANGED ) ) {
            //~ Ranged coverage
            info.emplace_back( bp_cat, string_format( "%s%s%s", space, _( "Ranged:" ), space ), "",
                               iteminfo::no_flags, get_coverage( sbp, item::cover_type::COVER_RANGED ) );
        }
        if( get_coverage( sbp, item::cover_type::COVER_VITALS ) > 0 ) {
            //~ Vitals coverage
            info.emplace_back( bp_cat, string_format( "%s%s%s", space, _( "Vitals:" ), space ), "",
                               iteminfo::no_flags, get_coverage( sbp, item::cover_type::COVER_VITALS ) );
        }

        bool printed_any = false;

        // gather all the protection data
        // the rolls are basically a perfect hit for protection and a
        // worst possible
        resistances worst_res = resistances( *this, false, 99, sbp );
        resistances best_res = resistances( *this, false, 0, sbp );
        resistances median_res = resistances( *this, false, 50, sbp );

        int percent_best = 100;
        int percent_worst = 0;
        const armor_portion_data *portion = portion_for_bodypart( sbp );
        // if there isn't a portion this is probably pet armor
        if( portion ) {
            percent_best = portion->best_protection_chance;
            percent_worst = portion->worst_protection_chance;
        }

        bool display_median = percent_best < 50 && percent_worst < 50;

        if( display_median ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( "<bold>%s</bold>: <bad>%d%%</bad>, <color_c_yellow>Median</color>, <good>%d%%</good>",
                                              _( "Protection" ), percent_worst, percent_best ) );
        } else if( percent_worst > 0 ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( "<bold>%s</bold>: <bad>%d%%</bad>, <good>%d%%</good>", _( "Protection" ),
                                              percent_worst, percent_best ) );
        } else {
            info.emplace_back( "DESCRIPTION", string_format( "<bold>%s</bold>:", _( "Protection" ) ) );
        }

        for( const damage_info_order &dio : damage_info_order::get_all(
                 damage_info_order::info_type::PROT ) ) {
            if( best_res.resist_vals.count( dio.dmg_type ) <= 0 ||
                best_res.type_resist( dio.dmg_type ) < 1.0f ) {
                continue;
            }
            bool skipped_detailed = false;
            if( dio.info_display == damage_info_order::info_disp::DETAILED ) {
                if( display_median ) {
                    info.emplace_back( bp_cat,
                                       string_format( "%s%s:  <bad>%.2f</bad>, <color_c_yellow>%.2f</color>, <good>%.2f</good>", space,
                                                      uppercase_first_letter( dio.dmg_type->name.translated() ), worst_res.type_resist( dio.dmg_type ),
                                                      median_res.type_resist( dio.dmg_type ), best_res.type_resist( dio.dmg_type ) ), "",
                                       iteminfo::no_flags );
                    printed_any = true;
                } else if( percent_worst > 0 ) {
                    info.emplace_back( bp_cat, string_format( "%s%s:  <bad>%.2f</bad>, <good>%.2f</good>", space,
                                       uppercase_first_letter( dio.dmg_type->name.translated() ),
                                       worst_res.type_resist( dio.dmg_type ), best_res.type_resist( dio.dmg_type ) ), "",
                                       iteminfo::no_flags );
                    printed_any = true;
                } else {
                    skipped_detailed = true;
                }
            }
            if( skipped_detailed || dio.info_display == damage_info_order::info_disp::BASIC ) {
                info.emplace_back( bp_cat, string_format( "%s%s: ", space,
                                   uppercase_first_letter( dio.dmg_type->name.translated() ) ), "",
                                   iteminfo::is_decimal, best_res.type_resist( dio.dmg_type ) );
                printed_any = true;
            }
        }
        if( get_base_env_resist( *this ) >= 1 ) {
            info.emplace_back( bp_cat, string_format( "%s%s", space, _( "Environmental: " ) ),
                               get_base_env_resist( *this ) );
            printed_any = true;
        }
        // if we haven't printed any armor data acknowledge that
        if( !printed_any ) {
            info.emplace_back( bp_cat, string_format( "%s%s", space, _( "Negligible Protection" ) ) );
        }
        if( type->can_use( "GASMASK" ) || type->can_use( "DIVE_TANK" ) ) {
            info.emplace_back( "ARMOR", string_format( "<bold>%s</bold>:",
                               _( "Protection when active" ) ) );
            info.emplace_back( bp_cat, space + _( "Acid: " ), "",
                               iteminfo::no_newline | iteminfo::is_decimal,
                               resist( damage_acid, false, sbp, get_base_env_resist_w_filter() ) );
            info.emplace_back( bp_cat, space + _( "Fire: " ), "",
                               iteminfo::no_newline | iteminfo::is_decimal,
                               resist( damage_heat, false, sbp, get_base_env_resist_w_filter() ) );
            info.emplace_back( bp_cat, space + _( "Environmental: " ),
                               get_env_resist( get_base_env_resist_w_filter() ) );
        }

        if( sbp == sub_bodypart_id() && damage() > 0 ) {
            armor_protect_dmg_info( damage(), info );
        }
    }
}
void item::armor_material_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                                int /*batch*/, bool /*debug*/, const sub_bodypart_id &sbp ) const
{
    if( !is_armor() && !is_pet_armor() ) {
        return;
    }

    if( parts->test( iteminfo_parts::ARMOR_MATERIALS ) ) {
        const std::string space = "  ";
        // NOLINTNEXTLINE(cata-translate-string-literal)
        std::string bp_cat = string_format( "ARMOR" );
        std::string materials_list;
        std::string units_info = pgettext( "length unit: milimeter", "mm" );
        for( const part_material *mat : armor_made_of( sbp ) ) {

            materials_list.append( string_format( "  %s %.2f%s (%d%%).", mat->id->name(), mat->thickness,
                                                  units_info, mat->cover ) );
        }

        info.emplace_back( bp_cat, string_format( "%s%s", _( "<bold>Materials</bold>:" ),
                           materials_list ), "", iteminfo::no_flags );
    }
}

void item::armor_attribute_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                                 int /*batch*/, bool /*debug*/, const sub_bodypart_id &sbp ) const
{
    if( !is_armor() && !is_pet_armor() ) {
        return;
    }

    if( parts->test( iteminfo_parts::ARMOR_ADDITIONAL_INFO ) ) {
        const std::string space = "  ";
        // NOLINTNEXTLINE(cata-translate-string-literal)
        std::string bp_cat = string_format( "ARMOR" );

        if( const armor_portion_data *portion_data = portion_for_bodypart( sbp ) ) {
            if( portion_data->rigid_layer_only ) {
                info.emplace_back( bp_cat, _( "<info>Rigid items only conflict on shared layers</info>" ),
                                   "", iteminfo::no_flags );
            }
        }
    }
}

void item::pet_armor_protection_info( std::vector<iteminfo> &info,
                                      const iteminfo_query *parts ) const
{
    if( !is_pet_armor() ) {
        return;
    }

    if( parts->test( iteminfo_parts::ARMOR_PROTECTION ) ) {
        const std::string space = "  ";

        info.emplace_back( "ARMOR", _( "<bold>Protection</bold>:" ), "" );

        for( const damage_info_order &dio : damage_info_order::get_all(
                 damage_info_order::info_type::PET ) ) {
            info.emplace_back( "ARMOR", string_format( "%s%s: ", space,
                               uppercase_first_letter( dio.dmg_type->name.translated() ) ), "",
                               iteminfo::is_decimal, resist( dio.dmg_type ) );
        }

        info.emplace_back( "ARMOR", space + _( "Environmental: " ),
                           get_base_env_resist( *this ) );

        if( damage() > 0 ) {
            info.emplace_back( "ARMOR",
                               _( "Protection values are <bad>reduced by damage</bad> and "
                                  "you may be able to <info>improve them by repairing this "
                                  "item</info>." ) );
        }
    }
}

// simple struct used for organizing encumberance in an ordered set
struct armor_encumb_data {
    int encumb;
    int encumb_max;
    int encumb_min;

    bool operator==( const armor_encumb_data &a ) const {
        return encumb == a.encumb &&
               encumb_min == a.encumb_min &&
               encumb_max == a.encumb_max;
    }

    armor_encumb_data( int in_encumb_min, int in_encumb, int in_encumb_max ) {
        encumb = in_encumb;
        encumb_min = in_encumb_min;
        encumb_max = in_encumb_max;
    }
};

static bool operator<( const armor_encumb_data &lhs, const armor_encumb_data &rhs )
{
    return lhs.encumb < rhs.encumb;
}

void item::armor_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                       bool debug ) const
{
    if( !is_armor() ) {
        return;
    }

    const std::string space = "  ";
    body_part_set covered_parts = get_covered_body_parts();
    bool covers_anything = covered_parts.any();
    const bool show_bodygraph = get_option<bool>( "ITEM_BODYGRAPH" ) &&
                                parts->test( iteminfo_parts::ARMOR_BODYGRAPH );

    if( parts->test( iteminfo_parts::ARMOR_BODYPARTS ) ) {
        insert_separation_line( info );
        std::vector<sub_bodypart_id> covered = get_covered_sub_body_parts();
        std::string coverage = _( "<bold>Covers</bold>:" );

        if( !covered.empty() ) {
            std::set<translation, localized_comparator> to_print = body_part_type::consolidate( covered );
            for( const translation &entry : to_print ) {
                coverage += string_format( _( " The <info>%s</info>." ), entry );
            }
        }

        if( !covers_anything ) {
            coverage += _( " <info>Nothing</info>." );
        }

        info.emplace_back( "ARMOR", coverage );
    }

    if( parts->test( iteminfo_parts::ARMOR_RIGIDITY ) && ( is_rigid() || is_ablative() ) ) {
        // if the item has no armor data it doesn't cover that part
        const islot_armor *armor = find_armor_data();
        if( armor != nullptr ) {
            if( armor->rigid ) {
                std::string coverage = _( "<bold>This armor is rigid</bold>" );
                info.emplace_back( "ARMOR", coverage );
            } else {
                // only some parts are rigid
                std::string coverage = _( "<bold>Rigid Locations</bold>:" );
                std::set<sub_bodypart_id> covered;
                // if ablative check if there are hard plates in locations
                if( armor->ablative ) {
                    // if item has ablative armor we should check those too.
                    for( const item_pocket *pocket : get_all_contained_pockets() ) {
                        // if the pocket is ablative and not empty we should use its values
                        if( pocket->get_pocket_data()->ablative && !pocket->empty() ) {
                            // get the contained plate
                            const item &ablative_armor = pocket->front();
                            for( const armor_portion_data &entry : ablative_armor.find_armor_data()->sub_data ) {
                                if( entry.rigid ) {
                                    for( const sub_bodypart_str_id &sbp : entry.sub_coverage ) {
                                        covered.emplace( sbp );
                                    }
                                }
                            }
                        }
                    }
                }
                for( const armor_portion_data &entry : armor->sub_data ) {
                    if( entry.rigid ) {
                        for( const sub_bodypart_str_id &sbp : entry.sub_coverage ) {
                            covered.emplace( sbp );
                        }
                    }
                }

                if( !covered.empty() ) {
                    std::vector<sub_bodypart_id> covered_vec( covered.begin(), covered.end() );
                    std::set<translation, localized_comparator> to_print = body_part_type::consolidate( covered_vec );
                    for( const translation &entry : to_print ) {
                        coverage += string_format( _( " The <info>%s</info>." ), entry );
                    }
                    info.emplace_back( "ARMOR", coverage );
                }
            }
        }
    }

    if( parts->test( iteminfo_parts::ARMOR_RIGIDITY ) && is_comfortable() ) {
        // if the item has no armor data it doesn't cover that part
        const islot_armor *armor = find_armor_data();
        if( armor != nullptr ) {
            if( armor->comfortable ) {
                std::string coverage = _( "<bold>This armor is comfortable</bold>" );
                info.emplace_back( "ARMOR", coverage );
            } else {
                // only some parts are comfortable
                std::string coverage = _( "<bold>Comfortable Locations</bold>:" );
                std::vector<sub_bodypart_id> covered;
                for( const armor_portion_data &entry : armor->sub_data ) {
                    if( entry.comfortable ) {
                        for( const sub_bodypart_str_id &sbp : entry.sub_coverage ) {
                            covered.emplace_back( sbp );
                        }
                    }
                }
                if( !covered.empty() ) {
                    std::set<translation, localized_comparator> to_print = body_part_type::consolidate( covered );
                    for( const translation &entry : to_print ) {
                        coverage += string_format( _( " The <info>%s</info>." ), entry );
                    }
                    info.emplace_back( "ARMOR", coverage );
                }
            }
        }
    }

    if( parts->test( iteminfo_parts::ARMOR_COVERAGE ) && covers_anything ) {
        std::map<int, std::vector<bodypart_id>> limb_groups;
        for( const bodypart_str_id &bp : get_covered_body_parts() ) {
            const armor_portion_data *portion = portion_for_bodypart( bp );
            if( portion ) {
                limb_groups[portion->coverage].emplace_back( bp );
            }
        }
        // keep on one line if only 1 entry
        if( limb_groups.size() == 1 ) {
            info.emplace_back( "ARMOR", _( "<bold>Total Coverage</bold>" ), iteminfo::no_newline );
        } else {
            info.emplace_back( "ARMOR", _( "<bold>Total Coverage</bold>:" ) );
        }

        for( auto &entry : limb_groups ) {
            std::set<translation, localized_comparator> to_print = body_part_type::consolidate( entry.second );
            std::string coverage;
            for( const translation &entry : to_print ) {
                coverage += string_format( _( " The <info>%s</info>." ), entry );
            }
            info.emplace_back( "ARMOR", "", string_format( "  <num>%%:%s", coverage ), iteminfo::no_flags,
                               entry.first );
        }
    }

    if( parts->test( iteminfo_parts::ARMOR_ENCUMBRANCE ) && covers_anything ) {
        std::map<armor_encumb_data, std::vector<bodypart_id>> limb_groups;
        const Character &c = get_player_character();

        armor_encumb_header_info( *this, info );
        for( const bodypart_str_id &bp : get_covered_body_parts() ) {
            armor_encumb_data encumbrance( get_encumber( c, bp, item::encumber_flags::assume_empty ),
                                           get_encumber( c, bp ), get_encumber( c, bp, item::encumber_flags::assume_full ) );

            limb_groups[encumbrance].emplace_back( bp );
        }
        // keep on one line if only 1 entry
        if( limb_groups.size() == 1 ) {
            info.emplace_back( "ARMOR", _( "<bold>Encumbrance</bold>" ), iteminfo::no_newline );
        } else {
            info.emplace_back( "ARMOR", _( "<bold>Encumbrance</bold>:" ) );
        }
        for( auto &entry : limb_groups ) {
            std::set<translation, localized_comparator> to_print = body_part_type::consolidate( entry.second );
            std::string coverage;
            std::string encumbrance_info;
            for( const translation &entry : to_print ) {
                coverage += string_format( _( " The <info>%s</info>." ), entry );
            }

            //encumbrance_info = string_format( "%d", entry.first.encumb );

            // NOLINTNEXTLINE(cata-translate-string-literal)
            info.emplace_back( "ARMOR", space, "", iteminfo::no_newline | iteminfo::lower_is_better,
                               entry.first.encumb );

            // if it has a max value
            if( entry.first.encumb != entry.first.encumb_max ) {
                std::string when_full_message = _( ", When full" ) + space;
                info.emplace_back( "ARMOR", when_full_message, "", iteminfo::no_newline | iteminfo::lower_is_better,
                                   entry.first.encumb_max );
            }

            // if it has a min value
            if( entry.first.encumb != entry.first.encumb_min ) {
                std::string when_empty_message = _( ", When empty" ) + space;
                info.emplace_back( "ARMOR", when_empty_message, "",
                                   iteminfo::no_newline | iteminfo::lower_is_better, entry.first.encumb_min );
            }
            info.emplace_back( "ARMOR", string_format( ":%s", coverage ) );
        }
    }

    if( parts->test( iteminfo_parts::ARMOR_WARMTH ) && covers_anything ) {
        // TODO: Make this less inflexible on anatomy
        std::map<int, std::vector<bodypart_id>> limb_groups;
        for( const bodypart_str_id &bp : get_covered_body_parts() ) {
            limb_groups[get_warmth( bp )].emplace_back( bp );
        }
        // keep on one line if only 1 entry
        if( limb_groups.size() == 1 ) {
            info.emplace_back( "ARMOR", _( "<bold>Warmth</bold>" ), iteminfo::no_newline );
        } else {
            info.emplace_back( "ARMOR", _( "<bold>Warmth</bold>:" ) );
        }
        for( auto &entry : limb_groups ) {
            std::set<translation, localized_comparator> to_print = body_part_type::consolidate( entry.second );
            std::string coverage;
            for( const translation &entry : to_print ) {
                coverage += string_format( _( " The <info>%s</info>." ), entry );
            }
            info.emplace_back( "ARMOR", "", string_format( "  <num>:%s", coverage ), iteminfo::no_flags,
                               entry.first );
        }
    }

    if( parts->test( iteminfo_parts::ARMOR_BREATHABILITY ) && covers_anything ) {

        std::map<int, std::vector<bodypart_id>> limb_groups;
        for( const bodypart_str_id &bp : get_covered_body_parts() ) {
            const armor_portion_data *portion = portion_for_bodypart( bp );
            if( portion ) {
                limb_groups[portion->breathability].emplace_back( bp );
            }
        }
        // keep on one line if only 1 entry
        if( limb_groups.size() == 1 ) {
            info.emplace_back( "ARMOR", _( "<bold>Breathability</bold>" ), iteminfo::no_newline );
        } else {
            info.emplace_back( "ARMOR", _( "<bold>Breathability</bold>:" ) );
        }
        for( auto &entry : limb_groups ) {
            std::set<translation, localized_comparator> to_print = body_part_type::consolidate( entry.second );
            std::string coverage;
            for( const translation &entry : to_print ) {
                coverage += string_format( _( " The <info>%s</info>." ), entry );
            }
            info.emplace_back( "ARMOR", "", string_format( "  <num>%%:%s", coverage ), iteminfo::no_flags,
                               entry.first );
        }
    }

    insert_separation_line( info );

    if( covers_anything ) {

        if( is_power_armor() || type->get_id() == itype_rm13_armor ) {
            item tmp = *this;

            //no need to clutter the ui with inactive versions when the armor is already active
            if( !( active || ( type->tool && type->tool->power_draw > 0_W ) ) ) {
                bool print_prot = true;
                if( parts->test( iteminfo_parts::ARMOR_PROTECTION ) ) {
                    print_prot = !tmp.armor_full_protection_info( info, parts );
                }
                if( print_prot ) {
                    tmp.armor_protection_info( info, parts, batch, debug );
                }
                armor_protect_dmg_info( tmp.damage(), info );

                insert_separation_line( info );
                info.emplace_back( "ARMOR", _( "<bold>When active</bold>:" ) );
                tmp = tmp.convert( itype_id( tmp.typeId().str() + "_on" ) );
            }

            bool print_prot = true;
            if( parts->test( iteminfo_parts::ARMOR_PROTECTION ) ) {
                print_prot = !tmp.armor_full_protection_info( info, parts );
            }
            if( print_prot ) {
                tmp.armor_protection_info( info, parts, batch, debug );
            }
            armor_protect_dmg_info( tmp.damage(), info );
        } else {
            bool print_prot = true;
            if( parts->test( iteminfo_parts::ARMOR_PROTECTION ) ) {
                print_prot = !armor_full_protection_info( info, parts );
            }
            if( print_prot ) {
                armor_protection_info( info, parts, batch, debug );
            }
            armor_protect_dmg_info( damage(), info );
        }
    }

    // Whatever the last entry was, we want a newline at this point
    info.back().bNewLine = true;

    const units::mass weight_bonus = get_weight_capacity_bonus();
    const float weight_modif = get_weight_capacity_modifier();
    if( weight_modif != 1 ) {
        std::string modifier;
        if( weight_modif < 1 ) {
            modifier = "<num><bad>x</bad>";
        } else {
            modifier = "<num><color_light_green>x</color>";
        }
        info.emplace_back( "ARMOR",
                           _( "<bold>Weight capacity modifier</bold>: " ), modifier,
                           iteminfo::no_newline | iteminfo::is_decimal, weight_modif );
    }
    if( weight_bonus != 0_gram ) {
        std::string bonus;
        if( weight_bonus < 0_gram ) {
            bonus = string_format( "<num> <bad>%s</bad>", weight_units() );
        } else {
            bonus = string_format( "<num> <color_light_green> %s</color>", weight_units() );
        }
        info.emplace_back( "ARMOR", _( "<bold>Weight capacity bonus</bold>: " ), bonus,
                           iteminfo::no_newline | iteminfo::is_decimal,
                           convert_weight( weight_bonus ) );
    }

    if( show_bodygraph ) {
        insert_separation_line( info );

        auto bg_cb = [this]( const bodygraph_part * bgp, const std::string & sym ) {
            if( !bgp ) {
                return colorize( sym, bodygraph_full_body_iteminfo->fill_color );
            }
            std::set<sub_bodypart_id> grp{ bgp->sub_bodyparts.begin(), bgp->sub_bodyparts.end() };
            for( const bodypart_id &bid : bgp->bodyparts ) {
                grp.insert( bid->sub_parts.begin(), bid->sub_parts.end() );
            }
            nc_color clr = c_dark_gray;
            int cov_val = 0;
            for( const sub_bodypart_id &sid : grp ) {
                int tmp = get_coverage( sid );
                cov_val = tmp > cov_val ? tmp : cov_val;
            }
            if( cov_val <= 5 ) {
                clr = c_dark_gray;
            } else if( cov_val <= 10 ) {
                clr = c_light_gray;
            } else if( cov_val <= 25 ) {
                clr = c_light_red;
            } else if( cov_val <= 60 ) {
                clr = c_yellow;
            } else if( cov_val <= 80 ) {
                clr = c_green;
            } else {
                clr = c_light_green;
            }
            return colorize( sym, clr );
        };
        std::vector<std::string> bg_lines = get_bodygraph_lines( get_player_character(), bg_cb,
                                            bodygraph_full_body_iteminfo );
        for( const std::string &line : bg_lines ) {
            info.emplace_back( "ARMOR", line );
        }
        insert_separation_line( info );
    }
}

void item::animal_armor_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                              int /*batch*/, bool /*debug*/ ) const
{
    if( !is_pet_armor() ) {
        return;
    }
    pet_armor_protection_info( info, parts );
}

void item::armor_fit_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /*batch*/,
                           bool /*debug*/ ) const
{
    if( !is_armor() ) {
        return;
    }

    Character &player_character = get_player_character();
    const sizing sizing_level = get_sizing( player_character );

    if( parts->test( iteminfo_parts::DESCRIPTION_FLAGS_FITS ) ) {
        switch( sizing_level ) {
            case sizing::human_sized_human_char:
                if( has_flag( flag_FIT ) ) {
                    info.emplace_back( "DESCRIPTION",
                                       _( "* This clothing <info>fits</info> you perfectly." ) );
                }
                break;
            case sizing::big_sized_big_char:
                if( has_flag( flag_FIT ) ) {
                    info.emplace_back( "DESCRIPTION", _( "* This clothing <info>fits</info> "
                                                         "your large frame perfectly." ) );
                }
                break;
            case sizing::small_sized_small_char:
                if( has_flag( flag_FIT ) ) {
                    info.emplace_back( "DESCRIPTION", _( "* This clothing <info>fits</info> "
                                                         "your small frame perfectly." ) );
                }
                break;
            case sizing::big_sized_human_char:
                info.emplace_back( "DESCRIPTION", _( "* This clothing is <bad>oversized</bad> "
                                                     "and does <bad>not fit</bad> you." ) );
                break;
            case sizing::big_sized_small_char:
                info.emplace_back( "DESCRIPTION",
                                   _( "* This clothing is hilariously <bad>oversized</bad> "
                                      "and does <bad>not fit</bad> your <info>abnormally "
                                      "small mutated anatomy</info>." ) );
                break;
            case sizing::human_sized_big_char:
                info.emplace_back( "DESCRIPTION",
                                   _( "* This clothing is <bad>normal sized</bad> and does "
                                      "<bad>not fit</info> your <info>abnormally large "
                                      "mutated anatomy</info>." ) );
                break;
            case sizing::human_sized_small_char:
                info.emplace_back( "DESCRIPTION",
                                   _( "* This clothing is <bad>normal sized</bad> and does "
                                      "<bad>not fit</bad> your <info>abnormally small "
                                      "mutated anatomy</info>." ) );
                break;
            case sizing::small_sized_big_char:
                info.emplace_back( "DESCRIPTION",
                                   _( "* This clothing is hilariously <bad>undersized</bad> "
                                      "and does <bad>not fit</bad> your <info>abnormally "
                                      "large mutated anatomy</info>." ) );
                break;
            case sizing::small_sized_human_char:
                info.emplace_back( "DESCRIPTION", _( "* This clothing is <bad>undersized</bad> "
                                                     "and does <bad>not fit</bad> you." ) );
                break;
            default:
                break;
        }
    }

    if( parts->test( iteminfo_parts::DESCRIPTION_FLAGS_VARSIZE ) ) {
        if( has_flag( flag_VARSIZE ) ) {
            std::string resize_str;
            if( has_flag( flag_FIT ) ) {
                switch( sizing_level ) {
                    case sizing::small_sized_human_char:
                        resize_str = _( "<info>can be upsized</info>" );
                        break;
                    case sizing::human_sized_small_char:
                        resize_str = _( "<info>can be downsized</info>" );
                        break;
                    case sizing::big_sized_human_char:
                    case sizing::big_sized_small_char:
                        resize_str = _( "<bad>can not be downsized</bad>" );
                        break;
                    case sizing::small_sized_big_char:
                    case sizing::human_sized_big_char:
                        resize_str = _( "<bad>can not be upsized</bad>" );
                        break;
                    default:
                        break;
                }
                if( !resize_str.empty() ) {
                    std::string info_str = string_format( _( "* This clothing %s." ), resize_str );
                    info.emplace_back( "DESCRIPTION", info_str );
                }
            } else {
                switch( sizing_level ) {
                    case sizing::small_sized_human_char:
                        resize_str = _( " and <info>upsized</info>" );
                        break;
                    case sizing::human_sized_small_char:
                        resize_str = _( " and <info>downsized</info>" );
                        break;
                    case sizing::big_sized_human_char:
                    case sizing::big_sized_small_char:
                        resize_str = _( " but <bad>not downsized</bad>" );
                        break;
                    case sizing::small_sized_big_char:
                    case sizing::human_sized_big_char:
                        resize_str = _( " but <bad>not upsized</bad>" );
                        break;
                    default:
                        break;
                }
                std::string info_str = string_format( _( "* This clothing <info>can be "
                                                      "refitted</info>%s." ), resize_str );
                info.emplace_back( "DESCRIPTION", info_str );
            }
        } else {
            info.emplace_back( "DESCRIPTION", _( "* This clothing <bad>can not be refitted, "
                                                 "upsized, or downsized</bad>." ) );
        }
    }

    if( is_sided() && parts->test( iteminfo_parts::DESCRIPTION_FLAGS_SIDED ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This item can be worn on <info>either side</info> of "
                              "the body." ) );
    }
    if( is_power_armor() && parts->test( iteminfo_parts::DESCRIPTION_FLAGS_POWERARMOR ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This gear is a part of power armor." ) );
        if( parts->test( iteminfo_parts::DESCRIPTION_FLAGS_POWERARMOR_RADIATIONHINT ) ) {
            if( covers( bodypart_id( "head" ) ) ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "* When worn with a power armor suit, it will "
                                      "<good>fully protect</good> you from "
                                      "<info>radiation</info>." ) );
            } else {
                info.emplace_back( "DESCRIPTION",
                                   _( "* When worn with a power armor helmet, it will "
                                      "<good>fully protect</good> you from "
                                      "<info>radiation</info>." ) );
            }
        }
    }

    if( typeId() == itype_rad_badge && parts->test( iteminfo_parts::DESCRIPTION_IRRADIATION ) ) {
        info.emplace_back( "DESCRIPTION",
                           string_format( _( "* The film strip on the badge is %s." ),
                                          rad_badge_color( irradiation ).first ) );
    }
}

void item::book_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /* batch */,
                      bool /* debug */ ) const
{
    if( !is_book() ) {
        return;
    }

    insert_separation_line( info );
    const islot_book &book = *type->book;
    avatar &player_character = get_avatar();
    // Some things about a book you CAN tell by it's cover.
    if( !book.skill && !type->can_use( "MA_MANUAL" ) && parts->test( iteminfo_parts::BOOK_SUMMARY ) &&
        player_character.studied_all_recipes( *type ) ) {
        info.emplace_back( "BOOK", _( "Just for fun." ) );
    }

    if( type->can_use( "MA_MANUAL" ) && parts->test( iteminfo_parts::BOOK_SUMMARY ) ) {
        info.emplace_back( "BOOK",
                           _( "Some sort of <info>martial arts training "
                              "manual</info>." ) );
        if( player_character.has_identified( typeId() ) ) {
            const matype_id style_to_learn = martial_art_learned_from( *type );
            info.emplace_back( "BOOK",
                               string_format( _( "You can learn <info>%s</info> style "
                                                 "from it." ), style_to_learn->name ) );
            info.emplace_back( "BOOK",
                               string_format( _( "This fighting style is <info>%s</info> "
                                                 "to learn." ),
                                              martialart_difficulty( style_to_learn ) ) );
            info.emplace_back( "BOOK",
                               string_format( _( "It'd be easier to master if you'd have "
                                                 "skill expertise in <info>%s</info>." ),
                                              style_to_learn->primary_skill->name() ) );
        }
    }
    if( book.req == 0 && parts->test( iteminfo_parts::BOOK_REQUIREMENTS_BEGINNER ) ) {
        info.emplace_back( "BOOK", _( "It can be <info>understood by "
                                      "beginners</info>." ) );
    }
    if( player_character.has_identified( typeId() ) ) {
        if( book.skill ) {
            const SkillLevel &skill = player_character.get_skill_level_object( book.skill );
            if( skill.can_train() && parts->test( iteminfo_parts::BOOK_SKILLRANGE_MAX ) ) {
                const std::string skill_name = book.skill->name();
                std::string fmt;
                if( book.level != 0 ) {
                    fmt = string_format( _( "Can bring your <info>%s skill to</info> "
                                            "<num>." ), skill_name );
                    info.emplace_back( "BOOK", "", fmt, iteminfo::no_flags, book.level );
                }
                fmt = string_format( _( "Your current <stat>%s skill</stat> is <num>." ),
                                     skill_name );
                info.emplace_back( "BOOK", "", fmt, iteminfo::no_flags, skill.knowledgeLevel() );
            }

            if( book.req != 0 && parts->test( iteminfo_parts::BOOK_SKILLRANGE_MIN ) ) {
                const std::string fmt = string_format(
                                            _( "<info>Requires %s level</info> <num> to "
                                               "understand." ), book.skill.obj().name() );
                info.emplace_back( "BOOK", "", fmt,
                                   iteminfo::lower_is_better, book.req );
            }
        }

        if( book.intel != 0 && parts->test( iteminfo_parts::BOOK_REQUIREMENTS_INT ) ) {
            info.emplace_back( "BOOK", "",
                               _( "Requires <info>intelligence of</info> <num> to easily "
                                  "read." ), iteminfo::lower_is_better, book.intel );
        }
        if( player_character.book_fun_for( *this, player_character ) != 0 &&
            parts->test( iteminfo_parts::BOOK_MORALECHANGE ) ) {
            info.emplace_back( "BOOK", "",
                               _( "Reading this book affects your morale by <num>" ),
                               iteminfo::show_plus, player_character.book_fun_for( *this, player_character ) );
        }
        if( parts->test( iteminfo_parts::BOOK_TIMEPERCHAPTER ) ) {
            std::string fmt = n_gettext(
                                  "A chapter of this book takes <num> <info>minute to "
                                  "read</info>.",
                                  "A chapter of this book takes <num> <info>minutes to "
                                  "read</info>.", to_minutes<int>( book.time ) );
            if( type->use_methods.count( "MA_MANUAL" ) ) {
                fmt = n_gettext(
                          "<info>A training session</info> with this book takes "
                          "<num> <info>minute</info>.",
                          "<info>A training session</info> with this book takes "
                          "<num> <info>minutes</info>.", to_minutes<int>( book.time ) );
            }
            info.emplace_back( "BOOK", "", fmt,
                               iteminfo::lower_is_better, to_minutes<int>( book.time ) );
        }

        if( book.chapters > 0 && parts->test( iteminfo_parts::BOOK_NUMUNREADCHAPTERS ) ) {
            const int unread = get_remaining_chapters( player_character );
            std::string fmt = n_gettext( "This book has <num> <info>unread chapter</info>.",
                                         "This book has <num> <info>unread chapters</info>.",
                                         unread );
            info.emplace_back( "BOOK", "", fmt, iteminfo::no_flags, unread );
        }

        if( !book.proficiencies.empty() ) {
            const auto enumerate_profs = []( const book_proficiency_bonus & prof ) {
                return prof.id->name();
            };
            const std::string profs = string_format(
                                          _( "This book can help with the following proficiencies: %s" ),
                                          enumerate_as_string( book.proficiencies, enumerate_profs ) );
            info.emplace_back( "BOOK", profs );
        }

        if( parts->test( iteminfo_parts::BOOK_INCLUDED_RECIPES ) ) {
            std::vector<std::string> known_recipe_list;
            std::vector<std::string> learnable_recipe_list;
            std::vector<std::string> practice_recipe_list;
            std::vector<std::string> unlearnable_recipe_list;
            for( const islot_book::recipe_with_description_t &elem : book.recipes ) {
                const bool knows_it = player_character.knows_recipe( elem.recipe );
                // If the player knows it, they recognize it even if it's not clearly stated.
                if( elem.is_hidden() && !knows_it ) {
                    continue;
                }
                const bool can_learn = player_character.get_knowledge_level( elem.recipe->skill_used ) >=
                                       elem.skill_level;
                if( elem.recipe->is_practice() ) {
                    const char *const format = can_learn ? "<dark>%s</dark>" : "<color_brown>%s</color>";
                    practice_recipe_list.emplace_back( string_format( format, elem.recipe->result_name() ) );
                } else if( knows_it ) {
                    // In case the recipe is known, but has a different name in the book, use the
                    // real name to avoid confusing the player.
                    known_recipe_list.emplace_back( string_format( "<bold>%s</bold>", elem.recipe->result_name() ) );
                } else if( !can_learn ) {
                    unlearnable_recipe_list.emplace_back( string_format( "<color_brown>%s</color>", elem.name() ) );
                } else {
                    learnable_recipe_list.emplace_back( string_format( "<dark>%s</dark>", elem.name() ) );
                }
            }

            const std::size_t num_crafting_recipes = known_recipe_list.size() + learnable_recipe_list.size() +
                    unlearnable_recipe_list.size();
            const std::size_t num_total_recipes = num_crafting_recipes + practice_recipe_list.size();
            if( num_total_recipes > 0 && parts->test( iteminfo_parts::DESCRIPTION_BOOK_RECIPES ) ) {
                std::vector<std::string> lines;
                if( num_crafting_recipes > 0 ) {
                    lines.emplace_back(
                        string_format( n_gettext( "This book contains %u crafting recipe.",
                                                  "This book contains %u crafting recipes.",
                                                  num_crafting_recipes ), num_crafting_recipes ) );
                }
                if( !known_recipe_list.empty() ) {
                    lines.emplace_back( _( "You already know how to craft:" ) );
                    lines.emplace_back( enumerate_lcsorted_with_limit( known_recipe_list ) );
                }
                if( !learnable_recipe_list.empty() ) {
                    lines.emplace_back( _( "You understand how to craft:" ) );
                    lines.emplace_back( enumerate_lcsorted_with_limit( learnable_recipe_list ) );
                }
                if( !unlearnable_recipe_list.empty() ) {
                    lines.emplace_back( _( "You lack the skills to understand:" ) );
                    lines.emplace_back( enumerate_lcsorted_with_limit( unlearnable_recipe_list ) );
                }
                if( !practice_recipe_list.empty() ) {
                    lines.emplace_back( _( "This book can help you practice:" ) );
                    lines.emplace_back( enumerate_lcsorted_with_limit( practice_recipe_list ) );
                }
                insert_separation_line( info );
                for( const std::string &recipe_line : lines ) {
                    info.emplace_back( "DESCRIPTION", recipe_line );
                }
            }

            if( num_total_recipes < book.recipes.size() &&
                parts->test( iteminfo_parts::DESCRIPTION_BOOK_ADDITIONAL_RECIPES ) ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "It might help you figuring out some <good>more recipes</good>." ) );
            }
        }

    } else {
        if( parts->test( iteminfo_parts::BOOK_UNREAD ) ) {
            info.emplace_back( "BOOK", _( "You need to <info>read this book to see its contents</info>." ) );
        }
    }
}

void item::battery_info( std::vector<iteminfo> &info, const iteminfo_query * /*parts*/,
                         int /*batch*/, bool /*debug*/ ) const
{
    if( !is_vehicle_battery() ) {
        return;
    }

    std::string info_string;
    if( type->battery->max_capacity < 1_J ) {
        info_string = string_format( _( "<bold>Capacity</bold>: %dmJ" ),
                                     to_millijoule( type->battery->max_capacity ) );
    } else if( type->battery->max_capacity < 1_kJ ) {
        info_string = string_format( _( "<bold>Capacity</bold>: %dJ" ),
                                     to_joule( type->battery->max_capacity ) );
    } else if( type->battery->max_capacity >= 1_kJ ) {
        info_string = string_format( _( "<bold>Capacity</bold>: %dkJ" ),
                                     to_kilojoule( type->battery->max_capacity ) );
    }
    insert_separation_line( info );
    info.emplace_back( "BATTERY", info_string );
}

void item::tool_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /*batch*/,
                      bool /*debug*/ ) const
{
    avatar &player_character = get_avatar();

    if( !is_tool() ) {
        return;
    }

    insert_separation_line( info );
    if( !ammo_types().empty() && parts->test( iteminfo_parts::TOOL_CHARGES ) ) {
        info.emplace_back( "TOOL", string_format( _( "<bold>Charges</bold>: %d" ),
                           ammo_remaining() ) );
    }

    if( !magazine_integral() ) {
        if( magazine_current() && parts->test( iteminfo_parts::TOOL_MAGAZINE_CURRENT ) ) {
            info.emplace_back( "TOOL", _( "Magazine: " ),
                               string_format( "<stat>%s</stat>", magazine_current()->tname() ) );
        }

        if( parts->test( iteminfo_parts::TOOL_MAGAZINE_COMPATIBLE ) ) {
            if( uses_magazine() ) {
                // Keep this identical with gun magazines in item::gun_info
                const std::vector<itype_id> compat_sorted = sorted_lex( magazine_compatible() );
                const std::string mag_names = enumerate_as_string( compat_sorted,
                []( const itype_id & id ) {
                    return item::nname( id );
                } );

                const std::set<flag_id> flag_restrictions = contents.magazine_flag_restrictions();
                const std::string flag_names = enumerate_as_string( flag_restrictions,
                []( const flag_id & e ) {
                    return e->name();
                } );

                std::string display = _( "<bold>Compatible magazines</bold>:" );
                if( !compat_sorted.empty() ) {
                    display += _( "\n<bold>Types</bold>: " ) + mag_names;
                }
                if( !flag_restrictions.empty() ) {
                    display += _( "\n<bold>Form factors</bold>: " ) + flag_names;
                }

                info.emplace_back( "DESCRIPTION", display );
            }
        }
    } else if( !ammo_types().empty() && parts->test( iteminfo_parts::TOOL_CAPACITY ) ) {
        for( const ammotype &at : ammo_types() ) {
            info.emplace_back(
                "TOOL",
                "",
                string_format(
                    n_gettext(
                        "Maximum <num> charge of %s.",
                        "Maximum <num> charges of %s.",
                        ammo_capacity( at ) ),
                    at->name() ),
                iteminfo::no_flags,
                ammo_capacity( at ) );
        }
    }

    // UPS, rechargeable power cells, and bionic power
    if( has_flag( flag_USE_UPS ) && parts->test( iteminfo_parts::DESCRIPTION_RECHARGE_UPSMODDED ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This tool has been modified to use a <info>universal "
                              "power supply</info> and is <neutral>not compatible"
                              "</neutral> with <info>standard batteries</info>." ) );
    } else if( has_flag( flag_RECHARGE ) && has_flag( flag_NO_RELOAD ) &&
               parts->test( iteminfo_parts::DESCRIPTION_RECHARGE_NORELOAD ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This tool has a <info>rechargeable power cell</info> "
                              "and is <neutral>not compatible</neutral> with "
                              "<info>standard batteries</info>." ) );
    } else if( has_flag( flag_RECHARGE ) &&
               parts->test( iteminfo_parts::DESCRIPTION_RECHARGE_UPSCAPABLE ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This tool has a <info>rechargeable power cell</info> "
                              "and can be recharged in any <neutral>UPS-compatible "
                              "recharging station</neutral>. You could charge it with "
                              "<info>standard batteries</info>, but unloading it is "
                              "impossible." ) );
    } else if( has_flag( flag_USES_BIONIC_POWER ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This tool <info>runs on bionic power</info>." ) );
    } else if( has_flag( flag_BURNOUT ) && parts->test( iteminfo_parts::TOOL_BURNOUT ) ) {
        int percent_left = 0;
        if( ammo_data() != nullptr ) {
            // Candle items that use ammo instead of charges
            // The capacity should never be zero but clang complains about it
            percent_left = 100 * ammo_remaining() / std::max( ammo_capacity( ammo_data()->ammo->type ), 1 );
        } else if( type->maximum_charges() > 0 ) {
            // Candle items that use charges instead of ammo
            percent_left = 100 * ammo_remaining() / type->maximum_charges();
        }

        std::string feedback;
        if( percent_left == 100 ) {
            feedback = _( "It's new, and ready to burn." );
        } else if( percent_left >= 75 ) {
            feedback = _( "Almost new, with much material to burn." );
        } else if( percent_left >= 50 ) {
            feedback = _( "More than a quarter has burned away." );
        } else if( percent_left >= 25 ) {
            feedback = _( "More than half has burned away." );
        } else if( percent_left >= 10 ) {
            feedback = _( "Less than a quarter left to burn." );
        } else {
            feedback = _( "Almost completely burned out." );
        }
        feedback = _( "<bold>Fuel</bold>: " ) + feedback;
        info.emplace_back( "DESCRIPTION", feedback );
    }

    // Display e-ink tablet copied recipes from SD cards
    const std::set<recipe_id> saved_recipes = get_saved_recipes();
    if( !saved_recipes.empty() ) {
        std::vector<std::string> known_recipe_list;
        std::vector<std::string> learnable_recipe_list;
        std::vector<std::string> unlearnable_recipe_list;

        for( const recipe_id &rid : saved_recipes ) {
            const recipe *r = &rid.obj();

            const bool knows_it = player_character.knows_recipe( r );
            const bool can_learn = player_character.get_skill_level( r->skill_used ) >= r->difficulty;

            // In case the recipe is known, but has a different name in the item, use the
            // real name to avoid confusing the player.
            const std::string name = r->result_name();

            if( knows_it ) {
                known_recipe_list.emplace_back( "<bold>" + name + "</bold>" );
            } else if( !can_learn ) {
                unlearnable_recipe_list.emplace_back( "<color_brown>" + name + "</color>" );
            } else {
                learnable_recipe_list.emplace_back( "<dark>" + name + "</dark>" );
            }
        }

        int total_recipes = known_recipe_list.size() + learnable_recipe_list.size() +
                            unlearnable_recipe_list.size();

        if( ( !known_recipe_list.empty() || !learnable_recipe_list.empty() ||
              !unlearnable_recipe_list.empty() ) &&
            parts->test( iteminfo_parts::DESCRIPTION_BOOK_RECIPES ) ) {
            const std::string recipe_line =
                string_format( n_gettext( "Contains %1$d copied crafting recipe:",
                                          "Contains %1$d copied crafting recipes:",
                                          total_recipes ), total_recipes );

            insert_separation_line( info );
            info.emplace_back( "DESCRIPTION", recipe_line );

            if( !known_recipe_list.empty() ) {
                const std::string recipe_line =
                    string_format( n_gettext( "\nYou already know %1$d recipe:\n%2$s",
                                              "\nYou already know %1$d recipes:\n%2$s",
                                              known_recipe_list.size() ),
                                   known_recipe_list.size(), enumerate_lcsorted_with_limit( known_recipe_list ) );

                info.emplace_back( "DESCRIPTION", recipe_line );
            }

            if( !learnable_recipe_list.empty() ) {
                const std::string recipe_line =
                    string_format( n_gettext( "\nYou have the skills to craft %1$d recipe:\n%2$s",
                                              "\nYou have the skills to craft %1$d recipes:\n%2$s",
                                              learnable_recipe_list.size() ),
                                   learnable_recipe_list.size(), enumerate_lcsorted_with_limit( learnable_recipe_list ) );

                info.emplace_back( "DESCRIPTION", recipe_line );
            }

            if( !unlearnable_recipe_list.empty() ) {
                const std::string recipe_line =
                    string_format( n_gettext( "\nYou lack the skills to craft %1$d recipe:\n%2$s",
                                              "\nYou lack the skills to craft %1$d recipes:\n%2$s",
                                              unlearnable_recipe_list.size() ),
                                   unlearnable_recipe_list.size(), enumerate_lcsorted_with_limit( unlearnable_recipe_list ) );

                info.emplace_back( "DESCRIPTION", recipe_line );
            }
        }
    }

    // Display e-ink tablet ebook recipes
    if( is_ebook_storage() && !is_broken_on_active() ) {
        std::vector<std::string> known_recipe_list;
        std::vector<std::string> learnable_recipe_list;
        std::vector<std::string> unlearnable_recipe_list;
        std::vector<const item *> book_list = ebooks();
        int total_ebooks = book_list.size();

        for( const item *ebook : book_list ) {
            const islot_book &book = *ebook->type->book;
            for( const islot_book::recipe_with_description_t &elem : book.recipes ) {
                const bool knows_it = player_character.knows_recipe( elem.recipe );
                const bool can_learn = player_character.get_skill_level( elem.recipe->skill_used ) >=
                                       elem.skill_level;
                // If the player knows it, they recognize it even if it's not clearly stated.
                if( elem.is_hidden() && !knows_it ) {
                    continue;
                }

                // In case the recipe is known, but has a different name in the book, use the
                // real name to avoid confusing the player.
                const std::string name = elem.recipe->result_name( /*decorated=*/true );

                if( knows_it ) {
                    std::string recipe_formated = "<bold>" + name + "</bold>";
                    if( known_recipe_list.end() == std::find( known_recipe_list.begin(), known_recipe_list.end(),
                            name ) ) {
                        known_recipe_list.emplace_back( "<bold>" + name + "</bold>" );
                    }
                } else if( !can_learn ) {
                    std::string recipe_formated = "<color_brown>" + elem.name() + "</color>";
                    if( unlearnable_recipe_list.end() == std::find( unlearnable_recipe_list.begin(),
                            unlearnable_recipe_list.end(), recipe_formated ) ) {
                        unlearnable_recipe_list.emplace_back( recipe_formated );
                    }
                } else {
                    std::string recipe_formated = "<dark>" + elem.name() + "</dark>";
                    if( learnable_recipe_list.end() == std::find( learnable_recipe_list.begin(),
                            learnable_recipe_list.end(), recipe_formated ) ) {
                        learnable_recipe_list.emplace_back( "<dark>" + elem.name() + "</dark>" );
                    }
                }
            }
        }

        int total_recipes = known_recipe_list.size() + learnable_recipe_list.size() +
                            unlearnable_recipe_list.size();

        if( ( !known_recipe_list.empty() || !learnable_recipe_list.empty() ||
              !unlearnable_recipe_list.empty() ) &&
            parts->test( iteminfo_parts::DESCRIPTION_BOOK_RECIPES ) ) {
            std::string recipe_line =
                string_format( n_gettext( "Contains %1$d unique crafting recipe,",
                                          "Contains %1$d unique crafting recipes,",
                                          total_recipes ), total_recipes );
            std::string source_line =
                string_format( n_gettext( "from %1$d stored e-book:",
                                          "from %1$d stored e-books:",
                                          total_ebooks ), total_ebooks );

            insert_separation_line( info );
            info.emplace_back( "DESCRIPTION", recipe_line );
            info.emplace_back( "DESCRIPTION", source_line );

            if( !known_recipe_list.empty() ) {
                const std::string recipe_line =
                    string_format( n_gettext( "\nYou already know %1$d recipe:\n%2$s",
                                              "\nYou already know %1$d recipes:\n%2$s",
                                              known_recipe_list.size() ),
                                   known_recipe_list.size(), enumerate_lcsorted_with_limit( known_recipe_list ) );

                info.emplace_back( "DESCRIPTION", recipe_line );
            }

            if( !learnable_recipe_list.empty() ) {
                std::string recipe_line =
                    string_format( n_gettext( "\nYou have the skills to craft %1$d recipe:\n%2$s",
                                              "\nYou have the skills to craft %1$d recipes:\n%2$s",
                                              learnable_recipe_list.size() ),
                                   learnable_recipe_list.size(), enumerate_lcsorted_with_limit( learnable_recipe_list ) );

                info.emplace_back( "DESCRIPTION", recipe_line );
            }

            if( !unlearnable_recipe_list.empty() ) {
                std::string recipe_line =
                    string_format( n_gettext( "\nYou lack the skills to craft %1$d recipe:\n%2$s",
                                              "\nYou lack the skills to craft %1$d recipes:\n%2$s",
                                              unlearnable_recipe_list.size() ),
                                   unlearnable_recipe_list.size(), enumerate_lcsorted_with_limit( unlearnable_recipe_list ) );

                info.emplace_back( "DESCRIPTION", recipe_line );
            }
        }
    }
}

void item::actions_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                         int /*batch*/, bool /*debug*/ ) const
{

    const std::map<std::string, use_function> &use_methods = type->use_methods;
    if( use_methods.empty() || !parts->test( iteminfo_parts::ACTIONS ) ) {
        return;
    }
    insert_separation_line( info );
    std::string actions = enumerate_as_string( use_methods,
    []( const std::pair<std::string, use_function> &val ) {
        return string_format( "<info>%s</info>", val.second.get_name() );
    } );
    info.emplace_back( "DESCRIPTION", string_format( _( "<bold>Actions</bold>: %s" ), actions ) );
}

void item::component_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /*batch*/,
                           bool /*debug*/ ) const
{
    if( components.empty() || !parts->test( iteminfo_parts::DESCRIPTION_COMPONENTS_MADEFROM ) ) {
        return;
    }
    if( is_craft() ) {
        info.emplace_back( "DESCRIPTION", string_format( _( "Using: %s" ),
                           components_to_string() ) );
    } else {
        info.emplace_back( "DESCRIPTION", string_format( _( "Made from: %s" ),
                           components_to_string() ) );
    }
}

static void enchantment_info_helper( const item &it,
                                     enchantment::has type,
                                     std::vector<std::string> &always,
                                     std::vector<std::string> &active,
                                     std::vector<std::string> &inactive )
{
    for( const enchantment &ench : it.get_defined_enchantments() ) {
        // only check conditions matching the given type
        if( ench.active_conditions.first == type ) {
            // if a description is provided use that
            if( !ench.description.empty() ) {
                std::string info;
                if( !ench.name.empty() ) {
                    info = string_format( "<info>%s</info>: %s", ench.name.translated(),
                                          ench.description.translated() );
                } else {
                    info = string_format( _( "<info>Description</info>: %s" ), ench.description.translated() );
                }
                switch( ench.active_conditions.second ) {
                    case enchantment::condition::ALWAYS:
                        always.push_back( info );
                        break;
                    case enchantment::condition::ACTIVE:
                        active.push_back( info );
                        break;
                    case enchantment::condition::INACTIVE:
                        inactive.push_back( info );
                        break;
                    case enchantment::condition::DIALOG_CONDITION:
                    case enchantment::condition::NUM_CONDITION:
                        // skip for dialog conditions
                        break;
                }
            }
        }
    }
    for( const enchant_cache &ench : it.get_proc_enchantments() ) {
        // only check conditions matching the given type
        if( ench.active_conditions.first == type ) {
            // if a description is provided use that
            if( !ench.description.empty() ) {
                std::string info;
                if( !ench.name.empty() ) {
                    info = string_format( "<info>%s</info>: %s", ench.name.translated(),
                                          ench.description.translated() );
                } else {
                    info = string_format( _( "<info>Description</info>: %s" ), ench.description.translated() );
                }
                switch( ench.active_conditions.second ) {
                    case enchantment::condition::ALWAYS:
                        always.push_back( info );
                        break;
                    case enchantment::condition::ACTIVE:
                        active.push_back( info );
                        break;
                    case enchantment::condition::INACTIVE:
                        inactive.push_back( info );
                        break;
                    case enchantment::condition::DIALOG_CONDITION:
                    case enchantment::condition::NUM_CONDITION:
                        // skip for dialog conditions
                        break;
                }
            }
        }
    }
}

static void enchantment_info_printer( std::vector<iteminfo> &info,
                                      std::vector<std::string> &always,
                                      std::vector<std::string> &active,
                                      std::vector<std::string> &inactive )
{
    if( !always.empty() ) {
        info.emplace_back( "DESCRIPTION", string_format( _( "<bold>Always</bold>:" ) ) );
        for( const std::string &entry : always ) {
            info.emplace_back( "DESCRIPTION", string_format( "* %s", entry ) );
        }
    }
    if( !active.empty() ) {
        info.emplace_back( "DESCRIPTION", string_format( _( "<bold>When Active</bold>:" ) ) );
        for( const std::string &entry : active ) {
            info.emplace_back( "DESCRIPTION", string_format( "* %s", entry ) );
        }
    }
    if( !inactive.empty() ) {
        info.emplace_back( "DESCRIPTION", string_format( _( "<bold>When Inactive</bold>:" ) ) );
        for( const std::string &entry : inactive ) {
            info.emplace_back( "DESCRIPTION", string_format( "* %s", entry ) );
        }
    }
}

void item::enchantment_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                             int /*batch*/, bool /*debug*/ ) const
{
    if( !parts->test( iteminfo_parts::DESCRIPTION_ENCHANTMENTS ) ) {
        return;
    }
    insert_separation_line( info );
    // if the item has relic data and the item is mundane show some details
    if( type->relic_data ) {
        // check WIELD
        std::vector<std::string> always;
        std::vector<std::string> active;
        std::vector<std::string> inactive;
        enchantment_info_helper( *this, enchantment::has::WIELD, always, active, inactive );
        if( !always.empty() || !active.empty() || !inactive.empty() ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "When <bold>wielded</bold> this item provides:" ) ) );
            enchantment_info_printer( info, always, active, inactive );
        }
        always.clear();
        active.clear();
        inactive.clear();
        enchantment_info_helper( *this, enchantment::has::WORN, always, active, inactive );
        if( !always.empty() || !active.empty() || !inactive.empty() ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "When <bold>worn</bold> this item provides:" ) ) );
            enchantment_info_printer( info, always, active, inactive );
        }
        always.clear();
        active.clear();
        inactive.clear();
        enchantment_info_helper( *this, enchantment::has::HELD, always, active, inactive );
        if( !always.empty() || !active.empty() || !inactive.empty() ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "When <bold>carried</bold> this item provides:" ) ) );
            enchantment_info_printer( info, always, active, inactive );
        }
    }
}

void item::repair_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                        int /*batch*/, bool /*debug*/ ) const
{
    if( !parts->test( iteminfo_parts::DESCRIPTION_REPAIREDWITH ) ) {
        return;
    }
    insert_separation_line( info );
    const std::vector<itype_id> rep = sorted_lex( repaired_with() );
    if( !rep.empty() ) {
        info.emplace_back( "DESCRIPTION", string_format( _( "<bold>Repair</bold> using %s." ),
                           enumerate_as_string( rep,
        []( const itype_id & e ) {
            return nname( e );
        }, enumeration_conjunction::or_ ) ) );

        const std::string repairs_with = enumerate_as_string( type->repairs_with,
        []( const material_id & e ) {
            return string_format( "<info>%s</info>", e->name() );
        } );
        if( !repairs_with.empty() ) {
            info.emplace_back( "DESCRIPTION", string_format( _( "<bold>With</bold> %s." ), repairs_with ) );
        }
    } else {
        info.emplace_back( "DESCRIPTION", _( "* This item is <bad>not repairable</bad>." ) );
    }
}

void item::disassembly_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                             int /*batch*/, bool /*debug*/ ) const
{
    if( !components.empty() && parts->test( iteminfo_parts::DESCRIPTION_COMPONENTS_MADEFROM ) ) {
        return;
    }
    if( !parts->test( iteminfo_parts::DESCRIPTION_COMPONENTS_DISASSEMBLE ) ) {
        return;
    }

    const recipe &dis = ( typeId() == itype_disassembly ) ? get_making() :
                        recipe_dictionary::get_uncraft( typeId() );
    const requirement_data &req = dis.disassembly_requirements();
    if( !req.is_empty() ) {
        const std::string approx_time = to_string_approx( dis.time_to_craft( get_player_character(),
                                        recipe_time_flag::ignore_proficiencies ) );

        const requirement_data::alter_item_comp_vector &comps_list = req.get_components();
        const std::string comps_str = enumerate_as_string( comps_list,
        []( const std::vector<item_comp> &comp_opts ) {
            return comp_opts.front().to_string();
        } );

        std::vector<std::string> reqs_list;
        const requirement_data::alter_tool_comp_vector &tools_list = req.get_tools();
        for( const std::vector<tool_comp> &it : tools_list ) {
            if( !it.empty() ) {
                reqs_list.push_back( it.front().to_string() );
            }
        }
        const requirement_data::alter_quali_req_vector &quals_list = req.get_qualities();
        for( const std::vector<quality_requirement> &it : quals_list ) {
            if( !it.empty() ) {
                reqs_list.push_back( it.front().to_colored_string() );
            }
        }

        std::string descr;
        if( reqs_list.empty() ) {
            //~ 1 is approx. time (e.g. 'about 5 minutes'), 2 is a list of items
            descr = string_format( _( "<bold>Disassembly</bold> takes %1$s and might yield: %2$s." ),
                                   approx_time, comps_str );
        } else {
            descr = string_format(
                        //~ 1 is approx. time, 2 is a list of items and tools with qualities, 3 is a list of items.
                        //~ Bold text in the middle makes it easier to see where the second list starts.
                        _( "<bold>Disassembly</bold> takes %1$s, requires %2$s and <bold>might yield</bold>: %3$s." ),
                        approx_time, enumerate_as_string( reqs_list ), comps_str );
        }

        insert_separation_line( info );
        info.emplace_back( "DESCRIPTION", descr );
    }
}

void item::qualities_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /*batch*/,
                           bool /*debug*/ ) const
{
    // Inline function to emplace a given quality (and its level) onto the info vector
    auto name_quality = [&info]( const std::pair<quality_id, int> &q ) {
        std::string str;
        if( q.first == qual_JACK || q.first == qual_LIFT ) {
            //~ %1$d is the numeric quality level, and %2$s is the name of the quality.
            //~ %3$d is the amount of weight the jack can lift, and %4$s is the units of that weight.
            str = string_format( _( "Level <info>%1$d %2$s</info> quality, "
                                    "rated at <info>%3$d</info> %4$s" ),
                                 q.second, q.first.obj().name,
                                 static_cast<int>( convert_weight( lifting_quality_to_mass( q.second ) ) ),
                                 weight_units() );
        } else {
            //~ %1$d is the numeric quality level, and %2$s is the name of the quality
            str = string_format( _( "Level <info>%1$d %2$s</info> quality" ),
                                 q.second, q.first.obj().name );
        }
        info.emplace_back( "QUALITIES", "", str );
    };

    // List all qualities of this item granted by its item type
    const bool has_any_qualities = !type->qualities.empty() || !type->charged_qualities.empty();
    if( parts->test( iteminfo_parts::QUALITIES ) && has_any_qualities ) {
        insert_separation_line( info );
        // List all inherent (unconditional) qualities
        if( !type->qualities.empty() ) {
            info.emplace_back( "QUALITIES", "", _( "<bold>Has qualities</bold>:" ) );
            for( const std::pair<quality_id, int> &q : sorted_lex( type->qualities ) ) {
                name_quality( q );
            }
        }
        // Tools with "charged_qualities" defined may have additional qualities when charged.
        // List them, and show whether there is enough charge to use those qualities.
        if( !type->charged_qualities.empty() && type->charges_to_use() > 0 ) {
            // Use ammo_sufficient() with player character to include bionic/UPS power
            if( ammo_sufficient( &get_player_character() ) ) {
                info.emplace_back( "QUALITIES", "", _( "<good>Has enough charges</good> for qualities:" ) );
            } else {
                info.emplace_back( "QUALITIES", "",
                                   string_format( _( "<bad>Needs %d or more charges</bad> for qualities:" ),
                                                  type->charges_to_use() ) );
            }
            for( const std::pair<quality_id, int> &q : sorted_lex( type->charged_qualities ) ) {
                name_quality( q );
            }
        }
    }

    // Accumulate and list all qualities of items contained within this item
    if( parts->test( iteminfo_parts::QUALITIES_CONTAINED ) &&
    contents.has_any_with( []( const item & e ) {
    return !e.type->qualities.empty();
    }, item_pocket::pocket_type::CONTAINER ) ) {

        info.emplace_back( "QUALITIES", "", _( "Contains items with qualities:" ) );
        std::map<quality_id, int, quality_id::LexCmp> most_quality;
        for( const item *e : contents.all_items_top() ) {
            for( const std::pair<const quality_id, int> &q : e->type->qualities ) {
                auto emplace_result = most_quality.emplace( q );
                if( !emplace_result.second &&
                    most_quality.at( emplace_result.first->first ) < q.second ) {
                    most_quality[ q.first ] = q.second;
                }
            }
        }
        for( const std::pair<const quality_id, int> &q : most_quality ) {
            name_quality( q );
        }
    }
}

void item::bionic_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int /*batch*/,
                        bool /*debug*/ ) const
{
    if( !is_bionic() ) {
        return;
    }

    insert_separation_line( info );

    // TODO: Unhide when enforcing limits
    if( get_option < bool >( "CBM_SLOTS_ENABLED" )
        && parts->test( iteminfo_parts::DESCRIPTION_CBM_SLOTS ) ) {
        info.emplace_back( "DESCRIPTION", list_occupied_bps( type->bionic->id,
                           _( "This bionic is installed in the following body "
                              "part(s):" ) ) );
    }

    if( is_bionic() && has_flag( flag_NO_STERILE ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This bionic is <bad>not sterile</bad>, use an <info>autoclave</info> and an <info>autoclave pouch</info> to sterilize it. " ) );
    }
    insert_separation_line( info );

    const bionic_id bid = type->bionic->id;
    const std::vector<material_id> &fuels = bid->fuel_opts;
    if( !fuels.empty() ) {
        const std::string fuels_str = enumerate_as_string( fuels,
        []( const material_id & id ) {
            return "<info>" + id->name() + "</info>";
        } );

        info.emplace_back( "DESCRIPTION",
                           n_gettext( "* This bionic can produce power from the following fuel: ",
                                      "* This bionic can produce power from the following fuels: ", fuels.size() )
                           + fuels_str );
    }

    insert_separation_line( info );

    if( bid->capacity > 0_mJ ) {
        info.emplace_back( "CBM", _( "<bold>Power Capacity</bold>:" ), _( " <num> mJ" ),
                           iteminfo::no_newline,
                           static_cast<double>( units::to_millijoule( bid->capacity ) ) );
    }

    insert_separation_line( info );

    if( bid->required_bionic ) {
        info.emplace_back( "CBM", string_format( "* This CBM requires another CBM to also be installed: %s",
                           bid->required_bionic->name ) );
    }
    insert_separation_line( info );

    // TODO refactor these almost identical blocks
    if( !bid->encumbrance.empty() ) {
        info.emplace_back( "DESCRIPTION", _( "<bold>Encumbrance</bold>:" ),
                           iteminfo::no_newline );
        for( const std::pair<bodypart_str_id, int> &element : sorted_lex( bid->encumbrance ) ) {
            info.emplace_back( "CBM", " " + body_part_name_as_heading( element.first.id(), 1 ),
                               " <num>", iteminfo::no_newline, element.second );
        }
    }

    if( !bid->env_protec.empty() ) {
        info.emplace_back( "DESCRIPTION",
                           _( "<bold>Environmental Protection</bold>:" ),
                           iteminfo::no_newline );
        for( const std::pair< bodypart_str_id, size_t > &element : sorted_lex( bid->env_protec ) ) {
            info.emplace_back( "CBM", " " + body_part_name_as_heading( element.first, 1 ),
                               " <num>", iteminfo::no_newline, static_cast<double>( element.second ) );
        }
    }

    for( const damage_info_order &dio : damage_info_order::get_all(
             damage_info_order::info_type::BIO ) ) {
        std::map<bodypart_str_id, float> dt_protec;
        for( const std::pair<const bodypart_str_id, resistances> &prot : bid->protec ) {
            float res = prot.second.type_resist( dio.dmg_type );
            if( res >= 0.f ) {
                dt_protec.emplace( prot.first, res );
            }
        }
        if( !dt_protec.empty() ) {
            //~ Type of protection (ex: "Bash Protection")
            std::string label = string_format( _( "<bold>%s Protection</bold>:" ),
                                               uppercase_first_letter( dio.dmg_type->name.translated() ) );
            info.emplace_back( "DESCRIPTION", label, iteminfo::no_newline );
            for( const std::pair<bodypart_str_id, float> &element : sorted_lex( dt_protec ) ) {
                info.emplace_back( "CBM", " " + body_part_name_as_heading( element.first, 1 ),
                                   " <num>", iteminfo::no_newline, element.second );
            }
        }
    }

    if( !bid->stat_bonus.empty() ) {
        info.emplace_back( "DESCRIPTION", _( "<bold>Stat Bonus</bold>:" ),
                           iteminfo::no_newline );
        for( const auto &element : bid->stat_bonus ) {
            info.emplace_back( "CBM", " " + get_stat_name( element.first ), " <num>",
                               iteminfo::no_newline, static_cast<double>( element.second ) );
        }
    }

    const units::mass weight_bonus = bid->weight_capacity_bonus;
    const float weight_modif = bid->weight_capacity_modifier;
    if( weight_modif != 1 ) {
        std::string modifier;
        if( weight_modif < 1 ) {
            modifier = "<num><bad>x</bad>";
        } else {
            modifier = "<num><color_light_green>x</color>";
        }
        info.emplace_back( "CBM",
                           _( "<bold>Weight capacity modifier</bold>: " ), modifier,
                           iteminfo::no_newline | iteminfo::is_decimal,
                           weight_modif );
    }
    if( weight_bonus != 0_gram ) {
        std::string bonus;
        if( weight_bonus < 0_gram ) {
            bonus = string_format( "<num> <bad>%s</bad>", weight_units() );
        } else {
            bonus = string_format( "<num> <color_light_green>%s</color>", weight_units() );
        }
        info.emplace_back( "CBM", _( "<bold>Weight capacity bonus</bold>: " ), bonus,
                           iteminfo::no_newline | iteminfo::is_decimal,
                           convert_weight( weight_bonus ) );
    }
}

void item::melee_combat_info( std::vector<iteminfo> &info, const iteminfo_query *parts,
                              int /*batch*/,
                              bool /*debug*/ ) const
{
    const std::string space = "  ";

    std::map<damage_type_id, int> dmg_types;
    for( const damage_type &dt : damage_type::get_all() ) {
        if( dt.physical && dt.melee_only ) {
            int val = damage_melee( dt.id );
            if( val ) {
                dmg_types.emplace( dt.id, val );
            }
        }
    }

    Character &player_character = get_player_character();
    if( parts->test( iteminfo_parts::BASE_DAMAGE ) ) {
        insert_separation_line( info );
        std::string sep;
        if( !dmg_types.empty() ) {
            info.emplace_back( "BASE", _( "<bold>Melee damage</bold>: " ), "", iteminfo::no_newline );
        }
        for( const auto &dmg_type : dmg_types ) {
            if( !dmg_type.first->melee_only ) {
                continue;
            }
            info.emplace_back( "BASE", sep + uppercase_first_letter( dmg_type.first->name.translated() ) + ": ",
                               "", iteminfo::no_newline, dmg_type.second );
            sep = space;
        }
    }

    if( !dmg_types.empty() ) {
        int stam = 0;
        float stam_pct = 0.0f;
        std::map<std::string, double> dps_data;

        bool base_tohit = parts->test( iteminfo_parts::BASE_TOHIT );
        bool base_moves = parts->test( iteminfo_parts::BASE_MOVES );
        bool base_dps = parts->test( iteminfo_parts::BASE_DPS );
        bool base_stamina = parts->test( iteminfo_parts::BASE_STAMINA );
        bool base_dpstam = parts->test( iteminfo_parts::BASE_DPSTAM );

        if( base_stamina || base_dpstam ) {
            stam = player_character.get_total_melee_stamina_cost( this ) * -1;
            stam_pct = std::truncf( stam * 1000.0 / player_character.get_stamina_max() ) / 10;
        }

        if( base_dps || base_dpstam ) {
            dps_data = dps( true, false );
        }

        if( base_tohit ) {
            info.emplace_back( "BASE", space + _( "To-hit bonus: " ), "",
                               iteminfo::show_plus, type->m_to_hit );
        }

        if( base_moves ) {
            info.emplace_back( "BASE", _( "Base moves per attack: " ), "",
                               iteminfo::lower_is_better, attack_time( player_character ) );
        }

        if( base_dps ) {
            info.emplace_back( "BASE", _( "Typical damage per second:" ), "" );
            std::string sep;
            for( const std::pair<const std::string, double> &dps_entry : dps_data ) {
                info.emplace_back( "BASE", sep + dps_entry.first + ": ", "",
                                   iteminfo::no_newline | iteminfo::is_decimal,
                                   dps_entry.second );
                sep = space;
            }
            info.emplace_back( "BASE", "" );
        }

        if( base_stamina ) {
            info.emplace_back( "BASE", _( "<bold>Stamina use</bold>: Costs " ), "", iteminfo::no_newline );
            info.emplace_back( "BASE", ( stam_pct ? _( "about " ) : _( "less than " ) ), "",
                               iteminfo::no_newline | iteminfo::is_decimal | iteminfo::lower_is_better,
                               ( stam_pct > 0.1 ? stam_pct : 0.1 ) );
            info.emplace_back( "BASE", _( "% stamina to swing." ), "" );
        }

        if( base_dpstam ) {
            info.emplace_back( "BASE", _( "Typical damage per stamina:" ), "" );
            std::string sep;
            for( const std::pair<const std::string, double> &dps_entry : dps_data ) {
                info.emplace_back( "BASE", sep + dps_entry.first + ": ", "",
                                   iteminfo::no_newline | iteminfo::is_decimal,
                                   100 * dps_entry.second / stam );
                sep = space;
            }
            info.emplace_back( "BASE", "" );
        }
    }

    if( parts->test( iteminfo_parts::DESCRIPTION_TECHNIQUES ) ) {
        std::set<matec_id> all_techniques = type->techniques;
        all_techniques.insert( techniques.begin(), techniques.end() );

        if( !all_techniques.empty() ) {
            const std::vector<matec_id> all_tec_sorted = sorted_lex( all_techniques );
            insert_separation_line( info );
            info.emplace_back( "DESCRIPTION", _( "<bold>Techniques when wielded</bold>: " ) +
            enumerate_as_string( all_tec_sorted, []( const matec_id & tid ) {
                return string_format( "<stat>%s</stat>: <info>%s</info> <info>%s</info>", tid.obj().name,
                                      tid.obj().description, tid.obj().condition_desc );
            } ) );
        }
    }

    // display which martial arts styles character can use with this weapon
    if( parts->test( iteminfo_parts::DESCRIPTION_APPLICABLEMARTIALARTS ) ) {
        const std::string valid_styles = player_character.martial_arts_data->enumerate_known_styles(
                                             typeId() );
        if( !valid_styles.empty() ) {
            insert_separation_line( info );
            info.emplace_back( "DESCRIPTION",
                               _( "You know how to use this with these martial arts "
                                  "styles: " ) + valid_styles );
        }
    }

    if( !is_gunmod() && has_flag( flag_REACH_ATTACK ) &&
        parts->test( iteminfo_parts::DESCRIPTION_GUNMOD_ADDREACHATTACK ) ) {
        insert_separation_line( info );
        if( has_flag( flag_REACH3 ) ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* This item can be used to make <stat>long reach "
                                  "attacks</stat>." ) );
        } else {
            info.emplace_back( "DESCRIPTION",
                               _( "* This item can be used to make <stat>reach "
                                  "attacks</stat>." ) );
        }
    }

    ///\EFFECT_MELEE >2 allows seeing melee damage stats on weapons
    if( ( player_character.get_skill_level( skill_melee ) >= 3 &&
          ( !dmg_types.empty() || type->m_to_hit > 0 ) ) || debug_mode ) {
        bodypart_id bp = bodypart_id( "torso" );
        damage_instance non_crit;
        player_character.roll_all_damage( false, non_crit, true, *this, "WEAPON", nullptr, bp );
        damage_instance crit;
        player_character.roll_all_damage( true, crit, true, *this, "WEAPON", nullptr, bp );
        int attack_cost = player_character.attack_speed( *this );
        insert_separation_line( info );
        if( parts->test( iteminfo_parts::DESCRIPTION_MELEEDMG ) ) {
            info.emplace_back( "DESCRIPTION", _( "<bold>Average melee damage</bold>:" ) );
        }
        // Chance of critical hit
        if( parts->test( iteminfo_parts::DESCRIPTION_MELEEDMG_CRIT ) ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "Critical hit chance <neutral>%d%% - %d%%</neutral>" ),
                                              static_cast<int>( player_character.crit_chance( 0, 100, *this ) *
                                                      100 ),
                                              static_cast<int>( player_character.crit_chance( 100, 0, *this ) *
                                                      100 ) ) );
        }
        if( parts->test( iteminfo_parts::DESCRIPTION_MELEEDMG_TYPES ) ) {
            for( const damage_info_order &dio : damage_info_order::get_all(
                     damage_info_order::info_type::MELEE ) ) {
                // NOTE: Using "BASE" instead of "DESCRIPTION", so numerical formatting will work
                // (output.cpp:format_item_info does not interpolate <num> for DESCRIPTION info)
                info.emplace_back( "BASE", string_format( "%s: ", uppercase_first_letter( dio.verb.translated() ) ),
                                   "<num>", iteminfo::no_newline, non_crit.type_damage( dio.dmg_type ) );
                //~ Label used in the melee damage section in the item info screen (ex: "  Critical bash: ")
                //~ %1$s = a prepended space, %2$s = the name of the damage type (bash, cut, pierce, etc.)
                info.emplace_back( "BASE", string_format( _( "%1$sCritical %2$s: " ), space,
                                   dio.dmg_type->name.translated() ),
                                   "<num>", iteminfo::no_flags, crit.type_damage( dio.dmg_type ) );
            }
        }
        // Moves
        if( parts->test( iteminfo_parts::DESCRIPTION_MELEEDMG_MOVES ) ) {
            info.emplace_back( "BASE", _( "Adjusted moves per attack: " ), "<num>",
                               iteminfo::lower_is_better, attack_cost );
        }
        insert_separation_line( info );
    }
}

void item::contents_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                          bool /*debug*/ ) const
{
    if( ( toolmods().empty() && gunmods().empty() && contents.empty() ) ||
        !parts->test( iteminfo_parts::DESCRIPTION_CONTENTS ) ) {
        return;
    }
    const std::string space = "  ";

    for( const item *mod : is_gun() ? gunmods() : toolmods() ) {
        // mod_str = "(Integrated) Mod: %mod_name% (%mod_location%) "
        std::string mod_str;
        if( mod->is_irremovable() ) {
            mod_str = _( "Integrated mod: " );
        } else {
            mod_str = _( "Mod: " );
        }
        mod_str += string_format( "<bold>%s</bold>", mod->tname() );
        if( mod->type->gunmod ) {
            mod_str += string_format( " (%s) ", mod->type->gunmod->location.name() );
        }
        insert_separation_line( info );
        info.emplace_back( "DESCRIPTION", mod_str );
        info.emplace_back( "DESCRIPTION", mod->type->description.translated() );
    }
    bool contents_header = false;
    for( const item *contents_item : contents.all_items_top() ) {
        if( !contents_header ) {
            insert_separation_line( info );
            info.emplace_back( "DESCRIPTION", _( "<bold>Contents of this item</bold>:" ) );
            contents_header = true;
        } else {
            // Separate items with a blank line
            info.emplace_back( "DESCRIPTION", space );
        }

        const translation &description = contents_item->type->description;

        if( contents_item->made_of_from_type( phase_id::LIQUID ) ) {
            units::volume contents_volume = contents_item->volume() * batch;
            info.emplace_back( "DESCRIPTION", colorize( contents_item->display_name(),
                               contents_item->color_in_inventory() ) );
            info.emplace_back( vol_to_info( "CONTAINER", description + space, contents_volume ) );
        } else {
            info.emplace_back( "DESCRIPTION", colorize( contents_item->display_name(),
                               contents_item->color_in_inventory() ) );
            info.emplace_back( "DESCRIPTION", description.translated() );
        }
    }
}

void item::properties_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                            bool debug ) const
{
    if( is_null() ) {
        return;
    }

    insert_separation_line( info );

    if( parts->test( iteminfo_parts::BASE_RIGIDITY ) ) {
        bool not_rigid = false;
        if( const islot_armor *t = find_armor_data() ) {
            bool any_encumb_increase = std::any_of( t->data.begin(), t->data.end(),
            []( const armor_portion_data & data ) {
                return data.encumber != data.max_encumber;
            } );
            if( any_encumb_increase ) {
                info.emplace_back( "BASE",
                                   _( "* This items pockets are <info>not rigid</info>.  Its"
                                      " volume and encumbrance increase with contents." ) );
                not_rigid = true;
            }
        }
        if( !not_rigid && !all_pockets_rigid() && !is_corpse() ) {
            info.emplace_back( "BASE",
                               _( "* This items pockets are <info>not rigid</info>.  Its"
                                  " volume increases with contents." ) );
        }
    }

    if( parts->test( iteminfo_parts::DESCRIPTION_CONDUCTIVITY ) ) {
        if( !conductive() ) {
            info.emplace_back( "BASE", _( "* This item <good>does not "
                                          "conduct</good> electricity." ) );
        } else if( has_flag( flag_CONDUCTIVE ) ) {
            info.emplace_back( "BASE",
                               _( "* This item effectively <bad>conducts</bad> "
                                  "electricity, as it has no guard." ) );
        } else {
            info.emplace_back( "BASE", _( "* This item <bad>conducts</bad> electricity." ) );
        }
    }

    if( parts->test( iteminfo_parts::DESCRIPTION_FLAGS ) ) {
        // concatenate base and acquired flags...
        cata::flat_set<flag_id> flags;
        flags.insert( get_flags().begin(), get_flags().end() );
        flags.insert( type->get_flags().begin(), type->get_flags().end() );
        flags.insert( inherited_tags_cache.begin(), inherited_tags_cache.end() );

        // ...and display those which have an info description
        for( const flag_id &e : sorted_lex( flags ) ) {
            const json_flag &f = e.obj();
            if( !f.info().empty() ) {
                info.emplace_back( "DESCRIPTION", string_format( "* %s", f.info() ) );
            }
        }
    }

    armor_fit_info( info, parts, batch, debug );

    if( ethereal ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This item disappears as soon as its timer runs out, unless it is permanent or a comestible." ) );
    }

    if( has_flag( flag_RADIO_ACTIVATION ) &&
        parts->test( iteminfo_parts::DESCRIPTION_RADIO_ACTIVATION ) ) {
        if( has_flag( flag_RADIO_MOD ) ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* This item has been modified to listen to <info>radio "
                                  "signals</info>.  It can still be activated manually." ) );
        } else {
            info.emplace_back( "DESCRIPTION",
                               _( "* This item can only be activated by a <info>radio "
                                  "signal</info>." ) );
        }

        std::string signame;
        if( has_flag( flag_RADIOSIGNAL_1 ) ) {
            signame = _( "<color_c_red>red</color> radio signal" );
        } else if( has_flag( flag_RADIOSIGNAL_2 ) ) {
            signame = _( "<color_c_blue>blue</color> radio signal" );
        } else if( has_flag( flag_RADIOSIGNAL_3 ) ) {
            signame = _( "<color_c_green>green</color> radio signal" );
        }
        if( parts->test( iteminfo_parts::DESCRIPTION_RADIO_ACTIVATION_CHANNEL ) ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "* It will be activated by the %s." ),
                                              signame ) );
        }

        if( has_flag( flag_BOMB ) && has_flag( flag_RADIO_INVOKE_PROC ) &&
            parts->test( iteminfo_parts::DESCRIPTION_RADIO_ACTIVATION_PROC ) ) {
            info.emplace_back( "DESCRIPTION",
                               _( "* Activating this item with a <info>radio signal</info> will "
                                  "<neutral>detonate</neutral> it immediately." ) );
        }
    }

    if( parts->test( iteminfo_parts::DESCRIPTION_DIE ) && this->get_var( "die_num_sides", 0 ) != 0 ) {
        info.emplace_back( "DESCRIPTION",
                           string_format( _( "* This item can be used as a <info>die</info>, "
                                             "and has <info>%d</info> sides." ),
                                          static_cast<int>( this->get_var( "die_num_sides",
                                                  0 ) ) ) );
    }

}

void item::final_info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch,
                       bool /* debug */ ) const
{
    if( is_null() ) {
        return;
    }

    avatar &player_character = get_avatar();

    if( has_flag( flag_LEAK_DAM ) && has_flag( flag_RADIOACTIVE ) && damage() > 0
        && parts->test( iteminfo_parts::DESCRIPTION_RADIOACTIVITY_DAMAGED ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* The casing of this item has <neutral>cracked</neutral>, "
                              "revealing an <info>ominous green glow</info>." ) );
    }

    if( has_flag( flag_LEAK_ALWAYS ) && has_flag( flag_RADIOACTIVE ) &&
        parts->test( iteminfo_parts::DESCRIPTION_RADIOACTIVITY_ALWAYS ) ) {
        info.emplace_back( "DESCRIPTION",
                           _( "* This object is <neutral>surrounded</neutral> by a "
                              "<info>sickly green glow</info>." ) );
    }

    if( is_brewable() ) {
        const item &brewed = *this;
        if( parts->test( iteminfo_parts::DESCRIPTION_BREWABLE_DURATION ) ) {
            const time_duration btime = brewed.brewing_time();
            int btime_i = to_days<int>( btime );
            if( btime <= 2_days ) {
                btime_i = to_hours<int>( btime );
                info.emplace_back( "DESCRIPTION",
                                   string_format( n_gettext( "* Once set in a vat, this "
                                                  "will ferment in around %d hour.",
                                                  "* Once set in a vat, this will ferment in "
                                                  "around %d hours.", btime_i ), btime_i ) );
            } else {
                info.emplace_back( "DESCRIPTION",
                                   string_format( n_gettext( "* Once set in a vat, this "
                                                  "will ferment in around %d day.",
                                                  "* Once set in a vat, this will ferment in "
                                                  "around %d days.", btime_i ), btime_i ) );
            }
        }
        if( parts->test( iteminfo_parts::DESCRIPTION_BREWABLE_PRODUCTS ) ) {
            for( const itype_id &res : brewed.brewing_results() ) {
                info.emplace_back( "DESCRIPTION",
                                   string_format( _( "* Fermenting this will produce "
                                                     "<neutral>%s</neutral>." ),
                                                  nname( res, brewed.charges ) ) );
            }
        }
    }

    if( parts->test( iteminfo_parts::DESCRIPTION_FAULTS ) ) {
        for( const fault_id &e : faults ) {
            //~ %1$s is the name of a fault and %2$s is the description of the fault
            info.emplace_back( "DESCRIPTION", string_format( _( "* <bad>%1$s</bad>.  %2$s" ),
                               e.obj().name(), e.obj().description() ) );
        }
    }

    // does the item fit in any holsters?
    std::vector<const itype *> holsters = Item_factory::find( [this]( const itype & e ) {
        if( !e.can_use( "holster" ) ) {
            return false;
        }
        const holster_actor *ptr = dynamic_cast<const holster_actor *>
                                   ( e.get_use( "holster" )->get_actor_ptr() );
        const item holster_item( &e );
        return ptr->can_holster( holster_item, *this ) && !item_is_blacklisted( holster_item.typeId() );
    } );

    if( !holsters.empty() && parts->test( iteminfo_parts::DESCRIPTION_HOLSTERS ) ) {
        insert_separation_line( info );
        std::vector<std::string> holsters_str;
        holsters_str.reserve( holsters.size() );
        for( const itype *e : holsters ) {
            holsters_str.emplace_back( player_character.is_wearing( e->get_id() )
                                       ? string_format( "<good>%s</good>", e->nname( 1 ) )
                                       : e->nname( 1 ) );
        }
        info.emplace_back( "DESCRIPTION", _( "<bold>Can be stored in</bold>: " ) +
                           enumerate_lcsorted_with_limit( holsters_str, 30 ) );
        info.back().sName += ".";
    }

    if( parts->test( iteminfo_parts::DESCRIPTION_ACTIVATABLE_TRANSFORMATION ) ) {
        insert_separation_line( info );
        for( const auto &u : type->use_methods ) {
            const delayed_transform_iuse *tt = dynamic_cast<const delayed_transform_iuse *>
                                               ( u.second.get_actor_ptr() );
            if( tt == nullptr ) {
                continue;
            }
            const int time_to_do = tt->time_to_do( *this );
            if( time_to_do <= 0 ) {
                info.emplace_back( "DESCRIPTION",
                                   _( "It's done and <info>can be activated</info>." ) );
            } else {
                const std::string time = to_string_clipped( time_duration::from_turns( time_to_do ) );
                info.emplace_back( "DESCRIPTION",
                                   string_format( _( "It will be done in %s." ),
                                                  time.c_str() ) );
            }
        }
    }

    std::map<std::string, std::string>::const_iterator item_note = item_vars.find( "item_note" );

    if( item_note != item_vars.end() && parts->test( iteminfo_parts::DESCRIPTION_NOTES ) ) {
        insert_separation_line( info );
        std::string ntext;
        std::map<std::string, std::string>::const_iterator item_note_tool =
            item_vars.find( "item_note_tool" );
        const use_function *use_func =
            item_note_tool != item_vars.end() ?
            item_controller->find_template(
                itype_id( item_note_tool->second ) )->get_use( "inscribe" ) :
            nullptr;
        const inscribe_actor *use_actor =
            use_func ? dynamic_cast<const inscribe_actor *>( use_func->get_actor_ptr() ) : nullptr;
        if( use_actor ) {
            //~ %1$s: gerund (e.g. carved), %2$s: item name, %3$s: inscription text
            ntext = string_format( pgettext( "carving", "%1$s on the %2$s is: %3$s" ),
                                   use_actor->gerund, tname(), item_note->second );
        } else {
            //~ %1$s: inscription text
            ntext = string_format( pgettext( "carving", "Note: %1$s" ), item_note->second );
        }
        info.emplace_back( "DESCRIPTION", ntext );
    }

    // Price and barter value
    const int price_preapoc = price( false ) * batch;
    const int price_postapoc = price( true ) * batch;
    if( parts->test( iteminfo_parts::BASE_PRICE ) ) {
        insert_separation_line( info );
        info.emplace_back( "BASE", _( "Price: " ), _( "$<num>" ),
                           iteminfo::is_decimal | iteminfo::no_newline,
                           static_cast<double>( price_preapoc ) / 100 );
    }
    if( price_preapoc != price_postapoc && parts->test( iteminfo_parts::BASE_BARTER ) ) {
        const std::string space = "  ";
        info.emplace_back( "BASE", space + _( "Barter value: " ), _( "$<num>" ),
                           iteminfo::is_decimal,
                           static_cast<double>( price_postapoc ) / 100 );
    }

    // Recipes using this item as an ingredient
    if( parts->test( iteminfo_parts::DESCRIPTION_APPLICABLE_RECIPES ) ) {
        // with the inventory display allowing you to select items, showing the things you could make with contained items could be confusing.
        const itype_id &tid = typeId();
        const inventory &crafting_inv = player_character.crafting_inventory();
        const recipe_subset available_recipe_subset = player_character.get_available_recipes(
                    crafting_inv );
        const std::set<const recipe *> &item_recipes = available_recipe_subset.of_component( tid );

        insert_separation_line( info );
        if( item_recipes.empty() ) {
            info.emplace_back( "DESCRIPTION", _( "You know of nothing you could craft with it." ) );
        } else {
            std::vector<std::string> crafts;
            for( const recipe *r : item_recipes ) {
                const bool can_make = r->deduped_requirements()
                                      .can_make_with_inventory( crafting_inv, r->get_component_filter() );

                // Endarken recipes that can't be constructed with the survivor's inventory
                const std::string name = r->result_name( /* decorated = */ true );
                crafts.emplace_back( can_make ? name : string_format( "<dark>%s</dark>", name ) );
            }
            const std::string recipes = enumerate_lcsorted_with_limit( crafts, 15 );
            info.emplace_back( " DESCRIPTION", string_format( _( "You could use it to craft: %s" ), recipes ) );
        }
    }

    if( is_armor() ) {
        const ret_val<void> can_wear = player_character.can_wear( *this, true );
        if( !can_wear.success() ) {
            insert_separation_line( info );
            // can_wear returns a translated string
            info.emplace_back( "DESCRIPTION", string_format( "<bad>%s</bad>", can_wear.str() ) );
        }
    }

    // Vehicle parts or appliances using this item as a component
    if( parts->test( iteminfo_parts::DESCRIPTION_VEHICLE_PARTS ) ) {
        const itype_id tid = typeId();
        std::vector<vpart_info> vparts;

        for( const vpart_info &vpi : vehicles::parts::get_all() ) {
            if( vpi.base_item == tid ) {
                vparts.push_back( vpi );
            }
        }

        const auto print_parts = [&info, &player_character](
                                     const std::vector<vpart_info> &vparts,
                                     const std::string_view install_where,
                                     const std::function<bool( const vpart_info & )> &predicate
        ) {
            std::vector<std::string> result_parts;
            for( const vpart_info &vp : vparts ) {
                const bool is_duplicate = std::any_of( result_parts.begin(), result_parts.end(),
                [name = vp.name()]( const std::string & r ) {
                    return r == name;
                } );

                if( is_duplicate || !predicate( vp ) ) {
                    continue; // skip part variants, they have same part names
                }

                const bool can_install = player_character.meets_skill_requirements( vp.install_skills );
                const std::string name = vp.name();

                // Endarken parts that can't be installed with the character's skills
                result_parts.emplace_back( can_install ? name : string_format( "<dark>%s</dark>", name ) );
            }

            if( result_parts.empty() ) {
                return;
            }

            const std::string installable_parts = enumerate_lcsorted_with_limit( result_parts, 15 );

            insert_separation_line( info );
            info.emplace_back( " DESCRIPTION", string_format( install_where, installable_parts ) );
        };

        print_parts( vparts, _( "You could install it in a vehicle: %s" ),
        []( const vpart_info & vp ) {
            return !vp.has_flag( vpart_bitflags::VPFLAG_APPLIANCE );
        } );

        print_parts( vparts, _( "You could install it as an appliance: %s" ),
        []( const vpart_info & vp ) {
            return vp.has_flag( vpart_bitflags::VPFLAG_APPLIANCE );
        } );
    }
}

void item::ascii_art_info( std::vector<iteminfo> &info, const iteminfo_query * /* parts */,
                           int  /* batch */,
                           bool /* debug */ ) const
{
    if( is_null() ) {
        return;
    }

    if( get_option<bool>( "ENABLE_ASCII_ART" ) ) {
        ascii_art_id art = type->picture_id;
        if( has_itype_variant() && itype_variant().art.is_valid() ) {
            art = itype_variant().art;
        }
        if( art.is_valid() ) {
            for( const std::string &line : art->picture ) {
                info.emplace_back( "DESCRIPTION", line );
            }
        }
    }
}

std::string item::info( std::vector<iteminfo> &info, const iteminfo_query *parts, int batch ) const
{
    const bool debug = g != nullptr && debug_mode;

    if( parts == nullptr ) {
        parts = &iteminfo_query::all;
    }

    info.clear();

    if( !is_null() ) {
        basic_info( info, parts, batch, debug );
        debug_info( info, parts, batch, debug );
    }

    // Different items might want to present the info blocks in different order, depending on what
    // they consider more relevant.
    std::vector<std::string> all_info_blocks = {"big_block_of_stuff", "contents", "melee_combat_info", "footer"};
    std::vector<std::string> prio_info_blocks = { };
    item_category_id my_cat_id = get_category_shallow().id;
    if( is_maybe_melee_weapon() ) {
        prio_info_blocks.emplace_back( "melee_combat_info" );
    } else if( my_cat_id == item_category_container ) {
        prio_info_blocks.emplace_back( "contents" );
    }
    std::vector<std::string> info_block_ordered = prio_info_blocks;
    for( const std::string &blockname : all_info_blocks ) {
        if( std::find( info_block_ordered.begin(), info_block_ordered.end(),
                       blockname ) == info_block_ordered.end() ) {
            info_block_ordered.push_back( blockname );
        }
    }

    for( const auto &blockname : info_block_ordered ) {
        if( blockname == "melee_combat_info" ) {

            melee_combat_info( info, parts, batch, debug );

        } else if( blockname == "big_block_of_stuff" ) {

            if( is_medication() ) {
                med_info( this, info, parts, batch, debug );
            }

            if( is_food() ) {
                food_info( this, info, parts, batch, debug );
            }

            magazine_info( info, parts, batch, debug );
            ammo_info( info, parts, batch, debug );

            const item *gun = nullptr;
            if( is_gun() ) {
                gun = this;
                const gun_mode aux = gun_current_mode();
                // if we have an active auxiliary gunmod display stats for this instead
                if( aux && aux->is_gunmod() && aux->is_gun() &&
                    parts->test( iteminfo_parts::DESCRIPTION_AUX_GUNMOD_HEADER ) ) {
                    gun = &*aux;
                    info.emplace_back( "DESCRIPTION",
                                       string_format( _( "Stats of the active <info>gunmod (%s)</info> "
                                                         "are shown." ), gun->tname() ) );
                }
            }
            if( gun != nullptr ) {
                gun_info( gun, info, parts, batch, debug );
            }

            gunmod_info( info, parts, batch, debug );
            armor_info( info, parts, batch, debug );
            animal_armor_info( info, parts, batch, debug );
            book_info( info, parts, batch, debug );
            battery_info( info, parts, batch, debug );
            tool_info( info, parts, batch, debug );
            actions_info( info, parts, batch, debug );
            component_info( info, parts, batch, debug );
            qualities_info( info, parts, batch, debug );

            // Uses for item (bandaging quality, holster capacity, grenade activation)
            if( parts->test( iteminfo_parts::DESCRIPTION_USE_METHODS ) ) {
                for( const std::pair<const std::string, use_function> &method : type->use_methods ) {
                    insert_separation_line( info );
                    method.second.dump_info( *this, info );
                }
            }

            repair_info( info, parts, batch, debug );
            enchantment_info( info, parts, batch, debug );
            disassembly_info( info, parts, batch, debug );
            properties_info( info, parts, batch, debug );
            bionic_info( info, parts, batch, debug );

        } else if( blockname == "contents" ) {

            contents.info( info, parts );
            contents_info( info, parts, batch, debug );

        } else if( blockname == "footer" ) {

            final_info( info, parts, batch, debug );
            ascii_art_info( info, parts, batch, debug );

        } else {

            debugmsg( "Trying to show info block named %s which is not valid.", blockname );

        }
    }

    if( !info.empty() && info.back().sName == "--" ) {
        info.pop_back();
    }

    return format_item_info( info, {} );
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
    for( const item *elem : contents.all_items_top( item_pocket::pocket_type::MOD ) ) {
        const cata::value_ptr<islot_gunmod> &mod = elem->type->gunmod;
        if( mod && mod->location == location ) {
            result--;
        }
    }
    return result;
}

int item::engine_displacement() const
{
    return type->engine ? type->engine->displacement : 0;
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
            if( player_character.magic->knows_spell( sp_id ) &&
                !player_character.magic->get_spell( sp_id ).is_max_level( player_character ) ) {
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
    } else if( is_relic() && !has_flag( flag_MUNDANE ) ) {
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
        if( player_character.allergy_type( *this ) != morale_null ) {
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
        bool has_gun = player_character.has_item_with( [this]( const item & i ) {
            return i.is_gun() && i.ammo_types().count( ammo_type() );
        } );
        bool has_mag = player_character.has_item_with( [this]( const item & i ) {
            return ( i.is_gun() && i.magazine_integral() && i.ammo_types().count( ammo_type() ) ) ||
                   ( i.is_magazine() && i.ammo_types().count( ammo_type() ) );
        } );
        if( has_gun && has_mag ) {
            ret = c_green;
        } else if( has_gun || has_mag ) {
            ret = c_light_red;
        }
    } else if( is_magazine() ) {
        // Magazines are green if you have guns and ammo for them
        // ltred if you have one but not the other
        bool has_gun = player_character.has_item_with( [this]( const item & it ) {
            return it.is_gun() && it.magazine_compatible().count( typeId() ) > 0;
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

void item::on_wear( Character &p )
{
    if( is_sided() && get_side() == side::BOTH ) {
        if( has_flag( flag_SPLINT ) ) {
            set_side( side::LEFT );
            if( ( covers( bodypart_id( "leg_l" ) ) && p.is_limb_broken( bodypart_id( "leg_r" ) ) &&
                  !p.worn_with_flag( flag_SPLINT, bodypart_id( "leg_r" ) ) ) ||
                ( covers( bodypart_id( "arm_l" ) ) && p.is_limb_broken( bodypart_id( "arm_r" ) ) &&
                  !p.worn_with_flag( flag_SPLINT, bodypart_id( "arm_r" ) ) ) ) {
                set_side( side::RIGHT );
            }
        } else if( has_flag( flag_TOURNIQUET ) ) {
            set_side( side::LEFT );
            if( ( covers( bodypart_id( "leg_l" ) ) &&
                  p.has_effect( effect_bleed, body_part_leg_r ) &&
                  !p.worn_with_flag( flag_TOURNIQUET, bodypart_id( "leg_r" ) ) ) ||
                ( covers( bodypart_id( "arm_l" ) ) && p.has_effect( effect_bleed, body_part_arm_r ) &&
                  !p.worn_with_flag( flag_TOURNIQUET, bodypart_id( "arm_r" ) ) ) ) {
                set_side( side::RIGHT );
            }
        } else {
            // for sided items wear the item on the side which results in least encumbrance
            int lhs = 0;
            int rhs = 0;
            set_side( side::LEFT );
            for( const bodypart_id &bp : p.get_all_body_parts() ) {
                lhs += p.get_part_encumbrance_data( bp ).encumbrance;
            }

            set_side( side::RIGHT );
            for( const bodypart_id &bp : p.get_all_body_parts() ) {
                rhs += p.get_part_encumbrance_data( bp ).encumbrance;
            }

            set_side( lhs <= rhs ? side::LEFT : side::RIGHT );
        }
    }

    // if game is loaded - don't want ownership assigned during char creation
    if( get_player_character().getID().is_valid() ) {
        handle_pickup_ownership( p );
    }
    p.on_item_acquire( *this );
    p.on_item_wear( *this );
}

void item::on_takeoff( Character &p )
{
    p.on_item_takeoff( *this );

    if( is_sided() ) {
        set_side( side::BOTH );
    }
    // reset any pockets to not have restrictions
    if( is_ablative() ) {
        for( item_pocket *pocket : get_all_standard_pockets() ) {
            pocket->set_no_rigid( {} );
        }
    }
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

        int penalty = get_var( "volume", volume() / units::legacy_volume_factor ) * d;
        mv += penalty;
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

void item::on_wield( Character &you )
{
    int wield_cost = on_wield_cost( you );
    you.moves -= wield_cost;

    std::string msg;

    msg = _( "You wield your %s." );

    // if game is loaded - don't want ownership assigned during char creation
    if( get_player_character().getID().is_valid() ) {
        handle_pickup_ownership( you );
    }
    you.add_msg_if_player( m_neutral, msg, tname() );

    if( !you.martial_arts_data->selected_is_none() ) {
        you.martial_arts_data->martialart_use_message( you );
    }

    // Update encumbrance and discomfort in case we were wearing it
    you.flag_encumbrance();
    you.calc_discomfort();
    you.on_item_acquire( *this );
}

void item::handle_pickup_ownership( Character &c )
{
    if( is_owned_by( c ) ) {
        return;
    }
    Character &player_character = get_player_character();
    // Add ownership to item if unowned
    if( owner.is_null() ) {
        set_owner( c );
    } else {
        if( !is_owned_by( c ) && c.is_avatar() ) {
            std::vector<npc *> witnesses;
            for( npc &elem : g->all_npcs() ) {
                if( rl_dist( elem.pos(), player_character.pos() ) < MAX_VIEW_DISTANCE &&
                    elem.get_faction() && is_owned_by( elem ) && elem.sees( player_character.pos() ) ) {
                    elem.say( "<witnessed_thievery>", 7 );
                    npc *npc_to_add = &elem;
                    witnesses.push_back( npc_to_add );
                }
            }
            if( !witnesses.empty() ) {
                set_old_owner( get_owner() );
                bool guard_chosen = false;
                for( npc *elem : witnesses ) {
                    if( elem->myclass == NC_BOUNTY_HUNTER ) {
                        guard_chosen = true;
                        elem->witness_thievery( &*this );
                        break;
                    }
                }
                if( !guard_chosen ) {
                    random_entry( witnesses )->witness_thievery( &*this );
                }
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
    p.invalidate_weight_carried_cache();
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
}

void item::on_contents_changed()
{
    contents.update_open_pockets();
    cached_relative_encumbrance.reset();
    encumbrance_update_ = true;
    update_inherited_flags();
    if( empty_container() ) {
        clear_automatic_whitelist();
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

std::string item::tname( unsigned int quantity, bool with_prefix, unsigned int truncate,
                         bool with_contents_full, bool with_collapsed, bool with_contents_abbrev ) const
{
    // item damage and/or fouling level
    std::string damtext;

    // add first prefix if item has a fault that defines a prefix (prioritize?)
    for( const fault_id &f : faults ) {
        const std::string prefix = f->item_prefix();
        if( !prefix.empty() ) {
            damtext = prefix + " ";
            break;
        }
    }
    damtext += dirt_symbol();

    if( get_option<std::string>( "ASTERISK_POSITION" ) == "prefix" && with_prefix ) {
        if( is_favorite ) {
            damtext += _( "* " ); // Display asterisk for favorite items, before item's name
        }
    }

    // for portions of string that have <color_ etc in them, this aims to truncate the whole string correctly
    unsigned int truncate_override = 0;

    if( ( damage() != 0 || ( degradation() > 0 && degradation() >= max_damage() / 5 ) ||
          ( get_option<bool>( "ITEM_HEALTH_BAR" ) && is_armor() ) ) && !is_null() && with_prefix ) {
        damtext += durability_indicator();
        if( get_option<bool>( "ITEM_HEALTH_BAR" ) ) {
            // get the utf8 width of the tags
            truncate_override = utf8_width( damtext, false ) - utf8_width( damtext, true );
        }
    }

    std::string vehtext;
    if( is_engine() && engine_displacement() > 0 ) {
        vehtext = string_format( pgettext( "vehicle adjective", "%gL " ),
                                 engine_displacement() / 100.0f );


    } else if( is_wheel() && type->wheel->diameter > 0 ) {
        vehtext = string_format( pgettext( "vehicle adjective", "%d\" " ), type->wheel->diameter );
    }

    std::string burntext;
    if( with_prefix && !made_of_from_type( phase_id::LIQUID ) ) {
        if( volume() >= 1_liter && burnt * 125_ml >= volume() ) {
            burntext = pgettext( "burnt adjective", "badly burnt " );
        } else if( burnt > 0 ) {
            burntext = pgettext( "burnt adjective", "burnt " );
        }
    }

    std::string maintext;
    std::string contents_suffix_text;

    if( is_corpse() || typeId() == itype_blood || item_vars.find( "name" ) != item_vars.end() ) {
        maintext = type_name( quantity );
    } else if( ( ( is_gun() || is_tool() || is_magazine() ) && !is_power_armor() ) ||
               contents.has_additional_pockets() ) {
        int amt = 0;
        maintext = label( quantity );
        for( const item *mod : is_gun() ? gunmods() : toolmods() ) {
            if( !type->gun || !type->gun->built_in_mods.count( mod->typeId() ) ||
                !type->gun->default_mods.count( mod->typeId() ) ) {
                amt++;
            }
        }
        amt += contents.get_added_pockets().size();
        if( amt ) {
            maintext += string_format( "+%d", amt );
        }
    } else if( is_craft() ) {
        if( typeId() == itype_disassembly ) {
            maintext = string_format( _( "in progress disassembly of %s" ),
                                      craft_data_->making->result_name() );
        } else {
            maintext = string_format( _( "in progress %s" ), craft_data_->making->result_name() );
        }
        if( charges > 1 ) {
            maintext += string_format( " (%d)", charges );
        }
        const int percent_progress = item_counter / 100000;
        maintext += string_format( " (%d%%)", percent_progress );
    } else {
        maintext = label( quantity ) + ( is_armor() && has_clothing_mod() ? "+1" : "" );
    }

    std::vector<const item_pocket *> pkts = get_all_contained_pockets();
    bool wl = false;
    bool bl = false;
    bool player_wbl = false;
    for( item_pocket const *pkt : pkts ) {
        bool const wl_ = !pkt->settings.get_item_whitelist().empty() ||
                         !pkt->settings.get_category_whitelist().empty();
        bool const bl_ = !pkt->settings.get_item_blacklist().empty() ||
                         !pkt->settings.get_category_blacklist().empty();
        player_wbl |= ( wl_ || bl_ ) && pkt->settings.was_edited();
        wl |= wl_;
        bl |= bl_;
    }
    std::string const wltext =
        wl || bl ? colorize( "⁺", player_wbl ? c_light_gray : c_dark_gray ) : std::string();

    /* only expand full contents name if with_contents == true */
    if( with_contents_full && !contents.empty() && contents_only_one_type() ) {
        const item &contents_item = contents.first_item();
        const unsigned contents_count =
            ( ( contents_item.made_of( phase_id::LIQUID ) ||
                contents_item.is_food() || contents_item.count_by_charges() ) &&
              contents_item.charges > 1 )
            ? contents_item.charges
            : contents.num_item_stacks();

        if( !contents_item.is_null() ) {
            // with_contents=false for nested items to prevent excessively long names
            const std::string contents_tname = contents_item.tname( contents_count, true, 0, false, false,
                                               contents_count == 1 );
            std::string const ctnc = colorize( contents_tname, contents_item.color_in_inventory() );
            if( contents_count == 1 || !ammo_types().empty() ) {
                // Don't append an item count for single items, or items that are ammo-exclusive
                // (eg: quivers), as they format their own counts.
                contents_suffix_text = string_format( pgettext( "item name",
                                                      //~ [container item name] " > [inner item name]
                                                      " > %1$s" ), ctnc );
            } else if( contents_count != 0 ) {
                // Otherwise, add a contents count!
                contents_suffix_text = string_format( pgettext( "item name",
                                                      //~ [container item name] " > [inner item name] (qty)
                                                      " > %1$s (%2$zd)" ), ctnc, contents_count );
            }

            if( is_collapsed() && with_collapsed ) {
                contents_suffix_text += string_format( " %s", _( "hidden" ) );
            }
        }
    } else if( with_contents_abbrev && !contents.empty_container() &&
               contents.num_item_stacks() != 0 ) {
        std::string const suffix =
            npgettext( "item name",
                       //~ [container item name] " > [count] item"
                       " > %1$zd%2$s item", " > %1$zd%2$s items", contents.num_item_stacks() );
        std::string const hidden =
            is_collapsed() && with_collapsed ? string_format( " %s", _( "hidden" ) ) : std::string();
        contents_suffix_text = string_format( suffix, contents.num_item_stacks(), hidden );
    }

    Character &player_character = get_player_character();
    std::string tagtext;
    if( is_food() ) {
        if( has_flag( flag_HIDDEN_POISON ) && player_character.get_skill_level( skill_survival ) >= 3 ) {
            tagtext += _( " (poisonous)" );
        } else if( has_flag( flag_HIDDEN_HALLU ) &&
                   player_character.get_skill_level( skill_survival ) >= 5 ) {
            tagtext += _( " (hallucinogenic)" );
        }
    }
    if( has_var( "spawn_location_omt" ) ) {
        tripoint_abs_omt loc( get_var( "spawn_location_omt", tripoint_zero ) );
        tripoint_abs_omt player_loc( ms_to_omt_copy( get_map().getabs( player_character.pos() ) ) );
        int dist = rl_dist( player_loc, loc );
        if( dist < 1 ) {
            tagtext += _( " (from here)" );
        } else if( dist < 6 ) {
            tagtext += _( " (from nearby)" );
        } else if( dist < 30 ) {
            tagtext += _( " (from this area)" );
        } else {
            tagtext += _( " (from far away)" );
        }
    }
    if( ethereal ) {
        tagtext += string_format( _( " (%s turns)" ), get_var( "ethereal" ) );
    } else if( goes_bad() || is_food() ) {
        if( has_own_flag( flag_DIRTY ) ) {
            tagtext += _( " (dirty)" );
        } else if( rotten() ) {
            tagtext += _( " (rotten)" );
        } else if( has_flag( flag_MUSHY ) ) {
            tagtext += _( " (mushy)" );
        } else if( is_going_bad() ) {
            tagtext += _( " (old)" );
        } else if( is_fresh() ) {
            tagtext += _( " (fresh)" );
        }
    }
    if( has_temperature() ) {
        if( has_flag( flag_HOT ) ) {
            tagtext += _( " (hot)" );
        }
        if( has_flag( flag_COLD ) ) {
            tagtext += _( " (cold)" );
        }
        if( has_flag( flag_FROZEN ) ) {
            tagtext += _( " (frozen)" );
        } else if( has_flag( flag_MELTS ) ) {
            tagtext += _( " (melted)" ); // he melted
        }
    }

    const sizing sizing_level = get_sizing( player_character );

    if( sizing_level == sizing::human_sized_small_char ) {
        tagtext += _( " (too big)" );
    } else if( sizing_level == sizing::big_sized_small_char ) {
        tagtext += _( " (huge!)" );
    } else if( sizing_level == sizing::human_sized_big_char ||
               sizing_level == sizing::small_sized_human_char ) {
        tagtext += _( " (too small)" );
    } else if( sizing_level == sizing::small_sized_big_char ) {
        tagtext += _( " (tiny!)" );
    } else if( !has_flag( flag_FIT ) && has_flag( flag_VARSIZE ) ) {
        tagtext += _( " (poor fit)" );
    }

    if( is_filthy() ) {
        tagtext += _( " (filthy)" );
    }
    if( is_gun() && ( has_flag( flag_COLLAPSED_STOCK ) || has_flag( flag_FOLDED_STOCK ) ) &&
        with_collapsed ) {
        tagtext += _( " (folded)" );
    }
    if( is_broken() ) {
        tagtext += _( " (broken)" );
    }
    if( is_bionic() && !has_flag( flag_NO_PACKED ) ) {
        if( !has_flag( flag_NO_STERILE ) ) {
            tagtext += _( " (sterile)" );
        } else {
            tagtext += _( " (packed)" );
        }
    }

    if( is_tool() && has_flag( flag_USE_UPS ) ) {
        tagtext += _( " (UPS)" );
    }

    if( has_var( "NANOFAB_ITEM_ID" ) ) {
        if( has_flag( flag_NANOFAB_TEMPLATE_SINGLE_USE ) ) {
            //~ Single-use descriptor for nanofab templates. %s = name of resulting item. The leading space is intentional.
            tagtext += string_format( _( " (SINGLE USE %s)" ),
                                      nname( itype_id( get_var( "NANOFAB_ITEM_ID" ) ) ) );
        } else {
            tagtext += string_format( " (%s)", nname( itype_id( get_var( "NANOFAB_ITEM_ID" ) ) ) );
        }
    }

    if( has_flag( flag_RADIO_MOD ) ) {
        tagtext += _( " (radio:" );
        if( has_flag( flag_RADIOSIGNAL_1 ) ) {
            tagtext += pgettext( "The radio mod is associated with the [R]ed button.", "R)" );
        } else if( has_flag( flag_RADIOSIGNAL_2 ) ) {
            tagtext += pgettext( "The radio mod is associated with the [B]lue button.", "B)" );
        } else if( has_flag( flag_RADIOSIGNAL_3 ) ) {
            tagtext += pgettext( "The radio mod is associated with the [G]reen button.", "G)" );
        } else {
            debugmsg( "Why is the radio neither red, blue, nor green?" );
            tagtext += "?)";
        }
    }

    if( has_flag( flag_WET ) || wetness ) {
        tagtext += _( " (wet)" );
    }
    if( already_used_by_player( player_character ) ) {
        tagtext += _( " (used)" );
    }
    if( active && ( has_flag( flag_WATER_EXTINGUISH ) || has_flag( flag_LITCIG ) ) ) {
        tagtext += _( " (lit)" );
    } else if( has_flag( flag_IS_UPS ) && get_var( "cable" ) == "plugged_in" ) {
        tagtext += _( " (plugged in)" );
    } else if( active && !has_temperature() && !string_ends_with( typeId().str(), "_on" ) ) {
        // Usually the items whose ids end in "_on" have the "active" or "on" string already contained
        // in their name, also food is active while it rots.
        tagtext += _( " (active)" );
    }

    if( all_pockets_sealed() ) {
        tagtext += _( " (sealed)" );
    } else if( any_pockets_sealed() ) {
        tagtext += _( " (part sealed)" );
    }

    if( get_option<std::string>( "ASTERISK_POSITION" ) == "suffix" ) {
        if( is_favorite ) {
            tagtext += _( " *" ); // Display asterisk for favorite items, after item's name
        }
    }

    std::string modtext;
    if( gunmod_find( itype_barrel_small ) ) {
        modtext += _( "sawn-off " );
    }
    if( is_gun() && has_flag( flag_REMOVED_STOCK ) ) {
        modtext += _( "pistol " );
    }
    if( is_relic() && relic_data->max_charges() > 0 && relic_data->charges_per_use() > 0 ) {
        tagtext += string_format( " (%d/%d)", relic_data->charges(), relic_data->max_charges() );
    }
    if( has_flag( flag_DIAMOND ) ) {
        modtext += std::string( pgettext( "Adjective, as in diamond katana", "diamond" ) ) + " ";
    }

    //~ This is a string to construct the item name as it is displayed. This format string has been added for maximum flexibility. The strings are: %1$s: Damage text (e.g. "bruised"). %2$s: burn adjectives (e.g. "burnt"). %3$s: tool modifier text (e.g. "atomic"). %4$s: vehicle part text (e.g. "3.8-Liter"). %5$s: main item text (e.g. "apple"). %6$s: whitelist marker (e.g. "⁺"). %7$s: tags (e.g. "(wet) (poor fit)").%8$s: inner contents suffix (e.g. " > rock" or " > 5 items").
    std::string ret = string_format( _( "%1$s%2$s%3$s%4$s%5$s%6$s%7$s%8$s" ), damtext, burntext,
                                     modtext, vehtext, maintext, wltext, tagtext, contents_suffix_text );

    if( truncate != 0 ) {
        ret = utf8_truncate( ret, truncate + truncate_override );
    }

    if( item_vars.find( "item_note" ) != item_vars.end() ) {
        //~ %s is an item name. This style is used to denote items with notes.
        return string_format( _( "*%s*" ), ret );
    } else {
        return ret;
    }
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
        amount = ammo_remaining();
        const itype *adata = mag->ammo_data();
        if( adata ) {
            max_amount = mag->ammo_capacity( adata->ammo->type );
        } else {
            max_amount = mag->ammo_capacity( item_controller->find_template(
                                                 mag->ammo_default() )->ammo->type );
        }

    } else if( !ammo_types().empty() ) {
        // anything that can be reloaded including tools, magazines, guns and auxiliary gunmods
        // but excluding bows etc., which have ammo, but can't be reloaded
        amount = ammo_remaining();
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

    if( amount || show_amt ) {
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

    if( link ) {
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
        // TODO: fix point types
        tripoint map_pos_omt =
            get_var( "reveal_map_center_omt", player_character.global_omt_location().raw() );
        tripoint_abs_sm map_pos =
            project_to<coords::sm>( tripoint_abs_omt( map_pos_omt ) );
        const city *c = overmap_buffer.closest_city( map_pos ).city;
        if( c != nullptr ) {
            name = string_format( "%s %s", c->name, name );
        }
    }

    return string_format( "%s%s%s%s", name, sidetxt, amt, cable );
}

bool item::is_collapsed() const
{
    return !contents.empty() && !contents.get_pockets( []( item_pocket const & pocket ) {
        return pocket.settings.is_collapsed() && pocket.is_standard_type();
    } ).empty();
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
        price *= ammo_remaining() / static_cast< double >( std::max( type->charges_default(), 1 ) );

    }

    if( is_filthy() ) {
        // Filthy items receieve a fixed price malus. This means common clothing ends up
        // with no value (it's *everywhere*), but valuable items retain most of their value.
        // https://github.com/CleverRaven/Cataclysm-DDA/issues/49469
        price = std::max( price - PRICE_FILTHY_MALUS, 0 );
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
    std::string local_str_mass = integral ? get_var( "integral_weight" ) : get_var( "weight" );
    if( local_str_mass.empty() ) {
        ret = integral ? type->integral_weight : type->weight;
    } else {
        ret = units::from_milligram( std::stoll( local_str_mass ) );
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

    ret *= ret_mul;

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
        links.charges = ammo_remaining();
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
    return ( type->longest_side / 100 ) * barrel_percentage;
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
                              ( type->volume - type->gun->barrel_volume ) / units::legacy_volume_factor );
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
    if( corpse_volume > 0_ml ) {
        return corpse_volume;
    }
    debugmsg( "invalid monster volume for corpse" );
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
        ret = local_volume * units::legacy_volume_factor;
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
    if( type->melee.count( dt ) > 0 ) {
        res = type->melee.at( dt );
    }
    res = damage_adjusted_melee_weapon_damage( res );

    // apply type specific flags
    // FIXME: Hardcoded damage types
    if( dt == damage_bash && has_flag( flag_REDUCED_BASHING ) ) {
        res *= 0.5;
    }
    if( dt->edged && has_flag( flag_DIAMOND ) ) {
        res *= 1.3;
    }

    // consider any melee gunmods
    if( is_gun() ) {
        std::vector<int> opts = { res };
        for( const std::pair<const gun_mode_id, gun_mode> &e : gun_all_modes() ) {
            if( e.second.target != this && e.second.melee() ) {
                opts.push_back( e.second.target->damage_melee( dt ) );
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

    if( has_flag( flag_REACH_ATTACK ) ) {
        res = has_flag( flag_REACH3 ) ? 3 : 2;
    }

    // for guns consider any attached gunmods
    if( is_gun() && !is_gunmod() ) {
        for( const std::pair<const gun_mode_id, gun_mode> &m : gun_all_modes() ) {
            if( guy.is_npc() && m.second.flags.count( "NPC_AVOID" ) ) {
                continue;
            }
            if( m.second.melee() ) {
                res = std::max( res, m.second.qty );
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
        requires_tags_processing = true;
    } else {
        debugmsg( "Attempted to set invalid flag_id %s", flag.str() );
    }
    return *this;
}

item &item::unset_flag( const flag_id &flag )
{
    item_tags.erase( flag );
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
        for( item *e : contents.all_items_top( item_pocket::pocket_type::MOD ) ) {
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
        for( const item *e : contents.all_items_top( item_pocket::pocket_type::MOD ) ) {
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
    const float safe_temp = 145; // F, ~63 C, safe temperature above which food stops rotting

    if( temp <= temperatures::freezing ||
        temp > units::from_fahrenheit( safe_temp ) ) {
        return 0.f;
    } else if( temp < dropoff ) {
        // ditch our fancy equation and do a linear approach to 0 rot from 38 F (3 C) -> 32 F (0 C)
        return 600.f * std::exp2( -27.f / 16.f ) * ( units::to_fahrenheit( temp ) - units::to_fahrenheit(
                    temperatures::freezing ) );
    } else if( temp < units::from_fahrenheit( max_rot_temp ) ) {
        // Exponential progress from 38 F (3 C) to 105 F (41 C)
        return 3600.f * std::exp2( ( units::to_fahrenheit( temp ) - 65.f ) / 16.f );
    } else {
        // Constant rot from 105 F (41 C) to 145 F (63 C)
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

    if( has_own_flag( flag_COLD ) ) {
        temp = std::min( temperatures::fridge, temp );
    }

    rot += factor * time_delta / 1_hours * calc_hourly_rotpoints_at_temp( temp ) * 1_turns;
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

float item::get_weight_capacity_modifier() const
{
    const islot_armor *t = find_armor_data();
    if( t == nullptr ) {
        return 1;
    }
    return t->weight_capacity_modifier;
}

units::mass item::get_weight_capacity_bonus() const
{
    const islot_armor *t = find_armor_data();
    if( t == nullptr ) {
        return 0_gram;
    }
    return t->weight_capacity_bonus;
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

bool item::is_power_armor() const
{
    const islot_armor *t = find_armor_data();
    if( t == nullptr ) {
        return is_pet_armor() ? type->pet_armor->power_armor : false;
    }
    return t->power_armor;
}

int item::get_avg_encumber( const Character &p, encumber_flags flags ) const
{
    const islot_armor *t = find_armor_data();
    if( !t ) {
        return 0;
    }

    int avg_encumber = 0;
    int avg_ctr = 0;

    for( const armor_portion_data &entry : t->data ) {
        if( entry.covers.has_value() ) {
            for( const bodypart_str_id &limb : entry.covers.value() ) {
                int encumber = get_encumber( p, bodypart_id( limb ), flags );
                if( encumber ) {
                    avg_encumber += encumber;
                    ++avg_ctr;
                }
            }
        }
    }
    if( avg_encumber == 0 ) {
        return 0;
    } else {
        avg_encumber /= avg_ctr;
        return avg_encumber;
    }
}

int item::get_encumber( const Character &p, const bodypart_id &bodypart,
                        encumber_flags flags ) const
{
    const islot_armor *t = find_armor_data();
    int encumber = 0;
    if( !t ) {
        return 0;
    }

    float relative_encumbrance;

    if( flags & encumber_flags::assume_full ) {
        relative_encumbrance = 1.0f;
    } else if( flags & encumber_flags::assume_empty ) {
        relative_encumbrance = 0.0f;
    } else {
        // Additional encumbrance from non-rigid pockets

        // p.get_check_encumbrance() may be set when it's not possible
        // to reset `cached_relative_encumbrance` for individual items
        // (e.g. when dropping via AIM, see #42983)
        if( !cached_relative_encumbrance || p.get_check_encumbrance() ) {
            cached_relative_encumbrance = contents.relative_encumbrance();
        }
        relative_encumbrance = *cached_relative_encumbrance;
    }

    if( const armor_portion_data *portion_data = portion_for_bodypart( bodypart ) ) {
        encumber = portion_data->encumber;
        if( is_gun() ) {
            encumber += volume() * portion_data->volume_encumber_modifier /
                        armor_portion_data::volume_per_encumbrance;
        }

        // encumbrance from added or modified pockets
        int additional_encumbrance = get_contents().get_additional_pocket_encumbrance(
                                         portion_data->volume_encumber_modifier );
        encumber += std::ceil( relative_encumbrance * ( portion_data->max_encumber +
                               additional_encumbrance - portion_data->encumber ) );

        // add the encumbrance values of any ablative plates and additional encumbrance pockets
        if( has_additional_encumbrance() ) {
            for( const item_pocket *pocket : contents.get_all_contained_pockets() ) {
                if( pocket->get_pocket_data()->extra_encumbrance > 0 && !pocket->empty() ) {
                    encumber += pocket->get_pocket_data()->extra_encumbrance;
                }
            }
        }
    }

    // even if we don't have data we might have ablative armor draped over it
    if( is_ablative() ) {
        for( const item_pocket *pocket : contents.get_all_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();
                encumber += ablative_armor.get_encumber( p, bodypart, flags );
            }
        }
    }

    // Fit checked before changes, fitting shouldn't reduce penalties from patching.
    if( has_flag( flag_FIT ) && has_flag( flag_VARSIZE ) ) {
        encumber = std::max( encumber / 2, encumber - 10 );
    }

    // TODO: Should probably have sizing affect coverage
    const sizing sizing_level = get_sizing( p );
    switch( sizing_level ) {
        case sizing::small_sized_human_char:
        case sizing::small_sized_big_char:
            // non small characters have a HARD time wearing undersized clothing
            encumber *= 3;
            break;
        case sizing::human_sized_small_char:
        case sizing::big_sized_small_char:
            // clothes bag up around smol characters and encumber them more
            encumber *= 2;
            break;
        default:
            break;
    }

    encumber += static_cast<int>( std::ceil( get_clothing_mod_val( clothing_mod_type_encumbrance ) ) );

    return encumber;
}

std::vector<layer_level> item::get_layer() const
{
    const islot_armor *armor = find_armor_data();
    if( armor == nullptr ) {
        return std::vector<layer_level>();
    }
    return armor->all_layers;
}

static void get_layer_helper( const item *i, bodypart_id bp, std::set<layer_level> &layers )
{
    const islot_armor *t = i->find_armor_data();
    if( t == nullptr ) {
        return;
    }

    for( const armor_portion_data &data : t->data ) {
        if( !data.covers.has_value() ) {
            continue;
        }
        for( const bodypart_str_id &bpid : data.covers.value() ) {
            if( bp == bpid ) {
                layers.insert( data.layers.begin(), data.layers.end() );
            }
        }
    }
}

std::vector<layer_level> item::get_layer( bodypart_id bp ) const
{

    std::set<layer_level> layers;

    get_layer_helper( this, bp, layers );

    //get layers for ablative armor too
    if( is_ablative() ) {
        for( const item_pocket *pocket : get_all_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();
                get_layer_helper( &ablative_armor, bp, layers );
            }
        }
    }

    // if the item has additional pockets and is on the torso it should also be strapped
    if( bp == body_part_torso && contents.has_additional_pockets() ) {
        layers.insert( layer_level::BELTED );
    }

    return std::vector<layer_level>( layers.begin(), layers.end() );
}

std::vector<layer_level> item::get_layer( sub_bodypart_id sbp ) const
{
    const islot_armor *t = find_armor_data();
    if( t == nullptr ) {
        return std::vector<layer_level>();
    }

    for( const armor_portion_data &data : t->sub_data ) {
        for( const sub_bodypart_str_id &bpid : data.sub_coverage ) {
            if( sbp == bpid ) {
                // if the item has additional pockets and is on the torso it should also be strapped
                if( ( sbp == sub_body_part_torso_upper || sbp == sub_body_part_torso_lower ) &&
                    contents.has_additional_pockets() ) {
                    std::set<layer_level> with_belted = data.layers;
                    with_belted.insert( layer_level::BELTED );
                    return std::vector<layer_level>( with_belted.begin(), with_belted.end() );
                } else {
                    return std::vector<layer_level>( data.layers.begin(), data.layers.end() );
                }
            }
        }
    }
    // body part not covered by this armour
    return std::vector<layer_level>();
}

bool item::has_layer( const std::vector<layer_level> &ll ) const
{
    const islot_armor *t = find_armor_data();
    if( t == nullptr ) {
        return false;
    }
    for( const layer_level &test_level : ll ) {
        if( std::find( t->all_layers.begin(), t->all_layers.end(), test_level ) != t->all_layers.end() ) {
            return true;
        }
    }
    return false;
}

bool item::has_layer( const std::vector<layer_level> &ll, const bodypart_id &bp ) const
{
    const std::vector<layer_level> layers = get_layer( bp );
    for( const layer_level &test_level : ll ) {
        if( std::find( layers.begin(), layers.end(), test_level ) != layers.end() ) {
            return true;
        }
    }
    return false;
}

bool item::has_layer( const std::vector<layer_level> &ll, const sub_bodypart_id &sbp ) const
{
    const std::vector<layer_level> layers = get_layer( sbp );
    for( const layer_level &test_level : ll ) {
        if( std::find( layers.begin(), layers.end(), test_level ) != layers.end() ) {
            return true;
        }
    }
    return false;
}

layer_level item::get_highest_layer( const sub_bodypart_id &sbp ) const
{
    layer_level highest_layer = layer_level::PERSONAL;

    // find highest layer from this item
    for( layer_level our_layer : get_layer( sbp ) ) {
        if( our_layer > highest_layer ) {
            highest_layer = our_layer;
        }
    }

    return highest_layer;
}

item::cover_type item::get_cover_type( const damage_type_id &type )
{
    item::cover_type ctype = item::cover_type::COVER_DEFAULT;
    if( type->physical ) {
        if( type->melee_only ) {
            ctype = item::cover_type::COVER_MELEE;
        } else {
            ctype = item::cover_type::COVER_RANGED;
        }
    }
    return ctype;
}

int item::get_avg_coverage( const cover_type &type ) const
{
    const islot_armor *t = find_armor_data();
    if( !t ) {
        return 0;
    }
    int avg_coverage = 0;
    int avg_ctr = 0;
    for( const armor_portion_data &entry : t->data ) {
        if( entry.covers.has_value() ) {
            for( const bodypart_str_id &limb : entry.covers.value() ) {
                int coverage = get_coverage( limb, type );
                if( coverage ) {
                    avg_coverage += coverage;
                    ++avg_ctr;
                }
            }
        }
    }
    if( avg_coverage == 0 ) {
        return 0;
    } else {
        avg_coverage /= avg_ctr;
        return avg_coverage;
    }
}

int item::get_coverage( const bodypart_id &bodypart, const cover_type &type ) const
{
    if( const armor_portion_data *portion_data = portion_for_bodypart( bodypart ) ) {
        switch( type ) {
            case cover_type::COVER_DEFAULT:
                return portion_data->coverage;
            case cover_type::COVER_MELEE:
                return portion_data->cover_melee;
            case cover_type::COVER_RANGED:
                return portion_data->cover_ranged;
            case cover_type::COVER_VITALS:
                return portion_data->cover_vitals;
        }
    }
    return 0;
}

int item::get_coverage( const sub_bodypart_id &bodypart, const cover_type &type ) const
{
    if( const armor_portion_data *portion_data = portion_for_bodypart( bodypart ) ) {
        switch( type ) {
            case cover_type::COVER_DEFAULT:
                return portion_data->coverage;
            case cover_type::COVER_MELEE:
                return portion_data->cover_melee;
            case cover_type::COVER_RANGED:
                return portion_data->cover_ranged;
            case cover_type::COVER_VITALS:
                return portion_data->cover_vitals;
        }
    }
    return 0;
}

bool item::has_sublocations() const
{
    const islot_armor *t = find_armor_data();
    if( !t ) {
        return false;
    }
    return t->has_sub_coverage;
}

const armor_portion_data *item::portion_for_bodypart( const bodypart_id &bodypart ) const
{
    const islot_armor *t = find_armor_data();
    if( !t ) {
        return nullptr;
    }
    for( const armor_portion_data &entry : t->data ) {
        if( entry.covers.has_value() && entry.covers->test( bodypart.id() ) ) {
            return &entry;
        }
    }
    return nullptr;
}

const armor_portion_data *item::portion_for_bodypart( const sub_bodypart_id &bodypart ) const
{
    const islot_armor *t = find_armor_data();
    if( !t ) {
        return nullptr;
    }
    for( const armor_portion_data &entry : t->sub_data ) {
        for( const sub_bodypart_str_id &tmp : entry.sub_coverage ) {
            const sub_bodypart_id &subpart = tmp;
            if( subpart == bodypart ) {
                return &entry;
            }
        }
    }
    return nullptr;
}

float item::get_thickness() const
{
    const islot_armor *t = find_armor_data();
    // if an item isn't armor we assume it is 1mm thick
    // TODO: Estimate normal items thickness or protection based on weight and density
    if( t == nullptr ) {
        return is_pet_armor() ? type->pet_armor->thickness : 1.0f;
    }
    return t->avg_thickness();
}

float item::get_thickness( const bodypart_id &bp ) const
{
    const islot_armor *t = find_armor_data();
    // don't return a fixed value for this one since an item is never going to be covering a body part
    if( t == nullptr ) {
        return is_pet_armor() ? type->pet_armor->thickness : 0.0f;
    }

    for( const armor_portion_data &data : t->data ) {
        if( !data.covers.has_value() ) {
            continue;
        }
        for( const bodypart_str_id &bpid : data.covers.value() ) {
            if( bp == bpid ) {
                return data.avg_thickness;
            }
        }
    }
    // body part not covered by this armour
    return 0.0f;
}

float item::get_thickness( const sub_bodypart_id &bp ) const
{
    return get_thickness( bp->parent );
}

int item::get_warmth() const
{
    const islot_armor *t = find_armor_data();
    if( t == nullptr ) {
        return 0;
    }
    int result = t->warmth;

    result += get_clothing_mod_val( clothing_mod_type_warmth );

    return result;
}

int item::get_warmth( const bodypart_id &bp ) const
{
    double warmth_val = 0.0;
    float limb_coverage = 0.0f;
    if( !covers( bp ) ) {
        return 0;
    }
    warmth_val = get_warmth();
    // calculate how much of the limb the armor ideally covers
    // idea being that an item that covers the shoulders and torso shouldn't
    // heat the whole arm like it covers it
    // TODO: fully configure this per armor entry
    if( !has_sublocations() ) {
        // if it doesn't have sublocations it has 100% covered
        limb_coverage = 100;
    } else {
        for( const sub_bodypart_str_id &sbp : bp->sub_parts ) {
            if( !covers( sbp ) ) {
                continue;
            }

            // TODO: handle non 100% sub body part coverages
            limb_coverage += sbp->max_coverage;
        }
    }
    return std::round( warmth_val * limb_coverage / 100.0f );
}

units::volume item::get_pet_armor_max_vol() const
{
    return is_pet_armor() ? type->pet_armor->max_vol : 0_ml;
}

units::volume item::get_pet_armor_min_vol() const
{
    return is_pet_armor() ? type->pet_armor->min_vol : 0_ml;
}

std::string item::get_pet_armor_bodytype() const
{
    return is_pet_armor() ? type->pet_armor->bodytype : "";
}

time_duration item::brewing_time() const
{
    return is_brewable() ? type->brewable->time * calendar::season_from_default_ratio() : 0_turns;
}

const std::vector<itype_id> &item::brewing_results() const
{
    static const std::vector<itype_id> nulresult{};
    return is_brewable() ? type->brewable->results : nulresult;
}

bool item::can_revive() const
{
    return is_corpse() && ( corpse->has_flag( mon_flag_REVIVES ) || has_var( "zombie_form" ) ) &&
           damage() < max_damage() &&
           !( has_flag( flag_FIELD_DRESS ) || has_flag( flag_FIELD_DRESS_FAILED ) ||
              has_flag( flag_QUARTERED ) ||
              has_flag( flag_SKINNED ) || has_flag( flag_PULPED ) );
}

bool item::ready_to_revive( map &here, const tripoint &pos ) const
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
            const int distance = rl_dist( pos, get_player_character().pos() );
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
    return ammo_types().count( ammo_money );
}

bool item::is_software() const
{
    if( const std::optional<itype_id> &cont = type->default_container ) {
        return item( *cont ).is_software_storage();
    }
    return false;
}

bool item::is_software_storage() const
{
    return contents.has_pocket_type( item_pocket::pocket_type::SOFTWARE );
}

bool item::is_ebook_storage() const
{
    return contents.has_pocket_type( item_pocket::pocket_type::EBOOK );
}

bool item::is_maybe_melee_weapon() const
{
    item_category_id my_cat_id = get_category_shallow().id;
    return my_cat_id == item_category_weapons || my_cat_id == item_category_spare_parts ||
           my_cat_id == item_category_tools;
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

int item::count() const
{
    return count_by_charges() ? charges : 1;
}

bool item::craft_has_charges() const
{
    return count_by_charges() || ammo_types().empty();
}

float item::get_clothing_mod_val_for_damage_type( const damage_type_id &dmg_type ) const
{
    // FIXME: Hardcoded damage types
    if( dmg_type == damage_bash ) {
        return get_clothing_mod_val( clothing_mod_type_bash );
    } else if( dmg_type == damage_cut ) {
        return get_clothing_mod_val( clothing_mod_type_cut );
    } else if( dmg_type == damage_bullet ) {
        return get_clothing_mod_val( clothing_mod_type_bullet );
    } else if( dmg_type == damage_heat ) {
        return get_clothing_mod_val( clothing_mod_type_fire );
    } else if( dmg_type == damage_acid ) {
        return get_clothing_mod_val( clothing_mod_type_acid );
    }
    return 0.0f; // No mods for other damage types
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

    const float damage_scale = damage_adjusted_armor_resist( 1.0f );

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

int item::breathability( const bodypart_id &bp ) const
{
    const bool bp_null = bp == bodypart_id();
    // if the armor doesn't have armor data
    // or its not on a specific limb
    if( bp_null || armor_made_of( bp ).empty() ) {
        return breathability();
    }
    const armor_portion_data *a = portion_for_bodypart( bp );
    if( a == nullptr ) {
        // if it doesn't cover it breathes great
        return 100;
    }

    return a->breathability;
}

int item::breathability() const
{
    // very rudimentary average for simple armor and non limb based situations
    int breathability = 0;

    const int total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;
    const std::map<material_id, int> mats = made_of();
    if( !mats.empty() ) {
        for( const auto &m : mats ) {
            breathability += m.first->breathability() * m.second;
        }
        // Average based portion of materials
        breathability /= total;
    }

    return breathability;
}

int item::chip_resistance( bool worst, const bodypart_id &bp ) const
{
    int res = worst ? INT_MAX : INT_MIN;
    if( bp != bodypart_id() ) {
        const std::vector<const part_material *> &armor_mats = armor_made_of( bp );
        // If we have armour portion materials for this body part, use that instead
        if( !armor_mats.empty() ) {
            for( const part_material *m : armor_mats ) {
                const int val = m->id->chip_resist() * m->cover;
                res = worst ? std::min( res, val ) : std::max( res, val );
            }
            if( res == INT_MAX || res == INT_MIN ) {
                return 2;
            }
            res /= 100;
            if( res <= 0 ) {
                return 0;
            }
            return res;
        }
    }

    const int total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;
    for( const std::pair<material_id, int> mat : made_of() ) {
        const int val = ( mat.first->chip_resist() * mat.second ) / total;
        res = worst ? std::min( res, val ) : std::max( res, val );
    }

    if( res == INT_MAX || res == INT_MIN ) {
        return 2;
    }

    if( res <= 0 ) {
        return 0;
    }

    return res;
}

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

item::armor_status item::damage_armor_durability( damage_unit &du, const bodypart_id &bp )
{
    if( has_flag( flag_UNBREAKABLE ) ) {
        return armor_status::UNDAMAGED;
    }

    // We want armor's own resistance to this type, not the resistance it grants
    const float armors_own_resist = resist( du.type, true, bp );
    if( armors_own_resist > 1000.0f ) {
        // This is some weird type that doesn't damage armors
        return armor_status::UNDAMAGED;
    }

    // Scale chance of article taking damage based on the number of parts it covers.
    // This represents large articles being able to take more punishment
    // before becoming ineffective or being destroyed.
    const int num_parts_covered = get_covered_body_parts().count();
    if( !one_in( num_parts_covered ) ) {
        return armor_status::UNDAMAGED;
    }

    // Don't damage armor as much when bypassed by armor piercing
    // Most armor piercing damage comes from bypassing armor, not forcing through
    const float post_mitigated_dmg = du.amount;
    // more gradual damage chance calc
    const float damaged_chance = 0.11 * ( post_mitigated_dmg / ( armors_own_resist + 2 ) ) + 0.1;
    if( post_mitigated_dmg > armors_own_resist ) {
        // handle overflow, if you take a lot of damage your armor should be damaged
        if( damaged_chance >= 1 ) {
            return armor_status::DAMAGED;
        }
        if( one_in( 1 / ( 1 - damaged_chance ) ) ) {
            return armor_status::UNDAMAGED;
        }
    } else {
        // Sturdy items and power armors never take chip damage.
        // Other armors have 0.5% of getting damaged from hits below their armor value.
        if( has_flag( flag_STURDY ) || is_power_armor() || !one_in( 200 ) ) {
            return armor_status::UNDAMAGED;
        }
    }

    const int damage = has_flag( flag_FRAGILE )
                       ? rng( 2 * itype::damage_scale, 3 * itype::damage_scale )
                       : itype::damage_scale;

    return mod_damage( damage ) ? armor_status::DESTROYED : armor_status::DAMAGED;
}

item::armor_status item::damage_armor_transforms( damage_unit &du ) const
{
    // We want armor's own resistance to this type, not the resistance it grants
    const float armors_own_resist = resist( du.type, true );

    // to avoid dumb chip damage plates shouldn't ever transform if they take less than
    // 20% of their protection in damage
    if( du.amount / armors_own_resist < 0.2f ) {
        return armor_status::UNDAMAGED;
    }

    // plates are rated to survive 3 shots at the caliber they protect
    // linearly scale off the scale value to find the chance it breaks
    float break_chance = 33.3f * ( du.amount / armors_own_resist );

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
    std::string outputstring;

    if( has_flag( flag_CORPSE ) ) {
        if( damage() > 0 ) {
            switch( damage_level() ) {
                case 1:
                    outputstring = pgettext( "damage adjective", "bruised " );
                    break;
                case 2:
                    outputstring = pgettext( "damage adjective", "damaged " );
                    break;
                case 3:
                    outputstring = pgettext( "damage adjective", "mangled " );
                    break;
                default:
                    outputstring = pgettext( "damage adjective", "pulped " );
                    break;
            }
        }
    } else if( get_option<bool>( "ITEM_HEALTH_BAR" ) ) {
        outputstring = damage_indicator() + degradation_symbol() + "\u00A0";
    } else {
        outputstring = string_format( "%s ", get_base_material().dmg_adj( damage_level() ) );
        if( include_intact && outputstring == " " ) {
            outputstring = _( "fully intact " );
        }
    }

    return outputstring;
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

void item::mitigate_damage( damage_unit &du, const bodypart_id &bp, int roll ) const
{
    const resistances res = resistances( *this, false, roll, bp );
    const float mitigation = res.get_effective_resist( du );
    // get_effective_resist subtracts the flat penetration value before multiplying the remaining armor.
    // therefore, res_pen is reduced by the full value of the item's armor value even though mitigation might be smaller (such as an attack with a 0.5 armor multiplier)
    du.res_pen -= res.type_resist( du.type );
    du.res_pen = std::max( 0.0f, du.res_pen );
    du.amount -= mitigation;
    du.amount = std::max( 0.0f, du.amount );
}

void item::mitigate_damage( damage_unit &du, const sub_bodypart_id &bp, int roll ) const
{
    const resistances res = resistances( *this, false, roll, bp );
    const float mitigation = res.get_effective_resist( du );
    // get_effective_resist subtracts the flat penetration value before multiplying the remaining armor.
    // therefore, res_pen is reduced by the full value of the item's armor value even though mitigation might be smaller (such as an attack with a 0.5 armor multiplier)
    du.res_pen -= res.type_resist( du.type );
    du.res_pen = std::max( 0.0f, du.res_pen );
    du.amount -= mitigation;
    du.amount = std::max( 0.0f, du.amount );
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

std::vector<const part_material *> item::armor_made_of( const bodypart_id &bp ) const
{
    std::vector<const part_material *> matlist;
    const islot_armor *a = find_armor_data();
    if( a == nullptr || a->data.empty() || !covers( bp ) ) {
        return matlist;
    }
    for( const armor_portion_data &d : a->data ) {
        if( !d.covers.has_value() ) {
            continue;
        }
        for( const bodypart_str_id &bpid : d.covers.value() ) {
            if( bp != bpid ) {
                continue;
            }
            for( const part_material &m : d.materials ) {
                matlist.emplace_back( &m );
            }
            return matlist;
        }
    }
    return matlist;
}

std::vector<const part_material *> item::armor_made_of( const sub_bodypart_id &bp ) const
{
    std::vector<const part_material *> matlist;
    const islot_armor *a = find_armor_data();
    if( a == nullptr || a->data.empty() || !covers( bp ) ) {
        return matlist;
    }
    for( const armor_portion_data &d : a->sub_data ) {
        if( d.sub_coverage.empty() ) {
            continue;
        }
        for( const sub_bodypart_str_id &bpid : d.sub_coverage ) {
            if( bp != bpid ) {
                continue;
            }
            for( const part_material &m : d.materials ) {
                matlist.emplace_back( &m );
            }
            return matlist;
        }
    }
    return matlist;
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

    for( const itype_variant_data &option : type->variants ) {
        if( option.id == variant ) {
            _itype_variant = &option;
            return;
        }
    }

    debugmsg( "item '%s' has no variant '%s'!", typeId().str(), variant );
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
    return !!type->magazine || contents.has_pocket_type( item_pocket::pocket_type::MAGAZINE );
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

bool item::is_holster() const
{
    return type->can_use( "holster" );
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
    return is_comestible() || is_corpse();
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
    }, item_pocket::pocket_type::CONTAINER );
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

const islot_armor *item::find_armor_data() const
{
    if( type->armor ) {
        return &*type->armor;
    }
    // Currently the only way to make a non-armor item into armor is to install a gun mod.
    // The gunmods are stored in the items contents, as are the contents of a container, and the
    // tools in a tool belt (a container actually), or the ammo in a quiver (container again).
    for( const item *mod : gunmods() ) {
        if( mod->type->armor ) {
            return &*mod->type->armor;
        }
    }
    return nullptr;
}

bool item::is_pet_armor( bool on_pet ) const
{
    bool is_worn = on_pet && !get_var( "pet_armor", "" ).empty();
    return has_flag( flag_IS_PET_ARMOR ) && ( is_worn || !on_pet );
}

bool item::is_armor() const
{
    return find_armor_data() != nullptr || has_flag( flag_IS_ARMOR );
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
    return get_category_shallow().get_id() == item_category_maps;
}

bool item::seal()
{
    if( is_container_full() ) {
        return contents.seal_all_pockets();
    } else {
        return false;
    }
}

bool item::all_pockets_sealed() const
{
    return contents.all_pockets_sealed();
}

bool item::any_pockets_sealed() const
{
    return contents.any_pockets_sealed();
}

bool item::is_container() const
{
    return contents.has_pocket_type( item_pocket::pocket_type::CONTAINER );
}

bool item::is_container_with_restriction() const
{
    if( !is_container() ) {
        return false;
    }
    return contents.is_restricted_container();
}

bool item::is_single_container_with_restriction() const
{
    return contents.is_single_restricted_container();
}

bool item::has_pocket_type( item_pocket::pocket_type pk_type ) const
{
    return contents.has_pocket_type( pk_type );
}

bool item::has_any_with( const std::function<bool( const item & )> &filter,
                         item_pocket::pocket_type pk_type ) const
{
    return contents.has_any_with( filter, pk_type );
}

bool item::all_pockets_rigid() const
{
    return contents.all_pockets_rigid();
}

std::vector<const item_pocket *> item::get_all_contained_pockets() const
{
    return contents.get_all_contained_pockets();
}

std::vector<item_pocket *> item::get_all_contained_pockets()
{
    return contents.get_all_contained_pockets();
}

std::vector<const item_pocket *> item::get_all_standard_pockets() const
{
    return contents.get_all_standard_pockets();
}

std::vector<item_pocket *> item::get_all_standard_pockets()
{
    return contents.get_all_standard_pockets();
}

std::vector<item_pocket *> item::get_all_ablative_pockets()
{
    return contents.get_all_ablative_pockets();
}

std::vector<const item_pocket *> item::get_all_ablative_pockets() const
{
    return contents.get_all_ablative_pockets();
}

item_pocket *item::contained_where( const item &contained )
{
    return contents.contained_where( contained );
}

const item_pocket *item::contained_where( const item &contained ) const
{
    return const_cast<item *>( this )->contained_where( contained );
}

bool item::is_watertight_container() const
{
    return contents.can_contain_liquid( true );
}

bool item::is_bucket_nonempty() const
{
    return !contents.empty() && will_spill();
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

bool item::is_broken() const
{
    return has_flag( flag_ITEM_BROKEN );
}

bool item::is_broken_on_active() const
{
    return has_flag( flag_ITEM_BROKEN ) || ( wetness && has_flag( flag_WATER_BREAK_ACTIVE ) );
}

int item::wind_resist() const
{
    std::vector<const material_type *> materials = made_of_types();
    if( materials.empty() ) {
        debugmsg( "Called item::wind_resist on an item (%s) made of nothing!", tname() );
        return 99;
    }

    int best = -1;
    for( const material_type *mat : materials ) {
        std::optional<int> resistance = mat->wind_resist();
        if( resistance && *resistance > best ) {
            best = *resistance;
        }
    }

    // Default to 99% effective
    if( best == -1 ) {
        return 99;
    }

    return best;
}

std::set<fault_id> item::faults_potential() const
{
    std::set<fault_id> res;
    res.insert( type->faults.begin(), type->faults.end() );
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

bool item::is_container_empty() const
{
    return contents.empty();
}

bool item::is_container_full( bool allow_bucket ) const
{
    return contents.full( allow_bucket );
}

bool item::is_magazine_full() const
{
    return contents.is_magazine_full();
}

bool item::can_unload_liquid() const
{
    if( has_flag( flag_NO_UNLOAD ) ) {
        return false;
    }

    return contents.can_unload_liquid();
}

bool item::contains_no_solids() const
{
    return contents.contains_no_solids();
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

bool item::is_funnel_container( units::volume &bigger_than ) const
{
    if( get_total_capacity() <= bigger_than ) {
        return false; // skip contents check, performance
    }
    return contents.is_funnel_container( bigger_than );
}

bool item::is_emissive() const
{
    return light.luminance > 0 || type->light_emission > 0;
}

bool item::is_deployable() const
{
    return type->can_use( "deploy_furn" );
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
    return ( !is_relic() && !rhs.is_relic() ) || *relic_data == *rhs.relic_data;
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

double item::calculate_by_enchantment( const Character &owner, double modify,
                                       enchant_vals::mod value, bool round_value ) const
{
    double add_value = 0.0;
    double mult_value = 1.0;
    for( const enchant_cache &ench : get_proc_enchantments() ) {
        if( ench.is_active( owner, *this ) ) {
            add_value += ench.get_value_add( value );
            mult_value += ench.get_value_multiply( value );
        }
    }
    if( type->relic_data ) {
        for( const enchantment &ench : type->relic_data->get_defined_enchantments() ) {
            if( ench.is_active( owner, *this ) ) {
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

units::length item::max_containable_length( const bool unrestricted_pockets_only ) const
{
    return contents.max_containable_length( unrestricted_pockets_only );
}

units::length item::min_containable_length() const
{
    return contents.min_containable_length();
}

units::volume item::max_containable_volume() const
{
    return contents.max_containable_volume();
}

ret_val<void> item::is_compatible( const item &it ) const
{
    if( this == &it ) {
        // does the set of all sets contain itself?
        return ret_val<void>::make_failure();
    }
    // disallow putting portable holes into bags of holding
    if( contents.bigger_on_the_inside( volume() ) &&
        it.contents.bigger_on_the_inside( it.volume() ) ) {
        return ret_val<void>::make_failure();
    }

    return contents.is_compatible( it );
}

ret_val<void> item::can_contain_directly( const item &it ) const
{
    return can_contain( it, false, false, true, item_location(), 10000000_ml, false );
}

ret_val<void> item::can_contain( const item &it, const bool nested, const bool ignore_rigidity,
                                 const bool ignore_pkt_settings, const item_location &parent_it,
                                 units::volume remaining_parent_volume, const bool allow_nested ) const
{
    int copies = 1;
    return can_contain( it, copies, nested, ignore_rigidity,
                        ignore_pkt_settings, parent_it,
                        remaining_parent_volume, allow_nested );
}

ret_val<void> item::can_contain( const item &it, int &copies_remaining, const bool nested,
                                 const bool ignore_rigidity, const bool ignore_pkt_settings,
                                 const item_location &parent_it, units::volume remaining_parent_volume,
                                 const bool allow_nested ) const
{
    if( copies_remaining <= 0 ) {
        return ret_val<void>::make_success();
    }
    if( this == &it || ( parent_it.where() != item_location::type::invalid &&
                         this == parent_it.get_item() ) ) {
        // does the set of all sets contain itself?
        // or does this already contain it?
        return ret_val<void>::make_failure();
    }
    if( nested && !this->is_container() ) {
        return ret_val<void>::make_failure();
    }
    // disallow putting portable holes into bags of holding
    if( contents.bigger_on_the_inside( volume() ) &&
        it.contents.bigger_on_the_inside( it.volume() ) ) {
        return ret_val<void>::make_failure();
    }

    if( allow_nested ) {
        for( const item_pocket *pkt : contents.get_all_contained_pockets() ) {
            if( pkt->empty() ) {
                continue;
            }

            // early exit for max length no nested item is gonna fix this
            if( pkt->max_containable_length() < it.length() ) {
                continue;
            }

            // If the current pocket has restrictions or blacklists the item,
            // try the nested pocket regardless of whether it's soft or rigid.
            const bool ignore_nested_rigidity =
                !pkt->settings.accepts_item( it ) ||
                !pkt->get_pocket_data()->get_flag_restrictions().empty();
            for( const item *internal_it : pkt->all_items_top() ) {
                if( parent_it.where() != item_location::type::invalid && internal_it == parent_it.get_item() ) {
                    continue;
                }
                if( !internal_it->is_container() ) {
                    continue;
                }
                auto ret = internal_it->can_contain( it, copies_remaining, true, ignore_nested_rigidity,
                                                     ignore_pkt_settings, parent_it, pkt->remaining_volume() );
                if( copies_remaining <= 0 ) {
                    return ret;
                }
            }
        }
    }

    return nested && !ignore_rigidity ?
           contents.can_contain_rigid( it, copies_remaining, ignore_pkt_settings ) :
           contents.can_contain( it, copies_remaining, ignore_pkt_settings, remaining_parent_volume );
}

bool item::can_contain( const itype &tp ) const
{
    return can_contain( item( &tp ) ).success();
}

bool item::can_contain_partial( const item &it ) const
{
    item i_copy = it;
    if( i_copy.count_by_charges() ) {
        i_copy.charges = 1;
    }
    return can_contain( i_copy ).success();
}

bool item::can_contain_partial_directly( const item &it ) const
{
    item i_copy = it;
    if( i_copy.count_by_charges() ) {
        i_copy.charges = 1;
    }
    return can_contain( i_copy, false, false, true, item_location(), 10000000_ml, false ).success();
}

std::pair<item_location, item_pocket *> item::best_pocket( const item &it, item_location &this_loc,
        const item *avoid, const bool allow_sealed, const bool ignore_settings,
        const bool nested, bool ignore_rigidity )
{
    return contents.best_pocket( it, this_loc, avoid, allow_sealed, ignore_settings,
                                 nested, ignore_rigidity );
}

bool item::spill_contents( Character &c )
{
    if( !is_container() || is_container_empty() ) {
        return true;
    }

    if( c.is_npc() ) {
        return spill_contents( c.pos() );
    }

    contents.handle_liquid_or_spill( c, /*avoid=*/this );
    on_contents_changed();

    return is_container_empty();
}

bool item::spill_contents( const tripoint &pos )
{
    if( !is_container() || is_container_empty() ) {
        return true;
    }
    return contents.spill_contents( pos );
}

bool item::spill_open_pockets( Character &guy, const item *avoid )
{
    return contents.spill_open_pockets( guy, avoid );
}

void item::overflow( const tripoint &pos, const item_location &loc )
{
    contents.overflow( pos, loc );
}

book_proficiency_bonuses item::get_book_proficiency_bonuses() const
{
    book_proficiency_bonuses ret;

    if( is_ebook_storage() ) {
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

int item::get_chapters() const
{
    if( !type->book ) {
        return 0;
    }
    return type->book->chapters;
}

int item::get_remaining_chapters( const Character &u ) const
{
    // NOLINTNEXTLINE(cata-translate-string-literal)
    const std::string var = string_format( "remaining-chapters-%d", u.getID().get_value() );
    return get_var( var, get_chapters() );
}

void item::mark_chapter_as_read( const Character &u )
{
    // NOLINTNEXTLINE(cata-translate-string-literal)
    const std::string var = string_format( "remaining-chapters-%d", u.getID().get_value() );
    if( type->book && type->book->chapters == 0 ) {
        // books without chapters will always have remaining chapters == 0, so we don't need to store them
        erase_var( var );
        return;
    }
    const int remain = std::max( 0, get_remaining_chapters( u ) - 1 );
    set_var( var, remain );
}

std::set<recipe_id> item::get_saved_recipes() const
{
    if( is_broken_on_active() ) {
        return {};
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
    for( auto mat : mats ) {
        matlist.emplace_back( mat.first );
    }
    return *random_entry( matlist, material_id::NULL_ID() );
}

const material_type &item::get_base_material() const
{
    const std::map<material_id, int> &mats = made_of();
    const material_type *m = &material_id::NULL_ID().obj();
    int portion = 0;
    for( const std::pair<material_id, int> mat : mats ) {
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
    if( with_ammo && ammo_data() ) {
        dispersion_sum += ammo_data()->ammo->dispersion;
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

    if( with_ammo && ammo_data() ) {
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

int item::gun_recoil( const Character &p, bool bipod ) const
{
    if( !is_gun() || ( ammo_required() && !ammo_remaining() ) ) {
        return 0;
    }

    ///\ARM_STR improves the handling of heavier weapons
    // we consider only base weight to avoid exploits
    double wt = std::min( type->weight, p.get_arm_str() * 333_gram ) / 333.0_gram;

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
    if( ammo_data() ) {
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
    if( with_ammo && ammo_data() ) {
        ret += ammo_data()->ammo->range;
        range_multiplier *= ammo_data()->ammo->range_multiplier;
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

    // Reduce bow range until player has twice minimm required strength
    if( has_flag( flag_STR_DRAW ) ) {
        ret += std::max( 0.0, ( p->get_str() - get_min_str() ) * 0.5 );
    }

    ret = p->enchantment_cache->modify_value( enchant_vals::mod::RANGE, ret );

    return std::max( 0, ret );
}

int item::shots_remaining( const Character *carrier ) const
{
    int ret = 1000; // Arbitrary large number for things that do not require ammo.
    if( ammo_required() ) {
        ret = std::min( ammo_remaining( carrier, true ) / ammo_required(), ret );
    }
    if( get_gun_energy_drain() > 0_kJ ) {
        ret = std::min( static_cast<int>( energy_remaining( carrier ) / get_gun_energy_drain() ), ret );
    }
    return ret;
}

int item::ammo_remaining( const Character *carrier, const bool include_linked ) const
{
    int ret = 0;

    // Magazine in the item
    const item *mag = magazine_current();
    if( mag ) {
        ret += mag->ammo_remaining();
    }

    // Cable connections
    if( include_linked && link_length() >= 0 && link->efficiency >= MIN_LINK_EFFICIENCY ) {
        if( link->t_veh_safe ) {
            ret += link->t_veh_safe->connected_battery_power_level().first;
        } else {
            const optional_vpart_position vp = get_map().veh_at( link->t_abs_pos );
            if( vp ) {
                ret += vp->vehicle().connected_battery_power_level().first;
            }
        }
    }

    std::set<ammotype> ammo = ammo_types();
    // Non ammo using item that uses charges
    if( ammo.empty() ) {
        ret += charges;
    }

    // Magazines and integral magazines on their own
    if( is_magazine() ) {
        for( const item *e : contents.all_items_top( item_pocket::pocket_type::MAGAZINE ) ) {
            if( e->is_ammo() ) {
                ret += e->charges;
            }
        }
    }

    // Handle non-magazines with ammo_restriction in a CONTAINER type pocket (like quivers)
    if( !( mag || is_magazine() || ammo.empty() ) ) {
        for( const item *e : contents.all_items_top( item_pocket::pocket_type::CONTAINER ) ) {
            if( e->is_ammo() && ammo.find( e->ammo_type() ) != ammo.end() ) {
                ret += e->charges;
            }
        }
    }

    // UPS/bionic power can replace ammo requirement.
    // Only for tools. Guns should always use energy_drain for electricity use.
    if( carrier != nullptr && type->tool ) {
        if( has_flag( flag_USES_BIONIC_POWER ) ) {
            ret += units::to_kilojoule( carrier->get_power_level() );
        }
        if( has_flag( flag_USE_UPS ) ) {
            ret += units::to_kilojoule( carrier->available_ups() );
        }
    }

    return ret;
}

int item::ammo_remaining( const bool include_linked ) const
{
    return ammo_remaining( nullptr, include_linked );
}

units::energy item::energy_remaining( const Character *carrier ) const
{
    units::energy ret = 0_kJ;

    // Future energy based batteries
    if( is_vehicle_battery() ) {
        ret += energy;
    }

    // Magazine in the item
    const item *mag = magazine_current();
    if( mag ) {
        ret += mag->energy_remaining( carrier );
    }

    // Power from bionic
    if( carrier != nullptr && has_flag( flag_USES_BIONIC_POWER ) ) {
        ret += carrier->get_power_level();
    }

    // Extra power from UPS
    if( carrier != nullptr && has_flag( flag_USE_UPS ) ) {
        ret += carrier->available_ups();
    }

    // Battery(ammo) contained within
    if( is_magazine() ) {
        for( const item *e : contents.all_items_top( item_pocket::pocket_type::MAGAZINE ) ) {
            if( e->typeId() == itype_battery ) {
                ret += units::from_kilojoule( e->charges );
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
        return ammo_capacity( item::find_type( ammo_default() )->ammo->type ) - ammo_remaining();
    } else {
        return ammo_capacity( loaded_ammo->ammo->type ) - ammo_remaining();
    }
}

int item::ammo_capacity( const ammotype &ammo, bool include_linked ) const
{
    const item *mag = magazine_current();
    if( include_linked && link ) {
        return link->t_veh_safe ? link->t_veh_safe->connected_battery_power_level().second : 0;
    } else if( mag ) {
        return mag->ammo_capacity( ammo );
    } else if( has_flag( flag_USES_BIONIC_POWER ) ) {
        return units::to_kilojoule( get_player_character().get_max_power_level() );
    }

    if( contents.has_pocket_type( item_pocket::pocket_type::MAGAZINE ) ) {
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

void item::handle_liquid_or_spill( Character &guy, const item *avoid )
{
    contents.handle_liquid_or_spill( guy, avoid );
}

bool item::ammo_sufficient( const Character *carrier, int qty ) const
{
    if( count_by_charges() ) {
        return ammo_remaining( carrier, true ) >= qty;
    }

    if( is_comestible() ) {
        return true;
    }

    return shots_remaining( carrier ) >= qty;
}

bool item::ammo_sufficient( const Character *carrier, const std::string &method, int qty ) const
{
    auto iter = type->ammo_scale.find( method );
    if( iter != type->ammo_scale.end() ) {
        qty *= iter->second;
    }

    return ammo_sufficient( carrier, qty );
}

int item::ammo_consume( int qty, const tripoint &pos, Character *carrier )
{
    if( qty < 0 ) {
        debugmsg( "Cannot consume negative quantity of ammo for %s", tname() );
        return 0;
    }
    const int wanted_qty = qty;

    // Consume power from appliances/vehicles connected with cables
    if( link ) {
        if( link->t_veh_safe && link->efficiency >= MIN_LINK_EFFICIENCY ) {
            qty = link->t_veh_safe->discharge_battery( qty, true );
        } else {
            const optional_vpart_position vp = get_map().veh_at( link->t_abs_pos );
            if( vp ) {
                qty = vp->vehicle().discharge_battery( qty, true );
            }
        }
    }

    // Consume charges loaded in the item or its magazines
    if( is_magazine() || uses_magazine() ) {
        qty -= contents.ammo_consume( qty, pos );
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

    // Modded tools can consume UPS/bionic energy instead of ammo.
    // Guns handle energy in energy_consume()
    if( carrier != nullptr && type->tool ) {
        units::energy wanted_energy = units::from_kilojoule( qty );

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

units::energy item::energy_consume( units::energy qty, const tripoint &pos, Character *carrier,
                                    float fuel_efficiency )
{
    if( qty < 0_kJ ) {
        debugmsg( "Cannot consume negative quantity of energy for %s", tname() );
        return 0_kJ;
    }

    const units::energy wanted_energy = qty;

    // Consume battery(ammo) and other fuel (if allowed)
    if( is_battery() || fuel_efficiency >= 0 ) {
        int consumed_kj = contents.ammo_consume( units::to_kilojoule( qty ), pos, fuel_efficiency );
        qty -= units::from_kilojoule( consumed_kj );
        // fix negative quantity
        if( qty < 0_J ) {
            qty = 0_J;
        }
    }

    // Consume energy from contained magazine
    if( magazine_current() ) {
        qty -= magazine_current()->energy_consume( qty, pos, carrier );
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

    // If consumption is not integer kJ we need to consume one extra battery charge to "round up".
    // Should happen only if battery powered and energy per shot is not integer kJ.
    if( qty > 0_kJ && is_battery() ) {
        int consumed_kj = contents.ammo_consume( 1, pos );
        qty -= units::from_kilojoule( consumed_kj );
    }

    return wanted_energy - qty;
}

int item::activation_consume( int qty, const tripoint &pos, Character *carrier )
{
    return ammo_consume( qty * ammo_required(), pos, carrier );
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

    if( is_gun() && ammo_remaining() != 0 ) {
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
    return contents.ammo_types();
}

ammotype item::ammo_type() const
{
    if( is_ammo() ) {
        return type->ammo->type;
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

std::set<std::string> item::ammo_effects( bool with_ammo ) const
{
    if( !is_gun() ) {
        return std::set<std::string>();
    }

    std::set<std::string> res = type->gun->ammo_effects;
    if( with_ammo && ammo_data() ) {
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
    return contents.has_pocket_type( item_pocket::pocket_type::MAGAZINE );
}

bool item::uses_magazine() const
{
    return contents.has_pocket_type( item_pocket::pocket_type::MAGAZINE_WELL );
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
    } else if( mod.typeId() == itype_waterproof_gunmod && has_flag( flag_WATERPROOF_GUN ) ) {
        return ret_val<void>::make_failure( _( "is already waterproof" ) );

    } else if( mod.typeId() == itype_tuned_mechanism && has_flag( flag_NEVER_JAMS ) ) {
        return ret_val<void>::make_failure( _( "is already eminently reliable" ) );

    } else if( mod.typeId() == itype_brass_catcher && has_flag( flag_RELOAD_EJECT ) ) {
        return ret_val<void>::make_failure( _( "cannot have a brass catcher" ) );

    } else if( ( mod.type->gunmod->location.name() == "magazine" ||
                 mod.type->gunmod->location.name() == "mechanism" ) &&
               ( ammo_remaining() > 0 || magazine_current() ) ) {
        return ret_val<void>::make_failure( _( "must be unloaded before installing this mod" ) );

    } else if( gunmod_find( itype_stock_none ) &&
               mod.type->gunmod->location.name() == "stock accessory" ) {
        return ret_val<void>::make_failure( _( "doesn't have a stock to attach this mod" ) );
    }

    for( const gunmod_location &slot : mod.type->gunmod->blacklist_mod ) {
        if( get_mod_locations().count( slot ) ) {
            return ret_val<void>::make_failure( _( "cannot be installed on a weapon with \"%s\"" ),
                                                slot.name() );
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
                     *ammo->contents.all_items_top( item_pocket::pocket_type::CONTAINER ).front() : *ammo;

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
        available_ammo = ammo_obj.ammo_remaining();
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
        limit = ammo_capacity( ammo->ammo_type() ) - ammo_remaining();
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

        put_in( item_copy, item_pocket::pocket_type::MAGAZINE );

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

        put_in( *ammo, item_pocket::pocket_type::MAGAZINE_WELL );
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

    if( count_by_charges() ) {
        int stack_burnt = rng( type->stack_size / 2, type->stack_size );
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
            charges -= roll_remainder( burn_added * units::legacy_volume_factor * type->stack_size /
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

    const int vol = base_volume() / units::legacy_volume_factor;
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
    for( const std::pair<material_id, int> m : mats ) {
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
        flammability = flammability * vol / units::legacy_volume_factor;
    }

    return flammability > threshold;
}

itype_id item::typeId() const
{
    return type ? type->get_id() : itype_id::NULL_ID();
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
    float lumint = type->light_emission;

    if( lumint == 0 ) {
        return 0;
    }

    if( ammo_required() == 0 || ( has_flag( flag_USE_UPS ) && ammo_capacity( ammo_battery ) == 0 ) ||
        has_flag( flag_USES_BIONIC_POWER ) ) {
        return lumint;
    }
    if( ammo_remaining( true ) == 0 ) {
        return 0;
    }
    if( has_flag( flag_CHARGEDIM ) && is_tool() && !has_flag( flag_USE_UPS ) ) {
        // Falloff starts at 1/5 total charge and scales linearly from there to 0.
        const ammotype &loaded_ammo = ammo_data()->ammo->type;
        if( ammo_capacity( loaded_ammo ) &&
            ammo_remaining( true ) < ( ammo_capacity( loaded_ammo ) / 5 ) ) {
            lumint *= ammo_remaining( true ) * 5.0 / ammo_capacity( loaded_ammo );
        }
    }
    return lumint;
}

units::volume item::get_total_capacity( const bool unrestricted_pockets_only ) const
{
    return contents.total_container_capacity( unrestricted_pockets_only );
}

units::mass item::get_total_weight_capacity( const bool unrestricted_pockets_only ) const
{
    return contents.total_container_weight_capacity( unrestricted_pockets_only );
}

units::volume item::get_remaining_capacity( const bool unrestricted_pockets_only ) const
{
    return contents.remaining_container_capacity( unrestricted_pockets_only );
}
units::mass item::get_remaining_weight_capacity( const bool unrestricted_pockets_only ) const
{
    return contents.remaining_container_capacity_weight( unrestricted_pockets_only );
}

units::volume item::get_total_contained_volume( const bool unrestricted_pockets_only ) const
{
    return contents.total_contained_volume( unrestricted_pockets_only );
}
units::mass item::get_total_contained_weight( const bool unrestricted_pockets_only ) const
{
    return contents.total_contained_weight( unrestricted_pockets_only );
}

int item::get_used_holsters() const
{
    return contents.get_used_holsters();
}
int item::get_total_holsters() const
{
    return contents.get_total_holsters();
}
units::volume item::get_total_holster_volume() const
{
    return contents.get_total_holster_volume();
}
units::volume item::get_used_holster_volume() const
{
    return contents.get_used_holster_volume();
}
units::mass item::get_total_holster_weight() const
{
    return contents.get_total_holster_weight();
}
units::mass item::get_used_holster_weight() const
{
    return contents.get_used_holster_weight();
}

int item::get_remaining_capacity_for_liquid( const item &liquid, bool allow_bucket,
        std::string *err ) const
{
    const auto error = [ &err ]( const std::string & message ) {
        if( err != nullptr ) {
            *err = message;
        }
        return 0;
    };

    int remaining_capacity = 0;

    if( can_contain_partial( liquid ) ) {
        if( !contents.can_contain_liquid( allow_bucket ) ) {
            return error( string_format( _( "That %s must be on the ground or held to hold contents!" ),
                                         tname() ) );
        }
        remaining_capacity = contents.remaining_capacity_for_liquid( liquid );
    } else {
        return error( string_format( _( "That %1$s won't hold %2$s." ), tname(),
                                     liquid.tname() ) );
    }

    if( remaining_capacity <= 0 ) {
        return error( string_format( _( "Your %1$s can't hold any more %2$s." ), tname(),
                                     liquid.tname() ) );
    }

    return remaining_capacity;
}

int item::get_remaining_capacity_for_liquid( const item &liquid, const Character &p,
        std::string *err ) const
{
    const bool allow_bucket = ( p.get_wielded_item() && this == &*p.get_wielded_item() ) ||
                              !p.has_item( *this );
    int res = get_remaining_capacity_for_liquid( liquid, allow_bucket, err );

    if( res > 0 ) {
        res = std::min( contents.remaining_capacity_for_liquid( liquid ), res );

        if( res == 0 && err != nullptr ) {
            *err = string_format( _( "That %s doesn't have room to expand." ), tname() );
        }
    }

    return res;
}

units::volume item::total_contained_volume() const
{
    return contents.total_contained_volume();
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
    for( item *contained : all_items_ptr( item_pocket::pocket_type::CONTAINER ) ) {
        if( contained->use_amount_internal( it, quantity, used, filter ) ) {
            removed_items.push_back( contained );
        }
    }

    for( item *removed : removed_items ) {
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
    if( ( is_toolmod() && is_irremovable() ) || has_flag( flag_PSEUDO ) ) {
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

int item::fill_with( const item &contained, const int amount,
                     const bool unseal_pockets,
                     const bool allow_sealed,
                     const bool ignore_settings,
                     const bool into_bottom )
{
    if( amount <= 0 ) {
        return 0;
    }

    item contained_item( contained );
    const bool count_by_charges = contained.count_by_charges();
    if( count_by_charges ) {
        contained_item.charges = 1;
    }
    item_location loc;
    item_pocket *pocket = nullptr;

    int num_contained = 0;
    while( amount > num_contained ) {
        if( count_by_charges || pocket == nullptr ||
            !pocket->can_contain( contained_item ).success() ) {
            if( count_by_charges ) {
                contained_item.charges = 1;
            }
            pocket = best_pocket( contained_item, loc, /*avoid=*/nullptr, allow_sealed,
                                  ignore_settings ).second;
        }
        if( pocket == nullptr ) {
            break;
        }
        if( count_by_charges ) {
            ammotype ammo = contained.ammo_type();
            if( pocket->ammo_capacity( ammo ) ) {
                contained_item.charges = std::min( amount - num_contained,
                                                   pocket->remaining_ammo_capacity( ammo ) );
            } else {
                contained_item.charges = std::min( { amount - num_contained,
                                                     pocket->charges_per_remaining_volume( contained_item ),
                                                     pocket->charges_per_remaining_weight( contained_item )
                                                   } );
            }

            if( contained_item.charges == 0 ) {
                break;
            }
        }

        if( !pocket->insert_item( contained_item, into_bottom ).success() ) {
            if( count_by_charges ) {
                debugmsg( "charges per remaining pocket volume does not fit in that very volume" );
            } else {
                debugmsg( "best pocket for item cannot actually contain the item" );
            }
            break;
        }
        if( count_by_charges ) {
            num_contained += contained_item.charges;
        } else {
            num_contained++;
        }
        if( unseal_pockets ) {
            pocket->unseal();
        }
    }
    if( num_contained == 0 ) {
        debugmsg( "tried to put an item (%s, amount %d) in a container (%s) that cannot contain it",
                  contained_item.typeId().str(), contained_item.charges, typeId().str() );
    }
    on_contents_changed();
    get_avatar().invalidate_weight_carried_cache();
    return num_contained;
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
                        const tripoint &pos, const std::function<bool( const item & )> &filter,
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

        if( e->is_tool() ) {
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

const item_category &item::get_category_of_contents() const
{
    if( type->category_force == item_category_container && contents_only_one_type() ) {
        return contents.first_item().get_category_of_contents();
    } else {
        return this->get_category_shallow();
    }
}

iteminfo::iteminfo( const std::string &Type, const std::string &Name, const std::string &Fmt,
                    flags Flags, double Value, double UnitVal )
{
    sType = Type;
    sName = replace_colors( Name );
    sFmt = replace_colors( Fmt );
    is_int = !( Flags & is_decimal || Flags & is_three_decimal );
    three_decimal = ( Flags & is_three_decimal );
    dValue = Value;
    dUnitAdjustedVal = UnitVal < std::numeric_limits<float>::epsilon() &&
                       UnitVal > -std::numeric_limits<float>::epsilon() ? Value : UnitVal;
    bShowPlus = static_cast<bool>( Flags & show_plus );
    std::stringstream convert;
    if( bShowPlus ) {
        convert << std::showpos;
    }
    if( is_int ) {
        convert << std::setprecision( 0 );
    } else if( three_decimal ) {
        convert << std::setprecision( 3 );
    } else {
        convert << std::setprecision( 2 );
    }
    convert << std::fixed << Value;
    sValue = convert.str();
    bNewLine = !( Flags & no_newline );
    bLowerIsBetter = static_cast<bool>( Flags & lower_is_better );
    bDrawName = !( Flags & no_name );
}

iteminfo::iteminfo( const std::string &Type, const std::string &Name, flags Flags )
    : iteminfo( Type, Name, "", Flags )
{
}

iteminfo::iteminfo( const std::string &Type, const std::string &Name, double Value,
                    double UnitVal )
    : iteminfo( Type, Name, "", no_flags, Value, UnitVal )
{
}

iteminfo vol_to_info( const std::string &type, const std::string &left,
                      const units::volume &vol, int decimal_places, bool lower_is_better )
{
    iteminfo::flags f = iteminfo::no_newline;
    if( lower_is_better ) {
        f |= iteminfo::lower_is_better;
    }
    int converted_volume_scale = 0;
    const double converted_volume =
        round_up( convert_volume( vol.value(), &converted_volume_scale ), decimal_places );
    if( converted_volume_scale != 0 ) {
        f |= iteminfo::is_decimal;
    }
    return iteminfo( type, left, string_format( "<num> %s", volume_units_abbr() ), f,
                     converted_volume );
}

iteminfo weight_to_info( const std::string &type, const std::string &left,
                         const units::mass &weight, int /* decimal_places */, bool lower_is_better )
{
    iteminfo::flags f = iteminfo::no_newline;
    if( lower_is_better ) {
        f |= iteminfo::lower_is_better;
    }
    const double converted_weight = convert_weight( weight );
    f |= iteminfo::is_decimal;
    return iteminfo( type, left, string_format( "<num> %s", weight_units() ), f,
                     converted_weight );
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

bool item::detonate( const tripoint &p, std::vector<item> &drops )
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
            apply_ammo_effects( nullptr, p, type->ammo->ammo_effects );
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
    return it->second.find( needle ) != std::string::npos;
}

void item::mark_as_used_by_player( const Character &p )
{
    std::string &used_by_ids = item_vars[ USED_BY_IDS ];
    if( used_by_ids.empty() ) {
        // *always* start with a ';'
        used_by_ids = ";";
    }
    // and always end with a ';'
    used_by_ids += string_format( "%d;", p.getID().get_value() );
}

bool item::can_holster( const item &obj, bool ) const
{
    if( !type->can_use( "holster" ) ) {
        return false; // item is not a holster
    }

    const holster_actor *ptr = dynamic_cast<const holster_actor *>
                               ( type->get_use( "holster" )->get_actor_ptr() );
    return ptr->can_holster( *this, obj );
}

bool item::will_spill() const
{
    return contents.will_spill();
}

bool item::will_spill_if_unsealed() const
{
    return contents.will_spill_if_unsealed();
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

    if( active || ethereal || wetness || link ||
        has_flag( flag_RADIO_ACTIVATION ) || has_relic_recharge() ||
        has_fault_flag( flag_BLACKPOWDER_FOULING_DAMAGE ) ) {
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

bool item::process_temperature_rot( float insulation, const tripoint &pos, map &here,
                                    Character *carrier, const temperature_flag flag, float spoil_modifier )
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
            temp = AVERAGE_ANNUAL_TEMPERATURE;
            break;
        default:
            debugmsg( "Temperature flag enum not valid.  Using current temperature." );
    }

    bool carried = carrier != nullptr && carrier->has_item( *this );
    // body heat increases inventory temperature by 5 F (2.77 K) and insulation by 50%
    if( carried ) {
        insulation *= 1.5;
        temp += units::from_fahrenheit_delta( 5 );
    }

    time_point time = last_temp_check;
    item_internal::scoped_goes_bad_cache _cache( this );
    const bool process_rot = goes_bad() && spoil_modifier != 0;

    if( now - time > 1_hours ) {
        // This code is for items that were left out of reality bubble for long time

        const weather_generator &wgen = get_weather().get_cur_weather_gen();
        const unsigned int seed = g->get_seed();

        units::temperature_delta temp_mod;
        // Toilets and vending machines will try to get the heat radiation and convection during mapgen and segfault.
        if( !g->new_game ) {
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
            if( pos.z >= 0 && flag != temperature_flag::ROOT_CELLAR ) {
                env_temperature = wgen.get_weather_temperature( pos, time, seed );
            } else {
                env_temperature = AVERAGE_ANNUAL_TEMPERATURE;
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
                    env_temperature = AVERAGE_ANNUAL_TEMPERATURE;
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

std::vector<trait_id> item::mutations_from_wearing( const Character &guy, bool removing ) const
{
    if( !is_relic() ) {
        return std::vector<trait_id> {};
    }
    std::vector<trait_id> muts;

    for( const enchant_cache &ench : relic_data->get_proc_enchantments() ) {
        for( const trait_id &mut : ench.get_mutations() ) {
            // this may not be perfectly accurate due to conditions
            muts.push_back( mut );
        }
    }
    if( type->relic_data ) {
        for( const enchantment &ench : type->relic_data->get_defined_enchantments() ) {
            for( const trait_id &mut : ench.get_mutations() ) {
                // this may not be perfectly accurate due to conditions
                muts.push_back( mut );
            }
        }
    }
    for( const trait_id &char_mut : guy.get_mutations( true, removing ) ) {
        for( auto iter = muts.begin(); iter != muts.end(); ) {
            if( char_mut == *iter ) {
                iter = muts.erase( iter );
            } else {
                ++iter;
            }
        }
    }

    return muts;
}

void item::overwrite_relic( const relic &nrelic )
{
    this->relic_data = cata::make_value<relic>( nrelic );
}

bool item::use_relic( Character &guy, const tripoint &pos )
{
    return relic_data->activate( guy, pos );
}

void item::process_relic( Character *carrier, const tripoint &pos )
{
    if( !is_relic() ) {
        return;
    }

    relic_data->try_recharge( *this, carrier, pos );
}

bool item::process_corpse( map &here, Character *carrier, const tripoint &pos )
{
    // some corpses rez over time
    if( corpse == nullptr || damage() >= max_damage() ) {
        return false;
    }

    // handle human corpses rising as zombies
    if( corpse->id == mtype_id::NULL_ID() && !has_var( "zombie_form" ) &&
        !mon_human->zombify_into.is_empty() ) {
        set_var( "zombie_form", mon_human->zombify_into.c_str() );
    }

    if( !ready_to_revive( here, pos ) ) {
        return false;
    }
    if( rng( 0, volume() / units::legacy_volume_factor ) > burnt && g->revive_corpse( pos, *this ) ) {
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

bool item::process_fake_mill( map &here, Character * /*carrier*/, const tripoint &pos )
{
    if( here.furn( pos ) != furn_f_wind_mill_active &&
        here.furn( pos ) != furn_f_water_mill_active ) {
        item_counter = 0;
        return true; //destroy fake mill
    }
    if( age() >= 6_hours || item_counter == 0 ) {
        iexamine::mill_finalize( get_avatar(), pos,
                                 birthday() ); //activate effects when timers goes to zero
        return true; //destroy fake mill item
    }

    return false;
}

bool item::process_fake_smoke( map &here, Character * /*carrier*/, const tripoint &pos )
{
    if( here.furn( pos ) != furn_f_smoking_rack_active &&
        here.furn( pos ) != furn_f_metal_smoking_rack_active ) {
        item_counter = 0;
        return true; //destroy fake smoke
    }

    if( age() >= 6_hours || item_counter == 0 ) {
        iexamine::on_smoke_out( pos, birthday() ); //activate effects when timers goes to zero
        return true; //destroy fake smoke when it 'burns out'
    }

    return false;
}

bool item::process_litcig( map &here, Character *carrier, const tripoint &pos )
{
    // cig dies out
    if( item_counter == 0 ) {
        if( carrier != nullptr ) {
            carrier->add_msg_if_player( m_neutral, _( "You finish your %s." ), tname() );
        }
        if( typeId() == itype_cig_lit ) {
            convert( itype_cig_butt );
        } else if( typeId() == itype_cigar_lit ) {
            convert( itype_cigar_butt );
        } else { // joint
            convert( itype_joint_roach );
            if( carrier != nullptr ) {
                carrier->add_effect( effect_weed_high, 1_minutes ); // one last puff
                here.add_field( pos + point( rng( -1, 1 ), rng( -1, 1 ) ), field_type_id( "fd_weedsmoke" ), 2 );
                weed_msg( *carrier );
            }
        }
        active = false;
        return false;
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
        carrier->add_msg_if_player( m_neutral, _( "You take a puff of your %s." ), tname() );
        if( has_flag( flag_TOBACCO ) ) {
            carrier->add_effect( effect_cig, duration );
        } else {
            carrier->add_effect( effect_weed_high, duration / 2 );
        }
        carrier->moves -= 15;

        if( ( carrier->has_effect( effect_shakes ) && one_in( 10 ) ) ||
            ( carrier->has_trait( trait_JITTERY ) && one_in( 200 ) ) ) {
            carrier->add_msg_if_player( m_bad, _( "Your shaking hand causes you to drop your %s." ),
                                        tname() );
            here.add_item_or_charges( pos + point( rng( -1, 1 ), rng( -1, 1 ) ), *this );
            return true; // removes the item that has just been added to the map
        }

        if( carrier->has_effect( effect_sleep ) ) {
            carrier->add_msg_if_player( m_bad, _( "You fall asleep and drop your %s." ),
                                        tname() );
            here.add_item_or_charges( pos + point( rng( -1, 1 ), rng( -1, 1 ) ), *this );
            return true; // removes the item that has just been added to the map
        }
    } else {
        // If not carried by someone, but laying on the ground:
        if( item_counter % 5 == 0 ) {
            // lit cigarette can start fires
            if( here.flammable_items_at( pos ) ||
                here.has_flag( ter_furn_flag::TFLAG_FLAMMABLE, pos ) ||
                here.has_flag( ter_furn_flag::TFLAG_FLAMMABLE_ASH, pos ) ) {
                here.add_field( pos, fd_fire, 1 );
            }
        }
    }

    // Item remains
    return false;
}

bool item::process_extinguish( map &here, Character *carrier, const tripoint &pos )
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

    // cig dies out
    if( has_flag( flag_LITCIG ) ) {
        if( typeId() == itype_cig_lit ) {
            convert( itype_cig_butt );
        } else if( typeId() == itype_cigar_lit ) {
            convert( itype_cigar_butt );
        } else { // joint
            convert( itype_joint_roach );
        }
    } else { // transform (lit) items
        if( type->revert_to ) {
            convert( *type->revert_to );
        } else {
            type->invoke( carrier, *this, pos, "transform" );
        }

    }
    active = false;
    // Item remains
    return false;
}

void item::set_link_traits( const bool assign_t_state )
{
    if( !link || !type->can_use( "link_up" ) ) {
        return;
    }

    const link_up_actor *it_actor = static_cast<const link_up_actor *>
                                    ( get_use( "link_up" )->get_actor_ptr() );
    link->max_length = it_actor->cable_length == -1 ? type->maximum_charges() : it_actor->cable_length;
    link->efficiency = it_actor->efficiency < MIN_LINK_EFFICIENCY ? 0.0f : it_actor->efficiency;
    // Reset s_bub_pos to force the item to check the length during process_link.
    link->s_bub_pos = tripoint_min;

    for( const item *cable : cables() ) {
        if( !cable->type->can_use( "link_up" ) ) {
            continue;
        }
        const link_up_actor *actor = static_cast<const link_up_actor *>
                                     ( cable->get_use( "link_up" )->get_actor_ptr() );
        link->max_length += actor->cable_length == -1 ? cable->type->maximum_charges() :
                            actor->cable_length;
        link->efficiency = link->efficiency < MIN_LINK_EFFICIENCY ? 0.0f :
                           link->efficiency * actor->efficiency;
    }

    link->charge_rate = link->efficiency < MIN_LINK_EFFICIENCY ? 0 : it_actor->charge_rate.value();
    // Convert charge_rate to how long it takes to charge 1 kW, the unit batteries use.
    // 0 means batteries won't be charged, but it can still provide power to devices unless efficiency is also 0.
    link->charge_interval = link->efficiency < MIN_LINK_EFFICIENCY || it_actor->charge_rate == 0_W ? 0 :
                            std::max( static_cast<int>( std::floor( 1000000.0 / abs( it_actor->charge_rate.value() ) + 0.5 ) ),
                                      1 );

    if( assign_t_state && link->t_veh_safe ) {
        // Assign t_state based on the parts available at the connected mount point.
        if( it_actor->targets.find( link_state::vehicle_port ) != it_actor->targets.end() &&
            ( link->t_veh_safe->avail_part_with_feature( link->t_mount, "CABLE_PORTS" ) != -1 ||
              link->t_veh_safe->avail_part_with_feature( link->t_mount, "APPLIANCE" ) != -1 ) ) {
            link->t_state = link_state::vehicle_port;
        } else if( it_actor->targets.find( link_state::vehicle_battery ) != it_actor->targets.end() &&
                   ( link->t_veh_safe->avail_part_with_feature( link->t_mount, "BATTERY" ) != -1 ||
                     link->t_veh_safe->avail_part_with_feature( link->t_mount, "APPLIANCE" ) != -1 ) ) {
            link->t_state = link_state::vehicle_battery;
        }
    }
}

int item::link_length() const
{
    return !link || link->has_no_links() ? -2 :
           link->has_state( link_state::needs_reeling ) ? -1 : link->length;
}

int item::max_link_length() const
{
    return !link ? -2 : link->max_length != -1 ? link->max_length : type->maximum_charges();
}

int item::link_sort_key() const
{
    const int length = link_length();
    int key = length >= 0 ? -1000000000 : length == -1 ? 0 : 1000000000;
    key += max_link_length() * 100000;
    return key - length;
}

bool item::process_link( map &here, Character *carrier, const tripoint &pos )
{
    if( link_length() < 0 ) {
        return false;
    }

    const bool is_cable_item = has_flag( flag_CABLE_SPOOL );

    // Handle links to items in the inventory.
    if( link->s_state == link_state::solarpack ) {
        if( carrier == nullptr || !carrier->worn_with_flag( flag_SOLARPACK_ON ) ) {
            add_msg_if_player_sees( pos, m_bad,
                                    string_format( is_cable_item ? _( "The %s has come loose from the solar pack." ) :
                                                   _( "The %s's cable has come loose from the solar pack." ), type_name() ) );
            reset_link( carrier );
            return false;
        }
    }
    const item_filter used_ups = [&]( const item & itm ) {
        return itm.has_flag( flag_IS_UPS ) && itm.get_var( "cable" ) == "plugged_in";
    };
    if( link->s_state == link_state::ups ) {
        if( carrier == nullptr || !carrier->has_item_with( used_ups ) ) {
            add_msg_if_player_sees( pos, m_bad,
                                    string_format( is_cable_item ? _( "The %s has come loose from the UPS." ) :
                                                   _( "The %s's cable has come loose from the UPS." ), type_name() ) );
            reset_link( carrier );
            return false;
        }
    }
    // Certain cable states should skip processing and also become inactive if dropped.
    if( ( link->t_state == link_state::no_link && link->s_state != link_state::vehicle_tow ) ||
        link->t_state == link_state::bio_cable ) {
        if( carrier == nullptr ) {
            return reset_link( nullptr, -1, true, pos );
        }
        return false;
    }

    const bool last_t_abs_pos_is_oob = !here.inbounds( link->t_abs_pos );

    // Lambda function for setting a cable's length, then checking if it's now stretched too long, resetting it if so.
    // @return True if the cable is disconnected.
    const auto set_length_and_check = [this, carrier, pos, last_t_abs_pos_is_oob,
          is_cable_item]( float new_length ) {
        link->length = new_length;
        if( debug_mode ) {
            add_msg_debug( debugmode::DF_IUSE, "%s linked to %s%s, length %d/%d", type_name(),
                           link->t_abs_pos.to_string_writable(), last_t_abs_pos_is_oob ? " (OoB)" : "",
                           link->length, link->max_length );
        }
        if( link->length > link->max_length ) {
            if( carrier != nullptr ) {
                carrier->add_msg_if_player( m_bad,
                                            string_format( is_cable_item ? _( "Your over-extended %s breaks loose!" ) :
                                                    _( "Your %s's over-extended cable breaks loose!" ), type_name() ) );
            } else {
                add_msg_if_player_sees( pos, m_bad,
                                        string_format( is_cable_item ? _( "The over-extended %s breaks loose!" ) :
                                                       _( "The %s's over-extended cable breaks loose!" ), type_name() ) );
            }
            return true;
        } else if( new_length + M_SQRT2 >= link->max_length + 1 && carrier != nullptr ) {
            carrier->add_msg_if_player( m_warning,
                                        string_format( is_cable_item ? _( "Your %s is stretched to its limit!" ) :
                                                _( "Your %s's cable is stretched to its limit!" ), type_name() ) );
        }
        return false;
    };

    // Check if the item has moved positions this turn.
    bool check_length = false;
    if( link->s_bub_pos != pos ) {
        link->s_bub_pos = pos;
        check_length = true;
    }

    // Re-establish vehicle pointer if it got lost or if this item just got loaded.
    if( !link->t_veh_safe ) {
        vehicle *found_veh = vehicle::find_vehicle( link->t_abs_pos );
        if( !found_veh ) {
            return reset_link( carrier, -2, true, pos );
        }
        if( debug_mode ) {
            add_msg_debug( debugmode::DF_IUSE, "Re-established link of %s to %s.", type_name(),
                           found_veh->disp_name() );
        }
        link->t_veh_safe = found_veh->get_safe_reference();
    }

    // Regular pointers are faster, so make one now that we know the reference is valid.
    vehicle *t_veh = link->t_veh_safe.get();

    // We should skip processing if the last saved target point is out of bounds, since vehicles give innacurate absolute coordinates when out of bounds.
    // However, if the linked vehicle is moving fast enough, we should always do processing to avoid erroneously skipping linked items riding inside of it.
    if( last_t_abs_pos_is_oob && t_veh->velocity < HALF_MAPSIZE_X * 400 ) {
        if( !check_length ) {
            return false;
        }
        if( trigdist ) {
            if( set_length_and_check( trig_dist( here.getabs( pos ), link->t_abs_pos.raw() ) +
                                      link->t_mount.abs().x + link->t_mount.abs().y ) ) {
                return reset_link( carrier );
            }
        } else if( set_length_and_check( square_dist( here.getabs( pos ), link->t_abs_pos.raw() ) +
                                         link->t_mount.abs().x + link->t_mount.abs().y ) ) {
            return reset_link( carrier );
        }
        return false;
    }

    // Find the vp_part index the cable is linked to.
    int link_vp_index = -1;
    if( link->t_state == link_state::vehicle_port ) {
        for( int idx : t_veh->cable_ports ) {
            if( t_veh->part( idx ).mount == link->t_mount ) {
                link_vp_index = idx;
                break;
            }
        }
    } else if( link->t_state == link_state::vehicle_battery ) {
        for( int idx : t_veh->batteries ) {
            if( t_veh->part( idx ).mount == link->t_mount ) {
                link_vp_index = idx;
                break;
            }
        }
        if( link_vp_index == -1 ) {
            // Check cable_ports, since that includes appliances
            for( int idx : t_veh->cable_ports ) {
                if( t_veh->part( idx ).mount == link->t_mount ) {
                    link_vp_index = idx;
                    break;
                }
            }
        }
    } else if( link->t_state == link_state::vehicle_tow || link->s_state == link_state::vehicle_tow ) {
        link_vp_index = t_veh->part_at( t_veh->coord_translate( link->t_mount ) );
    }
    if( link_vp_index == -1 ) {
        // The part with cable ports was lost, so disconnect the cable.
        return reset_link( carrier, -2, true, pos );
    }

    if( link->last_processed <= t_veh->part( link_vp_index ).last_disconnected ) {
        add_msg_if_player_sees( pos, m_warning, string_format( _( "You detached the %s." ), type_name() ) );
        return reset_link( carrier, -2 );
    }
    t_veh->part( link_vp_index ).set_flag( vp_flag::linked_flag );

    int turns_elapsed = to_turns<int>( calendar::turn - link->last_processed );
    link->last_processed = calendar::turn;

    // Set the new absolute position to the vehicle's origin.
    tripoint t_veh_bub_pos = t_veh->global_pos3();
    tripoint_abs_ms new_t_abs_pos = here.getglobal( t_veh_bub_pos );
    if( link->t_abs_pos != new_t_abs_pos ) {
        link->t_abs_pos = new_t_abs_pos;
        check_length = true;
    }

    // If either of the link's connected sides moved, check the cable's length.
    if( check_length ) {
        if( trigdist ) {
            if( set_length_and_check( trig_dist( pos, t_veh_bub_pos +
                                                 t_veh->part( link_vp_index ).precalc[0] ) ) ) {
                return reset_link( carrier, link_vp_index );
            }
        } else if( set_length_and_check( square_dist( pos, t_veh_bub_pos +
                                         t_veh->part( link_vp_index ).precalc[0] ) ) ) {
            return reset_link( carrier, link_vp_index );
        }
    }

    // Extra behaviors for the cabled item.
    if( !is_cable_item ) {
        int power_draw = 0;

        if( link->efficiency < MIN_LINK_EFFICIENCY ) {
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
    if( link->charge_rate == 0 || turns_elapsed < 1 ||
        link->charge_interval < 1 || link->efficiency < MIN_LINK_EFFICIENCY ) {
        return link->charge_rate;
    }

    if( !is_battery() ) {
        const item *parent_mag = magazine_current();
        if( !parent_mag || !parent_mag->has_flag( flag_RECHARGE ) ) {
            return 0;
        }
    }

    const bool power_in = link->charge_rate > 0;
    if( power_in ? ammo_remaining() >= ammo_capacity( ammo_battery ) :
        ammo_remaining() <= 0 ) {
        return 0;
    }

    // Normally efficiency is the chance to get a charge every charge_interval, but if we're catching up from
    // time spent ouside the reality bubble it should be applied as a percentage of the total instead.
    bool short_time_passed = turns_elapsed <= link->charge_interval;

    if( short_time_passed &&
        !calendar::once_every( time_duration::from_turns( link->charge_interval ) ) ) {
        return link->charge_rate;
    }

    // If a long time passed, multiply the total by the efficiency rather than cancelling a charge.
    int transfer_total = short_time_passed ? 1 :
                         ( turns_elapsed * 1.0f / link->charge_interval ) * link->charge_interval;

    if( power_in ) {
        const int battery_deficit = linked_veh.discharge_battery( transfer_total, true );
        // Around 85% efficient by default; a few of the discharges don't actually recharge
        if( battery_deficit == 0 && ( !short_time_passed || rng_float( 0.0, 1.0 ) <= link->efficiency ) ) {
            ammo_set( itype_battery, ammo_remaining() + transfer_total );
        }
    } else {
        // Around 85% efficient by default; a few of the discharges don't actually charge
        if( !short_time_passed || rng_float( 0.0, 1.0 ) <= link->efficiency ) {
            const int battery_surplus = linked_veh.charge_battery( transfer_total, true );
            if( battery_surplus == 0 ) {
                ammo_set( itype_battery, ammo_remaining() - transfer_total );
            }
        } else {
            const std::pair<int, int> linked_levels = linked_veh.connected_battery_power_level();
            if( linked_levels.first < linked_levels.second ) {
                ammo_set( itype_battery, ammo_remaining() - transfer_total );
            }
        }
    }
    return link->charge_rate;
}

bool item::reset_link( Character *p, int vpart_index,
                       const bool loose_message, const tripoint cable_position )
{
    if( !link ) {
        return has_flag( flag_NO_DROP );
    }
    // Cables that need reeling should be reset with a reel_cable_activity_actor instead.
    if( link->has_state( link_state::needs_reeling ) ) {
        return false;
    }

    if( vpart_index != -2 && link->t_veh_safe ) {
        if( vpart_index == -1 ) {
            vehicle *t_veh = link->t_veh_safe.get();
            // Find the vp_part index the cable is linked to.
            if( link->t_state == link_state::vehicle_port ) {
                for( int idx : t_veh->cable_ports ) {
                    if( t_veh->part( idx ).mount == link->t_mount ) {
                        vpart_index = idx;
                        break;
                    }
                }
            } else if( link->t_state == link_state::vehicle_battery ) {
                for( int idx : t_veh->batteries ) {
                    if( t_veh->part( idx ).mount == link->t_mount ) {
                        vpart_index = idx;
                        break;
                    }
                }
            } else if( link->t_state == link_state::vehicle_tow || link->s_state == link_state::vehicle_tow ) {
                vpart_index = t_veh->part_at( t_veh->coord_translate( link->t_mount ) );
            }
        }
        if( vpart_index != -1 ) {
            link->t_veh_safe->part( vpart_index ).remove_flag( vp_flag::linked_flag );
        }
    }

    const bool is_cable_item = has_flag( flag_CABLE_SPOOL );

    if( loose_message ) {
        if( p != nullptr ) {
            p->add_msg_if_player( m_warning,
                                  string_format( is_cable_item ? _( "Your %s has come loose." ) :
                                                 _( "Your %s's cable has come loose." ), type_name() ) );
        } else {
            add_msg_if_player_sees( cable_position, m_warning,
                                    string_format( is_cable_item ? _( "The %s has come loose." ) :
                                                   _( "The %s's cable has come loose." ), type_name() ) );
        }
    }

    const int respool_threshold = 6;
    if( link->length > respool_threshold ) {
        // Cables that are too long need to be manually rewound before reuse.
        link->s_state = link_state::needs_reeling;
        return false;
    }

    if( loose_message && p ) {
        p->add_msg_if_player( m_info, string_format( is_cable_item ?
                              _( "You reel in the %s and wind it up." ) :
                              _( "You reel in the %s's cable and wind it up." ), tname() ) );
    }

    link.reset();
    if( !cables().empty() ) {
        // If there are extensions, keep link active to maintain max_length.
        link = cata::make_value<item::link_data>();
        set_link_traits();
    }
    return has_flag( flag_NO_DROP );
}

bool item::process_linked_item( Character *carrier, const tripoint & /*pos*/,
                                const link_state required_state )
{
    if( carrier == nullptr ) {
        erase_var( "cable" );
        active = false;
        return false;
    }
    bool has_connected_cable = carrier->has_item_with( [&required_state]( const item & it ) {
        return it.link && it.link->has_state( required_state );
    } );
    if( !has_connected_cable ) {
        erase_var( "cable" );
        active = false;
    }
    return false;
}

bool item::process_wet( Character * /*carrier*/, const tripoint & /*pos*/ )
{
    if( item_counter == 0 ) {
        if( type->revert_to ) {
            convert( *type->revert_to );
        }
        unset_flag( flag_WET );
        active = false;
    }
    // Always return true so our caller will bail out instead of processing us as a tool.
    return true;
}

bool item::process_tool( Character *carrier, const tripoint &pos )
{
    // FIXME: remove this once power armors don't need to be TOOL_ARMOR anymore
    if( is_power_armor() && carrier && carrier->can_interface_armor() && carrier->has_power() ) {
        return false;
    }

    // if insufficient available charges shutdown the tool
    if( ( type->tool->turns_per_charge > 0 || type->tool->power_draw > 0_W ) &&
        ammo_remaining( carrier, true ) == 0 ) {
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

    int energy = 0;
    if( type->tool->turns_per_charge > 0 &&
        to_turn<int>( calendar::turn ) % type->tool->turns_per_charge == 0 ) {
        energy = std::max( ammo_required(), 1 );
    } else if( type->tool->power_draw > 0_W ) {
        // kJ (battery unit) per second
        energy = units::to_kilowatt( type->tool->power_draw );
        // energy_bat remainder results in chance at additional charge/discharge
        const int kw_in_mw = units::to_milliwatt( 1_kW );
        energy += x_in_y( units::to_milliwatt( type->tool->power_draw ) % kw_in_mw, kw_in_mw ) ? 1 : 0;
    }

    if( energy > 0 ) {
        ammo_consume( energy, pos, carrier );
    }

    type->tick( carrier, *this, pos );
    return false;
}

bool item::process_blackpowder_fouling( Character *carrier )
{
    // rust is deterministic. 12 hours for first rust, then 24 (36 total), then 36 (72 total) and finally 48 (120 hours to go to XX)
    // this speeds up by the amount the gun is dirty, 2-6x as fast depending on dirt level.
    set_var( "rust_timer", get_var( "rust_timer", 0 ) + 1 + get_var( "dirt", 0 ) / 2000 );
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

bool item::process( map &here, Character *carrier, const tripoint &pos, float insulation,
                    temperature_flag flag, float spoil_multiplier_parent, bool recursive )
{
    process_relic( carrier, pos );
    if( recursive ) {
        contents.process( here, carrier, pos, type->insulation_factor * insulation, flag,
                          spoil_multiplier_parent );
    }
    return process_internal( here, carrier, pos, insulation, flag, spoil_multiplier_parent );
}

bool item::leak( map &here, Character *carrier, const tripoint &pos, item_pocket *pocke )
{
    if( is_container() ) {
        contents.leak( here, carrier, pos, pocke );
        return false;
    } else if( this->made_of( phase_id::LIQUID ) && !this->is_frozen_liquid() ) {
        if( pocke ) {
            if( pocke->watertight() ) {
                unset_flag( flag_FROM_FROZEN_LIQUID );
                return false;
            }
            return true;
        }
        return true;
    }
    return false;
}

void item::set_last_temp_check( const time_point &pt )
{
    last_temp_check = pt;
}

bool item::process_internal( map &here, Character *carrier, const tripoint &pos,
                             float insulation, const temperature_flag flag, float spoil_modifier )
{
    if( ethereal ) {
        if( !has_var( "ethereal" ) ) {
            return true;
        }
        set_var( "ethereal", std::stoi( get_var( "ethereal" ) ) - 1 );
        const bool processed = std::stoi( get_var( "ethereal" ) ) <= 0;
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

    if( link ) {
        process_link( here, carrier, pos );
    }

    // Remaining stuff is only done for active items.
    if( active ) {

        if( wetness && has_flag( flag_WATER_BREAK ) ) {
            deactivate();
            set_flag( flag_ITEM_BROKEN );
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
                convert( *type->revert_to );

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
            process_temperature_rot( insulation, pos, here, carrier, flag, spoil_modifier ) ) {
            if( is_comestible() ) {
                here.rotten_item_spawn( *this, pos );
            }
            return true;
        }
    } else {
        // guns are never active so we only need to tick this on inactive items. For performance reasons.
        if( has_fault_flag( flag_BLACKPOWDER_FOULING_DAMAGE ) ) {
            return process_blackpowder_fouling( carrier );
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

time_duration item::get_plant_epoch() const
{
    if( !type->seed ) {
        return 0_turns;
    }
    // Growing times have been based around real world season length rather than
    // the default in-game season length to give
    // more accuracy for longer season lengths
    // Also note that seed->grow is the time it takes from seeding to harvest, this is
    // divided by 3 to get the time it takes from one plant state to the next.
    // TODO: move this into the islot_seed
    return type->seed->grow * calendar::season_ratio() / 3;
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

bool item::is_soft() const
{
    if( has_flag( flag_SOFT ) ) {
        return true;
    } else if( has_flag( flag_HARD ) ) {
        return false;
    }

    const std::map<material_id, int> &mats = made_of();
    return std::all_of( mats.begin(), mats.end(), []( const std::pair<material_id, int> &mid ) {
        return mid.first->soft();
    } );
}

bool item::is_rigid() const
{
    // overrides for the item overall
    if( has_flag( flag_SOFT ) ) {
        return false;
    } else if( has_flag( flag_HARD ) ) {
        return true;
    }

    // if the item has no armor data it isn't rigid
    const islot_armor *armor = find_armor_data();
    if( armor == nullptr ) {
        return false;
    }

    bool is_rigid = false;

    for( const armor_portion_data &portion : armor->sub_data ) {
        is_rigid |= portion.rigid;
    }

    // check if ablative pieces are rigid too
    if( is_ablative() ) {
        for( const item_pocket *pocket : contents.get_all_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();
                is_rigid |= ablative_armor.is_rigid();
            }
        }
    }

    return is_rigid;
}

bool item::is_comfortable() const
{
    // overrides for the item overall
    // NO_WEAR_EFFECT is there for jewelry and the like which is too small to be considered
    if( has_flag( flag_SOFT ) || has_flag( flag_PADDED ) || has_flag( flag_NO_WEAR_EFFECT ) ) {
        return true;
    } else if( has_flag( flag_HARD ) ) {
        return false;
    }

    // if the item has no armor data it isn't rigid
    const islot_armor *armor = find_armor_data();
    if( armor == nullptr ) {
        return true;
    }

    for( const armor_portion_data &portion : armor->sub_data ) {
        if( portion.comfortable ) {
            return true;
        }
    }

    return false;
}

template <typename T>
bool item::is_bp_rigid( const T &bp ) const
{
    // overrides for the item overall
    if( has_flag( flag_SOFT ) ) {
        return false;
    } else if( has_flag( flag_HARD ) ) {
        return true;
    }

    const armor_portion_data *portion = portion_for_bodypart( bp );

    bool is_rigid = false;

    if( portion ) {
        is_rigid |= portion->rigid;
    }

    // check if ablative pieces are rigid too
    if( is_ablative() ) {
        for( const item_pocket *pocket : contents.get_all_ablative_pockets() ) {
            if( !pocket->empty() ) {
                // get the contained plate
                const item &ablative_armor = pocket->front();
                is_rigid |= ablative_armor.is_bp_rigid( bp );
            }
        }
    }

    return is_rigid;
}

// initialize for sub_bodyparts and body parts
template bool item::is_bp_rigid<sub_bodypart_id>( const sub_bodypart_id &bp ) const;

template bool item::is_bp_rigid<bodypart_id>( const bodypart_id &bp ) const;

template <typename T>
bool item::is_bp_rigid_selective( const T &bp ) const
{
    bool is_rigid;

    const armor_portion_data *portion = portion_for_bodypart( bp );

    if( !portion ) {
        return false;
    }

    // overrides for the item overall
    if( has_flag( flag_SOFT ) ) {
        is_rigid = false;
    } else if( has_flag( flag_HARD ) ) {
        is_rigid = true;
    } else {
        is_rigid = portion->rigid;
    }

    return is_rigid && portion->rigid_layer_only;
}

// initialize for sub_bodyparts and body parts
template bool item::is_bp_rigid_selective<sub_bodypart_id>( const sub_bodypart_id &bp ) const;

template bool item::is_bp_rigid_selective<bodypart_id>( const bodypart_id &bp ) const;

template <typename T>
bool item::is_bp_comfortable( const T &bp ) const
{
    // overrides for the item overall
    // NO_WEAR_EFFECT is there for jewelry and the like which is too small to be considered
    if( has_flag( flag_SOFT ) || has_flag( flag_PADDED ) || has_flag( flag_NO_WEAR_EFFECT ) ) {
        return true;
    } else if( has_flag( flag_HARD ) ) {
        return false;
    }

    const armor_portion_data *portion = portion_for_bodypart( bp );

    if( !portion ) {
        return false;
    }

    return portion->comfortable;
}

// initialize for sub_bodyparts and body parts
template bool item::is_bp_comfortable<sub_bodypart_id>( const sub_bodypart_id &bp ) const;

template bool item::is_bp_comfortable<bodypart_id>( const bodypart_id &bp ) const;

bool item::is_reloadable() const
{
    if( has_flag( flag_NO_RELOAD ) && !has_flag( flag_VEHICLE ) ) {
        return false; // turrets ignore NO_RELOAD flag

    }

    for( const item_pocket *pocket : contents.get_all_reloadable_pockets() ) {
        if( pocket->is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            if( pocket->empty() || !pocket->front().is_magazine_full() ) {
                return true;
            }
        } else if( pocket->is_type( item_pocket::pocket_type::MAGAZINE ) ) {
            if( remaining_ammo_capacity() > 0 ) {
                return true;
            }
        } else if( pocket->is_type( item_pocket::pocket_type::CONTAINER ) ) {
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

std::string item::type_name( unsigned int quantity ) const
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
        return iter->second;
    } else if( has_itype_variant() ) {
        ret_name = itype_variant().alt_name.translated();
    } else {
        ret_name = type->nname( quantity );
    }

    // Apply conditional names, in order.
    for( const conditional_name &cname : type->conditional_names ) {
        // Lambda for searching for a item ID among all components.
        std::function<bool( item_components )> component_id_equals =
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
        std::function<bool ( item_components )> component_id_contains =
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
    if( corpse != nullptr && has_flag( flag_CORPSE ) ) {
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

std::string item::get_corpse_name() const
{
    return corpse_name;
}

std::string item::nname( const itype_id &id, unsigned int quantity )
{
    const itype *t = find_type( id );
    return t->nname( quantity );
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

std::string item::label( unsigned int quantity ) const
{
    if( has_label() ) {
        return get_var( "item_label" );
    }

    return type_name( quantity );
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

bool item::is_filthy() const
{
    return has_flag( flag_FILTHY );
}

bool item::on_drop( const tripoint &pos )
{
    return on_drop( pos, get_map() );
}

bool item::on_drop( const tripoint &pos, map &m )
{
    // dropping liquids, even currently frozen ones, on the ground makes them
    // dirty
    if( made_of_from_type( phase_id::LIQUID ) && !m.has_flag( ter_furn_flag::TFLAG_LIQUIDCONT, pos ) &&
        !has_own_flag( flag_DIRTY ) ) {
        set_flag( flag_DIRTY );
    }

    avatar &player_character = get_avatar();
    player_character.flag_encumbrance();
    player_character.invalidate_weight_carried_cache();
    return type->drop_action && type->drop_action.call( &player_character, *this, pos );
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
        min_str -= p.get_proficiency_bonus( "archery", proficiency_bonus_type::strength );

        for( const item *mod : gunmods() ) {
            min_str += mod->type->gunmod->min_str_required_mod;
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
                // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
                auto iter = std::find_if( ret.begin(), ret.end(), [component]( item_comp & obj ) {
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

bool item::has_clothing_mod() const
{
    for( const clothing_mod &cm : clothing_mods::get_all() ) {
        if( has_own_flag( cm.flag ) ) {
            return true;
        }
    }
    return false;
}

static const std::string &get_clothing_mod_val_key( clothing_mod_type type )
{
    const static auto cache = ( []() {
        std::array<std::string, clothing_mods::all_clothing_mod_types.size()> res;
        for( const clothing_mod_type &type : clothing_mods::all_clothing_mod_types ) {
            res[type] = CLOTHING_MOD_VAR_PREFIX + clothing_mods::string_from_clothing_mod_type(
                            clothing_mods::all_clothing_mod_types[type]
                        );
        }
        return res;
    } )();

    return cache[ type ];
}

float item::get_clothing_mod_val( clothing_mod_type type ) const
{
    return get_var( get_clothing_mod_val_key( type ), 0.0f );
}

void item::update_clothing_mod_val()
{
    for( const clothing_mod_type &type : clothing_mods::all_clothing_mod_types ) {
        float tmp = 0.0f;
        for( const clothing_mod &cm : clothing_mods::get_all_with( type ) ) {
            if( has_own_flag( cm.flag ) ) {
                tmp += cm.get_mod_val( type, *this );
            }
        }
        set_var( get_clothing_mod_val_key( type ), tmp );
    }
}

units::volume item::check_for_free_space() const
{
    units::volume volume;

    for( const item *container : contents.all_items_top( item_pocket::pocket_type::CONTAINER ) ) {
        std::vector<const item_pocket *> containedPockets =
            container->contents.get_all_contained_pockets();
        if( !containedPockets.empty() ) {
            volume += container->check_for_free_space();

            for( const item_pocket *pocket : containedPockets ) {
                if( pocket->rigid() && ( pocket->empty() || pocket->contains_phase( phase_id::SOLID ) ) &&
                    pocket->get_pocket_data()->ammo_restriction.empty() ) {
                    volume += pocket->remaining_volume();
                }
            }
        }
    }
    return volume;
}

int item::get_pocket_size() const
{
    // set the amount of space that will be used on the vest based on the size of the item
    if( has_flag( flag_PALS_SMALL ) ) {
        return 1;
    } else if( has_flag( flag_PALS_MEDIUM ) ) {
        return 2;
    } else {
        return 3;
    }
}

bool item::can_attach_as_pocket() const
{
    return has_flag( flag_PALS_SMALL ) || has_flag( flag_PALS_MEDIUM ) || has_flag( flag_PALS_LARGE );
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

bool item::has_unrestricted_pockets() const
{
    return contents.has_unrestricted_pockets();
}

units::volume item::get_contents_volume_with_tweaks( const std::map<const item *, int> &without )
const
{
    return contents.get_contents_volume_with_tweaks( without );
}

units::volume item::get_nested_content_volume_recursive( const std::map<const item *, int>
        &without )
const
{
    return contents.get_nested_content_volume_recursive( without );
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

void item::remove_internal( const std::function<bool( item & )> &filter,
                            int &count, std::list<item> &res )
{
    contents.remove_internal( filter, count, res );
}

std::list<const item *> item::all_items_top() const
{
    return contents.all_items_top();
}

std::list<item *> item::all_items_top()
{
    return contents.all_items_top();
}

std::list<const item *> item::all_items_top( item_pocket::pocket_type pk_type ) const
{
    return contents.all_items_top( pk_type );
}

std::list<item *> item::all_items_top( item_pocket::pocket_type pk_type, bool unloading )
{
    return contents.all_items_top( pk_type, unloading );
}

item const *item::this_or_single_content() const
{
    return type->category_force == item_category_container && contents_only_one_type()
           ? &contents.first_item()
           : this;
}

bool item::contents_only_one_type() const
{
    std::list<const item *> const items = contents.all_items_top( []( item_pocket const & pkt ) {
        return pkt.is_type( item_pocket::pocket_type::CONTAINER ) ||
               pkt.is_type( item_pocket::pocket_type::SOFTWARE );
    } );
    return items.size() == 1 ||
           ( items.size() > 1 &&
    std::all_of( ++items.begin(), items.end(), [&items]( item const * e ) {
        return e->stacks_with( *items.front() );
    } ) );
}

std::list<const item *> item::all_items_ptr() const
{
    std::list<const item *> all_items_internal;
    for( int i = static_cast<int>( item_pocket::pocket_type::CONTAINER );
         i < static_cast<int>( item_pocket::pocket_type::LAST ); i++ ) {
        std::list<const item *> inserted{ all_items_top_recursive( static_cast<item_pocket::pocket_type>( i ) ) };
        all_items_internal.insert( all_items_internal.end(), inserted.begin(), inserted.end() );
    }
    return all_items_internal;
}

std::list<const item *> item::all_items_ptr( item_pocket::pocket_type pk_type ) const
{
    return all_items_top_recursive( pk_type );
}

std::list<item *> item::all_items_ptr( item_pocket::pocket_type pk_type )
{
    return all_items_top_recursive( pk_type );
}

std::list<const item *> item::all_items_top_recursive( item_pocket::pocket_type pk_type )
const
{
    std::list<const item *> contained = contents.all_items_top( pk_type );
    std::list<const item *> all_items_internal{ contained };
    for( const item *it : contained ) {
        std::list<const item *> recursion_items = it->all_items_top_recursive( pk_type );
        all_items_internal.insert( all_items_internal.end(), recursion_items.begin(),
                                   recursion_items.end() );
    }

    return all_items_internal;
}

std::list<item *> item::all_items_top_recursive( item_pocket::pocket_type pk_type )
{
    std::list<item *> contained = contents.all_items_top( pk_type );
    std::list<item *> all_items_internal{ contained };
    for( item *it : contained ) {
        std::list<item *> recursion_items = it->all_items_top_recursive( pk_type );
        all_items_internal.insert( all_items_internal.end(), recursion_items.begin(),
                                   recursion_items.end() );
    }

    return all_items_internal;
}

std::list<item *> item::all_known_contents()
{
    return contents.all_known_contents();
}

std::list<const item *> item::all_known_contents() const
{
    return contents.all_known_contents();
}

std::list<item *> item::all_ablative_armor()
{
    return contents.all_ablative_armor();
}

std::list<const item *> item::all_ablative_armor() const
{
    return contents.all_ablative_armor();
}

void item::clear_items()
{
    contents.clear_items();
}

bool item::empty() const
{
    return contents.empty();
}

bool item::empty_container() const
{
    return contents.empty_container();
}

item &item::only_item()
{
    return contents.only_item();
}

const item &item::only_item() const
{
    return contents.only_item();
}

item *item::get_item_with( const std::function<bool( const item & )> &filter )
{
    return contents.get_item_with( filter );
}

size_t item::num_item_stacks() const
{
    return contents.num_item_stacks();
}

item &item::legacy_front()
{
    return contents.legacy_front();
}

const item &item::legacy_front() const
{
    return contents.legacy_front();
}

void item::favorite_settings_menu()
{
    contents.favorite_settings_menu( this );
}

void item::combine( const item_contents &read_input, bool convert )
{
    contents.combine( read_input, convert );
}
