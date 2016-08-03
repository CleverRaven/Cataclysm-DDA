#ifndef SEFEMODE_UI_H
#define SEFEMODE_UI_H

#include <unordered_map>
#include <string>
#include <vector>
#include <locale>
#include <algorithm>
#include "json.h"
#include "enums.h"
#include "creature.h"

class safemode : public JsonSerializer, public JsonDeserializer
{
    private:
        void test_pattern( const int iCurrentPage, const int iCurrentLine );

        void load( const bool bCharacter );
        bool save( const bool bCharacter );

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
                int attCreature;
                int iProxyDist;

                cRules() {
                    this->sRule = "";
                    this->bActive = false;
                    this->bExclude = false;
                    this->attCreature = Creature::A_HOSTILE;
                    this->iProxyDist = 0;
                }

                cRules( std::string sRuleIn, bool bActiveIn, bool bExcludeIn, int attCreatureIn, int iProxyDistIn ) {
                    this->sRule = sRuleIn;
                    this->bActive = bActiveIn;
                    this->bExclude = bExcludeIn;
                    this->attCreature = attCreatureIn;
                    this->iProxyDist = iProxyDistIn;
                }

                ~cRules() {};
        };

        /**
         * The currently-active set of safemode rules, in a form that allows quick
         * lookup. When this is filled (by @ref safemode::create_rules()), every
         * monster existing in the game that matches a rule (either white- or blacklist)
         * is added as the key, with RULE_WHITELISTED or RULE_BLACKLISTED as the values.
         */
        //                 mob name     monAttitude          rule_state  proxy dist
        std::unordered_map<std::string, std::array<std::pair<rule_state, int>, Creature::A_MAX> > map_monsters;

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
        rule_state check_monster( const std::string &sMonsterName, const int att, const int iDist ) const;

        void show();
        void show( const std::string &custom_name, bool is_autopickup );

        bool save_character();
        bool save_global();
        void load_character();
        void load_global();

        bool empty() const;

        using JsonSerializer::serialize;
        void serialize( JsonOut &json ) const override;
        void deserialize( JsonIn &jsin ) override;
};

safemode &get_safemode();

#endif
