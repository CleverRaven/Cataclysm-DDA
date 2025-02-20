#pragma once
#ifndef CATA_SRC_OVERMAP_CONNECTION_H
#define CATA_SRC_OVERMAP_CONNECTION_H

#include <list>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "omdata.h"
#include "type_id.h"

class JsonObject;
struct overmap_location;

class overmap_connection
{
    public:
        class subtype
        {
                friend overmap_connection;

            public:
                enum class flag : int { orthogonal };

                string_id<oter_type_t> terrain;

                int basic_cost = 0;

                bool allows_terrain( const int_id<oter_t> &oter ) const;
                bool allows_turns() const {
                    return terrain->is_linear();
                }

                bool is_orthogonal() const {
                    return flags.count( flag::orthogonal );
                }

                void load( const JsonObject &jo );
                void deserialize( const JsonObject &jo );

            private:
                std::set<string_id<overmap_location>> locations;
                std::set<flag> flags;
        };

        const subtype *pick_subtype_for( const int_id<oter_t> &ground ) const;
        bool has( const int_id<oter_t> &oter ) const;

        void load( const JsonObject &jo, std::string_view src );
        void check() const;
        void finalize();

        string_id<overmap_connection> id;
        std::vector<std::pair<string_id<overmap_connection>, mod_id>> src;
        bool was_loaded = false;

    private:
        struct cache {
            const subtype *value = nullptr;
            bool assigned = false;
            explicit operator bool() const {
                return assigned;
            }
        };

        std::list<subtype> subtypes;
        mutable std::vector<cache> cached_subtypes;
};

namespace overmap_connections
{

void load( const JsonObject &jo, const std::string &src );
void finalize();
void check_consistency();
void reset();

string_id<overmap_connection> guess_for( const int_id<oter_type_t> &oter_id );
string_id<overmap_connection> guess_for( const int_id<oter_t> &oter_id );

} // namespace overmap_connections

#endif // CATA_SRC_OVERMAP_CONNECTION_H
