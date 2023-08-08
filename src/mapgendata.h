#pragma once
#ifndef CATA_SRC_MAPGENDATA_H
#define CATA_SRC_MAPGENDATA_H

#include "calendar.h"
#include "cata_variant.h"
#include "coordinates.h"
#include "cube_direction.h"
#include "enum_bitset.h"
#include "jmapgen_flags.h"
#include "mapgen.h"
#include "type_id.h"
#include "weighted_list.h"

class JsonValue;
class map;
class mission;
struct point;
struct regional_settings;

namespace om_direction
{
enum class type : int;
} // namespace om_direction

struct mapgen_arguments {
    mapgen_arguments() = default;

    template <
        typename InputRange,
        std::enable_if_t <
            std::is_same <
                typename InputRange::value_type, std::pair<std::string, cata_variant>
                >::value ||
            std::is_same <
                typename InputRange::value_type, std::pair<const std::string, cata_variant>
                >::value
            > * = nullptr >
    explicit mapgen_arguments( const InputRange &map_ )
        : map( map_.begin(), map_.end() )
    {}

    std::unordered_map<std::string, cata_variant> map;

    bool operator==( const mapgen_arguments &other ) const {
        return map == other.map;
    }

    void merge( const mapgen_arguments & );
    void serialize( JsonOut & ) const;
    void deserialize( const JsonValue &ji );
};

namespace std
{

template<>
struct hash<mapgen_arguments> {
    size_t operator()( const mapgen_arguments & ) const noexcept;
};

} // namespace std

namespace mapgendata_detail
{

// helper to get a variant value with any variant being extractable as a string
template<typename Result>
inline Result extract_variant_value( const cata_variant &v )
{
    return v.get<Result>();
}
template<>
inline std::string extract_variant_value<std::string>( const cata_variant &v )
{
    return v.get_string();
}
template<>
inline cata_variant extract_variant_value<cata_variant>( const cata_variant &v )
{
    return v;
}

} // namespace mapgendata_detail

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
        mapgen_arguments mapgen_args_;
        enum_bitset<jmapgen_flags> mapgen_flags_;
        std::vector<oter_id> predecessors_;

    public:
        std::array<oter_id, 8> t_nesw;

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

        std::unordered_map<cube_direction, std::string> joins;

        const regional_settings &region;

        map &m;

        weighted_int_list<ter_id> default_groundcover;

        struct dummy_settings_t {};
        static constexpr dummy_settings_t dummy_settings = {};

        mapgendata( map &, dummy_settings_t );

        mapgendata( const tripoint_abs_omt &over, map &m, float density, const time_point &when,
                    ::mission *miss );

        std::vector<mapgen_phase> skip;

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
        mapgendata( const mapgendata &other, const mapgen_arguments & );

        /**
         * Creates a copy of this mapgendata, but stores new parameter values
         * and flags.
         */
        mapgendata( const mapgendata &other, const mapgen_arguments &,
                    const enum_bitset<jmapgen_flags> & );

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
        const oter_id &neighbor_at( direction ) const;
        void fill_groundcover() const;
        void square_groundcover( const point &p1, const point &p2 ) const;
        ter_id groundcover() const;
        bool is_groundcover( const ter_id &iid ) const;

        bool has_flag( jmapgen_flags ) const;

        bool has_join( cube_direction, const std::string &join_id ) const;

        bool has_predecessor() const;
        const oter_id &first_predecessor() const;
        const oter_id &last_predecessor() const;
        void clear_predecessors();
        void pop_last_predecessor();

        template<typename Result>
        Result get_arg( const std::string &name ) const {
            auto it = mapgen_args_.map.find( name );
            if( it == mapgen_args_.map.end() ) {
                debugmsg( "No such parameter \"%s\"", name );
                return Result();
            }
            return mapgendata_detail::extract_variant_value<Result>( it->second );
        }

        template<typename Result>
        Result get_arg_or( const std::string &name, const Result &fallback ) const {
            auto it = mapgen_args_.map.find( name );
            if( it == mapgen_args_.map.end() ) {
                return fallback;
            }
            return mapgendata_detail::extract_variant_value<Result>( it->second );
        }
};

#endif // CATA_SRC_MAPGENDATA_H
