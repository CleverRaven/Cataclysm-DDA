#pragma once
#ifndef CATA_SRC_HARVEST_H
#define CATA_SRC_HARVEST_H

#include <iosfwd>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "translations.h"
#include "type_id.h"

class JsonObject;
class butchery_requirements;

using butchery_requirements_id = string_id<butchery_requirements>;

// Could be reused for butchery
struct harvest_entry {
    // drop can be either an itype_id or a group id
    std::string drop = "null";
    std::pair<float, float> base_num = { 1.0f, 1.0f };
    // This is multiplied by survival and added to the above
    // TODO: Make it a map: skill->scaling
    std::pair<float, float> scale_num = { 0.0f, 0.0f };

    int max = 1000;
    std::string type = "null";
    float mass_ratio = 0.00f;

    static harvest_entry load( const JsonObject &jo, const std::string &src );

    std::vector<flag_id> flags;
    std::vector<fault_id> faults;

    bool was_loaded = false;
    void load( const JsonObject &jo );
    void deserialize( JsonIn &jsin );
};

class harvest_list
{
    public:
        harvest_list();

        itype_id leftovers = itype_id( "ruined_chunks" );

        harvest_id id;

        std::string message() const;

        bool is_null() const;

        const std::list<harvest_entry> &entries() const {
            return entries_;
        }

        bool empty() const {
            return entries().empty();
        }

        bool has_entry_type( const std::string &type ) const;

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

        std::list<harvest_entry>::const_iterator begin() const;
        std::list<harvest_entry>::const_iterator end() const;
        std::list<harvest_entry>::const_reverse_iterator rbegin() const;
        std::list<harvest_entry>::const_reverse_iterator rend() const;

        /** Fills out the set of cached names. */
        static void finalize_all();

        /** Check consistency of all loaded harvest data */
        static void check_consistency();
        /** Reset all loaded harvest data */
        static void reset();

        bool was_loaded = false;
        void load( const JsonObject &obj, const std::string & );
        static void load_harvest_list( const JsonObject &jo, const std::string &src );
        static const std::vector<harvest_list> &get_all();

    private:
        std::list<harvest_entry> entries_;
        std::set<std::string> names_;
        translation message_;
        butchery_requirements_id butchery_requirements_;

        void finalize();
};

#endif // CATA_SRC_HARVEST_H
