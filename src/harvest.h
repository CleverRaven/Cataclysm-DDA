#pragma once
#ifndef CATA_SRC_HARVEST_H
#define CATA_SRC_HARVEST_H

#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "translations.h"
#include "type_id.h"

using itype_id = std::string;
class JsonObject;

// Could be reused for butchery
struct harvest_entry {
    itype_id drop = "null";
    std::pair<float, float> base_num = { 1.0f, 1.0f };
    // This is multiplied by survival and added to the above
    // TODO: Make it a map: skill->scaling
    std::pair<float, float> scale_num = { 0.0f, 0.0f };

    int max = 1000;
    std::string type = "null";
    float mass_ratio = 0.00f;

    static harvest_entry load( const JsonObject &jo, const std::string &src );

    std::vector<std::string> flags;
    std::vector<fault_id> faults;
};

class harvest_list
{
    public:
        harvest_list();

        const harvest_id &id() const;

        std::string message() const;

        bool is_null() const;

        const std::list<harvest_entry> &entries() const {
            return entries_;
        }

        bool empty() const {
            return entries().empty();
        }

        bool has_entry_type( std::string type ) const;

        /**
         * Returns a set of cached, translated names of the items this harvest entry could produce.
         * Filled in at finalization and not valid before that stage.
         */
        const std::set<std::string> &names() const {
            return names_;
        }

        std::string describe( int at_skill = -1 ) const;

        std::list<harvest_entry>::const_iterator begin() const;
        std::list<harvest_entry>::const_iterator end() const;
        std::list<harvest_entry>::const_reverse_iterator rbegin() const;
        std::list<harvest_entry>::const_reverse_iterator rend() const;

        /** Load harvest data, create relevant global entries, then return the id of the new list */
        static const harvest_id &load( const JsonObject &jo, const std::string &src,
                                       const std::string &force_id = "" );

        /** Get all currently loaded harvest data */
        static const std::map<harvest_id, harvest_list> &all();

        /** Fills out the set of cached names. */
        static void finalize_all();

        /** Check consistency of all loaded harvest data */
        static void check_consistency();

        /** Clear all loaded harvest data (invalidating any pointers) */
        static void reset();
    private:
        harvest_id id_;
        std::list<harvest_entry> entries_;
        std::set<std::string> names_;
        translation message_;

        void finalize();
};

#endif // CATA_SRC_HARVEST_H
