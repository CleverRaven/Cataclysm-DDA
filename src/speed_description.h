#pragma once
#ifndef CATA_SRC_SPEED_DESCRIPTION_H
#define CATA_SRC_SPEED_DESCRIPTION_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "translation.h"
#include "type_id.h"

class JsonObject;
class speed_description_value;
template<typename T>
class generic_factory;

class speed_description
{
    public:
        static void load_speed_descriptions( const JsonObject &jo, const std::string &src );
        static void reset();

        void load( const JsonObject &jo, std::string_view src );

        static const std::vector<speed_description> &get_all();

        const std::vector<speed_description_value> &values() const {
            return values_;
        }

    private:
        friend class generic_factory<speed_description>;
        friend struct mod_tracker;

        speed_description_id id;
        std::vector<std::pair<speed_description_id, mod_id>> src;
        bool was_loaded = false;

        // Always sorted with highest value first
        std::vector<speed_description_value> values_;
};

class speed_description_value
{
    public:

        bool was_loaded = false;
        void load( const JsonObject &jo );
        void deserialize( const JsonObject &data );

        double value() const {
            return value_;
        }

        const std::vector<translation> &descriptions() const {
            return descriptions_;
        }

    private:
        double value_ = 0.00;
        std::vector<translation> descriptions_;
};

#endif // CATA_SRC_SPEED_DESCRIPTION_H
