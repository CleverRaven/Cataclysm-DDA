#pragma once
#ifndef CATA_SRC_VIEWER_H
#define CATA_SRC_VIEWER_H

#include "coordinates.h"

class Creature;

// This is strictly an interface that provides access to a game entity's Field of View.
class viewer
{
    public:
        // TODO: fix point types (remove the first overload)
        virtual bool sees( const tripoint &target, bool is_avatar = false, int range_mod = 0 ) const = 0;
        virtual bool sees( const tripoint_bub_ms &target, bool is_avatar = false,
                           int range_mod = 0 ) const = 0;
        virtual bool sees( const Creature &target ) const = 0;
        virtual ~viewer() = default;
};

viewer &get_player_view();

#endif // CATA_SRC_VIEWER_H
