#pragma once
#ifndef CATA_SRC_VIEWER_H
#define CATA_SRC_VIEWER_H

#include "coords_fwd.h"

class Creature;
class map;

// This is strictly an interface that provides access to a game entity's Field of View.
class viewer
{
    public:
        virtual bool sees( const map &here, const tripoint_bub_ms &target, bool is_avatar = false,
                           int range_mod = 0 ) const = 0;
        virtual bool sees( const map &here, const Creature &target ) const = 0;
        virtual ~viewer() = default;
};

viewer &get_player_view();

#endif // CATA_SRC_VIEWER_H
