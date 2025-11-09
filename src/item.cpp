#include "item.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "activity_actor_definitions.h"
#include "ammo.h"
#include "avatar.h"
#include "bionics.h"
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
#include "debug.h"
#include "effect.h" // for weed_msg
#include "enum_bitset.h"
#include "enums.h"
#include "faction.h"
#include "fault.h"
#include "field_type.h"
#include "flag.h"
#include "flat_set.h"
#include "game.h"
#include "game_constants.h"
#include "iexamine.h"
#include "item_category.h"
#include "item_factory.h"
#include "item_group.h"
#include "item_tname.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "localized_comparator.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "martialarts.h"
#include "material.h"
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
#include "vitamin.h"
#include "vpart_position.h"
#include "weather.h"
#include "weather_gen.h"
#include "weather_type.h"
#include "weighted_list.h"

static const ammotype ammo_battery( "battery" );
static const ammotype ammo_money( "money" );

static const efftype_id effect_cig( "cig" );
static const efftype_id effect_shakes( "shakes" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_weed_high( "weed_high" );

static const fault_id fault_emp_reboot( "fault_emp_reboot" );

static const furn_str_id furn_f_metal_smoking_rack_active( "f_metal_smoking_rack_active" );
static const furn_str_id furn_f_smoking_rack_active( "f_smoking_rack_active" );
static const furn_str_id furn_f_water_mill_active( "f_water_mill_active" );
static const furn_str_id furn_f_wind_mill_active( "f_wind_mill_active" );

static const item_category_id item_category_container( "container" );
static const item_category_id item_category_maps( "maps" );
static const item_category_id item_category_software( "software" );

static const itype_id itype_barrel_small( "barrel_small" );
static const itype_id itype_blood( "blood" );
static const itype_id itype_cash_card( "cash_card" );
static const itype_id itype_corpse( "corpse" );
static const itype_id itype_corpse_generic_human( "corpse_generic_human" );
static const itype_id itype_craft( "craft" );
static const itype_id itype_disassembly( "disassembly" );
static const itype_id itype_efile_photos( "efile_photos" );
static const itype_id itype_efile_recipes( "efile_recipes" );
static const itype_id itype_joint_lit( "joint_lit" );
static const itype_id itype_stock_none( "stock_none" );
static const itype_id itype_water( "water" );
static const itype_id itype_water_clean( "water_clean" );

static const material_id material_wool( "wool" );

static const mtype_id mon_human( "mon_human" );

static const quality_id qual_BOIL( "BOIL" );

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
    return g->faction_manager_ptr->get( get_old_owner() )->get_name();
}

std::string item::get_owner_name() const
{
    if( !g->faction_manager_ptr->get( get_owner() ) ) {
        debugmsg( "item::get_owner_name() item %s has no valid nor null faction id ", tname() );
        return "no owner";
    }
    return g->faction_manager_ptr->get( get_owner() )->get_name();
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
            length_adjusted = type->longest_side - sawn_off_reduction( );
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

void item::unset_flags()
{
    item_tags.clear();
    requires_tags_processing = true;
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

const std::map<std::pair<itype_id, std::string>, int> &item::brewing_results() const
{
    static const std::map<std::pair<itype_id, std::string>, int> nulresult{};
    return is_brewable() ? type->brewable->results : nulresult;
}

time_duration item::composting_time() const
{
    return is_compostable() ? type->compostable->time * calendar::season_from_default_ratio() : 0_turns;
}

const std::map<std::pair<itype_id, std::string>, int> &item::composting_results() const
{
    static const std::map<std::pair<itype_id, std::string>, int> nulresult{};
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

bool item::is_bionic() const
{
    return !!type->bionic;
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

bool item::is_irremovable() const
{
    return has_flag( flag_IRREMOVABLE );
}

bool item::is_identifiable() const
{
    return is_book();
}

int item::wheel_area() const
{
    return is_wheel() ? type->wheel->diameter * type->wheel->width : 0;
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

bool item::can_use_relic( const Character &guy ) const
{
    return is_relic() && relic_data->can_activate( *this, guy );
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

bool item::is_smokable() const
{
    return is_comestible() && get_comestible()->smoking_result != itype_id::NULL_ID();
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

bool item::is_upgrade() const
{
    if( !type->bionic ) {
        return false;
    }
    return type->bionic->is_upgrade;
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
