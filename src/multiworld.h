#pragma once
#ifndef CATA_SRC_MULTIWORLD_H
#define CATA_SRC_MULTIWORLD_H

#include <climits>
#include <cstdint>
#include <iosfwd>
#include <new>
#include <optional>
#include <string>
#include <vector>

#include "game.h"
#include "calendar.h"
#include "catacharset.h"
#include "color.h"
#include "damage.h"
#include "translations.h"
#include "type_id.h"
#include "point.h"
class multiworld
{
        //allow access to set_world_prefix
        friend void game::unserialize( std::istream &fin, const cata_path &path );
    public:
        multiworld();
        ~multiworld();
        std::string get_world_prefix();
        //currently just the player
        bool travel_to_world( const std::string &prefix );
    private:
        /** @param world_prefix tells the game which world folder it should load data from
         *  @param set_world_prefix sets the current prefix, it shouldn't be used on it's own
        */
        std::string world_prefix;

        void set_world_prefix( std::string prefix );
};


extern multiworld MULTIWORLD;
#endif // CATA_SRC_MULTIWORLD_H
