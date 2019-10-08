#include <cstddef>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <list>
#include <map>
#include <memory>
#include <type_traits>
#include <utility>

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
#include "cata_utility.h"
#include "game_constants.h"
#include "inventory.h"
#include "monster.h"
#include "regional_settings.h"
#include "rng.h"
#include "type_id.h"
#include "flat_set.h"
#include "point.h"

namespace std
{
template <>
struct hash<talk_topic_enum> {
    // Operator overload required by std API.
    std::size_t operator()( const talk_topic_enum &k ) const {
        return k; // the most trivial hash of them all
    }
};
} // namespace std

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
        activity_id::NULL_ID(), // ACT_LONGCRAFT is deprecated
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
