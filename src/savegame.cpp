#include "game.h" // IWYU pragma: associated

#include <clocale>
#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "achievement.h"
#include "avatar.h"
#include "basecamp.h"
#include "cata_io.h"
#include "coordinate_conversions.h"
#include "creature_tracker.h"
#include "debug.h"
#include "faction.h"
#include "hash_utils.h"
#include "json.h"
#include "kill_tracker.h"
#include "map.h"
#include "messages.h"
#include "mission.h"
#include "mongroup.h"
#include "monster.h"
#include "npc.h"
#include "omdata.h"
#include "options.h"
#include "overmap.h"
#include "overmap_types.h"
#include "regional_settings.h"
#include "scent_map.h"
#include "stats_tracker.h"

class overmap_connection;

#if defined(__ANDROID__)
#include "input.h"

extern std::map<std::string, std::list<input_event>> quick_shortcuts_map;
#endif

/*
 * Changes that break backwards compatibility should bump this number, so the game can
 * load a legacy format loader.
 */
const int savegame_version = 33;

/*
 * This is a global set by detected version header in .sav, maps.txt, or overmap.
 * This allows loaders for classes that exist in multiple files (such as item) to have
 * support for backwards compatibility as well.
 */
int savegame_loading_version = savegame_version;

/*
 * Save to opened character.sav
 */
void game::serialize( std::ostream &fout )
{
    /*
     * Format version 12: Fully json, save the header. Weather and memorial exist elsewhere.
     * To prevent (or encourage) confusion, there is no version 8. (cata 0.8 uses v7)
     */
    // Header
    fout << "# version " << savegame_version << std::endl;

    JsonOut json( fout, true ); // pretty-print

    json.start_object();
    // basic game state information.
    json.member( "turn", calendar::turn );
    json.member( "calendar_start", calendar::start_of_cataclysm );
    json.member( "game_start", calendar::start_of_game );
    json.member( "initial_season", static_cast<int>( calendar::initial_season ) );
    json.member( "auto_travel_mode", auto_travel_mode );
    json.member( "run_mode", static_cast<int>( safe_mode ) );
    json.member( "mostseen", mostseen );
    // current map coordinates
    tripoint pos_sm = m.get_abs_sub();
    const point pos_om = sm_to_om_remain( pos_sm.x, pos_sm.y );
    json.member( "levx", pos_sm.x );
    json.member( "levy", pos_sm.y );
    json.member( "levz", pos_sm.z );
    json.member( "om_x", pos_om.x );
    json.member( "om_y", pos_om.y );

    json.member( "grscent", scent.serialize() );
    json.member( "typescent", scent.serialize( true ) );

    // Then each monster
    json.member( "active_monsters", *critter_tracker );
    json.member( "stair_monsters", coming_to_stairs );

    // save stats.
    json.member( "kill_tracker", *kill_tracker_ptr );
    json.member( "stats_tracker", *stats_tracker_ptr );
    json.member( "achievements_tracker", *achievements_tracker_ptr );

    json.member( "player", u );
    Messages::serialize( json );

    json.end_object();
}

std::string scent_map::serialize( bool is_type ) const
{
    std::ostringstream rle_out;
    rle_out.imbue( std::locale::classic() );
    if( is_type ) {
        rle_out << typescent.str();
    } else {
        int rle_lastval = -1;
        int rle_count = 0;
        for( const auto &elem : grscent ) {
            for( const int &val : elem ) {
                if( val == rle_lastval ) {
                    rle_count++;
                } else {
                    if( rle_count ) {
                        rle_out << rle_count << " ";
                    }
                    rle_out << val << " ";
                    rle_lastval = val;
                    rle_count = 1;
                }
            }
        }
        rle_out << rle_count;
    }

    return rle_out.str();
}

static void chkversion( std::istream &fin )
{
    if( fin.peek() == '#' ) {
        std::string vline;
        getline( fin, vline );
        std::string tmphash;
        std::string tmpver;
        int savedver = -1;
        std::stringstream vliness( vline );
        vliness >> tmphash >> tmpver >> savedver;
        if( tmpver == "version" && savedver != -1 ) {
            savegame_loading_version = savedver;
        }
    }
}

/*
 * Parse an open .sav file.
 */
void game::unserialize( std::istream &fin )
{
    chkversion( fin );
    int tmpturn = 0;
    int tmpcalstart = 0;
    int tmprun = 0;
    tripoint_om_sm lev;
    point_abs_om com;
    JsonIn jsin( fin );
    try {
        JsonObject data = jsin.get_object();

        data.read( "turn", tmpturn );
        data.read( "calendar_start", tmpcalstart );
        calendar::initial_season = static_cast<season_type>( data.get_int( "initial_season",
                                   static_cast<int>( SPRING ) ) );

        data.read( "auto_travel_mode", auto_travel_mode );
        data.read( "run_mode", tmprun );
        data.read( "mostseen", mostseen );
        data.read( "levx", lev.x() );
        data.read( "levy", lev.y() );
        data.read( "levz", lev.z() );
        data.read( "om_x", com.x() );
        data.read( "om_y", com.y() );

        calendar::turn = time_point( tmpturn );
        calendar::start_of_cataclysm = time_point( tmpcalstart );

        if( !data.read( "game_start", calendar::start_of_game ) ) {
            calendar::start_of_game = calendar::start_of_cataclysm;
        }

        load_map( project_combine( com, lev ) );

        safe_mode = static_cast<safe_mode_type>( tmprun );
        if( get_option<bool>( "SAFEMODE" ) && safe_mode == SAFE_MODE_OFF ) {
            safe_mode = SAFE_MODE_ON;
        }

        std::string linebuff;
        std::string linebuf;
        if( data.read( "grscent", linebuf ) && data.read( "typescent", linebuff ) ) {
            scent.deserialize( linebuf );
            scent.deserialize( linebuff, true );
        } else {
            scent.reset();
        }
        data.read( "active_monsters", *critter_tracker );

        coming_to_stairs.clear();
        for( JsonValue elem : data.get_array( "stair_monsters" ) ) {
            monster stairtmp;
            elem.read( stairtmp );
            coming_to_stairs.push_back( stairtmp );
        }

        if( data.has_object( "kill_tracker" ) ) {
            data.read( "kill_tracker", *kill_tracker_ptr );
        } else {
            // Legacy support for when kills were stored directly in game
            std::map<mtype_id, int> kills;
            std::vector<std::string> npc_kills;
            for( const JsonMember member : data.get_object( "kills" ) ) {
                kills[mtype_id( member.name() )] = member.get_int();
            }

            for( const std::string npc_name : data.get_array( "npc_kills" ) ) {
                npc_kills.push_back( npc_name );
            }

            kill_tracker_ptr->reset( kills, npc_kills );
        }

        data.read( "player", u );
        data.read( "stats_tracker", *stats_tracker_ptr );
        data.read( "achievements_tracker", *achievements_tracker_ptr );
        Messages::deserialize( data );

    } catch( const JsonError &jsonerr ) {
        debugmsg( "Bad save json\n%s", jsonerr.c_str() );
        return;
    }
}

void scent_map::deserialize( const std::string &data, bool is_type )
{
    std::istringstream buffer( data );
    buffer.imbue( std::locale::classic() );
    if( is_type ) {
        std::string str;
        buffer >> str;
        typescent = scenttype_id( str );
    } else {
        int stmp = 0;
        int count = 0;
        for( auto &elem : grscent ) {
            for( auto &val : elem ) {
                if( count == 0 ) {
                    buffer >> stmp >> count;
                }
                count--;
                val = stmp;
            }
        }
    }
}

#if defined(__ANDROID__)
///// quick shortcuts
void game::load_shortcuts( std::istream &fin )
{
    JsonIn jsin( fin );
    try {
        JsonObject data = jsin.get_object();

        if( get_option<bool>( "ANDROID_SHORTCUT_PERSISTENCE" ) ) {
            quick_shortcuts_map.clear();
            for( const JsonMember &member : data.get_object( "quick_shortcuts" ) ) {
                std::list<input_event> &qslist = quick_shortcuts_map[member.name()];
                for( const int i : member.get_array() ) {
                    qslist.push_back( input_event( i, input_event_t::keyboard_char ) );
                }
            }
        }
    } catch( const JsonError &jsonerr ) {
        debugmsg( "Bad shortcuts json\n%s", jsonerr.c_str() );
        return;
    }
}

void game::save_shortcuts( std::ostream &fout )
{
    JsonOut json( fout, true ); // pretty-print

    json.start_object();
    if( get_option<bool>( "ANDROID_SHORTCUT_PERSISTENCE" ) ) {
        json.member( "quick_shortcuts" );
        json.start_object();
        for( auto &e : quick_shortcuts_map ) {
            json.member( e.first );
            const std::list<input_event> &qsl = e.second;
            json.start_array();
            for( const auto &event : qsl ) {
                json.write( event.get_first_input() );
            }
            json.end_array();
        }
        json.end_object();
    }
    json.end_object();
}
#endif

static std::unordered_set<std::string> obsolete_terrains;

void overmap::load_obsolete_terrains( const JsonObject &jo )
{
    for( const std::string line : jo.get_array( "terrains" ) ) {
        obsolete_terrains.emplace( line );
    }
}

bool overmap::obsolete_terrain( const std::string &ter )
{
    return obsolete_terrains.find( ter ) != obsolete_terrains.end();
}

/*
 * Complex conversion of outdated overmap terrain ids.
 * This is used when loading saved games with old oter_ids.
 */
void overmap::convert_terrain(
    const std::unordered_map<tripoint_om_omt, std::string> &needs_conversion )
{
    std::vector<point_om_omt> bridge_points;
    for( const auto &convert : needs_conversion ) {
        const tripoint_om_omt pos = convert.first;
        const std::string old = convert.second;

        struct convert_nearby {
            point offset;
            std::string x_id;
            std::string y_id;
            std::string new_id;
        };

        std::vector<convert_nearby> nearby;
        std::vector<std::pair<tripoint, std::string>> convert_unrelated_adjacent_tiles;

        if( old == "fema" || old == "fema_entrance" || old == "fema_1_3" ||
            old == "fema_2_1" || old == "fema_2_2" || old == "fema_2_3" ||
            old == "fema_3_1" || old == "fema_3_2" || old == "fema_3_3" ||
            old == "s_lot" || old == "mine_entrance" ) {
            ter_set( pos, oter_id( old + "_north" ) );
        } else if( old.compare( 0, 6, "bridge" ) == 0 ) {
            ter_set( pos, oter_id( old ) );
            const oter_id oter_ground = ter( tripoint_om_omt( pos.xy(), 0 ) );
            const oter_id oter_above = ter( pos + tripoint_above );
            if( is_ot_match( "bridge", oter_ground, ot_match_type::type ) &&
                !is_ot_match( "bridge_road", oter_above, ot_match_type::type ) ) {
                ter_set( pos + tripoint_above, oter_id( "bridge_road" + oter_get_rotation_string( oter_ground ) ) );
                bridge_points.emplace_back( pos.xy() );
            }
        } else if( old.compare( 0, 10, "mass_grave" ) == 0 ) {
            ter_set( pos, oter_id( "field" ) );
        } else if( old == "mine_shaft" ) {
            ter_set( pos, oter_id( "mine_shaft_middle_north" ) );
        } else if( old.compare( 0, 23, "office_tower_1_entrance" ) == 0 ) {
            ter_set( pos, oter_id( "office_tower_ne_north" ) );
            ter_set( pos + point_west, oter_id( "office_tower_nw_north" ) );
            ter_set( pos + point_south, oter_id( "office_tower_se_north" ) );
            ter_set( pos + point_south_west, oter_id( "office_tower_sw_north" ) );
        } else if( old.compare( 0, 23, "office_tower_b_entrance" ) == 0 ) {
            ter_set( pos, oter_id( "office_tower_underground_ne_north" ) );
            ter_set( pos + point_west, oter_id( "office_tower_underground_nw_north" ) );
            ter_set( pos + point_south, oter_id( "office_tower_underground_se_north" ) );
            ter_set( pos + point_south_west, oter_id( "office_tower_underground_sw_north" ) );
        }

        for( const auto &conv : nearby ) {
            const auto x_it = needs_conversion.find( pos + point( conv.offset.x, 0 ) );
            const auto y_it = needs_conversion.find( pos + point( 0, conv.offset.y ) );
            if( x_it != needs_conversion.end() && x_it->second == conv.x_id &&
                y_it != needs_conversion.end() && y_it->second == conv.y_id ) {
                ter_set( pos, oter_id( conv.new_id ) );
                break;
            }
        }

        for( const std::pair<tripoint, std::string> &conv : convert_unrelated_adjacent_tiles ) {
            ter_set( pos + conv.first, oter_id( conv.second ) );
        }
    }

    generate_bridgeheads( bridge_points );
}

void overmap::load_monster_groups( JsonIn &jsin )
{
    jsin.start_array();
    while( !jsin.end_array() ) {
        jsin.start_array();

        mongroup new_group;
        new_group.deserialize( jsin );

        jsin.start_array();
        tripoint_om_sm temp;
        while( !jsin.end_array() ) {
            temp.deserialize( jsin );
            new_group.pos = temp;
            add_mon_group( new_group );
        }

        jsin.end_array();
    }
}

void overmap::load_legacy_monstergroups( JsonIn &jsin )
{
    jsin.start_array();
    while( !jsin.end_array() ) {
        mongroup new_group;
        new_group.deserialize_legacy( jsin );
        add_mon_group( new_group );
    }
}

// throws std::exception
void overmap::unserialize( std::istream &fin )
{
    chkversion( fin );
    JsonIn jsin( fin );
    jsin.start_object();
    while( !jsin.end_object() ) {
        const std::string name = jsin.get_member_name();
        if( name == "layers" ) {
            std::unordered_map<tripoint_om_omt, std::string> needs_conversion;
            jsin.start_array();
            for( int z = 0; z < OVERMAP_LAYERS; ++z ) {
                jsin.start_array();
                int count = 0;
                std::string tmp_ter;
                oter_id tmp_otid( 0 );
                for( int j = 0; j < OMAPY; j++ ) {
                    for( int i = 0; i < OMAPX; i++ ) {
                        if( count == 0 ) {
                            jsin.start_array();
                            jsin.read( tmp_ter );
                            jsin.read( count );
                            jsin.end_array();
                            if( obsolete_terrain( tmp_ter ) ) {
                                for( int p = i; p < i + count; p++ ) {
                                    needs_conversion.emplace(
                                        tripoint_om_omt( p, j, z - OVERMAP_DEPTH ), tmp_ter );
                                }
                                tmp_otid = oter_id( 0 );
                            } else if( oter_str_id( tmp_ter ).is_valid() ) {
                                tmp_otid = oter_id( tmp_ter );
                            } else {
                                debugmsg( "Loaded bad ter!  ter %s", tmp_ter.c_str() );
                                tmp_otid = oter_id( 0 );
                            }
                        }
                        count--;
                        layer[z].terrain[i][j] = tmp_otid;
                    }
                }
                jsin.end_array();
            }
            jsin.end_array();
            convert_terrain( needs_conversion );
        } else if( name == "region_id" ) {
            std::string new_region_id;
            jsin.read( new_region_id );
            if( settings.id != new_region_id ) {
                t_regional_settings_map_citr rit = region_settings_map.find( new_region_id );
                if( rit != region_settings_map.end() ) {
                    // TODO: optimize
                    settings = rit->second;
                }
            }
        } else if( name == "mongroups" ) {
            load_legacy_monstergroups( jsin );
        } else if( name == "monster_groups" ) {
            load_monster_groups( jsin );
        } else if( name == "cities" ) {
            jsin.start_array();
            while( !jsin.end_array() ) {
                jsin.start_object();
                city new_city;
                while( !jsin.end_object() ) {
                    std::string city_member_name = jsin.get_member_name();
                    if( city_member_name == "name" ) {
                        jsin.read( new_city.name );
                    } else if( city_member_name == "x" ) {
                        jsin.read( new_city.pos.x() );
                    } else if( city_member_name == "y" ) {
                        jsin.read( new_city.pos.y() );
                    } else if( city_member_name == "size" ) {
                        jsin.read( new_city.size );
                    }
                }
                cities.push_back( new_city );
            }
        } else if( name == "connections_out" ) {
            jsin.read( connections_out );
        } else if( name == "roads_out" ) {
            // Legacy data, superceded by that stored in the "connections_out" member. A load and save
            // cycle will migrate this to "connections_out".
            std::vector<tripoint_om_omt> &roads_out =
                connections_out[string_id<overmap_connection>( "local_road" )];
            jsin.start_array();
            while( !jsin.end_array() ) {
                jsin.start_object();
                tripoint_om_omt new_road;
                while( !jsin.end_object() ) {
                    std::string road_member_name = jsin.get_member_name();
                    if( road_member_name == "x" ) {
                        jsin.read( new_road.x() );
                    } else if( road_member_name == "y" ) {
                        jsin.read( new_road.y() );
                    }
                }
                roads_out.push_back( new_road );
            }
        } else if( name == "radios" ) {
            jsin.start_array();
            while( !jsin.end_array() ) {
                jsin.start_object();
                radio_tower new_radio{ point_om_sm( point_min ) };
                while( !jsin.end_object() ) {
                    const std::string radio_member_name = jsin.get_member_name();
                    if( radio_member_name == "type" ) {
                        const std::string radio_name = jsin.get_string();
                        const auto mapping =
                            find_if( radio_type_names.begin(), radio_type_names.end(),
                        [radio_name]( const std::pair<radio_type, std::string> &p ) {
                            return p.second == radio_name;
                        } );
                        if( mapping != radio_type_names.end() ) {
                            new_radio.type = mapping->first;
                        }
                    } else if( radio_member_name == "x" ) {
                        jsin.read( new_radio.pos.x() );
                    } else if( radio_member_name == "y" ) {
                        jsin.read( new_radio.pos.y() );
                    } else if( radio_member_name == "strength" ) {
                        jsin.read( new_radio.strength );
                    } else if( radio_member_name == "message" ) {
                        jsin.read( new_radio.message );
                    }
                }
                radios.push_back( new_radio );
            }
        } else if( name == "monster_map" ) {
            jsin.start_array();
            while( !jsin.end_array() ) {
                tripoint_om_sm monster_location;
                monster new_monster;
                monster_location.deserialize( jsin );
                new_monster.deserialize( jsin );
                monster_map.insert( std::make_pair( monster_location,
                                                    std::move( new_monster ) ) );
            }
        } else if( name == "tracked_vehicles" ) {
            jsin.start_array();
            while( !jsin.end_array() ) {
                jsin.start_object();
                om_vehicle new_tracker;
                int id;
                while( !jsin.end_object() ) {
                    std::string tracker_member_name = jsin.get_member_name();
                    if( tracker_member_name == "id" ) {
                        jsin.read( id );
                    } else if( tracker_member_name == "x" ) {
                        jsin.read( new_tracker.p.x() );
                    } else if( tracker_member_name == "y" ) {
                        jsin.read( new_tracker.p.y() );
                    } else if( tracker_member_name == "name" ) {
                        jsin.read( new_tracker.name );
                    }
                }
                vehicles[id] = new_tracker;
            }
        } else if( name == "scent_traces" ) {
            jsin.start_array();
            while( !jsin.end_array() ) {
                jsin.start_object();
                tripoint_abs_omt pos;
                time_point time = calendar::before_time_starts;
                int strength = 0;
                while( !jsin.end_object() ) {
                    std::string scent_member_name = jsin.get_member_name();
                    if( scent_member_name == "pos" ) {
                        jsin.read( pos );
                    } else if( scent_member_name == "time" ) {
                        jsin.read( time );
                    } else if( scent_member_name == "strength" ) {
                        jsin.read( strength );
                    }
                }
                scents[pos] = scent_trace( time, strength );
            }
        } else if( name == "npcs" ) {
            jsin.start_array();
            while( !jsin.end_array() ) {
                shared_ptr_fast<npc> new_npc = make_shared_fast<npc>();
                new_npc->deserialize( jsin );
                if( !new_npc->get_fac_id().str().empty() ) {
                    new_npc->set_fac( new_npc->get_fac_id() );
                }
                npcs.push_back( new_npc );
            }
        } else if( name == "camps" ) {
            jsin.start_array();
            while( !jsin.end_array() ) {
                basecamp new_camp;
                new_camp.deserialize( jsin );
                camps.push_back( new_camp );
            }
        } else if( name == "overmap_special_placements" ) {
            jsin.start_array();
            while( !jsin.end_array() ) {
                jsin.start_object();
                overmap_special_id s;
                while( !jsin.end_object() ) {
                    std::string name = jsin.get_member_name();
                    if( name == "special" ) {
                        jsin.read( s );
                    } else if( name == "placements" ) {
                        jsin.start_array();
                        while( !jsin.end_array() ) {
                            jsin.start_object();
                            while( !jsin.end_object() ) {
                                std::string name = jsin.get_member_name();
                                if( name == "points" ) {
                                    jsin.start_array();
                                    while( !jsin.end_array() ) {
                                        jsin.start_object();
                                        tripoint_om_omt p;
                                        while( !jsin.end_object() ) {
                                            std::string name = jsin.get_member_name();
                                            if( name == "p" ) {
                                                jsin.read( p );
                                                overmap_special_placements[p] = s;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static void unserialize_array_from_compacted_sequence( JsonIn &jsin, bool ( &array )[OMAPX][OMAPY] )
{
    int count = 0;
    bool value = false;
    for( int j = 0; j < OMAPY; j++ ) {
        for( auto &array_col : array ) {
            if( count == 0 ) {
                jsin.start_array();
                jsin.read( value );
                jsin.read( count );
                jsin.end_array();
            }
            count--;
            array_col[j] = value;
        }
    }
}

// throws std::exception
void overmap::unserialize_view( std::istream &fin )
{
    chkversion( fin );
    JsonIn jsin( fin );
    jsin.start_object();
    while( !jsin.end_object() ) {
        const std::string name = jsin.get_member_name();
        if( name == "visible" ) {
            jsin.start_array();
            for( int z = 0; z < OVERMAP_LAYERS; ++z ) {
                jsin.start_array();
                unserialize_array_from_compacted_sequence( jsin, layer[z].visible );
                jsin.end_array();
            }
            jsin.end_array();
        } else if( name == "explored" ) {
            jsin.start_array();
            for( int z = 0; z < OVERMAP_LAYERS; ++z ) {
                jsin.start_array();
                unserialize_array_from_compacted_sequence( jsin, layer[z].explored );
                jsin.end_array();
            }
            jsin.end_array();
        } else if( name == "notes" ) {
            jsin.start_array();
            for( int z = 0; z < OVERMAP_LAYERS; ++z ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    om_note tmp;
                    jsin.start_array();
                    jsin.read( tmp.p.x() );
                    jsin.read( tmp.p.y() );
                    jsin.read( tmp.text );
                    jsin.read( tmp.dangerous );
                    jsin.read( tmp.danger_radius );
                    jsin.end_array();

                    layer[z].notes.push_back( tmp );
                }
            }
            jsin.end_array();
        } else if( name == "extras" ) {
            jsin.start_array();
            for( int z = 0; z < OVERMAP_LAYERS; ++z ) {
                jsin.start_array();
                while( !jsin.end_array() ) {
                    om_map_extra tmp;
                    jsin.start_array();
                    jsin.read( tmp.p.x() );
                    jsin.read( tmp.p.y() );
                    jsin.read( tmp.id );
                    jsin.end_array();

                    layer[z].extras.push_back( tmp );
                }
            }
            jsin.end_array();
        }
    }
}

static void serialize_array_to_compacted_sequence( JsonOut &json,
        const bool ( &array )[OMAPX][OMAPY] )
{
    int count = 0;
    int lastval = -1;
    for( int j = 0; j < OMAPY; j++ ) {
        for( const auto &array_col : array ) {
            const int value = array_col[j];
            if( value != lastval ) {
                if( count ) {
                    json.write( count );
                    json.end_array();
                }
                lastval = value;
                json.start_array();
                json.write( static_cast<bool>( value ) );
                count = 1;
            } else {
                count++;
            }
        }
    }
    json.write( count );
    json.end_array();
}

void overmap::serialize_view( std::ostream &fout ) const
{
    fout << "# version " << savegame_version << std::endl;

    JsonOut json( fout, false );
    json.start_object();

    json.member( "visible" );
    json.start_array();
    for( int z = 0; z < OVERMAP_LAYERS; ++z ) {
        json.start_array();
        serialize_array_to_compacted_sequence( json, layer[z].visible );
        json.end_array();
        fout << std::endl;
    }
    json.end_array();

    json.member( "explored" );
    json.start_array();
    for( int z = 0; z < OVERMAP_LAYERS; ++z ) {
        json.start_array();
        serialize_array_to_compacted_sequence( json, layer[z].explored );
        json.end_array();
        fout << std::endl;
    }
    json.end_array();

    json.member( "notes" );
    json.start_array();
    for( int z = 0; z < OVERMAP_LAYERS; ++z ) {
        json.start_array();
        for( const om_note &i : layer[z].notes ) {
            json.start_array();
            json.write( i.p.x() );
            json.write( i.p.y() );
            json.write( i.text );
            json.write( i.dangerous );
            json.write( i.danger_radius );
            json.end_array();
            fout << std::endl;
        }
        json.end_array();
    }
    json.end_array();

    json.member( "extras" );
    json.start_array();
    for( int z = 0; z < OVERMAP_LAYERS; ++z ) {
        json.start_array();
        for( const om_map_extra &i : layer[z].extras ) {
            json.start_array();
            json.write( i.p.x() );
            json.write( i.p.y() );
            json.write( i.id );
            json.end_array();
            fout << std::endl;
        }
        json.end_array();
    }
    json.end_array();

    json.end_object();
}

// Compares all fields except position and monsters
// If any group has monsters, it is never equal to any group (because monsters are unique)
struct mongroup_bin_eq {
    bool operator()( const mongroup &a, const mongroup &b ) const {
        return a.monsters.empty() &&
               b.monsters.empty() &&
               a.type == b.type &&
               a.radius == b.radius &&
               a.population == b.population &&
               a.target == b.target &&
               a.interest == b.interest &&
               a.dying == b.dying &&
               a.horde == b.horde &&
               a.horde_behaviour == b.horde_behaviour &&
               a.diffuse == b.diffuse;
    }
};

struct mongroup_hash {
    std::size_t operator()( const mongroup &mg ) const {
        // Note: not hashing monsters or position
        size_t ret = std::hash<mongroup_id>()( mg.type );
        cata::hash_combine( ret, mg.radius );
        cata::hash_combine( ret, mg.population );
        cata::hash_combine( ret, mg.target );
        cata::hash_combine( ret, mg.interest );
        cata::hash_combine( ret, mg.dying );
        cata::hash_combine( ret, mg.horde );
        cata::hash_combine( ret, mg.horde_behaviour );
        cata::hash_combine( ret, mg.diffuse );
        return ret;
    }
};

void overmap::save_monster_groups( JsonOut &jout ) const
{
    jout.member( "monster_groups" );
    jout.start_array();
    // Bin groups by their fields, except positions and monsters
    std::unordered_map<mongroup, std::list<tripoint_om_sm>, mongroup_hash, mongroup_bin_eq>
    binned_groups;
    binned_groups.reserve( zg.size() );
    for( const auto &pos_group : zg ) {
        // Each group in bin adds only position
        // so that 100 identical groups are 1 group data and 100 tripoints
        std::list<tripoint_om_sm> &positions = binned_groups[pos_group.second];
        positions.emplace_back( pos_group.first );
    }

    for( auto &group_bin : binned_groups ) {
        jout.start_array();
        // Zero the bin position so that it isn't serialized
        // The position is stored separately, in the list
        // TODO: Do it without the copy
        mongroup saved_group = group_bin.first;
        saved_group.pos = tripoint_om_sm();
        jout.write( saved_group );
        jout.write( group_bin.second );
        jout.end_array();
    }
    jout.end_array();
}

void overmap::serialize( std::ostream &fout ) const
{
    fout << "# version " << savegame_version << std::endl;

    JsonOut json( fout, false );
    json.start_object();

    json.member( "layers" );
    json.start_array();
    for( int z = 0; z < OVERMAP_LAYERS; ++z ) {
        const auto &layer_terrain = layer[z].terrain;
        int count = 0;
        oter_id last_tertype( -1 );
        json.start_array();
        for( int j = 0; j < OMAPY; j++ ) {
            // NOLINTNEXTLINE(modernize-loop-convert)
            for( int i = 0; i < OMAPX; i++ ) {
                oter_id t = layer_terrain[i][j];
                if( t != last_tertype ) {
                    if( count ) {
                        json.write( count );
                        json.end_array();
                    }
                    last_tertype = t;
                    json.start_array();
                    json.write( t.id() );
                    count = 1;
                } else {
                    count++;
                }
            }
        }
        json.write( count );
        // End the last entry for a z-level.
        json.end_array();
        // End the z-level
        json.end_array();
        // Insert a newline occasionally so the file isn't totally unreadable.
        fout << std::endl;
    }
    json.end_array();

    // temporary, to allow user to manually switch regions during play until regionmap is done.
    json.member( "region_id", settings.id );
    fout << std::endl;

    save_monster_groups( json );
    fout << std::endl;

    json.member( "cities" );
    json.start_array();
    for( const city &i : cities ) {
        json.start_object();
        json.member( "name", i.name );
        json.member( "x", i.pos.x() );
        json.member( "y", i.pos.y() );
        json.member( "size", i.size );
        json.end_object();
    }
    json.end_array();
    fout << std::endl;

    json.member( "connections_out", connections_out );
    fout << std::endl;

    json.member( "radios" );
    json.start_array();
    for( const radio_tower &i : radios ) {
        json.start_object();
        json.member( "x", i.pos.x() );
        json.member( "y", i.pos.y() );
        json.member( "strength", i.strength );
        json.member( "type", radio_type_names[i.type] );
        json.member( "message", i.message );
        json.end_object();
    }
    json.end_array();
    fout << std::endl;

    json.member( "monster_map" );
    json.start_array();
    for( const auto &i : monster_map ) {
        i.first.serialize( json );
        i.second.serialize( json );
    }
    json.end_array();
    fout << std::endl;

    json.member( "tracked_vehicles" );
    json.start_array();
    for( const auto &i : vehicles ) {
        json.start_object();
        json.member( "id", i.first );
        json.member( "name", i.second.name );
        json.member( "x", i.second.p.x() );
        json.member( "y", i.second.p.y() );
        json.end_object();
    }
    json.end_array();
    fout << std::endl;

    json.member( "scent_traces" );
    json.start_array();
    for( const auto &scent : scents ) {
        json.start_object();
        json.member( "pos", scent.first );
        json.member( "time", scent.second.creation_time );
        json.member( "strength", scent.second.initial_strength );
        json.end_object();
    }
    json.end_array();
    fout << std::endl;

    json.member( "npcs" );
    json.start_array();
    for( const auto &i : npcs ) {
        json.write( *i );
    }
    json.end_array();
    fout << std::endl;

    json.member( "camps" );
    json.start_array();
    for( const auto &i : camps ) {
        json.write( i );
    }
    json.end_array();
    fout << std::endl;

    // Condense the overmap special placements so that all placements of a given special
    // are grouped under a single key for that special.
    std::map<overmap_special_id, std::vector<tripoint_om_omt>> condensed_overmap_special_placements;
    for( const auto &placement : overmap_special_placements ) {
        condensed_overmap_special_placements[placement.second].emplace_back( placement.first );
    }

    json.member( "overmap_special_placements" );
    json.start_array();
    for( const auto &placement : condensed_overmap_special_placements ) {
        json.start_object();
        json.member( "special", placement.first );
        json.member( "placements" );
        json.start_array();
        // When we have a discriminator for different instances of a given special,
        // we'd use that that group them, but since that doesn't exist yet we'll
        // dump all the points of a given special into a single entry.
        json.start_object();
        json.member( "points" );
        json.start_array();
        for( const tripoint_om_omt &pos : placement.second ) {
            json.start_object();
            json.member( "p", pos );
            json.end_object();
        }
        json.end_array();
        json.end_object();
        json.end_array();
        json.end_object();
    }
    json.end_array();
    fout << std::endl;

    json.end_object();
    fout << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////
///// mongroup
template<typename Archive>
void mongroup::io( Archive &archive )
{
    archive.io( "type", type );
    archive.io( "pos", pos, tripoint_om_sm() );
    archive.io( "radius", radius, 1u );
    archive.io( "population", population, 1u );
    archive.io( "diffuse", diffuse, false );
    archive.io( "dying", dying, false );
    archive.io( "horde", horde, false );
    archive.io( "target", target, tripoint_om_sm() );
    archive.io( "interest", interest, 0 );
    archive.io( "horde_behaviour", horde_behaviour, io::empty_default_tag() );
    archive.io( "monsters", monsters, io::empty_default_tag() );
}

void mongroup::deserialize( JsonIn &data )
{
    JsonObject jo = data.get_object();
    jo.allow_omitted_members();
    io::JsonObjectInputArchive archive( jo );
    io( archive );
}

void mongroup::serialize( JsonOut &json ) const
{
    io::JsonObjectOutputArchive archive( json );
    const_cast<mongroup *>( this )->io( archive );
}

void mongroup::deserialize_legacy( JsonIn &json )
{
    json.start_object();
    while( !json.end_object() ) {
        std::string name = json.get_member_name();
        if( name == "type" ) {
            type = mongroup_id( json.get_string() );
        } else if( name == "pos" ) {
            pos.deserialize( json );
        } else if( name == "radius" ) {
            radius = json.get_int();
        } else if( name == "population" ) {
            population = json.get_int();
        } else if( name == "diffuse" ) {
            diffuse = json.get_bool();
        } else if( name == "dying" ) {
            dying = json.get_bool();
        } else if( name == "horde" ) {
            horde = json.get_bool();
        } else if( name == "target" ) {
            target.deserialize( json );
        } else if( name == "interest" ) {
            interest = json.get_int();
        } else if( name == "horde_behaviour" ) {
            horde_behaviour = json.get_string();
        } else if( name == "monsters" ) {
            json.start_array();
            while( !json.end_array() ) {
                monster new_monster;
                new_monster.deserialize( json );
                monsters.push_back( new_monster );
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////
///// mapbuffer

///////////////////////////////////////////////////////////////////////////////////////
///// SAVE_MASTER (i.e. master.gsav)

void mission::unserialize_all( JsonIn &jsin )
{
    jsin.start_array();
    while( !jsin.end_array() ) {
        mission mis;
        mis.deserialize( jsin );
        add_existing( mis );
    }
}

void game::unserialize_master( std::istream &fin )
{
    savegame_loading_version = 0;
    chkversion( fin );
    try {
        // single-pass parsing example
        JsonIn jsin( fin );
        jsin.start_object();
        while( !jsin.end_object() ) {
            std::string name = jsin.get_member_name();
            if( name == "next_mission_id" ) {
                next_mission_id = jsin.get_int();
            } else if( name == "next_npc_id" ) {
                next_npc_id.deserialize( jsin );
            } else if( name == "active_missions" ) {
                mission::unserialize_all( jsin );
            } else if( name == "factions" ) {
                jsin.read( *faction_manager_ptr );
            } else if( name == "seed" ) {
                jsin.read( seed );
            } else if( name == "weather" ) {
                weather_manager::unserialize_all( jsin );
            } else {
                // silently ignore anything else
                jsin.skip_value();
            }
        }
    } catch( const JsonError &e ) {
        debugmsg( "error loading %s: %s", SAVE_MASTER, e.c_str() );
    }
}

void mission::serialize_all( JsonOut &json )
{
    json.start_array();
    for( auto &e : get_all_active() ) {
        e->serialize( json );
    }
    json.end_array();
}

void weather_manager::unserialize_all( JsonIn &jsin )
{
    JsonObject w = jsin.get_object();
    w.read( "lightning", get_weather().lightning_active );
    w.read( "weather_id", get_weather().weather_id );
    w.read( "next_weather", get_weather().nextweather );
    w.read( "temperature", get_weather().temperature );
    w.read( "winddirection", get_weather().winddirection );
    w.read( "windspeed", get_weather().windspeed );
}

void game::serialize_master( std::ostream &fout )
{
    fout << "# version " << savegame_version << std::endl;
    try {
        JsonOut json( fout, true ); // pretty-print
        json.start_object();

        json.member( "next_mission_id", next_mission_id );
        json.member( "next_npc_id", next_npc_id );

        json.member( "active_missions" );
        mission::serialize_all( json );

        json.member( "factions", *faction_manager_ptr );
        json.member( "seed", seed );

        json.member( "weather" );
        json.start_object();
        json.member( "lightning", weather.lightning_active );
        json.member( "weather_id", weather.weather_id );
        json.member( "next_weather", weather.nextweather );
        json.member( "temperature", weather.temperature );
        json.member( "winddirection", weather.winddirection );
        json.member( "windspeed", weather.windspeed );
        json.end_object();

        json.end_object();
    } catch( const JsonError &e ) {
        debugmsg( "error saving to %s: %s", SAVE_MASTER, e.c_str() );
    }
}

void faction_manager::serialize( JsonOut &jsout ) const
{
    std::vector<faction> local_facs;
    for( const auto &elem : factions ) {
        local_facs.push_back( elem.second );
    }
    jsout.write( local_facs );
}

void faction_manager::deserialize( JsonIn &jsin )
{
    if( jsin.test_object() ) {
        // whoops - this recovers factions saved under the wrong format.
        jsin.start_object();
        while( !jsin.end_object() ) {
            faction add_fac;
            add_fac.id = faction_id( jsin.get_member_name() );
            jsin.read( add_fac );
            faction *old_fac = get( add_fac.id, false );
            if( old_fac ) {
                *old_fac = add_fac;
                // force a revalidation of add_fac
                get( add_fac.id, false );
            } else {
                factions[add_fac.id] = add_fac;
            }
        }
    } else if( jsin.test_array() ) {
        // how it should have been serialized.
        jsin.start_array();
        while( !jsin.end_array() ) {
            faction add_fac;
            jsin.read( add_fac );
            faction *old_fac = get( add_fac.id, false );
            if( old_fac ) {
                *old_fac = add_fac;
                // force a revalidation of add_fac
                get( add_fac.id, false );
            } else {
                factions[add_fac.id] = add_fac;
            }
        }
    }
}

void Creature_tracker::deserialize( JsonIn &jsin )
{
    monsters_list.clear();
    monsters_by_location.clear();
    jsin.start_array();
    while( !jsin.end_array() ) {
        // TODO: would be nice if monster had a constructor using JsonIn or similar, so this could be one statement.
        shared_ptr_fast<monster> mptr = make_shared_fast<monster>();
        jsin.read( *mptr );
        add( mptr );
    }
}

void Creature_tracker::serialize( JsonOut &jsout ) const
{
    jsout.start_array();
    for( const auto &monster_ptr : monsters_list ) {
        jsout.write( *monster_ptr );
    }
    jsout.end_array();
}
