#ifndef AUTO_PICKUP_H
#define AUTO_PICKUP_H

#include <map>
#include <string>
#include <vector>
#include <locale>
#include <algorithm>
#include "json.h"

class apu : public JsonSerializer, public JsonDeserializer {
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
    public:
        enum apu_type {
            APU_MERGED = 0,
            APU_GLOBAL,
            APU_CHARACTER
        };

        class cPickupRules
        {
            public:
                std::string sRule;
                bool bActive;
                bool bExclude;

                cPickupRules()
                {
                    this->sRule = "";
                    this->bActive = false;
                    this->bExclude = false;
                }

                cPickupRules(std::string sRuleIn, bool bActiveIn, bool bExcludeIn)
                {
                    this->sRule = sRuleIn;
                    this->bActive = bActiveIn;
                    this->bExclude = bExcludeIn;
                }

                ~cPickupRules() {};
        };

        void test_pattern(int iCurrentPage, int iCurrentLine);
        std::string trim_rule(std::string sPattern);
        void merge_vector();
        bool hasPickupRule(std::string sRule);
        void addPickupRule(std::string sRule);
        void removePickupRule(std::string sRule);
        void createPickupRules(const std::string sItemNameIn = "");
        bool checkExcludeRules(const std::string sItemNameIn);
        void save_reset_changes(bool bReset);

        void show_auto_pickup();
        void load_auto_pickup(bool bCharacter);
        bool save_auto_pickup(bool bCharacter);

        std::string auto_pickup_header(bool bCharacter);
        void create_default_auto_pickup(bool bCharacter);
        bool auto_pickup_match(std::string sText, std::string sPattern);
        std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
        template<typename charT>
        int ci_find_substr( const charT &str1, const charT &str2, const std::locale &loc = std::locale() );

        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const override;
        void deserialize(JsonIn &jsin) override;
};

extern std::map<std::string, std::string> mapAutoPickupItems;
extern std::vector<apu::cPickupRules> vAutoPickupRules[5];

apu &get_apu();

#endif
