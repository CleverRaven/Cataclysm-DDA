#pragma once
#ifndef CATA_SRC_TALKER_TOPIC_H
#define CATA_SRC_TALKER_TOPIC_H

#include <functional>
#include <iosfwd>
#include <list>
#include <vector>

#include "coordinates.h"
#include "talker.h"
#include "type_id.h"

struct tripoint;

/*
 * Talker wrapper class for an empty talker thats just topics
 */
class talker_topic: public talker_cloner<talker_topic>
{
    public:
        explicit talker_topic( std::vector<std::string> new_topics ) {
            topics = std::move( new_topics );
        }
        ~talker_topic() override = default;

        // identity and location
        std::string disp_name() const override;
        int posx() const override;
        int posy() const override;
        int posz() const override;
        tripoint pos() const override;
        tripoint_abs_ms global_pos() const override;
        tripoint_abs_omt global_omt_location() const override;

        void set_value( const std::string &var_name, const std::string &value ) override;
        void remove_value( const std::string & ) override;

        std::vector<std::string> get_topics( bool radio_contact ) override;
        bool will_talk_to_u( const Character &you, bool force ) override;

    protected:
        talker_topic() = default;
        std::vector<std::string> topics;
};
std::unique_ptr<talker> get_talker_for( const std::vector<std::string> &topics );
#endif // CATA_SRC_TALKER_TOPIC_H
