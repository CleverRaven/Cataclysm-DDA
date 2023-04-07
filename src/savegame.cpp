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
#include "city.h"
#include "coordinate_conversions.h"
#include "creature_tracker.h"
#include "debug.h"
#include "faction.h"
#include "hash_utils.h"
#include "json.h"
#include "json_loader.h"
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
#include "overmapbuffer.h"
#include "overmap_types.h"
#include "path_info.h"
#include "regional_settings.h"
#include "scent_map.h"
#include "stats_tracker.h"
#include "timed_event.h"

class overmap_connection;

static const oter_str_id oter_forest( "forest" );
static const oter_str_id oter_forest_thick( "forest_thick" );
static const oter_str_id oter_lake_bed( "lake_bed" );
static const oter_str_id oter_lake_shore( "lake_shore" );
static const oter_str_id oter_lake_surface( "lake_surface" );
static const oter_str_id oter_lake_water_cube( "lake_water_cube" );
static const oter_str_id oter_omt_obsolete( "omt_obsolete" );

static const string_id<overmap_connection> overmap_connection_local_road( "local_road" );

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
    tripoint_abs_sm pos_abs_sm = m.get_abs_sub();
    point_abs_om pos_om;
    tripoint_om_sm pos_sm;
    std::tie( pos_om, pos_sm ) = project_remain<coords::om>( pos_abs_sm );
    json.member( "levx", pos_sm.x() );
    json.member( "levy", pos_sm.y() );
    json.member( "levz", pos_sm.z() );
    json.member( "om_x", pos_om.x() );
    json.member( "om_y", pos_om.y() );
    // view offset
    json.member( "view_offset_x", u.view_offset.x );
    json.member( "view_offset_y", u.view_offset.y );
    json.member( "view_offset_z", u.view_offset.z );

    json.member( "grscent", scent.serialize() );
    json.member( "typescent", scent.serialize( true ) );

    // Then each monster
    json.member( "active_monsters", *critter_tracker );

    json.member( "driving_view_offset", driving_view_offset );
    json.member( "turnssincelastmon", turnssincelastmon );
    json.member( "bVMonsterLookFire", bVMonsterLookFire );

    // save stats.
    json.member( "kill_tracker", *kill_tracker_ptr );
    json.member( "stats_tracker", *stats_tracker_ptr );
    json.member( "achievements_tracker", *achievements_tracker_ptr );

    json.member( "player", u );
    json.member( "inactive_global_effect_on_condition_vector",
                 inactive_global_effect_on_condition_vector );

    //save queued effect_on_conditions
    std::priority_queue<queued_eoc, std::vector<queued_eoc>, eoc_compare> temp_queued(
        queued_global_effect_on_conditions );
    json.member( "queued_global_effect_on_conditions" );
    json.start_array();
    while( !temp_queued.empty() ) {
        json.start_object();
        json.member( "time", temp_queued.top().time );
        json.member( "eoc", temp_queued.top().eoc );
        json.end_object();
        temp_queued.pop();
    }
    json.end_array();
    global_variables_instance.serialize( json );
    Messages::serialize( json );
    json.member( "unique_npcs", unique_npcs );
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

static size_t chkversion( std::istream &fin )
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
    return fin.tellg();
}

/*
 * Parse an open .sav file.
 */
void game::unserialize( std::istream &fin, const cata_path &path )
{
    size_t json_file_offset = chkversion( fin );
    int tmpturn = 0;
    int tmpcalstart = 0;
    int tmprun = 0;
    tripoint_om_sm lev;
    point_abs_om com;
    JsonValue jsin = json_loader::from_path_at_offset( path, json_file_offset );
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

        data.read( "view_offset_x", u.view_offset.x );
        data.read( "view_offset_y", u.view_offset.y );
        data.read( "view_offset_z", u.view_offset.z );

        calendar::turn = time_point( tmpturn );
        calendar::start_of_cataclysm = time_point( tmpcalstart );

        if( !data.read( "game_start", calendar::start_of_game ) ) {
            calendar::start_of_game = calendar::start_of_cataclysm;
        }

        load_map( project_combine( com, lev ), /*pump_events=*/true );

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

        data.has_null( "stair_monsters" ); // TEMPORARY until 0.G
        data.has_null( "monstairz" ); // TEMPORARY until 0.G

        data.read( "driving_view_offset", driving_view_offset );
        data.read( "turnssincelastmon", turnssincelastmon );
        data.read( "bVMonsterLookFire", bVMonsterLookFire );

        data.read( "kill_tracker", *kill_tracker_ptr );

        data.read( "player", u );
        data.read( "inactive_global_effect_on_condition_vector",
                   inactive_global_effect_on_condition_vector );
        //load queued_eocs
        for( JsonObject elem : data.get_array( "queued_global_effect_on_conditions" ) ) {
            queued_eoc temp;
            temp.time = time_point( elem.get_int( "time" ) );
            temp.eoc = effect_on_condition_id( elem.get_string( "eoc" ) );
            queued_global_effect_on_conditions.push( temp );
        }
        global_variables_instance.unserialize( data );
        data.read( "unique_npcs", unique_npcs );
        inp_mngr.pump_events();
        data.read( "stats_tracker", *stats_tracker_ptr );
        data.read( "achievements_tracker", *achievements_tracker_ptr );
        inp_mngr.pump_events();
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
            for( int &val : elem ) {
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
void game::load_shortcuts( const cata_path &path )
{
    try {
        JsonValue jsin = json_loader::from_path( path );
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

void overmap::load_monster_groups( const JsonArray &jsin )
{
    for( JsonArray mongroup_with_tripoints : jsin ) {
        mongroup new_group;
        new_group.deserialize( mongroup_with_tripoints.next_object() );

        JsonArray tripoints_json = mongroup_with_tripoints.next_array();
        tripoint_om_sm temp;
        for( JsonValue tripoint_json : tripoints_json ) {
            temp.deserialize( tripoint_json );
            new_group.abs_pos = project_combine( pos(), temp );
            add_mon_group( new_group );
        }

        if( mongroup_with_tripoints.has_more() ) {
            mongroup_with_tripoints.throw_error( 2, "Unexpected value for mongroups json" );
        }
    }
}

void overmap::load_legacy_monstergroups( const JsonArray &jsin )
{
    for( JsonObject mongroup_json : jsin ) {
        mongroup new_group;
        new_group.deserialize_legacy( mongroup_json );
        add_mon_group( new_group );
    }
}

// throws std::exception
void overmap::unserialize( const cata_path &file_name, std::istream &fin )
{
    size_t json_offset = chkversion( fin );
    JsonValue jsin = json_loader::from_path_at_offset( file_name, json_offset );
    unserialize( jsin.get_object() );
}

void overmap::unserialize( const JsonObject &jsobj )
{
    // These must be read in this order.
    if( jsobj.has_member( "mapgen_arg_storage" ) ) {
        jsobj.read( "mapgen_arg_storage", mapgen_arg_storage, true );
    }
    if( jsobj.has_member( "mapgen_arg_index" ) ) {
        std::vector<std::pair<tripoint_om_omt, int>> flat_index;
        jsobj.read( "mapgen_arg_index", flat_index, true );
        for( const std::pair<tripoint_om_omt, int> &p : flat_index ) {
            auto it = mapgen_arg_storage.get_iterator_from_index( p.second );
            mapgen_args_index.emplace( p.first, &*it );
        }
    }
    for( JsonMember om_member : jsobj ) {
        const std::string name = om_member.name();
        if( name == "layers" ) {
            std::unordered_map<tripoint_om_omt, std::string> oter_id_migrations;
            JsonArray layers_json = om_member;

            for( int z = 0; z < OVERMAP_LAYERS; ++z ) {
                JsonArray layer_json = layers_json.next_array();
                int count = 0;
                std::string tmp_ter;
                oter_id tmp_otid( 0 );
                for( int j = 0; j < OMAPY; j++ ) {
                    for( int i = 0; i < OMAPX; i++ ) {
                        if( count == 0 ) {
                            {
                                JsonArray rle_terrain = layer_json.next_array();
                                tmp_ter = rle_terrain.next_string();
                                count = rle_terrain.next_int();
                                if( rle_terrain.has_more() ) {
                                    rle_terrain.throw_error( 2, "Unexpected value in RLE encoding" );
                                }
                            }
                            if( is_oter_id_obsolete( tmp_ter ) ) {
                                for( int p = i; p < i + count; p++ ) {
                                    oter_id_migrations.emplace( tripoint_om_omt( p, j, z - OVERMAP_DEPTH ), tmp_ter );
                                }
                            } else if( oter_str_id( tmp_ter ).is_valid() ) {
                                tmp_otid = oter_id( tmp_ter );
                            } else {
                                debugmsg( "Loaded invalid oter_id '%s'", tmp_ter.c_str() );
                                tmp_otid = oter_omt_obsolete;
                            }
                        }
                        count--;
                        layer[z].terrain[i][j] = tmp_otid;
                    }
                }
            }
            migrate_oter_ids( oter_id_migrations );
        } else if( name == "region_id" ) {
            std::string new_region_id;
            om_member.read( new_region_id );
            if( settings->id != new_region_id ) {
                t_regional_settings_map_citr rit = region_settings_map.find( new_region_id );
                if( rit != region_settings_map.end() ) {
                    // TODO: optimize
                    settings = &rit->second;
                }
            }
        } else if( name == "mongroups" ) {
            load_legacy_monstergroups( om_member );
        } else if( name == "monster_groups" ) {
            load_monster_groups( om_member );
        } else if( name == "cities" ) {
            JsonArray cities_json = om_member;
            for( JsonObject city_json : cities_json ) {
                city new_city;
                for( JsonMember city_member : city_json ) {
                    std::string city_member_name = city_member.name();
                    if( city_member_name == "name" ) {
                        city_member.read( new_city.name );
                    } else if( city_member_name == "id" ) {
                        city_member.read( new_city.id );
                    } else if( city_member_name == "database_id" ) {
                        city_member.read( new_city.database_id );
                    } else if( city_member_name == "pos" ) {
                        city_member.read( new_city.pos );
                    } else if( city_member_name == "pos_om" ) {
                        city_member.read( new_city.pos_om );
                    } else if( city_member_name == "x" ) {
                        city_member.read( new_city.pos.x() );
                    } else if( city_member_name == "y" ) {
                        city_member.read( new_city.pos.y() );
                    } else if( city_member_name == "population" ) {
                        city_member.read( new_city.population );
                    } else if( city_member_name == "size" ) {
                        city_member.read( new_city.size );
                    }
                }
                cities.push_back( new_city );
            }
        } else if( name == "connections_out" ) {
            om_member.read( connections_out );
        } else if( name == "roads_out" ) {
            // Legacy data, superseded by that stored in the "connections_out" member. A load and save
            // cycle will migrate this to "connections_out".
            std::vector<tripoint_om_omt> &roads_out =
                connections_out[overmap_connection_local_road];
            JsonArray roads_json = om_member;
            for( JsonObject road_json : roads_json ) {
                tripoint_om_omt new_road;
                for( JsonMember road_member : road_json ) {
                    std::string road_member_name = road_member.name();
                    if( road_member_name == "x" ) {
                        road_member.read( new_road.x() );
                    } else if( road_member_name == "y" ) {
                        road_member.read( new_road.y() );
                    }
                }
                roads_out.push_back( new_road );
            }
        } else if( name == "radios" ) {
            JsonArray radios_json = om_member;
            for( JsonObject radio_json : radios_json ) {
                radio_tower new_radio{ point_om_sm( point_min ) };
                for( JsonMember radio_member : radio_json ) {
                    const std::string radio_member_name = radio_member.name();
                    if( radio_member_name == "type" ) {
                        const std::string radio_name = radio_member.get_string();
                        const auto mapping =
                            find_if( radio_type_names.begin(), radio_type_names.end(),
                        [radio_name]( const std::pair<radio_type, std::string> &p ) {
                            return p.second == radio_name;
                        } );
                        if( mapping != radio_type_names.end() ) {
                            new_radio.type = mapping->first;
                        }
                    } else if( radio_member_name == "x" ) {
                        radio_member.read( new_radio.pos.x() );
                    } else if( radio_member_name == "y" ) {
                        radio_member.read( new_radio.pos.y() );
                    } else if( radio_member_name == "strength" ) {
                        radio_member.read( new_radio.strength );
                    } else if( radio_member_name == "message" ) {
                        radio_member.read( new_radio.message );
                    } else if( radio_member_name == "frequency" ) {
                        radio_member.read( new_radio.frequency );
                    }
                }
                radios.push_back( new_radio );
            }
        } else if( name == "monster_map" ) {
            JsonArray monster_map_json = om_member;
            while( monster_map_json.has_more() ) {
                tripoint_om_sm monster_location;
                monster new_monster;
                monster_location.deserialize( monster_map_json.next_value() );
                new_monster.deserialize( monster_map_json.next_object(), project_combine( loc, monster_location ) );
                monster_map.insert( std::make_pair( monster_location,
                                                    std::move( new_monster ) ) );
            }
        } else if( name == "tracked_vehicles" ) {
            JsonArray tracked_vehicles_json = om_member;
            for( JsonObject tracked_vehicle_json : tracked_vehicles_json ) {
                om_vehicle new_tracker;
                int id;
                for( JsonMember tracker_member : tracked_vehicle_json ) {
                    std::string tracker_member_name = tracker_member.name();
                    if( tracker_member_name == "id" ) {
                        tracker_member.read( id );
                    } else if( tracker_member_name == "x" ) {
                        tracker_member.read( new_tracker.p.x() );
                    } else if( tracker_member_name == "y" ) {
                        tracker_member.read( new_tracker.p.y() );
                    } else if( tracker_member_name == "name" ) {
                        tracker_member.read( new_tracker.name );
                    }
                }
                vehicles[id] = new_tracker;
            }
        } else if( name == "scent_traces" ) {
            JsonArray scents_json = om_member;
            for( JsonObject scent_json : scents_json ) {
                tripoint_abs_omt pos;
                time_point time = calendar::before_time_starts;
                int strength = 0;
                for( JsonMember scent_member : scent_json ) {
                    std::string scent_member_name = scent_member.name();
                    if( scent_member_name == "pos" ) {
                        scent_member.read( pos );
                    } else if( scent_member_name == "time" ) {
                        scent_member.read( time );
                    } else if( scent_member_name == "strength" ) {
                        scent_member.read( strength );
                    }
                }
                scents[pos] = scent_trace( time, strength );
            }
        } else if( name == "npcs" ) {
            JsonArray npcs_json = om_member;
            for( JsonObject npc_json : npcs_json ) {
                shared_ptr_fast<npc> new_npc = make_shared_fast<npc>();
                new_npc->deserialize( npc_json );
                if( !new_npc->get_fac_id().str().empty() ) {
                    new_npc->set_fac( new_npc->get_fac_id() );
                }
                npcs.push_back( new_npc );
            }
        } else if( name == "camps" ) {
            JsonArray camps_json = om_member;
            for( JsonObject camp_json : camps_json ) {
                basecamp new_camp;
                new_camp.deserialize( camp_json );
                camps.push_back( new_camp );
            }
        } else if( name == "overmap_special_placements" ) {
            JsonArray special_placements_json = om_member;
            for( JsonObject special_placement_json : special_placements_json ) {
                overmap_special_id s;
                bool is_safe_zone = false;
                if( special_placement_json.has_member( "special" ) ) {
                    special_placement_json.read( "special", s );
                    s = overmap_special_migration::migrate( s );
                    if( !s.is_null() ) {
                        is_safe_zone = s->has_flag( "SAFE_AT_WORLDGEN" );
                    }
                }
                for( JsonMember special_placement_member : special_placement_json ) {
                    std::string name = special_placement_member.name();
                    if( name == "placements" ) {
                        JsonArray placements_json = special_placement_member;
                        for( JsonObject placement_json : placements_json ) {
                            for( JsonMember placement_member : placement_json ) {
                                std::string name = placement_member.name();
                                if( name == "points" ) {
                                    JsonArray points_json = placement_member;
                                    for( JsonObject point_json : points_json ) {
                                        tripoint_om_omt p;
                                        for( JsonMember point_member : point_json ) {
                                            std::string name = point_member.name();
                                            if( name == "p" ) {
                                                point_member.read( p );
                                                if( !s.is_null() ) {
                                                    overmap_special_placements[p] = s;
                                                    if( is_safe_zone ) {
                                                        safe_at_worldgen.emplace( p );
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
        } else if( name == "joins_used" ) {
            std::vector<std::pair<om_pos_dir, std::string>> flat_index;
            om_member.read( flat_index, true );
            for( const std::pair<om_pos_dir, std::string> &p : flat_index ) {
                joins_used.insert( p );
            }
        } else if( name == "predecessors" ) {
            std::vector<std::pair<tripoint_om_omt, std::vector<oter_id>>> flattened_predecessors;
            om_member.read( flattened_predecessors, true );
            for( std::pair<tripoint_om_omt, std::vector<oter_id>> &p : flattened_predecessors ) {
                predecessors_.insert( std::move( p ) );
            }
        }
    }
}

// throws std::exception
void overmap::unserialize_omap( const JsonValue &jsin, const cata_path &json_path )
{
    JsonArray ja = jsin.get_array();
    JsonObject jo = ja.next_object();

    std::string type;
    point_abs_om om_pos( point_min );
    int z = 0;

    jo.read( "type", type );
    jo.read( "om_pos", om_pos );
    jo.read( "z", z );
    JsonArray jal = jo.get_array( "layers" );

    std::vector<tripoint_om_omt> lake_points;
    std::vector<tripoint_om_omt> forest_points;

    if( type == "overmap" ) {
        std::unordered_map<tripoint_om_omt, std::string> oter_id_migrations;
        if( om_pos != pos() ) {
            debugmsg( "Loaded invalid overmap from omap file %s. Loaded %s, expected %s",
                      json_path.generic_u8string(), om_pos.to_string(), pos().to_string() );
        } else {
            int count = 0;
            std::string tmp_ter;
            oter_id tmp_otid( 0 );
            for( int j = 0; j < OMAPY; j++ ) {
                for( int i = 0; i < OMAPX; i++ ) {
                    if( count == 0 ) {
                        JsonArray jat = jal.next_array();
                        tmp_ter = jat.next_string();
                        count = jat.next_int();
                        if( is_oter_id_obsolete( tmp_ter ) ) {
                            for( int p = i; p < i + count; p++ ) {
                                oter_id_migrations.emplace( tripoint_om_omt( p, j, z - OVERMAP_DEPTH ), tmp_ter );
                            }
                        } else if( oter_str_id( tmp_ter ).is_valid() ) {
                            tmp_otid = oter_id( tmp_ter );
                        } else {
                            debugmsg( "Loaded invalid oter_id '%s'", tmp_ter.c_str() );
                            tmp_otid = oter_omt_obsolete;
                        }
                    }
                    count--;
                    layer[z + OVERMAP_DEPTH].terrain[i][j] = tmp_otid;
                    if( tmp_otid == oter_lake_shore || tmp_otid == oter_lake_surface ) {
                        lake_points.emplace_back( tripoint_om_omt( i, j, z ) );
                    }
                    if( tmp_otid == oter_forest || tmp_otid == oter_forest_thick ) {
                        forest_points.emplace_back( tripoint_om_omt( i, j, z ) );
                    }
                }
            }
        }
        migrate_oter_ids( oter_id_migrations );
    }

    std::unordered_set<tripoint_om_omt> lake_set;
    for( auto &p : lake_points ) {
        lake_set.emplace( p );
    }

    for( auto &p : lake_points ) {
        if( !inbounds( p ) ) {
            continue;
        }

        bool shore = false;
        for( int ni = -1; ni <= 1 && !shore; ni++ ) {
            for( int nj = -1; nj <= 1 && !shore; nj++ ) {
                const tripoint_om_omt n = p + point( ni, nj );
                if( inbounds( n, 1 ) && lake_set.find( n ) == lake_set.end() ) {
                    shore = true;
                }
            }
        }

        ter_set( tripoint_om_omt( p ), shore ? oter_lake_shore : oter_lake_surface );

        // If this is not a shore, we'll make our subsurface lake cubes and beds.
        if( !shore ) {
            for( int z = -1; z > settings->overmap_lake.lake_depth; z-- ) {
                ter_set( tripoint_om_omt( p.xy(), z ), oter_lake_water_cube );
            }
            ter_set( tripoint_om_omt( p.xy(), settings->overmap_lake.lake_depth ), oter_lake_bed );
            layer[p.z() + OVERMAP_DEPTH].terrain[p.x()][p.y()] = oter_lake_surface;
        }
    }

    std::unordered_set<tripoint_om_omt> forest_set;
    for( auto &p : forest_points ) {
        forest_set.emplace( p );
    }

    for( auto &p : forest_points ) {
        if( !inbounds( p ) ) {
            continue;
        }
        bool forest_border = false;
        for( int ni = -1; ni <= 1 && !forest_border; ni++ ) {
            for( int nj = -1; nj <= 1 && !forest_border; nj++ ) {
                const tripoint_om_omt n = p + point( ni, nj );
                if( inbounds( n, 1 ) && forest_set.find( n ) == forest_set.end() ) {
                    forest_border = true;
                }
            }
        }

        ter_set( tripoint_om_omt( p ), forest_border ? oter_forest : oter_forest_thick );
    }
}

template<typename MdArray>
static void unserialize_array_from_compacted_sequence( JsonArray &ja, MdArray &array )
{
    int count = 0;
    using Value = typename MdArray::value_type;
    Value value = Value();
    for( size_t j = 0; j < MdArray::size_y; ++j ) {
        for( size_t i = 0; i < MdArray::size_x; ++i ) {
            if( count == 0 ) {
                JsonArray sequence = ja.next_array();
                sequence.read_next( value );
                sequence.read_next( count );
                if( sequence.size() > 2 ) {
                    sequence.throw_error( "Too many values for compacted sequence" );
                }
            }
            count--;
            array[i][j] = value;
        }
    }
}

// throws std::exception
void overmap::unserialize_view( const cata_path &file_name, std::istream &fin )
{
    size_t json_offset = chkversion( fin );
    JsonValue jsin = json_loader::from_path_at_offset( file_name, json_offset );
    unserialize_view( jsin.get_object() );
}

void overmap::unserialize_view( const JsonObject &jsobj )
{
    for( JsonMember view_member : jsobj ) {
        const std::string name = view_member.name();
        if( name == "visible" ) {
            JsonArray visible_json = view_member;
            for( int z = 0; z < OVERMAP_LAYERS; ++z ) {
                JsonArray visible_by_z_json = visible_json.next_array();
                unserialize_array_from_compacted_sequence( visible_by_z_json, layer[z].visible );
                if( visible_by_z_json.has_more() ) {
                    visible_by_z_json.throw_error( "Too many sequences for z visible view" );
                }
            }
            if( visible_json.has_more() ) {
                visible_json.throw_error( "Too many views by z count" );
            }
        } else if( name == "explored" ) {
            JsonArray explored_json = view_member;
            for( int z = 0; z < OVERMAP_LAYERS; ++z ) {
                JsonArray explored_by_z_json = explored_json.next_array();
                unserialize_array_from_compacted_sequence( explored_by_z_json, layer[z].explored );
                if( explored_by_z_json.has_more() ) {
                    explored_by_z_json.throw_error( "Too many sequences for z explored view" );
                }
            }
            if( explored_json.has_more() ) {
                explored_json.throw_error( "Too many views by z count" );
            }
        } else if( name == "notes" ) {
            JsonArray notes_json = view_member;
            for( int z = 0; z < OVERMAP_LAYERS; ++z ) {
                JsonArray notes_by_z_json = notes_json.next_array();
                for( JsonArray note_json : notes_by_z_json ) {
                    om_note tmp;
                    note_json.read_next( tmp.p.x() );
                    note_json.read_next( tmp.p.y() );
                    note_json.read_next( tmp.text );
                    note_json.read_next( tmp.dangerous );
                    note_json.read_next( tmp.danger_radius );
                    if( note_json.size() > 5 ) {
                        note_json.throw_error( "Too many values for note" );
                    }

                    layer[z].notes.push_back( tmp );
                }
            }
            if( notes_json.has_more() ) {
                notes_json.throw_error( "Too many notes by z count" );
            }
        } else if( name == "extras" ) {
            JsonArray extras_json = view_member;
            for( int z = 0; z < OVERMAP_LAYERS; ++z ) {
                JsonArray extras_by_z = extras_json.next_array();
                for( JsonArray extra_json : extras_by_z ) {
                    om_map_extra tmp;
                    extra_json.read_next( tmp.p.x() );
                    extra_json.read_next( tmp.p.y() );
                    extra_json.read_next( tmp.id );
                    if( extra_json.has_more() ) {
                        extra_json.throw_error( "Too many values for extra" );
                    }

                    layer[z].extras.push_back( tmp );
                }
            }
            if( extras_json.has_more() ) {
                extras_json.throw_error( "Too many extras by z count" );
            }
        }
    }
}
template<typename MdArray>
static void serialize_array_to_compacted_sequence( JsonOut &json, const MdArray &array )
{
    static_assert( std::is_same<typename MdArray::value_type, bool>::value,
                   "This implementation assumes bool, in that the initial value of lastval has "
                   "to not be a valid value of the content" );
    int count = 0;
    int lastval = -1;
    for( size_t j = 0; j < MdArray::size_y; ++j ) {
        for( size_t i = 0; i < MdArray::size_x; ++i ) {
            const int value = array[i][j];
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
               a.population == b.population &&
               a.target == b.target &&
               a.interest == b.interest &&
               a.dying == b.dying &&
               a.horde == b.horde &&
               a.behaviour == b.behaviour;
    }
};

struct mongroup_hash {
    std::size_t operator()( const mongroup &mg ) const {
        // Note: not hashing monsters or position
        size_t ret = std::hash<mongroup_id>()( mg.type );
        cata::hash_combine( ret, mg.population );
        cata::hash_combine( ret, mg.target );
        cata::hash_combine( ret, mg.interest );
        cata::hash_combine( ret, mg.dying );
        cata::hash_combine( ret, mg.horde );
        cata::hash_combine( ret, mg.behaviour );
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
        saved_group.abs_pos = tripoint_abs_sm();
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
    json.member( "region_id", settings->id );
    fout << std::endl;

    save_monster_groups( json );
    fout << std::endl;

    json.member( "cities" );
    json.start_array();
    for( const city &i : cities ) {
        json.start_object();
        json.member( "name", i.name );
        json.member( "id", i.id );
        json.member( "database_id", i.database_id );
        json.member( "pos_om", i.pos_om );
        json.member( "pos", i.pos );
        json.member( "population", i.population );
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
        json.member( "frequency", i.frequency );
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
    for( const basecamp &i : camps ) {
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

    json.member( "mapgen_arg_storage", mapgen_arg_storage );
    fout << std::endl;
    json.member( "mapgen_arg_index" );
    json.start_array();
    for( const std::pair<const tripoint_om_omt, std::optional<mapgen_arguments> *> &p :
         mapgen_args_index ) {
        json.start_array();
        json.write( p.first );
        auto it = mapgen_arg_storage.get_iterator_from_pointer( p.second );
        int index = mapgen_arg_storage.get_index_from_iterator( it );
        json.write( index );
        json.end_array();
    }
    json.end_array();
    fout << std::endl;

    std::vector<std::pair<om_pos_dir, std::string>> flattened_joins_used(
                joins_used.begin(), joins_used.end() );
    json.member( "joins_used", flattened_joins_used );
    fout << std::endl;

    std::vector<std::pair<tripoint_om_omt, std::vector<oter_id>>> flattened_predecessors(
        predecessors_.begin(), predecessors_.end() );
    json.member( "predecessors", flattened_predecessors );
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
    archive.io( "abs_pos", abs_pos, tripoint_abs_sm() );
    archive.io( "population", population, 1u );
    archive.io( "dying", dying, false );
    archive.io( "horde", horde, false );
    archive.io( "target", target, point_abs_sm() );
    archive.io( "nemesis_target", nemesis_target, point_abs_sm() );
    archive.io( "interest", interest, 0 );
    archive.io( "horde_behaviour", behaviour, horde_behaviour::none );
    archive.io( "monsters", monsters, io::empty_default_tag() );
}

void mongroup::deserialize( const JsonObject &jo )
{
    jo.allow_omitted_members();
    io::JsonObjectInputArchive archive( jo );
    io( archive );
}

void mongroup::serialize( JsonOut &json ) const
{
    io::JsonObjectOutputArchive archive( json );
    const_cast<mongroup *>( this )->io( archive );
}

void mongroup::deserialize_legacy( const JsonObject &jo )
{
    for( JsonMember json : jo ) {
        std::string name = json.name();
        if( name == "type" ) {
            type = mongroup_id( json.get_string() );
        } else if( name == "abs_pos" ) {
            abs_pos.deserialize( json );
        } else if( name == "population" ) {
            population = json.get_int();
        } else if( name == "dying" ) {
            dying = json.get_bool();
        } else if( name == "horde" ) {
            horde = json.get_bool();
        } else if( name == "target" ) {
            target.deserialize( json );
        } else if( name == "nemesis_target" ) {
            nemesis_target.deserialize( json );
        } else if( name == "interest" ) {
            interest = json.get_int();
        } else if( name == "horde_behaviour" ) {
            json.read( behaviour );
        } else if( name == "monsters" ) {
            JsonArray ja = json;
            for( JsonObject monster_json : ja ) {
                monster new_monster;
                new_monster.deserialize( monster_json );
                monsters.push_back( new_monster );
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////
///// mapbuffer

///////////////////////////////////////////////////////////////////////////////////////
///// SAVE_MASTER (i.e. master.gsav)

void mission::unserialize_all( const JsonArray &ja )
{
    for( JsonObject jo : ja ) {
        mission mis;
        mis.deserialize( jo );
        add_existing( mis );
    }
}

void game::unserialize_master( const cata_path &file_name, std::istream &fin )
{
    savegame_loading_version = 0;
    size_t json_offset = chkversion( fin );
    try {
        JsonValue jv = json_loader::from_path_at_offset( file_name, json_offset );
        unserialize_master( jv );
    } catch( const JsonError &e ) {
        debugmsg( "error loading %s: %s", SAVE_MASTER, e.c_str() );
    }
}

void game::unserialize_master( const JsonValue &jv )
{
    JsonObject game_json = jv;
    for( JsonMember jsin : game_json ) {
        std::string name = jsin.name();
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
        } else if( name == "timed_events" ) {
            timed_event_manager::unserialize_all( jsin );
        } else if( name == "placed_unique_specials" ) {
            overmap_buffer.deserialize_placed_unique_specials( jsin );
        }
    }
}

void mission::serialize_all( JsonOut &json )
{
    json.start_array();
    for( mission *&e : get_all_active() ) {
        e->serialize( json );
    }
    json.end_array();
}

void weather_manager::serialize_all( JsonOut &json )
{
    weather_manager &weather = get_weather();
    json.start_object();
    json.member( "lightning", weather.lightning_active );
    json.member( "weather_id", weather.weather_id );
    json.member( "next_weather", weather.nextweather );
    json.member( "temperature", units::to_fahrenheit( weather.temperature ) );
    json.member( "winddirection", weather.winddirection );
    json.member( "windspeed", weather.windspeed );
    if( weather.forced_temperature ) {
        json.member( "forced_temperature", units::to_fahrenheit( *weather.forced_temperature ) );
    }
    json.end_object();
}

void weather_manager::unserialize_all( const JsonObject &w )
{
    w.read( "lightning", get_weather().lightning_active );
    w.read( "weather_id", get_weather().weather_id );
    w.read( "next_weather", get_weather().nextweather );
    float read_temperature;
    w.read( "temperature", read_temperature );
    get_weather().temperature = units::from_fahrenheit( read_temperature );
    w.read( "winddirection", get_weather().winddirection );
    w.read( "windspeed", get_weather().windspeed );
    if( w.has_member( "forced_temperature" ) ) {
        float read_forced_temp;
        w.read( "forced_temperature", read_forced_temp );
        get_weather().forced_temperature = units::from_fahrenheit( read_forced_temp );
    } else {
        get_weather().forced_temperature.reset();
    }
}

void global_variables::unserialize( JsonObject &jo )
{
    jo.read( "global_vals", global_values );
}

void timed_event_manager::unserialize_all( const JsonArray &ja )
{
    for( JsonObject jo : ja ) {
        int type;
        time_point when;
        int faction_id;
        int strength;
        tripoint_abs_ms map_square;
        tripoint_abs_sm map_point;
        std::string string_id;
        std::string key;
        submap revert;
        jo.read( "faction", faction_id );
        jo.read( "map_point", map_point );
        jo.read( "map_square", map_square, false );
        jo.read( "strength", strength );
        jo.read( "string_id", string_id );
        jo.read( "type", type );
        jo.read( "when", when );
        jo.read( "key", key );
        point pt;
        if( jo.has_string( "revert" ) ) {
            revert.set_all_ter( ter_id( jo.get_string( "revert" ) ), true );
        } else {
            for( JsonObject jp : jo.get_array( "revert" ) ) {
                if( jp.has_member( "point" ) ) {
                    jp.get_member( "point" ).read( pt, false );
                }
                revert.set_furn( pt, furn_id( jp.get_string( "furn" ) ) );
                revert.set_ter( pt, ter_id( jp.get_string( "ter" ) ) );
                revert.set_trap( pt, trap_id( jp.get_string( "trap" ) ) );
                if( jp.has_member( "items" ) ) {
                    cata::colony<item> itm;
                    jp.get_member( "items" ).read( itm, false );
                    revert.get_items( pt ) = std::move( itm );
                }
                // We didn't always save the point, this is the original logic, it doesn't work right but for older saves at least they won't crash
                if( !jp.has_member( "point" ) ) {
                    if( pt.x++ < SEEX ) {
                        pt.x = 0;
                        pt.y++;
                    }
                }
            }
        }
        get_timed_events().add( static_cast<timed_event_type>( type ), when, faction_id, map_square,
                                strength,
                                string_id, std::move( revert ), key );
    }
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
        json.member( "placed_unique_specials" );
        overmap_buffer.serialize_placed_unique_specials( json );

        json.member( "timed_events" );
        timed_event_manager::serialize_all( json );

        json.member( "factions", *faction_manager_ptr );
        json.member( "seed", seed );

        json.member( "weather" );
        weather_manager::serialize_all( json );

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

void global_variables::serialize( JsonOut &jsout ) const
{
    jsout.member( "global_vals", global_values );
}

void timed_event_manager::serialize_all( JsonOut &jsout )
{
    jsout.start_array();
    for( const timed_event &elem : get_timed_events().events ) {
        jsout.start_object();
        jsout.member( "faction", elem.faction_id );
        jsout.member( "map_point", elem.map_point );
        jsout.member( "map_square", elem.map_square );
        jsout.member( "strength", elem.strength );
        jsout.member( "string_id", elem.string_id );
        jsout.member( "type", elem.type );
        jsout.member( "when", elem.when );
        jsout.member( "key", elem.key );
        if( elem.revert.is_uniform() ) {
            jsout.member( "revert", elem.revert.get_ter( point_zero ) );
        } else {
            jsout.member( "revert" );
            jsout.start_array();
            for( int y = 0; y < SEEY; y++ ) {
                for( int x = 0; x < SEEX; x++ ) {
                    jsout.start_object();
                    point pt( x, y );
                    jsout.member( "point", pt );
                    jsout.member( "furn", elem.revert.get_furn( pt ) );
                    jsout.member( "ter", elem.revert.get_ter( pt ) );
                    jsout.member( "trap", elem.revert.get_trap( pt ) );
                    jsout.member( "items", elem.revert.get_items( pt ) );
                    jsout.end_object();
                }
            }
            jsout.end_array();
        }
        jsout.end_object();
    }
    jsout.end_array();
}

void faction_manager::deserialize( const JsonValue &jv )
{
    if( jv.test_object() ) {
        // whoops - this recovers factions saved under the wrong format.
        JsonObject jo = jv;
        for( JsonMember jm : jo ) {
            faction add_fac;
            add_fac.id = faction_id( jm.name() );
            jm.read( add_fac );
            faction *old_fac = get( add_fac.id, false );
            if( old_fac ) {
                *old_fac = add_fac;
                // force a revalidation of add_fac
                get( add_fac.id, false );
            } else {
                factions[add_fac.id] = add_fac;
            }
        }
    } else if( jv.test_array() ) {
        // how it should have been serialized.
        JsonArray ja = jv;
        for( JsonValue jav : ja ) {
            faction add_fac;
            jav.read( add_fac );
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

void creature_tracker::deserialize( const JsonArray &ja )
{
    monsters_list.clear();
    monsters_by_location.clear();
    for( JsonValue jv : ja ) {
        // TODO: would be nice if monster had a constructor using JsonIn or similar, so this could be one statement.
        shared_ptr_fast<monster> mptr = make_shared_fast<monster>();
        jv.read( *mptr );
        add( mptr );
    }
}

void creature_tracker::serialize( JsonOut &jsout ) const
{
    jsout.start_array();
    for( const auto &monster_ptr : monsters_list ) {
        jsout.write( *monster_ptr );
    }
    jsout.end_array();
}

void overmapbuffer::serialize_placed_unique_specials( JsonOut &json ) const
{
    json.write_as_array( placed_unique_specials );
}

void overmapbuffer::deserialize_placed_unique_specials( const JsonValue &jsin )
{
    placed_unique_specials.clear();
    JsonArray ja = jsin.get_array();
    for( const JsonValue &special : ja ) {
        placed_unique_specials.emplace( special.get_string() );
    }
}
