#pragma once
#ifndef AUTO_PICKUP_H
#define AUTO_PICKUP_H

#include <array>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

#include "enums.h"
#include "type_id.h"

class JsonOut;
class JsonIn;
class item;
struct itype;

/**
 * The currently-active set of auto-pickup rules, in a form that allows quick
 * lookup. When this is filled (by @ref auto_pickup::create_rule()), every
 * item existing in the game that matches a rule (either white- or blacklist)
 * is added as the key, with RULE_WHITELISTED or RULE_BLACKLISTED as the values.
 */
class auto_pickup_cache : public std::unordered_map<std::string, rule_state>
{
    public:
        /// Defines whether this cache has been filled.
        bool ready = false;

        /// Temporary data used while filling the cache.
        std::unordered_map<std::string, const itype *> temp_items;
};

/**
 * A single entry in the list of auto pickup entries @ref auto_pickup_rule_list.
 * The data contained can be edited by the player and determines what to pick/ignore.
 */
class auto_pickup_rule
{
    public:
        std::string sRule;
        bool bActive = false;
        bool bExclude = false;

        auto_pickup_rule() = default;

        auto_pickup_rule( const std::string &sRuleIn, bool bActiveIn, bool bExcludeIn ) {
            this->sRule = sRuleIn;
            this->bActive = bActiveIn;
            this->bExclude = bExcludeIn;
        }

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );

        void test_pattern() const;

        static bool check_special_rule( const std::vector<material_id> &materials,
                                        const std::string &rule );
};

/**
 * A list of rules. This is primarily a container with a few convenient functions (like saving/loading).
 */
class auto_pickup_rule_list : public std::vector<auto_pickup_rule>
{
    public:
        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );

        void load_legacy_rules( std::istream &fin );

        void refresh_map_items( auto_pickup_cache &map_items ) const;

        void create_rule( auto_pickup_cache &map_items, const std::string &to_match );
        void create_rule( auto_pickup_cache &map_items, const item &it );
};

class auto_pickup
{
    private:
        void load( bool bCharacter );
        bool save( bool bCharacter );
        bool load_legacy( bool bCharacter );

        mutable auto_pickup_cache map_items;

        auto_pickup_rule_list global_rules;
        auto_pickup_rule_list character_rules;

        void refresh_map_items() const; //< Only modifies mutable state

    public:
        void create_rule( const std::string &to_match );
        void create_rule( const item *it );
        bool has_rule( const item *it );
        void add_rule( const item *it );
        void remove_rule( const item *it );

        void clear_character_rules();
        rule_state check_item( const std::string &sItemName ) const;

        void show();
        void show( const std::string &custom_name, bool is_autopickup = true );
        bool save_character();
        bool save_global();
        void load_character();
        void load_global();

        bool empty() const;
};

auto_pickup &get_auto_pickup();

#endif
