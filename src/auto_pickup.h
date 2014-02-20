#ifndef _AUTO_PICKUP_H_
#define _AUTO_PICKUP_H_

#include <string>
#include <vector>
#include <locale>
#include <algorithm>

class cPickupRules {
    public:
        std::string sRule;
        bool bActive;
        bool bExclude;

        cPickupRules() {
            this->sRule = "";
            this->bActive = false;
            this->bExclude = false;
        }

        cPickupRules(std::string sRuleIn, bool bActiveIn, bool bExcludeIn) {
            this->sRule = sRuleIn;
            this->bActive = bActiveIn;
            this->bExclude = bExcludeIn;
        }

        ~cPickupRules() {};
};

extern std::map<std::string, std::string> mapAutoPickupItems;
extern std::vector<cPickupRules> vAutoPickupRules[5];

void test_pattern(int iCurrentPage, int iCurrentLine);
std::string trim_rule(std::string sPattern);
void merge_vector();
bool hasPickupRule(std::string sRule);
void addPickupRule(std::string sRule);
void removePickupRule(std::string sRule);
void createPickupRules(const std::string sItemNameIn = "");
void save_reset_changes(bool bReset);
void show_auto_pickup();
void load_auto_pickup(bool bCharacter);
bool save_auto_pickup(bool bCharacter);
void show_auto_pickup();
std::string auto_pickup_header(bool bCharacter);
void create_default_auto_pickup(bool bCharacter);
bool auto_pickup_match(std::string sText, std::string sPattern);
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
template<typename charT>
int ci_find_substr( const charT& str1, const charT& str2, const std::locale& loc = std::locale() );

#endif
