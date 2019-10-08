#pragma once
#ifndef SAFEMODE_UI_H
#define SAFEMODE_UI_H

#include <string>
#include <unordered_map>
#include <vector>
#include <array>

#include "creature.h"
#include "enums.h"

class JsonIn;
class JsonOut;

class safemode
{
    private:
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
                bool whitelist;
                Creature::Attitude attitude;
                int proximity;

                rules_class() : active( false ), whitelist( false ), attitude( Creature::A_HOSTILE ),
                    proximity( 0 ) {}
                rules_class( const std::string &rule_in, bool active_in, bool whitelist_in,
                             Creature::Attitude attitude_in, int proximity_in ) : rule( rule_in ),
                    active( active_in ), whitelist( whitelist_in ),
                    attitude( attitude_in ), proximity( proximity_in ) {}
        };

        class rule_state_class
        {
            public:
                rule_state state;
                int proximity;

                rule_state_class() : state( RULE_NONE ), proximity( 0 ) {}
                rule_state_class( rule_state state_in, int proximity_in ) : state( state_in ),
                    proximity( proximity_in ) {}
        };

        /**
         * The currently-active set of safemode rules, in a form that allows quick
         * lookup. When this is filled (by @ref safemode::create_rules()), every
         * monster existing in the game that matches a rule (either white- or blacklist)
         * is added as the key, with RULE_WHITELISTED or RULE_BLACKLISTED as the values.
         * safemode_rules[ 'creature name' ][ 'attitude' ].rule_state_class('rule_state', 'proximity')
         */
        std::unordered_map < std::string, std::array < rule_state_class, 3 > > safemode_rules;

        /**
         * current rules for global and character tab
         */
        std::vector<rules_class> global_rules;
        std::vector<rules_class> character_rules;

        void test_pattern( int tab_in, int row_in );

        void load( bool is_character_in );
        bool save( bool is_character_in );

        bool is_character = false;

        void create_rules();
        void add_rules( const std::vector<rules_class> &rules_in );
        void set_rule( const rules_class &rule_in, const std::string &name_in, rule_state rs_in );

    public:
        std::string lastmon_whitelist;

        bool has_rule( const std::string &rule_in, Creature::Attitude attitude_in );
        void add_rule( const std::string &rule_in, Creature::Attitude attitude_in,
                       int proximity_in, rule_state state_in );
        void remove_rule( const std::string &rule_in, Creature::Attitude attitude_in );
        void clear_character_rules();
        rule_state check_monster( const std::string &creature_name_in, Creature::Attitude attitude_in,
                                  int proximity_in ) const;

        std::string npc_type_name();

        void show();
        void show( const std::string &custom_name_in, bool is_safemode_in );

        bool save_character();
        bool save_global();
        void load_character();
        void load_global();

        bool empty() const;

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
};

safemode &get_safemode();

#endif
