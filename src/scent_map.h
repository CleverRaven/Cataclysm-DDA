#pragma once
#ifndef CATA_SRC_SCENT_MAP_H
#define CATA_SRC_SCENT_MAP_H

#include <array>
#include <set>
#include <string>
#include <vector>

#include "calendar.h"
#include "enums.h" // IWYU pragma: keep
#include "game_constants.h"
#include "json.h"
#include "optional.h"
#include "point.h"
#include "type_id.h"

static constexpr int SCENT_MAP_Z_REACH = 1;

class game;
class map;

namespace catacurses
{
class window;
} // namespace catacurses

class scent_type
{
    public:
        static void load_scent_type( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, const std::string & );
        static const std::vector<scent_type> &get_all();
        static void check_scent_consistency();
        bool was_loaded;

        scenttype_id id;
        std::set<species_id> receptive_species;
        static void reset();
};

class scent_map
{
    protected:
        template<typename T>
        using scent_array = std::array<std::array<T, MAPSIZE_Y>, MAPSIZE_X>;

        scent_array<int> grscent;
        scenttype_id typescent;
        cata::optional<tripoint> player_last_position;
        time_point player_last_moved = calendar::before_time_starts;

        const game &gm;

    public:
        scent_map( const game &g ) : gm( g ) { }

        void deserialize( const std::string &data, bool is_type = false );
        std::string serialize( bool is_type = false ) const;

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
        void set( const tripoint &p, int value, const scenttype_id &type = scenttype_id() );
        int get( const tripoint &p ) const;
        /**@}*/
        void set_unsafe( const tripoint &p, int value, const scenttype_id &type = scenttype_id() );
        int get_unsafe( const tripoint &p ) const;

        scenttype_id get_type( const tripoint &p ) const;

        bool inbounds( const tripoint &p ) const;
        bool inbounds( const point &p ) const {
            return inbounds( tripoint( p, 0 ) );
        }
};

#endif // CATA_SRC_SCENT_MAP_H
