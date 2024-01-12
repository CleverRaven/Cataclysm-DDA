#include "item_pocket.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include "ammo.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "color.h"
#include "crafting.h"
#include "creature_tracker.h"
#include "debug.h"
#include "enums.h"
#include "flag.h"
#include "generic_factory.h"
#include "handle_liquid.h"
#include "item.h"
#include "item_category.h"
#include "item_factory.h"
#include "item_location.h"
#include "itype.h"
#include "json.h"
#include "json_loader.h"
#include "localized_comparator.h"
#include "map.h"
#include "math_defines.h"
#include "messages.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "units.h"
#include "units_utility.h"

namespace io
{
// *INDENT-OFF*
template<>
std::string enum_to_string<pocket_type>( pocket_type data )
{
    switch ( data ) {
    case pocket_type::CONTAINER: return "CONTAINER";
    case pocket_type::MAGAZINE: return "MAGAZINE";
    case pocket_type::MAGAZINE_WELL: return "MAGAZINE_WELL";
    case pocket_type::MOD: return "MOD";
    case pocket_type::CORPSE: return "CORPSE";
    case pocket_type::SOFTWARE: return "SOFTWARE";
    case pocket_type::EBOOK: return "EBOOK";
    case pocket_type::CABLE: return "CABLE";
    case pocket_type::MIGRATION: return "MIGRATION";
    case pocket_type::LAST: break;
    }
    cata_fatal( "Invalid valid_target" );
}
// *INDENT-ON*
} // namespace io

std::vector<item_pocket::favorite_settings> item_pocket::pocket_presets;

std::string pocket_data::check_definition() const
{
    if( type == pocket_type::MOD ||
        type == pocket_type::CORPSE ||
        type == pocket_type::MIGRATION ) {
        return "";
    }
    for( const itype_id &it : item_id_restriction ) {
        if( !it.is_valid() ) {
            return string_format( "item_id %s used in restriction invalid\n", it.str() );
        }
        if( it->phase == phase_id::LIQUID && !watertight ) {
            return string_format( "restricted to liquid item %s but not watertight\n",
                                  it->get_id().str() );
        }
        if( it->phase == phase_id::GAS && !airtight ) {
            return string_format( "restricted to gas item %s but not airtight\n",
                                  it->get_id().str() );
        }
    }
    for( const std::pair<const ammotype, int> &ammo_res : ammo_restriction ) {
        const ammotype &at = ammo_res.first;
        if( !at.is_valid() ) {
            return string_format( "invalid ammotype %s", at.str() );
        }
        const itype_id &it = at->default_ammotype();
        if( it->phase == phase_id::LIQUID && !watertight ) {
            return string_format( "restricted to liquid item %s but not watertight\n",
                                  it->get_id().str() );
        }
        if( it->phase == phase_id::GAS && !airtight ) {
            return string_format( "restricted to gas item %s but not airtight\n",
                                  it->get_id().str() );
        }
    }
    if( max_contains_volume() == 0_ml ) {
        return "has zero max volume\n";
    }
    if( magazine_well > 0_ml && rigid ) {
        return "rigid overrides magazine_well\n";
    }
    if( magazine_well >= max_contains_volume() ) {
        return string_format(
                   "magazine well (%s) larger than pocket volume (%s); "
                   "consider using rigid instead.\n",
                   quantity_to_string( magazine_well ),
                   quantity_to_string( max_contains_volume() ) );
    }
    if( max_item_volume && *max_item_volume < min_item_volume ) {
        return "max_item_volume is greater than min_item_volume.  no item can fit.\n";
    }
    if( ( watertight || airtight ) && min_item_volume > 0_ml ) {
        return "watertight or gastight is incompatible with min_item_volume\n";
    }
    return "";
}

void pocket_data::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "pocket_type", type, pocket_type::CONTAINER );
    optional( jo, was_loaded, "ammo_restriction", ammo_restriction );
    optional( jo, was_loaded, "flag_restriction", flag_restrictions );
    optional( jo, was_loaded, "item_restriction", item_id_restriction );
    optional( jo, was_loaded, "material_restriction", material_restriction );
    optional( jo, was_loaded, "allowed_speedloaders", allowed_speedloaders );
    optional( jo, was_loaded, "default_magazine", default_magazine );
    optional( jo, was_loaded, "description", description );
    optional( jo, was_loaded, "name", pocket_name );
    if( jo.has_member( "ammo_restriction" ) && ammo_restriction.empty() ) {
        jo.throw_error( "pocket defines empty ammo_restriction" );
    }
    if( jo.has_member( "flag_restrictions" ) && flag_restrictions.empty() ) {
        jo.throw_error( "pocket defines empty flag_restrictions" );
    }
    if( jo.has_member( "item_restriction" ) && item_id_restriction.empty() ) {
        jo.throw_error( "pocket defines empty item_restriction" );
    }
    if( !item_id_restriction.empty() ) {
        std::vector<itype_id> item_restriction;
        mandatory( jo, was_loaded, "item_restriction", item_restriction );
        default_magazine = item_restriction.front();
    }
    // ammo_restriction is a type of override, making the mandatory members not mandatory and superfluous
    // putting it in an if statement like this should allow for report_unvisited_member to work here
    if( ammo_restriction.empty() ) {
        optional( jo, was_loaded, "min_item_volume", min_item_volume, volume_reader(), 0_ml );
        units::volume temp = -1_ml;
        optional( jo, was_loaded, "max_item_volume", temp, volume_reader(), temp );
        if( temp != -1_ml ) {
            max_item_volume = temp;
        }
        optional( jo, was_loaded, "max_contains_volume", volume_capacity, volume_reader(),
                  max_volume_for_container );
        optional( jo, was_loaded, "max_contains_weight", max_contains_weight, mass_reader(),
                  max_weight_for_container );
        optional( jo, was_loaded, "max_item_length", max_item_length,
                  units::default_length_from_volume( volume_capacity ) * M_SQRT2 );
        optional( jo, was_loaded, "min_item_length", min_item_length );
        optional( jo, was_loaded, "extra_encumbrance", extra_encumbrance, 0 );
        optional( jo, was_loaded, "volume_encumber_modifier", volume_encumber_modifier, 1 );
        optional( jo, was_loaded, "ripoff", ripoff, 0 );
        optional( jo, was_loaded, "activity_noise", activity_noise );
    }
    optional( jo, was_loaded, "spoil_multiplier", spoil_multiplier, 1.0f );
    optional( jo, was_loaded, "weight_multiplier", weight_multiplier, 1.0f );
    optional( jo, was_loaded, "volume_multiplier", volume_multiplier, 1.0f );
    optional( jo, was_loaded, "magazine_well", magazine_well, volume_reader(), 0_ml );
    optional( jo, was_loaded, "moves", moves, 100 );
    optional( jo, was_loaded, "fire_protection", fire_protection, false );
    optional( jo, was_loaded, "watertight", watertight, false );
    optional( jo, was_loaded, "airtight", airtight, false );
    optional( jo, was_loaded, "open_container", open_container, false );
    optional( jo, was_loaded, "transparent", transparent, false );
    optional( jo, was_loaded, "rigid", rigid, false );
    optional( jo, was_loaded, "forbidden", forbidden, false );
    optional( jo, was_loaded, "holster", holster );
    optional( jo, was_loaded, "ablative", ablative );
    optional( jo, was_loaded, "inherits_flags", inherits_flags );
    // if ablative also flag as a holster so it only holds 1 item
    if( ablative ) {
        holster = true;
    }
    optional( jo, was_loaded, "sealed_data", sealed_data );
}

void sealable_data::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "spoil_multiplier", spoil_multiplier, 1.0f );
}

void pocket_noise::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "volume", volume, 0 );
    optional( jo, was_loaded, "chance", chance, 0 );
}

bool item_pocket::operator==( const item_pocket &rhs ) const
{
    return *data == *rhs.data;
}

bool pocket_data::operator==( const pocket_data &rhs ) const
{
    return rigid == rhs.rigid &&
           watertight == rhs.watertight &&
           airtight == rhs.airtight &&
           fire_protection == rhs.fire_protection &&
           get_flag_restrictions() == rhs.get_flag_restrictions() &&
           ammo_restriction == rhs.ammo_restriction &&
           item_id_restriction == rhs.item_id_restriction &&
           material_restriction == rhs.material_restriction &&
           type == rhs.type &&
           volume_capacity == rhs.volume_capacity &&
           min_item_volume == rhs.min_item_volume &&
           max_contains_weight == rhs.max_contains_weight &&
           spoil_multiplier == rhs.spoil_multiplier &&
           weight_multiplier == rhs.weight_multiplier &&
           moves == rhs.moves;
}

const pocket_data::FlagsSetType &pocket_data::get_flag_restrictions() const
{
    return flag_restrictions;
}

void pocket_data::add_flag_restriction( const flag_id &flag )
{
    flag_restrictions.insert( flag );
}

bool item_pocket::same_contents( const item_pocket &rhs ) const
{
    if( contents.size() != rhs.contents.size() ) {
        return false;
    }
    return std::equal( contents.begin(), contents.end(),
                       rhs.contents.begin(), rhs.contents.end(),
    []( const item & a, const item & b ) {
        return a.typeId() == b.typeId() &&
               a.charges == b.charges &&
               a.same_contents( b );
    } );
}

void item_pocket::restack()
{
    if( contents.size() <= 1 ) {
        return;
    }
    if( is_type( pocket_type::MAGAZINE ) ) {
        // Restack magazine contents in a way that preserves order of items
        for( auto iter = contents.begin(); iter != contents.end(); ) {
            if( !iter->count_by_charges() ) {
                continue;
            }

            auto next = std::next( iter, 1 );
            if( next == contents.end() ) {
                break;
            }

            if( iter->combine( *next ) ) {
                iter = contents.erase( next );
            } else {
                ++iter;
            }
        }
        return;
    }
    for( auto outer_iter = contents.begin(); outer_iter != contents.end(); ++outer_iter ) {
        if( !outer_iter->count_by_charges() ) {
            continue;
        }
        for( auto inner_iter = contents.begin(); inner_iter != contents.end(); ) {
            if( outer_iter == inner_iter || !inner_iter->count_by_charges() ) {
                ++inner_iter;
                continue;
            }
            if( outer_iter->combine( *inner_iter ) ) {
                inner_iter = contents.erase( inner_iter );
                outer_iter = contents.begin();
            } else {
                ++inner_iter;
            }
        }
    }
}

item *item_pocket::restack( /*const*/ item *it )
{
    item *ret = it;
    if( contents.size() <= 1 ) {
        return ret;
    }
    if( is_type( pocket_type::MAGAZINE ) ) {
        // Restack magazine contents in a way that preserves order of items
        for( auto iter = contents.begin(); iter != contents.end(); ) {
            if( !iter->count_by_charges() ) {
                continue;
            }

            auto next = std::next( iter, 1 );
            if( next == contents.end() ) {
                break;
            }

            if( iter->combine( *next ) ) {
                // next was placed in iter, check if next was the item that we track
                if( &( *next ) == ret ) {
                    ret = &( *iter );
                }
                iter = contents.erase( next );
            } else {
                ++iter;
            }
        }
        return ret;
    }
    for( auto outer_iter = contents.begin(); outer_iter != contents.end(); ++outer_iter ) {
        if( !outer_iter->count_by_charges() ) {
            continue;
        }
        for( auto inner_iter = contents.begin(); inner_iter != contents.end(); ) {
            if( outer_iter == inner_iter || !inner_iter->count_by_charges() ) {
                ++inner_iter;
                continue;
            }
            if( outer_iter->combine( *inner_iter ) ) {
                // inner was placed in outer, check if inner was the item that we track
                if( &( *inner_iter ) == ret ) {
                    ret = &( *outer_iter );
                }
                inner_iter = contents.erase( inner_iter );
                outer_iter = contents.begin();
            } else {
                ++inner_iter;
            }
        }
    }
    return ret;
}

bool item_pocket::has_item_stacks_with( const item &it ) const
{
    for( const item &inside : contents ) {
        if( it.stacks_with( inside ) ) {
            return true;
        }
    }
    return false;
}

bool item_pocket::better_pocket( const item_pocket &rhs, const item &it, bool nested ) const
{
    if( this->settings.priority() != rhs.settings.priority() ) {
        // Priority overrides all other factors.
        return rhs.settings.priority() > this->settings.priority();
    }

    // if we've somehow bypassed the pocket autoinsert rules, check them here
    if( !rhs.settings.accepts_item( it ) ) {
        return false;
    } else if( !this->settings.accepts_item( it ) ) {
        return true;
    }

    const bool rhs_whitelisted = !rhs.settings.get_item_whitelist().empty() ||
                                 !rhs.settings.get_category_whitelist().empty();
    const bool lhs_whitelisted = !this->settings.get_item_whitelist().empty() ||
                                 !this->settings.get_category_whitelist().empty();
    if( rhs_whitelisted != lhs_whitelisted ) {
        return rhs_whitelisted;
    }

    const bool rhs_it_stack = rhs.has_item_stacks_with( it );
    if( has_item_stacks_with( it ) != rhs_it_stack ) {
        return rhs_it_stack;
    }

    if( data->ammo_restriction.empty() != rhs.data->ammo_restriction.empty() ) {
        // pockets restricted by ammo should try to get filled first
        return !rhs.data->ammo_restriction.empty();
    }

    if( data->get_flag_restrictions().empty() != rhs.data->get_flag_restrictions().empty() ) {
        // pockets restricted by flag should try to get filled first
        return !rhs.data->get_flag_restrictions().empty();
    }

    if( it.is_comestible() && it.get_comestible()->spoils != 0_seconds ) {
        // a lower spoil multiplier is better
        return rhs.spoil_multiplier() < spoil_multiplier();
    }

    if( it.made_of( phase_id::SOLID ) && !it.is_frozen_liquid() ) {
        if( data->watertight != rhs.data->watertight ) {
            return !rhs.data->watertight;
        }
    }

    if( it.is_frozen_liquid() ) {
        if( data->watertight != rhs.data->watertight ) {
            return rhs.data->watertight;
        }
    }

    //Skip irrelevant properties of nested containers
    if( nested ) {
        return false;
    }

    if( rhs.data->extra_encumbrance < data->extra_encumbrance ) {
        // pockets with less extra encumbrance should be prioritized
        return true;
    }

    if( data->ripoff > rhs.data->ripoff ) {
        // pockets without ripoff chance should be prioritized
        return true;
    }

    if( data->rigid != rhs.data->rigid ) {
        // Prefer rigid containers, as their fixed volume is wasted unless filled
        return rhs.data->rigid;
    }

    if( remaining_volume() != rhs.remaining_volume() ) {
        // we want the least amount of remaining volume
        return rhs.remaining_volume() < remaining_volume();
    }

    return rhs.obtain_cost( it ) < obtain_cost( it );
}

bool item_pocket::stacks_with( const item_pocket &rhs, int depth, int maxdepth ) const
{
    if( _sealed != rhs._sealed ) {
        return false;
    }
    return ( empty() && rhs.empty() ) || std::equal( contents.begin(), contents.end(),
            rhs.contents.begin(), rhs.contents.end(),
    [depth, maxdepth]( const item & a, const item & b ) {
        return depth < maxdepth && a.charges == b.charges &&
               a.stacks_with( b, false, false, depth + 1, maxdepth );
    } );
}

bool item_pocket::is_funnel_container( units::volume &bigger_than ) const
{
    // should not be static, since after reloading some members of the item objects,
    // such as item types, may be invalidated.
    const std::vector<item> allowed_liquids{
        item( "water", calendar::turn_zero, 1 ),
    };
    if( !data->watertight ) {
        return false;
    }
    if( sealable() && _sealed ) {
        return false;
    }
    for( const item &liquid : allowed_liquids ) {
        if( can_contain( liquid ).success() ) {
            bigger_than = remaining_volume();
            return true;
        }
    }
    return false;
}

bool item_pocket::is_restricted() const
{
    return !data->get_flag_restrictions().empty() || !data->ammo_restriction.empty();
}

std::list<item *> item_pocket::all_items_top()
{
    std::list<item *> items;
    for( item &it : contents ) {
        items.push_back( &it );
    }
    return items;
}

std::list<const item *> item_pocket::all_items_top() const
{
    std::list<const item *> items;
    for( const item &it : contents ) {
        items.push_back( &it );
    }
    return items;
}

std::list<item *> item_pocket::all_items_ptr( pocket_type pk_type )
{
    if( !is_type( pk_type ) ) {
        return std::list<item *>();
    }
    std::list<item *> all_items_top_level{ all_items_top() };
    for( item *it : all_items_top_level ) {
        std::list<item *> all_items_internal{ it->all_items_ptr( pk_type ) };
        all_items_top_level.insert( all_items_top_level.end(), all_items_internal.begin(),
                                    all_items_internal.end() );
    }
    return all_items_top_level;
}

std::list<const item *> item_pocket::all_items_ptr( pocket_type pk_type ) const
{
    if( !is_type( pk_type ) ) {
        return std::list<const item *>();
    }
    std::list<const item *> all_items_top_level{ all_items_top() };
    for( const item *it : all_items_top_level ) {
        std::list<const item *> all_items_internal{ it->all_items_ptr( pk_type ) };
        all_items_top_level.insert( all_items_top_level.end(), all_items_internal.begin(),
                                    all_items_internal.end() );
    }
    return all_items_top_level;
}

bool item_pocket::has_any_with( const std::function<bool( const item &it )> &filter ) const
{
    return std::any_of( contents.begin(), contents.end(), filter );
}

item &item_pocket::back()
{
    return contents.back();
}

const item &item_pocket::back() const
{
    return contents.back();
}

item &item_pocket::front()
{
    return contents.front();
}

const item &item_pocket::front() const
{
    return contents.front();
}

void item_pocket::pop_back()
{
    contents.pop_back();
}

size_t item_pocket::size() const
{
    return contents.size();
}

units::volume item_pocket::volume_capacity() const
{
    return data->volume_capacity;
}

units::volume item_pocket::magazine_well() const
{
    return data->magazine_well;
}

units::mass item_pocket::weight_capacity() const
{
    return data->max_contains_weight;
}

units::volume item_pocket::max_contains_volume() const
{
    return data->max_contains_volume();
}

units::volume item_pocket::remaining_volume() const
{
    return volume_capacity() - contains_volume();
}

int item_pocket::remaining_capacity_for_item( const item &it ) const
{
    if( it.count_by_charges() ) {
        item it_copy = it;
        it_copy.charges = 1;
        if( can_contain( it_copy ).success() ) {
            return std::min( {
                it.charges,
                charges_per_remaining_volume( it ),
                charges_per_remaining_weight( it )
            } );
        } else {
            return 0;
        }
    } else if( can_contain( it ).success() ) {
        return std::min( {
            charges_per_remaining_volume( it ),
            charges_per_remaining_weight( it )
        } );
    } else {
        return 0;
    }
}

units::volume item_pocket::item_size_modifier() const
{
    if( data->rigid ) {
        return 0_ml;
    }
    units::volume total_vol = 0_ml;
    for( const item &it : contents ) {
        total_vol += it.volume( is_type( pocket_type::MOD ) );
    }
    total_vol -= data->magazine_well;
    total_vol *= data->volume_multiplier;
    return std::max( 0_ml, total_vol );
}

units::mass item_pocket::item_weight_modifier() const
{
    units::mass total_mass = 0_gram;
    for( const item &it : contents ) {
        if( is_type( pocket_type::MOD ) ) {
            total_mass += it.weight( true, true ) * data->weight_multiplier;
        } else {
            total_mass += it.weight() * data->weight_multiplier;
        }
    }
    return total_mass;
}

units::length item_pocket::item_length_modifier() const
{
    if( is_type( pocket_type::EBOOK ) ) {
        return 0_mm;
    }
    units::length total_length = 0_mm;
    for( const item &it : contents ) {
        total_length = std::max( static_cast<units::length>( it.length() * std::cbrt(
                                     data->volume_multiplier ) ),
                                 total_length );
    }
    return total_length;
}

float item_pocket::spoil_multiplier() const
{
    if( sealed() ) {
        return data->sealed_data->spoil_multiplier;
    } else {
        return data->spoil_multiplier;
    }
}

int item_pocket::moves() const
{
    if( data ) {
        return data->moves;
    } else {
        return -1;
    }
}

std::vector<item *> item_pocket::gunmods()
{
    std::vector<item *> mods;
    for( item &it : contents ) {
        if( it.is_gunmod() ) {
            mods.push_back( &it );
        }
    }
    return mods;
}

std::vector<const item *> item_pocket::gunmods() const
{
    std::vector<const item *> mods;
    for( const item &it : contents ) {
        if( it.is_gunmod() ) {
            mods.push_back( &it );
        }
    }
    return mods;
}

cata::flat_set<itype_id> item_pocket::item_type_restrictions() const
{
    return data->item_id_restriction;
}

item *item_pocket::magazine_current()
{
    auto iter = std::find_if( contents.begin(), contents.end(), []( const item & it ) {
        return !it.is_null();
    } );
    return iter != contents.end() ? &*iter : nullptr;
}

itype_id item_pocket::magazine_default() const
{
    if( is_type( pocket_type::MAGAZINE_WELL ) ) {
        return data->default_magazine;
    }
    return itype_id::NULL_ID();
}

int item_pocket::ammo_consume( int qty )
{
    int need = qty;
    int used = 0;
    std::list<item>::iterator it;
    for( it = contents.begin(); it != contents.end(); ) {
        if( it->has_flag( flag_CASING ) ) {
            it++;
            continue;
        }
        if( need >= it->charges ) {
            need -= it->charges;
            used += it->charges;
            it = contents.erase( it );
        } else {
            it->charges -= need;
            used += need;
            break;
        }
    }
    return used;
}

int item_pocket::ammo_capacity( const ammotype &ammo ) const
{
    const auto found_ammo = data->ammo_restriction.find( ammo );
    if( found_ammo == data->ammo_restriction.end() ) {
        return 0;
    } else {
        return found_ammo->second;
    }
}

int item_pocket::remaining_ammo_capacity( const ammotype &ammo ) const
{
    int total_capacity = ammo_capacity( ammo );
    if( total_capacity == 0 ) {
        return 0;
    }
    int ammo_count = 0;
    for( const item &it : contents ) {
        if( it.has_flag( flag_CASING ) ) {
            continue;
        } else if( ammo != it.ammo_type() ) {
            return 0;
        }
        ammo_count += it.count();
    }
    return total_capacity - ammo_count;
}

std::set<ammotype> item_pocket::ammo_types() const
{
    std::set<ammotype> ret;
    for( const std::pair<const ammotype, int> &type_pair : data->ammo_restriction ) {
        ret.emplace( type_pair.first );
    }
    return ret;
}

void item_pocket::casings_handle( const std::function<bool( item & )> &func )
{
    for( auto it = contents.begin(); it != contents.end(); ) {
        if( it->has_flag( flag_CASING ) ) {
            it->unset_flag( flag_CASING );
            if( func( *it ) ) {
                it = contents.erase( it );
                continue;
            }
            // didn't handle the casing so reset the flag ready for next call
            it->set_flag( flag_CASING );
        }
        ++it;
    }
}

void item_pocket::handle_liquid_or_spill( Character &guy, const item *avoid )
{
    if( guy.is_npc() ) {
        spill_contents( guy.pos() );
        return;
    }

    for( auto iter = contents.begin(); iter != contents.end(); ) {
        if( iter->made_of( phase_id::LIQUID ) ) {
            while( iter->charges > 0 && liquid_handler::handle_liquid( *iter, avoid, 1 ) ) {
                // query until completely handled or explicitly canceled
            }
            if( iter->charges == 0 ) {
                iter = contents.erase( iter );
            } else {
                return;
            }
        } else {
            item i_copy( *iter );
            guy.i_add_or_drop( i_copy, 1, avoid, &*iter );
            iter = contents.erase( iter );
        }
    }
}

bool item_pocket::use_amount( const itype_id &it, int &quantity, std::list<item> &used )
{
    bool used_item = false;
    for( auto a = contents.begin(); a != contents.end() && quantity > 0; ) {
        if( a->use_amount( it, quantity, used ) ) {
            used_item = true;
            a = contents.erase( a );
        } else {
            ++a;
        }
    }
    restack();
    return used_item;
}

bool item_pocket::will_explode_in_a_fire() const
{
    if( data->fire_protection ) {
        return false;
    }
    return std::any_of( contents.begin(), contents.end(), []( const item & it ) {
        return it.will_explode_in_fire();
    } );
}

bool item_pocket::will_spill() const
{
    if( sealed() ) {
        return false;
    } else {
        return data->open_container;
    }
}

bool item_pocket::will_spill_if_unsealed() const
{
    return data->open_container;
}

bool item_pocket::seal()
{
    if( !sealable() || empty() ) {
        return false;
    }
    _sealed = true;
    return true;
}

void item_pocket::unseal()
{
    _sealed = false;
}

bool item_pocket::sealed() const
{
    if( !sealable() ) {
        return false;
    } else {
        return _sealed;
    }
}
bool item_pocket::sealable() const
{
    // if it has sealed data it is understood that the
    // data is different when sealed than when not
    // In other words, a pocket is sealable IFF it has sealed_data.
    return !!data->sealed_data;
}

std::string item_pocket::translated_sealed_prefix() const
{
    if( sealed() ) {
        return _( "sealed" );
    } else {
        return _( "open" );
    }
}

bool item_pocket::detonate( const tripoint &pos, std::vector<item> &drops )
{
    const auto new_end = std::remove_if( contents.begin(), contents.end(), [&pos, &drops]( item & it ) {
        return it.detonate( pos, drops );
    } );
    if( new_end != contents.end() ) {
        contents.erase( new_end, contents.end() );
        // If any of the contents explodes, so does the container
        return true;
    }
    return false;
}

bool item_pocket::process( const itype &type, map &here, Character *carrier, const tripoint &pos,
                           float insulation, const temperature_flag flag )
{
    bool processed = false;
    float spoil_multiplier = 1.0f;
    for( auto it = contents.begin(); it != contents.end(); ) {
        if( _sealed ) {
            spoil_multiplier = 0.0f;
        }
        if( it->process( here, carrier, pos, type.insulation_factor * insulation, flag,
                         spoil_multiplier ) ) {
            it->spill_contents( pos );
            it = contents.erase( it );
            processed = true;
        } else {
            ++it;
        }
    }
    return processed;
}

void item_pocket::remove_all_ammo( Character &guy )
{
    for( auto iter = contents.begin(); iter != contents.end(); ) {
        if( iter->is_irremovable() ) {
            iter++;
            continue;
        }
        drop_or_handle( *iter, guy );
        iter = contents.erase( iter );
    }
}

void item_pocket::remove_all_mods( Character &guy )
{
    for( auto iter = contents.begin(); iter != contents.end(); ) {
        if( iter->is_toolmod() ) {
            guy.i_add_or_drop( *iter );
            iter = contents.erase( iter );
        } else {
            ++iter;
        }
    }
    on_contents_changed();
}

void item_pocket::set_item_defaults()
{
    for( item &contained_item : contents ) {
        /* for guns and other items defined to have a magazine but don't use "ammo" */
        if( contained_item.is_magazine() ) {
            contained_item.ammo_set(
                contained_item.ammo_default(),
                contained_item.ammo_capacity( item_controller->find_template(
                                                  contained_item.ammo_default() )->ammo->type ) / 2
            );
        } else { //Contents are batteries or food
            contained_item.charges =
                item::find_type( contained_item.typeId() )->charges_default();
        }
    }
}

static void insert_separation_line( std::vector<iteminfo> &info )
{
    if( info.empty() || info.back().sName != "--" ) {
        info.emplace_back( "DESCRIPTION", "--" );
    }
}

void item_pocket::general_info( std::vector<iteminfo> &info, int pocket_number,
                                bool disp_pocket_number ) const
{
    if( disp_pocket_number ) {
        const std::string pocket_num = string_format( _( "Pocket %d:" ), pocket_number );
        info.emplace_back( "DESCRIPTION", pocket_num );
    }

    if( !get_description().empty() ) {
        info.emplace_back( "DESCRIPTION", string_format( "<info>%s</info>",
                           get_description().translated() ) );
    } else if( name_as_description ) {
        // fallback to the original items name as a description
        info.emplace_back( "DESCRIPTION", string_format( "<info>%s</info>",
                           get_name().translated() ) );
    }

    // NOLINTNEXTLINE(cata-translate-string-literal)
    const std::string cont_type_str = string_format( "{%d}CONTAINER", pocket_number );
    // NOLINTNEXTLINE(cata-translate-string-literal)
    const std::string base_type_str = string_format( "{%d}BASE", pocket_number );

    // Show volume/weight for normal containers, or ammo capacity if ammo_restriction is defined if its an ablative pocket don't display any info
    if( is_ablative() ) {
        // its an ablative pocket volume and weight maxes don't matter
        info.emplace_back( "DESCRIPTION",
                           _( "Holds a <info>single armored plate</info>." ) );
    } else if( data->ammo_restriction.empty() ) {
        info.push_back( vol_to_info( cont_type_str, _( "Volume: " ),
                                     volume_capacity(), 2, false ) );
        info.push_back( weight_to_info( cont_type_str, _( "  Weight: " ),
                                        weight_capacity(), 2, false ) );
        info.back().bNewLine = true;
    } else {
        std::vector<std::pair<std::string, int>> fmts;
        for( const ammotype &at : ammo_types() ) {
            int capacity = ammo_capacity( at );
            fmts.emplace_back( string_format( n_gettext( "<num> round of %s",
                                              "<num> rounds of %s", capacity ),
                                              at->name() ),
                               capacity );
        }
        std::sort( fmts.begin(), fmts.end(), localized_compare );
        for( const std::pair<std::string, int> &fmt : fmts ) {
            // NOLINTNEXTLINE(cata-translate-string-literal)
            info.emplace_back( string_format( "{%d}MAGAZINE", pocket_number ), _( "Holds: " ), fmt.first,
                               iteminfo::no_flags,
                               fmt.second );
        }
    }

    if( data->max_item_length != 0_mm && !is_ablative() ) {
        info.back().bNewLine = true;
        // if the item has no minimum length then keep it in the same units as max
        if( data->min_item_length == 0_mm ) {
            info.emplace_back( base_type_str, _( "Item length: " ),
                               string_format( "<num> %s", length_units( data->max_item_length ) ),
                               iteminfo::no_newline,
                               0 );
        } else {
            info.emplace_back( base_type_str, _( "Item length: " ),
                               string_format( "<num> %s", length_units( data->min_item_length ) ),
                               iteminfo::no_newline,
                               convert_length( data->min_item_length ), data->min_item_length.value() );
        }
        info.emplace_back( base_type_str, _( " to " ),
                           string_format( "<num> %s", length_units( data->max_item_length ) ),
                           iteminfo::no_flags,
                           convert_length( data->max_item_length ), data->max_item_length.value() );
    } else if( data->min_item_length > 0_mm && !is_ablative() ) {
        info.back().bNewLine = true;
        info.emplace_back( base_type_str, _( "Minimum item length: " ),
                           string_format( "<num> %s", length_units( data->min_item_length ) ),
                           iteminfo::no_flags,
                           convert_length( data->min_item_length ), data->min_item_length.value() );
    }

    if( data->min_item_volume > 0_ml ) {
        std::string fmt = string_format( "<num> %s", volume_units_abbr() );
        info.emplace_back( base_type_str, _( "Minimum item volume: " ), fmt,
                           iteminfo::lower_is_better | iteminfo::is_three_decimal,
                           convert_volume( data->min_item_volume.value(), nullptr ), data->min_item_volume.value() );
    }

    if( data->max_item_volume ) {
        std::string fmt = string_format( "<num> %s", volume_units_abbr() );
        info.emplace_back( base_type_str, _( "Maximum item volume: " ), fmt,
                           iteminfo::is_three_decimal, convert_volume( data->max_item_volume.value().value(), nullptr ),
                           data->max_item_volume.value().value() );
    }

    info.emplace_back( base_type_str, _( "Base moves to remove item: " ),
                       "<num>", iteminfo::lower_is_better, data->moves );

    if( !data->material_restriction.empty() ) {
        std::string materials;
        bool first = true;
        for( material_id mat : data->material_restriction ) {
            if( first ) {
                materials += mat.obj().name();
                first = false;
            } else {
                materials += ", " + mat.obj().name();
            }
        }
        info.emplace_back( "DESCRIPTION", string_format( _( "Allowed materials: %s" ), materials ) );
    }

    if( data->rigid ) {
        info.emplace_back( "DESCRIPTION", _( "This pocket is <info>rigid</info>." ) );
    }

    if( data->holster ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This is a <info>holster</info>, it only holds <info>one item at a time</info>." ) );
    }

    if( data->extra_encumbrance > 0 ) {
        info.emplace_back( "DESCRIPTION",
                           string_format( _( "Causes %d <bad>additional encumbrance</bad> while in use." ),
                                          data->extra_encumbrance ) ) ;
    }

    if( data->ripoff > 0 ) {
        info.emplace_back( "DESCRIPTION",
                           _( "Items may <bad>fall out</bad> if grabbed." ) );
    }

    if( data->activity_noise.chance > 0 ) {
        info.emplace_back( "DESCRIPTION",
                           _( "Items will <bad>make noise</bad> when you move." ) );
    }

    if( data->watertight ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This pocket can <info>contain a liquid</info>." ) );
    }
    if( data->airtight ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This pocket can <info>contain a gas</info>." ) );
    }
    if( will_spill() ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This pocket will <bad>spill</bad> if placed into another item or worn." ) );
    }
    if( data->fire_protection ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This pocket <info>protects its contents from fire</info>." ) );
    }
    if( spoil_multiplier() != 1.0f ) {
        if( spoil_multiplier() != 0.0f ) {
            info.emplace_back( "DESCRIPTION",
                               string_format( _( "Contained items spoil at <neutral>%.0f%%</neutral> their original rate." ),
                                              spoil_multiplier() * 100 ) );
        } else {
            info.emplace_back( "DESCRIPTION", _( "Contained items <info>won't spoil</info>." ) );
        }
    }
    if( data->weight_multiplier != 1.0f ) {
        info.emplace_back( "DESCRIPTION",
                           string_format( _( "Items in this pocket weigh <neutral>%.0f%%</neutral> their original weight." ),
                                          data->weight_multiplier * 100 ) );
    }
    if( data->volume_multiplier != 1.0f ) {
        info.emplace_back( "DESCRIPTION",
                           string_format(
                               _( "This pocket expands at <neutral>%.0f%%</neutral> of the rate of volume of items inside." ),
                               data->volume_multiplier * 100 ) );
    }

    // Display flags items need to be stored in this pocket
    if( !data->get_flag_restrictions().empty() ) {
        info.emplace_back( "DESCRIPTION", _( "<bold>Restrictions</bold>:" ) );
        bool first = true;
        for( const flag_id &e : data->get_flag_restrictions() ) {
            const json_flag &f = *e;
            if( !f.restriction().empty() ) {
                if( first ) {
                    info.emplace_back( "DESCRIPTION", string_format( "* %s", f.restriction() ) );
                    first = false;
                } else {
                    info.emplace_back( "DESCRIPTION", string_format( _( "* <bold>or</bold> %s" ), f.restriction() ) );
                }
            }
        }
    }
    if( !no_rigid.empty() ) {
        std::vector<sub_bodypart_id> no_rigid_vec( no_rigid.begin(), no_rigid.end() );
        std::set<translation, localized_comparator> to_print = body_part_type::consolidate( no_rigid_vec );
        std::string bps = enumerate_as_string( to_print.begin(),
        to_print.end(), []( const translation & t ) {
            return t.translated();
        } );
        info.emplace_back( "DESCRIPTION", string_format( _( "<bold>Can't put hard armor on: %s</bold>:" ),
                           bps ) );
    }
}

void item_pocket::contents_info( std::vector<iteminfo> &info, int pocket_number,
                                 bool disp_pocket_number ) const
{
    if( is_forbidden() ) {
        return;
    }
    const std::string space = "  ";

    insert_separation_line( info );
    if( disp_pocket_number ) {
        if( sealable() ) {
            info.emplace_back( "DESCRIPTION", string_format( _( "<bold>%s pocket %d</bold>" ),
                               translated_sealed_prefix(),
                               pocket_number ) );
        } else if( is_ablative() && !contents.empty() ) {
            info.emplace_back( "DESCRIPTION", string_format( _( "<bold>Pocket %d</bold>: %s" ),
                               pocket_number, contents.front().display_name() ) );
        } else {
            info.emplace_back( "DESCRIPTION", string_format( _( "<bold>Pocket %d</bold>" ),
                               pocket_number ) );
        }
    }
    if( contents.empty() ) {
        info.emplace_back( "DESCRIPTION", _( "This pocket is empty." ) );
        return;
    }
    if( sealed() ) {
        info.emplace_back( "DESCRIPTION", _( "This pocket is <info>sealed</info>." ) );
    }

    // NOLINTNEXTLINE(cata-translate-string-literal)
    const std::string cont_type_str = string_format( "{%d}CONTAINER", pocket_number );
    // NOLINTNEXTLINE(cata-translate-string-literal)
    const std::string arm_type_str = string_format( "{%d}ARMOR", pocket_number );

    if( is_ablative() ) {
        // if we have contents for an ablative pocket display the armor data
        if( !contents.empty() ) {
            const item &ablative_armor = contents.front();
            info.emplace_back( arm_type_str, string_format( "%s%s", _( "Coverage:" ), space ), "",
                               iteminfo::no_newline,
                               ablative_armor.get_avg_coverage() );
            //~ (M)elee coverage
            info.emplace_back( arm_type_str, string_format( "%s%s%s", space, _( "(M):" ), space ), "",
                               iteminfo::no_newline,
                               ablative_armor.get_avg_coverage( item::cover_type::COVER_MELEE ) );
            //~ (R)anged coverage
            info.emplace_back( arm_type_str, string_format( "%s%s%s", space, _( "(R):" ), space ), "",
                               iteminfo::no_newline,
                               ablative_armor.get_avg_coverage( item::cover_type::COVER_RANGED ) );
            //~ (V)itals coverage
            info.emplace_back( arm_type_str, string_format( "%s%s%s", space, _( "(V):" ), space ), "",
                               iteminfo::no_flags,
                               ablative_armor.get_avg_coverage( item::cover_type::COVER_VITALS ) );

            info.back().bNewLine = true;

            size_t idx = 0;
            const std::vector<damage_info_order> &all_ablate = damage_info_order::get_all(
                        damage_info_order::info_type::ABLATE );
            for( const damage_info_order &dio : all_ablate ) {
                std::string label = string_format( idx == 0 ? _( "<bold>Protection</bold>: %s: " ) :
                                                   pgettext( "protection info", " %s: " ),
                                                   uppercase_first_letter( dio.dmg_type->name.translated() ) );
                iteminfo::flags flgs = idx == all_ablate.size() - 1 ?
                                       iteminfo::is_decimal : iteminfo::no_newline | iteminfo::is_decimal;
                info.emplace_back( arm_type_str, label, "", flgs, ablative_armor.resist( dio.dmg_type ) );
                idx++;
            }
        }
    } else if( data->ammo_restriction.empty() ) {
        // With no ammo_restriction defined, show current volume/weight, and total capacity
        info.emplace_back( vol_to_info( cont_type_str, _( "Volume: " ),
                                        contains_volume() ) );
        info.emplace_back( vol_to_info( cont_type_str, _( " of " ),
                                        volume_capacity() ) );
        info.back().bNewLine = true;
        info.emplace_back( weight_to_info( cont_type_str, _( "Weight: " ),
                                           contains_weight() ) );
        info.emplace_back( weight_to_info( cont_type_str, _( " of " ),
                                           weight_capacity() ) );
    } else {
        // With ammo_restriction, total capacity does not matter, but show current volume/weight
        info.emplace_back( vol_to_info( cont_type_str, _( "Volume: " ),
                                        contains_volume() ) );
        info.back().bNewLine = true;
        info.emplace_back( weight_to_info( cont_type_str, _( "Weight: " ),
                                           contains_weight() ) );
    }

    if( remaining_volume() < 0_ml || remaining_weight() < 0_gram ) {
        info.emplace_back( "DESCRIPTION",
                           colorize( _( "This pocket is over capacity and will spill if moved!" ), c_red ) );
    }

    // ablative pockets have their contents displayed earlier in the UI
    if( !is_ablative() ) {
        std::vector<std::pair<item const *, int>> counted_contents;
        bool contents_header = false;
        for( const item &contents_item : contents ) {
            if( !contents_header ) {
                info.emplace_back( "DESCRIPTION", _( "<bold>Contents of this pocket</bold>:" ) );
                contents_header = true;
            }

            const translation &desc = contents_item.type->description;

            if( contents_item.made_of_from_type( phase_id::LIQUID ) ) {
                info.emplace_back( "DESCRIPTION", colorize( space + contents_item.display_name(),
                                   contents_item.color_in_inventory() ) );
                info.emplace_back( vol_to_info( cont_type_str, desc + space, contents_item.volume() ) );
            } else {
                bool found = false;
                for( std::pair<item const *, int> &content : counted_contents ) {
                    if( content.first->display_stacked_with( contents_item ) ) {
                        content.second += 1;
                        found = true;
                    }
                }
                if( !found ) {
                    std::pair<item const *, int> new_content( &contents_item, 1 );
                    counted_contents.push_back( new_content );
                }
            }
        }
        for( std::pair<item const *, int> content : counted_contents ) {
            if( content.second > 1 ) {
                info.emplace_back( "DESCRIPTION",
                                   space + std::to_string( content.second ) + " " + colorize( content.first->display_name(
                                               content.second ), content.first->color_in_inventory() ) );
            } else {
                info.emplace_back( "DESCRIPTION", space + colorize( content.first->display_name(),
                                   content.first->color_in_inventory() ) );
            }
        }
    }
}

void item_pocket::favorite_info( std::vector<iteminfo> &info ) const
{
    settings.info( info );
}

// for soft containers check all content items to see if they all would fit
static int charges_per_volume_recursive( const units::volume &max_item_volume,
        const item &it )
{
    int min_charges = item::INFINITE_CHARGES;

    if( !it.is_soft() ) {
        return it.charges_per_volume( max_item_volume );
    }

    for( const item *content : it.all_items_top() ) {
        min_charges = std::min( min_charges, charges_per_volume_recursive( max_item_volume, *content ) );
        if( min_charges == 0 ) {
            return 0;
        }
    }

    return min_charges;
}

ret_val<item_pocket::contain_code> item_pocket::is_compatible( const item &it ) const
{
    if( data->type == pocket_type::MIGRATION ) {
        // migration pockets need to always succeed
        return ret_val<item_pocket::contain_code>::make_success();
    }

    if( data->type == pocket_type::MOD ) {
        if( it.is_toolmod() || it.is_gunmod() ) {
            return ret_val<item_pocket::contain_code>::make_success();
        } else {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_MOD, _( "only mods can go into mod pocket" ) );
        }
    }

    if( data->type == pocket_type::CORPSE ) {
        // corpses can't have items stored in them the normal way,
        // we simply don't want them to "spill"
        return ret_val<item_pocket::contain_code>::make_success();
    }

    if( data->type == pocket_type::SOFTWARE ) {
        if( it.has_flag( flag_NO_DROP ) && it.has_flag( flag_IRREMOVABLE ) ) {
            return ret_val<item_pocket::contain_code>::make_success();
        } else {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_MOD, _( "only immaterial items can go into software pocket" ) );
        }
    }

    if( data->type == pocket_type::EBOOK ) {
        if( it.is_book() ) {
            return ret_val<item_pocket::contain_code>::make_success();
        } else {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_MOD, _( "only books can go into ebook pocket" ) );
        }
    }

    if( data->type == pocket_type::CABLE ) {
        if( it.has_flag( flag_CABLE_SPOOL ) ) {
            return ret_val<item_pocket::contain_code>::make_success();
        } else {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_MOD, _( "only cables can go into cable pocket" ) );
        }
    }

    if( it.has_flag( flag_NO_UNWIELD ) ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_MOD, _( "cannot unwield item" ) );
    }

    if( !data->item_id_restriction.empty() || !data->get_flag_restrictions().empty() ||
        !data->material_restriction.empty() ) {
        if( data->item_id_restriction.count( it.typeId() ) == 0 &&
            !it.has_any_flag( data->get_flag_restrictions() ) &&
            !data->material_restriction.count( it.get_base_material().id ) ) {
            return ret_val<item_pocket::contain_code>::make_failure( contain_code::ERR_FLAG,
                    _( "holster does not accept this item type or form factor" ) );
        }
    }

    // ammo restriction overrides item volume/weight and watertight/airtight data
    if( !data->ammo_restriction.empty() ) {
        if( !it.is_ammo() ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_AMMO, _( "item is not ammunition" ) );
        }

        const auto ammo_restriction_iter = data->ammo_restriction.find( it.ammo_type() );

        if( ammo_restriction_iter == data->ammo_restriction.end() ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_AMMO, _( "item is not the correct ammo type" ) );
        }

        return ret_val<item_pocket::contain_code>::make_success();
    }

    if( it.made_of( phase_id::LIQUID ) ) {
        if( !data->watertight && !it.has_flag( flag_FROM_FROZEN_LIQUID ) ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_LIQUID, _( "can't contain liquid" ) );
        }
    }
    if( it.made_of( phase_id::GAS ) ) {
        if( !data->airtight ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_GAS, _( "can't contain gas" ) );
        }
    }

    // liquids and gases avoid the size limit altogether
    // soft items also avoid the size limit
    // count_by_charges items check volume of single charge
    if( !it.made_of( phase_id::LIQUID ) && !it.made_of( phase_id::GAS ) &&
        !it.is_frozen_liquid() && data->max_item_volume &&
        !charges_per_volume_recursive( *data->max_item_volume, it ) ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_BIG, _( "item is too big" ) );
    }
    if( it.length() > data->max_item_length ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_BIG, _( "item is too long" ) );
    }

    if( it.length() < data->min_item_length ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_SMALL, _( "item is too short" ) );
    }

    if( it.volume() < data->min_item_volume ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_SMALL, _( "item is too small" ) );
    }
    return ret_val<item_pocket::contain_code>::make_success();
}

ret_val<item_pocket::contain_code> item_pocket::can_contain( const item &it,
        int &copies_remaining, bool ignore_contents ) const
{
    return _can_contain( it, copies_remaining, ignore_contents );
}

ret_val<item_pocket::contain_code> item_pocket::can_contain( const item &it,
        bool ignore_contents ) const
{
    int copies = 1;
    return _can_contain( it, copies, ignore_contents );
}

ret_val<item_pocket::contain_code> item_pocket::_can_contain( const item &it,
        int &copies_remaining, const bool ignore_contents ) const
{
    ret_val<item_pocket::contain_code> compatible = is_compatible( it );

    if( copies_remaining <= 0 ) {
        return ret_val<item_pocket::contain_code>::make_success();
    }
    // To prevent debugmsg. Casings can only be inserted in a magazine during firing.
    if( data->type == pocket_type::MAGAZINE && it.has_flag( flag_CASING ) ) {
        copies_remaining = 0;
        return ret_val<item_pocket::contain_code>::make_success();
    }

    if( !compatible.success() ) {
        return compatible;
    }
    if( !is_standard_type() ) {
        copies_remaining = 0;
        return ret_val<item_pocket::contain_code>::make_success();
    }

    if( it.made_of( phase_id::LIQUID ) ) {
        if( size() != 0 && !contents.front().can_combine( it ) && data->watertight ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_LIQUID, _( "can't mix liquid with contained item" ) );
        }
    } else if( size() == 1 && !it.is_frozen_liquid() &&
               contents.front().made_of( phase_id::LIQUID ) && data->watertight ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_LIQUID, _( "can't put non liquid into pocket with liquid" ) );
    }

    if( it.is_frozen_liquid() ) {
        if( size() != 0 && !contents.front().can_combine( it ) && data->watertight ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_LIQUID,
                       _( "can't mix frozen liquid with contained item in the watertight container" ) );
        }
    } else if( data->watertight ) {
        if( size() == 1 && contents.front().is_frozen_liquid() && !contents.front().can_combine( it ) ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_LIQUID,
                       _( "can't mix item with contained frozen liquid in the watertight container" ) );
        }
    }

    if( it.made_of( phase_id::GAS ) ) {
        if( size() != 0 && !contents.front().can_combine( it ) ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_GAS, _( "can't mix gas with contained item" ) );
        }
    } else if( size() == 1 && contents.front().made_of( phase_id::GAS ) ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_GAS, _( "can't put non gas into pocket with gas" ) );
    }

    if( ignore_contents ) {
        // Skip all the checks against other pocket contents.
        if( it.weight() > weight_capacity() ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_TOO_HEAVY, _( "item is too heavy" ) );
        }
        if( it.volume() > volume_capacity() ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_TOO_BIG, _( "item is too big" ) );
        }
        return ret_val<item_pocket::contain_code>::make_success();
    }

    if( data->type == pocket_type::MAGAZINE && !empty() ) {
        for( const item &contained : contents ) {
            if( contained.has_flag( flag_CASING ) ) {
                continue;
            } else if( contained.ammo_type() != it.ammo_type() ) {
                return ret_val<item_pocket::contain_code>::make_failure(
                           contain_code::ERR_NO_SPACE, _( "can't mix different ammo" ) );
            }
        }
    }

    if( data->ablative ) {
        if( !contents.empty() ) {
            if( contents.front().can_combine( it ) ) {
                // Only items with charges can succeed here.
                return ret_val<item_pocket::contain_code>::make_success();
            } else {
                return ret_val<item_pocket::contain_code>::make_failure(
                           contain_code::ERR_NO_SPACE, _( "ablative pocket already contains a plate" ) );
            }
        }

        if( it.is_rigid() ) {
            for( const sub_bodypart_id &sbp : it.get_covered_sub_body_parts() ) {
                if( it.is_bp_rigid( sbp ) && std::count( no_rigid.begin(), no_rigid.end(), sbp ) != 0 ) {
                    return ret_val<item_pocket::contain_code>::make_failure(
                               contain_code::ERR_NO_SPACE,
                               _( "ablative pocket is being worn with hard armor can't support hard plate" ) );
                }
            }
        }
        copies_remaining = std::max( 0, copies_remaining - 1 );
        return ret_val<item_pocket::contain_code>::make_success();
    }

    if( data->holster && !contents.empty() ) {
        if( contents.front().can_combine( it ) ) {
            // Only items with charges can succeed here.
            return ret_val<item_pocket::contain_code>::make_success();
        } else {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_NO_SPACE, _( "holster already contains an item" ) );
        }
    }

    if( !data->ammo_restriction.empty() ) {
        const ammotype it_ammo = it.ammo_type();
        if( it.count() > remaining_ammo_capacity( it_ammo ) ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_NO_SPACE, _( "tried to put too many charges of ammo in item" ) );
        }
        // ammo restriction overrides item volume/weight and watertight/airtight data
        copies_remaining = std::max( 0, copies_remaining - remaining_ammo_capacity( it_ammo ) );
        return ret_val<item_pocket::contain_code>::make_success();
    }

    units::mass weight = it.weight();
    if( weight > weight_capacity() ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_HEAVY, _( "item is too heavy" ) );
    }
    units::volume volume = it.volume();
    if( volume > volume_capacity() ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_BIG, _( "item is too big" ) );
    }

    int fallback_capacity = it.count_by_charges() ? it.charges : copies_remaining;
    int copy_weight_capacity = weight <= 0_gram ? fallback_capacity :
                               charges_per_remaining_weight( it );
    int copy_volume_capacity = volume <= 0_ml ? fallback_capacity :
                               charges_per_remaining_volume( it );

    if( copy_weight_capacity < it.count() ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_CANNOT_SUPPORT, _( "pocket is holding too much weight" ) );
    }
    if( copy_volume_capacity < it.count() ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_NO_SPACE, _( "not enough space" ) );
    }

    if( copies_remaining > 1 ) {
        int how_many_copies_fit = std::min( { copies_remaining, copy_weight_capacity, copy_volume_capacity } );
        copies_remaining -= how_many_copies_fit;
    } else {
        copies_remaining = 0;
    }

    return ret_val<item_pocket::contain_code>::make_success();
}

bool item_pocket::can_contain_liquid( bool held_or_ground ) const
{
    if( held_or_ground ) {
        return data->watertight;
    } else {
        if( will_spill() ) {
            return false;
        }
        return data->watertight;
    }
}

bool item_pocket::contains_phase( phase_id phase ) const
{
    return !empty() && contents.front().made_of( phase );
}

bool item_pocket::can_reload_with( const item &ammo, const bool now ) const
{
    if( is_type( pocket_type::CONTAINER ) ) {
        // Only watertight container pockets are reloadable
        if( !watertight() ) {
            return false;
        }

        // CONTAINER pockets can reload liquids only
        if( !ammo.made_of( phase_id::LIQUID ) ) {
            return false;
        }
    }

    if( ammo.has_flag( flag_SPEEDLOADER ) ) {
        // The speedloader needs to be compatible,
        // The ammo in it needs to be compatible,
        // and the opcket needs to have enough space (except casings)
        return allows_speedloader( ammo.typeId() ) &&
               is_compatible( ammo.loaded_ammo() ).success() &&
               ( remaining_ammo_capacity( ammo.loaded_ammo().ammo_type() ) >= ammo.ammo_remaining() );
    }

    if( ammo.has_flag( flag_SPEEDLOADER_CLIP ) ) {
        // The speedloader clip needs to be compatible,
        // The ammo in it needs to be compatible,
        // and the pocket don't needs have enough space (except casings(if any))
        return allows_speedloader( ammo.typeId() ) &&
               is_compatible( ammo.loaded_ammo() ).success() &&
               ( remaining_ammo_capacity( ammo.loaded_ammo().ammo_type() ) + ammo_capacity(
                     ammo.loaded_ammo().ammo_type() ) > ammo_capacity( ammo.loaded_ammo().ammo_type() ) );
    }

    if( !is_compatible( ammo ).success() ) {
        return false;
    }

    if( !now ) {
        return true;
    } else {
        // Require that the new ammo can fit in with the old ammo.

        // No need to combine if empty.
        if( empty() ) {
            return true;
        }

        if( is_type( pocket_type::MAGAZINE ) ) {
            // Reloading is refused if
            // Pocket is full of ammo (casings are ignored)
            // Ammos are of different ammo type
            // If either of ammo is liquid while the other is not or they are different types

            if( full( false ) ) {
                return false;
            }

            for( const item *loaded : all_items_top() ) {
                if( loaded->has_flag( flag_CASING ) ) {
                    continue;
                }
                if( loaded->ammo_type() != ammo.ammo_type() ) {
                    return false;
                }
                if( loaded->made_of( phase_id::LIQUID ) || ammo.made_of( phase_id::LIQUID ) ) {
                    bool cant_combine = !loaded->made_of( phase_id::LIQUID ) || !ammo.made_of( phase_id::LIQUID ) ||
                                        loaded->type != ammo.type;
                    if( cant_combine ) {
                        return false;
                    }
                }
            }

            return true;
        } else if( is_type( pocket_type::MAGAZINE_WELL ) ) {
            // Reloading is refused if there already is full magazine here
            // Reloading with another identical mag with identical contents is also pointless so it is not allowed
            // Pocket can't know what ammo are compatible with the item so that is checked elsewhere

            return !front().is_magazine_full() && !( front().typeId() == ammo.typeId() &&
                    front().same_contents( ammo ) );
        } else if( is_type( pocket_type::CONTAINER ) ) {
            // Reloading is possible if liquid combines with old liquid
            if( front().can_combine( ammo ) ) {
                return true;
            }
            if( !ammo.made_of( phase_id::LIQUID ) ) {
                return true;
            }
        }
    }
    return false;
}

std::optional<item> item_pocket::remove_item( const item &it )
{
    item ret( it );
    const size_t sz = contents.size();
    contents.remove_if( [&it]( const item & rhs ) {
        return &rhs == &it;
    } );
    if( sz == contents.size() ) {
        return std::nullopt;
    } else {
        return ret;
    }
}

bool item_pocket::remove_internal( const std::function<bool( item & )> &filter,
                                   int &count, std::list<item> &res )
{
    for( auto it = contents.begin(); it != contents.end(); ) {
        if( filter( *it ) ) {
            res.splice( res.end(), contents, it++ );
            if( --count == 0 ) {
                return true;
            }
        } else {
            it->remove_internal( filter, count, res );
            ++it;
        }
    }
    return false;
}

std::optional<item> item_pocket::remove_item( const item_location &it )
{
    if( !it ) {
        return std::nullopt;
    }
    return remove_item( *it );
}

static void move_to_parent_pocket_recursive( const tripoint &pos, item &it,
        const item_location &loc, Character *carrier )
{
    if( loc ) {
        item_pocket *parent_pocket = loc.parent_pocket();
        if( parent_pocket && parent_pocket->can_contain( it ).success() ) {
            if( carrier ) {
                carrier->add_msg_if_player( m_bad, _( "Your %1$s falls into your %2$s." ),
                                            it.display_name(), loc.parent_item()->label( 1 ) );
            }
            parent_pocket->insert_item( it );
            return;
        }
        if( loc.where() == item_location::type::container ) {
            move_to_parent_pocket_recursive( pos, it, loc.parent_item(), carrier );
            return;
        }
    }

    map &here = get_map();
    if( carrier ) {
        carrier->invalidate_weight_carried_cache();
        carrier->add_msg_player_or_npc( m_bad, _( "Your %s falls to the ground." ),
                                        _( "<npcname>'s %s falls to the ground." ), it.display_name() );
    } else {
        add_msg_if_player_sees( pos, m_bad, _( "The %s falls to the ground." ), it.display_name() );
    }
    here.add_item_or_charges( pos, it );
}

void item_pocket::overflow( const tripoint &pos, const item_location &loc )
{
    if( is_type( pocket_type::MOD ) || is_type( pocket_type::CORPSE ) ||
        is_type( pocket_type::EBOOK ) || is_type( pocket_type::CABLE ) ) {
        return;
    }
    if( empty() ) {
        // no items to overflow
        return;
    }

    // overflow recursively
    for( item &it : contents ) {
        if( loc ) {
            item_location content_loc( loc, &it );
            content_loc.overflow();
        } else {
            it.overflow( pos );
        }
    }

    Character *carrier = loc.carrier();

    // first remove items that shouldn't be in there anyway
    std::unordered_map<itype_id, bool> contained_type_validity;
    for( auto iter = contents.begin(); iter != contents.end(); ) {

        // if item has any contents, check it individually
        if( !iter->get_contents().empty_with_no_mods() ) {
            if( !is_type( pocket_type::MIGRATION ) && can_contain( *iter, true ).success() ) {
                ++iter;
            } else {
                move_to_parent_pocket_recursive( pos, *iter, loc, carrier );
                iter = contents.erase( iter );
            }
            continue;
        }

        // otherwise, use cached results per item type
        auto cont_copy_type = contained_type_validity.emplace( iter->typeId(), true );
        if( cont_copy_type.second ) {
            cont_copy_type.first->second = !is_type( pocket_type::MIGRATION ) &&
                                           can_contain( *iter, true ).success();
        }
        if( cont_copy_type.first->second ) {
            ++iter;
        } else {
            move_to_parent_pocket_recursive( pos, *iter, loc, carrier );
            iter = contents.erase( iter );
        }
    }

    if( empty() ) {
        // we've removed the migration pockets and items that didn't fit
        return;
    }

    if( !data->ammo_restriction.empty() ) {
        const ammotype contained_ammotype = contents.front().ammo_type();
        const auto ammo_iter = data->ammo_restriction.find( contained_ammotype );
        if( ammo_iter == data->ammo_restriction.end() ) {
            // only one ammotype is allowed in an ammo restricted pocket
            // so if the first one is wrong, they're all wrong
            spill_contents( pos );
            return;
        }
        int total_qty = 0;
        for( auto iter = contents.begin(); iter != contents.end(); ) {
            item &ammo = *iter;
            total_qty += ammo.count();
            const int overflow_count = total_qty - ammo_iter->second;
            if( overflow_count > 0 ) {
                ammo.charges -= overflow_count;
                item dropped_ammo( ammo.typeId(), ammo.birthday(), overflow_count );
                move_to_parent_pocket_recursive( pos, *iter, loc, carrier );
                total_qty -= overflow_count;
            }
            if( ammo.count() == 0 ) {
                iter = contents.erase( iter );
            } else {
                ++iter;
            }
        }
        // return early, the rest of this function checks against volume and weight
        // and ammo_restriction is an override
        return;
    }

    if( remaining_volume() < 0_ml ) {
        contents.sort( []( const item & left, const item & right ) {
            return left.volume() > right.volume();
        } );
        while( remaining_volume() < 0_ml && !contents.empty() ) {
            move_to_parent_pocket_recursive( pos, contents.front(), loc, carrier );
            contents.pop_front();
        }
    }
    if( remaining_weight() < 0_gram ) {
        contents.sort( []( const item & left, const item & right ) {
            return left.weight() > right.weight();
        } );
        while( remaining_weight() < 0_gram && !contents.empty() ) {
            move_to_parent_pocket_recursive( pos, contents.front(), loc, carrier );
            contents.pop_front();
        }
    }
}

void item_pocket::on_pickup( Character &guy, item *avoid )
{
    if( will_spill() ) {
        while( !empty() ) {
            handle_liquid_or_spill( guy, avoid );
        }
        restack();
    }
}

void item_pocket::on_contents_changed()
{
    unseal();
    restack();
}

bool item_pocket::spill_contents( const tripoint &pos )
{
    if( is_type( pocket_type::EBOOK ) || is_type( pocket_type::CORPSE ) ||
        is_type( pocket_type::CABLE ) ) {
        return false;
    }

    map &here = get_map();
    for( item &it : contents ) {
        here.add_item_or_charges( pos, it );
    }

    contents.clear();
    return true;
}

void item_pocket::clear_items()
{
    contents.clear();
}

bool item_pocket::has_item( const item &it ) const
{
    return contents.end() !=
    std::find_if( contents.begin(), contents.end(), [&it]( const item & e ) {
        return &it == &e || e.has_item( it );
    } );
}

item *item_pocket::get_item_with( const std::function<bool( const item & )> &filter )
{
    for( item &it : contents ) {
        if( filter( it ) ) {
            return &it;
        }
    }
    return nullptr;
}

const item *item_pocket::get_item_with( const std::function<bool( const item & )> &filter ) const
{
    for( const item &it : contents ) {
        if( filter( it ) ) {
            return &it;
        }
    }
    return nullptr;
}

void item_pocket::remove_items_if( const std::function<bool( item & )> &filter )
{
    contents.remove_if( filter );
    on_contents_changed();
}

void item_pocket::process( map &here, Character *carrier, const tripoint &pos, float insulation,
                           temperature_flag flag, float spoil_multiplier_parent )
{
    for( auto iter = contents.begin(); iter != contents.end(); ) {
        if( iter->process( here, carrier, pos, insulation, flag,
                           // spoil multipliers on pockets are not additive or multiplicative, they choose the best
                           std::min( spoil_multiplier_parent, spoil_multiplier() ) ) ) {
            iter->spill_contents( pos );
            iter = contents.erase( iter );
        } else {
            ++iter;
        }
    }
}

void item_pocket::leak( map &here, Character *carrier, const tripoint &pos,
                        item_pocket *pocke )
{
    std::vector<item *> erases;
    for( auto iter = contents.begin(); iter != contents.end(); ) {
        if( iter->leak( here, carrier, pos, this ) ) {
            if( watertight() ) {
                ++iter;
                continue;
            }
            item *it = &*iter;

            if( pocke != nullptr ) {
                if( pocke->watertight() ) {
                    ++iter;
                    continue;
                }
                pocke->add( *it );
            } else {
                iter->unset_flag( flag_FROM_FROZEN_LIQUID );
                iter->on_drop( pos );
                here.add_item_or_charges( pos, *iter );
                if( carrier != nullptr ) {
                    carrier->invalidate_weight_carried_cache();
                    carrier->add_msg_if_player( _( "Liquid leaked out from the %s and dripped onto the ground!" ),
                                                this->get_name() );
                }
            }
            iter = contents.erase( iter );
        } else {
            ++iter;
        }
    }
}

bool item_pocket::is_default_state() const
{
    return empty() && settings.is_null() && !sealable();
}

bool item_pocket::empty() const
{
    return contents.empty();
}

bool item_pocket::full( bool allow_bucket ) const
{
    if( !allow_bucket && will_spill() ) {
        return true;
    }

    if( is_type( pocket_type::MAGAZINE ) && !data->ammo_restriction.empty() ) {
        // Fullness from remaining ammo capacity instead of volume
        for( const item &it : contents ) {
            if( it.has_flag( flag_CASING ) ) {
                continue;
            } else {
                return remaining_ammo_capacity( it.ammo_type() ) == 0;
            }
        }
        return false;
    }

    return remaining_volume() == 0_ml;
}

bool item_pocket::rigid() const
{
    return data->rigid;
}

bool item_pocket::watertight() const
{
    return data->watertight;
}

bool item_pocket::transparent() const
{
    return data->transparent || ( data->open_container && !sealed() );
}

bool item_pocket::is_standard_type() const
{
    return data->type == pocket_type::CONTAINER ||
           data->type == pocket_type::MAGAZINE ||
           data->type == pocket_type::MAGAZINE_WELL;
}

bool item_pocket::is_forbidden() const
{
    return data->forbidden;
}

bool item_pocket::airtight() const
{
    return data->airtight;
}

bool item_pocket::inherits_flags() const
{
    return data->inherits_flags;
}

bool item_pocket::allows_speedloader( const itype_id &speedloader_id ) const
{
    if( data->allowed_speedloaders.empty() ) {
        return false;
    } else {
        return data->allowed_speedloaders.count( speedloader_id );
    }
}

const pocket_data *item_pocket::get_pocket_data() const
{
    return data;
}

void item_pocket::add( const item &it, item **ret )
{
    contents.push_back( it );
    if( ret == nullptr ) {
        restack();
    } else {
        *ret = restack( &contents.back() );
    }
}

void item_pocket::add( const item &it, const int copies, std::vector<item *> &added )
{
    for( auto iter = contents.insert( contents.end(), copies, it ); iter != contents.end(); iter++ ) {
        added.push_back( &*iter );
    }
}

bool item_pocket::can_unload_liquid() const
{
    if( contents.size() != 1 ) {
        return true;
    }

    const item &cts = contents.front();
    bool cts_is_frozen_liquid = cts.made_of_from_type( phase_id::LIQUID ) &&
                                cts.made_of( phase_id::SOLID );
    return will_spill() || !cts_is_frozen_liquid;
}

int item_pocket::fill_with( const item &contained, Character &guy, int amount,
                            bool allow_unseal, bool ignore_settings )
{
    int num_contained = 0;

    if( !contained.count_by_charges() || amount <= 0 ) {
        return 0;
    }
    if( !ignore_settings && !settings.accepts_item( contained ) ) {
        return 0;
    }
    if( !allow_unseal && sealed() ) {
        return 0;
    }

    item contained_item( contained );
    item_location loc;
    ammotype ammo = contained.ammo_type();
    if( ammo_capacity( ammo ) ) {
        contained_item.charges = std::min( amount - num_contained,
                                           remaining_ammo_capacity( ammo ) );
    } else {
        contained_item.charges = std::min( { amount - num_contained,
                                             charges_per_remaining_volume( contained_item ),
                                             charges_per_remaining_weight( contained_item )
                                           } );
    }
    if( contained_item.charges == 0 ) {
        return 0;
    }

    contained_item.handle_pickup_ownership( guy );
    ret_val<item *> result = insert_item( contained_item );
    if( !result.success() ) {
        debugmsg( "charges per remaining pocket volume does not fit in that very volume" );
        return 0;
    }

    num_contained += contained_item.charges;
    if( allow_unseal ) {
        unseal();
    }
    result.value()->on_pickup( guy );
    return num_contained;
}

std::list<item> &item_pocket::edit_contents()
{
    return contents;
}

ret_val<item *> item_pocket::insert_item( const item &it,
        const bool into_bottom, bool restack_charges, bool ignore_contents )
{
    ret_val<item_pocket::contain_code> containable = can_contain( it, ignore_contents );

    if( !containable.success() ) {
        return ret_val<item *>::make_failure( nullptr, containable.str() );
    }

    item *inserted = nullptr;
    if( !into_bottom ) {
        contents.push_front( it );
        inserted = &contents.front();
    } else {
        contents.push_back( it );
        inserted = &contents.back();
    }
    if( restack_charges ) {
        inserted = restack( inserted );
    }
    return ret_val<item *>::make_success( inserted );
}

std::pair<item_location, item_pocket *> item_pocket::best_pocket_in_contents(
    item_location &this_loc, const item &it, const item *avoid,
    const bool allow_sealed, const bool ignore_settings )
{
    std::pair<item_location, item_pocket *> ret( this_loc, nullptr );
    // If the current pocket has restrictions or blacklists the item,
    // try the nested pocket regardless of whether it's soft or rigid.
    const bool ignore_rigidity =
        !settings.accepts_item( it ) ||
        !get_pocket_data()->get_flag_restrictions().empty() ||
        settings.priority() > 0;

    for( item &contained_item : contents ) {
        if( &contained_item == &it || &contained_item == avoid ) {
            continue;
        }
        item_location new_loc( this_loc, &contained_item );
        std::pair<item_location, item_pocket *> nested_pocket = contained_item.best_pocket( it, new_loc,
                avoid, allow_sealed, ignore_settings, /*nested=*/true, ignore_rigidity );
        if( nested_pocket.second != nullptr &&
            ( ret.second == nullptr ||
              ret.second->better_pocket( *nested_pocket.second, it, /*nested=*/true ) ) ) {
            ret = nested_pocket;
        }
    }
    return ret;
}

int item_pocket::obtain_cost( const item &it ) const
{
    if( has_item( it ) ) {
        return moves();
    }
    return 0;
}

bool item_pocket::is_type( pocket_type ptype ) const
{
    return ptype == data->type;
}

bool item_pocket::is_ablative() const
{
    return get_pocket_data()->ablative;
}

bool item_pocket::is_holster() const
{
    return get_pocket_data()->holster;
}

const translation &item_pocket::get_description() const
{
    return get_pocket_data()->description;
}

const translation &item_pocket::get_name() const
{
    return get_pocket_data()->name;
}

bool item_pocket::holster_full() const
{
    const pocket_data *p_data = get_pocket_data();
    return p_data->holster && !all_items_top().empty();
}

bool item_pocket::is_valid() const
{
    return data != nullptr;
}

units::length item_pocket::max_containable_length() const
{
    if( data ) {
        return data->max_item_length;
    }
    return 0_mm;
}

units::length item_pocket::min_containable_length() const
{
    if( data ) {
        return data->min_item_length;
    }
    return 0_mm;
}

units::volume item_pocket::contains_volume() const
{
    units::volume vol = 0_ml;
    for( const item &it : contents ) {
        vol += it.volume();
    }
    return vol;
}

units::mass item_pocket::contains_weight() const
{
    units::mass weight = 0_gram;
    for( const item &it : contents ) {
        weight += it.weight();
    }
    return weight;
}

units::mass item_pocket::remaining_weight() const
{
    return weight_capacity() - contains_weight();
}

int item_pocket::charges_per_remaining_volume( const item &it ) const
{
    if( it.count_by_charges() ) {
        units::volume non_it_volume = volume_capacity();
        int contained_charges = 0;
        for( const item &contained : contents ) {
            if( contained.stacks_with( it ) ) {
                contained_charges += contained.charges;
            } else {
                non_it_volume -= contained.volume();
            }
        }
        return it.charges_per_volume( non_it_volume, true ) - contained_charges;
    } else {
        return it.charges_per_volume( remaining_volume(), true );
    }
}

int item_pocket::charges_per_remaining_weight( const item &it ) const
{
    if( it.count_by_charges() ) {
        units::mass non_it_weight = weight_capacity();
        int contained_charges = 0;
        for( const item &contained : contents ) {
            if( contained.stacks_with( it ) ) {
                contained_charges += contained.charges;
            } else {
                non_it_weight -= contained.weight();
            }
        }
        return it.charges_per_weight( non_it_weight, true ) - contained_charges;
    } else {
        return it.charges_per_weight( remaining_weight(), true );
    }
}

int item_pocket::best_quality( const quality_id &id ) const
{
    int ret = 0;
    for( const item &it : contents ) {
        ret = std::max( ret, it.get_quality( id ) );
    }
    return ret;
}

void item_pocket::heat_up()
{
    for( item &it : contents ) {
        if( it.has_temperature() ) {
            it.heat_up();
        }
    }
}

void item_pocket::add_preset( const item_pocket::favorite_settings &preset )
{
    pocket_presets.emplace_back( preset );
    save_presets();
}

void item_pocket::save_presets()
{
    cata_path file = PATH_INFO::pocket_presets();

    write_to_file( file, [&]( std::ostream & fout ) {
        JsonOut jout( fout, true );
        serialize_presets( jout );
    }, _( "pocket preset configuration" ) );
}

bool item_pocket::has_preset( const std::string &s )
{
    return find_preset( s ) != pocket_presets.end();
}

std::vector<item_pocket::favorite_settings>::iterator item_pocket::find_preset(
    const std::string &s )
{
    std::vector<item_pocket::favorite_settings>::iterator iter = std::find_if( pocket_presets.begin(),
            pocket_presets.end(),
    [&s]( const item_pocket::favorite_settings & preset ) {
        return preset.get_preset_name().value() == s;
    } );
    return iter;
}

void item_pocket::delete_preset( const std::vector<item_pocket::favorite_settings>::iterator
                                 iter )
{
    pocket_presets.erase( iter );
    save_presets();
}

void item_pocket::set_no_rigid( const std::set<sub_bodypart_id> &is_no_rigid )
{
    no_rigid = is_no_rigid;
}

void item_pocket::load_presets()
{
    std::ifstream fin;
    cata_path file = PATH_INFO::pocket_presets();

    fs::path file_path = file.get_unrelative_path();
    fin.open( file_path, std::ifstream::in | std::ifstream::binary );

    if( fin.good() ) {
        try {
            JsonValue jsin = json_loader::from_path( file );
            deserialize_presets( jsin.get_array() );
        } catch( const JsonError &e ) {
            debugmsg( "Error while loading pocket presets: %s", e.what() );
        }
    }

    fin.close();
}

void item_pocket::serialize_presets( JsonOut &json )
{
    json.start_array();
    for( const item_pocket::favorite_settings &preset : pocket_presets ) {
        preset.serialize( json );
    }
    json.end_array();
}

void item_pocket::deserialize_presets( const JsonArray &ja )
{
    pocket_presets.clear();
    for( JsonObject jo : ja ) {
        item_pocket::favorite_settings preset;
        preset.deserialize( jo );
        pocket_presets.emplace_back( preset );
    }
}

units::volume pocket_data::max_contains_volume() const
{
    if( ammo_restriction.empty() ) {
        return volume_capacity;
    }

    // Find all valid ammo itypes
    std::vector<const itype *> ammo_types = Item_factory::find( [&]( const itype & t ) {
        return t.ammo && ammo_restriction.count( t.ammo->type );
    } );
    // Figure out which has the greatest volume and calculate on that basis
    units::volume max_total_volume = 0_ml;
    for( const auto *ammo_type : ammo_types ) {
        int stack_size = ammo_type->stack_size ? ammo_type->stack_size : 1;
        int max_count = ammo_restriction.at( ammo_type->ammo->type );
        units::volume this_volume =
            1_ml * divide_round_up( ammo_type->volume / 1_ml * max_count, stack_size );
        max_total_volume = std::max( max_total_volume, this_volume );
    }
    return max_total_volume;
}

void item_pocket::favorite_settings::clear()
{
    preset_name = std::nullopt;
    priority_rating = 0;
    item_whitelist.clear();
    item_blacklist.clear();
    category_whitelist.clear();
    category_blacklist.clear();
}

void item_pocket::favorite_settings::set_priority( const int priority )
{
    priority_rating = priority;
}

bool item_pocket::favorite_settings::is_null() const
{
    return item_whitelist.empty() && item_blacklist.empty() &&
           category_whitelist.empty() && category_blacklist.empty() &&
           priority() == 0 && !collapsed && !disabled && unload;
}

void item_pocket::favorite_settings::whitelist_item( const itype_id &id )
{
    // whitelisting twice removes the item from the list
    if( item_whitelist.count( id ) ) {
        item_whitelist.erase( id );
        return;
    }
    // remove the item from the blacklist if listed
    if( item_blacklist.count( id ) ) {
        item_blacklist.erase( id );
    }
    item_whitelist.insert( id );
}

void item_pocket::favorite_settings::blacklist_item( const itype_id &id )
{
    // blacklisting twice removes the item from the list
    if( item_blacklist.count( id ) ) {
        item_blacklist.erase( id );
        return;
    }
    // remove the item from the whitelist if listed
    if( item_whitelist.count( id ) ) {
        item_whitelist.erase( id );
    }
    item_blacklist.insert( id );
}

void item_pocket::favorite_settings::clear_item( const itype_id &id )
{
    item_whitelist.erase( id );
    item_blacklist.erase( id );
}

const cata::flat_set<itype_id> &item_pocket::favorite_settings::get_item_whitelist() const
{
    return item_whitelist;
}

const cata::flat_set<itype_id> &item_pocket::favorite_settings::get_item_blacklist() const
{
    return item_blacklist;
}

const cata::flat_set<item_category_id> &
item_pocket::favorite_settings::get_category_whitelist() const
{
    return category_whitelist;
}

const cata::flat_set<item_category_id> &
item_pocket::favorite_settings::get_category_blacklist() const
{
    return category_blacklist;
}

void item_pocket::favorite_settings::whitelist_category( const item_category_id &id )
{
    // whitelisting twice removes the category from the list
    if( category_whitelist.count( id ) ) {
        category_whitelist.erase( id );
        return;
    }
    // remove the category from the blacklist if listed
    if( category_blacklist.count( id ) ) {
        category_blacklist.erase( id );
    }
    category_whitelist.insert( id );
}

void item_pocket::favorite_settings::blacklist_category( const item_category_id &id )
{
    // blacklisting twice removes the category from the list
    if( category_blacklist.count( id ) ) {
        category_blacklist.erase( id );
        return;
    }
    // remove the category from the whitelist if listed
    if( category_whitelist.count( id ) ) {
        category_whitelist.erase( id );
    }
    category_blacklist.insert( id );
}

void item_pocket::favorite_settings::clear_category( const item_category_id &id )
{
    category_blacklist.erase( id );
    category_whitelist.erase( id );
}

/**
 * Check to see if the given item is accepted by this pocket based on player configured rules.
 *
 * The rules are defined in two list:
 *
 * - whitelist - when one or more items are on the list then only those items are accepted.
 * - blacklist - those items listed here are never accepted.
 *
 * When a whitelist is empty and the item is not blacklisted then it will be accepted.
 * Container items will be accepted only when all items inside are accepted.
 *
 * Note that the rules take into account both item id and category which have
 * to be accepted by the rules outlined above for an item to be accepted.
 *
 * @param it item to check.
 * @return true if the item is accepted by this pocket, false otherwise.
 * @see item_pocket::favorite_settings::accepts_container(const item&)
 */
bool item_pocket::favorite_settings::accepts_item( const item &it ) const
{
    // if this pocket is disabled it accepts nothing
    if( disabled ) {
        return false;
    }
    const itype_id &id = it.typeId();
    const item_category_id &cat = it.get_category_of_contents().id;

    // if the item is explicitly listed in either of the lists, then it's clear what to do with it
    if( item_blacklist.count( id ) ) {
        return false;
    }
    if( item_whitelist.count( id ) ) {
        return true;
    }

    // otherwise check the category, the same way
    if( category_blacklist.count( cat ) ) {
        return false;
    }
    if( category_whitelist.count( cat ) ) {
        return true;
    }

    // when the item is container then we are actually checking if pocket accepts
    // container content and not the container itself unless container is blacklisted
    if( it.is_container() && !it.empty_container() ) {
        const std::list<const item *> items = it.all_items_top();
        return std::all_of( items.begin(), items.end(), [this]( const item * it ) {
            return accepts_item( *it );
        } );
    }
    // finally, if no match was found, see if there were any filters at all,
    // and either allow or deny everything that's fallen through to here.
    if( !category_whitelist.empty() ) {
        return false;  // we've whitelisted only some categories, and this item is not out of those.
    }
    if( !item_whitelist.empty() && category_blacklist.empty() ) {
        // whitelisting only certain items, and not as a means to tweak blacklist.
        return false;
    }
    // no whitelist - everything goes.
    return true;
}

bool item_pocket::favorite_settings::is_collapsed() const
{
    return collapsed;
}

void item_pocket::favorite_settings::set_collapse( bool flag )
{
    collapsed = flag;
}

bool item_pocket::favorite_settings::is_disabled() const
{
    return disabled;
}

void item_pocket::favorite_settings::set_disabled( bool flag )
{
    disabled = flag;
}

bool item_pocket::favorite_settings::is_unloadable() const
{
    return unload;
}

void item_pocket::favorite_settings::set_unloadable( bool flag )
{
    unload = flag;
}

void item_pocket::favorite_settings::set_preset_name( const std::string &s )
{
    preset_name = s;
}

void item_pocket::favorite_settings::set_was_edited()
{
    player_edited = true;
}

bool item_pocket::favorite_settings::was_edited() const
{
    return player_edited;
}

const std::optional<std::string> &item_pocket::favorite_settings::get_preset_name() const
{
    return preset_name;
}

template<typename T>
std::string enumerate( cata::flat_set<T> container )
{
    std::vector<std::string> output;
    for( const T &id : container ) {
        output.push_back( id.c_str() );
    }
    return enumerate_as_string( output );
}

void item_pocket::favorite_settings::info( std::vector<iteminfo> &info ) const
{
    if( disabled ) {
        info.emplace_back( "BASE", string_format(
                               _( "Items <bad>won't be inserted</bad> into this pocket unless you manually insert them." ) ) );
    }
    if( !unload ) {
        info.emplace_back( "BASE", string_format(
                               _( "Items in this pocket <bad>won't be unloaded</bad> unless you manually drop them." ) ) );
    }
    if( preset_name.has_value() ) {
        info.emplace_back( "BASE", string_format( _( "Preset Name: %s" ), preset_name.value() ) );
    }

    info.emplace_back( "BASE", string_format( "%s %d", _( "Priority:" ), priority_rating ) );
    info.emplace_back( "BASE", string_format( _( "Item Whitelist: %s" ),
                       item_whitelist.empty() ? _( "(empty)" ) :
    enumerate_as_string( item_whitelist.begin(), item_whitelist.end(), []( const itype_id & id ) {
        return id->nname( 1 );
    } ) ) );
    info.emplace_back( "BASE", string_format( _( "Item Blacklist: %s" ),
                       item_blacklist.empty() ? _( "(empty)" ) :
    enumerate_as_string( item_blacklist.begin(), item_blacklist.end(), []( const itype_id & id ) {
        return id->nname( 1 );
    } ) ) );
    info.emplace_back( "BASE", string_format( _( "Category Whitelist: %s" ),
                       category_whitelist.empty() ? _( "(empty)" ) : enumerate( category_whitelist ) ) );
    info.emplace_back( "BASE", string_format( _( "Category Blacklist: %s" ),
                       category_blacklist.empty() ? _( "(empty)" ) : enumerate( category_blacklist ) ) );
}
