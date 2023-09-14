#pragma once
#ifndef CATA_SRC_TALKER_NPC_H
#define CATA_SRC_TALKER_NPC_H

#include <iosfwd>
#include <vector>

#include "faction.h"
#include "npc.h"
#include "talker_character.h"
#include "type_id.h"

class Character;
class item;
class mission;
class talker;

/*
 */
class talker_npc : public talker_cloner<talker_npc, talker_character>
{
    public:
        explicit talker_npc( npc *new_me );
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
        bool will_talk_to_u( const Character &you, bool force ) override;
        std::vector<std::string> get_topics( bool radio_contact ) override;
        void check_missions() override;
        void update_missions( const std::vector<mission *> &missions_assigned ) override;
        bool check_hostile_response( int anger ) const override;
        int parse_mod( const std::string &attribute, int factor ) const override;
        int trial_chance_mod( const std::string &trial_type ) const override;
        void store_chosen_training( const skill_id &c_skill, const matype_id &c_style,
                                    const spell_id &c_spell, const proficiency_id &c_proficiency ) override;

        // inventory, buying, and selling
        int debt() const override;
        void add_debt( int cost ) override;
        int sold() const override;
        void add_sold( int value ) override;
        int cash_to_favor( int value ) const override;
        std::string give_item_to( bool to_use ) override;
        bool buy_from( int amount ) override;
        int value( const item &it ) const override;

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
        void add_opinion( const npc_opinion &op ) override;
        bool enslave_mind() override;
        void set_first_topic( const std::string &chat_topic ) override;
        bool is_safe() const override;

        // opinions
        void set_npc_trust( int trust ) override;
        int get_npc_trust() const override;
        void set_npc_fear( int fear ) override;
        int get_npc_fear() const override;
        void set_npc_value( int value ) override;
        int get_npc_value() const override;
        void set_npc_anger( int anger ) override;
        int get_npc_anger() const override;

    protected:
        npc *me_npc;
};
#endif // CATA_SRC_TALKER_NPC_H
