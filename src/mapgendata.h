#pragma once
#ifndef MAPGENDATA_H
#define MAPGENDATA_H

#include "type_id.h"
#include "calendar.h"
#include "weighted_list.h"

struct tripoint;
class mission;
struct regional_settings;
class map;
namespace om_direction
{
enum class type : int;
} // namespace om_direction

/**
 * Contains various information regarding the individual mapgen instance
 * (generating a specific part of the map), used by the various mapgen
 * functions to do their thing.
 *
 * Contains for example:
 * - the @ref map to generate the map onto,
 * - the overmap terrain of the area to generate and its surroundings,
 * - regional settings to use.
 *
 * An instance of this class is passed through most of the mapgen code.
 * If any of these functions need more information, add them here.
 */
// @todo documentation
// @todo encapsulate data member
class mapgendata
{
    private:
        oter_id terrain_type_;
        float density_;
        time_point when_;
        ::mission *mission_;
        int zlevel_;

    public:
        oter_id t_nesw[8];

        int n_fac = 0;  // dir == 0
        int e_fac = 0;  // dir == 1
        int s_fac = 0;  // dir == 2
        int w_fac = 0;  // dir == 3
        int ne_fac = 0; // dir == 4
        int se_fac = 0; // dir == 5
        int sw_fac = 0; // dir == 6
        int nw_fac = 0; // dir == 7

        oter_id t_above;
        oter_id t_below;

        const regional_settings &region;

        map &m;

        weighted_int_list<ter_id> default_groundcover;

        mapgendata( oter_id t_north, oter_id t_east, oter_id t_south, oter_id t_west,
                    oter_id northeast, oter_id southeast, oter_id southwest, oter_id northwest,
                    oter_id up, oter_id down, int z, const regional_settings &rsettings, map &mp,
                    const oter_id &terrain_type, float density, const time_point &when, ::mission *miss );

        mapgendata( const tripoint &over, map &m, float density, const time_point &when, ::mission *miss );

        /**
         * Creates a copy of this mapgen data, but stores a different @ref terrain_type.
         * Useful when you want to create a base map (e.g. forest/field/river), that gets
         * refined later:
         * @code
         * void generate_foo( mapgendata &dat ) {
         *     mapgendata base_dat( dat, oter_id( "forest" ) );
         *     generate( base_dat );
         *     ... // refine map some more.
         * }
         * @endcode
         */
        mapgendata( const mapgendata &other, const oter_id &other_id );

        const oter_id &terrain_type() const {
            return terrain_type_;
        }
        float monster_density() const {
            return density_;
        }
        const time_point &when() const {
            return when_;
        }
        ::mission *mission() const {
            return mission_;
        }
        int zlevel() const {
            // @todo should be able to determine this from the map itself
            return zlevel_;
        }

        void set_dir( int dir_in, int val );
        void fill( int val );
        int &dir( int dir_in );
        const oter_id &north() const {
            return t_nesw[0];
        }
        const oter_id &east()  const {
            return t_nesw[1];
        }
        const oter_id &south() const {
            return t_nesw[2];
        }
        const oter_id &west()  const {
            return t_nesw[3];
        }
        const oter_id &neast() const {
            return t_nesw[4];
        }
        const oter_id &seast() const {
            return t_nesw[5];
        }
        const oter_id &swest() const {
            return t_nesw[6];
        }
        const oter_id &nwest() const {
            return t_nesw[7];
        }
        const oter_id &above() const {
            return t_above;
        }
        const oter_id &below() const {
            return t_below;
        }
        const oter_id &neighbor_at( om_direction::type dir ) const;
        void fill_groundcover();
        void square_groundcover( int x1, int y1, int x2, int y2 );
        ter_id groundcover();
        bool is_groundcover( const ter_id &iid ) const;
        bool has_basement() const;
};

#endif
