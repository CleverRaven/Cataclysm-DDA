#pragma once
#ifndef CATA_SRC_TALKER_VEHICLE_H
#define CATA_SRC_TALKER_VEHICLE_H

#include <functional>
#include <iosfwd>
#include <list>
#include <vector>

#include "coords_fwd.h"
#include "talker.h"
#include "type_id.h"

struct tripoint;


/*
 * Talker wrapper class for vehicle.
 */
class talker_vehicle_const: public talker_cloner<talker_vehicle_const>
{
    public:
        explicit talker_vehicle_const( const vehicle *new_me ): me_veh_const( new_me ) {
        }
        ~talker_vehicle_const() override = default;

        // identity and location
        std::string disp_name() const override;
        std::string get_name() const override;
        int posx() const override;
        int posy() const override;
        int posz() const override;
        tripoint pos() const override;
        tripoint_abs_ms global_pos() const override;
        tripoint_abs_omt global_omt_location() const override;

        std::optional<std::string> maybe_get_value( const std::string &var_name ) const override;

        std::vector<std::string> get_topics( bool radio_contact ) override;
        bool will_talk_to_u( const Character &you, bool force ) override;

        int get_weight() const override;
        int unloaded_weight() const override;
        bool is_driven() const override;
        int vehicle_facing() const override;
        bool can_fly() const override;
        bool is_flying() const override;
        bool can_float() const override;
        bool is_floating() const override;
        int current_speed() const override;
        int friendly_passenger_count() const override;
        int hostile_passenger_count() const override;
        bool has_part_flag( const std::string &flag, bool enabled ) const override;
        bool is_falling() const override;
        bool is_skidding() const override;
        bool is_sinking() const override;
        bool is_on_rails() const override;
        bool is_remote_controlled() const override;
        bool is_passenger( Character & ) const override;
    protected:
        talker_vehicle_const() = default;
        const vehicle *me_veh_const;
};

class talker_vehicle: public talker_cloner<talker_vehicle, talker_vehicle_const>
{
    public:
        explicit talker_vehicle( vehicle *new_me );
        ~talker_vehicle() override = default;

        // underlying element accessor functions
        vehicle *get_vehicle() override {
            return me_veh;
        }
        vehicle *get_vehicle() const override {
            return me_veh;
        }

        void set_value( const std::string &var_name, const std::string &value ) override;
        void remove_value( const std::string & ) override;

    protected:
        talker_vehicle() = default;
        vehicle *me_veh;
};

#endif // CATA_SRC_TALKER_VEHICLE_H
#pragma once
