#pragma once
#ifndef SKILL_BOOST_H
#define SKILL_BOOST_H

#include <vector>
#include <string>

#include "optional.h"
#include "string_id.h"

template<typename T>
class generic_factory;
class JsonObject;

class skill_boost
{
    public:
        skill_boost() = default;

        std::string stat() const;
        const std::vector<std::string> &skills() const;
        float calc_bonus( int skill_total ) const;

        static void load_boost( JsonObject &jo, const std::string &src );
        static void reset();

        static const std::vector<skill_boost> &get_all();
        static cata::optional<skill_boost> get( const std::string &stat_str );

    private:
        friend class generic_factory<skill_boost>;
        string_id<skill_boost> id;
        bool was_loaded = false;
        std::vector<std::string> _skills;
        int _offset;
        float _power;

        void load( JsonObject &jo, const std::string &src );
};

#endif
