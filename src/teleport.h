#pragma once
#ifndef CATA_SRC_TELEPORT_H
#define CATA_SRC_TELEPORT_H
#include "coords_fwd.h"

class Creature;
class vehicle;
class teleport
{
    public:
        /** Teleports a creature to a tile within min_distance and max_distance tiles. Limited to 2D.
        *bool safe determines whether the teleported creature can telefrag others/itself.
        */
        static bool teleport_creature( Creature &critter, int min_distance = 2, int max_distance = 12,
                                       bool safe = false,
                                       bool add_teleglow = true );
        static bool teleport_to_point( Creature &critter, tripoint_bub_ms target, bool safe,
                                       bool add_teleglow, bool display_message = true, bool force = false, bool force_safe = false );
        static bool teleport_vehicle( vehicle &veh, const tripoint_abs_ms &dp );

};

#endif // CATA_SRC_TELEPORT_H
