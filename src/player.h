#pragma once
#ifndef CATA_SRC_PLAYER_H
#define CATA_SRC_PLAYER_H

#include <iosfwd>
#include <list>
#include <map>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

#include "character.h"
#include "character_id.h"
#include "creature.h"
#include "enums.h"
#include "game_constants.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "memory_fast.h"
#include "optional.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"

class JsonIn;
class JsonObject;
class JsonOut;

namespace catacurses
{
class window;
}  // namespace catacurses

class player : public Character
{
    public:
        player();
        player( const player & ) = delete;
        player( player && ) noexcept( map_is_noexcept );
        ~player() override;
        player &operator=( const player & ) = delete;
        player &operator=( player && ) noexcept( list_is_noexcept );

        player *as_player() override {
            return this;
        }
        const player *as_player() const override {
            return this;
        }

        bool is_npc() const override {
            return false;    // Overloaded for NPCs in npc.h
        }

        // populate variables, inventory items, and misc from json object
        virtual void deserialize( JsonIn &jsin ) = 0;

        // by default save all contained info
        virtual void serialize( JsonOut &jsout ) const = 0;


        using Character::query_yn;
        bool query_yn( const std::string &mes ) const override;

    protected:

        void store( JsonOut &json ) const;
        void load( const JsonObject &data );
};

#endif // CATA_SRC_PLAYER_H
