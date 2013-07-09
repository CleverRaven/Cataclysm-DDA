#ifndef _AUTO_PICKUP_H_
#define _AUTO_PICKUP_H_

#include <string>
#include <vector>

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

extern std::vector<cPickupRules> vAutoPickupRules[5];

void merge_vector();
void save_reset_changes(bool bReset);
void show_auto_pickup();
void load_auto_pickup(bool bCharacter);
void save_auto_pickup(bool bCharacter);
std::string auto_pickup_header(bool bCharacter);
void create_default_auto_pickup(bool bCharacter);
bool auto_pickup_match(std::string sText, std::string sPattern);
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);

#endif
