#pragma once
#ifndef CATA_SRC_TALKER_VEHICLE_H
#define CATA_SRC_TALKER_VEHICLE_H

#include <memory>
#include <string>

#include "coords_fwd.h"
#include "talker.h"

class vehicle;

/*
 * Talker wrapper class for vehicle.
 */
class talker_vehicle_const: public const_talker_cloner<talker_vehicle_const>
{
    public:
        explicit talker_vehicle_const( const vehicle *new_me ) : me_veh_const( new_me ) {};
        talker_vehicle_const() = default;
        talker_vehicle_const( const talker_vehicle_const & ) = default;
        talker_vehicle_const( talker_vehicle_const && ) = delete;
        talker_vehicle_const &operator=( const talker_vehicle_const & ) = default;
        talker_vehicle_const &operator=( talker_vehicle_const && ) = delete;
        ~talker_vehicle_const() override = default;

        vehicle const *get_const_vehicle() const override {
            return me_veh_const;
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

        diag_value const *maybe_get_value( const std::string &var_name ) const override;

        std::vector<std::string> get_topics( bool radio_contact ) const override;
        bool will_talk_to_u( const Character &you, bool force ) const override;

        int get_weight() const override;
        int get_unloaded_weight() const override;
        bool is_driven() const override;
        int get_vehicle_facing() const override;
        bool can_fly() const override;
        bool is_flying() const override;
        bool can_float() const override;
        bool is_floating() const override;
        int get_current_speed() const override;
        int get_friendly_passenger_count() const override;
        int get_hostile_passenger_count() const override;
        bool has_part_flag( const std::string &flag, bool enabled ) const override;
        bool is_falling() const override;
        bool is_skidding() const override;
        bool is_sinking() const override;
        bool is_on_rails() const override;
        bool is_remote_controlled() const override;
        bool is_passenger( Character & ) const override;
    private:
        const vehicle *me_veh_const{};
};

class talker_vehicle: public talker_vehicle_const, public talker_cloner<talker_vehicle>
{
    public:
        explicit talker_vehicle( vehicle *new_me ) : talker_vehicle_const( new_me ), me_veh( new_me ) {};
        talker_vehicle() = default;
        talker_vehicle( const talker_vehicle & ) = default;
        talker_vehicle( talker_vehicle && ) = delete;
        talker_vehicle &operator=( const talker_vehicle & ) = default;
        talker_vehicle &operator=( talker_vehicle && ) = delete;
        ~talker_vehicle() override = default;

        // underlying element accessor functions
        vehicle *get_vehicle() override {
            return me_veh;
        }

        void set_value( const std::string &var_name, diag_value const &value ) override;
        void remove_value( const std::string & ) override;

    private:
        vehicle *me_veh{};
};

#endif // CATA_SRC_TALKER_VEHICLE_H
#pragma once
