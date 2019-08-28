#pragma once
#ifndef CLZONES_H
#define CLZONES_H

#include <stddef.h>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <functional>
#include <memory>
#include <string>

#include "optional.h"
#include "point.h"
#include "string_id.h"
#include "type_id.h"

class JsonIn;
class JsonOut;
class JsonObject;
class item;
class faction;
class map;

using faction_id = string_id<faction>;
const faction_id your_fac( "your_followers" );
const std::string type_fac_hash_str = "__FAC__";

class zone_type
{
    private:
        std::string name_;
        std::string desc_;
    public:
        explicit zone_type( const std::string &name, const std::string &desc ) : name_( name ),
            desc_( desc ) {}
        std::string name() const;
        std::string desc() const;
};

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
        }

        /* query only necessary options at zone creation, one by one
         * returns true if successful, returns false if fails or canceled */
        virtual bool query_at_creation() {
            return true;
        }

        /* query options, first uilist should allow to pick an option to edit (if more than one)
         * returns true if something is changed, otherwise returns false */
        virtual bool query() {
            return false;
        }

        /* suggest a name for the zone, depending on options */
        virtual std::string get_zone_name_suggestion() const {
            return "";
        }

        /* vector of pairs of each option's description and value */
        virtual std::vector<std::pair<std::string, std::string>> get_descriptions() const {
            return std::vector<std::pair<std::string, std::string>>();
        }

        virtual void serialize( JsonOut & ) const {}
        virtual void deserialize( JsonObject & ) {}
};

// mark option interface
class mark_option
{
    public:
        virtual ~mark_option() = default;

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
        }
        std::string get_seed() const {
            return seed;
        }

        bool has_options() const override {
            return true;
        }

        bool query_at_creation() override;
        bool query() override;

        std::string get_zone_name_suggestion() const override;

        std::vector<std::pair<std::string, std::string>> get_descriptions() const override;

        void serialize( JsonOut &json ) const override;
        void deserialize( JsonObject &jo_zone ) override;
};

class blueprint_options : public zone_options, public mark_option
{
    private:
        std::string mark; // furn/ter id as string.
        std::string con;
        int index;

        enum query_con_result {
            canceled,
            successful,
            changed,
        };

        query_con_result query_con();

    public:
        std::string get_mark() const override {
            return mark;
        }
        std::string get_con() const {
            return con;
        }
        int get_index() const {
            return index;
        }

        bool has_options() const override {
            return true;
        }

        bool query_at_creation() override;
        bool query() override;

        std::string get_zone_name_suggestion() const override;

        std::vector<std::pair<std::string, std::string>> get_descriptions() const override;

        void serialize( JsonOut &json ) const override;
        void deserialize( JsonObject &jo_zone ) override;
};

class loot_options : public zone_options, public mark_option
{
    private:
        std::string mark; // basic item filter.

        enum query_loot_result {
            canceled,
            successful,
            changed,
        };

        query_loot_result query_loot();

    public:
        std::string get_mark() const override {
            return mark;
        }

        bool has_options() const override {
            return true;
        }

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
class zone_data
{
    private:
        std::string name;
        zone_type_id type;
        faction_id faction;
        bool invert;
        bool enabled;
        bool is_vehicle;
        tripoint start;
        tripoint end;
        std::shared_ptr<zone_options> options;

    public:
        zone_data() {
            type = zone_type_id( "" );
            invert = false;
            enabled = false;
            is_vehicle = false;
            start = tripoint_zero;
            end = tripoint_zero;
            options = nullptr;
        }

        zone_data( const std::string &_name, const zone_type_id &_type, const faction_id &_faction,
                   bool _invert, const bool _enabled,
                   const tripoint &_start, const tripoint &_end,
                   std::shared_ptr<zone_options> _options = nullptr ) {
            name = _name;
            type = _type;
            faction = _faction;
            invert = _invert;
            enabled = _enabled;
            is_vehicle = false;
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
        void set_position( const std::pair<tripoint, tripoint> &position, bool manual = true );
        void set_enabled( bool enabled_arg );
        void set_is_vehicle( bool is_vehicle_arg );

        static std::string make_type_hash( const zone_type_id &_type, const faction_id &_fac ) {
            return _type.c_str() + type_fac_hash_str + _fac.c_str();
        }
        static zone_type_id unhash_type( const std::string &hash_type ) {
            size_t end = hash_type.find( type_fac_hash_str );
            if( end != std::string::npos && end < hash_type.size() ) {
                return zone_type_id( hash_type.substr( 0, end ) );
            }
            return zone_type_id( "" );
        }
        std::string get_name() const {
            return name;
        }
        const faction_id &get_faction() const {
            return faction;
        }
        std::string get_type_hash() const {
            return make_type_hash( type, faction );
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
        bool get_is_vehicle() const {
            return is_vehicle;
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
        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
};

class zone_manager
{
    public:
        using ref_zone_data = std::reference_wrapper<zone_data>;
        using ref_const_zone_data = std::reference_wrapper<const zone_data>;

    private:
        static const int MAX_DISTANCE = 10;
        std::vector<zone_data> zones;
        //Containers for Revert functionality for Vehicle Zones
        //Pointer to added zone to be removed
        std::vector<zone_data *> added_vzones;
        //Copy of original data, pointer to the zone
        std::vector<std::pair<zone_data, zone_data *>> changed_vzones;
        //copy of original data to be re-added
        std::vector<zone_data> removed_vzones;

        std::map<zone_type_id, zone_type> types;
        std::unordered_map<std::string, std::unordered_set<tripoint>> area_cache;
        std::unordered_map<std::string, std::unordered_set<tripoint>> vzone_cache;
        std::unordered_set<tripoint> get_point_set( const zone_type_id &type,
                const faction_id &fac = your_fac ) const;
        std::unordered_set<tripoint> get_vzone_set( const zone_type_id &type,
                const faction_id &fac = your_fac ) const;

        //Cache number of items already checked on each source tile when sorting
        std::unordered_map<tripoint, int> num_processed;

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

        void add( const std::string &name, const zone_type_id &type, const faction_id &faction,
                  bool invert, bool enabled,
                  const tripoint &start, const tripoint &end,
                  std::shared_ptr<zone_options> options = nullptr );
        const zone_data *get_zone_at( const tripoint &where, const zone_type_id &type ) const;
        void create_vehicle_loot_zone( class vehicle &vehicle, const point &mount_point,
                                       zone_data &new_zone );

        bool remove( zone_data &zone );

        unsigned int size() const {
            return zones.size();
        }
        const std::map<zone_type_id, zone_type> &get_types() const {
            return types;
        }
        std::string get_name_from_type( const zone_type_id &type ) const;
        bool has_type( const zone_type_id &type ) const;
        bool has_defined( const zone_type_id &type, const faction_id &fac = your_fac ) const;
        void cache_data();
        void cache_vzones();
        bool has( const zone_type_id &type, const tripoint &where,
                  const faction_id &fac = your_fac ) const;
        bool has_near( const zone_type_id &type, const tripoint &where, int range = MAX_DISTANCE,
                       const faction_id &fac = your_fac ) const;
        bool has_loot_dest_near( const tripoint &where ) const;
        bool custom_loot_has( const tripoint &where, const item *it ) const;
        std::unordered_set<tripoint> get_near( const zone_type_id &type, const tripoint &where,
                                               int range = MAX_DISTANCE, const item *it = nullptr, const faction_id &fac = your_fac ) const;
        cata::optional<tripoint> get_nearest( const zone_type_id &type, const tripoint &where,
                                              int range = MAX_DISTANCE, const faction_id &fac = your_fac ) const;
        zone_type_id get_near_zone_type_for_item( const item &it, const tripoint &where,
                int range = MAX_DISTANCE ) const;
        std::vector<zone_data> get_zones( const zone_type_id &type, const tripoint &where,
                                          const faction_id &fac = your_fac ) const;
        const zone_data *get_zone_at( const tripoint &where ) const;
        const zone_data *get_bottom_zone( const tripoint &where,
                                          const faction_id &fac = your_fac ) const;
        cata::optional<std::string> query_name( const std::string &default_name = "" ) const;
        cata::optional<zone_type_id> query_type() const;
        void swap( zone_data &a, zone_data &b );
        void rotate_zones( map &target_map, int turns );
        // list of tripoints of zones that are loot zones only
        std::unordered_set<tripoint> get_point_set_loot( const tripoint &where, int radius,
                const faction_id &fac = your_fac ) const;
        std::unordered_set<tripoint> get_point_set_loot( const tripoint &where, int radius,
                bool npc_search, const faction_id &fac = your_fac ) const;
        void start_sort( const std::vector<tripoint> &src_sorted );
        void end_sort();
        bool is_sorting() const;
        int get_num_processed( const tripoint &src ) const;
        void increment_num_processed( const tripoint &src );
        void decrement_num_processed( const tripoint &src );

        // 'direct' access to zone_manager::zones, giving direct access was nono
        std::vector<ref_zone_data> get_zones( const faction_id &fac = your_fac );
        std::vector<ref_const_zone_data> get_zones( const faction_id &fac = your_fac ) const;

        bool save_zones();
        void load_zones();
        void zone_edited( zone_data &zone );
        void revert_vzones();
        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
};

#endif
