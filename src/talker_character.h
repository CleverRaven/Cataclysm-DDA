#pragma once
#ifndef CATA_SRC_TALKER_CHARACTER_H
#define CATA_SRC_TALKER_CHARACTER_H

#include <functional>
#include <iosfwd>
#include <list>
#include <vector>

#include "coordinates.h"
#include "npc.h"
#include "talker.h"
#include "type_id.h"

class character_id;
class faction;
class item;
class player;
class time_duration;
class vehicle;
struct tripoint;

/*
 * Talker wrapper class for Character.  well, ideally, but since Character is such a mess,
 * it's the wrapper class for player
 * Should never be invoked directly.  Only talker_avatar and talker_npc are really valid.
 */
class talker_character: public talker
{
    public:
        explicit talker_character( player *new_me ): me_chr( new_me ) {
        }
        ~talker_character() override = default;

        // underlying element accessor functions
        player *get_character() override {
            return me_chr;
        }
        player *get_character() const override {
            return me_chr;
        }
        // identity and location
        std::string disp_name() const override;
        character_id getID() const override;
        bool is_male() const override;
        std::vector<std::string> get_grammatical_genders() const override;
        int posx() const override;
        int posy() const override;
        int posz() const override;
        tripoint pos() const override;
        tripoint_abs_omt global_omt_location() const override;

        // stats, skills, traits, bionics, and magic
        int str_cur() const override;
        int dex_cur() const override;
        int int_cur() const override;
        int per_cur() const override;
        int pain_cur() const override;
        units::energy power_cur() const override;
        bool has_trait( const trait_id &trait_to_check ) const override;
        void set_mutation( const trait_id &new_trait ) override;
        void unset_mutation( const trait_id &old_trait ) override;
        bool has_trait_flag( const json_character_flag &trait_flag_to_check ) const override;
        bool crossed_threshold() const override;
        int num_bionics() const override;
        bool has_max_power() const override;
        bool has_bionic( const bionic_id &bionics_id ) const override;
        bool knows_spell( const spell_id &sp ) const override;
        int get_skill_level( const skill_id &skill ) const override;
        bool knows_proficiency( const proficiency_id &proficiency ) const override;

        // effects and values
        bool has_effect( const efftype_id &effect_id ) const override;
        bool is_deaf() const override;
        bool is_mute() const override;
        void add_effect( const efftype_id &new_effect, const time_duration &dur,
                         std::string bp, bool permanent, bool force, int intensity ) override;
        void remove_effect( const efftype_id &old_effect ) override;
        std::string get_value( const std::string &var_name ) const override;
        void set_value( const std::string &var_name, const std::string &value ) override;
        void remove_value( const std::string &var_name ) override;

        // inventory, buying, and selling
        bool is_wearing( const itype_id &item_id ) const override;
        int charges_of( const itype_id &item_id ) const override;
        bool has_charges( const itype_id &item_id, int count ) const override;
        std::list<item> use_charges( const itype_id &item_name, int count ) override;
        bool has_amount( const itype_id &item_id, int count ) const override;
        std::list<item> use_amount( const itype_id &item_name, int count ) override;
        int cash() const override;
        std::vector<item *> items_with( const std::function<bool( const item & )> &filter ) const override;
        void i_add( const item &new_item ) override;
        void remove_items_with( const std::function<bool( const item & )> &filter ) override;
        bool unarmed_attack() const override;
        bool can_stash_weapon() const override;
        bool has_stolen_item( const talker &guy ) const override;

        // factions and alliances
        faction *get_faction() const override;

        // other descriptors
        std::string short_description() const override;
        bool has_activity() const override;
        bool is_mounted() const override;
        int get_fatigue() const override;
        int get_hunger() const override;
        int get_thirst() const override;
        bool is_in_control_of( const vehicle &veh ) const override;

        // speaking
        void shout( const std::string &speech = "", bool order = false ) override;

        bool worn_with_flag( const flag_id &flag ) const override;
        bool wielded_with_flag( const flag_id &flag ) const override;

        void mod_fatigue( int amount ) override;
        void mod_pain( int amount ) override;
        bool can_see() const override;
        void mod_healthy_mod( int, int ) override;
    protected:
        talker_character() = default;
        player *me_chr;
};
#endif // CATA_SRC_TALKER_CHARACTER_H
