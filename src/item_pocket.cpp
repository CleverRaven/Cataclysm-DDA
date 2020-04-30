#include "item_pocket.h"

#include "assign.h"
#include "cata_utility.h"
#include "crafting.h"
#include "enums.h"
#include "game.h"
#include "generic_factory.h"
#include "handle_liquid.h"
#include "item.h"
#include "itype.h"
#include "json.h"
#include "map.h"
#include "player.h"
#include "point.h"
#include "units.h"

namespace io
{
// *INDENT-OFF*
template<>
std::string enum_to_string<item_pocket::pocket_type>( item_pocket::pocket_type data )
{
    switch ( data ) {
    case item_pocket::pocket_type::CONTAINER: return "CONTAINER";
    case item_pocket::pocket_type::MAGAZINE: return "MAGAZINE";
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

void pocket_data::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "pocket_type", type, item_pocket::pocket_type::CONTAINER );
    optional( jo, was_loaded, "ammo_restriction", ammo_restriction );
    // ammo_restriction is a type of override, making the mandatory members not mandatory and superfluous
    // putting it in an if statement like this should allow for report_unvisited_member to work here
    if( ammo_restriction.empty() ) {
        optional( jo, was_loaded, "min_item_volume", min_item_volume, volume_reader(), 0_ml );
        mandatory( jo, was_loaded, "max_contains_volume", max_contains_volume, volume_reader() );
        mandatory( jo, was_loaded, "max_contains_weight", max_contains_weight, mass_reader() );
    }
    optional( jo, was_loaded, "spoil_multiplier", spoil_multiplier, 1.0f );
    optional( jo, was_loaded, "weight_multiplier", weight_multiplier, 1.0f );
    optional( jo, was_loaded, "magazine_well", magazine_well, volume_reader(), 0_ml );
    optional( jo, was_loaded, "moves", moves, 100 );
    optional( jo, was_loaded, "fire_protection", fire_protection, false );
    optional( jo, was_loaded, "watertight", watertight, false );
    optional( jo, was_loaded, "gastight", gastight, false );
    optional( jo, was_loaded, "open_container", open_container, false );
    optional( jo, was_loaded, "flag_restriction", flag_restriction );
    optional( jo, was_loaded, "rigid", rigid, false );
    optional( jo, was_loaded, "holster", holster );
    optional( jo, was_loaded, "sealed_data", sealed_data );
}

void resealable_data::load( const JsonObject &jo )
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
           gastight == rhs.gastight &&
           fire_protection == rhs.fire_protection &&
           flag_restriction == rhs.flag_restriction &&
           type == rhs.type &&
           max_contains_volume == rhs.max_contains_volume &&
           min_item_volume == rhs.min_item_volume &&
           max_contains_weight == rhs.max_contains_weight &&
           spoil_multiplier == rhs.spoil_multiplier &&
           weight_multiplier == rhs.weight_multiplier &&
           moves == rhs.moves;
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
    const bool rhs_it_stack = rhs.has_item_stacks_with( it );
    if( has_item_stacks_with( it ) != rhs_it_stack ) {
        return rhs_it_stack;
    }
    if( data->ammo_restriction.empty() != rhs.data->ammo_restriction.empty() ) {
        // pockets restricted by ammo should try to get filled first
        return !rhs.data->ammo_restriction.empty();
    }
    if( data->flag_restriction.empty() != rhs.data->flag_restriction.empty() ) {
        // pockets restricted by flag should try to get filled first
        return !rhs.data->flag_restriction.empty();
    }
    if( it.is_comestible() && it.get_comestible()->spoils != 0_seconds ) {
        // a lower spoil multiplier is better
        return rhs.spoil_multiplier() < spoil_multiplier();
    }
    if( data->rigid != rhs.data->rigid ) {
        return rhs.data->rigid;
    }
    if( it.made_of( SOLID ) ) {
        if( data->watertight != rhs.data->watertight ) {
            return rhs.data->watertight;
        }
    }
    if( remaining_volume() == rhs.remaining_volume() ) {
        return rhs.obtain_cost( it ) < obtain_cost( it );
    }
    // we want the least amount of remaining volume
    return rhs.remaining_volume() < remaining_volume();
}

bool item_pocket::stacks_with( const item_pocket &rhs ) const
{
    return empty() || rhs.empty() || std::equal( contents.begin(), contents.end(),
            rhs.contents.begin(), rhs.contents.end(),
    []( const item & a, const item & b ) {
        return a.charges == b.charges && a.stacks_with( b );
    } );
}

bool item_pocket::is_funnel_container( units::volume &bigger_than ) const
{
    static const std::vector<item> allowed_liquids{
        item( "water", calendar::turn_zero, 1 ),
        item( "water_acid", calendar::turn_zero, 1 ),
        item( "water_acid_weak", calendar::turn_zero, 1 )
    };
    if( !data->watertight ) {
        return false;
    }
    if( !resealable() && _sealed ) {
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
    return data->max_contains_volume;
}

units::volume item_pocket::remaining_volume() const
{
    return data->max_contains_volume - contains_volume();
}

int item_pocket::remaining_capacity_for_item( const item &it ) const
{
    item item_copy( it );
    if( item_copy.count_by_charges() ) {
        item_copy.charges = 1;
    }
    int count_of_item = 0;
    item_pocket pocket_copy( *this );
    while( pocket_copy.can_contain( item_copy ).success()
           && count_of_item < it.count() ) {
        pocket_copy.insert_item( item_copy );
        count_of_item++;
    }
    return count_of_item;
}

units::volume item_pocket::item_size_modifier() const
{
    if( data->rigid ) {
        return 0_ml;
    }
    units::volume total_vol = 0_ml;
    for( const item &it : contents ) {
        if( is_type( item_pocket::pocket_type::MOD ) ) {
            total_vol += it.volume( true );
        } else {
            total_vol += it.volume();
        }
    }
    total_vol -= data->magazine_well;
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

item *item_pocket::magazine_current()
{
    auto iter = std::find_if( contents.begin(), contents.end(), []( const item & it ) {
        return it.is_magazine();
    } );
    return iter != contents.end() ? &*iter : nullptr;
}

int item_pocket::ammo_consume( int qty )
{
    int need = qty;
    while( !contents.empty() ) {
        item &e = contents.front();
        if( need >= e.charges ) {
            need -= e.charges;
            contents.erase( contents.begin() );
        } else {
            e.charges -= need;
            need = 0;
            break;
        }
    }
    return qty - need;
}

void item_pocket::casings_handle( const std::function<bool( item & )> &func )
{
    for( auto it = contents.begin(); it != contents.end(); ) {
        if( it->has_flag( "CASING" ) ) {
            it->unset_flag( "CASING" );
            if( func( *it ) ) {
                it = contents.erase( it );
                continue;
            }
            // didn't handle the casing so reset the flag ready for next call
            it->set_flag( "CASING" );
        }
        ++it;
    }
}

void item_pocket::handle_liquid_or_spill( Character &guy )
{
    for( auto iter = contents.begin(); iter != contents.end(); ) {
        if( iter->made_of( LIQUID ) ) {
            item liquid( *iter );
            iter = contents.erase( iter );
            liquid_handler::handle_all_liquid( liquid, 1 );
        } else {
            item i_copy( *iter );
            iter = contents.erase( iter );
            guy.i_add_or_drop( i_copy );
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

bool item_pocket::resealable() const
{
    // if it has sealed data it is understood that the
    // data is different when sealed than when not
    return !data->sealed_data;
}

bool item_pocket::seal()
{
    if( resealable() || empty() ) {
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
    if( resealable() ) {
        return false;
    } else {
        return _sealed;
    }
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

bool item_pocket::process( const itype &type, player *carrier, const tripoint &pos, bool activate,
                           float insulation, const temperature_flag flag )
{
    bool processed = false;
    for( auto it = contents.begin(); it != contents.end(); ) {
        if( _sealed ) {
            // Simulate that the item has already "rotten" up to last_rot_check, but as item::rot
            // is not changed, the item is still fresh.
            it->set_last_rot_check( calendar::turn );
        }
        if( it->process( carrier, pos, activate, type.insulation_factor * insulation, flag ) ) {
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
                contained_item.ammo_default(), contained_item.ammo_capacity() / 2
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
    const std::string space = "  ";

    if( disp_pocket_number ) {
        const std::string pocket_num = string_format( _( "Pocket %d:" ), pocket_number );
        info.emplace_back( "DESCRIPTION", pocket_num );
    }
    if( data->rigid ) {
        info.emplace_back( "DESCRIPTION", _( "This pocket is <info>rigid</info>." ) );
    }
    if( data->min_item_volume > 0_ml ) {
        info.emplace_back( "DESCRIPTION",
                           string_format( _( "Minimum volume of item allowed: <neutral>%s</neutral>" ),
                                          vol_to_string( data->min_item_volume ) ) );
    }

    info.emplace_back( "DESCRIPTION", string_format( _( "Volume Capacity: <neutral>%s</neutral>" ),
                       vol_to_string( data->max_contains_volume ) ) );

    info.emplace_back( "DESCRIPTION", string_format( _( "Weight Capacity: <neutral>%s</neutral>" ),
                       weight_to_string( data->max_contains_weight ) ) );

    info.emplace_back( "DESCRIPTION",
                       string_format( _( "Base moves to take an item out: <neutral>%d</neutral>" ),
                                      data->moves ) );

    if( data->watertight ) {
        info.emplace_back( "DESCRIPTION",
                           _( "This pocket can <info>contain a liquid</info>." ) );
    }
    if( data->gastight ) {
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
}

void item_pocket::contents_info( std::vector<iteminfo> &info, int pocket_number,
                                 bool disp_pocket_number ) const
{
    const std::string space = "  ";

    insert_separation_line( info );
    if( disp_pocket_number ) {
        if( !resealable() ) {
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
    info.emplace_back( "DESCRIPTION",
                       string_format( "%s: <neutral>%s / %s</neutral>", _( "Volume" ),
                                      vol_to_string( contains_volume() ),
                                      vol_to_string( data->max_contains_volume ) ) );
    info.emplace_back( "DESCRIPTION",
                       string_format( "%s: <neutral>%s / %s</neutral>", _( "Weight" ),
                                      weight_to_string( contains_weight() ),
                                      weight_to_string( data->max_contains_weight ) ) );

    bool contents_header = false;
    for( const item &contents_item : contents ) {
        if( !contents_item.type->mod ) {
            if( !contents_header ) {
                info.emplace_back( "DESCRIPTION", _( "<bold>Contents of this pocket</bold>:" ) );
                contents_header = true;
            } else {
                // Separate items with a blank line
                info.emplace_back( "DESCRIPTION", space );
            }

            const translation &description = contents_item.type->description;

            if( contents_item.made_of_from_type( LIQUID ) ) {
                info.emplace_back( "DESCRIPTION", contents_item.display_name() );
                info.emplace_back( vol_to_info( "CONTAINER", description + space,
                                                contents_item.volume() ) );
            } else {
                info.emplace_back( "DESCRIPTION", contents_item.display_name() );
            }
        }
    }
}

ret_val<item_pocket::contain_code> item_pocket::can_contain( const item &it ) const
{

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

    if( data->holster && !contents.empty() ) {
        item item_copy( contents.front() );
        if( item_copy.combine( it ) ) {
            return ret_val<item_pocket::contain_code>::make_success();
        } else {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_NO_SPACE, _( "holster already contains an item" ) );
        }
    }

    if( it.made_of( phase_id::LIQUID ) ) {
        if( !data->watertight ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_LIQUID, _( "can't contain liquid" ) );
        }
        if( size() != 0 && !item( contents.front() ).combine( it ) ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_LIQUID, _( "can't mix liquid with contained item" ) );
        }
    } else if( size() == 1 && contents.front().made_of( phase_id::LIQUID ) ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_LIQUID, _( "can't put non liquid into pocket with liquid" ) );
    }
    if( it.made_of( phase_id::GAS ) ) {
        if( !data->gastight ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_GAS, _( "can't contain gas" ) );
        }
        if( size() != 0 && !item( contents.front() ).combine( it ) ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_GAS, _( "can't mix gas with contained item" ) );
        }
    } else if( size() == 1 && contents.front().made_of( phase_id::GAS ) ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_LIQUID, _( "can't put non gas into pocket with gas" ) );
    }
    if( !data->flag_restriction.empty() && !it.has_any_flag( data->flag_restriction ) ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_FLAG, _( "item does not have correct flag" ) );
    }

    // ammo restriction overrides item volume and weight data
    if( !data->ammo_restriction.empty() ) {
        if( !it.is_ammo() ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_AMMO, _( "item is not an ammo" ) );
        }

        const ammotype it_ammo = it.ammo_type();
        const auto ammo_restriction_iter = data->ammo_restriction.find( it_ammo );

        if( ammo_restriction_iter == data->ammo_restriction.end() ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_AMMO, _( "item is not the correct ammo type" ) );
        }

        // how much ammo is inside the pocket
        int internal_count = 0;
        // the ammo must match what's inside
        if( !contents.empty() ) {
            if( it_ammo != contents.front().ammo_type() ) {
                return ret_val<item_pocket::contain_code>::make_failure(
                           contain_code::ERR_AMMO, _( "item is not the correct ammo type" ) );
            } else {
                internal_count = contents.front().count();
            }
        }

        if( it.count() + internal_count > ammo_restriction_iter->second ) {
            return ret_val<item_pocket::contain_code>::make_failure(
                       contain_code::ERR_NO_SPACE, _( "tried to put too many charges of ammo in item" ) );
        }

        return ret_val<item_pocket::contain_code>::make_success();
    }

    if( it.volume() < data->min_item_volume ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_SMALL, _( "item is too small" ) );
    }
    if( it.weight() > data->max_contains_weight ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_HEAVY, _( "item is too heavy" ) );
    }
    if( it.weight() > remaining_weight() ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_CANNOT_SUPPORT, _( "pocket is holding too much weight" ) );
    }
    if( it.volume() > data->max_contains_volume ) {
        return ret_val<item_pocket::contain_code>::make_failure(
                   contain_code::ERR_TOO_BIG, _( "item too big" ) );
    }
    if( it.volume() > remaining_volume() ) {
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
    // first remove items that shouldn't be in there anyway
    for( auto iter = contents.begin(); iter != contents.end(); ) {
        ret_val<item_pocket::contain_code> ret_contain = can_contain( *iter );
        if( is_type( item_pocket::pocket_type::MIGRATION ) ||
            ( !ret_contain.success() &&
              ret_contain.value() != contain_code::ERR_NO_SPACE &&
              ret_contain.value() != contain_code::ERR_CANNOT_SUPPORT ) ) {
            g->m.add_item_or_charges( pos, *iter );
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
            const int overflow_count = ammo_iter->second - ammo.count() - total_qty;
            if( overflow_count > 0 ) {
                ammo.charges -= overflow_count;
                item dropped_ammo( ammo.typeId(), ammo.birthday(), overflow_count );
                g->m.add_item_or_charges( pos, contents.front() );
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
            g->m.add_item_or_charges( pos, contents.front() );
            contents.pop_front();
        }
    }
    if( remaining_weight() < 0_gram ) {
        contents.sort( []( const item & left, const item & right ) {
            return left.weight() > right.weight();
        } );
        while( remaining_weight() < 0_gram && !contents.empty() ) {
            g->m.add_item_or_charges( pos, contents.front() );
            contents.pop_front();
        }
    }
}

void item_pocket::on_pickup( Character &guy )
{
    if( will_spill() ) {
        handle_liquid_or_spill( guy );
        restack();
    }
}

void item_pocket::on_contents_changed()
{
    _sealed = false;
    restack();
}

bool item_pocket::spill_contents( const tripoint &pos )
{
    for( item &it : contents ) {
        g->m.add_item_or_charges( pos, it );
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

void item_pocket::has_rotten_away()
{
    for( auto it = contents.begin(); it != contents.end(); ) {
        if( it->has_rotten_away() ) {
            it = contents.erase( it );
        } else {
            ++it;
        }
    }
}

void item_pocket::remove_rotten( const tripoint &pnt )
{
    for( auto iter = contents.begin(); iter != contents.end(); ) {
        if( iter->has_rotten_away( pnt, spoil_multiplier() ) ) {
            iter = contents.erase( iter );
        } else {
            ++iter;
        }
    }
}

void item_pocket::process( player *carrier, const tripoint &pos, bool activate, float insulation,
                           temperature_flag flag, float spoil_multiplier_parent )
{
    for( auto iter = contents.begin(); iter != contents.end(); ) {
        if( iter->process( carrier, pos, activate, insulation, flag,
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

void item_pocket::add( const item &it )
{
    contents.push_back( it );
    restack();
}

void item_pocket::fill_with( item contained )
{
    if( contained.count_by_charges() ) {
        contained.charges = 1;
    }
    while( can_contain( contained ).success() ) {
        add( contained );
    }
    restack();
}

bool item_pocket::can_unload_liquid() const
{
    if( contents.size() != 1 ) {
        return true;
    }

    const item &cts = contents.front();
    bool cts_is_frozen_liquid = cts.made_of_from_type( LIQUID ) && cts.made_of( SOLID );
    return will_spill() || !cts_is_frozen_liquid;
}

std::list<item> &item_pocket::edit_contents()
{
    return contents;
}

void item_pocket::migrate_item( item &obj, const std::set<itype_id> &migrations )
{
    for( const std::string &c : migrations ) {
        if( std::none_of( contents.begin(), contents.end(), [&]( const item & e ) {
        return e.typeId() == c;
        } ) ) {
            add( item( c, obj.birthday() ) );
        }
    }
}

ret_val<item_pocket::contain_code> item_pocket::insert_item( const item &it )
{
    const bool contain_override = !is_type( pocket_type::CONTAINER ) &&
                                  !is_type( pocket_type::MAGAZINE );
    const ret_val<item_pocket::contain_code> ret = can_contain( it );
    if( contain_override || ret.success() ) {
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
    return data->max_contains_weight - contains_weight();
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
