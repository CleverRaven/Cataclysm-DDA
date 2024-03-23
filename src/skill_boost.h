#pragma once
#ifndef CATA_SRC_SKILL_BOOST_H
#define CATA_SRC_SKILL_BOOST_H

#include <iosfwd>
#include <optional>
#include <vector>

#include "string_id.h"
#include "type_id.h"

class JsonObject;
template<typename T>
class generic_factory;

class skill_boost
{
    public:
        skill_boost() = default;

        std::string stat() const;
        const std::vector<std::string> &skills() const;
        float calc_bonus( int skill_total ) const;

        static void load_boost( const JsonObject &jo, const std::string &src );
        static void reset();

        static const std::vector<skill_boost> &get_all();
        static std::optional<skill_boost> get( const std::string &stat_str );

    private:
        friend class generic_factory<skill_boost>;
        friend struct mod_tracker;
        string_id<skill_boost> id;
        std::vector<std::pair<string_id<skill_boost>, mod_id>> src;
        bool was_loaded = false;
        std::vector<std::string> _skills;
        int _offset = 0;
        float _power = 0.0f;

        void load( const JsonObject &jo, std::string_view src );
};

#endif // CATA_SRC_SKILL_BOOST_H
