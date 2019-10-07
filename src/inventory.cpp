#include "inventory.h"

#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <iterator>
#include <memory>

#include "avatar.h"
#include "debug.h"
#include "game.h"
#include "iexamine.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h" //for rust message
#include "npc.h"
#include "options.h"
#include "translations.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "calendar.h"
#include "character.h"
#include "damage.h"
#include "enums.h"
#include "optional.h"
#include "player.h"
#include "rng.h"
#include "material.h"
#include "type_id.h"
#include "colony.h"
#include "flat_set.h"
#include "point.h"

struct itype;

const invlet_wrapper
inv_chars( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#&()+.:;=@[\\]^_{|}" );

bool invlet_wrapper::valid( const int invlet ) const
{
    if( invlet > std::numeric_limits<char>::max() || invlet < std::numeric_limits<char>::min() ) {
        return false;
    }
    return find( static_cast<char>( invlet ) ) != std::string::npos;
}

invlet_favorites::invlet_favorites( const std::unordered_map<itype_id, std::string> &map )
{
    for( const auto &p : map ) {
        if( p.second.empty() ) {
            // The map gradually accumulates empty lists; remove those here
            continue;
        }
        invlets_by_id.insert( p );
        for( char invlet : p.second ) {
            uint8_t invlet_u = invlet;
            if( !ids_by_invlet[invlet_u].empty() ) {
                debugmsg( "Duplicate invlet: %s and %s both mapped to %c",
                          ids_by_invlet[invlet_u], p.first, invlet );
            }
            ids_by_invlet[invlet_u] = p.first;
        }
    }
}

void invlet_favorites::set( char invlet, const itype_id &id )
{
    if( contains( invlet, id ) ) {
        return;
    }
    erase( invlet );
    uint8_t invlet_u = invlet;
    ids_by_invlet[invlet_u] = id;
    invlets_by_id[id].push_back( invlet );
}

void invlet_favorites::erase( char invlet )
{
    uint8_t invlet_u = invlet;
    const std::string &id = ids_by_invlet[invlet_u];
    if( id.empty() ) {
        return;
    }
    std::string &invlets = invlets_by_id[id];
    std::string::iterator it = std::find( invlets.begin(), invlets.end(), invlet );
    invlets.erase( it );
    ids_by_invlet[invlet_u].clear();
}

bool invlet_favorites::contains( char invlet, const itype_id &id ) const
{
    uint8_t invlet_u = invlet;
    return ids_by_invlet[invlet_u] == id;
}

std::string invlet_favorites::invlets_for( const itype_id &id ) const
{
    auto map_iterator = invlets_by_id.find( id );
    if( map_iterator == invlets_by_id.end() ) {
        return {};
    }
    return map_iterator->second;
}

const std::unordered_map<itype_id, std::string> &
invlet_favorites::get_invlets_by_id() const
{
    return invlets_by_id;
}

inventory::inventory() = default;

invslice inventory::slice()
{
    invslice stacks;
    for( auto &elem : items ) {
        stacks.push_back( &elem );
    }
    return stacks;
}

const_invslice inventory::const_slice() const
{
    const_invslice stacks;
    for( const auto &item : items ) {
        stacks.push_back( &item );
    }
    return stacks;
}

const std::list<item> &inventory::const_stack( int i ) const
{
    if( i < 0 || i >= static_cast<int>( items.size() ) ) {
        debugmsg( "Attempted to access stack %d in an inventory (size %d)", i, items.size() );
        static const std::list<item> nullstack{};
        return nullstack;
    }

    invstack::const_iterator iter = items.begin();
    for( int j = 0; j < i; ++j ) {
        ++iter;
    }
    return *iter;
}

size_t inventory::size() const
{
    return items.size();
}

inventory &inventory::operator+= ( const inventory &rhs )
{
    for( size_t i = 0; i < rhs.size(); i++ ) {
        push_back( rhs.const_stack( i ) );
    }
    return *this;
}

inventory &inventory::operator+= ( const std::list<item> &rhs )
{
    for( const auto &rh : rhs ) {
        add_item( rh, false, false );
    }
    return *this;
}

inventory &inventory::operator+= ( const std::vector<item> &rhs )
{
    for( const auto &rh : rhs ) {
        add_item( rh, true );
    }
    return *this;
}

inventory &inventory::operator+= ( const item &rhs )
{
    add_item( rhs );
    return *this;
}

inventory &inventory::operator+= ( const item_stack &rhs )
{
    for( const auto &p : rhs ) {
        if( !p.made_of( LIQUID ) ) {
            add_item( p, true );
        }
    }
    return *this;
}

inventory inventory::operator+ ( const inventory &rhs )
{
    return inventory( *this ) += rhs;
}

inventory inventory::operator+ ( const std::list<item> &rhs )
{
    return inventory( *this ) += rhs;
}

inventory inventory::operator+ ( const item &rhs )
{
    return inventory( *this ) += rhs;
}

void inventory::unsort()
{
    binned = false;
}

static bool stack_compare( const std::list<item> &lhs, const std::list<item> &rhs )
{
    return lhs.front() < rhs.front();
}

void inventory::clear()
{
    items.clear();
    binned = false;
}

void inventory::push_back( const std::list<item> &newits )
{
    for( const auto &newit : newits ) {
        add_item( newit, true );
    }
}

// This function keeps the invlet cache updated when a new item is added.
void inventory::update_cache_with_item( item &newit )
{
    // This function does two things:
    // 1. It adds newit's invlet to the list of favorite letters for newit's item type.
    // 2. It removes newit's invlet from the list of favorite letters for all other item types.

    // no invlet item, just return.
    // TODO: Should we instead remember that the invlet was cleared?
    if( newit.invlet == 0 ) {
        return;
    }

    invlet_cache.set( newit.invlet, newit.typeId() );
}

char inventory::find_usable_cached_invlet( const std::string &item_type )
{
    // Some of our preferred letters might already be used.
    for( auto invlet : invlet_cache.invlets_for( item_type ) ) {
        // Don't overwrite user assignments.
        if( assigned_invlet.count( invlet ) ) {
            continue;
        }
        // Check if anything is using this invlet.
        if( g->u.invlet_to_position( invlet ) != INT_MIN ) {
            continue;
        }
        return invlet;
    }

    return 0;
}

item &inventory::add_item( item newit, bool keep_invlet, bool assign_invlet, bool should_stack )
{
    binned = false;

    if( should_stack ) {
        // See if we can't stack this item.
        for( auto &elem : items ) {
            std::list<item>::iterator it_ref = elem.begin();
            if( it_ref->stacks_with( newit ) ) {
                if( it_ref->merge_charges( newit ) ) {
                    return *it_ref;
                }
                if( it_ref->invlet == '\0' ) {
                    if( !keep_invlet ) {
                        update_invlet( newit, assign_invlet );
                    }
                    update_cache_with_item( newit );
                    it_ref->invlet = newit.invlet;
                } else {
                    newit.invlet = it_ref->invlet;
                }
                elem.push_back( newit );
                return elem.back();
            } else if( keep_invlet && assign_invlet && it_ref->invlet == newit.invlet ) {
                // If keep_invlet is true, we'll be forcing other items out of their current invlet.
                assign_empty_invlet( *it_ref, g->u );
            }
        }
    }

    // Couldn't stack the item, proceed.
    if( !keep_invlet ) {
        update_invlet( newit, assign_invlet );
    }
    update_cache_with_item( newit );

    std::list<item> newstack;
    newstack.push_back( newit );
    items.push_back( newstack );
    return items.back().back();
}

void inventory::add_item_keep_invlet( item newit )
{
    add_item( newit, true );
}

void inventory::push_back( item newit )
{
    add_item( newit );
}

#if defined(__ANDROID__)
extern void remove_stale_inventory_quick_shortcuts();
#endif

void inventory::restack( player &p )
{
    // tasks that the old restack seemed to do:
    // 1. reassign inventory letters
    // 2. remove items from non-matching stacks
    // 3. combine matching stacks

    binned = false;
    std::list<item> to_restack;
    int idx = 0;
    for( invstack::iterator iter = items.begin(); iter != items.end(); ++iter, ++idx ) {
        std::list<item> &stack = *iter;
        item &topmost = stack.front();

        const int ipos = p.invlet_to_position( topmost.invlet );
        if( !inv_chars.valid( topmost.invlet ) || ( ipos != INT_MIN && ipos != idx ) ) {
            assign_empty_invlet( topmost, p );
            for( auto &stack_iter : stack ) {
                stack_iter.invlet = topmost.invlet;
            }
        }

        // remove non-matching items, stripping off end of stack so the first item keeps the invlet.
        while( stack.size() > 1 && !topmost.stacks_with( stack.back() ) ) {
            to_restack.splice( to_restack.begin(), *iter, --stack.end() );
        }
    }

    // combine matching stacks
    // separate loop to ensure that ALL stacks are homogeneous
    for( invstack::iterator iter = items.begin(); iter != items.end(); ++iter ) {
        for( invstack::iterator other = iter; other != items.end(); ++other ) {
            if( iter != other && iter->front().stacks_with( other->front() ) ) {
                if( other->front().count_by_charges() ) {
                    iter->front().charges += other->front().charges;
                } else {
                    iter->splice( iter->begin(), *other );
                }
                other = items.erase( other );
                --other;
            }
        }
    }

    //re-add non-matching items
    for( auto &elem : to_restack ) {
        add_item( elem );
    }

    //Ensure that all items in the same stack have the same invlet.
    for( std::list< item > &outer : items ) {
        for( item &inner : outer ) {
            inner.invlet = outer.front().invlet;
        }
    }
    items.sort( stack_compare );

#if defined(__ANDROID__)
    remove_stale_inventory_quick_shortcuts();
#endif
}

static int count_charges_in_list( const itype *type, const map_stack &items )
{
    for( const auto &candidate : items ) {
        if( candidate.type == type ) {
            return candidate.charges;
        }
    }
    return 0;
}

void inventory::form_from_map( const tripoint &origin, int range, const player *pl,
                               bool assign_invlet,
                               bool clear_path )
{
    form_from_map( g->m, origin, range, pl, assign_invlet, clear_path );
}

void inventory::form_from_map( map &m, const tripoint &origin, int range, const player *pl,
                               bool assign_invlet,
                               bool clear_path )
{
    // populate a grid of spots that can be reached
    std::vector<tripoint> reachable_pts = {};
    // If we need a clear path we care about the reachability of points
    if( clear_path ) {
        m.reachable_flood_steps( reachable_pts, origin, range, 1, 100 );
    } else {
        // Fill reachable points with points_in_radius
        tripoint_range in_radius = m.points_in_radius( origin, range );
        for( const tripoint &p : in_radius ) {
            reachable_pts.emplace_back( p );
        }
    }

    items.clear();
    for( const tripoint &p : reachable_pts ) {
        if( m.has_furn( p ) ) {
            const furn_t &f = m.furn( p ).obj();
            const itype *type = f.crafting_pseudo_item_type();
            if( type != nullptr ) {
                const itype *ammo = f.crafting_ammo_item_type();
                item furn_item( type, calendar::turn, 0 );
                furn_item.item_tags.insert( "PSEUDO" );
                furn_item.charges = ammo ? count_charges_in_list( ammo, m.i_at( p ) ) : 0;
                add_item( furn_item );
            }
        }
        if( m.accessible_items( p ) ) {
            for( auto &i : m.i_at( p ) ) {
                // if its *the* player requesting this from from map inventory
                // then dont allow items owned by another faction to be factored into recipe components etc.
                if( pl && !i.is_owned_by( *pl, true ) ) {
                    continue;
                }
                if( !i.made_of( LIQUID ) ) {
                    add_item( i, false, assign_invlet );
                }
            }
        }
        // Kludges for now!
        if( m.has_nearby_fire( p, 0 ) ) {
            item fire( "fire", 0 );
            fire.charges = 1;
            add_item( fire );
        }
        // Handle any water from infinite map sources.
        item water = m.water_from( p );
        if( !water.is_null() ) {
            add_item( water );
        }
        // kludge that can probably be done better to check specifically for toilet water to use in
        // crafting
        if( m.furn( p ).obj().examine == &iexamine::toilet ) {
            // get water charges at location
            auto toilet = m.i_at( p );
            auto water = toilet.end();
            for( auto candidate = toilet.begin(); candidate != toilet.end(); ++candidate ) {
                if( candidate->typeId() == "water" ) {
                    water = candidate;
                    break;
                }
            }
            if( water != toilet.end() && water->charges > 0 ) {
                add_item( *water );
            }
        }

        // keg-kludge
        if( m.furn( p ).obj().examine == &iexamine::keg ) {
            auto liq_contained = m.i_at( p );
            for( auto &i : liq_contained ) {
                if( i.made_of( LIQUID ) ) {
                    add_item( i );
                }
            }
        }

        // WARNING: The part below has a bug that's currently quite minor
        // When a vehicle has multiple faucets in range, available water is
        //  multiplied by the number of faucets.
        // Same thing happens for all other tools and resources, but not cargo
        const optional_vpart_position vp = m.veh_at( p );
        if( !vp ) {
            continue;
        }
        vehicle *const veh = &vp->vehicle();

        //Adds faucet to kitchen stuff; may be horribly wrong to do such....
        //ShouldBreak into own variable
        const cata::optional<vpart_reference> kpart = vp.part_with_feature( "KITCHEN", true );
        const cata::optional<vpart_reference> faupart = vp.part_with_feature( "FAUCET", true );
        const cata::optional<vpart_reference> weldpart = vp.part_with_feature( "WELDRIG", true );
        const cata::optional<vpart_reference> craftpart = vp.part_with_feature( "CRAFTRIG", true );
        const cata::optional<vpart_reference> forgepart = vp.part_with_feature( "FORGE", true );
        const cata::optional<vpart_reference> kilnpart = vp.part_with_feature( "KILN", true );
        const cata::optional<vpart_reference> chempart = vp.part_with_feature( "CHEMLAB", true );
        const cata::optional<vpart_reference> cargo = vp.part_with_feature( "CARGO", true );

        if( cargo ) {
            const auto items = veh->get_items( cargo->part_index() );
            *this += std::list<item>( items.begin(), items.end() );
        }

        if( faupart ) {
            for( const auto &it : veh->fuels_left() ) {
                item fuel( it.first, 0 );
                if( fuel.made_of( LIQUID ) ) {
                    fuel.charges = it.second;
                    add_item( fuel );
                }
            }
        }

        if( kpart ) {
            item hotplate( "hotplate", 0 );
            hotplate.charges = veh->fuel_left( "battery", true );
            hotplate.item_tags.insert( "PSEUDO" );
            add_item( hotplate );

            item pot( "pot", 0 );
            pot.item_tags.insert( "PSEUDO" );
            add_item( pot );
            item pan( "pan", 0 );
            pan.item_tags.insert( "PSEUDO" );
            add_item( pan );
        }
        if( weldpart ) {
            item welder( "welder", 0 );
            welder.charges = veh->fuel_left( "battery", true );
            welder.item_tags.insert( "PSEUDO" );
            add_item( welder );

            item soldering_iron( "soldering_iron", 0 );
            soldering_iron.charges = veh->fuel_left( "battery", true );
            soldering_iron.item_tags.insert( "PSEUDO" );
            add_item( soldering_iron );
        }
        if( craftpart ) {
            item vac_sealer( "vac_sealer", 0 );
            vac_sealer.charges = veh->fuel_left( "battery", true );
            vac_sealer.item_tags.insert( "PSEUDO" );
            add_item( vac_sealer );

            item dehydrator( "dehydrator", 0 );
            dehydrator.charges = veh->fuel_left( "battery", true );
            dehydrator.item_tags.insert( "PSEUDO" );
            add_item( dehydrator );

            item food_processor( "food_processor", 0 );
            food_processor.charges = veh->fuel_left( "battery", true );
            food_processor.item_tags.insert( "PSEUDO" );
            add_item( food_processor );

            item press( "press", 0 );
            press.charges = veh->fuel_left( "battery", true );
            press.item_tags.insert( "PSEUDO" );
            add_item( press );
        }
        if( forgepart ) {
            item forge( "forge", 0 );
            forge.charges = veh->fuel_left( "battery", true );
            forge.item_tags.insert( "PSEUDO" );
            add_item( forge );
        }
        if( kilnpart ) {
            item kiln( "kiln", 0 );
            kiln.charges = veh->fuel_left( "battery", true );
            kiln.item_tags.insert( "PSEUDO" );
            add_item( kiln );
        }
        if( chempart ) {
            item hotplate( "hotplate", 0 );
            hotplate.charges = veh->fuel_left( "battery", true );
            hotplate.item_tags.insert( "PSEUDO" );
            add_item( hotplate );

            item chemistry_set( "chemistry_set", 0 );
            chemistry_set.charges = veh->fuel_left( "battery", true );
            chemistry_set.item_tags.insert( "PSEUDO" );
            add_item( chemistry_set );
        }
    }
    reachable_pts.clear();
}

std::list<item> inventory::reduce_stack( const int position, const int quantity )
{
    int pos = 0;
    std::list<item> ret;
    for( invstack::iterator iter = items.begin(); iter != items.end(); ++iter ) {
        if( position == pos ) {
            binned = false;
            if( quantity >= static_cast<int>( iter->size() ) || quantity < 0 ) {
                ret = *iter;
                items.erase( iter );
            } else {
                for( int i = 0 ; i < quantity ; i++ ) {
                    ret.push_back( remove_item( &iter->front() ) );
                }
            }
            break;
        }
        ++pos;
    }
    return ret;
}

item inventory::remove_item( const item *it )
{
    auto tmp = remove_items_with( [&it]( const item & i ) {
        return &i == it;
    }, 1 );
    if( !tmp.empty() ) {
        binned = false;
        return tmp.front();
    }
    debugmsg( "Tried to remove a item not in inventory." );
    return item();
}

item inventory::remove_item( const int position )
{
    int pos = 0;
    for( invstack::iterator iter = items.begin(); iter != items.end(); ++iter ) {
        if( position == pos ) {
            binned = false;
            if( iter->size() > 1 ) {
                std::list<item>::iterator stack_member = iter->begin();
                char invlet = stack_member->invlet;
                ++stack_member;
                stack_member->invlet = invlet;
            }
            item ret = iter->front();
            iter->erase( iter->begin() );
            if( iter->empty() ) {
                items.erase( iter );
            }
            return ret;
        }
        ++pos;
    }

    return item();
}

std::list<item> inventory::remove_randomly_by_volume( const units::volume &volume )
{
    std::list<item> result;
    units::volume volume_dropped = 0_ml;
    while( volume_dropped < volume ) {
        units::volume cumulative_volume = 0_ml;
        auto chosen_stack = items.begin();
        auto chosen_item = chosen_stack->begin();
        for( auto stack = items.begin(); stack != items.end(); ++stack ) {
            for( auto stack_it = stack->begin(); stack_it != stack->end(); ++stack_it ) {
                cumulative_volume += stack_it->volume();
                if( x_in_y( stack_it->volume().value(), cumulative_volume.value() ) ) {
                    chosen_item = stack_it;
                    chosen_stack = stack;
                }
            }
        }
        volume_dropped += chosen_item->volume();
        result.push_back( std::move( *chosen_item ) );
        chosen_item = chosen_stack->erase( chosen_item );
        if( chosen_item == chosen_stack->begin() && !chosen_stack->empty() ) {
            // preserve the invlet when removing the first item of a stack
            chosen_item->invlet = result.back().invlet;
        }
        if( chosen_stack->empty() ) {
            binned = false;
            items.erase( chosen_stack );
        }
    }
    return result;
}

void inventory::dump( std::vector<item *> &dest )
{
    for( auto &elem : items ) {
        for( auto &elem_stack_iter : elem ) {
            dest.push_back( &elem_stack_iter );
        }
    }
}

const item &inventory::find_item( int position ) const
{
    if( position < 0 || position >= static_cast<int>( items.size() ) ) {
        return null_item_reference();
    }
    invstack::const_iterator iter = items.begin();
    for( int j = 0; j < position; ++j ) {
        ++iter;
    }
    return iter->front();
}

item &inventory::find_item( int position )
{
    return const_cast<item &>( const_cast<const inventory *>( this )->find_item( position ) );
}

int inventory::invlet_to_position( char invlet ) const
{
    int i = 0;
    for( const auto &elem : items ) {
        if( elem.begin()->invlet == invlet ) {
            return i;
        }
        ++i;
    }
    return INT_MIN;
}

int inventory::position_by_item( const item *it ) const
{
    int p = 0;
    for( const auto &stack : items ) {
        for( const auto &e : stack ) {
            if( e.has_item( *it ) ) {
                return p;
            }
        }
        p++;
    }
    return INT_MIN;
}

int inventory::position_by_type( const itype_id &type ) const
{
    int i = 0;
    for( auto &elem : items ) {
        if( elem.front().typeId() == type ) {
            return i;
        }
        ++i;
    }
    return INT_MIN;
}

std::list<item> inventory::use_amount( itype_id it, int quantity,
                                       const std::function<bool( const item & )> &filter )
{
    items.sort( stack_compare );
    std::list<item> ret;
    for( invstack::iterator iter = items.begin(); iter != items.end() && quantity > 0; /* noop */ ) {
        for( std::list<item>::iterator stack_iter = iter->begin();
             stack_iter != iter->end() && quantity > 0;
             /* noop */ ) {
            if( stack_iter->use_amount( it, quantity, ret, filter ) ) {
                stack_iter = iter->erase( stack_iter );
            } else {
                ++stack_iter;
            }
        }
        if( iter->empty() ) {
            binned = false;
            iter = items.erase( iter );
        } else if( iter != items.end() ) {
            ++iter;
        }
    }
    return ret;
}

bool inventory::has_tools( const itype_id &it, int quantity,
                           const std::function<bool( const item & )> &filter ) const
{
    return has_amount( it, quantity, true, filter );
}

bool inventory::has_components( const itype_id &it, int quantity,
                                const std::function<bool( const item & )> &filter ) const
{
    return has_amount( it, quantity, false, filter );
}

bool inventory::has_charges( const itype_id &it, int quantity,
                             const std::function<bool( const item & )> &filter ) const
{
    return ( charges_of( it, INT_MAX, filter ) >= quantity );
}

int inventory::leak_level( const std::string &flag ) const
{
    int ret = 0;

    for( const auto &elem : items ) {
        for( const auto &elem_stack_iter : elem ) {
            if( elem_stack_iter.has_flag( flag ) ) {
                if( elem_stack_iter.has_flag( "LEAK_ALWAYS" ) ) {
                    ret += elem_stack_iter.volume() / units::legacy_volume_factor;
                } else if( elem_stack_iter.has_flag( "LEAK_DAM" ) && elem_stack_iter.damage() > 0 ) {
                    ret += elem_stack_iter.damage_level( 4 );
                }
            }
        }
    }
    return ret;
}

int inventory::worst_item_value( npc *p ) const
{
    int worst = 99999;
    for( const auto &elem : items ) {
        const item &it = elem.front();
        int val = p->value( it );
        if( val < worst ) {
            worst = val;
        }
    }
    return worst;
}

bool inventory::has_enough_painkiller( int pain ) const
{
    for( const auto &elem : items ) {
        const item &it = elem.front();
        if( ( pain <= 35 && it.typeId() == "aspirin" ) ||
            ( pain >= 50 && it.typeId() == "oxycodone" ) ||
            it.typeId() == "tramadol" || it.typeId() == "codeine" ) {
            return true;
        }
    }
    return false;
}

item *inventory::most_appropriate_painkiller( int pain )
{
    int difference = 9999;
    item *ret = &null_item_reference();
    for( auto &elem : items ) {
        int diff = 9999;
        itype_id type = elem.front().typeId();
        if( type == "aspirin" ) {
            diff = abs( pain - 15 );
        } else if( type == "codeine" ) {
            diff = abs( pain - 30 );
        } else if( type == "oxycodone" ) {
            diff = abs( pain - 60 );
        } else if( type == "heroin" ) {
            diff = abs( pain - 100 );
        } else if( type == "tramadol" ) {
            diff = abs( pain - 40 ) / 2; // Bonus since it's long-acting
        }

        if( diff < difference ) {
            difference = diff;
            ret = &elem.front();
        }
    }
    return ret;
}

void inventory::rust_iron_items()
{
    for( auto &elem : items ) {
        for( auto &elem_stack_iter : elem ) {
            if( elem_stack_iter.made_of( material_id( "iron" ) ) &&
                !elem_stack_iter.has_flag( "WATERPROOF_GUN" ) &&
                !elem_stack_iter.has_flag( "WATERPROOF" ) &&
                elem_stack_iter.damage() < elem_stack_iter.max_damage() / 2 &&
                //Passivation layer prevents further rusting
                one_in( 500 ) &&
                //Scale with volume, bigger = slower (see #24204)
                one_in( static_cast<int>( 14 * std::cbrt( 0.5 * std::max( 0.05,
                                          static_cast<double>( elem_stack_iter.base_volume().value() ) / 250 ) ) ) ) &&
                //                       ^season length   ^14/5*0.75/3.14 (from volume of sphere)
                g->m.water_from( g->u.pos() ).typeId() ==
                "salt_water" ) { //Freshwater without oxygen rusts slower than air
                elem_stack_iter.inc_damage( DT_ACID ); // rusting never completely destroys an item
                add_msg( m_bad, _( "Your %s is damaged by rust." ), elem_stack_iter.tname() );
            }
        }
    }
}

units::mass inventory::weight() const
{
    units::mass ret = 0_gram;
    for( const auto &elem : items ) {
        for( const auto &elem_stack_iter : elem ) {
            ret += elem_stack_iter.weight();
        }
    }
    return ret;
}

// Helper function to iterate over the intersection of the inventory and a list
// of items given
template<typename F>
void for_each_item_in_both(
    const invstack &items, const std::map<const item *, int> &other, const F &f )
{
    // Shortcut the logic in the common case where other is empty
    if( other.empty() ) {
        return;
    }

    for( const auto &elem : items ) {
        const item &representative = elem.front();
        auto other_it = other.find( &representative );
        if( other_it == other.end() ) {
            continue;
        }

        int num_to_count = other_it->second;
        if( representative.count_by_charges() ) {
            item copy = representative;
            copy.charges = std::min( copy.charges, num_to_count );
            f( copy );
        } else {
            for( const auto &elem_stack_iter : elem ) {
                f( elem_stack_iter );
                if( --num_to_count <= 0 ) {
                    break;
                }
            }
        }
    }
}

units::mass inventory::weight_without( const std::map<const item *, int> &without ) const
{
    units::mass ret = weight();

    for_each_item_in_both( items, without,
    [&]( const item & i ) {
        ret -= i.weight();
    }
                         );

    if( ret < 0_gram ) {
        debugmsg( "Negative mass after removing some of inventory" );
        ret = {};
    }

    return ret;
}

units::volume inventory::volume() const
{
    units::volume ret = 0_ml;
    for( const auto &elem : items ) {
        for( const auto &elem_stack_iter : elem ) {
            ret += elem_stack_iter.volume();
        }
    }
    return ret;
}

units::volume inventory::volume_without( const std::map<const item *, int> &without ) const
{
    units::volume ret = volume();

    for_each_item_in_both( items, without,
    [&]( const item & i ) {
        ret -= i.volume();
    }
                         );

    if( ret < 0_ml ) {
        debugmsg( "Negative volume after removing some of inventory" );
        ret = 0_ml;
    }

    return ret;
}

std::vector<item *> inventory::active_items()
{
    std::vector<item *> ret;
    for( std::list<item> &elem : items ) {
        for( item &elem_stack_iter : elem ) {
            if( elem_stack_iter.needs_processing() ) {
                ret.push_back( &elem_stack_iter );
            }
        }
    }
    return ret;
}

enchantment inventory::get_active_enchantment_cache( const Character &owner ) const
{
    enchantment temp_cache;
    for( const std::list<item> &elem : items ) {
        for( const item &check_item : elem ) {
            for( const enchantment &ench : check_item.get_enchantments() ) {
                if( ench.is_active( owner, check_item ) ) {
                    temp_cache.force_add( ench );
                }
            }
        }
    }
    return temp_cache;
}

void inventory::assign_empty_invlet( item &it, const Character &p, const bool force )
{
    const std::string auto_setting = get_option<std::string>( "AUTO_INV_ASSIGN" );
    if( auto_setting == "disabled" || ( ( auto_setting == "favorites" ) && !it.is_favorite ) ) {
        return;
    }

    invlets_bitset cur_inv = p.allocated_invlets();
    itype_id target_type = it.typeId();
    for( auto iter : assigned_invlet ) {
        if( iter.second == target_type && !cur_inv[iter.first] ) {
            it.invlet = iter.first;
            return;
        }
    }
    if( cur_inv.count() < inv_chars.size() ) {
        for( const auto &inv_char : inv_chars ) {
            if( assigned_invlet.count( inv_char ) ) {
                // don't overwrite assigned keys
                continue;
            }
            if( !cur_inv[inv_char] ) {
                it.invlet = inv_char;
                return;
            }
        }
    }
    if( !force ) {
        it.invlet = 0;
        return;
    }
    // No free hotkey exist, re-use some of the existing ones
    for( auto &elem : items ) {
        item &o = elem.front();
        if( o.invlet != 0 ) {
            it.invlet = o.invlet;
            o.invlet = 0;
            return;
        }
    }
    debugmsg( "could not find a hotkey for %s", it.tname() );
}

void inventory::reassign_item( item &it, char invlet, bool remove_old )
{
    if( it.invlet == invlet ) { // no change needed
        return;
    }
    if( remove_old && it.invlet ) {
        invlet_cache.erase( it.invlet );
    }
    it.invlet = invlet;
    update_cache_with_item( it );
}

void inventory::update_invlet( item &newit, bool assign_invlet )
{
    // Avoid letters that have been manually assigned to other things.
    if( newit.invlet && assigned_invlet.find( newit.invlet ) != assigned_invlet.end() &&
        assigned_invlet[newit.invlet] != newit.typeId() ) {
        newit.invlet = '\0';
    }

    // Remove letters that are not in the favorites cache
    if( newit.invlet ) {
        if( !invlet_cache.contains( newit.invlet, newit.typeId() ) ) {
            newit.invlet = '\0';
        }
    }

    // Remove letters that have been assigned to other items in the inventory
    if( newit.invlet ) {
        char tmp_invlet = newit.invlet;
        newit.invlet = '\0';
        if( g->u.invlet_to_position( tmp_invlet ) == INT_MIN ) {
            newit.invlet = tmp_invlet;
        }
    }

    if( assign_invlet ) {
        // Assign a cached letter to the item
        if( !newit.invlet ) {
            newit.invlet = find_usable_cached_invlet( newit.typeId() );
        }

        // Give the item an invlet if it has none
        if( !newit.invlet ) {
            assign_empty_invlet( newit, g->u );
        }
    }
}

void inventory::set_stack_favorite( const int position, const bool favorite )
{
    for( auto &e : *std::next( items.begin(), position ) ) {
        e.set_favorite( favorite );
    }
}

invlets_bitset inventory::allocated_invlets() const
{
    invlets_bitset invlets;

    for( const auto &stack : items ) {
        const char invlet = stack.front().invlet;
        invlets.set( invlet );
    }
    invlets[0] = false;
    return invlets;
}

const itype_bin &inventory::get_binned_items() const
{
    if( binned ) {
        return binned_items;
    }

    binned_items.clear();

    // Hack warning
    inventory *this_nonconst = const_cast<inventory *>( this );
    this_nonconst->visit_items( [ this ]( item * e ) {
        binned_items[ e->typeId() ].push_back( e );
        return VisitResponse::NEXT;
    } );

    binned = true;
    return binned_items;
}

void inventory::copy_invlet_of( const inventory &other )
{
    assigned_invlet = other.assigned_invlet;
    invlet_cache = other.invlet_cache;
}
