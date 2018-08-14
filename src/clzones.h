#pragma once
#ifndef CLZONES_H
#define CLZONES_H

#include "enums.h"
#include "string_id.h"
#include "item.h"

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
        bool has_subtype_;
    public:
        explicit zone_type( const std::string name, bool has_subtype = false ) : name_( name ),
            has_subtype_( has_subtype ) {}
        std::string name() const;
        bool has_subtype() const;
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
        const int MAX_DISTANCE = 10;
        std::map<zone_type_id, zone_type> types;
        std::unordered_map<zone_type_id, std::unordered_set<tripoint>> area_cache;
        std::unordered_set<tripoint> get_point_set( const zone_type_id &type ) const;

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
                std::string subtype;
                bool invert;
                bool enabled;
                tripoint start;
                tripoint end;

            public:
                zone_data( const std::string &_name, const zone_type_id &_type,
                           bool _invert, const bool _enabled,
                           const tripoint &_start, const tripoint &_end,
                           const std::string &_subtype = "" ) {
                    name = _name;
                    type = _type;
                    subtype = _subtype;
                    invert = _invert;
                    enabled = _enabled;
                    start = _start;
                    end = _end;
                }

                void set_name();
                void set_type();
                void set_subtype();
                void set_position( const std::pair<tripoint, tripoint> position );
                void set_enabled( const bool enabled );

                std::string get_name() const {
                    return name;
                }
                const zone_type_id &get_type() const {
                    return type;
                }
                std::string get_subtype() const {
                    return subtype;
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
                  const tripoint &start, const tripoint &end,
                  const std::string &subtype = "" );

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
        std::string get_name_from_subtype( const zone_type_id &type, const std::string &subtype ) const;
        bool has_subtype( const zone_type_id &type ) const;
        bool has_type( const zone_type_id &type ) const;
        void cache_data();
        bool has( const zone_type_id &type, const tripoint &where ) const;
        bool has_near( const zone_type_id &type, const tripoint &where ) const;
        std::unordered_set<tripoint> get_near( const zone_type_id &type, const tripoint &where ) const;
        zone_type_id get_near_zone_type_for_item( const item &it, const tripoint &where ) const;
        std::vector<zone_data> get_zones( const zone_type_id &type, const tripoint &where ) const;
        std::vector<std::string> get_subtypes( const zone_type_id &type, const tripoint &where ) const;
        std::string query_name( std::string default_name = "" ) const;
        zone_type_id query_type() const;
        std::string query_subtype( const zone_type_id &type ) const;

        bool save_zones();
        void load_zones();
        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
};

#endif
