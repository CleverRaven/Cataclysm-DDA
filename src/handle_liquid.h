#pragma once
#ifndef CATA_SRC_HANDLE_LIQUID_H
#define CATA_SRC_HANDLE_LIQUID_H

#include "coordinates.h"
#include "item_location.h"
#include "point.h"

class item;
class monster;
class vehicle;

enum liquid_dest : int {
    LD_NULL,
    LD_CONSUME,
    LD_ITEM,
    LD_VEH,
    LD_KEG,
    LD_GROUND
};

struct liquid_dest_opt {
    liquid_dest dest_opt = LD_NULL;
    tripoint_bub_ms pos;
    item_location item_loc;
    vehicle *veh = nullptr;
};

// Contains functions that handle liquid
namespace liquid_handler
{
/**
 * Consume / handle all of the liquid. The function can be used when the liquid needs
 * to be handled and can not be put back to where it came from (e.g. when it's a newly
 * created item from crafting).
 * The player is forced to handle all of it, which may required them to pour it onto
 * the ground (if they don't have enough container space available) and essentially
 * loose the item.
 * @return Whether any of the liquid has been consumed. `false` indicates the player has
 * declined all options to handle the liquid (essentially canceled the action) and no
 * charges of the liquid have been transferred.
 * `true` indicates some charges have been transferred (but not necessarily all of them).
 */
void handle_all_liquid( item liquid, int radius, const item *avoid = nullptr );

/**
 * Consume / handle as much of the liquid as possible in varying ways. This function can
 * be used when the action can be canceled, which implies the liquid can be put back
 * to wherever it came from and is *not* lost if the player cancels the action.
 * It returns when all liquid has been handled or if the player has explicitly canceled
 * the action (use the charges count to distinguish).
 * @return Whether any of the liquid has been consumed. `false` indicates the player has
 * declined all options to handle the liquid and no charges of the liquid have been transferred.
 * `true` indicates some charges have been transferred (but not necessarily all of them).
 */
bool consume_liquid( item &liquid, int radius = 0, const item *avoid = nullptr );

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

/**
 * This may start a player activity if either \p source_pos or \p source_veh is not
 * null.
 * The function consumes moves of the player as needed.
 * Supply one of the source parameters to prevent the player from pouring the liquid back
 * into that "container". If no source parameter is given, the liquid must not be in a
 * container at all (e.g. freshly crafted, or already removed from the container).
 * @param liquid The actual liquid
 * @param liquid_target is the destination, with LD_NULL indicating it should be fetched.
 * @param source The container that currently contains the liquid.
 * @param radius Radius to look for liquid around pos
 * @param source_pos The source of the liquid when it's from the map.
 * @param source_veh The vehicle that currently contains the liquid in its tank.
 * @return Whether the user has handled the liquid (at least part of it). `false` indicates
 * the user has rejected all possible actions. But note that `true` does *not* indicate any
 * liquid was actually consumed, the user may have chosen an option that turned out to be
 * invalid (chose to fill into a full/unsuitable container).
 * Basically `false` indicates the user does not *want* to handle the liquid, `true`
 * indicates they want to handle it.
 */
bool handle_liquid( item &liquid, liquid_dest_opt &liquid_target, const item *source = nullptr,
                    int radius = 0,
                    const tripoint_bub_ms *source_pos = nullptr,
                    const vehicle *source_veh = nullptr, int part_num = -1,
                    const monster *source_mon = nullptr, bool silent = true );
bool handle_liquid( item_location &liquid, const item *source = nullptr, int radius = 0 );

/* Not to be used directly. Use liquid_handler::handle_liquid instead. */
bool perform_liquid_transfer( item &liquid, const tripoint_bub_ms *source_pos,
                              const vehicle *source_veh, int part_num,
                              const monster * /*source_mon*/, liquid_dest_opt &target, bool silent );
bool perform_liquid_transfer( item_location &liquid, liquid_dest_opt &target );

// Select destination to use, but don't actually do anything with it. Does not allow for spilling.
liquid_dest_opt select_liquid_target( item &liquid, int radius );

} // namespace liquid_handler

#endif // CATA_SRC_HANDLE_LIQUID_H
