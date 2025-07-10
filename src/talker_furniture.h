#pragma once
#ifndef CATA_SRC_TALKER_FURNITURE_H
#define CATA_SRC_TALKER_FURNITURE_H

#include <memory>
#include <string>

#include "coords_fwd.h"
#include "talker.h"

class computer;

/*
 * Talker wrapper class for furniture
 */
class talker_furniture_const: public const_talker_cloner<talker_furniture_const>
{
    public:
        explicit talker_furniture_const( computer *new_me ): me_comp( new_me ) {};
        talker_furniture_const() = default;
        talker_furniture_const( const talker_furniture_const & ) = default;
        talker_furniture_const( talker_furniture_const && ) = delete;
        talker_furniture_const &operator=( const talker_furniture_const & ) = default;
        talker_furniture_const &operator=( talker_furniture_const && ) = delete;
        ~talker_furniture_const() override = default;

        computer const *get_const_computer() const override {
            return me_comp;
        }
        // identity and location
        std::string disp_name() const override;
        int posx( const map &here ) const override;
        int posy( const map &here ) const override;
        int posz() const override;
        tripoint_bub_ms pos_bub( const map &here ) const override;
        tripoint_abs_ms pos_abs() const override;
        tripoint_abs_omt pos_abs_omt() const override;

        diag_value const *maybe_get_value( const std::string &var_name ) const override;

        std::vector<std::string> get_topics( bool radio_contact ) const override;
        bool will_talk_to_u( const Character &you, bool force ) const override;

    private:
        computer *me_comp{};
};
class talker_furniture: public talker_furniture_const, public talker_cloner<talker_furniture>
{
    public:
        explicit talker_furniture( computer *new_me ): talker_furniture_const( new_me ),
            me_comp( new_me ) {};
        talker_furniture() = default;
        talker_furniture( const talker_furniture & ) = default;
        talker_furniture( talker_furniture && ) = delete;
        talker_furniture &operator=( const talker_furniture & ) = default;
        talker_furniture &operator=( talker_furniture && ) = delete;
        ~talker_furniture() override = default;

        computer *get_computer() override {
            return me_comp;
        }

        void set_value( const std::string &var_name, diag_value const &value ) override;
        void remove_value( const std::string & ) override;

    private:
        computer *me_comp{};
};
#endif // CATA_SRC_TALKER_FURNITURE_H
