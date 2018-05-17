#pragma once
#ifndef CLZONES_H
#define CLZONES_H

#include "enums.h"
#include "string_id.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <utility>

class JsonIn;
class JsonOut;

class zone_type
{
    private:
        std::string name_;
    public:
        explicit zone_type( const std::string name ) : name_( name ) {}
        std::string name() const;
};
using zone_type_id = string_id<zone_type>;

/**
 * These are zones the player can designate.
 *
 * They currently don't serve much use, other than to designate
 * where to auto-pickup and where not to and restricting friendly npc pickup.
 */
class zone_manager
{
    private:
        std::map<zone_type_id, zone_type> types;
        std::unordered_map<zone_type_id, std::unordered_set<tripoint>> area_cache;

    public:
        zone_manager();
        ~zone_manager() = default;
        zone_manager( zone_manager && ) = default;
        zone_manager( const zone_manager & ) = default;
        zone_manager &operator=( zone_manager && ) = default;
        zone_manager &operator=( const zone_manager & ) = default;

        static zone_manager &get_manager() {
            static zone_manager manager;
            return manager;
        }

        class zone_data
        {
            private:
                std::string name;
                zone_type_id type;
                bool invert;
                bool enabled;
                tripoint start;
                tripoint end;

            public:
                zone_data( const std::string &_name, const zone_type_id &_type,
                           const bool _invert, const bool _enabled,
                           const tripoint &_start, const tripoint &_end ) {
                    name = _name;
                    type = _type;
                    invert = _invert;
                    enabled = _enabled;
                    start = _start;
                    end = _end;
                }

                void set_name();
                void set_type();
                void set_enabled( const bool enabled );

                std::string get_name() const {
                    return name;
                }
                const zone_type_id &get_type() const {
                    return type;
                }
                bool get_invert() const {
                    return invert;
                }
                bool get_enabled() const {
                    return enabled;
                }
                tripoint get_start_point() const {
                    return start;
                }
                tripoint get_end_point() const {
                    return end;
                }
                tripoint get_center_point() const;
        };

        std::vector<zone_data> zones;

        void add( const std::string &name, const zone_type_id &type,
                  const bool invert, const bool enabled,
                  const tripoint &start, const tripoint &end );

        bool remove( const size_t index ) {
            if( index < zones.size() ) {
                zones.erase( zones.begin() + index );
                return true;
            }

            return false;
        }

        unsigned int size() const {
            return zones.size();
        }
        const std::map<zone_type_id, zone_type> &get_types() const {
            return types;
        }
        std::string get_name_from_type( const zone_type_id &type ) const;
        bool has_type( const zone_type_id &type ) const;
        void cache_data();
        bool has( const zone_type_id &type, const tripoint &where ) const;

        bool save_zones();
        void load_zones();
        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
};

#endif
