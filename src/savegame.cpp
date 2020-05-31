#include "game.h" // IWYU pragma: associated

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
#include "int_id.h"
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
#include "output.h"
#include "overmap.h"
#include "overmap_types.h"
#include "popup.h"
#include "regional_settings.h"
#include "scent_map.h"
#include "stats_tracker.h"
#include "string_id.h"
#include "translations.h"
#include "ui_manager.h"

class overmap_connection;

#if defined(__ANDROID__)
#include "input.h"

extern std::map<std::string, std::list<input_event>> quick_shortcuts_map;
#endif

/*
 * Changes that break backwards compatibility should bump this number, so the game can
 * load a legacy format loader.
 */
const int savegame_version = 29;

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
    std::stringstream rle_out;
    if( is_type ) {
        rle_out << typescent.str();
    } else {
        int rle_lastval = -1;
        int rle_count = 0;
        for( auto &elem : grscent ) {
            for( auto &val : elem ) {
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
    int levx = 0;
    int levy = 0;
    int levz = 0;
    int comx = 0;
    int comy = 0;
    JsonIn jsin( fin );
    try {
        JsonObject data = jsin.get_object();

        data.read( "turn", tmpturn );
        data.read( "calendar_start", tmpcalstart );
        calendar::initial_season = static_cast<season_type>( data.get_int( "initial_season",
                                   static_cast<int>( SPRING ) ) );
        // 0.E stable
        if( savegame_loading_version < 26 ) {
            tmpturn *= 6;
            tmpcalstart *= 6;
        }
        data.read( "auto_travel_mode", auto_travel_mode );
        data.read( "run_mode", tmprun );
        data.read( "mostseen", mostseen );
        data.read( "levx", levx );
        data.read( "levy", levy );
        data.read( "levz", levz );
        data.read( "om_x", comx );
        data.read( "om_y", comy );

        calendar::turn = tmpturn;
        calendar::start_of_cataclysm = tmpcalstart;

        if( !data.read( "game_start", calendar::start_of_game ) ) {
            calendar::start_of_game = calendar::start_of_cataclysm;
        }

        load_map( tripoint( levx + comx * OMAPX * 2, levy + comy * OMAPY * 2, levz ) );

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
        for( auto elem : data.get_array( "stair_monsters" ) ) {
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
                    qslist.push_back( input_event( i, CATA_INPUT_KEYBOARD ) );
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

std::unordered_set<std::string> obsolete_terrains;

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
void overmap::convert_terrain( const std::unordered_map<tripoint, std::string> &needs_conversion )
{
    for( const auto &convert : needs_conversion ) {
        const tripoint pos = convert.first;
        const std::string old = convert.second;

        struct convert_nearby {
            point offset;
            std::string x_id;
            std::string y_id;
            std::string new_id;
        };

        std::vector<convert_nearby> nearby;
        std::vector<std::pair<tripoint, std::string>> convert_unrelated_adjacent_tiles;

        if( old == "apartments_con_tower_1_entrance" ||
            old == "apartments_mod_tower_1_entrance" ) {
            const std::string base = old.substr( 0, old.rfind( "1_entrance" ) );
            const std::string other = base + "1";
            nearby.push_back( { point_north_east, other, other, base + "SW_north" } );
            nearby.push_back( { point_south_west, other, other, base + "SW_south" } );
            nearby.push_back( { point_south_east, other, other, base + "SW_east" } );
            nearby.push_back( { point_north_west, other, other, base + "SW_west" } );

        } else if( old == "apartments_con_tower_1" || old == "apartments_mod_tower_1" ) {
            const std::string base = old.substr( 0, old.rfind( '1' ) );
            const std::string entr = base + "1_entrance";
            nearby.push_back( { point_south_east, old, entr, base + "NW_north" } );
            nearby.push_back( { point_north_west, old, entr, base + "NW_south" } );
            nearby.push_back( { point_south_west, entr, old, base + "NW_east" } );
            nearby.push_back( { point_north_east, entr, old, base + "NW_west" } );
            nearby.push_back( { point_south_west, old, old, base + "NE_north" } );
            nearby.push_back( { point_north_east, old, old, base + "NE_south" } );
            nearby.push_back( { point_north_west, old, old, base + "NE_east" } );
            nearby.push_back( { point_south_east, old, old, base + "NE_west" } );
            nearby.push_back( { point_north_west, entr, old, base + "SE_north" } );
            nearby.push_back( { point_south_east, entr, old, base + "SE_south" } );
            nearby.push_back( { point_north_east, old, entr, base + "SE_east" } );
            nearby.push_back( { point_south_west, old, entr, base + "SE_west" } );

        } else if( old == "subway_station" ) {
            ter_set( pos, oter_id( "underground_sub_station" ) );
        } else if( old == "bridge_ew" ) {
            ter_set( pos, oter_id( "bridge_east" ) );
        } else if( old == "bridge_ns" ) {
            ter_set( pos, oter_id( "bridge_north" ) );
        } else if( old == "public_works_entrance" ) {
            const std::string base = "public_works_";
            const std::string other = "public_works";
            nearby.push_back( { point_north_east, other, other, base + "SW_north" } );
            nearby.push_back( { point_south_west, other, other, base + "SW_south" } );
            nearby.push_back( { point_south_east, other, other, base + "SW_east" } );
            nearby.push_back( { point_north_west, other, other, base + "SW_west" } );

        } else if( old == "public_works" ) {
            const std::string base = "public_works_";
            const std::string entr = "public_works_entrance";
            nearby.push_back( { point_south_east, old, entr, base + "NW_north" } );
            nearby.push_back( { point_north_west, old, entr, base + "NW_south" } );
            nearby.push_back( { point_south_west, entr, old, base + "NW_east" } );
            nearby.push_back( { point_north_east, entr, old, base + "NW_west" } );
            nearby.push_back( { point_south_west, old, old, base + "NE_north" } );
            nearby.push_back( { point_north_east, old, old, base + "NE_south" } );
            nearby.push_back( { point_north_west, old, old, base + "NE_east" } );
            nearby.push_back( { point_south_east, old, old, base + "NE_west" } );
            nearby.push_back( { point_north_west, entr, old, base + "SE_north" } );
            nearby.push_back( { point_south_east, entr, old, base + "SE_south" } );
            nearby.push_back( { point_north_east, old, entr, base + "SE_east" } );
            nearby.push_back( { point_south_west, old, entr, base + "SE_west" } );

        } else if( old.compare( 0, 7, "school_" ) == 0 ) {
            const std::string school = "school_";
            const std::string school_1 = school + "1_";
            if( old == school + "1" ) {
                nearby.push_back( { point_south_west, school + "2", school + "4", school_1 + "1_north" } );
                nearby.push_back( { point_north_west, school + "4", school + "2", school_1 + "1_east" } );
                nearby.push_back( { point_north_east, school + "2", school + "4", school_1 + "1_south" } );
                nearby.push_back( { point_south_east, school + "4", school + "2", school_1 + "1_west" } );
            } else if( old == school + "2" ) {
                nearby.push_back( { point_south_west, school + "3", school + "5", school_1 + "2_north" } );
                nearby.push_back( { point_north_west, school + "5", school + "3", school_1 + "2_east" } );
                nearby.push_back( { point_north_east, school + "3", school + "5", school_1 + "2_south" } );
                nearby.push_back( { point_south_east, school + "5", school + "3", school_1 + "2_west" } );
            } else if( old == school + "3" ) {
                nearby.push_back( { point_south_east, school + "2", school + "6", school_1 + "3_north" } );
                nearby.push_back( { point_south_west, school + "6", school + "2", school_1 + "3_east" } );
                nearby.push_back( { point_north_west, school + "2", school + "6", school_1 + "3_south" } );
                nearby.push_back( { point_north_east, school + "6", school + "2", school_1 + "3_west" } );
            } else if( old == school + "4" ) {
                nearby.push_back( { point_south_west, school + "5", school + "7", school_1 + "4_north" } );
                nearby.push_back( { point_north_west, school + "7", school + "5", school_1 + "4_east" } );
                nearby.push_back( { point_north_east, school + "5", school + "7", school_1 + "4_south" } );
                nearby.push_back( { point_south_east, school + "7", school + "5", school_1 + "4_west" } );
            } else if( old == school + "5" ) {
                nearby.push_back( { point_south_west, school + "6", school + "8", school_1 + "5_north" } );
                nearby.push_back( { point_north_west, school + "8", school + "6", school_1 + "5_east" } );
                nearby.push_back( { point_north_east, school + "6", school + "8", school_1 + "5_south" } );
                nearby.push_back( { point_south_east, school + "8", school + "6", school_1 + "5_west" } );
            } else if( old == school + "6" ) {
                nearby.push_back( { point_south_east, school + "5", school + "9", school_1 + "6_north" } );
                nearby.push_back( { point_south_west, school + "9", school + "5", school_1 + "6_east" } );
                nearby.push_back( { point_north_west, school + "5", school + "9", school_1 + "6_south" } );
                nearby.push_back( { point_north_east, school + "9", school + "5", school_1 + "6_west" } );
            } else if( old == school + "7" ) {
                nearby.push_back( { point_north_west, school + "8", school + "4", school_1 + "7_north" } );
                nearby.push_back( { point_north_east, school + "4", school + "8", school_1 + "7_east" } );
                nearby.push_back( { point_south_east, school + "8", school + "4", school_1 + "7_south" } );
                nearby.push_back( { point_south_west, school + "4", school + "8", school_1 + "7_west" } );
            } else if( old == school + "8" ) {
                nearby.push_back( { point_north_west, school + "9", school + "5", school_1 + "8_north" } );
                nearby.push_back( { point_north_east, school + "5", school + "9", school_1 + "8_east" } );
                nearby.push_back( { point_south_east, school + "9", school + "5", school_1 + "8_south" } );
                nearby.push_back( { point_south_west, school + "5", school + "9", school_1 + "8_west" } );
            } else if( old == school + "9" ) {
                nearby.push_back( { point_north_east, school + "8", school + "6", school_1 + "9_north" } );
                nearby.push_back( { point_south_east, school + "6", school + "8", school_1 + "9_east" } );
                nearby.push_back( { point_south_west, school + "8", school + "6", school_1 + "9_south" } );
                nearby.push_back( { point_north_west, school + "6", school + "8", school_1 + "9_west" } );
            }

        } else if( old.compare( 0, 7, "prison_" ) == 0 ) {
            const std::string prison = "prison_";
            const std::string prison_1 = prison + "1_";
            if( old == "prison_b_entrance" ) {
                ter_set( pos, oter_id( "prison_1_b_2_north" ) );
            } else if( old == "prison_b" ) {
                if( pos.z < 0 ) {
                    nearby.push_back( { point_south_west, "prison_b_entrance", "prison_b",          "prison_1_b_1_north" } );
                    nearby.push_back( { point_south_east, "prison_b_entrance", "prison_b",          "prison_1_b_3_north" } );
                    nearby.push_back( { point( -2, 1 ), "prison_b", "prison_b",          "prison_1_b_4_north" } );
                    nearby.push_back( { point_north, "prison_b", "prison_b_entrance", "prison_1_b_5_north" } );
                    nearby.push_back( { point( 2, 1 ), "prison_b", "prison_b",          "prison_1_b_6_north" } );
                    nearby.push_back( { point( -2, -2 ), "prison_b", "prison_b",          "prison_1_b_7_north" } );
                    nearby.push_back( { point( 0, -2 ), "prison_b", "prison_b_entrance", "prison_1_b_8_north" } );
                    nearby.push_back( { point( 2, -2 ), "prison_b", "prison_b",          "prison_1_b_9_north" } );
                }
            } else if( old == prison + "1" ) {
                nearby.push_back( { point_south_west, prison + "2", prison + "4", prison_1 + "1_north" } );
                nearby.push_back( { point_north_west, prison + "4", prison + "2", prison_1 + "1_east" } );
                nearby.push_back( { point_north_east, prison + "2", prison + "4", prison_1 + "1_south" } );
                nearby.push_back( { point_south_east, prison + "4", prison + "2", prison_1 + "1_west" } );
            } else if( old == prison + "2" ) {
                nearby.push_back( { point_south_west, prison + "3", prison + "5", prison_1 + "2_north" } );
                nearby.push_back( { point_north_west, prison + "5", prison + "3", prison_1 + "2_east" } );
                nearby.push_back( { point_north_east, prison + "3", prison + "5", prison_1 + "2_south" } );
                nearby.push_back( { point_south_east, prison + "5", prison + "3", prison_1 + "2_west" } );
            } else if( old == prison + "3" ) {
                nearby.push_back( { point_south_east, prison + "2", prison + "6", prison_1 + "3_north" } );
                nearby.push_back( { point_south_west, prison + "6", prison + "2", prison_1 + "3_east" } );
                nearby.push_back( { point_north_west, prison + "2", prison + "6", prison_1 + "3_south" } );
                nearby.push_back( { point_north_east, prison + "6", prison + "2", prison_1 + "3_west" } );
            } else if( old == prison + "4" ) {
                nearby.push_back( { point_south_west, prison + "5", prison + "7", prison_1 + "4_north" } );
                nearby.push_back( { point_north_west, prison + "7", prison + "5", prison_1 + "4_east" } );
                nearby.push_back( { point_north_east, prison + "5", prison + "7", prison_1 + "4_south" } );
                nearby.push_back( { point_south_east, prison + "7", prison + "5", prison_1 + "4_west" } );
            } else if( old == prison + "5" ) {
                nearby.push_back( { point_south_west, prison + "6", prison + "8", prison_1 + "5_north" } );
                nearby.push_back( { point_north_west, prison + "8", prison + "6", prison_1 + "5_east" } );
                nearby.push_back( { point_north_east, prison + "6", prison + "8", prison_1 + "5_south" } );
                nearby.push_back( { point_south_east, prison + "8", prison + "6", prison_1 + "5_west" } );
            } else if( old == prison + "6" ) {
                nearby.push_back( { point_south_east, prison + "5", prison + "9", prison_1 + "6_north" } );
                nearby.push_back( { point_south_west, prison + "9", prison + "5", prison_1 + "6_east" } );
                nearby.push_back( { point_north_west, prison + "5", prison + "9", prison_1 + "6_south" } );
                nearby.push_back( { point_north_east, prison + "9", prison + "5", prison_1 + "6_west" } );
            } else if( old == prison + "7" ) {
                nearby.push_back( { point_north_west, prison + "8", prison + "4", prison_1 + "7_north" } );
                nearby.push_back( { point_north_east, prison + "4", prison + "8", prison_1 + "7_east" } );
                nearby.push_back( { point_south_east, prison + "8", prison + "4", prison_1 + "7_south" } );
                nearby.push_back( { point_south_west, prison + "4", prison + "8", prison_1 + "7_west" } );
            } else if( old == prison + "8" ) {
                nearby.push_back( { point_north_west, prison + "9", prison + "5", prison_1 + "8_north" } );
                nearby.push_back( { point_north_east, prison + "5", prison + "9", prison_1 + "8_east" } );
                nearby.push_back( { point_south_east, prison + "9", prison + "5", prison_1 + "8_south" } );
                nearby.push_back( { point_south_west, prison + "5", prison + "9", prison_1 + "8_west" } );
            } else if( old == prison + "9" ) {
                nearby.push_back( { point_north_east, prison + "8", prison + "6", prison_1 + "9_north" } );
                nearby.push_back( { point_south_east, prison + "6", prison + "8", prison_1 + "9_east" } );
                nearby.push_back( { point_south_west, prison + "8", prison + "6", prison_1 + "9_south" } );
                nearby.push_back( { point_north_west, prison + "6", prison + "8", prison_1 + "9_west" } );
            }

        } else if( old.compare( 0, 8, "hospital" ) == 0 ) {
            const std::string hospital = "hospital";
            const std::string hospital_entrance = "hospital_entrance";
            if( old == hospital_entrance ) {
                ter_set( pos, oter_id( hospital + "_2_north" ) );
            } else if( old == hospital ) {
                nearby.push_back( { point_south_west, hospital_entrance, hospital,          hospital + "_1_north" } );
                nearby.push_back( { point_south_east, hospital_entrance, hospital,          hospital + "_3_north" } );
                nearby.push_back( { point( -2, 1 ), hospital, hospital,          hospital + "_4_north" } );
                nearby.push_back( { point_north, hospital, hospital_entrance, hospital + "_5_north" } );
                nearby.push_back( { point( 2, 1 ), hospital, hospital,          hospital + "_6_north" } );
                nearby.push_back( { point( -2, -2 ), hospital, hospital,          hospital + "_7_north" } );
                nearby.push_back( { point( 0, -2 ), hospital, hospital_entrance, hospital + "_8_north" } );
                nearby.push_back( { point( 2, -2 ), hospital, hospital,          hospital + "_9_north" } );
            }

        } else if( old == "sewage_treatment" ) {
            ter_set( pos, oter_id( "sewage_treatment_0_1_0_north" ) );
            convert_unrelated_adjacent_tiles.push_back( { tripoint_north, "sewage_treatment_0_0_0_north" } );
            convert_unrelated_adjacent_tiles.push_back( { tripoint_east, "sewage_treatment_1_1_0_north" } );
            convert_unrelated_adjacent_tiles.push_back( { tripoint_north_east, "sewage_treatment_1_0_0_north" } );
            convert_unrelated_adjacent_tiles.push_back( { tripoint_above, "sewage_treatment_0_1_roof_north" } );
            convert_unrelated_adjacent_tiles.push_back( { tripoint_north + tripoint_above, "sewage_treatment_0_0_roof_north" } );
            convert_unrelated_adjacent_tiles.push_back( { tripoint_east + tripoint_above, "sewage_treatment_1_1_roof_north" } );
            convert_unrelated_adjacent_tiles.push_back( { tripoint_north_east + tripoint_above, "sewage_treatment_1_0_roof_north" } );
        } else if( old == "sewage_treatment_under" ) {
            const std::string base = "sewage_treatment_under";
            const std::string hub = "sewage_treatment_hub";
            nearby.push_back( { point_west, hub, base, "sewage_treatment_1_1_-1_north" } );
            nearby.push_back( { point_south_west, base, hub,  "sewage_treatment_0_0_-1_north" } );
            nearby.push_back( { point_south_west, base, base, "sewage_treatment_1_0_-1_north" } );
            // Fill empty space with something other than drivethrus.
            nearby.push_back( { point_east, hub, base, "empty_rock" } );
            nearby.push_back( { point_east, base, base, "empty_rock" } );
            nearby.push_back( { point_north_west, base, base, "empty_rock" } );
            nearby.push_back( { point_north, base, base, "empty_rock" } );
            nearby.push_back( { point_north_east, base, base, "empty_rock" } );
        } else if( old == "sewage_treatment_hub" ) {
            ter_set( pos, oter_id( "sewage_treatment_0_1_-1_north" ) );
            convert_unrelated_adjacent_tiles.push_back( { tripoint( 2, 0, 0 ), "sewage_treatment_2_1_-1_north" } );
            convert_unrelated_adjacent_tiles.push_back( { tripoint( 2, -1, 0 ), "sewage_treatment_2_0_-1_north" } );
        } else if( old == "cathedral_1_entrance" ) {
            const std::string base = "cathedral_1_";
            const std::string other = "cathedral_1";
            nearby.push_back( { point_north_east, other, other, base + "SW_north" } );
            nearby.push_back( { point_south_west, other, other, base + "SW_south" } );
            nearby.push_back( { point_south_east, other, other, base + "SW_east" } );
            nearby.push_back( { point_north_west, other, other, base + "SW_west" } );

        } else if( old == "cathedral_1" ) {
            const std::string base = "cathedral_1_";
            const std::string entr = "cathedral_1_entrance";
            nearby.push_back( { point_south_east, old, entr, base + "NW_north" } );
            nearby.push_back( { point_north_west, old, entr, base + "NW_south" } );
            nearby.push_back( { point_south_west, entr, old, base + "NW_east" } );
            nearby.push_back( { point_north_east, entr, old, base + "NW_west" } );
            nearby.push_back( { point_south_west, old, old, base + "NE_north" } );
            nearby.push_back( { point_north_east, old, old, base + "NE_south" } );
            nearby.push_back( { point_north_west, old, old, base + "NE_east" } );
            nearby.push_back( { point_south_east, old, old, base + "NE_west" } );
            nearby.push_back( { point_north_west, entr, old, base + "SE_north" } );
            nearby.push_back( { point_south_east, entr, old, base + "SE_south" } );
            nearby.push_back( { point_north_east, old, entr, base + "SE_east" } );
            nearby.push_back( { point_south_west, old, entr, base + "SE_west" } );

        } else if( old == "cathedral_b_entrance" ) {
            const std::string base = "cathedral_b_";
            const std::string other = "cathedral_b";
            nearby.push_back( { point_north_east, other, other, base + "SW_north" } );
            nearby.push_back( { point_south_west, other, other, base + "SW_south" } );
            nearby.push_back( { point_south_east, other, other, base + "SW_east" } );
            nearby.push_back( { point_north_west, other, other, base + "SW_west" } );

        } else if( old == "cathedral_b" ) {
            const std::string base = "cathedral_b_";
            const std::string entr = "cathedral_b_entrance";
            nearby.push_back( { point_south_east, old, entr, base + "NW_north" } );
            nearby.push_back( { point_north_west, old, entr, base + "NW_south" } );
            nearby.push_back( { point_south_west, entr, old, base + "NW_east" } );
            nearby.push_back( { point_north_east, entr, old, base + "NW_west" } );
            nearby.push_back( { point_south_west, old, old, base + "NE_north" } );
            nearby.push_back( { point_north_east, old, old, base + "NE_south" } );
            nearby.push_back( { point_north_west, old, old, base + "NE_east" } );
            nearby.push_back( { point_south_east, old, old, base + "NE_west" } );
            nearby.push_back( { point_north_west, entr, old, base + "SE_north" } );
            nearby.push_back( { point_south_east, entr, old, base + "SE_south" } );
            nearby.push_back( { point_north_east, old, entr, base + "SE_east" } );
            nearby.push_back( { point_south_west, old, entr, base + "SE_west" } );

        } else if( old.compare( 0, 14, "hotel_tower_1_" ) == 0 ) {
            const std::string hotel = "hotel_tower_1_";
            if( old == hotel + "1" ) {
                nearby.push_back( { point_south_west, hotel + "2", hotel + "4", hotel + "1_north" } );
                nearby.push_back( { point_north_west, hotel + "4", hotel + "2", hotel + "1_east" } );
                nearby.push_back( { point_north_east, hotel + "2", hotel + "4", hotel + "1_south" } );
                nearby.push_back( { point_south_east, hotel + "4", hotel + "2", hotel + "1_west" } );
            } else if( old == hotel + "2" ) {
                nearby.push_back( { point_south_west, hotel + "3", hotel + "5", hotel + "2_north" } );
                nearby.push_back( { point_north_west, hotel + "5", hotel + "3", hotel + "2_east" } );
                nearby.push_back( { point_north_east, hotel + "3", hotel + "5", hotel + "2_south" } );
                nearby.push_back( { point_south_east, hotel + "5", hotel + "3", hotel + "2_west" } );
            } else if( old == hotel + "3" ) {
                nearby.push_back( { point_south_east, hotel + "2", hotel + "6", hotel + "3_north" } );
                nearby.push_back( { point_south_west, hotel + "6", hotel + "2", hotel + "3_east" } );
                nearby.push_back( { point_north_west, hotel + "2", hotel + "6", hotel + "3_south" } );
                nearby.push_back( { point_north_east, hotel + "6", hotel + "2", hotel + "3_west" } );
            } else if( old == hotel + "4" ) {
                nearby.push_back( { point_south_west, hotel + "5", hotel + "7", hotel + "4_north" } );
                nearby.push_back( { point_north_west, hotel + "7", hotel + "5", hotel + "4_east" } );
                nearby.push_back( { point_north_east, hotel + "5", hotel + "7", hotel + "4_south" } );
                nearby.push_back( { point_south_east, hotel + "7", hotel + "5", hotel + "4_west" } );
            } else if( old == hotel + "5" ) {
                nearby.push_back( { point_south_west, hotel + "6", hotel + "8", hotel + "5_north" } );
                nearby.push_back( { point_north_west, hotel + "8", hotel + "6", hotel + "5_east" } );
                nearby.push_back( { point_north_east, hotel + "6", hotel + "8", hotel + "5_south" } );
                nearby.push_back( { point_south_east, hotel + "8", hotel + "6", hotel + "5_west" } );
            } else if( old == hotel + "6" ) {
                nearby.push_back( { point_south_east, hotel + "5", hotel + "9", hotel + "6_north" } );
                nearby.push_back( { point_south_west, hotel + "9", hotel + "5", hotel + "6_east" } );
                nearby.push_back( { point_north_west, hotel + "5", hotel + "9", hotel + "6_south" } );
                nearby.push_back( { point_north_east, hotel + "9", hotel + "5", hotel + "6_west" } );
            } else if( old == hotel + "7" ) {
                nearby.push_back( { point_north_west, hotel + "8", hotel + "4", hotel + "7_north" } );
                nearby.push_back( { point_north_east, hotel + "4", hotel + "8", hotel + "7_east" } );
                nearby.push_back( { point_south_east, hotel + "8", hotel + "4", hotel + "7_south" } );
                nearby.push_back( { point_south_west, hotel + "4", hotel + "8", hotel + "7_west" } );
            } else if( old == hotel + "8" ) {
                nearby.push_back( { point_north_west, hotel + "9", hotel + "5", hotel + "8_north" } );
                nearby.push_back( { point_north_east, hotel + "5", hotel + "9", hotel + "8_east" } );
                nearby.push_back( { point_south_east, hotel + "9", hotel + "5", hotel + "8_south" } );
                nearby.push_back( { point_south_west, hotel + "5", hotel + "9", hotel + "8_west" } );
            } else if( old == hotel + "9" ) {
                nearby.push_back( { point_north_east, hotel + "8", hotel + "6", hotel + "9_north" } );
                nearby.push_back( { point_south_east, hotel + "6", hotel + "8", hotel + "9_east" } );
                nearby.push_back( { point_south_west, hotel + "8", hotel + "6", hotel + "9_south" } );
                nearby.push_back( { point_north_west, hotel + "6", hotel + "8", hotel + "9_west" } );
            }

        } else if( old.compare( 0, 14, "hotel_tower_b_" ) == 0 ) {
            const std::string hotelb = "hotel_tower_b_";
            if( old == hotelb + "1" ) {
                nearby.push_back( { point_west, hotelb + "2", hotelb + "1", hotelb + "1_north" } );
                nearby.push_back( { point_north, hotelb + "1", hotelb + "2", hotelb + "1_east" } );
                nearby.push_back( { point_east, hotelb + "2", hotelb + "1", hotelb + "1_south" } );
                nearby.push_back( { point_south, hotelb + "1", hotelb + "2", hotelb + "1_west" } );
            } else if( old == hotelb + "2" ) {
                nearby.push_back( { point_west, hotelb + "3", hotelb + "2", hotelb + "2_north" } );
                nearby.push_back( { point_north, hotelb + "2", hotelb + "3", hotelb + "2_east" } );
                nearby.push_back( { point_east, hotelb + "3", hotelb + "2", hotelb + "2_south" } );
                nearby.push_back( { point_south, hotelb + "2", hotelb + "3", hotelb + "2_west" } );
            } else if( old == hotelb + "3" ) {
                nearby.push_back( { point_east, hotelb + "2", hotelb + "3", hotelb + "3_north" } );
                nearby.push_back( { point_south, hotelb + "3", hotelb + "2", hotelb + "3_east" } );
                nearby.push_back( { point_west, hotelb + "2", hotelb + "3", hotelb + "3_south" } );
                nearby.push_back( { point_north, hotelb + "3", hotelb + "2", hotelb + "3_west" } );
            }
        } else if( old == "bunker" ) {
            if( pos.z < 0 ) {
                ter_set( pos, oter_id( "bunker_basement" ) );
            } else if( is_ot_match( "road", ter( pos + point_east ), ot_match_type::type ) ) {
                ter_set( pos, oter_id( "bunker_west" ) );
            } else if( is_ot_match( "road", ter( pos + point_west ), ot_match_type::type ) ) {
                ter_set( pos, oter_id( "bunker_east" ) );
            } else if( is_ot_match( "road", ter( pos + point_south ), ot_match_type::type ) ) {
                ter_set( pos, oter_id( "bunker_north" ) );
            } else {
                ter_set( pos, oter_id( "bunker_south" ) );
            }

        } else if( old == "farm" ) {
            ter_set( pos, oter_id( "farm_2_north" ) );

        } else if( old == "farm_field" ) {
            nearby.push_back( { point_south_west, "farm", "farm_field", "farm_1_north" } );
            nearby.push_back( { point_south_east, "farm", "farm_field", "farm_3_north" } );
            nearby.push_back( { point( -2, 1 ), "farm_field", "farm_field", "farm_4_north" } );
            nearby.push_back( { point_north, "farm_field", "farm",       "farm_5_north" } );
            nearby.push_back( { point( 2, 1 ), "farm_field", "farm_field", "farm_6_north" } );
            nearby.push_back( { point( -2, -2 ), "farm_field", "farm_field", "farm_7_north" } );
            nearby.push_back( { point( 0, -2 ), "farm_field", "farm",       "farm_8_north" } );
            nearby.push_back( { point( 2, -2 ), "farm_field", "farm_field", "farm_9_north" } );
        } else if( old.compare( 0, 7, "mansion" ) == 0 ) {
            if( old == "mansion_entrance" ) {
                ter_set( pos, oter_id( "mansion_e1_north" ) );
            } else if( old == "mansion" ) {
                nearby.push_back( { point_south_west, "mansion_entrance", "mansion",          "mansion_c1_east" } );
                nearby.push_back( { point_south_east, "mansion_entrance", "mansion",          "mansion_c3_north" } );
                nearby.push_back( { point( -2, 1 ), "mansion", "mansion",          "mansion_t2_west" } );
                nearby.push_back( { point_north, "mansion", "mansion_entrance", "mansion_+4_north" } );
                nearby.push_back( { point( 2, 1 ), "mansion", "mansion",          "mansion_t4_east" } );
                nearby.push_back( { point( -2, -2 ), "mansion", "mansion",          "mansion_c4_south" } );
                nearby.push_back( { point( 0, -2 ), "mansion", "mansion_entrance", "mansion_t2_north" } );
                nearby.push_back( { point( 2, -2 ), "mansion", "mansion",          "mansion_c2_west" } );
            }

            // Migrate terrains with NO_ROTATE flag to rotatable
        } else if( old.compare( 0, 4, "lmoe" ) == 0 ||
                   old.compare( 0, 5, "cabin" ) == 0 ||
                   old.compare( 0, 5, "pond_" ) == 0 ||
                   old.compare( 0, 6, "bandit" ) == 0 ||
                   old.compare( 0, 7, "shelter" ) == 0 ||
                   old.compare( 0, 8, "campsite" ) == 0 ||
                   old.compare( 0, 9, "pwr_large" ) == 0 ||
                   old.compare( 0, 9, "shipwreck" ) == 0 ||
                   old.compare( 0, 9, "robofachq" ) == 0 ||
                   old.compare( 0, 10, "ranch_camp" ) == 0 ||
                   old.compare( 0, 11, "hdwr_large_" ) == 0 ||
                   old.compare( 0, 14, "loffice_tower_" ) == 0 ||
                   old.compare( 0, 17, "cemetery_4square_" ) == 0 ) {
            ter_set( pos, oter_id( old + "_north" ) );

        } else if( old == "hunter_shack" ||
                   old == "magic_basement" ||
                   old == "basement_bionic" ||
                   old == "outpost" ||
                   old == "park" ||
                   old == "pool" ||
                   old == "pwr_sub_s" ||
                   old == "radio_tower" ||
                   old == "sai" ||
                   old == "toxic_dump" ||
                   old == "orchard_stall" ||
                   old == "orchard_tree_apple" ||
                   old == "orchard_processing" ||
                   old == "dairy_farm_NW" ||
                   old == "dairy_farm_NE" ||
                   old == "dairy_farm_SW" ||
                   old == "dairy_farm_SE" ) {
            ter_set( pos, oter_id( old + "_north" ) );

        } else if( old == "megastore_entrance" ) {
            const std::string megastore = "megastore";
            const auto ter_test_n = needs_conversion.find( pos + point( 0, -2 ) );
            const auto ter_test_s = needs_conversion.find( pos + point( 0,  2 ) );
            const auto ter_test_e = needs_conversion.find( pos + point( 2,  0 ) );
            const auto ter_test_w = needs_conversion.find( pos + point( -2,  0 ) );
            //North
            if( ter_test_n != needs_conversion.end() && ter_test_n->second == megastore ) {
                ter_set( pos + point_north + point_north_west, oter_id( megastore + "_0_0_0_north" ) );
                ter_set( pos + point_north + point_north, oter_id( megastore + "_1_0_0_north" ) );
                ter_set( pos + point_north + point_north_east, oter_id( megastore + "_2_0_0_north" ) );
                ter_set( pos + point_north_west, oter_id( megastore + "_0_1_0_north" ) );
                ter_set( pos + point_north, oter_id( megastore + "_1_1_0_north" ) );
                ter_set( pos + point_north_east, oter_id( megastore + "_2_1_0_north" ) );
                ter_set( pos + point_west, oter_id( megastore + "_0_2_0_north" ) );
                ter_set( pos + point_zero, oter_id( megastore + "_1_2_0_north" ) );
                ter_set( pos + point_east, oter_id( megastore + "_2_2_0_north" ) );
            } else if( ter_test_s != needs_conversion.end() && ter_test_s->second == megastore ) {
                ter_set( pos + point_west, oter_id( megastore + "_2_2_0_south" ) );
                ter_set( pos + point_zero, oter_id( megastore + "_1_2_0_south" ) );
                ter_set( pos + point_east, oter_id( megastore + "_0_2_0_south" ) );
                ter_set( pos + point_south_west, oter_id( megastore + "_2_1_0_south" ) );
                ter_set( pos + point_south, oter_id( megastore + "_1_1_0_south" ) );
                ter_set( pos + point_south_east, oter_id( megastore + "_0_1_0_south" ) );
                ter_set( pos + point_south + point_south_west, oter_id( megastore + "_2_0_0_south" ) );
                ter_set( pos + point_south + point_south, oter_id( megastore + "_1_0_0_south" ) );
                ter_set( pos + point_south + point_south_east, oter_id( megastore + "_0_0_0_south" ) );
            } else if( ter_test_e != needs_conversion.end() && ter_test_e->second == megastore ) {
                ter_set( pos + point_north, oter_id( megastore + "_0_2_0_east" ) );
                ter_set( pos + point_north_east, oter_id( megastore + "_0_1_0_east" ) );
                ter_set( pos + point_east + point_north_east, oter_id( megastore + "_0_0_0_east" ) );
                ter_set( pos + point_zero, oter_id( megastore + "_1_2_0_east" ) );
                ter_set( pos + point_east, oter_id( megastore + "_1_1_0_east" ) );
                ter_set( pos + point_east + point_east, oter_id( megastore + "_1_0_0_east" ) );
                ter_set( pos + point_south, oter_id( megastore + "_2_2_0_east" ) );
                ter_set( pos + point_south_east, oter_id( megastore + "_2_1_0_east" ) );
                ter_set( pos + point_east + point_south_east, oter_id( megastore + "_2_0_0_east" ) );
            } else if( ter_test_w != needs_conversion.end() && ter_test_w->second == megastore ) {
                ter_set( pos + point_west + point_north_west, oter_id( megastore + "_2_0_0_west" ) );
                ter_set( pos + point_north_west, oter_id( megastore + "_2_1_0_west" ) );
                ter_set( pos + point_north, oter_id( megastore + "_2_2_0_west" ) );
                ter_set( pos + point_west + point_west, oter_id( megastore + "_1_0_0_west" ) );
                ter_set( pos + point_west, oter_id( megastore + "_1_1_0_west" ) );
                ter_set( pos + point_zero, oter_id( megastore + "_1_2_0_west" ) );
                ter_set( pos + point_west + point_south_west, oter_id( megastore + "_0_0_0_west" ) );
                ter_set( pos + point_south_west, oter_id( megastore + "_0_1_0_west" ) );
                ter_set( pos + point_south, oter_id( megastore + "_0_2_0_west" ) );
            } else {
                debugmsg( "Malformed Megastore" );
            }

        } else if( old.compare( 0, 7, "haz_sar" ) == 0 ) {
            if( old == "haz_sar_entrance" || old == "haz_sar_entrance_north" ) {
                ter_set( pos, oter_id( "haz_sar_1_1_north" ) );
                ter_set( pos + point_west, oter_id( "haz_sar_1_2_north" ) );
                ter_set( pos + point_south, oter_id( "haz_sar_1_3_north" ) );
                ter_set( pos + point_south_west, oter_id( "haz_sar_1_4_north" ) );
            } else if( old == "haz_sar_entrance_south" ) {
                ter_set( pos, oter_id( "haz_sar_1_1_south" ) );
                ter_set( pos + point_north, oter_id( "haz_sar_1_2_south" ) );
                ter_set( pos + point_west, oter_id( "haz_sar_1_3_south" ) );
                ter_set( pos + point_north_west, oter_id( "haz_sar_1_4_south" ) );
            } else if( old == "haz_sar_entrance_east" ) {
                ter_set( pos, oter_id( "haz_sar_1_1_east" ) );
                ter_set( pos + point_north, oter_id( "haz_sar_1_2_east" ) );
                ter_set( pos + point_west, oter_id( "haz_sar_1_3_east" ) );
                ter_set( pos + point_north_west, oter_id( "haz_sar_1_4_east" ) );
            } else if( old == "haz_sar_entrance_west" ) {
                ter_set( pos, oter_id( "haz_sar_1_1_west" ) );
                ter_set( pos + point_south, oter_id( "haz_sar_1_2_west" ) );
                ter_set( pos + point_east, oter_id( "haz_sar_1_3_west" ) );
                ter_set( pos + point_south_east, oter_id( "haz_sar_1_4_west" ) );
            }

            if( old == "haz_sar_entrance_b1" || old == "haz_sar_entrance_b1_north" ) {
                ter_set( pos, oter_id( "haz_sar_b_1_north" ) );
                ter_set( pos + point_west, oter_id( "haz_sar_b_2_north" ) );
                ter_set( pos + point_south, oter_id( "haz_sar_b_3_north" ) );
                ter_set( pos + point_south_west, oter_id( "haz_sar_b_4_north" ) );
            } else if( old == "haz_sar_entrance_b1_south" ) {
                ter_set( pos, oter_id( "haz_sar_b_1_south" ) );
                ter_set( pos + point_north, oter_id( "haz_sar_b_2_south" ) );
                ter_set( pos + point_west, oter_id( "haz_sar_b_3_south" ) );
                ter_set( pos + point_north_west, oter_id( "haz_sar_b_4_south" ) );
            } else if( old == "haz_sar_entrance_b1_east" ) {
                ter_set( pos, oter_id( "haz_sar_b_1_east" ) );
                ter_set( pos + point_north, oter_id( "haz_sar_b_2_east" ) );
                ter_set( pos + point_west, oter_id( "haz_sar_b_3_east" ) );
                ter_set( pos + point_north_west, oter_id( "haz_sar_b_4_east" ) );
            } else if( old == "haz_sar_entrance_b1_west" ) {
                ter_set( pos, oter_id( "haz_sar_b_1_west" ) );
                ter_set( pos + point_south, oter_id( "haz_sar_b_2_west" ) );
                ter_set( pos + point_east, oter_id( "haz_sar_b_3_west" ) );
                ter_set( pos + point_south_east, oter_id( "haz_sar_b_4_west" ) );
            }

        } else if( old == "house_base_north" || old == "house_north" ||
                   old == "house_base" || old == "house" ) {
            ter_set( pos, oter_id( "house_w_1_north" ) );
        } else if( old == "house_base_south" || old == "house_south" ) {
            ter_set( pos, oter_id( "house_w_1_south" ) );
        } else if( old == "house_base_east" || old == "house_east" ) {
            ter_set( pos, oter_id( "house_w_1_east" ) );
        } else if( old == "house_base_west" || old == "house_west" ) {
            ter_set( pos, oter_id( "house_w_1_west" ) );
        } else if( old == "rural_house" || old == "rural_house_north" ) {
            ter_set( pos, oter_id( "rural_house1_north" ) );
        } else if( old == "rural_house_south" ) {
            ter_set( pos, oter_id( "rural_house1_south" ) );
        } else if( old == "rural_house_east" ) {
            ter_set( pos, oter_id( "rural_house1_east" ) );
        } else if( old == "rural_house_west" ) {
            ter_set( pos, oter_id( "rural_house1_west" ) );
        } else if( old.compare( 0, 10, "mass_grave" ) == 0 ) {
            ter_set( pos, oter_id( "field" ) );
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
}

void overmap::load_monster_groups( JsonIn &jsin )
{
    jsin.start_array();
    while( !jsin.end_array() ) {
        jsin.start_array();

        mongroup new_group;
        new_group.deserialize( jsin );

        jsin.start_array();
        tripoint temp;
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
            std::unordered_map<tripoint, std::string> needs_conversion;
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
                                    needs_conversion.emplace( tripoint( p, j, z - OVERMAP_DEPTH ),
                                                              tmp_ter );
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
                        jsin.read( new_city.pos.x );
                    } else if( city_member_name == "y" ) {
                        jsin.read( new_city.pos.y );
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
            std::vector<tripoint> &roads_out = connections_out[string_id<overmap_connection>( "local_road" )];
            jsin.start_array();
            while( !jsin.end_array() ) {
                jsin.start_object();
                tripoint new_road;
                while( !jsin.end_object() ) {
                    std::string road_member_name = jsin.get_member_name();
                    if( road_member_name == "x" ) {
                        jsin.read( new_road.x );
                    } else if( road_member_name == "y" ) {
                        jsin.read( new_road.y );
                    }
                }
                roads_out.push_back( new_road );
            }
        } else if( name == "radios" ) {
            jsin.start_array();
            while( !jsin.end_array() ) {
                jsin.start_object();
                radio_tower new_radio( point_min );
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
                        jsin.read( new_radio.pos.x );
                    } else if( radio_member_name == "y" ) {
                        jsin.read( new_radio.pos.y );
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
                tripoint monster_location;
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
                        jsin.read( new_tracker.p.x );
                    } else if( tracker_member_name == "y" ) {
                        jsin.read( new_tracker.p.y );
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
                tripoint pos;
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
                                        tripoint p;
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
                    jsin.read( tmp.p.x );
                    jsin.read( tmp.p.y );
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
                    jsin.read( tmp.p.x );
                    jsin.read( tmp.p.y );
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
        for( auto &i : layer[z].notes ) {
            json.start_array();
            json.write( i.p.x );
            json.write( i.p.y );
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
        for( auto &i : layer[z].extras ) {
            json.start_array();
            json.write( i.p.x );
            json.write( i.p.y );
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
    std::unordered_map<mongroup, std::list<tripoint>, mongroup_hash, mongroup_bin_eq> binned_groups;
    binned_groups.reserve( zg.size() );
    for( const auto &pos_group : zg ) {
        // Each group in bin adds only position
        // so that 100 identical groups are 1 group data and 100 tripoints
        std::list<tripoint> &positions = binned_groups[pos_group.second];
        positions.emplace_back( pos_group.first );
    }

    for( auto &group_bin : binned_groups ) {
        jout.start_array();
        // Zero the bin position so that it isn't serialized
        // The position is stored separately, in the list
        // TODO: Do it without the copy
        mongroup saved_group = group_bin.first;
        saved_group.pos = tripoint_zero;
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
        auto &layer_terrain = layer[z].terrain;
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
    for( auto &i : cities ) {
        json.start_object();
        json.member( "name", i.name );
        json.member( "x", i.pos.x );
        json.member( "y", i.pos.y );
        json.member( "size", i.size );
        json.end_object();
    }
    json.end_array();
    fout << std::endl;

    json.member( "connections_out", connections_out );
    fout << std::endl;

    json.member( "radios" );
    json.start_array();
    for( auto &i : radios ) {
        json.start_object();
        json.member( "x", i.pos.x );
        json.member( "y", i.pos.y );
        json.member( "strength", i.strength );
        json.member( "type", radio_type_names[i.type] );
        json.member( "message", i.message );
        json.end_object();
    }
    json.end_array();
    fout << std::endl;

    json.member( "monster_map" );
    json.start_array();
    for( auto &i : monster_map ) {
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
        json.member( "x", i.second.p.x );
        json.member( "y", i.second.p.y );
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
    for( auto &i : npcs ) {
        json.write( *i );
    }
    json.end_array();
    fout << std::endl;

    json.member( "camps" );
    json.start_array();
    for( auto &i : camps ) {
        json.write( i );
    }
    json.end_array();
    fout << std::endl;

    // Condense the overmap special placements so that all placements of a given special
    // are grouped under a single key for that special.
    std::map<overmap_special_id, std::vector<tripoint>> condensed_overmap_special_placements;
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
        for( const tripoint &pos : placement.second ) {
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
    archive.io( "pos", pos, tripoint_zero );
    archive.io( "radius", radius, 1u );
    archive.io( "population", population, 1u );
    archive.io( "diffuse", diffuse, false );
    archive.io( "dying", dying, false );
    archive.io( "horde", horde, false );
    archive.io( "target", target, tripoint_zero );
    archive.io( "interest", interest, 0 );
    archive.io( "horde_behaviour", horde_behaviour, io::empty_default_tag() );
    archive.io( "monsters", monsters, io::empty_default_tag() );
}

void mongroup::deserialize( JsonIn &data )
{
    io::JsonObjectInputArchive archive( data );
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
    if( savegame_loading_version < 11 ) {
        std::unique_ptr<static_popup>popup = std::make_unique<static_popup>();
        popup->message(
            _( "Cannot find loader for save data in old version %d, attempting to load as current version %d." ),
            savegame_loading_version, savegame_version );
        ui_manager::redraw();
        refresh_display();
    }
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
                JsonObject w = jsin.get_object();
                w.read( "lightning", weather.lightning_active );
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
        json.end_object();

        json.end_object();
    } catch( const JsonError &e ) {
        debugmsg( "error saving to %s: %s", SAVE_MASTER, e.c_str() );
    }
}

void faction_manager::serialize( JsonOut &jsout ) const
{
    std::vector<faction> local_facs;
    for( auto &elem : factions ) {
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
