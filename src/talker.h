#pragma once
#ifndef CATA_SRC_TALKER_H
#define CATA_SRC_TALKER_H

#include "coords_fwd.h"
#include "effect.h"
#include "item.h"
#include "math_parser_diag_value.h"
#include "messages.h"
#include "type_id.h"
#include "units.h"
#include "units_fwd.h"
#include <list>

class computer;
class faction;
class item_location;
class map;
class mission;
class monster;
class npc;
struct npc_opinion;
class Character;
class recipe;
struct tripoint;
class vehicle;
struct mutation_variant;
enum class get_body_part_flags;

namespace npc_factions
{
enum class relationship : int;
} // namespace npc_factions

using bodytype_id = std::string;

/*
 * Talker is an entity independent way of providing a participant in a dialogue.
 * Talker is a virtual abstract class and should never really be used.  Instead,
 * entity specific talker child classes such as character_talker should be used.
 */
class const_talker
{
    public:
        const_talker() = default;
        const_talker( const const_talker & ) = default;
        const_talker( const_talker && ) = delete;
        const_talker &operator=( const const_talker & ) = default;
        const_talker &operator=( const_talker && ) = delete;
        virtual ~const_talker() = default;

        virtual std::unique_ptr<const_talker> const_clone() const {
            return std::make_unique<const_talker>();
        }
        // virtual member accessor functions
        virtual Character const *get_const_character() const {
            return nullptr;
        }
        virtual npc const *get_const_npc() const {
            return nullptr;
        }
        virtual item_location const *get_const_item() const {
            return nullptr;
        }
        virtual monster const *get_const_monster() const {
            return nullptr;
        }
        virtual Creature const *get_const_creature() const {
            return nullptr;
        }
        virtual computer const *get_const_computer() const {
            return nullptr;
        }
        virtual vehicle const *get_const_vehicle() const {
            return nullptr;
        }

        // identity and location
        virtual std::string disp_name() const {
            return "";
        }
        virtual std::string get_name() const {
            return "";
        }
        virtual character_id getID() const {
            return character_id( 0 );
        }
        virtual bool is_male() const {
            return false;
        }
        virtual std::vector<std::string> get_grammatical_genders() const {
            return {};
        }
        virtual int posx( const map & ) const {
            return 0;
        }
        virtual int posy( const map & ) const {
            return 0;
        }
        virtual int posz() const {
            return 0;
        }
        virtual tripoint_bub_ms pos_bub( const map & ) const {
            return {};
        }
        virtual tripoint_abs_ms pos_abs() const {
            return {};
        }
        virtual tripoint_abs_omt pos_abs_omt() const {
            return {};
        }
        virtual std::string distance_to_goal() const {
            return "";
        }

        // mandatory functions for starting a dialogue
        virtual bool will_talk_to_u( const Character &, bool ) const {
            return false;
        }
        virtual std::vector<std::string> get_topics( bool ) const {
            return {};
        }
        virtual bool check_hostile_response( int ) const {
            return false;
        }
        virtual int parse_mod( const std::string &, int ) const {
            return 0;
        }
        virtual int trial_chance_mod( const std::string & ) const {
            return 0;
        }
        virtual int get_cur_hp( const bodypart_id & ) const {
            return 0;
        }
        virtual int get_degradation() const {
            return 0;
        }
        virtual int get_hp_max( const bodypart_id & ) const {
            return 0;
        }
        virtual int get_count() const {
            return 0;
        }
        virtual units::temperature get_cur_part_temp( const bodypart_id & ) const {
            return 0_K;
        }
        virtual bool get_is_alive() const {
            return false;
        }

        virtual bool is_warm() const {
            return false;
        }
        // stats, skills, traits, bionics, and magic
        virtual int str_cur() const {
            return 0;
        }
        virtual int dex_cur() const {
            return 0;
        }
        virtual int int_cur() const {
            return 0;
        }
        virtual int per_cur() const {
            return 0;
        }
        virtual int get_str_max() const {
            return 0;
        }
        virtual int get_dex_max() const {
            return 0;
        }
        virtual int get_int_max() const {
            return 0;
        }
        virtual int get_per_max() const {
            return 0;
        }
        virtual int get_str_bonus() const {
            return 0;
        }
        virtual int get_dex_bonus() const {
            return 0;
        }
        virtual int get_int_bonus() const {
            return 0;
        }
        virtual int get_per_bonus() const {
            return 0;
        }
        virtual int get_skill_level( const skill_id & ) const {
            return 0;
        }
        virtual int get_skill_exp( const skill_id &, bool ) const {
            return 0;
        }
        virtual int get_spell_level( const trait_id & ) const {
            return 0;
        }
        virtual int get_spell_level( const spell_id & ) const {
            return 0;
        }
        virtual int get_spell_difficulty( const spell_id & ) const {
            return 0;
        }
        virtual int get_spell_exp( const spell_id & ) const {
            return 0;
        }
        virtual int get_highest_spell_level() const {
            return 0;
        }
        virtual int get_spell_count( const trait_id & ) const {
            return 0;
        }
        virtual int get_spell_sum( const trait_id &, int ) const {
            return 0;
        }
        virtual bool has_trait( const trait_id & ) const {
            return false;
        }
        virtual int get_total_in_category( const mutation_category_id &, enum mut_count_type ) const {
            return 0;
        }
        virtual int get_total_in_category_char_has( const mutation_category_id &,
                enum mut_count_type ) const {
            return 0;
        }
        virtual bool is_trait_purifiable( const trait_id & ) const {
            return false;
        }
        virtual bool has_recipe( const recipe_id & ) const {
            return false;
        }
        virtual int get_daily_calories( int, std::string const & ) const {
            return 0;
        }
        virtual bool has_flag( const json_character_flag & ) const {
            return false;
        }
        virtual bool has_species( const species_id & ) const {
            return false;
        }
        virtual bool bodytype( const bodytype_id & ) const {
            return false;
        }
        virtual int get_grab_strength() const {
            return 0;
        }
        virtual bool crossed_threshold() const {
            return false;
        }
        virtual int num_bionics() const {
            return 0;
        }
        virtual bool has_max_power() const {
            return false;
        }
        virtual bool has_bionic( const bionic_id & ) const {
            return false;
        }
        virtual bool knows_spell( const spell_id & ) const {
            return false;
        }
        virtual bool knows_proficiency( const proficiency_id & ) const {
            return false;
        }
        virtual time_duration proficiency_practiced_time( const proficiency_id & ) const {
            return 0_seconds;
        }
        virtual std::vector<skill_id> skills_offered_to( const_talker const & ) const {
            return {};
        }
        virtual std::vector<skill_id> skills_teacheable() const {
            return {};
        }
        virtual std::string skill_seminar_text( const skill_id & ) const {
            return {};
        }
        virtual std::string skill_training_text( const_talker const &, const skill_id & ) const {
            return {};
        }
        virtual std::vector<proficiency_id> proficiencies_teacheable() const {
            return {};
        }
        virtual std::vector<proficiency_id> proficiencies_offered_to( const_talker const & ) const {
            return {};
        }
        virtual std::string proficiency_seminar_text( const proficiency_id & ) const {
            return {};
        }
        virtual std::string proficiency_training_text( const_talker const &,
                const proficiency_id & ) const {
            return {};
        }
        virtual std::vector<matype_id> styles_teacheable() const {
            return {};
        }
        virtual std::vector<matype_id> styles_offered_to( const_talker const & ) const {
            return {};
        }
        virtual std::string style_seminar_text( const matype_id & ) const {
            return {};
        }
        virtual std::string style_training_text( const_talker const &, const matype_id & ) const {
            return {};
        }
        virtual std::vector<spell_id> spells_teacheable() const {
            return {};
        }
        virtual std::vector<spell_id> spells_offered_to( const_talker const & ) const {
            return {};
        }
        virtual std::string spell_seminar_text( const spell_id & ) const {
            return {};
        }
        virtual std::string spell_training_text( const_talker const &, const spell_id & ) const {
            return {};
        }

        // effects and values
        virtual bool has_effect( const efftype_id &, const bodypart_id & ) const {
            return false;
        }
        virtual effect get_effect( const efftype_id &, const bodypart_id & ) const {
            return effect::null_effect;
        }
        virtual bool is_deaf() const {
            return false;
        }
        virtual bool can_see() const {
            return false;
        }
        virtual bool can_see_location( const tripoint_bub_ms & ) const {
            return false;
        }
        virtual bool is_mute() const {
            return false;
        }
        diag_value const &get_value( const std::string &key ) const {
            static diag_value const null_val;
            diag_value const *ret = maybe_get_value( key );
            return ret ? *ret : null_val;
        }

        virtual diag_value const *maybe_get_value( const std::string & ) const {
            return nullptr;
        }

        // inventory, buying, and selling
        virtual bool is_wearing( const itype_id & ) const {
            return false;
        }
        virtual int charges_of( const itype_id & ) const {
            return 0;
        }
        virtual bool has_charges( const itype_id &, int ) const {
            return false;
        }
        // bool = match tool containing charges of itype_id
        virtual bool has_charges( const itype_id &, int, bool ) const {
            return false;
        }
        virtual bool has_amount( const itype_id &, int ) const {
            return false;
        }
        virtual int get_amount( const itype_id & ) const {
            return 0;
        }
        virtual int value( const item & ) const {
            return 0;
        }
        virtual int cash() const {
            return 0;
        }
        virtual int debt() const {
            return 0;
        }
        virtual int sold() const {
            return 0;
        }
        virtual std::vector<const item *> const_items_with( const std::function<bool( const item & )> & )
        const {
            return {};
        }
        virtual bool unarmed_attack() const {
            return false;
        }
        virtual bool can_stash_weapon() const {
            return false;
        }
        virtual bool has_stolen_item( const_talker const & ) const {
            return false;
        }
        virtual int cash_to_favor( int ) const {
            return 0;
        }

        // missions
        virtual std::vector<mission *> available_missions() const {
            return {};
        }
        virtual std::vector<mission *> assigned_missions() const {
            return {};
        }
        virtual mission *selected_mission() const {
            return nullptr;
        }

        // factions and alliances
        virtual faction *get_faction() const {
            return nullptr;
        }
        virtual bool is_following() const {
            return false;
        }
        virtual bool is_friendly( const Character & )  const {
            return false;
        }
        virtual bool is_player_ally()  const {
            return false;
        }
        virtual bool turned_hostile() const {
            return false;
        }
        virtual bool is_enemy() const {
            return false;
        }

        // ai rules
        virtual bool has_ai_rule( const std::string &, const std::string & ) const {
            return false;
        }

        // other descriptors
        virtual std::string get_job_description() const {
            return "";
        }
        virtual std::string evaluation_by( const_talker const & ) const {
            return "";
        }
        virtual std::string view_personality_traits() const {
            return "";
        }
        virtual std::string short_description() const {
            return "";
        }
        virtual bool has_activity() const {
            return false;
        }
        virtual bool is_mounted() const {
            return false;
        }
        virtual bool is_myclass( const npc_class_id & ) const {
            return false;
        }
        virtual int get_activity_level() const {
            return 0;
        }
        virtual int get_sleepiness() const {
            return 0;
        }
        virtual int get_hunger() const {
            return 0;
        }
        virtual int get_thirst() const {
            return 0;
        }
        virtual int get_instant_thirst() const {
            return 0;
        }
        virtual int get_stored_kcal() const {
            return 0;
        }
        virtual int get_healthy_kcal() const {
            return 0;
        }
        virtual int get_size() const {
            return 0;
        }
        virtual int get_stim() const {
            return 0;
        }
        virtual int get_addiction_intensity( const addiction_id & ) const {
            return 0;
        }
        virtual int get_addiction_turns( const addiction_id & ) const {
            return 0;
        }
        virtual bool is_in_control_of( const vehicle & ) const {
            return false;
        }

        virtual std::string opinion_text() const {
            return "";
        }
        virtual bool is_safe() const {
            return true;
        }
        virtual int pain_cur() const {
            return 0;
        }
        virtual int perceived_pain_cur() const {
            return 0;
        }
        virtual int attack_speed() const {
            return 0;
        }
        virtual int get_speed() const {
            return 0;
        }
        virtual dealt_damage_instance deal_damage( Creature *, bodypart_id,
                const damage_instance & ) const {
            return dealt_damage_instance();
        };
        virtual double armor_at( damage_type_id &, bodypart_id & ) const {
            return 0;
        }
        virtual int coverage_at( bodypart_id & ) const {
            return 0;
        }
        virtual int encumbrance_at( bodypart_id & ) const {
            return 0;
        }
        virtual bool worn_with_flag( const flag_id &, const bodypart_id & ) const {
            return false;
        }
        virtual bool wielded_with_flag( const flag_id & ) const {
            return false;
        }
        virtual bool wielded_with_weapon_category( const weapon_category_id & ) const {
            return false;
        }
        virtual bool wielded_with_weapon_skill( const skill_id & ) const {
            return false;
        }
        virtual bool wielded_with_item_ammotype( const ammotype & ) const {
            return false;
        }
        virtual bool has_item_with_flag( const flag_id & ) const {
            return false;
        }
        virtual int item_rads( const flag_id &, aggregate_type ) const {
            return 0;
        }
        virtual units::energy power_cur() const {
            return 0_kJ;
        }
        virtual units::energy power_max() const {
            return 0_kJ;
        }
        virtual int mana_cur() const {
            return 0;
        }
        virtual int mana_max() const {
            return 0;
        }
        virtual int morale_cur() const {
            return 0;
        }
        virtual int focus_cur() const {
            return 0;
        }
        virtual int focus_effective_cur() const {
            return 0;
        }
        virtual int get_pkill() const {
            return 0;
        }
        virtual int get_stamina() const {
            return 0;
        }
        virtual int get_sleep_deprivation() const {
            return 0;
        }
        virtual int get_rad() const {
            return 0;
        }
        virtual int get_anger() const {
            return 0;
        }
        virtual int get_friendly() const {
            return 0;
        }
        virtual int get_difficulty() const {
            return 0;
        }
        virtual int get_kill_xp() const {
            return 0;
        }
        virtual int get_age() const {
            return 0;
        }
        virtual int get_height() const {
            return 0;
        }
        virtual int get_volume() const {
            return 0;
        }
        virtual int get_weight() const {
            return 0;
        }
        virtual int get_npc_trust() const {
            return 0;
        }
        virtual int get_npc_fear() const {
            return 0;
        }
        virtual int get_npc_value() const {
            return 0;
        }
        virtual int get_npc_anger() const {
            return 0;
        }
        virtual int get_ugliness() const {
            return 0;
        }
        virtual int get_bmi_permil() const {
            return 0;
        }
        virtual const move_mode_id &get_move_mode() const {
            return move_mode_id::NULL_ID();
        }
        virtual int get_fine_detail_vision_mod() const {
            return 0;
        }
        virtual int get_health() const {
            return 0;
        }
        virtual units::temperature get_body_temp() const {
            return 0_K;
        }
        virtual units::temperature_delta get_body_temp_delta() const {
            return 0_C_delta;
        }
        virtual std::vector<bodypart_id> get_all_body_parts( get_body_part_flags /* flags */ ) const {
            return std::vector<bodypart_id>();
        }
        virtual int get_part_hp_cur( const bodypart_id & ) const {
            return 0;
        }
        virtual int get_part_hp_max( const bodypart_id & ) const {
            return 0;
        }
        virtual matec_id get_random_technique( Creature const &, bool, bool,
                                               bool, const std::vector<matec_id> & = {} ) const {
            return matec_id();
        }
        virtual bool knows_martial_art( const matype_id & ) const {
            return false;
        }
        virtual bool using_martial_art( const matype_id & ) const {
            return false;
        }
        virtual int climate_control_str_heat() const {
            return 0;
        }
        virtual int climate_control_str_chill() const {
            return 0;
        }
        virtual int get_quality( const std::string &, bool ) const {
            return 0;
        }
        virtual bool is_driven() const {
            return false;
        }
        virtual bool is_remote_controlled() const {
            return false;
        }
        virtual int get_vehicle_facing() const {
            return 0;
        }
        virtual bool can_fly() const {
            return false;
        }
        virtual bool is_flying() const {
            return false;
        }
        virtual bool can_float() const {
            return false;
        }
        virtual bool is_floating() const {
            return false;
        }
        virtual bool is_falling() const {
            return false;
        }
        virtual bool is_skidding() const {
            return false;
        }
        virtual bool is_sinking() const {
            return false;
        }
        virtual bool is_on_rails() const {
            return false;
        }
        virtual int get_current_speed() const {
            return 0;
        }
        virtual int get_unloaded_weight() const {
            return 0;
        }
        virtual int get_friendly_passenger_count() const {
            return 0;
        }
        virtual int get_hostile_passenger_count() const {
            return 0;
        }
        virtual bool has_part_flag( const std::string &, bool ) const {
            return false;
        }
        virtual bool is_passenger( Character & ) const {
            return false;
        }
};

class talker: virtual public const_talker
{
    public:
        talker() = default;
        talker( const talker & ) = default;
        talker( talker && ) = delete;
        talker &operator=( const talker & ) = default;
        talker &operator=( talker && ) = delete;
        ~talker() noexcept override = default;

        virtual std::unique_ptr<talker> clone() const {
            return std::make_unique<talker>();
        }

        virtual Character *get_character() {
            return nullptr;
        }
        virtual npc *get_npc() {
            return nullptr;
        }
        virtual item_location *get_item() {
            return nullptr;
        }
        virtual monster *get_monster() {
            return nullptr;
        }
        virtual Creature *get_creature() {
            return nullptr;
        }
        virtual computer *get_computer() {
            return nullptr;
        }
        virtual vehicle *get_vehicle() {
            return nullptr;
        }
        virtual void set_pos( tripoint_bub_ms ) {}
        virtual void set_pos( tripoint_abs_ms ) {}
        virtual void update_missions( const std::vector<mission *> & ) {}
        virtual void set_str_max( int ) {}
        virtual void set_dex_max( int ) {}
        virtual void set_int_max( int ) {}
        virtual void set_per_max( int ) {}
        virtual void set_str_bonus( int ) {}
        virtual void set_dex_bonus( int ) {}
        virtual void set_int_bonus( int ) {}
        virtual void set_per_bonus( int ) {}
        virtual void set_cash( int ) {}
        virtual void set_spell_level( const spell_id &, int ) {}
        virtual void set_spell_exp( const spell_id &, int ) {}
        virtual void set_skill_level( const skill_id &, int ) {}
        virtual void set_skill_exp( const skill_id &, int, bool ) {}
        virtual void learn_recipe( const recipe_id & ) {}
        virtual void forget_recipe( const recipe_id & ) {}
        virtual void mutate( const int &, const bool & ) {}
        virtual void mutate_category( const mutation_category_id &, const bool &, const bool & ) {}
        virtual void mutate_towards( const trait_id &, const mutation_category_id &, const bool & ) {};
        virtual void set_mutation( const trait_id &, const mutation_variant * = nullptr ) {}
        virtual void unset_mutation( const trait_id & ) {}
        virtual void activate_mutation( const trait_id & ) {}
        virtual void deactivate_mutation( const trait_id & ) {}
        virtual void set_trait_purifiability( const trait_id &, const bool & ) {}
        virtual void set_sleepiness( int ) {};
        virtual void set_proficiency_practiced_time( const proficiency_id &, int ) {}
        virtual void train_proficiency_for( const proficiency_id &, int ) {}
        virtual void store_chosen_training( const skill_id &, const matype_id &,
                                            const spell_id &, const proficiency_id & ) {
        }
        virtual void add_effect( const efftype_id &, const time_duration &, const std::string &, bool, bool,
                                 int ) {}
        virtual void remove_effect( const efftype_id &, const std::string & ) {}
        virtual void add_bionic( const bionic_id & ) {}
        virtual void remove_bionic( const bionic_id & ) {}
        virtual void set_value( const std::string &, diag_value const & ) {}
        template <typename... Args>
        void set_value( const std::string &key, Args... args ) {
            set_value( key, diag_value{ std::forward<Args>( args )... } );
        }
        virtual void remove_value( const std::string & ) {}
        virtual std::list<item> use_charges( const itype_id &, int ) {
            return {};
        }
        // bool = match tool containing charges of itype_id
        virtual std::list<item> use_charges( const itype_id &, int, bool ) {
            return {};
        }
        virtual std::list<item> use_amount( const itype_id &, int ) {
            return {};
        }
        virtual void add_debt( int ) {}
        virtual void i_add( const item & ) {}
        virtual void i_add_or_drop( item &, bool = false ) {}
        virtual void remove_items_with( const std::function<bool( const item & )> & ) {}
        virtual std::string give_item_to( bool ) {
            return _( "Nope." );
        }
        virtual bool buy_from( int ) {
            return false;
        }
        virtual bool buy_monster( talker &, const mtype_id &, int, int, bool,
                                  const translation & ) {
            return false;
        }
        virtual void select_mission( mission * ) {
        }
        virtual void check_missions() {}
        virtual void add_mission( const mission_type_id & ) {}
        virtual void set_companion_mission( const std::string & ) {}
        virtual void set_fac( const faction_id & ) {}
        virtual void add_faction_rep( int ) {}
        virtual void make_angry() {}
        virtual void add_sold( int ) {}
        virtual void toggle_ai_rule( const std::string &, const std::string & ) {}
        virtual void set_ai_rule( const std::string &, const std::string & ) {}
        virtual void clear_ai_rule( const std::string &, const std::string & ) {}
        virtual void set_class( const npc_class_id & ) {}
        virtual void set_addiction_turns( const addiction_id &, int ) {}
        virtual void mod_stored_kcal( int, bool ) {}
        virtual void set_stored_kcal( int ) {}
        virtual void set_stim( int ) {}
        virtual void set_thirst( int ) {}
        virtual void say( const std::string & ) {}
        virtual void shout( const std::string & = "", bool = false ) {}
        virtual bool enslave_mind() {
            return false;
        }
        virtual void add_opinion( const npc_opinion & ) {}
        virtual void set_first_topic( const std::string & ) {}
        virtual void mod_pain( int ) {}
        virtual void set_pain( int ) {}
        virtual void set_power_cur( units::energy ) {}
        virtual void set_part_hp_cur( const bodypart_id &, int ) {}
        virtual void set_sleep_deprivation( int ) {}
        virtual void set_rad( int ) {}
        virtual void set_anger( int ) {}
        virtual void set_morale( int ) {}
        virtual void set_friendly( int ) {}
        virtual void add_morale( const morale_type &, int, int, time_duration, time_duration, bool ) {}
        virtual void remove_morale( const morale_type & ) {}
        virtual void set_kill_xp( int ) {}
        virtual void set_age( int ) {}
        virtual void set_height( int ) {}
        virtual void set_npc_trust( int ) {}
        virtual void set_npc_fear( int ) {}
        virtual void set_npc_value( int ) {}
        virtual void set_npc_anger( int ) {}
        virtual void set_all_parts_hp_cur( int ) {}
        virtual void set_degradation( int ) {}
        virtual void die( map * ) {}
        virtual void set_fault( const fault_id &, bool, bool ) {};
        virtual void set_random_fault_of_type( const std::string &, bool, bool ) {};
        virtual void set_mana_cur( int ) {}
        virtual void mod_daily_health( int, int ) {}
        virtual void mod_livestyle( int ) {}
        virtual void mod_focus( int ) {}
        virtual void set_pkill( int ) {}
        virtual void set_stamina( int ) {}
        virtual void learn_martial_art( const matype_id & ) {}
        virtual void forget_martial_art( const matype_id & ) {}
        virtual void attack_target( Creature &, bool, const matec_id &, bool, int ) {}
        virtual void set_fac_relation( const Character *, npc_factions::relationship, bool ) {}
        virtual std::vector<item *> items_with( const std::function<bool( const item & )> & ) {
            return {};
        }
};

template <class T, class B = const_talker>
class const_talker_cloner : virtual public B
{
    public:
        std::unique_ptr<const_talker> const_clone() const override {
            return std::make_unique<T>( static_cast<T const &>( *this ) );
        }
};

template <class T, class B = talker>
class talker_cloner : virtual public B
{
    public:
        std::unique_ptr<talker> clone() const override {
            return std::make_unique<T>( static_cast<T const &>( *this ) );
        }
};
#endif // CATA_SRC_TALKER_H
