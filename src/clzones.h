#ifndef CLZONES_H
#define CLZONES_H

#include "json.h"
#include "enums.h"
#include "translations.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

/**
 * These are zones the player can designate.
 *
 * They currently don't serve much use, other than to designate
 * where to auto-pickup and where not to and restricting friendly npc pickup.
 */
class zone_manager : public JsonSerializer, public JsonDeserializer
{
private:
    std::map<std::string, std::string> types;
    std::unordered_map<std::string, std::unordered_set<tripoint> > area_cache;

public:
    zone_manager();
    ~zone_manager() {};
    zone_manager( zone_manager && ) = default;
    zone_manager( const zone_manager & ) = default;
    zone_manager &operator=( zone_manager && ) = default;
    zone_manager &operator=( const zone_manager & ) = default;

    static zone_manager &get_manager()
    {
        static zone_manager manager;
        return manager;
    }

    class zone_data
    {
    private:
        std::string name;
        std::string type;
        bool invert;
        bool enabled;
        tripoint start;
        tripoint end;

    public:
        zone_data( const std::string &_name, const std::string &_type,
                   const bool _invert, const bool _enabled,
                   const tripoint &_start, const tripoint &_end )
        {
            name = _name;
            type = _type;
            invert = _invert;
            enabled = _enabled;
            start = _start;
            end = _end;
        }

        ~zone_data() {};

        void set_name();
        void set_type();
        void set_enabled( const bool enabled );

        std::string get_name() const
        {
            return name;
        }
        std::string get_type() const
        {
            return type;
        }
        bool get_invert() const
        {
            return invert;
        }
        bool get_enabled() const
        {
            return enabled;
        }
        tripoint get_start_point() const
        {
            return start;
        }
        tripoint get_end_point() const
        {
            return end;
        }
        tripoint get_center_point() const;
    };

    std::vector<zone_data> zones;

    void add( const std::string &name, const std::string &type,
              const bool invert, const bool enabled,
              const tripoint &start, const tripoint &end );

    bool remove( const size_t index )
    {
        if( index < zones.size() ) {
            zones.erase( zones.begin() + index );
            return true;
        }

        return false;
    }

    unsigned int size() const
    {
        return zones.size();
    }
    std::map<std::string, std::string> get_types() const
    {
        return types;
    }
    std::string get_name_from_type( const std::string &type ) const;
    bool has_type( const std::string &type ) const;
    void cache_data();
    bool has( const std::string &type, const tripoint &where ) const;

    bool save_zones();
    void load_zones();
    using JsonSerializer::serialize;
    void serialize( JsonOut &json ) const override;
    void deserialize( JsonIn &jsin ) override;
};

#endif
