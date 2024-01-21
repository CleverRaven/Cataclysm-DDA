#pragma once
#ifndef CATA_SRC_TALKER_FURNITURE_H
#define CATA_SRC_TALKER_FURNITURE_H

#include <functional>
#include <iosfwd>
#include <list>
#include <vector>

#include "coordinates.h"
#include "talker.h"
#include "type_id.h"

class computer;
struct tripoint;

/*
 * Talker wrapper class for furniture
 */
class talker_furniture: public talker_cloner<talker_furniture>
{
    public:
        explicit talker_furniture( computer *new_me ): me_comp( new_me ) {
        }
        ~talker_furniture() override = default;

        computer *get_computer() override {
            return me_comp;
        }
        computer *get_computer() const override {
            return me_comp;
        }
        // identity and location
        std::string disp_name() const override;
        int posx() const override;
        int posy() const override;
        int posz() const override;
        tripoint pos() const override;
        tripoint_abs_ms global_pos() const override;
        tripoint_abs_omt global_omt_location() const override;

        std::optional<std::string> maybe_get_value( const std::string &var_name ) const override;
        void set_value( const std::string &var_name, const std::string &value ) override;
        void remove_value( const std::string & ) override;

        std::vector<std::string> get_topics( bool radio_contact ) override;
        bool will_talk_to_u( const Character &you, bool force ) override;

    protected:
        talker_furniture() = default;
        computer *me_comp;
};
#endif // CATA_SRC_TALKER_FURNITURE_H
