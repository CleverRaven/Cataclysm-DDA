#pragma once
#ifndef SCENT_H
#define SCENT_H

#include <array>
#include <string>

#include "calendar.h"
#include "enums.h" // IWYU pragma: keep
#include "game_constants.h"
#include "optional.h"
#include "point.h"

static constexpr int SCENT_MAP_Z_REACH = 1;

class map;
class game;

namespace catacurses
{
class window;
} // namespace catacurses

class scent_map
{
    protected:
        template<typename T>
        using scent_array = std::array<std::array<T, MAPSIZE_Y>, MAPSIZE_X>;

        scent_array<int> grscent;
        cata::optional<tripoint> player_last_position;
        time_point player_last_moved = calendar::before_time_starts;

        const game &gm;

    public:
        scent_map( const game &g ) : gm( g ) { }

        void deserialize( const std::string &data );
        std::string serialize() const;

        void draw( const catacurses::window &win, int div, const tripoint &center ) const;

        void update( const tripoint &center, map &m );
        void reset();
        void decay();
        void shift( int sm_shift_x, int sm_shift_y );

        /**
         * Get the scent value at the given position.
         * An invalid position is allows and will yield a 0 value.
         * The coordinate system is the same as the @ref map (`g->m`) uses.
         */
        /**@{*/
        void set( const tripoint &p, int value );
        int get( const tripoint &p ) const;
        /**@}*/
        void set_unsafe( const tripoint &p, int value );
        int get_unsafe( const tripoint &p ) const;

        bool inbounds( const tripoint &p ) const;
        bool inbounds( const point &p ) const {
            return inbounds( tripoint( p, 0 ) );
        }
};

#endif
