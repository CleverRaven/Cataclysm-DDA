#pragma once
#ifndef CATA_SRC_TALKER_ITEM_H
#define CATA_SRC_TALKER_ITEM_H

#include <vector>

#include "coords_fwd.h"
#include "talker.h"
#include "type_id.h"

class item;

struct tripoint;

/*
 * Talker wrapper class for item.
 */
class talker_item_const: public const_talker_cloner<talker_item_const>
{
    public:
        explicit talker_item_const( const item_location *new_me ): me_it_const( new_me ) {}
        talker_item_const() = default;
        talker_item_const( const talker_item_const & ) = default;
        talker_item_const( talker_item_const && ) = delete;
        talker_item_const &operator=( const talker_item_const & ) = default;
        talker_item_const &operator=( talker_item_const && ) = delete;
        ~talker_item_const() override = default;

        item_location const *get_const_item() const override {
            return me_it_const;
        }

        // identity and location
        std::string disp_name() const override;
        std::string get_name() const override;
        int posx() const override;
        int posy() const override;
        int posz() const override;
        tripoint pos() const override;
        tripoint_bub_ms pos_bub() const override;
        tripoint_abs_ms global_pos() const override;
        tripoint_abs_omt global_omt_location() const override;

        std::optional<std::string> maybe_get_value( const std::string &var_name ) const override;

        bool has_flag( const flag_id &f ) const override;

        std::vector<std::string> get_topics( bool radio_contact ) const override;
        bool will_talk_to_u( const Character &you, bool force ) const override;

        int get_cur_hp( const bodypart_id & ) const override;
        int get_degradation() const override;
        int get_hp_max( const bodypart_id & ) const override;
        units::energy power_cur() const override;
        units::energy power_max() const override;

        int get_count() const override;
        int coverage_at( bodypart_id & ) const override;
        int encumbrance_at( bodypart_id & ) const override;
        int get_volume() const override;
        int get_weight() const override;

    private:
        const item_location *me_it_const{};
};

class talker_item: public talker_item_const, public talker_cloner<talker_item>
{
    public:
        explicit talker_item( item_location *new_me ) : talker_item_const( new_me ), me_it( new_me ) {};
        talker_item() = default;
        talker_item( const talker_item & ) = default;
        talker_item( talker_item && ) = delete;
        talker_item &operator=( const talker_item & ) = default;
        talker_item &operator=( talker_item && ) = delete;
        ~talker_item() override = default;

        // underlying element accessor functions
        item_location *get_item() override {
            return me_it;
        }

        void set_value( const std::string &var_name, const std::string &value ) override;
        void remove_value( const std::string & ) override;

        void set_power_cur( units::energy value ) override;
        void set_all_parts_hp_cur( int ) override;
        void set_degradation( int ) override;
        void die() override;

    private:
        item_location *me_it{};
};
#endif // CATA_SRC_TALKER_ITEM_H
