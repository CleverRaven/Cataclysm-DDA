#pragma once
#ifndef MAPGEN_H
#define MAPGEN_H

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <utility>

#include "regional_settings.h"
#include "type_id.h"
#include "memory_fast.h"

class time_point;
struct point;
class JsonArray;
class JsonObject;
class mission;
struct tripoint;
class map;
template <typename T> struct weighted_int_list;
class mapgendata;
using building_gen_pointer = void ( * )( mapgendata & );

//////////////////////////////////////////////////////////////////////////
///// function pointer class; provides abstract referencing of
///// map generator functions written in multiple ways for per-terrain
///// random selection pool
class mapgen_function
{
    public:
        int weight;
    protected:
        mapgen_function( const int w ) : weight( w ) { }
    public:
        virtual ~mapgen_function() = default;
        virtual void setup() { } // throws
        virtual void check( const std::string & /*oter_name*/ ) const { }
        virtual void generate( mapgendata & ) = 0;
};

/////////////////////////////////////////////////////////////////////////////////
///// builtin mapgen
class mapgen_function_builtin : public virtual mapgen_function
{
    public:
        building_gen_pointer fptr;
        mapgen_function_builtin( building_gen_pointer ptr, int w = 1000 ) : mapgen_function( w ),
            fptr( ptr ) {
        }
        void generate( mapgendata &mgd ) override;
};

/////////////////////////////////////////////////////////////////////////////////
///// json mapgen (and friends)
/*
 * Actually a pair of shorts that can rng, for numbers that will never exceed 32768
 */
struct jmapgen_int {
    short val;
    short valmax;
    jmapgen_int( int v ) : val( v ), valmax( v ) {}
    jmapgen_int( int v, int v2 ) : val( v ), valmax( v2 ) {}
    jmapgen_int( point p );
    /**
     * Throws as usually if the json is invalid or missing.
     */
    jmapgen_int( const JsonObject &jo, const std::string &tag );
    /**
     * Throws is the json is malformed (e.g. a string not an integer, but does not throw
     * if the member is just missing (the default values are used instead).
     */
    jmapgen_int( const JsonObject &jo, const std::string &tag, short def_val, short def_valmax );

    int get() const;
};

enum jmapgen_setmap_op {
    JMAPGEN_SETMAP_OPTYPE_POINT = 0,
    JMAPGEN_SETMAP_TER,
    JMAPGEN_SETMAP_FURN,
    JMAPGEN_SETMAP_TRAP,
    JMAPGEN_SETMAP_RADIATION,
    JMAPGEN_SETMAP_BASH,
    JMAPGEN_SETMAP_OPTYPE_LINE = 100,
    JMAPGEN_SETMAP_LINE_TER,
    JMAPGEN_SETMAP_LINE_FURN,
    JMAPGEN_SETMAP_LINE_TRAP,
    JMAPGEN_SETMAP_LINE_RADIATION,
    JMAPGEN_SETMAP_OPTYPE_SQUARE = 200,
    JMAPGEN_SETMAP_SQUARE_TER,
    JMAPGEN_SETMAP_SQUARE_FURN,
    JMAPGEN_SETMAP_SQUARE_TRAP,
    JMAPGEN_SETMAP_SQUARE_RADIATION
};

struct jmapgen_setmap {
    jmapgen_int x;
    jmapgen_int y;
    jmapgen_int x2;
    jmapgen_int y2;
    jmapgen_setmap_op op;
    jmapgen_int val;
    int chance;
    jmapgen_int repeat;
    int rotation;
    int fuel;
    int status;

    jmapgen_setmap(
        jmapgen_int ix, jmapgen_int iy, jmapgen_int ix2, jmapgen_int iy2,
        jmapgen_setmap_op iop, jmapgen_int ival,
        int ione_in = 1, jmapgen_int irepeat = jmapgen_int( 1, 1 ), int irotation = 0, int ifuel = -1,
        int istatus = -1
    ) :
        x( ix ), y( iy ), x2( ix2 ), y2( iy2 ), op( iop ), val( ival ), chance( ione_in ),
        repeat( irepeat ), rotation( irotation ),
        fuel( ifuel ), status( istatus ) {}
    bool apply( mapgendata &dat, const point &offset ) const;
    /**
     * checks if applying these objects to data would cause cause a collision with vehicles
     * on the same map
     **/
    bool has_vehicle_collision( mapgendata &dat, const point &offset ) const;
};

/**
 * Basic mapgen object. It is supposed to place or do something on a specific square on the map.
 * Inherit from this class and implement the @ref apply function.
 *
 * Instructions for adding more mapgen things:
 * 1. create a new class inheriting from @ref jmapgen_piece.
 *   - It should have a constructor that a accepts a @ref JsonObject and initializes all it local
 *     members from it. On errors / invalid data, call @ref JsonObject::throw_error or similar.
 *   - Implement the @ref apply function to do the actual change on the map.
 * 2. Go into @ref mapgen_function_json::setup and look for the lines with load_objects, add a new
 *    line with your own class there. It should look like
 *    @code load_objects<your_own_class_from_step_1>( jo, "place_something" ); @endcode
 *    Use a descriptive name for "something", preferably matching the name of your class.
 * 3. Go into @ref mapgen_function_json::setup and look for the lines with load_place_mapings, add
 *    a new line with your class there. It should look like
 *    @code load_place_mapings<your_own_class_from_step_1>( jo, "something", format_placings ); @endcode
 *    Using the same "something" as in step 2 is preferred.
 *
 * For actual examples look at the commits that introduced the load_objects/load_place_mapings
 * lines (ignore the changes to the json files).
 */
class jmapgen_piece
{
    protected:
        jmapgen_piece() : repeat( 1, 1 ) { }
    public:
        /** Sanity-check this piece */
        virtual void check( const std::string &/*oter_name*/ ) const { }
        /** Place something on the map from mapgendata &dat, at (x,y). */
        virtual void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const = 0;
        virtual ~jmapgen_piece() = default;
        jmapgen_int repeat;
        virtual bool has_vehicle_collision( mapgendata &/*dat*/, int /*offset_x*/,
                                            int /*offset_y*/ ) const {
            return false;
        }
};

/**
 * Where to place something on a map.
 */
class jmapgen_place
{
    public:
        jmapgen_place() : x( 0, 0 ), y( 0, 0 ), repeat( 1, 1 ) { }
        jmapgen_place( const point &p ) : x( p.x ), y( p.y ), repeat( 1, 1 ) { }
        jmapgen_place( const JsonObject &jsi );
        void offset( const point & );
        jmapgen_int x;
        jmapgen_int y;
        jmapgen_int repeat;
};

using palette_id = std::string;

class mapgen_palette
{
    public:
        palette_id id;
        /**
         * The mapping from character code (key) to a list of things that should be placed. This is
         * similar to objects, but it uses key to get the actual position where to place things
         * out of the json "bitmap" (which is used to paint the terrain/furniture).
         */
        using placing_map = std::map< int, std::vector< shared_ptr_fast<const jmapgen_piece> > >;

        std::map<int, ter_id> format_terrain;
        std::map<int, furn_id> format_furniture;
        placing_map format_placings;

        template<typename PieceType>
        /**
         * Load (append to format_placings) the places that should be put there.
         * member_name is the name of an optional object / array in the json object jsi.
         */
        void load_place_mapings( const JsonObject &jo, const std::string &member_name,
                                 placing_map &format_placings );
        /**
         * Loads a palette object and returns it. Doesn't save it anywhere.
         */
        static mapgen_palette load_temp( const JsonObject &jo, const std::string &src );
        /**
         * Load a palette object and adds it to the global set of palettes.
         * If "palette" field is specified, those palettes will be loaded recursively.
         */
        static void load( const JsonObject &jo, const std::string &src );

        /**
         * Returns a palette with given id. If not found, debugmsg and returns a dummy.
         */
        static const mapgen_palette &get( const palette_id &id );

    private:
        static mapgen_palette load_internal( const JsonObject &jo, const std::string &src, bool require_id,
                                             bool allow_recur );

        /**
         * Adds a palette to this one. New values take preference over the old ones.
         *
         */
        void add( const palette_id &rh );
        void add( const mapgen_palette &rh );
};

struct jmapgen_objects {

        jmapgen_objects( const point &offset, const point &mapsize );

        bool check_bounds( const jmapgen_place &place, const JsonObject &jso );

        void add( const jmapgen_place &place, shared_ptr_fast<const jmapgen_piece> piece );

        /**
         * PieceType must be inheriting from jmapgen_piece. It must have constructor that accepts a
         * const JsonObject &as parameter. The function loads all objects from the json array and stores
         * them in @ref objects.
         */
        template<typename PieceType>
        void load_objects( JsonArray parray );

        /**
         * Loads the mapgen objects from the array inside of jsi. If jsi has no member of that name,
         * nothing is loaded and the function just returns.
         */
        template<typename PieceType>
        void load_objects( const JsonObject &jsi, const std::string &member_name );

        void check( const std::string &oter_name ) const;

        void apply( mapgendata &dat ) const;
        void apply( mapgendata &dat, const point &offset ) const;

        /**
         * checks if applying these objects to data would cause cause a collision with vehicles
         * on the same map
         **/
        bool has_vehicle_collision( mapgendata &dat, const point &offset ) const;

    private:
        /**
         * Combination of where to place something and what to place.
         */
        using jmapgen_obj = std::pair<jmapgen_place, shared_ptr_fast<const jmapgen_piece> >;
        std::vector<jmapgen_obj> objects;
        point m_offset;
        point mapgensize;
};

class mapgen_function_json_base
{
    public:
        bool check_inbounds( const jmapgen_int &x, const jmapgen_int &y, const JsonObject &jso ) const;
        size_t calc_index( const point &p ) const;
        bool has_vehicle_collision( mapgendata &dat, const point &offset ) const;

    private:
        std::string jdata;

    protected:
        mapgen_function_json_base( const std::string &s );
        virtual ~mapgen_function_json_base();

        void setup_common();
        bool setup_common( const JsonObject &jo );
        void setup_setmap( const JsonArray &parray );
        // Returns true if the mapgen qualifies at this point already
        virtual bool setup_internal( const JsonObject &jo ) = 0;
        virtual void setup_setmap_internal() { }

        void check_common( const std::string &oter_name ) const;

        void formatted_set_incredibly_simple( map &m, const point &offset ) const;

        bool do_format;
        bool is_ready;

        point mapgensize;
        point m_offset;
        std::vector<ter_furn_id> format;
        std::vector<jmapgen_setmap> setmap_points;

        jmapgen_objects objects;
};

class mapgen_function_json : public mapgen_function_json_base, public virtual mapgen_function
{
    public:
        void setup() override;
        void check( const std::string &oter_name ) const override;
        void generate( mapgendata & ) override;
        mapgen_function_json( const std::string &s, int w,
                              const point &grid_offset = point_zero );
        ~mapgen_function_json() override = default;

        ter_id fill_ter;
        oter_id predecessor_mapgen;

    protected:
        bool setup_internal( const JsonObject &jo ) override;

    private:
        jmapgen_int rotation;
};

class update_mapgen_function_json : public mapgen_function_json_base
{
    public:
        update_mapgen_function_json( const std::string &s );
        ~update_mapgen_function_json() override = default;

        void setup();
        bool setup_update( const JsonObject &jo );
        void check( const std::string &oter_name ) const;
        bool update_map( const tripoint &omt_pos, const point &offset,
                         mission *miss, bool verify = false ) const;
        bool update_map( mapgendata &md, const point &offset = point_zero,
                         bool verify = false ) const;

    protected:
        bool setup_internal( const JsonObject &/*jo*/ ) override;
        ter_id fill_ter;
};

class mapgen_function_json_nested : public mapgen_function_json_base
{
    public:
        void setup();
        void check( const std::string &oter_name ) const;
        mapgen_function_json_nested( const std::string &s );
        ~mapgen_function_json_nested() override = default;

        void nest( mapgendata &dat, const point &offset ) const;
    protected:
        bool setup_internal( const JsonObject &jo ) override;

    private:
        jmapgen_int rotation;
};

/////////////////////////////////////////////////////////
///// global per-terrain mapgen function lists
/*
 * Load mapgen function of any type from a json object
 */
std::shared_ptr<mapgen_function> load_mapgen_function( const JsonObject &jio,
        const std::string &id_base, int default_idx, const point &offset = point_zero );
/*
 * Load the above directly from a file via init, as opposed to riders attached to overmap_terrain. Added check
 * for oter_mapgen / oter_mapgen_weights key, multiple possible ( ie, [ "house", "house_base" ] )
 */
void load_mapgen( const JsonObject &jo );
void reset_mapgens();
/*
 * stores function ref and/or required data
 */
extern std::map<std::string, std::vector<std::shared_ptr<mapgen_function>> > oter_mapgen;
/*
 * random selector list for the nested vector above, as per individual mapgen_function_::weight value
 */
extern std::map<std::string, std::map<int, int> > oter_mapgen_weights;
/*
 * Sets the above after init, and initializes mapgen_function_json instances as well
 */
void calculate_mapgen_weights(); // throws

void check_mapgen_definitions();

/// move to building_generation
enum room_type {
    room_null,
    room_closet,
    room_lobby,
    room_chemistry,
    room_teleport,
    room_goo,
    room_cloning,
    room_vivisect,
    room_bionics,
    room_dorm,
    room_living,
    room_bathroom,
    room_kitchen,
    room_bedroom,
    room_backyard,
    room_study,
    room_mine_shaft,
    room_mine_office,
    room_mine_storage,
    room_mine_fuel,
    room_mine_housing,
    room_split
};

// helpful functions
bool connects_to( const oter_id &there, int dir );
void mapgen_rotate( map *m, oter_id terrain_type, bool north_is_down = false );
// wrappers for map:: functions
void line( map *m, const ter_id &type, int x1, int y1, int x2, int y2 );
void line_furn( map *m, const furn_id &type, int x1, int y1, int x2, int y2 );
void fill_background( map *m, const ter_id &type );
void fill_background( map *m, ter_id( *f )() );
void square( map *m, const ter_id &type, int x1, int y1, int x2, int y2 );
void square( map *m, ter_id( *f )(), int x1, int y1, int x2, int y2 );
void square( map *m, const weighted_int_list<ter_id> &f, int x1, int y1, int x2, int y2 );
void square_furn( map *m, const furn_id &type, int x1, int y1, int x2, int y2 );
void rough_circle( map *m, const ter_id &type, int x, int y, int rad );
void rough_circle_furn( map *m, const furn_id &type, int x, int y, int rad );
void circle( map *m, const ter_id &type, double x, double y, double rad );
void circle( map *m, const ter_id &type, int x, int y, int rad );
void circle_furn( map *m, const furn_id &type, int x, int y, int rad );
void add_corpse( map *m, int x, int y );

#endif
