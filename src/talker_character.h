#pragma once
#ifndef CATA_SRC_TALKER_CHARACTER_H
#define CATA_SRC_TALKER_CHARACTER_H

#include <functional>
#include <iosfwd>
#include <list>
#include <vector>

#include "character.h"
#include "coordinates.h"
#include "npc.h"
#include "talker.h"
#include "type_id.h"

class character_id;
class faction;
class item;

class time_duration;
class vehicle;
struct tripoint;

/*
 * Talker wrapper class for const Character access.
 * Should never be invoked directly.  Only talker_avatar and talker_npc are really valid.
 */
class talker_character_const: public talker_cloner<talker_character_const>
{
    public:
        explicit talker_character_const( const Character *new_me ): me_chr_const( new_me ) {
        }
        ~talker_character_const() override = default;

        // underlying element accessor functions
        Character *get_character() override {
            return nullptr;
        }
        const Character *get_character() const override {
            return me_chr_const;
        }
        Creature *get_creature() override {
            return nullptr;
        }
        const Creature *get_creature() const override {
            return me_chr_const;
        }

        // identity and location
        std::string disp_name() const override;
        std::string get_name() const override;
        character_id getID() const override;
        bool is_male() const override;
        std::vector<std::string> get_grammatical_genders() const override;
        int posx() const override;
        int posy() const override;
        int posz() const override;
        tripoint pos() const override;
        tripoint_abs_ms global_pos() const override;
        tripoint_abs_omt global_omt_location() const override;
        int get_cur_hp( const bodypart_id &bp ) const override;
        int get_hp_max( const bodypart_id &bp ) const override;
        units::temperature get_cur_part_temp( const bodypart_id &bp ) const override;

        // stats, skills, traits, bionics, and magic
        int str_cur() const override;
        int dex_cur() const override;
        int int_cur() const override;
        int per_cur() const override;
        int attack_speed() const override;
        int pain_cur() const override;
        double armor_at( damage_type_id &dt, bodypart_id &bp ) const override;
        int coverage_at( bodypart_id & ) const override;
        int encumbrance_at( bodypart_id & ) const override;
        int get_str_max() const override;
        int get_dex_max() const override;
        int get_int_max() const override;
        int get_per_max() const override;
        int get_str_bonus() const override;
        int get_dex_bonus() const override;
        int get_int_bonus() const override;
        int get_per_bonus() const override;
        units::energy power_cur() const override;
        units::energy power_max() const override;
        int mana_cur() const override;
        int mana_max() const override;
        bool has_trait( const trait_id &trait_to_check ) const override;
        bool has_recipe( const recipe_id &recipe_to_check ) const override;
        bool has_flag( const json_character_flag &trait_flag_to_check ) const override;
        bool has_species( const species_id &species ) const override;
        bool bodytype( const bodytype_id &bt ) const override;
        bool crossed_threshold() const override;
        int num_bionics() const override;
        bool has_max_power() const override;
        bool has_bionic( const bionic_id &bionics_id ) const override;
        bool knows_spell( const spell_id &sp ) const override;
        int get_skill_level( const skill_id & ) const override;
        int get_skill_exp( const skill_id &skill, bool raw = false ) const override;
        int get_spell_level( const trait_id & ) const override;
        int get_spell_level( const spell_id & ) const override;
        int get_spell_exp( const spell_id & ) const override;
        int get_highest_spell_level() const override;
        int get_spell_count( const trait_id & ) const override;
        bool knows_proficiency( const proficiency_id &proficiency ) const override;
        time_duration proficiency_practiced_time( const proficiency_id & ) const override;

        // effects and values
        bool has_effect( const efftype_id &effect_id, const bodypart_id &bp ) const override;
        effect get_effect( const efftype_id &effect_id, const bodypart_id &bp ) const override;
        bool is_deaf() const override;
        bool is_mute() const override;
        std::string get_value( const std::string &var_name ) const override;

        // stats, skills, traits, bionics, magic, and proficiencies
        std::vector<skill_id> skills_teacheable() const override;
        std::vector<skill_id> skills_offered_to( const talker &student ) const override;
        std::string skill_seminar_text( const skill_id &s ) const override;
        std::string skill_training_text( const talker &, const skill_id & ) const override;
        std::vector<proficiency_id> proficiencies_teacheable() const override;
        std::vector<proficiency_id> proficiencies_offered_to( const talker &student ) const override;
        std::string proficiency_seminar_text( const proficiency_id & ) const override;
        std::string proficiency_training_text( const talker &student,
                                               const proficiency_id &proficiency ) const override;
        std::vector<matype_id> styles_teacheable() const override;
        std::vector<matype_id> styles_offered_to( const talker &student ) const override;
        std::string style_seminar_text( const matype_id & ) const override;
        std::string style_training_text( const talker &, const matype_id & ) const override;
        std::vector<spell_id> spells_teacheable() const override;
        std::vector<spell_id> spells_offered_to( talker &student ) const override;
        std::string spell_seminar_text( const spell_id & ) const override;
        std::string spell_training_text( talker &, const spell_id & ) const override;

        // inventory, buying, and selling
        bool is_wearing( const itype_id &item_id ) const override;
        int charges_of( const itype_id &item_id ) const override;
        bool has_charges( const itype_id &item_id, int count ) const override;
        bool has_charges( const itype_id &item_id, int count, bool in_tools ) const override;
        bool has_amount( const itype_id &item_id, int count ) const override;
        int get_amount( const itype_id &item_id ) const override;
        int cash() const override;
        std::vector<const item *> const_items_with( const std::function<bool( const item & )> &filter )
        const override;
        bool unarmed_attack() const override;
        bool can_stash_weapon() const override;
        bool has_stolen_item( const talker &guy ) const override;

        // factions and alliances
        faction *get_faction() const override;

        // other descriptors
        std::string short_description() const override;
        bool has_activity() const override;
        bool is_mounted() const override;
        int get_activity_level() const override;
        int get_fatigue() const override;
        int get_hunger() const override;
        int get_thirst() const override;
        int get_instant_thirst() const override;
        int get_stored_kcal() const override;
        int get_healthy_kcal() const override;
        int get_size() const override;
        bool is_in_control_of( const vehicle &veh ) const override;

        bool worn_with_flag( const flag_id &flag, const bodypart_id &bp ) const override;
        bool wielded_with_flag( const flag_id &flag ) const override;
        bool wielded_with_weapon_category( const weapon_category_id &w_cat ) const override;
        bool has_item_with_flag( const flag_id &flag ) const override;
        int item_rads( const flag_id &flag, aggregate_type agg_func ) const override;

        bool can_see() const override;
        int morale_cur() const override;
        int focus_cur() const override;
        int get_rad() const override;
        int get_stim() const override;
        int get_addiction_intensity( const addiction_id &add_id ) const override;
        int get_addiction_turns( const addiction_id &add_id ) const override;
        int get_pkill() const override;
        int get_stamina() const override;
        int get_sleep_deprivation() const override;
        int get_kill_xp() const override;
        int get_age() const override;
        int get_height() const override;
        int get_bmi_permil() const override;
        int get_weight() const override;
        const move_mode_id &get_move_mode() const override;
        int get_fine_detail_vision_mod() const override;
        int get_health() const override;
        units::temperature get_body_temp() const override;
        units::temperature_delta get_body_temp_delta() const override;
        bool knows_martial_art( const matype_id &id ) const override;
        bool using_martial_art( const matype_id &id ) const override;
    protected:
        talker_character_const() = default;
        const Character *me_chr_const;
};

/*
 * Talker wrapper class for mutable Character access.
 * Should never be invoked directly.  Only talker_avatar and talker_npc are really valid.
 */
class talker_character: public talker_cloner<talker_character, talker_character_const>
{
    public:
        explicit talker_character( Character *new_me );
        ~talker_character() override = default;

        // underlying element accessor functions
        Character *get_character() override {
            return me_chr;
        }
        const Character *get_character() const override {
            return me_chr_const;
        }
        Creature *get_creature() override {
            return me_chr;
        }
        const Creature *get_creature() const override {
            return me_chr_const;
        }
        void set_pos( tripoint new_pos ) override;

        // stats, skills, traits, bionics, and magic
        void set_str_max( int value ) override;
        void set_dex_max( int value ) override;
        void set_int_max( int value ) override;
        void set_per_max( int value ) override;
        void set_str_bonus( int value ) override;
        void set_dex_bonus( int value ) override;
        void set_int_bonus( int value ) override;
        void set_per_bonus( int value ) override;
        void set_power_cur( units::energy value ) override;
        void set_mana_cur( int value ) override;
        void set_spell_level( const spell_id &, int ) override;
        void set_spell_exp( const spell_id &, int ) override;
        void set_proficiency_practiced_time( const proficiency_id &prof, int turns ) override;
        void mutate( const int &highest_cat_chance, const bool &use_vitamins ) override;
        void mutate_category( const mutation_category_id &mut_cat, const bool &use_vitamins ) override;
        void set_mutation( const trait_id &new_trait ) override;
        void unset_mutation( const trait_id &old_trait ) override;
        void activate_mutation( const trait_id &trait ) override;
        void deactivate_mutation( const trait_id &trait ) override;
        void set_skill_level( const skill_id &skill, int value ) override;
        void set_skill_exp( const skill_id &skill, int value, bool raw = false ) override;
        void learn_recipe( const recipe_id &recipe_to_learn ) override;
        void forget_recipe( const recipe_id &recipe_to_forget ) override;
        void add_effect( const efftype_id &new_effect, const time_duration &dur,
                         const std::string &bp, bool permanent, bool force, int intensity
                       ) override;
        void remove_effect( const efftype_id &old_effect ) override;
        void set_value( const std::string &var_name, const std::string &value ) override;
        void remove_value( const std::string &var_name ) override;

        // inventory, buying, and selling
        std::vector<item *> items_with( const std::function<bool( const item & )> &filter ) const override;
        std::list<item> use_charges( const itype_id &item_name, int count ) override;
        std::list<item> use_charges( const itype_id &item_name, int count, bool in_tools ) override;
        std::list<item> use_amount( const itype_id &item_name, int count ) override;
        void i_add( const item &new_item ) override;
        void i_add_or_drop( item &new_item, bool force_equip = false ) override;
        void remove_items_with( const std::function<bool( const item & )> &filter ) override;

        void set_stored_kcal( int value ) override;
        void set_thirst( int value ) override;

        // speaking
        void shout( const std::string &speech = "", bool order = false ) override;

        void set_fatigue( int amount ) override;
        void mod_pain( int amount ) override;
        void set_pain( int amount ) override;
        void mod_daily_health( int, int ) override;
        void add_morale( const morale_type &new_morale, int bonus, int max_bonus, time_duration duration,
                         time_duration decay_started, bool capped ) override;
        void remove_morale( const morale_type &old_morale ) override;
        void set_addiction_turns( const addiction_id &add_id, int amount ) override;
        void mod_focus( int ) override;
        void set_rad( int ) override;
        void set_stim( int ) override;
        void set_pkill( int ) override;
        void set_stamina( int ) override;
        void set_sleep_deprivation( int ) override;
        void set_kill_xp( int ) override;
        void set_age( int ) override;
        void set_height( int ) override;
        void add_bionic( const bionic_id &new_bionic ) override;
        void remove_bionic( const bionic_id &old_bionic ) override;
        std::vector<bodypart_id> get_all_body_parts( bool all, bool main_only ) const override;
        int get_part_hp_cur( const bodypart_id &id ) const override;
        int get_part_hp_max( const bodypart_id &id ) const override;
        void set_all_parts_hp_cur( int ) const override;
        void set_part_hp_cur( const bodypart_id &id, int set ) const override;
        bool get_is_alive() const override;
        void die() override;
        void attack_target( Creature &t, bool allow_special, const matec_id &force_technique,
                            bool allow_unarmed, int forced_movecost ) override;
        matec_id get_random_technique( Creature &t, bool crit, bool dodge_counter, bool block_counter,
                                       const std::vector<matec_id> &blacklist = {} )
        const override;
        void learn_martial_art( const matype_id &id ) const override;
        void forget_martial_art( const matype_id &id ) const override;
    protected:
        talker_character() = default;
        Character *me_chr;
};
#endif // CATA_SRC_TALKER_CHARACTER_H
