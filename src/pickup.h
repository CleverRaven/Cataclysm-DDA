#ifndef PICKUP_H
#define PICKUP_H

#include "enums.h"

#include <map>
#include <list>
#include <vector>
#include <string>

class vehicle;
class item;

class Pickup
{
    public:
        static void do_pickup( const tripoint &pickup_target, bool from_vehicle,
                               std::list<int> &indices, std::list<int> &quantities, bool autopickup );
        static void pick_up( const tripoint &p, int min ); // Pick up items; ',' or via examine()

    private:
        // No instances of Pickup allowed.
        Pickup() {}

        typedef std::pair<item, int> ItemCount;
        typedef std::map<std::string, ItemCount> PickupMap;

        // Pickup helper functions
        static void pick_one_up( const tripoint &pickup_target, item &newit,
                                 vehicle *veh, int cargo_part, int index, int quantity,
                                 bool &got_water, bool &offered_swap,
                                 PickupMap &mapPickup, bool autopickup );

        typedef enum {
            DONE, ITEMS_FROM_CARGO, ITEMS_FROM_GROUND,
        } interact_results;

        static interact_results interact_with_vehicle( vehicle *veh, const tripoint &vpos,
                int veh_root_part );

        static int handle_quiver_insertion( item &here, int &moves_to_decrement, bool &picked_up );
        static void remove_from_map_or_vehicle( const tripoint &pos, vehicle *veh, int cargo_part,
                                                int &moves_taken, int curmit );
        static void show_pickup_message( const PickupMap &mapPickup );

        /**
        * Helper function for pick_up. Adjusts the projected volume and weight when a selection is changed.
        *
        * For unstackable items, if new_amt > old_amt, then the weight and volume are increased by the item's.
        * Otherwise, if new_amt < old_amt, the weight and volume are decreased by the item's.
        * For stackable items, old_amt and new_amt represent the old/new number of charges that were selected.
        *@param it The item whose selection was changed.
        *@param weight Where to store the new weight
        *@param volume Where to store the new volume
        *@param prev_amt How many of the item were selected before the change (must be non-negative)
        *@param old_amt How many of the item were selected after the change (must be non-negative)
        */
        static void adjust_weight_and_volume( const item &it, int &weight, int &volume, int old_amt,
                                              int new_amt );
};

#endif
