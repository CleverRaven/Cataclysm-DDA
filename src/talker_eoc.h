#pragma once
#ifndef CATA_SRC_TALKER_EOC_H
#define CATA_SRC_TALKER_EOC_H

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
 * talker_eoc is a lightweight dialogue partner that complains if any of it's functions
 * are ever called. This is because the junior partner in an EOC should never be queried
 */
class talker_eoc : public talker
{
        const std::string *_eoc_name = nullptr;

        talker_eoc() = default;

    public:
        talker_eoc( const std::string &eoc ) : _eoc_name( &eoc ) {}

        ~talker_eoc() override = default;
        // virtual member accessor functions
        player *get_character() override;
        player *get_character() const override;
        npc *get_npc() override;
        npc *get_npc() const override;
        item_location *get_item() override;
        item_location *get_item() const override;
        monster *get_monster() override;
        monster *get_monster() const override;
        Creature *get_creature() override;
        Creature *get_creature() const override;
        // identity and location
        std::string disp_name() const override;
        character_id getID() const override;
        bool is_male() const override;
        std::vector<std::string> get_grammatical_genders() const override;
        std::string distance_to_goal() const override;

        // mandatory functions for starting a dialogue
        bool will_talk_to_u( const player &, bool ) override;
        std::vector<std::string> get_topics( bool ) override;
        void check_missions() override;
        void update_missions( const std::vector<mission *> & ) override;
        bool check_hostile_response( int ) const override;
        int parse_mod( const std::string &, int ) const override;
        int trial_chance_mod( const std::string & ) const override;

        // stats, skills, traits, bionics, and magic
        int str_cur() const override;
        int dex_cur() const override;
        int int_cur() const override;
        int per_cur() const override;
        int get_skill_level( const skill_id & ) const override;
        bool has_trait( const trait_id & ) const override;
        void set_mutation( const trait_id & ) override;
        void unset_mutation( const trait_id & ) override;
        void mod_fatigue( int ) override;
        bool has_trait_flag( const json_character_flag & ) const override;
        bool crossed_threshold() const override;
        int num_bionics() const override;
        bool has_max_power() const override;
        bool has_bionic( const bionic_id & ) const override;
        bool knows_spell( const spell_id & ) const override;
        bool knows_proficiency( const proficiency_id & ) const override;
        std::vector<skill_id> skills_offered_to( const talker & ) const override;
        std::string skill_training_text( const talker &, const skill_id & ) const override;
        std::vector<proficiency_id> proficiencies_offered_to( const talker & ) const override;
        std::string proficiency_training_text( const talker &, const proficiency_id & ) const override;
        std::vector<matype_id> styles_offered_to( const talker & ) const override;
        std::string style_training_text( const talker &, const matype_id & ) const override;
        std::vector<spell_id> spells_offered_to( talker & ) override;
        std::string spell_training_text( talker &, const spell_id & ) override;
        void store_chosen_training( const skill_id &, const matype_id &, const spell_id &,
                                    const proficiency_id & ) override;

        // effects and values
        bool has_effect( const efftype_id & ) const override;
        bool is_deaf() const override;
        bool can_see() const override;
        bool is_mute() const override;
        void add_effect( const efftype_id &, const time_duration &, std::string, bool, bool, int ) override;
        void remove_effect( const efftype_id & ) override;
        std::string get_value( const std::string & ) const override;
        void set_value( const std::string &, const std::string & ) override;
        void remove_value( const std::string & ) override;

        // inventory, buying, and selling
        bool is_wearing( const itype_id & ) const override;
        int charges_of( const itype_id & ) const override;
        bool has_charges( const itype_id &, int ) const override;
        std::list<item> use_charges( const itype_id &, int ) override;
        bool has_amount( const itype_id &, int ) const override;
        std::list<item> use_amount( const itype_id &, int ) override;
        int value( const item & ) const override;
        int cash() const override;
        int debt() const override;
        void add_debt( int ) override;
        std::vector<item *> items_with( const std::function<bool( const item & )> & ) const override;
        void i_add( const item & ) override;
        void remove_items_with( const std::function<bool( const item & )> & ) override;
        bool unarmed_attack() const override;
        bool can_stash_weapon() const override;
        bool has_stolen_item( const talker & ) const override;
        int cash_to_favor( int ) const override;
        std::string give_item_to( bool ) override;
        bool buy_from( int ) override;
        void buy_monster( talker &, const mtype_id &, int, int, bool, const translation & ) override;

        // missions
        std::vector<mission *> available_missions() const override;
        std::vector<mission *> assigned_missions() const override;
        mission *selected_mission() const override;
        void select_mission( mission * ) override;
        void add_mission( const mission_type_id & ) override;
        void set_companion_mission( const std::string & ) override;

        // factions and alliances
        faction *get_faction() const override;
        void set_fac( const faction_id & ) override;
        void add_faction_rep( int ) override;
        bool is_following() const override;
        bool is_friendly( const Character & )  const override;
        bool is_player_ally()  const override;
        bool turned_hostile() const override;
        bool is_enemy() const override;
        void make_angry() override;

        // ai rules
        bool has_ai_rule( const std::string &, const std::string & ) const override;
        void toggle_ai_rule( const std::string &, const std::string & ) override;
        void set_ai_rule( const std::string &, const std::string & ) override;
        void clear_ai_rule( const std::string &, const std::string & ) override;

        // other descriptors
        std::string get_job_description() const override;
        std::string evaluation_by( const talker & ) const override;
        std::string short_description() const override;
        bool has_activity() const override;
        bool is_mounted() const override;
        bool is_myclass( const npc_class_id & ) const override;
        void set_class( const npc_class_id & ) override;
        int get_fatigue() const override;
        int get_hunger() const override;
        int get_thirst() const override;
        bool is_in_control_of( const vehicle & ) const override;

        // speaking
        void say( const std::string & ) override;
        void shout( const std::string & = "", bool = false ) override;

        // miscellaneous
        bool enslave_mind() override;
        std::string opinion_text() const override;
        void add_opinion( int /*trust*/, int /*fear*/, int /*value*/, int /*anger*/,
                          int /*debt*/ ) override;
        void set_first_topic( const std::string & ) override;
        bool is_safe() const override;
        void mod_pain( int ) override;
        int pain_cur() const override;
        bool worn_with_flag( const flag_id & ) const override;
        bool wielded_with_flag( const flag_id & ) const override;
        units::energy power_cur() const override;
        void mod_healthy_mod( int, int ) override;
        int morale_cur() const override;
        int focus_cur() const override;
        void mod_focus( int ) override;
        void add_morale( const morale_type &, int, int, time_duration, time_duration, bool ) override;
        void remove_morale( const morale_type & ) override;

        // Position
        int posx() const override;
        int posy() const override;
        int posz() const override;
        tripoint pos() const override;
        tripoint_abs_omt global_omt_location() const override;

};
#endif // CATA_SRC_TALKER_EOC_H
