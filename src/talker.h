#pragma once
#ifndef CATA_SRC_TALKER_H
#define CATA_SRC_TALKER_H

/*
 * Talker is an entity independent way of providing a participant in a dialogue.
 * Talker is a virtual abstract class and should never really be used.  Instead,
 * entity specific talker child classes such as character_talker should be used.
 */
class talker
{
        // virtual functions called in condition.cpp
        virtual std::string disp_name() const = { return ""; };
        virtual bool has_trait( const &trait_id trait_to_check ) const = { return false; }
                virtual bool crossed_threshold() const = { return false; };
        virtual bool has_activity() const = { return false; };
        virtual bool is_mounted() const = { return false; };
        virtual bool myclass( const npc_class_id &class_to_check ) const = { return false; };
        virtual int str_cur() const = { return 0; };
        virtual int str_dex() const = { return 0; };
        virtual int str_int() const = { return 0; };
        virtual int str_per() const = { return 0; };
        virtual bool is_wearing( const itype_id &item_id ) const = { return false; };
        virtual bool charges_of( const itype_id &item_id ) const = { return false; };
        virtual bool has_charges( const itype_id &item_id, const int count ) const = { return false; };
        virtual bool has_amount( const itype_id &item_id, const int count ) const = { return false; };
        virtual something items_with( const item_category_id &category_id ) const = { return something };
        virtual int num_bionics() const = { return 0; };
        virtual bool has_max_power() const = { return false; };
        virtual bool has_bionic( const bionic_id &bionics_id ) const = { return false; };
        virtual bool has_effect( const efftype_id &effect_id ) const = { return false; };
        virtual bool get_fatigue() const = { return 0; };
        virtual bool get_hunger() const = { return 0; };
        virtual bool get_thirst() const = { return 0; };
        virtual tripont global_omt_location() const = { return tripoint_zero; };
        virtual std::string get_value( const std::string &var_name ) const = { return ""; };
        virtual int posx() const = { return 0; };
        virtual int posy() const = { return 0; };
        virtual int posz() const = { return 0; };
        virtual tripoint pos() const = { return tripoint_zero; };
        virtual int cash() const = { return 0; };
        virtual int debt() const = { return 0; };
        virtual bool has_ai_rule( const std::string &type, const std::string &rule ) const = { return 0; };
        virtual bool is_male() const = { return false; };
        virtual std::vector<missions *> available_missions() const = { return {}; };
        virtual mission *selected_mission() const = { return nullptr; };
        virtual bool is_following() const = { return false; };
        virtual bool is_friendly( const talker &guy ) const = { return false; };
        virtual bool is_enemy() const = { return false; };
        virtual bool has_skills_to_train( const talker &guy ) const = { return false; };
        virtual bool has_spells_to_train( const talker &guy ) const = { return false; };
        virtual bool unarmed_attack() const = { return false; };
        virtual bool can_stash_weapon() const = { return false; };
        virtual bool has_stolen_item( const talker &guy ) const = { return false; };
        virtual int get_skill_level( const &skill_id skill ) const = { return false; };
        virtual bool knows_recipe( const recipe &rep ) const = { return false; };
        // virtual functions called in npctalk.cpp
        virtual faction *get_faction() const = { return nullptr; };
        virtual bool turned_hostile() const = { return false; };
        // virtual functions called in npctalk.cpp
        virtual void set_companion_mission( const std::string &role_id ) = {};
        virtual void add_effect( const efftype_id &new_effect, const time_duration &dur,
                                 const bool permanent ) = {};
        virtual void remove_effect( const efftype_id &old_effect ) = {};
        virtual void set_mutation( const trait_id &new_trait ) = {};
        virtual void unset_mutation( const trait_id &old_trait ) = {};
        virtual void set_value( const std::string &var_name, const std::string &value ) = {};
        virtual void remove_value( const std::string &var_name ) = {};
        virtual void i_add( const item &new_item ) = {};
        virtual void add_debt( const int cost ) = {};
        virtual void use_charges( const itype_id &item_name, const int count ) = {};
        virtual void use_amount( const itype_id &item_name, const int count ) = {};
        virtual void remove_items with( const item &it ) = {};
        virtual void spend_cash( const int amount ) = {};
        virtual void set_fac( const faction_id &new_fac_name ) = {};
        virtual void set_class( const npc_class_id &new_class ) = {};
        virtual void add_faction_rep( const int rep_change ) = {};
        virtual void toggle_ai_rule( const std::string &type, const std::string &rule ) = {};
        virtual void set_ai_rule( const std::string &type, const std::string &rule ) = {};
        virtual void clear_ai_rule( const std::string &type, const std::string &rule ) = {};
        virtual void give_item_to( const bool to_use ) = {};
        virtual void add_mission( const std::string &mission_id ) = {};
        virtual void buy_monster( const talker &seller, const mtype_id &mtype ) = {};
        virtual void learn_recipe( const &recipe r ) = {};
        virtual void add_opinion( const int trust, const int fear, const int value, const int anger ) =
            {};
        virtual void make_angry() = {};
};



















};
#endif
