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

extern std::vector<cPickupRules> vAutoPickupRules;

void show_auto_pickup();
void load_auto_pickup();
void save_auto_pickup();
std::string auto_pickup_header();
void create_default_auto_pickup();

#endif
