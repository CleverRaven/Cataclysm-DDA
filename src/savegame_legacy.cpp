#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "debug.h"
// for legacy classdata loaders
#include "item.h"
#include "calendar.h"
#include "itype.h"
#include "json.h"
#include "mongroup.h"
#include "npc.h"
#include "options.h"
#include "overmap.h"
#include "player_activity.h"

namespace std
{
template <>
struct hash<talk_topic_enum> {
    // Operator overload required by std API.
    std::size_t operator()( const talk_topic_enum &k ) const {
        return k; // the most trivial hash of them all
    }
};
}

std::string convert_talk_topic( talk_topic_enum const old_value )
{
    static const std::unordered_map<talk_topic_enum, std::string> talk_topic_enum_mapping = { {
            // This macro creates the appropriate new names (as string) for each enum value, so one does not
            // have to repeat so much (e.g. 'WRAP(TALK_ARSONIST)' instead of '{ TALK_ARSONIST, "TALK_ARSONIST" }')
            // It also ensures that each name is exactly as the name of the enum value.
#define WRAP(value) { value, #value }
            WRAP( TALK_NONE ),
            WRAP( TALK_DONE ),
            WRAP( TALK_GUARD ),
            WRAP( TALK_MISSION_LIST ),
            WRAP( TALK_MISSION_LIST_ASSIGNED ),
            WRAP( TALK_MISSION_DESCRIBE ),
            WRAP( TALK_MISSION_OFFER ),
            WRAP( TALK_MISSION_ACCEPTED ),
            WRAP( TALK_MISSION_REJECTED ),
            WRAP( TALK_MISSION_ADVICE ),
            WRAP( TALK_MISSION_INQUIRE ),
            WRAP( TALK_MISSION_SUCCESS ),
            WRAP( TALK_MISSION_SUCCESS_LIE ),
            WRAP( TALK_MISSION_FAILURE ),
            WRAP( TALK_MISSION_REWARD ),
            WRAP( TALK_EVAC_MERCHANT ),
            WRAP( TALK_EVAC_MERCHANT_NEW ),
            WRAP( TALK_EVAC_MERCHANT_PLANS ),
            WRAP( TALK_EVAC_MERCHANT_PLANS2 ),
            WRAP( TALK_EVAC_MERCHANT_PLANS3 ),
            WRAP( TALK_EVAC_MERCHANT_WORLD ),
            WRAP( TALK_EVAC_MERCHANT_HORDES ),
            WRAP( TALK_EVAC_MERCHANT_PRIME_LOOT ),
            WRAP( TALK_EVAC_MERCHANT_ASK_JOIN ),
            WRAP( TALK_EVAC_MERCHANT_NO ),
            WRAP( TALK_EVAC_MERCHANT_HELL_NO ),
            WRAP( TALK_FREE_MERCHANT_STOCKS ),
            WRAP( TALK_FREE_MERCHANT_STOCKS_NEW ),
            WRAP( TALK_FREE_MERCHANT_STOCKS_WHY ),
            WRAP( TALK_FREE_MERCHANT_STOCKS_ALL ),
            WRAP( TALK_FREE_MERCHANT_STOCKS_JERKY ),
            WRAP( TALK_FREE_MERCHANT_STOCKS_CORNMEAL ),
            WRAP( TALK_FREE_MERCHANT_STOCKS_FLOUR ),
            WRAP( TALK_FREE_MERCHANT_STOCKS_SUGAR ),
            WRAP( TALK_FREE_MERCHANT_STOCKS_WINE ),
            WRAP( TALK_FREE_MERCHANT_STOCKS_BEER ),
            WRAP( TALK_FREE_MERCHANT_STOCKS_SMMEAT ),
            WRAP( TALK_FREE_MERCHANT_STOCKS_SMFISH ),
            WRAP( TALK_FREE_MERCHANT_STOCKS_OIL ),
            WRAP( TALK_FREE_MERCHANT_STOCKS_DELIVERED ),
            WRAP( TALK_EVAC_GUARD1 ),
            WRAP( TALK_EVAC_GUARD1_PLACE ),
            WRAP( TALK_EVAC_GUARD1_GOVERNMENT ),
            WRAP( TALK_EVAC_GUARD1_TRADE ),
            WRAP( TALK_EVAC_GUARD1_JOIN ),
            WRAP( TALK_EVAC_GUARD1_JOIN2 ),
            WRAP( TALK_EVAC_GUARD1_JOIN3 ),
            WRAP( TALK_EVAC_GUARD1_ATTITUDE ),
            WRAP( TALK_EVAC_GUARD1_JOB ),
            WRAP( TALK_EVAC_GUARD1_OLDGUARD ),
            WRAP( TALK_EVAC_GUARD1_BYE ),
            WRAP( TALK_EVAC_GUARD2 ),
            WRAP( TALK_EVAC_GUARD2_NEW ),
            WRAP( TALK_EVAC_GUARD2_RULES ),
            WRAP( TALK_EVAC_GUARD2_RULES_BASEMENT ),
            WRAP( TALK_EVAC_GUARD2_WHO ),
            WRAP( TALK_EVAC_GUARD2_TRADE ),
            WRAP( TALK_EVAC_GUARD3 ),
            WRAP( TALK_EVAC_GUARD3_NEW ),
            WRAP( TALK_EVAC_GUARD3_RULES ),
            WRAP( TALK_EVAC_GUARD3_HIDE1 ),
            WRAP( TALK_EVAC_GUARD3_HIDE2 ),
            WRAP( TALK_EVAC_GUARD3_WASTE ),
            WRAP( TALK_EVAC_GUARD3_DEAD ),
            WRAP( TALK_EVAC_GUARD3_HOSTILE ),
            WRAP( TALK_EVAC_GUARD3_INSULT ),
            WRAP( TALK_EVAC_HUNTER ),
            WRAP( TALK_EVAC_HUNTER_SMELL ),
            WRAP( TALK_EVAC_HUNTER_DO ),
            WRAP( TALK_EVAC_HUNTER_LIFE ),
            WRAP( TALK_EVAC_HUNTER_HUNT ),
            WRAP( TALK_EVAC_HUNTER_SALE ),
            WRAP( TALK_EVAC_HUNTER_ADVICE ),
            WRAP( TALK_EVAC_HUNTER_BYE ),
            WRAP( TALK_OLD_GUARD_REP ),
            WRAP( TALK_OLD_GUARD_REP_NEW ),
            WRAP( TALK_OLD_GUARD_REP_NEW_DOING ),
            WRAP( TALK_OLD_GUARD_REP_NEW_DOWNSIDE ),
            WRAP( TALK_OLD_GUARD_REP_WORLD ),
            WRAP( TALK_OLD_GUARD_REP_WORLD_2NDFLEET ),
            WRAP( TALK_OLD_GUARD_REP_WORLD_FOOTHOLDS ),
            WRAP( TALK_OLD_GUARD_REP_ASK_JOIN ),
            WRAP( TALK_ARSONIST ),
            WRAP( TALK_ARSONIST_NEW ),
            WRAP( TALK_ARSONIST_DOING ),
            WRAP( TALK_ARSONIST_DOING_REBAR ),
            WRAP( TALK_ARSONIST_WORLD ),
            WRAP( TALK_ARSONIST_WORLD_OPTIMISTIC ),
            WRAP( TALK_ARSONIST_JOIN ),
            WRAP( TALK_ARSONIST_MUTATION ),
            WRAP( TALK_ARSONIST_MUTATION_INSULT ),
            WRAP( TALK_SCAVENGER_MERC ),
            WRAP( TALK_SCAVENGER_MERC_NEW ),
            WRAP( TALK_SCAVENGER_MERC_TIPS ),
            WRAP( TALK_SCAVENGER_MERC_HIRE ),
            WRAP( TALK_SCAVENGER_MERC_HIRE_SUCCESS ),
            WRAP( TALK_SHELTER ),
            WRAP( TALK_SHELTER_PLANS ),
            WRAP( TALK_SHARE_EQUIPMENT ),
            WRAP( TALK_GIVE_EQUIPMENT ),
            WRAP( TALK_DENY_EQUIPMENT ),
            WRAP( TALK_TRAIN ),
            WRAP( TALK_TRAIN_START ),
            WRAP( TALK_TRAIN_FORCE ),
            WRAP( TALK_SUGGEST_FOLLOW ),
            WRAP( TALK_AGREE_FOLLOW ),
            WRAP( TALK_DENY_FOLLOW ),
            WRAP( TALK_SHOPKEEP ),
            WRAP( TALK_LEADER ),
            WRAP( TALK_LEAVE ),
            WRAP( TALK_PLAYER_LEADS ),
            WRAP( TALK_LEADER_STAYS ),
            WRAP( TALK_HOW_MUCH_FURTHER ),
            WRAP( TALK_FRIEND ),
            WRAP( TALK_FRIEND_GUARD ),
            WRAP( TALK_DENY_GUARD ),
            WRAP( TALK_DENY_TRAIN ),
            WRAP( TALK_DENY_PERSONAL ),
            WRAP( TALK_FRIEND_UNCOMFORTABLE ),
            WRAP( TALK_COMBAT_COMMANDS ),
            WRAP( TALK_COMBAT_ENGAGEMENT ),
            WRAP( TALK_STRANGER_NEUTRAL ),
            WRAP( TALK_STRANGER_WARY ),
            WRAP( TALK_STRANGER_SCARED ),
            WRAP( TALK_STRANGER_FRIENDLY ),
            WRAP( TALK_STRANGER_AGGRESSIVE ),
            WRAP( TALK_MUG ),
            WRAP( TALK_DESCRIBE_MISSION ),
            WRAP( TALK_WEAPON_DROPPED ),
            WRAP( TALK_DEMAND_LEAVE ),
            WRAP( TALK_SIZE_UP ),
            WRAP( TALK_LOOK_AT ),
            WRAP( TALK_OPINION )
        }
    };
#undef WRAP
    const auto iter = talk_topic_enum_mapping.find( old_value );
    if( iter == talk_topic_enum_mapping.end() ) {
        debugmsg( "could not convert %d to new talk topic string", static_cast<int>( old_value ) );
        return "TALK_NONE";
    }
    return iter->second;
}

///// item.h
bool itag2ivar( std::string &item_tag, std::map<std::string, std::string> &item_vars );

void item::load_info( const std::string &data )
{
    std::istringstream dump( data );
    char check = dump.peek();
    if( check == ' ' ) {
        // sigh..
        check = data[1];
    }
    if( check == '{' ) {
        JsonIn jsin( dump );
        try {
            deserialize( jsin );
        } catch( const JsonError &jsonerr ) {
            debugmsg( "Bad item json\n%s", jsonerr.c_str() );
        }
        return;
    }

    unset_flags();
    clear_vars();
    std::string idtmp;
    std::string ammotmp;
    std::string item_tag;
    std::string mode;
    int lettmp = 0;
    int damtmp = 0;
    int acttmp = 0;
    int corp = 0;
    int tag_count = 0;
    int bday_ = 0;
    int owned; // Ignoring an obsolete member.
    dump >> lettmp >> idtmp >> charges >> damtmp >> tag_count;
    for( int i = 0; i < tag_count; ++i ) {
        dump >> item_tag;
        if( !itag2ivar( item_tag, item_vars ) ) {
            item_tags.insert( item_tag );
        }
    }

    dump >> burnt >> poison >> ammotmp >> owned >> bday_ >>
         mode >> acttmp >> corp >> mission_id >> player_id;
    bday = time_point::from_turn( bday_ );
    corpse = nullptr;
    getline( dump, corpse_name );
    if( corpse_name == " ''" ) {
        corpse_name.clear();
    } else {
        size_t pos = corpse_name.find_first_of( "@@" );
        while( pos != std::string::npos )  {
            corpse_name.replace( pos, 2, "\n" );
            pos = corpse_name.find_first_of( "@@" );
        }
        corpse_name = corpse_name.substr( 2, corpse_name.size() - 3 ); // s/^ '(.*)'$/\1/
    }
    gun_set_mode( gun_mode_id( mode ) );

    if( idtmp == "UPS_on" ) {
        idtmp = "UPS_off";
    } else if( idtmp == "adv_UPS_on" ) {
        idtmp = "adv_UPS_off" ;
    }
    convert( idtmp );

    invlet = char( lettmp );
    set_damage( damtmp * itype::damage_scale );
    active = false;
    if( acttmp == 1 ) {
        active = true;
    }
}

///// overmap legacy deserialization, replaced with json serialization June 2015
// throws std::exception (most likely as JsonError)
void overmap::unserialize_legacy( std::istream &fin )
{
    // DEBUG VARS
    int nummg = 0;
    char datatype;
    int cx = 0;
    int cy = 0;
    int cz = 0;
    int cs = 0;
    int cp = 0;
    int cd = 0;
    int cdying = 0;
    int horde = 0;
    int tx = 0;
    int ty = 0;
    int intr = 0;
    std::string cstr;
    city tmp;
    std::list<item> npc_inventory;

    if( fin.peek() == '#' ) {
        std::string vline;
        getline( fin, vline );
    }

    int z = 0; // assumption
    while( fin >> datatype ) {
        if( datatype == 'L' ) { // Load layer data, and switch to layer
            fin >> z;

            std::string tmp_ter;
            oter_id tmp_otid( 0 );
            if( z >= 0 && z < OVERMAP_LAYERS ) {
                int count = 0;
                std::unordered_map<tripoint, std::string> needs_conversion;
                for( int j = 0; j < OMAPY; j++ ) {
                    for( int i = 0; i < OMAPX; i++ ) {
                        if( count == 0 ) {
                            fin >> tmp_ter >> count;
                            if( obsolete_terrain( tmp_ter ) ) {
                                for( int p = i; p < i + count; p++ ) {
                                    needs_conversion.emplace( tripoint( p, j, z - OVERMAP_DEPTH ),
                                                              tmp_ter );
                                }
                                tmp_otid = oter_id( 0 );
                            } else if( oter_str_id( tmp_ter ).is_valid() ) {
                                tmp_otid = oter_id( tmp_ter );
                            } else if( tmp_ter.compare( 0, 7, "mall_a_" ) == 0 &&
                                       oter_str_id( tmp_ter + "_north" ).is_valid() ) {
                                tmp_otid = oter_id( tmp_ter + "_north" );
                            } else if( tmp_ter.compare( 0, 13, "necropolis_a_" ) == 0 &&
                                       oter_str_id( tmp_ter + "_north" ).is_valid() ) {
                                tmp_otid = oter_id( tmp_ter + "_north" );
                            } else {
                                debugmsg( "Loaded bad ter! ter %s", tmp_ter.c_str() );
                                tmp_otid = oter_id( 0 );
                            }
                        }
                        count--;
                        layer[z].terrain[i][j] = tmp_otid; //otermap[tmp_ter].loadid;
                        layer[z].visible[i][j] = false;
                    }
                }
                convert_terrain( needs_conversion );
            } else {
                debugmsg( "Loaded z level out of range (z: %d)", z );
            }
        } else if( datatype == 'Z' ) { // Monster group
            // save compatibility hack: read the line, initialize new members to 0,
            // "parse" line,
            std::string tmp;
            getline( fin, tmp );
            std::istringstream buffer( tmp );
            horde = 0;
            tx = 0;
            ty = 0;
            intr = 0;
            buffer >> cstr >> cx >> cy >> cz >> cs >> cp >> cd >> cdying >> horde >> tx >> ty >> intr;
            mongroup mg( mongroup_id( cstr ), cx, cy, cz, cs, cp );
            // Bugfix for old saves: population of 2147483647 is far too much and will
            // crash the game. This specific number was caused by a bug in
            // overmap::add_mon_group.
            if( mg.population == 2147483647ul ) {
                mg.population = rng( 1, 10 );
            }
            mg.diffuse = cd;
            mg.dying = cdying;
            mg.horde = horde;
            mg.set_target( tx, ty );
            mg.interest = intr;
            add_mon_group( mg );
            nummg++;
        } else if( datatype == 'M' ) {
            tripoint mon_loc;
            monster new_monster;
            fin >> mon_loc.x >> mon_loc.y >> mon_loc.z;
            std::string data;
            getline( fin, data );
            deserialize( new_monster, data );
            monster_map.insert( std::make_pair( std::move( mon_loc ),
                                                std::move( new_monster ) ) );
        } else if( datatype == 't' ) { // City
            fin >> cx >> cy >> cs;
            tmp.pos.x = cx;
            tmp.pos.y = cy;
            tmp.size = cs;
            cities.push_back( tmp );
        } else if( datatype == 'R' ) { // Road leading out
            fin >> cx >> cy;
            tmp.pos.x = cx;
            tmp.pos.y = cy;
            tmp.size = -1;
            roads_out.push_back( tmp );
        } else if( datatype == 'T' ) { // Radio tower
            radio_tower tmp;
            int tmp_type;
            fin >> tmp.x >> tmp.y >> tmp.strength >> tmp_type;
            tmp.type = static_cast<radio_type>( tmp_type );
            getline( fin, tmp.message ); // Chomp endl
            getline( fin, tmp.message );
            radios.push_back( tmp );
        } else if( datatype == 'v' ) {
            om_vehicle v;
            int id;
            fin >> id >> v.name >> v.x >> v.y;
            vehicles[id] = v;
        } else if( datatype == 'n' ) { // NPC
            // When we start loading a new NPC, check to see if we've accumulated items for
            //   assignment to an NPC.

            if( !npc_inventory.empty() && !npcs.empty() ) {
                npcs.back()->inv.push_back( npc_inventory );
                npc_inventory.clear();
            }
            std::string npcdata;
            getline( fin, npcdata );
            std::shared_ptr<npc> tmp = std::make_shared<npc>();
            tmp->load_info( npcdata );
            npcs.push_back( tmp );
        } else if( datatype == 'P' ) {
            // Chomp the invlet_cache, since the npc doesn't use it.
            std::string itemdata;
            getline( fin, itemdata );
        } else if( datatype == 'I' || datatype == 'C' || datatype == 'W' ||
                   datatype == 'w' || datatype == 'c' ) {
            std::string itemdata;
            getline( fin, itemdata );
            if( npcs.empty() ) {
                debugmsg( "Overmap %d:%d tried to load object data, without an NPC!\n%s",
                          loc.x, loc.y, itemdata.c_str() );
            } else {
                item tmp;
                tmp.load_info( itemdata );
                npc *last = npcs.back().get();
                switch( datatype ) {
                    case 'I':
                        npc_inventory.push_back( tmp );
                        break;
                    case 'C':
                        npc_inventory.back().contents.push_back( tmp );
                        break;
                    case 'W':
                        last->worn.push_back( tmp );
                        break;
                    case 'w':
                        last->weapon = tmp;
                        break;
                    case 'c':
                        last->weapon.contents.push_back( tmp );
                        break;
                }
            }
        } else if( datatype == '!' ) {  // temporary holder for future sanity
            std::string tmpstr;
            getline( fin, tmpstr );
            if( tmpstr.size() > 1 && ( tmpstr[0] == '{' || tmpstr[1] == '{' ) ) {
                std::stringstream derp;
                derp << tmpstr;
                JsonIn jsin( derp );
                try {
                    JsonObject data = jsin.get_object();

                    if( data.read( "region_id",
                                   tmpstr ) ) { // temporary, until option DEFAULT_REGION becomes start_scenario.region_id
                        if( settings.id != tmpstr ) {
                            t_regional_settings_map_citr rit = region_settings_map.find( tmpstr );
                            if( rit != region_settings_map.end() ) {
                                // temporary; user changed option, this overmap should remain whatever it was set to.
                                settings = rit->second; // @todo: optimize
                            } else { // ruh-roh! user changed option and deleted the .json with this overmap's region. We'll have to become current default. And whine about it.
                                std::string tmpopt = get_option<std::string>( "DEFAULT_REGION" );
                                rit = region_settings_map.find( tmpopt );
                                if( rit ==
                                    region_settings_map.end() ) {  // ...oy. Hopefully 'default' exists. If not, it's crash time anyway.
                                    debugmsg( "               WARNING: overmap uses missing region settings '%s'                 \n\
                ERROR, 'default_region' option uses missing region settings '%s'. Falling back to 'default'               \n\
                ....... good luck.                 \n",
                                              tmpstr.c_str(), tmpopt.c_str() );
                                    // fallback means we already loaded default and got a warning earlier.
                                } else {
                                    debugmsg( "               WARNING: overmap uses missing region settings '%s', falling back to '%s'                \n",
                                              tmpstr.c_str(), tmpopt.c_str() );
                                    // fallback means we already loaded the default region
                                }
                            }
                        }
                    }
                } catch( const JsonError &jsonerr ) {
                    debugmsg( "load overmap: json error\n%s", jsonerr.c_str() );
                    // just continue with default region
                }
            }
        }
    }

    // If we accrued an npc_inventory, assign it now
    if( !npc_inventory.empty() && !npcs.empty() ) {
        npcs.back()->inv.push_back( npc_inventory );
    }
}

void overmap::unserialize_view_legacy( std::istream &fin )
{
    // Private/per-character data
    int z = 0; // assumption
    char datatype;
    while( fin >> datatype ) {
        if( datatype == 'L' ) { // Load layer data, and switch to layer
            fin >> z;

            std::string dataline;
            getline( fin, dataline ); // Chomp endl

            int count = 0;
            int vis = 0;
            if( z >= 0 && z < OVERMAP_LAYERS ) {
                for( int j = 0; j < OMAPY; j++ ) {
                    for( int i = 0; i < OMAPX; i++ ) {
                        if( count == 0 ) {
                            fin >> vis >> count;
                        }
                        count--;
                        layer[z].visible[i][j] = ( vis == 1 );
                    }
                }
            }
        } else if( datatype == 'E' ) { //Load explored areas
            fin >> z;

            std::string dataline;
            getline( fin, dataline ); // Chomp endl

            int count = 0;
            int explored = 0;
            if( z >= 0 && z < OVERMAP_LAYERS ) {
                for( int j = 0; j < OMAPY; j++ ) {
                    for( int i = 0; i < OMAPX; i++ ) {
                        if( count == 0 ) {
                            fin >> explored >> count;
                        }
                        count--;
                        layer[z].explored[i][j] = ( explored == 1 );
                    }
                }
            }
        } else if( datatype == 'N' ) { // Load notes
            om_note tmp;
            fin >> tmp.x >> tmp.y;
            getline( fin, tmp.text ); // Chomp endl
            getline( fin, tmp.text );
            if( z >= 0 && z < OVERMAP_LAYERS ) {
                layer[z].notes.push_back( tmp );
            }
        }
    }
}

// player_activity.h
void player_activity::deserialize_legacy_type( int legacy_type, activity_id &dest )
{
    static const std::vector< activity_id > legacy_map = {
        activity_id::NULL_ID(),
        activity_id( "ACT_RELOAD" ),
        activity_id( "ACT_READ" ),
        activity_id( "ACT_GAME" ),
        activity_id( "ACT_WAIT" ),
        activity_id( "ACT_CRAFT" ),
        activity_id( "ACT_LONGCRAFT" ),
        activity_id( "ACT_DISASSEMBLE" ),
        activity_id( "ACT_BUTCHER" ),
        activity_id( "ACT_LONGSALVAGE" ),
        activity_id( "ACT_FORAGE" ),
        activity_id( "ACT_BUILD" ),
        activity_id( "ACT_VEHICLE" ),
        activity_id::NULL_ID(), // ACT_REFILL_VEHICLE is deprecated
        activity_id( "ACT_TRAIN" ),
        activity_id( "ACT_WAIT_WEATHER" ),
        activity_id( "ACT_FIRSTAID" ),
        activity_id( "ACT_FISH" ),
        activity_id( "ACT_PICKAXE" ),
        activity_id( "ACT_BURROW" ),
        activity_id( "ACT_PULP" ),
        activity_id( "ACT_VIBE" ),
        activity_id( "ACT_MAKE_ZLAVE" ),
        activity_id( "ACT_DROP" ),
        activity_id( "ACT_STASH" ),
        activity_id( "ACT_PICKUP" ),
        activity_id( "ACT_MOVE_ITEMS" ),
        activity_id( "ACT_ADV_INVENTORY" ),
        activity_id( "ACT_ARMOR_LAYERS" ),
        activity_id( "ACT_START_FIRE" ),
        activity_id( "ACT_OPEN_GATE" ),
        activity_id( "ACT_FILL_LIQUID" ),
        activity_id( "ACT_HOTWIRE_CAR" ),
        activity_id( "ACT_AIM" ),
        activity_id( "ACT_ATM" ),
        activity_id( "ACT_START_ENGINES" ),
        activity_id( "ACT_OXYTORCH" ),
        activity_id( "ACT_CRACKING" ),
        activity_id( "ACT_REPAIR_ITEM" ),
        activity_id( "ACT_MEND_ITEM" ),
        activity_id( "ACT_GUNMOD_ADD" ),
        activity_id( "ACT_WAIT_NPC" ),
        activity_id( "ACT_CLEAR_RUBBLE" ),
        activity_id( "ACT_MEDITATE" ),
        activity_id::NULL_ID() // NUM_ACTIVITIES
    };

    if( legacy_type < 0 || static_cast<size_t>( legacy_type ) >= legacy_map.size() ) {
        debugmsg( "Bad legacy activity data. Got %d, expected something from 0 to %d", legacy_type,
                  legacy_map.size() );
        dest = activity_id::NULL_ID();
        return;
    }
    dest = legacy_map[ legacy_type ];
}
