#pragma once
#ifndef CATA_SRC_FAULT_H
#define CATA_SRC_FAULT_H

#include <iosfwd>
#include <map>
#include <new>
#include <optional>
#include <set>
#include <string>

#include "calendar.h"
#include "memory_fast.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
struct requirement_data;

class fault_fix
{
    public:
        fault_fix_id id_ = fault_fix_id::NULL_ID();
        translation name;
        translation success_msg; // message to print on applying successfully
        time_duration time;
        std::map<std::string, std::string> set_variables; // item vars applied to item
        std::map<skill_id, int> skills; // map of skill_id to required level
        std::set<fault_id> faults_removed; // which faults are removed on applying
        std::set<fault_id> faults_added; // which faults are added on applying
        int mod_damage = 0; // mod_damage with this value is called on item applied to
        int mod_degradation = 0; // mod_degradation with this value is called on item applied to
        std::map<proficiency_id, float>
        time_save_profs; // map of proficiency_id and the time saving for posessing it
        std::map<flag_id, float>
        time_save_flags; // map of flag_id and the time saving for the mendee item posessing it

        const requirement_data &get_requirements() const;

        static void load( const JsonObject &jo );
        static const std::map<fault_fix_id, fault_fix> &all();
        static void reset();
        static void finalize();
        static void check_consistency();
    private:
        friend class fault;
        shared_ptr_fast<requirement_data> requirements = make_shared_fast<requirement_data>();
};

class fault
{
    public:
        const fault_id &id() const;
        std::string name() const;
        std::string description() const;
        std::string item_prefix() const;
        bool has_flag( const std::string &flag ) const;

        const std::set<fault_fix_id> &get_fixes() const;

        static const std::map<fault_id, fault> &all();
        static void load( const JsonObject &jo );
        static void reset();
        static void check_consistency();

    private:
        friend class fault_fix;
        fault_id id_ = fault_id::NULL_ID();
        translation name_;
        translation description_;
        translation item_prefix_; // prefix added to affected item's name
        std::set<fault_fix_id> fixes;
        std::set<std::string> flags;
};

#endif // CATA_SRC_FAULT_H
