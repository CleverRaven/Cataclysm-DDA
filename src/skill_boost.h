#pragma once
#ifndef CATA_SRC_SKILL_BOOST_H
#define CATA_SRC_SKILL_BOOST_H

#include <algorithm>
#include <string>
#include <vector>

#include "optional.h"
#include "string_id.h"

class JsonObject;
template<typename T>
class generic_factory;

class skill_boost
{
    public:
        skill_boost() = default;

        std::string stat() const;
        const std::vector<std::string> &skills_practice() const;
        const std::vector<std::string> &skills_knowledge() const;
        float calc_bonus( int exp_total ) const;

        static void load_boost( const JsonObject &jo, const std::string &src );
        static void reset();

        static const std::vector<skill_boost> &get_all();
        static cata::optional<skill_boost> get( const std::string &stat_str );

    private:
        friend class generic_factory<skill_boost>;
        string_id<skill_boost> id;
        bool was_loaded = false;
        std::vector<std::string> _skills_practice;
        std::vector<std::string> _skills_knowledge;
        float _coefficient = 0.0f;
        float _offset = 0.0f;
        float _power = 0.0f;
        float _max_stat = 0.0f;

        void load( const JsonObject &jo, const std::string &src );
};

#endif // CATA_SRC_SKILL_BOOST_H
