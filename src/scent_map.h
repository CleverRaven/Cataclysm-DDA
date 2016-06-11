#ifndef SCENT_H
#define SCENT_H

#include "enums.h"
#include "game_constants.h"
#include "cursesdef.h"

#include <array>

class map;

class scent_map
{
    protected:
        template<typename T>
        using scent_array = std::array<std::array<T, SEEY *MAPSIZE>, SEEX *MAPSIZE>;

        scent_array<int> grscent;
        int null_scent = 0;
        tripoint player_last_position = tripoint_min;
        int player_last_moved = -1;

    public:
        scent_map();

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
        int &operator()( size_t x, size_t y );
        int &operator()( const point &p ) {
            return operator()( p.x, p.y );
        }
        int &operator()( const tripoint &p ) {
            return operator()( p.x, p.y );
        }
        int operator()( size_t x, size_t y ) const;
        int operator()( const point &p ) const {
            return operator()( p.x, p.y );
        }
        int operator()( const tripoint &p ) const {
            return operator()( p.x, p.y );
        }
        /**@}*/

        bool inbounds( size_t x, size_t y ) const {
            return x < SEEX * MAPSIZE && y < SEEY * MAPSIZE;
        }
};

#endif
