#pragma once
#ifndef CATA_SRC_ADDICTION_H
#define CATA_SRC_ADDICTION_H

#include <string>
#include <string_view>
#include <vector>

#include "calendar.h"
#include "translation.h"
#include "type_id.h"

class Character;
class JsonObject;
class JsonOut;

struct add_type {
    private:
        translation _type_name;
        translation _name;
        translation _desc;
        morale_type _craving_morale;
        effect_on_condition_id _effect;
        std::string _builtin;
    public:
        addiction_id id;
        bool was_loaded = false;
        static void load_add_types( const JsonObject &jo, const std::string &src );
        static void reset();
        static void check_add_types();
        void load( const JsonObject &jo, std::string_view src );
        static const std::vector<add_type> &get_all();

        const translation &get_name() const {
            return _name;
        }
        /**
         * Returns the name of an addiction. It should be able to finish the sentence
         * "Became addicted to ______".
         */
        const translation &get_type_name() const {
            return _type_name;
        }
        const translation &get_description() const {
            return _desc;
        }
        const morale_type &get_craving_morale() const {
            return _craving_morale;
        }
        const effect_on_condition_id &get_effect() const {
            return _effect;
        }
        const std::string &get_builtin() const {
            return _builtin;
        }
};

class addiction
{
    public:
        addiction_id type;
        int intensity = 0;
        time_duration sated = 1_hours;

        addiction() = default;
        explicit addiction( const addiction_id &t, const int i = 1 ) : type {t}, intensity {i} { }

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );

        /** Runs one update of this addiction's effects **/
        bool run_effect( Character &u );
};

std::string add_type_legacy_conv( std::string const &v );

// Minimum intensity before effects are seen
constexpr int MIN_ADDICTION_LEVEL = 3;
constexpr int MAX_ADDICTION_LEVEL = 20;

#endif // CATA_SRC_ADDICTION_H
