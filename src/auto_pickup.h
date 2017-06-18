#pragma once
#ifndef AUTO_PICKUP_H
#define AUTO_PICKUP_H

#include <unordered_map>
#include <string>
#include <vector>
#include <locale>
#include <algorithm>
#include <iosfwd>
#include "json.h"
#include "enums.h"

class auto_pickup : public JsonSerializer, public JsonDeserializer
{
    private:
        void test_pattern( const int iCurrentPage, const int iCurrentLine );
        void load( const bool bCharacter );
        bool save( const bool bCharacter );
        bool load_legacy( const bool bCharacter );

        bool bChar;

        enum TAB : int {
            GLOBAL_TAB,
            CHARACTER_TAB,
            MAX_TAB
        };

        class cRules
        {
            public:
                std::string sRule;
                bool bActive;
                bool bExclude;

                cRules() {
                    this->sRule = "";
                    this->bActive = false;
                    this->bExclude = false;
                }

                cRules( std::string sRuleIn, bool bActiveIn, bool bExcludeIn ) {
                    this->sRule = sRuleIn;
                    this->bActive = bActiveIn;
                    this->bExclude = bExcludeIn;
                }

                ~cRules() {};
        };

        mutable bool ready; //< true if map_items has been populated from vRules

        /**
         * The currently-active set of auto-pickup rules, in a form that allows quick
         * lookup. When this is filled (by @ref auto_pickup::create_rule()), every
         * item existing in the game that matches a rule (either white- or blacklist)
         * is added as the key, with RULE_WHITELISTED or RULE_BLACKLISTED as the values.
         */
        mutable std::unordered_map<std::string, rule_state> map_items;

        /**
         * - vRules[0,1] aka vRules[GLOBAL,CHARACTER]: current rules split into global and
         *      character-specific. Allows the editor to show one or the other.
         */
        std::array<std::vector<cRules>, MAX_TAB> vRules;

        void load_legacy_rules( std::vector<cRules> &rules, std::istream &fin );

        void refresh_map_items() const; //< Only modifies mutable state

    public:
        auto_pickup() : bChar( false ), ready( false ) {}

        bool has_rule( const std::string &sRule );
        void add_rule( const std::string &sRule );
        void remove_rule( const std::string &sRule );
        void create_rule( const std::string &to_match );
        void clear_character_rules();
        rule_state check_item( const std::string &sItemName ) const;

        void show();
        void show( const std::string &custom_name, bool is_autopickup = true );
        bool save_character();
        bool save_global();
        void load_character();
        void load_global();

        bool empty() const;

        using JsonSerializer::serialize;
        void serialize( JsonOut &json ) const override;
        void deserialize( JsonIn &jsin ) override;
};

auto_pickup &get_auto_pickup();

#endif
