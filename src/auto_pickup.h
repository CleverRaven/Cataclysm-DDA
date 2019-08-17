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

class auto_pickup
{
    private:
        class rules_list;

        void test_pattern( const rules_list &rules, int iRow );
        void load( bool bCharacter );
        bool save( bool bCharacter );
        bool load_legacy( bool bCharacter );

        bool bChar;

        class cRules
        {
            public:
                std::string sRule;
                bool bActive = false;
                bool bExclude = false;

                cRules() = default;

                cRules( const std::string &sRuleIn, bool bActiveIn, bool bExcludeIn ) {
                    this->sRule = sRuleIn;
                    this->bActive = bActiveIn;
                    this->bExclude = bExcludeIn;
                }

                void serialize( JsonOut &json ) const;
                void deserialize( JsonIn &jsin );
        };

        class rules_list : public std::vector<cRules>
        {
            public:
                void serialize( JsonOut &json ) const;
                void deserialize( JsonIn &jsin );
        };

        mutable bool ready; //< true if map_items has been populated from vRules

        /**
         * The currently-active set of auto-pickup rules, in a form that allows quick
         * lookup. When this is filled (by @ref auto_pickup::create_rule()), every
         * item existing in the game that matches a rule (either white- or blacklist)
         * is added as the key, with RULE_WHITELISTED or RULE_BLACKLISTED as the values.
         */
        mutable std::unordered_map<std::string, rule_state> map_items;

        rules_list global_rules;
        rules_list character_rules;

        void load_legacy_rules( rules_list &rules, std::istream &fin );

        void refresh_map_items() const; //< Only modifies mutable state
        void refresh_map_items( const rules_list &rules,
                                std::unordered_map<std::string, const itype *> &temp_items ) const;

        void create_rule( const rules_list &rules, const std::string &to_match );
        void create_rule( const rules_list &rules, const item &it );

    public:
        auto_pickup() : bChar( false ), ready( false ) {}

        void create_rule( const std::string &to_match );
        void create_rule( const item *it );
        bool has_rule( const item *it );
        void add_rule( const item *it );
        void remove_rule( const item *it );
        bool check_special_rule( const std::vector<material_id> &materials, const std::string &rule ) const;

        void clear_character_rules();
        rule_state check_item( const std::string &sItemName ) const;

        void show();
        void show( const std::string &custom_name, bool is_autopickup = true );
        bool save_character();
        bool save_global();
        void load_character();
        void load_global();

        bool empty() const;

        void deserialize( JsonIn &jsin );
};

auto_pickup &get_auto_pickup();

#endif
