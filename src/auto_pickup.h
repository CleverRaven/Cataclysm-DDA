#ifndef AUTO_PICKUP_H
#define AUTO_PICKUP_H

#include <unordered_map>
#include <string>
#include <vector>
#include <locale>
#include <algorithm>
#include "json.h"

enum pickup_rule_state : int {
    RULE_NONE,
    RULE_WHITELISTED,
    RULE_BLACKLISTED
};

class auto_pickup : public JsonSerializer, public JsonDeserializer
{
    private:
        // templated version of my_equal so it could work with both char and wchar_t
        template<typename charT>
        struct my_equal {
            public:
                my_equal( const std::locale &loc ) : loc_( loc ) {}

                bool operator()( charT ch1, charT ch2 ) {
                    return std::toupper( ch1, loc_ ) == std::toupper( ch2, loc_ );
                }
            private:
                const std::locale &loc_;
        };

        void test_pattern( const int iCurrentPage, const int iCurrentLine );
        std::string trim_rule( const std::string &sPatternIn );
        bool match( const std::string &sTextIn, const std::string &sPatternIn );
        std::vector<std::string> &split( const std::string &s, char delim,
                                         std::vector<std::string> &elems );
        template<typename charT>
        int ci_find_substr( const charT &str1, const charT &str2, const std::locale &loc = std::locale() );

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

        /**
         * The currently-active set of auto-pickup rules, in a form that allows quick
         * lookup. When this is filled (by @ref auto_pickup::create_rules()), every
         * item existing in the game that matches a rule (either white- or blacklist)
         * is added as the key, with RULE_WHITELISTED or RULE_BLACKLISTED as the values.
         */
        std::unordered_map<std::string, pickup_rule_state> map_items;

        /**
         * - vRules[0,1] aka vRules[GLOBAL,CHARACTER]: current rules split into global and
         *      character-specific. Allows the editor to show one or the other.
         */
        std::array<std::vector<cRules>, MAX_TAB> vRules;

    public:
        bool has_rule( const std::string &sRule );
        void add_rule( const std::string &sRule );
        void remove_rule( const std::string &sRule );
        void create_rules();
        void create_rule( const std::string &to_match );
        void clear_character_rules();
        pickup_rule_state check_item( const std::string &sItemName ) const;

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
