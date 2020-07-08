#pragma once
#ifndef CATA_SRC_CHARACTER_TALKER_H
#define CATA_SRC_CHARACTER_TALKER_H

#include "talker.h"

class character;
/*
 */
class talker_character : public talker
{
    public:
        talker_character( character &me );
        // override functions called in condition.cpp
        std::string disp_name() const override;
        bool has_trait( const &trait_id trait_to_check ) const override
        bool crossed_threshold() const override;
        bool has_activity() const override;
        bool is_mounted() const override;
        bool myclass( const npc_class_id &class_to_check ) const override;
        int str_cur() const override;
        int str_dex() const override;
        int str_int() const override;
        int str_per() const override;
        bool is_wearing( const itype_id &item_id ) const override;
        bool charges_of( const itype_id &item_id ) const override;
        bool has_charges( const itype_id &item_id, const int count ) const override;
        bool has_amount( const itype_id &item_id, const int count ) const override;
        something items_with( const item_category_id &category_id ) const override;
        int num_bionics() const override;
        bool has_max_power() const override;
        bool has_bionic( const bionic_id &bionics_id ) override;
        bool has_effect( const efftype_id &effect_id ) override;
        bool get_fatigue() const override;
        bool get_hunger() const override;
        bool get_thirst() const override;
        tripont global_omt_location() const override;
        std::string get_value( const std::string &var_name ) const override;
        int posx() const override;
        int posy() const override;
        int posz() const override;
        tripoint pos() const override;
        int cash() const override;
        int debt() const override;
        bool has_ai_rule( const std::string &type, const std::string &rule ) const override;
        bool is_male() const override;
        std::vector<missions *> available_missions() const override;
        mission *selected_mission() const override;
        bool is_following() const override;
        bool is_friendly( const talker &guy ) const override;
        bool is_enemy() const override;
        bool has_skills_to_train( const talker &guy ) override;
        bool has_spells_to_train( const talker &guy ) override;
        bool unarmed_attack() const override;
        bool can_stash_weapon() const override;
        bool has_stolen_item( const talker &guy ) const override;
        int get_skill_level( const &skill_id skill ) const override;
        bool knows_recipe( const recipe &rep ) const override;
        // override functions called in npctalk.cpp
        faction *get_faction() const override;
        bool turned_hostile() const override;
        // override functions called in npctalk.cpp
        void set_companion_mission( const std::string &role_id ) override;
        void add_effect( const efftype_id &new_effect, const time_duration &dur,
                         const bool permanent ) override;
        void remove_effect( const efftype_id &old_effect ) override;
        void set_mutation( const trait_id &new_trait ) override;
        void unset_mutation( const trait_id &old_trait ) override;
        void set_value( const std::string &var_name, const std::string &value ) override;
        void remove_value( const std::string &var_name ) override;
        void i_add( const item &new_item ) override;
        void add_debt( const int cost ) override;
        void use_charges( const itype_id &item_name, const int count ) override;
        void use_amount( const itype_id &item_name, const int count ) override;
        void remove_items with( const item &it ) override;
        void spend_cash( const int amount ) override;
        void set_fac( const faction_id &new_fac_name ) override;
        void set_class( const npc_class_id &new_class ) override;
        void add_faction_rep( const int rep_change ) override;
        void toggle_ai_rule( const std::string &type, const std::string &rule ) override;
        void set_ai_rule( const std::string &type, const std::string &rule ) override;
        void clear_ai_rule( const std::string &type, const std::string &rule ) override;
        void give_item_to( const bool to_use ) override;
        void add_mission( const std::string &mission_id ) override;
        void buy_monster( const talker &seller, const mtype_id &mtype ) override;
        void learn_recipe( const &recipe r ) override;
        void add_opinion( const int trust, const int fear, const int value, const int anger ) override;
        void make_angry() override;
    private:
        character &me;
};



















};

#endif
