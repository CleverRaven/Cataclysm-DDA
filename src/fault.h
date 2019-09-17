#pragma once
#ifndef FAULT_H
#define FAULT_H

#include <map>
#include <string>

#include "string_id.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
struct requirement_data;

class fault
{
    public:
        fault() : id_( fault_id( "null" ) ) {}

        const fault_id &id() const {
            return id_;
        }

        bool is_null() const {
            return id_ == fault_id( "null" );
        }

        std::string name() const {
            return name_.translated();
        }

        std::string description() const {
            return description_.translated();
        }

        int time() const {
            return time_;
        }

        const std::map<skill_id, int> &skills() const {
            return skills_;
        }

        const requirement_data &requirements() const {
            return requirements_.obj();
        }

        /** Load fault from JSON definition */
        static void load_fault( JsonObject &jo );

        /** Get all currently loaded faults */
        static const std::map<fault_id, fault> &all();

        /** Clear all loaded faults (invalidating any pointers) */
        static void reset();

        /** Checks all loaded from JSON are valid */
        static void check_consistency();

    private:
        fault_id id_;
        translation name_;
        translation description_;
        int time_;
        std::map<skill_id, int> skills_;
        requirement_id requirements_;
};

#endif
