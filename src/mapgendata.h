#pragma once
#ifndef CATA_SRC_MAPGENDATA_H
#define CATA_SRC_MAPGENDATA_H

#include "calendar.h"
#include "cata_variant.h"
#include "coordinates.h"
#include "json.h"
#include "type_id.h"
#include "weighted_list.h"

class map;
class mission;
struct point;
struct regional_settings;

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
// TODO: documentation
// TODO: encapsulate data member
class mapgendata
{
    private:
        oter_id terrain_type_;
        float density_;
        time_point when_;
        ::mission *mission_;
        int zlevel_;
        std::unordered_map<std::string, cata_variant> mapgen_params_;

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

        mapgendata( const tripoint_abs_omt &over, map &m, float density, const time_point &when,
                    ::mission *miss );

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

        /**
         * Creates a copy of this mapgendata, but stores new parameter values.
         */
        mapgendata( const mapgendata &other,
                    const std::unordered_map<std::string, cata_variant> & );

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
            // TODO: should be able to determine this from the map itself
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
        void fill_groundcover() const;
        void square_groundcover( const point &p1, const point &p2 ) const;
        ter_id groundcover() const;
        bool is_groundcover( const ter_id &iid ) const;

        template<typename Result>
        Result get_param( const std::string &name ) const {
            auto it = mapgen_params_.find( name );
            if( it == mapgen_params_.end() ) {
                debugmsg( "No such parameter \"%s\"", name );
                return Result();
            }
            return it->second.get<Result>();
        }
};

#endif // CATA_SRC_MAPGENDATA_H
