#pragma once
#ifndef CATA_SRC_CLZONES_H
#define CATA_SRC_CLZONES_H

#include <functional>
#include <cstddef>
#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "memory_fast.h"
#include "point.h"
#include "translations.h"
#include "type_id.h"
#include "avatar.h"
#include "map.h"

class JsonObject;
class JsonOut;
class JsonValue;
class faction;
class item;
class map;
struct construction;

using faction_id = string_id<faction>;
inline const faction_id your_fac( "your_followers" );
const std::string type_fac_hash_str = "__FAC__";

//Generic activity: maximum search distance for zones, constructions, etc.
constexpr int ACTIVITY_SEARCH_DISTANCE = 60;

class zone_type
{
    private:
        translation name_;
        translation desc_;
    public:

        zone_type_id id;
        std::vector<std::pair<zone_type_id, mod_id>> src;
        bool was_loaded = false;

        zone_type() = default;
        explicit zone_type( const translation &name, const translation &desc ) : name_( name ),
            desc_( desc ) {}

        std::string name() const;
        std::string desc() const;

        bool can_be_personal = false;
        bool hidden = false;

        static void load_zones( const JsonObject &jo, const std::string &src );
        static void reset();
        void load( const JsonObject &jo, std::string_view );
        /**
         * All spells in the game.
         */
        static const std::vector<zone_type> &get_all();
        bool is_valid() const;
};

class zone_options
{
    public:
        virtual ~zone_options() = default;

        /* create valid instance for zone type */
        static shared_ptr_fast<zone_options> create( const zone_type_id &type );

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
        virtual void deserialize( const JsonObject & ) {}
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
        itype_id mark;
        itype_id seed;

        enum query_seed_result {
            canceled,
            successful,
            changed,
        };

        query_seed_result query_seed();

    public:
        std::string get_mark() const override {
            return mark.str();
        }
        itype_id get_seed() const {
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
        void deserialize( const JsonObject &jo_zone ) override;
};

class blueprint_options : public zone_options, public mark_option
{
    private:
        // furn/ter id as string.
        std::string mark;
        construction_group_str_id group = construction_group_str_id::NULL_ID();
        construction_id index;

        enum query_con_result {
            canceled,
            successful,
            changed,
        };

        query_con_result query_con();

    public:
        blueprint_options() = default;
        blueprint_options( std::string mark, construction_group_str_id const &group,
                           construction_id const &index )
            : mark( std::move( mark ) ), group( group ), index( index ) {
        }

        std::string get_mark() const override {
            return mark;
        }
        construction_id get_index() const {
            return index;
        }

        bool has_options() const override {
            return true;
        }

        construction_id get_final_construction(
            const std::vector<construction> &list_constructions,
            const construction_id &idx,
            std::set<construction_id> &skip_index );

        bool query_at_creation() override;
        bool query() override;

        std::string get_zone_name_suggestion() const override;

        std::vector<std::pair<std::string, std::string>> get_descriptions() const override;

        void serialize( JsonOut &json ) const override;
        void deserialize( const JsonObject &jo_zone ) override;
};

class loot_options : public zone_options, public mark_option
{
    private:
        // basic item filter.
        std::string mark;

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

        void set_mark( std::string const &nmark ) {
            mark = nmark;
        }

        bool has_options() const override {
            return true;
        }

        bool query_at_creation() override;
        bool query() override;

        std::string get_zone_name_suggestion() const override;

        std::vector<std::pair<std::string, std::string>> get_descriptions() const override;

        void serialize( JsonOut &json ) const override;
        void deserialize( const JsonObject &jo_zone ) override;
};

class unload_options : public zone_options, public mark_option
{
    private:
        // what to unload
        std::string mark;
        bool mods;
        bool molle;
        bool always_unload;

        enum query_unload_result {
            canceled,
            successful,
            changed,
        };

        query_unload_result query_unload();

    public:
        std::string get_mark() const override {
            return mark;
        }

        bool unload_mods() const {
            return mods;
        }

        bool unload_molle() const {
            return molle;
        }

        bool unload_always() const {
            return always_unload;
        }

        void set_mark( std::string const &nmark ) {
            mark = nmark;
        }

        bool has_options() const override {
            return true;
        }

        bool query_at_creation() override;
        bool query() override;

        std::string get_zone_name_suggestion() const override;

        std::vector<std::pair<std::string, std::string>> get_descriptions() const override;

        void serialize( JsonOut &json ) const override;
        void deserialize( const JsonObject &jo_zone ) override;
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
        // if the zone has been turned off for an action
        bool temporarily_disabled; // NOLINT(cata-serialize)
        bool is_vehicle;
        tripoint start;
        tripoint end;
        //centered on the player
        bool is_personal;
        // for personal zones a cached value for the global shift to where the player was at activity start
        tripoint_abs_ms cached_shift;
        shared_ptr_fast<zone_options> options;

    public:
        zone_data() {
            type = zone_type_id( "" );
            invert = false;
            enabled = false;
            temporarily_disabled = false;
            is_vehicle = false;
            is_personal = false;
            start = tripoint_zero;
            end = tripoint_zero;
            cached_shift = {};
            options = nullptr;
        }

        zone_data( const std::string &_name, const zone_type_id &_type, const faction_id &_faction,
                   bool _invert, const bool _enabled,
                   const tripoint &_start, const tripoint &_end,
                   const shared_ptr_fast<zone_options> &_options = nullptr, bool personal = false ) {
            name = _name;
            type = _type;
            faction = _faction;
            invert = _invert;
            enabled = _enabled;
            is_vehicle = false;
            is_personal = personal;
            start = _start;
            end = _end;

            // ensure that supplied options is of correct class
            if( _options == nullptr || !zone_options::is_valid( type, *_options ) ) {
                options = zone_options::create( type );
            } else {
                options = _options;
            }
        }

        // returns true if name is changed
        bool set_name();
        // returns true if type is changed
        bool set_type();
        void set_position( const std::pair<tripoint, tripoint> &position, bool manual = true,
                           bool update_avatar = true, bool skip_cache_update = false );
        void set_enabled( bool enabled_arg );
        void set_temporary_disabled( bool enabled_arg );
        void set_is_vehicle( bool is_vehicle_arg );

        static std::string make_type_hash( const zone_type_id &_type, const faction_id &_fac ) {
            return _type.c_str() + type_fac_hash_str + _fac.c_str();
        }
        static zone_type_id unhash_type( const std::string_view hash_type ) {
            size_t end = hash_type.find( type_fac_hash_str );
            if( end != std::string::npos && end < hash_type.size() ) {
                return zone_type_id( hash_type.substr( 0, end ) );
            }
            return zone_type_id( "" );
        }
        static faction_id unhash_fac( const std::string_view hash_type ) {
            size_t start = hash_type.find( type_fac_hash_str ) + type_fac_hash_str.size();
            if( start != std::string::npos ) {
                return faction_id( hash_type.substr( start ) );
            }
            return faction_id( "" );
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
        bool get_temporarily_disabled() const {
            return temporarily_disabled;
        }

        bool get_is_vehicle() const {
            return is_vehicle;
        }
        bool get_is_personal() const {
            return is_personal;
        }
        tripoint_abs_ms get_start_point() const {
            if( is_personal ) {
                return start + cached_shift;
            }
            return tripoint_abs_ms{ start };
        }
        tripoint_abs_ms get_end_point() const {
            if( is_personal ) {
                return end + cached_shift;
            }
            return tripoint_abs_ms{ end };
        }
        void update_cached_shift( const tripoint_abs_ms &player_loc ) {
            cached_shift = player_loc;
        }
        tripoint_abs_ms get_center_point() const;
        bool has_options() const {
            return options->has_options();
        }
        const zone_options &get_options() const {
            return *options;
        }
        zone_options &get_options() {
            return *options;
        }
        // check if the entry is inside
        // if cached is set to true, use the cached location instead of the current player location
        // for personal zones. This is used when checking for a zone DURING an activity which can otherwise
        // cause issues of zones moving around
        bool has_inside( const tripoint_abs_ms &p ) const {
            // if it is personal then the zone is local
            if( is_personal ) {
                return inclusive_cuboid<tripoint_abs_ms>(
                           start + cached_shift, end + cached_shift ).contains( p );
            }
            return inclusive_cuboid<tripoint>( start, end ).contains( p.raw() );
        }
        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );
};

class zone_manager
{
    public:
        using ref_zone_data = std::reference_wrapper<zone_data>;
        using ref_const_zone_data = std::reference_wrapper<const zone_data>;

    private:
        static const int MAX_DISTANCE = ACTIVITY_SEARCH_DISTANCE;
        std::vector<zone_data> zones;
        //Containers for Revert functionality for Vehicle Zones
        //Pointer to added zone to be removed
        std::vector<zone_data *> added_vzones; // NOLINT(cata-serialize)
        //Copy of original data, pointer to the zone
        std::vector<std::pair<zone_data, zone_data *>> changed_vzones; // NOLINT(cata-serialize)
        //copy of original data to be re-added
        std::vector<zone_data> removed_vzones; // NOLINT(cata-serialize)

        std::map<zone_type_id, zone_type> types; // NOLINT(cata-serialize)

        // a count of the number of personal zones the character has
        int num_personal_zones = 0; // NOLINT(cata-serialize)

        // NOLINTNEXTLINE(cata-serialize)
        std::unordered_map<std::string, std::unordered_set<tripoint_abs_ms>> area_cache;
        // NOLINTNEXTLINE(cata-serialize)
        std::unordered_map<std::string, std::unordered_set<tripoint_abs_ms>> vzone_cache;
        std::unordered_set<tripoint_abs_ms> get_point_set( const zone_type_id &type,
                const faction_id &fac = your_fac ) const;
        std::unordered_set<tripoint_abs_ms> get_vzone_set( const zone_type_id &type,
                const faction_id &fac = your_fac ) const;
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

        void clear();

        void add( const std::string &name, const zone_type_id &type, const faction_id &faction,
                  bool invert, bool enabled,
                  const tripoint &start, const tripoint &end,
                  const shared_ptr_fast<zone_options> &options = nullptr, bool personal = false,
                  bool silent = false, map *pmap = nullptr );
        // get first matching zone
        const zone_data *get_zone_at( const tripoint_abs_ms &where, const zone_type_id &type,
                                      const faction_id &fac = your_fac ) const;
        // get all matching zones (useful for LOOT_CUSTOM and LOOT_ITEM_GROUP)
        std::vector<zone_data const *> get_zones_at( const tripoint_abs_ms &where, const zone_type_id &type,
                const faction_id &fac = your_fac ) const;
        void create_vehicle_loot_zone( class vehicle &vehicle, const point &mount_point,
                                       zone_data &new_zone, map *pmap = nullptr );

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
        void cache_data( bool update_avatar = true );
        void reset_disabled();
        void cache_avatar_location();
        void cache_vzones( map *pmap = nullptr );
        bool has( const zone_type_id &type, const tripoint_abs_ms &where,
                  const faction_id &fac = your_fac ) const;
        bool has_near( const zone_type_id &type, const tripoint_abs_ms &where,
                       int range = MAX_DISTANCE, const faction_id &fac = your_fac ) const;
        bool has_loot_dest_near( const tripoint_abs_ms &where ) const;
        bool custom_loot_has( const tripoint_abs_ms &where, const item *it,
                              const zone_type_id &ztype, const faction_id &fac = your_fac ) const;
        std::vector<zone_data const *> get_near_zones( const zone_type_id &type,
                const tripoint_abs_ms &where, int range,
                const faction_id &fac = your_fac ) const;
        std::unordered_set<tripoint_abs_ms> get_near(
            const zone_type_id &type, const tripoint_abs_ms &where, int range = MAX_DISTANCE,
            const item *it = nullptr, const faction_id &fac = your_fac ) const;
        std::optional<tripoint_abs_ms> get_nearest(
            const zone_type_id &type, const tripoint_abs_ms &where, int range = MAX_DISTANCE,
            const faction_id &fac = your_fac ) const;
        zone_type_id get_near_zone_type_for_item( const item &it, const tripoint_abs_ms &where,
                int range = MAX_DISTANCE, const faction_id &fac = your_fac ) const;
        std::vector<zone_data> get_zones( const zone_type_id &type, const tripoint_abs_ms &where,
                                          const faction_id &fac = your_fac ) const;
        const zone_data *get_zone_at( const tripoint_abs_ms &where, bool loot_only = false,
                                      const faction_id &fac = your_fac ) const;
        const zone_data *get_bottom_zone( const tripoint_abs_ms &where,
                                          const faction_id &fac = your_fac ) const;
        std::optional<std::string> query_name( const std::string &default_name = "" ) const;
        std::optional<zone_type_id> query_type( bool personal = false ) const;
        void swap( zone_data &a, zone_data &b );
        void rotate_zones( map &target_map, int turns );
        // list of tripoints of zones that are loot zones only
        std::unordered_set<tripoint> get_point_set_loot(
            const tripoint_abs_ms &where, int radius, const faction_id &fac = your_fac ) const;
        std::unordered_set<tripoint> get_point_set_loot(
            const tripoint_abs_ms &where, int radius, bool npc_search,
            const faction_id &fac = your_fac ) const;

        // 'direct' access to zone_manager::zones, giving direct access was nono
        std::vector<ref_zone_data> get_zones( const faction_id &fac = your_fac );
        std::vector<ref_const_zone_data> get_zones( const faction_id &fac = your_fac ) const;

        bool has_personal_zones() const;

        bool save_zones( std::string const &suffix = {} );
        bool save_world_zones( std::string const &suffix = {} );
        void load_zones( std::string const &suffix = {} );
        void load_world_zones( std::string const &suffix = {} );
        void zone_edited( zone_data &zone );
        void revert_vzones();
        void serialize( JsonOut &json ) const;
        void deserialize( const JsonValue &jv );
};

void mapgen_place_zone( tripoint const &start, tripoint const &end, zone_type_id const &type,
                        faction_id const &fac = your_fac, std::string const &name = {},
                        std::string const &filter = {}, map *pmap = nullptr );

#endif // CATA_SRC_CLZONES_H
