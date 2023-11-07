#include "npc.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <ostream>

#include "activity_type.h"
#include "activity_actor_definitions.h"
#include "auto_pickup.h"
#include "basecamp.h"
#include "bodypart.h"
#include "catacharset.h"
#include "character.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "clzones.h"
#include "coordinate_conversions.h"
#include "creature_tracker.h"
#include "cursesdef.h"
#include "damage.h"
#include "debug.h"
#include "dialogue.h"
#include "dialogue_chatbin.h"
#include "effect.h"
#include "effect_on_condition.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "flag.h"
#include "game.h"
#include "gates.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "item.h"
#include "item_group.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "json.h"
#include "magic.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "mission.h"
#include "monster.h"
#include "morale_types.h"
#include "mtype.h"
#include "mutation.h"
#include "npc_class.h"
#include "npctrade.h"
#include "npctrade_utils.h"
#include "npctalk.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "player_activity.h"
#include "ret_val.h"
#include "rng.h"
#include "skill.h"
#include "sounds.h"
#include "stomach.h"
#include "string_formatter.h"
#include "talker.h"
#include "talker_npc.h"
#include "text_snippets.h"
#include "tileray.h"
#include "trait_group.h"
#include "translations.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "viewer.h"
#include "visitable.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "name.h"

static const efftype_id effect_bouldering( "bouldering" );
static const efftype_id effect_controlled( "controlled" );
static const efftype_id effect_drunk( "drunk" );
static const efftype_id effect_high( "high" );
static const efftype_id effect_infection( "infection" );
static const efftype_id effect_mending( "mending" );
static const efftype_id effect_npc_flee_player( "npc_flee_player" );
static const efftype_id effect_npc_suspend( "npc_suspend" );
static const efftype_id effect_pkill1( "pkill1" );
static const efftype_id effect_pkill2( "pkill2" );
static const efftype_id effect_pkill3( "pkill3" );
static const efftype_id effect_pkill_l( "pkill_l" );
static const efftype_id effect_ridden( "ridden" );
static const efftype_id effect_riding( "riding" );

static const faction_id faction_amf( "amf" );
static const faction_id faction_no_faction( "no_faction" );
static const faction_id faction_your_followers( "your_followers" );

static const item_group_id Item_spawn_data_guns_pistol_common( "guns_pistol_common" );
static const item_group_id Item_spawn_data_guns_rifle_common( "guns_rifle_common" );
static const item_group_id Item_spawn_data_guns_shotgun_common( "guns_shotgun_common" );
static const item_group_id Item_spawn_data_guns_smg_common( "guns_smg_common" );
static const item_group_id Item_spawn_data_npc_eyes( "npc_eyes" );
static const item_group_id Item_spawn_data_survivor_bashing( "survivor_bashing" );
static const item_group_id Item_spawn_data_survivor_cutting( "survivor_cutting" );
static const item_group_id Item_spawn_data_survivor_stabbing( "survivor_stabbing" );

static const mfaction_str_id monfaction_bee( "bee" );
static const mfaction_str_id monfaction_human( "human" );
static const mfaction_str_id monfaction_player( "player" );

static const mon_flag_str_id mon_flag_RIDEABLE_MECH( "RIDEABLE_MECH" );

static const overmap_location_str_id overmap_location_source_of_ammo( "source_of_ammo" );
static const overmap_location_str_id overmap_location_source_of_anything( "source_of_anything" );
static const overmap_location_str_id overmap_location_source_of_drink( "source_of_drink" );
static const overmap_location_str_id overmap_location_source_of_food( "source_of_food" );
static const overmap_location_str_id overmap_location_source_of_guns( "source_of_guns" );
static const overmap_location_str_id overmap_location_source_of_safety( "source_of_safety" );
static const overmap_location_str_id overmap_location_source_of_weapons( "source_of_weapons" );

static const skill_id skill_archery( "archery" );
static const skill_id skill_bashing( "bashing" );
static const skill_id skill_cutting( "cutting" );
static const skill_id skill_dodge( "dodge" );
static const skill_id skill_gun( "gun" );
static const skill_id skill_launcher( "launcher" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_pistol( "pistol" );
static const skill_id skill_rifle( "rifle" );
static const skill_id skill_shotgun( "shotgun" );
static const skill_id skill_smg( "smg" );
static const skill_id skill_speech( "speech" );
static const skill_id skill_stabbing( "stabbing" );
static const skill_id skill_throw( "throw" );
static const skill_id skill_unarmed( "unarmed" );

static const trait_id trait_BEE( "BEE" );
static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_DEBUG_MIND_CONTROL( "DEBUG_MIND_CONTROL" );
static const trait_id trait_HALLUCINATION( "HALLUCINATION" );
static const trait_id trait_MUTE( "MUTE" );
static const trait_id trait_NO_BASH( "NO_BASH" );
static const trait_id trait_PROF_DICEMASTER( "PROF_DICEMASTER" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );
static const trait_id trait_SQUEAMISH( "SQUEAMISH" );
static const trait_id trait_TERRIFYING( "TERRIFYING" );

class monfaction;

static void starting_clothes( npc &who, const npc_class_id &type, bool male );
static void starting_inv( npc &who, const npc_class_id &type );

bool job_data::set_task_priority( const activity_id &task, int new_priority )
{
    auto it = task_priorities.find( task );
    if( it != task_priorities.end() ) {
        task_priorities[task] = new_priority;
        return true;
    }
    return false;
}
void job_data::clear_all_priorities()
{
    for( auto &elem : task_priorities ) {
        elem.second = 0;
    }
}
bool job_data::has_job() const
{
    for( const auto &elem : task_priorities ) {
        if( elem.second > 0 ) {
            return true;
        }
    }
    return false;
}
int job_data::get_priority_of_job( const activity_id &req_job ) const
{
    auto it = task_priorities.find( req_job );
    if( it != task_priorities.end() ) {
        return it->second;
    } else {
        return 0;
    }
}

std::vector<activity_id> job_data::get_prioritised_vector() const
{
    std::vector<std::pair<activity_id, int>> pairs( begin( task_priorities ), end( task_priorities ) );

    std::vector<activity_id> ret;
    sort( begin( pairs ), end( pairs ), []( const std::pair<activity_id, int> &a,
    const std::pair<activity_id, int> &b ) {
        return a.second > b.second;
    } );
    ret.reserve( pairs.size() );
    for( const std::pair<activity_id, int> &elem : pairs ) {
        ret.push_back( elem.first );
    }
    return ret;
}

npc::npc()
    : restock( calendar::turn_zero )
    , companion_mission_time( calendar::before_time_starts )
    , companion_mission_time_ret( calendar::before_time_starts )
{
    last_updated = calendar::turn;
    last_player_seen_pos = std::nullopt;
    last_seen_player_turn = 999;
    wanted_item_pos = tripoint_min;
    guard_pos = std::nullopt;
    goal = tripoint_abs_omt( tripoint_min );
    fetching_item = false;
    has_new_items = true;
    worst_item_value = 0;
    str_max = 0;
    dex_max = 0;
    int_max = 0;
    per_max = 0;
    marked_for_death = false;
    death_drops = true;
    dead = false;
    hit_by_player = false;
    hallucination = false;
    moves = 100;
    mission = NPC_MISSION_NULL;
    myclass = npc_class_id::NULL_ID();
    fac_id = faction_id::NULL_ID();
    patience = 0;
    attitude = NPCATT_NULL;

    *path_settings = pathfinding_settings( 0, 1000, 1000, 10, true, true, true, true, false, true );
    for( direction threat_dir : npc_threat_dir ) {
        ai_cache.threat_map[ threat_dir ] = 0.0f;
    }
}

standard_npc::standard_npc( const std::string &name, const tripoint &pos,
                            const std::vector<std::string> &clothing,
                            int sk_lvl, int s_str, int s_dex, int s_int, int s_per )
{
    this->name = name;
    set_pos_only( pos );

    str_cur = std::max( s_str, 0 );
    str_max = std::max( s_str, 0 );
    dex_cur = std::max( s_dex, 0 );
    dex_max = std::max( s_dex, 0 );
    per_cur = std::max( s_per, 0 );
    per_max = std::max( s_per, 0 );
    int_cur = std::max( s_int, 0 );
    int_max = std::max( s_int, 0 );

    set_body();
    recalc_hp();

    for( const Skill &e : Skill::skills ) {
        set_skill_level( e.ident(), std::max( sk_lvl, 0 ) );
    }

    for( const std::string &e : clothing ) {
        wear_item( item( e ), false );
    }

    worn.set_fitted();
}

npc::npc( npc && ) noexcept( map_is_noexcept ) = default;
npc &npc::operator=( npc && ) noexcept( list_is_noexcept ) = default;

static std::map<string_id<npc_template>, npc_template> npc_templates;

void npc_template::load( const JsonObject &jsobj )
{
    npc_template tem;
    npc &guy = tem.guy;
    guy.idz = npc_class_id( jsobj.get_string( "id" ) );
    guy.name.clear();
    jsobj.read( "name_unique", tem.name_unique );
    jsobj.read( "name_suffix", tem.name_suffix );
    if( jsobj.has_string( "gender" ) ) {
        if( jsobj.get_string( "gender" ) == "male" ) {
            tem.gender_override = gender::male;
        } else {
            tem.gender_override = gender::female;
        }
    }
    if( jsobj.has_string( "faction" ) ) {
        guy.set_fac_id( jsobj.get_string( "faction" ) );
    }

    if( jsobj.has_int( "class" ) ) {
        guy.myclass = npc_class::from_legacy_int( jsobj.get_int( "class" ) );
    } else if( jsobj.has_string( "class" ) ) {
        guy.myclass = npc_class_id( jsobj.get_string( "class" ) );
    }

    guy.set_attitude( static_cast<npc_attitude>( jsobj.get_int( "attitude" ) ) );
    guy.mission = static_cast<npc_mission>( jsobj.get_int( "mission" ) );
    guy.chatbin.first_topic = jsobj.get_string( "chat" );
    if( jsobj.has_string( "mission_offered" ) ) {
        guy.miss_ids.emplace_back( jsobj.get_string( "mission_offered" ) );
    } else if( jsobj.has_array( "mission_offered" ) ) {
        for( const std::string line : jsobj.get_array( "mission_offered" ) ) {
            guy.miss_ids.emplace_back( line );
        }
    }
    if( jsobj.has_string( "talk_radio" ) ) {
        guy.chatbin.talk_radio = jsobj.get_string( "talk_radio" );
    }
    if( jsobj.has_string( "talk_leader" ) ) {
        guy.chatbin.talk_leader = jsobj.get_string( "talk_leader" );
    }
    if( jsobj.has_string( "talk_friend" ) ) {
        guy.chatbin.talk_friend = jsobj.get_string( "talk_friend" );
    }
    if( jsobj.has_string( "talk_stole_item" ) ) {
        guy.chatbin.talk_stole_item = jsobj.get_string( "talk_stole_item" );
    }
    if( jsobj.has_string( "talk_wake_up" ) ) {
        guy.chatbin.talk_wake_up = jsobj.get_string( "talk_wake_up" );
    }
    if( jsobj.has_string( "talk_mug" ) ) {
        guy.chatbin.talk_mug = jsobj.get_string( "talk_mug" );
    }
    if( jsobj.has_string( "talk_stranger_aggressive" ) ) {
        guy.chatbin.talk_stranger_aggressive = jsobj.get_string( "talk_stranger_aggressive" );
    }
    if( jsobj.has_string( "talk_stranger_scared" ) ) {
        guy.chatbin.talk_stranger_scared = jsobj.get_string( "talk_stranger_scared" );
    }
    if( jsobj.has_string( "talk_stranger_wary" ) ) {
        guy.chatbin.talk_stranger_wary = jsobj.get_string( "talk_stranger_wary" );
    }
    if( jsobj.has_string( "talk_stranger_friendly" ) ) {
        guy.chatbin.talk_stranger_friendly = jsobj.get_string( "talk_stranger_friendly" );
    }
    if( jsobj.has_string( "talk_stranger_neutral" ) ) {
        guy.chatbin.talk_stranger_neutral = jsobj.get_string( "talk_stranger_neutral" );
    }
    if( jsobj.has_string( "talk_friend_guard" ) ) {
        guy.chatbin.talk_friend_guard = jsobj.get_string( "talk_friend_guard" );
    }
    if( jsobj.has_string( "<acknowledged>" ) ) {
        guy.chatbin.snip_acknowledged = jsobj.get_string( "<acknowledged>" );
    }
    if( jsobj.has_string( "<camp_food_thanks>" ) ) {
        guy.chatbin.snip_camp_food_thanks = jsobj.get_string( "<camp_food_thanks>" );
    }
    if( jsobj.has_string( "<camp_larder_empty>" ) ) {
        guy.chatbin.snip_camp_larder_empty = jsobj.get_string( "<camp_larder_empty>" );
    }
    if( jsobj.has_string( "<camp_water_thanks>" ) ) {
        guy.chatbin.snip_camp_water_thanks = jsobj.get_string( "<camp_water_thanks>" );
    }
    if( jsobj.has_string( "<cant_flee>" ) ) {
        guy.chatbin.snip_cant_flee = jsobj.get_string( "<cant_flee>" );
    }
    if( jsobj.has_string( "<close_distance>" ) ) {
        guy.chatbin.snip_close_distance = jsobj.get_string( "<close_distance>" );
    }
    if( jsobj.has_string( "<combat_noise_warning>" ) ) {
        guy.chatbin.snip_combat_noise_warning = jsobj.get_string( "<combat_noise_warning>" );
    }
    if( jsobj.has_string( "<danger_close_distance>" ) ) {
        guy.chatbin.snip_danger_close_distance = jsobj.get_string( "<danger_close_distance>" );
    }
    if( jsobj.has_string( "<done_mugging>" ) ) {
        guy.chatbin.snip_done_mugging = jsobj.get_string( "<done_mugging>" );
    }
    if( jsobj.has_string( "<far_distance>" ) ) {
        guy.chatbin.snip_far_distance = jsobj.get_string( "<far_distance>" );
    }
    if( jsobj.has_string( "<fire_bad>" ) ) {
        guy.chatbin.snip_fire_bad = jsobj.get_string( "<fire_bad>" );
    }
    if( jsobj.has_string( "<fire_in_the_hole_h>" ) ) {
        guy.chatbin.snip_fire_in_the_hole_h = jsobj.get_string( "<fire_in_the_hole_h>" );
    }
    if( jsobj.has_string( "<fire_in_the_hole>" ) ) {
        guy.chatbin.snip_fire_in_the_hole = jsobj.get_string( "<fire_in_the_hole>" );
    }
    if( jsobj.has_string( "<general_danger_h>" ) ) {
        guy.chatbin.snip_general_danger_h = jsobj.get_string( "<general_danger_h>" );
    }
    if( jsobj.has_string( "<general_danger>" ) ) {
        guy.chatbin.snip_general_danger = jsobj.get_string( "<general_danger>" );
    }
    if( jsobj.has_string( "<heal_self>" ) ) {
        guy.chatbin.snip_heal_self = jsobj.get_string( "<heal_self>" );
    }
    if( jsobj.has_string( "<hungry>" ) ) {
        guy.chatbin.snip_hungry = jsobj.get_string( "<hungry>" );
    }
    if( jsobj.has_string( "<im_leaving_you>" ) ) {
        guy.chatbin.snip_im_leaving_you = jsobj.get_string( "<im_leaving_you>" );
    }
    if( jsobj.has_string( "<its_safe_h>" ) ) {
        guy.chatbin.snip_its_safe_h = jsobj.get_string( "<its_safe_h>" );
    }
    if( jsobj.has_string( "<its_safe>" ) ) {
        guy.chatbin.snip_its_safe = jsobj.get_string( "<its_safe>" );
    }
    if( jsobj.has_string( "<keep_up>" ) ) {
        guy.chatbin.snip_keep_up = jsobj.get_string( "<keep_up>" );
    }
    if( jsobj.has_string( "<kill_npc_h>" ) ) {
        guy.chatbin.snip_kill_npc_h = jsobj.get_string( "<kill_npc_h>" );
    }
    if( jsobj.has_string( "<kill_npc>" ) ) {
        guy.chatbin.snip_kill_npc = jsobj.get_string( "<kill_npc>" );
    }
    if( jsobj.has_string( "<kill_player_h>" ) ) {
        guy.chatbin.snip_kill_player_h = jsobj.get_string( "<kill_player_h>" );
    }
    if( jsobj.has_string( "<let_me_pass>" ) ) {
        guy.chatbin.snip_let_me_pass = jsobj.get_string( "<let_me_pass>" );
    }
    if( jsobj.has_string( "<lets_talk>" ) ) {
        guy.chatbin.snip_lets_talk = jsobj.get_string( "<lets_talk>" );
    }
    if( jsobj.has_string( "<medium_distance>" ) ) {
        guy.chatbin.snip_medium_distance = jsobj.get_string( "<medium_distance>" );
    }
    if( jsobj.has_string( "<monster_warning_h>" ) ) {
        guy.chatbin.snip_monster_warning_h = jsobj.get_string( "<monster_warning_h>" );
    }
    if( jsobj.has_string( "<monster_warning>" ) ) {
        guy.chatbin.snip_monster_warning = jsobj.get_string( "<monster_warning>" );
    }
    if( jsobj.has_string( "<movement_noise_warning>" ) ) {
        guy.chatbin.snip_movement_noise_warning = jsobj.get_string( "<movement_noise_warning>" );
    }
    if( jsobj.has_string( "<need_batteries>" ) ) {
        guy.chatbin.snip_need_batteries = jsobj.get_string( "<need_batteries>" );
    }
    if( jsobj.has_string( "<need_booze>" ) ) {
        guy.chatbin.snip_need_booze = jsobj.get_string( "<need_booze>" );
    }
    if( jsobj.has_string( "<need_fuel>" ) ) {
        guy.chatbin.snip_need_fuel = jsobj.get_string( "<need_fuel>" );
    }
    if( jsobj.has_string( "<no_to_thorazine>" ) ) {
        guy.chatbin.snip_no_to_thorazine = jsobj.get_string( "<no_to_thorazine>" );
    }
    if( jsobj.has_string( "<run_away>" ) ) {
        guy.chatbin.snip_run_away = jsobj.get_string( "<run_away>" );
    }
    if( jsobj.has_string( "<speech_warning>" ) ) {
        guy.chatbin.snip_speech_warning = jsobj.get_string( "<speech_warning>" );
    }
    if( jsobj.has_string( "<thirsty>" ) ) {
        guy.chatbin.snip_thirsty = jsobj.get_string( "<thirsty>" );
    }
    if( jsobj.has_string( "<wait>" ) ) {
        guy.chatbin.snip_wait = jsobj.get_string( "<wait>" );
    }
    if( jsobj.has_string( "<warn_sleep>" ) ) {
        guy.chatbin.snip_warn_sleep = jsobj.get_string( "<warn_sleep>" );
    }
    if( jsobj.has_string( "<yawn>" ) ) {
        guy.chatbin.snip_yawn = jsobj.get_string( "<yawn>" );
    }
    if( jsobj.has_string( "<yes_to_lsd>" ) ) {
        guy.chatbin.snip_yes_to_lsd = jsobj.get_string( "<yes_to_lsd>" );
    }
    if( jsobj.has_string( "snip_pulp_zombie" ) ) {
        guy.chatbin.snip_pulp_zombie = jsobj.get_string( "snip_pulp_zombie" );
    }
    if( jsobj.has_string( "snip_heal_player" ) ) {
        guy.chatbin.snip_heal_player = jsobj.get_string( "snip_heal_player" );
    }
    if( jsobj.has_string( "snip_mug_dontmove" ) ) {
        guy.chatbin.snip_mug_dontmove = jsobj.get_string( "snip_mug_dontmove" );
    }
    if( jsobj.has_string( "snip_wound_infected" ) ) {
        guy.chatbin.snip_wound_infected = jsobj.get_string( "snip_wound_infected" );
    }
    if( jsobj.has_string( "snip_wound_bite" ) ) {
        guy.chatbin.snip_wound_bite = jsobj.get_string( "snip_wound_bite" );
    }
    if( jsobj.has_string( "snip_radiation_sickness" ) ) {
        guy.chatbin.snip_radiation_sickness = jsobj.get_string( "snip_radiation_sickness" );
    }
    if( jsobj.has_string( "snip_bleeding" ) ) {
        guy.chatbin.snip_bleeding = jsobj.get_string( "snip_bleeding" );
    }
    if( jsobj.has_string( "snip_bleeding_badly" ) ) {
        guy.chatbin.snip_bleeding_badly = jsobj.get_string( "snip_bleeding_badly" );
    }
    if( jsobj.has_string( "snip_lost_blood" ) ) {
        guy.chatbin.snip_lost_blood = jsobj.get_string( "snip_lost_blood" );
    }
    if( jsobj.has_string( "snip_bye" ) ) {
        guy.chatbin.snip_bye = jsobj.get_string( "snip_bye" );
    }
    if( jsobj.has_string( "snip_consume_cant_accept" ) ) {
        guy.chatbin.snip_consume_cant_accept = jsobj.get_string( "snip_consume_cant_accept" );
    }
    if( jsobj.has_string( "snip_consume_cant_consume" ) ) {
        guy.chatbin.snip_consume_cant_consume = jsobj.get_string( "snip_consume_cant_consume" );
    }
    if( jsobj.has_string( "snip_consume_rotten" ) ) {
        guy.chatbin.snip_consume_rotten = jsobj.get_string( "snip_consume_rotten" );
    }
    if( jsobj.has_string( "snip_consume_eat" ) ) {
        guy.chatbin.snip_consume_eat = jsobj.get_string( "snip_consume_eat" );
    }
    if( jsobj.has_string( "snip_consume_need_item" ) ) {
        guy.chatbin.snip_consume_need_item = jsobj.get_string( "snip_consume_need_item" );
    }
    if( jsobj.has_string( "snip_consume_med" ) ) {
        guy.chatbin.snip_consume_med = jsobj.get_string( "snip_consume_med" );
    }
    if( jsobj.has_string( "snip_consume_nocharge" ) ) {
        guy.chatbin.snip_consume_nocharge = jsobj.get_string( "snip_consume_nocharge" );
    }
    if( jsobj.has_string( "snip_consume_use_med" ) ) {
        guy.chatbin.snip_consume_use_med = jsobj.get_string( "snip_consume_use_med" );
    }
    if( jsobj.has_string( "snip_give_nope" ) ) {
        guy.chatbin.snip_give_nope = jsobj.get_string( "snip_give_nope" );
    }
    if( jsobj.has_string( "snip_give_to_hallucination" ) ) {
        guy.chatbin.snip_give_to_hallucination = jsobj.get_string( "snip_give_to_hallucination" );
    }
    if( jsobj.has_string( "snip_give_cancel" ) ) {
        guy.chatbin.snip_give_cancel = jsobj.get_string( "snip_give_cancel" );
    }
    if( jsobj.has_string( "snip_give_dangerous" ) ) {
        guy.chatbin.snip_give_dangerous = jsobj.get_string( "snip_give_dangerous" );
    }
    if( jsobj.has_string( "snip_give_wield" ) ) {
        guy.chatbin.snip_give_wield = jsobj.get_string( "snip_give_wield" );
    }
    if( jsobj.has_string( "snip_give_weapon_weak" ) ) {
        guy.chatbin.snip_give_weapon_weak = jsobj.get_string( "snip_give_weapon_weak" );
    }
    if( jsobj.has_string( "snip_give_carry" ) ) {
        guy.chatbin.snip_give_carry = jsobj.get_string( "snip_give_carry" );
    }
    if( jsobj.has_string( "snip_give_carry_cant" ) ) {
        guy.chatbin.snip_give_carry_cant = jsobj.get_string( "snip_give_carry_cant" );
    }
    if( jsobj.has_string( "snip_give_carry_cant_few_space" ) ) {
        guy.chatbin.snip_give_carry_cant_few_space = jsobj.get_string( "snip_give_carry_cant_few_space" );
    }
    if( jsobj.has_string( "snip_give_carry_cant_no_space" ) ) {
        guy.chatbin.snip_give_carry_cant_no_space = jsobj.get_string( "snip_give_carry_cant_no_space" );
    }
    if( jsobj.has_string( "snip_give_carry_too_heavy" ) ) {
        guy.chatbin.snip_give_carry_too_heavy = jsobj.get_string( "snip_give_carry_too_heavy" );
    }
    if( jsobj.has_string( "snip_wear" ) ) {
        guy.chatbin.snip_wear = jsobj.get_string( "snip_wear" );
    }
    jsobj.read( "age", tem.age );
    jsobj.read( "height", tem.height );
    jsobj.read( "str", tem.str );
    jsobj.read( "dex", tem.dex );
    jsobj.read( "int", tem.intl );
    jsobj.read( "per", tem.per );
    if( jsobj.has_object( "personality" ) ) {
        const JsonObject personality = jsobj.get_object( "personality" );
        tem.personality = npc_personality();
        tem.personality->aggression = personality.get_int( "aggression" );
        tem.personality->bravery = personality.get_int( "bravery" );
        tem.personality->collector = personality.get_int( "collector" );
        tem.personality->altruism = personality.get_int( "altruism" );
    }
    for( JsonValue jv : jsobj.get_array( "death_eocs" ) ) {
        guy.death_eocs.emplace_back( effect_on_conditions::load_inline_eoc( jv, "" ) );
    }

    npc_templates.emplace( string_id<npc_template>( guy.idz.str() ), std::move( tem ) );
}

void npc_template::reset()
{
    npc_templates.clear();
}

void npc_template::check_consistency()
{
    for( const auto &e : npc_templates ) {
        const npc &guy = e.second.guy;
        if( !guy.myclass.is_valid() ) {
            debugmsg( "Invalid NPC class %s", guy.myclass.c_str() );
        }
        std::string first_topic = guy.chatbin.first_topic;
        if( const json_talk_topic *topic = get_talk_topic( first_topic ) ) {
            cata::flat_set<std::string> reachable_topics =
                topic->get_directly_reachable_topics( true );
            if( reachable_topics.count( "TALK_MISSION_OFFER" ) ) {
                debugmsg(
                    "NPC template \"%s\" has dialogue \"%s\" which leads unconditionally to "
                    "\"TALK_MISSION_OFFER\", which doesn't check for an available mission.  "
                    "You should probably prefer \"TALK_MISSION_LIST\"",
                    e.first.str(), first_topic );
            }
        }
    }
}

template<>
bool string_id<npc_template>::is_valid() const
{
    return npc_templates.count( *this ) > 0;
}

template<>
const npc_template &string_id<npc_template>::obj() const
{
    const auto found = npc_templates.find( *this );
    if( found == npc_templates.end() ) {
        debugmsg( "Tried to get invalid npc: %s", c_str() );
        static const npc_template dummy{};
        return dummy;
    }
    return found->second;
}

void npc::load_npc_template( const string_id<npc_template> &ident )
{
    auto found = npc_templates.find( ident );
    if( found == npc_templates.end() ) {
        debugmsg( "Tried to get invalid npc: %s", ident.c_str() );
        return;
    }
    const npc_template &tem = found->second;
    const npc &tguy = tem.guy;

    idz = tguy.idz;
    myclass = npc_class_id( tguy.myclass );
    randomize( myclass, ident );
    if( tem.gender_override != npc_template::gender::random ) {
        male = tem.gender_override == npc_template::gender::male;
    }
    name = Name::generate( male );
    if( !tem.name_unique.empty() ) {
        name = tem.name_unique.translated();
    }
    if( !tem.name_suffix.empty() ) {
        //~ %1$s: npc name, %2$s: name suffix
        name = string_format( pgettext( "npc name", "%1$s, %2$s" ), name, tem.name_suffix );
    }
    fac_id = tguy.fac_id;
    set_fac( fac_id );
    attitude = tguy.attitude;
    mission = tguy.mission;
    chatbin.first_topic = tguy.chatbin.first_topic;
    chatbin.talk_radio = tguy.chatbin.talk_radio;
    chatbin.talk_leader = tguy.chatbin.talk_leader;
    chatbin.talk_friend = tguy.chatbin.talk_friend;
    chatbin.talk_stole_item = tguy.chatbin.talk_stole_item;
    chatbin.talk_wake_up = tguy.chatbin.talk_wake_up;
    chatbin.talk_mug = tguy.chatbin.talk_mug;
    chatbin.talk_stranger_aggressive = tguy.chatbin.talk_stranger_aggressive;
    chatbin.talk_stranger_scared = tguy.chatbin.talk_stranger_scared;
    chatbin.talk_stranger_wary = tguy.chatbin.talk_stranger_wary;
    chatbin.talk_stranger_friendly = tguy.chatbin.talk_stranger_friendly;
    chatbin.talk_stranger_neutral = tguy.chatbin.talk_stranger_neutral;
    chatbin.talk_friend_guard = tguy.chatbin.talk_friend_guard;
    chatbin.snip_acknowledged = tguy.chatbin.snip_acknowledged;
    chatbin.snip_camp_food_thanks = tguy.chatbin.snip_camp_food_thanks;
    chatbin.snip_camp_larder_empty = tguy.chatbin.snip_camp_larder_empty;
    chatbin.snip_camp_water_thanks = tguy.chatbin.snip_camp_water_thanks;
    chatbin.snip_cant_flee = tguy.chatbin.snip_cant_flee;
    chatbin.snip_close_distance = tguy.chatbin.snip_close_distance;
    chatbin.snip_combat_noise_warning = tguy.chatbin.snip_combat_noise_warning;
    chatbin.snip_danger_close_distance = tguy.chatbin.snip_danger_close_distance;
    chatbin.snip_done_mugging = tguy.chatbin.snip_done_mugging;
    chatbin.snip_far_distance = tguy.chatbin.snip_far_distance;
    chatbin.snip_fire_bad = tguy.chatbin.snip_fire_bad;
    chatbin.snip_fire_in_the_hole_h = tguy.chatbin.snip_fire_in_the_hole_h;
    chatbin.snip_fire_in_the_hole = tguy.chatbin.snip_fire_in_the_hole;
    chatbin.snip_general_danger_h = tguy.chatbin.snip_general_danger_h;
    chatbin.snip_general_danger = tguy.chatbin.snip_general_danger;
    chatbin.snip_heal_self = tguy.chatbin.snip_heal_self;
    chatbin.snip_hungry = tguy.chatbin.snip_hungry;
    chatbin.snip_im_leaving_you = tguy.chatbin.snip_im_leaving_you;
    chatbin.snip_its_safe_h = tguy.chatbin.snip_its_safe_h;
    chatbin.snip_its_safe = tguy.chatbin.snip_its_safe;
    chatbin.snip_keep_up = tguy.chatbin.snip_keep_up;
    chatbin.snip_kill_npc_h = tguy.chatbin.snip_kill_npc_h;
    chatbin.snip_kill_npc = tguy.chatbin.snip_kill_npc;
    chatbin.snip_kill_player_h = tguy.chatbin.snip_kill_player_h;
    chatbin.snip_let_me_pass = tguy.chatbin.snip_let_me_pass;
    chatbin.snip_lets_talk = tguy.chatbin.snip_lets_talk;
    chatbin.snip_medium_distance = tguy.chatbin.snip_medium_distance;
    chatbin.snip_monster_warning_h = tguy.chatbin.snip_monster_warning_h;
    chatbin.snip_monster_warning = tguy.chatbin.snip_monster_warning;
    chatbin.snip_movement_noise_warning = tguy.chatbin.snip_movement_noise_warning;
    chatbin.snip_need_batteries = tguy.chatbin.snip_need_batteries;
    chatbin.snip_need_booze = tguy.chatbin.snip_need_booze;
    chatbin.snip_need_fuel = tguy.chatbin.snip_need_fuel;
    chatbin.snip_no_to_thorazine = tguy.chatbin.snip_no_to_thorazine;
    chatbin.snip_run_away = tguy.chatbin.snip_run_away;
    chatbin.snip_speech_warning = tguy.chatbin.snip_speech_warning;
    chatbin.snip_thirsty = tguy.chatbin.snip_thirsty;
    chatbin.snip_wait = tguy.chatbin.snip_wait;
    chatbin.snip_warn_sleep = tguy.chatbin.snip_warn_sleep;
    chatbin.snip_yawn = tguy.chatbin.snip_yawn;
    chatbin.snip_yes_to_lsd = tguy.chatbin.snip_yes_to_lsd;
    chatbin.snip_pulp_zombie = tguy.chatbin.snip_pulp_zombie;
    chatbin.snip_heal_player = tguy.chatbin.snip_heal_player;
    chatbin.snip_mug_dontmove = tguy.chatbin.snip_mug_dontmove;
    chatbin.snip_wound_infected = tguy.chatbin.snip_wound_infected;
    chatbin.snip_wound_bite = tguy.chatbin.snip_wound_bite;
    chatbin.snip_radiation_sickness = tguy.chatbin.snip_radiation_sickness;
    chatbin.snip_bleeding = tguy.chatbin.snip_bleeding;
    chatbin.snip_bleeding_badly = tguy.chatbin.snip_bleeding_badly;
    chatbin.snip_lost_blood = tguy.chatbin.snip_lost_blood;
    chatbin.snip_bye = tguy.chatbin.snip_bye;
    chatbin.snip_consume_cant_accept = tguy.chatbin.snip_consume_cant_accept;
    chatbin.snip_consume_cant_consume = tguy.chatbin.snip_consume_cant_consume;
    chatbin.snip_consume_rotten = tguy.chatbin.snip_consume_rotten;
    chatbin.snip_consume_eat = tguy.chatbin.snip_consume_eat;
    chatbin.snip_consume_need_item = tguy.chatbin.snip_consume_need_item;
    chatbin.snip_consume_med = tguy.chatbin.snip_consume_med;
    chatbin.snip_consume_nocharge = tguy.chatbin.snip_consume_nocharge;
    chatbin.snip_consume_use_med = tguy.chatbin.snip_consume_use_med;
    chatbin.snip_give_nope = tguy.chatbin.snip_give_nope;
    chatbin.snip_give_to_hallucination = tguy.chatbin.snip_give_to_hallucination;
    chatbin.snip_give_cancel = tguy.chatbin.snip_give_cancel;
    chatbin.snip_give_dangerous = tguy.chatbin.snip_give_dangerous;
    chatbin.snip_give_wield = tguy.chatbin.snip_give_wield;
    chatbin.snip_give_weapon_weak = tguy.chatbin.snip_give_weapon_weak;
    chatbin.snip_give_carry = tguy.chatbin.snip_give_carry;
    chatbin.snip_give_carry_cant = tguy.chatbin.snip_give_carry_cant;
    chatbin.snip_give_carry_cant_few_space = tguy.chatbin.snip_give_carry_cant_few_space;
    chatbin.snip_give_carry_cant_no_space = tguy.chatbin.snip_give_carry_cant_no_space;
    chatbin.snip_give_carry_too_heavy = tguy.chatbin.snip_give_carry_too_heavy;
    chatbin.snip_wear = tguy.chatbin.snip_wear;

    for( const mission_type_id &miss_id : tguy.miss_ids ) {
        add_new_mission( mission::reserve_new( miss_id, getID() ) );
    }
    death_eocs = tguy.death_eocs;
}

npc::~npc() = default;

void npc::randomize( const npc_class_id &type, const npc_template_id &tem_id )
{
    if( !getID().is_valid() ) {
        setID( g->assign_npc_id() );
    }

    set_wielded_item( item( "null", calendar::turn_zero ) );
    inv->clear();
    personality.aggression = rng( -10, 10 );
    personality.bravery    = rng( -3, 10 );
    personality.collector  = rng( -1, 10 );
    // Normal distribution. Mean = 0, stddev = 3, clamp at -10 and 10. Rounded to return integer value.
    personality.altruism   = std::round( std::max( -10.0, std::min( normal_roll( 0, 3 ), 10.0 ) ) );
    moves = 100;
    mission = NPC_MISSION_NULL;
    male = one_in( 2 );
    pick_name();
    randomize_height();
    set_base_age( rng( 18, 55 ) );
    str_max = dice( 4, 3 );
    dex_max = dice( 4, 3 );
    int_max = dice( 4, 3 );
    per_max = dice( 4, 3 );

    if( tem_id.is_valid() ) {
        const npc_template &tem = tem_id.obj();
        if( tem.personality.has_value() ) {
            personality.aggression = tem.personality->aggression;
            personality.bravery = tem.personality->bravery;
            personality.collector = tem.personality->collector;
            personality.altruism = tem.personality->altruism;
        }
        if( tem.str.has_value() ) {
            str_max = tem.str.value();
        }
        if( tem.dex.has_value() ) {
            dex_max = tem.dex.value();
        }
        if( tem.intl.has_value() ) {
            int_max = tem.intl.value();
        }
        if( tem.per.has_value() ) {
            per_max = tem.per.value();
        }
        if( tem.height.has_value() ) {
            set_base_height( tem.height.value() );
        }
        if( tem.age.has_value() ) {
            set_base_age( tem.age.value() );
        }
    }

    if( !type.is_valid() ) {
        debugmsg( "Invalid NPC class %s", type.c_str() );
        myclass = npc_class_id::NULL_ID();
    } else if( type.is_null() ) {
        myclass = npc_class::random_common();
    } else {
        myclass = type;
    }

    const npc_class &the_class = myclass.obj();
    str_max += the_class.roll_strength();
    dex_max += the_class.roll_dexterity();
    int_max += the_class.roll_intelligence();
    per_max += the_class.roll_perception();

    for( Skill &skill : Skill::skills ) {
        int level = myclass->roll_skill( skill.ident() );

        set_skill_level( skill.ident(), level );
    }

    if( type.is_null() ) { // Untyped; no particular specialization
    } else if( type == NC_EVAC_SHOPKEEP || type == NC_BARTENDER || type == NC_JUNK_SHOPKEEP ) {
        personality.collector += rng( 1, 5 );

    } else if( type == NC_ARSONIST ) {
        personality.aggression += rng( 0, 1 );
        personality.collector += rng( 0, 2 );

    } else if( type == NC_SOLDIER ) {
        personality.aggression += rng( 1, 3 );
        personality.bravery += rng( 0, 5 );

    } else if( type == NC_HACKER ) {
        personality.bravery -= rng( 1, 3 );
        personality.aggression -= rng( 0, 2 );

    } else if( type == NC_DOCTOR ) {
        personality.aggression -= rng( 0, 4 );
        cash += 10000 * rng( 0, 3 ) * rng( 0, 3 );

    } else if( type == NC_TRADER ) {
        personality.collector += rng( 1, 5 );
        cash += 25000 * rng( 1, 10 );

    } else if( type == NC_NINJA ) {
        personality.bravery += rng( 0, 3 );
        personality.collector -= rng( 1, 6 );
        // TODO: give ninja his styles back

    } else if( type == NC_COWBOY ) {
        personality.aggression += rng( 0, 2 );
        personality.bravery += rng( 1, 5 );

    } else if( type == NC_SCIENTIST ) {
        personality.aggression -= rng( 1, 5 );
        personality.bravery -= rng( 2, 8 );
        personality.collector += rng( 0, 2 );

    } else if( type == NC_BOUNTY_HUNTER || type == NC_THUG ) {
        personality.aggression += rng( 1, 6 );
        personality.bravery += rng( 0, 5 );

    } else if( type == NC_SCAVENGER ) {
        personality.aggression += rng( 1, 3 );
        personality.bravery += rng( 1, 4 );

    }
    //A universal barter boost to keep NPCs competitive with players
    //The int boost from trade wasn't active... now that it is, most
    //players will vastly outclass npcs in trade without a little help.
    mod_skill_level( skill_speech, rng( 2, 4 ) );

    set_body();
    recalc_hp();
    int days_since_cata = to_days<int>( calendar::turn - calendar::start_of_cataclysm );
    double time_influence = days_since_cata >= 180 ? 3.0 : 6.0 - 3.0 * days_since_cata / 180.0;
    double weight_percent = std::clamp<double>( chi_squared_roll( time_influence ) / 5.0,
                            0.2, 5.0 );
    set_stored_kcal( weight_percent * get_healthy_kcal() );
    starting_weapon( myclass );
    starting_clothes( *this, myclass, male );
    starting_inv( *this, myclass );
    has_new_items = true;
    clear_mutations();

    // Add fixed traits
    for( const trait_and_var &cur : trait_group::traits_from( myclass->traits ) ) {
        const trait_id &tid = cur.trait;
        const std::string &var = cur.variant;
        set_mutation( tid, tid->variant( var ) );
    }

    // Run mutation rounds
    for( const auto &mr : type->mutation_rounds ) {
        int rounds = mr.second.roll();
        for( int i = 0; i < rounds; ++i ) {
            mutate_category( mr.first );
        }
    }
    // Add bionics
    for( const auto &bl : type->bionic_list ) {
        int chance = bl.second;
        if( rng( 0, 100 ) <= chance ) {
            add_bionic( bl.first );
        }
    }
    // Add proficiencies
    for( const proficiency_id &prof : type->_starting_proficiencies ) {
        add_proficiency( prof );
    }
    // Add martial arts
    learn_ma_styles_from_traits();
    // Add spells for magiclysm mod
    for( std::pair<spell_id, int> spell_pair : type->_starting_spells ) {
        this->magic->learn_spell( spell_pair.first, *this, true );
        spell &sp = this->magic->get_spell( spell_pair.first );
        while( sp.get_level() < spell_pair.second && !sp.is_max_level( *this ) ) {
            sp.gain_level( *this );
        }
    }

    // Add eocs
    effect_on_conditions::load_new_character( *this );
}

void npc::learn_ma_styles_from_traits()
{
    for( const trait_id &iter : get_mutations() ) {
        if( !iter->initial_ma_styles.empty() ) {
            std::vector<matype_id> shuffled_trait_styles = iter->initial_ma_styles;
            std::shuffle( shuffled_trait_styles.begin(), shuffled_trait_styles.end(), rng_get_engine() );

            for( const matype_id &style : shuffled_trait_styles ) {
                if( !martial_arts_data->has_martialart( style ) ) {
                    martial_arts_data->learn_style( style, false );
                    break;
                }
            }
        }
    }
}

void npc::randomize_from_faction( faction *fac )
{
    // Personality = aggression, bravery, altruism, collector
    set_fac( fac->id );
    randomize( npc_class_id::NULL_ID() );
}

void npc::set_fac( const faction_id &id )
{
    if( my_fac ) {
        my_fac->remove_member( getID() );
    }
    my_fac = g->faction_manager_ptr->get( id );
    if( my_fac ) {
        if( !is_fake() && !is_hallucination() ) {
            my_fac->add_to_membership( getID(), disp_name(), known_to_u );
        }
        fac_id = my_fac->id;
    } else {
        return;
    }
    apply_ownership_to_inv();
}

void npc::apply_ownership_to_inv()
{
    for( item *&e : inv_dump() ) {
        e->set_owner( *this );
    }
}

faction_id npc::get_fac_id() const
{
    return fac_id;
}

faction *npc::get_faction() const
{
    if( !my_fac ) {
        return g->faction_manager_ptr->get( faction_no_faction );
    }
    return my_fac;
}

// item id from group "<class-name>_<what>" or from fallback group
// may still be a null item!
static item random_item_from( const npc_class_id &type, const std::string &what,
                              const item_group_id &fallback )
{
    item result = item_group::item_from( item_group_id( type.str() + "_" + what ), calendar::turn );
    if( result.is_null() ) {
        result = item_group::item_from( fallback, calendar::turn );
    }
    return result;
}

// item id from "<class-name>_<what>" or from "npc_<what>"
static item random_item_from( const npc_class_id &type, const std::string &what )
{
    return random_item_from( type, what, item_group_id( "npc_" + what ) );
}

// item id from "<class-name>_<what>_<gender>" or from "npc_<what>_<gender>"
static item get_clothing_item( const npc_class_id &type, const std::string &what, bool male )
{
    item result;
    //Check if class has gendered clothing
    //Then check if it has an ungendered version
    //Only if all that fails, grab from the default class.
    if( male ) {
        result = random_item_from( type, what + "_male", item_group_id::NULL_ID() );
    } else {
        result = random_item_from( type, what + "_female", item_group_id::NULL_ID() );
    }
    if( result.is_null() ) {
        if( male ) {
            result = random_item_from( type, what, item_group_id( "npc_" + what + "_male" ) );
        } else {
            result = random_item_from( type, what, item_group_id( "npc_" + what + "_female" ) );
        }
    }

    return result;
}

void starting_clothes( npc &who, const npc_class_id &type, bool male )
{
    std::vector<item> ret;
    if( item_group::group_is_defined( type->worn_override ) ) {
        ret = item_group::items_from( type->worn_override );
    } else {
        ret.push_back( get_clothing_item( type, "pants", male ) );
        ret.push_back( get_clothing_item( type, "shirt", male ) );
        ret.push_back( get_clothing_item( type, "underwear_top", male ) );
        ret.push_back( get_clothing_item( type, "underwear_bottom", male ) );
        ret.push_back( get_clothing_item( type, "underwear_feet", male ) );
        ret.push_back( get_clothing_item( type, "shoes", male ) );
        ret.push_back( random_item_from( type, "gloves" ) );
        ret.push_back( random_item_from( type, "coat" ) );
        ret.push_back( random_item_from( type, "vest" ) );
        ret.push_back( random_item_from( type, "masks" ) );
        // Why is the alternative group not named "npc_glasses" but "npc_eyes"?
        ret.push_back( random_item_from( type, "glasses", Item_spawn_data_npc_eyes ) );
        ret.push_back( random_item_from( type, "hat" ) );
        ret.push_back( random_item_from( type, "scarf" ) );
        ret.push_back( random_item_from( type, "storage" ) );
        ret.push_back( random_item_from( type, "holster" ) );
        ret.push_back( random_item_from( type, "belt" ) );
        ret.push_back( random_item_from( type, "wrist" ) );
        ret.push_back( random_item_from( type, "extra" ) );
    }

    who.worn.on_takeoff( who );
    who.clear_worn();
    for( item &it : ret ) {
        if( it.has_flag( flag_VARSIZE ) ) {
            it.set_flag( flag_FIT );
        }
        if( who.can_wear( it ).success() ) {
            it.on_wear( who );
            who.worn.wear_item( who, it, false, false );
            it.set_owner( who );
        }
    }
}

void starting_inv( npc &who, const npc_class_id &type )
{
    std::list<item> res;
    who.inv->clear();
    if( item_group::group_is_defined( type->carry_override ) ) {
        *who.inv += item_group::items_from( type->carry_override );
        return;
    }

    // If wielding a gun, get some additional ammo for it
    const item_location weapon = who.get_wielded_item();
    if( weapon && weapon->is_gun() ) {
        item ammo;
        if( !weapon->magazine_default().is_null() ) {
            item mag( weapon->magazine_default() );
            mag.ammo_set( mag.ammo_default() );
            ammo = item( mag.ammo_default() );
            res.push_back( mag );
        } else if( !weapon->ammo_default().is_null() ) {
            ammo = item( weapon->ammo_default() );
            // TODO: Move to npc_class
            // NC_COWBOY and NC_BOUNTY_HUNTER get 5-15 whilst all others get 3-6
            int qty = 1 + ( type == NC_COWBOY ||
                            type == NC_BOUNTY_HUNTER );
            qty = rng( qty, qty * 2 );

            while( qty-- != 0 && who.can_stash( ammo ) ) {
                res.push_back( ammo );
            }
        }
    }

    if( type == NC_ARSONIST ) {
        res.emplace_back( "molotov" );
    }

    int qty = ( type == NC_EVAC_SHOPKEEP ||
                type == NC_TRADER ) ? 5 : 2;
    qty = rng( qty, qty * 3 );

    while( qty-- != 0 ) {
        item tmp = random_item_from( type, "misc" ).in_its_container();
        if( !tmp.is_null() ) {
            if( !one_in( 3 ) && tmp.has_flag( flag_VARSIZE ) ) {
                tmp.set_flag( flag_FIT );
            }
            if( who.can_pickVolume( tmp ) ) {
                res.push_back( tmp );
            }
        }
    }

    res.erase( std::remove_if( res.begin(), res.end(), [&]( const item & e ) {
        return e.has_flag( flag_TRADER_AVOID );
    } ), res.end() );
    for( item &it : res ) {
        it.set_owner( who );
    }
    *who.inv += res;
}

void npc::revert_after_activity()
{
    mission = previous_mission;
    attitude = previous_attitude;
    activity = player_activity();
    current_activity_id = activity_id::NULL_ID();
    clear_destination();
    backlog.clear();
}

npc_mission npc::get_previous_mission() const
{
    return previous_mission;
}

npc_attitude npc::get_previous_attitude()
{
    return previous_attitude;
}

bool npc::get_known_to_u() const
{
    return known_to_u;
}

void npc::set_known_to_u( bool known )
{
    known_to_u = known;
    if( my_fac ) {
        my_fac->add_to_membership( getID(), disp_name(), known_to_u );
    }
}

void npc::on_move( const tripoint_abs_ms &old_pos )
{
    Character::on_move( old_pos );
    const point_abs_om pos_om_old = project_to<coords::om>( old_pos.xy() );
    const point_abs_om pos_om_new = project_to<coords::om>( get_location().xy() );
    if( !is_fake() && pos_om_old != pos_om_new ) {
        overmap &om_old = overmap_buffer.get( pos_om_old );
        overmap &om_new = overmap_buffer.get( pos_om_new );
        if( !unique_id.empty() ) {
            g->update_unique_npc_location( unique_id, pos_om_new );
        }
        if( const auto ptr = om_old.erase_npc( getID() ) ) {
            om_new.insert_npc( ptr );
        } else {
            // Don't move the npc pointer around to avoid having two overmaps
            // with the same npc pointer
            debugmsg( "could not find npc %s on its old overmap", get_name() );
        }
    }
}

void npc::travel_overmap( const tripoint_abs_omt &pos )
{
    // TODO: fix point types
    const point_abs_om pos_om_old = project_to<coords::om>( global_omt_location().xy() );
    spawn_at_omt( pos );
    const point_abs_om pos_om_new = project_to<coords::om>( global_omt_location().xy() );
    if( global_omt_location() == goal ) {
        reach_omt_destination();
    }
    if( !is_fake() && pos_om_old != pos_om_new ) {
        overmap &om_old = overmap_buffer.get( pos_om_old );
        overmap &om_new = overmap_buffer.get( pos_om_new );
        if( const auto ptr = om_old.erase_npc( getID() ) ) {
            om_new.insert_npc( ptr );
        } else {
            // Don't move the npc pointer around to avoid having two overmaps
            // with the same npc pointer
            debugmsg( "could not find npc %s on its old overmap", get_name() );
        }
    }
}

void npc::spawn_at_omt( const tripoint_abs_omt &p )
{
    const int max_coord = coords::map_squares_per( coords::omt ) - 1;
    const point_rel_ms local_pos( rng( 0, max_coord ), rng( 0, max_coord ) );
    spawn_at_precise( project_to<coords::ms>( p ) + local_pos );
}

void npc::spawn_at_precise( const tripoint_abs_ms &p )
{
    set_location( p );
}

void npc::place_on_map()
{
    if( g->is_empty( pos() ) || is_mounted() ) {
        return;
    }

    for( const tripoint &p : closest_points_first( pos(), SEEX + 1 ) ) {
        if( g->is_empty( p ) ) {
            setpos( p );
            return;
        }
    }

    debugmsg( "Failed to place NPC in a valid location near (%d,%d,%d)", posx(), posy(), posz() );
}

//Subset: whether "combat skill" includes all combat skills, no "general" (dodge, melee, marksman) skills, or only weapons you would expect NPCs to wield
//Returns a pair with the skill_id (first) of the best skill, and the level (int) of that skill. If there is no best skill, defaults to stabbing.
std::pair<skill_id, int> npc::best_combat_skill( combat_skills subset ) const
{
    std::pair<skill_id, int> highest_skill( skill_stabbing, 0 );

    for( const auto &p : *_skills ) {
        if( p.first.obj().is_combat_skill() ) {
            switch( subset ) {
                case combat_skills::ALL:
                    break;
                case combat_skills::NO_GENERAL:
                    if( p.first == skill_dodge || p.first == skill_gun || p.first == skill_melee ) {
                        continue;
                    }
                    break;
                case combat_skills::WEAPONS_ONLY:
                    if( p.first == skill_dodge || p.first == skill_gun || p.first == skill_melee ||
                        p.first == skill_unarmed || p.first == skill_launcher ) {
                        continue;
                    }
                    break;
            }

            const int level = p.second.level();
            if( level > highest_skill.second ) {
                highest_skill.second = level;
                highest_skill.first = p.first;
            }
        }
    }

    return highest_skill;
}

void npc::starting_weapon( const npc_class_id &type )
{
    if( item_group::group_is_defined( type->weapon_override ) ) {
        set_wielded_item( item_group::item_from( type->weapon_override, calendar::turn ) );
        return;
    }

    const skill_id best = best_combat_skill( combat_skills::WEAPONS_ONLY ).first;

    if( best == skill_stabbing ) {
        set_wielded_item( random_item_from( type, "stabbing", Item_spawn_data_survivor_stabbing ) );
    } else if( best == skill_bashing ) {
        set_wielded_item( random_item_from( type, "bashing", Item_spawn_data_survivor_bashing ) );
    } else if( best == skill_cutting ) {
        set_wielded_item( random_item_from( type, "cutting", Item_spawn_data_survivor_cutting ) );
    } else if( best == skill_throw ) {
        set_wielded_item( random_item_from( type, "throw" ) );
    } else if( best == skill_archery ) {
        set_wielded_item( random_item_from( type, "archery" ) );
    } else if( best == skill_pistol ) {
        set_wielded_item( random_item_from( type, "pistol", Item_spawn_data_guns_pistol_common ) );
    } else if( best == skill_shotgun ) {
        set_wielded_item( random_item_from( type, "shotgun", Item_spawn_data_guns_shotgun_common ) );
    } else if( best == skill_smg ) {
        set_wielded_item( random_item_from( type, "smg", Item_spawn_data_guns_smg_common ) );
    } else if( best == skill_rifle ) {
        set_wielded_item( random_item_from( type, "rifle", Item_spawn_data_guns_rifle_common ) );
    }
    item_location weapon = get_wielded_item();
    if( weapon ) {
        if( weapon->is_gun() ) {
            if( !weapon->magazine_default().is_null() ) {
                weapon->ammo_set( weapon->magazine_default()->magazine->default_ammo );
            } else if( !weapon->ammo_default().is_null() ) {
                weapon->ammo_set( weapon->ammo_default() );
            } else {
                debugmsg( "tried setting ammo for %s which has no magazine or ammo", weapon->typeId().c_str() );
            }
            //You should be able to wield your starting weapon
            if( !meets_stat_requirements( *weapon ) ) {
                if( weapon->get_min_str() > get_str() ) {
                    str_max = weapon->get_min_str();
                }
                if( weapon->type->min_dex > get_dex() ) {
                    dex_max = weapon->type->min_dex;
                }
                if( weapon->type->min_int > get_int() ) {
                    int_max = weapon->type->min_int;
                }
                if( weapon->type->min_per > get_per() ) {
                    per_max = weapon->type->min_per;
                }
            }
        }

        cata::event e = cata::event::make<event_type::character_wields_item>( getID(), weapon->typeId() );
        get_event_bus().send_with_talker( this, &weapon, e );

        weapon->set_owner( get_faction()->id );
    }
}

bool npc::can_read( const item &book, std::vector<std::string> &fail_reasons )
{
    Character *pl = dynamic_cast<Character *>( this );
    if( !pl ) {
        return false;
    }
    read_condition_result condition = check_read_condition( book );
    if( condition & read_condition_result::NOT_BOOK ) {
        fail_reasons.push_back( string_format( _( "This %s is not good reading material." ),
                                               book.tname() ) );
        return false;
    }
    if( condition & read_condition_result::CANT_UNDERSTAND ) {
        fail_reasons.push_back( string_format( _( "I'm not smart enough to read this book." ) ) );
        return false;
    }
    if( condition & read_condition_result::MASTERED ) {
        fail_reasons.push_back( string_format( _( "I won't learn anything from this book." ) ) );
        return false;
    }

    // Check for conditions that disqualify us
    if( condition & read_condition_result::ILLITERATE ) {
        fail_reasons.emplace_back( _( "I can't read!" ) );
    } else if( condition & read_condition_result::NEED_GLASSES ) {
        fail_reasons.emplace_back( _( "I can't read without my glasses." ) );
    } else if( condition & read_condition_result::TOO_DARK ) {
        // Too dark to read only applies if the player can read to himself
        fail_reasons.emplace_back( _( "It's too dark to read!" ) );
        return false;
    }
    return true;
}

time_duration npc::time_to_read( const item &book, const Character &reader ) const
{
    const auto &type = book.type->book;
    const skill_id &skill = type->skill;
    // The reader's reading speed has an effect only if they're trying to understand the book as they read it
    // Reading speed is assumed to be how well you learn from books (as opposed to hands-on experience)
    const bool try_understand = reader.fun_to_read( book ) ||
                                reader.get_knowledge_level( skill ) < type->level;
    int reading_speed = try_understand ? std::max( reader.read_speed(), read_speed() ) : read_speed();

    time_duration retval = type->time * reading_speed / 100;
    retval *= std::min( fine_detail_vision_mod(), reader.fine_detail_vision_mod() );

    if( type->intel > reader.get_int() && !reader.has_trait( trait_PROF_DICEMASTER ) ) {
        retval += type->time * ( time_duration::from_seconds( type->intel - reader.get_int() ) /
                                 1_minutes );
    }
    return retval;
}

void npc::do_npc_read( bool ebook )
{
    // Can read items from inventory or within one tile (including in vehicles)
    Character *npc_player = as_character();
    if( !npc_player ) {
        return;
    }

    item_location book;
    item_location ereader;

    if( !ebook ) {
        book = game_menus::inv::read( *npc_player );
    } else {
        ereader = game_menus::inv::ereader_to_use( *npc_player );
        if( !ereader ) {
            add_msg( _( "Never mind." ) );
            return;
        }
        book = game_menus::inv::ebookread( *npc_player, ereader );
    }

    if( !book ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    std::vector<std::string> fail_reasons;
    Character *npc_character = as_character();
    if( !npc_character ) {
        return;
    }

    book = book.obtain( *npc_character );
    if( can_read( *book, fail_reasons ) ) {
        add_msg_if_player_sees( pos(), _( "%s starts reading." ), disp_name() );

        // NPCs can't read to other NPCs yet
        const time_duration time_taken = time_to_read( *book, *this );

        // NPCs read until they gain a level
        read_activity_actor actor( time_taken, book, ereader, true, getID().get_value() );
        assign_activity( actor );

    } else {
        for( const std::string &reason : fail_reasons ) {
            say( reason );
        }
    }
}

bool npc::wear_if_wanted( const item &it, std::string &reason )
{
    // Note: this function isn't good enough to use with NPC AI alone
    // Restrict it to player's orders for now
    if( !it.is_armor() ) {
        reason = _( "This can't be worn." );
        return false;
    }

    // Splints ignore limits, but only when being equipped on a broken part
    // TODO: Drop splints when healed
    if( it.has_flag( flag_SPLINT ) ) {
        for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
            if( is_limb_broken( bp ) && !has_effect( effect_mending, bp.id() ) &&
                it.covers( bp ) ) {
                reason = _( chatbin.snip_wear );
                return !!wear_item( it, false );
            }
        }
    }

    while( !worn.empty() ) {
        size_t size_before = worn.size();
        // Strip until we can put the new item on
        // This is one of the reasons this command is not used by the AI
        if( can_wear( it ).success() ) {
            // TODO: Hazmat/power armor makes this not work due to 1 boots/headgear limit

            if( !!wear_item( it, false ) ) {
                reason = _( chatbin.snip_wear );
                return true;
            } else {
                reason = _( "I tried but couldn't wear it." );
                return false;
            }
        }
        // Otherwise, maybe we should take off one or more items and replace them
        bool took_off = false;
        for( const bodypart_id &bp : get_all_body_parts() ) {
            if( !it.covers( bp ) ) {
                continue;
            }
            // Find an item that covers the same body part as the new item
            item_location armor_covering = worn.first_item_covering_bp( *this, bp );
            if( armor_covering && !( is_limb_broken( bp ) && armor_covering->has_flag( flag_SPLINT ) ) ) {
                //create an item_location for player::takeoff to handle.
                took_off = takeoff( armor_covering );
                break;
            }
        }

        if( !took_off || worn.size() >= size_before ) {
            // Shouldn't happen, but does
            reason = _( "I tried but couldn't wear it." );
            return false;
        }
    }
    reason = _( chatbin.snip_wear );
    return worn.empty() && wear_item( it, false );
}

void npc::stow_item( item &it )
{
    bool avatar_sees = get_player_view().sees( pos() );
    if( wear_item( it, false ) ) {
        // Wearing the item was successful, remove weapon and post message.
        if( avatar_sees ) {
            add_msg_if_npc( m_info, _( "<npcname> wears the %s." ), it.tname() );
        }
        remove_item( it );
        moves -= 15;
        // Weapon cannot be worn or wearing was not successful. Store it in inventory if possible,
        // otherwise drop it.
    } else if( can_stash( it ) ) {
        item_location ret = i_add( remove_item( it ), true, nullptr, nullptr, true, false );
        if( avatar_sees ) {
            add_msg_if_npc( m_info, _( "<npcname> puts away the %s." ), ret->tname() );
        }
        moves -= 15;
    } else { // No room for weapon, so we drop it
        if( avatar_sees ) {
            add_msg_if_npc( m_info, _( "<npcname> drops the %s." ), it.tname() );
        }
        get_map().add_item_or_charges( pos(), remove_item( it ) );
    }
}

bool npc::wield( item &it )
{
    // sanity check: exit early if we're trying to wield the current weapon
    // needed for ranged_balance_test
    if( is_wielding( it ) ) {
        return true;
    }

    item to_wield;
    if( has_item( it ) ) {
        to_wield = remove_item( it );
    } else {
        to_wield = it;
    }
    invalidate_leak_level_cache();
    invalidate_inventory_validity_cache();
    cached_info.erase( "weapon_value" );
    item_location weapon = get_wielded_item();
    if( has_wield_conflicts( to_wield ) ) {
        stow_item( *weapon );
        weapon = get_wielded_item();
    }

    if( to_wield.is_null() ) {
        set_wielded_item( item() );
        get_event_bus().send<event_type::character_wields_item>( getID(), item().typeId() );
        return true;
    }

    moves -= 15;
    if( weapon && to_wield.can_combine( *weapon ) ) {
        weapon->combine( to_wield );
    } else {
        set_wielded_item( to_wield );
    }

    weapon = get_wielded_item();
    cata::event e = cata::event::make<event_type::character_wields_item>( getID(), weapon->typeId() );
    get_event_bus().send_with_talker( this, &weapon, e );

    if( get_player_view().sees( pos() ) ) {
        add_msg_if_npc( m_info, _( "<npcname> wields a %s." ),  weapon->tname() );
    }
    invalidate_range_cache();
    return true;
}

void npc::drop( const drop_locations &what, const tripoint &target,
                bool stash )
{
    Character::drop( what, target, stash );
    // TODO: Remove the hack. Its here because npcs didn't process activities, but they do now
    // so is this necessary?
    activity.do_turn( *this );
}

void npc::invalidate_range_cache()
{
    const item_location weapon = get_wielded_item();

    if( !weapon ) {
        confident_range_cache = 1;
    } else if( weapon->is_gun() ) {
        confident_range_cache = confident_shoot_range( *weapon,
                                most_accurate_aiming_method_limit( *weapon ) );
    } else {
        confident_range_cache = weapon->reach_range( *this );
    }
}

void npc::form_opinion( const Character &you )
{
    op_of_u = get_opinion_values( you );

    decide_needs();
    for( const npc_need &i : needs ) {
        if( i == need_food || i == need_drink ) {
            op_of_u.value += 2;
        }
    }

    if( op_of_u.fear < personality.bravery + 10 &&
        op_of_u.fear - personality.aggression > -10 && op_of_u.trust > -8 ) {
        set_attitude( NPCATT_TALK );
    } else if( op_of_u.fear - 2 * personality.aggression - personality.bravery < -30 ) {
        set_attitude( NPCATT_KILL );
    } else if( my_fac && my_fac->likes_u < -10 ) {
        if( is_player_ally() ) {
            mutiny();
        }
        set_attitude( NPCATT_KILL );
    } else {
        set_attitude( NPCATT_FLEE_TEMP );
    }

    add_msg_debug( debugmode::DF_NPC, "%s formed an opinion of you: %s", get_name(),
                   npc_attitude_id( attitude ) );
}

npc_opinion npc::get_opinion_values( const Character &you ) const
{
    npc_opinion npc_values = op_of_u;

    const item_location weapon = you.get_wielded_item();
    // FEAR
    if( !you.is_armed() ) {
        // Unarmed, but actually unarmed ("unarmed weapons" are not unarmed)
        npc_values.fear -= 3;
    } else if( weapon->is_gun() ) {
        // TODO: Make bows not guns
        if( weapon->has_flag( flag_PRIMITIVE_RANGED_WEAPON ) ) {
            npc_values.fear += 2;
        } else {
            npc_values.fear += 6;
        }
    } else if( you.weapon_value( *weapon ) > 20 ) {
        npc_values.fear += 2;
    }

    ///\EFFECT_STR increases NPC fear of the player
    npc_values.fear += ( you.str_max / 4 ) - 2;

    // is your health low
    for( const std::pair<const bodypart_str_id, bodypart> &elem : get_player_character().get_body() ) {
        const int hp_max = elem.second.get_hp_max();
        const int hp_cur = elem.second.get_hp_cur();
        if( hp_cur <= hp_max / 2 ) {
            npc_values.fear--;
        }
    }

    // is my health low
    for( const std::pair<const bodypart_str_id, bodypart> &elem : get_body() ) {
        const int hp_max = elem.second.get_hp_max();
        const int hp_cur = elem.second.get_hp_cur();
        if( hp_cur <= hp_max / 2 ) {
            npc_values.fear++;
        }
    }

    if( you.has_trait( trait_SAPIOVORE ) ) {
        npc_values.fear += 10; // Sapiovores = Scary
    }
    if( you.has_trait( trait_TERRIFYING ) ) {
        npc_values.fear += 6;
    }

    int u_ugly = 0;
    for( trait_id &mut : you.get_mutations() ) {
        u_ugly += mut.obj().ugliness;
    }
    for( const bodypart_id &bp : you.get_all_body_parts() ) {
        if( bp->ugliness == 0 && bp->ugliness_mandatory == 0 ) {
            continue;
        }
        u_ugly += bp->ugliness_mandatory;
        u_ugly += bp->ugliness - ( bp->ugliness * worn.get_coverage( bp ) / 100 );
    }
    npc_values.fear += u_ugly / 2;
    npc_values.trust -= u_ugly / 3;

    if( you.get_stim() > 20 ) {
        npc_values.fear++;
    }

    if( you.has_effect( effect_drunk ) ) {
        npc_values.fear -= 2;
    }

    // TRUST
    if( op_of_u.fear > 0 ) {
        npc_values.trust -= 3;
    } else {
        npc_values.trust += 1;
    }

    if( weapon && weapon->is_gun() ) {
        npc_values.trust -= 2;
    } else if( !you.is_armed() ) {
        npc_values.trust += 2;
    }

    // TODO: More effects
    if( you.has_effect( effect_high ) ) {
        npc_values.trust -= 1;
    }
    if( you.has_effect( effect_drunk ) ) {
        npc_values.trust -= 2;
    }
    if( you.get_stim() > 20 || you.get_stim() < -20 ) {
        npc_values.trust -= 1;
    }
    if( you.get_painkiller() > 30 ) {
        npc_values.trust -= 1;
    }

    if( op_of_u.trust > 0 ) {
        // Trust is worth a lot right now
        npc_values.trust /= 2;
    }

    // VALUE
    npc_values.value = 0;
    for( const std::pair<const bodypart_str_id, bodypart> &elem : get_body() ) {
        if( elem.second.get_hp_cur() < elem.second.get_hp_max() * 0.8f ) {
            npc_values.value++;
        }
    }

    return npc_values;
}

void npc::mutiny()
{
    if( !my_fac || !is_player_ally() ) {
        return;
    }
    const bool seen = get_player_view().sees( pos() );
    if( seen ) {
        add_msg( m_bad, _( "%s is tired of your incompetent leadership and abuse!" ), disp_name() );
    }
    // NPCs leaving your faction due to mistreatment further reduce their opinion of you
    if( my_fac->likes_u < -10 ) {
        op_of_u.trust += my_fac->respects_u / 10;
        op_of_u.anger += my_fac->likes_u / 10;
    }
    // NPCs leaving your faction for abuse reduce the hatred your (remaining) followers
    // feel for you, but also reduces their respect for you.
    my_fac->likes_u = std::max( 0, my_fac->likes_u / 2 + 10 );
    my_fac->respects_u -= 5;
    my_fac->trusts_u -= 5;
    g->remove_npc_follower( getID() );
    set_fac( faction_amf );
    job.clear_all_priorities();
    if( assigned_camp ) {
        assigned_camp = std::nullopt;
    }
    chatbin.first_topic = chatbin.talk_stranger_neutral;
    set_attitude( NPCATT_NULL );
    say( _( "<follower_mutiny>  Adios, motherfucker!" ), sounds::sound_t::order );
    if( seen ) {
        my_fac->known_by_u = true;
    }
}

float npc::vehicle_danger( int radius ) const
{
    const tripoint from( posx() - radius, posy() - radius, posz() );
    const tripoint to( posx() + radius, posy() + radius, posz() );
    VehicleList vehicles = get_map().get_vehicles( from, to );

    int danger = 0;

    // TODO: check for most dangerous vehicle?
    for( size_t i = 0; i < vehicles.size(); ++i ) {
        const wrapped_vehicle &wrapped_veh = vehicles[i];
        if( wrapped_veh.v->is_moving() ) {
            // FIXME: this can't be the right way to do this
            units::angle facing = wrapped_veh.v->face.dir();

            point a( wrapped_veh.v->global_pos3().xy() );
            point b( static_cast<int>( a.x + units::cos( facing ) * radius ),
                     static_cast<int>( a.y + units::sin( facing ) * radius ) );

            // fake size
            /* This will almost certainly give the wrong size/location on customized
             * vehicles. This should just count frames instead. Or actually find the
             * size. */
            vehicle_part last_part;
            // vehicle_part_range is a forward only iterator, see comment in vpart_range.h
            for( const vpart_reference &vpr : wrapped_veh.v->get_all_parts() ) {
                last_part = vpr.part();
            }
            int size = std::max( last_part.mount.x, last_part.mount.y );

            double normal = std::sqrt( static_cast<float>( ( b.x - a.x ) * ( b.x - a.x ) + ( b.y - a.y ) *
                                       ( b.y - a.y ) ) );
            int closest = static_cast<int>( std::abs( ( posx() - a.x ) * ( b.y - a.y ) - ( posy() - a.y ) *
                                            ( b.x - a.x ) ) / normal );

            if( size > closest ) {
                danger = i;
            }
        }
    }
    return danger;
}

bool npc::turned_hostile() const
{
    return op_of_u.anger >= hostile_anger_level();
}

int npc::hostile_anger_level() const
{
    return 20 + op_of_u.fear - personality.aggression;
}

void npc::make_angry()
{
    if( is_enemy() ) {
        return; // We're already angry!
    }

    // player allies that become angry should stop being player allies
    if( is_player_ally() ) {
        mutiny();
    }

    // Make associated faction, if any, angry at the player too.
    if( my_fac && my_fac->id != faction_no_faction && my_fac->id != faction_amf ) {
        my_fac->likes_u = std::min( -15, my_fac->likes_u - 5 );
        my_fac->respects_u = std::min( -15, my_fac->respects_u - 5 );
        my_fac->trusts_u = std::min( -15, my_fac->trusts_u - 5 );
    }
    if( op_of_u.fear > 10 + personality.aggression + personality.bravery ) {
        set_attitude( NPCATT_FLEE_TEMP ); // We don't want to take u on!
    } else {
        set_attitude( NPCATT_KILL ); // Yeah, we think we could take you!
    }
}

void npc::on_attacked( const Creature &attacker )
{
    if( is_hallucination() ) {
        die( nullptr );
    }
    if( attacker.is_avatar() && !is_enemy() && !is_dead() ) {
        make_angry();
        hit_by_player = true;
    }
}

int npc::assigned_missions_value()
{
    int ret = 0;
    for( ::mission *m : chatbin.missions_assigned ) {
        ret += m->get_value();
    }
    return ret;
}

void npc::decide_needs()
{
    const item_location weapon = get_wielded_item();
    std::array<double, num_needs> needrank;
    for( double &elem : needrank ) {
        elem = 20;
    }
    if( weapon && weapon->is_gun() ) {
        units::energy energy_drain = weapon->get_gun_energy_drain();
        if( energy_drain > 0_kJ ) {
            units::energy energy_charges = weapon->energy_remaining( this );
            needrank[need_ammo] = static_cast<double>( energy_charges / energy_drain );
        } else {
            const ammotype ammo_type = weapon->ammo_type();
            if( ammo_type != ammotype::NULL_ID() ) {
                needrank[need_ammo] = get_ammo( ammo_type ).size();
            }
        }
        needrank[need_ammo] *= 5;
    }
    if( !base_location ) {
        needrank[need_safety] = 1;
    }

    const item &weap = weapon ? *weapon : null_item_reference();
    needrank[need_weapon] = weapon_value( weap );
    needrank[need_food] = 15 - get_hunger();
    needrank[need_drink] = 15 - get_thirst();
    cache_visit_items_with( "is_food", &item::is_food, [&]( const item & it ) {
        needrank[ need_food ] += nutrition_for( it ) / 4.0;
        needrank[ need_drink ] += it.get_comestible()->quench / 4.0;
    } );
    needs.clear();
    size_t j;
    bool serious = false;
    for( int i = 1; i < num_needs; i++ ) {
        if( needrank[i] < 10 ) {
            serious = true;
        }
    }
    if( !serious ) {
        needs.push_back( need_none );
        needrank[0] = 10;
    }
    for( int i = 1; i < num_needs; i++ ) {
        if( needrank[i] < 20 ) {
            for( j = 0; j < needs.size(); j++ ) {
                if( needrank[i] < needrank[needs[j]] ) {
                    needs.insert( needs.begin() + j, static_cast<npc_need>( i ) );
                    j = needs.size() + 1;
                }
            }
            if( j == needs.size() ) {
                needs.push_back( static_cast<npc_need>( i ) );
            }
        }
    }
}

void npc::say( const std::string &line, const sounds::sound_t spriority ) const
{
    std::string formatted_line = line;
    Character &player_character = get_player_character();
    parse_tags( formatted_line, player_character, *this );
    if( has_trait( trait_MUTE ) ) {
        return;
    }

    if( player_character.is_deaf() && !player_character.is_blind() ) {
        add_msg_if_player_sees( *this, m_warning, _( "%1$s says something but you can't hear it!" ),
                                get_name() );
    }
    if( player_character.is_mute() ) {
        add_msg_if_player_sees( *this, m_warning, _( "%1$s says something but you can't reply to it!" ),
                                get_name() );
    }
    // Hallucinations don't make noise when they speak
    if( is_hallucination() ) {
        add_msg( _( "%1$s saying \"%2$s\"" ), get_name(), formatted_line );
        return;
    }

    std::string sound = string_format( _( "%1$s saying \"%2$s\"" ), get_name(), formatted_line );

    // Sound happens even if we can't hear it
    if( spriority == sounds::sound_t::order || spriority == sounds::sound_t::alert ) {
        sounds::sound( pos(), get_shout_volume(), spriority, sound, false, "speech",
                       male ? "NPC_m" : "NPC_f" );
    } else {
        sounds::sound( pos(), 16, sounds::sound_t::speech, sound, false, "speech",
                       male ? "NPC_m_loud" : "NPC_f_loud" );
    }
}

bool npc::wants_to_sell( const item_location &it ) const
{
    if( !it->is_owned_by( *this ) ) {
        return false;
    }
    return wants_to_sell( it, value( *it ) ).success();
}

ret_val<void> npc::wants_to_sell( const item_location &it, int at_price ) const
{
    if( will_exchange_items_freely() ) {
        return ret_val<void>::make_success();
    }

    // Keep items that we never want to trade and the ones we don't want to trade while in use.
    if( it->has_flag( flag_TRADER_KEEP ) ||
        is_worn( *it ) ||
        ( ( !myclass->sells_belongings || it->has_flag( flag_TRADER_KEEP_EQUIPPED ) ) &&
          it.held_by( *this ) ) ) {
        return ret_val<void>::make_failure( _( "<npcname> will never sell this" ) );
    }

    for( const shopkeeper_item_group &ig : myclass->get_shopkeeper_items() ) {
        if( ig.can_sell( *this ) ) {
            continue;
        }
        item const *const check_it = it->this_or_single_content();
        if( item_group::group_contains_item( ig.id, check_it->typeId() ) ) {
            return ret_val<void>::make_failure( ig.get_refusal() );
        }
    }

    // TODO: Base on inventory
    return at_price >= 0 ? ret_val<void>::make_success() : ret_val<void>::make_failure();
}

bool npc::wants_to_buy( const item &it ) const
{
    return wants_to_buy( it, value( it ) ).success();
}

ret_val<void> npc::wants_to_buy( const item &it, int at_price ) const
{
    if( it.has_flag( flag_DANGEROUS ) || ( it.has_flag( flag_BOMB ) && it.active ) ) {
        return ret_val<void>::make_failure();
    }

    if( will_exchange_items_freely() ) {
        return ret_val<void>::make_success();
    }

    if( it.has_flag( flag_TRADER_AVOID ) || it.has_var( VAR_TRADE_IGNORE ) ) {
        return ret_val<void>::make_failure( _( "<npcname> will never buy this" ) );
    }

    if( !is_shopkeeper() && has_trait( trait_SQUEAMISH ) && it.is_filthy() ) {
        return ret_val<void>::make_failure( _( "<npcname> will not buy filthy items" ) );
    }

    icg_entry const *bl = myclass->get_shopkeeper_blacklist().matches( it, *this );
    if( bl != nullptr ) {
        return ret_val<void>::make_failure( bl->message );
    }

    // TODO: Base on inventory
    return at_price >= 0 ? ret_val<void>::make_success() : ret_val<void>::make_failure();
}

// Will the NPC freely exchange items with the player?
bool npc::will_exchange_items_freely() const
{
    return is_player_ally();
}

// What's the maximum credit the NPC is willing to extend to the player?
// This is currently very scrooge-like; NPCs are only likely to extend a few dollars
// of credit at most.
int npc::max_credit_extended() const
{
    if( is_player_ally() ) {
        return INT_MAX;
    }

    const int credit_trust    = 50;
    const int credit_value    = 50;
    const int credit_fear     = 50;
    const int credit_altruism = 100;
    const int credit_anger    = -200;

    return std::max( 0,
                     op_of_u.trust * credit_trust +
                     op_of_u.value * credit_value +
                     op_of_u.fear  * credit_fear  +
                     personality.altruism * credit_altruism +
                     op_of_u.anger * credit_anger
                   );
}

// How much is the NPC willing to owe the player?
// This is much more generous, as it's the essentially the player holding the risk here.
int npc::max_willing_to_owe() const
{
    if( is_player_ally() ) {
        return INT_MAX;
    }

    const int credit_trust    = 10000;
    const int credit_value    = 10000;
    const int credit_fear     = 10000;
    const int credit_altruism = 10000;
    const int credit_anger    = -10000;
    const int credit_default  = 10000;

    // NPCs will usually be happy to owe at least credit_default, but may be willing to owe
    // more if they trust, value, are fearful, or altruistic.
    // Angry NPCs could conceiveably refuse to owe you money, out of spite.
    return std::max( 0,
                     credit_default +
                     std::max( 0, op_of_u.trust ) * credit_trust +
                     std::max( 0, op_of_u.value ) * credit_value +
                     std::max( 0, op_of_u.fear )  * credit_fear  +
                     std::max( 0, static_cast<int>( personality.altruism ) ) * credit_altruism +
                     op_of_u.anger * credit_anger
                   );

}

void npc::shop_restock()
{
    // Shops restock once every restock_interval
    time_duration const elapsed =
        restock != calendar::turn_zero ? calendar::turn - restock : 0_days;
    if( ( restock != calendar::turn_zero ) && ( elapsed < 0_days ) ) {
        return;
    }

    if( is_player_ally() || !is_shopkeeper() ) {
        return;
    }
    restock = calendar::turn + myclass->get_shop_restock_interval();

    std::vector<item_group_id> rigid_groups;
    std::vector<item_group_id> value_groups;
    for( const shopkeeper_item_group &ig : myclass->get_shopkeeper_items() ) {
        if( ig.can_restock( *this ) ) {
            if( ig.rigid ) {
                rigid_groups.emplace_back( ig.id );
            } else {
                value_groups.emplace_back( ig.id );
            }
        }
    }

    std::list<item> ret;
    int shop_value = 75000;
    if( my_fac ) {
        shop_value = my_fac->wealth * 0.0075;
        if( !my_fac->currency.is_empty() ) {
            item my_currency( my_fac->currency );
            if( !my_currency.is_null() ) {
                my_currency.set_owner( *this );
                int my_amount = rng( 5, 15 ) * shop_value / 100 / my_currency.price( true );
                for( int lcv = 0; lcv < my_amount; lcv++ ) {
                    ret.push_back( my_currency );
                }
            }
        }
    }

    // First, populate trade goods using rigid groups.
    // Rigid groups are always processed a single time, regardless of the shopkeeper's inventory size or desired total value of goods.
    for( const item_group_id &rg : rigid_groups ) {
        item_group::ItemList rigid_items = item_group::items_from( rg, calendar::turn );
        if( !rigid_items.empty() ) {
            for( item &tmpit : rigid_items ) {
                if( !tmpit.is_null() ) {
                    tmpit.set_owner( *this );
                    ret.push_back( tmpit );
                }
            }
        }
    }

    // Then, populate with valued groups.
    // Value groups will run many times until the shopkeeper's total space is full or "shop_value"  is completely spent.
    if( !value_groups.empty() ) {
        int count = 0;
        bool last_item = false;
        while( shop_value > 0 && !last_item ) {
            item tmpit = item_group::item_from( random_entry( value_groups ), calendar::turn );
            if( !tmpit.is_null() ) {
                tmpit.set_owner( *this );
                ret.push_back( tmpit );
                shop_value -= tmpit.price( true );
                count += 1;
                last_item = count > 10 && one_in( 100 );
            }
        }

        // This removes some items according to item spawn scaling factor
        const float spawn_rate = get_option<float>( "ITEM_SPAWNRATE" );
        if( spawn_rate < 1 ) {
            ret.remove_if( [spawn_rate]( auto & ) {
                return !( rng_float( 0, 1 ) < spawn_rate );
            } );
        }
    }

    add_fallback_zone( *this );
    consume_items_in_zones( *this, elapsed );
    distribute_items_to_npc_zones( ret, *this );
}

std::string npc::get_restock_interval() const
{
    time_duration const restock_remaining =
        restock - calendar::turn;
    std::string restock_rem = to_string( restock_remaining );
    return restock_rem;
}

bool npc::is_shopkeeper() const
{
    return !is_player_ally() && !myclass->get_shopkeeper_items().empty();
}

int npc::minimum_item_value() const
{
    // TODO: Base on inventory
    int ret = 20;
    ret -= personality.collector;
    return ret;
}

void npc::update_worst_item_value()
{
    worst_item_value = 99999;
    // TODO: Cache this
    int inv_val = inv->worst_item_value( this );
    if( inv_val < worst_item_value ) {
        worst_item_value = inv_val;
    }
}

double npc::value( const item &it ) const
{
    if( it.is_dangerous() || ( it.has_flag( flag_BOMB ) && it.active ) ) {
        return -1000;
    }

    int market_price = it.price( true );
    return value( it, market_price );
}

double npc::value( const item &it, double market_price ) const
{
    if( is_shopkeeper() ||
        // faction currency trades at market price
        ( my_fac != nullptr && my_fac->currency == it.typeId() ) ) {
        return market_price;
    }

    const item_location weapon = get_wielded_item();
    float ret = 1;
    if( it.is_maybe_melee_weapon() || it.is_gun() ) {
        // todo: remove when weapon_value takes an item_location
        double wield_val = weapon ? weapon_value( *weapon ) : weapon_value( null_item_reference() );
        double weapon_val = weapon_value( it ) - wield_val;

        if( weapon_val > 0 ) {
            ret += weapon_val * 0.0002;
        }
    }

    if( it.is_food() && will_eat( it ).success() ) {
        int const kcal_need = get_healthy_kcal() - get_stored_kcal() + stomach.get_calories();
        int const quench_need = get_thirst();
        if( kcal_need > compute_effective_nutrients( it ).kcal() * 2 ) {
            ret += std::min( 3.0, 0.00005 * kcal_need );
        }
        if( quench_need > it.get_comestible()->quench * 2 ) {
            ret += std::min( 3.0, 0.0055 * quench_need );
        }
    } else if( it.is_ammo() ) {
        // TODO: magazines - don't count ammo as usable if the weapon isn't.
        if( ( weapon && weapon->is_gun() && weapon->ammo_types().count( it.ammo_type() ) ) ||
            has_gun_for_ammo( it.ammo_type() ) ) {
            ret += 0.2;
        }
    } else if( it.is_book() ) {
        islot_book &book = *it.type->book;
        ret += book.fun * 0.01;
        int const skill = get_knowledge_level( book.skill );
        if( book.skill && skill < book.level && skill >= book.req ) {
            ret += ( book.level - skill ) * 0.1;
        }
    } else if( it.is_tool() && !has_amount( it.typeId(), 1 ) ) {
        // TODO: Sometimes we want more than one tool?  Also we don't want EVERY tool.
        ret += 0.1;
    }
    return std::round( ret * market_price );
}

faction_price_rule const *npc::get_price_rules( item const &it ) const
{
    faction_price_rule const *ret = myclass->get_price_rules( it, *this );
    if( ret == nullptr && get_faction() != nullptr ) {
        ret = get_faction()->get_price_rules( it, *this );
    }
    return ret;
}

void healing_options::clear_all()
{
    bandage = false;
    disinfect = false;
    bleed = false;
    bite = false;
    infect = false;
}

bool healing_options::all_false() const
{
    return !any_true();
}

bool healing_options::any_true() const
{
    return bandage || bleed || bite || infect || disinfect;
}

void healing_options::set_all()
{
    bandage = true;
    bleed = true;
    bite = true;
    infect = true;
    disinfect = true;
}

bool npc::has_healing_item( healing_options try_to_fix )
{
    return !get_healing_item( try_to_fix, true ).is_null();
}

healing_options npc::has_healing_options()
{
    healing_options try_to_fix;
    try_to_fix.set_all();
    return has_healing_options( try_to_fix );
}

healing_options npc::has_healing_options( healing_options try_to_fix )
{
    healing_options can_fix;
    can_fix.clear_all();
    healing_options *fix_p = &can_fix;

    visit_items( [&fix_p, try_to_fix]( item * node, item * ) {
        const use_function *use = node->type->get_use( "heal" );
        if( use == nullptr ) {
            return VisitResponse::NEXT;
        }

        const heal_actor &actor = dynamic_cast<const heal_actor &>( *use->get_actor_ptr() );
        if( try_to_fix.bandage && !fix_p->bandage && actor.bandages_power > 0.0f ) {
            fix_p->bandage = true;
        }
        if( try_to_fix.disinfect && !fix_p->disinfect && actor.disinfectant_power > 0.0f ) {
            fix_p->disinfect = true;
        }
        if( try_to_fix.bleed && !fix_p->bleed && actor.bleed > 0 ) {
            fix_p->bleed = true;
        }
        if( try_to_fix.bite && !fix_p->bite && actor.bite > 0 ) {
            fix_p->bite = true;
        }
        if( try_to_fix.infect && !fix_p->infect && actor.infect > 0 ) {
            fix_p->infect = true;
        }
        // if we've found items for everything we're looking for, we're done
        if( ( !try_to_fix.bandage || fix_p->bandage ) &&
            ( !try_to_fix.disinfect || fix_p->disinfect ) &&
            ( !try_to_fix.bleed || fix_p->bleed ) &&
            ( !try_to_fix.bite || fix_p->bite ) &&
            ( !try_to_fix.infect || fix_p->infect ) ) {
            return VisitResponse::ABORT;
        }

        return VisitResponse::NEXT;
    } );
    return can_fix;
}

item &npc::get_healing_item( healing_options try_to_fix, bool first_best )
{
    item *best = &null_item_reference();
    visit_items( [&best, try_to_fix, first_best]( item * node, item * ) {
        const use_function *use = node->type->get_use( "heal" );
        if( use == nullptr ) {
            return VisitResponse::NEXT;
        }

        const heal_actor &actor = dynamic_cast<const heal_actor &>( *use->get_actor_ptr() );
        if( ( try_to_fix.bandage && actor.bandages_power > 0.0f ) ||
            ( try_to_fix.disinfect && actor.disinfectant_power > 0.0f ) ||
            ( try_to_fix.bleed && actor.bleed > 0 ) ||
            ( try_to_fix.bite && actor.bite > 0 ) ||
            ( try_to_fix.infect && actor.infect > 0 ) ) {
            best = node;
            if( first_best ) {
                return VisitResponse::ABORT;
            }
        }

        return VisitResponse::NEXT;
    } );

    return *best;
}

bool npc::has_painkiller()
{
    return inv->has_enough_painkiller( get_pain() );
}

bool npc::took_painkiller() const
{
    return has_effect( effect_pkill1 ) || has_effect( effect_pkill2 ) ||
           has_effect( effect_pkill3 ) || has_effect( effect_pkill_l );
}

int npc::get_faction_ver() const
{
    return faction_api_version;
}

void npc::set_faction_ver( int new_version )
{
    faction_api_version = new_version;
}

bool npc::has_faction_relationship( const Character &you,
                                    const npc_factions::relationship flag ) const
{
    faction *p_fac = you.get_faction();
    if( !my_fac || !p_fac ) {
        return false;
    }

    return my_fac->has_relationship( p_fac->id, flag );
}

bool npc::is_ally( const Character &p ) const
{
    if( p.getID() == getID() ) {
        return true;
    }
    if( p.is_avatar() ) {
        if( my_fac && my_fac->id == faction_your_followers ) {
            return true;
        }
        if( faction_api_version < 2 ) {
            // legacy attitude support so let's be specific here
            if( attitude == NPCATT_FOLLOW || attitude == NPCATT_LEAD ||
                attitude == NPCATT_WAIT || mission == NPC_MISSION_ACTIVITY ||
                mission == NPC_MISSION_TRAVELLING || mission == NPC_MISSION_GUARD_ALLY ||
                has_companion_mission() ) {
                return true;
            }
        }
    } else {
        const npc &guy = dynamic_cast<const npc &>( p );
        if( my_fac && guy.get_faction() && my_fac->id == guy.get_faction()->id ) {
            return true;
        }
        if( faction_api_version < 2 ) {
            Character &player_character = get_player_character();
            if( is_ally( player_character ) && guy.is_ally( player_character ) ) {
                return true;
            }
            if( get_attitude_group( get_attitude() ) ==
                guy.get_attitude_group( guy.get_attitude() ) ) {
                return true;
            }
        }
    }
    return false;
}

bool npc::is_player_ally() const
{
    return is_ally( get_player_character() );
}

bool npc::is_friendly( const Character &p ) const
{
    return is_ally( p ) || ( p.is_avatar() && ( is_walking_with() || is_player_ally() ) );
}

bool npc::is_minion() const
{
    return is_player_ally() && op_of_u.trust >= 5;
}

bool npc::guaranteed_hostile() const
{
    return is_enemy() || ( my_fac && my_fac->likes_u < -10 );
}

bool npc::is_walking_with() const
{
    return attitude == NPCATT_FOLLOW || attitude == NPCATT_LEAD || attitude == NPCATT_WAIT;
}

bool npc::is_obeying( const Character &p ) const
{
    return is_ally( p ) && ( ( p.is_avatar() && is_walking_with() ) || is_stationary( true ) );
}

bool npc::is_following() const
{
    return attitude == NPCATT_FOLLOW || attitude == NPCATT_WAIT;
}

bool npc::is_leader() const
{
    return attitude == NPCATT_LEAD;
}

bool npc::within_boundaries_of_camp() const
{
    const point_abs_omt p( global_omt_location().xy() );
    for( int x2 = -3; x2 < 3; x2++ ) {
        for( int y2 = -3; y2 < 3; y2++ ) {
            const point_abs_omt nearby = p + point( x2, y2 );
            std::optional<basecamp *> bcp = overmap_buffer.find_camp( nearby );
            if( bcp ) {
                return true;
            }
        }
    }
    return false;
}

bool npc::is_enemy() const
{
    return attitude == NPCATT_KILL || attitude == NPCATT_FLEE || attitude == NPCATT_FLEE_TEMP;
}

bool npc::is_stationary( bool include_guards ) const
{
    if( include_guards && is_guarding() ) {
        return true;
    }
    return mission == NPC_MISSION_SHELTER || mission == NPC_MISSION_SHOPKEEP ||
           has_effect( effect_infection );
}

bool npc::is_guarding( ) const
{
    return mission == NPC_MISSION_GUARD || mission == NPC_MISSION_GUARD_ALLY || is_patrolling();
}

bool npc::is_patrolling() const
{
    return mission == NPC_MISSION_GUARD_PATROL;
}

bool npc::has_player_activity() const
{
    return activity && mission == NPC_MISSION_ACTIVITY && attitude == NPCATT_ACTIVITY;
}

bool npc::is_travelling() const
{
    return mission == NPC_MISSION_TRAVELLING;
}

Creature::Attitude npc::attitude_to( const Creature &other ) const
{
    const auto same_as = []( const Creature * lhs, const Creature * rhs ) {
        return &lhs == &rhs;
    };

    for( const weak_ptr_fast<Creature> &buddy : ai_cache.friends ) {
        if( same_as( &other, buddy.lock().get() ) ) {
            return Creature::Attitude::FRIENDLY;
        }
    }
    for( const weak_ptr_fast<Creature> &enemy : ai_cache.hostile_guys ) {
        if( same_as( &other, enemy.lock().get() ) ) {
            return Creature::Attitude::HOSTILE;
        }
    }
    for( const weak_ptr_fast<Creature> &neutral : ai_cache.neutral_guys ) {
        if( same_as( &other, neutral.lock().get() ) ) {
            return Creature::Attitude::NEUTRAL;
        }
    }

    if( other.is_npc() || other.is_avatar() ) {
        const Character &guy = dynamic_cast<const Character &>( other );
        // check faction relationships first
        if( has_faction_relationship( guy, npc_factions::kill_on_sight ) ) {
            return Attitude::HOSTILE;
        } else if( has_faction_relationship( guy, npc_factions::watch_your_back ) ) {
            return Attitude::FRIENDLY;
        }
    }

    Character &player_character = get_player_character();
    if( is_player_ally() ) {
        // Friendly NPCs share player's alliances
        return player_character.attitude_to( other );
    }

    if( other.is_npc() ) {
        // Hostile NPCs are also hostile towards player's allies
        if( is_enemy() && other.attitude_to( player_character ) == Attitude::FRIENDLY ) {
            return Attitude::HOSTILE;
        }

        return Attitude::NEUTRAL;
    } else if( other.is_avatar() ) {
        // For now, make it symmetric.
        return other.attitude_to( *this );
    }

    // TODO: Get rid of the ugly cast without duplicating checks
    const monster &m = dynamic_cast<const monster &>( other );
    switch( m.attitude( this ) ) {
        case MATT_FOLLOW:
        case MATT_FPASSIVE:
        case MATT_IGNORE:
        case MATT_FLEE:
            return Attitude::NEUTRAL;
        case MATT_FRIEND:
            return Attitude::FRIENDLY;
        case MATT_ATTACK:
            return Attitude::HOSTILE;
        case MATT_NULL:
        case NUM_MONSTER_ATTITUDES:
            break;
    }

    return Attitude::NEUTRAL;
}

void npc::npc_dismount()
{
    if( !mounted_creature || !has_effect( effect_riding ) ) {
        add_msg_debug( debugmode::DF_NPC,
                       "NPC %s tried to dismount, but they have no mount, or they are not riding",
                       disp_name() );
        return;
    }
    std::optional<tripoint> pnt;
    for( const tripoint &elem : get_map().points_in_radius( pos(), 1 ) ) {
        if( g->is_empty( elem ) ) {
            pnt = elem;
            break;
        }
    }
    if( !pnt ) {
        add_msg_debug( debugmode::DF_NPC, "NPC %s could not find a place to dismount.", disp_name() );
        return;
    }
    remove_effect( effect_riding );
    if( mounted_creature->has_flag( mon_flag_RIDEABLE_MECH ) &&
        !mounted_creature->type->mech_weapon.is_empty() ) {
        get_wielded_item().remove_item();
    }
    mounted_creature->remove_effect( effect_ridden );
    mounted_creature->add_effect( effect_controlled, 5_turns );
    mounted_creature = nullptr;
    setpos( *pnt );
    mod_moves( -100 );
}

int npc::smash_ability() const
{
    if( !is_hallucination() && ( !is_player_ally() || rules.has_flag( ally_rule::allow_bash ) ) ) {
        ///\EFFECT_STR_NPC increases smash ability
        int dmg = get_wielded_item() ? get_wielded_item()->damage_melee( STATIC(
                      damage_type_id( "bash" ) ) ) : 0;
        return str_cur + dmg;
    }

    // Not allowed to bash
    return 0;
}

float npc::danger_assessment() const
{
    return ai_cache.danger_assessment;
}

float npc::average_damage_dealt()
{
    item &weap = get_wielded_item() ? *get_wielded_item() : null_item_reference();
    return static_cast<float>( melee_value( weap ) );
}

bool npc::bravery_check( int diff ) const
{
    return dice( 10 + personality.bravery, 6 ) >= dice( diff, 4 );
}

bool npc::emergency() const
{
    return emergency( ai_cache.danger_assessment );
}

bool npc::emergency( float danger ) const
{
    return danger > ( personality.bravery * 3 * hp_percentage() ) / 100.0;
}

//Check if this npc is currently in the list of active npcs.
//Active npcs are the npcs near the player that are actively simulated.
bool npc::is_active() const
{
    return get_creature_tracker().creature_at<npc>( pos() ) == this;
}

int npc::follow_distance() const
{
    Character &player_character = get_player_character();
    map &here = get_map();
    // HACK: If the player is standing on stairs, follow closely
    // This makes the stair hack less painful to use
    if( is_walking_with() &&
        ( here.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, player_character.pos() ) ||
          here.has_flag( ter_furn_flag::TFLAG_GOES_UP, player_character.pos() ) ) ) {
        return 1;
    }
    // Uses ally_rule follow_distance_2 to determine if should follow by 2 or 4 tiles
    if( rules.has_flag( ally_rule::follow_distance_2 ) ) {
        return 2;
    }
    // If NPC doesn't see player, change follow distance to 2
    if( !sees( player_character ) ) {
        return 2;
    }
    return 4;
}

nc_color npc::basic_symbol_color() const
{
    if( attitude == NPCATT_KILL ) { // NOLINT(bugprone-branch-clone)
        return c_red;
    } else if( attitude == NPCATT_FLEE || attitude == NPCATT_FLEE_TEMP ) {
        return c_light_red;
    } else if( is_player_ally() ) {
        return c_green;
    } else if( is_walking_with() ) {
        return c_light_green;
    } else if( guaranteed_hostile() ) {
        return c_red;
    }
    return c_pink;
}

int npc::print_info( const catacurses::window &w, int line, int vLines, int column ) const
{
    const int last_line = line + vLines;
    const int iWidth = getmaxx( w ) - 1 - column;
    // First line of w is the border; the next 4 are terrain info, and after that
    // is a blank line. w is 13 characters tall, and we can't use the last one
    // because it's a border as well; so we have lines 6 through 11.
    // w is also 53 characters wide - 2 characters for border = 51 characters for us

    // Print health bar and NPC name on the first line.
    std::pair<std::string, nc_color> bar = get_hp_bar( hp_percentage(), 100 );
    mvwprintz( w, point( column, line ), bar.second, bar.first );
    const int bar_max_width = 5;
    const int bar_width = utf8_width( bar.first );
    for( int i = 0; i < bar_max_width - bar_width; ++i ) {
        mvwprintz( w, point( column + 4 - i, line ), c_white, "." );
    }
    line += fold_and_print( w, point( column + bar_max_width + 1, line ),
                            iWidth - bar_max_width - 1, basic_symbol_color(), get_name() );

    Character &player_character = get_player_character();
    // Hostility and current attitude indicator on the second line.
    Attitude att = attitude_to( player_character );
    const std::pair<translation, nc_color> res = Creature::get_attitude_ui_data( att );
    mvwprintz( w, point( column, line++ ), res.second, res.first.translated() );
    wprintz( w, c_light_gray, ";" );
    wprintz( w, symbol_color(), " %s", npc_attitude_name( get_attitude() ) );

    // Awareness indicator on the third line.
    std::string senses_str = sees( player_character ) ? _( "Aware of your presence" ) :
                             _( "Unaware of you" );
    line += fold_and_print( w, point( column, line ), iWidth,
                            sees( player_character ) ? c_yellow : c_green,
                            senses_str );

    // Print what item the NPC is holding if any on the fourth line.
    if( is_armed() ) {
        line += fold_and_print( w, point( column, line ), iWidth, c_red,
                                std::string( "<color_light_gray>" ) + _( "Wielding: " ) + std::string( "</color>" ) +
                                get_wielded_item()->tname() );
    }

    // Worn gear list on following lines.
    const std::list<item> visible_worn_items = get_visible_worn_items();
    const std::string worn_str = enumerate_as_string( visible_worn_items.begin(),
    visible_worn_items.end(), []( const item & it ) {
        return it.tname();
    } );
    if( !worn_str.empty() ) {
        std::vector<std::string> worn_lines = foldstring( _( "Wearing: " ) + worn_str, iWidth );
        int worn_numlines = worn_lines.size();
        for( int i = 0; i < worn_numlines && line < last_line; i++ ) {
            if( line + 1 == last_line ) {
                worn_lines[i].append( "…" );
            }
            trim_and_print( w, point( column, line++ ), iWidth, c_light_gray, worn_lines[i] );
        }
    }

    if( line == last_line ) {
        return line;
    }

    int visibility_cap = player_character.get_mutation_visibility_cap( this );
    const std::string trait_str = visible_mutations( visibility_cap );
    if( !trait_str.empty() ) {
        std::vector<std::string> trait_lines = foldstring( _( "Traits: " ) + trait_str, iWidth );
        int trait_numlines = trait_lines.size();
        for( int i = 0; i < trait_numlines && line < last_line; i++ ) {
            if( line + 1 == last_line ) {
                trait_lines[i].append( "…" );
            }
            trim_and_print( w, point( column, line++ ), iWidth, c_light_gray, trait_lines[i] );
        }
    }

    return line;
}

std::string npc::opinion_text() const
{
    std::string ret;
    std::string desc;

    if( op_of_u.trust <= -10 ) {
        desc = _( "Completely untrusting" );
    } else if( op_of_u.trust <= -6 ) {
        desc = _( "Very untrusting" );
    } else if( op_of_u.trust <= -3 ) {
        desc = _( "Untrusting" );
    } else if( op_of_u.trust <= 2 ) {
        desc = _( "Uneasy" );
    } else if( op_of_u.trust <= 4 ) {
        desc = _( "Trusting" );
    } else if( op_of_u.trust < 10 ) {
        desc = _( "Very trusting" );
    } else {
        desc = _( "Completely trusting" );
    }

    ret += string_format( _( "Trust: %d (%s);\n" ), op_of_u.trust, desc );

    if( op_of_u.fear <= -10 ) {
        desc = _( "Thinks you're laughably harmless" );
    } else if( op_of_u.fear <= -6 ) {
        desc = _( "Thinks you're harmless" );
    } else if( op_of_u.fear <= -3 ) {
        desc = _( "Unafraid" );
    } else if( op_of_u.fear <= 2 ) {
        desc = _( "Wary" );
    } else if( op_of_u.fear <= 5 ) {
        desc = _( "Afraid" );
    } else if( op_of_u.fear < 10 ) {
        desc = _( "Very afraid" );
    } else {
        desc = _( "Terrified" );
    }

    ret += string_format( _( "Fear: %d (%s);\n" ), op_of_u.fear, desc );

    if( op_of_u.value <= -10 ) {
        desc = _( "Considers you a major liability" );
    } else if( op_of_u.value <= -6 ) {
        desc = _( "Considers you a burden" );
    } else if( op_of_u.value <= -3 ) {
        desc = _( "Considers you an annoyance" );
    } else if( op_of_u.value <= 2 ) {
        desc = _( "Doesn't care about you" );
    } else if( op_of_u.value <= 5 ) {
        desc = _( "Values your presence" );
    } else if( op_of_u.value < 10 ) {
        desc = _( "Treasures you" );
    } else {
        desc = _( "Best Friends Forever!" );
    }

    ret += string_format( _( "Value: %d (%s);\n" ), op_of_u.value, desc );

    if( op_of_u.anger <= -10 ) {
        desc = _( "You can do no wrong!" );
    } else if( op_of_u.anger <= -6 ) {
        desc = _( "You're good people" );
    } else if( op_of_u.anger <= -3 ) {
        desc = _( "Thinks well of you" );
    } else if( op_of_u.anger <= 2 ) {
        desc = _( "Ambivalent" );
    } else if( op_of_u.anger <= 5 ) {
        desc = _( "Pissed off" );
    } else if( op_of_u.anger < 10 ) {
        desc = _( "Angry" );
    } else {
        desc = _( "About to kill you" );
    }

    ret += string_format( _( "Anger: %d (%s)." ), op_of_u.anger, desc );

    return ret;
}

static void maybe_shift( tripoint &pos, const point &d )
{
    if( pos != tripoint_min ) {
        pos += d;
    }
}

void npc::shift( const point &s )
{
    const point shift = sm_to_ms_copy( s );
    // TODO: convert these to absolute coords and get rid of shift()
    maybe_shift( wanted_item_pos, point( -shift.x, -shift.y ) );
    path.clear();
}

bool npc::is_dead() const
{
    return dead || is_dead_state();
}

void npc::reboot()
{
    //The NPC got into an infinite loop, in game.cpp  -monmove() - a debugmsg just popped up
    // informing player of this.
    // put them to sleep and reboot their brain.
    // they can be woken up by the player, and if their brain is fixed, great,
    // if not, they will faint again, and the NPC can be kept asleep until the bug is fixed.
    cancel_activity();
    path.clear();
    last_player_seen_pos = std::nullopt;
    last_seen_player_turn = 999;
    wanted_item_pos = tripoint_min;
    guard_pos = std::nullopt;
    goal = no_goal_point;
    fetching_item = false;
    has_new_items = true;
    worst_item_value = 0;
    mission = NPC_MISSION_NULL;
    patience = 0;
    ai_cache.danger = 0;
    ai_cache.total_danger = 0;
    ai_cache.danger_assessment = 0;
    ai_cache.target.reset();
    ai_cache.ally.reset();
    ai_cache.can_heal.clear_all();
    ai_cache.sound_alerts.clear();
    ai_cache.s_abs_pos = tripoint_zero;
    ai_cache.stuck = 0;
    ai_cache.guard_pos = std::nullopt;
    ai_cache.my_weapon_value = 0;
    ai_cache.friends.clear();
    ai_cache.dangerous_explosives.clear();
    ai_cache.threat_map.clear();
    ai_cache.searched_tiles.clear();
    activity = player_activity();
    clear_destination();
    add_effect( effect_npc_suspend, 24_hours, true, 1 );
}

void npc::die( Creature *nkiller )
{
    if( dead ) {
        // We are already dead, don't die again, note that npc::dead is
        // *only* set to true in this function!
        return;
    }

    prevent_death_reminder = false;
    dialogue d( get_talker_for( this ), nkiller == nullptr ? nullptr : get_talker_for( nkiller ) );
    for( effect_on_condition_id &eoc : death_eocs ) {
        if( eoc->type == eoc_type::NPC_DEATH ) {
            eoc->activate( d );
        } else {
            debugmsg( "Tried to use non NPC_DEATH eoc_type %s for an npc death.", eoc.c_str() );
        }
    }
    get_event_bus().send<event_type::character_dies>( getID() );
    // Check if npc doesn't die due to EoC as a result
    if( prevent_death_reminder ) {
        prevent_death_reminder = false;
        if( !is_dead() ) {
            return;
        }
    }

    if( assigned_camp ) {
        std::optional<basecamp *> bcp = overmap_buffer.find_camp( ( *assigned_camp ).xy() );
        if( bcp ) {
            ( *bcp )->remove_assignee( getID() );
        }
    }
    assigned_camp = std::nullopt;
    // Need to unboard from vehicle before dying, otherwise
    // the vehicle code cannot find us
    if( in_vehicle ) {
        get_map().unboard_vehicle( pos(), true );
    }
    if( is_mounted() ) {
        monster *critter = mounted_creature.get();
        critter->remove_effect( effect_ridden );
        critter->mounted_player = nullptr;
        critter->mounted_player_id = character_id();
    }
    // if this NPC was the only member of a micro-faction, clean it up.
    if( my_fac ) {
        if( !is_fake() && !is_hallucination() ) {
            if( my_fac->members.size() == 1 ) {
                for( const item *elem : inv_dump() ) {
                    elem->remove_owner();
                    elem->remove_old_owner();
                }
            }
            my_fac->remove_member( getID() );
            my_fac = nullptr;
        }
    }
    dead = true;
    Character::die( nkiller );

    if( is_hallucination() || lifespan_end ) {
        add_msg_if_player_sees( *this, _( "%s disappears." ), get_name().c_str() );
        return;
    }

    add_msg_if_player_sees( *this, _( "%s dies!" ), get_name() );

    if( Character *ch = dynamic_cast<Character *>( killer ) ) {
        get_event_bus().send<event_type::character_kills_character>( ch->getID(), getID(), get_name() );
    }

    Character &player_character = get_player_character();
    if( killer == &player_character && ( !guaranteed_hostile() || hit_by_player ) ) {
        bool cannibal = player_character.has_trait( trait_CANNIBAL );
        bool psycho = player_character.has_trait( trait_PSYCHOPATH );
        if( player_character.has_trait( trait_SAPIOVORE ) || psycho ) {
            // No morale effect
        } else if( cannibal ) {
            player_character.add_morale( MORALE_KILLED_INNOCENT, -5, 0, 2_days, 3_hours );
        } else {
            player_character.add_morale( MORALE_KILLED_INNOCENT, -100, 0, 2_days, 3_hours );
        }
    }

    place_corpse();
}

void npc::prevent_death()
{
    marked_for_death = false;
    prevent_death_reminder = true;
    Character::prevent_death();
}

std::string npc_attitude_id( npc_attitude att )
{
    static const std::map<npc_attitude, std::string> npc_attitude_ids = {
        { NPCATT_NULL, "NPCATT_NULL" },
        { NPCATT_TALK, "NPCATT_TALK" },
        { NPCATT_FOLLOW, "NPCATT_FOLLOW" },
        { NPCATT_LEAD, "NPCATT_LEAD" },
        { NPCATT_WAIT, "NPCATT_WAIT" },
        { NPCATT_MUG, "NPCATT_MUG" },
        { NPCATT_WAIT_FOR_LEAVE, "NPCATT_WAIT_FOR_LEAVE" },
        { NPCATT_KILL, "NPCATT_KILL" },
        { NPCATT_FLEE, "NPCATT_FLEE" },
        { NPCATT_FLEE_TEMP, "NPCATT_FLEE_TEMP" },
        { NPCATT_HEAL, "NPCATT_HEAL" },
        { NPCATT_ACTIVITY, "NPCATT_ACTIVITY" },
        { NPCATT_RECOVER_GOODS, "NPCATT_RECOVER_GOODS" },
        { NPCATT_LEGACY_1, "NPCATT_LEGACY_1" },
        { NPCATT_LEGACY_2, "NPCATT_LEGACY_2" },
        { NPCATT_LEGACY_3, "NPCATT_LEGACY_3" },
        { NPCATT_LEGACY_4, "NPCATT_LEGACY_4" },
        { NPCATT_LEGACY_5, "NPCATT_LEGACY_5" },
        { NPCATT_LEGACY_6, "NPCATT_LEGACY_6" },
    };
    const auto &iter = npc_attitude_ids.find( att );
    if( iter == npc_attitude_ids.end() ) {
        debugmsg( "Invalid attitude: %d", att );
        return "NPCATT_INVALID";
    }

    return iter->second;
}

std::optional<int> npc::closest_enemy_to_friendly_distance() const
{
    return ai_cache.closest_enemy_to_friendly_distance();
}

const std::vector<weak_ptr_fast<Creature>> &npc::get_cached_friends() const
{
    return ai_cache.friends;
}

std::string npc_attitude_name( npc_attitude att )
{
    switch( att ) {
        // Don't care/ignoring player
        case NPCATT_NULL:
            return _( "Ignoring" );
        // Move to and talk to player
        case NPCATT_TALK:
            return _( "Wants to talk" );
        // Follow the player
        case NPCATT_FOLLOW:
            return _( "Following" );
        // Lead the player, wait for them if they're behind
        case NPCATT_LEAD:
            return _( "Leading" );
        // Waiting for the player
        case NPCATT_WAIT:
            return _( "Waiting for you" );
        // Mug the player
        case NPCATT_MUG:
            return _( "Mugging you" );
        // Attack the player if our patience runs out
        case NPCATT_WAIT_FOR_LEAVE:
            return _( "Waiting for you to leave" );
        // Kill the player
        case NPCATT_KILL:
            return _( "Attacking to kill" );
        // Get away from the player
        case NPCATT_FLEE:
        case NPCATT_FLEE_TEMP:
            return _( "Fleeing" );
        // Get to the player and heal them
        case NPCATT_HEAL:
            return _( "Healing you" );
        case NPCATT_ACTIVITY:
            return _( "Performing a task" );
        case NPCATT_RECOVER_GOODS:
            return _( "Trying to recover stolen goods" );
        case NPCATT_LEGACY_1:
        case NPCATT_LEGACY_2:
        case NPCATT_LEGACY_3:
        case NPCATT_LEGACY_4:
        case NPCATT_LEGACY_5:
        case NPCATT_LEGACY_6:
            return _( "NPC Legacy Attitude" );
        default:
            break;
    }

    debugmsg( "Invalid attitude: %d", att );
    return _( "Unknown attitude" );
}

//message related stuff

//message related stuff
void npc::add_msg_if_npc( const std::string &msg ) const
{
    add_msg( replace_with_npc_name( msg ) );
}

void npc::add_msg_player_or_npc( const std::string &/*player_msg*/,
                                 const std::string &npc_msg ) const
{
    add_msg_if_player_sees( *this, replace_with_npc_name( npc_msg ) );
}

void npc::add_msg_if_npc( const game_message_params &params, const std::string &msg ) const
{
    add_msg( params, replace_with_npc_name( msg ) );
}

void npc::add_msg_debug_if_npc( debugmode::debug_filter type, const std::string &msg ) const
{
    add_msg_debug( type, replace_with_npc_name( msg ) );
}

void npc::add_msg_player_or_npc( const game_message_params &params,
                                 const std::string &/*player_msg*/,
                                 const std::string &npc_msg ) const
{
    if( get_player_view().sees( *this ) ) {
        add_msg( params, replace_with_npc_name( npc_msg ) );
    }
}

void npc::add_msg_debug_player_or_npc( debugmode::debug_filter type,
                                       const std::string &/*player_msg*/,
                                       const std::string &npc_msg ) const
{
    if( get_player_view().sees( *this ) ) {
        add_msg_debug( type, replace_with_npc_name( npc_msg ) );
    }
}

void npc::add_msg_player_or_say( const std::string &/*player_msg*/,
                                 const std::string &npc_speech ) const
{
    say( npc_speech );
}

void npc::add_msg_player_or_say( const game_message_params &/*params*/,
                                 const std::string &/*player_msg*/, const std::string &npc_speech ) const
{
    say( npc_speech );
}

void npc::add_new_mission( class mission *miss )
{
    chatbin.add_new_mission( miss );
}

void npc::on_unload()
{
}

// A throtled version of player::update_body since npc's don't need to-the-turn updates.
void npc::npc_update_body()
{
    if( calendar::once_every( 10_seconds ) ) {
        update_body( last_updated, calendar::turn );
        last_updated = calendar::turn;
    }
}

void npc::on_load()
{
    const auto advance_effects = [&]( const time_duration & elapsed_dur ) {
        for( auto &elem : *effects ) {
            for( auto &_effect_it : elem.second ) {
                effect &e = _effect_it.second;
                const time_duration &time_left = e.get_duration();
                if( time_left > 1_turns ) {
                    if( time_left < elapsed_dur ) {
                        e.set_duration( 1_turns );
                    } else {
                        e.set_duration( time_left - elapsed_dur );
                    }
                }
            }
        }
    };
    const auto advance_focus = [this]( const int minutes ) {
        // scale to match focus_pool magnitude
        const int equilibrium = 1000 * focus_equilibrium_fatigue_cap( calc_focus_equilibrium() );
        const double focus_ratio = std::pow( 0.99, minutes );
        // Approximate new focus pool, every minute focus_pool contributes 99%, the remainder comes from equilibrium
        // This is pretty accurate as long as the equilibrium doesn't change too much during the period
        focus_pool = static_cast<int>( focus_ratio * focus_pool + ( 1 - focus_ratio ) * equilibrium );
    };

    // Cap at some reasonable number, say 2 days
    const time_duration dt = std::min( calendar::turn - last_updated, 2_days );
    // TODO: Sleeping, healing etc.
    last_updated = calendar::turn;
    time_point cur = calendar::turn - dt;
    add_msg_debug( debugmode::DF_NPC, "on_load() by %s, %d turns", get_name(), to_turns<int>( dt ) );
    // First update with 30 minute granularity, then 5 minutes, then turns
    for( ; cur < calendar::turn - 30_minutes; cur += 30_minutes + 1_turns ) {
        update_body( cur, cur + 30_minutes );
        advance_effects( 30_minutes );
        advance_focus( 30 );
    }
    for( ; cur < calendar::turn - 5_minutes; cur += 5_minutes + 1_turns ) {
        update_body( cur, cur + 5_minutes );
        advance_effects( 5_minutes );
        advance_focus( 5 );
    }
    for( ; cur < calendar::turn; cur += 1_turns ) {
        update_body( cur, cur + 1_turns );
        process_effects();
        if( ( cur - calendar::turn_zero ) % 1_minutes == 0_turns ) {
            update_mental_focus();
        }
    }

    if( dt > 0_turns ) {
        // This ensures food is properly rotten at load
        // Otherwise NPCs try to eat rotten food and fail
        process_items();
        // give NPCs that are doing activities a pile of moves
        if( has_destination() || activity ) {
            mod_moves( to_moves<int>( dt ) );
        }
    }

    // Not necessarily true, but it's not a bad idea to set this
    has_new_items = true;

    map &here = get_map();
    // for spawned npcs
    if( here.has_flag( ter_furn_flag::TFLAG_UNSTABLE, pos_bub() ) &&
        !here.has_vehicle_floor( pos_bub() ) ) {
        add_effect( effect_bouldering, 1_turns,  true );
    } else if( has_effect( effect_bouldering ) ) {
        remove_effect( effect_bouldering );
    }
    if( here.veh_at( pos() ).part_with_feature( VPFLAG_BOARDABLE, true ) && !in_vehicle ) {
        here.board_vehicle( pos(), this );
    }
    if( has_effect( effect_riding ) && !mounted_creature ) {
        if( const monster *const mon = get_creature_tracker().creature_at<monster>( pos() ) ) {
            mounted_creature = g->shared_from( *mon );
        } else {
            add_msg_debug( debugmode::DF_NPC,
                           "NPC is meant to be riding, though the mount is not found when %s is loaded",
                           disp_name() );
        }
    }
    if( has_trait( trait_HALLUCINATION ) ) {
        hallucination = true;
    }
    effect_on_conditions::load_existing_character( *this );
    shop_restock();
}

bool npc::query_yn( const std::string &/*msg*/ ) const
{
    // NPCs don't like queries - most of them are in the form of "Do you want to get hurt?".
    return false;
}

float npc::speed_rating() const
{
    float ret = get_speed() / 100.0f;
    ret *= 100.0f / run_cost( 100, false );

    return ret;
}

bool npc::dispose_item( item_location &&obj, const std::string & )
{
    stow_item( *obj.get_item() );
    return true;
}

void npc::process_turn()
{
    Character::process_turn();

    // NPCs shouldn't be using stamina, but if they have, set it back to max
    // If the stamina is higher than the max (Languorous), set it back to max
    if( calendar::once_every( 1_minutes ) && get_stamina() != get_stamina_max() ) {
        set_stamina( get_stamina_max() );
    }

    if( is_player_ally() && calendar::once_every( 1_hours ) &&
        get_hunger() < 200 && get_thirst() < 100 && op_of_u.trust < 5 ) {
        // Friends who are well fed will like you more
        // 24 checks per day, best case chance at trust 0 is 1 in 48 for +1 trust per 2 days
        float trust_chance = 5 - op_of_u.trust;
        // Penalize for bad impression
        // TODO: Penalize for traits and actions (especially murder, unless NPC is psycho)
        int op_penalty = std::max( 0, op_of_u.anger ) +
                         std::max( 0, -op_of_u.value ) +
                         std::max( 0, op_of_u.fear );
        // Being barely hungry and thirsty, not in pain and not wounded means good care
        int state_penalty = get_hunger() + get_thirst() + ( 100 - hp_percentage() ) + get_pain();
        if( x_in_y( trust_chance, 240 + 10 * op_penalty + state_penalty ) ) {
            op_of_u.trust++;
        }

        // TODO: Similar checks for fear and anger
    }

    // TODO: Add decreasing trust/value/etc. here when player doesn't provide food
    // TODO: Make NPCs leave the player if there's a path out of map and player is sleeping/unseen/etc.
}

bool npc::invoke_item( item *used, const tripoint &pt, int )
{
    const auto &use_methods = used->type->use_methods;

    if( use_methods.empty() ) {
        return false;
    } else if( use_methods.size() == 1 ) {
        return Character::invoke_item( used, use_methods.begin()->first, pt );
    }
    return false;
}

bool npc::invoke_item( item *used, const std::string &method )
{
    return Character::invoke_item( used, method );
}

bool npc::invoke_item( item *used )
{
    return Character::invoke_item( used );
}

std::array<std::pair<std::string, overmap_location_str_id>, npc_need::num_needs> npc::need_data = {
    {
        { "need_none", overmap_location_source_of_anything },
        { "need_ammo", overmap_location_source_of_ammo },
        { "need_weapon", overmap_location_source_of_weapons},
        { "need_gun", overmap_location_source_of_guns },
        { "need_food", overmap_location_source_of_food},
        { "need_drink", overmap_location_source_of_drink },
        { "need_safety", overmap_location_source_of_safety }
    }
};

std::string npc::get_need_str_id( const npc_need &need )
{
    return need_data[static_cast<size_t>( need )].first;
}

overmap_location_str_id npc::get_location_for( const npc_need &need )
{
    return need_data[static_cast<size_t>( need )].second;
}

std::ostream &operator<< ( std::ostream &os, const npc_need &need )
{
    return os << npc::get_need_str_id( need );
}

bool npc::will_accept_from_player( const item &it ) const
{
    if( is_hallucination() ) {
        return false;
    }

    if( is_minion() || get_player_character().has_trait( trait_DEBUG_MIND_CONTROL ) ||
        it.has_flag( flag_NPC_SAFE ) ) {
        return true;
    }

    if( !it.type->use_methods.empty() ) {
        return false;
    }

    if( it.is_comestible() ) {
        if( it.get_comestible_fun() < 0 || it.poison > 0 ) {
            return false;
        }
    }

    return true;
}

const pathfinding_settings &npc::get_pathfinding_settings() const
{
    return get_pathfinding_settings( false );
}

const pathfinding_settings &npc::get_pathfinding_settings( bool no_bashing ) const
{
    path_settings->bash_strength = no_bashing ? 0 : smash_ability();
    if( has_trait( trait_NO_BASH ) ) {
        path_settings->bash_strength = 0;
    }
    // TODO: Extract climb skill
    const int climb = std::min( 20, get_dex() );
    if( climb > 1 ) {
        // Success is !one_in(dex), so 0%, 50%, 66%, 75%...
        // Penalty for failure chance is 1/success = 1/(1-failure) = 1/(1-(1/dex)) = dex/(dex-1)
        path_settings->climb_cost = ( 10 - climb / 5 ) * climb / ( climb - 1 );
    } else {
        // Climbing at this dexterity will always fail
        path_settings->climb_cost = 0;
    }

    return *path_settings;
}

std::set<tripoint> npc::get_path_avoid() const
{
    std::set<tripoint> ret;
    for( Creature &critter : g->all_creatures() ) {
        // TODO: Cache this somewhere
        ret.insert( critter.pos() );
    }
    map &here = get_map();
    if( rules.has_flag( ally_rule::avoid_doors ) ) {
        for( const tripoint &p : here.points_in_radius( pos(), 30 ) ) {
            if( here.open_door( *this, p, true, true ) ) {
                ret.insert( p );
            }
        }
    }
    if( rules.has_flag( ally_rule::avoid_locks ) ) {
        for( const tripoint &p : here.points_in_radius( pos(), 30 ) ) {
            if( doors::can_unlock_door( here, *this, p ) ) {
                ret.insert( p );
            }
        }
    }
    if( rules.has_flag( ally_rule::hold_the_line ) ) {
        for( const tripoint &p : here.points_in_radius( get_player_character().pos(), 1 ) ) {
            if( here.close_door( p, true, true ) || here.move_cost( p ) > 2 ) {
                ret.insert( p );
            }
        }
    }

    for( const tripoint &p : here.points_in_radius( pos(), 5 ) ) {
        if( sees_dangerous_field( p ) ) {
            ret.insert( p );
        }
    }

    return ret;
}

mfaction_id npc::get_monster_faction() const
{
    if( my_fac ) {
        if( my_fac->mon_faction.is_valid() ) {
            return my_fac->mon_faction;
        }
    }

    // legacy checks
    if( is_player_ally() ) {
        return monfaction_player.id();
    }

    if( has_trait( trait_BEE ) ) {
        return monfaction_bee.id();
    }

    return monfaction_human.id();
}

std::string npc::extended_description() const
{
    std::string ss;
    // For some reason setting it using str or constructor doesn't work
    ss += Character::extended_description();

    ss += "\n--\n";
    if( attitude == NPCATT_KILL ) {
        ss += _( "Is trying to kill you." );
    } else if( attitude == NPCATT_FLEE || attitude == NPCATT_FLEE_TEMP ) {
        ss += _( "Is trying to flee from you." );
    } else if( is_player_ally() ) {
        ss += _( "Is your friend." );
    } else if( is_following() ) {
        ss += _( "Is following you." );
    } else if( is_leader() ) {
        ss += _( "Is guiding you." );
    } else if( guaranteed_hostile() ) {
        ss += _( "Will try to kill you or flee from you if you reveal yourself." );
    } else {
        ss += _( "Is neutral." );
    }

    if( hit_by_player ) {
        ss += "--\n";
        ss += _( "Is still innocent and killing them will be considered murder." );
        // TODO: "But you don't care because you're an edgy psycho"
    }

    return replace_colors( ss );
}

std::string npc::get_epilogue() const
{
    return SNIPPET.random_from_category(
               male ? "epilogue_npc_male" : "epilogue_npc_female"
           ).value_or( translation() ).translated();
}

void npc::set_companion_mission( npc &p, const mission_id &miss_id )
{
    const tripoint_abs_omt omt_pos = p.global_omt_location();
    set_companion_mission( omt_pos, p.companion_mission_role_id, miss_id );
}

std::pair<std::string, nc_color> npc::hp_description() const
{
    int cur_hp = hp_percentage();
    std::string damage_info;
    std::string pronoun;
    if( male ) {
        pronoun = _( "He " );
    } else {
        pronoun = _( "She " );
    }
    nc_color col;
    if( cur_hp == 100 ) {
        damage_info = pronoun + _( "is uninjured." );
        col = c_green;
    } else if( cur_hp >= 80 ) {
        damage_info = pronoun + _( "is lightly injured." );
        col = c_light_green;
    } else if( cur_hp >= 60 ) {
        damage_info = pronoun + _( "is moderately injured." );
        col = c_yellow;
    } else if( cur_hp >= 30 ) {
        damage_info = pronoun + _( "is heavily injured." );
        col = c_yellow;
    } else if( cur_hp >= 10 ) {
        damage_info = pronoun + _( "is severely injured." );
        col = c_light_red;
    } else {
        damage_info = pronoun + _( "is nearly dead!" );
        col = c_red;
    }
    return std::make_pair( damage_info, col );
}
void npc::set_companion_mission( const tripoint_abs_omt &omt_pos, const std::string &role_id,
                                 const mission_id &miss_id )
{
    comp_mission.position = omt_pos;
    comp_mission.miss_id = miss_id;
    comp_mission.role_id = role_id;
}

void npc::set_companion_mission( const tripoint_abs_omt &omt_pos, const std::string &role_id,
                                 const mission_id &miss_id, const tripoint_abs_omt &destination )
{
    comp_mission.position = omt_pos;
    comp_mission.miss_id = miss_id;
    comp_mission.role_id = role_id;
    comp_mission.destination = destination;
}

void npc::reset_companion_mission()
{
    comp_mission.position = tripoint_abs_omt( -999, -999, -999 );
    reset_miss_id( comp_mission.miss_id );
    comp_mission.role_id.clear();
    if( comp_mission.destination ) {
        comp_mission.destination = std::nullopt;
    }
}

std::optional<tripoint_abs_omt> npc::get_mission_destination() const
{
    if( comp_mission.destination ) {
        return comp_mission.destination;
    } else {
        return std::nullopt;
    }
}

bool npc::has_companion_mission() const
{
    return comp_mission.miss_id.id != No_Mission;
}

npc_companion_mission npc::get_companion_mission() const
{
    return comp_mission;
}

attitude_group npc::get_attitude_group( npc_attitude att ) const
{
    switch( att ) {
        case NPCATT_MUG:
        case NPCATT_WAIT_FOR_LEAVE:
        case NPCATT_KILL:
            return attitude_group::hostile;
        case NPCATT_FLEE:
        case NPCATT_FLEE_TEMP:
            return attitude_group::fearful;
        case NPCATT_FOLLOW:
        case NPCATT_ACTIVITY:
        case NPCATT_LEAD:
            return attitude_group::friendly;
        default:
            break;
    }
    return attitude_group::neutral;
}

void npc::set_unique_id( const std::string &id )
{
    if( !unique_id.empty() ) {
        debugmsg( "Tried to set unique_id of npc with one already of value: ", unique_id );
    } else {
        unique_id = id;
        g->update_unique_npc_location( id, project_to<coords::om>( get_location().xy() ) );
    }
}

std::string npc::get_unique_id() const
{
    return unique_id;
}

void npc::set_mission( npc_mission new_mission )
{
    if( new_mission != mission ) {
        previous_mission = mission;
        mission = new_mission;
    }
    if( mission == NPC_MISSION_ACTIVITY ) {
        current_activity_id = activity.id();
    }
}

bool npc::has_activity() const
{
    return mission == NPC_MISSION_ACTIVITY && attitude == NPCATT_ACTIVITY;
}

npc_attitude npc::get_attitude() const
{
    return attitude;
}

void npc::set_attitude( npc_attitude new_attitude )
{
    if( new_attitude == attitude ) {
        return;
    }
    previous_attitude = attitude;
    if( new_attitude == NPCATT_FLEE ) {
        new_attitude = NPCATT_FLEE_TEMP;
    }
    if( new_attitude == NPCATT_FLEE_TEMP && !has_effect( effect_npc_flee_player ) ) {
        add_effect( effect_npc_flee_player, 24_hours );
    }

    add_msg_debug( debugmode::DF_NPC, "%s changes attitude from %s to %s",
                   get_name(), npc_attitude_id( attitude ), npc_attitude_id( new_attitude ) );
    attitude_group new_group = get_attitude_group( new_attitude );
    attitude_group old_group = get_attitude_group( attitude );
    if( new_group != old_group && !is_fake() && get_player_view().sees( *this ) ) {
        switch( new_group ) {
            case attitude_group::hostile:
                add_msg_if_npc( m_bad, _( "<npcname> gets angry!" ) );
                break;
            case attitude_group::fearful:
                add_msg_if_npc( m_warning, _( "<npcname> gets scared!" ) );
                break;
            default:
                if( old_group == attitude_group::hostile ) {
                    add_msg_if_npc( m_good, _( "<npcname> calms down." ) );
                } else if( old_group == attitude_group::fearful ) {
                    add_msg_if_npc( _( "<npcname> is no longer afraid." ) );
                }
                break;
        }
    }
    attitude = new_attitude;
}

bool npc::theft_witness_compare( const npc *lhs, const npc *rhs )
{
    const auto class_score = []( const npc * const guy ) {
        // Apparently a guard?
        if( guy->myclass == NC_BOUNTY_HUNTER ) {
            return 0;
            // If the guard? doesn't notice, the shopkeep will complain
            // Because they're probably the person who you have to talk to
            // and so will ensure the theft is noticed
        } else if( guy->mission == NPC_MISSION_SHOPKEEP ) {
            return 1;
            // Patrolling NPCs more likely to notice/be guards?
        } else if( guy->mission == NPC_MISSION_GUARD_PATROL ) {
            return 2;
        }
        return 3;
    };
    return class_score( lhs ) < class_score( rhs );
}

npc_follower_rules::npc_follower_rules()
{
    engagement = combat_engagement::CLOSE;
    aim = aim_rule::WHEN_CONVENIENT;
    overrides = ally_rule::DEFAULT;
    override_enable = ally_rule::DEFAULT;

    set_flag( ally_rule::use_guns );
    set_flag( ally_rule::use_grenades );
    clear_flag( ally_rule::use_silent );
    set_flag( ally_rule::avoid_friendly_fire );

    clear_flag( ally_rule::allow_pick_up );
    clear_flag( ally_rule::allow_bash );
    clear_flag( ally_rule::allow_sleep );
    set_flag( ally_rule::allow_complain );
    set_flag( ally_rule::allow_pulp );
    clear_flag( ally_rule::close_doors );
    clear_flag( ally_rule::follow_close );
    clear_flag( ally_rule::avoid_doors );
    clear_flag( ally_rule::hold_the_line );
    clear_flag( ally_rule::ignore_noise );
    clear_flag( ally_rule::forbid_engage );
    set_flag( ally_rule::follow_distance_2 );
}

bool npc_follower_rules::has_flag( ally_rule test, bool check_override ) const
{
    if( check_override && ( static_cast<int>( test ) & static_cast<int>( override_enable ) ) ) {
        // if the override is set and false, return false
        if( static_cast<int>( test ) & ~static_cast<int>( overrides ) ) {
            return false;
            // if the override is set and true, return true
        } else if( static_cast<int>( test ) & static_cast<int>( overrides ) ) {
            return true;
        }
    }
    return static_cast<int>( test ) & static_cast<int>( flags );
}

void npc_follower_rules::set_flag( ally_rule setit )
{
    flags = static_cast<ally_rule>( static_cast<int>( flags ) | static_cast<int>( setit ) );
}

void npc_follower_rules::clear_flag( ally_rule clearit )
{
    flags = static_cast<ally_rule>( static_cast<int>( flags ) & ~static_cast<int>( clearit ) );
}

void npc_follower_rules::toggle_flag( ally_rule toggle )
{
    if( has_flag( toggle ) ) {
        clear_flag( toggle );
    } else {
        set_flag( toggle );
    }
}

void npc_follower_rules::set_specific_override_state( ally_rule rule, bool state )
{
    if( state ) {
        set_override( rule );
    } else {
        clear_override( rule );
    }
    enable_override( rule );
}

void npc_follower_rules::toggle_specific_override_state( ally_rule rule, bool state )
{
    if( has_override_enable( rule ) && has_override( rule ) == state ) {
        clear_override( rule );
        disable_override( rule );
    } else {
        set_specific_override_state( rule, state );
    }
}

bool npc_follower_rules::has_override_enable( ally_rule test ) const
{
    return static_cast<int>( test ) & static_cast<int>( override_enable );
}

void npc_follower_rules::enable_override( ally_rule setit )
{
    override_enable = static_cast<ally_rule>( static_cast<int>( override_enable ) |
                      static_cast<int>( setit ) );
}

void npc_follower_rules::disable_override( ally_rule clearit )
{
    override_enable = static_cast<ally_rule>( static_cast<int>( override_enable ) &
                      ~static_cast<int>( clearit ) );
}

bool npc_follower_rules::has_override( ally_rule test ) const
{
    return static_cast<int>( test ) & static_cast<int>( overrides );
}

void npc_follower_rules::set_override( ally_rule setit )
{
    overrides = static_cast<ally_rule>( static_cast<int>( overrides ) | static_cast<int>( setit ) );
}

void npc_follower_rules::clear_override( ally_rule clearit )
{
    overrides = static_cast<ally_rule>( static_cast<int>( overrides ) &
                                        ~static_cast<int>( clearit ) );
}

void npc_follower_rules::set_danger_overrides()
{
    overrides = ally_rule::DEFAULT;
    override_enable = ally_rule::DEFAULT;
    set_override( ally_rule::follow_close );
    set_override( ally_rule::avoid_doors );
    set_override( ally_rule::hold_the_line );
    enable_override( ally_rule::follow_close );
    enable_override( ally_rule::allow_sleep );
    enable_override( ally_rule::close_doors );
    enable_override( ally_rule::avoid_doors );
    enable_override( ally_rule::hold_the_line );
}

void npc_follower_rules::clear_overrides()
{
    overrides = ally_rule::DEFAULT;
    override_enable = ally_rule::DEFAULT;
}

int npc::get_thirst() const
{
    return Character::get_thirst() - units::to_milliliter<int>( stomach.get_water() ) / 5;
}

std::string npc::describe_mission() const
{
    switch( mission ) {
        case NPC_MISSION_SHELTER:
            return string_format( _( "I'm holing up here for safety.  Long term, %s" ),
                                  myclass.obj().get_job_description() );
        case NPC_MISSION_SHOPKEEP:
            return _( "I run the shop here." );
        case NPC_MISSION_GUARD:
        case NPC_MISSION_GUARD_ALLY:
        case NPC_MISSION_GUARD_PATROL:
            return string_format( _( "Currently, I'm guarding this location.  Overall, %s" ),
                                  myclass.obj().get_job_description() );
        case NPC_MISSION_ACTIVITY:
            return string_format( _( "Right now, I'm <current_activity>.  In general, %s" ),
                                  myclass.obj().get_job_description() );
        case NPC_MISSION_TRAVELLING:
        case NPC_MISSION_NULL:
            return myclass.obj().get_job_description();
        default:
            debugmsg( "ERROR: Someone forgot to code an npc_mission text for mission: %d.",
                      static_cast<int>( mission ) );
            return "";
    } // switch (mission)
}

std::string npc::name_and_activity() const
{
    if( current_activity_id ) {
        //~ %1$s - npc name, %2$s - npc current activity name.
        return string_format( _( "%1$s (%2$s)" ), get_name(), get_current_activity() );
    } else {
        return get_name();
    }
}

std::string npc::name_and_maybe_activity() const
{
    return name_and_activity();
}

std::string npc::get_current_activity() const
{
    if( current_activity_id ) {
        return current_activity_id.obj().verb().translated();
    } else {
        return _( "nothing" );
    }
}

void npc::update_missions_target( character_id old_character, character_id new_character )
{
    for( ::mission *&temp : chatbin.missions_assigned ) {
        if( temp->get_assigned_player_id() == old_character ||
            temp->get_assigned_player_id() == character_id( - 1 ) ) {
            temp->set_assigned_player_id( new_character );
        }
    }
}

std::unique_ptr<talker> get_talker_for( npc &guy )
{
    return std::make_unique<talker_npc>( &guy );
}
std::unique_ptr<talker> get_talker_for( npc *guy )
{
    return std::make_unique<talker_npc>( guy );
}
