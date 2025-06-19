#pragma once
#ifndef CATA_SRC_TALKER_TOPIC_H
#define CATA_SRC_TALKER_TOPIC_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "talker.h"

/*
 * Talker wrapper class for an empty talker thats just topics
 */
class talker_topic_const: public const_talker_cloner<talker_topic_const>
{
    public:
        explicit talker_topic_const( std::vector<std::string> new_topics )
            : topics( std::move( new_topics ) ) {}
        talker_topic_const() = default;
        talker_topic_const( const talker_topic_const & ) = default;
        talker_topic_const( talker_topic_const && ) = delete;
        talker_topic_const &operator=( const talker_topic_const & ) = default;
        talker_topic_const &operator=( talker_topic_const && ) = delete;
        ~talker_topic_const() override = default;

        std::vector<std::string> get_topics( bool radio_contact ) const override;
        bool will_talk_to_u( const Character &/* you */, bool /* force */ ) const override {
            return true;
        }

    private:
        std::vector<std::string> topics;
};

class talker_topic: virtual public talker_topic_const, public talker_cloner<talker_topic>
{
    public:
        explicit talker_topic( std::vector<std::string> new_topics )
            : talker_topic_const( std::move( new_topics ) ) {}
        talker_topic() = default;
        talker_topic( const talker_topic & ) = default;
        talker_topic( talker_topic && ) = delete;
        talker_topic &operator=( const talker_topic & ) = default;
        talker_topic &operator=( talker_topic && ) = delete;
        ~talker_topic() override = default;
};

std::unique_ptr<talker> get_talker_for( const std::vector<std::string> &topics );

#endif // CATA_SRC_TALKER_TOPIC_H
