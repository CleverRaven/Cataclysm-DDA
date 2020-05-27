#include "item_contents.h"

#include <algorithm>
#include <memory>

#include "character.h"
#include "enums.h"
#include "game.h"
#include "item.h"
#include "iteminfo_query.h"
#include "itype.h"
#include "item_pocket.h"
#include "map.h"
#include "units.h"

struct tripoint;

static const std::vector<item_pocket::pocket_type> avail_types{
    item_pocket::pocket_type::CONTAINER,
    item_pocket::pocket_type::MAGAZINE,
    item_pocket::pocket_type::MAGAZINE_WELL
};

item_contents::item_contents( const std::vector<pocket_data> &pockets )
{

    for( const pocket_data &data : pockets ) {
        contents.push_back( item_pocket( &data ) );
    }
}

bool item_contents::empty() const
{
    if( contents.empty() ) {
        return true;
    }
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MOD ) ) {
            // item mods aren't really contents per se
            continue;
        }
        if( !pocket.empty() ) {
            return false;
        }
    }
    return true;
}

bool item_contents::empty_container() const
{
    if( contents.empty() ) {
        return true;
    }
    for( const item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        if( !pocket.empty() ) {
            return false;
        }
    }
    return true;
}

bool item_contents::full( bool allow_bucket ) const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER )
            && !pocket.full( allow_bucket ) ) {
            return false;
        }
    }
    return true;
}

size_t item_contents::size() const
{
    return contents.size();
}

void item_contents::combine( const item_contents &read_input )
{
    for( const item_pocket &pocket : read_input.contents ) {
        for( const item *it : pocket.all_items_top() ) {
            const ret_val<bool> inserted = insert_item( *it, pocket.saved_type() );
            if( !inserted.success() ) {
                debugmsg( "error: tried to put an item into a pocket that can't fit into it while loading.  err: %s",
                          inserted.str() );
            }
        }
    }
}

ret_val<item_pocket *> item_contents::find_pocket_for( const item &it,
        item_pocket::pocket_type pk_type )
{
    static item_pocket *null_pocket = nullptr;
    ret_val<item_pocket *> ret = ret_val<item_pocket *>::make_failure( null_pocket,
                                 _( "is not a container" ) );
    for( item_pocket &pocket : contents ) {
        if( !pocket.is_type( pk_type ) ) {
            continue;
        }
        if( pk_type != item_pocket::pocket_type::CONTAINER &&
            pk_type != item_pocket::pocket_type::MAGAZINE &&
            pk_type != item_pocket::pocket_type::MAGAZINE_WELL &&
            pocket.is_type( pk_type ) ) {
            return ret_val<item_pocket *>::make_success( &pocket, "special pocket type override" );
        }
        ret_val<item_pocket::contain_code> ret_contain = pocket.can_contain( it );
        if( ret_contain.success() ) {
            return ret_val<item_pocket *>::make_success( &pocket, ret_contain.str() );
        }
    }
    return ret;
}

ret_val<const item_pocket *> item_contents::find_pocket_for( const item &it,
        item_pocket::pocket_type pk_type ) const
{
    static item_pocket *null_pocket = nullptr;
    ret_val<const item_pocket *> ret = ret_val<const item_pocket *>::make_failure( null_pocket,
                                       _( "is not a container" ) );
    for( const item_pocket &pocket : contents ) {
        if( !pocket.is_type( pk_type ) ) {
            continue;
        }
        ret_val<item_pocket::contain_code> ret_contain = pocket.can_contain( it );
        if( ret_contain.success() ) {
            return ret_val<const item_pocket *>::make_success( &pocket, ret_contain.str() );
        }
    }
    return ret;
}

int item_contents::obtain_cost( const item &it ) const
{
    for( const item_pocket &pocket : contents ) {
        const int mv = pocket.obtain_cost( it );
        if( mv != 0 ) {
            return mv;
        }
    }
    return 0;
}

int item_contents::insert_cost( const item &it ) const
{
    ret_val<const item_pocket *> pocket = find_pocket_for( it, item_pocket::pocket_type::CONTAINER );
    if( pocket.success() ) {
        return pocket.value()->moves();
    } else {
        return -1;
    }
}

ret_val<bool> item_contents::insert_item( const item &it, item_pocket::pocket_type pk_type )
{
    if( pk_type == item_pocket::pocket_type::LAST ) {
        // LAST is invalid, so we assume it will be a regular container
        pk_type = item_pocket::pocket_type::CONTAINER;
    }
    ret_val<item_pocket *> pocket = find_pocket_for( it, pk_type );

    if( pocket.value() == nullptr ) {
        return ret_val<bool>::make_failure( "No success" );
    }

    ret_val<item_pocket::contain_code> pocket_contain_code = pocket.value()->insert_item( it );
    if( pocket_contain_code.success() ) {
        return ret_val<bool>::make_success();
    }
    return ret_val<bool>::make_failure( "No success" );
}

void item_contents::force_insert_item( const item &it, item_pocket::pocket_type pk_type )
{
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( pk_type ) ) {
            pocket.add( it );
            return;
        }
    }
    debugmsg( "ERROR: Could not insert item %s as contents does not have pocket type", it.tname() );
}

void item_contents::fill_with( const item &contained )
{
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            pocket.fill_with( contained );
        }
    }
}

item_pocket *item_contents::best_pocket( const item &it, bool nested )
{
    if( !can_contain( it ).success() ) {
        return nullptr;
    }
    item_pocket *ret = nullptr;
    for( item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            // best pocket is for picking stuff up.
            // containers are the only pockets that are available for such
            continue;
        }
        if( nested && !pocket.rigid() ) {
            continue;
        }
        if( pocket.sealed() ) {
            // we don't want to unseal a pocket to put something in it automatically
            // that needs to be something a player explicitly does
            continue;
        }
        if( ret == nullptr ) {
            if( pocket.can_contain( it ).success() ) {
                ret = &pocket;
            }
        } else if( pocket.can_contain( it ).success() && ret->better_pocket( pocket, it ) ) {
            ret = &pocket;
            for( item *contained : all_items_top( item_pocket::pocket_type::CONTAINER ) ) {
                item_pocket *internal_pocket = contained->contents.best_pocket( it, nested );
                if( internal_pocket != nullptr && ret->better_pocket( pocket, it ) ) {
                    ret = internal_pocket;
                }
            }
        }
    }
    return ret;
}

item_pocket *item_contents::contained_where( const item &contained )
{
    for( item_pocket &pocket : contents ) {
        if( pocket.has_item( contained ) ) {
            return &pocket;
        }
    }
    return nullptr;
}

ret_val<bool> item_contents::can_contain_rigid( const item &it ) const
{
    ret_val<bool> ret = ret_val<bool>::make_failure( _( "is not a container" ) );
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MOD ) ||
            pocket.is_type( item_pocket::pocket_type::CORPSE ) ||
            pocket.is_type( item_pocket::pocket_type::MIGRATION ) ) {
            continue;
        }
        if( !pocket.rigid() ) {
            ret = ret_val<bool>::make_failure( _( "is not rigid" ) );
            continue;
        }
        const ret_val<item_pocket::contain_code> pocket_contain_code = pocket.can_contain( it );
        if( pocket_contain_code.success() ) {
            return ret_val<bool>::make_success();
        }
        ret = ret_val<bool>::make_failure( pocket_contain_code.str() );
    }
    return ret;
}

ret_val<bool> item_contents::can_contain( const item &it ) const
{
    ret_val<bool> ret = ret_val<bool>::make_failure( _( "is not a container" ) );
    for( const item_pocket &pocket : contents ) {
        // mod, migration, corpse, and software aren't regular pockets.
        if( !pocket.is_standard_type() ) {
            continue;
        }
        const ret_val<item_pocket::contain_code> pocket_contain_code = pocket.can_contain( it );
        if( pocket_contain_code.success() ) {
            return ret_val<bool>::make_success();
        }
        ret = ret_val<bool>::make_failure( pocket_contain_code.str() );
    }
    return ret;
}

bool item_contents::can_contain_liquid( bool held_or_ground ) const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) &&
            pocket.can_contain_liquid( held_or_ground ) ) {
            return true;
        }
    }
    return false;
}

bool item_contents::can_unload_liquid() const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) && pocket.can_unload_liquid() ) {
            return true;
        }
    }
    return false;
}

size_t item_contents::num_item_stacks() const
{
    size_t num = 0;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MOD ) ) {
            // mods aren't really a contained item, which this function gets
            continue;
        }
        num += pocket.size();
    }
    return num;
}

void item_contents::on_pickup( Character &guy )
{
    for( item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::MOD ) ) {
            pocket.on_pickup( guy );
        }
    }
}

bool item_contents::spill_contents( const tripoint &pos )
{
    bool spilled = false;
    for( item_pocket &pocket : contents ) {
        spilled = pocket.spill_contents( pos ) || spilled;
    }
    return spilled;
}

void item_contents::overflow( const tripoint &pos )
{
    for( item_pocket &pocket : contents ) {
        pocket.overflow( pos );
    }
}

void item_contents::heat_up()
{
    for( item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        pocket.heat_up();
    }
}

int item_contents::ammo_consume( int qty, const tripoint &pos )
{
    int consumed = 0;
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            // we are assuming only one magazine per well
            if( pocket.empty() ) {
                return 0;
            }
            // assuming only one mag
            item &mag = pocket.front();
            const int res = mag.ammo_consume( qty, pos );
            if( res && mag.ammo_remaining() == 0 ) {
                if( mag.has_flag( "MAG_DESTROY" ) ) {
                    pocket.remove_item( mag );
                } else if( mag.has_flag( "MAG_EJECT" ) ) {
                    g->m.add_item( pos, mag );
                    pocket.remove_item( mag );
                }
            }
            qty -= res;
            consumed += res;
        } else if( pocket.is_type( item_pocket::pocket_type::MAGAZINE ) ) {
            const int res = pocket.ammo_consume( qty );
            consumed += res;
            qty -= res;
        }
    }
    return consumed;
}

item *item_contents::magazine_current()
{
    for( item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            continue;
        }
        item *mag = pocket.magazine_current();
        if( mag != nullptr ) {
            return mag;
        }
    }
    return nullptr;
}

int item_contents::ammo_capacity( const ammotype &ammo ) const
{
    int ret = 0;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE ) ) {
            ret += pocket.ammo_capacity( ammo );
        }
    }
    return ret;
}

std::set<ammotype> item_contents::ammo_types() const
{
    std::set<ammotype> ret;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE ) ) {
            for( const ammotype &ammo : pocket.ammo_types() ) {
                ret.emplace( ammo );
            }
        }
    }
    return ret;
}

item &item_contents::first_ammo()
{
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            return pocket.front().contents.first_ammo();
        }
        if( !pocket.is_type( item_pocket::pocket_type::MAGAZINE ) || pocket.empty() ) {
            continue;
        }
        return pocket.front();
    }
    debugmsg( "Error: Tried to get first ammo in container not containing ammo" );
    return null_item_reference();
}

const item &item_contents::first_ammo() const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            return pocket.front().contents.first_ammo();
        }
        if( !pocket.is_type( item_pocket::pocket_type::MAGAZINE ) || pocket.empty() ) {
            continue;
        }
        return pocket.front();
    }
    debugmsg( "Error: Tried to get first ammo in container not containing ammo" );
    return null_item_reference();
}

bool item_contents::will_spill() const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) && pocket.will_spill() ) {
            return true;
        }
    }
    return false;
}

bool item_contents::spill_open_pockets( Character &guy )
{
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) && pocket.will_spill() ) {
            pocket.handle_liquid_or_spill( guy );
            if( !pocket.empty() ) {
                return false;
            }
        }
    }
    return true;
}

void item_contents::handle_liquid_or_spill( Character &guy )
{
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            pocket.handle_liquid_or_spill( guy );
        }
    }
}

void item_contents::casings_handle( const std::function<bool( item & )> &func )
{
    for( item_pocket &pocket : contents ) {
        pocket.casings_handle( func );
    }
}

void item_contents::clear_items()
{
    for( item_pocket &pocket : contents ) {
        pocket.clear_items();
    }
}

void item_contents::update_open_pockets()
{
    for( item_pocket &pocket : contents ) {
        if( pocket.empty() ) {
            pocket.unseal();
        }
    }
}

void item_contents::set_item_defaults()
{
    /* For Items with a magazine or battery in its contents */
    for( item_pocket &pocket : contents ) {
        if( !pocket.is_standard_type() ) {
            continue;
        }

        pocket.set_item_defaults();
    }
}

bool item_contents::seal_all_pockets()
{
    bool any_sealed = false;
    for( item_pocket &pocket : contents ) {
        any_sealed = pocket.seal() || any_sealed;
    }
    return any_sealed;
}

void item_contents::migrate_item( item &obj, const std::set<itype_id> &migrations )
{
    for( item_pocket &pocket : contents ) {
        pocket.migrate_item( obj, migrations );
    }
}

bool item_contents::has_pocket_type( const item_pocket::pocket_type pk_type ) const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( pk_type ) ) {
            return true;
        }
    }
    return false;
}

bool item_contents::has_any_with( const std::function<bool( const item &it )> &filter,
                                  item_pocket::pocket_type pk_type ) const
{
    for( const item_pocket &pocket : contents ) {
        if( !pocket.is_type( pk_type ) ) {
            continue;
        }
        if( pocket.has_any_with( filter ) ) {
            return true;
        }
    }
    return false;
}

bool item_contents::stacks_with( const item_contents &rhs ) const
{
    if( contents.size() != rhs.contents.size() ) {
        return false;
    }
    return ( empty() && rhs.empty() ) ||
           std::equal( contents.begin(), contents.end(),
                       rhs.contents.begin(),
    []( const item_pocket & a, const item_pocket & b ) {
        return a.stacks_with( b );
    } );
}

bool item_contents::same_contents( const item_contents &rhs ) const
{
    if( contents.size() != rhs.contents.size() ) {
        return false;
    }
    return std::equal( contents.begin(), contents.end(),
                       rhs.contents.begin(), rhs.contents.end(),
    []( const item_pocket & a, const item_pocket & b ) {
        return a.same_contents( b );
    } );
}

bool item_contents::is_funnel_container( units::volume &bigger_than ) const
{
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            if( pocket.is_funnel_container( bigger_than ) ) {
                return true;
            }
        }
    }
    return false;
}

item &item_contents::only_item()
{
    if( num_item_stacks() != 1 ) {
        debugmsg( "ERROR: item_contents::only_item called with %d items contained", num_item_stacks() );
        return null_item_reference();
    }
    for( item_pocket &pocket : contents ) {
        if( pocket.empty() || !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        // the first item we come to is the only one.
        return pocket.front();
    }
    return null_item_reference();
}

const item &item_contents::only_item() const
{
    if( num_item_stacks() != 1 ) {
        debugmsg( "ERROR: item_contents::only_item called with %d items contained", num_item_stacks() );
        return null_item_reference();
    }
    for( const item_pocket &pocket : contents ) {
        if( pocket.empty() || !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        // the first item we come to is the only one.
        return pocket.front();
    }
    return null_item_reference();
}

item *item_contents::get_item_with( const std::function<bool( const item &it )> &filter )
{
    for( item_pocket &pocket : contents ) {
        item *it = pocket.get_item_with( filter );
        if( it != nullptr ) {
            return it;
        }
    }
    return nullptr;
}

void item_contents::remove_items_if( const std::function<bool( item & )> &filter )
{
    for( item_pocket &pocket : contents ) {
        pocket.remove_items_if( filter );
    }
}

std::list<item *> item_contents::all_items_top( item_pocket::pocket_type pk_type )
{
    std::list<item *> all_items_internal;
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( pk_type ) ) {
            std::list<item *> contained_items = pocket.all_items_top();
            all_items_internal.insert( all_items_internal.end(), contained_items.begin(),
                                       contained_items.end() );
        }
    }
    return all_items_internal;
}

std::list<const item *> item_contents::all_items_top( item_pocket::pocket_type pk_type ) const
{
    std::list<const item *> all_items_internal;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( pk_type ) ) {
            std::list<const item *> contained_items = pocket.all_items_top();
            all_items_internal.insert( all_items_internal.end(), contained_items.begin(),
                                       contained_items.end() );
        }
    }
    return all_items_internal;
}

std::list<item *> item_contents::all_items_top()
{
    std::list<item *> ret;
    for( const item_pocket::pocket_type pk_type : avail_types ) {
        std::list<item *> top{ all_items_top( pk_type ) };
        ret.insert( ret.end(), top.begin(), top.end() );
    }
    return ret;
}

std::list<const item *> item_contents::all_items_top() const
{
    std::list<const item *> ret;
    for( const item_pocket::pocket_type pk_type : avail_types ) {
        std::list<const item *> top{ all_items_top( pk_type ) };
        ret.insert( ret.end(), top.begin(), top.end() );
    }
    return ret;
}

std::list<item *> item_contents::all_items_ptr( item_pocket::pocket_type pk_type )
{
    std::list<item *> all_items_internal;
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( pk_type ) ) {
            std::list<item *> contained_items = pocket.all_items_ptr( pk_type );
            all_items_internal.insert( all_items_internal.end(), contained_items.begin(),
                                       contained_items.end() );
        }
    }
    return all_items_internal;
}

std::list<const item *> item_contents::all_items_ptr( item_pocket::pocket_type pk_type ) const
{
    std::list<const item *> all_items_internal;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( pk_type ) ) {
            std::list<const item *> contained_items = pocket.all_items_ptr( pk_type );
            all_items_internal.insert( all_items_internal.end(), contained_items.begin(),
                                       contained_items.end() );
        }
    }
    return all_items_internal;
}

std::list<const item *> item_contents::all_items_ptr() const
{
    std::list<const item *> all_items_internal;
    for( int i = item_pocket::pocket_type::CONTAINER; i < item_pocket::pocket_type::LAST; i++ ) {
        std::list<const item *> inserted{ all_items_ptr( static_cast<item_pocket::pocket_type>( i ) ) };
        all_items_internal.insert( all_items_internal.end(), inserted.begin(), inserted.end() );
    }
    return all_items_internal;
}

item &item_contents::legacy_front()
{
    if( empty() ) {
        debugmsg( "naively asked for first content item and will get a nullptr" );
        return null_item_reference();
    }
    return *all_items_top().front();
}

const item &item_contents::legacy_front() const
{
    if( empty() ) {
        debugmsg( "naively asked for first content item and will get a nullptr" );
        return null_item_reference();
    }
    return *all_items_top().front();
}

std::vector<item *> item_contents::gunmods()
{
    std::vector<item *> mods;
    for( item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MOD ) ) {
            std::vector<item *> internal_mods{ pocket.gunmods() };
            mods.insert( mods.end(), internal_mods.begin(), internal_mods.end() );
        }
    }
    return mods;
}

std::vector<const item *> item_contents::gunmods() const
{
    std::vector<const item *> mods;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MOD ) ) {
            std::vector<const item *> internal_mods{ pocket.gunmods() };
            mods.insert( mods.end(), internal_mods.begin(), internal_mods.end() );
        }
    }
    return mods;
}

std::set<itype_id> item_contents::magazine_compatible() const
{
    std::set<itype_id> ret;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
            for( const itype_id &id : pocket.item_type_restrictions() ) {
                ret.emplace( id );
            }
        }
    }
    return ret;
}

units::mass item_contents::total_container_weight_capacity() const
{
    units::mass total_weight = 0_gram;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            total_weight += pocket.weight_capacity();
        }
    }
    return total_weight;
}

units::volume item_contents::total_container_capacity() const
{
    units::volume total_vol = 0_ml;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            total_vol += pocket.volume_capacity();
        }
    }
    return total_vol;
}

units::volume item_contents::remaining_container_capacity() const
{
    units::volume total_vol = 0_ml;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            total_vol += pocket.remaining_volume();
        }
    }
    return total_vol;
}

units::volume item_contents::total_contained_volume() const
{
    units::volume total_vol = 0_ml;
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            total_vol += pocket.contains_volume();
        }
    }
    return total_vol;
}

void item_contents::remove_rotten( const tripoint &pnt )
{
    for( item_pocket &pocket : contents ) {
        // no reason to check mods, they won't rot
        if( !pocket.is_type( item_pocket::pocket_type::MOD ) ) {
            pocket.remove_rotten( pnt );
        }
    }
}

void item_contents::remove_internal( const std::function<bool( item & )> &filter,
                                     int &count, std::list<item> &res )
{
    for( item_pocket &pocket : contents ) {
        if( pocket.remove_internal( filter, count, res ) ) {
            return;
        }
    }
}

void item_contents::process( player *carrier, const tripoint &pos, bool activate, float insulation,
                             temperature_flag flag, float spoil_multiplier_parent )
{
    for( item_pocket &pocket : contents ) {
        // no reason to check mods, they won't rot
        if( !pocket.is_type( item_pocket::pocket_type::MOD ) ) {
            pocket.process( carrier, pos, activate, insulation, flag, spoil_multiplier_parent );
        }
    }
}

int item_contents::remaining_capacity_for_liquid( const item &liquid ) const
{
    int charges_of_liquid = 0;
    item liquid_copy = liquid;
    liquid_copy.charges = 1;
    for( const item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        charges_of_liquid += pocket.remaining_capacity_for_item( liquid );
    }
    return charges_of_liquid;
}

float item_contents::relative_encumbrance() const
{
    units::volume nonrigid_max_volume;
    units::volume nonrigid_volume;
    for( const item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        if( pocket.rigid() ) {
            continue;
        }
        nonrigid_volume += pocket.contains_volume();
        nonrigid_max_volume += pocket.max_contains_volume();
    }
    if( nonrigid_volume > nonrigid_max_volume ) {
        debugmsg( "volume exceeds capacity (%sml > %sml)",
                  to_milliliter( nonrigid_volume ), to_milliliter( nonrigid_max_volume ) );
        return 1;
    }
    if( nonrigid_max_volume == 0_ml ) {
        return 0;
    }
    return nonrigid_volume * 1.0 / nonrigid_max_volume;
}

bool item_contents::all_pockets_rigid() const
{
    for( const item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            continue;
        }
        if( !pocket.rigid() ) {
            return false;
        }
    }
    return true;
}

units::volume item_contents::item_size_modifier() const
{
    units::volume total_vol = 0_ml;
    for( const item_pocket &pocket : contents ) {
        total_vol += pocket.item_size_modifier();
    }
    return total_vol;
}

units::mass item_contents::item_weight_modifier() const
{
    units::mass total_mass = 0_gram;
    for( const item_pocket &pocket : contents ) {
        total_mass += pocket.item_weight_modifier();
    }
    return total_mass;
}

int item_contents::best_quality( const quality_id &id ) const
{
    int ret = 0;
    for( const item_pocket &pocket : contents ) {
        ret = std::max( pocket.best_quality( id ), ret );
    }
    return ret;
}

static void insert_separation_line( std::vector<iteminfo> &info )
{
    if( info.empty() || info.back().sName != "--" ) {
        info.push_back( iteminfo( "DESCRIPTION", "--" ) );
    }
}

void item_contents::info( std::vector<iteminfo> &info, const iteminfo_query *parts ) const
{
    int pocket_number = 1;
    std::vector<iteminfo> contents_info;
    std::vector<item_pocket> found_pockets;
    std::map<int, int> pocket_num; // index, amount
    for( const item_pocket &pocket : contents ) {
        if( pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            bool found = false;
            int idx = 0;
            for( const item_pocket &found_pocket : found_pockets ) {
                if( found_pocket == pocket ) {
                    found = true;
                    pocket_num[idx]++;
                }
                idx++;
            }
            if( !found ) {
                found_pockets.push_back( pocket );
                pocket_num[idx]++;
            }
            pocket.contents_info( contents_info, pocket_number++, contents.size() != 1 );
        }
    }
    if( parts->test( iteminfo_parts::DESCRIPTION_POCKETS ) ) {
        // If multiple pockets and/or multiple kinds of pocket, show total capacity section
        if( found_pockets.size() > 1 || pocket_num[0] > 1 ) {
            insert_separation_line( info );
            info.emplace_back( "CONTAINER", _( "<bold>Total capacity</bold>:" ) );
            info.push_back( vol_to_info( "CONTAINER", _( "Volume: " ), total_container_capacity() ) );
            info.push_back( weight_to_info( "CONTAINER", _( "  Weight: " ),
                                            total_container_weight_capacity() ) );
            info.back().bNewLine = true;
        }

        int idx = 0;
        for( const item_pocket &pocket : found_pockets ) {
            insert_separation_line( info );
            // If there are multiple similar pockets, show their capacity as a set
            if( pocket_num[idx] > 1 ) {
                info.emplace_back( "DESCRIPTION", string_format( _( "<bold>%d Pockets</bold> with capacity:" ),
                                   pocket_num[idx] ) );
            } else {
                // If this is the only pocket the item has, label it "Total capacity"
                // Otherwise, give it a generic "Pocket" heading (is one of several pockets)
                bool only_one_pocket = ( found_pockets.size() == 1 && pocket_num[0] == 1 );
                if( only_one_pocket ) {
                    info.emplace_back( "DESCRIPTION", _( "<bold>Total capacity</bold>:" ) );
                } else {
                    info.emplace_back( "DESCRIPTION", _( "<bold>Pocket</bold> with capacity:" ) );
                }
            }
            idx++;
            pocket.general_info( info, idx, false );
        }
    }
    if( parts->test( iteminfo_parts::DESCRIPTION_CONTENTS ) ) {
        info.insert( info.end(), contents_info.begin(), contents_info.end() );
    }
}
