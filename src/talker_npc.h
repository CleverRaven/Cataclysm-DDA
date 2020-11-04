#pragma once
#ifndef CATA_SRC_TALKER_NPC_H
#define CATA_SRC_TALKER_NPC_H

#include <string>
#include <vector>

#include "talker_character.h"
#include "type_id.h"

class Character;
class character_id;
class faction;
class item;
class mission;
class npc;
class player;
class recipe;
class talker;
class vehicle;
struct tripoint;

/*
 */
class talker_npc : public talker_character
{
    public:
        talker_npc( npc *new_me ): talker_character( new_me ), me_npc( new_me ) {
        }
        ~talker_npc() override = default;

        npc *get_npc() override {
            return me_npc;
        }
        npc *get_npc() const override {
            return me_npc;
        }

        // identity and location
        std::string distance_to_goal() const override;

        // mandatory functions for starting a dialogue
        bool will_talk_to_u( const player &u, bool force ) override;
        std::vector<std::string> get_topics( bool radio_contact ) override;
        void check_missions() override;
        void update_missions( const std::vector<mission *> &missions_assigned,
                              const character_id &charID ) override;
        bool check_hostile_response( int anger ) const override;
        int parse_mod( const std::string &attribute, int factor ) const override;
        int trial_chance_mod( const std::string &trial_type ) const override;

        // stats, skills, traits, bionics, magic, and proficiencies
        std::vector<skill_id> skills_offered_to( const talker &student ) const override;
        std::string skill_training_text( const talker &, const skill_id & ) const override;
        std::vector<proficiency_id> proficiencies_offered_to( const talker &student ) const override;
        std::string proficiency_training_text( const talker &student,
                                               const proficiency_id &proficiency ) const override;
        std::vector<matype_id> styles_offered_to( const talker &student ) const override;
        std::string style_training_text( const talker &, const matype_id & ) const override;
        std::vector<spell_id> spells_offered_to( talker &student ) override;
        std::string spell_training_text( talker &, const spell_id & ) override;
        void store_chosen_training( const skill_id &c_skill, const matype_id &c_style,
                                    const spell_id &c_spell, const proficiency_id &c_proficiency ) override;

        // inventory, buying, and selling
        void add_debt( int cost ) override;
        int debt() const override;
        int cash_to_favor( int value ) const override;
        std::string give_item_to( bool to_use ) override;
        bool buy_from( int amount ) override;

        // missions
        std::vector<mission *> available_missions() const override;
        std::vector<mission *> assigned_missions() const override;
        mission *selected_mission() const override;
        void select_mission( mission *selected ) override;
        void add_mission( const mission_type_id &mission_id ) override;
        void set_companion_mission( const std::string &role_id ) override;

        // factions and alliances
        void set_fac( const faction_id &new_fac_name ) override;
        void add_faction_rep( int rep_change ) override;
        bool is_following() const override;
        bool is_friendly( const Character &guy ) const override;
        bool is_enemy() const override;
        bool is_player_ally()  const override;
        bool turned_hostile() const override;
        void make_angry() override;

        // ai rules
        bool has_ai_rule( const std::string &type, const std::string &rule ) const override;
        void toggle_ai_rule( const std::string &type, const std::string &rule ) override;
        void set_ai_rule( const std::string &type, const std::string &rule ) override;
        void clear_ai_rule( const std::string &type, const std::string &rule ) override;

        // other descriptors
        std::string get_job_description() const override;
        std::string evaluation_by( const talker &alpha ) const override;
        bool has_activity() const override;
        bool is_myclass( const npc_class_id &class_to_check ) const override;
        void set_class( const npc_class_id &new_class ) override;

        // speaking
        void say( const std::string &speech ) override;

        // miscellaneous
        std::string opinion_text() const override;
        void add_opinion( int trust, int fear, int value, int anger, int debt ) override;
        bool enslave_mind() override;
        void set_first_topic( const std::string &chat_topic ) override;
        bool is_safe() const override;

    protected:
        npc *me_npc;
};
#endif // CATA_SRC_TALKER_NPC_H
