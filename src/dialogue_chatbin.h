#pragma once
#ifndef CATA_SRC_DIALOGUE_CHATBIN_H
#define CATA_SRC_DIALOGUE_CHATBIN_H

#include <iosfwd>
#include <vector>

#include "translations.h"
#include "type_id.h"

class JsonObject;
class JsonOut;
class mission;

struct dialogue_chatbin {
    /**
     * Add a new mission to the available missions (@ref missions). For compatibility it silently
     * ignores null pointers passed to it.
     */
    void add_new_mission( mission *miss );
    /**
     * Check that assigned missions are still assigned if not move them back to the
     * unassigned vector. This is called directly before talking.
     */
    void check_missions();
    /**
     * Missions that the talker can give out. All missions in this vector should be unassigned,
     * when given out, they should be moved to @ref missions_assigned.
     */
    std::vector<mission *> missions;
    /**
     * Mission that have been assigned by this dialogue to a player character.
     */
    std::vector<mission *> missions_assigned;
    /**
     * The mission (if any) that we talk about right now. Can be null. Should be one of the
     * missions in @ref missions or @ref missions_assigned.
     */
    mission *mission_selected = nullptr;
    /**
     * The skill this dialogue offers to train.
     */
    skill_id skill = skill_id();
    /**
     * The martial art style this dialogue offers to train.
     */
    matype_id style = matype_id();
    /**
     * The spell this dialogue offers to train
     */
    spell_id dialogue_spell;
    /**
     * The proficiency this dialogue offers to train
     */
    proficiency_id proficiency;
    void store_chosen_training( const skill_id &c_skill, const matype_id &c_style,
                                const spell_id &c_spell, const proficiency_id &c_proficiency );
    void clear_training();
    std::string first_topic = "TALK_NONE";
    std::string talk_radio = "TALK_RADIO";
    std::string talk_leader = "TALK_LEADER";
    std::string talk_friend = "TALK_FRIEND";
    std::string talk_stole_item = "TALK_STOLE_ITEM";
    std::string talk_wake_up = "TALK_WAKE_UP";
    std::string talk_mug = "TALK_MUG";
    std::string talk_stranger_aggressive = "TALK_STRANGER_AGGRESSIVE";
    std::string talk_stranger_scared = "TALK_STRANGER_SCARED";
    std::string talk_stranger_wary = "TALK_STRANGER_WARY";
    std::string talk_stranger_friendly = "TALK_STRANGER_FRIENDLY";
    std::string talk_stranger_neutral = "TALK_STRANGER_NEUTRAL";
    std::string talk_friend_guard = "TALK_FRIEND_GUARD";

    // talk from npcmove.cpp(can use snippets in json)
    std::string snip_camp_food_thanks = "<camp_food_thanks>";
    std::string snip_camp_larder_empty = "<camp_larder_empty>";
    std::string snip_camp_water_thanks = "<camp_water_thanks>";
    std::string snip_cant_flee = "<cant_flee>";
    std::string snip_close_distance = "<close_distance>";
    std::string snip_combat_noise_warning = "<combat_noise_warning>";
    std::string snip_danger_close_distance = "<danger_close_distance>";
    std::string snip_done_mugging = "<done_mugging>";
    std::string snip_far_distance = "<far_distance>";
    std::string snip_fire_bad = "<fire_bad>";
    std::string snip_fire_in_the_hole_h = "<fire_in_the_hole_h>";
    std::string snip_fire_in_the_hole = "<fire_in_the_hole>";
    std::string snip_general_danger_h = "<general_danger_h>";
    std::string snip_general_danger = "<general_danger>";
    std::string snip_heal_self = "<heal_self>";
    std::string snip_hungry = "<hungry>";
    std::string snip_im_leaving_you = "<im_leaving_you>";
    std::string snip_its_safe_h = "<its_safe_h>";
    std::string snip_its_safe = "<its_safe>";
    std::string snip_keep_up = "<keep_up>";
    std::string snip_kill_npc_h = "<kill_npc_h>";
    std::string snip_kill_npc = "<kill_npc>";
    std::string snip_kill_player_h = "<kill_player_h>";
    std::string snip_let_me_pass = "<let_me_pass>";
    std::string snip_lets_talk = "<lets_talk>";
    std::string snip_medium_distance = "<medium_distance>";
    std::string snip_monster_warning_h = "<monster_warning_h>";
    std::string snip_monster_warning = "<monster_warning>";
    std::string snip_movement_noise_warning = "<movement_noise_warning>";
    std::string snip_need_batteries = "<need_batteries>";
    std::string snip_need_booze = "<need_booze>";
    std::string snip_need_fuel = "<need_fuel>";
    std::string snip_no_to_thorazine = "<no_to_thorazine>";
    std::string snip_run_away = "<run_away>";
    std::string snip_speech_warning = "<speech_warning>";
    std::string snip_thirsty = "<thirsty>";
    std::string snip_wait = "<wait>";
    std::string snip_warn_sleep = "<warn_sleep>";
    std::string snip_yawn = "<yawn>";
    std::string snip_yes_to_lsd = "<yes_to_lsd>";
    std::string snip_mug_dontmove = _( "Don't move a <swear> muscle…" );
    std::string snip_lost_blood = _( "I've lost lot of blood." );
    std::string snip_pulp_zombie = _( "Hold on, I want to pulp that %s." );
    std::string snip_heal_player = _( "Hold still %s, I'm coming to help you." );
    std::string snip_wound_infected = _( "My %s wound is infected…" );
    std::string snip_wound_bite = _( "The bite wound on my %s looks bad." );
    std::string snip_bleeding = _( "My %s is bleeding!" );
    std::string snip_bleeding_badly = _( "My %s is bleeding badly!" );
    std::string snip_radiation_sickness = _( "I'm suffering from radiation sickness…" );

    // talk from npctalk.cpp(can use snippets in json)
    std::string snip_acknowledged = "<acknowledged>";
    std::string snip_bye = _( "Bye." );

    // talk from talker_npc.cpp(can use snippets in json)
    std::string snip_consume_cant_accept = _( "I don't <swear> trust you enough to eat THIS…" );
    std::string snip_consume_cant_consume = _( "It doesn't look like a good idea to consume this…" );
    std::string snip_consume_rotten = _( "This is rotten!  I won't eat that." );
    std::string snip_consume_eat = _( "Thanks, that hit the spot." );
    std::string snip_consume_need_item = _( "I need a %s to consume that!" );
    std::string snip_consume_med = _( "Thanks, I feel better already." );
    std::string snip_consume_nocharge = _( "It doesn't look like a good idea to consume this…" );
    std::string snip_consume_use_med = _( "Thanks, I used it." );
    std::string snip_give_nope = _( "Nope." );
    std::string snip_give_to_hallucination = _( "No thanks, I'm good." );
    std::string snip_give_cancel = _( "Changed your mind?" );
    std::string snip_give_dangerous = _( "Are you <swear> insane!?" );
    std::string snip_give_wield = _( "Thanks, I'll wield that now." );
    std::string snip_give_weapon_weak = _( "My current weapon is better than this.\n" );
    std::string snip_give_carry = _( "Thanks, I'll carry that now." );
    std::string snip_give_carry_cant = _( "I have no space to store it." );
    std::string snip_give_carry_cant_few_space = _( "I can only store %s %s more." );
    std::string snip_give_carry_cant_no_space = _( "…or to store anything else for that matter." );
    std::string snip_give_carry_too_heavy = _( "It is too heavy for me to carry." );

    // talk from npc.cpp(can use snippets in json)
    std::string snip_wear = _( "Thanks, I'll wear that now." );

    dialogue_chatbin() = default;

    void clear_all();
    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &data );
};

#endif // CATA_SRC_DIALOGUE_CHATBIN_H
