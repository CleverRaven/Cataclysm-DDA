/**
* This file is for container-related functions strictly in the `item` class.
*
* This file is NOT for `item_pocket` or `item_contents` function definitions;
* many of this file's functions forward to identically-named `item_pocket` or `item_contents`
* functions, which is intended.
*/

#include "item.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "debug.h"
#include "enum_bitset.h"
#include "enums.h"
#include "flag.h"
#include "flat_set.h"
#include "item_category.h"
#include "item_contents.h"
#include "item_location.h"
#include "item_pocket.h"
#include "item_tname.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

class map;

static const item_category_id item_category_container( "container" );

item item::in_its_container( int qty ) const
{
    return in_container( type->default_container.value_or( itype_id::NULL_ID() ), qty,
                         type->default_container_sealed, type->default_container_variant.value_or( "" ) );
}

item item::in_container( const itype_id &cont, int qty, bool sealed,
                         const std::string &variant ) const
{
    if( cont.is_null() ) {
        return *this;
    }

    if( qty <= 0 ) {
        qty = count();
    }
    item container( cont, birthday() );
    if( !variant.empty() ) {
        container.set_itype_variant( variant );
    }
    if( container.is_container() || container.is_estorage() ) {
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
    }
    return *this;
}

void item::add_automatic_whitelist()
{
    std::vector<item_pocket *> pkts = get_container_pockets();
    if( pkts.size() == 1 && contents_only_one_type() ) {
        pkts.front()->settings.whitelist_item( contents.first_item().typeId() );
        pkts.front()->settings.set_priority( 100 );
        pkts.front()->settings.set_collapse( true );
    }
}

void item::clear_automatic_whitelist()
{
    std::vector<item_pocket *> pkts = get_container_pockets();
    if( pkts.size() == 1 && !pkts.front()->settings.was_edited() ) {
        cata::flat_set<itype_id> const &wl = pkts.front()->settings.get_item_whitelist();
        if( !wl.empty() ) {
            pkts.front()->settings.clear_item( *wl.begin() );
            pkts.front()->settings.set_priority( 0 );
        }
    }
}

void item::update_modified_pockets()
{
    std::optional<const pocket_data *> mag_or_mag_well;
    std::vector<const pocket_data *> container_pockets;

    // Prevent cleanup of pockets belonging to the item base type
    for( const pocket_data &pocket : type->pockets ) {
        if( pocket.type == pocket_type::CONTAINER ) {
            container_pockets.push_back( &pocket );
        } else if( pocket.type == pocket_type::MAGAZINE ||
                   pocket.type == pocket_type::MAGAZINE_WELL ) {
            mag_or_mag_well = &pocket;
        }
    }

    // Prevent cleanup of added modular pockets
    for( const item *it : contents.get_added_pockets() ) {
        for( const pocket_data &pocket : it->type->pockets ) {
            if( pocket.type == pocket_type::CONTAINER ) {
                container_pockets.push_back( &pocket );
            }
        }
    }

    for( const item *mod : mods() ) {
        if( mod->type->mod ) {
            for( const pocket_data &pocket : mod->type->mod->add_pockets ) {
                if( pocket.type == pocket_type::CONTAINER ) {
                    container_pockets.push_back( &pocket );
                } else if( pocket.type == pocket_type::MAGAZINE ||
                           pocket.type == pocket_type::MAGAZINE_WELL ) {
                    mag_or_mag_well = &pocket;
                }
            }
        }
    }

    contents.update_modified_pockets( mag_or_mag_well, container_pockets );
}

bool item::same_contents( const item &rhs ) const
{
    return get_contents().same_contents( rhs.get_contents() );
}

int item::obtain_cost( const item &it ) const
{
    return contents.obtain_cost( it );
}

int item::insert_cost( const item &it ) const
{
    return contents.insert_cost( it );
}

ret_val<void> item::put_in( const item &payload, pocket_type pk_type,
                            const bool unseal_pockets, Character *carrier )
{
    ret_val<item *> result = contents.insert_item( payload, pk_type, false, unseal_pockets );
    if( !result.success() ) {
        debugmsg( "tried to put an item (%s) count (%d) in a container (%s) that cannot contain it: %s",
                  payload.typeId().str(), payload.count(), typeId().str(), result.str() );
        return ret_val<void>::make_failure( result.str() );
    }
    if( pk_type == pocket_type::MOD ) {
        update_modified_pockets();
    }
    if( carrier ) {
        result.value()->on_pickup( *carrier );
    }
    on_contents_changed();
    return ret_val<void>::make_success( result.str() );
}

void item::force_insert_item( const item &it, pocket_type pk_type )
{
    contents.force_insert_item( it, pk_type );
    on_contents_changed();
}

void item::on_contents_changed()
{
    contents.update_open_pockets();
    cached_relative_encumbrance.reset();
    encumbrance_update_ = true;
    update_inherited_flags();
    cached_category.timestamp = calendar::turn_max;
    if( empty_container() ) {
        clear_automatic_whitelist();
    }
}

bool item::is_collapsed() const
{
    return !contents.empty() && !contents.get_pockets( []( item_pocket const & pocket ) {
        return pocket.settings.is_collapsed() && pocket.is_standard_type();
    } ).empty();
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
    return contents.has_pocket_type( pocket_type::CONTAINER );
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

bool item::has_pocket_type( pocket_type pk_type ) const
{
    return contents.has_pocket_type( pk_type );
}

bool item::has_any_with( const std::function<bool( const item & )> &filter,
                         pocket_type pk_type ) const
{
    return contents.has_any_with( filter, pk_type );
}

bool item::all_pockets_rigid() const
{
    return contents.all_pockets_rigid();
}

bool item::container_type_pockets_empty() const
{
    return contents.container_type_pockets_empty();
}

std::vector<item_pocket*> item::get_pockets(std::function<bool(const item_pocket& pocket)> include_pocket)
{
    return contents.get_pockets(include_pocket);
}

std::vector<const item_pocket*> item::get_pockets(std::function<bool(const item_pocket& pocket)> include_pocket) const
{
    return contents.get_pockets(include_pocket);
}

std::vector<const item_pocket *> item::get_container_pockets() const
{
    return contents.get_container_pockets();
}

std::vector<item_pocket *> item::get_container_pockets()
{
    return contents.get_container_pockets();
}

std::vector<const item_pocket *> item::get_standard_pockets() const
{
    return contents.get_standard_pockets();
}

std::vector<item_pocket *> item::get_standard_pockets()
{
    return contents.get_standard_pockets();
}

std::vector<item_pocket *> item::get_ablative_pockets()
{
    return contents.get_ablative_pockets();
}

std::vector<const item_pocket *> item::get_ablative_pockets() const
{
    return contents.get_ablative_pockets();
}

std::vector<const item_pocket *> item::get_container_and_mod_pockets() const
{
    return contents.get_container_and_mod_pockets();
}

std::vector<item_pocket *> item::get_container_and_mod_pockets()
{
    return contents.get_container_and_mod_pockets();
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

bool item::is_container_empty() const
{
    return contents.empty();
}

bool item::is_container_full( bool allow_bucket ) const
{
    return contents.full( allow_bucket );
}

bool item::can_unload() const
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

bool item::is_funnel_container( units::volume &bigger_than ) const
{
    if( get_volume_capacity(item_pocket::ok_container) <= bigger_than ) {
        return false; // skip contents check, performance
    }
    return contents.is_funnel_container( bigger_than );
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
    return contents.max_containable_volume(); // TODO: Remove
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
    return can_contain( it, false, false, true, false, item_location(), 10000000_ml, false );
}

ret_val<void> item::can_contain( const item &it, const bool nested, const bool ignore_rigidity,
                                 const bool ignore_pkt_settings, const bool ignore_non_container_pocket,
                                 const item_location &parent_it,
                                 units::volume remaining_parent_volume, const bool allow_nested ) const
{
    int copies = 1;
    return can_contain( it, copies, nested, ignore_rigidity,
                        ignore_pkt_settings, ignore_non_container_pocket, parent_it,
                        remaining_parent_volume, allow_nested );
}

ret_val<void> item::can_contain( const item &it, int &copies_remaining, const bool nested,
                                 const bool ignore_rigidity, const bool ignore_pkt_settings,
                                 const bool ignore_non_container_pocket, const item_location &parent_it,
                                 units::volume remaining_parent_volume,
                                 const bool allow_nested ) const
{
    if( copies_remaining <= 0 ) {
        return ret_val<void>::make_success();
    }
    if( this == &it || ( parent_it.where() != item_location::type::invalid &&
                         this == parent_it.get_item() ) ) {
        // does the set of all sets contain itself?
        // or does this already contain it?
        return ret_val<void>::make_failure( "can't put a container into itself" );
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
        for( const item_pocket *pkt : contents.get_container_pockets() ) {
            if( pkt->empty() ) {
                continue;
            }

            // early exit for max length no nested item is gonna fix this
            if( pkt->max_containable_length() < it.length() ) {
                continue;
            }

            // If the current pocket has restrictions or blacklists the item or is a holster,
            // try the nested pocket regardless of whether it's soft or rigid.
            const bool ignore_nested_rigidity =
                !pkt->settings.accepts_item( it ) ||
                !pkt->get_pocket_data()->get_flag_restrictions().empty() || pkt->is_holster();
            units::volume remaining_volume = pkt->remaining_volume();
            for( const item *internal_it : pkt->all_items_top() ) {
                const int copies_remaining_before = copies_remaining;
                auto ret = internal_it->can_contain( it, copies_remaining, true, ignore_nested_rigidity,
                                                     ignore_pkt_settings, ignore_non_container_pocket, parent_it,
                                                     remaining_volume );
                if( copies_remaining <= 0 ) {
                    return ret;
                }
                if( copies_remaining_before != copies_remaining ) {
                    remaining_volume = pkt->remaining_volume();
                }
            }
        }
    }

    return nested && !ignore_rigidity
           ? contents.can_contain_rigid( it, copies_remaining, ignore_pkt_settings,
                                         ignore_non_container_pocket )
           : contents.can_contain( it, copies_remaining, ignore_pkt_settings, ignore_non_container_pocket,
                                   remaining_parent_volume );
}

ret_val<void> item::can_contain( const itype &tp ) const
{
    return can_contain( item( &tp ) );
}

ret_val<void> item::can_contain_partial( const item &it ) const
{
    item i_copy = it;
    if( i_copy.count_by_charges() ) {
        i_copy.charges = 1;
    }
    return can_contain( i_copy );
}

ret_val<void> item::can_contain_partial( const item &it, int &copies_remaining,
        bool nested ) const
{
    item i_copy = it;
    if( i_copy.count_by_charges() ) {
        i_copy.charges = 1;
    }
    return can_contain( i_copy, copies_remaining, nested );
}

ret_val<void> item::can_contain_partial_directly( const item &it ) const
{
    item i_copy = it;
    if( i_copy.count_by_charges() ) {
        i_copy.charges = 1;
    }
    return can_contain( i_copy, false, false, true, false, item_location(), 10000000_ml, false );
}

std::pair<item_location, item_pocket *> item::best_pocket( const item &it, item_location &this_loc,
        const item *avoid, const bool allow_sealed, const bool ignore_settings,
        const bool nested, bool ignore_rigidity, bool allow_nested )
{
    return contents.best_pocket( it, this_loc, avoid, allow_sealed, ignore_settings,
                                 nested, ignore_rigidity, allow_nested );
}

bool item::spill_contents( Character &c )
{
    if( ( !is_container() && !is_magazine() && !uses_magazine() ) ||
        is_container_empty() ) {
        return true;
    }

    if( c.is_npc() ) {
        return spill_contents( c.pos_bub() );
    }

    contents.handle_liquid_or_spill( c, /*avoid=*/this );
    on_contents_changed();

    return is_container_empty();
}

bool item::spill_contents( const tripoint_bub_ms &pos )
{
    if( ( !is_container() && !is_magazine() && !uses_magazine() ) ||
        is_container_empty() ) {
        return true;
    }
    return contents.spill_contents( pos );
}

bool item::spill_contents( map *here, const tripoint_bub_ms &pos )
{
    if( ( !is_container() && !is_magazine() && !uses_magazine() ) ||
        is_container_empty() ) {
        return true;
    }
    return contents.spill_contents( here, pos );
}

bool item::spill_open_pockets( Character &guy, const item *avoid )
{
    return contents.spill_open_pockets( guy, avoid );
}

void item::overflow( map &here, const tripoint_bub_ms &pos, const item_location &loc )
{
    contents.overflow( here, pos, loc );
}

void item::handle_liquid_or_spill( Character &guy, const item *avoid )
{
    contents.handle_liquid_or_spill( guy, avoid );
}

units::volume item::get_volume_capacity(std::function<bool(const item_pocket&)> include_pocket) const
{
    return contents.volume_capacity(include_pocket);
}

units::volume item::get_volume_capacity_recursive(std::function<bool(const item_pocket&)> include_pocket, 
    std::function<bool(const item_pocket&)> check_pocket_tree,
    units::volume& out_volume_expansion) const
{
    return contents.volume_capacity_recursive(include_pocket, check_pocket_tree, out_volume_expansion);
}


units::mass item::get_total_weight_capacity( const bool unrestricted_pockets_only ) const
{
    return contents.total_container_weight_capacity( unrestricted_pockets_only );
}

units::volume item::get_remaining_volume(std::function<bool(const item_pocket&)> include_pocket) const
{
    return contents.remaining_volume(include_pocket);
}

units::volume item::get_remaining_volume_recursive(std::function<bool(const item_pocket&)> include_pocket,
    std::function<bool(const item_pocket&)> check_pocket_tree,
    units::volume& out_volume_expansion) const
{
    return contents.remaining_volume_recursive(include_pocket, check_pocket_tree, out_volume_expansion);
}

units::mass item::get_remaining_weight_capacity( const bool unrestricted_pockets_only ) const
{
    return contents.remaining_container_capacity_weight( unrestricted_pockets_only );
}

units::volume item::get_contents_volume(std::function<bool(const item_pocket&)> include_pocket) const
{
    return contents.contents_volume(include_pocket);
}

units::mass item::get_total_contained_weight( const bool unrestricted_pockets_only ) const
{
    return contents.total_contained_weight( unrestricted_pockets_only );
}

units::volume item::get_biggest_pocket_capacity() const
{
    return contents.biggest_pocket_capacity();
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

    if( can_contain_partial( liquid ).success() ) {
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

int item::fill_with( const item &contained, const int amount,
                     const bool unseal_pockets,
                     const bool allow_sealed,
                     const bool ignore_settings,
                     const bool into_bottom,
                     const bool allow_nested,
                     Character *carrier )
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
                                  ignore_settings, false, false, allow_nested ).second;
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

        ret_val<item *> result = pocket->insert_item( contained_item, into_bottom );
        if( !result.success() ) {
            if( count_by_charges ) {
                debugmsg( "charges per remaining pocket volume does not fit in that very volume: %s",
                          result.str() );
            } else {
                debugmsg( "best pocket for item cannot actually contain the item: %s", result.str() );
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
        if( carrier ) {
            result.value()->on_pickup( *carrier );
        }
    }
    if( num_contained == 0 ) {
        debugmsg( "tried to put an item (%s, amount %d) in a container (%s) that cannot contain it",
                  contained_item.typeId().str(), contained_item.charges, typeId().str() );
    }
    on_contents_changed();
    return num_contained;
}

bool item::can_holster( const item &obj ) const
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

bool item::leak( map &here, Character *carrier, const tripoint_bub_ms &pos, item_pocket *pocke )
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

/*
units::volume item::check_for_free_space() const
{
    units::volume volume;

    for( const item *container : contents.all_items_top( pocket_type::CONTAINER ) ) {
        std::vector<const item_pocket *> containedPockets =
            container->contents.get_container_pockets();
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
*/

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

bool item::has_unrestricted_pockets() const
{
    return contents.has_unrestricted_pockets();
}

units::volume item::get_nested_content_volume_recursive( const std::map<const item *, int>
        &without )
const
{
    return contents.get_nested_content_volume_recursive( without );
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

std::list<const item *> item::all_items_container_top() const
{
    return contents.all_items_container_top();
}

std::list<item *> item::all_items_container_top()
{
    return contents.all_items_container_top();
}

std::list<const item *> item::all_items_top( pocket_type pk_type ) const
{
    return contents.all_items_top( pk_type );
}

std::list<item *> item::all_items_top( pocket_type pk_type, bool unloading )
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
        return pkt.is_type( pocket_type::CONTAINER );
    } );
    return items.size() == 1 ||
           ( items.size() > 1 &&
    std::all_of( ++items.begin(), items.end(), [&items]( item const * e ) {
        return e->stacks_with( *items.front() );
    } ) );
}

namespace
{
template <class C>
void _aggregate( C &container, typename C::key_type const &key, item const *it,
                 item::aggregate_t *&running_max, item::aggregate_t *&max_type, int depth,
                 int maxdepth )
{
    auto const [iter, emplaced] = container.emplace( key, it );
    if( emplaced && running_max == nullptr ) {
        running_max = &iter->second;
        max_type = running_max;
    } else if( !emplaced ) {
        iter->second.info.bits &=
            iter->second.header->stacks_with( *it, false, false, true, depth, maxdepth ).bits;
        ++iter->second.count;

        if constexpr( std::is_same_v<itype_id, typename C::key_type> ) {
            max_type = max_type->count < iter->second.count ? &iter->second : max_type;
        } else {
            iter->second.info.bits.reset( tname::segments::TYPE );
        }

        running_max = running_max->count < iter->second.count ? &iter->second : running_max;
    }
}
} // namespace

item::aggregate_t item::aggregated_contents( int depth, int maxdepth ) const
{
    constexpr double cutoff = 0.5;

    std::unordered_map<itype_id, aggregate_t> types;
    std::unordered_map<item_category_id, aggregate_t> cats;
    unsigned int total{};
    aggregate_t *running_max{};
    aggregate_t *max_type{};
    auto const cont_and_soft = []( item_pocket const & pkt ) {
        return pkt.is_type( pocket_type::CONTAINER ) || pkt.is_type( pocket_type::E_FILE_STORAGE );
    };


    for( item_pocket const *pk : contents.get_pockets( cont_and_soft ) ) {
        if( pk->is_type( pocket_type::E_FILE_STORAGE ) && ( is_broken() || !is_browsed() ) ) {
            continue;
        }
        for( item const *pkit : pk->all_items_top() ) {
            total++;
            item_category_id const cat = pkit->get_category_of_contents( depth, maxdepth ).get_id();
            bool const type_ok = pkit->type->category_force != item_category_container ||
                                 cat == item_category_container;
            if( type_ok ) {
                _aggregate( types, pkit->typeId(), pkit, running_max, max_type, depth, maxdepth );
            }
            _aggregate( cats, cat, pkit, running_max, max_type, depth, maxdepth );
        }
    }
    if( running_max == nullptr ) {
        return {};
    }

    unsigned int const cutoff_check =
        total < 3 ? total - 1 : static_cast<int>( std::floor( total * cutoff ) );

    // alow a type to still dominate over its own category
    if( max_type->count > cutoff_check &&
        max_type->header->type->category_force != item_category_container &&
        max_type->header->cached_category.id == running_max->header->cached_category.id ) {

        max_type->info.bits.set( tname::segments::TYPE );
        return *max_type;
    }

    return running_max->count > cutoff_check ? *running_max : aggregate_t{};
}

std::list<const item *> item::all_items_ptr() const
{
    std::list<const item *> all_items_internal;
    for( int i = static_cast<int>( pocket_type::CONTAINER );
         i < static_cast<int>( pocket_type::LAST ); i++ ) {
        std::list<const item *> inserted{ all_items_top_recursive( static_cast<pocket_type>( i ) ) };
        all_items_internal.insert( all_items_internal.end(), inserted.begin(), inserted.end() );
    }
    return all_items_internal;
}

std::list<item *> item::all_items_ptr()
{
    std::list<item *> all_items_internal;
    for( int i = static_cast<int>( pocket_type::CONTAINER );
         i < static_cast<int>( pocket_type::LAST ); i++ ) {
        std::list<item *> inserted{ all_items_top_recursive( static_cast<pocket_type>( i ) ) };
        all_items_internal.insert( all_items_internal.end(), inserted.begin(), inserted.end() );
    }
    return all_items_internal;
}

std::list<const item *> item::all_items_ptr( pocket_type pk_type ) const
{
    return all_items_top_recursive( pk_type );
}

std::list<item *> item::all_items_ptr( pocket_type pk_type )
{
    return all_items_top_recursive( pk_type );
}

std::list<const item *> item::all_items_top_recursive( pocket_type pk_type )
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

std::list<item *> item::all_items_top_recursive( pocket_type pk_type )
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

const item *item::get_item_with( const std::function<bool( const item & )> &filter ) const
{
    return contents.get_item_with( filter );
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
