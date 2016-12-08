#ifndef HARVEST_H
#define HARVEST_H

#include <list>
#include <set>
#include "string_id.h"

typedef std::string itype_id;
class JsonObject;
class harvest_list;

class inventory;

using harvest_id = string_id<harvest_list>;

struct requirement_data;
using requirement_id = string_id<requirement_data>;

struct harvest_entry {
    itype_id drop = "null";
    std::pair<float, float> base_num = { 1.0f, 1.0f };
    // This is multiplied by survival and added to the above
    // @todo Make it a map: skill->scaling
    std::pair<float, float> scale_num = { 0.0f, 0.0f };

    int max = 1000;

    static harvest_entry load( JsonObject &jo, const std::string &src );
};

class harvest_list {
    public:
        harvest_list() : id_( NULL_ID ) {}

        const harvest_id &id() const {
            return id_;
        }

        bool is_null() const {
            return id_ == NULL_ID;
        }

        const std::list<harvest_entry> &entries() const {
            return entries_;
        }

        bool empty() const {
            return entries().empty();
        }

        /**
         * Returns a set of cached, translated names of the items this harvest entry could produce.
         * Filled in at finalization and not valid before that stage.
         */
        const std::set<std::string> &names() const {
            return names_;
        }

        /** What tools or other consumables are required when using this harvest definition? */
        requirement_data requirements() const;

        /** Check requirements met optionally displaying failures if @param alert is set */
        bool can_harvest( const inventory &inv, bool alert = true ) const;

        std::list<harvest_entry>::const_iterator begin() const;
        std::list<harvest_entry>::const_iterator end() const;
        std::list<harvest_entry>::const_reverse_iterator rbegin() const;
        std::list<harvest_entry>::const_reverse_iterator rend() const;

        /** Load harvest data, create relevant global entries, then return the id of the new list */
        static const harvest_id &load( JsonObject &jo, const std::string &src,
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
        std::vector<std::pair<requirement_id, int>> reqs;

        void finalize();
};

#endif