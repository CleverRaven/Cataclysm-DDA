#ifndef AUTO_PICKUP_H
#define AUTO_PICKUP_H

#include <unordered_map>
#include <string>
#include <vector>
#include <locale>
#include <algorithm>
#include "json.h"

class auto_pickup : public JsonSerializer, public JsonDeserializer {
    private:
        // templated version of my_equal so it could work with both char and wchar_t
        template<typename charT>
        struct my_equal {
            public:
                my_equal( const std::locale &loc ) : loc_(loc) {}

                bool operator()(charT ch1, charT ch2)
                {
                    return std::toupper(ch1, loc_) == std::toupper(ch2, loc_);
                }
            private:
                const std::locale &loc_;
        };

        void test_pattern(const int iCurrentPage, const int iCurrentLine);
        std::string trim_rule(const std::string &sPatternIn);
        void merge_vector();
        void save_reset_changes(const bool bReset);
        bool match(const std::string &sTextIn, const std::string &sPatternIn);
        std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
        template<typename charT>
        int ci_find_substr( const charT &str1, const charT &str2, const std::locale &loc = std::locale() );

        void load(const bool bCharacter);
        bool save(const bool bCharacter);
        bool load_legacy(const bool bCharacter);

        bool bChar;

        enum type {
            MERGED = 0,
            GLOBAL,
            CHARACTER
        };

        class cRules
        {
            public:
                std::string sRule;
                bool bActive;
                bool bExclude;

                cRules()
                {
                    this->sRule = "";
                    this->bActive = false;
                    this->bExclude = false;
                }

                cRules(std::string sRuleIn, bool bActiveIn, bool bExcludeIn)
                {
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
         * is added as the key, with "true" or "false" as the values.
         */
        std::unordered_map<std::string, std::string> mapItems;

        /**
         * An ugly hackish mess. Contains:
         * - vRules[0] aka vRules[MERGED]: the current set of rules; used to fill @ref mapItems.
         *      Filled by a call to @ref auto_pickup::merge_vector()
         * - vRules[1,2] aka vRules[GLOBAL,CHARACTER]: current rules split into global and
         *      character-specific. Allows the editor to show one or the other.
         * - vRules[3,4]: temporary storage, filled before opening the GUI with the current rules
         *      from [1,2] (to allow cancelling changes).
         *      See also @ref auto_pickup::save_reset_changes(bool).
         */
        std::vector<cRules> vRules[5];

    public:
        bool has_rule(const std::string &sRule);
        void add_rule(const std::string &sRule);
        void remove_rule(const std::string &sRule);
        void create_rules(const std::string &sItemNameIn = "");
        void clear_character_rules();
        const std::string &check_item(const std::string &sItemName);

        void show();
        bool save_character();
        bool save_global();
        void load_character();
        void load_global();

        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const override;
        void deserialize(JsonIn &jsin) override;
};

auto_pickup &get_auto_pickup();

#endif
