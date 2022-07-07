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

static const oter_type_str_id oter_type_bridge( "bridge" );
static const oter_type_str_id oter_type_bridge_road( "bridge_road" );

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
void game::unserialize( std::istream &fin, const std::string &path )
{
    chkversion( fin );
    int tmpturn = 0;
    int tmpcalstart = 0;
    int tmprun = 0;
    tripoint_om_sm lev;
    point_abs_om com;
    JsonIn jsin( fin, path );
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
void game::load_shortcuts( std::istream &fin, const std::string &path )
{
    JsonIn jsin( fin, path );
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
            old == "s_lot" || old == "mine_entrance" || old == "mine_finale" ||
            old == "triffid_finale" ) {
            ter_set( pos, oter_id( old + "_north" ) );
        } else if( old.compare( 0, 6, "bridge" ) == 0 ) {
            ter_set( pos, oter_id( old ) );
            const oter_id oter_ground = ter( tripoint_om_omt( pos.xy(), 0 ) );
            const oter_id oter_above = ter( pos + tripoint_above );
            if( ( oter_ground->get_type_id() == oter_type_bridge ) &&
                !( oter_above->get_type_id() == oter_type_bridge_road ) ) {
                ter_set( pos + tripoint_above, oter_id( "bridge_road" + oter_get_rotation_string( oter_ground ) ) );
                bridge_points.emplace_back( pos.xy() );
            }
        } else if( old == "triffid_grove" ) {
            {
                ter_set( pos, oter_id( "triffid_grove_north" ) );
                ter_set( pos + point_north, oter_id( "triffid_field_north" ) );
                ter_set( pos + point_north_east, oter_id( "triffid_field_north" ) );
                ter_set( pos + point_east, oter_id( "triffid_field_north" ) );
                ter_set( pos + point_south_east, oter_id( "triffid_field_north" ) );
                ter_set( pos + point_south, oter_id( "triffid_field_north" ) );
                ter_set( pos + point_south_west, oter_id( "triffid_field_north" ) );
                ter_set( pos + point_west, oter_id( "triffid_field_north" ) );
                ter_set( pos + point_north_west, oter_id( "triffid_field_north" ) );
                ter_set( pos + tripoint_above, oter_id( "triffid_grove_z2_north" ) );
                ter_set( pos + tripoint( 0, 0, 2 ), oter_id( "triffid_grove_z3_north" ) );
                ter_set( pos + tripoint( 0, 0, 3 ), oter_id( "triffid_grove_roof_north" ) );
            }
        } else if( old == "triffid_roots" ) {
            {
                ter_set( pos, oter_id( "triffid_roots_north" ) );
                ter_set( pos + point_south, oter_id( "triffid_rootsn_north" ) );
                ter_set( pos + point_south_east, oter_id( "triffid_rootsen_north" ) );
                ter_set( pos + point_east, oter_id( "triffid_rootse_north" ) );
                ter_set( pos + point_north_east, oter_id( "triffid_rootsse_north" ) );
                ter_set( pos + point_north, oter_id( "triffid_rootss_north" ) );
                ter_set( pos + point_north_west, oter_id( "triffid_rootssw_north" ) );
                ter_set( pos + point_west, oter_id( "triffid_rootsw_north" ) );
                ter_set( pos + point_south_west, oter_id( "triffid_rootsnw_north" ) );
            }
        } else if( old.compare( 0, 10, "mass_grave" ) == 0 ) {
            ter_set( pos, oter_id( "field" ) );
        } else if( old.compare( 0, 11, "pond_forest" ) == 0 ) {
            ter_set( pos, oter_id( "forest" ) );
        } else if( old.compare( 0, 10, "pond_swamp" ) == 0 ) {
            ter_set( pos, oter_id( "forest_water" ) );
        } else if( old == "mine_shaft" ) {
            ter_set( pos, oter_id( "mine_shaft_middle_north" ) );
        } else if( old.compare( 0, 30, "microlab_generic_hallway_start" ) == 0 ||
                   old.compare( 0, 24, "microlab_generic_hallway" ) == 0 ) {
            ter_set( pos, oter_id( "microlab_generic" ) );
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
        } else if( old == "anthill" ||
                   old == "acid_anthill" ||
                   old == "ants_larvae" ||
                   old == "ants_larvae_acid" ||
                   old == "ants_queen" ||
                   old == "ants_queen_acid" ||
                   old == "ants_food" ) {
            std::string new_ = old;
            if( string_ends_with( new_, "_acid" ) ) {
                new_.erase( new_.end() - 5, new_.end() );
            }
            if( string_starts_with( new_, "acid_" ) ) {
                new_.erase( new_.begin(), new_.begin() + 5 );
            }
            ter_set( pos, oter_id( new_ + "_north" ) );
        }

        for( const convert_nearby &conv : nearby ) {
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
        new_group.deserialize( jsin.get_object() );

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
            if( settings->id != new_region_id ) {
                t_regional_settings_map_citr rit = region_settings_map.find( new_region_id );
                if( rit != region_settings_map.end() ) {
                    // TODO: optimize
                    settings = pimpl<regional_settings>( rit->second );
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
                    } else if( city_member_name == "id" ) {
                        jsin.read( new_city.id );
                    } else if( city_member_name == "database_id" ) {
                        jsin.read( new_city.database_id );
                    } else if( city_member_name == "pos" ) {
                        jsin.read( new_city.pos );
                    } else if( city_member_name == "pos_om" ) {
                        jsin.read( new_city.pos_om );
                    } else if( city_member_name == "x" ) {
                        jsin.read( new_city.pos.x() );
                    } else if( city_member_name == "y" ) {
                        jsin.read( new_city.pos.y() );
                    } else if( city_member_name == "population" ) {
                        jsin.read( new_city.population );
                    } else if( city_member_name == "size" ) {
                        jsin.read( new_city.size );
                    }
                }
                cities.push_back( new_city );
            }
        } else if( name == "connections_out" ) {
            jsin.read( connections_out );
        } else if( name == "roads_out" ) {
            // Legacy data, superseded by that stored in the "connections_out" member. A load and save
            // cycle will migrate this to "connections_out".
            std::vector<tripoint_om_omt> &roads_out =
                connections_out[overmap_connection_local_road];
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
                new_monster.deserialize( jsin.get_object(), project_combine( loc, monster_location ) );
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
                new_npc->deserialize( jsin.get_object() );
                if( !new_npc->get_fac_id().str().empty() ) {
                    new_npc->set_fac( new_npc->get_fac_id() );
                }
                npcs.push_back( new_npc );
            }
        } else if( name == "camps" ) {
            jsin.start_array();
            while( !jsin.end_array() ) {
                basecamp new_camp;
                new_camp.deserialize( jsin.get_object() );
                camps.push_back( new_camp );
            }
        } else if( name == "overmap_special_placements" ) {
            jsin.start_array();
            while( !jsin.end_array() ) {
                jsin.start_object();
                overmap_special_id s;
                bool is_safe_zone = false;
                while( !jsin.end_object() ) {
                    std::string name = jsin.get_member_name();
                    if( name == "special" ) {
                        jsin.read( s );
                        s = overmap_special_migration::migrate( s );
                        if( !s.is_null() ) {
                            is_safe_zone = s->has_flag( "SAFE_AT_WORLDGEN" );
                        }
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
        } else if( name == "mapgen_arg_storage" ) {
            jsin.read( mapgen_arg_storage, true );
        } else if( name == "mapgen_arg_index" ) {
            std::vector<std::pair<tripoint_om_omt, int>> flat_index;
            jsin.read( flat_index, true );
            for( const std::pair<tripoint_om_omt, int> &p : flat_index ) {
                auto it = mapgen_arg_storage.get_iterator_from_index( p.second );
                mapgen_args_index.emplace( p.first, &*it );
            }
        } else if( name == "joins_used" ) {
            std::vector<std::pair<om_pos_dir, std::string>> flat_index;
            jsin.read( flat_index, true );
            for( const std::pair<om_pos_dir, std::string> &p : flat_index ) {
                joins_used.insert( p );
            }
        } else if( name == "predecessors" ) {
            std::vector<std::pair<tripoint_om_omt, std::vector<oter_id>>> flattened_predecessors;
            jsin.read( flattened_predecessors, true );
            for( std::pair<tripoint_om_omt, std::vector<oter_id>> &p : flattened_predecessors ) {
                predecessors_.insert( std::move( p ) );
            }
        }
    }
}

// throws std::exception
void overmap::unserialize_omap( std::istream &fin )
{
    JsonIn jsin( fin );
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
        std::unordered_map<tripoint_om_omt, std::string> needs_conversion;
        if( om_pos != pos() ) {
            debugmsg( "Loaded invalid overmap from omap file %s. Loaded %s, expected %s",
                      *jsin.get_path(), om_pos.to_string(), pos().to_string() );
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
        convert_terrain( needs_conversion );
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
    for( const std::pair<const tripoint_om_omt, cata::optional<mapgen_arguments> *> &p :
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
    archive.io( "pos", pos, tripoint_om_sm() );
    archive.io( "abs_pos", abs_pos, tripoint_abs_sm() );
    archive.io( "radius", radius, 1u );
    archive.io( "population", population, 1u );
    archive.io( "diffuse", diffuse, false );
    archive.io( "dying", dying, false );
    archive.io( "horde", horde, false );
    archive.io( "target", target, tripoint_om_sm() );
    archive.io( "nemesis_target", nemesis_target, tripoint_abs_sm() );
    archive.io( "interest", interest, 0 );
    archive.io( "horde_behaviour", horde_behaviour, io::empty_default_tag() );
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

void mongroup::deserialize_legacy( JsonIn &json )
{
    json.start_object();
    while( !json.end_object() ) {
        std::string name = json.get_member_name();
        if( name == "type" ) {
            type = mongroup_id( json.get_string() );
        } else if( name == "pos" ) {
            pos.deserialize( json );
        } else if( name == "abs_pos" ) {
            abs_pos.deserialize( json );
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
        } else if( name == "nemesis_target" ) {
            nemesis_target.deserialize( json );
        } else if( name == "interest" ) {
            interest = json.get_int();
        } else if( name == "horde_behaviour" ) {
            horde_behaviour = json.get_string();
        } else if( name == "monsters" ) {
            json.start_array();
            while( !json.end_array() ) {
                monster new_monster;
                new_monster.deserialize( json.get_object() );
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
        mis.deserialize( jsin.get_object() );
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
                next_npc_id.deserialize( jsin.get_int() );
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
    for( mission *&e : get_all_active() ) {
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

void global_variables::unserialize( JsonObject &jo )
{
    jo.read( "global_vals", global_values );
}

void timed_event_manager::unserialize_all( JsonIn &jsin )
{
    jsin.start_array();
    while( !jsin.end_array() ) {
        JsonObject jo = jsin.get_object();
        int type;
        time_point when;
        int faction_id;
        int strength;
        tripoint_abs_ms map_square;
        tripoint_abs_sm map_point;
        std::string string_id;
        std::string key;
        submap_revert revert;
        jo.read( "faction", faction_id );
        jo.read( "map_point", map_point );
        jo.read( "map_square", map_square, false );
        jo.read( "strength", strength );
        jo.read( "string_id", string_id );
        jo.read( "type", type );
        jo.read( "when", when );
        jo.read( "key", key );
        point pt;
        for( JsonObject jp : jo.get_array( "revert" ) ) {
            revert.set_furn( pt, furn_id( jp.get_string( "furn" ) ) );
            revert.set_ter( pt, ter_id( jp.get_string( "ter" ) ) );
            revert.set_trap( pt, trap_id( jp.get_string( "trap" ) ) );
            if( pt.x++ < SEEX ) {
                pt.x = 0;
                pt.y++;
            }
        }
        get_timed_events().add( static_cast<timed_event_type>( type ), when, faction_id, map_square,
                                strength,
                                string_id, revert, key );
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

        json.member( "timed_events" );
        timed_event_manager::serialize_all( json );

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
        jsout.member( "revert" );
        jsout.start_array();
        for( int y = 0; y < SEEY; y++ ) {
            for( int x = 0; x < SEEX; x++ ) {
                jsout.start_object();
                point pt( x, y );
                jsout.member( "furn", elem.revert.get_furn( pt ) );
                jsout.member( "ter", elem.revert.get_ter( pt ) );
                jsout.member( "trap", elem.revert.get_trap( pt ) );
                jsout.end_object();
            }
        }
        jsout.end_array();
        jsout.end_object();
    }
    jsout.end_array();
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

void creature_tracker::deserialize( JsonIn &jsin )
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

void creature_tracker::serialize( JsonOut &jsout ) const
{
    jsout.start_array();
    for( const auto &monster_ptr : monsters_list ) {
        jsout.write( *monster_ptr );
    }
    jsout.end_array();
}
