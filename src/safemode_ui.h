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
        void test_pattern( const int tab_in, const int row_in );

        void load( const bool character_in );
        bool save( const bool character_in );

        bool character;

        enum Tabs : int {
            GLOBAL_TAB,
            CHARACTER_TAB,
            MAX_TAB
        };

        class rules_class
        {
            public:
                std::string rule;
                bool active;
                bool exclude;
                Creature::Attitude attitude;
                int proximity;

                rules_class() {
                    this->rule = "";
                    this->active = false;
                    this->exclude = false;
                    this->attitude = Creature::A_HOSTILE;
                    this->proximity = 0;
                }

                rules_class( std::string rule_in, bool active_in, bool exclude_in,
                             Creature::Attitude attitude_in, int proximity_in ) {
                    this->rule = rule_in;
                    this->active = active_in;
                    this->exclude = exclude_in;
                    this->attitude = attitude_in;
                    this->proximity = proximity_in;
                }

                ~rules_class() {};
        };

        class rule_state_class
        {
            public:
                rule_state state;
                int proximity;

                rule_state_class() {
                    this->state = RULE_NONE;
                    this->proximity = 0;
                }

                rule_state_class( rule_state state_in, int proximity_in ) {
                    this->state = state_in;
                    this->proximity = proximity_in;
                }

                ~rule_state_class() {};
        };

        /**
         * The currently-active set of safemode rules, in a form that allows quick
         * lookup. When this is filled (by @ref safemode::create_rules()), every
         * monster existing in the game that matches a rule (either white- or blacklist)
         * is added as the key, with RULE_WHITELISTED or RULE_BLACKLISTED as the values.
         * safemode_rules[ 'creature name' ][ 'attitude' ].rule_state_class('rule_state', 'proximity')
         */
        std::unordered_map < std::string, std::array < rule_state_class,
            Creature::A_MAX - 1 > > safemode_rules;

        /**
         * - vRules[0,1] aka vRules[GLOBAL,CHARACTER]: current rules split into global and
         *      character-specific. Allows the editor to show one or the other.
         */
        std::array<std::vector<rules_class>, MAX_TAB> rules;

    public:
        std::string whitelist;

        bool has_rule( const std::string &rule_in, const Creature::Attitude attitude_in );
        void add_rule( const std::string &rule_in, const Creature::Attitude attitude_in,
                       const int proximity_in, const rule_state state_in );
        void remove_rule( const std::string &rule_in, const Creature::Attitude attitude_in );
        void create_rules();
        void clear_character_rules();
        rule_state check_monster( const std::string &creature_name_in, const Creature::Attitude attitude_in,
                                  const int proximity_in ) const;

        void show();
        void show( const std::string &custom_name_in, bool is_autopickup_in );

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
