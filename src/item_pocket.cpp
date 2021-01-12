#include "item_pocket.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <utility>

#include "ammo.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "crafting.h"
#include "debug.h"
#include "enums.h"
#include "flag.h"
#include "generic_factory.h"
#include "handle_liquid.h"
#include "item.h"
#include "item_category.h"
#include "item_contents.h"
#include "item_factory.h"
#include "item_location.h"
#include "itype.h"
#include "json.h"
#include "map.h"
#include "output.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"
#include "units.h"
#include "units_utility.h"

namespace io
{
// *INDENT-OFF*
template<>
std::string enum_to_string<item_pocket::pocket_type>( item_pocket::pocket_type data )
{
    switch ( data ) {
    case item_pocket::pocket_type::CONTAINER: return "CONTAINER";
    case item_pocket::pocket_type::MAGAZINE: return "MAGAZINE";
    case item_pocket::pocket_type::MAGAZINE_WELL: return "MAGAZINE_WELL";
    case item_pocket::pocket_type::MOD: return "MOD";
    case item_pocket::pocket_type::CORPSE: return "CORPSE";
    case item_pocket::pocket_type::SOFTWARE: return "SOFTWARE";
    case item_pocket::pocket_type::MIGRATION: return "MIGRATION";
    case item_pocket::pocket_type::LAST: break;
    }
    debugmsg( "Invalid valid_target" );
    abort();
}
// *INDENT-ON*
} // namespace io

std::string pocket_data::check_definition() const
{
    if( type == item_pocket::pocket_type::MOD ||
        type == item_pocket::pocket_type::CORPSE ||
        type == item_pocket::pocket_type::MIGRATION ) {
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
    optional( jo, was_loaded, "pocket_type", type, item_pocket::pocket_type::CONTAINER );
    optional( jo, was_loaded, "ammo_restriction", ammo_restriction );
    optional( jo, was_loaded, "item_restriction", item_id_restriction );
    optional( jo, was_loaded, "allowed_speedloaders", allowed_speedloaders );
    if( jo.has_member( "ammo_restriction" ) && ammo_restriction.empty() ) {
        jo.throw_error( "pocket defines empty ammo_restriction" );
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
        mandatory( jo, was_loaded, "max_contains_volume", volume_capacity, volume_reader() );
        mandatory( jo, was_loaded, "max_contains_weight", max_contains_weight, mass_reader() );
        optional( jo, was_loaded, "max_item_length", max_item_length,
                  units::default_length_from_volume( volume_capacity ) * M_SQRT2 );
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
    optional( jo, was_loaded, "flag_restriction", flag_restrictions );
    optional( jo, was_loaded, "rigid", rigid, false );
    optional( jo, was_loaded, "holster", holster );
    optional( jo, was_loaded, "sealed_data", sealed_data );
}

void sealable_data::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "spoil_multiplier", spoil_multiplier, 1.0f );
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
               a.charges == b.charges;
    } );
}

void item_pocket::restack()
{
    if( contents.size() <= 1 ) {
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

bool item_pocket::better_pocket( const item_pocket &rhs, const item &it ) const
{
    if( this->settings.priority() != rhs.settings.priority() ) {
        // Priority overrides all other factors.
        return rhs.settings.priority() > this->settings.priority();
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

    if( it.made_of( phase_id::SOLID ) ) {
        if( data->watertight != rhs.data->watertight ) {
            return !rhs.data->watertight;
        }
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

bool item_pocket::stacks_with( const item_pocket &rhs ) const
{
    if( _sealed != rhs._sealed ) {
        return false;
    }
    return ( empty() && rhs.empty() ) || std::equal( contents.begin(), contents.end(),
            rhs.contents.begin(), rhs.contents.end(),
    []( const item & a, const item & b ) {
        return a.charges == b.charges && a.stacks_with( b );
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

std::list<item *> item_pocket::all_items_ptr( item_pocket::pocket_type pk_type )
{
    if( !is_type( pk_type ) ) {
        return std::list<item *>();
    }
    std::list<item *> all_items_top_level{ all_items_top() };
    for( item *it : all_items_top_level ) {
        std::list<item *> all_items_internal{ it->contents.all_items_ptr( pk_type ) };
        all_items_top_level.insert( all_items_top_level.end(), all_items_internal.begin(),
                                    all_items_internal.end() );
    }
    return all_items_top_level;
}

std::list<const item *> item_pocket::all_items_ptr( item_pocket::pocket_type pk_type ) const
{
    if( !is_type( pk_type ) ) {
        return std::list<const item *>();
    }
    std::list<const item *> all_items_top_level{ all_items_top() };
    for( const item *it : all_items_top_level ) {
        std::list<const item *> all_items_internal{ it->contents.all_items_ptr( pk_type ) };
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
        return 1;
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
        total_vol += it.volume( is_type( item_pocket::pocket_type::MOD ) );
    }
    total_vol -= data->magazine_well;
    total_vol *= data->volume_multiplier;
    return std::max( 0_ml, total_vol );
}

units::mass item_pocket::item_weight_modifier() const
{
    units::mass total_mass = 0_gram;
    for( const item &it : contents ) {
        if( is_type( item_pocket::pocket_type::MOD ) ) {
            total_mass += it.weight( true, true ) * data->weight_multiplier;
        } else {
            total_mass += it.weight() * data->weight_multiplier;
        }
    }
    return total_mass;
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
        return it.is_magazine();
    } );
    return iter != contents.end() ? &*iter : nullptr;
}

itype_id item_pocket::magazine_default() const
{
    if( is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
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
            used = need;
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
    if( !contents.empty() ) {
        if( ammo != contents.front().ammo_type() ) {
            return 0;
        } else {
            for( const item &it : contents ) {
                ammo_count += it.count();
            }
        }
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
            iter = contents.erase( iter );
            guy.i_add_or_drop( i_copy, 1, avoid );
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

bool item_pocket::process( const itype &type, player *carrier, const tripoint &pos,
                           float insulation, const temperature_flag flag )
{
    bool processed = false;
    float spoil_multiplier = 1.0f;
    for( auto it = contents.begin(); it != contents.end(); ) {
        if( _sealed ) {
            spoil_multiplier = 0.0f;
        }
        if( it->process( carrier, pos, type.insulation_factor * insulation, flag, spoil_multiplier ) ) {
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
        info.push_back( iteminfo( "DESCRIPTION", "--" ) );
    }
}

void item_pocket::general_info( std::vector<iteminfo> &info, int pocket_number,
                                bool disp_pocket_number ) const
{
    if( disp_pocket_number ) {
        const std::string pocket_num = string_format( _( "Pocket %d:" ), pocket_number );
        info.emplace_back( "DESCRIPTION", pocket_num );
    }

    // Show volume/weight for normal containers, or ammo capacity if ammo_restriction is defined
    if( data->ammo_restriction.empty() ) {
        info.push_back( vol_to_info( "CONTAINER", _( "Volume: " ), volume_capacity() ) );
        info.push_back( weight_to_info( "CONTAINER", _( "  Weight: " ), weight_capacity() ) );
        info.back().bNewLine = true;
    } else {
        for( const ammotype &at : ammo_types() ) {
            const std::string fmt = string_format( ngettext( "<num> round of %s",
                                                   "<num> rounds of %s", ammo_capacity( at ) ),
                                                   at->name() );
            info.emplace_back( "MAGAZINE", _( "Holds: " ), fmt, iteminfo::no_flags,
                               ammo_capacity( at ) );
        }
    }

    if( data->max_item_length != 0_mm ) {
        info.back().bNewLine = true;
        info.push_back( iteminfo( "BASE", _( "Maximum item length: " ),
                                  string_format( "<num> %s", length_units( data->max_item_length ) ),
                                  iteminfo::lower_is_better,
                                  convert_length( data->max_item_length ) ) );
    }

    if( data->min_item_volume > 0_ml ) {
        info.emplace_back( "DESCRIPTION",
                           string_format( _( "Minimum item volume: <neutral>%s</neutral>" ),
                                          vol_to_string( data->min_item_volume ) ) );
    }

    if( data->max_item_volume ) {
        info.emplace_back( "DESCRIPTION",
                           string_format( _( "Maximum item volume: <neutral>%s</neutral>" ),
                                          vol_to_string( *data->max_item_volume ) ) );
    }

    info.emplace_back( "DESCRIPTION",
                       string_format( _( "Base moves to remove item: <neutral>%d</neutral>" ),
                                      data->moves ) );
    if( data->rigid ) {
        info.emplace_back( "DESCRIPTION", _( "This pocket is <info>rigid</info>." ) );
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
            info.emplace_back( "DESCRIPTION", "Contained items <info>won't spoil</info>." );
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
                    info.emplace_back( "DESCRIPTION", string_format( "* <bold>or</bold> %s", f.restriction() ) );
                }
            }
        }
    }
}

void item_pocket::contents_info( std::vector<iteminfo> &info, int pocket_number,
                                 bool disp_pocket_number ) const
{
    const std::string space = "  ";

    insert_separation_line( info );
    if( disp_pocket_number ) {
        if( sealable() ) {
            info.emplace_back( "DESCRIPTION", string_format( _( "<bold>%s pocket %d</bold>" ),
                               translated_sealed_prefix(),
                               pocket_number ) );
        } else {
            info.emplace_back( "DESCRIPTION", string_format( _( "<bold>pocket %d</bold>" ),
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

    if( data->ammo_restriction.empty() ) {
        // With no ammo_restriction defined, show current volume/weight, and total capacity
        info.emplace_back( vol_to_info( "CONTAINER", _( "Volume: " ), contains_volume() ) );
        info.emplace_back( vol_to_info( "CONTAINER", _( " of " ), volume_capacity() ) );
        info.back().bNewLine = true;
        info.emplace_back( weight_to_info( "CONTAINER", _( "Weight: " ), contains_weight() ) );
        info.emplace_back( weight_to_info( "CONTAINER", _( " of " ), weight_capacity() ) );
    } else {
        // With ammo_restriction, total capacity does not matter, but show current volume/weight
        info.emplace_back( vol_to_info( "CONTAINER", _( "Volume: " ), contains_volume() ) );
        info.back().bNewLine = true;
        info.emplace_back( weight_to_info( "CONTAINER", _( "Weight: " ), contains_weight() ) );
    }

    bool contents_header = false;
    for( const item &contents_item : contents ) {
        if( !contents_header ) {
            info.emplace_back( "DESCRIPTION", _( "<bold>Contents of this pocket</bold>:" ) );
            contents_header = true;
        }

        const translation &description = contents_item.type->description;

        if( contents_item.made_of_from_type( phase_id::LIQUID ) ) {
            info.emplace_back( "DESCRIPTION", colorize( space + contents_item.display_name(),
                               contents_item.color_in_inventory() ) );
            info.emplace_back( vol_to_info( "CONTAINER", description + space,
                                            contents_item.volume() ) );
        } else {
            info.emplace_back( "DESCRIPTION", colorize( space + contents_item.display_name(),
                               contents_item.color_in_inventory() ) );
        }
    }
}

void item_pocket::favorite_info( std::vector<iteminfo> &info )
{
    settings.info( info );
}

ret_val<item_pocket::contain_code> item_pocket::can_contain( const item &it ) const
{
    // To prevent debugmsg. Casings can only be inserted in a magazine during firing.
    if( data->type == item_pocket::pocket_type::MAGAZINE && it.has_flag( flag_CASING ) ) {
        return ret_val<item_pocket::contain_code>::make_success();
    }

    if( data->type == item_pocket::pocket_type::CORPSE ) {
        // corpses can't have items stored in them the normal way,
        // we simply don't want them to "spill"
        return ret_val<item_pocket::contain_code>::make_success();
    }

    if( data->type == item_pocket::pocket_type::MOD ) {
        if( it.is_toolmod() || it.is_gunmod() ) {
            return ret_val<item_pocket::contain_code>::make_success();
        } else {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_MOD, _( "only mods can go into mod pocket" ) );
        }
    }

    if( !data->item_id_restriction.empty() &&
        data->item_id_restriction.count( it.typeId() ) == 0 ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_FLAG, _( "holster does not accept this item type" ) );
    }

    if( data->holster && !contents.empty() ) {
        if( contents.front().can_combine( it ) ) {
            return ret_val<item_pocket::contain_code>::make_success();
        } else {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_NO_SPACE, _( "holster already contains an item" ) );
        }
    }

    if( !data->get_flag_restrictions().empty() && !it.has_any_flag( data->get_flag_restrictions() ) ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_FLAG, _( "item does not have correct flag" ) );
    }

    // ammo restriction overrides item volume/weight and watertight/airtight data
    if( !data->ammo_restriction.empty() ) {
        if( !it.is_ammo() ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_AMMO, _( "item is not an ammo" ) );
        }

        const ammotype it_ammo = it.ammo_type();
        const auto ammo_restriction_iter = data->ammo_restriction.find( it.ammo_type() );

        if( ammo_restriction_iter == data->ammo_restriction.end() || ( !contents.empty() &&
                it_ammo != contents.front().ammo_type() ) ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_AMMO, _( "item is not the correct ammo type" ) );
        }

        if( it.count() > remaining_ammo_capacity( it_ammo ) ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_NO_SPACE, _( "tried to put too many charges of ammo in item" ) );
        }

        return ret_val<item_pocket::contain_code>::make_success();
    }

    if( it.made_of( phase_id::LIQUID ) ) {
        if( !data->watertight ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_LIQUID, _( "can't contain liquid" ) );
        }
        if( size() != 0 && !contents.front().can_combine( it ) ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_LIQUID, _( "can't mix liquid with contained item" ) );
        }
    } else if( size() == 1 && !it.is_frozen_liquid() &&
               ( contents.front().made_of( phase_id::LIQUID ) ||
                 contents.front().is_frozen_liquid() ) ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_LIQUID, _( "can't put non liquid into pocket with liquid" ) );
    }
    if( it.made_of( phase_id::GAS ) ) {
        if( !data->airtight ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_GAS, _( "can't contain gas" ) );
        }
        if( size() != 0 && !contents.front().can_combine( it ) ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_GAS, _( "can't mix gas with contained item" ) );
        }
    } else if( size() == 1 && contents.front().made_of( phase_id::GAS ) ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_LIQUID, _( "can't put non gas into pocket with gas" ) );
    }

    // liquids and gases avoid the size limit altogether
    // soft items also avoid the size limit
    // count_by_charges items check volume of single charge
    if( !it.made_of( phase_id::LIQUID ) && !it.made_of( phase_id::GAS ) &&
        !it.is_frozen_liquid() &&
        !it.is_soft() && data->max_item_volume &&
        !it.charges_per_volume( *data->max_item_volume ) ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_BIG, _( "item too big" ) );
    }
    if( it.length() > data->max_item_length ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_BIG, _( "item is too long" ) );
    }

    if( it.volume() < data->min_item_volume ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_SMALL, _( "item is too small" ) );
    }
    if( it.weight() > weight_capacity() ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_HEAVY, _( "item is too heavy" ) );
    }
    if( it.weight() > 0_gram && charges_per_remaining_weight( it ) < it.count() ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_CANNOT_SUPPORT, _( "pocket is holding too much weight" ) );
    }
    if( it.volume() > volume_capacity() ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_BIG, _( "item too big" ) );
    }
    if( it.volume() > 0_ml && charges_per_remaining_volume( it ) < it.count() ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_NO_SPACE, _( "not enough space" ) );
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

cata::optional<item> item_pocket::remove_item( const item &it )
{
    item ret( it );
    const size_t sz = contents.size();
    contents.remove_if( [&it]( const item & rhs ) {
        return &rhs == &it;
    } );
    if( sz == contents.size() ) {
        return cata::nullopt;
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
            it->contents.remove_internal( filter, count, res );
            ++it;
        }
    }
    return false;
}

cata::optional<item> item_pocket::remove_item( const item_location &it )
{
    if( !it ) {
        return cata::nullopt;
    }
    return remove_item( *it );
}

void item_pocket::overflow( const tripoint &pos )
{
    if( is_type( item_pocket::pocket_type::MOD ) || is_type( item_pocket::pocket_type::CORPSE ) ) {
        return;
    }
    if( empty() ) {
        // no items to overflow
        return;
    }

    // overflow recursively
    for( item &it : contents ) {
        it.contents.overflow( pos );
    }

    map &here = get_map();
    // first remove items that shouldn't be in there anyway
    for( auto iter = contents.begin(); iter != contents.end(); ) {
        ret_val<item_pocket::contain_code> ret_contain = can_contain( *iter );
        if( is_type( item_pocket::pocket_type::MIGRATION ) ||
            ( !ret_contain.success() &&
              ret_contain.value() != contain_code::ERR_NO_SPACE &&
              ret_contain.value() != contain_code::ERR_CANNOT_SUPPORT ) ) {
            here.add_item_or_charges( pos, *iter );
            iter = contents.erase( iter );
        } else {
            ++iter;
        }
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
                here.add_item_or_charges( pos, contents.front() );
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
            here.add_item_or_charges( pos, contents.front() );
            contents.pop_front();
        }
    }
    if( remaining_weight() < 0_gram ) {
        contents.sort( []( const item & left, const item & right ) {
            return left.weight() > right.weight();
        } );
        while( remaining_weight() < 0_gram && !contents.empty() ) {
            here.add_item_or_charges( pos, contents.front() );
            contents.pop_front();
        }
    }
}

void item_pocket::on_pickup( Character &guy )
{
    if( will_spill() ) {
        while( !empty() ) {
            handle_liquid_or_spill( guy );
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

void item_pocket::remove_items_if( const std::function<bool( item & )> &filter )
{
    contents.remove_if( filter );
    on_contents_changed();
}

void item_pocket::process( player *carrier, const tripoint &pos, float insulation,
                           temperature_flag flag, float spoil_multiplier_parent )
{
    for( auto iter = contents.begin(); iter != contents.end(); ) {
        if( iter->process( carrier, pos, insulation, flag,
                           // spoil multipliers on pockets are not additive or multiplicative, they choose the best
                           std::min( spoil_multiplier_parent, spoil_multiplier() ) ) ) {
            iter = contents.erase( iter );
        } else {
            ++iter;
        }
    }
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

bool item_pocket::is_standard_type() const
{
    return data->type == pocket_type::CONTAINER ||
           data->type == pocket_type::MAGAZINE ||
           data->type == pocket_type::MAGAZINE_WELL;
}

bool item_pocket::airtight() const
{
    return data->airtight;
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

std::list<item> &item_pocket::edit_contents()
{
    return contents;
}

ret_val<item_pocket::contain_code> item_pocket::insert_item( const item &it )
{
    const ret_val<item_pocket::contain_code> ret = !is_standard_type() ?
            ret_val<item_pocket::contain_code>::make_success() : can_contain( it );
    if( ret.success() ) {
        contents.push_back( it );
    }
    restack();
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
        return it.charges_per_volume( non_it_volume ) - contained_charges;
    } else {
        return it.charges_per_volume( remaining_volume() );
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
        return it.charges_per_weight( non_it_weight ) - contained_charges;
    } else {
        return it.charges_per_weight( remaining_weight() );
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
           priority() == 0;
}

void item_pocket::favorite_settings::whitelist_item( const itype_id &id )
{
    item_blacklist.clear();
    if( item_whitelist.count( id ) ) {
        item_whitelist.erase( id );
    } else {
        item_whitelist.insert( id );
    }
}

void item_pocket::favorite_settings::blacklist_item( const itype_id &id )
{
    item_whitelist.clear();
    if( item_blacklist.count( id ) ) {
        item_blacklist.erase( id );
    } else {
        item_blacklist.insert( id );
    }
}

void item_pocket::favorite_settings::clear_item( const itype_id &id )
{
    item_whitelist.erase( id );
    item_blacklist.erase( id );
}

void item_pocket::favorite_settings::whitelist_category( const item_category_id &id )
{
    category_blacklist.clear();
    if( category_whitelist.count( id ) ) {
        category_whitelist.erase( id );
    } else {
        category_whitelist.insert( id );
    }
}

void item_pocket::favorite_settings::blacklist_category( const item_category_id &id )
{
    category_whitelist.clear();
    if( category_blacklist.count( id ) ) {
        category_blacklist.erase( id );
    } else {
        category_blacklist.insert( id );
    }
}

void item_pocket::favorite_settings::clear_category( const item_category_id &id )
{
    category_blacklist.erase( id );
    category_whitelist.erase( id );
}

bool item_pocket::favorite_settings::accepts_item( const item &it ) const
{
    const itype_id &id = it.typeId();
    const item_category_id &cat = it.get_category_of_contents().id;

    bool accept_category = ( category_whitelist.empty() || category_whitelist.count( cat ) ) &&
                           !category_blacklist.count( cat );
    if( accept_category ) {
        // Allowed unless explicitly disallowed by the item filters.
        return ( item_whitelist.empty() || item_whitelist.count( id ) ) && !item_blacklist.count( id );
    } else {
        // Disallowed unless explicitly whitelisted
        return item_whitelist.count( id );
    }
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
    info.push_back( iteminfo( "BASE", string_format( "%s %d", _( "Priority:" ), priority_rating ) ) );
    info.push_back( iteminfo( "BASE", string_format( _( "Item Whitelist: %s" ),
                              item_whitelist.empty() ? _( "(empty)" ) :
    enumerate_as_string( item_whitelist.begin(), item_whitelist.end(), []( const itype_id & id ) {
        return id->nname( 1 );
    } ) ) ) );
    info.push_back( iteminfo( "BASE", string_format( _( "Item Blacklist: %s" ),
                              item_blacklist.empty() ? _( "(empty)" ) :
    enumerate_as_string( item_blacklist.begin(), item_blacklist.end(), []( const itype_id & id ) {
        return id->nname( 1 );
    } ) ) ) );
    info.push_back( iteminfo( "BASE", string_format( _( "Category Whitelist: %s" ),
                              category_whitelist.empty() ? _( "(empty)" ) : enumerate( category_whitelist ) ) ) );
    info.push_back( iteminfo( "BASE", string_format( _( "Category Blacklist: %s" ),
                              category_blacklist.empty() ? _( "(empty)" ) : enumerate( category_blacklist ) ) ) );
}
