#pragma once
#ifndef MAPGEN_H
#define MAPGEN_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "int_id.h"

class time_point;
struct ter_t;
using ter_id = int_id<ter_t>;
struct furn_t;
using furn_id = int_id<furn_t>;
struct oter_t;
using oter_id = int_id<oter_t>;
struct point;
class JsonArray;
class JsonObject;
struct mapgendata;
struct tripoint;
class map;
typedef void ( *building_gen_pointer )( map *, oter_id, mapgendata, const time_point &, float );
struct ter_furn_id;

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
        virtual void generate( map *, const oter_id &, const mapgendata &, const time_point &, float ) = 0;
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
        void generate( map *m, const oter_id &terrain_type, const mapgendata &mgd, const time_point &t,
                       float d ) override;
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
    jmapgen_int( JsonObject &jo, const std::string &tag );
    /**
     * Throws is the json is malformed (e.g. a string not an integer, but does not throw
     * if the member is just missing (the default values are used instead).
     */
    jmapgen_int( JsonObject &jo, const std::string &tag, short def_val, short def_valmax );

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
    bool apply( const mapgendata &dat, int offset_x, int offset_y ) const;
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
        jmapgen_piece() = default;
    public:
        /** Sanity-check this piece */
        virtual void check( const std::string &/*oter_name*/ ) const { };
        /** Place something on the map from mapgendata dat, at (x,y). mon_density */
        virtual void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y,
                            float mon_density ) const = 0;
        virtual ~jmapgen_piece() = default;
};

/**
 * Where to place something on a map.
 */
class jmapgen_place
{
    public:
        jmapgen_place() : x( 0, 0 ), y( 0, 0 ), repeat( 1, 1 ) { }
        jmapgen_place( const int a, const int b ) : x( a ), y( b ), repeat( 1, 1 ) { }
        jmapgen_place( JsonObject &jsi );
        void offset( const int x_offset, const int y_offset );
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
        using placing_map = std::map< int, std::vector< std::shared_ptr<jmapgen_piece> > >;

        std::map<int, ter_id> format_terrain;
        std::map<int, furn_id> format_furniture;
        placing_map format_placings;

        template<typename PieceType>
        /**
         * Load (append to format_placings) the places that should be put there.
         * member_name is the name of an optional object / array in the json object jsi.
         */
        void load_place_mapings( JsonObject &jo, const std::string &member_name,
                                 placing_map &format_placings );
        /**
         * Loads a palette object and returns it. Doesn't save it anywhere.
         */
        static mapgen_palette load_temp( JsonObject &jo, const std::string &src );
        /**
         * Load a palette object and adds it to the global set of palettes.
         * If "palette" field is specified, those palettes will be loaded recursively.
         */
        static void load( JsonObject &jo, const std::string &src );

        /**
         * Returns a palette with given id. If not found, debugmsg and returns a dummy.
         */
        static const mapgen_palette &get( const palette_id &id );

    private:
        static mapgen_palette load_internal( JsonObject &jo, const std::string &src, bool require_id,
                                             bool allow_recur );

        /**
         * Adds a palette to this one. New values take preference over the old ones.
         *
         */
        void add( const palette_id &rh );
        void add( const mapgen_palette &rh );
};

struct jmapgen_objects {

        jmapgen_objects( int offset_x, int offset_y, size_t mapsize_x, size_t mapsize_x_y );

        bool check_bounds( const jmapgen_place place, JsonObject &jso );

        void add( const jmapgen_place &place, std::shared_ptr<jmapgen_piece> piece );

        /**
         * PieceType must be inheriting from jmapgen_piece. It must have constructor that accepts a
         * JsonObject as parameter. The function loads all objects from the json array and stores
         * them in @ref objects.
         */
        template<typename PieceType>
        void load_objects( JsonArray parray );

        /**
         * Loads the mapgen objects from the array inside of jsi. If jsi has no member of that name,
         * nothing is loaded and the function just returns.
         */
        template<typename PieceType>
        void load_objects( JsonObject &jsi, const std::string &member_name );

        void check( const std::string &oter_name ) const;

        void apply( const mapgendata &dat, float density ) const;
        void apply( const mapgendata &dat, int offset_x, int offset_y, float density ) const;

    private:
        /**
         * Combination of where to place something and what to place.
         */
        using jmapgen_obj = std::pair<jmapgen_place, std::shared_ptr<jmapgen_piece> >;
        std::vector<jmapgen_obj> objects;
        int offset_x;
        int offset_y;
        size_t mapgensize_x;
        size_t mapgensize_y;
};

class mapgen_function_json_base
{
    public:
        bool check_inbounds( const jmapgen_int &x, const jmapgen_int &y ) const;
        size_t calc_index( size_t x, size_t y ) const;

    private:
        std::string jdata;

    protected:
        mapgen_function_json_base( const std::string &s );
        virtual ~mapgen_function_json_base();

        void setup_common();
        void setup_setmap( JsonArray &parray );
        // Returns true if the mapgen qualifies at this point already
        virtual bool setup_internal( JsonObject &jo ) = 0;
        virtual void setup_setmap_internal() { }

        void check_common( const std::string &oter_name ) const;

        void formatted_set_incredibly_simple( map &m, int offset_x, int offset_y ) const;

        bool do_format;
        bool is_ready;

        size_t mapgensize_x;
        size_t mapgensize_y;
        int x_offset;
        int y_offset;
        std::vector<ter_furn_id> format;
        std::vector<jmapgen_setmap> setmap_points;

        jmapgen_objects objects;
};

class mapgen_function_json : public mapgen_function_json_base, public virtual mapgen_function
{
    public:
        void setup() override;
        void check( const std::string &oter_name ) const override;
        void generate( map *, const oter_id &, const mapgendata &, const time_point &, float ) override;
        mapgen_function_json( const std::string &s, int w,
                              const int x_grid_offset = 0, const int y_grid_offset = 0 );
        ~mapgen_function_json() override = default;

        ter_id fill_ter;
        std::string luascript;

    protected:
        bool setup_internal( JsonObject &jo ) override;

    private:
        jmapgen_int rotation;
};

class mapgen_function_json_nested : public mapgen_function_json_base
{
    public:
        void setup();
        void check( const std::string &oter_name ) const;
        mapgen_function_json_nested( const std::string &s );
        ~mapgen_function_json_nested() override = default;

        void nest( const mapgendata &dat, int offset_x, int offset_y, float density ) const;
    protected:
        bool setup_internal( JsonObject &jo ) override;

    private:
        jmapgen_int rotation;
};

/////////////////////////////////////////////////////////////////////////////////
///// lua mapgen
class mapgen_function_lua : public virtual mapgen_function
{
    public:
        const std::string scr;
        mapgen_function_lua( std::string s, int w = 1000 ) : mapgen_function( w ), scr( s ) {
            // scr = s; // @todo: if ( luaL_loadstring(L, scr.c_str() ) ) { error }
        }
        void generate( map *, const oter_id &, const mapgendata &, const time_point &, float ) override;
};
/////////////////////////////////////////////////////////
///// global per-terrain mapgen function lists
/*
 * Load mapgen function of any type from a json object
 */
std::shared_ptr<mapgen_function> load_mapgen_function( JsonObject &jio, const std::string &id_base,
        int default_idx, int x_offset = 0, int y_offset = 0 );
/*
 * Load the above directly from a file via init, as opposed to riders attached to overmap_terrain. Added check
 * for oter_mapgen / oter_mapgen_weights key, multiple possible ( ie, [ "house", "house_base" ] )
 */
void load_mapgen( JsonObject &jo );
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

void house_room( map *m, room_type type, int x1, int y1, int x2, int y2, mapgendata &dat );
// helpful functions
bool connects_to( const oter_id &there, int dir );
void mapgen_rotate( map *m, oter_id terrain_type, bool north_is_down = false );
// wrappers for map:: functions
void line( map *m, const ter_id type, int x1, int y1, int x2, int y2 );
void line_furn( map *m, furn_id type, int x1, int y1, int x2, int y2 );
void fill_background( map *m, ter_id type );
void fill_background( map *m, ter_id( *f )() );
void square( map *m, ter_id type, int x1, int y1, int x2, int y2 );
void square( map *m, ter_id( *f )(), int x1, int y1, int x2, int y2 );
void square_furn( map *m, furn_id type, int x1, int y1, int x2, int y2 );
void rough_circle( map *m, ter_id type, int x, int y, int rad );
void rough_circle_furn( map *m, furn_id type, int x, int y, int rad );
void circle( map *m, ter_id type, double x, double y, double rad );
void circle( map *m, ter_id type, int x, int y, int rad );
void circle_furn( map *m, furn_id type, int x, int y, int rad );
void add_corpse( map *m, int x, int y );

typedef void ( *map_special_pointer )( map &m, const tripoint &abs_sub );

#endif
