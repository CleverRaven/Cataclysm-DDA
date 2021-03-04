#include "mapgen.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <ostream>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

#include "calendar.h"
#include "cata_assert.h"
#include "catacharset.h"
#include "character_id.h"
#include "clzones.h"
#include "colony.h"
#include "common_types.h"
#include "computer.h"
#include "coordinate_conversions.h"
#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "drawing_primitives.h"
#include "enums.h"
#include "field_type.h"
#include "game.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "init.h"
#include "item.h"
#include "item_factory.h"
#include "item_group.h"
#include "item_pocket.h"
#include "itype.h"
#include "json.h"
#include "level_cache.h"
#include "line.h"
#include "magic_ter_furn_transform.h"
#include "map.h"
#include "map_extras.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "mapgen_functions.h"
#include "mapgendata.h"
#include "mapgenformat.h"
#include "memory_fast.h"
#include "mission.h"
#include "mongroup.h"
#include "npc.h"
#include "omdata.h"
#include "optional.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "ret_val.h"
#include "rng.h"
#include "string_formatter.h"
#include "submap.h"
#include "text_snippets.h"
#include "tileray.h"
#include "translations.h"
#include "trap.h"
#include "units.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "vehicle_group.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weighted_list.h"

static const mongroup_id GROUP_BREATHER( "GROUP_BREATHER" );
static const mongroup_id GROUP_BREATHER_HUB( "GROUP_BREATHER_HUB" );
static const mongroup_id GROUP_DARK_WYRM( "GROUP_DARK_WYRM" );
static const mongroup_id GROUP_DOG_THING( "GROUP_DOG_THING" );
static const mongroup_id GROUP_FUNGI_FUNGALOID( "GROUP_FUNGI_FUNGALOID" );
static const mongroup_id GROUP_HAZMATBOT( "GROUP_HAZMATBOT" );
static const mongroup_id GROUP_LAB( "GROUP_LAB" );
static const mongroup_id GROUP_LAB_CYBORG( "GROUP_LAB_CYBORG" );
static const mongroup_id GROUP_LAB_SECURITY( "GROUP_LAB_SECURITY" );
static const mongroup_id GROUP_NETHER( "GROUP_NETHER" );
static const mongroup_id GROUP_ROBOT_SECUBOT( "GROUP_ROBOT_SECUBOT" );
static const mongroup_id GROUP_SEWER( "GROUP_SEWER" );
static const mongroup_id GROUP_SLIME( "GROUP_SLIME" );
static const mongroup_id GROUP_SPIDER( "GROUP_SPIDER" );
static const mongroup_id GROUP_TRIFFID( "GROUP_TRIFFID" );
static const mongroup_id GROUP_TRIFFID_HEART( "GROUP_TRIFFID_HEART" );
static const mongroup_id GROUP_TRIFFID_OUTER( "GROUP_TRIFFID_OUTER" );
static const mongroup_id GROUP_TURRET( "GROUP_TURRET" );

static const trait_id trait_NPC_STATIC_NPC( "NPC_STATIC_NPC" );

#define dbg(x) DebugLog((x),D_MAP_GEN) << __FILE__ << ":" << __LINE__ << ": "

static constexpr int MON_RADIUS = 3;

static void science_room( map *m, const point &p1, const point &p2, int z, int rotate );

// (x,y,z) are absolute coordinates of a submap
// x%2 and y%2 must be 0!
void map::generate( const tripoint &p, const time_point &when )
{
    dbg( D_INFO ) << "map::generate( g[" << g.get() << "], p[" << p << "], "
                  "when[" << to_string( when ) << "] )";

    set_abs_sub( p );

    // First we have to create new submaps and initialize them to 0 all over
    // We create all the submaps, even if we're not a tinymap, so that map
    //  generation which overflows won't cause a crash.  At the bottom of this
    //  function, we save the upper-left 4 submaps, and delete the rest.
    // Mapgen is not z-level aware yet. Only actually initialize current z-level
    //  because other submaps won't be touched.
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            const size_t grid_pos = get_nonant( { gridx, gridy, p.z } );
            if( getsubmap( grid_pos ) ) {
                debugmsg( "Submap already exists at (%d, %d, %d)", gridx, gridy, p.z );
                continue;
            }
            setsubmap( grid_pos, new submap() );
            // TODO: memory leak if the code below throws before the submaps get stored/deleted!
        }
    }
    // x, and y are submap coordinates, convert to overmap terrain coordinates
    // TODO: fix point types
    tripoint_abs_omt abs_omt( sm_to_omt_copy( p ) );
    oter_id terrain_type = overmap_buffer.ter( abs_omt );

    // This attempts to scale density of zombies inversely with distance from the nearest city.
    // In other words, make city centers dense and perimeters sparse.
    float density = 0.0f;
    for( int i = -MON_RADIUS; i <= MON_RADIUS; i++ ) {
        for( int j = -MON_RADIUS; j <= MON_RADIUS; j++ ) {
            density += overmap_buffer.ter( abs_omt + point( i, j ) )->get_mondensity();
        }
    }
    density = density / 100;

    mapgendata dat( abs_omt, *this, density, when, nullptr );
    draw_map( dat );

    // At some point, we should add region information so we can grab the appropriate extras
    map_extras ex = region_settings_map["default"].region_extras[terrain_type->get_extras()];
    if( ex.chance > 0 && one_in( ex.chance ) ) {
        std::string *extra = ex.values.pick();
        if( extra == nullptr ) {
            debugmsg( "failed to pick extra for type %s", terrain_type->get_extras() );
        } else {
            MapExtras::apply_function( *( ex.values.pick() ), *this, abs_sub );
        }
    }

    const auto &spawns = terrain_type->get_static_spawns();

    float spawn_density = 1.0f;
    if( MonsterGroupManager::is_animal( spawns.group ) ) {
        spawn_density = get_option< float >( "SPAWN_ANIMAL_DENSITY" );
    } else {
        spawn_density = get_option< float >( "SPAWN_DENSITY" );
    }

    // Apply a multiplier to the number of monsters for really high densities.
    float odds_after_density = spawns.chance * spawn_density;
    const float max_odds = 100 - ( 100 - spawns.chance ) / 2.0f;
    float density_multiplier = 1.0f;
    if( odds_after_density > max_odds ) {
        density_multiplier = 1.0f * odds_after_density / max_odds;
        odds_after_density = max_odds;
    }
    const int spawn_count = roll_remainder( density_multiplier );

    if( spawns.group && x_in_y( odds_after_density, 100 ) ) {
        int pop = spawn_count * rng( spawns.population.min, spawns.population.max );
        for( ; pop > 0; pop-- ) {
            MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup( spawns.group, &pop );
            if( !spawn_details.name ) {
                continue;
            }
            if( const cata::optional<tripoint> pt =
            random_point( *this, [this]( const tripoint & n ) {
            return passable( n );
            } ) ) {
                add_spawn( spawn_details, *pt );
            }
        }
    }

    // Okay, we know who are neighbors are.  Let's draw!
    // And finally save used submaps and delete the rest.
    for( int i = 0; i < my_MAPSIZE; i++ ) {
        for( int j = 0; j < my_MAPSIZE; j++ ) {
            dbg( D_INFO ) << "map::generate: submap (" << i << "," << j << ")";

            const tripoint pos( i, j, p.z );
            if( i <= 1 && j <= 1 ) {
                saven( pos );
            } else {
                const size_t grid_pos = get_nonant( pos );
                delete getsubmap( grid_pos );
                setsubmap( grid_pos, nullptr );
            }
        }
    }
}

void mapgen_function_builtin::generate( mapgendata &mgd )
{
    ( *fptr )( mgd );
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
///// mapgen_function class.
///// all sorts of ways to apply our hellish reality to a grid-o-squares

class mapgen_basic_container
{
    private:
        std::vector<std::shared_ptr<mapgen_function>> mapgens_;
        weighted_int_list<std::shared_ptr<mapgen_function>> weights_;

    public:
        int add( const std::shared_ptr<mapgen_function> &ptr ) {
            cata_assert( ptr );
            mapgens_.push_back( ptr );
            return mapgens_.size() - 1;
        }
        /**
         * Pick a mapgen function randomly and call its generate function.
         * This basically runs the mapgen functions with the given @ref mapgendata
         * as argument.
         * @return Whether the mapgen function has been run. It may not get run if
         * the list of mapgen functions is effectively empty.
         * @p hardcoded_weight Weight for an additional entry. If that entry is chosen,
         * false is returned. If unsure, just use 0 for it.
         */
        bool generate( mapgendata &dat, const int hardcoded_weight ) const {
            if( hardcoded_weight > 0 &&
                rng( 1, weights_.get_weight() + hardcoded_weight ) > weights_.get_weight() ) {
                return false;
            }
            const std::shared_ptr<mapgen_function> *const ptr = weights_.pick();
            if( !ptr ) {
                return false;
            }
            cata_assert( *ptr );
            ( *ptr )->generate( dat );
            return true;
        }
        /**
         * Calls @ref mapgen_function::setup and sets up the internal weighted list using
         * the **current** value of @ref mapgen_function::weight. This value may have
         * changed since it was first added, so this is needed to recalculate the weighted list.
         */
        void setup() {
            for( const std::shared_ptr<mapgen_function> &ptr : mapgens_ ) {
                const int weight = ptr->weight;
                if( weight < 1 ) {
                    continue; // rejected!
                }
                weights_.add( ptr, weight );
                ptr->setup();
            }
            // Not needed anymore, pointers are now stored in weights_ (or not used at all)
            mapgens_.clear();
        }
        void check_consistency() {
            for( auto &mapgen_function_ptr : weights_ ) {
                mapgen_function_ptr.obj->check();
            }
        }
};

class mapgen_factory
{
    private:
        std::map<std::string, mapgen_basic_container> mapgens_;

        /// Collect all the possible and expected keys that may get used with @ref pick.
        static std::set<std::string> get_usages() {
            std::set<std::string> result;
            for( const oter_t &elem : overmap_terrains::get_all() ) {
                result.insert( elem.get_mapgen_id() );
                result.insert( elem.id.str() );
            }
            // Why do I have to repeat the MapExtras here? Wouldn't "MapExtras::factory" be enough?
            for( const map_extra &elem : MapExtras::mapExtraFactory().get_all() ) {
                if( elem.generator_method == map_extra_method::mapgen ) {
                    result.insert( elem.generator_id );
                }
            }
            // Used in C++ code only, see calls to `oter_mapgen.generate()` below
            result.insert( "lab_1side" );
            result.insert( "lab_4side" );
            result.insert( "lab_finale_1level" );
            return result;
        }

    public:
        void reset() {
            mapgens_.clear();
        }
        /// @see mapgen_basic_container::setup
        void setup() {
            for( std::pair<const std::string, mapgen_basic_container> &omw : mapgens_ ) {
                omw.second.setup();
            }
            // Dummy entry, overmap terrain null should never appear and is therefore never generated.
            mapgens_.erase( "null" );
        }
        void check_consistency() {
            // Cache all strings that may get looked up here so we don't have to go through
            // all the sources for them upon each loop.
            const std::set<std::string> usages = get_usages();
            for( std::pair<const std::string, mapgen_basic_container> &omw : mapgens_ ) {
                omw.second.check_consistency();
                if( usages.count( omw.first ) == 0 ) {
                    debugmsg( "Mapgen %s is not used by anything!", omw.first );
                }
            }
        }
        /**
         * Checks whether we have an entry for the given key.
         * Note that the entry itself may not contain any valid mapgen instance
         * (could all have been removed via @ref erase).
         */
        bool has( const std::string &key ) const {
            return mapgens_.count( key ) != 0;
        }
        /// @see mapgen_basic_container::add
        int add( const std::string &key, const std::shared_ptr<mapgen_function> &ptr ) {
            return mapgens_[key].add( ptr );
        }
        /// @see mapgen_basic_container::generate
        bool generate( mapgendata &dat, const std::string &key, const int hardcoded_weight = 0 ) const {
            const auto iter = mapgens_.find( key );
            if( iter == mapgens_.end() ) {
                return false;
            }
            return iter->second.generate( dat, hardcoded_weight );
        }
};

static mapgen_factory oter_mapgen;

/*
 * stores function ref and/or required data
 */
std::map<std::string, weighted_int_list<std::shared_ptr<mapgen_function_json_nested>> >
        nested_mapgen;
std::map<std::string, std::vector<std::unique_ptr<update_mapgen_function_json>> > update_mapgen;

/*
 * setup mapgen_basic_container::weights_ which mapgen uses to diceroll. Also setup mapgen_function_json
 */
void calculate_mapgen_weights()   // TODO: rename as it runs jsonfunction setup too
{
    oter_mapgen.setup();
    // Not really calculate weights, but let's keep it here for now
    for( auto &pr : nested_mapgen ) {
        for( weighted_object<int, std::shared_ptr<mapgen_function_json_nested>> &ptr : pr.second ) {
            ptr.obj->setup();
        }
    }
    for( auto &pr : update_mapgen ) {
        for( auto &ptr : pr.second ) {
            ptr->setup();
        }
    }

}

void check_mapgen_definitions()
{
    oter_mapgen.check_consistency();
    for( auto &oter_definition : nested_mapgen ) {
        for( auto &mapgen_function_ptr : oter_definition.second ) {
            mapgen_function_ptr.obj->check();
        }
    }
    for( auto &oter_definition : update_mapgen ) {
        for( auto &mapgen_function_ptr : oter_definition.second ) {
            mapgen_function_ptr->check();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
///// json mapgen functions
///// 1 - init():

/**
 * Tiny little namespace to hold error messages
 */
namespace mapgen_defer
{
std::string member;
std::string message;
bool defer;
JsonObject jsi;
} // namespace mapgen_defer

static void set_mapgen_defer( const JsonObject &jsi, const std::string &member,
                              const std::string &message )
{
    mapgen_defer::defer = true;
    mapgen_defer::jsi = jsi;
    mapgen_defer::member = member;
    mapgen_defer::message = message;
}

/*
 * load a single mapgen json structure; this can be inside an overmap_terrain, or on it's own.
 */
std::shared_ptr<mapgen_function>
load_mapgen_function( const JsonObject &jio, const std::string &id_base, const point &offset )
{
    int mgweight = jio.get_int( "weight", 1000 );
    std::shared_ptr<mapgen_function> ret;
    if( mgweight <= 0 || jio.get_bool( "disabled", false ) ) {
        jio.allow_omitted_members();
        return nullptr; // nothing
    }
    const std::string mgtype = jio.get_string( "method" );
    if( mgtype == "builtin" ) {
        if( const building_gen_pointer ptr = get_mapgen_cfunction( jio.get_string( "name" ) ) ) {
            ret = std::make_shared<mapgen_function_builtin>( ptr, mgweight );
            oter_mapgen.add( id_base, ret );
        } else {
            jio.throw_error( "function does not exist", "name" );
        }
    } else if( mgtype == "json" ) {
        JsonObject jo = jio.get_object( "object" );
        const json_source_location jsrc = jo.get_source_location();
        jo.allow_omitted_members();
        ret = std::make_shared<mapgen_function_json>(
                  jsrc, mgweight, "mapgen " + id_base, offset );
        oter_mapgen.add( id_base, ret );
    } else {
        jio.throw_error( R"(invalid value: must be "builtin" or "json")", "method" );
    }
    return ret;
}

static void load_nested_mapgen( const JsonObject &jio, const std::string &id_base )
{
    const std::string mgtype = jio.get_string( "method" );
    if( mgtype == "json" ) {
        if( jio.has_object( "object" ) ) {
            int weight = jio.get_int( "weight", 1000 );
            JsonObject jo = jio.get_object( "object" );
            const json_source_location jsrc = jo.get_source_location();
            jo.allow_omitted_members();
            nested_mapgen[id_base].add(
                std::make_shared<mapgen_function_json_nested>( jsrc, "nested mapgen " + id_base ),
                weight );
        } else {
            debugmsg( "Nested mapgen: Invalid mapgen function (missing \"object\" object)", id_base.c_str() );
        }
    } else {
        debugmsg( "Nested mapgen: type for id %s was %s, but nested mapgen only supports \"json\"",
                  id_base.c_str(), mgtype.c_str() );
    }
}

static void load_update_mapgen( const JsonObject &jio, const std::string &id_base )
{
    const std::string mgtype = jio.get_string( "method" );
    if( mgtype == "json" ) {
        if( jio.has_object( "object" ) ) {
            JsonObject jo = jio.get_object( "object" );
            const json_source_location jsrc = jo.get_source_location();
            jo.allow_omitted_members();
            update_mapgen[id_base].push_back(
                std::make_unique<update_mapgen_function_json>(
                    jsrc, "update mapgen " + id_base ) );
        } else {
            debugmsg( "Update mapgen: Invalid mapgen function (missing \"object\" object)",
                      id_base.c_str() );
        }
    } else {
        debugmsg( "Update mapgen: type for id %s was %s, but update mapgen only supports \"json\"",
                  id_base.c_str(), mgtype.c_str() );
    }
}

/*
 * feed bits `o json from standalone file to load_mapgen_function. (standalone json "type": "mapgen")
 */
void load_mapgen( const JsonObject &jo )
{
    if( jo.has_array( "om_terrain" ) ) {
        JsonArray ja = jo.get_array( "om_terrain" );
        if( ja.test_array() ) {
            point offset;
            for( JsonArray row_items : ja ) {
                for( const std::string mapgenid : row_items ) {
                    const auto mgfunc = load_mapgen_function( jo, mapgenid, offset );
                    if( mgfunc ) {
                        oter_mapgen.add( mapgenid, mgfunc );
                    }
                    offset.x++;
                }
                offset.y++;
                offset.x = 0;
            }
        } else {
            std::vector<std::string> mapgenid_list;
            for( const std::string line : ja ) {
                mapgenid_list.push_back( line );
            }
            if( !mapgenid_list.empty() ) {
                const std::string mapgenid = mapgenid_list[0];
                const auto mgfunc = load_mapgen_function( jo, mapgenid, point_zero );
                if( mgfunc ) {
                    for( auto &i : mapgenid_list ) {
                        oter_mapgen.add( i, mgfunc );
                    }
                }
            }
        }
    } else if( jo.has_string( "om_terrain" ) ) {
        load_mapgen_function( jo, jo.get_string( "om_terrain" ), point_zero );
    } else if( jo.has_string( "nested_mapgen_id" ) ) {
        load_nested_mapgen( jo, jo.get_string( "nested_mapgen_id" ) );
    } else if( jo.has_string( "update_mapgen_id" ) ) {
        load_update_mapgen( jo, jo.get_string( "update_mapgen_id" ) );
    } else {
        debugmsg( "mapgen entry requires \"om_terrain\" or \"nested_mapgen_id\"(string, array of strings, or array of array of strings)\n%s\n",
                  jo.str() );
    }
}

void reset_mapgens()
{
    oter_mapgen.reset();
    nested_mapgen.clear();
    update_mapgen.clear();
}

/////////////////////////////////////////////////////////////////////////////////
///// 2 - right after init() finishes parsing all game json and terrain info/etc is set..
/////   ...parse more json! (mapgen_function_json)

size_t mapgen_function_json_base::calc_index( const point &p ) const
{
    if( p.x >= mapgensize.x ) {
        debugmsg( "invalid value %zu for x in calc_index", p.x );
    }
    if( p.y >= mapgensize.y ) {
        debugmsg( "invalid value %zu for y in calc_index", p.y );
    }
    return p.y * mapgensize.y + p.x;
}

static bool common_check_bounds( const jmapgen_int &x, const jmapgen_int &y,
                                 const point &mapgensize, const JsonObject &jso )
{
    half_open_rectangle<point> bounds( point_zero, mapgensize );
    if( !bounds.contains( point( x.val, y.val ) ) ) {
        return false;
    }

    if( x.valmax > mapgensize.x - 1 ) {
        jso.throw_error( "coordinate range cannot cross grid boundaries", "x" );
    }

    if( y.valmax > mapgensize.y - 1 ) {
        jso.throw_error( "coordinate range cannot cross grid boundaries", "y" );
    }

    return true;
}

bool mapgen_function_json_base::check_inbounds( const jmapgen_int &x, const jmapgen_int &y,
        const JsonObject &jso ) const
{
    return common_check_bounds( x, y, mapgensize, jso );
}

mapgen_function_json_base::mapgen_function_json_base(
    const json_source_location &jsrcloc, const std::string &context )
    : jsrcloc( jsrcloc )
    , context_( context )
    , do_format( false )
    , is_ready( false )
    , mapgensize( SEEX * 2, SEEY * 2 )
    , objects( m_offset, mapgensize )
{
}

mapgen_function_json_base::~mapgen_function_json_base() = default;

mapgen_function_json::mapgen_function_json( const json_source_location &jsrcloc, const int w,
        const std::string &context, const point &grid_offset )
    : mapgen_function( w )
    , mapgen_function_json_base( jsrcloc, context )
    , fill_ter( t_null )
    , rotation( 0 )
{
    m_offset.x = grid_offset.x * mapgensize.x;
    m_offset.y = grid_offset.y * mapgensize.y;
    objects = jmapgen_objects( m_offset, mapgensize );
}

mapgen_function_json_nested::mapgen_function_json_nested(
    const json_source_location &jsrcloc, const std::string &context )
    : mapgen_function_json_base( jsrcloc, context )
    , rotation( 0 )
{
}

jmapgen_int::jmapgen_int( point p ) : val( p.x ), valmax( p.y ) {}

jmapgen_int::jmapgen_int( const JsonObject &jo, const std::string &tag )
{
    if( jo.has_array( tag ) ) {
        JsonArray sparray = jo.get_array( tag );
        if( sparray.size() < 1 || sparray.size() > 2 ) {
            jo.throw_error( "invalid data: must be an array of 1 or 2 values", tag );
        }
        val = sparray.get_int( 0 );
        if( sparray.size() == 2 ) {
            valmax = sparray.get_int( 1 );
        } else {
            valmax = val;
        }
    } else {
        val = valmax = jo.get_int( tag );
    }
}

jmapgen_int::jmapgen_int( const JsonObject &jo, const std::string &tag, const int &def_val,
                          const int &def_valmax )
    : val( def_val )
    , valmax( def_valmax )
{
    if( jo.has_array( tag ) ) {
        JsonArray sparray = jo.get_array( tag );
        if( sparray.size() > 2 ) {
            jo.throw_error( "invalid data: must be an array of 1 or 2 values", tag );
        }
        if( sparray.size() >= 1 ) {
            val = sparray.get_int( 0 );
        }
        if( sparray.size() >= 2 ) {
            valmax = sparray.get_int( 1 );
        }
    } else if( jo.has_member( tag ) ) {
        val = valmax = jo.get_int( tag );
    }
}

int jmapgen_int::get() const
{
    return val == valmax ? val : rng( val, valmax );
}

/*
 * Turn json gobbldigook into machine friendly gobbldigook, for applying
 * basic map 'set' functions, optionally based on one_in(chance) or repeat value
 */
void mapgen_function_json_base::setup_setmap( const JsonArray &parray )
{
    std::string tmpval;
    std::map<std::string, jmapgen_setmap_op> setmap_opmap;
    setmap_opmap[ "terrain" ] = JMAPGEN_SETMAP_TER;
    setmap_opmap[ "furniture" ] = JMAPGEN_SETMAP_FURN;
    setmap_opmap[ "trap" ] = JMAPGEN_SETMAP_TRAP;
    setmap_opmap[ "radiation" ] = JMAPGEN_SETMAP_RADIATION;
    setmap_opmap[ "bash" ] = JMAPGEN_SETMAP_BASH;
    std::map<std::string, jmapgen_setmap_op>::iterator sm_it;
    jmapgen_setmap_op tmpop;
    int setmap_optype = 0;

    for( const JsonObject pjo : parray ) {
        if( pjo.read( "point", tmpval ) ) {
            setmap_optype = JMAPGEN_SETMAP_OPTYPE_POINT;
        } else if( pjo.read( "set", tmpval ) ) {
            setmap_optype = JMAPGEN_SETMAP_OPTYPE_POINT;
            debugmsg( "Warning, set: [ { \"set\": … } is deprecated, use set: [ { \"point\": … " );
        } else if( pjo.read( "line", tmpval ) ) {
            setmap_optype = JMAPGEN_SETMAP_OPTYPE_LINE;
        } else if( pjo.read( "square", tmpval ) ) {
            setmap_optype = JMAPGEN_SETMAP_OPTYPE_SQUARE;
        } else {
            pjo.throw_error( R"(invalid data: must contain "point", "set", "line" or "square" member)" );
        }

        sm_it = setmap_opmap.find( tmpval );
        if( sm_it == setmap_opmap.end() ) {
            pjo.throw_error( string_format( "invalid subfunction %s", tmpval.c_str() ) );
        }

        tmpop = sm_it->second;
        jmapgen_int tmp_x2( 0, 0 );
        jmapgen_int tmp_y2( 0, 0 );
        jmapgen_int tmp_i( 0, 0 );
        int tmp_chance = 1;
        int tmp_rotation = 0;
        int tmp_fuel = -1;
        int tmp_status = -1;

        const jmapgen_int tmp_x( pjo, "x" );
        const jmapgen_int tmp_y( pjo, "y" );
        if( !check_inbounds( tmp_x, tmp_y, pjo ) ) {
            pjo.allow_omitted_members();
            continue;
        }
        if( setmap_optype != JMAPGEN_SETMAP_OPTYPE_POINT ) {
            tmp_x2 = jmapgen_int( pjo, "x2" );
            tmp_y2 = jmapgen_int( pjo, "y2" );
            if( !check_inbounds( tmp_x2, tmp_y2, pjo ) ) {
                pjo.allow_omitted_members();
                continue;
            }
        }
        if( tmpop == JMAPGEN_SETMAP_RADIATION ) {
            tmp_i = jmapgen_int( pjo, "amount" );
        } else if( tmpop == JMAPGEN_SETMAP_BASH ) {
            //suppress warning
        } else {
            std::string tmpid = pjo.get_string( "id" );
            switch( tmpop ) {
                case JMAPGEN_SETMAP_TER: {
                    const ter_str_id tid( tmpid );

                    if( !tid.is_valid() ) {
                        set_mapgen_defer( pjo, "id", "no such terrain" );
                        return;
                    }
                    tmp_i.val = tid.id().to_i();
                }
                break;
                case JMAPGEN_SETMAP_FURN: {
                    const furn_str_id fid( tmpid );

                    if( !fid.is_valid() ) {
                        set_mapgen_defer( pjo, "id", "no such furniture" );
                        return;
                    }
                    tmp_i.val = fid.id().to_i();
                }
                break;
                case JMAPGEN_SETMAP_TRAP: {
                    const trap_str_id sid( tmpid );
                    if( !sid.is_valid() ) {
                        set_mapgen_defer( pjo, "id", "no such trap" );
                        return;
                    }
                    tmp_i.val = sid.id().to_i();
                }
                break;

                default:
                    //Suppress warnings
                    break;
            }
            // TODO: ... support for random furniture? or not.
            tmp_i.valmax = tmp_i.val;
        }
        // TODO: sanity check?
        const jmapgen_int tmp_repeat = jmapgen_int( pjo, "repeat", 1, 1 );
        pjo.read( "chance", tmp_chance );
        pjo.read( "rotation", tmp_rotation );
        pjo.read( "fuel", tmp_fuel );
        pjo.read( "status", tmp_status );
        jmapgen_setmap tmp( tmp_x, tmp_y, tmp_x2, tmp_y2,
                            static_cast<jmapgen_setmap_op>( tmpop + setmap_optype ), tmp_i,
                            tmp_chance, tmp_repeat, tmp_rotation, tmp_fuel, tmp_status );

        setmap_points.push_back( tmp );
        tmpval.clear();
    }

}

jmapgen_place::jmapgen_place( const JsonObject &jsi )
    : x( jsi, "x" )
    , y( jsi, "y" )
    , repeat( jsi, "repeat", 1, 1 )
{
}

void jmapgen_place::offset( const point &offset )
{
    x.val -= offset.x;
    x.valmax -= offset.x;
    y.val -= offset.y;
    y.valmax -= offset.y;
}

map_key::map_key( const std::string &s ) : str( s )
{
    if( utf8_width( str ) != 1 ) {
        debugmsg( "map key '%s' must be 1 column", str );
    }
}

map_key::map_key( const JsonMember &member ) : str( member.name() )
{
    if( utf8_width( str ) != 1 ) {
        member.throw_error( "format map key must be 1 column" );
    }
}

/**
 * This is a generic mapgen piece, the template parameter PieceType should be another specific
 * type of jmapgen_piece. This class contains a vector of those objects and will chose one of
 * it at random.
 */
template<typename PieceType>
class jmapgen_alternatively : public jmapgen_piece
{
    public:
        // Note: this bypasses virtual function system, all items in this vector are of type
        // PieceType, they *can not* be of any other type.
        std::vector<PieceType> alternatives;
        jmapgen_alternatively() = default;
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            if( const auto chosen = random_entry_opt( alternatives ) ) {
                chosen->get().apply( dat, x, y );
            }
        }
        bool has_vehicle_collision( const mapgendata &dat, const point &p ) const override {
            return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
        }
};

/**
 * Places fields on the map.
 * "field": field type ident.
 * "intensity": initial field intensity.
 * "age": initial field age.
 */
class jmapgen_field : public jmapgen_piece
{
    public:
        field_type_id ftype;
        int intensity;
        time_duration age;
        jmapgen_field( const JsonObject &jsi, const std::string &/*context*/ ) :
            ftype( field_type_id( jsi.get_string( "field" ) ) )
            , intensity( jsi.get_int( "intensity", 1 ) )
            , age( time_duration::from_turns( jsi.get_int( "age", 0 ) ) ) {
            if( !ftype.id() ) {
                set_mapgen_defer( jsi, "field", "invalid field type" );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            dat.m.add_field( tripoint( x.get(), y.get(), dat.m.get_abs_sub().z ), ftype, intensity, age );
        }
};
/**
 * Place an NPC.
 * "class": the npc class, see @ref map::place_npc
 */
class jmapgen_npc : public jmapgen_piece
{
    public:
        string_id<npc_template> npc_class;
        bool target;
        std::vector<std::string> traits;
        jmapgen_npc( const JsonObject &jsi, const std::string &/*context*/ ) :
            npc_class( jsi.get_string( "class" ) )
            , target( jsi.get_bool( "target", false ) ) {
            if( !npc_class.is_valid() ) {
                set_mapgen_defer( jsi, "class", string_format( "unknown npc template %s",
                                  jsi.get_string( "class" ) ) );
            }
            if( jsi.has_string( "add_trait" ) ) {
                std::string new_trait = jsi.get_string( "add_trait" );
                traits.emplace_back( new_trait );
            } else if( jsi.has_array( "add_trait" ) ) {
                for( const std::string new_trait : jsi.get_array( "add_trait" ) ) {
                    traits.emplace_back( new_trait );
                }
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            character_id npc_id = dat.m.place_npc( point( x.get(), y.get() ), npc_class );
            if( dat.mission() && target ) {
                dat.mission()->set_target_npc_id( npc_id );
            }
            npc *p = g->find_npc( npc_id );
            if( p != nullptr ) {
                for( const std::string &new_trait : traits ) {
                    p->set_mutation( trait_id( new_trait ) );
                }
            }
        }
};
/**
* Place ownership area
*/
class jmapgen_faction : public jmapgen_piece
{
    public:
        faction_id id;
        jmapgen_faction( const JsonObject &jsi, const std::string &/*context*/ ) {
            if( jsi.has_string( "id" ) ) {
                id = faction_id( jsi.get_string( "id" ) );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            dat.m.apply_faction_ownership( point( x.val, y.val ), point( x.valmax, y.valmax ), id );
        }
};
/**
 * Place a sign with some text.
 * "signage": the text on the sign.
 */
class jmapgen_sign : public jmapgen_piece
{
    public:
        translation signage;
        std::string snippet;
        jmapgen_sign( const JsonObject &jsi, const std::string &/*context*/ ) :
            snippet( jsi.get_string( "snippet", "" ) ) {
            jsi.read( "signage", signage );
            if( signage.empty() && snippet.empty() ) {
                jsi.throw_error( "jmapgen_sign: needs either signage or snippet" );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            const point r( x.get(), y.get() );
            dat.m.furn_set( r, f_null );
            dat.m.furn_set( r, furn_str_id( "f_sign" ) );

            std::string signtext;

            if( !snippet.empty() ) {
                // select a snippet from the category
                signtext = SNIPPET.random_from_category( snippet ).value_or( translation() ).translated();
            } else if( !signage.empty() ) {
                signtext = signage.translated();
            }
            if( !signtext.empty() ) {
                // replace tags
                std::string cityname = "illegible city name";
                tripoint abs_sub = dat.m.get_abs_sub();
                // TODO: fix point types
                const city *c = overmap_buffer.closest_city( tripoint_abs_sm( abs_sub ) ).city;
                if( c != nullptr ) {
                    cityname = c->name;
                }
                signtext = apply_all_tags( signtext, cityname );
            }
            dat.m.set_signage( tripoint( r, dat.m.get_abs_sub().z ), signtext );
        }
        std::string apply_all_tags( std::string signtext, const std::string &cityname ) const {
            replace_city_tag( signtext, cityname );
            replace_name_tags( signtext );
            return signtext;
        }
        bool has_vehicle_collision( const mapgendata &dat, const point &p ) const override {
            return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
        }
};
/**
 * Place graffiti with some text or a snippet.
 * "text": the text of the graffiti.
 * "snippet": snippet category to pull from for text instead.
 */
class jmapgen_graffiti : public jmapgen_piece
{
    public:
        translation text;
        std::string snippet;
        jmapgen_graffiti( const JsonObject &jsi, const std::string &/*context*/ ) :
            snippet( jsi.get_string( "snippet", "" ) ) {
            jsi.read( "text", text );
            if( text.empty() && snippet.empty() ) {
                jsi.throw_error( "jmapgen_graffiti: needs either text or snippet" );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            const point r( x.get(), y.get() );

            std::string graffiti;

            if( !snippet.empty() ) {
                // select a snippet from the category
                graffiti = SNIPPET.random_from_category( snippet ).value_or( translation() ).translated();
            } else if( !text.empty() ) {
                graffiti = text.translated();
            }
            if( !graffiti.empty() ) {
                // replace tags
                std::string cityname = "illegible city name";
                tripoint abs_sub = dat.m.get_abs_sub();
                // TODO: fix point types
                const city *c = overmap_buffer.closest_city( tripoint_abs_sm( abs_sub ) ).city;
                if( c != nullptr ) {
                    cityname = c->name;
                }
                graffiti = apply_all_tags( graffiti, cityname );
            }
            dat.m.set_graffiti( tripoint( r, dat.m.get_abs_sub().z ), graffiti );
        }
        std::string apply_all_tags( std::string graffiti, const std::string &cityname ) const {
            replace_city_tag( graffiti, cityname );
            replace_name_tags( graffiti );
            return graffiti;
        }
};
/**
 * Place a vending machine with content.
 * "item_group": the item group that is used to generate the content of the vending machine.
 */
class jmapgen_vending_machine : public jmapgen_piece
{
    public:
        bool reinforced;
        item_group_id group_id;
        jmapgen_vending_machine( const JsonObject &jsi, const std::string &/*context*/ ) :
            reinforced( jsi.get_bool( "reinforced", false ) )
            , group_id( jsi.get_string( "item_group", "default_vending_machine" ) ) {
            if( !item_group::group_is_defined( group_id ) ) {
                set_mapgen_defer( jsi, "item_group", "no such item group" );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            const point r( x.get(), y.get() );
            dat.m.furn_set( r, f_null );
            dat.m.place_vending( r, group_id, reinforced );
        }
        bool has_vehicle_collision( const mapgendata &dat, const point &p ) const override {
            return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
        }
};
/**
 * Place a toilet with (dirty) water in it.
 * "amount": number of water charges to place.
 */
class jmapgen_toilet : public jmapgen_piece
{
    public:
        jmapgen_int amount;
        jmapgen_toilet( const JsonObject &jsi, const std::string &/*context*/ ) :
            amount( jsi, "amount", 0, 0 ) {
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            const point r( x.get(), y.get() );
            const int charges = amount.get();
            dat.m.furn_set( r, f_null );
            if( charges == 0 ) {
                dat.m.place_toilet( r ); // Use the default charges supplied as default values
            } else {
                dat.m.place_toilet( r, charges );
            }
        }
        bool has_vehicle_collision( const mapgendata &dat, const point &p ) const override {
            return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
        }
};
/**
 * Place a gas pump with fuel in it.
 * "amount": number of fuel charges to place.
 */
class jmapgen_gaspump : public jmapgen_piece
{
    public:
        jmapgen_int amount;
        std::string fuel;
        jmapgen_gaspump( const JsonObject &jsi, const std::string &/*context*/ ) :
            amount( jsi, "amount", 0, 0 ) {
            if( jsi.has_string( "fuel" ) ) {
                fuel = jsi.get_string( "fuel" );

                // may want to not force this, if we want to support other fuels for some reason
                if( fuel != "gasoline" && fuel != "diesel" && fuel != "jp8" ) {
                    jsi.throw_error( "invalid fuel", "fuel" );
                }
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            const point r( x.get(), y.get() );
            int charges = amount.get();
            dat.m.furn_set( r, f_null );
            if( charges == 0 ) {
                charges = rng( 10000, 50000 );
            }
            if( !fuel.empty() ) {
                dat.m.place_gas_pump( r, charges, fuel );
            } else {
                dat.m.place_gas_pump( r, charges );
            }
        }
        bool has_vehicle_collision( const mapgendata &dat, const point &p ) const override {
            return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
        }
};

/**
 * Place a specific liquid into the map.
 * "liquid": id of the liquid item (item should use charges)
 * "amount": quantity of liquid placed (a value of 0 uses the default amount)
 * "chance": chance of liquid being placed, see @ref map::place_items
 */
class jmapgen_liquid_item : public jmapgen_piece
{
    public:
        jmapgen_int amount;
        std::string liquid;
        jmapgen_int chance;
        jmapgen_liquid_item( const JsonObject &jsi, const std::string &/*context*/ ) :
            amount( jsi, "amount", 0, 0 )
            , liquid( jsi.get_string( "liquid" ) )
            , chance( jsi, "chance", 1, 1 ) {
            // Itemgroups apply migrations when being loaded, but we need to migrate
            // individual items here.
            liquid = item_controller->migrate_id( itype_id( liquid ) ).str();
            if( !item::type_is_defined( itype_id( liquid ) ) ) {
                set_mapgen_defer( jsi, "liquid", "no such item type '" + liquid + "'" );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            if( one_in( chance.get() ) ) {
                item newliquid( liquid, calendar::start_of_cataclysm );
                if( amount.valmax > 0 ) {
                    newliquid.charges = amount.get();
                }
                dat.m.add_item_or_charges( tripoint( x.get(), y.get(), dat.m.get_abs_sub().z ), newliquid );
            }
        }
};

/**
 * Place items from an item group.
 * "item": id of the item group.
 * "chance": chance of items being placed, see @ref map::place_items
 * "repeat": number of times to apply this piece
 */
class jmapgen_item_group : public jmapgen_piece
{
    public:
        item_group_id group_id;
        jmapgen_int chance;
        jmapgen_item_group( const JsonObject &jsi, const std::string &context ) :
            chance( jsi, "chance", 1, 1 ) {
            JsonValue group = jsi.get_member( "item" );
            group_id = item_group::load_item_group( group, "collection",
                                                    "mapgen item group " + context );
            repeat = jmapgen_int( jsi, "repeat", 1, 1 );
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            dat.m.place_items( group_id, chance.get(), point( x.val, y.val ), point( x.valmax, y.valmax ), true,
                               calendar::start_of_cataclysm );
        }
};

/** Place items from an item group */
class jmapgen_loot : public jmapgen_piece
{
        friend jmapgen_objects;

    public:
        explicit jmapgen_loot( const JsonObject &jsi ) :
            result_group( Item_group::Type::G_COLLECTION, 100, jsi.get_int( "ammo", 0 ),
                          jsi.get_int( "magazine", 0 ), "mapgen loot entry" )
            , chance( jsi.get_int( "chance", 100 ) ) {
            const item_group_id group( jsi.get_string( "group", std::string() ) );
            itype_id ity;
            jsi.read( "item", ity );

            if( group.is_empty() == ity.is_empty() ) {
                jsi.throw_error( "must provide either item or group" );
            }
            if( !group.is_empty() && !item_group::group_is_defined( group ) ) {
                set_mapgen_defer( jsi, "group", "no such item group" );
            }
            if( !ity.is_empty() && !item::type_is_defined( ity ) ) {
                set_mapgen_defer( jsi, "item", "no such item type '" + ity.str() + "'" );
            }

            // All the probabilities are 100 because we do the roll in @ref apply.
            if( group.is_empty() ) {
                // Migrations are applied to item *groups* on load, but single item spawns must be
                // migrated individually
                result_group.add_item_entry( item_controller->migrate_id( ity ), 100 );
            } else {
                result_group.add_group_entry( group, 100 );
            }
        }

        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            if( rng( 0, 99 ) < chance ) {
                const Item_spawn_data *const isd = &result_group;
                const std::vector<item> spawn = isd->create( calendar::start_of_cataclysm,
                                                spawn_flags::use_spawn_rate );
                dat.m.spawn_items( tripoint( rng( x.val, x.valmax ), rng( y.val, y.valmax ),
                                             dat.m.get_abs_sub().z ), spawn );
            }
        }

    private:
        Item_group result_group;
        int chance;
};

/**
 * Place spawn points for a monster group (actual monster spawning is done later).
 * "monster": id of the monster group.
 * "chance": see @ref map::place_spawns
 * "density": see @ref map::place_spawns
 */
class jmapgen_monster_group : public jmapgen_piece
{
    public:
        mongroup_id id;
        float density;
        jmapgen_int chance;
        jmapgen_monster_group( const JsonObject &jsi, const std::string &/*context*/ ) :
            id( jsi.get_string( "monster" ) )
            , density( jsi.get_float( "density", -1.0f ) )
            , chance( jsi, "chance", 1, 1 ) {
            if( !id.is_valid() ) {
                set_mapgen_defer( jsi, "monster", "no such monster group" );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            dat.m.place_spawns( id, chance.get(), point( x.val, y.val ), point( x.valmax, y.valmax ),
                                density == -1.0f ? dat.monster_density() : density );
        }
};
/**
 * Place spawn points for a specific monster.
 * "monster": id of the monster. or "group": id of the monster group.
 * "friendly": whether the new monster is friendly to the player character.
 * "name": the name of the monster (if it has one).
 * "chance": the percentage chance of a monster, affected by spawn density
 *     If high density means greater than one hundred percent, can place multiples.
 * "repeat": roll this many times for creatures, potentially spawning multiples.
 * "pack_size": place this many creatures each time a roll is successful.
 * "one_or_none": place max of 1 (or pack_size) monsters, even if spawn density > 1.
 *     Defaults to true if repeat and pack_size are unset, false if one is set.
 */
class jmapgen_monster : public jmapgen_piece
{
    public:
        weighted_int_list<mtype_id> ids;
        mongroup_id m_id = mongroup_id::NULL_ID();
        jmapgen_int chance;
        jmapgen_int pack_size;
        bool one_or_none;
        bool friendly;
        std::string name;
        bool target;
        struct spawn_data data;
        jmapgen_monster( const JsonObject &jsi, const std::string &/*context*/ ) :
            chance( jsi, "chance", 100, 100 )
            , pack_size( jsi, "pack_size", 1, 1 )
            , one_or_none( jsi.get_bool( "one_or_none",
                                         !( jsi.has_member( "repeat" ) || jsi.has_member( "pack_size" ) ) ) )
            , friendly( jsi.get_bool( "friendly", false ) )
            , name( jsi.get_string( "name", "NONE" ) )
            , target( jsi.get_bool( "target", false ) ) {
            if( jsi.has_string( "group" ) ) {
                m_id = mongroup_id( jsi.get_string( "group" ) );
                if( !m_id.is_valid() ) {
                    set_mapgen_defer( jsi, "group", "no such monster group" );
                    return;
                }
            } else if( jsi.has_array( "monster" ) ) {
                for( const JsonValue entry : jsi.get_array( "monster" ) ) {
                    mtype_id id;
                    int weight = 100;
                    if( entry.test_array() ) {
                        JsonArray inner = entry.get_array();
                        id = mtype_id( inner.get_string( 0 ) );
                        weight = inner.get_int( 1 );
                    } else {
                        id = mtype_id( entry.get_string() );
                    }
                    if( !id.is_valid() ) {
                        set_mapgen_defer( jsi, "monster", "no such monster" );
                        return;
                    }
                    ids.add( id, weight );
                }
            } else {
                mtype_id id = mtype_id( jsi.get_string( "monster" ) );
                if( !id.is_valid() ) {
                    set_mapgen_defer( jsi, "monster", "no such monster" );
                    return;
                }
                ids.add( id, 100 );
            }

            if( jsi.has_object( "spawn_data" ) ) {
                const JsonObject &sd = jsi.get_object( "spawn_data" );
                if( sd.has_array( "ammo" ) ) {
                    const JsonArray &ammos = sd.get_array( "ammo" );
                    for( const JsonObject adata : ammos ) {
                        data.ammo.emplace( itype_id( adata.get_string( "ammo_id" ) ), jmapgen_int( adata, "qty" ) );
                    }
                }
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {

            int raw_odds = chance.get();

            // Handle spawn density: Increase odds, but don't let the odds of absence go below half the odds at density 1.
            // Instead, apply a multiplier to the number of monsters for really high densities.
            // For example, a 50% chance at spawn density 4 becomes a 75% chance of ~2.7 monsters.
            int odds_after_density = raw_odds * get_option<float>( "SPAWN_DENSITY" );
            int max_odds = ( 100 + raw_odds ) / 2;
            float density_multiplier = 1.0f;
            if( odds_after_density > max_odds ) {
                density_multiplier = 1.0f * odds_after_density / max_odds;
                odds_after_density = max_odds;
            }

            int mission_id = -1;
            if( dat.mission() && target ) {
                mission_id = dat.mission()->get_id();
            }

            int spawn_count = roll_remainder( density_multiplier );

            if( one_or_none ) { // don't let high spawn density alone cause more than 1 to spawn.
                spawn_count = std::min( spawn_count, 1 );
            }
            if( raw_odds == 100 ) { // don't spawn less than 1 if odds were 100%, even with low spawn density.
                spawn_count = std::max( spawn_count, 1 );
            } else {
                if( !x_in_y( odds_after_density, 100 ) ) {
                    return;
                }
            }

            if( m_id != mongroup_id::NULL_ID() ) {
                MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup( m_id );
                dat.m.add_spawn( spawn_details.name, spawn_count * pack_size.get(),
                { x.get(), y.get(), dat.m.get_abs_sub().z },
                friendly, -1, mission_id, name, data );
            } else {
                dat.m.add_spawn( *( ids.pick() ), spawn_count * pack_size.get(),
                { x.get(), y.get(), dat.m.get_abs_sub().z },
                friendly, -1, mission_id, name, data );
            }
        }
};

/**
 * Place a vehicle.
 * "vehicle": id of the vehicle.
 * "chance": chance of spawning the vehicle: 0...100
 * "rotation": rotation of the vehicle, see @ref vehicle::vehicle
 * "fuel": fuel status of the vehicle, see @ref vehicle::vehicle
 * "status": overall (damage) status of the vehicle, see @ref vehicle::vehicle
 */
class jmapgen_vehicle : public jmapgen_piece
{
    public:
        vgroup_id type;
        jmapgen_int chance;
        std::vector<units::angle> rotation;
        int fuel;
        int status;
        jmapgen_vehicle( const JsonObject &jsi, const std::string &/*context*/ ) :
            type( jsi.get_string( "vehicle" ) )
            , chance( jsi, "chance", 1, 1 )
            //, rotation( jsi.get_int( "rotation", 0 ) ) // unless there is a way for the json parser to
            // return a single int as a list, we have to manually check this in the constructor below
            , fuel( jsi.get_int( "fuel", -1 ) )
            , status( jsi.get_int( "status", -1 ) ) {
            if( jsi.has_array( "rotation" ) ) {
                for( const JsonValue elt : jsi.get_array( "rotation" ) ) {
                    rotation.push_back( units::from_degrees( elt.get_int() ) );
                }
            } else {
                rotation.push_back( units::from_degrees( jsi.get_int( "rotation", 0 ) ) );
            }

            if( !type.is_valid() ) {
                set_mapgen_defer( jsi, "vehicle", "no such vehicle type or group" );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            if( !x_in_y( chance.get(), 100 ) ) {
                return;
            }
            dat.m.add_vehicle( type, point( x.get(), y.get() ), random_entry( rotation ), fuel, status );
        }
        bool has_vehicle_collision( const mapgendata &dat, const point &p ) const override {
            return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
        }
};
/**
 * Place a specific item.
 * "item": id of item type to spawn.
 * "chance": chance of spawning it (1 = always, otherwise one_in(chance)).
 * "amount": amount of items to spawn.
 * "repeat": number of times to apply this piece
 */
class jmapgen_spawn_item : public jmapgen_piece
{
    public:
        itype_id type;
        jmapgen_int amount;
        jmapgen_int chance;
        std::set<flag_id> flags;
        jmapgen_spawn_item( const JsonObject &jsi, const std::string &/*context*/ ) :
            type( jsi.get_string( "item" ) )
            , amount( jsi, "amount", 1, 1 )
            , chance( jsi, "chance", 100, 100 )
            , flags( jsi.get_tags<flag_id>( "custom-flags" ) ) {
            // Itemgroups apply migrations when being loaded, but we need to migrate
            // individual items here.
            type = item_controller->migrate_id( type );
            if( !item::type_is_defined( type ) ) {
                set_mapgen_defer( jsi, "item", "no such item" );
            }
            repeat = jmapgen_int( jsi, "repeat", 1, 1 );
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            const int c = chance.get();

            // 100% chance = exactly 1 item, otherwise scale by item spawn rate.
            const float spawn_rate = get_option<float>( "ITEM_SPAWNRATE" );
            int spawn_count = ( c == 100 ) ? 1 : roll_remainder( c * spawn_rate / 100.0f );
            for( int i = 0; i < spawn_count; i++ ) {
                dat.m.spawn_item( point( x.get(), y.get() ), type, amount.get(),
                                  0, calendar::start_of_cataclysm, 0, flags );
            }
        }
};
/**
 * Place a trap.
 * "trap": id of the trap.
 */
class jmapgen_trap : public jmapgen_piece
{
    public:
        trap_id id;
        jmapgen_trap( const JsonObject &jsi, const std::string &/*context*/ ) :
            id( 0 ) {
            const trap_str_id sid( jsi.get_string( "trap" ) );
            if( !sid.is_valid() ) {
                set_mapgen_defer( jsi, "trap", "no such trap" );
            }
            id = sid.id();
        }

        explicit jmapgen_trap( const std::string &tid ) :
            id( 0 ) {
            const trap_str_id sid( tid );
            if( !sid.is_valid() ) {
                throw std::runtime_error( "unknown trap type" );
            }
            id = sid.id();
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            const tripoint actual_loc = tripoint( x.get(), y.get(), dat.m.get_abs_sub().z );
            dat.m.trap_set( actual_loc, id );
        }
        bool has_vehicle_collision( const mapgendata &dat, const point &p ) const override {
            return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
        }
};
/**
 * Place a furniture.
 * "furn": id of the furniture.
 */
class jmapgen_furniture : public jmapgen_piece
{
    public:
        furn_id id;
        jmapgen_furniture( const JsonObject &jsi, const std::string &/*context*/ ) :
            jmapgen_furniture( jsi.get_string( "furn" ) ) {}
        explicit jmapgen_furniture( const std::string &fid ) : id( furn_id( fid ) ) {}
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            dat.m.furn_set( point( x.get(), y.get() ), id );
        }
        bool has_vehicle_collision( const mapgendata &dat, const point &p ) const override {
            return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
        }
};
/**
 * Place terrain.
 * "ter": id of the terrain.
 */
class jmapgen_terrain : public jmapgen_piece
{
    public:
        ter_id id;
        jmapgen_terrain( const JsonObject &jsi, const std::string &/*context*/ ) :
            jmapgen_terrain( jsi.get_string( "ter" ) ) {}
        explicit jmapgen_terrain( const std::string &tid ) : id( ter_id( tid ) ) {}
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            dat.m.ter_set( point( x.get(), y.get() ), id );
            // Delete furniture if a wall was just placed over it. TODO: need to do anything for fluid, monsters?
            if( dat.m.has_flag_ter( "WALL", point( x.get(), y.get() ) ) ) {
                dat.m.furn_set( point( x.get(), y.get() ), f_null );
                // and items, unless the wall has PLACE_ITEM flag indicating it stores things.
                if( !dat.m.has_flag_ter( "PLACE_ITEM", point( x.get(), y.get() ) ) ) {
                    dat.m.i_clear( tripoint( x.get(), y.get(), dat.m.get_abs_sub().z ) );
                }
            }
        }
        bool has_vehicle_collision( const mapgendata &dat, const point &p ) const override {
            return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
        }
};
/**
 * Run a transformation.
 * "transform": id of the ter_furn_transform to run.
 */
class jmapgen_ter_furn_transform: public jmapgen_piece
{
    public:
        ter_furn_transform_id id;
        jmapgen_ter_furn_transform( const JsonObject &jsi, const std::string &/*context*/ ) :
            jmapgen_ter_furn_transform( jsi.get_string( "transform" ) ) {}
        explicit jmapgen_ter_furn_transform( const std::string &rid ) : id( ter_furn_transform_id(
                        rid ) ) {}
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            id->transform( dat.m, tripoint( x.get(), y.get(), dat.m.get_abs_sub().z ) );
        }
};
/**
 * Calls @ref map::make_rubble to create rubble and destroy the existing terrain/furniture.
 * See map::make_rubble for explanation of the parameters.
 */
class jmapgen_make_rubble : public jmapgen_piece
{
    public:
        furn_id rubble_type = f_rubble;
        bool items = false;
        ter_id floor_type = t_dirt;
        bool overwrite = false;
        jmapgen_make_rubble( const JsonObject &jsi, const std::string &/*context*/ ) {
            if( jsi.has_string( "rubble_type" ) ) {
                rubble_type = furn_id( jsi.get_string( "rubble_type" ) );
            }
            jsi.read( "items", items );
            if( jsi.has_string( "floor_type" ) ) {
                floor_type = ter_id( jsi.get_string( "floor_type" ) );
            }
            jsi.read( "overwrite", overwrite );
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            dat.m.make_rubble( tripoint( x.get(), y.get(), dat.m.get_abs_sub().z ), rubble_type, items,
                               floor_type, overwrite );
        }
};

/**
 * Place a computer (console) with given stats and effects.
 * @param options Array of @ref computer_option
 * @param failures Array of failure effects (see @ref computer_failure)
 */
class jmapgen_computer : public jmapgen_piece
{
    public:
        translation name;
        translation access_denied;
        int security;
        std::vector<computer_option> options;
        std::vector<computer_failure> failures;
        bool target;
        jmapgen_computer( const JsonObject &jsi, const std::string &/*context*/ ) {
            jsi.read( "name", name );
            jsi.read( "access_denied", access_denied );
            security = jsi.get_int( "security", 0 );
            target = jsi.get_bool( "target", false );
            if( jsi.has_array( "options" ) ) {
                for( JsonObject jo : jsi.get_array( "options" ) ) {
                    options.emplace_back( computer_option::from_json( jo ) );
                }
            }
            if( jsi.has_array( "failures" ) ) {
                for( JsonObject jo : jsi.get_array( "failures" ) ) {
                    failures.emplace_back( computer_failure::from_json( jo ) );
                }
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            const point r( x.get(), y.get() );
            dat.m.furn_set( r, furn_str_id( "f_console" ) );
            computer *cpu = dat.m.add_computer( tripoint( r, dat.m.get_abs_sub().z ), name.translated(),
                                                security );
            for( const auto &opt : options ) {
                cpu->add_option( opt );
            }
            for( const auto &opt : failures ) {
                cpu->add_failure( opt );
            }
            if( target && dat.mission() ) {
                cpu->set_mission( dat.mission()->get_id() );
            }

            // The default access denied message is defined in computer's constructor
            if( !access_denied.empty() ) {
                cpu->set_access_denied_msg( access_denied.translated() );
            }
        }
        bool has_vehicle_collision( const mapgendata &dat, const point &p ) const override {
            return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
        }
};

/**
 * Place an item in furniture (expected to be used with NOITEM SEALED furniture like plants).
 * "item": item to spawn (object with usual parameters).
 * "items": item group to spawn (object with usual parameters).
 * "furniture": furniture to create around it.
 */
class jmapgen_sealed_item : public jmapgen_piece
{
    public:
        furn_id furniture;
        jmapgen_int chance;
        cata::optional<jmapgen_spawn_item> item_spawner;
        cata::optional<jmapgen_item_group> item_group_spawner;
        jmapgen_sealed_item( const JsonObject &jsi, const std::string &context )
            : furniture( jsi.get_string( "furniture" ) )
            , chance( jsi, "chance", 100, 100 ) {
            if( jsi.has_object( "item" ) ) {
                JsonObject item_obj = jsi.get_object( "item" );
                item_spawner = jmapgen_spawn_item( item_obj, "item for " + context );
            }
            if( jsi.has_object( "items" ) ) {
                JsonObject items_obj = jsi.get_object( "items" );
                item_group_spawner = jmapgen_item_group( items_obj, "items for " + context );
            }
        }

        void check( const std::string &context ) const override {
            const furn_t &furn = furniture.obj();
            std::string summary =
                string_format(
                    "sealed_item special in json mapgen for %s using furniture %s",
                    context, furn.id.str() );

            if( !furniture.is_valid() ) {
                debugmsg( "%s which is not valid furniture", summary );
            }

            if( !item_spawner && !item_group_spawner ) {
                debugmsg( "%s specifies neither an item nor an item group.  "
                          "It should specify at least one.",
                          summary );
                return;
            }

            if( furn.has_flag( "PLANT" ) ) {
                // plant furniture requires exactly one seed item within it
                if( item_spawner && item_group_spawner ) {
                    debugmsg( "%s (with flag PLANT) specifies both an item and an item group.  "
                              "It should specify exactly one.",
                              summary );
                    return;
                }

                if( item_spawner ) {
                    int count = item_spawner->amount.get();
                    if( count != 1 ) {
                        debugmsg( "%s (with flag PLANT) spawns %d items; it should spawn exactly "
                                  "one.", summary, count );
                        return;
                    }
                    int item_chance = item_spawner->chance.get();
                    if( item_chance != 100 ) {
                        debugmsg( "%s (with flag PLANT) spawns an item (%s) with probability %d%%; "
                                  "it should always spawn.  You can move the \"chance\" up to the "
                                  "sealed_item instead of the \"item\" within.",
                                  summary, item_spawner->type.str(), item_chance );
                        return;
                    }
                    const itype *spawned_type = item::find_type( item_spawner->type );
                    if( !spawned_type->seed ) {
                        debugmsg( "%s (with flag PLANT) spawns item type %s which is not a seed.",
                                  summary, spawned_type->get_id().str() );
                        return;
                    }
                }

                if( item_group_spawner ) {
                    int ig_chance = item_group_spawner->chance.get();
                    if( ig_chance != 100 ) {
                        debugmsg( "%s (with flag PLANT) spawns item group %s with chance %d.  "
                                  "It should have chance 100.  You can move the \"chance\" up to the "
                                  "sealed_item instead of the \"items\" within.",
                                  summary, item_group_spawner->group_id.str(), ig_chance );
                        return;
                    }
                    item_group_id group_id = item_group_spawner->group_id;
                    for( const itype *type : item_group::every_possible_item_from( group_id ) ) {
                        if( !type->seed ) {
                            debugmsg( "%s (with flag PLANT) spawns item group %s which can "
                                      "spawn item %s which is not a seed.",
                                      summary, group_id.str(), type->get_id().str() );
                            return;
                        }
                    }

                    /// TODO: Somehow check that the item group always produces exactly one item.
                }
            }
        }

        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            const int c = chance.get();

            // 100% chance = always generate, otherwise scale by item spawn rate.
            // (except is capped at 1)
            const float spawn_rate = get_option<float>( "ITEM_SPAWNRATE" );
            if( !x_in_y( ( c == 100 ) ? 1 : c * spawn_rate / 100.0f, 1 ) ) {
                return;
            }

            dat.m.furn_set( point( x.get(), y.get() ), f_null );
            if( item_spawner ) {
                item_spawner->apply( dat, x, y );
            }
            if( item_group_spawner ) {
                item_group_spawner->apply( dat, x, y );
            }
            dat.m.furn_set( point( x.get(), y.get() ), furniture );
        }
        bool has_vehicle_collision( const mapgendata &dat, const point &p ) const override {
            return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
        }
};
/**
 * Translate terrain from one ter_id to another.
 * "from": id of the starting terrain.
 * "to": id of the ending terrain
 * not useful for normal mapgen, very useful for mapgen_update
 */
class jmapgen_translate : public jmapgen_piece
{
    public:
        ter_id from;
        ter_id to;
        jmapgen_translate( const JsonObject &jsi, const std::string &/*context*/ ) {
            if( jsi.has_string( "from" ) && jsi.has_string( "to" ) ) {
                const std::string from_id = jsi.get_string( "from" );
                const std::string to_id = jsi.get_string( "to" );
                from = ter_id( from_id );
                to = ter_id( to_id );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &/*x*/,
                    const jmapgen_int &/*y*/ ) const override {
            dat.m.translate( from, to );
        }
};
/**
 * Place a zone
 */
class jmapgen_zone : public jmapgen_piece
{
    public:
        zone_type_id zone_type;
        faction_id faction;
        std::string name;
        jmapgen_zone( const JsonObject &jsi, const std::string &/*context*/ ) {
            if( jsi.has_string( "faction" ) && jsi.has_string( "type" ) ) {
                std::string fac_id = jsi.get_string( "faction" );
                faction = faction_id( fac_id );
                std::string zone_id = jsi.get_string( "type" );
                zone_type = zone_type_id( zone_id );
                if( jsi.has_string( "name" ) ) {
                    name = jsi.get_string( "name" );
                }
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            zone_manager &mgr = zone_manager::get_manager();
            const tripoint start = dat.m.getabs( tripoint( x.val, y.val, 0 ) );
            const tripoint end = dat.m.getabs( tripoint( x.valmax, y.valmax, 0 ) );
            mgr.add( name, zone_type, faction, false, true, start, end );
        }
};

static void load_weighted_entries( const JsonObject &jsi, const std::string &json_key,
                                   weighted_int_list<std::string> &list )
{
    for( const JsonValue entry : jsi.get_array( json_key ) ) {
        if( entry.test_array() ) {
            JsonArray inner = entry.get_array();
            list.add( inner.get_string( 0 ), inner.get_int( 1 ) );
        } else {
            list.add( entry.get_string(), 100 );
        }
    }
}

/**
 * Calls another mapgen call inside the current one.
 * Note: can't use regular overmap ids.
 * @param entries list of pairs [nested mapgen id, weight].
 */
class jmapgen_nested : public jmapgen_piece
{
    private:
        class neighborhood_check
        {
            private:
                // To speed up the most common case: no checks
                bool has_any = false;
                std::array<std::set<oter_str_id>, om_direction::size> neighbors;
                std::set<oter_str_id> above;
            public:
                explicit neighborhood_check( const JsonObject &jsi ) {
                    for( om_direction::type dir : om_direction::all ) {
                        int index = static_cast<int>( dir );
                        neighbors[index] = jsi.get_tags<oter_str_id>( om_direction::id( dir ) );
                        has_any |= !neighbors[index].empty();

                        above = jsi.get_tags<oter_str_id>( "above" );
                        has_any |= !above.empty();
                    }
                }

                bool test( const mapgendata &dat ) const {
                    if( !has_any ) {
                        return true;
                    }

                    bool all_directions_match  = true;
                    for( om_direction::type dir : om_direction::all ) {
                        int index = static_cast<int>( dir );
                        const std::set<oter_str_id> &allowed_neighbors = neighbors[index];

                        if( allowed_neighbors.empty() ) {
                            continue;  // no constraints on this direction, skip.
                        }

                        bool this_direction_matches = false;
                        for( const oter_str_id &allowed_neighbor : allowed_neighbors ) {
                            this_direction_matches |= is_ot_match( allowed_neighbor.str(), dat.neighbor_at( dir ).id(),
                                                                   ot_match_type::contains );
                        }
                        all_directions_match &= this_direction_matches;
                    }

                    if( !above.empty() ) {
                        bool above_matches = false;
                        for( const oter_str_id &allowed_neighbor : above ) {
                            above_matches |= is_ot_match( allowed_neighbor.str(), dat.above().id(), ot_match_type::contains );
                        }
                        all_directions_match &= above_matches;
                    }

                    return all_directions_match;
                }
        };

    public:
        weighted_int_list<std::string> entries;
        weighted_int_list<std::string> else_entries;
        neighborhood_check neighbors;
        jmapgen_nested( const JsonObject &jsi, const std::string &/*context*/ ) :
            neighbors( jsi.get_object( "neighbors" ) ) {
            load_weighted_entries( jsi, "chunks", entries );
            load_weighted_entries( jsi, "else_chunks", else_entries );
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y
                  ) const override {
            const std::string *res = neighbors.test( dat ) ? entries.pick() : else_entries.pick();
            if( res == nullptr || res->empty() || *res == "null" ) {
                // This will be common when neighbors.test(...) is false, since else_entires is often empty.
                return;
            }

            const auto iter = nested_mapgen.find( *res );
            if( iter == nested_mapgen.end() ) {
                debugmsg( "Unknown nested mapgen function id %s", res->c_str() );
                return;
            }

            // A second roll? Let's allow it for now
            const auto &ptr = iter->second.pick();
            if( ptr == nullptr ) {
                return;
            }

            ( *ptr )->nest( dat, point( x.get(), y.get() ) );
        }
        bool has_vehicle_collision( const mapgendata &dat, const point &p ) const override {
            const weighted_int_list<std::string> &selected_entries = neighbors.test(
                        dat ) ? entries : else_entries;
            if( selected_entries.empty() ) {
                return false;
            }

            for( const auto &entry : selected_entries ) {
                if( entry.obj == "null" ) {
                    continue;
                }
                const auto iter = nested_mapgen.find( entry.obj );
                if( iter == nested_mapgen.end() ) {
                    return false;
                }
                for( const auto &nest : iter->second ) {
                    if( nest.obj->has_vehicle_collision( dat, p ) ) {
                        return true;
                    }
                }
            }

            return false;
        }
};

jmapgen_objects::jmapgen_objects( const point &offset, const point &mapsize )
    : m_offset( offset )
    , mapgensize( mapsize )
{}

bool jmapgen_objects::check_bounds( const jmapgen_place &place, const JsonObject &jso )
{
    return common_check_bounds( place.x, place.y, mapgensize, jso );
}

void jmapgen_objects::add( const jmapgen_place &place,
                           const shared_ptr_fast<const jmapgen_piece> &piece )
{
    objects.emplace_back( place, piece );
}

template<typename PieceType>
void jmapgen_objects::load_objects( const JsonArray &parray, const std::string &context )
{
    for( JsonObject jsi : parray ) {
        jmapgen_place where( jsi );
        where.offset( m_offset );

        if( check_bounds( where, jsi ) ) {
            add( where, make_shared_fast<PieceType>( jsi, context ) );
        } else {
            jsi.allow_omitted_members();
        }
    }
}

template<>
void jmapgen_objects::load_objects<jmapgen_loot>(
    const JsonArray &parray, const std::string &/*context*/ )
{
    for( JsonObject jsi : parray ) {
        jmapgen_place where( jsi );
        where.offset( m_offset );

        if( !check_bounds( where, jsi ) ) {
            jsi.allow_omitted_members();
            continue;
        }

        auto loot = make_shared_fast<jmapgen_loot>( jsi );
        // spawn rates < 1 are handled in item_group
        const float rate = std::max( get_option<float>( "ITEM_SPAWNRATE" ), 1.0f );

        if( where.repeat.valmax != 1 ) {
            // if loot can repeat scale according to rate
            where.repeat.val = std::max( static_cast<int>( where.repeat.val * rate ), 1 );
            where.repeat.valmax = std::max( static_cast<int>( where.repeat.valmax * rate ), 1 );

        } else if( loot->chance != 100 ) {
            // otherwise except where chance is 100% scale probability
            loot->chance = std::max( std::min( static_cast<int>( loot->chance * rate ), 100 ), 1 );
        }

        add( where, loot );
    }
}

template<typename PieceType>
void jmapgen_objects::load_objects( const JsonObject &jsi, const std::string &member_name,
                                    const std::string &context )
{
    if( !jsi.has_member( member_name ) ) {
        return;
    }
    load_objects<PieceType>( jsi.get_array( member_name ), member_name + " in " + context );
}

template<typename PieceType>
void load_place_mapings( const JsonObject &jobj, mapgen_palette::placing_map::mapped_type &vect,
                         const std::string &context )
{
    vect.push_back( make_shared_fast<PieceType>( jobj, context ) );
}

/*
This is the default load function for mapgen pieces that only support loading from a json object,
not from a simple string.
Most non-trivial mapgen pieces (like item spawn which contains at least the item group and chance)
are like this. Other pieces (trap, furniture ...) can be loaded from a single string and have
an overload below.
The mapgen piece is loaded from the member of the json object named key.
*/
template<typename PieceType>
void load_place_mapings( const JsonValue &value, mapgen_palette::placing_map::mapped_type &vect,
                         const std::string &context )
{
    if( value.test_object() ) {
        load_place_mapings<PieceType>( value.get_object(), vect, context );
    } else {
        for( JsonObject jo : value.get_array() ) {
            load_place_mapings<PieceType>( jo, vect, context );
            jo.allow_omitted_members();
        }
    }
}

/*
This function allows loading the mapgen pieces from a single string, *or* a json object.
*/
template<typename PieceType>
void load_place_mapings_string(
    const JsonValue &value, mapgen_palette::placing_map::mapped_type &vect,
    const std::string &context )
{
    if( value.test_string() ) {
        try {
            vect.push_back( make_shared_fast<PieceType>( value.get_string() ) );
        } catch( const std::runtime_error &err ) {
            // Using the json object here adds nice formatting and context information
            value.throw_error( err.what() );
        }
    } else if( value.test_object() ) {
        load_place_mapings<PieceType>( value.get_object(), vect, context );
    } else {
        for( const JsonValue entry : value.get_array() ) {
            if( entry.test_string() ) {
                try {
                    vect.push_back( make_shared_fast<PieceType>( entry.get_string() ) );
                } catch( const std::runtime_error &err ) {
                    // Using the json object here adds nice formatting and context information
                    entry.throw_error( err.what() );
                }
            } else {
                load_place_mapings<PieceType>( entry.get_object(), vect, context );
            }
        }
    }
}
/*
This function is like load_place_mapings_string, except if the input is an array it will create an
instance of jmapgen_alternatively which will chose the mapgen piece to apply to the map randomly.
Use this with terrain or traps or other things that can not be applied twice to the same place.
*/
template<typename PieceType>
void load_place_mapings_alternatively(
    const JsonValue &value, mapgen_palette::placing_map::mapped_type &vect,
    const std::string &context )
{
    if( !value.test_array() ) {
        load_place_mapings_string<PieceType>( value, vect, context );
    } else {
        auto alter = make_shared_fast< jmapgen_alternatively<PieceType> >();
        for( const JsonValue entry : value.get_array() ) {
            if( entry.test_string() ) {
                try {
                    alter->alternatives.emplace_back( entry.get_string() );
                } catch( const std::runtime_error &err ) {
                    // Using the json object here adds nice formatting and context information
                    entry.throw_error( err.what() );
                }
            } else if( entry.test_object() ) {
                JsonObject jsi = entry.get_object();
                alter->alternatives.emplace_back( jsi, context );
            } else if( entry.test_array() ) {
                // If this is an array, it means it is an entry followed by a desired total count of instances.
                JsonArray piece_and_count_jarr = entry.get_array();
                if( piece_and_count_jarr.size() != 2 ) {
                    piece_and_count_jarr.throw_error( "Array must have exactly two entries: the object, then the count." );
                }

                // Test if this is a string or object, and then just emplace it.
                if( piece_and_count_jarr.test_string() ) {
                    try {
                        alter->alternatives.emplace_back( piece_and_count_jarr.next_string() );
                    } catch( const std::runtime_error &err ) {
                        piece_and_count_jarr.throw_error( err.what() );
                    }
                } else if( piece_and_count_jarr.test_object() ) {
                    JsonObject jsi = piece_and_count_jarr.next_object();
                    alter->alternatives.emplace_back( jsi, context );
                } else {
                    piece_and_count_jarr.throw_error( "First entry must be a string or object." );
                }

                if( piece_and_count_jarr.test_int() ) {
                    // We already emplaced the first instance, so do one less.
                    int repeat = std::max( 0, piece_and_count_jarr.next_int() - 1 );
                    PieceType piece_to_repeat = alter->alternatives.back();
                    for( int i = 0; i < repeat; i++ ) {
                        alter->alternatives.emplace_back( piece_to_repeat );
                    }
                } else {
                    piece_and_count_jarr.throw_error( "Second entry must be an integer." );
                }
            }
        }
        vect.push_back( alter );
    }
}

template<>
void load_place_mapings<jmapgen_trap>(
    const JsonValue &value, mapgen_palette::placing_map::mapped_type &vect,
    const std::string &context )
{
    load_place_mapings_alternatively<jmapgen_trap>( value, vect, context );
}

template<>
void load_place_mapings<jmapgen_furniture>( const JsonValue &value,
        mapgen_palette::placing_map::mapped_type &vect, const std::string &context )
{
    load_place_mapings_alternatively<jmapgen_furniture>( value, vect, context );
}

template<>
void load_place_mapings<jmapgen_terrain>( const JsonValue &value,
        mapgen_palette::placing_map::mapped_type &vect, const std::string &context )
{
    load_place_mapings_alternatively<jmapgen_terrain>( value, vect, context );
}

template<typename PieceType>
void mapgen_palette::load_place_mapings( const JsonObject &jo, const std::string &member_name,
        placing_map &format_placings, const std::string &context )
{
    if( jo.has_object( "mapping" ) ) {
        for( const JsonMember member : jo.get_object( "mapping" ) ) {
            const map_key key( member );
            JsonObject sub = member.get_object();
            sub.allow_omitted_members();
            if( !sub.has_member( member_name ) ) {
                continue;
            }
            auto &vect = format_placings[ key ];
            ::load_place_mapings<PieceType>( sub.get_member( member_name ), vect,
                                             member_name + " in mapping in " + context );
        }
    }
    if( !jo.has_object( member_name ) ) {
        return;
    }
    /* This is kind of a hack. Loading furniture/terrain from `jo` is already done in
     * mapgen_palette::load_temp, continuing here would load it again and cause trouble.
     */
    if( member_name == "terrain" || member_name == "furniture" ) {
        return;
    }
    for( const JsonMember member : jo.get_object( member_name ) ) {
        const map_key key( member );
        auto &vect = format_placings[ key ];
        ::load_place_mapings<PieceType>(
            member, vect, member_name + " " + member.name() + " in " + context );
    }
}

static std::map<std::string, mapgen_palette> palettes;

static bool check_furn( const furn_id &id, const std::string &context )
{
    const furn_t &furn = id.obj();
    if( furn.has_flag( "PLANT" ) ) {
        debugmsg( "json mapgen for %s specifies furniture %s, which has flag "
                  "PLANT.  Such furniture must be specified in a \"sealed_item\" special.",
                  context, furn.id.str() );
        // Only report once per mapgen object, otherwise the reports are
        // very repetitive
        return true;
    }
    return false;
}

void mapgen_palette::check()
{
    std::string context = "palette " + id;
    for( const std::pair<const map_key, furn_id> &p : format_furniture ) {
        if( check_furn( p.second, context ) ) {
            return;
        }
    }

    for( const std::pair<const map_key, std::vector<shared_ptr_fast<const jmapgen_piece>>> &p :
         format_placings ) {
        for( const shared_ptr_fast<const jmapgen_piece> &j : p.second ) {
            j->check( context );
        }
    }
}

mapgen_palette mapgen_palette::load_temp( const JsonObject &jo, const std::string &src )
{
    return load_internal( jo, src, false, true );
}

void mapgen_palette::load( const JsonObject &jo, const std::string &src )
{
    mapgen_palette ret = load_internal( jo, src, true, false );
    if( ret.id.empty() ) {
        jo.throw_error( "Named palette needs an id" );
    }

    palettes[ ret.id ] = ret;
}

const mapgen_palette &mapgen_palette::get( const palette_id &id )
{
    const auto iter = palettes.find( id );
    if( iter != palettes.end() ) {
        return iter->second;
    }

    debugmsg( "Requested palette with unknown id %s", id.c_str() );
    static mapgen_palette dummy;
    return dummy;
}

void mapgen_palette::check_definitions()
{
    for( auto &p : palettes ) {
        p.second.check();
    }
}

void mapgen_palette::reset()
{
    palettes.clear();
}

void mapgen_palette::add( const palette_id &rh )
{
    add( get( rh ) );
}

void mapgen_palette::add( const mapgen_palette &rh )
{
    for( const auto &placing : rh.format_placings ) {
        format_placings[ placing.first ] = placing.second;
    }
    for( const auto &placing : rh.format_terrain ) {
        format_terrain[ placing.first ] = placing.second;
    }
    for( const auto &placing : rh.format_furniture ) {
        format_furniture[ placing.first ] = placing.second;
    }
}

mapgen_palette mapgen_palette::load_internal( const JsonObject &jo, const std::string &,
        bool require_id, bool allow_recur )
{
    mapgen_palette new_pal;
    auto &format_placings = new_pal.format_placings;
    auto &format_terrain = new_pal.format_terrain;
    auto &format_furniture = new_pal.format_furniture;
    if( require_id ) {
        new_pal.id = jo.get_string( "id" );
    }

    if( jo.has_array( "palettes" ) ) {
        if( allow_recur ) {
            auto pals = jo.get_string_array( "palettes" );
            for( auto &p : pals ) {
                new_pal.add( p );
            }
        } else {
            jo.throw_error( "Recursive palettes are not implemented yet" );
        }
    }

    // mandatory: every character in rows must have matching entry, unless fill_ter is set
    // "terrain": { "a": "t_grass", "b": "t_lava" }
    if( jo.has_member( "terrain" ) ) {
        for( const JsonMember member : jo.get_object( "terrain" ) ) {
            const map_key key( member );
            if( member.test_string() ) {
                format_terrain[key] = ter_id( member.get_string() );
            } else {
                auto &vect = format_placings[ key ];
                ::load_place_mapings<jmapgen_terrain>(
                    member, vect, "terrain " + member.name() + " in palette " + new_pal.id );
                if( !vect.empty() ) {
                    // Dummy entry to signal that this terrain is actually defined, because
                    // the code below checks that each square on the map has a valid terrain
                    // defined somehow.
                    format_terrain[key] = t_null;
                }
            }
        }
    }

    if( jo.has_object( "furniture" ) ) {
        for( const JsonMember member : jo.get_object( "furniture" ) ) {
            const map_key key( member );
            if( member.test_string() ) {
                format_furniture[key] = furn_id( member.get_string() );
            } else {
                auto &vect = format_placings[ key ];
                ::load_place_mapings<jmapgen_furniture>(
                    member, vect, "furniture " + member.name() + " in palette " + new_pal.id );
            }
        }
    }
    std::string c = "palette " + new_pal.id;
    new_pal.load_place_mapings<jmapgen_field>( jo, "fields", format_placings, c );
    new_pal.load_place_mapings<jmapgen_npc>( jo, "npcs", format_placings, c );
    new_pal.load_place_mapings<jmapgen_sign>( jo, "signs", format_placings, c );
    new_pal.load_place_mapings<jmapgen_vending_machine>( jo, "vendingmachines", format_placings, c );
    new_pal.load_place_mapings<jmapgen_toilet>( jo, "toilets", format_placings, c );
    new_pal.load_place_mapings<jmapgen_gaspump>( jo, "gaspumps", format_placings, c );
    new_pal.load_place_mapings<jmapgen_item_group>( jo, "items", format_placings, c );
    new_pal.load_place_mapings<jmapgen_monster_group>( jo, "monsters", format_placings, c );
    new_pal.load_place_mapings<jmapgen_vehicle>( jo, "vehicles", format_placings, c );
    // json member name is not optimal, it should be plural like all the others above, but that conflicts
    // with the items entry with refers to item groups.
    new_pal.load_place_mapings<jmapgen_spawn_item>( jo, "item", format_placings, c );
    new_pal.load_place_mapings<jmapgen_trap>( jo, "traps", format_placings, c );
    new_pal.load_place_mapings<jmapgen_monster>( jo, "monster", format_placings, c );
    new_pal.load_place_mapings<jmapgen_furniture>( jo, "furniture", format_placings, c );
    new_pal.load_place_mapings<jmapgen_terrain>( jo, "terrain", format_placings, c );
    new_pal.load_place_mapings<jmapgen_make_rubble>( jo, "rubble", format_placings, c );
    new_pal.load_place_mapings<jmapgen_computer>( jo, "computers", format_placings, c );
    new_pal.load_place_mapings<jmapgen_sealed_item>( jo, "sealed_item", format_placings, c );
    new_pal.load_place_mapings<jmapgen_nested>( jo, "nested", format_placings, c );
    new_pal.load_place_mapings<jmapgen_liquid_item>( jo, "liquids", format_placings, c );
    new_pal.load_place_mapings<jmapgen_graffiti>( jo, "graffiti", format_placings, c );
    new_pal.load_place_mapings<jmapgen_translate>( jo, "translate", format_placings, c );
    new_pal.load_place_mapings<jmapgen_zone>( jo, "zones", format_placings, c );
    new_pal.load_place_mapings<jmapgen_ter_furn_transform>( jo, "ter_furn_transforms",
            format_placings, c );
    new_pal.load_place_mapings<jmapgen_faction>( jo, "faction_owner_character", format_placings, c );
    return new_pal;
}

bool mapgen_function_json::setup_internal( const JsonObject &jo )
{
    // Just to make sure no one does anything stupid
    if( jo.has_member( "mapgensize" ) ) {
        jo.throw_error( "\"mapgensize\" only allowed for nested mapgen" );
    }

    // something akin to mapgen fill_background.
    if( jo.has_string( "fill_ter" ) ) {
        fill_ter = ter_str_id( jo.get_string( "fill_ter" ) ).id();
    }

    if( jo.has_member( "rotation" ) ) {
        rotation = jmapgen_int( jo, "rotation" );
    }

    if( jo.has_member( "predecessor_mapgen" ) ) {
        predecessor_mapgen = oter_str_id( jo.get_string( "predecessor_mapgen" ) ).id();
    } else {
        predecessor_mapgen = oter_str_id::NULL_ID();
    }

    return fill_ter != t_null || predecessor_mapgen != oter_str_id::NULL_ID();
}

bool mapgen_function_json_nested::setup_internal( const JsonObject &jo )
{
    // Mandatory - nested mapgen must be explicitly sized
    if( jo.has_array( "mapgensize" ) ) {
        JsonArray jarr = jo.get_array( "mapgensize" );
        mapgensize = point( jarr.get_int( 0 ), jarr.get_int( 1 ) );
        if( mapgensize.x == 0 || mapgensize.x != mapgensize.y ) {
            // Non-square sizes not implemented yet
            jo.throw_error( "\"mapgensize\" must be an array of two identical, positive numbers" );
        }
    } else {
        jo.throw_error( "Nested mapgen must have \"mapgensize\" set" );
    }

    if( jo.has_member( "rotation" ) ) {
        rotation = jmapgen_int( jo, "rotation" );
    }

    // Nested mapgen is always halal because it can assume underlying map is.
    return true;
}

void mapgen_function_json::setup()
{
    setup_common();
}

void mapgen_function_json_nested::setup()
{
    setup_common();
}

void update_mapgen_function_json::setup()
{
    setup_common();
}

/*
 * Parse json, pre-calculating values for stuff, then cheerfully throw json away. Faster than regular mapf, in theory
 */
void mapgen_function_json_base::setup_common()
{
    if( is_ready ) {
        return;
    }
    if( !jsrcloc.path ) {
        debugmsg( "null json source location path" );
        return;
    }
    shared_ptr_fast<std::istream> stream = DynamicDataLoader::get_instance().get_cached_stream(
            *jsrcloc.path );
    JsonIn jsin( *stream, jsrcloc );
    JsonObject jo = jsin.get_object();
    mapgen_defer::defer = false;
    if( !setup_common( jo ) ) {
        jsin.error( "format: no terrain map" );
    }
    if( mapgen_defer::defer ) {
        mapgen_defer::jsi.throw_error( mapgen_defer::message, mapgen_defer::member );
    } else {
        mapgen_defer::jsi = JsonObject();
    }
}

bool mapgen_function_json_base::setup_common( const JsonObject &jo )
{
    bool fallback_terrain_exists = setup_internal( jo );
    JsonArray parray;
    JsonArray sparray;
    JsonObject pjo;

    format.resize( static_cast<size_t>( mapgensize.x * mapgensize.y ) );
    // just like mapf::basic_bind("stuff",blargle("foo", etc) ), only json input and faster when applying
    if( jo.has_array( "rows" ) ) {
        mapgen_palette palette = mapgen_palette::load_temp( jo, "dda" );
        auto &format_terrain = palette.format_terrain;
        auto &format_furniture = palette.format_furniture;
        auto &format_placings = palette.format_placings;

        if( format_terrain.empty() ) {
            return false;
        }

        // mandatory: mapgensize rows of mapgensize character lines, each of which must have a
        // matching key in "terrain", unless fill_ter is set
        // "rows:" [ "aaaajustlikeinmapgen.cpp", "this.must!be!exactly.24!", "and_must_match_terrain_", .... ]
        point expected_dim = mapgensize + m_offset;
        cata_assert( expected_dim.x >= 0 );
        cata_assert( expected_dim.y >= 0 );

        parray = jo.get_array( "rows" );
        if( static_cast<int>( parray.size() ) < expected_dim.y ) {
            parray.throw_error( string_format( "format: rows: must have at least %d rows, not %d",
                                               expected_dim.y, parray.size() ) );
        }
        for( int c = m_offset.y; c < expected_dim.y; c++ ) {
            const std::string row = parray.get_string( c );
            std::vector<map_key> row_keys;
            for( const std::string &key : utf8_display_split( row ) ) {
                row_keys.emplace_back( key );
            }
            if( row_keys.size() < static_cast<size_t>( expected_dim.x ) ) {
                parray.throw_error(
                    string_format( "  format: row %d must have at least %d columns, not %d",
                                   c + 1, expected_dim.x, row_keys.size() ) );
            }
            for( int i = m_offset.x; i < expected_dim.x; i++ ) {
                const point p = point( i, c ) - m_offset;
                const map_key key = row_keys[i];
                const auto iter_ter = format_terrain.find( key );
                const auto iter_furn = format_furniture.find( key );
                const auto fpi = format_placings.find( key );

                const bool has_terrain = iter_ter != format_terrain.end();
                const bool has_furn = iter_furn != format_furniture.end();
                const bool has_placing = fpi != format_placings.end();

                if( !has_terrain && !fallback_terrain_exists ) {
                    parray.string_error(
                        string_format( "format: rows: row %d column %d: "
                                       "'%s' is not in 'terrain', and no 'fill_ter' is set!",
                                       c + 1, i + 1, key.str ), c, i + 1 );
                }
                if( !has_terrain && !has_furn && !has_placing &&
                    key.str != " " && key.str != "." ) {
                    try {
                        parray.string_error(
                            string_format( "format: rows: row %d column %d: "
                                           "'%s' has no terrain, furniture, or other definition",
                                           c + 1, i + 1, key.str ), c, i + 1 );
                    } catch( const JsonError &e ) {
                        debugmsg( "(json-error)\n%s", e.what() );
                    }
                }
                if( has_terrain ) {
                    format[ calc_index( p ) ].ter = iter_ter->second;
                }
                if( has_furn ) {
                    format[ calc_index( p ) ].furn = iter_furn->second;
                }
                if( has_placing ) {
                    jmapgen_place where( p );
                    for( auto &what : fpi->second ) {
                        objects.add( where, what );
                    }
                }
            }
        }
        fallback_terrain_exists = true;
        do_format = true;
    }

    // No fill_ter? No format? GTFO.
    if( !fallback_terrain_exists ) {
        jo.throw_error(
            "Need one of 'fill_terrain' or 'predecessor_mapgen' or 'rows' + 'terrain'" );
        // TODO: write TFM.
    }

    if( jo.has_array( "set" ) ) {
        setup_setmap( jo.get_array( "set" ) );
    }

    // "add" is deprecated in favor of "place_item", but kept to support mods
    // which are not under our control.
    objects.load_objects<jmapgen_spawn_item>( jo, "add", context_ );
    objects.load_objects<jmapgen_spawn_item>( jo, "place_item", context_ );
    objects.load_objects<jmapgen_field>( jo, "place_fields", context_ );
    objects.load_objects<jmapgen_npc>( jo, "place_npcs", context_ );
    objects.load_objects<jmapgen_sign>( jo, "place_signs", context_ );
    objects.load_objects<jmapgen_vending_machine>( jo, "place_vendingmachines", context_ );
    objects.load_objects<jmapgen_toilet>( jo, "place_toilets", context_ );
    objects.load_objects<jmapgen_liquid_item>( jo, "place_liquids", context_ );
    objects.load_objects<jmapgen_gaspump>( jo, "place_gaspumps", context_ );
    objects.load_objects<jmapgen_item_group>( jo, "place_items", context_ );
    objects.load_objects<jmapgen_loot>( jo, "place_loot", context_ );
    objects.load_objects<jmapgen_monster_group>( jo, "place_monsters", context_ );
    objects.load_objects<jmapgen_vehicle>( jo, "place_vehicles", context_ );
    objects.load_objects<jmapgen_trap>( jo, "place_traps", context_ );
    objects.load_objects<jmapgen_furniture>( jo, "place_furniture", context_ );
    objects.load_objects<jmapgen_terrain>( jo, "place_terrain", context_ );
    objects.load_objects<jmapgen_monster>( jo, "place_monster", context_ );
    objects.load_objects<jmapgen_make_rubble>( jo, "place_rubble", context_ );
    objects.load_objects<jmapgen_computer>( jo, "place_computers", context_ );
    objects.load_objects<jmapgen_nested>( jo, "place_nested", context_ );
    objects.load_objects<jmapgen_graffiti>( jo, "place_graffiti", context_ );
    objects.load_objects<jmapgen_translate>( jo, "translate_ter", context_ );
    objects.load_objects<jmapgen_zone>( jo, "place_zones", context_ );
    objects.load_objects<jmapgen_ter_furn_transform>( jo, "place_ter_furn_transforms", context_ );
    // Needs to be last as it affects other placed items
    objects.load_objects<jmapgen_faction>( jo, "faction_owner", context_ );
    if( !mapgen_defer::defer ) {
        is_ready = true; // skip setup attempts from any additional pointers
    }
    return true;
}

void mapgen_function_json::check() const
{
    check_common();
}

void mapgen_function_json_nested::check() const
{
    check_common();
}

void mapgen_function_json_base::check_common() const
{
    for( const ter_furn_id &id : format ) {
        if( check_furn( id.furn, context_ ) ) {
            return;
        }
    }

    for( const jmapgen_setmap &setmap : setmap_points ) {
        if( setmap.op != JMAPGEN_SETMAP_FURN &&
            setmap.op != JMAPGEN_SETMAP_LINE_FURN &&
            setmap.op != JMAPGEN_SETMAP_SQUARE_FURN ) {
            continue;
        }
        furn_id id( setmap.val.get() );
        if( check_furn( id, context_ ) ) {
            return;
        }
    }

    objects.check( context_ );
}

void jmapgen_objects::check( const std::string &context ) const
{
    for( const jmapgen_obj &obj : objects ) {
        obj.second->check( context );
    }
}

/////////////////////////////////////////////////////////////////////////////////
///// 3 - mapgen (gameplay)
///// stuff below is the actual in-game map generation (ill)logic

/*
 * (set|line|square)_(ter|furn|trap|radiation); simple (x, y, int) or (x1,y1,x2,y2, int) functions
 * TODO: optimize, though gcc -O2 optimizes enough that splitting the switch has no effect
 */
bool jmapgen_setmap::apply( const mapgendata &dat, const point &offset ) const
{
    if( chance != 1 && !one_in( chance ) ) {
        return true;
    }

    const auto get = []( const jmapgen_int & v, int offset ) {
        return v.get() + offset;
    };
    const auto x_get = std::bind( get, x, offset.x );
    const auto y_get = std::bind( get, y, offset.y );
    const auto x2_get = std::bind( get, x2, offset.x );
    const auto y2_get = std::bind( get, y2, offset.y );

    map &m = dat.m;
    const int trepeat = repeat.get();
    for( int i = 0; i < trepeat; i++ ) {
        switch( op ) {
            case JMAPGEN_SETMAP_TER: {
                // TODO: the ter_id should be stored separately and not be wrapped in an jmapgen_int
                m.ter_set( point( x_get(), y_get() ), ter_id( val.get() ) );
            }
            break;
            case JMAPGEN_SETMAP_FURN: {
                // TODO: the furn_id should be stored separately and not be wrapped in an jmapgen_int
                m.furn_set( point( x_get(), y_get() ), furn_id( val.get() ) );
            }
            break;
            case JMAPGEN_SETMAP_TRAP: {
                // TODO: the trap_id should be stored separately and not be wrapped in an jmapgen_int
                mtrap_set( &m, point( x_get(), y_get() ), trap_id( val.get() ) );
            }
            break;
            case JMAPGEN_SETMAP_RADIATION: {
                m.set_radiation( point( x_get(), y_get() ), val.get() );
            }
            break;
            case JMAPGEN_SETMAP_BASH: {
                m.bash( tripoint( x_get(), y_get(), m.get_abs_sub().z ), 9999 );
            }
            break;

            case JMAPGEN_SETMAP_LINE_TER: {
                // TODO: the ter_id should be stored separately and not be wrapped in an jmapgen_int
                m.draw_line_ter( ter_id( val.get() ), point( x_get(), y_get() ), point( x2_get(), y2_get() ) );
            }
            break;
            case JMAPGEN_SETMAP_LINE_FURN: {
                // TODO: the furn_id should be stored separately and not be wrapped in an jmapgen_int
                m.draw_line_furn( furn_id( val.get() ), point( x_get(), y_get() ), point( x2_get(), y2_get() ) );
            }
            break;
            case JMAPGEN_SETMAP_LINE_TRAP: {
                const std::vector<point> line = line_to( point( x_get(), y_get() ), point( x2_get(), y2_get() ),
                                                0 );
                for( const point &i : line ) {
                    // TODO: the trap_id should be stored separately and not be wrapped in an jmapgen_int
                    mtrap_set( &m, i, trap_id( val.get() ) );
                }
            }
            break;
            case JMAPGEN_SETMAP_LINE_RADIATION: {
                const std::vector<point> line = line_to( point( x_get(), y_get() ), point( x2_get(), y2_get() ),
                                                0 );
                for( const point &i : line ) {
                    m.set_radiation( i, static_cast<int>( val.get() ) );
                }
            }
            break;
            case JMAPGEN_SETMAP_SQUARE_TER: {
                // TODO: the ter_id should be stored separately and not be wrapped in an jmapgen_int
                m.draw_square_ter( ter_id( val.get() ), point( x_get(), y_get() ), point( x2_get(), y2_get() ) );
            }
            break;
            case JMAPGEN_SETMAP_SQUARE_FURN: {
                // TODO: the furn_id should be stored separately and not be wrapped in an jmapgen_int
                m.draw_square_furn( furn_id( val.get() ), point( x_get(), y_get() ), point( x2_get(), y2_get() ) );
            }
            break;
            case JMAPGEN_SETMAP_SQUARE_TRAP: {
                const point c( x_get(), y_get() );
                const int cx2 = x2_get();
                const int cy2 = y2_get();
                for( int tx = c.x; tx <= cx2; tx++ ) {
                    for( int ty = c.y; ty <= cy2; ty++ ) {
                        // TODO: the trap_id should be stored separately and not be wrapped in an jmapgen_int
                        mtrap_set( &m, point( tx, ty ), trap_id( val.get() ) );
                    }
                }
            }
            break;
            case JMAPGEN_SETMAP_SQUARE_RADIATION: {
                const int cx = x_get();
                const int cy = y_get();
                const int cx2 = x2_get();
                const int cy2 = y2_get();
                for( int tx = cx; tx <= cx2; tx++ ) {
                    for( int ty = cy; ty <= cy2; ty++ ) {
                        m.set_radiation( point( tx, ty ), static_cast<int>( val.get() ) );
                    }
                }
            }
            break;

            default:
                //Suppress warnings
                break;
        }
    }
    return true;
}

bool jmapgen_setmap::has_vehicle_collision( const mapgendata &dat, const point &offset ) const
{
    const auto get = []( const jmapgen_int & v, int v_offset ) {
        return v.get() + v_offset;
    };
    const auto x_get = std::bind( get, x, offset.x );
    const auto y_get = std::bind( get, y, offset.y );
    const auto x2_get = std::bind( get, x2, offset.x );
    const auto y2_get = std::bind( get, y2, offset.y );
    const tripoint start = tripoint( x_get(), y_get(), 0 );
    tripoint end = start;
    switch( op ) {
        case JMAPGEN_SETMAP_TER:
        case JMAPGEN_SETMAP_FURN:
        case JMAPGEN_SETMAP_TRAP:
            break;
        /* lines and squares are the same thing for this purpose */
        case JMAPGEN_SETMAP_LINE_TER:
        case JMAPGEN_SETMAP_LINE_FURN:
        case JMAPGEN_SETMAP_LINE_TRAP:
        case JMAPGEN_SETMAP_SQUARE_TER:
        case JMAPGEN_SETMAP_SQUARE_FURN:
        case JMAPGEN_SETMAP_SQUARE_TRAP:
            end.x = x2_get();
            end.y = y2_get();
            break;
        /* if it's not a terrain, furniture, or trap, it can't collide */
        default:
            return false;
    }
    for( const tripoint &p : dat.m.points_in_rectangle( start, end ) ) {
        if( dat.m.veh_at( p ) ) {
            return true;
        }
    }
    return false;
}

void mapgen_function_json_base::formatted_set_incredibly_simple( map &m, const point &offset ) const
{
    for( int y = 0; y < mapgensize.y; y++ ) {
        for( int x = 0; x < mapgensize.x; x++ ) {
            point p( x, y );
            const size_t index = calc_index( p );
            const ter_furn_id &tdata = format[index];
            const point map_pos = p + offset;
            if( tdata.furn != f_null ) {
                if( tdata.ter != t_null ) {
                    m.set( map_pos, tdata.ter, tdata.furn );
                } else {
                    m.furn_set( map_pos, tdata.furn );
                }
            } else if( tdata.ter != t_null ) {
                m.ter_set( map_pos, tdata.ter );
            }
        }
    }
}

bool mapgen_function_json_base::has_vehicle_collision( const mapgendata &dat,
        const point &offset ) const
{
    if( do_format ) {
        for( int y = 0; y < mapgensize.y; y++ ) {
            for( int x = 0; x < mapgensize.x; x++ ) {
                const point p( x, y );
                const ter_furn_id &tdata = format[calc_index( p )];
                const point map_pos = p + offset;
                if( ( tdata.furn != f_null || tdata.ter != t_null ) &&
                    dat.m.veh_at( tripoint( map_pos, dat.zlevel() ) ).has_value() ) {
                    return true;
                }
            }
        }
    }

    for( const jmapgen_setmap &elem : setmap_points ) {
        if( elem.has_vehicle_collision( dat, offset ) ) {
            return true;
        }
    }

    return objects.has_vehicle_collision( dat, offset );
}

/*
 * Apply mapgen as per a derived-from-json recipe; in theory fast, but not very versatile
 */
void mapgen_function_json::generate( mapgendata &md )
{
    map *const m = &md.m;
    if( fill_ter != t_null ) {
        m->draw_fill_background( fill_ter );
    }
    if( predecessor_mapgen != oter_str_id::NULL_ID() ) {
        mapgendata predecessor_mapgen_dat( md, predecessor_mapgen );
        run_mapgen_func( predecessor_mapgen.id().str(), predecessor_mapgen_dat );

        // Now we have to do some rotation shenanigans. We need to ensure that
        // our predecessor is not rotated out of alignment as part of rotating this location,
        // and there are actually two sources of rotation--the mapgen can rotate explicitly, and
        // the entire overmap terrain may be rotatable. To ensure we end up in the right rotation,
        // we basically have to initially reverse the rotation that we WILL do in the future so that
        // when we apply that rotation, our predecessor is back in its original state while this
        // location is rotated as desired.

        m->rotate( ( -rotation.get() + 4 ) % 4 );

        if( md.terrain_type()->is_rotatable() ) {
            m->rotate( ( -static_cast<int>( md.terrain_type()->get_dir() ) + 4 ) % 4 );
        }
    }
    if( do_format ) {
        formatted_set_incredibly_simple( *m, point_zero );
    }
    for( auto &elem : setmap_points ) {
        elem.apply( md, point_zero );
    }

    objects.apply( md, point_zero );

    resolve_regional_terrain_and_furniture( md );

    m->rotate( rotation.get() );

    if( md.terrain_type()->is_rotatable() ) {
        mapgen_rotate( m, md.terrain_type(), false );
    }
}

void mapgen_function_json_nested::nest( const mapgendata &dat, const point &offset ) const
{
    // TODO: Make rotation work for submaps, then pass this value into elem & objects apply.
    //int chosen_rotation = rotation.get() % 4;

    if( do_format ) {
        formatted_set_incredibly_simple( dat.m, offset );
    }

    for( const jmapgen_setmap &elem : setmap_points ) {
        elem.apply( dat, offset );
    }

    objects.apply( dat, offset );

    resolve_regional_terrain_and_furniture( dat );
}

/*
 * Apply mapgen as per a derived-from-json recipe; in theory fast, but not very versatile
 */
void jmapgen_objects::apply( const mapgendata &dat ) const
{
    for( const jmapgen_obj &obj : objects ) {
        const auto &where = obj.first;
        const auto &what = *obj.second;
        // The user will only specify repeat once in JSON, but it may get loaded both
        // into the what and where in some cases--we just need the greater value of the two.
        const int repeat = std::max( where.repeat.get(), what.repeat.get() );
        for( int i = 0; i < repeat; i++ ) {
            what.apply( dat, where.x, where.y );
        }
    }
}

void jmapgen_objects::apply( const mapgendata &dat, const point &offset ) const
{
    if( offset == point_zero ) {
        // It's a bit faster
        apply( dat );
        return;
    }

    for( const jmapgen_obj &obj : objects ) {
        jmapgen_place where = obj.first;
        where.offset( -offset );

        const jmapgen_piece &what = *obj.second;
        // The user will only specify repeat once in JSON, but it may get loaded both
        // into the what and where in some cases--we just need the greater value of the two.
        const int repeat = std::max( where.repeat.get(), what.repeat.get() );
        for( int i = 0; i < repeat; i++ ) {
            what.apply( dat, where.x, where.y );
        }
    }
}

bool jmapgen_objects::has_vehicle_collision( const mapgendata &dat, const point &offset ) const
{
    for( const jmapgen_obj &obj : objects ) {
        jmapgen_place where = obj.first;
        where.offset( -offset );
        const auto &what = *obj.second;
        if( what.has_vehicle_collision( dat, point( where.x.get(), where.y.get() ) ) ) {
            return true;
        }
    }
    return false;
}

/////////////
void map::draw_map( mapgendata &dat )
{
    const oter_id &terrain_type = dat.terrain_type();
    const std::string function_key = terrain_type->get_mapgen_id();
    bool found = true;

    const bool generated = run_mapgen_func( function_key, dat );

    if( !generated ) {
        if( is_ot_match( "slimepit", terrain_type, ot_match_type::prefix ) ||
            is_ot_match( "slime_pit", terrain_type, ot_match_type::prefix ) ) {
            draw_slimepit( dat );
        } else if( is_ot_match( "triffid", terrain_type, ot_match_type::prefix ) ) {
            draw_triffid( dat );
        } else if( is_ot_match( "spider", terrain_type, ot_match_type::prefix ) ) {
            draw_spider_pit( dat );
        } else if( is_ot_match( "spiral", terrain_type, ot_match_type::prefix ) ) {
            draw_spiral( dat );
        } else if( is_ot_match( "temple", terrain_type, ot_match_type::prefix ) ) {
            draw_temple( dat );
        } else if( is_ot_match( "mine", terrain_type, ot_match_type::prefix ) ) {
            draw_mine( dat );
        } else if( is_ot_match( "anthill", terrain_type, ot_match_type::contains ) ) {
            draw_anthill( dat );
        } else if( is_ot_match( "lab", terrain_type, ot_match_type::contains ) ) {
            draw_lab( dat );
        } else {
            found = false;
        }
    }

    if( !found ) {
        // not one of the hardcoded ones!
        // load from JSON???
        debugmsg( "Error: tried to generate map for omtype %s, \"%s\" (id_mapgen %s)",
                  terrain_type.id().c_str(), terrain_type->get_name(), function_key.c_str() );
        fill_background( this, t_floor );
    }

    draw_connections( dat );
}

static const int SOUTH_EDGE = 2 * SEEY - 1;
static const int EAST_EDGE = 2 * SEEX  - 1;

void map::draw_lab( mapgendata &dat )
{
    const oter_id &terrain_type = dat.terrain_type();
    // To distinguish between types of labs
    bool ice_lab = true;
    bool central_lab = false;
    bool tower_lab = false;

    point p2;

    int lw = 0;
    int rw = 0;
    int tw = 0;
    int bw = 0;

    if( terrain_type == "lab" || terrain_type == "lab_stairs" || terrain_type == "lab_core" ||
        terrain_type == "ants_lab" || terrain_type == "ants_lab_stairs" ||
        terrain_type == "ice_lab" || terrain_type == "ice_lab_stairs" ||
        terrain_type == "ice_lab_core" ||
        terrain_type == "central_lab" || terrain_type == "central_lab_stairs" ||
        terrain_type == "central_lab_core" ||
        terrain_type == "tower_lab" || terrain_type == "tower_lab_stairs" ) {

        ice_lab = is_ot_match( "ice_lab", terrain_type, ot_match_type::prefix );
        central_lab = is_ot_match( "central_lab", terrain_type, ot_match_type::prefix );
        tower_lab = is_ot_match( "tower_lab", terrain_type, ot_match_type::prefix );

        if( ice_lab ) {
            int temperature = -20 + 30 * ( dat.zlevel() );
            set_temperature( p2, temperature );
            set_temperature( p2 + point( SEEX, 0 ), temperature );
            set_temperature( p2 + point( 0, SEEY ), temperature );
            set_temperature( p2 + point( SEEX, SEEY ), temperature );
        }

        // Check for adjacent sewers; used below
        tw = 0;
        rw = 0;
        bw = 0;
        lw = 0;
        if( is_ot_match( "sewer", dat.north(), ot_match_type::type ) && connects_to( dat.north(), 2 ) ) {
            tw = SOUTH_EDGE + 1;
        }
        if( is_ot_match( "sewer", dat.east(), ot_match_type::type ) && connects_to( dat.east(), 3 ) ) {
            rw = EAST_EDGE + 1;
        }
        if( is_ot_match( "sewer", dat.south(), ot_match_type::type ) && connects_to( dat.south(), 0 ) ) {
            bw = SOUTH_EDGE + 1;
        }
        if( is_ot_match( "sewer", dat.west(), ot_match_type::type ) && connects_to( dat.west(), 1 ) ) {
            lw = EAST_EDGE + 1;
        }
        if( dat.zlevel() == 0 ) { // We're on ground level
            for( int i = 0; i < SEEX * 2; i++ ) {
                for( int j = 0; j < SEEY * 2; j++ ) {
                    if( i <= 1 || i >= SEEX * 2 - 2 ||
                        ( j > 1 && j < SEEY * 2 - 2 && ( i == SEEX - 2 || i == SEEX + 1 ) ) ) {
                        ter_set( point( i, j ), t_concrete_wall );
                    } else if( j <= 1 || j >= SEEY * 2 - 2 ) {
                        ter_set( point( i, j ), t_concrete_wall );
                    } else {
                        ter_set( point( i, j ), t_floor );
                    }
                }
            }
            ter_set( point( SEEX - 1, 0 ), t_door_metal_locked );
            ter_set( point( SEEX - 1, 1 ), t_floor );
            ter_set( point( SEEX, 0 ), t_door_metal_locked );
            ter_set( point( SEEX, 1 ), t_floor );
            ter_set( point( SEEX - 2 + rng( 0, 1 ) * 3, 0 ), t_card_science );
            ter_set( point( SEEX - 2, SEEY ), t_door_metal_c );
            ter_set( point( SEEX + 1, SEEY ), t_door_metal_c );
            ter_set( point( SEEX - 2, SEEY - 1 ), t_door_metal_c );
            ter_set( point( SEEX + 1, SEEY - 1 ), t_door_metal_c );
            ter_set( point( SEEX - 1, SEEY * 2 - 3 ), t_stairs_down );
            ter_set( point( SEEX, SEEY * 2 - 3 ), t_stairs_down );
            science_room( this, point( 2, 2 ), point( SEEX - 3, SEEY * 2 - 3 ), dat.zlevel(), 1 );
            science_room( this, point( SEEX + 2, 2 ), point( SEEX * 2 - 3, SEEY * 2 - 3 ), dat.zlevel(), 3 );

            place_spawns( GROUP_TURRET, 1, point( SEEX, 5 ), point( SEEX, 5 ), 1, true );

            if( is_ot_match( "road", dat.east(), ot_match_type::type ) ) {
                rotate( 1 );
            } else if( is_ot_match( "road", dat.south(), ot_match_type::type ) ) {
                rotate( 2 );
            } else if( is_ot_match( "road", dat.west(), ot_match_type::type ) ) {
                rotate( 3 );
            }
        } else if( tw != 0 || rw != 0 || lw != 0 || bw != 0 ) { // Sewers!
            for( int i = 0; i < SEEX * 2; i++ ) {
                for( int j = 0; j < SEEY * 2; j++ ) {
                    ter_set( point( i, j ), t_thconc_floor );
                    if( ( ( i < lw || i > EAST_EDGE - rw ) && j > SEEY - 3 && j < SEEY + 2 ) ||
                        ( ( j < tw || j > SOUTH_EDGE - bw ) && i > SEEX - 3 && i < SEEX + 2 ) ) {
                        ter_set( point( i, j ), t_sewage );
                    }
                    if( ( i == 0 && is_ot_match( "lab", dat.east(), ot_match_type::contains ) ) || i == EAST_EDGE ) {
                        if( ter( point( i, j ) ) == t_sewage ) {
                            ter_set( point( i, j ), t_bars );
                        } else if( j == SEEY - 1 || j == SEEY ) {
                            ter_set( point( i, j ), t_door_metal_c );
                        } else {
                            ter_set( point( i, j ), t_concrete_wall );
                        }
                    } else if( ( j == 0 && is_ot_match( "lab", dat.north(), ot_match_type::contains ) ) ||
                               j == SOUTH_EDGE ) {
                        if( ter( point( i, j ) ) == t_sewage ) {
                            ter_set( point( i, j ), t_bars );
                        } else if( i == SEEX - 1 || i == SEEX ) {
                            ter_set( point( i, j ), t_door_metal_c );
                        } else {
                            ter_set( point( i, j ), t_concrete_wall );
                        }
                    }
                }
            }
        } else { // We're below ground, and no sewers
            // Set up the boundaries of walls (connect to adjacent lab squares)
            tw = is_ot_match( "lab", dat.north(), ot_match_type::contains ) ? 0 : 2;
            rw = is_ot_match( "lab", dat.east(), ot_match_type::contains ) ? 1 : 2;
            bw = is_ot_match( "lab", dat.south(), ot_match_type::contains ) ? 1 : 2;
            lw = is_ot_match( "lab", dat.west(), ot_match_type::contains ) ? 0 : 2;

            int boarders = 0;
            if( tw == 0 ) {
                boarders++;
            }
            if( rw == 1 ) {
                boarders++;
            }
            if( bw == 1 ) {
                boarders++;
            }
            if( lw == 0 ) {
                boarders++;
            }

            const auto maybe_insert_stairs = [this]( const oter_id & terrain,  const ter_id & t_stair_type ) {
                if( is_ot_match( "stairs", terrain, ot_match_type::contains ) ) {
                    const auto predicate = [this]( const tripoint & p ) {
                        return ter( p ) == t_thconc_floor && furn( p ) == f_null && tr_at( p ).is_null();
                    };
                    const auto range = points_in_rectangle( { 0, 0, abs_sub.z }, { SEEX * 2 - 2, SEEY * 2 - 2, abs_sub.z } );

                    if( const auto p = random_point( range, predicate ) ) {
                        ter_set( *p, t_stair_type );
                    }
                }
            };

            //A lab area with only one entrance
            if( boarders == 1 ) {
                // If you remove the usage of "lab_1side" here, remove it from mapgen_factory::get_usages above as well.
                if( oter_mapgen.generate( dat, "lab_1side" ) ) {
                    if( tw == 2 ) {
                        rotate( 2 );
                    }
                    if( rw == 2 ) {
                        rotate( 1 );
                    }
                    if( lw == 2 ) {
                        rotate( 3 );
                    }
                } else {
                    debugmsg( "Error: Tried to generate 1-sided lab but no lab_1side json exists." );
                }
                maybe_insert_stairs( dat.above(), t_stairs_up );
                maybe_insert_stairs( terrain_type, t_stairs_down );
            } else {
                const int hardcoded_4side_map_weight = 1500; // weight of all hardcoded maps.
                // If you remove the usage of "lab_4side" here, remove it from mapgen_factory::get_usages above as well.
                if( oter_mapgen.generate( dat, "lab_4side", hardcoded_4side_map_weight ) ) {
                    // If the map template hasn't handled borders, handle them in code.
                    // Rotated maps cannot handle borders and have to be caught in code.
                    // We determine if a border isn't handled by checking the east-facing
                    // border space where the door normally is -- it should be a wall or door.
                    tripoint east_border( 23, 11, abs_sub.z );
                    if( !has_flag_ter( "WALL", east_border ) &&
                        !has_flag_ter( "DOOR", east_border ) ) {
                        // TODO: create a ter_reset function that does ter_set,
                        // furn_set, and i_clear?
                        ter_id lw_type = tower_lab ? t_reinforced_glass : t_concrete_wall;
                        ter_id tw_type = tower_lab ? t_reinforced_glass : t_concrete_wall;
                        ter_id rw_type = tower_lab && rw == 2 ? t_reinforced_glass :
                                         t_concrete_wall;
                        ter_id bw_type = tower_lab && bw == 2 ? t_reinforced_glass :
                                         t_concrete_wall;
                        for( int i = 0; i < SEEX * 2; i++ ) {
                            ter_set( point( 23, i ), rw_type );
                            furn_set( point( 23, i ), f_null );
                            i_clear( tripoint( 23, i, get_abs_sub().z ) );

                            ter_set( point( i, 23 ), bw_type );
                            furn_set( point( i, 23 ), f_null );
                            i_clear( tripoint( i, 23, get_abs_sub().z ) );

                            if( lw == 2 ) {
                                ter_set( point( 0, i ), lw_type );
                                furn_set( point( 0, i ), f_null );
                                i_clear( tripoint( 0, i, get_abs_sub().z ) );
                            }
                            if( tw == 2 ) {
                                ter_set( point( i, 0 ), tw_type );
                                furn_set( point( i, 0 ), f_null );
                                i_clear( tripoint( i, 0, get_abs_sub().z ) );
                            }
                        }
                        if( rw != 2 ) {
                            ter_set( point( 23, 11 ), t_door_metal_c );
                            ter_set( point( 23, 12 ), t_door_metal_c );
                        }
                        if( bw != 2 ) {
                            ter_set( point( 11, 23 ), t_door_metal_c );
                            ter_set( point( 12, 23 ), t_door_metal_c );
                        }
                    }

                    maybe_insert_stairs( dat.above(), t_stairs_up );
                    maybe_insert_stairs( terrain_type, t_stairs_down );
                } else { // then no json maps for lab_4side were found
                    switch( rng( 1, 3 ) ) {
                        case 1:
                            // Cross shaped
                            for( int i = 0; i < SEEX * 2; i++ ) {
                                for( int j = 0; j < SEEY * 2; j++ ) {
                                    if( ( i < lw || i > EAST_EDGE - rw ) ||
                                        ( ( j < SEEY - 1 || j > SEEY ) &&
                                          ( i == SEEX - 2 || i == SEEX + 1 ) ) ) {
                                        ter_set( point( i, j ), t_concrete_wall );
                                    } else if( ( j < tw || j > SOUTH_EDGE - bw ) ||
                                               ( ( i < SEEX - 1 || i > SEEX ) &&
                                                 ( j == SEEY - 2 || j == SEEY + 1 ) ) ) {
                                        ter_set( point( i, j ), t_concrete_wall );
                                    } else {
                                        ter_set( point( i, j ), t_thconc_floor );
                                    }
                                }
                            }
                            if( is_ot_match( "stairs", dat.above(), ot_match_type::contains ) ) {
                                ter_set( point( rng( SEEX - 1, SEEX ), rng( SEEY - 1, SEEY ) ),
                                         t_stairs_up );
                            }
                            // Top left
                            if( one_in( 2 ) ) {
                                ter_set( point( SEEX - 2, static_cast<int>( SEEY / 2 ) ), t_door_glass_frosted_c );
                                science_room( this, point( lw, tw ), point( SEEX - 3, SEEY - 3 ), dat.zlevel(), 1 );
                            } else {
                                ter_set( point( SEEX / 2, SEEY - 2 ), t_door_glass_frosted_c );
                                science_room( this, point( lw, tw ), point( SEEX - 3, SEEY - 3 ), dat.zlevel(), 2 );
                            }
                            // Top right
                            if( one_in( 2 ) ) {
                                ter_set( point( SEEX + 1, static_cast<int>( SEEY / 2 ) ), t_door_glass_frosted_c );
                                science_room( this, point( SEEX + 2, tw ), point( EAST_EDGE - rw, SEEY - 3 ),
                                              dat.zlevel(), 3 );
                            } else {
                                ter_set( point( SEEX + static_cast<int>( SEEX / 2 ), SEEY - 2 ), t_door_glass_frosted_c );
                                science_room( this, point( SEEX + 2, tw ), point( EAST_EDGE - rw, SEEY - 3 ),
                                              dat.zlevel(), 2 );
                            }
                            // Bottom left
                            if( one_in( 2 ) ) {
                                ter_set( point( SEEX / 2, SEEY + 1 ), t_door_glass_frosted_c );
                                science_room( this, point( lw, SEEY + 2 ), point( SEEX - 3, SOUTH_EDGE - bw ),
                                              dat.zlevel(), 0 );
                            } else {
                                ter_set( point( SEEX - 2, SEEY + static_cast<int>( SEEY / 2 ) ), t_door_glass_frosted_c );
                                science_room( this, point( lw, SEEY + 2 ), point( SEEX - 3, SOUTH_EDGE - bw ),
                                              dat.zlevel(), 1 );
                            }
                            // Bottom right
                            if( one_in( 2 ) ) {
                                ter_set( point( SEEX + static_cast<int>( SEEX / 2 ), SEEY + 1 ), t_door_glass_frosted_c );
                                science_room( this, point( SEEX + 2, SEEY + 2 ), point( EAST_EDGE - rw, SOUTH_EDGE - bw ),
                                              dat.zlevel(), 0 );
                            } else {
                                ter_set( point( SEEX + 1, SEEY + static_cast<int>( SEEY / 2 ) ), t_door_glass_frosted_c );
                                science_room( this, point( SEEX + 2, SEEY + 2 ), point( EAST_EDGE - rw, SOUTH_EDGE - bw ),
                                              dat.zlevel(), 3 );
                            }
                            if( rw == 1 ) {
                                ter_set( point( EAST_EDGE, SEEY - 1 ), t_door_metal_c );
                                ter_set( point( EAST_EDGE, SEEY ), t_door_metal_c );
                            }
                            if( bw == 1 ) {
                                ter_set( point( SEEX - 1, SOUTH_EDGE ), t_door_metal_c );
                                ter_set( point( SEEX, SOUTH_EDGE ), t_door_metal_c );
                            }
                            if( is_ot_match( "stairs", terrain_type, ot_match_type::contains ) ) { // Stairs going down
                                std::vector<point> stair_points;
                                if( tw != 0 ) {
                                    stair_points.push_back( point( SEEX - 1, 2 ) );
                                    stair_points.push_back( point( SEEX - 1, 2 ) );
                                    stair_points.push_back( point( SEEX, 2 ) );
                                    stair_points.push_back( point( SEEX, 2 ) );
                                }
                                if( rw != 1 ) {
                                    stair_points.push_back( point( SEEX * 2 - 3, SEEY - 1 ) );
                                    stair_points.push_back( point( SEEX * 2 - 3, SEEY - 1 ) );
                                    stair_points.push_back( point( SEEX * 2 - 3, SEEY ) );
                                    stair_points.push_back( point( SEEX * 2 - 3, SEEY ) );
                                }
                                if( bw != 1 ) {
                                    stair_points.push_back( point( SEEX - 1, SEEY * 2 - 3 ) );
                                    stair_points.push_back( point( SEEX - 1, SEEY * 2 - 3 ) );
                                    stair_points.push_back( point( SEEX, SEEY * 2 - 3 ) );
                                    stair_points.push_back( point( SEEX, SEEY * 2 - 3 ) );
                                }
                                if( lw != 0 ) {
                                    stair_points.push_back( point( 2, SEEY - 1 ) );
                                    stair_points.push_back( point( 2, SEEY - 1 ) );
                                    stair_points.push_back( point( 2, SEEY ) );
                                    stair_points.push_back( point( 2, SEEY ) );
                                }
                                stair_points.push_back( point( static_cast<int>( SEEX / 2 ), SEEY ) );
                                stair_points.push_back( point( static_cast<int>( SEEX / 2 ), SEEY - 1 ) );
                                stair_points.push_back( point( static_cast<int>( SEEX / 2 ) + SEEX, SEEY ) );
                                stair_points.push_back( point( static_cast<int>( SEEX / 2 ) + SEEX, SEEY - 1 ) );
                                stair_points.push_back( point( SEEX, static_cast<int>( SEEY / 2 ) ) );
                                stair_points.push_back( point( SEEX + 2, static_cast<int>( SEEY / 2 ) ) );
                                stair_points.push_back( point( SEEX, static_cast<int>( SEEY / 2 ) + SEEY ) );
                                stair_points.push_back( point( SEEX + 2, static_cast<int>( SEEY / 2 ) + SEEY ) );
                                const point p = random_entry( stair_points );
                                ter_set( p, t_stairs_down );
                            }

                            break;

                        case 2:
                            // tic-tac-toe # layout
                            for( int i = 0; i < SEEX * 2; i++ ) {
                                for( int j = 0; j < SEEY * 2; j++ ) {
                                    if( i < lw || i > EAST_EDGE - rw || i == SEEX - 4 ||
                                        i == SEEX + 3 ) {
                                        ter_set( point( i, j ), t_concrete_wall );
                                    } else if( j < tw || j > SOUTH_EDGE - bw || j == SEEY - 4 ||
                                               j == SEEY + 3 ) {
                                        ter_set( point( i, j ), t_concrete_wall );
                                    } else {
                                        ter_set( point( i, j ), t_thconc_floor );
                                    }
                                }
                            }
                            if( is_ot_match( "stairs", dat.above(), ot_match_type::contains ) ) {
                                ter_set( point( SEEX - 1, SEEY - 1 ), t_stairs_up );
                                ter_set( point( SEEX, SEEY - 1 ), t_stairs_up );
                                ter_set( point( SEEX - 1, SEEY ), t_stairs_up );
                                ter_set( point( SEEX, SEEY ), t_stairs_up );
                            }
                            ter_set( point( SEEX - rng( 0, 1 ), SEEY - 4 ), t_door_glass_frosted_c );
                            ter_set( point( SEEX - rng( 0, 1 ), SEEY + 3 ), t_door_glass_frosted_c );
                            ter_set( point( SEEX - 4, SEEY + rng( 0, 1 ) ), t_door_glass_frosted_c );
                            ter_set( point( SEEX + 3, SEEY + rng( 0, 1 ) ), t_door_glass_frosted_c );
                            ter_set( point( SEEX - 4, static_cast<int>( SEEY / 2 ) ), t_door_glass_frosted_c );
                            ter_set( point( SEEX + 3, static_cast<int>( SEEY / 2 ) ), t_door_glass_frosted_c );
                            ter_set( point( SEEX / 2, SEEY - 4 ), t_door_glass_frosted_c );
                            ter_set( point( SEEX / 2, SEEY + 3 ), t_door_glass_frosted_c );
                            ter_set( point( SEEX + static_cast<int>( SEEX / 2 ), SEEY - 4 ), t_door_glass_frosted_c );
                            ter_set( point( SEEX + static_cast<int>( SEEX / 2 ), SEEY + 3 ), t_door_glass_frosted_c );
                            ter_set( point( SEEX - 4, SEEY + static_cast<int>( SEEY / 2 ) ), t_door_glass_frosted_c );
                            ter_set( point( SEEX + 3, SEEY + static_cast<int>( SEEY / 2 ) ), t_door_glass_frosted_c );
                            science_room( this, point( lw, tw ), point( SEEX - 5, SEEY - 5 ), dat.zlevel(),
                                          rng( 1, 2 ) );
                            science_room( this, point( SEEX - 3, tw ), point( SEEX + 2, SEEY - 5 ), dat.zlevel(), 2 );
                            science_room( this, point( SEEX + 4, tw ), point( EAST_EDGE - rw, SEEY - 5 ),
                                          dat.zlevel(), rng( 2, 3 ) );
                            science_room( this, point( lw, SEEY - 3 ), point( SEEX - 5, SEEY + 2 ), dat.zlevel(), 1 );
                            science_room( this, point( SEEX + 4, SEEY - 3 ), point( EAST_EDGE - rw, SEEY + 2 ),
                                          dat.zlevel(), 3 );
                            science_room( this, point( lw, SEEY + 4 ), point( SEEX - 5, SOUTH_EDGE - bw ),
                                          dat.zlevel(), rng( 0, 1 ) );
                            science_room( this, point( SEEX - 3, SEEY + 4 ), point( SEEX + 2, SOUTH_EDGE - bw ),
                                          dat.zlevel(), 0 );
                            science_room( this, point( SEEX + 4, SEEX + 4 ), point( EAST_EDGE - rw, SOUTH_EDGE - bw ),
                                          dat.zlevel(), 3 * rng( 0, 1 ) );
                            if( rw == 1 ) {
                                ter_set( point( EAST_EDGE, SEEY - 1 ), t_door_metal_c );
                                ter_set( point( EAST_EDGE, SEEY ), t_door_metal_c );
                            }
                            if( bw == 1 ) {
                                ter_set( point( SEEX - 1, SOUTH_EDGE ), t_door_metal_c );
                                ter_set( point( SEEX, SOUTH_EDGE ), t_door_metal_c );
                            }
                            if( is_ot_match( "stairs", terrain_type, ot_match_type::contains ) ) {
                                ter_set( point( SEEX - 3 + 5 * rng( 0, 1 ), SEEY - 3 + 5 * rng( 0, 1 ) ),
                                         t_stairs_down );
                            }
                            break;

                        case 3:
                            // Big room
                            for( int i = 0; i < SEEX * 2; i++ ) {
                                for( int j = 0; j < SEEY * 2; j++ ) {
                                    if( i < lw || i >= EAST_EDGE - rw ) {
                                        ter_set( point( i, j ), t_concrete_wall );
                                    } else if( j < tw || j >= SOUTH_EDGE - bw ) {
                                        ter_set( point( i, j ), t_concrete_wall );
                                    } else {
                                        ter_set( point( i, j ), t_thconc_floor );
                                    }
                                }
                            }
                            science_room( this, point( lw, tw ), point( EAST_EDGE - rw, SOUTH_EDGE - bw ),
                                          dat.zlevel(), rng( 0, 3 ) );

                            if( rw == 1 ) {
                                ter_set( point( EAST_EDGE, SEEY - 1 ), t_door_metal_c );
                                ter_set( point( EAST_EDGE, SEEY ), t_door_metal_c );
                            }
                            if( bw == 1 ) {
                                ter_set( point( SEEX - 1, SOUTH_EDGE ), t_door_metal_c );
                                ter_set( point( SEEX, SOUTH_EDGE ), t_door_metal_c );
                            }
                            maybe_insert_stairs( dat.above(), t_stairs_up );
                            maybe_insert_stairs( terrain_type, t_stairs_down );
                            break;
                    }
                } // endif use_hardcoded_4side_map
            }  // end 1 vs 4 sides
        } // end aboveground vs belowground

        // Ants will totally wreck up the place
        if( is_ot_match( "ants", terrain_type, ot_match_type::contains ) ) {
            for( int i = 0; i < SEEX * 2; i++ ) {
                for( int j = 0; j < SEEY * 2; j++ ) {
                    // Carve out a diamond area that covers 2 spaces on each edge.
                    if( i + j > 10 && i + j < 36 && std::abs( i - j ) < 13 ) {
                        // Doors and walls get sometimes destroyed:
                        // 100% at the edge, usually in a central cross, occasionally elsewhere.
                        if( ( has_flag_ter( "DOOR", point( i, j ) ) || has_flag_ter( "WALL", point( i, j ) ) ) ) {
                            if( ( i == 0 || j == 0 || i == 23 || j == 23 ) ||
                                ( !one_in( 3 ) && ( i == 11 || i == 12 || j == 11 || j == 12 ) ) ||
                                one_in( 4 ) ) {
                                // bash and usually remove the rubble.
                                make_rubble( { i, j, abs_sub.z } );
                                ter_set( point( i, j ), t_rock_floor );
                                if( !one_in( 3 ) ) {
                                    furn_set( point( i, j ), f_null );
                                }
                            }
                            // and then randomly destroy 5% of the remaining nonstairs.
                        } else if( one_in( 20 ) &&
                                   !has_flag_ter( "GOES_DOWN", p2 ) &&
                                   !has_flag_ter( "GOES_UP", p2 ) ) {
                            destroy( { i, j, abs_sub.z } );
                            // bashed squares can create dirt & floors, but we want rock floors.
                            if( t_dirt == ter( point( i, j ) ) || t_floor == ter( point( i, j ) ) ) {
                                ter_set( point( i, j ), t_rock_floor );
                            }
                        }
                    }
                }
            }
        }

        // Slimes pretty much wreck up the place, too, but only underground
        tw = ( dat.north() == "slimepit" ? SEEY     : 0 );
        rw = ( dat.east()  == "slimepit" ? SEEX + 1 : 0 );
        bw = ( dat.south() == "slimepit" ? SEEY + 1 : 0 );
        lw = ( dat.west()  == "slimepit" ? SEEX     : 0 );
        if( tw != 0 || rw != 0 || bw != 0 || lw != 0 ) {
            for( int i = 0; i < SEEX * 2; i++ ) {
                for( int j = 0; j < SEEY * 2; j++ ) {
                    if( ( ( j <= tw || i >= rw ) && i >= j && ( EAST_EDGE - i ) <= j ) ||
                        ( ( j >= bw || i <= lw ) && i <= j && ( SOUTH_EDGE - j ) <= i ) ) {
                        if( one_in( 5 ) ) {
                            make_rubble( tripoint( i,  j, abs_sub.z ), f_rubble_rock, true,
                                         t_slime );
                        } else if( !one_in( 5 ) ) {
                            ter_set( point( i, j ), t_slime );
                        }
                    }
                }
            }
        }

        int light_odds = 0;
        // central labs are always fully lit, other labs have half chance of some lights.
        if( central_lab ) {
            light_odds = 1;
        } else if( one_in( 2 ) ) {
            // Create a spread of densities, from all possible lights on, to 1/3, ...
            // to ~1 per segment.
            light_odds = std::pow( rng( 1, 12 ), 1.6 );
        }
        if( light_odds > 0 ) {
            for( int i = 0; i < SEEX * 2; i++ ) {
                for( int j = 0; j < SEEY * 2; j++ ) {
                    if( !( ( i * j ) % 2 || ( i + j ) % 4 ) && one_in( light_odds ) ) {
                        if( t_thconc_floor == ter( point( i, j ) ) || t_strconc_floor == ter( point( i, j ) ) ) {
                            ter_set( point( i, j ), t_thconc_floor_olight );
                        }
                    }
                }
            }
        }

        if( tower_lab ) {
            place_spawns( GROUP_LAB, 1, point_zero, point( EAST_EDGE, EAST_EDGE ), abs_sub.z * 0.02f );
        }

        // Lab special effects.
        if( one_in( 10 ) ) {
            switch( rng( 1, 7 ) ) {
                // full flooding/sewage
                case 1: {
                    if( is_ot_match( "stairs", terrain_type, ot_match_type::contains ) ||
                        is_ot_match( "ice", terrain_type, ot_match_type::contains ) ) {
                        // don't flood if stairs because the floor below will not be flooded.
                        // don't flood if ice lab because there's no mechanic for freezing
                        // liquid floors.
                        break;
                    }
                    auto fluid_type = one_in( 3 ) ? t_sewage : t_water_sh;
                    for( int i = 0; i < EAST_EDGE; i++ ) {
                        for( int j = 0; j < SOUTH_EDGE; j++ ) {
                            // We spare some terrain to make it look better visually.
                            if( !one_in( 10 ) && ( t_thconc_floor == ter( point( i, j ) ) ||
                                                   t_strconc_floor == ter( point( i, j ) ) ||
                                                   t_thconc_floor_olight == ter( point( i, j ) ) ) ) {
                                ter_set( point( i, j ), fluid_type );
                            } else if( has_flag_ter( "DOOR", point( i, j ) ) && !one_in( 3 ) ) {
                                // We want the actual debris, but not the rubble marker or dirt.
                                make_rubble( { i, j, abs_sub.z } );
                                ter_set( point( i, j ), fluid_type );
                                furn_set( point( i, j ), f_null );
                            }
                        }
                    }
                    break;
                }
                // minor flooding/sewage
                case 2: {
                    if( is_ot_match( "stairs", terrain_type, ot_match_type::contains ) ||
                        is_ot_match( "ice", terrain_type, ot_match_type::contains ) ) {
                        // don't flood if stairs because the floor below will not be flooded.
                        // don't flood if ice lab because there's no mechanic for freezing
                        // liquid floors.
                        break;
                    }
                    auto fluid_type = one_in( 3 ) ? t_sewage : t_water_sh;
                    for( int i = 0; i < 2; ++i ) {
                        draw_rough_circle( [this, fluid_type]( const point & p ) {
                            if( t_thconc_floor == ter( p ) || t_strconc_floor == ter( p ) ||
                                t_thconc_floor_olight == ter( p ) ) {
                                ter_set( p, fluid_type );
                            } else if( has_flag_ter( "DOOR", p ) ) {
                                // We want the actual debris, but not the rubble marker or dirt.
                                make_rubble( { p, abs_sub.z } );
                                ter_set( p, fluid_type );
                                furn_set( p, f_null );
                            }
                        }, point( rng( 1, SEEX * 2 - 2 ), rng( 1, SEEY * 2 - 2 ) ), rng( 3, 6 ) );
                    }
                    break;
                }
                // toxic gas leaks and smoke-filled rooms.
                case 3:
                case 4: {
                    bool is_toxic = one_in( 3 );
                    for( int i = 0; i < SEEX * 2; i++ ) {
                        for( int j = 0; j < SEEY * 2; j++ ) {
                            if( one_in( 200 ) && ( t_thconc_floor == ter( point( i, j ) ) ||
                                                   t_strconc_floor == ter( point( i, j ) ) ) ) {
                                if( is_toxic ) {
                                    add_field( {i, j, abs_sub.z}, fd_gas_vent, 1 );
                                } else {
                                    add_field( {i, j, abs_sub.z}, fd_smoke_vent, 2 );
                                }
                            }
                        }
                    }
                    break;
                }
                // portal with an artifact effect.
                case 5: {
                    tripoint center( rng( 6, SEEX * 2 - 7 ), rng( 6, SEEY * 2 - 7 ), abs_sub.z );
                    std::vector<artifact_natural_property> valid_props = {
                        ARTPROP_BREATHING,
                        ARTPROP_CRACKLING,
                        ARTPROP_WARM,
                        ARTPROP_SCALED,
                        ARTPROP_WHISPERING,
                        ARTPROP_GLOWING
                    };
                    draw_rough_circle( [this]( const point & p ) {
                        if( has_flag_ter( "GOES_DOWN", p ) ||
                            has_flag_ter( "GOES_UP", p ) ||
                            has_flag_ter( "CONSOLE", p ) ) {
                            return; // spare stairs and consoles.
                        }
                        make_rubble( {p, abs_sub.z } );
                        ter_set( p, t_thconc_floor );
                    }, center.xy(), 4 );
                    furn_set( center.xy(), f_null );
                    trap_set( center, tr_portal );
                    create_anomaly( center, random_entry( valid_props ), false );
                    break;
                }
                // radioactive accident.
                case 6: {
                    tripoint center( rng( 6, SEEX * 2 - 7 ), rng( 6, SEEY * 2 - 7 ), abs_sub.z );
                    if( has_flag_ter( "WALL", center.xy() ) ) {
                        // just skip it, we don't want to risk embedding radiation out of sight.
                        break;
                    }
                    draw_rough_circle( [this]( const point & p ) {
                        set_radiation( p, 10 );
                    }, center.xy(), rng( 7, 12 ) );
                    draw_circle( [this]( const point & p ) {
                        set_radiation( p, 20 );
                    }, center.xy(), rng( 5, 8 ) );
                    draw_circle( [this]( const point & p ) {
                        set_radiation( p, 30 );
                    }, center.xy(), rng( 2, 4 ) );
                    draw_circle( [this]( const point & p ) {
                        set_radiation( p, 50 );
                    }, center.xy(), 1 );
                    draw_circle( [this]( const point & p ) {
                        if( has_flag_ter( "GOES_DOWN", p ) ||
                            has_flag_ter( "GOES_UP", p ) ||
                            has_flag_ter( "CONSOLE", p ) ) {
                            return; // spare stairs and consoles.
                        }
                        make_rubble( {p, abs_sub.z } );
                        ter_set( p, t_thconc_floor );
                    }, center.xy(), 1 );

                    place_spawns( GROUP_HAZMATBOT, 1, center.xy() + point_west,
                                  center.xy() + point_west, 1, true );
                    place_spawns( GROUP_HAZMATBOT, 2, center.xy() + point_west,
                                  center.xy() + point_west, 1, true );

                    // damaged mininuke/plut thrown past edge of rubble so the player can see it.
                    int marker_x = center.x - 2 + 4 * rng( 0, 1 );
                    int marker_y = center.y + rng( -2, 2 );
                    if( one_in( 4 ) ) {
                        spawn_item( point( marker_x, marker_y ),
                                    "mininuke", 1, 1, calendar::turn_zero, rng( 2, 4 ) );
                    } else {
                        item newliquid( "plut_slurry_dense", calendar::start_of_cataclysm );
                        newliquid.charges = 1;
                        add_item_or_charges( tripoint( marker_x, marker_y, get_abs_sub().z ),
                                             newliquid );
                    }
                    break;
                }
                // portal with fungal invasion
                case 7: {
                    for( int i = 0; i < EAST_EDGE; i++ ) {
                        for( int j = 0; j < SOUTH_EDGE; j++ ) {
                            // Create a mostly spread fungal area throughout entire lab.
                            if( !one_in( 5 ) && ( has_flag( "FLAT", point( i, j ) ) ) ) {
                                ter_set( point( i, j ), t_fungus_floor_in );
                                if( has_flag_furn( "ORGANIC", point( i, j ) ) ) {
                                    furn_set( point( i, j ), f_fungal_clump );
                                }
                            } else if( has_flag_ter( "DOOR", point( i, j ) ) && !one_in( 5 ) ) {
                                ter_set( point( i, j ), t_fungus_floor_in );
                            } else if( has_flag_ter( "WALL", point( i, j ) ) && one_in( 3 ) ) {
                                ter_set( point( i, j ), t_fungus_wall );
                            }
                        }
                    }
                    tripoint center( rng( 6, SEEX * 2 - 7 ), rng( 6, SEEY * 2 - 7 ), abs_sub.z );

                    // Make a portal surrounded by more dense fungal stuff and a fungaloid.
                    draw_rough_circle( [this]( const point & p ) {
                        if( has_flag_ter( "GOES_DOWN", p ) ||
                            has_flag_ter( "GOES_UP", p ) ||
                            has_flag_ter( "CONSOLE", p ) ) {
                            return; // spare stairs and consoles.
                        }
                        if( has_flag_ter( "WALL", p ) ) {
                            ter_set( p, t_fungus_wall );
                        } else {
                            ter_set( p, t_fungus_floor_in );
                            if( one_in( 3 ) ) {
                                furn_set( p, f_flower_fungal );
                            } else if( one_in( 10 ) ) {
                                ter_set( p, t_marloss );
                            }
                        }
                    }, center.xy(), 3 );
                    ter_set( center.xy(), t_fungus_floor_in );
                    furn_set( center.xy(), f_null );
                    trap_set( center, tr_portal );
                    place_spawns( GROUP_FUNGI_FUNGALOID, 1, center.xy() + point( -2, -2 ),
                                  center.xy() + point( 2, 2 ), 1, true );

                    break;
                }
            }
        }
    } else if( terrain_type == "lab_finale" || terrain_type == "ice_lab_finale" ||
               terrain_type == "central_lab_finale" || terrain_type == "tower_lab_finale" ) {

        ice_lab = is_ot_match( "ice_lab", terrain_type, ot_match_type::prefix );
        central_lab = is_ot_match( "central_lab", terrain_type, ot_match_type::prefix );
        tower_lab = is_ot_match( "tower_lab", terrain_type, ot_match_type::prefix );

        if( ice_lab ) {
            int temperature = -20 + 30 * dat.zlevel();
            set_temperature( p2, temperature );
            set_temperature( p2 + point( SEEX, 0 ), temperature );
            set_temperature( p2 + point( 0, SEEY ), temperature );
            set_temperature( p2 + point( SEEX, SEEY ), temperature );
        }

        tw = is_ot_match( "lab", dat.north(), ot_match_type::contains ) ? 0 : 2;
        rw = is_ot_match( "lab", dat.east(), ot_match_type::contains ) ? 1 : 2;
        bw = is_ot_match( "lab", dat.south(), ot_match_type::contains ) ? 1 : 2;
        lw = is_ot_match( "lab", dat.west(), ot_match_type::contains ) ? 0 : 2;

        const int hardcoded_finale_map_weight = 500; // weight of all hardcoded maps.
        // If you remove the usage of "lab_finale_1level" here, remove it from mapgen_factory::get_usages above as well.
        if( oter_mapgen.generate( dat, "lab_finale_1level", hardcoded_finale_map_weight ) ) {
            // If the map template hasn't handled borders, handle them in code.
            // Rotated maps cannot handle borders and have to be caught in code.
            // We determine if a border isn't handled by checking the east-facing
            // border space where the door normally is -- it should be a wall or door.
            tripoint east_border( 23, 11, abs_sub.z );
            if( !has_flag_ter( "WALL", east_border ) && !has_flag_ter( "DOOR", east_border ) ) {
                // TODO: create a ter_reset function that does ter_set, furn_set, and i_clear?
                ter_id lw_type = tower_lab ? t_reinforced_glass : t_concrete_wall;
                ter_id tw_type = tower_lab ? t_reinforced_glass : t_concrete_wall;
                ter_id rw_type = tower_lab && rw == 2 ? t_reinforced_glass : t_concrete_wall;
                ter_id bw_type = tower_lab && bw == 2 ? t_reinforced_glass : t_concrete_wall;
                for( int i = 0; i < SEEX * 2; i++ ) {
                    ter_set( point( 23, i ), rw_type );
                    furn_set( point( 23, i ), f_null );
                    i_clear( tripoint( 23, i, get_abs_sub().z ) );

                    ter_set( point( i, 23 ), bw_type );
                    furn_set( point( i, 23 ), f_null );
                    i_clear( tripoint( i, 23, get_abs_sub().z ) );

                    if( lw == 2 ) {
                        ter_set( point( 0, i ), lw_type );
                        furn_set( point( 0, i ), f_null );
                        i_clear( tripoint( 0, i, get_abs_sub().z ) );
                    }
                    if( tw == 2 ) {
                        ter_set( point( i, 0 ), tw_type );
                        furn_set( point( i, 0 ), f_null );
                        i_clear( tripoint( i, 0, get_abs_sub().z ) );
                    }
                }
                if( rw != 2 ) {
                    ter_set( point( 23, 11 ), t_door_metal_c );
                    ter_set( point( 23, 12 ), t_door_metal_c );
                }
                if( bw != 2 ) {
                    ter_set( point( 11, 23 ), t_door_metal_c );
                    ter_set( point( 12, 23 ), t_door_metal_c );
                }
            }
        } else { // then no json maps for lab_finale_1level were found
            // Start by setting up a large, empty room.
            for( int i = 0; i < SEEX * 2; i++ ) {
                for( int j = 0; j < SEEY * 2; j++ ) {
                    if( i < lw || i > EAST_EDGE - rw ) {
                        ter_set( point( i, j ), t_concrete_wall );
                    } else if( j < tw || j > SOUTH_EDGE - bw ) {
                        ter_set( point( i, j ), t_concrete_wall );
                    } else {
                        ter_set( point( i, j ), t_thconc_floor );
                    }
                }
            }
            if( rw == 1 ) {
                ter_set( point( EAST_EDGE, SEEY - 1 ), t_door_metal_c );
                ter_set( point( EAST_EDGE, SEEY ), t_door_metal_c );
            }
            if( bw == 1 ) {
                ter_set( point( SEEX - 1, SOUTH_EDGE ), t_door_metal_c );
                ter_set( point( SEEX, SOUTH_EDGE ), t_door_metal_c );
            }

            int loot_variant; //only used for weapons testing variant.
            computer *tmpcomp = nullptr;
            switch( rng( 1, 5 ) ) {
                // Weapons testing - twice as common because it has 4 variants.
                case 1:
                case 2:
                    loot_variant = rng( 1, 100 ); //The variants have a 67/22/7/4 split.
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point( 6, 6 ), point( 6, 6 ), 1, true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point( SEEX * 2 - 7, 6 ),
                                  point( SEEX * 2 - 7, 6 ), 1, true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point( 6, SEEY * 2 - 7 ),
                                  point( 6, SEEY * 2 - 7 ), 1, true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point( SEEX * 2 - 7, SEEY * 2 - 7 ),
                                  point( SEEX * 2 - 7, SEEY * 2 - 7 ), 1, true );
                    spawn_item( point( SEEX - 4, SEEY - 2 ), "id_science" );
                    if( loot_variant <= 96 ) {
                        mtrap_set( this, point( SEEX - 3, SEEY - 3 ), tr_dissector );
                        mtrap_set( this, point( SEEX + 2, SEEY - 3 ), tr_dissector );
                        mtrap_set( this, point( SEEX - 3, SEEY + 2 ), tr_dissector );
                        mtrap_set( this, point( SEEX + 2, SEEY + 2 ), tr_dissector );
                        line( this, t_reinforced_glass, point( SEEX + 1, SEEY + 1 ), point( SEEX - 2, SEEY + 1 ) );
                        line( this, t_reinforced_glass, point( SEEX - 2, SEEY ), point( SEEX - 2, SEEY - 2 ) );
                        line( this, t_reinforced_glass, point( SEEX - 1, SEEY - 2 ), point( SEEX + 1, SEEY - 2 ) );
                        ter_set( point( SEEX + 1, SEEY - 1 ), t_reinforced_glass );
                        ter_set( point( SEEX + 1, SEEY ), t_reinforced_door_glass_c );
                        furn_set( point( SEEX - 1, SEEY - 1 ), f_table );
                        furn_set( point( SEEX, SEEY - 1 ), f_table );
                        furn_set( point( SEEX - 1, SEEY ), f_table );
                        furn_set( point( SEEX, SEEY ), f_table );
                        if( loot_variant <= 67 ) {
                            spawn_item( point( SEEX, SEEY - 1 ), "UPS_off" );
                            spawn_item( point( SEEX, SEEY - 1 ), "heavy_battery_cell" );
                            spawn_item( point( SEEX - 1, SEEY ), "v29" );
                            spawn_item( point( SEEX - 1, SEEY ), "laser_rifle", dice( 1, 0 ) );
                            spawn_item( point( SEEX, SEEY ), "plasma_gun" );
                            spawn_item( point( SEEX, SEEY ), "plasma" );
                            spawn_item( point( SEEX - 1, SEEY ), "recipe_atomic_battery" );
                            spawn_item( point( SEEX + 1, SEEY ), "plut_cell", rng( 8, 20 ) );
                        } else if( loot_variant < 89 ) {
                            spawn_item( point( SEEX - 1, SEEY - 1 ), "mininuke", dice( 3, 6 ) );
                            spawn_item( point( SEEX, SEEY - 1 ), "mininuke", dice( 3, 6 ) );
                            spawn_item( point( SEEX - 1, SEEY ), "mininuke", dice( 3, 6 ) );
                            spawn_item( point( SEEX, SEEY ), "mininuke", dice( 3, 6 ) );
                            spawn_item( point( SEEX, SEEY ), "recipe_atomic_battery" );
                            spawn_item( point( SEEX + 1, SEEY ), "plut_cell", rng( 8, 20 ) );
                        }  else { // loot_variant between 90 and 96.
                            spawn_item( point( SEEX - 1, SEEY - 1 ), "rm13_armor" );
                            spawn_item( point( SEEX, SEEY - 1 ), "plut_cell" );
                            spawn_item( point( SEEX - 1, SEEY ), "plut_cell" );
                            spawn_item( point( SEEX, SEEY ), "recipe_caseless" );
                        }
                    } else { // 4% of the lab ends will be this weapons testing end.
                        mtrap_set( this, point( SEEX - 4, SEEY - 3 ), tr_dissector );
                        mtrap_set( this, point( SEEX + 3, SEEY - 3 ), tr_dissector );
                        mtrap_set( this, point( SEEX - 4, SEEY + 2 ), tr_dissector );
                        mtrap_set( this, point( SEEX + 3, SEEY + 2 ), tr_dissector );

                        furn_set( point( SEEX - 2, SEEY - 1 ), f_rack );
                        furn_set( point( SEEX - 1, SEEY - 1 ), f_rack );
                        furn_set( point( SEEX, SEEY - 1 ), f_rack );
                        furn_set( point( SEEX + 1, SEEY - 1 ), f_rack );
                        furn_set( point( SEEX - 2, SEEY ), f_rack );
                        furn_set( point( SEEX - 1, SEEY ), f_rack );
                        furn_set( point( SEEX, SEEY ), f_rack );
                        furn_set( point( SEEX + 1, SEEY ), f_rack );
                        line( this, t_reinforced_door_glass_c, point( SEEX - 2, SEEY - 2 ),
                              point( SEEX + 1, SEEY - 2 ) );
                        line( this, t_reinforced_door_glass_c, point( SEEX - 2, SEEY + 1 ),
                              point( SEEX + 1, SEEY + 1 ) );
                        line( this, t_reinforced_glass, point( SEEX - 3, SEEY - 2 ), point( SEEX - 3, SEEY + 1 ) );
                        line( this, t_reinforced_glass, point( SEEX + 2, SEEY - 2 ), point( SEEX + 2, SEEY + 1 ) );
                        place_items( item_group_id( "ammo_rare" ), 96,
                                     point( SEEX - 2, SEEY - 1 ),
                                     point( SEEX + 1, SEEY - 1 ), false,
                                     calendar::start_of_cataclysm );
                        place_items( item_group_id( "guns_rare" ), 96, point( SEEX - 2, SEEY ),
                                     point( SEEX + 1, SEEY ), false,
                                     calendar::start_of_cataclysm );
                        spawn_item( point( SEEX + 1, SEEY ), "plut_cell", rng( 1, 10 ) );
                    }
                    break;
                // Netherworld access
                case 3: {
                    bool monsters_end = false;
                    if( !one_in( 4 ) ) { // Trapped netherworld monsters
                        monsters_end = true;
                        tw = rng( SEEY + 3, SEEY + 5 );
                        bw = tw + 4;
                        lw = rng( SEEX - 6, SEEX - 2 );
                        rw = lw + 6;
                        for( int i = lw; i <= rw; i++ ) {
                            for( int j = tw; j <= bw; j++ ) {
                                if( j == tw || j == bw ) {
                                    if( ( i - lw ) % 2 == 0 ) {
                                        ter_set( point( i, j ), t_concrete_wall );
                                    } else {
                                        ter_set( point( i, j ), t_reinforced_glass );
                                    }
                                } else if( ( i - lw ) % 2 == 0 ) {
                                    ter_set( point( i, j ), t_concrete_wall );
                                } else if( j == tw + 2 ) {
                                    ter_set( point( i, j ), t_concrete_wall );
                                } else { // Empty space holds monsters!
                                    place_spawns( GROUP_NETHER, 1, point( i, j ), point( i, j ), 1, true );
                                }
                            }
                        }
                    }

                    spawn_item( point( SEEX - 1, 8 ), "id_science" );
                    tmpcomp = add_computer( tripoint( SEEX,  8, abs_sub.z ),
                                            _( "Sub-prime contact console" ), 7 );
                    if( monsters_end ) { //only add these options when there are monsters.
                        tmpcomp->add_option( _( "Terminate Specimens" ), COMPACT_TERMINATE, 2 );
                        tmpcomp->add_option( _( "Release Specimens" ), COMPACT_RELEASE, 3 );
                    }
                    tmpcomp->add_option( _( "Toggle Portal" ), COMPACT_PORTAL, 8 );
                    tmpcomp->add_option( _( "Activate Resonance Cascade" ), COMPACT_CASCADE, 10 );
                    tmpcomp->add_failure( COMPFAIL_MANHACKS );
                    tmpcomp->add_failure( COMPFAIL_SECUBOTS );
                    tmpcomp->set_access_denied_msg(
                        _( "ERROR!  Access denied!  Unauthorized access will be met with lethal force!" ) );
                    ter_set( point( SEEX - 2, 4 ), t_radio_tower );
                    ter_set( point( SEEX + 1, 4 ), t_radio_tower );
                    ter_set( point( SEEX - 2, 7 ), t_radio_tower );
                    ter_set( point( SEEX + 1, 7 ), t_radio_tower );
                }
                break;

                // Bionics
                case 4: {
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point( 6, 6 ), point( 6, 6 ), 1, true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point( SEEX * 2 - 7, 6 ),
                                  point( SEEX * 2 - 7, 6 ), 1, true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point( 6, SEEY * 2 - 7 ),
                                  point( 6, SEEY * 2 - 7 ), 1, true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point( SEEX * 2 - 7, SEEY * 2 - 7 ),
                                  point( SEEX * 2 - 7, SEEY * 2 - 7 ), 1, true );
                    mtrap_set( this, point( SEEX - 2, SEEY - 2 ), tr_dissector );
                    mtrap_set( this, point( SEEX + 1, SEEY - 2 ), tr_dissector );
                    mtrap_set( this, point( SEEX - 2, SEEY + 1 ), tr_dissector );
                    mtrap_set( this, point( SEEX + 1, SEEY + 1 ), tr_dissector );
                    square_furn( this, f_counter, point( SEEX - 1, SEEY - 1 ), point( SEEX, SEEY ) );
                    int item_count = 0;
                    while( item_count < 5 ) {
                        item_count +=
                            place_items(
                                item_group_id( "bionics" ), 75, point( SEEX - 1, SEEY - 1 ),
                                point( SEEX, SEEY ), false, calendar::start_of_cataclysm ).size();
                    }
                    line( this, t_reinforced_glass, point( SEEX - 2, SEEY - 2 ), point( SEEX + 1, SEEY - 2 ) );
                    line( this, t_reinforced_glass, point( SEEX - 2, SEEY + 1 ), point( SEEX + 1, SEEY + 1 ) );
                    line( this, t_reinforced_glass, point( SEEX - 2, SEEY - 1 ), point( SEEX - 2, SEEY ) );
                    line( this, t_reinforced_glass, point( SEEX + 1, SEEY - 1 ), point( SEEX + 1, SEEY ) );
                    spawn_item( point( SEEX - 4, SEEY - 3 ), "id_science" );
                    furn_set( point( SEEX - 3, SEEY - 3 ), furn_str_id( "f_console" ) );
                    tmpcomp = add_computer( tripoint( SEEX - 3,  SEEY - 3, abs_sub.z ),
                                            _( "Bionic access" ), 3 );
                    tmpcomp->add_option( _( "Manifest" ), COMPACT_LIST_BIONICS, 0 );
                    tmpcomp->add_option( _( "Open Chambers" ), COMPACT_RELEASE, 5 );
                    tmpcomp->add_failure( COMPFAIL_MANHACKS );
                    tmpcomp->add_failure( COMPFAIL_SECUBOTS );
                    tmpcomp->set_access_denied_msg(
                        _( "ERROR!  Access denied!  Unauthorized access will be met with lethal force!" ) );
                }
                break;

                // CVD Forge
                case 5:
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point( 6, 6 ), point( 6, 6 ), 1, true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point( SEEX * 2 - 7, 6 ),
                                  point( SEEX * 2 - 7, 6 ), 1, true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point( 6, SEEY * 2 - 7 ),
                                  point( 6, SEEY * 2 - 7 ), 1, true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point( SEEX * 2 - 7, SEEY * 2 - 7 ),
                                  point( SEEX * 2 - 7, SEEY * 2 - 7 ), 1, true );
                    line( this, t_cvdbody, point( SEEX - 2, SEEY - 2 ), point( SEEX - 2, SEEY + 1 ) );
                    line( this, t_cvdbody, point( SEEX - 1, SEEY - 2 ), point( SEEX - 1, SEEY + 1 ) );
                    line( this, t_cvdbody, point( SEEX, SEEY - 1 ), point( SEEX, SEEY + 1 ) );
                    line( this, t_cvdbody, point( SEEX + 1, SEEY - 2 ), point( SEEX + 1, SEEY + 1 ) );
                    ter_set( point( SEEX, SEEY - 2 ), t_cvdmachine );
                    spawn_item( point( SEEX, SEEY - 3 ), "id_science" );
                    break;
            }
        } // end use_hardcoded_lab_finale

        // Handle stairs in the unlikely case they are needed.

        const auto maybe_insert_stairs = [this]( const oter_id & terrain,  const ter_id & t_stair_type ) {
            if( is_ot_match( "stairs", terrain, ot_match_type::contains ) ) {
                const auto predicate = [this]( const tripoint & p ) {
                    return ter( p ) == t_thconc_floor && furn( p ) == f_null &&
                           tr_at( p ).is_null();
                };
                const auto range = points_in_rectangle( { 0, 0, abs_sub.z },
                { SEEX * 2 - 2, SEEY * 2 - 2, abs_sub.z } );
                if( const auto p = random_point( range, predicate ) ) {
                    ter_set( *p, t_stair_type );
                }
            }
        };
        maybe_insert_stairs( dat.above(), t_stairs_up );
        maybe_insert_stairs( terrain_type, t_stairs_down );

        int light_odds = 0;
        // central labs are always fully lit, other labs have half chance of some lights.
        if( central_lab ) {
            light_odds = 1;
        } else if( one_in( 2 ) ) {
            light_odds = std::pow( rng( 1, 12 ), 1.6 );
        }
        if( light_odds > 0 ) {
            for( int i = 0; i < SEEX * 2; i++ ) {
                for( int j = 0; j < SEEY * 2; j++ ) {
                    if( !( ( i * j ) % 2 || ( i + j ) % 4 ) && one_in( light_odds ) ) {
                        if( t_thconc_floor == ter( point( i, j ) ) || t_strconc_floor == ter( point( i, j ) ) ) {
                            ter_set( point( i, j ), t_thconc_floor_olight );
                        }
                    }
                }
            }
        }
    }
}

void map::draw_temple( const mapgendata &dat )
{
    const oter_id &terrain_type = dat.terrain_type();
    if( terrain_type == "temple" || terrain_type == "temple_stairs" ) {
        if( dat.zlevel() == 0 ) {
            // Ground floor
            // TODO: More varieties?
            fill_background( this, t_dirt );
            square( this, t_grate, point( SEEX - 1, SEEY - 1 ), point( SEEX, SEEX ) );
            ter_set( point( SEEX + 1, SEEY + 1 ), t_pedestal_temple );
        } else {
            // Underground!  Shit's about to get interesting!
            // Start with all rock floor
            square( this, t_rock_floor, point_zero, point( EAST_EDGE, SOUTH_EDGE ) );
            // We always start at the south and go north.
            // We use (y / 2 + z) % 4 to guarantee that rooms don't repeat.
            switch( 1 + std::abs( abs_sub.y / 2 + dat.zlevel() + 4 ) % 4 ) { // TODO: More varieties!

                case 1:
                    // Flame bursts
                    square( this, t_rock, point_zero, point( SEEX - 1, SOUTH_EDGE ) );
                    square( this, t_rock, point( SEEX + 2, 0 ), point( EAST_EDGE, SOUTH_EDGE ) );
                    for( int i = 2; i < SEEY * 2 - 4; i++ ) {
                        add_field( {SEEX, i, abs_sub.z}, fd_fire_vent, rng( 1, 3 ) );
                        add_field( {SEEX + 1, i, abs_sub.z}, fd_fire_vent, rng( 1, 3 ) );
                    }
                    break;

                case 2:
                    // Spreading water
                    square( this, t_water_dp, point( 4, 4 ), point( 5, 5 ) );
                    // replaced mon_sewer_snake spawn with GROUP_SEWER
                    // Decide whether a group of only sewer snakes be made, probably not worth it
                    place_spawns( GROUP_SEWER, 1, point( 4, 4 ), point( 4, 4 ), 1, true );

                    square( this, t_water_dp, point( SEEX * 2 - 5, 4 ), point( SEEX * 2 - 4, 6 ) );
                    place_spawns( GROUP_SEWER, 1, point( 1, SEEX * 2 - 5 ), point( 1, SEEX * 2 - 5 ), 1, true );

                    square( this, t_water_dp, point( 4, SEEY * 2 - 5 ), point( 6, SEEY * 2 - 4 ) );

                    square( this, t_water_dp, point( SEEX * 2 - 5, SEEY * 2 - 5 ), point( SEEX * 2 - 4,
                            SEEY * 2 - 4 ) );

                    square( this, t_rock, point( 0, SEEY * 2 - 2 ), point( SEEX - 1, SOUTH_EDGE ) );
                    square( this, t_rock, point( SEEX + 2, SEEY * 2 - 2 ), point( EAST_EDGE, SOUTH_EDGE ) );
                    line( this, t_grate, point( SEEX, 1 ), point( SEEX + 1, 1 ) ); // To drain the water
                    mtrap_set( this, point( SEEX, SEEY * 2 - 2 ), tr_temple_flood );
                    mtrap_set( this, point( SEEX + 1, SEEY * 2 - 2 ), tr_temple_flood );
                    for( int y = 2; y < SEEY * 2 - 2; y++ ) {
                        for( int x = 2; x < SEEX * 2 - 2; x++ ) {
                            if( ter( point( x, y ) ) == t_rock_floor && one_in( 4 ) ) {
                                mtrap_set( this, point( x, y ), tr_temple_flood );
                            }
                        }
                    }
                    break;

                case 3: { // Flipping walls puzzle
                    line( this, t_rock, point_zero, point( SEEX - 1, 0 ) );
                    line( this, t_rock, point( SEEX + 2, 0 ), point( EAST_EDGE, 0 ) );
                    line( this, t_rock, point( SEEX - 1, 1 ), point( SEEX - 1, 6 ) );
                    line( this, t_bars, point( SEEX + 2, 1 ), point( SEEX + 2, 6 ) );
                    ter_set( point( 14, 1 ), t_switch_rg );
                    ter_set( point( 15, 1 ), t_switch_gb );
                    ter_set( point( 16, 1 ), t_switch_rb );
                    ter_set( point( 17, 1 ), t_switch_even );
                    // Start with clear floors--then work backwards to the starting state
                    line( this, t_floor_red,   point( SEEX, 1 ), point( SEEX + 1, 1 ) );
                    line( this, t_floor_green, point( SEEX, 2 ), point( SEEX + 1, 2 ) );
                    line( this, t_floor_blue,  point( SEEX, 3 ), point( SEEX + 1, 3 ) );
                    line( this, t_floor_red,   point( SEEX, 4 ), point( SEEX + 1, 4 ) );
                    line( this, t_floor_green, point( SEEX, 5 ), point( SEEX + 1, 5 ) );
                    line( this, t_floor_blue,  point( SEEX, 6 ), point( SEEX + 1, 6 ) );
                    // Now, randomly choose actions
                    // Set up an actions vector so that there's not undue repetition
                    std::vector<int> actions;
                    actions.push_back( 1 );
                    actions.push_back( 2 );
                    actions.push_back( 3 );
                    actions.push_back( 4 );
                    actions.push_back( rng( 1, 3 ) );
                    while( !actions.empty() ) {
                        const int action = random_entry_removed( actions );
                        for( int y = 1; y < 7; y++ ) {
                            for( int x = SEEX; x <= SEEX + 1; x++ ) {
                                switch( action ) {
                                    case 1:
                                        // Toggle RG
                                        if( ter( point( x, y ) ) == t_floor_red ) {
                                            ter_set( point( x, y ), t_rock_red );
                                        } else if( ter( point( x, y ) ) == t_rock_red ) {
                                            ter_set( point( x, y ), t_floor_red );
                                        } else if( ter( point( x, y ) ) == t_floor_green ) {
                                            ter_set( point( x, y ), t_rock_green );
                                        } else if( ter( point( x, y ) ) == t_rock_green ) {
                                            ter_set( point( x, y ), t_floor_green );
                                        }
                                        break;
                                    case 2:
                                        // Toggle GB
                                        if( ter( point( x, y ) ) == t_floor_blue ) {
                                            ter_set( point( x, y ), t_rock_blue );
                                        } else if( ter( point( x, y ) ) == t_rock_blue ) {
                                            ter_set( point( x, y ), t_floor_blue );
                                        } else if( ter( point( x, y ) ) == t_floor_green ) {
                                            ter_set( point( x, y ), t_rock_green );
                                        } else if( ter( point( x, y ) ) == t_rock_green ) {
                                            ter_set( point( x, y ), t_floor_green );
                                        }
                                        break;
                                    case 3:
                                        // Toggle RB
                                        if( ter( point( x, y ) ) == t_floor_blue ) {
                                            ter_set( point( x, y ), t_rock_blue );
                                        } else if( ter( point( x, y ) ) == t_rock_blue ) {
                                            ter_set( point( x, y ), t_floor_blue );
                                        } else if( ter( point( x, y ) ) == t_floor_red ) {
                                            ter_set( point( x, y ), t_rock_red );
                                        } else if( ter( point( x, y ) ) == t_rock_red ) {
                                            ter_set( point( x, y ), t_floor_red );
                                        }
                                        break;
                                    case 4:
                                        // Toggle Even
                                        if( y % 2 == 0 ) {
                                            if( ter( point( x, y ) ) == t_floor_blue ) {
                                                ter_set( point( x, y ), t_rock_blue );
                                            } else if( ter( point( x, y ) ) == t_rock_blue ) {
                                                ter_set( point( x, y ), t_floor_blue );
                                            } else if( ter( point( x, y ) ) == t_floor_red ) {
                                                ter_set( point( x, y ), t_rock_red );
                                            } else if( ter( point( x, y ) ) == t_rock_red ) {
                                                ter_set( point( x, y ), t_floor_red );
                                            } else if( ter( point( x, y ) ) == t_floor_green ) {
                                                ter_set( point( x, y ), t_rock_green );
                                            } else if( ter( point( x, y ) ) == t_rock_green ) {
                                                ter_set( point( x, y ), t_floor_green );
                                            }
                                        }
                                        break;
                                }
                            }
                        }
                    }
                }
                break;

                case 4: { // Toggling walls maze
                    square( this, t_rock, point_zero, point( SEEX     - 1, 1 ) );
                    square( this, t_rock, point( 0, SEEY * 2 - 2 ), point( SEEX     - 1, SOUTH_EDGE ) );
                    square( this, t_rock, point( 0, 2 ), point( SEEX     - 4, SEEY * 2 - 3 ) );
                    square( this, t_rock, point( SEEX + 2, 0 ), point( EAST_EDGE, 1 ) );
                    square( this, t_rock, point( SEEX + 2, SEEY * 2 - 2 ), point( EAST_EDGE, SOUTH_EDGE ) );
                    square( this, t_rock, point( SEEX + 5, 2 ), point( EAST_EDGE, SEEY * 2 - 3 ) );
                    int x = rng( SEEX - 1, SEEX + 2 ), y = 2;
                    std::vector<point> path; // Path, from end to start
                    while( x < SEEX - 1 || x > SEEX + 2 || y < SEEY * 2 - 2 ) {
                        static const std::vector<ter_id> terrains = {
                            t_floor_red, t_floor_green, t_floor_blue,
                        };
                        path.push_back( point( x, y ) );
                        ter_set( point( x, y ), random_entry( terrains ) );
                        if( y == SEEY * 2 - 2 ) {
                            if( x < SEEX - 1 ) {
                                x++;
                            } else if( x > SEEX + 2 ) {
                                x--;
                            }
                        } else {
                            std::vector<point> next;
                            for( int nx = x - 1; nx <= x + 1; nx++ ) {
                                for( int ny = y; ny <= y + 1; ny++ ) {
                                    if( ter( point( nx, ny ) ) == t_rock_floor ) {
                                        next.push_back( point( nx, ny ) );
                                    }
                                }
                            }
                            if( next.empty() ) {
                                break;
                            } else {
                                const point p = random_entry( next );
                                x = p.x;
                                y = p.y;
                            }
                        }
                    }
                    // Now go backwards through path (start to finish), toggling any tiles that need
                    bool toggle_red = false;
                    bool toggle_green = false;
                    bool toggle_blue = false;
                    for( int i = path.size() - 1; i >= 0; i-- ) {
                        if( ter( path[i] ) == t_floor_red ) {
                            toggle_green = !toggle_green;
                            if( toggle_red ) {
                                ter_set( path[i], t_rock_red );
                            }
                        } else if( ter( path[i] ) == t_floor_green ) {
                            toggle_blue = !toggle_blue;
                            if( toggle_green ) {
                                ter_set( path[i], t_rock_green );
                            }
                        } else if( ter( path[i] ) == t_floor_blue ) {
                            toggle_red = !toggle_red;
                            if( toggle_blue ) {
                                ter_set( path[i], t_rock_blue );
                            }
                        }
                    }
                    // Finally, fill in the rest with random tiles, and place toggle traps
                    for( int i = SEEX - 3; i <= SEEX + 4; i++ ) {
                        for( int j = 2; j <= SEEY * 2 - 2; j++ ) {
                            mtrap_set( this, point( i, j ), tr_temple_toggle );
                            if( ter( point( i, j ) ) == t_rock_floor ) {
                                static const std::vector<ter_id> terrains = {
                                    t_rock_red, t_rock_green, t_rock_blue,
                                    t_floor_red, t_floor_green, t_floor_blue,
                                };
                                ter_set( point( i, j ), random_entry( terrains ) );
                            }
                        }
                    }
                }
                break;
            } // Done with room type switch
            // Stairs down if we need them
            if( terrain_type == "temple_stairs" ) {
                line( this, t_stairs_down, point( SEEX, 0 ), point( SEEX + 1, 0 ) );
            }
            // Stairs at the south if dat.above() has stairs down.
            if( dat.above() == "temple_stairs" ) {
                line( this, t_stairs_up, point( SEEX, SOUTH_EDGE ), point( SEEX + 1, SOUTH_EDGE ) );
            }

        } // Done with underground-only stuff
    } else if( terrain_type == "temple_finale" ) {
        fill_background( this, t_rock );
        square( this, t_rock_floor, point( SEEX - 1, 1 ), point( SEEX + 2, 4 ) );
        square( this, t_rock_floor, point( SEEX, 5 ), point( SEEX + 1, SOUTH_EDGE ) );
        line( this, t_stairs_up, point( SEEX, SOUTH_EDGE ), point( SEEX + 1, SOUTH_EDGE ) );
        spawn_artifact( tripoint( rng( SEEX, SEEX + 1 ), rng( 2, 3 ), abs_sub.z ),
                        relic_procgen_id( "cult" ) );
        spawn_artifact( tripoint( rng( SEEX, SEEX + 1 ), rng( 2, 3 ), abs_sub.z ),
                        relic_procgen_id( "cult" ) );
        return;

    }
}

void map::draw_mine( mapgendata &dat )
{
    const oter_id &terrain_type = dat.terrain_type();
    if( terrain_type == "mine" || terrain_type == "mine_down" ) {
        if( is_ot_match( "mine", dat.north(), ot_match_type::prefix ) ) {
            dat.n_fac = ( one_in( 10 ) ? 0 : -2 );
        } else {
            dat.n_fac = 4;
        }
        if( is_ot_match( "mine", dat.east(), ot_match_type::prefix ) ) {
            dat.e_fac = ( one_in( 10 ) ? 0 : -2 );
        } else {
            dat.e_fac = 4;
        }
        if( is_ot_match( "mine", dat.south(), ot_match_type::prefix ) ) {
            dat.s_fac = ( one_in( 10 ) ? 0 : -2 );
        } else {
            dat.s_fac = 4;
        }
        if( is_ot_match( "mine", dat.west(), ot_match_type::prefix ) ) {
            dat.w_fac = ( one_in( 10 ) ? 0 : -2 );
        } else {
            dat.w_fac = 4;
        }

        for( int i = 0; i < SEEX * 2; i++ ) {
            for( int j = 0; j < SEEY * 2; j++ ) {
                if( i >= dat.w_fac + rng( 0, 2 ) && i <= EAST_EDGE - dat.e_fac - rng( 0, 2 ) &&
                    j >= dat.n_fac + rng( 0, 2 ) && j <= SOUTH_EDGE - dat.s_fac - rng( 0, 2 ) &&
                    i + j >= 4 && ( SEEX * 2 - i ) + ( SEEY * 2 - j ) >= 6 ) {
                    ter_set( point( i, j ), t_rock_floor );
                } else {
                    ter_set( point( i, j ), t_rock );
                }
            }
        }

        // Not an entrance; maybe some hazards!
        switch( rng( 0, 6 ) ) {
            case 0:
                break; // Nothing!  Lucky!

            case 1: { // Toxic gas
                point gas_vent_location( rng( 9, 14 ), rng( 9, 14 ) );
                ter_set( point( gas_vent_location ), t_rock );
                add_field( { gas_vent_location, abs_sub.z}, fd_gas_vent, 2 );
            }
            break;

            case 2: { // Lava
                point start_location( rng( 6, SEEX ), rng( 6, SEEY ) );
                point end_location( rng( SEEX + 1, SEEX * 2 - 7 ), rng( SEEY + 1, SEEY * 2 - 7 ) );
                const int num = rng( 2, 4 );
                for( int i = 0; i < num; i++ ) {
                    int lx1 = start_location.x + rng( -1, 1 ), lx2 = end_location.x + rng( -1, 1 ),
                        ly1 = start_location.y + rng( -1, 1 ), ly2 = end_location.y + rng( -1, 1 );
                    line( this, t_lava, point( lx1, ly1 ), point( lx2, ly2 ) );
                }
            }
            break;

            case 3: { // Wrecked equipment
                point wreck_location( rng( 9, 14 ), rng( 9, 14 ) );
                for( int i = wreck_location.x - 3; i < wreck_location.x + 3; i++ ) {
                    for( int j = wreck_location.y - 3; j < wreck_location.y + 3; j++ ) {
                        if( !one_in( 4 ) ) {
                            make_rubble( tripoint( i,  j, abs_sub.z ), f_wreckage, true );
                        }
                    }
                }
                place_items( item_group_id( "wreckage" ), 70, wreck_location + point( -3, -3 ),
                             wreck_location + point( 2, 2 ), false, calendar::start_of_cataclysm );
            }
            break;

            case 4: { // Dead miners
                const int num_bodies = rng( 4, 8 );
                for( int i = 0; i < num_bodies; i++ ) {
                    if( const auto body = random_point( *this, [this]( const tripoint & p ) {
                    return move_cost( p ) == 2;
                    } ) ) {
                        add_item( *body, item::make_corpse() );
                        place_items( item_group_id( "mine_equipment" ), 60, *body, *body,
                                     false, calendar::start_of_cataclysm );
                    }
                }
            }
            break;

            case 5: { // Dark worm!
                const int num_worms = rng( 1, 5 );
                for( int i = 0; i < num_worms; i++ ) {
                    std::vector<direction> sides;
                    if( dat.n_fac == 6 ) {
                        sides.push_back( direction::NORTH );
                    }
                    if( dat.e_fac == 6 ) {
                        sides.push_back( direction::EAST );
                    }
                    if( dat.s_fac == 6 ) {
                        sides.push_back( direction::SOUTH );
                    }
                    if( dat.w_fac == 6 ) {
                        sides.push_back( direction::WEST );
                    }
                    if( sides.empty() ) {
                        place_spawns( GROUP_DARK_WYRM, 1, point( SEEX, SEEY ), point( SEEX, SEEY ), 1, true );
                        i = num_worms;
                    } else {
                        point p;
                        switch( random_entry( sides ) ) {
                            case direction::NORTH:
                                p = point( rng( 1, SEEX * 2 - 2 ), rng( 1, 5 ) );
                                break;
                            case direction::EAST:
                                p = point( SEEX * 2 - rng( 2, 6 ), rng( 1, SEEY * 2 - 2 ) );
                                break;
                            case direction::SOUTH:
                                p = point( rng( 1, SEEX * 2 - 2 ), SEEY * 2 - rng( 2, 6 ) );
                                break;
                            case direction::WEST:
                                p = point( rng( 1, 5 ), rng( 1, SEEY * 2 - 2 ) );
                                break;
                            default:
                                break;
                        }
                        ter_set( p, t_rock_floor );
                        place_spawns( GROUP_DARK_WYRM, 1, p, p, 1, true );
                    }
                }
            }
            break;

            case 6: { // Spiral
                const int orx = rng( SEEX - 4, SEEX ), ory = rng( SEEY - 4, SEEY );
                line( this, t_rock, point( orx, ory ), point( orx + 5, ory ) );
                line( this, t_rock, point( orx + 5, ory ), point( orx + 5, ory + 5 ) );
                line( this, t_rock, point( orx + 1, ory + 5 ), point( orx + 5, ory + 5 ) );
                line( this, t_rock, point( orx + 1, ory + 2 ), point( orx + 1, ory + 4 ) );
                line( this, t_rock, point( orx + 1, ory + 2 ), point( orx + 3, ory + 2 ) );
                ter_set( point( orx + 3, ory + 3 ), t_rock );
                add_item( point( orx + 2, ory + 3 ), item::make_corpse() );
                place_items( item_group_id( "mine_equipment" ), 60, point( orx + 2, ory + 3 ),
                             point( orx + 2, ory + 3 ), false, calendar::start_of_cataclysm );
            }
            break;
        }

        if( terrain_type == "mine_down" ) { // Don't forget to build a slope down!
            std::vector<direction> open;
            if( dat.n_fac == 4 ) {
                open.push_back( direction::NORTH );
            }
            if( dat.e_fac == 4 ) {
                open.push_back( direction::EAST );
            }
            if( dat.s_fac == 4 ) {
                open.push_back( direction::SOUTH );
            }
            if( dat.w_fac == 4 ) {
                open.push_back( direction::WEST );
            }

            if( open.empty() ) { // We'll have to build it in the center
                int tries = 0;
                point p;
                bool okay = true;
                do {
                    p.x = rng( SEEX - 6, SEEX + 1 );
                    p.y = rng( SEEY - 6, SEEY + 1 );
                    okay = true;
                    for( int i = p.x; ( i <= p.x + 5 ) && okay; i++ ) {
                        for( int j = p.y; ( j <= p.y + 5 ) && okay; j++ ) {
                            if( ter( point( i, j ) ) != t_rock_floor ) {
                                okay = false;
                            }
                        }
                    }
                    if( !okay ) {
                        tries++;
                    }
                } while( !okay && tries < 10 );
                if( tries == 10 ) { // Clear the area around the slope down
                    square( this, t_rock_floor, p, p + point( 5, 5 ) );
                }
                // NOLINTNEXTLINE(cata-use-named-point-constants)
                square( this, t_slope_down, p + point( 1, 1 ), p + point( 2, 2 ) );
            } else { // We can build against a wall
                switch( random_entry( open ) ) {
                    case direction::NORTH:
                        square( this, t_rock_floor, point( SEEX - 3, 6 ), point( SEEX + 2, SEEY ) );
                        line( this, t_slope_down, point( SEEX - 2, 6 ), point( SEEX + 1, 6 ) );
                        break;
                    case direction::EAST:
                        square( this, t_rock_floor, point( SEEX + 1, SEEY - 3 ), point( SEEX * 2 - 7, SEEY + 2 ) );
                        line( this, t_slope_down, point( SEEX * 2 - 7, SEEY - 2 ), point( SEEX * 2 - 7, SEEY + 1 ) );
                        break;
                    case direction::SOUTH:
                        square( this, t_rock_floor, point( SEEX - 3, SEEY + 1 ), point( SEEX + 2, SEEY * 2 - 7 ) );
                        line( this, t_slope_down, point( SEEX - 2, SEEY * 2 - 7 ), point( SEEX + 1, SEEY * 2 - 7 ) );
                        break;
                    case direction::WEST:
                        square( this, t_rock_floor, point( 6, SEEY - 3 ), point( SEEX, SEEY + 2 ) );
                        line( this, t_slope_down, point( 6, SEEY - 2 ), point( 6, SEEY + 1 ) );
                        break;
                    default:
                        break;
                }
            }
        } // Done building a slope down

        if( dat.above() == "mine_down" ) { // Don't forget to build a slope up!
            std::vector<direction> open;
            if( dat.n_fac == 6 && ter( point( SEEX, 6 ) ) != t_slope_down ) {
                open.push_back( direction::NORTH );
            }
            if( dat.e_fac == 6 && ter( point( SEEX * 2 - 7, SEEY ) ) != t_slope_down ) {
                open.push_back( direction::EAST );
            }
            if( dat.s_fac == 6 && ter( point( SEEX, SEEY * 2 - 7 ) ) != t_slope_down ) {
                open.push_back( direction::SOUTH );
            }
            if( dat.w_fac == 6 && ter( point( 6, SEEY ) ) != t_slope_down ) {
                open.push_back( direction::WEST );
            }

            if( open.empty() ) { // We'll have to build it in the center
                int tries = 0;
                point p;
                bool okay = true;
                do {
                    p.x = rng( SEEX - 6, SEEX + 1 );
                    p.y = rng( SEEY - 6, SEEY + 1 );
                    okay = true;
                    for( int i = p.x; ( i <= p.x + 5 ) && okay; i++ ) {
                        for( int j = p.y; ( j <= p.y + 5 ) && okay; j++ ) {
                            if( ter( point( i, j ) ) != t_rock_floor ) {
                                okay = false;
                            }
                        }
                    }
                    if( !okay ) {
                        tries++;
                    }
                } while( !okay && tries < 10 );
                if( tries == 10 ) { // Clear the area around the slope down
                    square( this, t_rock_floor, p, p + point( 5, 5 ) );
                }
                // NOLINTNEXTLINE(cata-use-named-point-constants)
                square( this, t_slope_up, p + point( 1, 1 ), p + point( 2, 2 ) );

            } else { // We can build against a wall
                switch( random_entry( open ) ) {
                    case direction::NORTH:
                        line( this, t_slope_up, point( SEEX - 2, 6 ), point( SEEX + 1, 6 ) );
                        break;
                    case direction::EAST:
                        line( this, t_slope_up, point( SEEX * 2 - 7, SEEY - 2 ), point( SEEX * 2 - 7, SEEY + 1 ) );
                        break;
                    case direction::SOUTH:
                        line( this, t_slope_up, point( SEEX - 2, SEEY * 2 - 7 ), point( SEEX + 1, SEEY * 2 - 7 ) );
                        break;
                    case direction::WEST:
                        line( this, t_slope_up, point( 6, SEEY - 2 ), point( 6, SEEY + 1 ) );
                        break;
                    default:
                        break;
                }
            }
        } // Done building a slope up
    } else if( terrain_type == "mine_finale" ) {
        // Set up the basic chamber
        for( int i = 0; i < SEEX * 2; i++ ) {
            for( int j = 0; j < SEEY * 2; j++ ) {
                if( i > rng( 1, 3 ) && i < SEEX * 2 - rng( 2, 4 ) &&
                    j > rng( 1, 3 ) && j < SEEY * 2 - rng( 2, 4 ) ) {
                    ter_set( point( i, j ), t_rock_floor );
                } else {
                    ter_set( point( i, j ), t_rock );
                }
            }
        }
        std::vector<direction> face; // Which walls are solid, and can be a facing?
        // Now draw the entrance(s)
        if( dat.north() == "mine" ) {
            square( this, t_rock_floor, point( SEEX, 0 ), point( SEEX + 1, 3 ) );
        } else {
            face.push_back( direction::NORTH );
        }

        if( dat.east()  == "mine" ) {
            square( this, t_rock_floor, point( SEEX * 2 - 4, SEEY ), point( EAST_EDGE, SEEY + 1 ) );
        } else {
            face.push_back( direction::EAST );
        }

        if( dat.south() == "mine" ) {
            square( this, t_rock_floor, point( SEEX, SEEY * 2 - 4 ), point( SEEX + 1, SOUTH_EDGE ) );
        } else {
            face.push_back( direction::SOUTH );
        }

        if( dat.west()  == "mine" ) {
            square( this, t_rock_floor, point( 0, SEEY ), point( 3, SEEY + 1 ) );
        } else {
            face.push_back( direction::WEST );
        }

        // Now, pick and generate a type of finale!
        int rn = 0;
        if( face.empty() ) {
            rn = rng( 1, 3 );  // Amigara fault is not valid
        } else {
            rn = rng( 1, 4 );
        }

        computer *tmpcomp = nullptr;
        switch( rn ) {
            case 1: { // Wyrms
                const int x = rng( SEEX, SEEX + 1 ), y = rng( SEEY, SEEY + 1 );
                ter_set( point( x, y ), t_pedestal_wyrm );
                spawn_item( point( x, y ), "petrified_eye" );
            }
            break; // That's it!  game::examine handles the pedestal/wyrm spawns

            case 2: { // The Thing dog
                const int num_bodies = rng( 4, 8 );
                for( int i = 0; i < num_bodies; i++ ) {
                    int x = rng( 4, SEEX * 2 - 5 );
                    int y = rng( 4, SEEX * 2 - 5 );
                    add_item( point( x, y ), item::make_corpse() );
                    place_items( item_group_id( "mine_equipment" ), 60, point( x, y ),
                                 point( x, y ), false, calendar::start_of_cataclysm );
                }
                place_spawns( GROUP_DOG_THING, 1, point( SEEX, SEEX ), point( SEEX + 1, SEEX + 1 ), 1, true, true );
                spawn_artifact( tripoint( rng( SEEX, SEEX + 1 ), rng( SEEY, SEEY + 1 ), abs_sub.z ),
                                relic_procgen_id( "netherum_tunnels" ) );
            }
            break;

            case 3: { // Spiral down
                line( this, t_rock,  point( 5, 5 ),  point( 5, 18 ) );
                line( this, t_rock,  point( 5, 5 ), point( 18, 5 ) );
                line( this, t_rock, point( 18, 5 ), point( 18, 18 ) );
                line( this, t_rock,  point( 8, 18 ), point( 18, 18 ) );
                line( this, t_rock,  point( 8, 8 ),  point( 8, 18 ) );
                line( this, t_rock,  point( 8, 8 ), point( 15, 8 ) );
                line( this, t_rock, point( 15, 8 ), point( 15, 15 ) );
                line( this, t_rock, point( 10, 15 ), point( 15, 15 ) );
                line( this, t_rock, point( 10, 10 ), point( 10, 15 ) );
                line( this, t_rock, point( 10, 10 ), point( 13, 10 ) );
                line( this, t_rock, point( 13, 10 ), point( 13, 13 ) );
                ter_set( point( 12, 13 ), t_rock );
                ter_set( point( 12, 12 ), t_slope_down );
                ter_set( point( 12, 11 ), t_slope_down );
            }
            break;

            case 4: { // Amigara fault
                // Construct the fault on the appropriate face
                switch( random_entry( face ) ) {
                    case direction::NORTH:
                        square( this, t_rock, point_zero, point( EAST_EDGE, 4 ) );
                        line( this, t_fault, point( 4, 4 ), point( SEEX * 2 - 5, 4 ) );
                        break;
                    case direction::EAST:
                        square( this, t_rock, point( SEEX * 2 - 5, 0 ), point( SOUTH_EDGE, EAST_EDGE ) );
                        line( this, t_fault, point( SEEX * 2 - 5, 4 ), point( SEEX * 2 - 5, SEEY * 2 - 5 ) );
                        break;
                    case direction::SOUTH:
                        square( this, t_rock, point( 0, SEEY * 2 - 5 ), point( EAST_EDGE, SOUTH_EDGE ) );
                        line( this, t_fault, point( 4, SEEY * 2 - 5 ), point( SEEX * 2 - 5, SEEY * 2 - 5 ) );
                        break;
                    case direction::WEST:
                        square( this, t_rock, point_zero, point( 4, SOUTH_EDGE ) );
                        line( this, t_fault, point( 4, 4 ), point( 4, SEEY * 2 - 5 ) );
                        break;
                    default:
                        break;
                }

                furn_set( point( SEEX, SEEY ), furn_str_id( "f_console" ) );
                tmpcomp = add_computer( tripoint( SEEX,  SEEY, abs_sub.z ), _( "NEPowerOS" ), 0 );
                tmpcomp->add_option( _( "Read Logs" ), COMPACT_AMIGARA_LOG, 0 );
                tmpcomp->add_option( _( "Initiate Tremors" ), COMPACT_AMIGARA_START, 4 );
                tmpcomp->add_failure( COMPFAIL_AMIGARA );
            }
            break;
        }

    }
}

void map::draw_spiral( const mapgendata &dat )
{
    const oter_id &terrain_type = dat.terrain_type();
    if( terrain_type == "spiral_hub" ) {
        fill_background( this, t_rock_floor );
        line( this, t_rock, point( 23, 0 ), point( 23, 23 ) );
        line( this, t_rock,  point( 2, 23 ), point( 23, 23 ) );
        line( this, t_rock,  point( 2, 4 ),  point( 2, 23 ) );
        line( this, t_rock,  point( 2, 4 ), point( 18, 4 ) );
        line( this, t_rock, point( 18, 4 ), point( 18, 18 ) ); // bad
        line( this, t_rock,  point( 6, 18 ), point( 18, 18 ) );
        line( this, t_rock,  point( 6, 7 ),  point( 6, 18 ) );
        line( this, t_rock,  point( 6, 7 ), point( 15, 7 ) );
        line( this, t_rock, point( 15, 7 ), point( 15, 15 ) );
        line( this, t_rock,  point( 8, 15 ), point( 15, 15 ) );
        line( this, t_rock,  point( 8, 9 ),  point( 8, 15 ) );
        line( this, t_rock,  point( 8, 9 ), point( 13, 9 ) );
        line( this, t_rock, point( 13, 9 ), point( 13, 13 ) );
        line( this, t_rock, point( 10, 13 ), point( 13, 13 ) );
        line( this, t_rock, point( 10, 11 ), point( 10, 13 ) );
        square( this, t_slope_up, point( 11, 11 ), point( 12, 12 ) );
        rotate( rng( 0, 3 ) );
    } else if( terrain_type == "spiral" ) {
        fill_background( this, t_rock_floor );
        const int num_spiral = rng( 1, 4 );
        std::list<point> offsets;
        const int spiral_width = 8;
        // Divide the room into quadrants, and place a spiral origin
        // at a random offset within each quadrant.
        for( int x = 0; x < 2; ++x ) {
            for( int y = 0; y < 2; ++y ) {
                const int x_jitter = rng( 0, SEEX - spiral_width );
                const int y_jitter = rng( 0, SEEY - spiral_width );
                offsets.push_back( point( ( x * SEEX ) + x_jitter,
                                          ( y * SEEY ) + y_jitter ) );
            }
        }

        // Randomly place from 1 - 4 of the spirals at the chosen offsets.
        for( int i = 0; i < num_spiral; i++ ) {
            const point chosen_point = random_entry_removed( offsets );
            const int orx = chosen_point.x;
            const int ory = chosen_point.y;

            line( this, t_rock, point( orx, ory ), point( orx + 5, ory ) );
            line( this, t_rock, point( orx + 5, ory ), point( orx + 5, ory + 5 ) );
            line( this, t_rock, point( orx + 1, ory + 5 ), point( orx + 5, ory + 5 ) );
            line( this, t_rock, point( orx + 1, ory + 2 ), point( orx + 1, ory + 4 ) );
            line( this, t_rock, point( orx + 1, ory + 2 ), point( orx + 3, ory + 2 ) );
            ter_set( point( orx + 3, ory + 3 ), t_rock );
            ter_set( point( orx + 2, ory + 3 ), t_rock_floor );
            place_items( item_group_id( "spiral" ), 60, point( orx + 2, ory + 3 ),
                         point( orx + 2, ory + 3 ), false, calendar::turn_zero );
        }
    }
}

void map::draw_spider_pit( const mapgendata &dat )
{
    const oter_id &terrain_type = dat.terrain_type();
    if( terrain_type == "spider_pit_under" ) {
        for( int i = 0; i < SEEX * 2; i++ ) {
            for( int j = 0; j < SEEY * 2; j++ ) {
                if( ( i >= 3 && i <= SEEX * 2 - 4 && j >= 3 && j <= SEEY * 2 - 4 ) ||
                    one_in( 4 ) ) {
                    ter_set( point( i, j ), t_rock_floor );
                    if( !one_in( 3 ) ) {
                        add_field( {i, j, abs_sub.z}, fd_web, rng( 1, 3 ) );
                    }
                } else {
                    ter_set( point( i, j ), t_rock );
                }
            }
        }
        ter_set( point( rng( 3, SEEX * 2 - 4 ), rng( 3, SEEY * 2 - 4 ) ), t_slope_up );
        place_items( item_group_id( "spider" ), 85, point_zero, point( EAST_EDGE, SOUTH_EDGE ),
                     false, calendar::start_of_cataclysm );
    }
}

void map::draw_anthill( const mapgendata &dat )
{
    const oter_id &terrain_type = dat.terrain_type();
    if( terrain_type == "anthill" || terrain_type == "acid_anthill" ) {
        for( int i = 0; i < SEEX * 2; i++ ) {
            for( int j = 0; j < SEEY * 2; j++ ) {
                if( i < 8 || j < 8 || i > SEEX * 2 - 9 || j > SEEY * 2 - 9 ) {
                    ter_set( point( i, j ), dat.groundcover() );
                } else if( ( i == 11 || i == 12 ) && ( j == 11 || j == 12 ) ) {
                    ter_set( point( i, j ), t_slope_down );
                } else {
                    ter_set( point( i, j ), t_dirtmound );
                }
            }
        }
    }
}

void map::draw_slimepit( const mapgendata &dat )
{
    const oter_id &terrain_type = dat.terrain_type();
    if( is_ot_match( "slimepit", terrain_type, ot_match_type::prefix ) ) {
        for( int i = 0; i < SEEX * 2; i++ ) {
            for( int j = 0; j < SEEY * 2; j++ ) {
                if( !one_in( 10 ) && ( j < dat.n_fac * SEEX ||
                                       i < dat.w_fac * SEEX ||
                                       j > SEEY * 2 - dat.s_fac * SEEY ||
                                       i > SEEX * 2 - dat.e_fac * SEEX ) ) {
                    ter_set( point( i, j ), ( !one_in( 10 ) ? t_slime : t_rock_floor ) );
                } else if( rng( 0, SEEX ) > std::abs( i - SEEX ) && rng( 0, SEEY ) > std::abs( j - SEEY ) ) {
                    ter_set( point( i, j ), t_slime );
                } else if( dat.zlevel() == 0 ) {
                    ter_set( point( i, j ), t_dirt );
                } else {
                    ter_set( point( i, j ), t_rock_floor );
                }
            }
        }
        if( terrain_type == "slimepit_down" ) {
            ter_set( point( rng( 3, SEEX * 2 - 4 ), rng( 3, SEEY * 2 - 4 ) ), t_slope_down );
        }
        if( dat.above() == "slimepit_down" ) {
            switch( rng( 1, 4 ) ) {
                case 1:
                    ter_set( point( rng( 0, 2 ), rng( 0, 2 ) ), t_slope_up );
                    break;
                case 2:
                    ter_set( point( rng( 0, 2 ), SEEY * 2 - rng( 1, 3 ) ), t_slope_up );
                    break;
                case 3:
                    ter_set( point( SEEX * 2 - rng( 1, 3 ), rng( 0, 2 ) ), t_slope_up );
                    break;
                case 4:
                    ter_set( point( SEEX * 2 - rng( 1, 3 ), SEEY * 2 - rng( 1, 3 ) ), t_slope_up );
            }
        }
        place_spawns( GROUP_SLIME, 1, point( SEEX, SEEY ), point( SEEX, SEEY ), 0.15 );
        place_items( item_group_id( "sewer" ), 40, point_zero, point( EAST_EDGE, SOUTH_EDGE ), true,
                     calendar::start_of_cataclysm );
    }
}

void map::draw_triffid( const mapgendata &dat )
{
    const oter_id &terrain_type = dat.terrain_type();
    if( terrain_type == "triffid_roots" ) {
        fill_background( this, t_root_wall );
        int node = 0;
        int step = 0;
        bool node_built[16];
        bool done = false;
        for( auto &elem : node_built ) {
            elem = false;
        }
        do {
            node_built[node] = true;
            step++;
            point node2( 1 + 6 * ( node % 4 ), 1 + 6 * static_cast<int>( node / 4 ) );
            // Clear a 4x4 dirt square
            square( this, t_dirt, node2, node2 + point( 3, 3 ) );
            // Spawn a monster in there
            if( step > 2 ) { // First couple of chambers are safe
                int monrng = rng( 1, 25 );
                point spawn( node2 + point( rng( 0, 3 ), rng( 0, 3 ) ) );
                if( monrng <= 24 ) {
                    place_spawns( GROUP_TRIFFID_OUTER, 1, node2,
                                  node2 + point( 3, 3 ), 1, true );
                } else {
                    for( int webx = node2.x; webx <= node2.x + 3; webx++ ) {
                        for( int weby = node2.y; weby <= node2.y + 3; weby++ ) {
                            add_field( {webx, weby, abs_sub.z}, fd_web, rng( 1, 3 ) );
                        }
                    }
                    place_spawns( GROUP_SPIDER, 1, spawn, spawn, 1, true );
                }
            }
            // TODO: Non-monster hazards?
            // Next, pick a cell to move to
            std::vector<direction> move;
            if( node % 4 > 0 && !node_built[node - 1] ) {
                move.push_back( direction::WEST );
            }
            if( node % 4 < 3 && !node_built[node + 1] ) {
                move.push_back( direction::EAST );
            }
            if( static_cast<int>( node / 4 ) > 0 && !node_built[node - 4] ) {
                move.push_back( direction::NORTH );
            }
            if( static_cast<int>( node / 4 ) < 3 && !node_built[node + 4] ) {
                move.push_back( direction::SOUTH );
            }

            if( move.empty() ) { // Nowhere to go!
                square( this, t_slope_down, node2 + point_south_east, node2 + point( 2, 2 ) );
                done = true;
            } else {
                switch( random_entry( move ) ) {
                    case direction::NORTH:
                        square( this, t_dirt, node2 + point( 1, -2 ), node2 + point( 2, -1 ) );
                        node -= 4;
                        break;
                    case direction::EAST:
                        square( this, t_dirt, node2 + point( 4, 1 ), node2 + point( 5, 2 ) );
                        node++;
                        break;
                    case direction::SOUTH:
                        square( this, t_dirt, node2 + point( 1, 4 ), node2 + point( 2, 5 ) );
                        node += 4;
                        break;
                    case direction::WEST:
                        square( this, t_dirt, node2 + point( -2, 1 ), node2 + point( -1, 2 ) );
                        node--;
                        break;
                    default:
                        break;
                }
            }
        } while( !done );
        square( this, t_slope_up, point( 2, 2 ), point( 3, 3 ) );
        rotate( rng( 0, 3 ) );
    } else if( terrain_type == "triffid_finale" ) {
        fill_background( this, t_root_wall );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        square( this, t_dirt, point( 1, 1 ), point( 4, 4 ) );
        square( this, t_dirt, point( 19, 19 ), point( 22, 22 ) );
        // Drunken walk until we reach the heart (lower right, [19, 19])
        // Chance increases by 1 each turn, and gives the % chance of forcing a move
        // to the right or down.
        int chance = 0;
        int x = 4;
        int y = 4;
        do {
            ter_set( point( x, y ), t_dirt );

            if( chance >= 10 && one_in( 10 ) ) { // Add a spawn
                place_spawns( GROUP_TRIFFID, 1, point( x, y ), point( x, y ), 1, true );
            }

            if( rng( 0, 99 ) < chance ) { // Force movement down or to the right
                if( x >= 19 ) {
                    y++;
                } else if( y >= 19 ) {
                    x++;
                } else {
                    if( one_in( 2 ) ) {
                        x++;
                    } else {
                        y++;
                    }
                }
            } else {
                chance++; // Increase chance of forced movement down/right
                // Weigh movement towards directions with lots of existing walls
                int chance_west = 0;
                int chance_east = 0;
                int chance_north = 0;
                int chance_south = 0;
                for( int dist = 1; dist <= 5; dist++ ) {
                    if( ter( point( x - dist, y ) ) == t_root_wall ) {
                        chance_west++;
                    }
                    if( ter( point( x + dist, y ) ) == t_root_wall ) {
                        chance_east++;
                    }
                    if( ter( point( x, y - dist ) ) == t_root_wall ) {
                        chance_north++;
                    }
                    if( ter( point( x, y + dist ) ) == t_root_wall ) {
                        chance_south++;
                    }
                }
                int roll = rng( 0, chance_west + chance_east + chance_north + chance_south );
                if( roll < chance_west && x > 0 ) {
                    x--;
                } else if( roll < chance_west + chance_east && x < EAST_EDGE ) {
                    x++;
                } else if( roll < chance_west + chance_east + chance_north && y > 0 ) {
                    y--;
                } else if( y < SOUTH_EDGE ) {
                    y++;
                }
            } // Done with drunken walk
        } while( x < 19 || y < 19 );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        square( this, t_slope_up, point( 1, 1 ), point( 2, 2 ) );
        place_spawns( GROUP_TRIFFID_HEART, 1, point( 21, 21 ), point( 21, 21 ), 1, true );

    }
}

void map::draw_connections( const mapgendata &dat )
{
    const oter_id &terrain_type = dat.terrain_type();
    if( is_ot_match( "subway", terrain_type,
                     ot_match_type::type ) ) { // FUUUUU it's IF ELIF ELIF ELIF's mini-me =[
        if( is_ot_match( "sewer", dat.north(), ot_match_type::type ) &&
            !connects_to( terrain_type, 0 ) ) {
            if( connects_to( dat.north(), 2 ) ) {
                for( int i = SEEX - 2; i < SEEX + 2; i++ ) {
                    for( int j = 0; j < SEEY; j++ ) {
                        ter_set( point( i, j ), t_sewage );
                    }
                }
            } else {
                for( int j = 0; j < 3; j++ ) {
                    ter_set( point( SEEX, j ), t_rock_floor );
                    ter_set( point( SEEX - 1, j ), t_rock_floor );
                }
                ter_set( point( SEEX, 3 ), t_door_metal_c );
                ter_set( point( SEEX - 1, 3 ), t_door_metal_c );
            }
        }
        if( is_ot_match( "sewer", dat.east(), ot_match_type::type ) &&
            !connects_to( terrain_type, 1 ) ) {
            if( connects_to( dat.east(), 3 ) ) {
                for( int i = SEEX; i < SEEX * 2; i++ ) {
                    for( int j = SEEY - 2; j < SEEY + 2; j++ ) {
                        ter_set( point( i, j ), t_sewage );
                    }
                }
            } else {
                for( int i = SEEX * 2 - 3; i < SEEX * 2; i++ ) {
                    ter_set( point( i, SEEY ), t_rock_floor );
                    ter_set( point( i, SEEY - 1 ), t_rock_floor );
                }
                ter_set( point( SEEX * 2 - 4, SEEY ), t_door_metal_c );
                ter_set( point( SEEX * 2 - 4, SEEY - 1 ), t_door_metal_c );
            }
        }
        if( is_ot_match( "sewer", dat.south(), ot_match_type::type ) &&
            !connects_to( terrain_type, 2 ) ) {
            if( connects_to( dat.south(), 0 ) ) {
                for( int i = SEEX - 2; i < SEEX + 2; i++ ) {
                    for( int j = SEEY; j < SEEY * 2; j++ ) {
                        ter_set( point( i, j ), t_sewage );
                    }
                }
            } else {
                for( int j = SEEY * 2 - 3; j < SEEY * 2; j++ ) {
                    ter_set( point( SEEX, j ), t_rock_floor );
                    ter_set( point( SEEX - 1, j ), t_rock_floor );
                }
                ter_set( point( SEEX, SEEY * 2 - 4 ), t_door_metal_c );
                ter_set( point( SEEX - 1, SEEY * 2 - 4 ), t_door_metal_c );
            }
        }
        if( is_ot_match( "sewer", dat.west(), ot_match_type::type ) &&
            !connects_to( terrain_type, 3 ) ) {
            if( connects_to( dat.west(), 1 ) ) {
                for( int i = 0; i < SEEX; i++ ) {
                    for( int j = SEEY - 2; j < SEEY + 2; j++ ) {
                        ter_set( point( i, j ), t_sewage );
                    }
                }
            } else {
                for( int i = 0; i < 3; i++ ) {
                    ter_set( point( i, SEEY ), t_rock_floor );
                    ter_set( point( i, SEEY - 1 ), t_rock_floor );
                }
                ter_set( point( 3, SEEY ), t_door_metal_c );
                ter_set( point( 3, SEEY - 1 ), t_door_metal_c );
            }
        }
    } else if( is_ot_match( "sewer", terrain_type, ot_match_type::type ) ) {
        if( dat.above() == "road_nesw_manhole" ) {
            ter_set( point( rng( SEEX - 2, SEEX + 1 ), rng( SEEY - 2, SEEY + 1 ) ), t_ladder_up );
        }
        if( is_ot_match( "subway", dat.north(), ot_match_type::type ) &&
            !connects_to( terrain_type, 0 ) ) {
            for( int j = 0; j < SEEY - 3; j++ ) {
                ter_set( point( SEEX, j ), t_rock_floor );
                ter_set( point( SEEX - 1, j ), t_rock_floor );
            }
            ter_set( point( SEEX, SEEY - 3 ), t_door_metal_c );
            ter_set( point( SEEX - 1, SEEY - 3 ), t_door_metal_c );
        }
        if( is_ot_match( "subway", dat.east(), ot_match_type::type ) &&
            !connects_to( terrain_type, 1 ) ) {
            for( int i = SEEX + 3; i < SEEX * 2; i++ ) {
                ter_set( point( i, SEEY ), t_rock_floor );
                ter_set( point( i, SEEY - 1 ), t_rock_floor );
            }
            ter_set( point( SEEX + 2, SEEY ), t_door_metal_c );
            ter_set( point( SEEX + 2, SEEY - 1 ), t_door_metal_c );
        }
        if( is_ot_match( "subway", dat.south(), ot_match_type::type ) &&
            !connects_to( terrain_type, 2 ) ) {
            for( int j = SEEY + 3; j < SEEY * 2; j++ ) {
                ter_set( point( SEEX, j ), t_rock_floor );
                ter_set( point( SEEX - 1, j ), t_rock_floor );
            }
            ter_set( point( SEEX, SEEY + 2 ), t_door_metal_c );
            ter_set( point( SEEX - 1, SEEY + 2 ), t_door_metal_c );
        }
        if( is_ot_match( "subway", dat.west(), ot_match_type::type ) &&
            !connects_to( terrain_type, 3 ) ) {
            for( int i = 0; i < SEEX - 3; i++ ) {
                ter_set( point( i, SEEY ), t_rock_floor );
                ter_set( point( i, SEEY - 1 ), t_rock_floor );
            }
            ter_set( point( SEEX - 3, SEEY ), t_door_metal_c );
            ter_set( point( SEEX - 3, SEEY - 1 ), t_door_metal_c );
        }
    } else if( is_ot_match( "ants", terrain_type, ot_match_type::type ) ) {
        if( dat.above() == "anthill" ) {
            if( const auto p = random_point( *this, [this]( const tripoint & n ) {
            return ter( n ) == t_rock_floor;
            } ) ) {
                ter_set( *p, t_slope_up );
            }
        }
    }

    // finally, any terrain with SIDEWALKS should contribute sidewalks to neighboring diagonal roads
    if( terrain_type->has_flag( oter_flags::has_sidewalk ) ) {
        for( int dir = 4; dir < 8; dir++ ) { // NE SE SW NW
            bool n_roads_nesw[4] = {};
            int n_num_dirs = terrain_type_to_nesw_array( oter_id( dat.t_nesw[dir] ), n_roads_nesw );
            // only handle diagonal neighbors
            if( n_num_dirs == 2 &&
                n_roads_nesw[( ( dir - 4 ) + 3 ) % 4] &&
                n_roads_nesw[( ( dir - 4 ) + 2 ) % 4] ) {
                // make drawing simpler by rotating the map back and forth
                rotate( 4 - ( dir - 4 ) );
                // draw a small triangle of sidewalk in the northeast corner
                for( int y = 0; y < 4; y++ ) {
                    for( int x = SEEX * 2 - 4; x < SEEX * 2; x++ ) {
                        if( x - y > SEEX * 2 - 4 ) {
                            // TODO: more discriminating conditions
                            if( ter( point( x, y ) ) == t_grass || ter( point( x, y ) ) == t_dirt ||
                                ter( point( x, y ) ) == t_shrub ) {
                                ter_set( point( x, y ), t_sidewalk );
                            }
                        }
                    }
                }
                rotate( ( dir - 4 ) );
            }
        }
    }

    resolve_regional_terrain_and_furniture( dat );
}

void map::place_spawns( const mongroup_id &group, const int chance,
                        const point &p1, const point &p2, const float density,
                        const bool individual, const bool friendly, const std::string &name, const int mission_id )
{
    if( !group.is_valid() ) {
        // TODO: fix point types
        const tripoint_abs_omt omt( sm_to_omt_copy( get_abs_sub() ) );
        const oter_id &oid = overmap_buffer.ter( omt );
        debugmsg( "place_spawns: invalid mongroup '%s', om_terrain = '%s' (%s)", group.c_str(),
                  oid.id().c_str(), oid->get_mapgen_id().c_str() );
        return;
    }

    // Set chance to be 1 or less to guarantee spawn, else set higher than 1
    if( !one_in( chance ) ) {
        return;
    }

    float spawn_density = 1.0f;
    if( MonsterGroupManager::is_animal( group ) ) {
        spawn_density = get_option< float >( "SPAWN_ANIMAL_DENSITY" );
    } else {
        spawn_density = get_option< float >( "SPAWN_DENSITY" );
    }

    float multiplier = density * spawn_density;
    // Only spawn 1 creature if individual flag set, else scale by density
    float thenum = individual ? 1 : ( multiplier * rng_float( 10.0f, 50.0f ) );
    int num = roll_remainder( thenum );

    // GetResultFromGroup decrements num
    while( num > 0 ) {
        int tries = 10;
        point p;

        // Pick a spot for the spawn
        do {
            p.x = rng( p1.x, p2.x );
            p.y = rng( p1.y, p2.y );
            tries--;
        } while( impassable( p ) && tries > 0 );

        // Pick a monster type
        MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup( group, &num );
        add_spawn( spawn_details.name, spawn_details.pack_size, { p, abs_sub.z },
                   friendly, -1, mission_id, name, spawn_details.data );
    }
}

void map::place_gas_pump( const point &p, int charges, const std::string &fuel_type )
{
    item fuel( fuel_type, calendar::start_of_cataclysm );
    fuel.charges = charges;
    add_item( p, fuel );
    ter_set( p, ter_id( fuel.fuel_pump_terrain() ) );
}

void map::place_toilet( const point &p, int charges )
{
    item water( "water", calendar::start_of_cataclysm );
    water.charges = charges;
    add_item( p, water );
    furn_set( p, f_toilet );
}

void map::place_vending( const point &p, const item_group_id &type, bool reinforced )
{
    if( reinforced ) {
        furn_set( p, f_vending_reinforced );
        place_items( type, 100, p, p, false, calendar::start_of_cataclysm );
    } else {
        // The chance to find a non-ransacked vending machine reduces greatly with every day after the cataclysm
        if( !one_in( std::max( to_days<int>( calendar::turn - calendar::start_of_cataclysm ), 0 ) + 4 ) ) {
            furn_set( p, f_vending_o );
            for( const auto &loc : points_in_radius( { p, abs_sub.z }, 1 ) ) {
                if( one_in( 4 ) ) {
                    spawn_item( loc, "glass_shard", rng( 1, 25 ) );
                }
            }
        } else {
            furn_set( p, f_vending_c );
            place_items( type, 100, p, p, false, calendar::start_of_cataclysm );
        }
    }
}

character_id map::place_npc( const point &p, const string_id<npc_template> &type )
{
    shared_ptr_fast<npc> temp = make_shared_fast<npc>();
    temp->normalize();
    temp->load_npc_template( type );
    temp->spawn_at_precise( { abs_sub.xy() }, { p, abs_sub.z } );
    temp->toggle_trait( trait_NPC_STATIC_NPC );
    overmap_buffer.insert_npc( temp );
    return temp->getID();
}

void map::apply_faction_ownership( const point &p1, const point &p2, const faction_id &id )
{
    for( const tripoint &p : points_in_rectangle( tripoint( p1, abs_sub.z ), tripoint( p2,
            abs_sub.z ) ) ) {
        map_stack items = i_at( p.xy() );
        for( item &elem : items ) {
            elem.set_owner( id );
        }
        vehicle *source_veh = veh_pointer_or_null( veh_at( p ) );
        if( source_veh ) {
            if( !source_veh->has_owner() ) {
                source_veh->set_owner( id );
            }
        }
    }
}

// A chance of 100 indicates that items should always spawn,
// the item group should be responsible for determining the amount of items.
std::vector<item *> map::place_items(
    const item_group_id &group_id, const int chance, const tripoint &p1, const tripoint &p2,
    const bool ongrass, const time_point &turn, const int magazine, const int ammo )
{
    std::vector<item *> res;

    if( chance > 100 || chance <= 0 ) {
        debugmsg( "map::place_items() called with an invalid chance (%d)", chance );
        return res;
    }
    if( !item_group::group_is_defined( group_id ) ) {
        // TODO: fix point types
        const tripoint_abs_omt omt( sm_to_omt_copy( get_abs_sub() ) );
        const oter_id &oid = overmap_buffer.ter( omt );
        debugmsg( "place_items: invalid item group '%s', om_terrain = '%s' (%s)",
                  group_id.c_str(), oid.id().c_str(), oid->get_mapgen_id().c_str() );
        return res;
    }

    // spawn rates < 1 are handled in item_group
    const float spawn_rate = std::max( get_option<float>( "ITEM_SPAWNRATE" ), 1.0f ) ;
    const int spawn_count = roll_remainder( chance * spawn_rate / 100.0f );
    for( int i = 0; i < spawn_count; i++ ) {
        // Might contain one item or several that belong together like guns & their ammo
        int tries = 0;
        auto is_valid_terrain = [this, ongrass]( const tripoint & p ) {
            const ter_t &terrain = ter( p ).obj();
            return terrain.movecost == 0           &&
                   !terrain.has_flag( "PLACE_ITEM" ) &&
                   !ongrass                                   &&
                   !terrain.has_flag( "FLAT" );
        };

        tripoint p;
        do {
            p.x = rng( p1.x, p2.x );
            p.y = rng( p1.y, p2.y );
            p.z = rng( p1.z, p2.z );
            tries++;
        } while( is_valid_terrain( p ) && tries < 20 );
        if( tries < 20 ) {
            auto put = put_items_from_loc( group_id, p, turn );
            res.insert( res.end(), put.begin(), put.end() );
        }
    }
    for( item *e : res ) {
        if( e->is_tool() || e->is_gun() || e->is_magazine() ) {
            if( rng( 0, 99 ) < magazine && e->magazine_default() && !e->magazine_integral() &&
                !e->magazine_current() ) {
                e->put_in( item( e->magazine_default(), e->birthday() ), item_pocket::pocket_type::MAGAZINE_WELL );
            }
            if( rng( 0, 99 ) < ammo && e->ammo_default() && e->ammo_remaining() == 0 ) {
                e->ammo_set( e->ammo_default() );
            }
        }
    }
    return res;
}

std::vector<item *> map::put_items_from_loc( const item_group_id &group_id, const tripoint &p,
        const time_point &turn )
{
    const auto items = item_group::items_from( group_id, turn, spawn_flags::use_spawn_rate );
    return spawn_items( p, items );
}

void map::add_spawn( const MonsterGroupResult &spawn_details, const tripoint &p ) const
{
    add_spawn( spawn_details.name, spawn_details.pack_size, p, false, -1, -1, "NONE",
               spawn_details.data );
}

void map::add_spawn( const mtype_id &type, int count, const tripoint &p, bool friendly,
                     int faction_id, int mission_id, const std::string &name ) const
{
    add_spawn( type, count, p, friendly, faction_id, mission_id, name, spawn_data() );
}

void map::add_spawn( const mtype_id &type, int count, const tripoint &p, bool friendly,
                     int faction_id, int mission_id, const std::string &name, const spawn_data &data ) const
{
    if( p.x < 0 || p.x >= SEEX * my_MAPSIZE || p.y < 0 || p.y >= SEEY * my_MAPSIZE ) {
        debugmsg( "Bad add_spawn(%s, %d, %d, %d)", type.c_str(), count, p.x, p.y );
        return;
    }
    point offset;
    submap *place_on_submap = get_submap_at( p, offset );
    if( place_on_submap == nullptr ) {
        debugmsg( "Tried to add spawn at (%d,%d) but the submap is not loaded", offset.x, offset.y );
        return;
    }

    if( !place_on_submap ) {
        debugmsg( "centadodecamonant doesn't exist in grid; within add_spawn(%s, %d, %d, %d, %d)",
                  type.c_str(), count, p.x, p.y, p.z );
        return;
    }
    if( MonsterGroupManager::monster_is_blacklisted( type ) ) {
        return;
    }
    spawn_point tmp( type, count, offset, faction_id, mission_id, friendly, name, data );
    place_on_submap->spawns.push_back( tmp );
}

vehicle *map::add_vehicle( const vgroup_id &type, const tripoint &p, const units::angle &dir,
                           const int veh_fuel, const int veh_status, const bool merge_wrecks )
{
    return add_vehicle( type.obj().pick(), p, dir, veh_fuel, veh_status, merge_wrecks );
}

vehicle *map::add_vehicle( const vgroup_id &type, const point &p, const units::angle &dir,
                           int veh_fuel, int veh_status, bool merge_wrecks )
{
    return add_vehicle( type.obj().pick(), p, dir, veh_fuel, veh_status, merge_wrecks );
}

vehicle *map::add_vehicle( const vproto_id &type, const point &p, const units::angle &dir,
                           int veh_fuel, int veh_status, bool merge_wrecks )
{
    return add_vehicle( type, tripoint( p, abs_sub.z ), dir, veh_fuel, veh_status, merge_wrecks );
}

vehicle *map::add_vehicle( const vproto_id &type, const tripoint &p, const units::angle &dir,
                           const int veh_fuel, const int veh_status, const bool merge_wrecks )
{
    if( !type.is_valid() ) {
        debugmsg( "Nonexistent vehicle type: \"%s\"", type.c_str() );
        return nullptr;
    }
    if( !inbounds( p ) ) {
        dbg( D_WARNING ) << string_format( "Out of bounds add_vehicle t=%s d=%d p=%d,%d,%d",
                                           type.str(), to_degrees( dir ), p.x, p.y, p.z );
        return nullptr;
    }

    // debugmsg("n=%d x=%d y=%d MAPSIZE=%d ^2=%d", nonant, x, y, MAPSIZE, MAPSIZE*MAPSIZE);
    auto veh = std::make_unique<vehicle>( type, veh_fuel, veh_status );
    tripoint p_ms = p;
    veh->sm_pos = ms_to_sm_remain( p_ms );
    veh->pos = p_ms.xy();
    veh->place_spawn_items();
    veh->face.init( dir );
    veh->turn_dir = dir;
    // for backwards compatibility, we always spawn with a pivot point of (0,0) so
    // that the mount at (0,0) is located at the spawn position.
    veh->precalc_mounts( 0, dir, point() );
    //debugmsg("adding veh: %d, sm: %d,%d,%d, pos: %d, %d", veh, veh->smx, veh->smy, veh->smz, veh->posx, veh->posy);
    std::unique_ptr<vehicle> placed_vehicle_up =
        add_vehicle_to_map( std::move( veh ), merge_wrecks );
    vehicle *placed_vehicle = placed_vehicle_up.get();

    if( placed_vehicle != nullptr ) {
        submap *place_on_submap = get_submap_at_grid( placed_vehicle->sm_pos );
        if( place_on_submap == nullptr ) {
            debugmsg( "Tried to add vehicle at (%d,%d,%d) but the submap is not loaded",
                      placed_vehicle->sm_pos.x, placed_vehicle->sm_pos.y, placed_vehicle->sm_pos.z );
            return placed_vehicle;
        }
        place_on_submap->vehicles.push_back( std::move( placed_vehicle_up ) );
        place_on_submap->is_uniform = false;
        invalidate_max_populated_zlev( p.z );

        auto &ch = get_cache( placed_vehicle->sm_pos.z );
        ch.vehicle_list.insert( placed_vehicle );
        add_vehicle_to_cache( placed_vehicle );

        //debugmsg ("grid[%d]->vehicles.size=%d veh.parts.size=%d", nonant, grid[nonant]->vehicles.size(),veh.parts.size());
    }
    return placed_vehicle;
}

/**
 * Takes a vehicle already created with new and attempts to place it on the map,
 * checking for collisions. If the vehicle can't be placed, returns NULL,
 * otherwise returns a pointer to the placed vehicle, which may not necessarily
 * be the one passed in (if wreckage is created by fusing cars).
 * @param veh The vehicle to place on the map.
 * @param merge_wrecks Whether crashed vehicles become part of each other
 * @return The vehicle that was finally placed.
 */
std::unique_ptr<vehicle> map::add_vehicle_to_map(
    std::unique_ptr<vehicle> veh, const bool merge_wrecks )
{
    //We only want to check once per square, so loop over all structural parts
    std::vector<int> frame_indices = veh->all_parts_at_location( "structure" );

    //Check for boat type vehicles that should be placeable in deep water
    const bool can_float = size( veh->get_avail_parts( "FLOATS" ) ) > 2;

    //When hitting a wall, only smash the vehicle once (but walls many times)
    bool needs_smashing = false;

    for( std::vector<int>::const_iterator part = frame_indices.begin();
         part != frame_indices.end(); part++ ) {
        const tripoint p = veh->global_part_pos3( *part );

        //Don't spawn anything in water
        if( has_flag_ter( TFLAG_DEEP_WATER, p ) && !can_float ) {
            return nullptr;
        }

        // Don't spawn shopping carts on top of another vehicle or other obstacle.
        if( veh->type == vproto_id( "shopping_cart" ) ) {
            if( veh_at( p ) || impassable( p ) ) {
                return nullptr;
            }
        }

        //For other vehicles, simulate collisions with (non-shopping cart) stuff
        vehicle *const other_veh = veh_pointer_or_null( veh_at( p ) );
        if( other_veh != nullptr && other_veh->type != vproto_id( "shopping_cart" ) ) {
            if( !merge_wrecks ) {
                return nullptr;
            }

            // Hard wreck-merging limit: 200 tiles
            // Merging is slow for big vehicles which lags the mapgen
            if( frame_indices.size() + other_veh->all_parts_at_location( "structure" ).size() > 200 ) {
                return nullptr;
            }

            /* There's a vehicle here, so let's fuse them together into wreckage and
             * smash them up. It'll look like a nasty collision has occurred.
             * Trying to do a local->global->local conversion would be a major
             * headache, so instead, let's make another vehicle whose (0, 0) point
             * is the (0, 0) of the existing vehicle, convert the coordinates of both
             * vehicles into global coordinates, find the distance between them and
             * p and then install them that way.
             * Create a vehicle with type "null" so it starts out empty. */
            auto wreckage = std::make_unique<vehicle>();
            wreckage->pos = other_veh->pos;
            wreckage->sm_pos = other_veh->sm_pos;

            //Where are we on the global scale?
            const tripoint global_pos = wreckage->global_pos3();

            for( const vpart_reference &vpr : veh->get_all_parts() ) {
                const tripoint part_pos = veh->global_part_pos3( vpr.part() ) - global_pos;
                // TODO: change mount points to be tripoint
                wreckage->install_part( part_pos.xy(), vpr.part() );
            }

            for( const vpart_reference &vpr : other_veh->get_all_parts() ) {
                const tripoint part_pos = other_veh->global_part_pos3( vpr.part() ) - global_pos;
                wreckage->install_part( part_pos.xy(), vpr.part() );

            }

            wreckage->name = _( "Wreckage" );

            // Now get rid of the old vehicles
            std::unique_ptr<vehicle> old_veh = detach_vehicle( other_veh );
            // Failure has happened here when caches are corrupted due to bugs.
            // Add an assertion to avoid null-pointer dereference later.
            cata_assert( old_veh );

            // Try again with the wreckage
            std::unique_ptr<vehicle> new_veh = add_vehicle_to_map( std::move( wreckage ), true );
            if( new_veh != nullptr ) {
                new_veh->smash( *this );
                return new_veh;
            }

            // If adding the wreck failed, we want to restore the vehicle we tried to merge with
            add_vehicle_to_map( std::move( old_veh ), false );
            return nullptr;

        } else if( impassable( p ) ) {
            if( !merge_wrecks ) {
                return nullptr;
            }

            // There's a wall or other obstacle here; destroy it
            destroy( p, true );

            // Some weird terrain, don't place the vehicle
            if( impassable( p ) ) {
                return nullptr;
            }

            needs_smashing = true;
        }
    }

    if( needs_smashing ) {
        veh->smash( *this );
    }

    return veh;
}

computer *map::add_computer( const tripoint &p, const std::string &name, int security )
{
    // TODO: Turn this off?
    furn_set( p, furn_str_id( "f_console" ) );
    point l;
    submap *const place_on_submap = get_submap_at( p, l );
    if( place_on_submap == nullptr ) {
        debugmsg( "Tried to add computer at (%d,%d) but the submap is not loaded", l.x, l.y );
        static computer null_computer = computer( name, security );
        return &null_computer;
    }
    place_on_submap->set_computer( l, computer( name, security ) );
    return place_on_submap->get_computer( l );
}

/**
 * Rotates this map, and all of its contents, by the specified multiple of 90
 * degrees.
 * @param turns How many 90-degree turns to rotate the map.
 */
void map::rotate( int turns, const bool setpos_safe )
{

    //Handle anything outside the 1-3 range gracefully; rotate(0) is a no-op.
    turns = turns % 4;
    if( turns == 0 ) {
        return;
    }

    real_coords rc;
    const tripoint &abs_sub = get_abs_sub();
    rc.fromabs( point( abs_sub.x * SEEX, abs_sub.y * SEEY ) );

    // TODO: This radius can be smaller - how small?
    const int radius = HALF_MAPSIZE + 3;
    // uses submap coordinates
    // TODO: fix point types
    const std::vector<shared_ptr_fast<npc>> npcs =
            overmap_buffer.get_npcs_near( tripoint_abs_sm( abs_sub ), radius );
    for( const shared_ptr_fast<npc> &i : npcs ) {
        npc &np = *i;
        // I know we could break out earlier and waste less cycles, this is just easier.
        // This is here for tinymaps who don't need to rotate NPCs.
        if( skip_npc_rotation() ) {
            break;
        }
        const tripoint sq = np.global_square_location();
        real_coords np_rc;
        np_rc.fromabs( sq.xy() );
        // Note: We are rotating the entire overmap square (2x2 of submaps)
        if( np_rc.om_pos != rc.om_pos || sq.z != abs_sub.z ) {
            continue;
        }

        // OK, this is ugly: we remove the NPC from the whole map
        // Then we place it back from scratch
        // It could be rewritten to utilize the fact that rotation shouldn't cross overmaps

        point old( np_rc.sub_pos );
        if( np_rc.om_sub.x % 2 != 0 ) {
            old.x += SEEX;
        }
        if( np_rc.om_sub.y % 2 != 0 ) {
            old.y += SEEY;
        }

        const point new_pos = old .rotate( turns, { SEEX * 2, SEEY * 2 } );
        if( setpos_safe ) {
            const point local_sq = getlocal( sq ).xy();
            // setpos can't be used during mapgen, but spawn_at_precise clips position
            // to be between 0-11,0-11 and teleports NPCs when used inside of update_mapgen
            // calls
            const tripoint new_global_sq = sq - local_sq + new_pos;
            np.setpos( get_map().getlocal( new_global_sq ) );
        } else {
            // OK, this is ugly: we remove the NPC from the whole map
            // Then we place it back from scratch
            // It could be rewritten to utilize the fact that rotation shouldn't cross overmaps
            shared_ptr_fast<npc> npc_ptr = overmap_buffer.remove_npc( np.getID() );
            np.spawn_at_precise( { abs_sub.xy() }, { new_pos, abs_sub.z } );
            overmap_buffer.insert_npc( npc_ptr );
        }
    }

    clear_vehicle_level_caches();
    clear_vehicle_list( abs_sub.z );

    submap *pz = get_submap_at_grid( point_zero );
    submap *pse = get_submap_at_grid( point_south_east );
    submap *pe = get_submap_at_grid( point_east );
    submap *ps = get_submap_at_grid( point_south );
    if( pz == nullptr || pse == nullptr || pe == nullptr || ps == nullptr ) {
        debugmsg( "Tried to rotate map at (%d,%d) but the submap is not loaded", point_zero.x,
                  point_zero.y );
        return;
    }

    // Move the submaps around.
    if( turns == 2 ) {
        std::swap( *pz, *pse );
        std::swap( *pe, *ps );
    } else {
        point p;
        submap tmp;

        std::swap( *pse, tmp );

        for( int k = 0; k < 4; ++k ) {
            p = p.rotate( turns, { 2, 2 } );
            point tmpp = point_south_east - p;
            submap *psep = get_submap_at_grid( tmpp );
            if( psep == nullptr ) {
                debugmsg( "Tried to rotate map at (%d,%d) but the submap is not loaded", tmpp.x, tmpp.y );
                continue;
            }
            std::swap( *psep, tmp );
        }
    }

    // Then rotate them and recalculate vehicle positions.
    for( int j = 0; j < 2; ++j ) {
        for( int i = 0; i < 2; ++i ) {
            point p( i, j );
            submap *sm = get_submap_at_grid( p );
            if( sm == nullptr ) {
                debugmsg( "Tried to rotate map at (%d,%d) but the submap is not loaded", p.x, p.y );
                continue;
            }

            sm->rotate( turns );

            for( auto &veh : sm->vehicles ) {
                veh->sm_pos = tripoint( p, abs_sub.z );
            }

            update_vehicle_list( sm, abs_sub.z );
        }
    }
    rebuild_vehicle_level_caches();

    // rotate zones
    zone_manager &mgr = zone_manager::get_manager();
    mgr.rotate_zones( *this, turns );
}

// Hideous function, I admit...
bool connects_to( const oter_id &there, int dir )
{
    switch( dir ) {
        // South
        case 2:
            if( there == "sewer_ns"   || there == "sewer_es" || there == "sewer_sw" ||
                there == "sewer_nes"  || there == "sewer_nsw" || there == "sewer_esw" ||
                there == "sewer_nesw" || there == "ants_ns" || there == "ants_es" ||
                there == "ants_sw"    || there == "ants_nes" ||  there == "ants_nsw" ||
                there == "ants_esw"   || there == "ants_nesw" ) {
                return true;
            }
            return false;
        // West
        case 3:
            if( there == "sewer_ew"   || there == "sewer_sw" || there == "sewer_wn" ||
                there == "sewer_new"  || there == "sewer_nsw" || there == "sewer_esw" ||
                there == "sewer_nesw" || there == "ants_ew" || there == "ants_sw" ||
                there == "ants_wn"    || there == "ants_new" || there == "ants_nsw" ||
                there == "ants_esw"   || there == "ants_nesw" ) {
                return true;
            }
            return false;
        // North
        case 0:
            if( there == "sewer_ns"   || there == "sewer_ne" ||  there == "sewer_wn" ||
                there == "sewer_nes"  || there == "sewer_new" || there == "sewer_nsw" ||
                there == "sewer_nesw" || there == "ants_ns" || there == "ants_ne" ||
                there == "ants_wn"    || there == "ants_nes" || there == "ants_new" ||
                there == "ants_nsw"   || there == "ants_nesw" ) {
                return true;
            }
            return false;
        // East
        case 1:
            if( there == "sewer_ew"   || there == "sewer_ne" || there == "sewer_es" ||
                there == "sewer_nes"  || there == "sewer_new" || there == "sewer_esw" ||
                there == "sewer_nesw" || there == "ants_ew" || there == "ants_ne" ||
                there == "ants_es"    || there == "ants_nes" || there == "ants_new" ||
                there == "ants_esw"   || there == "ants_nesw" ) {
                return true;
            }
            return false;
        default:
            debugmsg( "Connects_to with dir of %d", dir );
            return false;
    }
}

void science_room( map *m, const point &p1, const point &p2, int z, int rotate )
{
    int height = p2.y - p1.y;
    int width  = p2.x - p1.x;
    if( rotate % 2 == 1 ) { // Swap width & height if we're a lateral room
        int tmp = height;
        height  = width;
        width   = tmp;
    }
    for( int i = p1.x; i <= p2.x; i++ ) {
        for( int j = p1.y; j <= p2.y; j++ ) {
            m->ter_set( point( i, j ), t_thconc_floor );
        }
    }
    int area = height * width;
    std::vector<room_type> valid_rooms;
    if( height < 5 && width < 5 ) {
        valid_rooms.push_back( room_closet );
    }
    if( height > 6 && width > 3 ) {
        valid_rooms.push_back( room_lobby );
    }
    if( height > 4 || width > 4 ) {
        valid_rooms.push_back( room_chemistry );
        valid_rooms.push_back( room_goo );
    }
    if( z != 0 && ( height > 7 || width > 7 ) && height > 2 && width > 2 ) {
        valid_rooms.push_back( room_teleport );
    }
    if( height > 7 && width > 7 ) {
        valid_rooms.push_back( room_bionics );
        valid_rooms.push_back( room_cloning );
    }
    if( area >= 9 ) {
        valid_rooms.push_back( room_vivisect );
    }
    if( height > 5 && width > 4 ) {
        valid_rooms.push_back( room_dorm );
    }
    if( width > 8 ) {
        for( int i = 8; i < width; i += rng( 1, 2 ) ) {
            valid_rooms.push_back( room_split );
        }
    }

    point trap( rng( p1.x + 1, p2.x - 1 ), rng( p1.y + 1, p2.y - 1 ) );
    switch( random_entry( valid_rooms ) ) {
        case room_closet:
            m->place_items( item_group_id( "cleaning" ), 80, p1, p2, false,
                            calendar::start_of_cataclysm );
            break;
        case room_lobby:
            if( rotate % 2 == 0 ) { // Vertical
                int desk = p1.y + rng( static_cast<int>( height / 2 ) - static_cast<int>( height / 4 ),
                                       static_cast<int>( height / 2 ) + 1 );
                for( int x = p1.x + static_cast<int>( width / 4 ); x < p2.x - static_cast<int>( width / 4 ); x++ ) {
                    m->furn_set( point( x, desk ), f_counter );
                }
                computer *tmpcomp = m->add_computer( tripoint( p2.x - static_cast<int>( width / 4 ), desk, z ),
                                                     _( "Log Console" ), 3 );
                tmpcomp->add_option( _( "View Research Logs" ), COMPACT_RESEARCH, 0 );
                tmpcomp->add_option( _( "Download Map Data" ), COMPACT_MAPS, 0 );
                tmpcomp->add_failure( COMPFAIL_SHUTDOWN );
                tmpcomp->add_failure( COMPFAIL_ALARM );
                tmpcomp->add_failure( COMPFAIL_DAMAGE );
                m->place_spawns( GROUP_LAB_SECURITY, 1,
                                 point( static_cast<int>( ( p1.x + p2.x ) / 2 ), desk ),
                                 point( static_cast<int>( ( p1.x + p2.x ) / 2 ), desk ), 1, true );
            } else {
                int desk = p1.x + rng( static_cast<int>( height / 2 ) - static_cast<int>( height / 4 ),
                                       static_cast<int>( height / 2 ) + 1 );
                for( int y = p1.y + static_cast<int>( width / 4 ); y < p2.y - static_cast<int>( width / 4 ); y++ ) {
                    m->furn_set( point( desk, y ), f_counter );
                }
                computer *tmpcomp = m->add_computer( tripoint( desk, p2.y - static_cast<int>( width / 4 ), z ),
                                                     _( "Log Console" ), 3 );
                tmpcomp->add_option( _( "View Research Logs" ), COMPACT_RESEARCH, 0 );
                tmpcomp->add_option( _( "Download Map Data" ), COMPACT_MAPS, 0 );
                tmpcomp->add_failure( COMPFAIL_SHUTDOWN );
                tmpcomp->add_failure( COMPFAIL_ALARM );
                tmpcomp->add_failure( COMPFAIL_DAMAGE );
                m->place_spawns( GROUP_LAB_SECURITY, 1,
                                 point( desk, static_cast<int>( ( p1.y + p2.y ) / 2 ) ),
                                 point( desk, static_cast<int>( ( p1.y + p2.y ) / 2 ) ), 1, true );
            }
            break;
        case room_chemistry:
            if( rotate % 2 == 0 ) { // Vertical
                for( int x = p1.x; x <= p2.x; x++ ) {
                    if( x % 3 == 0 ) {
                        for( int y = p1.y + 1; y <= p2.y - 1; y++ ) {
                            m->furn_set( point( x, y ), f_counter );
                        }
                        if( one_in( 3 ) ) {
                            m->place_items( item_group_id( "mut_lab" ), 35, point( x, p1.y + 1 ),
                                            point( x, p2.y - 1 ), false,
                                            calendar::start_of_cataclysm );
                        } else {
                            m->place_items( item_group_id( "chem_lab" ), 70, point( x, p1.y + 1 ),
                                            point( x, p2.y - 1 ), false,
                                            calendar::start_of_cataclysm );
                        }
                    }
                }
            } else {
                for( int y = p1.y; y <= p2.y; y++ ) {
                    if( y % 3 == 0 ) {
                        for( int x = p1.x + 1; x <= p2.x - 1; x++ ) {
                            m->furn_set( point( x, y ), f_counter );
                        }
                        if( one_in( 3 ) ) {
                            m->place_items( item_group_id( "mut_lab" ), 35, point( p1.x + 1, y ),
                                            point( p2.x - 1, y ), false,
                                            calendar::start_of_cataclysm );
                        } else {
                            m->place_items( item_group_id( "chem_lab" ), 70, point( p1.x + 1, y ),
                                            point( p2.x - 1, y ), false,
                                            calendar::start_of_cataclysm );
                        }
                    }
                }
            }
            break;
        case room_teleport:
            m->furn_set( point( ( p1.x + p2.x ) / 2, static_cast<int>( ( p1.y + p2.y ) / 2 ) ), f_counter );
            m->furn_set( point( static_cast<int>( ( p1.x + p2.x ) / 2 ) + 1,
                                static_cast<int>( ( p1.y + p2.y ) / 2 ) ),
                         f_counter );
            m->furn_set( point( ( p1.x + p2.x ) / 2, static_cast<int>( ( p1.y + p2.y ) / 2 ) + 1 ),
                         f_counter );
            m->furn_set( point( static_cast<int>( ( p1.x + p2.x ) / 2 ) + 1,
                                static_cast<int>( ( p1.y + p2.y ) / 2 ) + 1 ),
                         f_counter );
            mtrap_set( m, trap, tr_telepad );
            m->place_items( item_group_id( "teleport" ), 70, point( ( p1.x + p2.x ) / 2,
                            static_cast<int>( ( p1.y + p2.y ) / 2 ) ),
                            point( static_cast<int>( ( p1.x + p2.x ) / 2 ) + 1, static_cast<int>( ( p1.y + p2.y ) / 2 ) + 1 ),
                            false,
                            calendar::start_of_cataclysm );
            break;
        case room_goo:
            do {
                mtrap_set( m, trap, tr_goo );
                trap.x = rng( p1.x + 1, p2.x - 1 );
                trap.y = rng( p1.y + 1, p2.y - 1 );
            } while( !one_in( 5 ) );
            if( rotate == 0 ) {
                mremove_trap( m, point( p1.x, p2.y ) );
                m->furn_set( point( p1.x, p2.y ), f_fridge );
                m->place_items( item_group_id( "goo" ), 60, point( p1.x, p2.y ), point( p1.x, p2.y ),
                                false, calendar::start_of_cataclysm );
            } else if( rotate == 1 ) {
                mremove_trap( m, p1 );
                m->furn_set( p1, f_fridge );
                m->place_items( item_group_id( "goo" ), 60, p1, p1, false,
                                calendar::start_of_cataclysm );
            } else if( rotate == 2 ) {
                mremove_trap( m, point( p2.x, p1.y ) );
                m->furn_set( point( p2.x, p1.y ), f_fridge );
                m->place_items( item_group_id( "goo" ), 60, point( p2.x, p1.y ), point( p2.x, p1.y ),
                                false, calendar::start_of_cataclysm );
            } else {
                mremove_trap( m, p2 );
                m->furn_set( p2, f_fridge );
                m->place_items( item_group_id( "goo" ), 60, p2, p2, false,
                                calendar::start_of_cataclysm );
            }
            break;
        case room_cloning:
            for( int x = p1.x + 1; x <= p2.x - 1; x++ ) {
                for( int y = p1.y + 1; y <= p2.y - 1; y++ ) {
                    if( x % 3 == 0 && y % 3 == 0 ) {
                        m->ter_set( point( x, y ), t_vat );
                        m->place_items( item_group_id( "cloning_vat" ), 20, point( x, y ),
                                        point( x, y ), false, calendar::start_of_cataclysm );
                    }
                }
            }
            break;
        case room_vivisect:
            if( rotate == 0 ) {
                for( int x = p1.x; x <= p2.x; x++ ) {
                    m->furn_set( point( x, p2.y - 1 ), f_counter );
                }
                m->place_items( item_group_id( "dissection" ), 80, point( p1.x, p2.y - 1 ),
                                p2 + point_north, false, calendar::start_of_cataclysm );
            } else if( rotate == 1 ) {
                for( int y = p1.y; y <= p2.y; y++ ) {
                    m->furn_set( point( p1.x + 1, y ), f_counter );
                }
                m->place_items( item_group_id( "dissection" ), 80, p1 + point_east,
                                point( p1.x + 1, p2.y ), false, calendar::start_of_cataclysm );
            } else if( rotate == 2 ) {
                for( int x = p1.x; x <= p2.x; x++ ) {
                    m->furn_set( point( x, p1.y + 1 ), f_counter );
                }
                m->place_items( item_group_id( "dissection" ), 80, p1 + point_south,
                                point( p2.x, p1.y + 1 ), false, calendar::start_of_cataclysm );
            } else if( rotate == 3 ) {
                for( int y = p1.y; y <= p2.y; y++ ) {
                    m->furn_set( point( p2.x - 1, y ), f_counter );
                }
                m->place_items( item_group_id( "dissection" ), 80, point( p2.x - 1, p1.y ),
                                p2 + point_west, false, calendar::start_of_cataclysm );
            }
            mtrap_set( m, point( ( p1.x + p2.x ) / 2, static_cast<int>( ( p1.y + p2.y ) / 2 ) ),
                       tr_dissector );
            m->place_spawns( GROUP_LAB_CYBORG, 10,
                             point( static_cast<int>( ( ( p1.x + p2.x ) / 2 ) + 1 ),
                                    static_cast<int>( ( ( p1.y + p2.y ) / 2 ) + 1 ) ),
                             point( static_cast<int>( ( ( p1.x + p2.x ) / 2 ) + 1 ),
                                    static_cast<int>( ( ( p1.y + p2.y ) / 2 ) + 1 ) ), 1, true );
            break;

        case room_bionics:
            if( rotate % 2 == 0 ) {
                int biox = p1.x + 2;
                int bioy = static_cast<int>( ( p1.y + p2.y ) / 2 );
                mapf::formatted_set_simple( m, point( biox - 1, bioy - 1 ),
                                            "---\n"
                                            "|c|\n"
                                            "-=-\n",
                                            mapf::ter_bind( "- | =", t_concrete_wall, t_concrete_wall, t_reinforced_glass ),
                                            mapf::furn_bind( "c", f_counter ) );
                m->place_items( item_group_id( "bionics_common" ), 70, point( biox, bioy ),
                                point( biox, bioy ), false, calendar::start_of_cataclysm );

                m->furn_set( point( biox, bioy + 2 ), furn_str_id( "f_console" ) );
                computer *tmpcomp = m->add_computer( tripoint( biox,  bioy + 2, z ), _( "Bionic access" ), 2 );
                tmpcomp->add_option( _( "Manifest" ), COMPACT_LIST_BIONICS, 0 );
                tmpcomp->add_option( _( "Open Chambers" ), COMPACT_RELEASE_BIONICS, 3 );
                tmpcomp->add_failure( COMPFAIL_MANHACKS );
                tmpcomp->add_failure( COMPFAIL_SECUBOTS );
                tmpcomp->set_access_denied_msg(
                    _( "ERROR!  Access denied!  Unauthorized access will be met with lethal force!" ) );

                biox = p2.x - 2;
                mapf::formatted_set_simple( m, point( biox - 1, bioy - 1 ),
                                            "-=-\n"
                                            "|c|\n"
                                            "---\n",
                                            mapf::ter_bind( "- | =", t_concrete_wall, t_concrete_wall, t_reinforced_glass ),
                                            mapf::furn_bind( "c", f_counter ) );
                m->place_items( item_group_id( "bionics_common" ), 70, point( biox, bioy ),
                                point( biox, bioy ), false, calendar::start_of_cataclysm );

                m->furn_set( point( biox, bioy - 2 ), furn_str_id( "f_console" ) );
                computer *tmpcomp2 = m->add_computer( tripoint( biox,  bioy - 2, z ), _( "Bionic access" ), 2 );
                tmpcomp2->add_option( _( "Manifest" ), COMPACT_LIST_BIONICS, 0 );
                tmpcomp2->add_option( _( "Open Chambers" ), COMPACT_RELEASE_BIONICS, 3 );
                tmpcomp2->add_failure( COMPFAIL_MANHACKS );
                tmpcomp2->add_failure( COMPFAIL_SECUBOTS );
                tmpcomp2->set_access_denied_msg(
                    _( "ERROR!  Access denied!  Unauthorized access will be met with lethal force!" ) );
            } else {
                int bioy = p1.y + 2;
                int biox = static_cast<int>( ( p1.x + p2.x ) / 2 );
                mapf::formatted_set_simple( m, point( biox - 1, bioy - 1 ),
                                            "|-|\n"
                                            "|c=\n"
                                            "|-|\n",
                                            mapf::ter_bind( "- | =", t_concrete_wall, t_concrete_wall, t_reinforced_glass ),
                                            mapf::furn_bind( "c", f_counter ) );
                m->place_items( item_group_id( "bionics_common" ), 70, point( biox, bioy ),
                                point( biox, bioy ), false, calendar::start_of_cataclysm );

                m->furn_set( point( biox + 2, bioy ), furn_str_id( "f_console" ) );
                computer *tmpcomp = m->add_computer( tripoint( biox + 2,  bioy, z ), _( "Bionic access" ), 2 );
                tmpcomp->add_option( _( "Manifest" ), COMPACT_LIST_BIONICS, 0 );
                tmpcomp->add_option( _( "Open Chambers" ), COMPACT_RELEASE_BIONICS, 3 );
                tmpcomp->add_failure( COMPFAIL_MANHACKS );
                tmpcomp->add_failure( COMPFAIL_SECUBOTS );
                tmpcomp->set_access_denied_msg(
                    _( "ERROR!  Access denied!  Unauthorized access will be met with lethal force!" ) );

                bioy = p2.y - 2;
                mapf::formatted_set_simple( m, point( biox - 1, bioy - 1 ),
                                            "|-|\n"
                                            "=c|\n"
                                            "|-|\n",
                                            mapf::ter_bind( "- | =", t_concrete_wall, t_concrete_wall, t_reinforced_glass ),
                                            mapf::furn_bind( "c", f_counter ) );
                m->place_items( item_group_id( "bionics_common" ), 70, point( biox, bioy ),
                                point( biox, bioy ), false, calendar::turn_zero );

                m->furn_set( point( biox - 2, bioy ), furn_str_id( "f_console" ) );
                computer *tmpcomp2 = m->add_computer( tripoint( biox - 2,  bioy, z ), _( "Bionic access" ), 2 );
                tmpcomp2->add_option( _( "Manifest" ), COMPACT_LIST_BIONICS, 0 );
                tmpcomp2->add_option( _( "Open Chambers" ), COMPACT_RELEASE_BIONICS, 3 );
                tmpcomp2->add_failure( COMPFAIL_MANHACKS );
                tmpcomp2->add_failure( COMPFAIL_SECUBOTS );
                tmpcomp2->set_access_denied_msg(
                    _( "ERROR!  Access denied!  Unauthorized access will be met with lethal force!" ) );
            }
            break;
        case room_dorm:
            if( rotate % 2 == 0 ) {
                for( int y = p1.y + 1; y <= p2.y - 1; y += 3 ) {
                    m->furn_set( point( p1.x, y ), f_bed );
                    m->place_items( item_group_id( "bed" ), 60, point( p1.x, y ), point( p1.x, y ),
                                    false, calendar::start_of_cataclysm );
                    m->furn_set( point( p1.x + 1, y ), f_bed );
                    m->place_items( item_group_id( "bed" ), 60, point( p1.x + 1, y ),
                                    point( p1.x + 1, y ), false, calendar::start_of_cataclysm );
                    m->furn_set( point( p2.x, y ), f_bed );
                    m->place_items( item_group_id( "bed" ), 60, point( p2.x, y ), point( p2.x, y ),
                                    false, calendar::start_of_cataclysm );
                    m->furn_set( point( p2.x - 1, y ), f_bed );
                    m->place_items( item_group_id( "bed" ), 60, point( p2.x - 1, y ),
                                    point( p2.x - 1, y ), false, calendar::start_of_cataclysm );
                    m->furn_set( point( p1.x, y + 1 ), f_dresser );
                    m->furn_set( point( p2.x, y + 1 ), f_dresser );
                    m->place_items( item_group_id( "dresser" ), 70, point( p1.x, y + 1 ),
                                    point( p1.x, y + 1 ), false, calendar::start_of_cataclysm );
                    m->place_items( item_group_id( "dresser" ), 70, point( p2.x, y + 1 ),
                                    point( p2.x, y + 1 ), false, calendar::start_of_cataclysm );
                }
            } else if( rotate % 2 == 1 ) {
                for( int x = p1.x + 1; x <= p2.x - 1; x += 3 ) {
                    m->furn_set( point( x, p1.y ), f_bed );
                    m->place_items( item_group_id( "bed" ), 60, point( x, p1.y ), point( x, p1.y ),
                                    false, calendar::start_of_cataclysm );
                    m->furn_set( point( x, p1.y + 1 ), f_bed );
                    m->place_items( item_group_id( "bed" ), 60, point( x, p1.y + 1 ),
                                    point( x, p1.y + 1 ), false, calendar::start_of_cataclysm );
                    m->furn_set( point( x, p2.y ), f_bed );
                    m->place_items( item_group_id( "bed" ), 60, point( x, p2.y ), point( x, p2.y ),
                                    false, calendar::start_of_cataclysm );
                    m->furn_set( point( x, p2.y - 1 ), f_bed );
                    m->place_items( item_group_id( "bed" ), 60, point( x, p2.y - 1 ),
                                    point( x, p2.y - 1 ), false, calendar::start_of_cataclysm );
                    m->furn_set( point( x + 1, p1.y ), f_dresser );
                    m->furn_set( point( x + 1, p2.y ), f_dresser );
                    m->place_items( item_group_id( "dresser" ), 70, point( x + 1, p1.y ),
                                    point( x + 1, p1.y ), false, calendar::start_of_cataclysm );
                    m->place_items( item_group_id( "dresser" ), 70, point( x + 1, p2.y ),
                                    point( x + 1, p2.y ), false, calendar::start_of_cataclysm );
                }
            }
            m->place_items( item_group_id( "lab_dorm" ), 84, p1, p2, false,
                            calendar::start_of_cataclysm );
            break;
        case room_split:
            if( rotate % 2 == 0 ) {
                int w1 = static_cast<int>( ( p1.x + p2.x ) / 2 ) - 2;
                int w2 = static_cast<int>( ( p1.x + p2.x ) / 2 ) + 2;
                for( int y = p1.y; y <= p2.y; y++ ) {
                    m->ter_set( point( w1, y ), t_concrete_wall );
                    m->ter_set( point( w2, y ), t_concrete_wall );
                }
                m->ter_set( point( w1, static_cast<int>( ( p1.y + p2.y ) / 2 ) ), t_door_glass_frosted_c );
                m->ter_set( point( w2, static_cast<int>( ( p1.y + p2.y ) / 2 ) ), t_door_glass_frosted_c );
                science_room( m, p1, point( w1 - 1, p2.y ), z, 1 );
                science_room( m, point( w2 + 1, p1.y ), p2, z, 3 );
            } else {
                int w1 = static_cast<int>( ( p1.y + p2.y ) / 2 ) - 2;
                int w2 = static_cast<int>( ( p1.y + p2.y ) / 2 ) + 2;
                for( int x = p1.x; x <= p2.x; x++ ) {
                    m->ter_set( point( x, w1 ), t_concrete_wall );
                    m->ter_set( point( x, w2 ), t_concrete_wall );
                }
                m->ter_set( point( ( p1.x + p2.x ) / 2, w1 ), t_door_glass_frosted_c );
                m->ter_set( point( ( p1.x + p2.x ) / 2, w2 ), t_door_glass_frosted_c );
                science_room( m, p1, point( p2.x, w1 - 1 ), z, 2 );
                science_room( m, point( p1.x, w2 + 1 ), p2, z, 0 );
            }
            break;
        default:
            break;
    }
}

void map::create_anomaly( const tripoint &cp, artifact_natural_property prop, bool create_rubble )
{
    // TODO: Z
    point c( cp.xy() );
    if( create_rubble ) {
        rough_circle( this, t_dirt, c, 11 );
        rough_circle_furn( this, f_rubble, c, 5 );
        furn_set( c, f_null );
    }
    switch( prop ) {
        case ARTPROP_WRIGGLING:
        case ARTPROP_MOVING:
            for( int i = c.x - 5; i <= c.x + 5; i++ ) {
                for( int j = c.y - 5; j <= c.y + 5; j++ ) {
                    if( furn( point( i, j ) ) == f_rubble ) {
                        add_field( {i, j, abs_sub.z}, fd_push_items, 1 );
                        if( one_in( 3 ) ) {
                            spawn_item( point( i, j ), "rock" );
                        }
                    }
                }
            }
            break;

        case ARTPROP_GLOWING:
        case ARTPROP_GLITTERING:
            for( int i = c.x - 5; i <= c.x + 5; i++ ) {
                for( int j = c.y - 5; j <= c.y + 5; j++ ) {
                    if( furn( point( i, j ) ) == f_rubble && one_in( 2 ) ) {
                        mtrap_set( this, point( i, j ), tr_glow );
                    }
                }
            }
            break;

        case ARTPROP_HUMMING:
        case ARTPROP_RATTLING:
            for( int i = c.x - 5; i <= c.x + 5; i++ ) {
                for( int j = c.y - 5; j <= c.y + 5; j++ ) {
                    if( furn( point( i, j ) ) == f_rubble && one_in( 2 ) ) {
                        mtrap_set( this, point( i, j ), tr_hum );
                    }
                }
            }
            break;

        case ARTPROP_WHISPERING:
        case ARTPROP_ENGRAVED:
            for( int i = c.x - 5; i <= c.x + 5; i++ ) {
                for( int j = c.y - 5; j <= c.y + 5; j++ ) {
                    if( furn( point( i, j ) ) == f_rubble && one_in( 3 ) ) {
                        mtrap_set( this, point( i, j ), tr_shadow );
                    }
                }
            }
            break;

        case ARTPROP_BREATHING:
            for( int i = c.x - 1; i <= c.x + 1; i++ ) {
                for( int j = c.y - 1; j <= c.y + 1; j++ ) {
                    if( i == c.x && j == c.y ) {
                        place_spawns( GROUP_BREATHER_HUB, 1, point( i, j ), point( i, j ), 1,
                                      true );
                    } else {
                        place_spawns( GROUP_BREATHER, 1, point( i, j ), point( i, j ), 1, true );
                    }
                }
            }
            break;

        case ARTPROP_DEAD:
            for( int i = c.x - 5; i <= c.x + 5; i++ ) {
                for( int j = c.y - 5; j <= c.y + 5; j++ ) {
                    if( furn( point( i, j ) ) == f_rubble ) {
                        mtrap_set( this, point( i, j ), tr_drain );
                    }
                }
            }
            break;

        case ARTPROP_ITCHY:
            for( int i = c.x - 5; i <= c.x + 5; i++ ) {
                for( int j = c.y - 5; j <= c.y + 5; j++ ) {
                    if( furn( point( i, j ) ) == f_rubble ) {
                        set_radiation( point( i, j ), rng( 0, 10 ) );
                    }
                }
            }
            break;

        case ARTPROP_ELECTRIC:
        case ARTPROP_CRACKLING:
            add_field( {c, abs_sub.z}, fd_shock_vent, 3 );
            break;

        case ARTPROP_SLIMY:
            add_field( {c, abs_sub.z}, fd_acid_vent, 3 );
            break;

        case ARTPROP_WARM:
            for( int i = c.x - 5; i <= c.x + 5; i++ ) {
                for( int j = c.y - 5; j <= c.y + 5; j++ ) {
                    if( furn( point( i, j ) ) == f_rubble ) {
                        add_field( {i, j, abs_sub.z}, fd_fire_vent, 1 + ( rl_dist( c, point( i, j ) ) % 3 ) );
                    }
                }
            }
            break;

        case ARTPROP_SCALED:
            for( int i = c.x - 5; i <= c.x + 5; i++ ) {
                for( int j = c.y - 5; j <= c.y + 5; j++ ) {
                    if( furn( point( i, j ) ) == f_rubble ) {
                        mtrap_set( this, point( i, j ), tr_snake );
                    }
                }
            }
            break;

        case ARTPROP_FRACTAL:
            create_anomaly( c + point( -4, -4 ),
                            static_cast<artifact_natural_property>( rng( ARTPROP_NULL + 1, ARTPROP_MAX - 1 ) ) );
            create_anomaly( c + point( 4, -4 ),
                            static_cast<artifact_natural_property>( rng( ARTPROP_NULL + 1, ARTPROP_MAX - 1 ) ) );
            create_anomaly( c + point( -4, 4 ),
                            static_cast<artifact_natural_property>( rng( ARTPROP_NULL + 1, ARTPROP_MAX - 1 ) ) );
            create_anomaly( c + point( 4, -4 ),
                            static_cast<artifact_natural_property>( rng( ARTPROP_NULL + 1, ARTPROP_MAX - 1 ) ) );
            break;
        default:
            break;
    }
}
///////////////////// part of map

void line( map *m, const ter_id &type, const point &p1, const point &p2 )
{
    m->draw_line_ter( type, p1, p2 );
}
void line_furn( map *m, const furn_id &type, const point &p1, const point &p2 )
{
    m->draw_line_furn( type, p1, p2 );
}
void fill_background( map *m, const ter_id &type )
{
    m->draw_fill_background( type );
}
void fill_background( map *m, ter_id( *f )() )
{
    m->draw_fill_background( f );
}
void square( map *m, const ter_id &type, const point &p1, const point &p2 )
{
    m->draw_square_ter( type, p1, p2 );
}
void square_furn( map *m, const furn_id &type, const point &p1, const point &p2 )
{
    m->draw_square_furn( type, p1, p2 );
}
void square( map *m, ter_id( *f )(), const point &p1, const point &p2 )
{
    m->draw_square_ter( f, p1, p2 );
}
void square( map *m, const weighted_int_list<ter_id> &f, const point &p1, const point &p2 )
{
    m->draw_square_ter( f, p1, p2 );
}
void rough_circle( map *m, const ter_id &type, const point &p, int rad )
{
    m->draw_rough_circle_ter( type, p, rad );
}
void rough_circle_furn( map *m, const furn_id &type, const point &p, int rad )
{
    m->draw_rough_circle_furn( type, p, rad );
}
void circle( map *m, const ter_id &type, double x, double y, double rad )
{
    m->draw_circle_ter( type, rl_vec2d( x, y ), rad );
}
void circle( map *m, const ter_id &type, const point &p, int rad )
{
    m->draw_circle_ter( type, p, rad );
}
void circle_furn( map *m, const furn_id &type, const point &p, int rad )
{
    m->draw_circle_furn( type, p, rad );
}
void add_corpse( map *m, const point &p )
{
    m->add_corpse( tripoint( p, m->get_abs_sub().z ) );
}

//////////////////// mapgen update
update_mapgen_function_json::update_mapgen_function_json(
    const json_source_location &jsrcloc, const std::string &context ) :
    mapgen_function_json_base( jsrcloc, context )
{
}

void update_mapgen_function_json::check() const
{
    check_common();
}

bool update_mapgen_function_json::setup_update( const JsonObject &jo )
{
    return setup_common( jo );
}

bool update_mapgen_function_json::setup_internal( const JsonObject &/*jo*/ )
{
    fill_ter = t_null;
    /* update_mapgen doesn't care about fill_ter or rows */
    return true;
}

bool update_mapgen_function_json::update_map( const tripoint_abs_omt &omt_pos, const point &offset,
        mission *miss, bool verify ) const
{
    if( omt_pos == overmap::invalid_tripoint ) {
        debugmsg( "Mapgen update function called with overmap::invalid_tripoint" );
        return false;
    }
    tinymap update_tmap;
    const tripoint_abs_sm sm_pos = project_to<coords::sm>( omt_pos );
    update_tmap.load( sm_pos, true );

    mapgendata md( omt_pos, update_tmap, 0.0f, calendar::start_of_cataclysm, miss );

    return update_map( md, offset, verify );
}

bool update_mapgen_function_json::update_map( const mapgendata &md, const point &offset,
        const bool verify ) const
{
    class rotation_guard
    {
        public:
            explicit rotation_guard( const mapgendata &md )
                : md( md ), rotation( oter_get_rotation( md.terrain_type() ) ) {
                // If the existing map is rotated, we need to rotate it back to the north
                // orientation before applying our updates.
                if( rotation != 0 ) {
                    md.m.rotate( rotation, true );
                }
            }

            ~rotation_guard() {
                // If we rotated the map before applying updates, we now need to rotate
                // it back to where we found it.
                if( rotation != 0 ) {
                    md.m.rotate( 4 - rotation, true );
                }
            }
        private:
            const mapgendata &md;
            const int rotation;
    };
    rotation_guard rot( md );

    for( const jmapgen_setmap &elem : setmap_points ) {
        if( verify && elem.has_vehicle_collision( md, offset ) ) {
            return false;
        }
        elem.apply( md, offset );
    }

    if( verify && objects.has_vehicle_collision( md, offset ) ) {
        return false;
    }
    objects.apply( md, offset );

    resolve_regional_terrain_and_furniture( md );

    return true;
}

mapgen_update_func add_mapgen_update_func( const JsonObject &jo, bool &defer )
{
    if( jo.has_string( "mapgen_update_id" ) ) {
        const std::string mapgen_update_id = jo.get_string( "mapgen_update_id" );
        const auto update_function = [mapgen_update_id]( const tripoint_abs_omt & omt_pos,
        mission * miss ) {
            run_mapgen_update_func( mapgen_update_id, omt_pos, miss, false );
        };
        return update_function;
    }

    update_mapgen_function_json json_data( json_source_location {},
                                           "unknown object in add_mapgen_update_func" );
    mapgen_defer::defer = defer;
    if( !json_data.setup_update( jo ) ) {
        const auto null_function = []( const tripoint_abs_omt &, mission * ) {
        };
        return null_function;
    }
    const auto update_function = [json_data]( const tripoint_abs_omt & omt_pos, mission * miss ) {
        json_data.update_map( omt_pos, point_zero, miss );
    };
    defer = mapgen_defer::defer;
    mapgen_defer::jsi = JsonObject();
    return update_function;
}

bool run_mapgen_update_func( const std::string &update_mapgen_id, const tripoint_abs_omt &omt_pos,
                             mission *miss, bool cancel_on_collision )
{
    const auto update_function = update_mapgen.find( update_mapgen_id );

    if( update_function == update_mapgen.end() || update_function->second.empty() ) {
        return false;
    }
    return update_function->second[0]->update_map( omt_pos, point_zero, miss, cancel_on_collision );
}

bool run_mapgen_update_func( const std::string &update_mapgen_id, mapgendata &dat,
                             const bool cancel_on_collision )
{
    const auto update_function = update_mapgen.find( update_mapgen_id );
    if( update_function == update_mapgen.end() || update_function->second.empty() ) {
        return false;
    }
    return update_function->second[0]->update_map( dat, point_zero, cancel_on_collision );
}

std::pair<std::map<ter_id, int>, std::map<furn_id, int>> get_changed_ids_from_update(
            const std::string &update_mapgen_id )
{
    const int fake_map_z = -9;

    std::map<ter_id, int> terrains;
    std::map<furn_id, int> furnitures;

    const auto update_function = update_mapgen.find( update_mapgen_id );

    if( update_function == update_mapgen.end() || update_function->second.empty() ) {
        return std::make_pair( terrains, furnitures );
    }

    ::fake_map fake_map( f_null, t_dirt, tr_null, fake_map_z );

    oter_id any = oter_id( "field" );
    // just need a variable here, it doesn't need to be valid
    const regional_settings dummy_settings;

    mapgendata fake_md( any, any, any, any, any, any, any, any,
                        any, any, 0, dummy_settings, fake_map, any, 0.0f, calendar::turn, nullptr );

    if( update_function->second[0]->update_map( fake_md ) ) {
        for( const tripoint &pos : fake_map.points_on_zlevel( fake_map_z ) ) {
            ter_id ter_at_pos = fake_map.ter( pos );
            if( ter_at_pos != t_dirt ) {
                terrains[ter_at_pos] += 1;
            }
            if( fake_map.has_furn( pos ) ) {
                furn_id furn_at_pos = fake_map.furn( pos );
                furnitures[furn_at_pos] += 1;
            }
        }
    }
    return std::make_pair( terrains, furnitures );
}

bool run_mapgen_func( const std::string &mapgen_id, mapgendata &dat )
{
    return oter_mapgen.generate( dat, mapgen_id );
}

int register_mapgen_function( const std::string &key )
{
    if( const building_gen_pointer ptr = get_mapgen_cfunction( key ) ) {
        return oter_mapgen.add( key, std::make_shared<mapgen_function_builtin>( ptr ) );
    }
    return -1;
}

bool has_mapgen_for( const std::string &key )
{
    return oter_mapgen.has( key );
}

bool has_update_mapgen_for( const std::string &key )
{
    return update_mapgen.count( key );
}
