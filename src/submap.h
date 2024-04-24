#pragma once
#ifndef CATA_SRC_SUBMAP_H
#define CATA_SRC_SUBMAP_H

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "active_item_cache.h"
#include "basecamp.h"
#include "calendar.h"
#include "cata_type_traits.h"
#include "colony.h"
#include "compatibility.h"
#include "computer.h"
#include "construction.h"
#include "field.h"
#include "game_constants.h"
#include "item.h"
#include "mapgen.h"
#include "mdarray.h"
#include "point.h"
#include "trap.h"
#include "type_id.h"
#include "vehicle.h"

class JsonOut;
class map;
class vehicle;
struct furn_t;
struct ter_t;

struct spawn_point {
    point pos;
    int count;
    mtype_id type;
    int faction_id;
    int mission_id;
    bool friendly;
    std::optional<std::string> name;
    spawn_data data;
    explicit spawn_point( const mtype_id &T = mtype_id::NULL_ID(), int C = 0, point P = point_zero,
                          int FAC = -1, int MIS = -1, bool F = false,
                          const std::optional<std::string> &N = std::nullopt, const spawn_data &SD = spawn_data() ) :
        pos( P ), count( C ), type( T ), faction_id( FAC ),
        mission_id( MIS ), friendly( F ), name( N ), data( SD ) {}
};

// Suppression due to bug in clang-tidy 12
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
struct maptile_soa {
    cata::mdarray<ter_id, point_sm_ms>             ter; // Terrain on each square
    cata::mdarray<furn_id, point_sm_ms>            frn; // Furniture on each square
    cata::mdarray<std::uint8_t, point_sm_ms>       lum; // Num items emitting light on each square
    cata::mdarray<cata::colony<item>, point_sm_ms> itm; // Items on each square
    cata::mdarray<field, point_sm_ms>              fld; // Field on each square
    cata::mdarray<trap_id, point_sm_ms>            trp; // Trap on each square
    cata::mdarray<int, point_sm_ms>                rad; // Irradiation of each square

    void swap_soa_tile( const point &p1, const point &p2 );
};

class submap
{
    public:
        submap() = default;
        submap( submap && ) noexcept( map_is_noexcept );
        ~submap();

        submap &operator=( submap && ) noexcept;

        void ensure_nonuniform() {
            if( is_uniform() ) {
                m = std::make_unique<maptile_soa>();
                std::uninitialized_fill_n( &m->ter[0][0], elements, uniform_ter );
                std::uninitialized_fill_n( &m->frn[0][0], elements, furn_str_id::NULL_ID() );
                std::uninitialized_fill_n( &m->lum[0][0], elements, 0 );
                std::uninitialized_fill_n( &m->trp[0][0], elements, tr_null );
                std::uninitialized_fill_n( &m->rad[0][0], elements, 0 );
            }
        }

        void revert_submap( submap &sr );

        submap get_revert_submap() const;

        trap_id get_trap( const point &p ) const {
            if( is_uniform() ) {
                return tr_null;
            }
            return m->trp[p.x][p.y];
        }

        void set_trap( const point &p, trap_id trap ) {
            ensure_nonuniform();
            m->trp[p.x][p.y] = trap;
        }

        void set_all_traps( const trap_id &trap ) {
            ensure_nonuniform();
            std::uninitialized_fill_n( &m->trp[0][0], elements, trap );
        }

        furn_id get_furn( const point &p ) const {
            if( is_uniform() ) {
                return furn_str_id::NULL_ID();
            }
            return m->frn[p.x][p.y];
        }

        void set_furn( const point &p, furn_id furn ) {
            ensure_nonuniform();
            m->frn[p.x][p.y] = furn;
        }

        void set_all_furn( const furn_id &furn ) {
            ensure_nonuniform();
            std::uninitialized_fill_n( &m->frn[0][0], elements, furn );
        }
        int get_map_damage( const point_sm_ms &p ) const {
            auto it = ephemeral_data.find( p );
            if( it != ephemeral_data.end() ) {
                return it->second.damage;
            }
            return 0;
        }

        void set_map_damage( const point_sm_ms &p, int dmg ) {
            ephemeral_data[p] = { dmg };
        }

        ter_id get_ter( const point &p ) const {
            if( is_uniform() ) {
                return uniform_ter;
            }
            return m->ter[p.x][p.y];
        }

        void set_ter( const point &p, ter_id terr ) {
            ensure_nonuniform();
            m->ter[p.x][p.y] = terr;
        }

        void set_all_ter( const ter_id &terr, bool uniform_ok = false ) {
            if( !uniform_ok ) {
                ensure_nonuniform();
            }
            if( is_uniform() ) {
                uniform_ter = terr;
            } else {
                std::uninitialized_fill_n( &m->ter[0][0], elements, terr );
            }
        }

        int get_radiation( const point &p ) const {
            if( is_uniform() ) {
                return 0;
            }
            return m->rad[p.x][p.y];
        }

        void set_radiation( const point &p, const int radiation ) {
            ensure_nonuniform();
            m->rad[p.x][p.y] = radiation;
        }

        uint8_t get_lum( const point &p ) const {
            if( is_uniform() ) {
                return 0;
            }
            return m->lum[p.x][p.y];
        }

        void set_lum( const point &p, uint8_t luminance ) {
            ensure_nonuniform();
            m->lum[p.x][p.y] = luminance;
        }

        void update_lum_add( const point &p, const item &i ) {
            ensure_nonuniform();
            if( i.is_emissive() && m->lum[p.x][p.y] < 255 ) {
                m->lum[p.x][p.y]++;
            }
        }

        void update_lum_rem( const point &p, const item &i );

        // TODO: Replace this as it essentially makes itm public
        cata::colony<item> &get_items( const point &p ) {
            if( is_uniform() ) {
                cata::colony<item> static noitems;
                return noitems;
            }
            return m->itm[p.x][p.y];
        }

        const cata::colony<item> &get_items( const point &p ) const {
            if( is_uniform() ) {
                cata::colony<item> static noitems;
                return noitems;
            }
            return m->itm[p.x][p.y];
        }

        // TODO: Replace this as it essentially makes fld public
        field &get_field( const point &p ) {
            if( is_uniform() ) {
                field static nofield;
                return nofield;
            }
            return m->fld[p.x][p.y];
        }

        const field &get_field( const point &p ) const {
            if( is_uniform() ) {
                field static nofield;
                return nofield;
            }
            return m->fld[p.x][p.y];
        }

        void clear_fields( const point &p );

        struct cosmetic_t {
            point pos;
            std::string type;
            std::string str;
        };

        void insert_cosmetic( const point &p, const std::string &type, const std::string &str ) {
            cosmetic_t ins;

            ins.pos = p;
            ins.type = type;
            ins.str = str;

            cosmetics.push_back( ins );
        }

        units::temperature_delta get_temperature_mod() const {
            return units::from_fahrenheit_delta( temperature_mod );
        }

        void set_temperature_mod( units::temperature_delta new_temperature_mod ) {
            temperature_mod = units::to_fahrenheit_delta( new_temperature_mod );
        }

        bool has_graffiti( const point &p ) const;
        const std::string &get_graffiti( const point &p ) const;
        void set_graffiti( const point &p, const std::string &new_graffiti );
        void delete_graffiti( const point &p );

        // Signage is a pretend union between furniture on a square and stored
        // writing on the square. When both are present, we have signage.
        // Its effect is meant to be cosmetic and atmospheric only.
        bool has_signage( const point &p ) const;
        // Dependent on furniture + cosmetics.
        std::string get_signage( const point &p ) const;
        // Can be used anytime (prevents code from needing to place sign first.)
        void set_signage( const point &p, const std::string &s );
        // Can be used anytime (prevents code from needing to place sign first.)
        void delete_signage( const point &p );

        bool has_computer( const point &p ) const;
        const computer *get_computer( const point &p ) const;
        computer *get_computer( const point &p );
        void set_computer( const point &p, const computer &c );
        void delete_computer( const point &p );

        bool contains_vehicle( vehicle * );

        bool is_open_air( const point & ) const;

        void rotate( int turns );
        void mirror( bool horizontally );

        void store( JsonOut &jsout ) const;
        void load( const JsonValue &jv, const std::string &member_name, int version );

        // If is_uniform is true, this submap is a solid block of terrain
        // Uniform submaps aren't saved/loaded, because regenerating them is faster
        bool is_uniform() const {
            return !static_cast<bool>( m );
        }

        std::vector<cosmetic_t> cosmetics; // Textual "visuals" for squares

        active_item_cache active_items;

        int field_count = 0;
        time_point last_touched = calendar::turn_zero;
        bool reverted = false; // NOLINT(cata-serialize)
        std::vector<spawn_point> spawns;
        /**
         * Vehicles on this submap (their (0,0) point is on this submap).
         * This vehicle objects are deleted by this submap when it gets
         * deleted.
         */
        std::vector<std::unique_ptr<vehicle>> vehicles;
        std::map<tripoint_sm_ms, partial_con> partial_constructions;
        std::unique_ptr<basecamp> camp;  // only allowing one basecamp per submap

        struct tile_data {
            int damage;
        };

    private:
        std::map<point_sm_ms, tile_data> ephemeral_data;
        std::map<point, computer> computers;
        std::unique_ptr<maptile_soa> m;
        ter_id uniform_ter = t_null;
        int temperature_mod = 0; // delta in F

        static constexpr size_t elements = SEEX * SEEY;
};

/**
 * A wrapper for a submap point. Allows getting multiple map features
 * (terrain, furniture etc.) without directly accessing submaps or
 * doing multiple bounds checks and submap gets.
 *
 * Templated so that we can have const and non-const version; aliases in
 * maptile_fwd.h
 */
template<typename Submap>
class maptile_impl
{
        static_assert( std::is_same_v<std::remove_const_t<Submap>, submap>,
                       "Submap should be either submap or const submap" );
    private:
        friend map; // To allow "sliding" the tile in x/y without bounds checks
        friend submap;
        Submap *sm;
        point pos_;

        maptile_impl( Submap *sub, const point &p ) :
            sm( sub ), pos_( p ) { }
        template<typename OtherSubmap>
        // NOLINTNEXTLINE(google-explicit-constructor)
        maptile_impl( const maptile_impl<OtherSubmap> &other ) :
            sm( other.wrapped_submap() ), pos_( other.pos() ) { }
    public:
        Submap *wrapped_submap() const {
            return sm;
        }
        inline point pos() const {
            return pos_;
        }

        trap_id get_trap() const {
            return sm->get_trap( pos() );
        }

        furn_id get_furn() const {
            return sm->get_furn( pos() );
        }

        ter_id get_ter() const {
            return sm->get_ter( pos() );
        }

        const trap &get_trap_t() const {
            return sm->get_trap( pos() ).obj();
        }

        const furn_t &get_furn_t() const {
            return sm->get_furn( pos() ).obj();
        }
        const ter_t &get_ter_t() const {
            return sm->get_ter( pos() ).obj();
        }

        const field &get_field() const {
            return sm->get_field( pos() );
        }

        using FieldEntry = cata::copy_const<Submap, field_entry>;
        FieldEntry *find_field( const field_type_id &field_to_find ) {
            return sm->get_field( pos() ).find_field( field_to_find );
        }

        int get_radiation() const {
            return sm->get_radiation( pos() );
        }

        bool has_graffiti() const {
            return sm->has_graffiti( pos() );
        }

        const std::string &get_graffiti() const {
            return sm->get_graffiti( pos() );
        }

        bool has_signage() const {
            return sm->has_signage( pos() );
        }

        std::string get_signage() const {
            return sm->get_signage( pos() );
        }

        // For map::draw_maptile
        size_t get_item_count() const {
            return sm->get_items( pos() ).size();
        }

        // Assumes there is at least one item
        const item &get_uppermost_item() const {
            return *std::prev( sm->get_items( pos() ).cend() );
        }

        // Gets all items
        const cata::colony<item> &get_items() const {
            return sm->get_items( pos() );
        }
};

#endif // CATA_SRC_SUBMAP_H
