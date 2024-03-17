#pragma once
#ifndef CATA_SRC_MAPGEN_H
#define CATA_SRC_MAPGEN_H

#include <cstddef>
#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cata_variant.h"
#include "coordinates.h"
#include "dialogue_helpers.h"
#include "jmapgen_flags.h"
#include "json.h"
#include "memory_fast.h"
#include "point.h"
#include "regional_settings.h"
#include "type_id.h"
#include "weighted_list.h"

class map;
class mapgendata;
struct mapgen_arguments;
template <typename Id> class mapgen_value;
class mission;
class tinymap;

using building_gen_pointer = void ( * )( mapgendata & );

//////////////////////////////////////////////////////////////////////////
///// function pointer class; provides abstract referencing of
///// map generator functions written in multiple ways for per-terrain
///// random selection pool
class mapgen_function
{
    public:
        dbl_or_var weight;
    protected:
        explicit mapgen_function( dbl_or_var w ) : weight( std::move( w ) ) { }
        explicit mapgen_function( int w ) : weight( w ) { }
    public:
        virtual ~mapgen_function() = default;
        virtual void setup() { } // throws
        virtual void finalize_parameters() { }
        virtual void check() const { }
        virtual void check_consistent_with( const oter_t & ) const { }
        virtual bool expects_predecessor() const {
            return false;
        }
        virtual void generate( mapgendata & ) = 0;
        virtual mapgen_parameters get_mapgen_params( mapgen_parameter_scope ) const {
            return {};
        }
};

/////////////////////////////////////////////////////////////////////////////////
///// builtin mapgen
class mapgen_function_builtin : public virtual mapgen_function
{
    public:
        building_gen_pointer fptr;
        explicit mapgen_function_builtin( building_gen_pointer ptr,
                                          int w = 1000 ) : mapgen_function( w ),
            fptr( ptr ) { }
        explicit mapgen_function_builtin( building_gen_pointer ptr,
                                          dbl_or_var w ) : mapgen_function( std::move( w ) ),
            fptr( ptr ) { }
        void generate( mapgendata &mgd ) override;
};

/////////////////////////////////////////////////////////////////////////////////
///// json mapgen (and friends)
/*
 * Actually a pair of integers that can rng, for numbers that will never exceed INT_MAX
 */
struct jmapgen_int {
    int val;
    int valmax;
    explicit jmapgen_int( int v ) : val( v ), valmax( v ) {}
    jmapgen_int( int v, int v2 ) : val( v ), valmax( v2 ) {}
    explicit jmapgen_int( point p );
    /**
     * Throws as usually if the json is invalid or missing.
     */
    jmapgen_int( const JsonObject &jo, std::string_view tag );
    /**
     * Throws is the json is malformed (e.g. a string not an integer, but does not throw
     * if the member is just missing (the default values are used instead).
     */
    jmapgen_int( const JsonObject &jo, std::string_view tag, const int &def_val,
                 const int &def_valmax );

    int get() const;
};

/** Mapgen pieces will be applied in order of phases.  The phases are as
 * follows: */
enum class mapgen_phase {
    removal,
    terrain,
    furniture,
    default_,
    nested_mapgen,
    transform,
    faction_ownership,
    zones,
    last
};

template<>
struct enum_traits<mapgen_phase> {
    static constexpr mapgen_phase last = mapgen_phase::last;
};

inline bool operator<( const mapgen_phase l, const mapgen_phase r )
{
    return static_cast<int>( l ) < static_cast<int>( r );
}

enum jmapgen_setmap_op {
    JMAPGEN_SETMAP_OPTYPE_POINT = 0,
    JMAPGEN_SETMAP_TER,
    JMAPGEN_SETMAP_FURN,
    JMAPGEN_SETMAP_TRAP,
    JMAPGEN_SETMAP_RADIATION,
    JMAPGEN_SETMAP_TRAP_REMOVE,
    JMAPGEN_SETMAP_CREATURE_REMOVE,
    JMAPGEN_SETMAP_ITEM_REMOVE,
    JMAPGEN_SETMAP_FIELD_REMOVE,
    JMAPGEN_SETMAP_BASH,
    JMAPGEN_SETMAP_VARIABLE,
    JMAPGEN_SETMAP_OPTYPE_LINE = 100,
    JMAPGEN_SETMAP_LINE_TER,
    JMAPGEN_SETMAP_LINE_FURN,
    JMAPGEN_SETMAP_LINE_TRAP,
    JMAPGEN_SETMAP_LINE_RADIATION,
    JMAPGEN_SETMAP_LINE_TRAP_REMOVE,
    JMAPGEN_SETMAP_LINE_CREATURE_REMOVE,
    JMAPGEN_SETMAP_LINE_ITEM_REMOVE,
    JMAPGEN_SETMAP_LINE_FIELD_REMOVE,
    JMAPGEN_SETMAP_OPTYPE_SQUARE = 200,
    JMAPGEN_SETMAP_SQUARE_TER,
    JMAPGEN_SETMAP_SQUARE_FURN,
    JMAPGEN_SETMAP_SQUARE_TRAP,
    JMAPGEN_SETMAP_SQUARE_RADIATION,
    JMAPGEN_SETMAP_SQUARE_TRAP_REMOVE,
    JMAPGEN_SETMAP_SQUARE_CREATURE_REMOVE,
    JMAPGEN_SETMAP_SQUARE_ITEM_REMOVE,
    JMAPGEN_SETMAP_SQUARE_FIELD_REMOVE
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
    std::string string_val;
    jmapgen_setmap(
        jmapgen_int ix, jmapgen_int iy, jmapgen_int ix2, jmapgen_int iy2,
        jmapgen_setmap_op iop, jmapgen_int ival,
        int ione_in = 1, jmapgen_int irepeat = jmapgen_int( 1, 1 ), int irotation = 0, int ifuel = -1,
        int istatus = -1, std::string istring_val = ""
    ) :
        x( ix ), y( iy ), x2( ix2 ), y2( iy2 ), op( iop ), val( ival ), chance( ione_in ),
        repeat( irepeat ), rotation( irotation ),
        fuel( ifuel ), status( istatus ), string_val( std::move( istring_val ) ) {}

    mapgen_phase phase() const;

    bool apply( const mapgendata &dat, const point &offset ) const;

    /**
     * checks if applying these objects to data would cause cause a collision with vehicles
     * on the same map
     **/
    bool has_vehicle_collision( const mapgendata &dat, const point &offset ) const;
};

struct spawn_data {
    std::map<itype_id, jmapgen_int> ammo;
    std::vector<point> patrol_points_rel_ms;
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
        virtual bool is_nop() const {
            return false;
        }
        virtual mapgen_phase phase() const {
            return mapgen_phase::default_;
        }
        /** Sanity-check this piece */
        virtual void check( const std::string &/*context*/, const mapgen_parameters &,
                            const jmapgen_int &/*x*/, const jmapgen_int &/*y*/ ) const { }

        virtual void merge_parameters_into( mapgen_parameters &,
                                            const std::string &/*outer_context*/ ) const {}

        /** Place something on the map from mapgendata &dat, at (x,y). */
        virtual void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y,
                            const std::string &context ) const = 0;
        virtual ~jmapgen_piece() = default;
        jmapgen_int repeat;
        virtual bool has_vehicle_collision( const mapgendata &, const point &/*offset*/ ) const {
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
        explicit jmapgen_place( const point &p ) : x( p.x ), y( p.y ), repeat( 1, 1 ) { }
        explicit jmapgen_place( const JsonObject &jsi );
        void offset( const point & );
        jmapgen_int x;
        jmapgen_int y;
        jmapgen_int repeat;
};

// Strong typedef for strings used as map/palette keys
// Each key should be a UTF-8 string displayed in only one column (i.e.
// utf8_width of 1) but can contain multiple Unicode code points.
class map_key
{
    public:
        explicit map_key( const std::string & );
        explicit map_key( const JsonMember & );

        friend bool operator==( const map_key &l, const map_key &r ) {
            return l.str == r.str;
        }

        std::string str;
};

namespace std
{
template<>
struct hash<map_key> {
    size_t operator()( const map_key &k ) const noexcept {
        return hash<std::string> {}( k.str );
    }
};
} // namespace std

template<typename T>
struct mapgen_constraint {
    mapgen_constraint( const std::string &name, const T &val )
        : parameter_name( name )
        , value( val )
    {}

    std::string parameter_name;
    T value;
};

class mapgen_palette
{
    public:
        palette_id id;

        /**
         * The mapping from character (key) to a list of things that should be placed. This is
         * similar to objects, but it uses key to get the actual position where to place things
         * out of the json "bitmap" (which is used to paint the terrain/furniture).
         */
        using placing_map =
            std::unordered_map<map_key, std::vector< shared_ptr_fast<const jmapgen_piece>>>;

        std::unordered_set<map_key> keys_with_terrain;
        placing_map format_placings;

        template<typename PieceType>
        /**
         * Load (append to format_placings) the places that should be put there.
         * member_name is the name of an optional object / array in the json object jsi.
         */
        void load_place_mapings( const JsonObject &jo, const std::string &member_name,
                                 placing_map &format_placings, const std::string &context );

        void check() const;

        const mapgen_parameters &get_parameters() const {
            return parameters;
        }

        /**
         * Loads a palette object and returns it. Doesn't save it anywhere.
         */
        static mapgen_palette load_temp( const JsonObject &jo, std::string_view src,
                                         const std::string &context );
        /**
         * Load a palette object and adds it to the global set of palettes.
         * If "palette" field is specified, those palettes will be loaded recursively.
         */
        static void load( const JsonObject &jo, std::string_view src );

        /**
         * Returns a palette with given id. If not found, debugmsg and returns a dummy.
         */
        static const mapgen_palette &get( const palette_id &id );

        static void check_definitions();

        static void reset();
    private:
        mapgen_parameters parameters;

        // These would ideally be mapgen_value<palette_id> but because they get
        // transformed into parameters as an implementation detail it's easier
        // to just use std::string
        std::vector<mapgen_value<std::string>> palettes_used;

        static mapgen_palette load_internal(
            const JsonObject &jo, std::string_view src, const std::string &context,
            bool require_id, bool allow_recur );

        struct add_palette_context {
            add_palette_context( const std::string &ctx, mapgen_parameters * );

            std::string context;
            std::vector<palette_id> ancestors;
            mapgen_parameters *top_level_parameters;
            const mapgen_parameters *current_parameters;
            std::vector<mapgen_constraint<palette_id>> constraints;
        };

        /**
         * Adds a palette to this one. New values take preference over the old ones.
         *
         * The ancestors parameter is a set of ids from all the palettes
         * currently being added, when this addition is triggered by the
         * addition of another palette which includes rh.  This allows for
         * detection of loops in palette references.
         */
        void add( const mapgen_value<std::string> &rh, const add_palette_context & );
        void add( const palette_id &rh, const add_palette_context & );
        void add( const mapgen_palette &rh, const add_palette_context & );
};

struct jmapgen_objects {

        jmapgen_objects( const point &offset, const point &mapsize, const point &tot_size );

        bool check_bounds( const jmapgen_place &place, const JsonObject &jso );

        void add( const jmapgen_place &place, const shared_ptr_fast<const jmapgen_piece> &piece );

        /**
         * PieceType must be inheriting from jmapgen_piece. It must have constructor that accepts a
         * const JsonObject &as parameter. The function loads all objects from the json array and stores
         * them in @ref objects.
         */
        template<typename PieceType>
        void load_objects( const JsonArray &parray, std::string_view context );

        /**
         * Loads the mapgen objects from the array inside of jsi. If jsi has no member of that name,
         * nothing is loaded and the function just returns.
         */
        template<typename PieceType>
        void load_objects( const JsonObject &jsi, const std::string &member_name,
                           const std::string &context );

        void finalize();
        void check( const std::string &context, const mapgen_parameters & ) const;

        void merge_parameters_into( mapgen_parameters &, const std::string &outer_context ) const;

        void add_placement_coords_to( std::unordered_set<point> & ) const;

        void apply( const mapgendata &dat, mapgen_phase, const std::string &context ) const;
        void apply( const mapgendata &dat, mapgen_phase, const point &offset,
                    const std::string &context ) const;

        /**
         * checks if applying these objects to data would cause cause a collision with vehicles
         * on the same map
         **/
        bool has_vehicle_collision( const mapgendata &dat, const point &offset ) const;

    private:
        /**
         * Combination of where to place something and what to place.
         */
        using jmapgen_obj = std::pair<jmapgen_place, shared_ptr_fast<const jmapgen_piece> >;
        std::vector<jmapgen_obj> objects;
        point m_offset;
        point mapgensize;
        point total_size;
};

class mapgen_function_json_base
{
    public:
        void merge_non_nest_parameters_into( mapgen_parameters &,
                                             const std::string &outer_context ) const;
        bool check_inbounds( const jmapgen_int &x, const jmapgen_int &y, const JsonObject &jso ) const;
        size_t calc_index( const point &p ) const;
        bool has_vehicle_collision( const mapgendata &dat, const point &offset ) const;

        void add_placement_coords_to( std::unordered_set<point> & ) const;

        const mapgen_parameters &get_parameters() const {
            return parameters;
        }

    private:
        JsonObject jsobj;
    protected:
        mapgen_function_json_base( const JsonObject &jsobj, const std::string &context );
        virtual ~mapgen_function_json_base();

        void setup_common();
        bool setup_common( const JsonObject &jo );
        void setup_setmap( const JsonArray &parray );
        // Returns true if the mapgen qualifies at this point already
        virtual bool setup_internal( const JsonObject &jo ) = 0;
        virtual void setup_setmap_internal() { }
        void finalize_parameters_common();

        void check_common() const;

        mapgen_arguments get_args( const mapgendata &md, mapgen_parameter_scope ) const;

        std::string context_;
        enum_bitset<jmapgen_flags> flags_;
        bool is_ready;

        point mapgensize;
        point m_offset;
        point total_size;
        std::vector<jmapgen_setmap> setmap_points;

        jmapgen_objects objects;

        mapgen_parameters parameters;
};

class mapgen_function_json : public mapgen_function_json_base, public virtual mapgen_function
{
    public:
        void setup() override;
        void finalize_parameters() override;
        void check() const override;
        void check_consistent_with( const oter_t & ) const override;
        bool expects_predecessor() const override;
        void generate( mapgendata & ) override;
        mapgen_parameters get_mapgen_params( mapgen_parameter_scope ) const override;
        mapgen_function_json( const JsonObject &jsobj, dbl_or_var w,
                              const std::string &context,
                              const point &grid_offset, const point &grid_total );
        ~mapgen_function_json() override = default;

        ter_id fill_ter;
        oter_id predecessor_mapgen;

    protected:
        bool setup_internal( const JsonObject &jo ) override;

    private:
        jmapgen_int rotation;
        oter_str_id fallback_predecessor_mapgen_;
};

class update_mapgen_function_json : public mapgen_function_json_base
{
    public:
        update_mapgen_function_json( const JsonObject &jsobj, const std::string &context );
        ~update_mapgen_function_json() override = default;

        void setup();
        bool setup_update( const JsonObject &jo );
        void finalize_parameters();
        void check() const;
        bool update_map(
            const tripoint_abs_omt &omt_pos, const mapgen_arguments &, const point &offset,
            mission *miss, bool verify = false, bool mirror_horizontal = false,
            bool mirror_vertical = false, int rotation = 0 ) const;
        bool update_map( const mapgendata &md, const point &offset = point_zero,
                         bool verify = false ) const;

    protected:
        bool setup_internal( const JsonObject &/*jo*/ ) override;
        ter_id fill_ter;
};

class mapgen_function_json_nested : public mapgen_function_json_base
{
    public:
        void setup();
        void finalize_parameters();
        void check() const;
        mapgen_function_json_nested( const JsonObject &jsobj, const std::string &context );
        ~mapgen_function_json_nested() override = default;

        void nest( const mapgendata &md, const point &offset,
                   const std::string &outer_context ) const;
    protected:
        bool setup_internal( const JsonObject &jo ) override;

    private:
        jmapgen_int rotation;
};

class nested_mapgen
{
    public:
        const weighted_int_list<std::shared_ptr<mapgen_function_json_nested>> &funcs() const {
            return funcs_;
        }
        void add( const std::shared_ptr<mapgen_function_json_nested> &p, int weight ) {
            funcs_.add( p, weight );
        }
        // Returns a set containing every relative coordinate of a point that
        // might have something placed by this mapgen
        std::unordered_set<point> all_placement_coords() const;
    private:
        weighted_int_list<std::shared_ptr<mapgen_function_json_nested>> funcs_;
};

class update_mapgen
{
    public:
        const std::vector<std::unique_ptr<update_mapgen_function_json>> &funcs() const {
            return funcs_;
        }
        void add( std::unique_ptr<update_mapgen_function_json> &&p ) {
            funcs_.push_back( std::move( p ) );
        }
    private:
        std::vector<std::unique_ptr<update_mapgen_function_json>> funcs_;
};

/////////////////////////////////////////////////////////
///// global per-terrain mapgen function lists
/*
 * Load mapgen function of any type from a json object
 */
std::shared_ptr<mapgen_function> load_mapgen_function( const JsonObject &jio,
        const std::string &id_base, const point &offset, const point &total );
void load_and_add_mapgen_function(
    const JsonObject &jio, const std::string &id_base, const point &offset, const point &total );
/*
 * Load the above directly from a file via init, as opposed to riders attached to overmap_terrain. Added check
 * for oter_mapgen / oter_mapgen_weights key, multiple possible ( i.e., [ "house_w_1", "duplex" ] )
 */
void load_mapgen( const JsonObject &jo );
void reset_mapgens();
/**
 * Attempts to register the built-in function @p key as mapgen for the overmap terrain @p key.
 * If there is no matching function, it does nothing (no error message) and returns -1.
 * Otherwise it returns the index of the added entry in the vector of @ref oter_mapgen.
 */
// @TODO this should go away. It is only used for old built-in mapgen. Mapgen should be done via JSON.
int register_mapgen_function( const std::string &key );
/**
 * Check that @p key is present in @ref oter_mapgen.
 */
bool has_mapgen_for( const std::string &key );
/**
 * Verify that the properties of a particular mapgen match the properties of
 * its overmap_terrain */
void check_mapgen_consistent_with( const std::string &key, const oter_t & );
/**
 * Check whether @p key is a valid update_mapgen id.
 */
bool has_update_mapgen_for( const update_mapgen_id & );
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
    room_split
};

// helpful functions
bool connects_to( const oter_id &there, int dir );
// wrappers for map:: functions
void line( map *m, const ter_id &type, const point &p1, const point &p2 );
void line( tinymap *m, const ter_id &type, const point &p1, const point &p2 );
void line_furn( map *m, const furn_id &type, const point &p1, const point &p2 );
void line_furn( tinymap *m, const furn_id &type, const point &p1, const point &p2 );
void fill_background( map *m, const ter_id &type );
void fill_background( map *m, ter_id( *f )() );
void square( map *m, const ter_id &type, const point &p1, const point &p2 );
void square( map *m, ter_id( *f )(), const point &p1, const point &p2 );
void square( map *m, const weighted_int_list<ter_id> &f, const point &p1, const point &p2 );
void square_furn( map *m, const furn_id &type, const point &p1, const point &p2 );
void square_furn( tinymap *m, const furn_id &type, const point &p1, const point &p2 );
void rough_circle( map *m, const ter_id &type, const point &, int rad );
void rough_circle_furn( map *m, const furn_id &type, const point &, int rad );
void circle( map *m, const ter_id &type, double x, double y, double rad );
void circle( map *m, const ter_id &type, const point &, int rad );
void circle_furn( map *m, const furn_id &type, const point &, int rad );
void add_corpse( map *m, const point & );

extern std::map<nested_mapgen_id, nested_mapgen> nested_mapgens;
extern std::map<update_mapgen_id, update_mapgen> update_mapgens;

#endif // CATA_SRC_MAPGEN_H
