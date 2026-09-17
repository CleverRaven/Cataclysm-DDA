#include "inventory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "craft_reservation.h"
#include "debug.h"
#include "enums.h"
#include "flag.h"
#include "inventory_ui.h" // auto inventory blocking
#include "item_components.h"
#include "item_contents.h"
#include "item_stack.h"
#include "itype.h"
#include "map.h"
#include "messages.h" //for rust message
#include "npc.h"
#include "options.h"
#include "proficiency.h"
#include "rng.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

static const itype_id itype_acetaminophen( "acetaminophen" );
static const itype_id itype_aspirin( "aspirin" );
static const itype_id itype_codeine( "codeine" );
static const itype_id itype_heroin( "heroin" );
static const itype_id itype_ibuprofen( "ibuprofen" );
static const itype_id itype_oxycodone( "oxycodone" );
static const itype_id itype_salt_water( "salt_water" );
static const itype_id itype_tramadol( "tramadol" );

static const material_id material_iron( "iron" );

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
            if( !ids_by_invlet[invlet_u].is_empty() ) {
                debugmsg( "Duplicate invlet: %s and %s both mapped to %c",
                          ids_by_invlet[invlet_u].str(), p.first.str(), invlet );
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
    const itype_id &id = ids_by_invlet[invlet_u];
    if( id.is_empty() ) {
        return;
    }
    std::string &invlets = invlets_by_id[id];
    std::string::iterator it = std::find( invlets.begin(), invlets.end(), invlet );
    invlets.erase( it );
    ids_by_invlet[invlet_u] = itype_id();
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

#if defined(__ANDROID__)
extern void remove_stale_inventory_quick_shortcuts();
#endif

int count_charges_in_list( const itype *type, const map_stack &items )
{
    for( const item &candidate : items ) {
        if( candidate.type == type ) {
            return candidate.charges;
        }
    }
    return 0;
}

/**
* Finds the number of charges of the first item that matches ammotype.
*
* @param ammotype   Search target.
* @param items      Stack of items. Search stops at first match.
* @param [out] item_type Matching type.
*
* @return           Number of charges.
* */
int count_charges_in_list( const ammotype *ammotype, const map_stack &items,
                           itype_id &item_type )
{
    for( const item &candidate : items ) {
        if( candidate.is_ammo() && candidate.type->ammo->type == *ammotype ) {
            item_type = candidate.typeId();
            return candidate.charges;
        }
    }
    return 0;
}

// Helper function to iterate over the intersection of the inventory and a list
// of items given
template<typename F>
static void for_each_item_in_both(
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
            for( const item &elem_stack_iter : elem ) {
                f( elem_stack_iter );
                if( --num_to_count <= 0 ) {
                    break;
                }
            }
        }
    }
}
