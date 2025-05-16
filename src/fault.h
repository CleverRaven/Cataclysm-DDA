#pragma once
#ifndef CATA_SRC_FAULT_H
#define CATA_SRC_FAULT_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "calendar.h"
#include "math_parser_diag_value.h"
#include "memory_fast.h"
#include "requirements.h"
#include "translation.h"
#include "type_id.h"
#include "weighted_list.h"

class JsonObject;
class item;
template <typename T> class generic_factory;

namespace faults
{
void load_fault( const JsonObject &jo, const std::string &src );
void load_fix( const JsonObject &jo, const std::string &src );
void load_group( const JsonObject &jo, const std::string &src );

void reset();
void finalize();
void check_consistency();

std::vector<fault_id> all_of_type( const std::string &type );

const fault_id &random_of_type( const std::string &type );
const fault_id &random_of_type_item_has( const item &it, const std::string &type );
} // namespace faults

class fault_fix
{
    public:
        fault_fix_id id = fault_fix_id::NULL_ID();
        translation name;
        translation success_msg; // message to print on applying successfully
        time_duration time = 0_seconds;
        std::map<std::string, diag_value> set_variables; // item vars applied to item
        // item vars adjustment(s) applied to item via multiplication; // item vars adjustment(s) applied to item via multiplication
        std::map<std::string, double> adjust_variables_multiply;
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

        void finalize();
    private:
        void load( const JsonObject &jo, std::string_view src );
        void check() const;
        bool was_loaded = false; // used by generic_factory
        friend class generic_factory<fault_fix>;
        friend class fault;
        shared_ptr_fast<requirement_data> requirements = make_shared_fast<requirement_data>();
};

class fault
{
    public:
        fault_id id = fault_id::NULL_ID();
        std::string name() const;
        std::string type() const; // use a set of types?
        std::string description() const;
        std::string item_prefix() const;
        std::string item_suffix() const;
        std::string message() const;
        double price_mod() const;
        // having this faults adds this much of temporary (will be removed when fault is fixed) degradation
        int degradation_mod() const;
        // int is additive (default 0), float is multiplier (default 1)
        std::vector<std::tuple<int, float, damage_type_id>> melee_damage_mod() const;
        // int is additive (default 0), float is multiplier (default 1)
        std::vector<std::tuple<int, float, damage_type_id>> armor_mod() const;
        bool affected_by_degradation() const;
        bool has_flag( const std::string &flag ) const;
        const std::set<fault_id> &get_block_faults() const;

        const std::set<fault_fix_id> &get_fixes() const;
    private:
        void load( const JsonObject &jo, std::string_view );
        void check() const;
        bool was_loaded = false; // used by generic_factory
        friend class generic_factory<fault>;
        friend class fault_fix;

        std::string type_;
        translation name_;
        translation description_;
        translation item_prefix_; // prefix added to affected item's name
        translation item_suffix_;
        translation message_;
        std::set<fault_fix_id> fixes;
        std::set<std::string> flags;
        std::set<fault_id> block_faults;
        double price_modifier = 1.0;
        int degradation_mod_ = 0;
        std::vector<std::tuple<int, float, damage_type_id>> melee_damage_mod_;
        std::vector<std::tuple<int, float, damage_type_id>> armor_mod_;
        // todo add tool_quality_mod_; axe with no handle won't axe
        bool affected_by_degradation_ = false;

};

class fault_group
{
    public:
        fault_group_id id;
        weighted_int_list<fault_id> get_weighted_list() const;

    private:
        void load( const JsonObject &jo, std::string_view );
        bool was_loaded = false;
        friend class generic_factory<fault_group>;
        weighted_int_list<fault_id> fault_weighted_list;
};

#endif // CATA_SRC_FAULT_H
