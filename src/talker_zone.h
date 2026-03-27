#pragma once
#ifndef CATA_SRC_TALKER_ZONE_H
#define CATA_SRC_TALKER_ZONE_H

#include <memory>

#include "coords_fwd.h"
#include "talker.h"

class zone_data;

/*
 * Talker wrapper class for zone.
 */
class talker_zone_const : public const_talker_cloner<talker_zone_const>
{
    public:
        explicit talker_zone_const( const zone_data *new_me ) : me_zone_const( new_me ) {};
        talker_zone_const() = default;
        talker_zone_const( const talker_zone_const & ) = default;
        talker_zone_const( talker_zone_const && ) = delete;
        talker_zone_const &operator=( const talker_zone_const & ) = default;
        talker_zone_const &operator=( talker_zone_const && ) = delete;
        ~talker_zone_const() override = default;

        zone_data const *get_const_zone() const override {
            return me_zone_const;
        }

        tripoint_abs_ms pos_abs() const override;

    private:
        const zone_data *me_zone_const{};
};

class talker_zone : public talker_zone_const, public talker_cloner<talker_zone>
{
    public:
        explicit talker_zone( zone_data *new_me ) : talker_zone_const( new_me ), me_zone( new_me ) {};
        talker_zone() = default;
        talker_zone( const talker_zone & ) = default;
        talker_zone( talker_zone && ) = delete;
        talker_zone &operator=( const talker_zone & ) = default;
        talker_zone &operator=( talker_zone && ) = delete;
        ~talker_zone() override = default;

        // underlying element accessor functions
        zone_data *get_zone() override {
            return me_zone;
        }
    private:
        zone_data *me_zone{};
};

#endif // CATA_SRC_TALKER_ZONE_H
#pragma once
