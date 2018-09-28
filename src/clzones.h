#pragma once
#ifndef CLZONES_H
#define CLZONES_H

#include "enums.h"
#include "string_id.h"
#include "item.h"
#include "optional.h"

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
        explicit zone_type( const std::string &name ) : name_( name ) {}
        std::string name() const;
};
using zone_type_id = string_id<zone_type>;

class zone_options
{
    public:
        virtual ~zone_options() = default;

        /* create valid instance for zone type */
        static std::shared_ptr<zone_options> create( const zone_type_id &type );

        /* checks if options is correct base / derived class for zone type */
        static bool is_valid( const zone_type_id &type, const zone_options &options );

        /* derived classes must always return true */
        virtual bool has_options() const {
            return false;
        };

        /* query only necessary options at zone creation, one by one
         * returns true if successful, returns false if fails or canceled */
        virtual bool query_at_creation() {
            return true;
        };

        /* query options, first uimenu should allow to pick an option to edit (if more than one)
         * returns true if something is changed, otherwise returns false */
        virtual bool query() {
            return false;
        };

        /* suggest a name for the zone, depending on options */
        virtual std::string get_zone_name_suggestion() const {
            return "";
        };

        /* vector of pairs of each option's description and value */
        virtual std::vector<std::pair<std::string, std::string>> get_descriptions() const {
            return std::vector<std::pair<std::string, std::string>>();
        };

        virtual void serialize( JsonOut & ) const {};
        virtual void deserialize( JsonObject & ) {};
};

// mark option interface
class mark_option
{
    public:
        virtual ~mark_option() {}

        virtual std::string get_mark() const = 0;
};

class plot_options : public zone_options, public mark_option
{
    private:
        std::string mark;
        std::string seed;

        enum query_seed_result {
            canceled,
            successful,
            changed,
        };

        query_seed_result query_seed();

    public:
        std::string get_mark() const override {
            return mark;
        };
        std::string get_seed() const {
            return seed;
        };

        bool has_options() const override {
            return true;
        };

        bool query_at_creation() override;
        bool query() override;

        std::string get_zone_name_suggestion() const override;

        std::vector<std::pair<std::string, std::string>> get_descriptions() const override;

        void serialize( JsonOut &json ) const override;
        void deserialize( JsonObject &jo_zone ) override;
};

/**
 * These are zones the player can designate.
 */
class zone_manager
{
    public:
        class zone_data;
        using ref_zone_data = std::reference_wrapper<zone_manager::zone_data>;
        using ref_const_zone_data = std::reference_wrapper<const zone_manager::zone_data>;

    private:
        const int MAX_DISTANCE = 10;
        std::vector<zone_data> zones;
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

        zone_data &add( const std::string &name, const zone_type_id &type,
                        const bool invert, const bool enabled,
                        const tripoint &start, const tripoint &end,
                        std::shared_ptr<zone_options> options = nullptr );

        bool remove( const size_t index );
        bool remove( zone_data &zone );

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
        bool has_near( const zone_type_id &type, const tripoint &where ) const;
        std::unordered_set<tripoint> get_near( const zone_type_id &type, const tripoint &where ) const;
        zone_type_id get_near_zone_type_for_item( const item &it, const tripoint &where ) const;
        std::vector<zone_data> get_zones( const zone_type_id &type, const tripoint &where ) const;
        const zone_data *get_top_zone( const tripoint &where ) const;
        const zone_data *get_bottom_zone( const tripoint &where ) const;
        cata::optional<std::string> query_name( std::string default_name = "" ) const;
        cata::optional<zone_type_id> query_type() const;
        void swap( zone_data &a, zone_data &b );

        // 'direct' access to zone_manager::zones, giving direct access was nono
        std::vector<ref_zone_data> get_zones();
        std::vector<ref_const_zone_data> get_zones() const;

        bool save_zones();
        void load_zones();
        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
};

class zone_manager::zone_data
{
    private:
        std::string name;
        zone_type_id type;
        bool invert;
        bool enabled;
        tripoint start;
        tripoint end;
        std::shared_ptr<zone_options> options;

    public:
        zone_data( const std::string &_name, const zone_type_id &_type,
                   bool _invert, const bool _enabled,
                   const tripoint &_start, const tripoint &_end,
                   std::shared_ptr<zone_options> _options = nullptr ) {
            name = _name;
            type = _type;
            invert = _invert;
            enabled = _enabled;
            start = _start;
            end = _end;

            // ensure that suplied options is of correct class
            if( _options == nullptr || !zone_options::is_valid( type, *_options ) ) {
                options = zone_options::create( type );
            } else {
                options = _options;
            }
        }

        bool set_name(); // returns true if name is changed
        bool set_type(); // returns true if type is changed
        void set_position( const std::pair<tripoint, tripoint> position );
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
        bool has_options() const {
            return options->has_options();
        }
        const zone_options &get_options() const {
            return *options;
        }
        zone_options &get_options() {
            return *options;
        }
        bool has_inside( const tripoint &p ) const {
            return p.x >= start.x && p.x <= end.x &&
                   p.y >= start.y && p.y <= end.y &&
                   p.z >= start.z && p.z <= end.z;
        }
};


#endif
