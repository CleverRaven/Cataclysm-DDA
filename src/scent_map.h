#ifndef SCENT_H
#define SCENT_H

#include "enums.h"
#include "game_constants.h"
#include "cursesdef.h"

#include <array>

class map;
class game;

class scent_map
{
    protected:
        template<typename T>
        using scent_array = std::array<std::array<T, SEEY *MAPSIZE>, SEEX *MAPSIZE>;

        scent_array<int> grscent;
        tripoint player_last_position = tripoint_min;
        int player_last_moved = -1;

        const game &gm;

    public:
        scent_map( const game &g ) : gm( g ) { };

        void deserialize( const std::string &data );
        std::string serialize() const;

        void draw( WINDOW *w, int div, const tripoint &center ) const;

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

        bool inbounds( const tripoint &p ) const;
};

#endif
