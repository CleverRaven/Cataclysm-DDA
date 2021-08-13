#pragma once
#ifndef CATA_SRC_TALKER_BASE_H
#define CATA_SRC_TALKER_BASE_H

#include "coordinates.h"
#include "units.h"
#include "talker.h"
#include "units_fwd.h"
#include <list>

class faction;
class item;
class item_location;
class mission;
class monster;
class npc;
class player;
class recipe;
struct tripoint;
class vehicle;

/*
 * talker_base is a virtual class providing decent defaults for an actual
 * entity specific subclass. It should never really be used
 */
class talker_base : public talker
{
    public:
        virtual ~talker_base() override = default;
        // virtual member accessor functions
        virtual player *get_character() override {
            return nullptr;
        }
        virtual player *get_character() const override {
            return nullptr;
        }
        virtual npc *get_npc() override {
            return nullptr;
        }
        virtual npc *get_npc() const override {
            return nullptr;
        }
        virtual item_location *get_item() override {
            return nullptr;
        }
        virtual item_location *get_item() const override {
            return nullptr;
        }
        virtual monster *get_monster() override {
            return nullptr;
        }
        virtual monster *get_monster() const override {
            return nullptr;
        }
        virtual Creature *get_creature() override {
            return nullptr;
        }
        virtual Creature *get_creature() const override {
            return nullptr;
        }
        // identity and location
        virtual std::string disp_name() const override {
            return "";
        }
        virtual character_id getID() const override {
            return character_id( 0 );
        }
        virtual bool is_male() const override {
            return false;
        }
        virtual std::vector<std::string> get_grammatical_genders() const override {
            return {};
        }
        virtual std::string distance_to_goal() const override {
            return "";
        }

        // mandatory functions for starting a dialogue
        virtual bool will_talk_to_u( const player &, bool ) override {
            return false;
        }
        virtual std::vector<std::string> get_topics( bool ) override {
            return {};
        }
        virtual void check_missions() override {}
        virtual void update_missions( const std::vector<mission *> & ) override {}
        virtual bool check_hostile_response( int ) const override {
            return false;
        }
        virtual int parse_mod( const std::string &, int ) const override {
            return 0;
        }
        virtual int trial_chance_mod( const std::string & ) const override {
            return 0;
        }

        // stats, skills, traits, bionics, and magic
        virtual int str_cur() const override {
            return 0;
        }
        virtual int dex_cur() const override {
            return 0;
        }
        virtual int int_cur() const override {
            return 0;
        }
        virtual int per_cur() const override {
            return 0;
        }
        virtual int get_skill_level( const skill_id & ) const override {
            return 0;
        }
        virtual bool has_trait( const trait_id & ) const override {
            return false;
        }
        virtual void set_mutation( const trait_id & ) override {}
        virtual void unset_mutation( const trait_id & ) override {}
        virtual void mod_fatigue( int ) override {}
        virtual bool has_trait_flag( const json_character_flag & ) const override {
            return false;
        }
        virtual bool crossed_threshold() const override {
            return false;
        }
        virtual int num_bionics() const override {
            return 0;
        }
        virtual bool has_max_power() const override {
            return false;
        }
        virtual bool has_bionic( const bionic_id & ) const override {
            return false;
        }
        virtual bool knows_spell( const spell_id & ) const override {
            return false;
        }
        virtual bool knows_proficiency( const proficiency_id & ) const override {
            return false;
        }
        virtual std::vector<skill_id> skills_offered_to( const talker & ) const override {
            return {};
        }
        virtual std::string skill_training_text( const talker &, const skill_id & ) const override {
            return {};
        }
        virtual std::vector<proficiency_id> proficiencies_offered_to( const talker & ) const override {
            return {};
        }
        virtual std::string proficiency_training_text( const talker &,
                const proficiency_id & ) const override {
            return {};
        }
        virtual std::vector<matype_id> styles_offered_to( const talker & ) const override {
            return {};
        }
        virtual std::string style_training_text( const talker &, const matype_id & ) const override {
            return {};
        }
        virtual std::vector<spell_id> spells_offered_to( talker & ) override {
            return {};
        }
        virtual std::string spell_training_text( talker &, const spell_id & ) override {
            return {};
        }
        virtual void store_chosen_training( const skill_id &, const matype_id &,
                                            const spell_id &, const proficiency_id & ) override {}

        // effects and values
        virtual bool has_effect( const efftype_id & ) const override {
            return false;
        }
        virtual bool is_deaf() const override {
            return false;
        }
        virtual bool can_see() const override {
            return false;
        }
        virtual bool is_mute() const override {
            return false;
        }
        virtual void add_effect( const efftype_id &, const time_duration &, std::string, bool, bool,
                                 int ) override {}
        virtual void remove_effect( const efftype_id & ) override {}
        virtual std::string get_value( const std::string & ) const override {
            return "";
        }
        virtual void set_value( const std::string &, const std::string & ) override {}
        virtual void remove_value( const std::string & ) override {}

        // inventory, buying, and selling
        virtual bool is_wearing( const itype_id & ) const override {
            return false;
        }
        virtual int charges_of( const itype_id & ) const override {
            return 0;
        }
        virtual bool has_charges( const itype_id &, int ) const override {
            return false;
        }
        virtual std::list<item> use_charges( const itype_id &, int ) override {
            return {};
        }
        virtual bool has_amount( const itype_id &, int ) const override {
            return false;
        }
        virtual std::list<item> use_amount( const itype_id &, int ) override {
            return {};
        }
        virtual int value( const item & ) const override {
            return 0;
        }
        virtual int cash() const override {
            return 0;
        }
        virtual int debt() const override {
            return 0;
        }
        virtual void add_debt( int ) override {}
        virtual std::vector<item *> items_with( const std::function<bool( const item & )> & ) const
        override {
            return {};
        }
        virtual void i_add( const item & ) override {}
        virtual void remove_items_with( const std::function<bool( const item & )> & ) override {}
        virtual bool unarmed_attack() const override {
            return false;
        }
        virtual bool can_stash_weapon() const override {
            return false;
        }
        virtual bool has_stolen_item( const talker & ) const override {
            return false;
        }
        virtual int cash_to_favor( int ) const override {
            return 0;
        }
        virtual std::string give_item_to( bool ) override {
            return _( "Nope." );
        }
        virtual bool buy_from( int ) override {
            return false;
        }
        virtual void buy_monster( talker &, const mtype_id &, int, int, bool,
                                  const translation & ) override {}

        // missions
        virtual std::vector<mission *> available_missions() const override {
            return {};
        }
        virtual std::vector<mission *> assigned_missions() const override {
            return {};
        }
        virtual mission *selected_mission() const override {
            return nullptr;
        }
        virtual void select_mission( mission * ) override {
        }
        virtual void add_mission( const mission_type_id & ) override {}
        virtual void set_companion_mission( const std::string & ) override {}

        // factions and alliances
        virtual faction *get_faction() const override {
            return nullptr;
        }
        virtual void set_fac( const faction_id & ) override {}
        virtual void add_faction_rep( int ) override {}
        virtual bool is_following() const override {
            return false;
        }
        virtual bool is_friendly( const Character & )  const override {
            return false;
        }
        virtual bool is_player_ally()  const override {
            return false;
        }
        virtual bool turned_hostile() const override {
            return false;
        }
        virtual bool is_enemy() const override {
            return false;
        }
        virtual void make_angry() override {}

        // ai rules
        virtual bool has_ai_rule( const std::string &, const std::string & ) const override {
            return false;
        }
        virtual void toggle_ai_rule( const std::string &, const std::string & ) override {}
        virtual void set_ai_rule( const std::string &, const std::string & ) override {}
        virtual void clear_ai_rule( const std::string &, const std::string & ) override {}

        // other descriptors
        virtual std::string get_job_description() const override {
            return "";
        }
        virtual std::string evaluation_by( const talker & ) const override {
            return "";
        }
        virtual std::string short_description() const override {
            return "";
        }
        virtual bool has_activity() const override {
            return false;
        }
        virtual bool is_mounted() const override {
            return false;
        }
        virtual bool is_myclass( const npc_class_id & ) const override {
            return false;
        }
        virtual void set_class( const npc_class_id & ) override {}
        virtual int get_fatigue() const override {
            return 0;
        }
        virtual int get_hunger() const override {
            return 0;
        }
        virtual int get_thirst() const override {
            return 0;
        }
        virtual bool is_in_control_of( const vehicle & ) const override {
            return false;
        }

        // speaking
        virtual void say( const std::string & ) override {}
        virtual void shout( const std::string & = "", bool = false ) override {}

        // miscellaneous
        virtual bool enslave_mind() override {
            return false;
        }
        virtual std::string opinion_text() const override {
            return "";
        }
        virtual void add_opinion( int /*trust*/, int /*fear*/, int /*value*/, int /*anger*/,
                                  int /*debt*/ ) override {}
        virtual void set_first_topic( const std::string & ) override {}
        virtual bool is_safe() const override {
            return true;
        }
        virtual void mod_pain( int ) override {}
        virtual int pain_cur() const override {
            return 0;
        }
        virtual bool worn_with_flag( const flag_id & ) const override {
            return false;
        }
        virtual bool wielded_with_flag( const flag_id & ) const override {
            return false;
        }
        virtual units::energy power_cur() const override {
            return 0_kJ;
        }
        virtual void mod_healthy_mod( int, int ) override {}
        virtual int morale_cur() const override {
            return 0;
        }
        virtual int focus_cur() const override {
            return 0;
        }
        virtual void mod_focus( int ) override {}
        virtual void add_morale( const morale_type &, int, int, time_duration, time_duration,
                                 bool ) override {}
        virtual void remove_morale( const morale_type & ) override {}
};
#endif // CATA_SRC_TALKER_BASE_H
