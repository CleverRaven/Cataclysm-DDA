#include "map.h"
#include "omdata.h"
#include "output.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "debug.h"
#include "options.h"
#include "item_group.h"
#include "mapgen_functions.h"
#include "mapgenformat.h"
#include "overmapbuffer.h"
#include "enums.h"
#include "monstergenerator.h"
#include "mongroup.h"
#include "mapgen.h"
#include <algorithm>
#include <cassert>
#include <list>
#include <sstream>
#include "json.h"
#ifdef LUA
#include "catalua.h"
#endif

#ifndef sgn
#define sgn(x) (((x) < 0) ? -1 : 1)
#endif

#define dbg(x) DebugLog((DebugLevel)(x),D_MAP_GEN) << __FILE__ << ":" << __LINE__ << ": "

#define MON_RADIUS 3

bool connects_to(oter_id there, int dir_from_here);
void science_room(map *m, int x1, int y1, int x2, int y2, int z, int rotate);
void set_science_room(map *m, int x1, int y1, bool faces_right, int turn);
map_extra random_map_extra(map_extras);

room_type pick_mansion_room(int x1, int y1, int x2, int y2);
void build_mansion_room(map *m, room_type type, int x1, int y1, int x2, int y2, mapgendata & dat);
void mansion_room(map *m, int x1, int y1, int x2, int y2, mapgendata & dat); // pick & build

// (x,y,z) are absolute coordinates of a submap
// x%2 and y%2 must be 0!
void map::generate(const int x, const int y, const int z, const int turn)
{
    dbg(D_INFO) << "map::generate( g[" << g << "], x[" << x << "], "
                << "y[" << y << "], z[" << z <<"], turn[" << turn << "] )";

    set_abs_sub( x, y, z );

    // First we have to create new submaps and initialize them to 0 all over
    // We create all the submaps, even if we're not a tinymap, so that map
    //  generation which overflows won't cause a crash.  At the bottom of this
    //  function, we save the upper-left 4 submaps, and delete the rest.
    // Mapgen is not z-level aware yet. Only actually initialize current z-level
    //  because other submaps won't be touched.
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            setsubmap( get_nonant( gridx, gridy ), new submap() );
            // TODO: memory leak if the code below throws before the submaps get stored/deleted!
        }
    }

    unsigned zones = 0;
    // x, and y are submap coordinates, convert to overmap terrain coordinates
    int overx = x;
    int overy = y;
    overmapbuffer::sm_to_omt(overx, overy);
    const regional_settings *rsettings = &overmap_buffer.get_settings(overx, overy, z);
    oter_id t_above = overmap_buffer.ter(overx, overy, z + 1);
    oter_id terrain_type = overmap_buffer.ter(overx, overy, z);
    oter_id t_north = overmap_buffer.ter(overx, overy - 1, z);
    oter_id t_neast = overmap_buffer.ter(overx + 1, overy - 1, z);
    oter_id t_east = overmap_buffer.ter(overx + 1, overy, z);
    oter_id t_seast = overmap_buffer.ter(overx + 1, overy + 1, z);
    oter_id t_south = overmap_buffer.ter(overx, overy + 1, z);
    oter_id t_nwest = overmap_buffer.ter(overx - 1, overy - 1, z);
    oter_id t_west = overmap_buffer.ter(overx - 1, overy, z);
    oter_id t_swest = overmap_buffer.ter(overx - 1, overy + 1, z);

    // This attempts to scale density of zombies inversely with distance from the nearest city.
    // In other words, make city centers dense and perimiters sparse.
    float density = 0.0;
    for (int i = overx - MON_RADIUS; i <= overx + MON_RADIUS; i++) {
        for (int j = overy - MON_RADIUS; j <= overy + MON_RADIUS; j++) {
            density += otermap[overmap_buffer.ter(i, j, z)].mondensity;
        }
    }
    density = density / 100;

    draw_map(terrain_type, t_north, t_east, t_south, t_west, t_neast, t_seast, t_nwest, t_swest,
             t_above, turn, density, z, rsettings);

    map_extras ex = get_extras(otermap[terrain_type].extras);
    if ( one_in( ex.chance )) {
        add_extra( random_map_extra( ex ));
    }

    const overmap_spawns &spawns = terrain_type.t().static_spawns;
    if( spawns.group != "GROUP_NULL" && x_in_y( spawns.chance, 100 ) ) {
        int pop = rng( spawns.min_population, spawns.max_population );
        // place_spawns currently depends on the STATIC_SPAWN world option, this
        // must bypass it.
        for( ; pop > 0; pop-- ) {
            MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup( spawns.group, &pop );
            if( spawn_details.name == "mon_null" ) {
                continue;
            }
            int tries = 10;
            int monx = 0;
            int mony = 0;
            do {
                monx = rng( 0, SEEX * 2 - 1 );
                mony = rng( 0, SEEY * 2 - 1 );
                tries--;
            } while( move_cost( monx, mony ) == 0 && tries > 0 );
            if( tries > 0 ) {
                add_spawn( spawn_details.name, spawn_details.pack_size, monx, mony );
            }
        }
    }

    post_process(zones);

    // Okay, we know who are neighbors are.  Let's draw!
    // And finally save used submaps and delete the rest.
    for (int i = 0; i < my_MAPSIZE; i++) {
        for (int j = 0; j < my_MAPSIZE; j++) {
            dbg(D_INFO) << "map::generate: submap (" << i << "," << j << ")";

            if( i <= 1 && j <= 1 ) {
                saven( i, j, z );
            } else {
                delete get_submap_at_grid( i, j, z );
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
///// mapgen_function class.
///// all sorts of ways to apply our hellish reality to a grid-o-squares

/*
 * ptr storage.
 */
std::map<std::string, std::vector<mapgen_function*> > oter_mapgen;

/*
 * index to the above, adjusted to allow for rarity
 */
std::map<std::string, std::map<int, int> > oter_mapgen_weights;

/*
 * setup oter_mapgen_weights which which mapgen uses to diceroll. Also setup mapgen_function_json
 */
void calculate_mapgen_weights() { // todo; rename as it runs jsonfunction setup too
    oter_mapgen_weights.clear();
    for( std::map<std::string, std::vector<mapgen_function*> >::const_iterator oit = oter_mapgen.begin(); oit != oter_mapgen.end(); ++oit ) {
        int funcnum = 0;
        int wtotal = 0;
        oter_mapgen_weights[ oit->first ] = std::map<int, int>();
        for( std::vector<mapgen_function*>::const_iterator fit = oit->second.begin(); fit != oit->second.end(); ++fit ) {
            //
            int weight = (*fit)->weight;
            if ( weight < 1 ) {
                dbg(D_INFO) << "wcalc " << oit->first << "(" << funcnum << "): (rej(1), " << weight << ") = " << wtotal;
                funcnum++;
                continue; // rejected!
            }
            if ( ! (*fit)->setup() ) {
                dbg(D_INFO) << "wcalc " << oit->first << "(" << funcnum << "): (rej(2), " << weight << ") = " << wtotal;
                funcnum++;
                continue; // disqualify! doesn't get to play in the pool
            }
            //
            wtotal += weight;
            oter_mapgen_weights[ oit->first ][ wtotal ] = funcnum;
            dbg(D_INFO) << "wcalc " << oit->first << "(" << funcnum << "): +" << weight << " = " << wtotal;
            funcnum++;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
///// json mapgen functions
///// 1 - init():

/*
 * load a single mapgen json structure; this can be inside an overmap_terrain, or on it's own.
 */

mapgen_function * load_mapgen_function(JsonObject &jio, const std::string id_base, int default_idx) {
    int mgweight = jio.get_int("weight", 1000);
    mapgen_function * ret = NULL;
    if ( mgweight <= 0 || jio.get_bool("disabled", false) ) {
        const std::string mgtype = jio.get_string("method");
        if ( default_idx != -1 && mgtype == "builtin" ) {
            if ( jio.has_string("name") ) {
                const std::string mgname = jio.get_string("name");
                if ( mgname == id_base ) {
                    oter_mapgen[id_base][ default_idx ]->weight = 0;
                }
            }
        }
        return NULL; // nothing
    } else if ( jio.has_string("method") ) {
        const std::string mgtype = jio.get_string("method");
        if ( mgtype == "builtin" ) { // c-function
            if ( jio.has_string("name") ) {
                const std::string mgname = jio.get_string("name");
                const auto iter = mapgen_cfunction_map.find( mgname );
                if( iter != mapgen_cfunction_map.end() ) {
                    ret = new mapgen_function_builtin( iter->second, mgweight );
                    oter_mapgen[id_base].push_back( ret ); //new mapgen_function_builtin( mgname, mgweight ) );
                } else {
                    debugmsg("oter_t[%s]: builtin mapgen function \"%s\" does not exist.", id_base.c_str(), mgname.c_str() );
                }
            } else {
                debugmsg("oter_t[%s]: Invalid mapgen function (missing \"name\" value).", id_base.c_str(), mgtype.c_str() );
            }
        } else if ( mgtype == "lua" ) { // lua script
#ifdef LUA
            if ( jio.has_string("script") ) { // minified into one\nline
                const std::string mgscript = jio.get_string("script");
                ret = new mapgen_function_lua( mgscript, mgweight );
                oter_mapgen[id_base].push_back( ret ); //new mapgen_function_lua( mgscript, mgweight ) );
            } else if ( jio.has_array("script") ) { // or 1 line per entry array
                std::string mgscript = "";
                JsonArray jascr = jio.get_array("script");
                while ( jascr.has_more() ) {
                    mgscript += jascr.next_string();
                    mgscript += "\n";
                }
                ret = new mapgen_function_lua( mgscript, mgweight );
                oter_mapgen[id_base].push_back( ret ); //new mapgen_function_lua( mgscript, mgweight ) );
            // todo; pass dirname current.json, because the latter two are icky
            // } else if ( jio.has_string("file" ) { // or "same-dir-as-this/json/something.lua
            } else {
                debugmsg("oter_t[%s]: Invalid mapgen function (missing \"script\" or \"file\" value).", id_base.c_str() );
            }
#else
            debugmsg("oter_t[%s]: mapgen entry requires a build with LUA=1.",id_base.c_str() );
#endif
        } else if ( mgtype == "json" ) {
            if ( jio.has_object("object") ) {
                JsonObject jo = jio.get_object("object");
                std::string jstr = jo.str();
                ret = new mapgen_function_json( jstr, mgweight );
                oter_mapgen[id_base].push_back( ret ); //new mapgen_function_json( jstr ) );
            } else {
                debugmsg("oter_t[%s]: Invalid mapgen function (missing \"object\" object)", id_base.c_str() );
            }
        } else {
            debugmsg("oter_t[%s]: Invalid mapgen function type: %s", id_base.c_str(), mgtype.c_str() );
        }
    } else {
        debugmsg("oter_t[%s]: Invalid mapgen function (missing \"method\" value, must be \"builtin\", \"lua\", or \"json\").", id_base.c_str() );
    }
    return ret;
}

/*
 * feed bits `o json from standalone file to load_mapgen_function. (standalone json "type": "mapgen")
 */
void load_mapgen( JsonObject &jo ) {
    if ( jo.has_array( "om_terrain" ) ) {
        std::vector<std::string> mapgenid_list;
        JsonArray ja = jo.get_array( "om_terrain" );
        while( ja.has_more() ) {
            mapgenid_list.push_back( ja.next_string() );
        }
        if ( !mapgenid_list.empty() ) {
            std::string mapgenid = mapgenid_list[0];
            mapgen_function * mgfunc = load_mapgen_function(jo, mapgenid, -1);
            if ( mgfunc != NULL ) {
               for( auto &i : mapgenid_list) {
                   oter_mapgen[ i ].push_back( mgfunc );
               }
            }
        } else {

        }
    } else if ( jo.has_string( "om_terrain" ) ) {
        load_mapgen_function(jo, jo.get_string("om_terrain"), -1);
    } else {
        debugmsg("mapgen entry requires \"om_terrain\": \"something\", or \"om_terrain\": [ \"list\", \"of\" \"somethings\" ]\n%s\n", jo.str().c_str() );
    }
}

void reset_mapgens()
{
    // Because I don't know where that pointer is stored
    // might be at multiple locations, but we must only delete it once!
    typedef std::set<mapgen_function*> xset;
    xset s;
    for( auto &elem : oter_mapgen ) {
        for( auto &_b : elem.second ) {
            s.insert( _b );
        }
    }
    for( const auto &elem : s ) {
        delete elem;
    }
    oter_mapgen.clear();
}

/////////////////////////////////////////////////////////////////////////////////
///// 2 - right after init() finishes parsing all game json and terrain info/etc is set..
/////   ...parse more json! (mapgen_function_json)

size_t mapgen_function_json::calc_index( const size_t x, const size_t y) const
{
    if( x >= mapgensize ) {
        debugmsg( "invalid value %d for x in mapgen_function_json::calc_index", x );
    }
    if( y >= mapgensize ) {
        debugmsg( "invalid value %d for y in mapgen_function_json::calc_index", y );
    }
    return y * mapgensize + x;
}

bool mapgen_function_json::check_inbounds( const jmapgen_int & var ) const {
    const int min = 0;
    const int max = mapgensize - 1;
    if ( var.val < min || var.val > max || var.valmax < min || var.valmax > max ) {
         return false;
    }
    return true;
}
#define inboundchk(v,j) if (! check_inbounds(v) ) { j.throw_error(string_format("Value must be between 0 and %d",mapgensize)); }

jmapgen_int::jmapgen_int( JsonObject &jo, const std::string &tag )
{
    if( jo.has_array( tag ) ) {
        JsonArray sparray = jo.get_array( tag );
        if( sparray.size() < 1 || sparray.size() > 2 ) {
            jo.throw_error( "invalid data: must be an array of 1 or 2 values", tag );
        }
        val = sparray.get_int( 0 );
        if( sparray.size() == 2 ) {
            valmax = sparray.get_int( 1 );
        }
    } else {
        val = valmax = jo.get_int( tag );
    }
}

jmapgen_int::jmapgen_int( JsonObject &jo, const std::string &tag, const short def_val, const short def_valmax )
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

/*
 * Turn json gobbldigook into machine friendly gobbldigook, for applying
 * basic map 'set' functions, optionally based on one_in(chance) or repeat value
 */
void mapgen_function_json::setup_setmap( JsonArray &parray ) {
    std::string tmpval="";
    std::string err = "";
    std::map<std::string, jmapgen_setmap_op> setmap_opmap;
    setmap_opmap[ "terrain" ] = JMAPGEN_SETMAP_TER;
    setmap_opmap[ "furniture" ] = JMAPGEN_SETMAP_FURN;
    setmap_opmap[ "trap" ] = JMAPGEN_SETMAP_TRAP;
    setmap_opmap[ "radiation" ] = JMAPGEN_SETMAP_RADIATION;
    setmap_opmap[ "bash" ] = JMAPGEN_SETMAP_BASH;
    std::map<std::string, jmapgen_setmap_op>::iterator sm_it;
    jmapgen_setmap_op tmpop;
    int setmap_optype = 0;
    while ( parray.has_more() ) {
        JsonObject pjo = parray.next_object();
        if ( pjo.read("point",tmpval ) ) {
            setmap_optype = JMAPGEN_SETMAP_OPTYPE_POINT;
        } else if ( pjo.read("set",tmpval ) ) {
            setmap_optype = JMAPGEN_SETMAP_OPTYPE_POINT;
            debugmsg("Warning, set: [ { \"set\": ... } is deprecated, use set: [ { \"point\": ... ");
        } else if ( pjo.read("line",tmpval ) ) {
            setmap_optype = JMAPGEN_SETMAP_OPTYPE_LINE;
        } else if ( pjo.read("square",tmpval ) ) {
            setmap_optype = JMAPGEN_SETMAP_OPTYPE_SQUARE;
        } else {
            err = string_format("No idea what to do with:\n %s \n",pjo.str().c_str() ); throw err;
        }

        sm_it = setmap_opmap.find( tmpval );
        if ( sm_it == setmap_opmap.end() ) {
            err = string_format("set: invalid subfunction '%s'",tmpval.c_str() ); throw err;
        }

        tmpop = sm_it->second;
        jmapgen_int tmp_x2(0,0);
        jmapgen_int tmp_y2(0,0);
        jmapgen_int tmp_i(0,0);
        int tmp_chance = 1;
        int tmp_rotation = 0;
        int tmp_fuel = -1;
        int tmp_status = -1;

        const jmapgen_int tmp_x( pjo, "x" );
        inboundchk(tmp_x,pjo);
        const jmapgen_int tmp_y( pjo, "y" );
        inboundchk(tmp_x,pjo);
        if ( setmap_optype != JMAPGEN_SETMAP_OPTYPE_POINT ) {
            tmp_x2 = jmapgen_int( pjo, "x2" );
            inboundchk(tmp_x2,pjo);
            tmp_y2 = jmapgen_int( pjo, "y2" );
            inboundchk(tmp_y2,pjo);
        }
        if ( tmpop == JMAPGEN_SETMAP_RADIATION ) {
            tmp_i = jmapgen_int( pjo, "amount" );
        } else if (tmpop == JMAPGEN_SETMAP_BASH){
            //suppress warning
        } else {
            if ( ! pjo.has_string("id") ) {
                err = string_format("set %s: bad/missing value for 'id'",tmpval.c_str() ); throw err;
            }
            std::string tmpid = pjo.get_string("id");
            switch( tmpop ) {
                case JMAPGEN_SETMAP_TER: {
                    if ( termap.find( tmpid ) == termap.end() ) {
                        err = string_format("set %s: no such terrain '%s'",tmpval.c_str(), tmpid.c_str() ); throw err;
                    }
                    tmp_i.val = termap[ tmpid ].loadid;
                } break;
                case JMAPGEN_SETMAP_FURN: {
                    if ( furnmap.find( tmpid ) == furnmap.end() ) {
                        err = string_format("set %s: no such furniture '%s'",tmpval.c_str(), tmpid.c_str() ); throw err;
                    }
                    tmp_i.val = furnmap[ tmpid ].loadid;
                } break;
                case JMAPGEN_SETMAP_TRAP: {
                    if ( trapmap.find( tmpid ) == trapmap.end() ) {
                        err = string_format("set %s: no such trap '%s'",tmpval.c_str(), tmpid.c_str() ); throw err;
                    }
                    tmp_i.val = trapmap[ tmpid ];
                } break;

                default:
                    //Suppress warnings
                    break;
            }
            tmp_i.valmax = tmp_i.val; // todo... support for random furniture? or not.
        }
        const jmapgen_int tmp_repeat = jmapgen_int( pjo, "repeat", 1, 1 );  // todo, sanity check?
        pjo.read("chance", tmp_chance );
        pjo.read("rotation", tmp_rotation );
        pjo.read("fuel", tmp_fuel );
        pjo.read("status", tmp_status );
        jmapgen_setmap tmp( tmp_x, tmp_y, tmp_x2, tmp_y2, jmapgen_setmap_op(tmpop+setmap_optype), tmp_i, tmp_chance, tmp_repeat, tmp_rotation, tmp_fuel, tmp_status );

        setmap_points.push_back(tmp);
        tmpval = "";
    }

}

jmapgen_place::jmapgen_place( JsonObject &jsi )
: x( jsi, "x" )
, y( jsi, "y" )
, repeat( jsi, "repeat", 1, 1 )
{
}

/**
 * This is a generic mapgen piece, the template parameter PieceType should be another specific
 * type of jmapgen_piece. This class contains a vector of those objects and will chose one of
 * it at random.
 */
template<typename PieceType>
class jmapgen_alternativly : public jmapgen_piece {
public:
    // Note: this bypasses virtual function system, all items in this vector are of type
    // PieceType, they *can not* be of any other type.
    std::vector<PieceType> alternatives;
    jmapgen_alternativly() = default;
    void apply( map &m, const size_t x, const size_t y, const float mon_density ) const override
    {
        if( alternatives.empty() ) {
            return;
        }
        auto &chosen = alternatives[rng( 0, alternatives.size() - 1 )];
        chosen.apply( m, x, y, mon_density );
    }
};

/**
 * Places fields on the map.
 * "field": field type ident.
 * "density": initial field density.
 * "age": initial field age.
 */
class jmapgen_field : public jmapgen_piece {
public:
    field_id ftype;
    int density;
    int age;
    jmapgen_field( JsonObject &jsi ) : jmapgen_piece()
    , ftype( field_from_ident( jsi.get_string( "field" ) ) )
    , density( jsi.get_int( "density", 1 ) )
    , age( jsi.get_int( "age", 0 ) )
    {
        if( ftype == fd_null ) {
            jsi.throw_error( "invalid field type", "field" );
        }
    }
    void apply( map &m, const size_t x, const size_t y, const float /*mon_density*/ ) const override
    {
        m.add_field( point( x, y ), ftype, density, age );
    }
};
/**
 * Place an NPC.
 * "class": the npc class, see @ref map::place_npc
 */
class jmapgen_npc : public jmapgen_piece {
public:
    std::string npc_class;
    jmapgen_npc( JsonObject &jsi ) : jmapgen_piece()
    , npc_class( jsi.get_string( "class" ) )
    {
        if( npc::_all_npc.count( npc_class ) == 0 ) {
            jsi.throw_error( "unknown npc class", "class" );
        }
    }
    void apply( map &m, const size_t x, const size_t y, const float /*mon_density*/ ) const override
    {
        m.place_npc( x, y, npc_class );
    }
};
/**
 * Place a sign with some text.
 * "signage": the text on the sign.
 */
class jmapgen_sign : public jmapgen_piece {
public:
    std::string signage;
    jmapgen_sign( JsonObject &jsi ) : jmapgen_piece()
    , signage( jsi.get_string( "signage" ) )
    {
        if( !signage.empty() ) {
            signage = _( signage.c_str() );
        }
    }
    void apply( map &m, const size_t x, const size_t y, const float /*mon_density*/ ) const override
    {
        m.furn_set( x, y, f_null );
        m.furn_set( x, y, "f_sign" );
        m.set_signage( x, y, signage );
    }
};
/**
 * Place a vending machine with content.
 * "item_group": the item group that is used to generate the content of the vending machine.
 */
class jmapgen_vending_machine : public jmapgen_piece {
public:
    std::string item_group_id;
    jmapgen_vending_machine( JsonObject &jsi ) : jmapgen_piece()
    , item_group_id( jsi.get_string( "item_group", one_in( 2 ) ? "vending_food" : "vending_drink" ) )
    {
        if( !item_group::group_is_defined( item_group_id ) ) {
            jsi.throw_error( "no such item group", "item_group" );
        }
    }
    void apply( map &m, const size_t x, const size_t y, const float /*mon_density*/ ) const override
    {
        m.furn_set( x, y, f_null );
        m.place_vending( x, y, item_group_id );
    }
};
/**
 * Place a toilet with (dirty) water in it.
 * "amount": number of water charges to place.
 */
class jmapgen_toilet : public jmapgen_piece {
public:
    jmapgen_int amount;
    jmapgen_toilet( JsonObject &jsi ) : jmapgen_piece()
    , amount( jsi, "amount", 0, 0 )
    {
    }
    void apply( map &m, const size_t x, const size_t y, const float /*mon_density*/ ) const override
    {
        const long charges = amount.get();
        m.furn_set( x, y, f_null );
        if( charges == 0 ) {
            m.place_toilet( x, y ); // Use the default charges supplied as default values
        } else {
            m.place_toilet( x, y, charges );
        }
    }
};
/**
 * Place a gas pump with fuel in it.
 * "amount": number of fuel charges to place.
 */
class jmapgen_gaspump : public jmapgen_piece {
public:
    jmapgen_int amount;
    jmapgen_gaspump( JsonObject &jsi ) : jmapgen_piece()
    , amount( jsi, "amount", 0, 0 )
    {
    }
    void apply( map &m, const size_t x, const size_t y, const float /*mon_density*/ ) const override
    {
        const long charges = amount.get();
        m.furn_set( x, y, f_null );
        if( charges == 0 ) {
            m.place_gas_pump( x, y, rng( 10000, 50000 ) );
        } else {
            m.place_gas_pump( x, y, charges );
        }
    }
};
/**
 * Place items from an item group.
 * "item": id of the item group.
 * "chance": chance of items being placed, see @ref map::place_items
 */
class jmapgen_item_group : public jmapgen_piece {
public:
    std::string group_id;
    jmapgen_int chance;
    jmapgen_item_group( JsonObject &jsi ) : jmapgen_piece()
    , group_id( jsi.get_string( "item" ) )
    , chance( jsi, "chance", 1, 1 )
    {
        if( !item_group::group_is_defined( group_id ) ) {
            jsi.throw_error( "no such item group", "item" );
        }
    }
    void apply( map &m, const size_t x, const size_t y, const float /*mon_density*/ ) const override
    {
        m.place_items( group_id, chance.get(), x, y, x, y, true, 0 );
    }
};
/**
 * Place spawn points for a monster group (actual monster spawning is done later).
 * "monster": id of the monster group.
 * "chance": see @ref map::place_spawns
 * "density": see @ref map::place_spawns
 */
class jmapgen_monster_group : public jmapgen_piece {
public:
    std::string mongroup_id;
    float density;
    jmapgen_int chance;
    jmapgen_monster_group( JsonObject &jsi ) : jmapgen_piece()
    , mongroup_id( jsi.get_string( "monster" ) )
    , density( jsi.get_float( "density", -1.0f ) )
    , chance( jsi, "chance", 1, 1 )
    {
        if( !MonsterGroupManager::isValidMonsterGroup( mongroup_id ) ) {
            jsi.throw_error( "no such monster group", "monster" );
        }
    }
    void apply( map &m, const size_t x, const size_t y, const float mdensity ) const override
    {
        m.place_spawns( mongroup_id, chance.get(), x, y, x, y, density == -1.0f ? mdensity : density );
    }
};
/**
 * Place spawn points for a specific monster (not a group).
 * "monster": id of the monster.
 * "friendly": whether the new monster is friendly to the player character.
 * "name": the name of the monster (if it has one).
 */
class jmapgen_monster : public jmapgen_piece {
public:
    std::string id;
    bool friendly;
    std::string name;
    jmapgen_monster( JsonObject &jsi ) : jmapgen_piece()
    , id( jsi.get_string( "monster" ) )
    , friendly( jsi.get_bool( "friendly", false ) )
    , name( jsi.get_string( "name", "NONE" ) )
    {
        if( !MonsterGenerator::generator().has_mtype( id ) ) {
            jsi.throw_error( "no such monster", "monster" );
        }
    }
    void apply( map &m, const size_t x, const size_t y, const float /*mdensity*/ ) const override
    {
        m.add_spawn( id, 1, x, y, friendly, -1, -1, name );
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
class jmapgen_vehicle : public jmapgen_piece {
public:
    std::string type;
    jmapgen_int chance;
    int rotation;
    int fuel;
    int status;
    jmapgen_vehicle( JsonObject &jsi ) : jmapgen_piece()
    , type( jsi.get_string( "vehicle" ) )
    , chance( jsi, "chance", 1, 1 )
    , rotation( jsi.get_int( "rotation", 0 ) )
    , fuel( jsi.get_int( "fuel", -1 ) )
    , status( jsi.get_int( "status", -1 ) )
    {
        if( g->vtypes.count( type ) == 0 ) {
            jsi.throw_error( "no such vehicle type", "vehicle" );
        }
    }
    void apply( map &m, const size_t x, const size_t y, const float /*mon_density*/ ) const override
    {
        if( !x_in_y( chance.get(), 100 ) ) {
            return;
        }
        m.add_vehicle( type, x, y, rotation, fuel, status );
    }
};
/**
 * Place a specific item.
 * "item": id of item type to spawn.
 * "chance": chance of spawning it (1 = always, otherwise one_in(chance)).
 * "amount": amount of items to spawn.
 */
class jmapgen_spawn_item : public jmapgen_piece {
public:
    itype_id type;
    jmapgen_int amount;
    jmapgen_int chance;
    jmapgen_spawn_item( JsonObject &jsi ) : jmapgen_piece()
    , type( jsi.get_string( "item" ) )
    , amount( jsi, "amount", 1, 1 )
    , chance( jsi, "chance", 1, 1 )
    {
        if( !item::type_is_defined( type ) ) {
            jsi.throw_error( "no such item type", "item" );
        }
    }
    void apply( map &m, const size_t x, const size_t y, const float /*mon_density*/ ) const override
    {
        const int c = chance.get();
        if ( c == 1 || one_in( c ) ) {
            m.spawn_item( x, y, type, amount.get() );
        }
    }
};
/**
 * Place a trap.
 * "trap": id of the trap.
 */
class jmapgen_trap : public jmapgen_piece {
public:
    trap_id id;
    jmapgen_trap( JsonObject &jsi ) : jmapgen_piece()
    , id( 0 )
    {
        const auto iter = trapmap.find( jsi.get_string( "trap" ) );
        if( iter == trapmap.end() ) {
            jsi.throw_error( "no such trap", "trap" );
        }
        id = iter->second;
    }
    jmapgen_trap( const std::string &tid ) : jmapgen_piece()
    , id( 0 )
    {
        const auto iter = trapmap.find( tid );
        if( iter == trapmap.end() ) {
            throw std::string( "unknown trap type" );
        }
        id = iter->second;
    }
    void apply( map &m, const size_t x, const size_t y, const float /*mdensity*/ ) const override
    {
        m.add_trap( x, y, id );
    }
};
/**
 * Place a furniture.
 * "furn": id of the furniture.
 */
class jmapgen_furniture : public jmapgen_piece {
public:
    furn_id id;
    jmapgen_furniture( JsonObject &jsi ) : jmapgen_piece()
    , id( 0 )
    {
        const auto iter = furnmap.find( jsi.get_string( "furn" ) );
        if( iter == furnmap.end() ) {
            jsi.throw_error( "unknown furniture type", "furn" );
        }
        id = iter->second.loadid;
    }
    jmapgen_furniture( const std::string &tid ) : jmapgen_piece()
    , id( 0 )
    {
        const auto iter = furnmap.find( tid );
        if( iter == furnmap.end() ) {
            throw std::string( "unknown furniture type" );
        }
        id = iter->second.loadid;
    }
    void apply( map &m, const size_t x, const size_t y, const float /*mdensity*/ ) const override
    {
        m.furn_set( x, y, id );
    }
};
/**
 * Place terrain.
 * "ter": id of the terrain.
 */
class jmapgen_terrain : public jmapgen_piece {
public:
    ter_id id;
    jmapgen_terrain( JsonObject &jsi ) : jmapgen_piece()
    , id( 0 )
    {
        const auto iter = termap.find( jsi.get_string( "ter" ) );
        if( iter == termap.end() ) {
            jsi.throw_error( "unknown terrain type", "ter" );
        }
        id = iter->second.loadid;
    }
    jmapgen_terrain( const std::string &tid ) : jmapgen_piece()
    , id( 0 )
    {
        const auto iter = termap.find( tid );
        if( iter == termap.end() ) {
            throw std::string( "unknown terrain type" );
        }
        id = iter->second.loadid;
    }
    void apply( map &m, const size_t x, const size_t y, const float /*mdensity*/ ) const override
    {
        m.ter_set( x, y, id );
    }
};

template<typename PieceType>
void mapgen_function_json::load_objects( JsonArray parray )
{
    while( parray.has_more() ) {
        auto jsi = parray.next_object();
        const jmapgen_place where( jsi );
        std::shared_ptr<jmapgen_piece> what( new PieceType( jsi ) );
        objects.push_back( jmapgen_obj( where, what ) );
    }
}

template<typename PieceType>
void mapgen_function_json::load_objects( JsonObject &jsi, const std::string &member_name )
{
    if( !jsi.has_member( member_name ) ) {
        return;
    }
    load_objects<PieceType>( jsi.get_array( member_name ) );
}

template<typename PieceType>
void load_place_mapings( JsonObject jobj, mapgen_function_json::placing_map::mapped_type &vect )
{
    vect.emplace_back( new PieceType( jobj ) );
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
void load_place_mapings( JsonObject &pjo, const std::string &key, mapgen_function_json::placing_map::mapped_type &vect )
{
    if( pjo.has_object( key ) ) {
        load_place_mapings<PieceType>( pjo.get_object( key ), vect );
    } else {
        JsonArray jarr = pjo.get_array( key );
        while( jarr.has_more() ) {
            load_place_mapings<PieceType>( jarr.next_object(), vect );
        }
    }
}

/*
This function allows loading the mapgen pieces from a single string, *or* a json object.
The mapgen piece is loaded from the member of the json object named key.
*/
template<typename PieceType>
void load_place_mapings_string( JsonObject &pjo, const std::string &key, mapgen_function_json::placing_map::mapped_type &vect )
{
    if( pjo.has_string( key ) ) {
        try {
            vect.emplace_back( new PieceType( pjo.get_string( key ) ) );
        } catch( const std::string &err ) {
            // Using the json object here adds nice formatting and context information
            pjo.throw_error( err, key );
        }
    } else if( pjo.has_object( key ) ) {
        load_place_mapings<PieceType>( pjo.get_object( key ), vect );
    } else {
        JsonArray jarr = pjo.get_array( key );
        while( jarr.has_more() ) {
            if( jarr.test_string() ) {
                try {
                    vect.emplace_back( new PieceType( jarr.next_string() ) );
                } catch( const std::string &err ) {
                    // Using the json object here adds nice formatting and context information
                    jarr.throw_error( err );
                }
            } else {
                load_place_mapings<PieceType>( jarr.next_object(), vect );
            }
        }
    }
}
/*
This function is like load_place_mapings_string, except if the input is an array it will create an
instance of jmapgen_alternativly which will chose the mapgen piece to apply to the map randomly.
Use this with terrain or traps or other things that can not be applied twice to the same place.
*/
template<typename PieceType>
void load_place_mapings_alternatively( JsonObject &pjo, const std::string &key, mapgen_function_json::placing_map::mapped_type &vect )
{
    if( !pjo.has_array( key ) ) {
        load_place_mapings_string<PieceType>( pjo, key, vect );
    } else {
        auto alter = std::make_shared< jmapgen_alternativly<PieceType> >();
        JsonArray jarr = pjo.get_array( key );
        while( jarr.has_more() ) {
            if( jarr.test_string() ) {
                try {
                    alter->alternatives.emplace_back( jarr.next_string() );
                } catch( const std::string &err ) {
                    // Using the json object here adds nice formatting and context information
                    jarr.throw_error( err );
                }
            } else {
                JsonObject jsi = jarr.next_object();
                alter->alternatives.emplace_back( jsi );
            }
        }
        vect.push_back( alter );
    }
}

template<>
void load_place_mapings<jmapgen_trap>( JsonObject &pjo, const std::string &key, mapgen_function_json::placing_map::mapped_type &vect )
{
    load_place_mapings_alternatively<jmapgen_trap>( pjo, key, vect );
}

template<>
void load_place_mapings<jmapgen_furniture>( JsonObject &pjo, const std::string &key, mapgen_function_json::placing_map::mapped_type &vect )
{
    load_place_mapings_alternatively<jmapgen_furniture>( pjo, key, vect );
}

template<>
void load_place_mapings<jmapgen_terrain>( JsonObject &pjo, const std::string &key, mapgen_function_json::placing_map::mapped_type &vect )
{
    load_place_mapings_alternatively<jmapgen_terrain>( pjo, key, vect );
}

template<typename PieceType>
void mapgen_function_json::load_place_mapings( JsonObject &jo, const std::string &member_name, placing_map &format_placings )
{
    if( jo.has_object( "mapping" ) ) {
        JsonObject pjo = jo.get_object( "mapping" );
        for( auto & key : pjo.get_member_names() ) {
            if( key.size() != 1 ) {
                pjo.throw_error( "format map key must be 1 character", key );
            }
            JsonObject sub = pjo.get_object( key );
            if( !sub.has_member( member_name ) ) {
                continue;
            }
            auto &vect = format_placings[ key[0] ];
            ::load_place_mapings<PieceType>( sub, member_name, vect );
        }
    }
    if( !jo.has_object( member_name ) ) {
        return;
    }
    JsonObject pjo = jo.get_object( member_name );
    for( auto & key : pjo.get_member_names() ) {
        if( key.size() != 1 ) {
            pjo.throw_error( "format map key must be 1 character", key );
        }
        auto &vect = format_placings[ key[0] ];
        ::load_place_mapings<PieceType>( pjo, key, vect );
    }
}

/*
 * Parse json, pre-calculating values for stuff, then cheerfully throw json away. Faster than regular mapf, in theory
 */
bool mapgen_function_json::setup() {
    if ( is_ready ) {
        return true;
    }
    if ( jdata.empty() ) {
        return false;
    }
    std::istringstream iss( jdata );
    try {
        JsonIn jsin(iss);
        jsin.eat_whitespace();
        char ch = jsin.peek();
        if ( ch != '{' ) {
            throw string_format("Bad json:\n%s\n",jdata.substr(0,796).c_str());
        }
        JsonObject jo = jsin.get_object();
        bool qualifies = false;
        std::string tmpval = "";
        JsonArray parray;
        JsonArray sparray;
        JsonObject pjo;
        // mapgensize = jo.get_int("mapgensize", 24); // eventually..

        // something akin to mapgen fill_background.
        if ( jo.read("fill_ter", tmpval) ) {
            if ( termap.find( tmpval ) == termap.end() ) {
                jo.throw_error(string_format("  fill_ter: invalid terrain '%s'",tmpval.c_str() ));
            }
            fill_ter = (int)termap[ tmpval ].loadid;
            qualifies = true;
            tmpval = "";
        }

        format.reset(new ter_furn_id[ mapgensize * mapgensize ]);
        // just like mapf::basic_bind("stuff",blargle("foo", etc) ), only json input and faster when applying
        if ( jo.has_array("rows") ) {
            placing_map format_placings;
            std::map<int,int> format_terrain;
            std::map<int,int> format_furniture;
            // manditory: every character in rows must have matching entry, unless fill_ter is set
            // "terrain": { "a": "t_grass", "b": "t_lava" }
            if ( jo.has_object("terrain") ) {
                pjo = jo.get_object("terrain");
                for( const auto &key : pjo.get_member_names() ) {
                    if( key.size() != 1 ) {
                        pjo.throw_error( "format map key must be 1 character", key );
                    }
                    if( pjo.has_string( key ) ) {
                        const auto tmpval = pjo.get_string( key );
                        const auto iter = termap.find( tmpval );
                        if( iter == termap.end() ) {
                            jo.throw_error( "Invalid terrain", key );
                        }
                        format_terrain[key[0]] = iter->second.loadid;
                    } else {
                        auto &vect = format_placings[ key[0] ];
                        ::load_place_mapings<jmapgen_terrain>( pjo, key, vect );
                        if( !vect.empty() ) {
                            // Dummy entry to signal that this terrain is actually defined, because
                            // the code below checks that each square on the map has a valid terrain
                            // defined somehow.
                            format_terrain[key[0]] = t_null;
                        }
                    }
                }
            } else {
                throw string_format("  format: no terrain map\n%s\n",jo.str().substr(0,796).c_str());
            }
            if ( jo.has_object("furniture") ) {
                pjo = jo.get_object("furniture");
                for( const auto &key : pjo.get_member_names() ) {
                    if( key.size() != 1 ) {
                        pjo.throw_error( "format map key must be 1 character", key );
                    }
                    if( pjo.has_string( key ) ) {
                        const auto tmpval = pjo.get_string( key );
                        const auto iter = furnmap.find( tmpval );
                        if( iter == furnmap.end() ) {
                            jo.throw_error( "Invalid furniture", key );
                        }
                        format_furniture[key[0]] = iter->second.loadid;
                    } else {
                        auto &vect = format_placings[ key[0] ];
                        ::load_place_mapings<jmapgen_furniture>( pjo, key, vect );
                    }
                }
            }
            load_place_mapings<jmapgen_field>( jo, "fields", format_placings );
            load_place_mapings<jmapgen_npc>( jo, "npcs", format_placings );
            load_place_mapings<jmapgen_sign>( jo, "signs", format_placings );
            load_place_mapings<jmapgen_vending_machine>( jo, "vendingmachines", format_placings );
            load_place_mapings<jmapgen_toilet>( jo, "toilets", format_placings );
            load_place_mapings<jmapgen_gaspump>( jo, "gaspumps", format_placings );
            load_place_mapings<jmapgen_item_group>( jo, "items", format_placings );
            load_place_mapings<jmapgen_monster_group>( jo, "monsters", format_placings );
            load_place_mapings<jmapgen_vehicle>( jo, "vehicles", format_placings );
            // json member name is not optimal, it should be plural like all the others above, but that conflicts
            // with the items entry with refers to item groups.
            load_place_mapings<jmapgen_spawn_item>( jo, "item", format_placings );
            load_place_mapings<jmapgen_trap>( jo, "traps", format_placings );
            load_place_mapings<jmapgen_monster>( jo, "monster", format_placings );
            // manditory: 24 rows of 24 character lines, each of which must have a matching key in "terrain",
            // unless fill_ter is set
            // "rows:" [ "aaaajustlikeinmapgen.cpp", "this.must!be!exactly.24!", "and_must_match_terrain_", .... ]
            parray = jo.get_array( "rows" );
            if ( parray.size() != mapgensize ) {
                parray.throw_error( string_format("  format: rows: must have %d rows, not %d",mapgensize,parray.size() ));
            }
            for( size_t c = 0; c < mapgensize; c++ ) {
                const auto tmpval = parray.next_string();
                if ( tmpval.size() != mapgensize ) {
                    parray.throw_error(string_format("  format: row %d must have %d columns, not %d", c, mapgensize, tmpval.size()));
                }
                for ( size_t i = 0; i < tmpval.size(); i++ ) {
                    const int tmpkey = tmpval[i];
                    if ( format_terrain.find( tmpkey ) != format_terrain.end() ) {
                        format[ calc_index( i, c ) ].ter = format_terrain[ tmpkey ];
                    } else if ( ! qualifies ) { // fill_ter should make this kosher
                        parray.throw_error(string_format("  format: rows: row %d column %d: '%c' is not in 'terrain', and no 'fill_ter' is set!",c+1,i+1, (char)tmpkey ));
                    }
                    if ( format_furniture.find( tmpkey ) != format_furniture.end() ) {
                        format[ calc_index( i, c ) ].furn = format_furniture[ tmpkey ];
                    }
                    const auto fpi = format_placings.find( tmpkey );
                    if( fpi != format_placings.end() ) {
                        jmapgen_place where( i, c );
                        for( auto &what: fpi->second ) {
                            objects.push_back( jmapgen_obj( where, what ) );
                        }
                    }
                }
            }
            qualifies = true;
            do_format = true;
       }

       // No fill_ter? No format? GTFO.
       if ( ! qualifies ) {
           jo.throw_error("  Need either 'fill_terrain' or 'rows' + 'terrain' (RTFM)");
           // todo: write TFM.
       }

       if ( jo.has_array("set") ) {
            parray = jo.get_array("set");
            try {
                setup_setmap( parray );
            } catch (std::string smerr) {
                throw string_format("Bad JSON mapgen set array, discarding:\n    %s\n", smerr.c_str() );
            }
       }
        // this is for backwards compatibility, it should better be named place_items
        load_objects<jmapgen_spawn_item>( jo, "add" );
        load_objects<jmapgen_field>( jo, "place_fields" );
        load_objects<jmapgen_npc>( jo, "place_npcs" );
        load_objects<jmapgen_sign>( jo, "place_signs" );
        load_objects<jmapgen_vending_machine>( jo, "place_vendingmachines" );
        load_objects<jmapgen_toilet>( jo, "place_toilets" );
        load_objects<jmapgen_gaspump>( jo, "place_gaspumps" );
        load_objects<jmapgen_item_group>( jo, "place_items" );
        load_objects<jmapgen_monster_group>( jo, "place_monsters" );
        load_objects<jmapgen_vehicle>( jo, "place_vehicles" );
        load_objects<jmapgen_trap>( jo, "place_traps" );
        load_objects<jmapgen_furniture>( jo, "place_furniture" );
        load_objects<jmapgen_terrain>( jo, "place_terrain" );
        load_objects<jmapgen_monster>( jo, "place_monster" );

#ifdef LUA
       // silently ignore if unsupported in build
       if ( jo.has_string("lua") ) { // minified into one\nline
           luascript = jo.get_string("lua");
       } else if ( jo.has_array("lua") ) { // or 1 line per entry array
           luascript = "";
           JsonArray jascr = jo.get_array("lua");
           while ( jascr.has_more() ) {
               luascript += jascr.next_string();
               luascript += "\n";
           }
       }
#endif

    } catch (std::string e) {
        debugmsg("Bad JSON mapgen, discarding:\n  %s\n", e.c_str() );
        jdata.clear(); // silently fail further attempts
        return false;
    }
    jdata.clear(); // ssh, we're not -really- a json function <.<
    is_ready = true; // skip setup attempts from any additional pointers
    return true;
}

/////////////////////////////////////////////////////////////////////////////////
///// 3 - mapgen (gameplay)
///// stuff below is the actual in-game mapgeneration (ill)logic

/*
 * (set|line|square)_(ter|furn|trap|radiation); simple (x, y, int) or (x1,y1,x2,y2, int) functions
 * todo; optimize, though gcc -O2 optimizes enough that splitting the switch has no effect
 */
bool jmapgen_setmap::apply( map *m ) {
    if ( chance == 1 || one_in( chance ) ) {
        const int trepeat = repeat.get();
        for (int i = 0; i < trepeat; i++) {
            switch(op) {
                case JMAPGEN_SETMAP_TER: {
                    m->ter_set( x.get(), y.get(), val.get() );
                } break;
                case JMAPGEN_SETMAP_FURN: {
                    m->furn_set( x.get(), y.get(), val.get() );
                } break;
                case JMAPGEN_SETMAP_TRAP: {
                    m->trap_set( x.get(), y.get(), val.get() );
                } break;
                case JMAPGEN_SETMAP_RADIATION: {
                    m->set_radiation( x.get(), y.get(), val.get());
                } break;
                case JMAPGEN_SETMAP_BASH: {
                    m->bash( x.get(), y.get(), 9999);
                } break;

                case JMAPGEN_SETMAP_LINE_TER: {
                    m->draw_line_ter( (ter_id)val.get(), x.get(), y.get(), x2.get(), y2.get() );
                } break;
                case JMAPGEN_SETMAP_LINE_FURN: {
                    m->draw_line_furn( (furn_id)val.get(), x.get(), y.get(), x2.get(), y2.get() );
                } break;
                case JMAPGEN_SETMAP_LINE_TRAP: {
                    const std::vector<point> line = line_to(x.get(), y.get(), x2.get(), y2.get(), 0);
                    for (auto &i : line) {
                        m->trap_set( i.x, i.y, (trap_id)val.get() );
                    }
                } break;
                case JMAPGEN_SETMAP_LINE_RADIATION: {
                    const std::vector<point> line = line_to(x.get(), y.get(), x2.get(), y2.get(), 0);
                    for (auto &i : line) {
                        m->set_radiation( i.x, i.y, (int)val.get() );
                    }
                } break;


                case JMAPGEN_SETMAP_SQUARE_TER: {
                    m->draw_square_ter( (ter_id)val.get(), x.get(), y.get(), x2.get(), y2.get() );
                } break;
                case JMAPGEN_SETMAP_SQUARE_FURN: {
                    m->draw_square_furn( (furn_id)val.get(), x.get(), y.get(), x2.get(), y2.get() );
                } break;
                case JMAPGEN_SETMAP_SQUARE_TRAP: {
                    const int cx = x.get();
                    const int cy = y.get();
                    const int cx2 = x2.get();
                    const int cy2 = y2.get();
                    for (int tx = cx; tx <= cx2; tx++) {
                       for (int ty = cy; ty <= cy2; ty++) {
                           m->trap_set( tx, ty, (trap_id)val.get() );
                       }
                    }
                } break;
                case JMAPGEN_SETMAP_SQUARE_RADIATION: {
                    const int cx = x.get();
                    const int cy = y.get();
                    const int cx2 = x2.get();
                    const int cy2 = y2.get();
                    for (int tx = cx; tx <= cx2; tx++) {
                        for (int ty = cy; ty <= cy2; ty++) {
                            m->set_radiation( tx, ty, (int)val.get());
                        }
                    }
                } break;

                default:
                    //Suppress warnings
                    break;
            }
        }
    }
    return true;
}

void mapgen_lua(map * m, oter_id id, mapgendata md, int t, float d, const std::string & scr);
/*
 * Apply mapgen as per a derived-from-json recipe; in theory fast, but not very versatile
 */
void mapgen_function_json::generate( map *m, oter_id terrain_type, mapgendata md, int t, float d ) {
    if ( fill_ter != -1 ) {
        m->draw_fill_background( fill_ter );
    }
    if ( do_format ) {
        formatted_set_incredibly_simple(m, format.get(), mapgensize, mapgensize, 0, 0, fill_ter );
    }
    for( auto &elem : setmap_points ) {
        elem.apply( m );
    }
#ifdef LUA
    if ( ! luascript.empty() ) {
        mapgen_lua(m, terrain_type, md, t, d, luascript);
    }
#else
    (void)md;
    (void)t;
#endif
    for( auto &obj : objects ) {
        const auto &where = obj.first;
        const auto &what = *obj.second;
        const int repeat = where.repeat.get();
        for( int i = 0; i < repeat; i++ ) {
            const auto x = where.x.get();
            const auto y = where.y.get();
            what.apply( *m, x, y, d );
        }
    }
    if( terrain_type.t().has_flag(rotates) ) {
        mapgen_rotate(m, terrain_type, false );
    }
}

/////////////////////////////////////////////////////////////////////////////////
///// lua mapgen functions
// wip: need moar bindings. Basic stuff works

/*
 * Apply interpreted script; slowest, more versatile eventually.
 */
void mapgen_lua(map * m,oter_id id,mapgendata md ,int t,float d, const std::string & scr) {
#ifdef LUA
    lua_mapgen(m, std::string(id), md, t, d, scr);
#else
    (void)scr;
    mapgen_crater(m,id,md,t,d);
    mapf::formatted_set_simple(m, 0, 6,
"\
    *   *  ***\n\
    **  * *   *\n\
    * * * *   *\n\
    *  ** *   *\n\
    *   *  ***\n\
\n\
 *     *   *   *\n\
 *     *   *  * *\n\
 *     *   *  ***\n\
 *     *   * *   *\n\
 *****  ***  *   *\n\
", mapf::basic_bind("*", t_paper), mapf::basic_bind("")); // should never happen: overmap loader skips lua mapgens on !LUA builds.

#endif
}

#ifdef LUA
void mapgen_function_lua::generate( map *m, oter_id terrain_type, mapgendata dat, int t, float d ) {
    mapgen_lua(m, terrain_type, dat, t, d, scr );
}
#endif

/////////////
// TODO: clean up variable shadowing in this function
// unfortunately, due to how absurdly long the function is (over 8000 lines!), it'll be hard to
// track down what is and isn't supposed to be carried around between bits of code.
// I suggest that we break the function down into smaller parts

void map::draw_map(const oter_id terrain_type, const oter_id t_north, const oter_id t_east,
                   const oter_id t_south, const oter_id t_west, const oter_id t_neast,
                   const oter_id t_seast, const oter_id t_nwest, const oter_id t_swest,
                   const oter_id t_above, const int turn, const float density,
                   const int zlevel, const regional_settings * rsettings)
{
    // Big old switch statement with a case for each overmap terrain type.
    // Many of these can be copied from another type, then rotated; for instance,
    //  "house_east" is identical to "house_north", just rotated 90 degrees to
    //  the right.  The rotate(int) function is at the bottom of this file.

    // Below comment is outdated
    // TODO: Update comment
    // The place_items() function takes a item group ident type,
    //  an "odds" int giving the chance for a single item to be
    //  placed, four ints (x1, y1, x2, y2) corresponding to the upper left corner
    //  and lower right corner of a square where the items are placed, a boolean
    //  that indicates whether items may spawn on grass & dirt, and finally an
    //  integer that indicates on which turn the items were created.  This final
    //  integer should be 0, unless the items are "fresh-grown" like wild fruit.

    //these variables are used in regular house generation. Placed here by Whales
    int rn = 0;
    int lw = 0;
    int rw = 0;
    int mw = 0;
    int tw = 0;
    int bw = 0;
    int cw = 0;

    int x = 0;
    int y = 0;

    // To distinguish between types of labs
    bool ice_lab = true;

    oter_id t_nesw[] = {t_north, t_east, t_south, t_west, t_neast, t_seast, t_nwest, t_swest};
    int nesw_fac[] = {0, 0, 0, 0, 0, 0, 0, 0};
    int &n_fac = nesw_fac[0], &e_fac = nesw_fac[1], &s_fac = nesw_fac[2], &w_fac = nesw_fac[3];

    mapgendata dat(t_north, t_east, t_south, t_west, t_neast, t_seast, t_nwest, t_swest, t_above, zlevel, rsettings, this);

    computer *tmpcomp = NULL;
    bool terrain_type_found = true;
    const std::string function_key = terrain_type.t().id_mapgen;


    std::map<std::string, std::vector<mapgen_function*> >::const_iterator fmapit = oter_mapgen.find( function_key );
    if ( fmapit != oter_mapgen.end() && !fmapit->second.empty() ) {
        // int fidx = rng(0, fmapit->second.size() - 1); // simple unwieghted list
        std::map<std::string, std::map<int,int> >::const_iterator weightit = oter_mapgen_weights.find( function_key );
        const int rlast = weightit->second.rbegin()->first;
        const int roll = rng(1, rlast);
        const int fidx = weightit->second.lower_bound( roll )->second;
        //add_msg("draw_map: %s (%s): %d/%d roll %d/%d den %.4f", terrain_type.c_str(), function_key.c_str(), fidx+1, fmapit->second.size(), roll, rlast, density );

        fmapit->second[fidx]->generate(this, terrain_type, dat, turn, density);
    // todo; make these mappable functions
    } else if (terrain_type == "lab" ||
               terrain_type == "lab_stairs" ||
               terrain_type == "lab_core" ||
               terrain_type == "ice_lab" ||
               terrain_type == "ice_lab_stairs" ||
               terrain_type == "ice_lab_core") {

        if (is_ot_type("ice_lab", terrain_type)) {
            ice_lab = true;
        } else {
            ice_lab = false;
        }

        if (ice_lab) {
            int temperature = -20 + 30 * (zlevel);
            set_temperature(x, y, temperature);
        }

        // Check for adjacent sewers; used below
        tw = 0;
        rw = 0;
        bw = 0;
        lw = 0;
        if (is_ot_type("sewer", t_north) && connects_to(t_north, 2)) {
            tw = SEEY * 2;
        }
        if (is_ot_type("sewer", t_east) && connects_to(t_east, 3)) {
            rw = SEEX * 2;
        }
        if (is_ot_type("sewer", t_south) && connects_to(t_south, 0)) {
            bw = SEEY * 2;
        }
        if (is_ot_type("sewer", t_west) && connects_to(t_west, 1)) {
            lw = SEEX * 2;
        }
        if (zlevel == 0) { // We're on ground level
            for (int i = 0; i < SEEX * 2; i++) {
                for (int j = 0; j < SEEY * 2; j++) {
                    if (i <= 1 || i >= SEEX * 2 - 2 ||
                        (j > 1 && j < SEEY * 2 - 2 && (i == SEEX - 2 || i == SEEX + 1))) {
                        ter_set(i, j, t_wall_v);
                    } else if (j <= 1 || j >= SEEY * 2 - 2) {
                        ter_set(i, j, t_wall_h);
                    } else {
                        ter_set(i, j, t_floor);
                    }
                }
            }
            ter_set(SEEX - 1, 0, t_dirt);
            ter_set(SEEX - 1, 1, t_door_metal_locked);
            ter_set(SEEX    , 0, t_dirt);
            ter_set(SEEX    , 1, t_door_metal_locked);
            ter_set(SEEX - 2 + rng(0, 1) * 4, 0, t_card_science);
            ter_set(SEEX - 2, SEEY    , t_door_metal_c);
            ter_set(SEEX + 1, SEEY    , t_door_metal_c);
            ter_set(SEEX - 2, SEEY - 1, t_door_metal_c);
            ter_set(SEEX + 1, SEEY - 1, t_door_metal_c);
            ter_set(SEEX - 1, SEEY * 2 - 3, t_stairs_down);
            ter_set(SEEX    , SEEY * 2 - 3, t_stairs_down);
            science_room(this, 2       , 2, SEEX - 3    , SEEY * 2 - 3, zlevel, 1);
            science_room(this, SEEX + 2, 2, SEEX * 2 - 3, SEEY * 2 - 3, zlevel, 3);

            add_spawn("mon_turret", 1, SEEX, 5);

            if (is_ot_type("road", t_east)) {
                rotate(1);
            } else if (is_ot_type("road", t_south)) {
                rotate(2);
            } else if (is_ot_type("road", t_west)) {
                rotate(3);
            }
        } else if (tw != 0 || rw != 0 || lw != 0 || bw != 0) { // Sewers!
            for (int i = 0; i < SEEX * 2; i++) {
                for (int j = 0; j < SEEY * 2; j++) {
                    ter_set(i, j, t_rock_floor);
                    if (((i < lw || i > SEEX * 2 - 1 - rw) && j > SEEY - 3 && j < SEEY + 2) ||
                        ((j < tw || j > SEEY * 2 - 1 - bw) && i > SEEX - 3 && i < SEEX + 2)) {
                        ter_set(i, j, t_sewage);
                    }
                    if ((i == 0 && t_east >= "lab" && t_east <= "lab_core") ||
                        (i == 0 && t_east >= "ice_lab" && t_east <= "ice_lab_core") ||
                        i == SEEX * 2 - 1) {
                        if (ter(i, j) == t_sewage) {
                            ter_set(i, j, t_bars);
                        } else if (j == SEEY - 1 || j == SEEY) {
                            ter_set(i, j, t_door_metal_c);
                        } else {
                            ter_set(i, j, t_concrete_v);
                        }
                    } else if ((j == 0 && t_north >= "lab" && t_north <= "lab_core") ||
                               (j == 0 && t_north >= "ice_lab" && t_north <= "ice_lab_core") ||
                               j == SEEY * 2 - 1) {
                        if (ter(i, j) == t_sewage) {
                            ter_set(i, j, t_bars);
                        } else if (i == SEEX - 1 || i == SEEX) {
                            ter_set(i, j, t_door_metal_c);
                        } else {
                            ter_set(i, j, t_concrete_h);
                        }
                    }
                }
            }
        } else { // We're below ground, and no sewers
            // Set up the boudaries of walls (connect to adjacent lab squares)
            // Are we in an ice lab?
            if ( ice_lab ) {
                tw = is_ot_type("ice_lab", t_north) ? 0 : 2;
                rw = is_ot_type("ice_lab", t_east) ? 1 : 2;
                bw = is_ot_type("ice_lab", t_south) ? 1 : 2;
                lw = is_ot_type("ice_lab", t_west) ? 0 : 2;
            } else {
                tw = is_ot_type("lab", t_north) ? 0 : 2;
                rw = is_ot_type("lab", t_east) ? 1 : 2;
                bw = is_ot_type("lab", t_south) ? 1 : 2;
                lw = is_ot_type("lab", t_west) ? 0 : 2;
            }
            int boarders = 0;
            if (tw == 0 ) {
                boarders++;
            }
            if (rw == 1 ) {
                boarders++;
            }
            if (bw == 1 ) {
                boarders++;
            }
            if (lw == 0 ) {
                boarders++;
            }
            //A lab area with only one entrance
            if (boarders == 1) {
                fill_background(this, t_rock_floor);
                if (one_in(2)) { //armory and military barracks
                    mapf::formatted_set_simple(this, 0, 0,
                                               "\
|----------------------|\n\
|r....................r|\n\
|r..rr..rr....rr..rr..r|\n\
|r..rr..rr....rr..rr..r|\n\
|r..rr..rr....rr..rr..r|\n\
|r..rr..rr....rr..rr..r|\n\
|r..rr..rr....rr..rr..r|\n\
|......................|\n\
|......................|\n\
|..rrrrrr..........rrr.|\n\
|-----|----DD-|--+|--|-|\n\
|b.ddd|.......gc..|T.|T|\n\
|b..h.+.......g6h.|-+|+|\n\
|l....|.......gc..|....|\n\
|-----|.......|--D|...S|\n\
|b....+...........|...S|\n\
|b...l|...........|-+--|\n\
|-----|................|\n\
|b....+...x............|\n\
|b...l|..|-DD-|+-|+-|+-|\n\
|-----|..|....|.l|.l|.l|\n\
|b....+..|6...|..|..|..|\n\
|b...l|..|....|bb|bb|bb|\n\
|-----|--|-..-|--|--|--|\n",
                                               mapf::basic_bind("b l A r d C h 6 x g G , . - | + D t c S T", t_rock_floor, t_rock_floor, t_floor,
                                                       t_rock_floor, t_rock_floor, t_centrifuge, t_rock_floor, t_console, t_console_broken,
                                                       t_reinforced_glass_v, t_reinforced_glass_h, t_floor_blue, t_rock_floor, t_concrete_h, t_concrete_v,
                                                       t_door_metal_c, t_door_metal_locked, t_rock_floor, t_rock_floor, t_rock_floor, t_rock_floor),
                                               mapf::basic_bind("b l A r d C h 6 x g G , . - | + D t c S T", f_bed,        f_locker,     f_crate_c,
                                                       f_rack,       f_desk,       f_null,       f_chair,      f_null,    f_null,           f_null,
                                                       f_null,               f_null,       f_null,       f_null,       f_null,       f_null,
                                                       f_null,              f_table,      f_counter,    f_sink,       f_toilet));
                    for (int i = 0; i <= 23; i++) {
                        for (int j = 0; j <= 23; j++) {
                            if (this->furn(i, j) == f_locker) {
                                place_items("mil_surplus", 50,  i,  j, i,  j, false, 0);
                            } else if (this->furn(i, j) == f_desk) {
                                place_items("office", 50,  i,  j, i,  j, false, 0);
                            } else if (this->furn(i, j) == f_rack) {
                                if (one_in(3)) {
                                    place_items("mil_surplus", 30,  i,  j, i,  j, false, 0);
                                } else if (one_in(2)) {
                                    place_items("ammo", 30,  i,  j, i,  j, false, 0);
                                } else if (one_in(3)) {
                                    place_items("military", 30,  i,  j, i,  j, false, 0);
                                } else {
                                    place_items("mil_rifles", 30,  i,  j, i,  j, false, 0);
                                }
                            } else if (this->furn(i, j) == f_bed) {
                                place_items("bed", 50,  i,  j, i,  j, false, 0);
                            }
                        }
                    }
                    computer *tmpcomp2 = NULL;
                    tmpcomp2 = add_computer(10, 21, _("Barracks Entrance"), 4);
                    tmpcomp2->add_option(_("UNLOCK ENTRANCE"), COMPACT_UNLOCK, 6);
                    tmpcomp2->add_failure(COMPFAIL_DAMAGE);
                    tmpcomp2->add_failure(COMPFAIL_SHUTDOWN);
                    tmpcomp = add_computer(15, 12, _("Magazine Entrance"), 6);
                    tmpcomp->add_option(_("UNLOCK ENTRANCE"), COMPACT_UNLOCK, 7);
                    tmpcomp->add_failure(COMPFAIL_DAMAGE);
                    tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
                    if (one_in(2)) {
                        add_spawn("mon_zombie_soldier", rng(1, 4), 12, 12);
                    }
                    else if (one_in(5)) {
                        add_spawn("mon_zombie_bio_op", rng(1, 2), 12, 12);
                    }
                } else { //human containment
                    mapf::formatted_set_simple(this, 0, 0,
                                               "\
|----|-|----|----|-|---|\n\
|b.T.|.|.T.b|b.T.|.|A.A|\n\
|b...D.D...b|b...D.|..A|\n\
|....|.|....|....|.|...|\n\
|....|.|....|....|.|l..|\n\
|-GG-|+|-GG-|-GG-|.|-D-|\n\
|................+.....|\n\
|................|--D--|\n\
|................|...bb|\n\
|................g.....|\n\
|-GGGGGG-|.......g....T|\n\
|..cc6c..g.......|.....|\n\
|..ch.c..|-GGDGG-|-GGG-|\n\
|........g.............|\n\
|^.......|.............|\n\
|-GGG+GG-|.............|\n\
|ddd.....|.............|\n\
|.hd.....+....|-G+GGGG-|\n\
|........|...x|.......c|\n\
|.......r|-DD-|l......S|\n\
|ddd....r|...6|l......c|\n\
|.hd....r|....|........|\n\
|........|....|..cxcC..|\n\
|--------|-..-|--------|\n",
                                               mapf::basic_bind("b l A r d C h 6 x g G , . - | + D t c S T", t_rock_floor, t_rock_floor, t_floor,
                                                       t_rock_floor, t_rock_floor, t_centrifuge, t_rock_floor, t_console, t_console_broken,
                                                       t_reinforced_glass_v, t_reinforced_glass_h, t_floor_blue, t_rock_floor, t_concrete_h, t_concrete_v,
                                                       t_door_metal_c, t_door_metal_locked, t_rock_floor, t_rock_floor, t_rock_floor, t_rock_floor),
                                               mapf::basic_bind("b l A r d C h 6 x g G , . - | + D t c S T", f_bed,        f_locker,     f_crate_c,
                                                       f_rack,       f_desk,       f_null,       f_chair,      f_null,    f_null,           f_null,
                                                       f_null,               f_null,       f_null,       f_null,       f_null,       f_null,
                                                       f_null,              f_table,      f_counter,    f_sink,       f_toilet));
                    for (int i = 0; i <= 23; i++) {
                        for (int j = 0; j <= 23; j++) {
                            if (this->furn(i, j) == f_locker) {
                                place_items("science", 60,  i,  j, i,  j, false, 0);
                            }
                            if (this->furn(i, j) == f_desk) {
                                place_items("office", 60,  i,  j, i,  j, false, 0);
                            }
                            if (this->furn(i, j) == f_counter) {
                                place_items("office", 40,  i,  j, i,  j, false, 0);
                            }
                            if (this->furn(i, j) == f_rack || this->furn(i, j) == f_crate_c) {
                                place_items("softdrugs", 40,  i,  j, i,  j, false, 0);
                                place_items("harddrugs", 30,  i,  j, i,  j, false, 0);
                            }
                            if (this->furn(i, j) == f_bed) {
                                place_items("bed", 50,  i,  j, i,  j, false, 0);
                            }
                        }
                    }
                    computer *tmpcomp2 = NULL;
                    tmpcomp2 = add_computer(13, 20, _("Prisoner Containment Entrance"), 4);
                    tmpcomp2->add_option(_("UNLOCK ENTRANCE"), COMPACT_UNLOCK, 4);
                    tmpcomp = add_computer(5, 11, _("Containment Control"), 4);
                    tmpcomp->add_option(_("EMERGENCY CONTAINMENT RELEASE"), COMPACT_OPEN, 5);
                    add_trap(19, 19, tr_dissector);
                    item body;
                    body.make_corpse();
                    if (one_in(2)) {
                        add_item(1, 1, body);
                    } else {
                        add_spawn("mon_zombie_shrieker", 1, 1, 1);
                    }
                    if (one_in(2)) {
                        add_item(9, 3, body);
                    } else {
                        add_spawn("mon_zombie_brute", 1, 9, 3);
                    }
                    if (one_in(2)) {
                        add_item(14, 4, body);
                    } else {
                        add_spawn("mon_zombie_child", 1, 14, 4);
                    }
                    if (one_in(2)) {
                        add_item(19, 9, body);
                    } else {
                        add_spawn("mon_zombie_grabber", 1, 19, 9);
                    }
                    if (one_in(2)) {
                        add_spawn("mon_zombie_scientist", rng(1, 2), 12, 14);
                    }
                }
                if (bw == 2) {
                    rotate(2);
                }
                if (rw == 2) {
                    rotate(3);
                }
                if (lw == 2) {
                    rotate(1);
                }
                if (t_above == "lab_stairs" || t_above == "ice_lab_stairs") {
                    int sx, sy;
                    do {
                        sx = rng(lw, SEEX * 2 - 1 - rw);
                        sy = rng(tw, SEEY * 2 - 1 - bw);
                    } while (ter(sx, sy) != t_rock_floor);
                    ter_set(sx, sy, t_stairs_up);
                }

                if (terrain_type == "lab_stairs" || terrain_type == "ice_lab_stairs") {
                    int sx, sy;
                    do {
                        sx = rng(lw, SEEX * 2 - 1 - rw);
                        sy = rng(tw, SEEY * 2 - 1 - bw);
                    } while (ter(sx, sy) != t_rock_floor);
                    ter_set(sx, sy, t_stairs_down);
                }
            } else switch (rng(1, 4)) { // Pick a random lab layout
                case 1: // Cross shaped
                    for (int i = 0; i < SEEX * 2; i++) {
                        for (int j = 0; j < SEEY * 2; j++) {
                            if ((i < lw || i > SEEX * 2 - 1 - rw) ||
                                ((j < SEEY - 1 || j > SEEY) && (i == SEEX - 2 || i == SEEX + 1))) {
                                ter_set(i, j, t_concrete_v);
                            } else if ((j < tw || j > SEEY * 2 - 1 - bw) ||
                                       ((i < SEEX - 1 || i > SEEX) && (j == SEEY - 2 || j == SEEY + 1))) {
                                ter_set(i, j, t_concrete_h);
                            } else {
                                ter_set(i, j, t_rock_floor);
                            }
                        }
                    }
                    if (t_above == "lab_stairs" || t_above == "ice_lab_stairs") {
                        ter_set(rng(SEEX - 1, SEEX), rng(SEEY - 1, SEEY), t_stairs_up);
                    }
                    // Top left
                    if (one_in(2)) {
                        ter_set(SEEX - 2, int(SEEY / 2), t_door_metal_c);
                        science_room(this, lw, tw, SEEX - 3, SEEY - 3, zlevel, 1);
                    } else {
                        ter_set(int(SEEX / 2), SEEY - 2, t_door_metal_c);
                        science_room(this, lw, tw, SEEX - 3, SEEY - 3, zlevel, 2);
                    }
                    // Top right
                    if (one_in(2)) {
                        ter_set(SEEX + 1, int(SEEY / 2), t_door_metal_c);
                        science_room(this, SEEX + 2, tw, SEEX * 2 - 1 - rw, SEEY - 3, zlevel, 3);
                    } else {
                        ter_set(SEEX + int(SEEX / 2), SEEY - 2, t_door_metal_c);
                        science_room(this, SEEX + 2, tw, SEEX * 2 - 1 - rw, SEEY - 3, zlevel, 2);
                    }
                    // Bottom left
                    if (one_in(2)) {
                        ter_set(int(SEEX / 2), SEEY + 1, t_door_metal_c);
                        science_room(this, lw, SEEY + 2, SEEX - 3, SEEY * 2 - 1 - bw, zlevel, 0);
                    } else {
                        ter_set(SEEX - 2, SEEY + int(SEEY / 2), t_door_metal_c);
                        science_room(this, lw, SEEY + 2, SEEX - 3, SEEY * 2 - 1 - bw, zlevel, 1);
                    }
                    // Bottom right
                    if (one_in(2)) {
                        ter_set(SEEX + int(SEEX / 2), SEEY + 1, t_door_metal_c);
                        science_room(this, SEEX + 2, SEEY + 2, SEEX * 2 - 1 - rw, SEEY * 2 - 1 - bw,
                                     zlevel, 0);
                    } else {
                        ter_set(SEEX + 1, SEEY + int(SEEY / 2), t_door_metal_c);
                        science_room(this, SEEX + 2, SEEY + 2, SEEX * 2 - 1 - rw, SEEY * 2 - 1 - bw,
                                     zlevel, 3);
                    }
                    if (rw == 1) {
                        ter_set(SEEX * 2 - 1, SEEY - 1, t_door_metal_c);
                        ter_set(SEEX * 2 - 1, SEEY    , t_door_metal_c);
                    }
                    if (bw == 1) {
                        ter_set(SEEX - 1, SEEY * 2 - 1, t_door_metal_c);
                        ter_set(SEEX    , SEEY * 2 - 1, t_door_metal_c);
                    }
                    if (terrain_type == "lab_stairs" || terrain_type == "ice_lab_stairs") { // Stairs going down
                        std::vector<point> stair_points;
                        if (tw != 0) {
                            stair_points.push_back(point(SEEX - 1, 2));
                            stair_points.push_back(point(SEEX - 1, 2));
                            stair_points.push_back(point(SEEX    , 2));
                            stair_points.push_back(point(SEEX    , 2));
                        }
                        if (rw != 1) {
                            stair_points.push_back(point(SEEX * 2 - 3, SEEY - 1));
                            stair_points.push_back(point(SEEX * 2 - 3, SEEY - 1));
                            stair_points.push_back(point(SEEX * 2 - 3, SEEY    ));
                            stair_points.push_back(point(SEEX * 2 - 3, SEEY    ));
                        }
                        if (bw != 1) {
                            stair_points.push_back(point(SEEX - 1, SEEY * 2 - 3));
                            stair_points.push_back(point(SEEX - 1, SEEY * 2 - 3));
                            stair_points.push_back(point(SEEX    , SEEY * 2 - 3));
                            stair_points.push_back(point(SEEX    , SEEY * 2 - 3));
                        }
                        if (lw != 0) {
                            stair_points.push_back(point(2, SEEY - 1));
                            stair_points.push_back(point(2, SEEY - 1));
                            stair_points.push_back(point(2, SEEY    ));
                            stair_points.push_back(point(2, SEEY    ));
                        }
                        stair_points.push_back(point(int(SEEX / 2)       , SEEY    ));
                        stair_points.push_back(point(int(SEEX / 2)       , SEEY - 1));
                        stair_points.push_back(point(int(SEEX / 2) + SEEX, SEEY    ));
                        stair_points.push_back(point(int(SEEX / 2) + SEEX, SEEY - 1));
                        stair_points.push_back(point(SEEX    , int(SEEY / 2)       ));
                        stair_points.push_back(point(SEEX + 2, int(SEEY / 2)       ));
                        stair_points.push_back(point(SEEX    , int(SEEY / 2) + SEEY));
                        stair_points.push_back(point(SEEX + 2, int(SEEY / 2) + SEEY));
                        rn = rng(0, stair_points.size() - 1);
                        ter_set(stair_points[rn].x, stair_points[rn].y, t_stairs_down);
                    }

                    break;

                case 2: // tic-tac-toe # layout
                    for (int i = 0; i < SEEX * 2; i++) {
                        for (int j = 0; j < SEEY * 2; j++) {
                            if (i < lw || i > SEEX * 2 - 1 - rw || i == SEEX - 4 || i == SEEX + 3) {
                                ter_set(i, j, t_concrete_v);
                            } else if (j < lw || j > SEEY * 2 - 1 - bw || j == SEEY - 4 || j == SEEY + 3) {
                                ter_set(i, j, t_concrete_h);
                            } else {
                                ter_set(i, j, t_rock_floor);
                            }
                        }
                    }
                    if (t_above == "lab_stairs" || t_above == "ice_lab_stairs") {
                        ter_set(SEEX - 1, SEEY - 1, t_stairs_up);
                        ter_set(SEEX    , SEEY - 1, t_stairs_up);
                        ter_set(SEEX - 1, SEEY    , t_stairs_up);
                        ter_set(SEEX    , SEEY    , t_stairs_up);
                    }
                    ter_set(SEEX - rng(0, 1), SEEY - 4, t_door_metal_c);
                    ter_set(SEEX - rng(0, 1), SEEY + 3, t_door_metal_c);
                    ter_set(SEEX - 4, SEEY + rng(0, 1), t_door_metal_c);
                    ter_set(SEEX + 3, SEEY + rng(0, 1), t_door_metal_c);
                    ter_set(SEEX - 4, int(SEEY / 2), t_door_metal_c);
                    ter_set(SEEX + 3, int(SEEY / 2), t_door_metal_c);
                    ter_set(int(SEEX / 2), SEEY - 4, t_door_metal_c);
                    ter_set(int(SEEX / 2), SEEY + 3, t_door_metal_c);
                    ter_set(SEEX + int(SEEX / 2), SEEY - 4, t_door_metal_c);
                    ter_set(SEEX + int(SEEX / 2), SEEY + 3, t_door_metal_c);
                    ter_set(SEEX - 4, SEEY + int(SEEY / 2), t_door_metal_c);
                    ter_set(SEEX + 3, SEEY + int(SEEY / 2), t_door_metal_c);
                    science_room(this, lw, tw, SEEX - 5, SEEY - 5, zlevel, rng(1, 2));
                    science_room(this, SEEX - 3, tw, SEEX + 2, SEEY - 5, zlevel, 2);
                    science_room(this, SEEX + 4, tw, SEEX * 2 - 1 - rw, SEEY - 5, zlevel, rng(2, 3));
                    science_room(this, lw, SEEY - 3, SEEX - 5, SEEY + 2, zlevel, 1);
                    science_room(this, SEEX + 4, SEEY - 3, SEEX * 2 - 1 - rw, SEEY + 2, zlevel, 3);
                    science_room(this, lw, SEEY + 4, SEEX - 5, SEEY * 2 - 1 - bw, zlevel, rng(0, 1));
                    science_room(this, SEEX - 3, SEEY + 4, SEEX + 2, SEEY * 2 - 1 - bw, zlevel, 0);
                    science_room(this, SEEX + 4, SEEX + 4, SEEX * 2 - 1 - rw, SEEY * 2 - 1 - bw,
                                 zlevel, 3 * rng(0, 1));
                    if (rw == 1) {
                        ter_set(SEEX * 2 - 1, SEEY - 1, t_door_metal_c);
                        ter_set(SEEX * 2 - 1, SEEY    , t_door_metal_c);
                    }
                    if (bw == 1) {
                        ter_set(SEEX - 1, SEEY * 2 - 1, t_door_metal_c);
                        ter_set(SEEX    , SEEY * 2 - 1, t_door_metal_c);
                    }
                    if (terrain_type == "lab_stairs" || terrain_type == "ice_lab_stairs") {
                        ter_set(SEEX - 3 + 5 * rng(0, 1), SEEY - 3 + 5 * rng(0, 1), t_stairs_down);
                    }
                    break;

                case 3: // Big room
                    for (int i = 0; i < SEEX * 2; i++) {
                        for (int j = 0; j < SEEY * 2; j++) {
                            if (i < lw || i >= SEEX * 2 - 1 - rw) {
                                ter_set(i, j, t_concrete_v);
                            } else if (j < tw || j >= SEEY * 2 - 1 - bw) {
                                ter_set(i, j, t_concrete_h);
                            } else {
                                ter_set(i, j, t_rock_floor);
                            }
                        }
                    }
                    science_room(this, lw, tw, SEEX * 2 - 1 - rw, SEEY * 2 - 1 - bw,
                                 zlevel, rng(0, 3));
                    if (t_above == "lab_stairs" || t_above == "ice_lab_stairs") {
                        int sx, sy;
                        do {
                            sx = rng(lw, SEEX * 2 - 1 - rw);
                            sy = rng(tw, SEEY * 2 - 1 - bw);
                        } while (ter(sx, sy) != t_rock_floor);
                        ter_set(sx, sy, t_stairs_up);
                    }
                    if (rw == 1) {
                        ter_set(SEEX * 2 - 1, SEEY - 1, t_door_metal_c);
                        ter_set(SEEX * 2 - 1, SEEY    , t_door_metal_c);
                    }
                    if (bw == 1) {
                        ter_set(SEEX - 1, SEEY * 2 - 1, t_door_metal_c);
                        ter_set(SEEX    , SEEY * 2 - 1, t_door_metal_c);
                    }
                    if (terrain_type == "lab_stairs" || terrain_type == "ice_lab_stairs") {
                        int sx, sy;
                        do {
                            sx = rng(lw, SEEX * 2 - 1 - rw);
                            sy = rng(tw, SEEY * 2 - 1 - bw);
                        } while (ter(sx, sy) != t_rock_floor);
                        ter_set(sx, sy, t_stairs_down);
                    }
                    break;

                case 4: // alien containment
                    fill_background(this, t_rock_floor);
                    if (one_in(4)) {
                        mapf::formatted_set_simple(this, 0, 0,
                                                   "\
.....|..|.....|........|\n\
.....|6.|.....|..cxcC..|\n\
.....g..g.....g.......l|\n\
.....g..g.....g.......l|\n\
.....D..g.....|.......S|\n\
-----|..|.....|-GG+-GG-|\n\
.....D..g..............|\n\
.....g..g..............|\n\
.....g..g..............|\n\
.....|..|-GGGG-|.......|\n\
----||+-|,,,,,,|.......|\n\
....+...D,,,,,,g.......+\n\
....|-G-|,,,,,,g.......+\n\
........|,,,,,,|.......|\n\
........|-GGGG-|.......|\n\
.......................|\n\
.........cxc6cc........|\n\
.........ch.h.c........|\n\
.......................|\n\
.ccxcc............ccxcc|\n\
.c.h.c............c.h.c|\n\
.......................|\n\
.......................|\n\
-----------++----------|\n",
                                                   mapf::basic_bind("l A r d C h 6 x g G , . - | + D t c S", t_rock_floor, t_floor,   t_rock_floor,
                                                           t_rock_floor, t_centrifuge, t_rock_floor, t_console, t_console_broken, t_reinforced_glass_v,
                                                           t_reinforced_glass_h, t_floor_blue, t_rock_floor, t_concrete_h, t_concrete_v, t_door_metal_c,
                                                           t_door_metal_locked, t_rock_floor, t_rock_floor, t_rock_floor),
                                                   mapf::basic_bind("l A r d C h 6 x g G , . - | + D t c S", f_locker,     f_crate_c, f_rack,
                                                           f_desk,       f_null,       f_chair,      f_null,    f_null,           f_null,               f_null,
                                                           f_null,       f_null,       f_null,       f_null,       f_null,         f_null ,
                                                           f_table,      f_counter,    f_sink));
                        add_trap(19, 3, tr_dissector);
                        if (one_in(3)) {
                            add_spawn("mon_mi_go", 1, 12, 12);
                        } else {
                            add_spawn("mon_zombie_brute", 1 , 12, 12);
                        }
                        if (one_in(3)) {
                            add_spawn("mon_kreck", 1, 2, 2);
                        }
                        if (one_in(3)) {
                            add_spawn("mon_crawler", 1, 2, 7);
                        } else if (one_in(3)) {
                            add_spawn("mon_shoggoth", 1, 2, 7);
                        }
                        if (one_in(2)) {
                            add_spawn("mon_zombie_scientist", rng(1, 3), 12, 18);
                        }
                        for (int i = 0; i <= 23; i++) {
                            for (int j = 0; j <= 23; j++) {
                                if (this->furn(i, j) == f_counter) {
                                    place_items("office", 30,  i,  j, i,  j, false, 0);
                                } else if (this->furn(i, j) == f_locker) {
                                    place_items("science", 60,  i,  j, i,  j, false, 0);
                                }
                                item body;
                                body.make_corpse();
                                if (one_in(500) && this->ter(i, j) == t_rock_floor) {
                                    add_item(i, j, body);
                                }
                            }
                        }
                        computer *tmpcomp2 = NULL;
                        tmpcomp2 = add_computer(6, 1, _("Containment Terminal"), 4);
                        tmpcomp2->add_option(_("EMERGENCY CONTAINMENT UNLOCK"), COMPACT_UNLOCK, 4);
                        tmpcomp2->add_failure(COMPFAIL_DAMAGE);
                        tmpcomp2->add_failure(COMPFAIL_SHUTDOWN);
                        tmpcomp = add_computer(12, 16, _("Containment Control"), 4);
                        tmpcomp->add_option(_("EMERGENCY CONTAINMENT UNLOCK"), COMPACT_UNLOCK, 4);
                        tmpcomp->add_option(_("EMERGENCY CLEANSE"), COMPACT_DISCONNECT, 7);
                        tmpcomp->add_failure(COMPFAIL_DAMAGE);
                        tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
                        lw = lw > 0 ? 1 : 0; // only a single row (not two) of walls on the left
                    } else if (one_in(3)) { //operations or utility
                        mapf::formatted_set_simple(this, 0, 0,
                                                   "\
.....|...........f.....|\n\
.lll.|...........f.rrrr|\n\
.....+...........H.....|\n\
.ll..|...........fAA..r|\n\
-----|FFFF|---|..fAA..r|\n\
.....f....|...|..f....r|\n\
.pSp.f.PP.|.&.|..fAA..A|\n\
.pSp.f.PP.|.x.|..fAA..A|\n\
.....f.PP.|...|..f.....|\n\
.....H....|-+-|..fFFHFF|\n\
FFHFFf........f........|\n\
.....f........f........+\n\
.....fFFFHFFFFf........+\n\
.......................|\n\
.................|-G-G-|\n\
-------|.........|^....|\n\
AA..A..D.........+.....|\n\
AA.....D.........|..ddd|\n\
AAA....D.........g..dh.|\n\
-------|M........g.....|\n\
A.AA...D.........g.....|\n\
A......D.........|dh...|\n\
.A.AA..D.........|dxd.^|\n\
-------|---++----|-----|\n",
                                                   mapf::basic_bind("M D & P S p l H O f F A r d C h 6 x g G , . - | + D t c ^",
                                                           t_gates_control_concrete, t_door_metal_locked, t_radio_tower, t_generator_broken, t_sewage_pump,
                                                           t_sewage_pipe, t_floor,  t_chaingate_c, t_column, t_chainfence_v, t_chainfence_h, t_floor,
                                                           t_floor, t_floor, t_centrifuge, t_null,  t_console, t_console_broken, t_wall_glass_v,
                                                           t_wall_glass_h, t_rock_blue, t_rock_floor, t_concrete_h, t_concrete_v, t_door_metal_c,
                                                           t_door_metal_locked, t_floor,  t_floor,   t_floor),
                                                   mapf::basic_bind("M D & P S p l H O f F A r d C h 6 x g G , . - | + D t c ^", f_null,
                                                           f_null,              f_null,        f_null,             f_null,        f_null,        f_locker,
                                                           f_null,        f_null,   f_null,         f_null,         f_crate_c, f_rack,  f_desk,  f_null,
                                                           f_chair, f_null,    f_null,           f_null,         f_null,         f_null,      f_null,
                                                           f_null,       f_null,       f_null,         f_null,              f_table,  f_counter,
                                                           f_indoor_plant));
                        for (int i = 0; i <= 23; i++) {
                            for (int j = 0; j <= 23; j++) {
                                if (this->furn(i, j) == f_crate_c) {
                                    if (one_in(2)) {
                                        place_items("robots", 60,  i,  j, i,  j, false, 0);
                                    } else if (one_in(2)) {
                                        place_items("science", 60,  i,  j, i,  j, false, 0);
                                    } else {
                                        place_items("sewage_plant", 30,  i,  j, i,  j, false, 0);
                                    }
                                } else if (this->furn(i, j) == f_locker) {
                                    place_items("cleaning", 60,  i,  j, i,  j, false, 0);
                                } else if (this->furn(i, j) == f_rack) {
                                    place_items("mine_equipment", 30,  i,  j, i,  j, false, 0);
                                }
                                if (one_in(500) && this->ter(i, j) == t_rock_floor) {
                                    add_spawn("mon_zombie", 1, i, j);
                                }
                                item body;
                                body.make_corpse();
                                if (one_in(500) && this->ter(i, j) == t_rock_floor) {
                                    add_item(i, j, body);
                                }
                            }
                        }
                        lw = 0; // no wall on the left
                    } else if (one_in(2)) { //tribute
                        mapf::formatted_set_simple(this, 0, 0,
                                                   "\
%%%%%%%%%|....|%%%%%%%%|\n\
%|-|-|%%%|....|%%%|-|--|\n\
%|T|T|---|....|---|T|.T|\n\
%|.|.|EEE+....+EEE|.|..|\n\
%|=|=|EEE+....+EEE|=|=-|\n\
%|...|EEe|....|eEE|....|\n\
%|...|---|....|---|....|\n\
%|...+............+....|\n\
%|ScS|............|cScS|\n\
-|---|............|----|\n\
.......................|\n\
.......................+\n\
.................w.....+\n\
................www....|\n\
--GGG+GG-|....|-GGGGGG-|\n\
ff.......|....|WWWWWWWW|\n\
...htth..g....gWWWWWWWl|\n\
...htth..g....gWWWcWWWl|\n\
.........+....DWWWcWWWW|\n\
.........g....gWWWCWWWW|\n\
...htth..g....gWWWcWWWW|\n\
...htth..g....gWWWWWWhd|\n\
........^|....|rrrWWdxd|\n\
---------|-++-|--------|\n",
                                                   mapf::basic_bind("D l H O f A r d C h 6 x g G , . - | + D t c ^ w W e E % T S =",
                                                           t_door_metal_locked, t_floor,  t_chaingate_c, t_column, t_floor,  t_floor,   t_floor, t_floor,
                                                           t_centrifuge, t_floor, t_console, t_console_broken, t_reinforced_glass_v, t_reinforced_glass_h,
                                                           t_rock_blue, t_rock_floor, t_concrete_h, t_concrete_v, t_door_metal_c, t_door_metal_locked, t_floor,
                                                           t_floor,   t_floor,        t_water_sh, t_water_dp, t_elevator_control_off, t_elevator, t_rock,
                                                           t_floor,  t_floor, t_door_c),
                                                   mapf::basic_bind("D l H O f A r d C h 6 x g G , . - | + D t c ^ w W e E % T S =", f_null,
                                                           f_locker, f_null,        f_null,   f_fridge, f_crate_c, f_rack,  f_desk,  f_null,       f_chair,
                                                           f_null,    f_null,           f_null,               f_null,               f_null,      f_null,
                                                           f_null,       f_null,       f_null,         f_null,              f_table, f_counter, f_indoor_plant,
                                                           f_null,     f_null,     f_null,                 f_null,     f_null, f_toilet, f_sink,  f_null));
                        for (int i = 0; i <= 23; i++) {
                            for (int j = 0; j <= 23; j++) {
                                if (this->furn(i, j) == f_locker) {
                                    place_items("science", 60,  i,  j, i,  j, false, 0);
                                } else if (this->furn(i, j) == f_fridge) {
                                    place_items("fridge", 50,  i,  j, i,  j, false, 0);
                                }
                                if (one_in(500) && this->ter(i, j) == t_rock_floor) {
                                    add_spawn("mon_zombie", 1, i, j);
                                }
                            }
                        }
                        item body;
                        body.make_corpse();
                        add_item(17, 15, body);
                        add_item(8, 3, body);
                        add_item(10, 3, body);
                        spawn_item(18, 15, "fire_ax");
                        lw = 0; // no wall on the left
                    }
                    else { //analyzer
                        mapf::formatted_set_simple(this, 0, 0,
                                                   "\
.......................|\n\
.......................|\n\
.......................|\n\
.......................|\n\
....|-GGGGGGGGGGG-|....|\n\
....|.............|....|\n\
....g.....&.&.....g....|\n\
....g......,......g....|\n\
....g.....&6&.....g....|\n\
....g.............g....|\n\
....grrr.rrrrr.rrrg....|\n\
....gcxc..cxc..cxcg....+\n\
....gch....h....hcg....+\n\
....|.............|....|\n\
....|-+|..cxc..|+-|....|\n\
....+..g...h...g..+....|\n\
....g..g.......g..g....|\n\
....|..|.......|..|....|\n\
....|-G|GGGGGGG|G-|....|\n\
.......................|\n\
.......................|\n\
.......................|\n\
.......................|\n\
-----------++----------|\n",
                                                   mapf::basic_bind("r d h 6 x g G , . - | + D t c ^ % = &", t_railing_h, t_rock_floor, t_rock_floor,
                                                           t_console, t_console_broken, t_wall_glass_v, t_wall_glass_h, t_floor_blue, t_rock_floor,
                                                           t_concrete_h, t_concrete_v, t_door_metal_c, t_door_metal_locked, t_rock_floor, t_rock_floor,
                                                           t_floor,        t_rock, t_door_c, t_radio_tower),
                                                   mapf::basic_bind("r d h 6 x g G , . - | + D t c ^ % = &", f_null,      f_desk,       f_chair,
                                                           f_null,    f_null,           f_null,         f_null,         f_null,       f_null,       f_null,
                                                           f_null,       f_null,         f_null,              f_table,      f_counter,    f_indoor_plant,
                                                           f_null, f_null,   f_null));
                        for (int i = 0; i <= 23; i++) {
                            for (int j = 0; j <= 23; j++) {
                                if (this->furn(i, j) == f_counter) {
                                    place_items("cubical_office", 30,  i,  j, i,  j, false, 0);
                                }
                                if (one_in(500) && this->ter(i, j) == t_rock_floor) {
                                    add_spawn("mon_zombie", 1, i, j);
                                }
                                item body;
                                body.make_corpse();
                                if (one_in(400) && this->ter(i, j) == t_rock_floor) {
                                    add_item(i, j, body);
                                }
                            }
                        }
                        tmpcomp = add_computer(11, 8, _("Mk IV Algorithmic Data Analyzer"), 4);
                        tmpcomp->add_option(_("Run Decryption Algorithm"), COMPACT_DATA_ANAL, 4);
                        tmpcomp->add_option(_("Upload Data to Melchior"), COMPACT_DISCONNECT, 7);
                        tmpcomp->add_option(_("Access Melchior"), COMPACT_DISCONNECT, 12);
                        tmpcomp->add_failure(COMPFAIL_DAMAGE);
                        tmpcomp->add_failure(COMPFAIL_MANHACKS);
                        tmpcomp->add_failure(COMPFAIL_DESTROY_DATA);
                    }

                    for (int i = 0; i < SEEX * 2; i++) {
                        for (int j = 0; j < SEEY * 2; j++) {
                            if (i < lw || i >= SEEX * 2 - rw + 1) {
                                ter_set(i, j, t_concrete_v);
                            } else if (j < (tw - 1) || j >= SEEY * 2 - bw + 1) {
                                ter_set(i, j, t_concrete_h);
                            }
                        }
                    }
                    if (t_above == "lab_stairs" || t_above == "ice_lab_stairs") {
                        int sx, sy;
                        do {
                            sx = rng(lw, SEEX * 2 - 1 - rw);
                            sy = rng(tw, SEEY * 2 - 1 - bw);
                        } while (ter(sx, sy) != t_rock_floor);
                        ter_set(sx, sy, t_stairs_up);
                    }
                    if (terrain_type == "lab_stairs" || terrain_type == "ice_lab_stairs") {
                        int sx, sy;
                        do {
                            sx = rng(lw, SEEX * 2 - 1 - rw);
                            sy = rng(tw, SEEY * 2 - 1 - bw);
                        } while (ter(sx, sy) != t_rock_floor);
                        ter_set(sx, sy, t_stairs_down);
                    }
                    break;

                }
        }
        // Ants will totally wreck up the place
        tw = 0;
        rw = 0;
        bw = 0;
        lw = 0;
        if (is_ot_type("ants", t_north) && connects_to(t_north, 2)) {
            tw = SEEY;
        }
        if (is_ot_type("ants", t_east) && connects_to(t_east, 3)) {
            rw = SEEX;
        }
        if (is_ot_type("ants", t_south) && connects_to(t_south, 0)) {
            bw = SEEY + 1;
        }
        if (is_ot_type("ants", t_west) && connects_to(t_west, 1)) {
            lw = SEEX + 1;
        }
        if (tw != 0 || rw != 0 || bw != 0 || lw != 0) {
            for (int i = 0; i < SEEX * 2; i++) {
                for (int j = 0; j < SEEY * 2; j++) {
                    if ((i < SEEX * 2 - lw && (!one_in(3) || (j > SEEY - 6 && j < SEEY + 5))) ||
                        (i > rw &&          (!one_in(3) || (j > SEEY - 6 && j < SEEY + 5))) ||
                        (j > tw &&          (!one_in(3) || (i > SEEX - 6 && i < SEEX + 5))) ||
                        (j < SEEY * 2 - bw && (!one_in(3) || (i > SEEX - 6 && i < SEEX + 5)))) {
                        ter_set(i, j, t_rock_floor);
                        if (one_in(5)) {
                            make_rubble(i, j, f_rubble_rock, true, t_rock_floor);
                        }
                    }
                }
            }
        }

        // Slimes pretty much wreck up the place, too, but only underground
        tw = (t_north == "slimepit" ? SEEY     : 0);
        rw = (t_east  == "slimepit" ? SEEX + 1 : 0);
        bw = (t_south == "slimepit" ? SEEY + 1 : 0);
        lw = (t_west  == "slimepit" ? SEEX     : 0);
        if (tw != 0 || rw != 0 || bw != 0 || lw != 0) {
            for (int i = 0; i < SEEX * 2; i++) {
                for (int j = 0; j < SEEY * 2; j++) {
                    if (((j <= tw || i >= rw) && i >= j && (SEEX * 2 - 1 - i) <= j) ||
                        ((j >= bw || i <= lw) && i <= j && (SEEY * 2 - 1 - j) <= i)   ) {
                        if (one_in(5)) {
                            make_rubble(i, j, f_rubble_rock, true, t_slime);
                        } else if (!one_in(5)) {
                            ter_set(i, j, t_slime);
                        }
                    }
                }
            }
        }


    } else if (terrain_type == "lab_finale" ||
               terrain_type == "ice_lab_finale") {

        if (is_ot_type("ice_lab", terrain_type)) {
            ice_lab = true;
        } else {
            ice_lab = false;
        }

        if ( ice_lab ) {
            int temperature = -20 + 30 * zlevel;
            set_temperature(x, y, temperature);

            tw = is_ot_type("ice_lab", t_north) ? 0 : 2;
            rw = is_ot_type("ice_lab", t_east) ? 1 : 2;
            bw = is_ot_type("ice_lab", t_south) ? 1 : 2;
            lw = is_ot_type("ice_lab", t_west) ? 0 : 2;
        } else {
            tw = is_ot_type("lab", t_north) ? 0 : 2;
            rw = is_ot_type("lab", t_east) ? 1 : 2;
            bw = is_ot_type("lab", t_south) ? 1 : 2;
            lw = is_ot_type("lab", t_west) ? 0 : 2;
        }

        // Start by setting up a large, empty room.
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i < lw || i > SEEX * 2 - 1 - rw) {
                    ter_set(i, j, t_concrete_v);
                } else if (j < tw || j > SEEY * 2 - 1 - bw) {
                    ter_set(i, j, t_concrete_h);
                } else {
                    ter_set(i, j, t_floor);
                }
            }
        }
        if (rw == 1) {
            ter_set(SEEX * 2 - 1, SEEY - 1, t_door_metal_c);
            ter_set(SEEX * 2 - 1, SEEY    , t_door_metal_c);
        }
        if (bw == 1) {
            ter_set(SEEX - 1, SEEY * 2 - 1, t_door_metal_c);
            ter_set(SEEX    , SEEY * 2 - 1, t_door_metal_c);
        }

        switch (rng(1, 7)) {
        case 1:
        case 2: // Weapons testing
            add_spawn("mon_secubot", 1,            6,            6);
            add_spawn("mon_secubot", 1, SEEX * 2 - 7,            6);
            add_spawn("mon_secubot", 1,            6, SEEY * 2 - 7);
            add_spawn("mon_secubot", 1, SEEX * 2 - 7, SEEY * 2 - 7);
            add_trap(SEEX - 2, SEEY - 2, tr_dissector);
            add_trap(SEEX + 1, SEEY - 2, tr_dissector);
            add_trap(SEEX - 2, SEEY + 1, tr_dissector);
            add_trap(SEEX + 1, SEEY + 1, tr_dissector);
            if (!one_in(3)) {
                spawn_item(SEEX - 1, SEEY - 1, "laser_pack", dice(4, 3));
                spawn_item(SEEX    , SEEY - 1, "UPS_off");
                spawn_item(SEEX    , SEEY - 1, "battery", dice(4, 3));
                spawn_item(SEEX - 1, SEEY    , "v29");
                spawn_item(SEEX - 1, SEEY    , "laser_rifle", dice (1, 0));
                spawn_item(SEEX    , SEEY    , "ftk93");
                spawn_item(SEEX - 1, SEEY    , "recipe_atomic_battery");
                spawn_item(SEEX    , SEEY  -1, "solar_panel_v3"); //quantum solar panel, 5 panels in one!
            } else if (!one_in(3)) {
                spawn_item(SEEX - 1, SEEY - 1, "mininuke", dice(3, 6));
                spawn_item(SEEX    , SEEY - 1, "mininuke", dice(3, 6));
                spawn_item(SEEX - 1, SEEY    , "mininuke", dice(3, 6));
                spawn_item(SEEX    , SEEY    , "mininuke", dice(3, 6));
                spawn_item(SEEX    , SEEY    , "recipe_atomic_battery");
                spawn_item(SEEX    , SEEY    , "solar_panel_v3"); //quantum solar panel, 5 panels in one!
            }  else if (!one_in(3)) {
                spawn_item(SEEX - 1, SEEY - 1, "rm13_armor");
                spawn_item(SEEX    , SEEY - 1, "plut_cell");
                spawn_item(SEEX - 1, SEEY    , "plut_cell");
                spawn_item(SEEX    , SEEY    , "recipe_caseless");
            } else {
                furn_set(SEEX - 2, SEEY - 1, f_rack);
                furn_set(SEEX - 1, SEEY - 1, f_rack);
                furn_set(SEEX    , SEEY - 1, f_rack);
                furn_set(SEEX + 1, SEEY - 1, f_rack);
                furn_set(SEEX - 2, SEEY    , f_rack);
                furn_set(SEEX - 1, SEEY    , f_rack);
                furn_set(SEEX    , SEEY    , f_rack);
                furn_set(SEEX + 1, SEEY    , f_rack);
                place_items("ammo", 96, SEEX - 2, SEEY - 1, SEEX + 1, SEEY - 1, false, 0);
                place_items("allguns", 96, SEEX - 2, SEEY, SEEX + 1, SEEY, false, 0);
                spawn_item(SEEX + 1, SEEY    , "solar_panel_v3"); //quantum solar panel, 5 panels in one!
            }
            break;
        case 3:
        case 4: { // Netherworld access
            bool monsters_end = false;
            if (!one_in(4)) { // Trapped netherworld monsters
                monsters_end = true;
                std::string nethercreatures[11] = { "mon_flying_polyp", "mon_hunting_horror",
                                                    "mon_mi_go", "mon_yugg", "mon_gelatin",
                                                    "mon_flaming_eye", "mon_kreck", "mon_gracke",
                                                    "mon_blank", "mon_gozu", "mon_shoggoth" };
                tw = rng(SEEY + 3, SEEY + 5);
                bw = tw + 4;
                lw = rng(SEEX - 6, SEEX - 2);
                rw = lw + 6;
                for (int i = lw; i <= rw; i++) {
                    for (int j = tw; j <= bw; j++) {
                        if (j == tw || j == bw) {
                            if ((i - lw) % 2 == 0) {
                                ter_set(i, j, t_concrete_h);
                            } else {
                                ter_set(i, j, t_reinforced_glass_h);
                            }
                        } else if ((i - lw) % 2 == 0) {
                            ter_set(i, j, t_concrete_v);
                        } else if (j == tw + 2) {
                            ter_set(i, j, t_concrete_h);
                        } else { // Empty space holds monsters!
                            std::string type = nethercreatures[(rng(0, 10))];
                            add_spawn(type, 1, i, j);
                        }
                    }
                }
            }
            tmpcomp = add_computer(SEEX, 8, _("Sub-prime contact console"), 7);
            if(monsters_end) { //only add these options when there are monsters.
                tmpcomp->add_option(_("Terminate Specimens"), COMPACT_TERMINATE, 2);
                tmpcomp->add_option(_("Release Specimens"), COMPACT_RELEASE, 3);
            }
            tmpcomp->add_option(_("Toggle Portal"), COMPACT_PORTAL, 8);
            tmpcomp->add_option(_("Activate Resonance Cascade"), COMPACT_CASCADE, 10);
            tmpcomp->add_failure(COMPFAIL_MANHACKS);
            tmpcomp->add_failure(COMPFAIL_SECUBOTS);
            ter_set(SEEX - 2, 4, t_radio_tower);
            ter_set(SEEX + 1, 4, t_radio_tower);
            ter_set(SEEX - 2, 7, t_radio_tower);
            ter_set(SEEX + 1, 7, t_radio_tower);
        }
        break;
        case 5:
        case 6: { // Bionics
            add_spawn("mon_secubot", 1,            6,            6);
            add_spawn("mon_secubot", 1, SEEX * 2 - 7,            6);
            add_spawn("mon_secubot", 1,            6, SEEY * 2 - 7);
            add_spawn("mon_secubot", 1, SEEX * 2 - 7, SEEY * 2 - 7);
            add_trap(SEEX - 2, SEEY - 2, tr_dissector);
            add_trap(SEEX + 1, SEEY - 2, tr_dissector);
            add_trap(SEEX - 2, SEEY + 1, tr_dissector);
            add_trap(SEEX + 1, SEEY + 1, tr_dissector);
            square_furn(this, f_counter, SEEX - 1, SEEY - 1, SEEX, SEEY);
            int item_count = 0;
            while (item_count < 5) {
                item_count += place_items("bionics", 75, SEEX - 1, SEEY - 1, SEEX, SEEY, false, 0);
            }
            line(this, t_reinforced_glass_h, SEEX - 2, SEEY - 2, SEEX + 1, SEEY - 2);
            line(this, t_reinforced_glass_h, SEEX - 2, SEEY + 1, SEEX + 1, SEEY + 1);
            line(this, t_reinforced_glass_v, SEEX - 2, SEEY - 1, SEEX - 2, SEEY);
            line(this, t_reinforced_glass_v, SEEX + 1, SEEY - 1, SEEX + 1, SEEY);
            ter_set(SEEX - 3, SEEY - 3, t_console);
            tmpcomp = add_computer(SEEX - 3, SEEY - 3, _("Bionic access"), 3);
            tmpcomp->add_option(_("Manifest"), COMPACT_LIST_BIONICS, 0);
            tmpcomp->add_option(_("Open Chambers"), COMPACT_RELEASE, 5);
            tmpcomp->add_failure(COMPFAIL_MANHACKS);
            tmpcomp->add_failure(COMPFAIL_SECUBOTS);
            }
        break;

        case 7: // CVD Forge
            add_spawn("mon_secubot", 1,            6,            6);
            add_spawn("mon_secubot", 1, SEEX * 2 - 7,            6);
            add_spawn("mon_secubot", 1,            6, SEEY * 2 - 7);
            add_spawn("mon_secubot", 1, SEEX * 2 - 7, SEEY * 2 - 7);
            line(this, t_cvdbody, SEEX - 2, SEEY - 2, SEEX - 2, SEEY + 1);
            line(this, t_cvdbody, SEEX - 1, SEEY - 2, SEEX - 1, SEEY + 1);
            line(this, t_cvdbody, SEEX    , SEEY - 1, SEEX    , SEEY + 1);
            line(this, t_cvdbody, SEEX + 1, SEEY - 2, SEEX + 1, SEEY + 1);
            ter_set(SEEX   , SEEY - 2, t_cvdmachine);
            break;
        }

    } else if (terrain_type == "temple" ||
               terrain_type == "temple_stairs") {

        if (zlevel == 0) { // Ground floor
            // TODO: More varieties?
            fill_background(this, t_dirt);
            square(this, t_grate, SEEX - 1, SEEY - 1, SEEX, SEEX);
            ter_set(SEEX + 1, SEEY + 1, t_pedestal_temple);
        } else { // Underground!  Shit's about to get interesting!
            // Start with all rock floor
            square(this, t_rock_floor, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1);
            // We always start at the south and go north.
            // We use (y / 2 + z) % 4 to guarantee that rooms don't repeat.
            switch (1 + abs(abs_sub.y / 2 + zlevel + 4) % 4) {// TODO: More varieties!

            case 1: // Flame bursts
                square(this, t_rock, 0, 0, SEEX - 1, SEEY * 2 - 1);
                square(this, t_rock, SEEX + 2, 0, SEEX * 2 - 1, SEEY * 2 - 1);
                for (int i = 2; i < SEEY * 2 - 4; i++) {
                    add_field(SEEX    , i, fd_fire_vent, rng(1, 3));
                    add_field(SEEX + 1, i, fd_fire_vent, rng(1, 3));
                }
                break;

            case 2: // Spreading water
                square(this, t_water_dp, 4, 4, 5, 5);
                add_spawn("mon_sewer_snake", 1, 4, 4);

                square(this, t_water_dp, SEEX * 2 - 5, 4, SEEX * 2 - 4, 6);
                add_spawn("mon_sewer_snake", 1, SEEX * 2 - 5, 4);

                square(this, t_water_dp, 4, SEEY * 2 - 5, 6, SEEY * 2 - 4);

                square(this, t_water_dp, SEEX * 2 - 5, SEEY * 2 - 5, SEEX * 2 - 4,
                       SEEY * 2 - 4);

                square(this, t_rock, 0, SEEY * 2 - 2, SEEX - 1, SEEY * 2 - 1);
                square(this, t_rock, SEEX + 2, SEEY * 2 - 2, SEEX * 2 - 1, SEEY * 2 - 1);
                line(this, t_grate, SEEX, 1, SEEX + 1, 1); // To drain the water
                add_trap(SEEX, SEEY * 2 - 2, tr_temple_flood);
                add_trap(SEEX + 1, SEEY * 2 - 2, tr_temple_flood);
                for (int y = 2; y < SEEY * 2 - 2; y++) {
                    for (int x = 2; x < SEEX * 2 - 2; x++) {
                        if (ter(x, y) == t_rock_floor && one_in(4)) {
                            add_trap(x, y, tr_temple_flood);
                        }
                    }
                }
                break;

            case 3: { // Flipping walls puzzle
                line(this, t_rock, 0, 0, SEEX - 1, 0);
                line(this, t_rock, SEEX + 2, 0, SEEX * 2 - 1, 0);
                line(this, t_rock, SEEX - 1, 1, SEEX - 1, 6);
                line(this, t_bars, SEEX + 2, 1, SEEX + 2, 6);
                ter_set(14, 1, t_switch_rg);
                ter_set(15, 1, t_switch_gb);
                ter_set(16, 1, t_switch_rb);
                ter_set(17, 1, t_switch_even);
                // Start with clear floors--then work backwards to the starting state
                line(this, t_floor_red,   SEEX, 1, SEEX + 1, 1);
                line(this, t_floor_green, SEEX, 2, SEEX + 1, 2);
                line(this, t_floor_blue,  SEEX, 3, SEEX + 1, 3);
                line(this, t_floor_red,   SEEX, 4, SEEX + 1, 4);
                line(this, t_floor_green, SEEX, 5, SEEX + 1, 5);
                line(this, t_floor_blue,  SEEX, 6, SEEX + 1, 6);
                // Now, randomly choose actions
                // Set up an actions vector so that there's not undue repetion
                std::vector<int> actions;
                actions.push_back(1);
                actions.push_back(2);
                actions.push_back(3);
                actions.push_back(4);
                actions.push_back(rng(1, 3));
                while (!actions.empty()) {
                    int index = rng(0, actions.size() - 1);
                    int action = actions[index];
                    actions.erase(actions.begin() + index);
                    for (int y = 1; y < 7; y++) {
                        for (int x = SEEX; x <= SEEX + 1; x++) {
                            switch (action) {
                            case 1: // Toggle RG
                                if (ter(x, y) == t_floor_red) {
                                    ter_set(x, y, t_rock_red);
                                } else if (ter(x, y) == t_rock_red) {
                                    ter_set(x, y, t_floor_red);
                                } else if (ter(x, y) == t_floor_green) {
                                    ter_set(x, y, t_rock_green);
                                } else if (ter(x, y) == t_rock_green) {
                                    ter_set(x, y, t_floor_green);
                                }
                                break;
                            case 2: // Toggle GB
                                if (ter(x, y) == t_floor_blue) {
                                    ter_set(x, y, t_rock_blue);
                                } else if (ter(x, y) == t_rock_blue) {
                                    ter_set(x, y, t_floor_blue);
                                } else if (ter(x, y) == t_floor_green) {
                                    ter_set(x, y, t_rock_green);
                                } else if (ter(x, y) == t_rock_green) {
                                    ter_set(x, y, t_floor_green);
                                }
                                break;
                            case 3: // Toggle RB
                                if (ter(x, y) == t_floor_blue) {
                                    ter_set(x, y, t_rock_blue);
                                } else if (ter(x, y) == t_rock_blue) {
                                    ter_set(x, y, t_floor_blue);
                                } else if (ter(x, y) == t_floor_red) {
                                    ter_set(x, y, t_rock_red);
                                } else if (ter(x, y) == t_rock_red) {
                                    ter_set(x, y, t_floor_red);
                                }
                                break;
                            case 4: // Toggle Even
                                if (y % 2 == 0) {
                                    if (ter(x, y) == t_floor_blue) {
                                        ter_set(x, y, t_rock_blue);
                                    } else if (ter(x, y) == t_rock_blue) {
                                        ter_set(x, y, t_floor_blue);
                                    } else if (ter(x, y) == t_floor_red) {
                                        ter_set(x, y, t_rock_red);
                                    } else if (ter(x, y) == t_rock_red) {
                                        ter_set(x, y, t_floor_red);
                                    } else if (ter(x, y) == t_floor_green) {
                                        ter_set(x, y, t_rock_green);
                                    } else if (ter(x, y) == t_rock_green) {
                                        ter_set(x, y, t_floor_green);
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
                square(this, t_rock,        0,            0, SEEX     - 1,            1);
                square(this, t_rock,        0, SEEY * 2 - 2, SEEX     - 1, SEEY * 2 - 1);
                square(this, t_rock,        0,            2, SEEX     - 4, SEEY * 2 - 3);
                square(this, t_rock, SEEX + 2,            0, SEEX * 2 - 1,            1);
                square(this, t_rock, SEEX + 2, SEEY * 2 - 2, SEEX * 2 - 1, SEEY * 2 - 1);
                square(this, t_rock, SEEX + 5,            2, SEEX * 2 - 1, SEEY * 2 - 3);
                int x = rng(SEEX - 1, SEEX + 2), y = 2;
                std::vector<point> path; // Path, from end to start
                while (x < SEEX - 1 || x > SEEX + 2 || y < SEEY * 2 - 2) {
                    path.push_back( point(x, y) );
                    ter_set(x, y, ter_id( rng(t_floor_red, t_floor_blue) ));
                    if (y == SEEY * 2 - 2) {
                        if (x < SEEX - 1) {
                            x++;
                        } else if (x > SEEX + 2) {
                            x--;
                        }
                    } else {
                        std::vector<point> next;
                        for (int nx = x - 1; nx <= x + 1; nx++ ) {
                            for (int ny = y; ny <= y + 1; ny++) {
                                if (ter(nx, ny) == t_rock_floor) {
                                    next.push_back( point(nx, ny) );
                                }
                            }
                        }
                        if(next.empty()) {
                            break;
                        } else {
                            int index = rng(0, next.size() - 1);
                            x = next[index].x;
                            y = next[index].y;
                        }
                    }
                }
                // Now go backwards through path (start to finish), toggling any tiles that need
                bool toggle_red = false, toggle_green = false, toggle_blue = false;
                for (int i = path.size() - 1; i >= 0; i--) {
                    if (ter(path[i].x, path[i].y) == t_floor_red) {
                        toggle_green = !toggle_green;
                        if (toggle_red) {
                            ter_set(path[i].x, path[i].y, t_rock_red);
                        }
                    } else if (ter(path[i].x, path[i].y) == t_floor_green) {
                        toggle_blue = !toggle_blue;
                        if (toggle_green) {
                            ter_set(path[i].x, path[i].y, t_rock_green);
                        }
                    } else if (ter(path[i].x, path[i].y) == t_floor_blue) {
                        toggle_red = !toggle_red;
                        if (toggle_blue) {
                            ter_set(path[i].x, path[i].y, t_rock_blue);
                        }
                    }
                }
                // Finally, fill in the rest with random tiles, and place toggle traps
                for (int i = SEEX - 3; i <= SEEX + 4; i++) {
                    for (int j = 2; j <= SEEY * 2 - 2; j++) {
                        add_trap(i, j, tr_temple_toggle);
                        if (ter(i, j) == t_rock_floor) {
                            ter_set(i, j, ter_id( rng(t_rock_red, t_floor_blue) ));
                        }
                    }
                }
            }
            break;
            } // Done with room type switch
            // Stairs down if we need them
            if (terrain_type == "temple_stairs") {
                line(this, t_stairs_down, SEEX, 0, SEEX + 1, 0);
            }
            // Stairs at the south if t_above has stairs down.
            if (t_above == "temple_stairs") {
                line(this, t_stairs_up, SEEX, SEEY * 2 - 1, SEEX + 1, SEEY * 2 - 1);
            }

        } // Done with underground-only stuff


    } else if (terrain_type == "mine" ||
               terrain_type == "mine_down") {

        if (is_ot_type("mine", t_north)) {
            n_fac = (one_in(10) ? 0 : -2);
        } else {
            n_fac = 4;
        }
        if (is_ot_type("mine", t_east)) {
            e_fac = (one_in(10) ? 0 : -2);
        } else {
            e_fac = 4;
        }
        if (is_ot_type("mine", t_south)) {
            s_fac = (one_in(10) ? 0 : -2);
        } else {
            s_fac = 4;
        }
        if (is_ot_type("mine", t_west)) {
            w_fac = (one_in(10) ? 0 : -2);
        } else {
            w_fac = 4;
        }

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i >= w_fac + rng(0, 2) && i <= SEEX * 2 - 1 - e_fac - rng(0, 2) &&
                    j >= n_fac + rng(0, 2) && j <= SEEY * 2 - 1 - s_fac - rng(0, 2) &&
                    i + j >= 4 && (SEEX * 2 - i) + (SEEY * 2 - j) >= 6  ) {
                    ter_set(i, j, t_rock_floor);
                } else {
                    ter_set(i, j, t_rock);
                }
            }
        }

        if (t_above == "mine_shaft") { // We need the entrance room
            square(this, t_floor, 10, 10, 15, 15);
            line(this, t_wall_h,  9,  9, 16,  9);
            line(this, t_wall_h,  9, 16, 16, 16);
            line(this, t_wall_v,  9, 10,  9, 15);
            line(this, t_wall_v, 16, 10, 16, 15);
            line(this, t_wall_h, 10, 11, 12, 11);
            ter_set(10, 10, t_elevator_control);
            ter_set(11, 10, t_elevator);
            ter_set(10, 12, t_ladder_up);
            line_furn(this, f_counter, 10, 15, 15, 15);
            place_items("mine_equipment", 86, 10, 15, 15, 15, false, 0);
            if (one_in(2)) {
                ter_set(9, 12, t_door_c);
            } else {
                ter_set(16, 12, t_door_c);
            }

        } else { // Not an entrance; maybe some hazards!
            switch( rng(0, 6) ) {
            case 0:
                break; // Nothing!  Lucky!

            case 1: { // Toxic gas
                int cx = rng(9, 14), cy = rng(9, 14);
                ter_set(cx, cy, t_rock);
                add_field(cx, cy, fd_gas_vent, 1);
            }
            break;

            case 2: { // Lava
                int x1 = rng(6, SEEX),                y1 = rng(6, SEEY),
                    x2 = rng(SEEX + 1, SEEX * 2 - 7), y2 = rng(SEEY + 1, SEEY * 2 - 7);
                int num = rng(2, 4);
                for (int i = 0; i < num; i++) {
                    int lx1 = x1 + rng(-1, 1), lx2 = x2 + rng(-1, 1),
                        ly1 = y1 + rng(-1, 1), ly2 = y2 + rng(-1, 1);
                    line(this, t_lava, lx1, ly1, lx2, ly2);
                }
            }
            break;

            case 3: { // Wrecked equipment
                int x = rng(9, 14), y = rng(9, 14);
                for (int i = x - 3; i < x + 3; i++) {
                    for (int j = y - 3; j < y + 3; j++) {
                        if (!one_in(4)) {
                            make_rubble(i, j, f_wreckage, true);
                        }
                    }
                }
                place_items("wreckage", 70, x - 3, y - 3, x + 2, y + 2, false, 0);
            }
            break;

            case 4: { // Dead miners
                int num_bodies = rng(4, 8);
                for (int i = 0; i < num_bodies; i++) {
                    int tries = 0;
                    point body;
                    do {
                        body = point(-1, -1);
                        int x = rng(0, SEEX * 2 - 1), y = rng(0, SEEY * 2 - 1);
                        if (move_cost(x, y) == 2) {
                            body = point(x, y);
                        } else {
                            tries++;
                        }
                    } while (body.x == -1 && tries < 10);
                    if (tries < 10) {
                        item miner;
                        miner.make_corpse();
                        add_item(body.x, body.y, miner);
                        place_items("mine_equipment", 60, body.x, body.y, body.x, body.y,
                                    false, 0);
                    }
                }
            }
            break;

            case 5: { // Dark worm!
                int num_worms = rng(1, 5);
                for (int i = 0; i < num_worms; i++) {
                    std::vector<direction> sides;
                    if (n_fac == 6) {
                        sides.push_back(NORTH);
                    }
                    if (e_fac == 6) {
                        sides.push_back(EAST);
                    }
                    if (s_fac == 6) {
                        sides.push_back(SOUTH);
                    }
                    if (w_fac == 6) {
                        sides.push_back(WEST);
                    }
                    if (sides.empty()) {
                        add_spawn("mon_dark_wyrm", 1, SEEX, SEEY);
                        i = num_worms;
                    } else {
                        direction side = sides[rng(0, sides.size() - 1)];
                        point p;
                        switch (side) {
                        case NORTH:
                            p = point(rng(1, SEEX * 2 - 2), rng(1, 5)           );
                            break;
                        case EAST:
                            p = point(SEEX * 2 - rng(2, 6), rng(1, SEEY * 2 - 2));
                            break;
                        case SOUTH:
                            p = point(rng(1, SEEX * 2 - 2), SEEY * 2 - rng(2, 6));
                            break;
                        case WEST:
                            p = point(rng(1, 5)           , rng(1, SEEY * 2 - 2));
                            break;
                        default:
                            break;
                        }
                        ter_set(p.x, p.y, t_rock_floor);
                        add_spawn("mon_dark_wyrm", 1, p.x, p.y);
                    }
                }
            }
            break;

            case 6: { // Spiral
                int orx = rng(SEEX - 4, SEEX), ory = rng(SEEY - 4, SEEY);
                line(this, t_rock, orx    , ory    , orx + 5, ory    );
                line(this, t_rock, orx + 5, ory    , orx + 5, ory + 5);
                line(this, t_rock, orx + 1, ory + 5, orx + 5, ory + 5);
                line(this, t_rock, orx + 1, ory + 2, orx + 1, ory + 4);
                line(this, t_rock, orx + 1, ory + 2, orx + 3, ory + 2);
                ter_set(orx + 3, ory + 3, t_rock);
                item miner;
                miner.make_corpse();
                add_item(orx + 2, ory + 3, miner);
                place_items("mine_equipment", 60, orx + 2, ory + 3, orx + 2, ory + 3,
                            false, 0);
            }
            break;
            }
        }

        if (terrain_type == "mine_down") { // Don't forget to build a slope down!
            std::vector<direction> open;
            if (n_fac == 4) {
                open.push_back(NORTH);
            }
            if (e_fac == 4) {
                open.push_back(EAST);
            }
            if (s_fac == 4) {
                open.push_back(SOUTH);
            }
            if (w_fac == 4) {
                open.push_back(WEST);
            }

            if (open.empty()) { // We'll have to build it in the center
                int tries = 0;
                point p;
                bool okay = true;
                do {
                    p.x = rng(SEEX - 6, SEEX + 1);
                    p.y = rng(SEEY - 6, SEEY + 1);
                    okay = true;
                    for (int i = p.x; i <= p.x + 5 && okay; i++) {
                        for (int j = p.y; j <= p.y + 5 && okay; j++) {
                            if (ter(i, j) != t_rock_floor) {
                                okay = false;
                            }
                        }
                    }
                    if (!okay) {
                        tries++;
                    }
                } while (!okay && tries < 10);
                if (tries == 10) { // Clear the area around the slope down
                    square(this, t_rock_floor, p.x, p.y, p.x + 5, p.y + 5);
                }
                square(this, t_slope_down, p.x + 1, p.y + 1, p.x + 2, p.y + 2);

            } else { // We can build against a wall
                direction side = open[rng(0, open.size() - 1)];
                switch (side) {
                case NORTH:
                    square(this, t_rock_floor, SEEX - 3, 6, SEEX + 2, SEEY);
                    line(this, t_slope_down, SEEX - 2, 6, SEEX + 1, 6);
                    break;
                case EAST:
                    square(this, t_rock_floor, SEEX + 1, SEEY - 3, SEEX * 2 - 7, SEEY + 2);
                    line(this, t_slope_down, SEEX * 2 - 7, SEEY - 2, SEEX * 2 - 7, SEEY + 1);
                    break;
                case SOUTH:
                    square(this, t_rock_floor, SEEX - 3, SEEY + 1, SEEX + 2, SEEY * 2 - 7);
                    line(this, t_slope_down, SEEX - 2, SEEY * 2 - 7, SEEX + 1, SEEY * 2 - 7);
                    break;
                case WEST:
                    square(this, t_rock_floor, 6, SEEY - 3, SEEX, SEEY + 2);
                    line(this, t_slope_down, 6, SEEY - 2, 6, SEEY + 1);
                    break;
                default:
                    break;
                }
            }
        } // Done building a slope down

        if (t_above == "mine_down") {  // Don't forget to build a slope up!
            std::vector<direction> open;
            if (n_fac == 6 && ter(SEEX, 6) != t_slope_down) {
                open.push_back(NORTH);
            }
            if (e_fac == 6 && ter(SEEX * 2 - 7, SEEY) != t_slope_down) {
                open.push_back(EAST);
            }
            if (s_fac == 6 && ter(SEEX, SEEY * 2 - 7) != t_slope_down) {
                open.push_back(SOUTH);
            }
            if (w_fac == 6 && ter(6, SEEY) != t_slope_down) {
                open.push_back(WEST);
            }

            if (open.empty()) { // We'll have to build it in the center
                int tries = 0;
                point p;
                bool okay = true;
                do {
                    p.x = rng(SEEX - 6, SEEX + 1);
                    p.y = rng(SEEY - 6, SEEY + 1);
                    okay = true;
                    for (int i = p.x; i <= p.x + 5 && okay; i++) {
                        for (int j = p.y; j <= p.y + 5 && okay; j++) {
                            if (ter(i, j) != t_rock_floor) {
                                okay = false;
                            }
                        }
                    }
                    if (!okay) {
                        tries++;
                    }
                } while (!okay && tries < 10);
                if (tries == 10) { // Clear the area around the slope down
                    square(this, t_rock_floor, p.x, p.y, p.x + 5, p.y + 5);
                }
                square(this, t_slope_up, p.x + 1, p.y + 1, p.x + 2, p.y + 2);

            } else { // We can build against a wall
                direction side = open[rng(0, open.size() - 1)];
                switch (side) {
                case NORTH:
                    line(this, t_slope_up, SEEX - 2, 6, SEEX + 1, 6);
                    break;
                case EAST:
                    line(this, t_slope_up, SEEX * 2 - 7, SEEY - 2, SEEX * 2 - 7, SEEY + 1);
                    break;
                case SOUTH:
                    line(this, t_slope_up, SEEX - 2, SEEY * 2 - 7, SEEX + 1, SEEY * 2 - 7);
                    break;
                case WEST:
                    line(this, t_slope_up, 6, SEEY - 2, 6, SEEY + 1);
                    break;
                default:
                    break;
                }
            }
        } // Done building a slope up


    } else if (is_ot_type("station_radio", terrain_type)) {

        // Init to grass & dirt;
        dat.fill_groundcover();
        //Eventually the northern shed will house the main breaker or generator that must be activated prior to transmitting.
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
                        \n\
        FffffffffffffF  \n\
        F____________F  \n\
   |----|______&&&&__F  \n\
   |....=______&&&&__F  \n\
   |x.ll|______&&&&__F  \n\
   |----|______&&&&__F  \n\
        F____________F  \n\
 |--------|__________G  \n\
 |tS|eSc.r|__________F  \n\
 w..+.....=__________F  \n\
 |-----|..|----------|  \n\
 |..doo|..|..dW..h...|  \n\
 w..h..|..D.hxW.c6c..|  \n\
 |a....|..|...+......|  \n\
 |--+--|..|-----WWW--|  \n\
 |.+.................|  \n\
 |l|..............ch.|  \n\
 |-|+--|--+--|....c..|  \n\
 |o....|....o|--==-w-|  \n\
 |o.d..|..d.o|  ss      \n\
 |o.h..|..h..|  ss      \n\
 |-www-|-www-|  ss      \n\
                ss      \n",
                                   mapf::basic_bind(". - | 6 a r + = D W w t S e o h c d x l F f _ & G s", t_floor, t_wall_h, t_wall_v,
                                           t_console, t_floor,    t_floor,    t_door_c, t_door_locked_alarm, t_door_locked, t_window,
                                           t_window_alarm, t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor,
                                           t_console_broken, t_floor,  t_chainfence_v, t_chainfence_h, t_pavement, t_radio_tower,
                                           t_chaingate_l, t_sidewalk),
                                   mapf::basic_bind(". - | 6 a r + = D W w t S e o h c d x l F f _ & G s", f_null,  f_null,   f_null,
                                           f_null,    f_armchair, f_trashcan, f_null,   f_null,              f_null,        f_null,   f_null,
                                           f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_null,           f_locker,
                                           f_null,         f_null,         f_null,     f_null,        f_null,        f_null));
        tmpcomp = add_computer(17, 13, _("Broadcasting Control"), 0);
        tmpcomp->add_option(_("ERROR:  SIGNAL DISCONNECT"), COMPACT_TOWER_UNRESPONSIVE, 0);
        tmpcomp->add_option(_("Install Repeater Mod"), COMPACT_REPEATER_MOD, 0);
        spawn_item(18, 13, "record_weather");
        place_items("novels", 70,  5,  12, 6,  12, false, 0);
        place_items("novels", 70,  2,  21, 2,  19, false, 0);
        place_items("novels", 70,  12,  19, 12,  20, false, 0);
        place_items("fridge", 70,  5,  9, 7,  9, false, 0);
        place_items("fridge", 20,  5,  9, 7,  9, false, 0);
        place_items("fridge", 10,  5,  9, 7,  9, false, 0);
        place_items("cleaning", 70,  2,  16, 2,  17, false, 0);
        place_items("electronics", 80,  6,  5, 7,  5, false, 0);
        if (terrain_type == "station_radio_east") {
            rotate(3);
        }
        if (terrain_type == "station_radio_north") {
            rotate(2);
        }
        if (terrain_type == "station_radio_west") {
            rotate(1);
        }


    } else if (is_ot_type("office_doctor", terrain_type)) {

        // Init to grass & dirt;
        dat.fill_groundcover();
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
                        \n\
   |---|----|--------|  \n\
   |..l|.T.S|..eccScc|  \n\
   |...+....+........D  \n\
   |--------|.......6|r \n\
   |o.......|..|--X--|r \n\
   |d.hd.h..+..|l...6|  \n\
   |o.......|..|l...l|  \n\
   |--------|..|l...l|  \n\
   |l....ccS|..|lllll|  \n\
   |l..t....+..|-----|  \n\
   |l.......|..|....l|  \n\
   |--|-----|..|.t..l|  \n\
   |T.|d.......+....l|  \n\
   |S.|d.h..|..|Scc..|  \n\
   |-+|-ccc-|++|-----|  \n\
   |.................|  \n\
   w....####....####.w  \n\
   w.................w  \n\
   |....####....####.|  \n\
   |.................|  \n\
   |-++--wwww-wwww---|  \n\
     ss                 \n\
     ss                 \n",
                                   mapf::basic_bind(". - | 6 X # r t + = D w T S e o h c d l s", t_floor, t_wall_h, t_wall_v,
                                           t_console, t_door_metal_locked, t_floor, t_floor,    t_floor, t_door_c, t_door_locked_alarm,
                                           t_door_locked, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor,
                                           t_floor,  t_sidewalk),
                                   mapf::basic_bind(". - | 6 X # r t + = D w T S e o h c d l s", f_null,  f_null,   f_null,   f_null,
                                           f_null,              f_bench, f_trashcan, f_table, f_null,   f_null,              f_null,
                                           f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
        tmpcomp = add_computer(20, 4, _("Medical Supply Access"), 2);
        tmpcomp->add_option(_("Lock Door"), COMPACT_LOCK, 2);
        tmpcomp->add_option(_("Unlock Door"), COMPACT_UNLOCK, 2);
        tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
        tmpcomp->add_failure(COMPFAIL_ALARM);

        tmpcomp = add_computer(20, 6, _("Medical Supply Access"), 2);
        tmpcomp->add_option(_("Unlock Door"), COMPACT_UNLOCK, 2);
        tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
        tmpcomp->add_failure(COMPFAIL_ALARM);

        if (one_in(2)) {
            spawn_item(7, 6, "record_patient");
        }
        place_items("dissection", 60,  4,  9, 4,  11, false, 0);
        place_items("dissection", 60,  9,  9, 10,  9, false, 0);
        place_items("dissection", 60,  20,  11, 20,  13, false, 0);
        place_items("dissection", 60,  17,  14, 18,  14, false, 0);
        place_items("fridge", 50,  15,  2, 15,  2, false, 0);
        place_items("surgery", 30,  4,  9, 11,  11, false, 0);
        place_items("surgery", 30,  16,  11, 20, 4, false, 0);
        place_items("harddrugs", 60,  16,  6, 16, 9, false, 0);
        place_items("harddrugs", 60,  17,  9, 19, 9, false, 0);
        place_items("softdrugs", 60,  20,  9, 20, 7, false, 0);
        place_items("cleaning", 50,  4,  2, 6,  3, false, 0);

        if (terrain_type == "office_doctor_east") {
            rotate(3);
        }
        if (terrain_type == "office_doctor_north") {
            rotate(2);
        }
        if (terrain_type == "office_doctor_west") {
            rotate(1);
        }


    } else {
        terrain_type_found = false;
    }

    // MSVC can't handle a single "if/else if" with this many clauses. Hack to
    // break the clause in two so MSVC compiles work, until this file is refactored.
    // "please, shoot me now" - refactorer
    if (!terrain_type_found) {
    if (terrain_type == "farm") {

        if (!one_in(10)) {
            dat.fill_groundcover();
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                  &     \n\
                        \n\
          %%%       %%% \n\
###++### |-w---+-----w-|\n\
#DD____# |uSeuu........|\n\
#D_____# |o............|\n\
#_1__1_# |u..........H.|\n\
#______# |u...|---+----|\n\
#______# |-+-||kh......|\n\
#_1__1_# |...|.........w\n\
#______# |b..+......BB.|\n\
#____ll# |.ST|.....dBBd|\n\
###++### |-w-|----w----|\n\
                        \n\
                        \n\
FFFFFFFFFFFFFFFFFFFFFFFF\n\
,,,,,,,,,,,,,,,,,,,,,,,,\n",
                                       mapf::basic_bind(", F . _ H u e S T o b l # % 1 D + - | w k h B d &", t_dirt, t_fence_barbed, t_floor,
                                               t_dirtfloor, t_floor,    t_floor,    t_floor,  t_floor, t_floor,  t_floor, t_floor,   t_floor,
                                               t_wall_wood, t_shrub, t_column, t_dirtmound, t_door_c, t_wall_h, t_wall_v, t_window_domestic,
                                               t_floor, t_floor, t_floor, t_floor, t_water_pump),
                                       mapf::basic_bind(", F . _ H u e S T o b l # % 1 D + - | w k h B d", f_null, f_null,         f_null,
                                               f_null,      f_armchair, f_cupboard, f_fridge, f_sink,  f_toilet, f_oven,  f_bathtub, f_locker,
                                               f_null,      f_null,  f_null,   f_null,      f_null,   f_null,   f_null,   f_null,
                                               f_desk,  f_chair, f_bed,   f_dresser));
            place_items("fridge", 65, 12, 11, 12, 11, false, 0);
            place_items("kitchen", 70, 10, 15, 14, 11, false, 0);
            place_items("moonshine_brew", 65, 10, 15, 14, 11, false, 0);
            place_items("livingroom", 65, 15, 11, 22, 13, false, 0);
            place_items("dresser", 80, 19, 18, 19, 18, false, 0);
            place_items("dresser", 80, 22, 18, 22, 18, false, 0);
            place_items("bedroom", 65, 15, 15, 22, 18, false, 0);
            place_items("softdrugs", 70, 11, 16, 12, 17, false, 0);
            place_items("bigtools", 50, 1, 11, 6, 18, true, 0);
            place_items("homeguns", 20, 1, 11, 6, 18, true, 0);
            place_items("bed", 60, 20, 21, 17, 18, true, 0);
            if (one_in(2)) {
                add_spawn("mon_zombie", rng(1, 6), 4, 14);
            } else {
                place_spawns("GROUP_DOMESTIC", 2, 10, 15, 12, 17, 1);
            }
        } else {
            dat.fill_groundcover();
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                  &     \n\
                        \n\
          %%%       %%% \n\
###++### |-w---+-----w-|\n\
#DD____# |uSeuu..mm....|\n\
#D_____# |o...........m|\n\
#_1__1_# |u...........m|\n\
#______# |u...|---+----|\n\
#______# |-+-||mm....mm|\n\
#_1__1_# |...|.........w\n\
#______# |b..+......BB.|\n\
#____ll# |.ST|mm...dBBd|\n\
###++### |-w-|----w----|\n\
                        \n\
                        \n\
FFFFFFFFFFFFFFFFFFFFFFFF\n\
,,,,,,,,,,,,,,,,,,,,,,,,\n",
                                       mapf::basic_bind("m , F . _ H u e S T o b l # % 1 D + - | w k h B d &", t_floor,         t_dirt,
                                               t_fence_barbed, t_floor, t_dirtfloor, t_floor,    t_floor,    t_floor,  t_floor, t_floor,  t_floor,
                                               t_floor,   t_floor,  t_wall_wood, t_shrub, t_column, t_dirtmound, t_door_c, t_wall_h, t_wall_v,
                                               t_window_domestic, t_floor, t_floor, t_floor, t_floor, t_water_pump),
                                       mapf::basic_bind("m , F . _ H u e S T o b l # % 1 D + - | w k h B d", f_makeshift_bed, f_null,
                                               f_null,         f_null,  f_null,      f_armchair, f_cupboard, f_fridge, f_sink,  f_toilet, f_oven,
                                               f_bathtub, f_locker, f_null,      f_null,  f_null,   f_null,      f_null,   f_null,   f_null,
                                               f_null,            f_desk,  f_chair, f_bed,   f_dresser));
            place_items("cannedfood", 65, 12, 11, 12, 11, false, 0);
            place_items("bigtools", 50, 1, 11, 6, 18, true, 0);
            place_items("homeguns", 20, 1, 11, 6, 18, true, 0);
            place_items("bed", 60, 20, 21, 17, 18, true, 0);
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_dresser) {
                        place_items("dresser", 50,  i,  j, i,  j, false, 0);
                    }
                    if (this->ter(i, j) == t_floor) {
                        place_items("trash", 20,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_cupboard) {
                        place_items("kitchen", 70,  i,  j, i,  j, false, 0);
                        place_items("softdrugs", 40,  i,  j, i,  j, false, 0);
                        place_items("cannedfood", 40,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_makeshift_bed || this->furn(i, j) == f_bed) {
                        place_items("livingroom", 20,  i,  j, i,  j, false, 0);
                        place_items("survival_armor", 20,  i,  j, i,  j, false, 0);
                        place_items("camping", 20,  i,  j, i,  j, false, 0);
                        place_items("survival_tools", 20,  i,  j, i,  j, false, 0);
                        place_items("bed", 40,  i,  j, i,  j, false, 0);
                    }
                    if ( dat.is_groundcover( this->ter(i, j) ) ) {
                        if (one_in(20)) {
                            add_trap(i, j, tr_beartrap);
                        }
                        if (one_in(20)) {
                            add_trap(i, j, tr_tripwire);
                        }
                        if (one_in(15)) {
                            ter_set(i, j, t_pit);
                        }
                    }
                }
            }
        }


    } else if (terrain_type == "farm_field") {

        //build barn
        if (t_east == "farm") {
            dat.fill_groundcover();
            square(this, t_wall_wood, 3, 3, 20, 20);
            square(this, t_dirtfloor, 4, 4, 19, 19);
            line(this, t_door_metal_locked, 8, 20, 15, 20);
            ter_set(16, 19, t_barndoor);
            ter_set(16, 21, t_barndoor);
            line(this, t_door_metal_locked, 8, 3, 15, 3);
            ter_set(16, 2, t_barndoor);
            ter_set(16, 4, t_barndoor);
            square_furn(this, f_hay, 4, 4, 6, 6);
            line(this, t_fence_h, 4, 8, 6, 8);
            line(this, t_fence_v, 6, 9, 6, 14);
            line(this, t_fence_h, 4, 15, 6, 15);
            line(this, t_fencegate_c, 6, 11, 6, 12);

            line(this, t_fence_h, 17, 8, 19, 8);
            line(this, t_fence_v, 17, 9, 17, 14);
            line(this, t_fence_h, 17, 15, 19, 15);
            line(this, t_fencegate_c, 17, 11, 17, 12);
            line_furn(this, f_locker, 4, 19, 7, 19);
            ter_set(7, 7, t_column);
            ter_set(16, 7, t_column);
            ter_set(7, 16, t_column);
            ter_set(16, 16, t_column);
            ter_set(5, 3, t_window_boarded);
            ter_set(18, 3, t_window_boarded);
            line(this, t_window_boarded, 3, 5, 3, 6);
            line(this, t_window_boarded, 3, 11, 3, 12);
            line(this, t_window_boarded, 3, 17, 3, 18);
            line(this, t_window_boarded, 20, 5, 20, 6);
            line(this, t_window_boarded, 20, 11, 20, 12);
            line(this, t_window_boarded, 20, 17, 20, 18);
            ter_set(5, 20, t_window_boarded);
            ter_set(18, 20, t_window_boarded);

            if(t_south == "farm_field") {
                square(this, t_fence_barbed, 1, 20, 1, 23);
                ter_set(2, 20, t_fence_barbed);
                ter_set(1, 20, t_fence_barbed);
                square(this, t_fence_barbed, 22, 20, 22, 22);
                ter_set(21, 20, t_fence_barbed);
                ter_set(23, 22, t_fence_barbed);
                ter_set(22, 22, t_fence_barbed);
                ter_set(22, 20, t_fence_barbed);
                square(this, t_dirt, 2, 21, 21, 23);
                square(this, t_dirt, 22, 23, 23, 23);
                ter_set(16, 21, t_barndoor);
            }
            place_items("bigtools", 60, 4, 4, 7, 19, true, 0);
            place_items("bigtools", 60, 16, 5, 19, 19, true, 0);
            place_items("mechanics", 40, 8, 4, 15, 19, true, 0);
            place_items("home_hw", 50, 4, 19, 7, 19, true, 0);
            place_items("tools", 50, 4, 19, 7, 19, true, 0);
            for (int x = 4; x <= 6; x++) {
                for (int y = 4; y <= 6; y++) {
                    spawn_item(x, y, "straw_pile", rng(0, 8));
                }
            }
            for (int y = 9; y <= 14; y++) {
                if (one_in(2)) {
                    spawn_item(4, y, "straw_pile", rng(5, 15));
                }
                if (one_in(2)) {
                    spawn_item(19, y, "straw_pile", rng(5, 15));
                }
            }
            if (one_in(3)) {
                add_spawn("mon_zombie", rng(3, 6), 12, 12);
            } else {
                place_spawns("GROUP_DOMESTIC", 2, 0, 0, 15, 15, 1);
            }

        } else {
            fill_background(this, t_grass); // basic lot
            square(this, t_fence_barbed, 1, 1, 22, 22);
            square(this, t_dirt, 2, 2, 21, 21);
            ter_set(1, 1, t_fence_barbed);
            ter_set(22, 1, t_fence_barbed);
            ter_set(1, 22, t_fence_barbed);
            ter_set(22, 22, t_fence_barbed);

            int xStart = 4;
            int xEnd = 19;
            //acidia, connecting fields
            if(t_east == "farm_field") {
                square(this, t_fence_barbed, 22, 1, 23, 22);
                square(this, t_dirt, 21, 2, 23, 21);
                xEnd = 22;
            }
            if(t_west == "farm_field") {
                square(this, t_fence_barbed, 0, 1, 1, 22);
                square(this, t_dirt, 0, 2, 2, 21);
                xStart = 1;
            }
            if(t_south == "farm_field") {
                square(this, t_fence_barbed, 1, 22, 22, 23);
                square(this, t_dirt, 2, 21, 21, 23);
                line(this, t_dirtmound, xStart, 21, xEnd, 21);
                if(t_east == "farm_field") {
                    square(this, t_dirt, 20, 20, 23, 23);
                }
                if(t_west == "farm_field") {
                    square(this, t_dirt, 0, 20, 3, 23);
                }
            }
            if(t_north == "farm_field" || t_north == "farm") {
                square(this, t_fence_barbed, 1, 0, 22, 1);
                square(this, t_dirt, 2, 0, 21, 2);
                line(this, t_dirtmound, xStart, 1, xEnd, 1);
                if(t_east == "farm_field") {
                    square(this, t_dirt, 20, 0, 23, 3);
                }
                if(t_west == "farm_field") {
                    square(this, t_dirt, 0, 0, 3, 3);
                }
            }
            if(t_west == "farm") {
                square(this, t_fence_barbed, 0, 22, 1, 22);
                square(this, t_dirt, 0, 23, 2, 23);
                ter_set(1, 22, t_fence_barbed);
            }
            //standard field
            line(this, t_dirtmound, xStart, 3, xEnd, 3); //Crop rows
            line(this, t_dirtmound, xStart, 5, xEnd, 5);
            line(this, t_dirtmound, xStart, 7, xEnd, 7);
            line(this, t_dirtmound, xStart, 9, xEnd, 9);
            line(this, t_dirtmound, xStart, 11, xEnd, 11);
            line(this, t_dirtmound, xStart, 13, xEnd, 13);
            line(this, t_dirtmound, xStart, 15, xEnd, 15);
            line(this, t_dirtmound, xStart, 17, xEnd, 17);
            line(this, t_dirtmound, xStart, 19, xEnd, 19);

            place_items("hydro", 70, xStart, 3, xEnd, 3, true, turn); //Spawn crops
            place_items("hydro", 70, xStart, 5, xEnd, 5, true, turn);
            place_items("hydro", 70, xStart, 7, xEnd, 7, true, turn);
            place_items("hydro", 70, xStart, 9, xEnd, 9, true, turn);
            place_items("hydro", 70, xStart, 11, xEnd, 11, true, turn);
            place_items("hydro", 70, xStart, 13, xEnd, 13, true, turn);
            place_items("hydro", 70, xStart, 15, xEnd, 15, true, turn);
            place_items("hydro", 70, xStart, 17, xEnd, 17, true, turn);
            place_items("hydro", 70, xStart, 19, xEnd, 19, true, turn);
        }

    } else if (terrain_type == "megastore_entrance") {

        fill_background(this, t_floor);
        // Construct facing north; below, we'll rotate to face road
        line(this, t_wall_glass_h, 0, 0, SEEX * 2 - 1, 0);
        ter_set(SEEX, 0, t_door_glass_c);
        ter_set(SEEX + 1, 0, t_door_glass_c);
        //Vending
        std::vector<int> vset;
        int vnum = rng( 2, 6 );
        for(int a = 0; a < 21; a++ ) {
            vset.push_back(a);
        }
        std::random_shuffle(vset.begin(), vset.end());
        for(int a = 0; a < vnum; a++) {
            if (vset[a] < 12) {
                if (one_in(2)) {
                    place_vending(vset[a], 1, "vending_food");
                } else {
                    place_vending(vset[a], 1, "vending_drink");
                }
            } else {
                if (one_in(2)) {
                    place_vending(vset[a] + 2, 1, "vending_food");
                } else {
                    place_vending(vset[a] + 2, 1, "vending_drink");
                }
            }
        }
        vset.clear();
        // Long checkout lanes
        for (int x = 2; x <= 18; x += 4) {
            line_furn(this, f_counter, x, 4, x, 14);
            line_furn(this, f_rack, x + 3, 4, x + 3, 14);
            place_items("snacks",    80, x + 3, 4, x + 3, 14, false, 0);
            place_items("magazines", 70, x + 3, 4, x + 3, 14, false, 0);
        }
        for (int i = 0; i < 10; i++) {
            int x = rng(0, SEEX * 2 - 1), y = rng(0, SEEY * 2 - 1);
            if (ter(x, y) == t_floor) {
                add_spawn("mon_zombie", 1, x, y);
            }
        }
        // Finally, figure out where the road is; construct our entrance facing that.
        std::vector<direction> faces_road;
        if (is_ot_type("road", t_east) || is_ot_type("bridge", t_east)) {
            rotate(1);
        }
        if (is_ot_type("road", t_south) || is_ot_type("bridge", t_south)) {
            rotate(2);
        }
        if (is_ot_type("road", t_west) || is_ot_type("bridge", t_west)) {
            rotate(3);
        }


    } else if (terrain_type == "megastore") {

        square(this, t_floor, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1);

        // Randomly pick contents
        switch (rng(1, 5)) {
        case 1: { // Groceries
            bool fridge = false;
            for (int x = rng(2, 3); x < SEEX * 2 - 1; x += 3) {
                for (int y = 2; y <= SEEY; y += SEEY - 2) {
                    if (one_in(3)) {
                        fridge = !fridge;
                    }
                    if (fridge) {
                        line_furn(this, f_glass_fridge, x, y, x, y + SEEY - 4);
                        if (one_in(3)) {
                            place_items("fridgesnacks", 80, x, y, x, y + SEEY - 4, false, 0);
                        } else {
                            place_items("fridge",       70, x, y, x, y + SEEY - 4, false, 0);
                        }
                    } else {
                        line_furn(this, f_rack, x, y, x, y + SEEY - 4);
                        if (one_in(3)) {
                            place_items("cannedfood", 78, x, y, x, y + SEEY - 4, false, 0);
                        } else if (one_in(2)) {
                            place_items("pasta",      82, x, y, x, y + SEEY - 4, false, 0);
                        } else if (one_in(2)) {
                            place_items("produce",    65, x, y, x, y + SEEY - 4, false, 0);
                        } else {
                            place_items("snacks",     72, x, y, x, y + SEEY - 4, false, 0);
                        }
                    }
                }
            }
        }
        break;
        case 2: // Hardware
            for (int x = 2; x <= 22; x += 4) {
                line_furn(this, f_rack, x, 4, x, SEEY * 2 - 5);
                if (one_in(3)) {
                    place_items("tools",    70, x, 4, x, SEEY * 2 - 5, false, 0);
                } else if (one_in(2)) {
                    place_items("bigtools", 70, x, 4, x, SEEY * 2 - 5, false, 0);
                } else if (one_in(3)) {
                    place_items("hardware", 70, x, 4, x, SEEY * 2 - 5, false, 0);
                } else {
                    place_items("mischw",   70, x, 4, x, SEEY * 2 - 5, false, 0);
                }
            }
            break;
        case 3: // Clothing
            for (int x = 2; x < SEEX * 2; x += 6) {
                for (int y = 3; y <= 9; y += 6) {
                    square_furn(this, f_rack, x, y, x + 1, y + 1);
                    if (one_in(2)) {
                        place_items("shirts",  75, x, y, x + 1, y + 1, false, 0);
                    } else if (one_in(2)) {
                        place_items("pants",   72, x, y, x + 1, y + 1, false, 0);
                    } else if (one_in(2)) {
                        place_items("jackets", 65, x, y, x + 1, y + 1, false, 0);
                    } else {
                        place_items("winter",  62, x, y, x + 1, y + 1, false, 0);
                    }
                }
            }
            for (int y = 13; y <= SEEY * 2 - 2; y += 3) {
                line_furn(this, f_rack, 2, y, SEEX * 2 - 3, y);
                if (one_in(3)) {
                    place_items("shirts",     75, 2, y, SEEX * 2 - 3, y, false, 0);
                } else if (one_in(2)) {
                    place_items("shoes",      75, 2, y, SEEX * 2 - 3, y, false, 0);
                } else if (one_in(2)) {
                    place_items("bags",       75, 2, y, SEEX * 2 - 3, y, false, 0);
                } else {
                    place_items("allclothes", 75, 2, y, SEEX * 2 - 3, y, false, 0);
                }
            }
            break;
        case 4: // Cleaning and soft drugs and novels and junk
            for (int x = rng(2, 3); x < SEEX * 2 - 1; x += 3) {
                for (int y = 2; y <= SEEY; y += SEEY - 2) {
                    line_furn(this, f_rack, x, y, x, y + SEEY - 4);
                    if (one_in(3)) {
                        place_items("cleaning",  78, x, y, x, y + SEEY - 4, false, 0);
                    } else if (one_in(2)) {
                        place_items("softdrugs", 72, x, y, x, y + SEEY - 4, false, 0);
                    } else {
                        place_items("novels",    84, x, y, x, y + SEEY - 4, false, 0);
                    }
                }
            }
            break;
        case 5: // Sporting goods
            for (int x = rng(2, 3); x < SEEX * 2 - 1; x += 3) {
                for (int y = 2; y <= SEEY; y += SEEY - 2) {
                    line_furn(this, f_rack, x, y, x, y + SEEY - 4);
                    if (one_in(2)) {
                        place_items("sports",  72, x, y, x, y + SEEY - 4, false, 0);
                    } else if (one_in(10)) {
                        place_items("rifles",  20, x, y, x, y + SEEY - 4, false, 0);
                    } else {
                        place_items("camping", 68, x, y, x, y + SEEY - 4, false, 0);
                    }
                }
            }
            break;
        }

        // Add some spawns
        for (int i = 0; i < 15; i++) {
            int x = rng(0, SEEX * 2 - 1), y = rng(0, SEEY * 2 - 1);
            if (ter(x, y) == t_floor) {
                add_spawn("mon_zombie", 1, x, y);
            }
        }
        // Rotate randomly...
        rotate(rng(0, 3));
        // ... then place walls as needed.
        if (t_north != "megastore_entrance" && t_north != "megastore") {
            line(this, t_wall_h, 0, 0, SEEX * 2 - 1, 0);
        }
        if (t_east != "megastore_entrance" && t_east != "megastore") {
            line(this, t_wall_v, SEEX * 2 - 1, 0, SEEX * 2 - 1, SEEY * 2 - 1);
        }
        if (t_south != "megastore_entrance" && t_south != "megastore") {
            line(this, t_wall_h, 0, SEEY * 2 - 1, SEEX * 2 - 1, SEEY * 2 - 1);
        }
        if (t_west != "megastore_entrance" && t_west != "megastore") {
            line(this, t_wall_v, 0, 0, 0, SEEY * 2 - 1);
        }


    } else if (terrain_type == "hospital_entrance") {

        square(this, t_pavement, 0, 0, SEEX * 2 - 1, 5);
        square(this, t_floor, 0, 6, SEEX * 2 - 1, SEEY * 2 - 1);
        // Ambulance parking place
        line(this, t_pavement_y,  5, 1, 22, 1);
        line(this, t_pavement_y,  5, 2,  5, 5);
        line(this, t_pavement_y, 22, 2, 22, 5);
        // Top wall
        line(this, t_wall_h, 0, 6, 6, 6);
        line(this, t_door_glass_c, 3, 6, 4, 6);
        line(this, t_wall_glass_h, 7, 6, 19, 6);
        line(this, t_wall_h, 20, 6, SEEX * 2 - 1, 6);
        // Left wall
        line(this, t_wall_v, 0, 0, 0, SEEY * 2 - 1);
        line(this, t_floor, 0, 11, 0, 12);
        // Waiting area
        line_furn(this, f_bench,  8, 7, 11,  7);
        line_furn(this, f_bench, 13, 7, 17,  7);
        line_furn(this, f_bench, 20, 7, 22,  7);
        line_furn(this, f_bench, 22, 8, 22, 10);
        place_items("magazines", 70, 8, 7, 22, 10, false, 0);
        //Vending
        std::vector<int> vset;
        int vnum = rng(1, 3);
        for(int a = 0; a < 17; a++) {
            vset.push_back(a);
        }
        std::random_shuffle(vset.begin(), vset.end());
        for(int a = 0; a < vnum; a++) {
            if (vset[a] < 3) {
                if (one_in(2)) {
                    place_vending(5 + vset[a], 7, "vending_food");
                } else {
                    place_vending(5 + vset[a], 7, "vending_drink");
                }
            } else if (vset[a] < 5) {
                if (one_in(2)) {
                    place_vending(1 + vset[a] - 3, 7, "vending_food");
                } else {
                    place_vending(1 + vset[a] - 3, 7, "vending_drink");
                }
            } else if (vset[a] < 8) {
                if (one_in(2)) {
                    place_vending(1, 8 + vset[a] - 5, "vending_food");
                } else {
                    place_vending(1, 8 + vset[a] - 5, "vending_drink");
                }
            } else if (vset[a] < 13) {
                if (one_in(2)) {
                    place_vending(10 + vset[a] - 8, 12, "vending_food");
                } else {
                    place_vending(10 + vset[a] - 8, 12, "vending_drink");
                }
            } else {
                if (one_in(2)) {
                    place_vending(17 + vset[a] - 13, 12, "vending_food");
                } else {
                    place_vending(17 + vset[a] - 13, 12, "vending_drink");
                }
            }
        }
        vset.clear();
        // Reception and examination rooms
        line_furn(this, f_counter, 8, 13, 9, 13);
        line(this, t_wall_h, 10, 13, SEEX * 2 - 1, 13);
        line(this, t_door_c, 15, 13, 16, 13);
        line(this, t_wall_h,  8, 17, 13, 17);
        line(this, t_wall_h, 18, 17, 22, 17);
        line(this, t_wall_h,  8, 20, 13, 20);
        line(this, t_wall_h, 18, 20, 22, 20);
        line(this, t_wall_v,  7, 13,  7, 22);
        line(this, t_wall_v, 14, 15, 14, 20);
        line(this, t_wall_v, 17, 14, 17, 22);
        line_furn(this, f_bed,  8, 19,  9, 19);
        place_items("bed", 60, 8, 19, 9, 19, false, 0);
        line_furn(this, f_bed, 21, 19, 22, 19);
        place_items("bed", 60, 21, 19, 22, 19, false, 0);
        line_furn(this, f_bed, 21, 22, 22, 22);
        place_items("bed", 60, 21, 22, 22, 22, false, 0);
        line_furn(this, f_rack, 18, 14, 22, 14);
        place_items("harddrugs", 80, 18, 14, 22, 14, false, 0);
        line_furn(this, f_rack, 8, 21, 8, 22);
        place_items("softdrugs", 70, 8, 21, 8, 22, false, 0);
        ter_set(14, rng(18, 19), t_door_c);
        ter_set(17, rng(15, 16), t_door_locked); // Hard drugs room is locked
        ter_set(17, rng(18, 19), t_door_c);
        ter_set(17, rng(21, 22), t_door_c);
        // ER and bottom wall
        line(this, t_wall_h, 0, 16, 6, 16);
        line(this, t_door_c, 3, 16, 4, 16);
        square_furn(this, f_bed, 3, 19, 4, 20);
        place_items("bed", 60, 3, 19, 4, 20, false, 0);
        line_furn(this, f_counter, 1, 22, 6, 22);
        place_items("surgery", 78, 1, 22, 6, 22, false, 0);
        line(this, t_wall_h, 1, 23, 22, 23);
        line(this, t_floor, 11, 23, 12, 23);
        // Right side wall (needed sometimes!)
        line(this, t_wall_v, 23,  0, 23, 10);
        line(this, t_wall_v, 23, 13, 23, 23);

        /*
        // Generate bodies / zombies
          rn = rng(10, 15);
          for (int i = 0; i < rn; i++) {
           item body;
           body.make_corpse();
           int zx = rng(0, SEEX * 2 - 1), zy = rng(0, SEEY * 2 - 1);
           if (ter(zx, zy) == t_bed || one_in(3))
            add_item(zx, zy, body);
           else if (move_cost(zx, zy) > 0) {
            mon_id zom = mon_zombie;
            if (one_in(6))
             zom = mon_zombie_spitter;
            else if (!one_in(3))
             zom = mon_boomer;
            add_spawn(zom, 1, zx, zy);
           }
          }
        */
        // Rotate to face the road
        if (is_ot_type("road", t_east) || is_ot_type("bridge", t_east)) {
            rotate(1);
        }
        if (is_ot_type("road", t_south) || is_ot_type("bridge", t_south)) {
            rotate(2);
        }
        if (is_ot_type("road", t_west) || is_ot_type("bridge", t_west)) {
            rotate(3);
        }


    } else if (terrain_type == "hospital") {

        fill_background(this, t_floor);
        // We always have walls on the left and bottom
        line(this, t_wall_v, 0,  0,  0, 22);
        line(this, t_wall_h, 0, 23, 23, 23);
        // These walls contain doors if they lead to more hospital
        if (t_west == "hospital_entrance" || t_west == "hospital") {
            line(this, t_door_c, 0, 11, 0, 12);
        }
        if (t_south == "hospital_entrance" || t_south == "hospital") {
            line(this, t_door_c, 11, 23, 12, 23);
        }

        if ((t_north == "hospital_entrance" || t_north == "hospital") &&
            (t_east  == "hospital_entrance" || t_east  == "hospital") &&
            (t_south == "hospital_entrance" || t_south == "hospital") &&
            (t_west  == "hospital_entrance" || t_west  == "hospital")   ) {
            // We're in the center; center is always blood lab
            // Large lab
            line(this, t_wall_h,  1,  2, 21,  2);
            line(this, t_wall_h,  1, 10, 21, 10);
            line(this, t_wall_v, 21,  3, 21,  9);
            line_furn(this, f_counter, 2,  3,  2,  9);
            place_items("hospital_lab", 70, 2, 3, 2, 9, false, 0);
            square_furn(this, f_counter,  5,  4,  6,  8);
            place_items("hospital_lab", 74, 5, 4, 6, 8, false, 0);
            square_furn(this, f_counter, 10,  4, 11,  8);
            spawn_item(5, 17, "record_patient");
            place_items("hospital_lab", 74, 10, 4, 11, 8, false, 0);
            square_furn(this, f_counter, 15,  4, 16,  8);
            place_items("hospital_lab", 74, 15, 4, 16, 8, false, 0);
            ter_set(rng(3, 18),  2, t_door_c);
            ter_set(rng(3, 18), 10, t_door_c);
            if (one_in(4)) { // Door on the right side
                ter_set(21, rng(4, 8), t_door_c);
            } else { // Counter on the right side
                line_furn(this, f_counter, 20, 3, 20, 9);
                place_items("hospital_lab", 70, 20, 3, 20, 9, false, 0);
            }
            // Blood testing facility
            line(this, t_wall_h,  1, 13, 10, 13);
            line(this, t_wall_v, 10, 14, 10, 22);
            rn = rng(1, 3);
            if (rn == 1 || rn == 3) {
                ter_set(rng(3, 8), 13, t_door_c);
            }
            if (rn == 2 || rn == 3) {
                ter_set(10, rng(15, 21), t_door_c);
            }
            line_furn(this, f_counter, 2, 14,  2, 22);
            place_items("hospital_lab", 60, 2, 14, 2, 22, false, 0);
            square_furn(this, f_counter, 4, 17, 6, 19);
            ter_set(4, 18, t_centrifuge);
            furn_set(4, 18, f_null);
            line(this, t_floor, 5, 18, 6, rng(17, 19)); // Clear path to console
            tmpcomp = add_computer(5, 18, _("Centrifuge"), 0);
            tmpcomp->add_option(_("Analyze blood"), COMPACT_BLOOD_ANAL, 4);
            tmpcomp->add_failure(COMPFAIL_DESTROY_BLOOD);
            // Sample storage
            line(this, t_wall_h, 13, 13, 23, 13);
            line(this, t_wall_v, 13, 14, 13, 23);
            rn = rng(1, 3);
            if (rn == 1 || rn == 3) {
                ter_set(rng(14, 22), 13, t_door_c);
            }
            if (rn == 2 || rn == 3) {
                ter_set(13, rng(14, 21), t_door_c);
            }
            square_furn(this, f_rack, 16, 16, 21, 17);
            place_items("hospital_samples", 68, 16, 16, 21, 17, false, 0);
            place_items("bionics_common", 12, 16, 16, 21, 17, false, 0);
            square_furn(this, f_rack, 16, 19, 21, 20);
            place_items("hospital_samples", 68, 16, 19, 21, 20, false, 0);
            place_items("bionics_common", 12, 16, 19, 21, 20, false, 0);
            line_furn(this, f_rack, 14, 22, 23, 22);
            place_items("hospital_samples", 62, 14, 22, 23, 22, false, 0);
            place_items("bionics_common", 12, 14, 22, 23, 22, false, 0);

        } else { // We're NOT in the center; a random hospital type!

            switch (rng(1, 4)) { // What type?
            case 1: // Dorms
                // Upper left rooms
                line(this, t_wall_h, 1, 5, 9, 5);
                for (int i = 1; i <= 7; i += 3) {
                    line_furn(this, f_bed, i, 1, i, 2);
                    place_items("hospital_bed", 50, i + 1, 1, i + 1, 3, false, 0);
                    line(this, t_wall_v, i + 2, 0, i + 2, 4);
                    ter_set(rng(i, i + 1), 5, t_door_c);
                }
                // Upper right rooms
                line(this, t_wall_h, 14, 5, 23, 5);
                line(this, t_wall_v, 14, 0, 14, 4);
                line_furn(this, f_bed, 15, 1, 15, 2);
                place_items("hospital_bed", 50, 16, 1, 16, 2, false, 0);
                ter_set(rng(15, 16), 5, t_door_c);
                line(this, t_wall_v, 17, 0, 17, 4);
                line_furn(this, f_bed, 18, 1, 18, 2);
                place_items("hospital_bed", 50, 19, 1, 19, 2, false, 0);
                ter_set(rng(18, 19), 5, t_door_c);
                line(this, t_wall_v, 20, 0, 20, 4);
                line_furn(this, f_bed, 21, 1, 21, 2);
                place_items("hospital_bed", 50, 22, 1, 22, 2, false, 0);
                ter_set(rng(21, 22), 5, t_door_c);
                // Waiting area
                for (int i = 1; i <= 9; i += 4) {
                    line_furn(this, f_bench, i, 7, i, 10);
                }
                line_furn(this, f_table, 3, 8, 3, 9);
                place_items("magazines", 50, 3, 8, 3, 9, false, 0);
                line_furn(this, f_table, 7, 8, 7, 9);
                place_items("magazines", 50, 7, 8, 7, 9, false, 0);
                // Middle right rooms
                line(this, t_wall_v, 14, 7, 14, 10);
                line(this, t_wall_h, 15, 7, 23, 7);
                line(this, t_wall_h, 15, 10, 23, 10);
                line(this, t_wall_v, 19, 8, 19, 9);
                line_furn(this, f_bed, 18, 8, 18, 9);
                place_items("hospital_bed", 50, 17, 8, 17, 9, false, 0);
                line_furn(this, f_bed, 20, 8, 20, 9);
                place_items("hospital_bed", 50, 21, 8, 21, 9, false, 0);
                if (one_in(3)) { // Doors to north
                    ter_set(rng(15, 16), 7, t_door_c);
                    ter_set(rng(21, 22), 7, t_door_c);
                } else { // Doors to south
                    ter_set(rng(15, 16), 10, t_door_c);
                    ter_set(rng(21, 22), 10, t_door_c);
                }
                line(this, t_wall_v, 14, 13, 14, 16);
                line(this, t_wall_h, 15, 13, 23, 13);
                line(this, t_wall_h, 15, 16, 23, 16);
                line(this, t_wall_v, 19, 14, 19, 15);
                line_furn(this, f_bed, 18, 14, 18, 15);
                place_items("hospital_bed", 50, 17, 14, 17, 15, false, 0);
                line_furn(this, f_bed, 20, 14, 20, 15);
                place_items("hospital_bed", 50, 21, 14, 21, 15, false, 0);
                if (one_in(3)) { // Doors to south
                    ter_set(rng(15, 16), 16, t_door_c);
                    ter_set(rng(21, 22), 16, t_door_c);
                } else { // Doors to north
                    ter_set(rng(15, 16), 13, t_door_c);
                    ter_set(rng(21, 22), 13, t_door_c);
                }
                // Lower left rooms
                line(this, t_wall_v, 5, 13, 5, 22);
                line(this, t_wall_h, 1, 13, 4, 13);
                line_furn(this, f_bed, 1, 14, 1, 15);
                place_items("hospital_bed", 50, 2, 14, 2, 15, false, 0);
                line(this, t_wall_h, 1, 17, 4, 17);
                line_furn(this, f_bed, 1, 18, 1, 19);
                place_items("hospital_bed", 50, 2, 18, 2, 19, false, 0);
                line(this, t_wall_h, 1, 20, 4, 20);
                line_furn(this, f_bed, 1, 21, 1, 22);
                place_items("hospital_bed", 50, 2, 21, 2, 22, false, 0);
                ter_set(5, rng(14, 16), t_door_c);
                ter_set(5, rng(18, 19), t_door_c);
                ter_set(5, rng(21, 22), t_door_c);
                line(this, t_wall_h, 7, 13, 10, 13);
                line(this, t_wall_v, 7, 14, 7, 22);
                line(this, t_wall_v, 10, 14, 10, 22);
                line(this, t_wall_h, 8, 18, 9, 18);
                line_furn(this, f_bed, 8, 17, 9, 17);
                place_items("hospital_bed", 50, 8, 16, 9, 16, false, 0);
                line_furn(this, f_bed, 8, 22, 9, 22);
                place_items("hospital_bed", 50, 8, 21, 9, 21, false, 0);
                if (one_in(3)) { // Doors to west
                    ter_set(7, rng(14, 16), t_door_c);
                    ter_set(7, rng(19, 21), t_door_c);
                } else { // Doors to east
                    ter_set(10, rng(14, 16), t_door_c);
                    ter_set(10, rng(19, 21), t_door_c);
                }
                // Lower-right rooms
                line(this, t_wall_h, 14, 18, 23, 18);
                for (int i = 14; i <= 20; i += 3) {
                    line(this, t_wall_v, i, 19, i, 22);
                    line_furn(this, f_bed, i + 1, 21, i + 1, 22);
                    place_items("hospital_bed", 50, i + 2, 21, i + 2, 22, false, 0);
                    ter_set(rng(i + 1, i + 2), 18, t_door_c);
                }
                break;

            case 2: // Offices and cafeteria
                // Offices to north
                line(this, t_wall_v, 10, 0, 10, 8);
                line(this, t_wall_v, 13, 0, 13, 8);
                line(this, t_wall_h,  1, 5,  9, 5);
                line(this, t_wall_h, 14, 5, 23, 5);
                line(this, t_wall_h,  1, 9, 23, 9);
                line(this, t_door_c, 11, 9, 12, 9);
                line_furn(this, f_table,  3, 3,  7, 3);
                line_furn(this, f_table, 16, 3, 20, 3);
                line_furn(this, f_table,  3, 8,  7, 8);
                line_furn(this, f_table, 16, 8, 20, 8);
                ter_set(10, rng(2, 3), t_door_c);
                ter_set(13, rng(2, 3), t_door_c);
                ter_set(10, rng(6, 7), t_door_c);
                ter_set(13, rng(6, 7), t_door_c);
                place_items("office", 70,  1, 1,  9, 3, false, 0);
                place_items("office", 70, 14, 1, 22, 3, false, 0);
                place_items("office", 70,  1, 5,  9, 8, false, 0);
                place_items("office", 70, 14, 5, 22, 8, false, 0);
                // Cafeteria to south
                line(this, t_wall_h,  1, 14,  8, 14);
                line(this, t_wall_h, 15, 14, 23, 14);
                for (int i = 16; i <= 19; i += 3) {
                    for (int j = 17; j <= 20; j += 3) {
                        square_furn(this, f_table, i, j, i + 1, j + 1);
                        place_items("snacks",  60, i, j, i + 1, j + 1, false, 0);
                        place_items("produce", 65, i, j, i + 1, j + 1, false, 0);
                    }
                }
                for (int i = 3; i <= 6; i += 3) {
                    for (int j = 17; j <= 20; j += 3) {
                        square_furn(this, f_table, i, j, i + 1, j + 1);
                        place_items("snacks",  60, i, j, i + 1, j + 1, false, 0);
                        place_items("produce", 65, i, j, i + 1, j + 1, false, 0);
                    }
                }
                break;

            case 3: // Operating rooms
                // First, walls and doors; divide it into four big operating rooms
                line(this, t_wall_v, 10,  0, 10,  9);
                line(this, t_door_c, 10,  4, 10,  5);
                line(this, t_wall_v, 13,  0, 13,  9);
                line(this, t_door_c, 13,  4, 13,  5);
                line(this, t_wall_v, 10, 14, 10, 22);
                line(this, t_door_c, 10, 18, 10, 19);
                line(this, t_wall_v, 13, 14, 13, 22);
                line(this, t_door_c, 13, 18, 13, 19);
                // Horizontal walls/doors
                line(this, t_wall_h,  1, 10, 10, 10);
                line(this, t_door_c,  5, 10,  6, 10);
                line(this, t_wall_h, 13, 10, 23, 10);
                line(this, t_door_c, 18, 10, 19, 10);
                line(this, t_wall_h,  1, 13, 10, 13);
                line(this, t_door_c,  5, 13,  6, 13);
                line(this, t_wall_h, 13, 13, 23, 13);
                line(this, t_door_c, 18, 13, 19, 13);
                // Next, the contents of each operating room
                line_furn(this, f_counter, 1, 1, 1, 9);
                place_items("surgery", 70, 1, 1, 1, 9, false, 0);
                place_items("bionics_common", 5, 1, 1, 1, 9, false, 0);
                square_furn(this, f_bed, 5, 4, 6, 5);
                place_items("bed", 60, 5, 4, 6, 5, false, 0);

                line_furn(this, f_counter, 1, 14, 1, 22);
                place_items("surgery", 70, 1, 14, 1, 22, false, 0);
                place_items("bionics_common", 5, 1, 14, 1, 22, false, 0);
                square_furn(this, f_bed, 5, 18, 6, 19);
                place_items("bed", 60, 5, 18, 6, 19, false, 0);

                line_furn(this, f_counter, 14, 6, 14, 9);
                place_items("surgery", 60, 14, 6, 14, 9, false, 0);
                line_furn(this, f_counter, 15, 9, 17, 9);
                place_items("surgery", 60, 15, 9, 17, 9, false, 0);
                square_furn(this, f_bed, 18, 4, 19, 5);
                place_items("bed", 60, 18, 4, 19, 5, false, 0);

                line_furn(this, f_counter, 14, 14, 14, 17);
                place_items("surgery", 60, 14, 14, 14, 17, false, 0);
                line_furn(this, f_counter, 15, 14, 17, 14);
                place_items("surgery", 60, 15, 14, 17, 14, false, 0);
                square_furn(this, f_bed, 18, 18, 19, 19);
                place_items("bed", 60, 18, 18, 19, 19, false, 0);
                // computer to begin healing broken bones,
                tmpcomp = add_computer(16, 16, _("Mr. Stem Cell"), 3);
                tmpcomp->add_option(_("Stem Cell Treatment"), COMPACT_STEMCELL_TREATMENT, 3);
                tmpcomp->add_failure(COMPFAIL_ALARM);

                break;

            case 4: // Storage
                // Soft drug storage
                line(this, t_wall_h, 3,  2, 12,  2);
                line(this, t_wall_h, 3, 10, 12, 10);
                line(this, t_wall_v, 3,  3,  3,  9);
                ter_set(3, 6, t_door_c);
                line_furn(this, f_rack,   4,  3, 11,  3);
                place_items("softdrugs", 90, 4, 3, 11, 3, false, 0);
                line_furn(this, f_rack,   4,  9, 11,  9);
                place_items("softdrugs", 90, 4, 9, 11, 9, false, 0);
                line_furn(this, f_rack, 6, 5, 10, 5);
                place_items("softdrugs", 80, 6, 5, 10, 5, false, 0);
                line_furn(this, f_rack, 6, 7, 10, 7);
                place_items("softdrugs", 80, 6, 7, 10, 7, false, 0);
                // Hard drug storage
                line(this, t_wall_v, 13, 0, 13, 19);
                ter_set(13, 6, t_door_locked);
                line_furn(this, f_rack, 14, 1, 14, 4);
                place_items("harddrugs", 78, 14, 1, 14, 4, false, 0);
                line_furn(this, f_rack, 17, 1, 17, 7);
                place_items("harddrugs", 85, 17, 1, 17, 7, false, 0);
                line_furn(this, f_rack, 20, 1, 20, 7);
                place_items("harddrugs", 85, 20, 1, 20, 7, false, 0);
                line(this, t_wall_h, 20, 10, 23, 10);
                line_furn(this, f_rack, 16, 10, 19, 10);
                place_items("harddrugs", 78, 16, 10, 19, 10, false, 0);
                line_furn(this, f_rack, 16, 12, 19, 12);
                place_items("harddrugs", 78, 16, 12, 19, 12, false, 0);
                line(this, t_wall_h, 14, 14, 19, 14);
                ter_set(rng(14, 15), 14, t_door_locked);
                ter_set(rng(16, 18), 14, t_bars);
                line(this, t_wall_v, 20, 11, 20, 19);
                line(this, t_wall_h, 13, 20, 20, 20);
                line(this, t_door_c, 16, 20, 17, 20);
                // Laundry room
                line(this, t_wall_h, 1, 13, 10, 13);
                ter_set(rng(3, 8), 13, t_door_c);
                line(this, t_wall_v, 10, 14, 10, 22);
                ter_set(10, rng(16, 20), t_door_c);
                line_furn(this, f_counter, 1, 14, 1, 22);
                place_items("allclothes", 70, 1, 14, 1, 22, false, 0);
                for (int j = 15; j <= 21; j += 3) {
                    line_furn(this, f_rack, 4, j, 7, j);
                    if (one_in(2)) {
                        place_items("cleaning", 92, 4, j, 7, j, false, 0);
                    } else if (one_in(5)) {
                        place_items("cannedfood", 75, 4, j, 7, j, false, 0);
                    } else {
                        place_items("allclothes", 50, 4, j, 7, j, false, 0);
                    }
                }
                break;
            }


            // We have walls to the north and east if they're not hospitals
            if (t_east != "hospital_entrance" && t_east != "hospital") {
                line(this, t_wall_v, 23, 0, 23, 23);
            }
            if (t_north != "hospital_entrance" && t_north != "hospital") {
                line(this, t_wall_h, 0, 0, 23, 0);
            }
        }
        // Generate bodies / zombies
        rn = rng(15, 20);
        for (int i = 0; i < rn; i++) {
            item body;
            body.make_corpse();
            int zx = rng(0, SEEX * 2 - 1), zy = rng(0, SEEY * 2 - 1);
            if (move_cost(zx, zy) > 0) {
                if (furn(zx, zy) == f_bed || one_in(3)) {
                    add_item(zx, zy, body);
                } else {
                    std::string zom = "mon_zombie";
                    if (one_in(6)) {
                        zom = "mon_zombie_spitter";
                    } else if (!one_in(3)) {
                        zom = "mon_boomer";
                    }
                    add_spawn(zom, 1, zx, zy);
                }
            } else {
                //This is a wall: try again
                i--;
            }
        }


    } else if (terrain_type == "mansion_entrance") {
        dat.fill_groundcover();

        // Front wall
        line(this, t_wall_h,  0, 10,  SEEX * 2 - 1, 10);
        line(this, t_door_locked, SEEX - 1, 10, SEEX, 10);
        int winx1 = rng(2, 4);
        int winx2 = rng(4, 6);
        line(this, t_window, winx1, 10, winx2, 10);
        line(this, t_window, SEEX * 2 - 1 - winx1, 10, SEEX * 2 - 1 - winx2, 10);
        winx1 = rng(7, 10);
        winx2 = rng(10, 11);
        line(this, t_window, winx1, 10, winx2, 10);
        line(this, t_window, SEEX * 2 - 1 - winx1, 10, SEEX * 2 - 1 - winx2, 10);
        line(this, t_door_c, SEEX - 1, 10, SEEX, 10);

        build_mansion_room(this, room_mansion_courtyard, 0, 0, SEEX * 2 - 1, 9, dat);
        square(this, t_floor, 0, 11, SEEX * 2 - 1, SEEY * 2 - 1);
        build_mansion_room(this, room_mansion_entry, 0, 11, SEEX * 2 - 1, SEEY * 2 - 1, dat);
        // Rotate to face the road
        if (is_ot_type("road", t_east) || is_ot_type("bridge", t_east) ||
            ((t_east != "mansion") && (t_north == "mansion") && (t_south == "mansion"))) {
            rotate(1);
        }
        if (is_ot_type("road", t_south) || is_ot_type("bridge", t_south) ||
            ((t_south != "mansion") && (t_west == "mansion") && (t_east == "mansion"))) {
            rotate(2);
        }
        if (is_ot_type("road", t_west) || is_ot_type("bridge", t_west) ||
            ((t_west != "mansion") && (t_north == "mansion") && (t_south == "mansion"))) {
            rotate(3);
        }
        // add zombies
        if (one_in(3)) {
            add_spawn("mon_zombie", rng(1, 8), 12, 12);
        }
        // Left wall
        if( t_west == "mansion_entrance" || t_west == "mansion" ) {
            line(this, t_wall_v,  0,  0,  0, SEEY * 2 - 2);
            line(this, t_door_c,  0, SEEY - 1, 0, SEEY);
        }
        // Bottom wall
        if( t_south == "mansion_entrance" || t_south == "mansion" ) {
            line(this, t_wall_h,  0, SEEY * 2 - 1, SEEX * 2 - 1, SEEY * 2 - 1);
            line(this, t_door_c, SEEX - 1, SEEY * 2 - 1, SEEX, SEEY * 2 - 1);
        }

    } else if (terrain_type == "mansion") {

        // Start with floors all over
        square(this, t_floor, 1, 0, SEEX * 2 - 1, SEEY * 2 - 2);
        // We always have a left and bottom wall
        line(this, t_wall_v, 0, 0, 0, SEEY * 2 - 2);
        line(this, t_wall_h, 0, SEEY * 2 - 1, SEEX * 2 - 1, SEEY * 2 - 1);
        // tw and rw are the boundaries of the rooms inside...
        tw = 0;
        rw = SEEX * 2 - 1;
        // ...if we need outside walls, adjust tw & rw and build them
        // We build windows below.
        if (t_north != "mansion_entrance" && t_north != "mansion") {
            tw = 1;
            line(this, t_wall_h, 0, 0, SEEX * 2 - 1, 0);
        }
        if (t_east != "mansion_entrance" && t_east != "mansion") {
            rw = SEEX * 2 - 2;
            line(this, t_wall_v, SEEX * 2 - 1, 0, SEEX * 2 - 1, SEEX * 2 - 1);
        }
        // Now pick a random layout
        switch (rng(1, 10)) {

        case 1: // Just one. big. room.
            mansion_room(this, 1, tw, rw, SEEY * 2 - 2, dat);
            if (t_west == "mansion_entrance" || t_west == "mansion") {
                line(this, t_door_c, 0, SEEY - 1, 0, SEEY);
            }
            if (t_south == "mansion_entrance" || t_south == "mansion") {
                line(this, t_door_c, SEEX - 1, SEEY * 2 - 1, SEEX, SEEY * 2 - 1);
            }
            break;

        case 2: // Wide hallway, two rooms.
        case 3:
            if (one_in(2)) { // vertical hallway
                line(this, t_wall_v,  9,  tw,  9, SEEY * 2 - 2);
                line(this, t_wall_v, 14,  tw, 14, SEEY * 2 - 2);
                line(this, t_floor, SEEX - 1, SEEY * 2 - 1, SEEX, SEEY * 2 - 1);
                line(this, t_door_c, 0, SEEY - 1, 0, SEEY);
                mansion_room(this, 1, tw, 8, SEEY * 2 - 2, dat);
                mansion_room(this, 15, tw, rw, SEEY * 2 - 2, dat);
                ter_set( 9, rng(tw + 2, SEEX * 2 - 4), t_door_c);
                ter_set(14, rng(tw + 2, SEEX * 2 - 4), t_door_c);
            } else { // horizontal hallway
                line(this, t_wall_h, 1,  9, rw,  9);
                line(this, t_wall_h, 1, 14, rw, 14);
                line(this, t_door_c, SEEX - 1, SEEY * 2 - 1, SEEX, SEEY * 2 - 1);
                line(this, t_floor, 0, SEEY - 1, 0, SEEY);
                mansion_room(this, 1, tw, rw, 8, dat);
                mansion_room(this, 1, 15, rw, SEEY * 2 - 2, dat);
                ter_set(rng(3, rw - 2),  9, t_door_c);
                ter_set(rng(3, rw - 2), 14, t_door_c);
            }
            if (t_west == "mansion_entrance" || t_west == "mansion") {
                line(this, t_door_c, 0, SEEY - 1, 0, SEEY);
            }
            if (t_south == "mansion_entrance" || t_south == "mansion") {
                line(this, t_floor, SEEX - 1, SEEY * 2 - 1, SEEX, SEEY * 2 - 1);
            }
            break;

        case 4: // Four corners rooms
        case 5:
        case 6:
        case 7:
        case 8:
            line(this, t_wall_v, 10, tw, 10,  9);
            line(this, t_wall_v, 13, tw, 13,  9);
            line(this, t_wall_v, 10, 14, 10, SEEY * 2 - 2);
            line(this, t_wall_v, 13, 14, 13, SEEY * 2 - 2);
            line(this, t_wall_h,  1, 10, 10, 10);
            line(this, t_wall_h,  1, 13, 10, 13);
            line(this, t_wall_h, 13, 10, rw, 10);
            line(this, t_wall_h, 13, 13, rw, 13);
            // Doors
            if (one_in(2)) {
                ter_set(10, rng(tw + 1, 8), t_door_c);
            } else {
                ter_set(rng(2, 8), 10, t_door_c);
            }

            if (one_in(2)) {
                ter_set(13, rng(tw + 1, 8), t_door_c);
            } else {
                ter_set(rng(15, rw - 1), 10, t_door_c);
            }

            if (one_in(2)) {
                ter_set(10, rng(15, SEEY * 2 - 3), t_door_c);
            } else {
                ter_set(rng(2, 8), 13, t_door_c);
            }

            if (one_in(2)) {
                ter_set(13, rng(15, SEEY * 2 - 3), t_door_c);
            } else {
                ter_set(rng(15, rw - 1), 13, t_door_c);
            }

            mansion_room(this,  1, tw,  9,  9, dat);
            mansion_room(this, 14, tw, rw,  9, dat);
            mansion_room(this,  1, 14,  9, SEEY * 2 - 2, dat);
            mansion_room(this, 14, 14, rw, SEEY * 2 - 2, dat);
            if (t_west == "mansion_entrance" || t_west == "mansion") {
                line(this, t_floor, 0, SEEY - 1, 0, SEEY);
            }
            if (t_south == "mansion_entrance" || t_south == "mansion") {
                line(this, t_floor, SEEX - 1, SEEY * 2 - 1, SEEX, SEEY * 2 - 1);
            }
            break;

        case 9: // One large room in lower-left
        case 10:
            mw = rng( 4, 10);
            cw = rng(13, 19);
            x = rng(5, 10);
            y = rng(13, 18);
            line(this, t_wall_h,  1, mw, cw, mw);
            ter_set( rng(x + 1, cw - 1), mw, t_door_c);
            line(this, t_wall_v, cw, mw + 1, cw, SEEY * 2 - 2);
            ter_set(cw, rng(y + 2, SEEY * 2 - 3) , t_door_c);
            mansion_room(this, 1, mw + 1, cw - 1, SEEY * 2 - 2, dat);
            // And a couple small rooms in the UL LR corners
            line(this, t_wall_v, x, tw, x, mw - 1);
            mansion_room(this, 1, tw, x - 1, mw - 1, dat);
            if (one_in(2)) {
                ter_set(rng(2, x - 2), mw, t_door_c);
            } else {
                ter_set(x, rng(tw + 2, mw - 2), t_door_c);
            }
            line(this, t_wall_h, cw + 1, y, rw, y);
            mansion_room(this, cw + 1, y + 1, rw, SEEY * 2 - 2, dat);
            if (one_in(2)) {
                ter_set(rng(cw + 2, rw - 1), y, t_door_c);
            } else {
                ter_set(cw, rng(y + 2, SEEY * 2 - 3), t_door_c);
            }

            if (t_west == "mansion_entrance" || t_west == "mansion") {
                line(this, t_floor, 0, SEEY - 1, 0, SEEY);
            }
            if (t_south == "mansion_entrance" || t_south == "mansion") {
                line(this, t_floor, SEEX - 1, SEEY * 2 - 1, SEEX, SEEY * 2 - 1);
            }
            break;
        } // switch (rng(1, 4))

        // Finally, place windows on outside-facing walls if necessary
        if (t_west != "mansion_entrance" && t_west != "mansion") {
            int consecutive = 0;
            for (int i = 1; i < SEEY; i++) {
                if (move_cost(1, i) != 0 && move_cost(1, SEEY * 2 - 1 - i) != 0) {
                    if (consecutive == 3) {
                        consecutive = 0;    // No really long windows
                    } else {
                        consecutive++;
                        ter_set(0, i, t_window);
                        ter_set(0, SEEY * 2 - 1 - i, t_window);
                    }
                } else {
                    consecutive = 0;
                }
            }
        }
        if (t_south != "mansion_entrance" && t_south != "mansion") {
            int consecutive = 0;
            for (int i = 1; i < SEEX; i++) {
                if (move_cost(i, SEEY * 2 - 2) != 0 &&
                    move_cost(SEEX * 2 - 1 - i, SEEY * 2 - 2) != 0) {
                    if (consecutive == 3) {
                        consecutive = 0;    // No really long windows
                    } else {
                        consecutive++;
                        ter_set(i, SEEY * 2 - 1, t_window);
                        ter_set(SEEX * 2 - 1 - i, SEEY * 2 - 1, t_window);
                    }
                } else {
                    consecutive = 0;
                }
            }
        }
        if (t_east != "mansion_entrance" && t_east != "mansion") {
            int consecutive = 0;
            for (int i = 1; i < SEEY; i++) {
                if (move_cost(SEEX * 2 - 2, i) != 0 &&
                    move_cost(SEEX * 2 - 2, SEEY * 2 - 1 - i) != 0) {
                    if (consecutive == 3) {
                        consecutive = 0;    // No really long windows
                    } else {
                        consecutive++;
                        ter_set(SEEX * 2 - 1, i, t_window);
                        ter_set(SEEX * 2 - 1, SEEY * 2 - 1 - i, t_window);
                    }
                } else {
                    consecutive = 0;
                }
            }
        }

        if (t_north != "mansion_entrance" && t_north != "mansion") {
            int consecutive = 0;
            for (int i = 1; i < SEEX; i++) {
                if (move_cost(i, 1) != 0 && move_cost(SEEX * 2 - 1 - i, 1) != 0) {
                    if (consecutive == 3) {
                        consecutive = 0;    // No really long windows
                    } else {
                        consecutive++;
                        ter_set(i, 0, t_window);
                        ter_set(SEEX * 2 - 1 - i, 0, t_window);
                    }
                } else {
                    consecutive = 0;
                }
            }
        }
        // add zombies
        if (one_in(2)) {
            add_spawn("mon_zombie", rng(4, 8), 12, 12);
        }


    } else if (terrain_type == "fema_entrance") {

        fill_background(this, t_dirt);
        // Left wall
        line(this, t_chainfence_h, 0, 23, 23, 23);
        line(this, t_chaingate_l, 10, 23, 14, 23);
        line(this, t_chainfence_v,  0,  0,  0, 23);
        line(this, t_chainfence_v,  23,  0,  23, 23);
        line(this, t_fence_barbed, 1, 4, 9, 12);
        line(this, t_fence_barbed, 1, 5, 8, 12);
        line(this, t_fence_barbed, 22, 4, 15, 12);
        line(this, t_fence_barbed, 22, 5, 16, 12);
        square(this, t_wall_wood, 2, 13, 9, 21);
        square(this, t_floor, 3, 14, 8, 20);
        line(this, t_reinforced_glass_h, 5, 13, 6, 13);
        line(this, t_reinforced_glass_h, 5, 21, 6, 21);
        line(this, t_reinforced_glass_v, 9, 15, 9, 18);
        line(this, t_door_c, 9, 16, 9, 17);
        line_furn(this, f_locker, 3, 16, 3, 18);
        line_furn(this, f_chair, 5, 16, 5, 18);
        line_furn(this, f_desk, 6, 16, 6, 18);
        line_furn(this, f_chair, 7, 16, 7, 18);
        place_items("office", 80, 3, 16, 3, 18, false, 0);
        place_items("office", 80, 6, 16, 6, 18, false, 0);
        add_spawn("mon_zombie_soldier", rng(1, 6), 4, 17);

        // Rotate to face the road
        if (is_ot_type("road", t_east) || is_ot_type("bridge", t_east)) {
            rotate(1);
        }
        if (is_ot_type("road", t_south) || is_ot_type("bridge", t_south)) {
            rotate(2);
        }
        if (is_ot_type("road", t_west) || is_ot_type("bridge", t_west)) {
            rotate(3);
        }


    } else if (terrain_type == "fema") {

        fill_background(this, t_dirt);
        // check all sides for non fema/fema entrance, place fence on those sides
        if(t_north != "fema" && t_north != "fema_entrance") {
            line(this, t_chainfence_h, 0, 0, 23, 0);
        }
        if(t_south != "fema" && t_south != "fema_entrance") {
            line(this, t_chainfence_h, 0, 23, 23, 23);
        }
        if(t_west != "fema" && t_west != "fema_entrance") {
            line(this, t_chainfence_v, 0, 0, 0, 23);
        }
        if(t_east != "fema" && t_east != "fema_entrance") {
            line(this, t_chainfence_v, 23, 0, 23, 23);
        }
        if(t_west == "fema" && t_east == "fema" && t_south != "fema") {
            //lab bottom side
            square(this, t_dirt, 1, 1, 22, 22);
            square(this, t_floor, 4, 4, 19, 19);
            line(this, t_concrete_h, 4, 4, 19, 4);
            line(this, t_concrete_h, 4, 19, 19, 19);
            line(this, t_concrete_v, 4, 5, 4, 18);
            line(this, t_concrete_v, 19, 5, 19, 18);
            line(this, t_door_metal_c, 11, 4, 12, 4);
            line_furn(this, f_glass_fridge, 6, 5, 9, 5);
            line_furn(this, f_glass_fridge, 14, 5, 17, 5);
            square(this, t_grate, 6, 8, 8, 9);
            line_furn(this, f_table, 7, 8, 7, 9);
            square(this, t_grate, 6, 12, 8, 13);
            line_furn(this, f_table, 7, 12, 7, 13);
            square(this, t_grate, 6, 16, 8, 17);
            line_furn(this, f_table, 7, 16, 7, 17);
            line_furn(this, f_counter, 10, 8, 10, 17);
            line_furn(this, f_chair, 14, 8, 14, 10);
            line_furn(this, f_chair, 17, 8, 17, 10);
            square(this, t_console_broken, 15, 8, 16, 10);
            line_furn(this, f_desk, 15, 11, 16, 11);
            line_furn(this, f_chair, 15, 12, 16, 12);
            line(this, t_reinforced_glass_h, 13, 14, 18, 14);
            line(this, t_reinforced_glass_v, 13, 14, 13, 18);
            ter_set(15, 14, t_door_metal_locked);
            place_items("dissection", 90, 10, 8, 10, 17, false, 0);
            place_items("hospital_lab", 70, 5, 5, 18, 18, false, 0);
            place_items("harddrugs", 50, 6, 5, 9, 5, false, 0);
            place_items("harddrugs", 50, 14, 5, 17, 5, false, 0);
            place_items("hospital_samples", 50, 6, 5, 9, 5, false, 0);
            place_items("hospital_samples", 50, 14, 5, 17, 5, false, 0);
            add_spawn("mon_zombie_scientist", rng(1, 6), 11, 12);
            if (one_in(2)) {
                add_spawn("mon_zombie_brute", 1, 16, 17);
            }
        } else if (t_west == "fema_entrance") {
            square(this, t_dirt, 1, 1, 22, 22); //Supply tent
            line_furn(this, f_canvas_wall, 4, 4, 19, 4);
            line_furn(this, f_canvas_wall, 4, 4, 4, 19);
            line_furn(this, f_canvas_wall, 19, 19, 19, 4);
            line_furn(this, f_canvas_wall, 19, 19, 4, 19);
            square_furn(this, f_fema_groundsheet, 5, 5, 8, 18);
            square_furn(this, f_fema_groundsheet, 10, 5, 13, 5);
            square_furn(this, f_fema_groundsheet, 10, 18, 13, 18);
            square_furn(this, f_fema_groundsheet, 15, 5, 18, 7);
            square_furn(this, f_fema_groundsheet, 15, 16, 18, 18);
            square_furn(this, f_fema_groundsheet, 16, 10, 17, 14);
            square_furn(this, f_fema_groundsheet, 9, 7, 14, 16);
            line_furn(this, f_canvas_door, 11, 4, 12, 4);
            line_furn(this, f_canvas_door, 11, 19, 12, 19);
            square_furn(this, f_crate_c, 5, 6, 7, 7);
            square_furn(this, f_crate_c, 5, 11, 7, 12);
            square_furn(this, f_crate_c, 5, 16, 7, 17);
            line(this, t_chainfence_h, 9, 6, 14, 6);
            line(this, t_chainfence_h, 9, 17, 14, 17);
            ter_set(9, 5, t_chaingate_c);
            ter_set(14, 18, t_chaingate_c);
            ter_set(14, 5, t_chainfence_v);
            ter_set(9, 18, t_chainfence_v);
            furn_set(12, 17, f_counter);
            furn_set(11, 6, f_counter);
            line_furn(this, f_chair, 10, 10, 13, 10);
            square_furn(this, f_desk, 10, 11, 13, 12);
            line_furn(this, f_chair, 10, 13, 13, 13);
            line(this, t_chainfence_h, 15, 8, 18, 8);
            line(this, t_chainfence_h, 15, 15, 18, 15);
            line(this, t_chainfence_v, 15, 9, 15, 14);
            line(this, t_chaingate_c, 15, 11, 15, 12);
            line_furn(this, f_locker, 18, 9, 18, 14);
            place_items("allclothes", 90, 5, 6, 7, 7, false, 0);
            place_items("softdrugs", 90, 5, 11, 7, 12, false, 0);
            place_items("hardware", 90, 5, 16, 7, 17, false, 0);
            place_items("mil_rifles", 90, 18, 9, 18, 14, false, 0);
            place_items("office", 80, 10, 11, 13, 12, false, 0);
            add_spawn("mon_zombie_soldier", rng(1, 6), 12, 14);
        } else {
            switch (rng(1, 5)) {
            case 1:
            case 2:
            case 3:
                square(this, t_dirt, 1, 1, 22, 22);
                square_furn(this, f_canvas_wall, 4, 4, 19, 19); //Lodging
                square_furn(this, f_fema_groundsheet, 5, 5, 18, 18);
                line_furn(this, f_canvas_door, 11, 4, 12, 4);
                line_furn(this, f_canvas_door, 11, 19, 12, 19);
                line_furn(this, f_makeshift_bed, 6, 6, 6, 17);
                line_furn(this, f_makeshift_bed, 8, 6, 8, 17);
                line_furn(this, f_makeshift_bed, 10, 6, 10, 17);
                line_furn(this, f_makeshift_bed, 13, 6, 13, 17);
                line_furn(this, f_makeshift_bed, 15, 6, 15, 17);
                line_furn(this, f_makeshift_bed, 17, 6, 17, 17);
                line_furn(this, f_fema_groundsheet, 6, 8, 17, 8);
                line_furn(this, f_fema_groundsheet, 6, 8, 17, 8);
                square_furn(this, f_fema_groundsheet, 6, 11, 17, 12);
                line_furn(this, f_fema_groundsheet, 6, 15, 17, 15);
                line_furn(this, f_crate_o, 6, 7, 17, 7);
                line_furn(this, f_crate_o, 6, 10, 17, 10);
                line_furn(this, f_crate_o, 6, 14, 17, 14);
                line_furn(this, f_crate_o, 6, 17, 17, 17);
                line_furn(this, f_fema_groundsheet, 7, 5, 7, 18);
                line_furn(this, f_fema_groundsheet, 9, 5, 9, 18);
                square_furn(this, f_fema_groundsheet, 11, 5, 12, 18);
                line_furn(this, f_fema_groundsheet, 14, 5, 14, 18);
                line_furn(this, f_fema_groundsheet, 16, 5, 16, 18);
                place_items("livingroom", 80, 5, 5, 18, 18, false, 0);
                add_spawn("mon_zombie", rng(1, 5), 11, 12);
                break;
            case 4:
                square(this, t_dirt, 1, 1, 22, 22);
                square_furn(this, f_canvas_wall, 4, 4, 19, 19); //Mess hall/tent
                square_furn(this, f_fema_groundsheet, 5, 5, 18, 18);
                line_furn(this, f_canvas_door, 11, 4, 12, 4);
                line_furn(this, f_canvas_door, 11, 19, 12, 19);
                line_furn(this, f_crate_c, 5, 5, 5, 6);
                square_furn(this, f_counter, 6, 6, 10, 8);
                square(this, t_rock_floor, 6, 5, 9, 7);
                furn_set(7, 6, f_woodstove);
                line_furn(this, f_bench, 13, 6, 17, 6);
                line_furn(this, f_table, 13, 7, 17, 7);
                line_furn(this, f_bench, 13, 8, 17, 8);

                line_furn(this, f_bench, 13, 11, 17, 11);
                line_furn(this, f_table, 13, 12, 17, 12);
                line_furn(this, f_bench, 13, 13, 17, 13);

                line_furn(this, f_bench, 13, 15, 17, 15);
                line_furn(this, f_table, 13, 16, 17, 16);
                line_furn(this, f_bench, 13, 17, 17, 17);

                line_furn(this, f_bench, 6, 11, 10, 11);
                line_furn(this, f_table, 6, 12, 10, 12);
                line_furn(this, f_bench, 6, 13, 10, 13);

                line_furn(this, f_bench, 6, 15, 10, 15);
                line_furn(this, f_table, 6, 16, 10, 16);
                line_furn(this, f_bench, 6, 17, 10, 17);

                place_items("mil_food_nodrugs", 80, 5, 5, 5, 6, false, 0);
                place_items("snacks", 80, 5, 5, 18, 18, false, 0);
                place_items("kitchen", 70, 6, 5, 10, 8, false, 0);
                place_items("dining", 80, 13, 7, 17, 7, false, 0);
                place_items("dining", 80, 13, 12, 17, 12, false, 0);
                place_items("dining", 80, 13, 16, 17, 16, false, 0);
                place_items("dining", 80, 6, 12, 10, 12, false, 0);
                place_items("dining", 80, 6, 16, 10, 16, false, 0);
                add_spawn("mon_zombie", rng(1, 5), 11, 12);
                break;
            case 5:
                square(this, t_dirt, 1, 1, 22, 22);
                square(this, t_fence_barbed, 4, 4, 19, 19);
                square(this, t_dirt, 5, 5, 18, 18);
                square(this, t_pit_corpsed, 6, 6, 17, 17);
                add_spawn("mon_zombie", rng(5, 20), 11, 12);
                break;
            }
        }

    } else if (terrain_type == "spider_pit_under") {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((i >= 3 && i <= SEEX * 2 - 4 && j >= 3 && j <= SEEY * 2 - 4) ||
                    one_in(4)) {
                    ter_set(i, j, t_rock_floor);
                    if (!one_in(3)) {
                        add_field(x, y, fd_web, rng(1, 3));
                    }
                } else {
                    ter_set(i, j, t_rock);
                }
            }
        }
        ter_set(rng(3, SEEX * 2 - 4), rng(3, SEEY * 2 - 4), t_slope_up);
        place_items("spider", 85, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, false, 0);
        add_spawn("mon_spider_trapdoor", 1, rng(3, SEEX * 2 - 5), rng(3, SEEY * 2 - 4));


    } else if (terrain_type == "anthill") {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i < 8 || j < 8 || i > SEEX * 2 - 9 || j > SEEY * 2 - 9) {
                    ter_set(i, j, dat.groundcover());
                } else if ((i == 11 || i == 12) && (j == 11 || j == 12)) {
                    ter_set(i, j, t_slope_down);
                } else {
                    ter_set(i, j, t_dirtmound);
                }
            }
        }

    } else if (is_ot_type("slimepit", terrain_type)) {

        for (int i = 0; i < 4; i++) {
            nesw_fac[i] = (t_nesw[i] == "slimepit" || t_nesw[i] == "slimepit_down" ? 1 : 0);
        }

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (!one_in(10) && (j < n_fac * SEEX        || i < w_fac * SEEX ||
                                    j > SEEY * 2 - s_fac * SEEY || i > SEEX * 2 - e_fac * SEEX)) {
                    ter_set(i, j, (!one_in(10) ? t_slime : t_rock_floor));
                } else if (rng(0, SEEX) > abs(i - SEEX) && rng(0, SEEY) > abs(j - SEEY)) {
                    ter_set(i, j, t_slime);
                } else if (zlevel == 0) {
                    ter_set(i, j, t_dirt);
                } else {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }

        if (terrain_type == "slimepit_down") {
            ter_set(rng(3, SEEX * 2 - 4), rng(3, SEEY * 2 - 4), t_slope_down);
        }

        if (t_above == "slimepit_down") {
            switch (rng(1, 4)) {
            case 1:
                ter_set(rng(0, 2), rng(0, 2), t_slope_up);
            case 2:
                ter_set(rng(0, 2), SEEY * 2 - rng(1, 3), t_slope_up);
            case 3:
                ter_set(SEEX * 2 - rng(1, 3), rng(0, 2), t_slope_up);
            case 4:
                ter_set(SEEX * 2 - rng(1, 3), SEEY * 2 - rng(1, 3), t_slope_up);
            }
        }

        add_spawn("mon_blob", 8, SEEX, SEEY);
        place_items("sewer", 40, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);


    } else if (terrain_type == "triffid_grove") {

        fill_background(this, t_dirt);
        for (int rad = 5; rad < SEEX - 2; rad += rng(2, 3)) {
            square(this, t_tree, rad, rad, 23 - rad, 23 - rad);
            square(this, t_dirt, rad + 1, rad + 1, 22 - rad, 22 - rad);
            if (one_in(2)) { // Vertical side opening
                int x = (one_in(2) ? rad : 23 - rad), y = rng(rad + 1, 22 - rad);
                ter_set(x, y, t_dirt);
            } else { // Horizontal side opening
                int x = rng(rad + 1, 22 - rad), y = (one_in(2) ? rad : 23 - rad);
                ter_set(x, y, t_dirt);
            }
            add_spawn( (one_in(3) ? "mon_biollante" : "mon_triffid"), 1, rad + 1, rad + 1);
            add_spawn( (one_in(3) ? "mon_biollante" : "mon_triffid"), 1, 22 - rad, rad + 1);
            add_spawn( (one_in(3) ? "mon_biollante" : "mon_triffid"), 1, rad + 1, 22 - rad);
            add_spawn( (one_in(3) ? "mon_biollante" : "mon_triffid"), 1, 22 - rad, 22 - rad);
        }
        square(this, t_slope_down, SEEX - 1, SEEY - 1, SEEX, SEEY);


    } else if (terrain_type == "triffid_roots") {

        fill_background(this, t_root_wall);
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
            int nodex = 1 + 6 * (node % 4), nodey = 1 + 6 * int(node / 4);
            // Clear a 4x4 dirt square
            square(this, t_dirt, nodex, nodey, nodex + 3, nodey + 3);
            // Spawn a monster in there
            if (step > 2) { // First couple of chambers are safe
                int monrng = rng(1, 25);
                int spawnx = nodex + rng(0, 3), spawny = nodey + rng(0, 3);
                if (monrng <= 5) {
                    add_spawn("mon_triffid", rng(1, 4), spawnx, spawny);
                } else if (monrng <= 13) {
                    add_spawn("mon_creeper_hub", 1, spawnx, spawny);
                } else if (monrng <= 19) {
                    add_spawn("mon_biollante", 1, spawnx, spawny);
                } else if (monrng <= 24) {
                    add_spawn("mon_fungal_fighter", 1, spawnx, spawny);
                } else {
                    for (int webx = nodex; webx <= nodex + 3; webx++) {
                        for (int weby = nodey; weby <= nodey + 3; weby++) {
                            add_field(webx, weby, fd_web, rng(1, 3));
                        }
                    }
                    add_spawn("mon_spider_web", 1, spawnx, spawny);
                }
            }
            // TODO: Non-monster hazards?
            // Next, pick a cell to move to
            std::vector<direction> move;
            if (node % 4 > 0 && !node_built[node - 1]) {
                move.push_back(WEST);
            }
            if (node % 4 < 3 && !node_built[node + 1]) {
                move.push_back(EAST);
            }
            if (int(node / 4) > 0 && !node_built[node - 4]) {
                move.push_back(NORTH);
            }
            if (int(node / 4) < 3 && !node_built[node + 4]) {
                move.push_back(SOUTH);
            }

            if (move.empty()) { // Nowhere to go!
                square(this, t_slope_down, nodex + 1, nodey + 1, nodex + 2, nodey + 2);
                done = true;
            } else {
                int index = rng(0, move.size() - 1);
                switch (move[index]) {
                case NORTH:
                    square(this, t_dirt, nodex + 1, nodey - 2, nodex + 2, nodey - 1);
                    node -= 4;
                    break;
                case EAST:
                    square(this, t_dirt, nodex + 4, nodey + 1, nodex + 5, nodey + 2);
                    node++;
                    break;
                case SOUTH:
                    square(this, t_dirt, nodex + 1, nodey + 4, nodex + 2, nodey + 5);
                    node += 4;
                    break;
                case WEST:
                    square(this, t_dirt, nodex - 2, nodey + 1, nodex - 1, nodey + 2);
                    node--;
                    break;
                default:
                    break;
                }
            }
        } while (!done);
        square(this, t_slope_up, 2, 2, 3, 3);
        rotate(rng(0, 3));


    } else if (terrain_type == "triffid_finale") {

        fill_background(this, t_root_wall);
        square(this, t_dirt, 1, 1, 4, 4);
        square(this, t_dirt, 19, 19, 22, 22);
        // Drunken walk until we reach the heart (lower right, [19, 19])
        // Chance increases by 1 each turn, and gives the % chance of forcing a move
        // to the right or down.
        int chance = 0;
        int x = 4, y = 4;
        do {
            ter_set(x, y, t_dirt);

            if (chance >= 10 && one_in(10)) { // Add a spawn
                if (one_in(2)) {
                    add_spawn("mon_biollante", 1, x, y);
                } else if (!one_in(4)) {
                    add_spawn("mon_creeper_hub", 1, x, y);
                } else {
                    add_spawn("mon_triffid", 1, x, y);
                }
            }

            if (rng(0, 99) < chance) { // Force movement down or to the right
                if (x >= 19) {
                    y++;
                } else if (y >= 19) {
                    x++;
                } else {
                    if (one_in(2)) {
                        x++;
                    } else {
                        y++;
                    }
                }
            } else {
                chance++; // Increase chance of forced movement down/right
                // Weigh movement towards directions with lots of existing walls
                int chance_west = 0, chance_east = 0, chance_north = 0, chance_south = 0;
                for (int dist = 1; dist <= 5; dist++) {
                    if (ter(x - dist, y) == t_root_wall) {
                        chance_west++;
                    }
                    if (ter(x + dist, y) == t_root_wall) {
                        chance_east++;
                    }
                    if (ter(x, y - dist) == t_root_wall) {
                        chance_north++;
                    }
                    if (ter(x, y + dist) == t_root_wall) {
                        chance_south++;
                    }
                }
                int roll = rng(0, chance_west + chance_east + chance_north + chance_south);
                if (roll < chance_west && x > 0) {
                    x--;
                } else if (roll < chance_west + chance_east && x < 23) {
                    x++;
                } else if (roll < chance_west + chance_east + chance_north && y > 0) {
                    y--;
                } else if (y < 23) {
                    y++;
                }
            } // Done with drunken walk
        } while (x < 19 || y < 19);
        square(this, t_slope_up, 1, 1, 2, 2);
        add_spawn("mon_triffid_heart", 1, 21, 21);

    } else {
        // not one of the hardcoded ones!
        // load from JSON???
        debugmsg("Error: tried to generate map for omtype %s, \"%s\" (id_mapgen %s)",
                 terrain_type.c_str(), otermap[terrain_type].name.c_str(), function_key.c_str() );
        fill_background(this, t_floor);

    }}
    // THE END OF THE HUGE IF ELIF ELIF ELIF ELI FLIE FLIE FLIE FEL OMFG

    // WTF it is still going?...
    // omg why are there two braces I'm so dizzy with vertigo

    // Now, fix sewers and subways so that they interconnect.

    if (is_ot_type("subway", terrain_type)) { // FUUUUU it's IF ELIF ELIF ELIF's mini-me =[
        if (is_ot_type("sewer", t_north) &&
            !connects_to(terrain_type, 0)) {
            if (connects_to(t_north, 2)) {
                for (int i = SEEX - 2; i < SEEX + 2; i++) {
                    for (int j = 0; j < SEEY; j++) {
                        ter_set(i, j, t_sewage);
                    }
                }
            } else {
                for (int j = 0; j < 3; j++) {
                    ter_set(SEEX, j, t_rock_floor);
                    ter_set(SEEX - 1, j, t_rock_floor);
                }
                ter_set(SEEX, 3, t_door_metal_c);
                ter_set(SEEX - 1, 3, t_door_metal_c);
            }
        }
        if (is_ot_type("sewer", t_east) &&
            !connects_to(terrain_type, 1)) {
            if (connects_to(t_east, 3)) {
                for (int i = SEEX; i < SEEX * 2; i++) {
                    for (int j = SEEY - 2; j < SEEY + 2; j++) {
                        ter_set(i, j, t_sewage);
                    }
                }
            } else {
                for (int i = SEEX * 2 - 3; i < SEEX * 2; i++) {
                    ter_set(i, SEEY, t_rock_floor);
                    ter_set(i, SEEY - 1, t_rock_floor);
                }
                ter_set(SEEX * 2 - 4, SEEY, t_door_metal_c);
                ter_set(SEEX * 2 - 4, SEEY - 1, t_door_metal_c);
            }
        }
        if (is_ot_type("sewer", t_south) &&
            !connects_to(terrain_type, 2)) {
            if (connects_to(t_south, 0)) {
                for (int i = SEEX - 2; i < SEEX + 2; i++) {
                    for (int j = SEEY; j < SEEY * 2; j++) {
                        ter_set(i, j, t_sewage);
                    }
                }
            } else {
                for (int j = SEEY * 2 - 3; j < SEEY * 2; j++) {
                    ter_set(SEEX, j, t_rock_floor);
                    ter_set(SEEX - 1, j, t_rock_floor);
                }
                ter_set(SEEX, SEEY * 2 - 4, t_door_metal_c);
                ter_set(SEEX - 1, SEEY * 2 - 4, t_door_metal_c);
            }
        }
        if (is_ot_type("sewer", t_west) &&
            !connects_to(terrain_type, 3)) {
            if (connects_to(t_west, 1)) {
                for (int i = 0; i < SEEX; i++) {
                    for (int j = SEEY - 2; j < SEEY + 2; j++) {
                        ter_set(i, j, t_sewage);
                    }
                }
            } else {
                for (int i = 0; i < 3; i++) {
                    ter_set(i, SEEY, t_rock_floor);
                    ter_set(i, SEEY - 1, t_rock_floor);
                }
                ter_set(3, SEEY, t_door_metal_c);
                ter_set(3, SEEY - 1, t_door_metal_c);
            }
        }
    } else if (is_ot_type("sewer", terrain_type)) {
        if (t_above == "road_nesw_manhole") {
            ter_set(rng(SEEX - 2, SEEX + 1), rng(SEEY - 2, SEEY + 1), t_ladder_up);
        }
        if (is_ot_type("subway", t_north) &&
            !connects_to(terrain_type, 0)) {
            for (int j = 0; j < SEEY - 3; j++) {
                ter_set(SEEX, j, t_rock_floor);
                ter_set(SEEX - 1, j, t_rock_floor);
            }
            ter_set(SEEX, SEEY - 3, t_door_metal_c);
            ter_set(SEEX - 1, SEEY - 3, t_door_metal_c);
        }
        if (is_ot_type("subway", t_east) &&
            !connects_to(terrain_type, 1)) {
            for (int i = SEEX + 3; i < SEEX * 2; i++) {
                ter_set(i, SEEY, t_rock_floor);
                ter_set(i, SEEY - 1, t_rock_floor);
            }
            ter_set(SEEX + 2, SEEY, t_door_metal_c);
            ter_set(SEEX + 2, SEEY - 1, t_door_metal_c);
        }
        if (is_ot_type("subway", t_south) &&
            !connects_to(terrain_type, 2)) {
            for (int j = SEEY + 3; j < SEEY * 2; j++) {
                ter_set(SEEX, j, t_rock_floor);
                ter_set(SEEX - 1, j, t_rock_floor);
            }
            ter_set(SEEX, SEEY + 2, t_door_metal_c);
            ter_set(SEEX - 1, SEEY + 2, t_door_metal_c);
        }
        if (is_ot_type("subway", t_west) &&
            !connects_to(terrain_type, 3)) {
            for (int i = 0; i < SEEX - 3; i++) {
                ter_set(i, SEEY, t_rock_floor);
                ter_set(i, SEEY - 1, t_rock_floor);
            }
            ter_set(SEEX - 3, SEEY, t_door_metal_c);
            ter_set(SEEX - 3, SEEY - 1, t_door_metal_c);
        }
    } else if (is_ot_type("ants", terrain_type)) {
        if (t_above == "anthill") {
            bool done = false;
            do {
                int x = rng(0, SEEX * 2 - 1), y = rng(0, SEEY * 2 - 1);
                if (ter(x, y) == t_rock_floor) {
                    done = true;
                    ter_set(x, y, t_slope_up);
                }
            } while (!done);
        }
    }

}

void map::post_process(unsigned zones)
{
    if (zones & mfb(OMZONE_CITY)) {
        if (!one_in(10)) { // 90% chance of smashing stuff up
            for (int x = 0; x < 24; x++) {
                for (int y = 0; y < 24; y++) {
                    bash(x, y, 20, true);
                }
            }
        }
        if (one_in(10)) { // 10% chance of corpses
            int num_corpses = rng(1, 8);
            for (int i = 0; i < num_corpses; i++) {
                int x = rng(0, 23), y = rng(0, 23);
                if (move_cost(x, y) > 0) {
                    add_corpse(x, y);
                }
            }
        }
    } // OMZONE_CITY

    if (zones & mfb(OMZONE_BOMBED)) {
        while (one_in(4)) {
            point center( rng(4, 19), rng(4, 19) );
            int radius = rng(1, 4);
            for (int x = center.x - radius; x <= center.x + radius; x++) {
                for (int y = center.y - radius; y <= center.y + radius; y++) {
                    if (rl_dist(x, y, center.x, center.y) <= rng(1, radius)) {
                        destroy(x, y, true);
                    }
                }
            }
        }
    }

}

void map::place_spawns(std::string group, const int chance,
                       const int x1, const int y1, const int x2, const int y2, const float density)
{
    if (!ACTIVE_WORLD_OPTIONS["STATIC_SPAWN"]) {
        return;
    }

    if( !MonsterGroupManager::isValidMonsterGroup( group ) ) {
        const point omt = overmapbuffer::sm_to_omt_copy( get_abs_sub().x, get_abs_sub().y );
        const oter_id &oid = overmap_buffer.ter( omt.x, omt.y, get_abs_sub().z );
        debugmsg("place_spawns: invalid mongroup '%s', om_terrain = '%s' (%s)", group.c_str(), oid.t().id.c_str(), oid.t().id_mapgen.c_str() );
        return;
    }

    float multiplier = ACTIVE_WORLD_OPTIONS["SPAWN_DENSITY"];

    if( multiplier == 0.0 ) {
        return;
    }

    if (one_in(chance / multiplier)) {
        int num = density * (float)rng(10, 50) * multiplier;

        for (int i = 0; i < num; i++) {
            int tries = 10;
            int x = 0;
            int y = 0;

            // Pick a spot for the spawn
            do {
                x = rng(x1, x2);
                y = rng(y1, y2);
                tries--;
            } while( move_cost(x, y) == 0 && tries );

            // Pick a monster type
            MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup( group, &num );

            add_spawn(spawn_details.name, spawn_details.pack_size, x, y);
        }
    }
}

void map::place_gas_pump(int x, int y, int charges)
{
    if (one_in(4)) {
        item diesel("diesel", 0);
        diesel.charges = charges;
        add_item(x, y, diesel);
        ter_set(x, y, t_diesel_pump);
    } else {
        item gas("gasoline", 0);
        gas.charges = charges;
        add_item(x, y, gas);
        ter_set(x, y, t_gas_pump);
    }
}

void map::place_toilet(int x, int y, int charges)
{
    item water("water", 0);
    water.charges = charges;
    add_item(x, y, water);
    furn_set(x, y, f_toilet);
}

void map::place_vending(int x, int y, std::string type)
{
    const bool broken = one_in(5);
    if( broken ) {
        furn_set(x, y, f_vending_o);
    } else {
        furn_set(x, y, f_vending_c);
    }
    place_items(type, broken ? 40 : 99, x, y, x, y, false, 0, false);
}

int map::place_npc(int x, int y, std::string type)
{
    if(!ACTIVE_WORLD_OPTIONS["STATIC_NPC"]) {
        return -1; //Do not generate an npc.
    }
    npc *temp = new npc();
    temp->normalize();
    temp->load_npc_template(type);
    temp->spawn_at(abs_sub.x, abs_sub.y, abs_sub.z);
    temp->setx( x );
    temp->sety( y );
    g->load_npcs();
    return temp->getID();
}

// A chance of 100 indicates that items should always spawn,
// the item group should be responsible for determining the amount of items.
int map::place_items(items_location loc, int chance, int x1, int y1,
                     int x2, int y2, bool ongrass, int turn, bool)
{
    const float spawn_rate = ACTIVE_WORLD_OPTIONS["ITEM_SPAWNRATE"];

    if (chance > 100 || chance <= 0) {
        debugmsg("map::place_items() called with an invalid chance (%d)", chance);
        return 0;
    }
    if (!item_group::group_is_defined(loc)) {
        const point omt = overmapbuffer::sm_to_omt_copy( get_abs_sub().x, get_abs_sub().y );
        const oter_id &oid = overmap_buffer.ter( omt.x, omt.y, get_abs_sub().z );
        debugmsg("place_items: invalid item group '%s', om_terrain = '%s' (%s)",
                 loc.c_str(), oid.t().id.c_str(), oid.t().id_mapgen.c_str() );
        return 0;
    }

    int px, py;
    int item_num = 0;
    while (chance == 100 || rng(0, 99) < chance) {
        float lets_spawn = spawn_rate;
        while( rng_float( 0.0, 1.0 ) <= lets_spawn ) {
            lets_spawn -= 1.0;

            // Might contain one item or several that belong together like guns & their ammo
            int tries = 0;
            do {
                px = rng(x1, x2);
                py = rng(y1, y2);
                tries++;
                // Only place on valid terrain
            } while (( (terlist[ter(px, py)].movecost == 0 &&
                        !(terlist[ter(px, py)].has_flag("PLACE_ITEM")) ) &&
                       (!ongrass && !terlist[ter(px, py)].has_flag("FLAT")) ) &&
                     tries < 20);
            if (tries < 20) {
                item_num += put_items_from_loc( loc, px, py, turn );
            }
        }
        if (chance == 100) {
            break;
        }
    }
    return item_num;
}

int map::put_items_from_loc(items_location loc, int x, int y, int turn)
{
    const auto items = item_group::items_from(loc, turn);
    spawn_items(x, y, items);
    return items.size();
}

void map::add_spawn(std::string type, int count, int x, int y, bool friendly,
                    int faction_id, int mission_id, std::string name)
{
    if (x < 0 || x >= SEEX * my_MAPSIZE || y < 0 || y >= SEEY * my_MAPSIZE) {
        debugmsg("Bad add_spawn(%s, %d, %d, %d)", type.c_str(), count, x, y);
        return;
    }
    int offset_x, offset_y;
    submap *place_on_submap = get_submap_at(x, y, offset_x, offset_y);

    if(!place_on_submap) {
        debugmsg("centadodecamonant doesn't exist in grid; within add_spawn(%s, %d, %d, %d)",
                 type.c_str(), count, x, y);
        return;
    }
    if( ACTIVE_WORLD_OPTIONS["CLASSIC_ZOMBIES"] && !GetMType(type)->in_category("CLASSIC") &&
        !GetMType(type)->in_category("WILDLIFE") ) {
        // Don't spawn non-classic monsters in classic zombie mode.
        return;
    }
    if (monster_is_blacklisted(GetMType(type))) {
        return;
    }
    spawn_point tmp(type, count, offset_x, offset_y, faction_id, mission_id, friendly, name);
    place_on_submap->spawns.push_back(tmp);
}

vehicle *map::add_vehicle(std::string type, const int x, const int y, const int dir,
                          const int veh_fuel, const int veh_status, const bool merge_wrecks)
{
    if(g->vtypes.count(type) == 0) {
        debugmsg("Nonexistent vehicle type: \"%s\"", type.c_str());
        return NULL;
    }
    if (x < 0 || x >= SEEX * my_MAPSIZE || y < 0 || y >= SEEY * my_MAPSIZE) {
        debugmsg("Out of bounds add_vehicle t=%s d=%d x=%d y=%d", type.c_str(), dir, x, y);
        return NULL;
    }
    // debugmsg("add_vehicle t=%d d=%d x=%d y=%d", type, dir, x, y);

    const int smx = x / SEEX;
    const int smy = y / SEEY;
    // debugmsg("n=%d x=%d y=%d MAPSIZE=%d ^2=%d", nonant, x, y, MAPSIZE, MAPSIZE*MAPSIZE);
    vehicle *veh = new vehicle(type, veh_fuel, veh_status);
    veh->posx = x % SEEX;
    veh->posy = y % SEEY;
    veh->smx = smx;
    veh->smy = smy;
    veh->place_spawn_items();
    veh->face.init( dir );
    veh->turn_dir = dir;
    veh->precalc_mounts( 0, dir );
    // veh->init_veh_fuel = 50;
    // veh->init_veh_status = 0;

    vehicle *placed_vehicle = add_vehicle_to_map(veh, merge_wrecks);

    if(placed_vehicle != NULL) {
        submap *place_on_submap = get_submap_at_grid(placed_vehicle->smx, placed_vehicle->smy);
        place_on_submap->vehicles.push_back(placed_vehicle);

        vehicle_list.insert(placed_vehicle);
        update_vehicle_cache(placed_vehicle, true);

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
 * @return The vehicle that was finally placed.
 */
vehicle *map::add_vehicle_to_map(vehicle *veh, const bool merge_wrecks)
{
    //We only want to check once per square, so loop over all structural parts
    std::vector<int> frame_indices = veh->all_parts_at_location("structure");
    for (std::vector<int>::const_iterator part = frame_indices.begin();
         part != frame_indices.end(); part++) {
        const auto p = veh->global_pos() + veh->parts[*part].precalc[0];

        //Don't spawn anything in water
        if (ter_at(p.x, p.y).has_flag(TFLAG_DEEP_WATER)) {
            delete veh;
            return NULL;
        }

        // Don't spawn shopping carts on top of another vehicle or other obstacle.
        if (veh->type == "shopping_cart") {
            if (veh_at(p.x, p.y) != NULL || move_cost(p.x, p.y) == 0) {
                delete veh;
                return NULL;
            }
        }

        //When hitting a wall, only smash the vehicle once (but walls many times)
        bool veh_smashed = false;
        //For other vehicles, simulate collisions with (non-shopping cart) stuff
        vehicle *other_veh = veh_at(p.x, p.y);
        if (other_veh != NULL && other_veh->type != "shopping cart") {
            if( !merge_wrecks ) {
                delete veh;
                return NULL;
            }

            /* There's a vehicle here, so let's fuse them together into wreckage and
             * smash them up. It'll look like a nasty collision has occurred.
             * Trying to do a local->global->local conversion would be a major
             * headache, so instead, let's make another vehicle whose (0, 0) point
             * is the (0, 0) of the existing vehicle, convert the coordinates of both
             * vehicles into global coordinates, find the distance between them and
             * p and then install them that way.
             * Create a vehicle with type "null" so it starts out empty. */
            vehicle *wreckage = new vehicle();
            wreckage->posx = other_veh->posx;
            wreckage->posy = other_veh->posy;
            wreckage->smx = other_veh->smx;
            wreckage->smy = other_veh->smy;

            //Where are we on the global scale?
            const int global_x = wreckage->smx * SEEX + wreckage->posx;
            const int global_y = wreckage->smy * SEEY + wreckage->posy;

            for (auto &part_index : veh->parts) {

                const int local_x = (veh->smx * SEEX + veh->posx) +
                                     part_index.precalc[0].x - global_x;
                const int local_y = (veh->smy * SEEY + veh->posy) +
                                     part_index.precalc[0].y - global_y;

                wreckage->install_part(local_x, local_y, part_index);

            }
            for (auto &part_index : other_veh->parts) {

                const int local_x = (other_veh->smx * SEEX + other_veh->posx) +
                                     part_index.precalc[0].x - global_x;
                const int local_y = (other_veh->smy * SEEY + other_veh->posy) +
                                     part_index.precalc[0].y - global_y;

                wreckage->install_part(local_x, local_y, part_index);

            }

            wreckage->name = _("Wreckage");
            wreckage->smash();

            //Now get rid of the old vehicles
            destroy_vehicle(other_veh);
            delete veh;

            //Try again with the wreckage
            return add_vehicle_to_map(wreckage, true);

        } else if (move_cost(p.x, p.y) == 0) {
            if( !merge_wrecks ) {
                delete veh;
                return NULL;
            }

            //There's a wall or other obstacle here; destroy it
            destroy(p.x, p.y, true);

            //Then smash up the vehicle
            if(!veh_smashed) {
                veh->smash();
                veh_smashed = true;
            }

        }
    }

    return veh;
}

computer *map::add_computer(int x, int y, std::string name, int security)
{
    ter_set(x, y, t_console); // TODO: Turn this off?
    submap *place_on_submap = get_submap_at(x, y);
    place_on_submap->comp = computer(name, security);
    return &(place_on_submap->comp);
}

/**
 * Rotates this map, and all of its contents, by the specified multiple of 90
 * degrees.
 * @param turns How many 90-degree turns to rotate the map.
 */
void map::rotate(int turns)
{

    //Handle anything outside the 1-3 range gracefully; rotate(0) is a no-op.
    turns = turns % 4;
    if(turns == 0) {
        return;
    }

    real_coords rc;
    rc.fromabs(get_abs_sub().x*SEEX, get_abs_sub().y*SEEY);

    const int radius = int(MAPSIZE / 2) + 3;
    // uses submap coordinates
    std::vector<npc*> npcs = overmap_buffer.get_npcs_near_player(radius);
    for (auto &i : npcs) {
        npc *act_npc = i;
        if (act_npc->global_omt_location().x*2 == get_abs_sub().x &&
            act_npc->global_omt_location().y*2 == get_abs_sub().y ){
                rc.fromabs(act_npc->global_square_location().x, act_npc->global_square_location().y);
                int old_x = rc.sub_pos.x;
                int old_y = rc.sub_pos.y;
                if ( rc.om_sub.x % 2 != 0 )
                    old_x += SEEX;
                if ( rc.om_sub.y % 2 != 0 )
                    old_y += SEEY;
                int new_x = old_x;
                int new_y = old_y;
                switch(turns) {
                    case 3:
                        new_x = old_y;
                        new_y = SEEX * 2 - 1 - old_x;
                        break;
                    case 2:
                        new_x = SEEX * 2 - 1 - old_x;
                        new_y = SEEY * 2 - 1 - old_y;
                        break;
                    case 1:
                        new_x = SEEY * 2 - 1 - old_y;
                        new_y = old_x;
                        break;
                    }
                i->setx( i->posx() + new_x - old_x );
                i->sety( i->posy() + new_y - old_y );
            }
    }
    ter_id rotated [SEEX * 2][SEEY * 2];
    furn_id furnrot [SEEX * 2][SEEY * 2];
    trap_id traprot [SEEX * 2][SEEY * 2];
    std::vector<item> itrot[SEEX * 2][SEEY * 2];
    std::map<std::string, std::string> cosmetics_rot[SEEX * 2][SEEY * 2];
    field fldrot [SEEX * 2][SEEY * 2];
    int radrot [SEEX * 2][SEEY * 2];

    std::vector<spawn_point> sprot[MAPSIZE * MAPSIZE];
    std::vector<vehicle*> vehrot[MAPSIZE * MAPSIZE];
    computer tmpcomp[MAPSIZE * MAPSIZE];
    int field_count[MAPSIZE * MAPSIZE];
    int temperature[MAPSIZE * MAPSIZE];

    //Rotate terrain first
    for (int old_x = 0; old_x < SEEX * 2; old_x++) {
        for (int old_y = 0; old_y < SEEY * 2; old_y++) {
            int new_x = old_x;
            int new_y = old_y;
            switch(turns) {
            case 1:
                new_x = old_y;
                new_y = SEEX * 2 - 1 - old_x;
                break;
            case 2:
                new_x = SEEX * 2 - 1 - old_x;
                new_y = SEEY * 2 - 1 - old_y;
                break;
            case 3:
                new_x = SEEY * 2 - 1 - old_y;
                new_y = old_x;
                break;
            }
            int new_lx, new_ly;
            const auto new_sm = get_submap_at( new_x, new_y, new_lx, new_ly );
            new_sm->is_uniform = false;
            std::swap( rotated[old_x][old_y], new_sm->ter[new_lx][new_ly] );
            std::swap( furnrot[old_x][old_y], new_sm->frn[new_lx][new_ly] );
            std::swap( traprot[old_x][old_y], new_sm->trp[new_lx][new_ly] );
            std::swap( fldrot[old_x][old_y], new_sm->fld[new_lx][new_ly] );
            std::swap( radrot[old_x][old_y], new_sm->rad[new_lx][new_ly] );
            std::swap( cosmetics_rot[old_x][old_y], new_sm->cosmetics[new_lx][new_ly] );
            auto items = i_at(new_x, new_y);
            itrot[old_x][old_y].reserve( items.size() );
            // Copy items, if we move them, it'll wreck i_clear().
            std::copy( items.begin(), items.end(), std::back_inserter(itrot[old_x][old_y]) );
            i_clear(new_x, new_y);
        }
    }

    //Next, spawn points
    for (int sx = 0; sx < 2; sx++) {
        for (int sy = 0; sy < 2; sy++) {
            const auto from = get_submap_at_grid( sx, sy );
            size_t gridto = 0;
            switch(turns) {
            case 0:
                gridto = get_nonant( sx, sy );
                break;
            case 1:
                gridto = get_nonant( 1 - sy, sx );
                break;
            case 2:
                gridto = get_nonant( 1 - sx, 1 - sy );
                break;
            case 3:
                gridto = get_nonant( sy, 1 - sx );
                break;
            }
            for (auto &spawn : from->spawns) {
                spawn_point tmp = spawn;
                int new_x = tmp.posx;
                int new_y = tmp.posy;
                switch(turns) {
                case 1:
                    new_x = SEEY - 1 - tmp.posy;
                    new_y = tmp.posx;
                    break;
                case 2:
                    new_x = SEEX - 1 - tmp.posx;
                    new_y = SEEY - 1 - tmp.posy;
                    break;
                case 3:
                    new_x = tmp.posy;
                    new_y = SEEX - 1 - tmp.posx;
                    break;
                }
                tmp.posx = new_x;
                tmp.posy = new_y;
                sprot[gridto].push_back(tmp);
            }
            // as vehrot starts out empty, this clears the other vehicles vector
            vehrot[gridto].swap(from->vehicles);
            tmpcomp[gridto] = from->comp;
            field_count[gridto] = from->field_count;
            temperature[gridto] = from->temperature;
        }
    }

    // change vehicles' directions
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            const size_t i = get_nonant( gridx, gridy );
            for (auto &v : vehrot[i]) {
                vehicle *veh = v;
                // turn the steering wheel, vehicle::turn does not actually
                // move the vehicle.
                veh->turn(turns * 90);
                // The the facing direction and recalculate the positions of the parts
                veh->face = veh->turn_dir;
                veh->precalc_mounts(0, veh->turn_dir);
                // Update coordinates on a submap
                int new_x = veh->posx;
                int new_y = veh->posy;
                switch(turns) {
                case 1:
                    new_x = SEEY - 1 - veh->posy;
                    new_y = veh->posx;
                    break;
                case 2:
                    new_x = SEEX - 1 - veh->posx;
                    new_y = SEEY - 1 - veh->posy;
                    break;
                case 3:
                    new_x = veh->posy;
                    new_y = SEEX - 1 - veh->posx;
                    break;
                }
                veh->posx = new_x;
                veh->posy = new_y;
                // Update submap coordinates
                new_x = veh->smx;
                new_y = veh->smy;
                switch(turns) {
                case 1:
                    new_x = 1 - veh->smy;
                    new_y = veh->smx;
                    break;
                case 2:
                    new_x = 1 - veh->smx;
                    new_y = 1 - veh->smy;
                    break;
                case 3:
                    new_x = veh->smy;
                    new_y = 1 - veh->smx;
                    break;
                }
                veh->smx = new_x;
                veh->smy = new_y;
            }
            const auto to = getsubmap( i );
            // move back to the actuall submap object, vehrot is only temporary
            vehrot[i].swap(to->vehicles);
            sprot[i].swap(to->spawns);
            to->comp = tmpcomp[i];
            to->field_count = field_count[i];
            to->temperature = temperature[i];
        }
    }

    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            int lx, ly;
            const auto sm = get_submap_at( i, j, lx, ly );
            sm->is_uniform = false;
            std::swap( rotated[i][j], sm->ter[lx][ly] );
            std::swap( furnrot[i][j], sm->frn[lx][ly] );
            std::swap( traprot[i][j], sm->trp[lx][ly] );
            std::swap( fldrot[i][j], sm->fld[lx][ly] );
            std::swap( radrot[i][j], sm->rad[lx][ly] );
            std::swap( cosmetics_rot[i][j], sm->cosmetics[lx][ly] );
            for( auto &itm : itrot[i][j] ) {
                add_item( i, j, itm );
            }
            if (turns % 2 == 1) { // Rotate things like walls 90 degrees
                if (ter(i, j) == t_wall_v) {
                    ter_set(i, j, t_wall_h);
                } else if (ter(i, j) == t_wall_h) {
                    ter_set(i, j, t_wall_v);
                } else if (ter(i, j) == t_wall_metal_v) {
                    ter_set(i, j, t_wall_metal_h);
                } else if (ter(i, j) == t_wall_metal_h) {
                    ter_set(i, j, t_wall_metal_v);
                } else if (ter(i, j) == t_railing_v) {
                    ter_set(i, j, t_railing_h);
                } else if (ter(i, j) == t_railing_h) {
                    ter_set(i, j, t_railing_v);
                } else if (ter(i, j) == t_wall_glass_h) {
                    ter_set(i, j, t_wall_glass_v);
                } else if (ter(i, j) == t_wall_glass_v) {
                    ter_set(i, j, t_wall_glass_h);
                } else if (ter(i, j) == t_wall_glass_h_alarm) {
                    ter_set(i, j, t_wall_glass_v_alarm);
                } else if (ter(i, j) == t_wall_glass_v_alarm) {
                    ter_set(i, j, t_wall_glass_h_alarm);
                } else if (ter(i, j) == t_reinforced_glass_h) {
                    ter_set(i, j, t_reinforced_glass_v);
                } else if (ter(i, j) == t_reinforced_glass_v) {
                    ter_set(i, j, t_reinforced_glass_h);
                } else if (ter(i, j) == t_fence_v) {
                    ter_set(i, j, t_fence_h);
                } else if (ter(i, j) == t_fence_h) {
                    ter_set(i, j, t_fence_v);
                } else if (ter(i, j) == t_chainfence_h) {
                    ter_set(i, j, t_chainfence_v);
                } else if (ter(i, j) == t_chainfence_v) {
                    ter_set(i, j, t_chainfence_h);
                } else if (ter(i, j) == t_concrete_h) {
                    ter_set(i, j, t_concrete_v);
                } else if (ter(i, j) == t_concrete_v) {
                    ter_set(i, j, t_concrete_h);
                }
            }
        }
    }
}

// Hideous function, I admit...
bool connects_to(oter_id there, int dir)
{
    switch (dir) {
    case 2:
        if (there == "subway_ns"  || there == "subway_es" || there == "subway_sw" ||
            there == "subway_nes" || there == "subway_nsw" ||
            there == "subway_esw" || there == "subway_nesw" ||
            there == "sewer_ns"   || there == "sewer_es" || there == "sewer_sw" ||
            there == "sewer_nes"  || there == "sewer_nsw" || there == "sewer_esw" ||
            there == "sewer_nesw" || there == "ants_ns" || there == "ants_es" ||
            there == "ants_sw"    || there == "ants_nes" ||  there == "ants_nsw" ||
            there == "ants_esw"   || there == "ants_nesw") {
            return true;
        }
        return false;
    case 3:
        if (there == "subway_ew"  || there == "subway_sw" || there == "subway_wn" ||
            there == "subway_new" || there == "subway_nsw" ||
            there == "subway_esw" || there == "subway_nesw" ||
            there == "sewer_ew"   || there == "sewer_sw" || there == "sewer_wn" ||
            there == "sewer_new"  || there == "sewer_nsw" || there == "sewer_esw" ||
            there == "sewer_nesw" || there == "ants_ew" || there == "ants_sw" ||
            there == "ants_wn"    || there == "ants_new" || there == "ants_nsw" ||
            there == "ants_esw"   || there == "ants_nesw") {
            return true;
        }
        return false;
    case 0:
        if (there == "subway_ns"  || there == "subway_ne" || there == "subway_wn" ||
            there == "subway_nes" || there == "subway_new" ||
            there == "subway_nsw" || there == "subway_nesw" ||
            there == "sewer_ns"   || there == "sewer_ne" ||  there == "sewer_wn" ||
            there == "sewer_nes"  || there == "sewer_new" || there == "sewer_nsw" ||
            there == "sewer_nesw" || there == "ants_ns" || there == "ants_ne" ||
            there == "ants_wn"    || there == "ants_nes" || there == "ants_new" ||
            there == "ants_nsw"   || there == "ants_nesw") {
            return true;
        }
        return false;
    case 1:
        if (there == "subway_ew"  || there == "subway_ne" || there == "subway_es" ||
            there == "subway_nes" || there == "subway_new" ||
            there == "subway_esw" || there == "subway_nesw" ||
            there == "sewer_ew"   || there == "sewer_ne" || there == "sewer_es" ||
            there == "sewer_nes"  || there == "sewer_new" || there == "sewer_esw" ||
            there == "sewer_nesw" || there == "ants_ew" || there == "ants_ne" ||
            there == "ants_es"    || there == "ants_nes" || there == "ants_new" ||
            there == "ants_esw"   || there == "ants_nesw") {
            return true;
        }
        return false;
    default:
        debugmsg("Connects_to with dir of %d", dir);
        return false;
    }
}

void science_room(map *m, int x1, int y1, int x2, int y2, int z, int rotate)
{
    int height = y2 - y1;
    int width  = x2 - x1;
    if (rotate % 2 == 1) { // Swap width & height if we're a lateral room
        int tmp = height;
        height  = width;
        width   = tmp;
    }
    for (int i = x1; i <= x2; i++) {
        for (int j = y1; j <= y2; j++) {
            m->ter_set(i, j, t_rock_floor);
        }
    }
    int area = height * width;
    std::vector<room_type> valid_rooms;
    if (height < 5 && width < 5) {
        valid_rooms.push_back(room_closet);
    }
    if (height > 6 && width > 3) {
        valid_rooms.push_back(room_lobby);
    }
    if (height > 4 || width > 4) {
        valid_rooms.push_back(room_chemistry);
    }
    if ( z != 0 && (height > 7 || width > 7) && height > 2 && width > 2) {
        valid_rooms.push_back(room_teleport);
    }
    if (height > 4 && width > 4) {
        valid_rooms.push_back(room_goo);
    }
    if (height > 7 && width > 7) {
        valid_rooms.push_back(room_bionics);
    }
    if (height > 7 && width > 7) {
        valid_rooms.push_back(room_cloning);
    }
    if (area >= 9) {
        valid_rooms.push_back(room_vivisect);
    }
    if (height > 5 && width > 4) {
        valid_rooms.push_back(room_dorm);
    }
    if (width > 8) {
        for (int i = 8; i < width; i += rng(1, 2)) {
            valid_rooms.push_back(room_split);
        }
    }

    room_type chosen = valid_rooms[rng(0, valid_rooms.size() - 1)];
    int trapx = rng(x1 + 1, x2 - 1);
    int trapy = rng(y1 + 1, y2 - 1);
    switch (chosen) {
    case room_closet:
        m->place_items("cleaning", 80, x1, y1, x2, y2, false, 0);
        break;
    case room_lobby:
        if (rotate % 2 == 0) { // Vertical
            int desk = y1 + rng(int(height / 2) - int(height / 4), int(height / 2) + 1);
            for (int x = x1 + int(width / 4); x < x2 - int(width / 4); x++) {
                m->furn_set(x, desk, f_counter);
            }
            computer *tmpcomp = m->add_computer(x2 - int(width / 4), desk,
                                                _("Log Console"), 3);
            tmpcomp->add_option(_("View Research Logs"), COMPACT_RESEARCH, 0);
            tmpcomp->add_option(_("Download Map Data"), COMPACT_MAPS, 0);
            tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
            tmpcomp->add_failure(COMPFAIL_ALARM);
            tmpcomp->add_failure(COMPFAIL_DAMAGE);
            m->add_spawn("mon_turret", 1, int((x1 + x2) / 2), desk);
        } else {
            int desk = x1 + rng(int(height / 2) - int(height / 4), int(height / 2) + 1);
            for (int y = y1 + int(width / 4); y < y2 - int(width / 4); y++) {
                m->furn_set(desk, y, f_counter);
            }
            computer *tmpcomp = m->add_computer(desk, y2 - int(width / 4),
                                                _("Log Console"), 3);
            tmpcomp->add_option(_("View Research Logs"), COMPACT_RESEARCH, 0);
            tmpcomp->add_option(_("Download Map Data"), COMPACT_MAPS, 0);
            tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
            tmpcomp->add_failure(COMPFAIL_ALARM);
            tmpcomp->add_failure(COMPFAIL_DAMAGE);
            m->add_spawn("mon_turret", 1, desk, int((y1 + y2) / 2));
        }
        break;
    case room_chemistry:
        if (rotate % 2 == 0) { // Vertical
            for (int x = x1; x <= x2; x++) {
                if (x % 3 == 0) {
                    for (int y = y1 + 1; y <= y2 - 1; y++) {
                        m->furn_set(x, y, f_counter);
                    }
                    if (one_in(3)) {
                        m->place_items("mut_lab", 35, x, y1 + 1, x, y2 - 1, false, 0);
                    }
                    else {
                        m->place_items("chem_lab", 70, x, y1 + 1, x, y2 - 1, false, 0);
                    }
                }
            }
        } else {
            for (int y = y1; y <= y2; y++) {
                if (y % 3 == 0) {
                    for (int x = x1 + 1; x <= x2 - 1; x++) {
                        m->furn_set(x, y, f_counter);
                    }
                    if (one_in(3)) {
                        m->place_items("mut_lab", 35, x1 + 1, y, x2 - 1, y, false, 0);
                    }
                    else {
                        m->place_items("chem_lab", 70, x1 + 1, y, x2 - 1, y, false, 0);
                    }
                }
            }
        }
        break;
    case room_teleport:
        m->furn_set(int((x1 + x2) / 2)    , int((y1 + y2) / 2)    , f_counter);
        m->furn_set(int((x1 + x2) / 2) + 1, int((y1 + y2) / 2)    , f_counter);
        m->furn_set(int((x1 + x2) / 2)    , int((y1 + y2) / 2) + 1, f_counter);
        m->furn_set(int((x1 + x2) / 2) + 1, int((y1 + y2) / 2) + 1, f_counter);
        m->add_trap(trapx, trapy, tr_telepad);
        m->place_items("teleport", 70, int((x1 + x2) / 2),
                       int((y1 + y2) / 2), int((x1 + x2) / 2) + 1,
                       int((y1 + y2) / 2) + 1, false, 0);
        break;
    case room_goo:
        do {
            m->add_trap(trapx, trapy, tr_goo);
            trapx = rng(x1 + 1, x2 - 1);
            trapy = rng(y1 + 1, y2 - 1);
        } while(!one_in(5));
        if (rotate == 0) {
            m->remove_trap(x1, y2);
            m->furn_set(x1, y2, f_fridge);
            m->place_items("goo", 60, x1, y2, x1, y2, false, 0);
        } else if (rotate == 1) {
            m->remove_trap(x1, y1);
            m->furn_set(x1, y1, f_fridge);
            m->place_items("goo", 60, x1, y1, x1, y1, false, 0);
        } else if (rotate == 2) {
            m->remove_trap(x2, y1);
            m->furn_set(x2, y1, f_fridge);
            m->place_items("goo", 60, x2, y1, x2, y1, false, 0);
        } else {
            m->remove_trap(x2, y2);
            m->furn_set(x2, y2, f_fridge);
            m->place_items("goo", 60, x2, y2, x2, y2, false, 0);
        }
        break;
    case room_cloning:
        for (int x = x1 + 1; x <= x2 - 1; x++) {
            for (int y = y1 + 1; y <= y2 - 1; y++) {
                if (x % 3 == 0 && y % 3 == 0) {
                    m->ter_set(x, y, t_vat);
                    m->place_items("cloning_vat", 20, x, y, x, y, false, 0);
                }
            }
        }
        break;
    case room_vivisect:
        if        (rotate == 0) {
            for (int x = x1; x <= x2; x++) {
                m->furn_set(x, y2 - 1, f_counter);
            }
            m->place_items("dissection", 80, x1, y2 - 1, x2, y2 - 1, false, 0);
        } else if (rotate == 1) {
            for (int y = y1; y <= y2; y++) {
                m->furn_set(x1 + 1, y, f_counter);
            }
            m->place_items("dissection", 80, x1 + 1, y1, x1 + 1, y2, false, 0);
        } else if (rotate == 2) {
            for (int x = x1; x <= x2; x++) {
                m->furn_set(x, y1 + 1, f_counter);
            }
            m->place_items("dissection", 80, x1, y1 + 1, x2, y1 + 1, false, 0);
        } else if (rotate == 3) {
            for (int y = y1; y <= y2; y++) {
                m->furn_set(x2 - 1, y, f_counter);
            }
            m->place_items("dissection", 80, x2 - 1, y1, x2 - 1, y2, false, 0);
        }
        m->add_trap(int((x1 + x2) / 2), int((y1 + y2) / 2), tr_dissector);
        if (one_in(10)) {
            m->add_spawn("mon_broken_cyborg", 1, int(((x1 + x2) / 2)+1), int(((y1 + y2) / 2)+1));
        }
        break;

    case room_bionics:
        if (rotate % 2 == 0) {
            int biox = x1 + 2, bioy = int((y1 + y2) / 2);
            mapf::formatted_set_simple(m, biox - 1, bioy - 1,
                                       "\
---\n\
|c=\n\
---\n",
                                       mapf::basic_bind("- | =", t_wall_h, t_wall_v, t_reinforced_glass_v),
                                       mapf::basic_bind("c", f_counter));
            m->place_items("bionics_common", 70, biox, bioy, biox, bioy, false, 0);

            m->ter_set(biox, bioy+2, t_console);
            computer *tmpcomp = m->add_computer(biox, bioy+2, _("Bionic access"), 2);
            tmpcomp->add_option(_("Manifest"), COMPACT_LIST_BIONICS, 0);
            tmpcomp->add_option(_("Open Chambers"), COMPACT_RELEASE_BIONICS, 3);
            tmpcomp->add_failure(COMPFAIL_MANHACKS);
            tmpcomp->add_failure(COMPFAIL_SECUBOTS);

            biox = x2 - 2;
            mapf::formatted_set_simple(m, biox - 1, bioy - 1,
                                       "\
---\n\
=c|\n\
---\n",
                                       mapf::basic_bind("- | =", t_wall_h, t_wall_v, t_reinforced_glass_v),
                                       mapf::basic_bind("c", f_counter));
            m->place_items("bionics_common", 70, biox, bioy, biox, bioy, false, 0);

            m->ter_set(biox, bioy-2, t_console);
            computer *tmpcomp2 = m->add_computer(biox, bioy-2, _("Bionic access"), 2);
            tmpcomp2->add_option(_("Manifest"), COMPACT_LIST_BIONICS, 0);
            tmpcomp2->add_option(_("Open Chambers"), COMPACT_RELEASE_BIONICS, 3);
            tmpcomp2->add_failure(COMPFAIL_MANHACKS);
            tmpcomp2->add_failure(COMPFAIL_SECUBOTS);
        } else {
            int bioy = y1 + 2, biox = int((x1 + x2) / 2);
            mapf::formatted_set_simple(m, biox - 1, bioy - 1,
                                       "\
|-|\n\
|c|\n\
|=|\n",
                                       mapf::basic_bind("- | =", t_wall_h, t_wall_v, t_reinforced_glass_h),
                                       mapf::basic_bind("c", f_counter));
            m->place_items("bionics_common", 70, biox, bioy, biox, bioy, false, 0);

            m->ter_set(biox+2, bioy, t_console);
            computer *tmpcomp = m->add_computer(biox+2, bioy, _("Bionic access"), 2);
            tmpcomp->add_option(_("Manifest"), COMPACT_LIST_BIONICS, 0);
            tmpcomp->add_option(_("Open Chambers"), COMPACT_RELEASE_BIONICS, 3);
            tmpcomp->add_failure(COMPFAIL_MANHACKS);
            tmpcomp->add_failure(COMPFAIL_SECUBOTS);

            bioy = y2 - 2;
            mapf::formatted_set_simple(m, biox - 1, bioy - 1,
                                       "\
|=|\n\
|c|\n\
|-|\n",
                                       mapf::basic_bind("- | =", t_wall_h, t_wall_v, t_reinforced_glass_h),
                                       mapf::basic_bind("c", f_counter));
            m->place_items("bionics_common", 70, biox, bioy, biox, bioy, false, 0);

            m->ter_set(biox-2, bioy, t_console);
            computer *tmpcomp2 = m->add_computer(biox-2, bioy, _("Bionic access"), 2);
            tmpcomp2->add_option(_("Manifest"), COMPACT_LIST_BIONICS, 0);
            tmpcomp2->add_option(_("Open Chambers"), COMPACT_RELEASE_BIONICS, 3);
            tmpcomp2->add_failure(COMPFAIL_MANHACKS);
            tmpcomp2->add_failure(COMPFAIL_SECUBOTS);
        }
        break;
    case room_dorm:
        if (rotate % 2 == 0) {
            for (int y = y1 + 1; y <= y2 - 1; y += 3) {
                m->furn_set(x1    , y, f_bed);
                m->place_items("bed", 60, x1, y, x1, y, false, 0);
                m->furn_set(x1 + 1, y, f_bed);
                m->place_items("bed", 60, x1 + 1, y, x1 + 1, y, false, 0);
                m->furn_set(x2    , y, f_bed);
                m->place_items("bed", 60, x2, y, x2, y, false, 0);
                m->furn_set(x2 - 1, y, f_bed);
                m->place_items("bed", 60, x2 - 1, y, x2 - 1, y, false, 0);
                m->furn_set(x1, y + 1, f_dresser);
                m->furn_set(x2, y + 1, f_dresser);
                m->place_items("dresser", 70, x1, y + 1, x1, y + 1, false, 0);
                m->place_items("dresser", 70, x2, y + 1, x2, y + 1, false, 0);
            }
        } else if (rotate % 2 == 1) {
            for (int x = x1 + 1; x <= x2 - 1; x += 3) {
                m->furn_set(x, y1    , f_bed);
                m->place_items("bed", 60, x, y1, x, y1, false, 0);
                m->furn_set(x, y1 + 1, f_bed);
                m->place_items("bed", 60, x, y1 + 1, x, y1 + 1, false, 0);
                m->furn_set(x, y2    , f_bed);
                m->place_items("bed", 60, x, y2, x, y2, false, 0);
                m->furn_set(x, y2 - 1, f_bed);
                m->place_items("bed", 60, x, y2 - 1, x, y2 - 1, false, 0);
                m->furn_set(x + 1, y1, f_dresser);
                m->furn_set(x + 1, y2, f_dresser);
                m->place_items("dresser", 70, x + 1, y1, x + 1, y1, false, 0);
                m->place_items("dresser", 70, x + 1, y2, x + 1, y2, false, 0);
            }
        }
        m->place_items("lab_dorm", 84, x1, y1, x2, y2, false, 0);
        break;
    case room_split:
        if (rotate % 2 == 0) {
            int w1 = int((x1 + x2) / 2) - 2, w2 = int((x1 + x2) / 2) + 2;
            for (int y = y1; y <= y2; y++) {
                m->ter_set(w1, y, t_wall_v);
                m->ter_set(w2, y, t_wall_v);
            }
            m->ter_set(w1, int((y1 + y2) / 2), t_door_metal_c);
            m->ter_set(w2, int((y1 + y2) / 2), t_door_metal_c);
            science_room(m, x1, y1, w1 - 1, y2, z, 1);
            science_room(m, w2 + 1, y1, x2, y2, z, 3);
        } else {
            int w1 = int((y1 + y2) / 2) - 2, w2 = int((y1 + y2) / 2) + 2;
            for (int x = x1; x <= x2; x++) {
                m->ter_set(x, w1, t_wall_h);
                m->ter_set(x, w2, t_wall_h);
            }
            m->ter_set(int((x1 + x2) / 2), w1, t_door_metal_c);
            m->ter_set(int((x1 + x2) / 2), w2, t_door_metal_c);
            science_room(m, x1, y1, x2, w1 - 1, z, 2);
            science_room(m, x1, w2 + 1, x2, y2, z, 0);
        }
        break;
    default:
        break;
    }
}

void set_science_room(map *m, int x1, int y1, bool faces_right, int turn)
{
    // TODO: More types!
    int type = rng(0, 4);
    int x2 = x1 + 7;
    int y2 = y1 + 4;
    switch (type) {
    case 0: // Empty!
        return;
    case 1: // Chemistry.
        // #######.
        // #.......
        // #.......
        // #.......
        // #######.
        for (int i = x1; i <= x2; i++) {
            for (int j = y1; j <= y2; j++) {
                if ((i == x1 || j == y1 || j == y2) && i != x1) {
                    m->set(i, j, t_floor, f_counter);
                }
            }
        }
        m->place_items("chem_lab", 85, x1 + 1, y1, x2 - 1, y1, false, 0);
        m->place_items("chem_lab", 85, x1 + 1, y2, x2 - 1, y2, false, 0);
        m->place_items("chem_lab", 85, x1, y1 + 1, x1, y2 - 1, false, 0);
        break;

    case 2: // Hydroponics.
        // #.......
        // #.~~~~~.
        // #.......
        // #.~~~~~.
        // #.......
        for (int i = x1; i <= x2; i++) {
            for (int j = y1; j <= y2; j++) {
                if (i == x1) {
                    m->set(i, j, t_floor, f_counter);
                } else if (i > x1 + 1 && i < x2 && (j == y1 + 1 || j == y2 - 1)) {
                    m->ter_set(i, j, t_water_sh);
                }
            }
        }
        m->place_items("chem_lab", 80, x1, y1, x1, y2, false, turn - 50);
        m->place_items("hydro", 92, x1 + 1, y1 + 1, x2 - 1, y1 + 1, false, turn);
        m->place_items("hydro", 92, x1 + 1, y2 - 1, x2 - 1, y2 - 1, false, turn);
        break;

    case 3: // Electronics.
        // #######.
        // #.......
        // #.......
        // #.......
        // #######.
        for (int i = x1; i <= x2; i++) {
            for (int j = y1; j <= y2; j++) {
                if ((i == x1 || j == y1 || j == y2) && i != x1) {
                    m->set(i, j, t_floor, f_counter);
                }
            }
        }
        m->place_items("electronics", 85, x1 + 1, y1, x2 - 1, y1, false, turn - 50);
        m->place_items("electronics", 85, x1 + 1, y2, x2 - 1, y2, false, turn - 50);
        m->place_items("electronics", 85, x1, y1 + 1, x1, y2 - 1, false, turn - 50);
        break;

    case 4: // Monster research.
        // .|.####.
        // -|......
        // .|......
        // -|......
        // .|.####.
        for (int i = x1; i <= x2; i++) {
            for (int j = y1; j <= y2; j++) {
                if (i == x1 + 1) {
                    m->ter_set(i, j, t_wall_glass_v);
                } else if (i == x1 && (j == y1 + 1 || j == y2 - 1)) {
                    m->ter_set(i, j, t_wall_glass_h);
                } else if ((j == y1 || j == y2) && i >= x1 + 3 && i <= x2 - 1) {
                    m->set(i, j, t_floor, f_counter);
                }
            }
        }
        // TODO: Place a monster in the sealed areas.
        m->place_items("monparts", 70, x1 + 3, y1, 2 - 1, y1, false, turn - 100);
        m->place_items("monparts", 70, x1 + 3, y2, 2 - 1, y2, false, turn - 100);
        break;
    }

    if (!faces_right) { // Flip it.
        ter_id rotated[SEEX * 2][SEEY * 2];
        std::vector<item> itrot[SEEX * 2][SEEY * 2];
        for (int i = x1; i <= x2; i++) {
            for (int j = y1; j <= y2; j++) {
                rotated[i][j] = m->ter(i, j);
                auto items = m->i_at( i, j );
                itrot[i][j].reserve( items.size() );
                std::copy( items.begin(), items.end(), std::back_inserter(itrot[i][j]) );
                m->i_clear( i, j );
            }
        }
        for (int i = x1; i <= x2; i++) {
            for (int j = y1; j <= y2; j++) {
                m->ter_set(i, j, rotated[x2 - (i - x1)][j]);
                m->spawn_items(i, j, itrot[x2 - (i - x1)][j]);
            }
        }
    }
}

map_extra random_map_extra(map_extras embellishments)
{
    int pick = 0;
    // Set pick to the total of all the chances for map extras
    for (int i = 0; i < num_map_extras; i++) {
        if (!ACTIVE_WORLD_OPTIONS["CLASSIC_ZOMBIES"] || mfb(i) & classic_extras) {
            pick += embellishments.chances[i];
        }
    }
    // Set pick to a number between 0 and the total
    pick = rng(0, pick - 1);
    int choice = -1;
    while (pick >= 0) {
        choice++;
        if(!ACTIVE_WORLD_OPTIONS["CLASSIC_ZOMBIES"] || mfb(choice) & classic_extras) {
            pick -= embellishments.chances[choice];
        }
    }
    return map_extra(choice);
}

room_type pick_mansion_room(int x1, int y1, int x2, int y2)
{
    int dx = abs(x1 - x2), dy = abs(y1 - y2), area = dx * dy;
    int shortest = (dx < dy ? dx : dy), longest = (dx > dy ? dx : dy);
    std::vector<room_type> valid;
    if (shortest >= 12) {
        valid.push_back(room_mansion_courtyard);
    }
    if (shortest >= 7 && area >= 64 && area <= 100) {
        valid.push_back(room_mansion_bedroom);
    }
    if (shortest >= 9) {
        valid.push_back(room_mansion_library);
    }
    if (shortest >= 6 && longest <= 10) {
        valid.push_back(room_mansion_kitchen);
    }
    if (longest >= 7 && shortest >= 5) {
        valid.push_back(room_mansion_dining);
    }
    if (shortest >= 6 && longest <= 10) {
        valid.push_back(room_mansion_game);
    }
    if (shortest >= 6 && longest <= 10) {
        valid.push_back(room_mansion_study);
    }
    if (shortest >= 10) {
        valid.push_back(room_mansion_pool);
    }
    if (longest <= 6 || shortest <= 4) {
        valid.push_back(room_mansion_bathroom);
    }
    if (longest >= 8 && shortest <= 6) {
        valid.push_back(room_mansion_gallery);
    }

    if (valid.empty()) {
        debugmsg("x: %d - %d, dx: %d\n\
       y: %d - %d, dy: %d", x1, x2, dx,
                 y1, y2, dy);
        return room_null;
    }

    return valid[ rng(0, valid.size() - 1) ];
}

void build_mansion_room(map *m, room_type type, int x1, int y1, int x2, int y2, mapgendata & dat)
{
    int dx = abs(x1 - x2), dy = abs(y1 - y2);
    int cx_low = (x1 + x2) / 2, cx_hi = (x1 + x2 + 1) / 2,
        cy_low = (y1 + y2) / 2, cy_hi = (y1 + y2 + 1) / 2;

    /*
     debugmsg("\
    x: %d - %d, dx: %d cx: %d/%d\n\
    x: %d - %d, dx: %d cx: %d/%d", x1, x2, dx, cx_low, cx_hi,
                                   y1, y2, dy, cy_low, cy_hi);
    */
    bool walled_south = (y2 >= SEEY * 2 - 2);

    switch (type) {

    case room_mansion_courtyard:
        dat.square_groundcover( x1, y1, x2, y2);
        if (one_in(4)) { // Tree grid
            for (int x = 1; x <= dx / 2; x += 4) {
                for (int y = 1; y <= dx / 2; y += 4) {
                    m->ter_set(x1 + x, y1 + y, t_tree);
                    m->ter_set(x2 - x, y2 - y + 1, t_tree);
                }
            }
        }
        if (one_in(3)) { // shrub-lined
            for (int i = x1; i <= x2; i++) {
                if (m->ter(i, y2 + 1) != t_door_c) {
                    m->ter_set(i, y2, t_shrub);
                }
            }
            if (walled_south && x1 <= SEEX && SEEX <= x2) {
                m->ter_set(SEEX - 1, y2, dat.groundcover());
                m->ter_set(SEEX,     y2, dat.groundcover());
            }
        }
        break;

    case room_mansion_entry:
        if (!one_in(3)) { // Columns
            for (int y = y1 + 2; y <= y2; y += 3) {
                m->ter_set(cx_low - 3, y, t_column);
                m->ter_set(cx_low + 4, y, t_column);
            }
        }
        if (one_in(6)) { // Suits of armor
            int start = y1 + rng(2, 4), end = y2 - rng(0, 4), step = rng(3, 6);
            if (!one_in(4)) { // 75% for Euro ornamental, but good weapons maybe
            for (int y = start; y <= end; y += step) {
                m->spawn_item(x1 + 1, y, "helmet_plate");
                m->spawn_item(x1 + 1, y, "armor_plate");
                if (one_in(2)) {
                    m->spawn_item(x1 + 1, y, "pike");
                } else if (one_in(3)) {
                    m->spawn_item(x1 + 1, y, "broadsword");
                    m->spawn_item(x1 + 1, y, "scabbard");
                } else if (one_in(6)) {
                    m->spawn_item(x1 + 1, y, "mace");
                } else if (one_in(6)) {
                    m->spawn_item(x1 + 1, y, "morningstar");
                }
                m->spawn_item(x2 - 1, y, "helmet_plate");
                m->spawn_item(x2 - 1, y, "armor_plate");
                if (one_in(2)) {
                    m->spawn_item(x2 - 1, y, "pike");
                } else if (one_in(3)) {
                    m->spawn_item(x2 - 1, y, "broadsword");
                    m->spawn_item(x2 - 1, y, "scabbard");
                } else if (one_in(6)) {
                    m->spawn_item(x2 - 1, y, "mace");
                } else if (one_in(6)) {
                    m->spawn_item(x2 - 1, y, "morningstar");
                }
            }
          } else if (one_in(3)) { // Then 8.25% each for useful plate
              for (int y = start; y <= end; y += step) {
                m->spawn_item(x1 + 1, y, "helmet_barbute");
                m->spawn_item(x1 + 1, y, "armor_lightplate");
                if (one_in(2)) {
                    m->spawn_item(x1 + 1, y, "mace");
                } else if (one_in(3)) {
                    m->spawn_item(x1 + 1, y, "morningstar");
                } else if (one_in(6)) {
                    m->spawn_item(x1 + 1, y, "battleaxe");
                } else if (one_in(6)) {
                    m->spawn_item(x1 + 1, y, "broadsword");
                    m->spawn_item(x1 + 1, y, "scabbard");
                }
                m->spawn_item(x2 - 1, y, "helmet_barbute");
                m->spawn_item(x2 - 1, y, "armor_lightplate");
                if (one_in(2)) {
                    m->spawn_item(x2 - 1, y, "mace");
                } else if (one_in(3)) {
                    m->spawn_item(x2 - 1, y, "morningstar");
                } else if (one_in(6)) {
                    m->spawn_item(x2 - 1, y, "battleaxe");
                } else if (one_in(6)) {
                    m->spawn_item(x2 - 1, y, "broadsword");
                    m->spawn_item(x2 - 1, y, "scabbard");
                }
            }
          } else if (one_in(2)) { // or chainmail
              for (int y = start; y <= end; y += step) {
              // No helmets with the chainmail, sorry.
                m->spawn_item(x1 + 1, y, "chainmail_suit");
                if (one_in(2)) {
                    m->spawn_item(x1 + 1, y, "mace");
                } else if (one_in(3)) {
                    m->spawn_item(x1 + 1, y, "pike");
                } else if (one_in(6)) {
                    m->spawn_item(x1 + 1, y, "battleaxe");
                } else if (one_in(6)) {
                    m->spawn_item(x1 + 1, y, "broadsword");
                    m->spawn_item(x1 + 1, y, "scabbard");
                }
                m->spawn_item(x2 - 1, y, "chainmail_suit");
                if (one_in(2)) {
                    m->spawn_item(x2 - 1, y, "mace");
                } else if (one_in(3)) {
                    m->spawn_item(x2 - 1, y, "pike");
                } else if (one_in(6)) {
                    m->spawn_item(x2 - 1, y, "battleaxe");
                } else if (one_in(6)) {
                    m->spawn_item(x2 - 1, y, "broadsword");
                    m->spawn_item(x2 - 1, y, "scabbard");
                }
            }
          } else { // or samurai gear
                for (int y = start; y <= end; y += step) {
                    m->spawn_item(x1 + 1, y, "helmet_kabuto");
                    m->spawn_item(x1 + 1, y, "armor_samurai");
                    if (one_in(2)) {
                        m->spawn_item(x1 + 1, y, "katana");
                    } else if (one_in(3)) {
                        m->spawn_item(x1 + 1, y, "katana");
                        m->spawn_item(x1 + 1, y, "wakizashi");
                    } else if (one_in(6)) {
                        m->spawn_item(x1 + 1, y, "katana");
                        m->spawn_item(x1 + 1, y, "wakizashi");
                        m->spawn_item(x1 + 1, y, "tanto");
                    } else if (one_in(6)) {
                        m->spawn_item(x1 + 1, y, "nodachi");
                    } else if (one_in(4)) {
                        m->spawn_item(x1 + 1, y, "bokken");
                    }

                    m->spawn_item(x2 - 1, y, "helmet_kabuto");
                    m->spawn_item(x2 - 1, y, "armor_samurai");
                    if (one_in(2)) {
                        m->spawn_item(x2 - 1, y, "katana");
                    } else if (one_in(3)) {
                        m->spawn_item(x2 - 1, y, "katana");
                        m->spawn_item(x1 + 1, y, "wakizashi");
                    } else if (one_in(6)) {
                        m->spawn_item(x2 - 1, y, "katana");
                        m->spawn_item(x1 + 1, y, "wakizashi");
                        m->spawn_item(x1 + 1, y, "tanto");
                    } else if (one_in(6)) {
                        m->spawn_item(x2 - 1, y, "nodachi");
                    } else if (one_in(4)) {
                        m->spawn_item(x2 - 1, y, "bokken");
                    }

                    if(one_in(2)) {
                        m->spawn_item(x2 - 1, y, "scabbard");
                    } if (one_in(2)) {
                        m->spawn_item(x1 + 1, y, "scabbard");
                    }
            }
          }
        }
        break;

    case room_mansion_bedroom:
        if (dx > dy || (dx == dy && one_in(2))) { // horizontal
            if (one_in(2)) { // bed on left
                square_furn(m, f_bed, x1 + 1, cy_low - 1, x1 + 3, cy_low + 1);
                m->place_items("bed", 60, x1 + 1, cy_low - 1, x1 + 3, cy_low + 1, false, 0);
            } else { // bed on right
                square_furn(m, f_bed, x2 - 3, cy_low - 1, x2 - 1, cy_low + 1);
                m->place_items("bed", 60, x2 - 3, cy_low - 1, x2 - 1, cy_low + 1, false, 0);
            }
            m->furn_set(cx_hi - 2, y1, f_bookcase);
            m->furn_set(cx_hi - 1, y1, f_counter);
            m->ter_set(cx_hi    , y1, t_console_broken);
            m->furn_set(cx_hi + 1, y1, f_counter);
            m->furn_set(cx_hi + 2, y1, f_bookcase);
            m->place_items("bedroom", 60, cx_hi - 2, y1, cx_hi + 2, y1, false, 0);

            m->furn_set(cx_hi - 2, y2, f_dresser);
            m->furn_set(cx_hi - 1, y2, f_dresser);
            m->place_items("dresser", 80, cx_hi - 2, y2, cx_hi - 1, y2, false, 0);
            if (one_in(10)) {
                m->place_items("mansion_guns", 58, cx_hi - 2, y2, cx_hi - 1, y2, false, 0);
            }

            m->furn_set(cx_hi + 1, y2, f_desk);
            m->place_items("office", 50, cx_hi + 1, y2, cx_hi + 1, y2, false, 0);

            m->furn_set(cx_hi + 2, y2, f_chair);

            m->furn_set(x1, y1, f_indoor_plant);
            m->furn_set(x1, y2, f_indoor_plant);

        } else { // vertical
            if (one_in(2)) { // bed at top
                square_furn(m, f_bed, cx_low - 1, y1 + 1, cx_low + 1, y1 + 3);
                m->place_items("bed", 60, cx_low - 1, y1 + 1, cx_low + 1, y1 + 3, false, 0);
            } else { // bed at bottom
                square_furn(m, f_bed, cx_low - 1, y2 - 3, cx_low + 1, y2 - 1);
                m->place_items("bed", 60, cx_low - 1, y2 - 3, cx_low + 1, y2 - 1, false, 0);
            }
            m->furn_set(x1, cy_hi - 2, f_bookcase);
            m->furn_set(x1, cy_hi - 1, f_counter);
            m->ter_set(x1, cy_hi, t_console_broken);
            m->furn_set(x1, cy_hi + 1, f_counter);
            m->furn_set(x1, cy_hi + 2, f_bookcase);
            m->place_items("bedroom", 80, x1, cy_hi - 2, x1, cy_hi + 2, false, 0);

            m->furn_set(x2, cy_hi - 2, f_dresser);
            m->furn_set(x2, cy_hi - 1, f_dresser);
            m->place_items("dresser", 80, x2, cy_hi - 2, x2, cy_hi - 1, false, 0);
            if (one_in(10)) {
                m->place_items("mansion_guns", 58, x2, cy_hi - 2, x2, cy_hi - 1, false, 0);
            }

            m->furn_set(x2, cy_hi + 1, f_desk);
            m->place_items("office", 50, x2, cy_hi + 1, x2, cy_hi + 1, false, 0);

            m->furn_set(x2, cy_hi + 2, f_chair);

            m->furn_set(x1, y2, f_indoor_plant);
            m->furn_set(x2, y2, f_indoor_plant);
        }
        break;

    case room_mansion_library:
        if (dx < dy || (dx == dy && one_in(2))) { // vertically-aligned bookshelves
            for (int x = x1 + 1; x <= cx_low - 2; x += 3) {
                for (int y = y1 + 1; y <= y2 - 3; y += 4) {
                    square_furn(m, f_bookcase, x, y, x + 1, y + 2);
                    m->place_items("novels",    85, x, y, x + 1, y + 2, false, 0);
                    m->place_items("manuals",   62, x, y, x + 1, y + 2, false, 0);
                    m->place_items("textbooks", 40, x, y, x + 1, y + 2, false, 0);
                    m->place_items("mansion_books", 35, x, y, x + 1, y + 2, false, 0);
                }
            }
            for (int x = x2 - 1; x >= cx_low + 2; x -= 3) {
                for (int y = y1 + 1; y <= y2 - 3; y += 4) {
                    square_furn(m, f_bookcase, x - 1, y, x, y + 2);
                    m->place_items("novels",    85, x - 1, y, x, y + 2, false, 0);
                    m->place_items("manuals",   62, x - 1, y, x, y + 2, false, 0);
                    m->place_items("textbooks", 40, x - 1, y, x, y + 2, false, 0);
                    m->place_items("mansion_books", 35, x - 1, y, x, y + 2, false, 0);
                }
            }
        } else { // horizontally-aligned bookshelves
            for (int y = y1 + 1; y <= cy_low - 2; y += 3) {
                for (int x = x1 + 1; x <= x2 - 3; x += 4) {
                    square_furn(m, f_bookcase, x, y, x + 2, y + 1);
                    m->place_items("novels",    85, x, y, x + 2, y + 1, false, 0);
                    m->place_items("manuals",   62, x, y, x + 2, y + 1, false, 0);
                    m->place_items("textbooks", 40, x, y, x + 2, y + 1, false, 0);
                    m->place_items("mansion_books", 35, x, y, x + 2, y + 1, false, 0);
                }
            }
            for (int y = y2 - 1; y >= cy_low + 2; y -= 3) {
                for (int x = x1 + 1; x <= x2 - 3; x += 4) {
                    square_furn(m, f_bookcase, x, y - 1, x + 2, y);
                    m->place_items("novels",    85, x, y - 1, x + 2, y, false, 0);
                    m->place_items("manuals",   62, x, y - 1, x + 2, y, false, 0);
                    m->place_items("textbooks", 40, x, y - 1, x + 2, y, false, 0);
                    m->place_items("mansion_books", 35, x, y - 1, x + 2, y, false, 0);
                }
            }
        }
        break;

    case room_mansion_kitchen:
        line_furn(m, f_counter, cx_hi - 2, y1 + 1, cx_hi - 2, y2 - 1);
        line_furn(m, f_counter, cx_hi,     y1 + 1, cx_hi,     y2 - 1);
        m->place_items("kitchen",  60, cx_hi - 2, y1 + 1, cx_hi, y2 - 1, false, 0);

        line_furn(m, f_fridge, cx_hi + 2, y1 + 1, cx_hi + 2, cy_hi - 1);
        m->place_items("fridge",  80, cx_hi + 2, y1 + 1, cx_hi + 2, cy_hi - 1, false, 0);

        m->furn_set(cx_hi + 2, cy_hi, f_oven);
        m->place_items("oven", 70,  cx_hi + 2, cy_hi, cx_hi + 2, cy_hi, false, 0);

        line_furn(m, f_rack, cx_hi + 2, cy_hi + 1, cx_hi + 2, y2 - 1);
        m->place_items("cannedfood",  70, cx_hi + 2, cy_hi + 1, cx_hi + 2, y2 - 1, false, 0);
        m->place_items("pasta",  70, cx_hi + 2, cy_hi + 1, cx_hi + 2, y2 - 1, false, 0);
        break;

    case room_mansion_dining:
        if (dx < dy || (dx == dy && one_in(2))) { // vertically-aligned table
            line_furn(m, f_table, cx_low, y1 + 2, cx_low, y2 - 2);
            line_furn(m, f_bench, cx_low - 1, y1 + 2, cx_low - 1, y2 - 2);
            line_furn(m, f_bench, cx_low + 1, y1 + 2, cx_low + 1, y2 - 2);
            m->place_items("dining", 78, cx_low, y1 + 2, cx_low, y2 - 2, false, 0);
        } else { // horizontally-aligned table
            line_furn(m, f_table, x1 + 2, cy_low, x2 - 2, cy_low);
            line_furn(m, f_bench, x1 + 2, cy_low - 1, x2 - 2, cy_low - 1);
            line_furn(m, f_bench, x1 + 2, cy_low + 1, x2 - 2, cy_low + 1);
            m->place_items("dining", 78, x1 + 2, cy_low, x2 - 2, cy_low, false, 0);
        }
        m->furn_set(x1, y1, f_indoor_plant);
        m->furn_set(x2, y1, f_indoor_plant);
        m->furn_set(x1, y2, f_indoor_plant);
        m->furn_set(x2, y2, f_indoor_plant);
        break;

    case room_mansion_game:
        if (dx < dy || one_in(2)) { // vertically-aligned table
            square_furn(m, f_pool_table, cx_low, cy_low - 1, cx_low + 1, cy_low + 1);
            m->place_items("pool_table", 80, cx_low, cy_low - 1, cx_low + 1, cy_low + 1,
                           false, 0);
        } else { // horizontally-aligned table
            square_furn(m, f_pool_table, cx_low - 1, cy_low, cx_low + 1, cy_low + 1);
            m->place_items("pool_table", 80, cx_low - 1, cy_low, cx_low + 1, cy_low + 1,
                           false, 0);
        }

        if (one_in(2)) {
            line_furn(m, f_sofa, x1 + 1, cy_low - 1, x1 + 1, cy_low + 1);
            m->furn_set(x1 + 1, cy_low - 2, f_table);
            m->place_items("coffee_shop", 70, x1 + 1, cy_low + 2, x1 + 1, cy_low + 2, false, 0);
            m->place_items("magazines", 50, x1 + 1, cy_low + 2, x1 + 1, cy_low + 2, false, 0);
            m->furn_set(x1 + 1, cy_low + 2, f_table);
            m->place_items("coffee_shop", 70, x1 + 1, cy_low - 2, x1 + 1, cy_low - 2, false, 0);
            m->place_items("magazines", 70, x1 + 1, cy_low - 2, x1 + 1, cy_low - 2, false, 0);
            m->place_items("mansion_books", 30, x1 + 1, cy_low - 2, x1 + 1, cy_low - 2, false, 0);
        } else {
            line_furn(m, f_sofa, cx_low - 1, y1 + 1, cx_low + 1, y1 + 1);
            m->furn_set(cx_low - 2, y1 + 1, f_table);
            m->place_items("coffee_shop", 70, cx_low - 2, y1 + 1, cx_low - 2, y1 + 1, false, 0);
            m->place_items("magazines", 50, cx_low - 2, y1 + 1, cx_low - 2, y1 + 1, false, 0);
            m->furn_set(cx_low + 2, y1 + 1, f_table);
            m->place_items("coffee_shop", 70, cx_low + 2, y1 + 1, cx_low + 2, y1 + 1, false, 0);
            m->place_items("magazines", 70, cx_low + 2, y1 + 1, cx_low + 2, y1 + 1, false, 0);
            m->place_items("mansion_books", 30, cx_low + 2, y1 + 1, cx_low + 2, y1 + 1, false, 0);
        }
        m->furn_set(x1, y1, f_indoor_plant);
        m->furn_set(x2, y1, f_indoor_plant);
        m->furn_set(x1, y2, f_indoor_plant);
        m->furn_set(x2, y2, f_indoor_plant);
        break;

    case room_mansion_pool:
        square(m, t_water_pool, x1 + 3, y1 + 3, x2 - 3, y2 - 3);

        m->furn_set(rng(x1 + 1, cx_hi - 2), y1 + 2, f_chair);
        m->furn_set(cx_hi, y1 + 2, f_table);
        m->furn_set(rng(x1 + 1, cx_hi + 2), y1 + 2, f_chair);
        m->place_items("magazines", 60, cx_hi, y1 + 2, cx_hi, y1 + 2, false, 0);

        m->furn_set(x1, y1, f_indoor_plant);
        m->furn_set(x2, y1, f_indoor_plant);
        m->furn_set(x1, y2, f_indoor_plant);
        m->furn_set(x2, y2, f_indoor_plant);
        break;

    case room_mansion_study:
        int study_y;
        if (one_in(2)) {
            study_y = y1;
        } else {
            study_y = y2;
        }
        for (int x = x1 + 1; x <= x2 - 1; x++) {
            if (x % 2 == 0) {
                m->furn_set(x, study_y, f_rack);
                if (one_in(3)) {
                    m->place_items("alcohol", 60, x, study_y, x, study_y, false, 0);
                } else if (one_in(3)) {
                    m->place_items("church", 60, x, study_y, x, study_y, false, 0);
                } else if (one_in(2)) {
                    m->place_items("mansion_guns", 60, x, study_y, x, study_y, false, 0);
                } else {
                    m->place_items("art", 60, x, study_y, x, study_y, false, 0);
                }
            }
        }

        square_furn(m, f_table, cx_low, cy_low - 1, cx_low + 1, cy_low + 1);
        m->place_items("novels", 50, cx_low, cy_low - 1, cx_low + 1, cy_low + 1,
                       false, 0);
        m->place_items("mansion_books", 40, cx_low, cy_low - 1, cx_low + 1, cy_low + 1,
                       false, 0);
        m->place_items("magazines", 60, cx_low, cy_low - 1, cx_low + 1, cy_low + 1,
                       false, 0);
        m->place_items("office", 60, cx_low, cy_low - 1, cx_low + 1, cy_low + 1,
                       false, 0);
        if (one_in(2)) {
            m->furn_set(cx_low - 1, rng(cy_low - 1, cy_low + 1), f_chair);
        } else {
            m->furn_set(cx_low + 2, rng(cy_low - 1, cy_low + 1), f_chair);
        }
        m->furn_set(x1, y1, f_indoor_plant);
        m->furn_set(x2, y1, f_indoor_plant);
        m->furn_set(x1, y2, f_indoor_plant);
        m->furn_set(x2, y2, f_indoor_plant);
        break;

    case room_mansion_bathroom:
        m->place_toilet(rng(x1 + 1, cx_hi - 1), rng(y1 + 1, cy_hi - 1));
        m->furn_set( rng(cx_hi + 1, x2 - 1), rng(y1 + 1, cy_hi - 1) , f_bathtub);
        m->furn_set( rng(x1 + 1, cx_hi - 1), rng(cy_hi + 1, y2 - 1) , f_sink);

        m->furn_set(x1, y2, f_indoor_plant);
        m->furn_set(x2, y2, f_indoor_plant);

        m->place_items("harddrugs", 20, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
        m->place_items("softdrugs", 72, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
        m->place_items("cleaning",  48, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
        break;

    case room_mansion_gallery:

        m->furn_set(x2 + 2, y2 + 2, f_rack);
        if (one_in(3)) {
            m->place_items("mansion_guns", 70, x2 + 2, y2 - 2, x2 + 2, y2 - 2, false, 0);
      } else {
        m->place_items("medieval", 40, x2 + 2, y2 + 2, x2 + 2, y2 + 2, false, 0);
      }
        m->furn_set(x2 - 2, y2 + 2, f_rack);
        m->place_items("art", 70, x2 - 2, y2 + 2, x2 - 2, y2 + 2, false, 0);
        m->furn_set(x2 + 2, y2 - 2, f_rack);
        if (one_in(3)) {
            m->place_items("mansion_guns", 70, x2 + 2, y2 - 2, x2 + 2, y2 - 2, false, 0);
      } else {
            m->place_items("art", 70, x2 + 2, y2 - 2, x2 + 2, y2 - 2, false, 0);
      }
        m->furn_set(x2 - 2, y2 - 2, f_rack);
        m->place_items("alcohol", 80, x2 - 2, y2 - 2, x2 - 2, y2 - 2, false, 0);

        square_furn(m, f_table, cx_low - 1, cy_low - 1, cx_low + 1, cy_low + 1);
        m->furn_set(x1, y1, f_indoor_plant);
        m->furn_set(x2, y1, f_indoor_plant);
        m->furn_set(x1, y2, f_indoor_plant);
        m->furn_set(x2, y2, f_indoor_plant);

        break;
    default:
        break;
    }
}

void mansion_room(map *m, int x1, int y1, int x2, int y2, mapgendata & dat)
{
    room_type type = pick_mansion_room(x1, y1, x2, y2);
    build_mansion_room(m, type, x1, y1, x2, y2, dat);
}

void map::add_extra(map_extra type)
{
    item body;
    body.make_corpse();

    switch (type) {

    case mx_null:
        debugmsg("Tried to generate null map extra.");
        break;

    case mx_helicopter: {
        int cx = rng(4, SEEX * 2 - 5), cy = rng(4, SEEY * 2 - 5);
        for (int x = 0; x < SEEX * 2; x++) {
            for (int y = 0; y < SEEY * 2; y++) {
                if (x >= cx - 4 && x <= cx + 4 && y >= cy - 4 && y <= cy + 4) {
                    if (!one_in(5)) {
                        make_rubble(x, y, f_wreckage, true);
                    } else if (is_bashable(x, y)) {
                        destroy(x, y, true);
                    }
                } else if (one_in(10)) { // 1 in 10 chance of being wreckage anyway
                    make_rubble(x, y, f_wreckage, true);
                }
            }
        }

        spawn_item(rng(5, 18), rng(5, 18), "black_box");
        place_items("helicopter", 90, cx - 4, cy - 4, cx + 4, cy + 4, true, 0);
        place_items("helicopter", 20, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
        items_location extra_items = "helicopter";
        switch (rng(1, 4)) {
        case 1:
            extra_items = "military";
            break;
        case 2:
            extra_items = "science";
            break;
        case 3:
            extra_items = "allguns";
            break;
        case 4:
            extra_items = "bionics";
            break;
        }
        place_spawns("GROUP_MAYBE_MIL", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, 0.1f);//0.1 = 1-5
        place_items(extra_items, 70, cx - 4, cy - 4, cx + 4, cy + 4, true, 0);
    }
    break;

    case mx_military: {
        int num_bodies = dice(2, 6);
        for (int i = 0; i < num_bodies; i++) {
            int x, y, tries = 0;;
            do { // Loop until we find a valid spot to dump a body, or we give up
                x = rng(0, SEEX * 2 - 1);
                y = rng(0, SEEY * 2 - 1);
                tries++;
            } while (tries < 10 && move_cost(x, y) == 0);

            if (tries < 10) { // We found a valid spot!
                if (one_in(10)) {
                    add_spawn("mon_zombie_soldier", 1, x, y);
                } else if (one_in(25)) {
                    add_spawn("mon_zombie_bio_op", 1, x, y);
                } else {
                    place_items("map_extra_military", 100, x, y, x, y, true, 0);
                }
            }

        }
        std::string netherspawns[4] = {"mon_gelatin", "mon_mi_go",
                                         "mon_kreck", "mon_gracke"};
        int num_monsters = rng(0, 3);
        for (int i = 0; i < num_monsters; i++) {
            std::string type = netherspawns[( rng(0, 3) )];
            int mx = rng(1, SEEX * 2 - 2), my = rng(1, SEEY * 2 - 2);
            add_spawn(type, 1, mx, my);
        }
        place_spawns("GROUP_MAYBE_MIL", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1,
                     0.1f);//0.1 = 1-5
        place_items("rare", 25, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
    }
    break;

    case mx_science: {
        int num_bodies = dice(2, 5);
        for (int i = 0; i < num_bodies; i++) {
            int x, y, tries = 0;
            do { // Loop until we find a valid spot to dump a body, or we give up
                x = rng(0, SEEX * 2 - 1);
                y = rng(0, SEEY * 2 - 1);
                tries++;
            } while (tries < 10 && move_cost(x, y) == 0);

            if (tries < 10) { // We found a valid spot!
                if (one_in(10)) {
                    add_spawn("mon_zombie_scientist", 1, x, y);
                } else {
                    place_items("map_extra_science", 100, x, y, x, y, true, 0);
                }
            }
        }
        std::string spawncreatures[4] = {"mon_gelatin", "mon_mi_go",
                                         "mon_kreck", "mon_gracke"};
        int num_monsters = rng(0, 3);
        for (int i = 0; i < num_monsters; i++) {
            std::string type = spawncreatures[( rng(0, 3) )];
            int mx = rng(1, SEEX * 2 - 2), my = rng(1, SEEY * 2 - 2);
            add_spawn(type, 1, mx, my);
        }
        place_items("rare", 45, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
    }
    break;

    case mx_roadblock: {
    // OK, if there's a way to get ajacent road tiles w/o bringing in
    // the overmap-scan I'm not seeing it.  So gonna make it Generic.
    // Barricades to E/W
    line_furn(this, f_barricade_road, SEEX * 2 - 1, 4, SEEX * 2 - 1, 10);
    line_furn(this, f_barricade_road, SEEX * 2 - 3, 13, SEEX * 2 - 3, 19);
    line_furn(this, f_barricade_road, 3, 4, 3, 10);
    line_furn(this, f_barricade_road, 1, 13, 1, 19);

    // Vehicles to N/S
    bool mil = false;
    if (one_in(3)) {
        mil = true;
    }
    if (mil) { //Military doesn't joke around with their barricades!
        line(this, t_fence_barbed, SEEX * 2 - 1, 4, SEEX * 2 - 1, 10);
        line(this, t_fence_barbed, SEEX * 2 - 3, 13, SEEX * 2 - 3, 19);
        line(this, t_fence_barbed, 3, 4, 3, 10);
        line(this, t_fence_barbed, 1, 13, 1, 19);
        if (one_in(3)) {  // Chicken delivery truck
            add_vehicle("military_cargo_truck", 12, SEEY * 2 - 5, 0);
            add_spawn("mon_chickenbot", 1, 12, 12);
        } else if (one_in(2)) {  // TAAANK
            // The truck's wrecked...with fuel.  Explosive barrel?
            add_vehicle("military_cargo_truck", 12, SEEY * 2 - 5, 0, 70, -1);
            add_spawn("mon_tankbot", 1, 12, 12);
        } else {  // Truck & turrets
            add_vehicle("military_cargo_truck", 12, SEEY * 2 - 5, 0);
            add_spawn("mon_turret_bmg", 1, 12, 12);
            add_spawn("mon_turret_rifle", 1, 9, 12);
        }

        int num_bodies = dice(2, 5);
        for (int i = 0; i < num_bodies; i++) {
            int x, y, tries = 0;;
            do { // Loop until we find a valid spot to dump a body, or we give up
                x = rng(0, SEEX * 2 - 1);
                y = rng(0, SEEY * 2 - 1);
                tries++;
            } while (tries < 10 && move_cost(x, y) == 0);

            if (tries < 10) { // We found a valid spot!
                if (one_in(8)) {
                    add_spawn("mon_zombie_soldier", 1, x, y);
                } else {
                    place_items("map_extra_military", 100, x, y, x, y, true, 0);
                } int splatter_range = rng(1, 3);
                    for (int j = 0; j <= splatter_range; j++) {
                        add_field( x - (j * 1), y + (j * 1), fd_blood, 1);
                    }
                }

            }
        } else { // Police roadblock
            add_vehicle("policecar", 8, 5, 20);
            add_vehicle("policecar", 16, SEEY * 2 - 5, 145);
            add_spawn("mon_turret", 1, 1, 12);
            add_spawn("mon_turret", 1, SEEX * 2 - 1, 12);

            int num_bodies = dice(1, 6);
        for (int i = 0; i < num_bodies; i++) {
            int x, y, tries = 0;;
            do { // Loop until we find a valid spot to dump a body, or we give up
                x = rng(0, SEEX * 2 - 1);
                y = rng(0, SEEY * 2 - 1);
                tries++;
            } while (tries < 10 && move_cost(x, y) == 0);

            if (tries < 10) { // We found a valid spot!
                if (one_in(8)) {
                    add_spawn("mon_zombie_cop", 1, x, y);
                } else {
                    place_items("map_extra_police", 100, x, y, x, y, true, 0);
                } int splatter_range = rng(1, 3);
                    for (int j = 0; j <= splatter_range; j++) {
                        add_field( x +(j * 1), y - (j * 1), fd_blood, 1);
                    }
                }
            }
        }
    }
    break;

    case mx_drugdeal: {
        // Decide on a drug type
        int num_drugs = 0;
        itype_id drugtype;
        switch (rng(1, 10)) {
        case 1: // Weed
            num_drugs = rng(20, 30);
            drugtype = "weed";
            break;
        case 2:
        case 3:
        case 4:
        case 5: // Cocaine
            num_drugs = rng(10, 20);
            drugtype = "coke";
            break;
        case 6:
        case 7:
        case 8: // Meth
            num_drugs = rng(8, 14);
            drugtype = "meth";
            break;
        case 9:
        case 10: // Heroin
            num_drugs = rng(6, 12);
            drugtype = "heroin";
            break;
        }
        int num_bodies_a = dice(3, 3);
        int num_bodies_b = dice(3, 3);
        bool north_south = one_in(2);
        bool a_has_drugs = one_in(2);

        for (int i = 0; i < num_bodies_a; i++) {
            int x, y, x_offset, y_offset, tries = 0;
            do { // Loop until we find a valid spot to dump a body, or we give up
                if (north_south) {
                    x = rng(0, SEEX * 2 - 1);
                    y = rng(0, SEEY - 4);
                    x_offset = 0;
                    y_offset = -1;
                } else {
                    x = rng(0, SEEX - 4);
                    y = rng(0, SEEY * 2 - 1);
                    x_offset = -1;
                    y_offset = 0;
                }
                tries++;
            } while (tries < 10 && move_cost(x, y) == 0);

            if (tries < 10) { // We found a valid spot!
                if (one_in(10)) {
                    add_spawn("mon_zombie_spitter", 1, x, y);
                } else {
                    place_items("map_extra_drugdeal", 100, x, y, x, y, true, 0);
                    int splatter_range = rng(1, 3);
                    for (int j = 0; j <= splatter_range; j++) {
                        add_field(x + (j * x_offset), y + (j * y_offset),
                                  fd_blood, 1);
                    }
                }
                if (a_has_drugs && num_drugs > 0) {
                    int drugs_placed = rng(2, 6);
                    if (drugs_placed > num_drugs) {
                        drugs_placed = num_drugs;
                        num_drugs = 0;
                    }
                    spawn_item(x, y, drugtype, 0, drugs_placed);
                }
            }
        }
        for (int i = 0; i < num_bodies_b; i++) {
            int x, y, x_offset, y_offset, tries = 0;
            do { // Loop until we find a valid spot to dump a body, or we give up
                if (north_south) {
                    x = rng(0, SEEX * 2 - 1);
                    y = rng(SEEY + 3, SEEY * 2 - 1);
                    x_offset = 0;
                    y_offset = 1;
                } else {
                    x = rng(SEEX + 3, SEEX * 2 - 1);
                    y = rng(0, SEEY * 2 - 1);
                    x_offset = 1;
                    y_offset = 0;
                }
                tries++;
            } while (tries < 10 && move_cost(x, y) == 0);

            if (tries < 10) { // We found a valid spot!
                if (one_in(20)) {
                    add_spawn("mon_zombie_smoker", 1, x, y);
                } else {
                    place_items("map_extra_drugdeal", 100, x, y, x, y, true, 0);
                    int splatter_range = rng(1, 3);
                    for (int j = 0; j <= splatter_range; j++) {
                        add_field( x + (j * x_offset), y + (j * y_offset),
                                   fd_blood, 1 );
                    }
                    if (!a_has_drugs && num_drugs > 0) {
                        int drugs_placed = rng(2, 6);
                        if (drugs_placed > num_drugs) {
                            drugs_placed = num_drugs;
                            num_drugs = 0;
                        }
                        spawn_item(x, y, drugtype, 0, drugs_placed);
                    }
                }
            }
        }
        std::string spawncreatures[4] = {"mon_gelatin", "mon_mi_go",
                                         "mon_kreck", "mon_gracke"};
        int num_monsters = rng(0, 3);
        for (int i = 0; i < num_monsters; i++) {
            std::string type = spawncreatures[( rng(0, 3) )];
            int mx = rng(1, SEEX * 2 - 2), my = rng(1, SEEY * 2 - 2);
            add_spawn(type, 1, mx, my);
        }
    }
    break;

    case mx_supplydrop: {
        int num_crates = rng(1, 5);
        for (int i = 0; i < num_crates; i++) {
            int x, y, tries = 0;
            do { // Loop until we find a valid spot to dump a body, or we give up
                x = rng(0, SEEX * 2 - 1);
                y = rng(0, SEEY * 2 - 1);
                tries++;
            } while (tries < 10 && move_cost(x, y) == 0);
            furn_set(x, y, f_crate_c);
            std::string item_group;
            switch (rng(1, 10)) {
            case 1:
            case 2:
            case 3:
            case 4:
                item_group = "mil_food";
                break;
            case 5:
            case 6:
            case 7:
                item_group = "grenades";
                break;
            case 8:
            case 9:
                item_group = "mil_armor";
                break;
            case 10:
                item_group = "mil_rifles";
                break;
            }
            int items_created = 0;
            for(int i = 0; i < 10 && items_created < 2; i++) {
                items_created += place_items(item_group, 80, x, y, x, y, true, 0);
            }
            if (i_at(x, y).empty()) {
                destroy(x, y, true);
            }
        }
    }
    break;

    case mx_portal: {
        std::string spawncreatures[5] = {"mon_gelatin", "mon_flaming_eye",
                                         "mon_kreck", "mon_gracke", "mon_blank"};
        int x = rng(1, SEEX * 2 - 2), y = rng(1, SEEY * 2 - 2);
        for (int i = x - 1; i <= x + 1; i++) {
            for (int j = y - 1; j <= y + 1; j++) {
                make_rubble(i, j, f_rubble_rock, true);
            }
        }
        add_trap(x, y, tr_portal);
        int num_monsters = rng(0, 4);
        for (int i = 0; i < num_monsters; i++) {
            std::string type = spawncreatures[( rng(0, 4) )];
            int mx = rng(1, SEEX * 2 - 2), my = rng(1, SEEY * 2 - 2);
            make_rubble(mx, my, f_rubble_rock, true);
            add_spawn(type, 1, mx, my);
        }
    }
    break;

    case mx_minefield: {
        int num_mines = rng(6, 20);
        for (int x = 0; x < SEEX * 2; x++) {
            for (int y = 0; y < SEEY * 2; y++) {
                if (one_in(3)) {
                    ter_set(x, y, t_dirt);
                }
            }
        }
        for (int i = 0; i < num_mines; i++) {
            // No mines at the extreme edges: safe to walk on a sign tile
            int x = rng(1, SEEX * 2 - 2), y = rng(1, SEEY * 2 - 2);
            if (!has_flag("DIGGABLE", x, y) || one_in(8)) {
                ter_set(x, y, t_dirtmound);
            }
            add_trap(x, y, tr_landmine_buried);
        }
        int x1 = 0;
        int y1 = 0;
        int x2 = (SEEX * 2 - 1);
        int y2 = (SEEY * 2 - 1);
        furn_set(x1, y1, "f_sign");
        set_signage(x1, y1, "DANGER! MINEFIELD!");
        furn_set(x1, y2, "f_sign");
        set_signage(x1, y2, "DANGER! MINEFIELD!");
        furn_set(x2, y1, "f_sign");
        set_signage(x2, y1, "DANGER! MINEFIELD!");
        furn_set(x2, y2, "f_sign");
        set_signage(x2, y2, "DANGER! MINEFIELD!");
    }
    break;

    case mx_crater: {
        int size = rng(2, 6);
        int size_squared = size * size;
        int x = rng(size, SEEX * 2 - 1 - size), y = rng(size, SEEY * 2 - 1 - size);
        for (int i = x - size; i <= x + size; i++) {
            for (int j = y - size; j <= y + size; j++) {
                //If we're using circular distances, make circular craters
                //Pythagoras to the rescue, x^2 + y^2 = hypotenuse^2
                if(!trigdist || (((i - x) * (i - x) + (j - y) * (j - y)) <= size_squared)) {
                    destroy(i, j, true);
                    adjust_radiation(i, j, rng(20, 40));
                }
            }
        }
    }
    break;

    case mx_fumarole: {
        int x1 = rng(0,    SEEX     - 1), y1 = rng(0,    SEEY     - 1),
            x2 = rng(SEEX, SEEX * 2 - 1), y2 = rng(SEEY, SEEY * 2 - 1);
        std::vector<point> fumarole = line_to(x1, y1, x2, y2, 0);
        for (auto &i : fumarole) {
            ter_set(i.x, i.y, t_lava);
        }
    }
    break;

    case mx_portal_in: {
        std::string monids[5] = {"mon_gelatin", "mon_flaming_eye", "mon_kreck", "mon_gracke", "mon_blank"};
        int x = rng(5, SEEX * 2 - 6), y = rng(5, SEEY * 2 - 6);
        add_field(x, y, fd_fatigue, 3);
        for (int i = x - 5; i <= x + 5; i++) {
            for (int j = y - 5; j <= y + 5; j++) {
                if (rng(1, 9) >= trig_dist(x, y, i, j)) {
                    marlossify(i, j);
                    if (one_in(15)) {
                        add_spawn( monids[rng( 0, 4 )], 1, i, j );
                    }
                }
            }
        }
    }
    break;

    case mx_anomaly: {
        point center( rng(6, SEEX * 2 - 7), rng(6, SEEY * 2 - 7) );
        artifact_natural_property prop =
            artifact_natural_property(rng(ARTPROP_NULL + 1, ARTPROP_MAX - 1));
        create_anomaly(center.x, center.y, prop);
        spawn_natural_artifact(center.x, center.y, prop);
    }
    break;

    default:
        break;
    }
}

void map::create_anomaly(int cx, int cy, artifact_natural_property prop)
{
    rough_circle(this, t_dirt, cx, cy, 11);
    rough_circle_furn(this, f_rubble, cx, cy, 5);
    furn_set(cx, cy, f_null);
    switch (prop) {
    case ARTPROP_WRIGGLING:
    case ARTPROP_MOVING:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (furn(i, j) == f_rubble) {
                    add_field(i, j, fd_push_items, 1);
                    if (one_in(3)) {
                        spawn_item(i, j, "rock");
                    }
                }
            }
        }
        break;

    case ARTPROP_GLOWING:
    case ARTPROP_GLITTERING:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (furn(i, j) == f_rubble && one_in(2)) {
                    add_trap(i, j, tr_glow);
                }
            }
        }
        break;

    case ARTPROP_HUMMING:
    case ARTPROP_RATTLING:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (furn(i, j) == f_rubble && one_in(2)) {
                    add_trap(i, j, tr_hum);
                }
            }
        }
        break;

    case ARTPROP_WHISPERING:
    case ARTPROP_ENGRAVED:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (furn(i, j) == f_rubble && one_in(3)) {
                    add_trap(i, j, tr_shadow);
                }
            }
        }
        break;

    case ARTPROP_BREATHING:
        for (int i = cx - 1; i <= cx + 1; i++) {
            for (int j = cy - 1; j <= cy + 1; j++)
                if (i == cx && j == cy) {
                    add_spawn("mon_breather_hub", 1, i, j);
                } else {
                    add_spawn("mon_breather", 1, i, j);
                }
        }
        break;

    case ARTPROP_DEAD:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (furn(i, j) == f_rubble) {
                    add_trap(i, j, tr_drain);
                }
            }
        }
        break;

    case ARTPROP_ITCHY:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (furn(i, j) == f_rubble) {
                    set_radiation(i, j, rng(0, 10));
                }
            }
        }
        break;

    case ARTPROP_ELECTRIC:
    case ARTPROP_CRACKLING:
        add_field(cx, cy, fd_shock_vent, 3);
        break;

    case ARTPROP_SLIMY:
        add_field(cx, cy, fd_acid_vent, 3);
        break;

    case ARTPROP_WARM:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (furn(i, j) == f_rubble) {
                    add_field(i, j, fd_fire_vent, 1 + (rl_dist(cx, cy, i, j) % 3));
                }
            }
        }
        break;

    case ARTPROP_SCALED:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (furn(i, j) == f_rubble) {
                    add_trap(i, j, tr_snake);
                }
            }
        }
        break;

    case ARTPROP_FRACTAL:
        create_anomaly(cx - 4, cy - 4,
                       artifact_natural_property(rng(ARTPROP_NULL + 1, ARTPROP_MAX - 1)));
        create_anomaly(cx + 4, cy - 4,
                       artifact_natural_property(rng(ARTPROP_NULL + 1, ARTPROP_MAX - 1)));
        create_anomaly(cx - 4, cy + 4,
                       artifact_natural_property(rng(ARTPROP_NULL + 1, ARTPROP_MAX - 1)));
        create_anomaly(cx + 4, cy - 4,
                       artifact_natural_property(rng(ARTPROP_NULL + 1, ARTPROP_MAX - 1)));
        break;
    default:
        break;
    }
}
///////////////////// part of map

void line(map *m, const ter_id type, int x1, int y1, int x2, int y2) {
    m->draw_line_ter(type, x1, y1, x2, y2);
}
void line_furn(map *m, furn_id type, int x1, int y1, int x2, int y2) {
    m->draw_line_furn(type, x1, y1, x2, y2);
}
void fill_background(map *m, ter_id type) {
    m->draw_fill_background(type);
}
void fill_background(map *m, ter_id (*f)()) {
    m->draw_fill_background(f);
}
void fill_background(map *m, const id_or_id & f) {
    m->draw_fill_background(f);
}
void square(map *m, ter_id type, int x1, int y1, int x2, int y2) {
    m->draw_square_ter(type, x1, y1, x2, y2);
}
void square_furn(map *m, furn_id type, int x1, int y1, int x2, int y2) {
    m->draw_square_furn(type, x1, y1, x2, y2);
}
void square(map *m, ter_id (*f)(), int x1, int y1, int x2, int y2) {
    m->draw_square_ter(f, x1, y1, x2, y2);
}
void square(map *m, const id_or_id & f, int x1, int y1, int x2, int y2) {
    m->draw_square_ter(f, x1, y1, x2, y2);
}
void rough_circle(map *m, ter_id type, int x, int y, int rad) {
    m->draw_rough_circle(type, x, y, rad);
}
void rough_circle_furn(map *m, furn_id type, int x, int y, int rad) {
    m->draw_rough_circle_furn(type, x, y, rad);
}
void add_corpse(map *m, int x, int y) {
    m->add_corpse(x, y);
}

/////////
