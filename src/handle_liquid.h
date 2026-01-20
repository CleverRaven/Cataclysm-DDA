#pragma once
#ifndef CATA_SRC_HANDLE_LIQUID_H
#define CATA_SRC_HANDLE_LIQUID_H

#include "coordinates.h"
#include "creature_tracker.h"
#include "enums.h"
#include "item_location.h"
#include "monster.h"
#include "point.h"
#include "vehicle.h"
#include "vpart_position.h"

class Character;
class item;
class monster;
class vehicle;

enum liquid_dest : int {
    LD_NULL,
    LD_CONSUME, // immediately consumed by Character
    LD_ITEM, // put into another item (container)
    LD_VEH, // put into a vehicle part
    LD_KEG, // put into a keg (functionally identical to LD_GROUND)
    LD_GROUND // put onto a map tile
};

/**
* Wrapper for item sources that item_location can't currently handle that liquid handling must.
*/
struct liquid_wrapper {
    liquid_source_type type;
    // if present, this liquid_wrapper is functionally an item_location
    item_location standard_item;

    // otherwise, item is unorthodox and is from one of the below options:
    std::shared_ptr<item> liquid_source_item;
    // terrain that produces "infinite" liquid (e.g. flowing water)
    std::optional<tripoint_abs_ms> as_infinite_terrain;
    // specifically the position of a vehicle part with the source item -- cannot be referenced by item_location
    // this item is NOT instantiated and is serialized with its vehicle
    std::optional<tripoint_abs_ms> as_vehicle_part;
    size_t vp_index;
    // position of a monster that can be a liquid source (e.g. a cow)
    std::optional<tripoint_abs_ms> as_monster;

    // this wrapper was not created from a previously existing source item
    bool source_item_new;

    explicit liquid_wrapper( const item_location &source_loc ) : type( liquid_source_type::MAP_ITEM ),
        standard_item( source_loc ), source_item_new( false ) {
        validate();
    };
    // source_item_new = false: don't force handling infinite liquid
    explicit liquid_wrapper( const tripoint_abs_ms &infinite_liquid_pos,
                             item &infinite_liquid ) :
        type( liquid_source_type::INFINITE_MAP ),
        as_infinite_terrain( infinite_liquid_pos ),
        liquid_source_item( std::make_shared<item>( infinite_liquid ) ),
        source_item_new( false ) {
        validate();
    };
    explicit liquid_wrapper( const vpart_reference &vpr ) : type( liquid_source_type::VEHICLE ),
        as_vehicle_part( vpr.pos_abs() ), vp_index( vpr.part_index() ),
        liquid_source_item( std::make_shared<item>( vpr.part().get_base().only_item() ) ),
        source_item_new( false ) {
        validate();
    };
    explicit liquid_wrapper( monster &monster_source,
                             item &monster_liquid ) : type( liquid_source_type::MONSTER ),
        as_monster( monster_source.pos_abs() ),
        liquid_source_item( std::make_shared<item>( monster_liquid ) ),
        source_item_new( true ) {
        validate();
    };
    // this constructor is specifically for sourceless items
    // @param new_liquid -- is not preserved for future reference
    explicit liquid_wrapper( const item &new_liquid ) : type( liquid_source_type::NEW_ITEM ),
        liquid_source_item( std::make_shared<item>( new_liquid ) ),
        source_item_new( true ) {
        validate();
    };
    // for serialization ONLY. NEVER construct an empty liquid_wrapper.
    liquid_wrapper() = default;

    explicit operator bool() const {
        return type != liquid_source_type::NULL_SOURCE;
    }
    void validate() const;
    std::string get_menu_prompt() const;
    item *get_item();
    const item *get_item() const;
    tripoint_abs_ms get_item_pos() const;
    tripoint_bub_ms get_item_pos_bub( map &here ) const;
    const item *get_container() const;
    bool can_consume() const;
    bool source_item_was_new() const;
    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonValue &jsin );
};

// holds liquid destination information
// see liquid_dest for options
//
struct liquid_dest_opt {
    liquid_dest dest_opt = LD_NULL;
    tripoint_bub_ms pos;
    item_location item_loc;
    size_t vp_index; // of vehicle at pos
    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonValue &jsin );
};

// Contains functions that handle liquid
namespace liquid_handler
{
/**
 * Store any liquids produced during an NPC activity (e.g. crafting or disassembly) into
 * nearby containers. Will spill on the ground if no suitable containers are found.
 * Does not consume moves (the move cost is presumed to be covered by the activity that
 * produced the liquid).
 *
 * This logic is completely distinct from the rest of liquid_handler.
 */
void handle_npc_liquid( item &liquid, Character &who );

/** Call a forced handle_liquid for player or handle_npc_liquid for npc */
void handle_all_or_npc_liquid( Character &p, item &newit, int radius = 0 );

/**
 * Handle liquids from inside a container item. The function also handles consuming move points.
  * @param container Container of the liquid
 * @param radius around position to handle liquid for
 * @return Whether the item has been removed (which implies it was handled completely).
 * The iterator is invalidated in that case. Otherwise the item remains but may have
 * fewer charges.
 */
bool handle_all_liquids_from_container( item_location &container, int radius = 0 );

bool can_handle_liquid( const item &liquid );

/*
* Checks whether liquid can be handled, and if so,
* attempts to start a fill_liquid_activity_actor.
* @param source - the source of the liquid (required)
* @param destination - if not provided, query available destinations
* @return activity was successfully assigned
*/
bool handle_liquid( const liquid_wrapper &source,
                    const std::optional<liquid_dest_opt> &destination = std::nullopt,
                    int dest_container_search_radius = 0 );

// Select destination to use, but don't actually do anything with it. Does not allow for spilling.
liquid_dest_opt select_liquid_target( item &liquid, int radius );

} // namespace liquid_handler

#endif // CATA_SRC_HANDLE_LIQUID_H
