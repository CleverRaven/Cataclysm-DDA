#pragma once
#ifndef CATA_SRC_HARVEST_H
#define CATA_SRC_HARVEST_H

#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "translation.h"
#include "type_id.h"

class JsonObject;
class butchery_requirements;
template <typename T> class generic_factory;

using butchery_requirements_id = string_id<butchery_requirements>;

class harvest_drop_type
{
    public:
        static void load_harvest_drop_types( const JsonObject &jo, const std::string &src );
        static void reset();
        void load( const JsonObject &jo, std::string_view src );
        static const std::vector<harvest_drop_type> &get_all();

        const harvest_drop_type_id &getId() {
            return id;
        }
        // Get skills required for harvesting rolls
        const std::vector<skill_id> &get_harvest_skills() const {
            return harvest_skills;
        }
        // Is the associated harvest drop an item group?
        bool is_item_group() const {
            return is_group_;
        }
        // Is the associated harvest drop a single itype?
        bool is_itype() const {
            return !is_group_;
        }
        // Should the associated drop only spawn on dissection?
        bool dissect_only() const {
            return dissect_only_;
        }
        // Message to display for the associated drop when field dressing
        translation field_dress_msg( bool succeeded ) const;
        // Message to display for the associated drop when doing quick/full butchery
        translation butcher_msg( bool succeeded ) const;
        // Message to display when failed to dissect the associated drop
        translation dissect_msg( bool succeeded ) const;

    private:
        harvest_drop_type_id id;
        std::vector<std::pair<harvest_drop_type_id, mod_id>> src;
        bool is_group_;
        bool dissect_only_;
        bool was_loaded = false;
        std::vector<skill_id> harvest_skills;
        std::string msg_fielddress_success;
        std::string msg_fielddress_fail;
        std::string msg_butcher_success;
        std::string msg_butcher_fail;
        std::string msg_dissect_success;
        std::string msg_dissect_fail;
        friend class generic_factory<harvest_drop_type>;
        friend struct mod_tracker;
};

// Could be reused for butchery
struct harvest_entry {
    // drop can be either an itype_id or a group id
    std::string drop = "null";
    std::pair<float, float> base_num = { 1.0f, 1.0f };
    // This is multiplied by survival and added to the above
    // TODO: Make it a map: skill->scaling
    std::pair<float, float> scale_num = { 0.0f, 0.0f };

    int max = 1000;
    harvest_drop_type_id type;
    float mass_ratio = 0.00f;

    static harvest_entry load( const JsonObject &jo, const std::string &src );

    std::vector<flag_id> flags;
    std::vector<fault_id> faults;

    bool was_loaded = false;
    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );

    // only compares mandatory members for reader identity checks
    bool operator==( const harvest_entry &rhs ) const;
};

class harvest_list
{
    public:
        harvest_list();

        itype_id leftovers = itype_id( "ruined_chunks" );

        harvest_id id;
        std::vector<std::pair<harvest_id, mod_id>> src;

        std::string message() const;

        bool is_null() const;

        const std::vector<harvest_entry> &entries() const {
            return entries_;
        }

        bool empty() const {
            return entries().empty();
        }

        bool has_entry_type( const harvest_drop_type_id &type ) const;

        /**
         * Returns a set of cached, translated names of the items this harvest entry could produce.
         * Filled in at finalization and not valid before that stage.
         */
        const std::set<std::string> &names() const {
            return names_;
        }

        const butchery_requirements &get_butchery_requirements() const {
            return butchery_requirements_.obj();
        }

        std::string describe( int at_skill = -1 ) const;

        std::vector<harvest_entry>::const_iterator begin() const;
        std::vector<harvest_entry>::const_iterator end() const;
        std::vector<harvest_entry>::const_reverse_iterator rbegin() const;
        std::vector<harvest_entry>::const_reverse_iterator rend() const;

        /** Fills out the set of cached names. */
        static void finalize_all();

        /** Check consistency of all loaded harvest data */
        static void check_consistency();
        /** Reset all loaded harvest data */
        static void reset();

        bool was_loaded = false;
        void load( const JsonObject &obj, std::string_view );
        static void load_harvest_list( const JsonObject &jo, const std::string &src );
        static const std::vector<harvest_list> &get_all();

    private:
        std::vector<harvest_entry> entries_;
        std::set<std::string> names_;
        translation message_;
        butchery_requirements_id butchery_requirements_;

        void finalize();
};

#endif // CATA_SRC_HARVEST_H
