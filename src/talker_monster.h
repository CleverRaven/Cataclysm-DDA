#pragma once
#ifndef CATA_SRC_TALKER_MONSTER_H
#define CATA_SRC_TALKER_MONSTER_H

#include <memory>
#include <string>

#include "bodypart.h"
#include "coords_fwd.h"
#include "monster.h"
#include "talker.h"
#include "type_id.h"

/*
 * Talker wrapper class for monster.
 */
class talker_monster_const: public const_talker_cloner<talker_monster_const>
{
    public:
        explicit talker_monster_const( const monster *new_me ): me_mon_const( new_me ) {}
        talker_monster_const() = default;
        talker_monster_const( const talker_monster_const & ) = default;
        talker_monster_const( talker_monster_const && ) = delete;
        talker_monster_const &operator=( const talker_monster_const & ) = default;
        talker_monster_const &operator=( talker_monster_const && ) = delete;
        ~talker_monster_const() override = default;

        monster const *get_const_monster() const override {
            return me_mon_const;
        }
        Creature const *get_const_creature() const override {
            return me_mon_const;
        }

        // identity and location
        std::string disp_name() const override;
        std::string get_name() const override;

        int posx( const map &here ) const override;
        int posy( const map &here ) const override;
        int posz() const override;
        tripoint_bub_ms pos_bub( const map &here ) const override;
        tripoint_abs_ms pos_abs() const override;
        tripoint_abs_omt pos_abs_omt() const override;

        int pain_cur() const override;

        int perceived_pain_cur() const override;

        // effects and values
        bool has_effect( const efftype_id &effect_id, const bodypart_id &bp ) const override;
        effect get_effect( const efftype_id &effect_id, const bodypart_id &bp ) const override;

        diag_value const *maybe_get_value( const std::string &var_name ) const override;

        bool has_flag( const flag_id &f ) const override;
        bool has_species( const species_id &species ) const override;
        bool bodytype( const bodytype_id &bt ) const override;

        std::string short_description() const override;
        int get_anger() const override;
        int get_difficulty() const override;
        int morale_cur() const override;
        int get_friendly() const override;
        int get_size() const override;
        int get_speed() const override;
        int get_grab_strength() const override;
        std::vector<std::string> get_topics( bool radio_contact ) const override;
        bool will_talk_to_u( const Character &u, bool force ) const override;
        int get_cur_hp( const bodypart_id & ) const override;
        int get_hp_max( const bodypart_id & ) const override;
        double armor_at( damage_type_id &dt, bodypart_id &bp ) const override;

        bool can_see_location( const tripoint_bub_ms &pos ) const override;
        int get_volume() const override;
        int get_weight() const override;
        bool is_warm() const override;

    private:
        const monster *me_mon_const{};
};

class talker_monster: public talker_monster_const, public talker_cloner<talker_monster>
{
    public:
        explicit talker_monster( monster *new_me ): talker_monster_const( new_me ), me_mon( new_me ) {};
        talker_monster() = default;
        talker_monster( const talker_monster & ) = default;
        talker_monster( talker_monster && ) = delete;
        talker_monster &operator=( const talker_monster & ) = default;
        talker_monster &operator=( talker_monster && ) = delete;
        ~talker_monster() override = default;

        monster *get_monster() override {
            return me_mon;
        }
        Creature *get_creature() override {
            return me_mon;
        }

        // effects and values
        void add_effect( const efftype_id &new_effect, const time_duration &dur,
                         const std::string &bp, bool permanent, bool force, int intensity
                       ) override;
        void remove_effect( const efftype_id &old_effect, const std::string &bp ) override;
        void mod_pain( int amount ) override;

        void set_value( const std::string &var_name, diag_value const &value ) override;
        void remove_value( const std::string &var_name ) override;

        void set_anger( int ) override;
        void set_morale( int ) override;
        void set_friendly( int ) override;
        bool get_is_alive() const override;
        void die( map *here ) override;

        void set_all_parts_hp_cur( int ) override;
        dealt_damage_instance deal_damage( Creature *source, bodypart_id bp,
                                           const damage_instance &dam ) const override;

    private:
        monster *me_mon{};
};
#endif // CATA_SRC_TALKER_MONSTER_H
