#pragma once
#ifndef CATA_SRC_MAGIC_TELEPORTER_LIST_H
#define CATA_SRC_MAGIC_TELEPORTER_LIST_H

#include <map>
#include <set>
#include <string>

#include "optional.h"
#include "point.h"

class avatar;
class JsonIn;
class JsonOut;

class teleporter_list
{
    private:
        // OMT locations of all known teleporters
        std::map<tripoint, std::string> known_teleporters;
        // ui for selection of desired teleport location.
        // returns overmap tripoint, or nullopt if canceled
        cata::optional<tripoint> choose_teleport_location();
        // returns true if a teleport is successful
        // does not do any loading or unloading
        bool place_avatar_overmap( avatar &you, const tripoint &omt_pt ) const;
    public:
        bool knows_translocator( const tripoint &omt_pos ) const;
        // adds teleporter to known_teleporters and does any other activation necessary
        bool activate_teleporter( const tripoint &omt_pt, const tripoint &local_pt );
        void deactivate_teleporter( const tripoint &omt_pt, const tripoint &local_pt );

        // calls the necessary functions to select translocator location
        // and teleports the target(s) there
        void translocate( const std::set<tripoint> &targets );

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
};

#endif // CATA_SRC_MAGIC_TELEPORTER_LIST_H
