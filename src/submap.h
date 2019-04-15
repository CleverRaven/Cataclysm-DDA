#pragma once
#ifndef SUBMAP_H
#define SUBMAP_H

#include <list>
#include <memory>
#include <vector>

#include "active_item_cache.h"
#include "basecamp.h"
#include "calendar.h"
#include "computer.h"
#include "field.h"
#include "game_constants.h"
#include "int_id.h"
#include "item.h"
#include "string_id.h"

class map;
class vehicle;
class computer;
struct trap;
struct ter_t;
struct furn_t;

using trap_id = int_id<trap>;
using ter_id = int_id<ter_t>;
using furn_id = int_id<furn_t>;
using furn_str_id = string_id<furn_t>;
struct mtype;
using mtype_id = string_id<mtype>;

struct spawn_point {
    point pos;
    int count;
    mtype_id type;
    int faction_id;
    int mission_id;
    bool friendly;
    std::string name;
    spawn_point( const mtype_id &T = mtype_id::NULL_ID(), int C = 0, point P = point_zero,
                 int FAC = -1, int MIS = -1, bool F = false,
                 std::string N = "NONE" ) :
        pos( P ), count( C ), type( T ), faction_id( FAC ),
        mission_id( MIS ), friendly( F ), name( N ) {}
};

struct submap {
    trap_id get_trap( const point &p ) const {
        return trp[p.x][p.y];
    }

    void set_trap( const point &p, trap_id trap ) {
        is_uniform = false;
        trp[p.x][p.y] = trap;
    }

    furn_id get_furn( const point &p ) const {
        return frn[p.x][p.y];
    }

    void set_furn( const point &p, furn_id furn ) {
        is_uniform = false;
        frn[p.x][p.y] = furn;
    }

    ter_id get_ter( const point &p ) const {
        return ter[p.x][p.y];
    }

    void set_ter( const point &p, ter_id terr ) {
        is_uniform = false;
        ter[p.x][p.y] = terr;
    }

    int get_radiation( const point &p ) const {
        return rad[p.x][p.y];
    }

    void set_radiation( const point &p, const int radiation ) {
        is_uniform = false;
        rad[p.x][p.y] = radiation;
    }

    void update_lum_add( const point &p, const item &i ) {
        is_uniform = false;
        if( i.is_emissive() && lum[p.x][p.y] < 255 ) {
            lum[p.x][p.y]++;
        }
    }

    void update_lum_rem( const point &p, const item &i ) {
        is_uniform = false;
        if( !i.is_emissive() ) {
            return;
        } else if( lum[p.x][p.y] && lum[p.x][p.y] < 255 ) {
            lum[p.x][p.y]--;
            return;
        }

        // Have to scan through all items to be sure removing i will actually lower
        // the count below 255.
        int count = 0;
        for( const auto &it : itm[p.x][p.y] ) {
            if( it.is_emissive() ) {
                count++;
            }
        }

        if( count <= 256 ) {
            lum[p.x][p.y] = static_cast<uint8_t>( count - 1 );
        }
    }

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

    bool has_graffiti( const point &p ) const;
    const std::string &get_graffiti( const point &p ) const;
    void set_graffiti( const point &p, const std::string &new_graffiti );
    void delete_graffiti( const point &p );

    // Signage is a pretend union between furniture on a square and stored
    // writing on the square. When both are present, we have signage.
    // Its effect is meant to be cosmetic and atmospheric only.
    bool has_signage( const point &p ) const;
    // Dependent on furniture + cosmetics.
    const std::string get_signage( const point &p ) const;
    // Can be used anytime (prevents code from needing to place sign first.)
    void set_signage( const point &p, const std::string &s );
    // Can be used anytime (prevents code from needing to place sign first.)
    void delete_signage( const point &p );

    bool contains_vehicle( vehicle * );

    // TODO: make trp private once the horrible hack known as editmap is resolved
    ter_id          ter[SEEX][SEEY];  // Terrain on each square
    furn_id         frn[SEEX][SEEY];  // Furniture on each square
    std::uint8_t    lum[SEEX][SEEY];  // Number of items emitting light on each square
    std::list<item> itm[SEEX][SEEY];  // Items on each square
    field           fld[SEEX][SEEY];  // Field on each square
    trap_id         trp[SEEX][SEEY];  // Trap on each square
    int             rad[SEEX][SEEY];  // Irradiation of each square

    // If is_uniform is true, this submap is a solid block of terrain
    // Uniform submaps aren't saved/loaded, because regenerating them is faster
    bool is_uniform;

    std::vector<cosmetic_t> cosmetics; // Textual "visuals" for squares

    active_item_cache active_items;

    int field_count = 0;
    time_point last_touched = calendar::time_of_cataclysm;
    int temperature = 0;
    std::vector<spawn_point> spawns;
    /**
     * Vehicles on this submap (their (0,0) point is on this submap).
     * This vehicle objects are deleted by this submap when it gets
     * deleted.
     */
    std::vector<std::unique_ptr<vehicle>> vehicles;
    std::unique_ptr<computer> comp;
    basecamp camp;  // only allowing one basecamp per submap

    submap();
};

/**
 * A wrapper for a submap point. Allows getting multiple map features
 * (terrain, furniture etc.) without directly accessing submaps or
 * doing multiple bounds checks and submap gets.
 */
struct maptile {
    private:
        friend map; // To allow "sliding" the tile in x/y without bounds checks
        friend submap;
        submap *const sm;
        size_t x;
        size_t y;
        point pos() const {
            return point( x, y );
        };

        maptile( submap *sub, const size_t nx, const size_t ny ) :
            sm( sub ), x( nx ), y( ny ) { }
        maptile( submap *sub, const point &p ) :
            sm( sub ), x( p.x ), y( p.y ) { }
    public:
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
            return sm->fld[x][y];
        }

        field_entry *find_field( const field_id field_to_find ) {
            return sm->fld[x][y].findField( field_to_find );
        }

        bool add_field( const field_id field_to_add, const int new_density, const time_duration &new_age ) {
            const bool ret = sm->fld[x][y].addField( field_to_add, new_density, new_age );
            if( ret ) {
                sm->field_count++;
            }

            return ret;
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

        const std::string get_signage() const {
            return sm->get_signage( pos() );
        }

        // For map::draw_maptile
        size_t get_item_count() const {
            return sm->itm[x][y].size();
        }

        const item &get_uppermost_item() const {
            return sm->itm[x][y].back();
        }
};

#endif
