#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "debug.h"
#include "npc.h"
#include "player_activity.h"
#include "type_id.h"

static const activity_id ACT_AIM( "ACT_AIM" );
static const activity_id ACT_ARMOR_LAYERS( "ACT_ARMOR_LAYERS" );
static const activity_id ACT_ATM( "ACT_ATM" );
static const activity_id ACT_BUILD( "ACT_BUILD" );
static const activity_id ACT_BURROW( "ACT_BURROW" );
static const activity_id ACT_BUTCHER( "ACT_BUTCHER" );
static const activity_id ACT_CRACKING( "ACT_CRACKING" );
static const activity_id ACT_DISASSEMBLE( "ACT_DISASSEMBLE" );
static const activity_id ACT_FILL_LIQUID( "ACT_FILL_LIQUID" );
static const activity_id ACT_FISH( "ACT_FISH" );
static const activity_id ACT_GAME( "ACT_GAME" );
static const activity_id ACT_HOTWIRE_CAR( "ACT_HOTWIRE_CAR" );
static const activity_id ACT_MEDITATE( "ACT_MEDITATE" );
static const activity_id ACT_MEND_ITEM( "ACT_MEND_ITEM" );
static const activity_id ACT_MOVE_ITEMS( "ACT_MOVE_ITEMS" );
static const activity_id ACT_OPEN_GATE( "ACT_OPEN_GATE" );
static const activity_id ACT_PICKAXE( "ACT_PICKAXE" );
static const activity_id ACT_PICKUP( "ACT_PICKUP" );
static const activity_id ACT_PULP( "ACT_PULP" );
static const activity_id ACT_RELOAD( "ACT_RELOAD" );
static const activity_id ACT_REPAIR_ITEM( "ACT_REPAIR_ITEM" );
static const activity_id ACT_START_ENGINES( "ACT_START_ENGINES" );
static const activity_id ACT_START_FIRE( "ACT_START_FIRE" );
static const activity_id ACT_TRAIN( "ACT_TRAIN" );
static const activity_id ACT_VEHICLE( "ACT_VEHICLE" );
static const activity_id ACT_VIBE( "ACT_VIBE" );
static const activity_id ACT_WAIT( "ACT_WAIT" );
static const activity_id ACT_WAIT_NPC( "ACT_WAIT_NPC" );
static const activity_id ACT_WAIT_WEATHER( "ACT_WAIT_WEATHER" );

namespace std
{
template <>
struct hash<talk_topic_enum> {
    // Operator overload required by std API.
    std::size_t operator()( const talk_topic_enum &k ) const noexcept {
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
        ACT_RELOAD,
        activity_id::NULL_ID(), // ACT_READ is an actor now
        ACT_GAME,
        ACT_WAIT,
        activity_id::NULL_ID(), // ACT_CRAFT is an actor now
        activity_id::NULL_ID(), // ACT_LONGCRAFT is deprecated
        ACT_DISASSEMBLE,
        ACT_BUTCHER,
        activity_id::NULL_ID(), // ACT_LONGSALVAGE is an actor now
        activity_id::NULL_ID(), // ACT_FORAGE is an actor now
        ACT_BUILD,
        ACT_VEHICLE,
        activity_id::NULL_ID(), // ACT_REFILL_VEHICLE is deprecated
        ACT_TRAIN,
        ACT_WAIT_WEATHER,
        activity_id::NULL_ID(), // ACT_FIRSTAID is an actor now
        ACT_FISH,
        ACT_PICKAXE,
        ACT_BURROW,
        ACT_PULP,
        ACT_VIBE,
        activity_id::NULL_ID(), // ACT_MAKE_ZLAVE is deprecated
        activity_id::NULL_ID(), // ACT_DROP is an actor now
        activity_id::NULL_ID(), // ACT_STASH is an actor now
        ACT_PICKUP,
        ACT_MOVE_ITEMS,
        activity_id::NULL_ID(), // ACT_ADV_INVENTORY is uistate.open_menu now
        ACT_ARMOR_LAYERS,
        ACT_START_FIRE,
        ACT_OPEN_GATE,
        ACT_FILL_LIQUID,
        ACT_HOTWIRE_CAR,
        ACT_AIM,
        ACT_ATM,
        ACT_START_ENGINES,
        activity_id::NULL_ID(), // ACT_OXYTORCH is an actor now
        ACT_CRACKING,
        ACT_REPAIR_ITEM,
        ACT_MEND_ITEM,
        activity_id::NULL_ID(), // ACT_GUNMOD_ADD
        ACT_WAIT_NPC,
        activity_id::NULL_ID(), // ACT_CLEAR_RUBBLE
        ACT_MEDITATE,
        activity_id::NULL_ID() // NUM_ACTIVITIES
    };

    if( legacy_type < 0 || static_cast<size_t>( legacy_type ) >= legacy_map.size() ) {
        debugmsg( "Bad legacy activity data.  Got %d, expected something from 0 to %d", legacy_type,
                  legacy_map.size() );
        dest = activity_id::NULL_ID();
        return;
    }
    dest = legacy_map[ legacy_type ];
}
