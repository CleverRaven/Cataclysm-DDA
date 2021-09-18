#pragma once
#ifndef CATA_SRC_SPEED_DESCRIPTION_H
#define CATA_SRC_SPEED_DESCRIPTION_H

#include <string>
#include <vector>

#include "type_id.h"

class JsonIn;
class JsonObject;
class JsonOut;
template<typename T>
class generic_factory;
class speed_description_value;


class speed_description
{
    public:
        static void load_speed_descriptions( const JsonObject &jo, const std::string &src );
        static void reset();

        void load( const JsonObject &jo, const std::string &src );

        static const std::vector<speed_description> &get_all();

        const std::vector<speed_description_value> &values() const {
            return values_;
        }

    private:
        friend class generic_factory<speed_description>;

        speed_description_id id;
        bool was_loaded = false;

        // Always sorted with highest value first
        std::vector<speed_description_value> values_;
};

class speed_description_value
{
    public:

        bool was_loaded = false;
        void load( const JsonObject &jo );
        void deserialize( JsonIn &jsin );

        double value() const {
            return value_;
        }

        const std::vector<std::string> &descriptions() const {
            return descriptions_;
        }

    private:
        double value_ = 0.00;
        std::vector<std::string> descriptions_;
};

#endif // CATA_SRC_SPEED_DESCRIPTION_H
