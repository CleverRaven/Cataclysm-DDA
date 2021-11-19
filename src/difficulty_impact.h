#pragma once
#ifndef CATA_SRC_DIFFICULTY_IMPACT_H
#define CATA_SRC_DIFFICULTY_IMPACT_H

#include <string>
#include <map>

#include "json.h"
#include "translations.h"
#include "type_id.h"

/**
 * A value that represents difficulty rating (i.e.: easy, hard, etc.)
 * Values are intended to be set from very easy (1) to very hard (5)
*/
struct difficulty_opt {
    public:
        void load( const JsonObject &jo, const std::string &src );
        static const std::vector<difficulty_opt> &get_all();
        static void load_difficulty_opts( const JsonObject &jo, const std::string &src );
        static void reset();
        static void finalize();
        static int value( const difficulty_opt_id &id );
        static difficulty_opt_id getId( int value );
        static int avg_value();

        const difficulty_opt_id &getId() const {
            return id;
        }
        const translation &name() const {
            return name_;
        }
        int value() const {
            return value_;
        }
        const std::string &color() const {
            return color_;
        }
    private:
        bool was_loaded = false;
        difficulty_opt_id id;
        translation name_;
        std::string color_;
        int value_ = 0;
        static int avg_value_;
        friend class generic_factory<difficulty_opt>;
};

/**
 * An aspect of gameplay impacted by difficulty (i.e.: combat, crafting, etc.)
*/
struct difficulty_impact {
    public:
        enum difficulty_source {
            NONE,
            SCENARIO,
            PROFFESION,
            HOBBY,
            MUTATION
        };

        void load( const JsonObject &jo, const std::string &src );
        static const std::vector<difficulty_impact> &get_all();
        static void load_difficulty_impacts( const JsonObject &jo, const std::string &src );
        static void reset();

        const difficulty_impact_id &getId() const {
            return id;
        }
        const translation &name() const {
            return name_;
        }
        float weight( difficulty_source src ) const;
    private:
        bool was_loaded = false;
        difficulty_impact_id id;
        translation name_;
        std::map<difficulty_source, float> weight_;
        friend class generic_factory<difficulty_impact>;
};

#endif // CATA_SRC_DIFFICULTY_IMPACT_H
