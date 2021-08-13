#pragma once
#ifndef CATA_SRC_TALKER_H
#define CATA_SRC_TALKER_H

#include "coordinates.h"
#include "units.h"
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
 * Talker is an entity independent way of providing a participant in a dialogue.
 * Talker is a virtual abstract class and can never be used.  Instead,
 * entity specific talker child classes such as character_talker should be used.
 */
class talker
{
    public:
        virtual ~talker() = default;
        // virtual member accessor functions
        virtual player *get_character() = 0;
        virtual player *get_character() const = 0;
        virtual npc *get_npc() = 0;
        virtual npc *get_npc() const = 0;
        virtual item_location *get_item() = 0;
        virtual item_location *get_item() const = 0;
        virtual monster *get_monster() = 0;
        virtual monster *get_monster() const = 0;
        virtual Creature *get_creature() = 0;
        virtual Creature *get_creature() const = 0;
        // identity and location
        virtual std::string disp_name() const = 0;
        virtual character_id getID() const = 0;
        virtual bool is_male() const = 0;
        virtual std::vector<std::string> get_grammatical_genders() const = 0;

        virtual int posx() const = 0;
        virtual int posy() const = 0;
        virtual int posz() const = 0;
        virtual tripoint pos() const = 0;
        virtual tripoint_abs_omt global_omt_location() const = 0;
        virtual std::string distance_to_goal() const = 0;

        // mandatory functions for starting a dialogue
        virtual bool will_talk_to_u( const player &, bool ) = 0;
        virtual std::vector<std::string> get_topics( bool ) = 0;

        virtual void check_missions() = 0;
        virtual void update_missions( const std::vector<mission *> & ) = 0;
        virtual bool check_hostile_response( int ) const = 0;
        virtual int parse_mod( const std::string &, int ) const = 0;
        virtual int trial_chance_mod( const std::string & ) const = 0;

        // stats, skills, traits, bionics, and magic
        virtual int str_cur() const = 0;
        virtual int dex_cur() const = 0;
        virtual int int_cur() const = 0;
        virtual int per_cur() const = 0;
        virtual int get_skill_level( const skill_id & ) const = 0;
        virtual bool has_trait( const trait_id & ) const = 0;
        virtual void set_mutation( const trait_id & ) = 0;
        virtual void unset_mutation( const trait_id & ) = 0;
        virtual void mod_fatigue( int ) = 0;
        virtual bool has_trait_flag( const json_character_flag & ) const = 0;
        virtual bool crossed_threshold() const = 0;
        virtual int num_bionics() const = 0;
        virtual bool has_max_power() const = 0;
        virtual bool has_bionic( const bionic_id & ) const = 0;
        virtual bool knows_spell( const spell_id & ) const = 0;
        virtual bool knows_proficiency( const proficiency_id & ) const = 0;
        virtual std::vector<skill_id> skills_offered_to( const talker & ) const = 0;
        virtual std::string skill_training_text( const talker &, const skill_id & ) const = 0;
        virtual std::vector<proficiency_id> proficiencies_offered_to( const talker & ) const = 0;
        virtual std::string proficiency_training_text( const talker &, const proficiency_id & ) const = 0;
        virtual std::vector<matype_id> styles_offered_to( const talker & ) const = 0;
        virtual std::string style_training_text( const talker &, const matype_id & ) const = 0;
        virtual std::vector<spell_id> spells_offered_to( talker & ) = 0;
        virtual std::string spell_training_text( talker &, const spell_id & ) = 0;
        virtual void store_chosen_training( const skill_id &, const matype_id &,
                                            const spell_id &, const proficiency_id & ) = 0;

        // effects and values
        virtual bool has_effect( const efftype_id & ) const = 0;
        virtual bool is_deaf() const = 0;
        virtual bool can_see() const = 0;
        virtual bool is_mute() const = 0;
        virtual void add_effect( const efftype_id &, const time_duration &, std::string, bool, bool,
                                 int ) = 0;
        virtual void remove_effect( const efftype_id & ) = 0;
        virtual std::string get_value( const std::string & ) const = 0;
        virtual void set_value( const std::string &, const std::string & ) = 0;
        virtual void remove_value( const std::string & ) = 0;

        // inventory, buying, and selling
        virtual bool is_wearing( const itype_id & ) const = 0;
        virtual int charges_of( const itype_id & ) const = 0;
        virtual bool has_charges( const itype_id &, int ) const = 0;
        virtual std::list<item> use_charges( const itype_id &, int ) = 0;
        virtual bool has_amount( const itype_id &, int ) const = 0;
        virtual std::list<item> use_amount( const itype_id &, int ) = 0;
        virtual int value( const item & ) const = 0;
        virtual int cash() const = 0;
        virtual int debt() const = 0;
        virtual void add_debt( int ) = 0;
        virtual std::vector<item *> items_with( const std::function<bool( const item & )> & ) const = 0;
        virtual void i_add( const item & ) = 0;
        virtual void remove_items_with( const std::function<bool( const item & )> & ) = 0;
        virtual bool unarmed_attack() const = 0;
        virtual bool can_stash_weapon() const = 0;
        virtual bool has_stolen_item( const talker & ) const = 0;
        virtual int cash_to_favor( int ) const = 0;
        virtual std::string give_item_to( bool ) = 0;
        virtual bool buy_from( int ) = 0;
        virtual void buy_monster( talker &, const mtype_id &, int, int, bool,
                                  const translation & ) = 0;

        // missions
        virtual std::vector<mission *> available_missions() const = 0;
        virtual std::vector<mission *> assigned_missions() const = 0;
        virtual mission *selected_mission() const = 0;
        virtual void select_mission( mission * ) = 0;
        virtual void add_mission( const mission_type_id & ) = 0;
        virtual void set_companion_mission( const std::string & ) = 0;

        // factions and alliances
        virtual faction *get_faction() const = 0;
        virtual void set_fac( const faction_id & ) = 0;
        virtual void add_faction_rep( int ) = 0;
        virtual bool is_following() const = 0;
        virtual bool is_friendly( const Character & )  const = 0;
        virtual bool is_player_ally()  const = 0;
        virtual bool turned_hostile() const = 0;
        virtual bool is_enemy() const = 0;
        virtual void make_angry() = 0;

        // ai rules
        virtual bool has_ai_rule( const std::string &, const std::string & ) const = 0;
        virtual void toggle_ai_rule( const std::string &, const std::string & ) = 0;
        virtual void set_ai_rule( const std::string &, const std::string & ) = 0;
        virtual void clear_ai_rule( const std::string &, const std::string & ) = 0;

        // other descriptors
        virtual std::string get_job_description() const = 0;
        virtual std::string evaluation_by( const talker & ) const = 0;
        virtual std::string short_description() const = 0;
        virtual bool has_activity() const = 0;
        virtual bool is_mounted() const = 0;
        virtual bool is_myclass( const npc_class_id & ) const = 0;
        virtual void set_class( const npc_class_id & ) = 0;
        virtual int get_fatigue() const = 0;
        virtual int get_hunger() const = 0;
        virtual int get_thirst() const = 0;
        virtual bool is_in_control_of( const vehicle & ) const = 0;

        // speaking
        virtual void say( const std::string & ) = 0;
        virtual void shout( const std::string & = "", bool = false ) = 0;

        // miscellaneous
        virtual bool enslave_mind() = 0;
        virtual std::string opinion_text() const = 0;
        virtual void add_opinion( int /*trust*/, int /*fear*/, int /*value*/, int /*anger*/,
                                  int /*debt*/ ) = 0;
        virtual void set_first_topic( const std::string & ) = 0;
        virtual bool is_safe() const = 0;
        virtual void mod_pain( int ) = 0;
        virtual int pain_cur() const = 0;
        virtual bool worn_with_flag( const flag_id & ) const = 0;
        virtual bool wielded_with_flag( const flag_id & ) const = 0;
        virtual units::energy power_cur() const = 0;
        virtual void mod_healthy_mod( int, int ) = 0;
        virtual int morale_cur() const = 0;
        virtual int focus_cur() const = 0;
        virtual void mod_focus( int ) = 0;
        virtual void add_morale( const morale_type &, int, int, time_duration, time_duration, bool ) = 0;
        virtual void remove_morale( const morale_type & ) = 0;
};
#endif // CATA_SRC_TALKER_H
