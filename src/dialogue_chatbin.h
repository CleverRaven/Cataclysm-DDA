#pragma once
#ifndef CATA_SRC_DIALOGUE_CHATBIN_H
#define CATA_SRC_DIALOGUE_CHATBIN_H

#include <string>
#include <vector>

#include "translation.h"
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
    skill_id skill;
    /**
     * The martial art style this dialogue offers to train.
     */
    matype_id style;
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

    dialogue_chatbin() = default;

    void clear_all();
    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &data );

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
};

struct dialogue_chatbin_snippets {
    // talk from npcmove.cpp(can use snippets in json)
    translation snip_camp_food_thanks = no_translation( "<camp_food_thanks>" );
    translation snip_camp_larder_empty = no_translation( "<camp_larder_empty>" );
    translation snip_camp_water_thanks = no_translation( "<camp_water_thanks>" );
    translation snip_cant_flee = no_translation( "<cant_flee>" );
    translation snip_close_distance = no_translation( "<close_distance>" );
    translation snip_combat_noise_warning = no_translation( "<combat_noise_warning>" );
    translation snip_danger_close_distance = no_translation( "<danger_close_distance>" );
    translation snip_done_mugging = no_translation( "<done_mugging>" );
    translation snip_far_distance = no_translation( "<far_distance>" );
    translation snip_fire_bad = no_translation( "<fire_bad>" );
    translation snip_fire_in_the_hole_h = no_translation( "<fire_in_the_hole_h>" );
    translation snip_fire_in_the_hole = no_translation( "<fire_in_the_hole>" );
    translation snip_general_danger_h = no_translation( "<general_danger_h>" );
    translation snip_general_danger = no_translation( "<general_danger>" );
    translation snip_heal_self = no_translation( "<heal_self>" );
    translation snip_hungry = no_translation( "<hungry>" );
    translation snip_im_leaving_you = no_translation( "<im_leaving_you>" );
    translation snip_its_safe_h = no_translation( "<its_safe_h>" );
    translation snip_its_safe = no_translation( "<its_safe>" );
    translation snip_keep_up = no_translation( "<keep_up>" );
    translation snip_kill_npc_h = no_translation( "<kill_npc_h>" );
    translation snip_kill_npc = no_translation( "<kill_npc>" );
    translation snip_kill_player_h = no_translation( "<kill_player_h>" );
    translation snip_let_me_pass = no_translation( "<let_me_pass>" );
    translation snip_lets_talk = no_translation( "<lets_talk>" );
    translation snip_medium_distance = no_translation( "<medium_distance>" );
    translation snip_monster_warning_h = no_translation( "<monster_warning_h>" );
    translation snip_monster_warning = no_translation( "<monster_warning>" );
    translation snip_movement_noise_warning = no_translation( "<movement_noise_warning>" );
    translation snip_need_batteries = no_translation( "<need_batteries>" );
    translation snip_need_booze = no_translation( "<need_booze>" );
    translation snip_need_fuel = no_translation( "<need_fuel>" );
    translation snip_no_to_thorazine = no_translation( "<no_to_thorazine>" );
    translation snip_run_away = no_translation( "<run_away>" );
    translation snip_speech_warning = no_translation( "<speech_warning>" );
    translation snip_thirsty = no_translation( "<thirsty>" );
    translation snip_wait = no_translation( "<wait>" );
    translation snip_warn_sleep = no_translation( "<warn_sleep>" );
    translation snip_yawn = no_translation( "<yawn>" );
    translation snip_yes_to_lsd = no_translation( "<yes_to_lsd>" );
    translation snip_mug_dontmove = to_translation( "Don't move a <freaking> muscle…" );
    translation snip_lost_blood = to_translation( "I've lost lot of blood." );
    translation snip_pulp_zombie = to_translation( "Hold on, I want to pulp that %s." );
    translation snip_heal_player = to_translation( "Hold still %s, I'm coming to help you." );
    translation snip_wound_infected = to_translation( "My %s wound is infected…" );
    translation snip_wound_bite = to_translation( "The bite wound on my %s looks bad." );
    translation snip_bleeding = to_translation( "My %s is bleeding!" );
    translation snip_bleeding_badly = to_translation( "My %s is bleeding badly!" );
    translation snip_radiation_sickness = to_translation( "I'm suffering from radiation sickness…" );

    // talk from npctalk.cpp(can use snippets in json)
    translation snip_acknowledged = no_translation( "<acknowledged>" );
    translation snip_bye = to_translation( "<end_talking_bye>" );

    // talk from talker_npc.cpp(can use snippets in json)
    translation snip_consume_cant_accept =
        to_translation( "I don't fuckin' trust you enough to eat THIS…" );
    translation snip_consume_cant_consume =
        to_translation( "It doesn't look like a good idea to consume this…" );
    translation snip_consume_rotten = to_translation( "This is rotten!  I won't eat that." );
    translation snip_consume_eat = to_translation( "Thanks, that hit the spot." );
    translation snip_consume_need_item = to_translation( "I need a %s to consume that!" );
    translation snip_consume_med = to_translation( "Thanks, I feel better already." );
    translation snip_consume_nocharge =
        to_translation( "It doesn't look like a good idea to consume this…" );
    translation snip_consume_use_med = to_translation( "Thanks, I used it." );
    translation snip_give_nope = to_translation( "Nope." );
    translation snip_give_to_hallucination = to_translation( "No thanks, I'm good." );
    translation snip_give_cancel = to_translation( "Changed your mind?" );
    translation snip_give_dangerous = to_translation( "Are you <freaking> insane!?" );
    translation snip_give_wield = to_translation( "Thanks, I'll wield that now." );
    translation snip_give_weapon_weak = to_translation( "My current weapon is better than this.\n" );
    translation snip_give_carry = to_translation( "Thanks, I'll carry that now." );
    translation snip_give_carry_cant = to_translation( "I have no space to store it." );
    translation snip_give_carry_cant_few_space = to_translation( "I can only store %s %s more." );
    translation snip_give_carry_cant_no_space =
        to_translation( "…or to store anything else for that matter." );
    translation snip_give_carry_too_heavy = to_translation( "It is too heavy for me to carry." );

    // talk from npc.cpp(can use snippets in json)
    translation snip_wear = to_translation( "Thanks, I'll wear that now." );
};

#endif // CATA_SRC_DIALOGUE_CHATBIN_H
