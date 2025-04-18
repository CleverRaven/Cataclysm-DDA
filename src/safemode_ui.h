#pragma once
#ifndef CATA_SRC_SAFEMODE_UI_H
#define CATA_SRC_SAFEMODE_UI_H

#include <array>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "creature.h"
#include "enums.h"

class JsonArray;
class JsonOut;

class safemode
{
    private:
        enum Tabs : int {
            GLOBAL_TAB,
            CHARACTER_TAB,
            MAX_TAB
        };

        enum class Categories : int {
            HOSTILE_SPOTTED,
            SOUND
        };

        enum class MovementModes : int {
            WALKING,
            DRIVING,
            BOTH
        };

        class rules_class
        {
            public:
                std::string rule;
                bool active;
                bool whitelist;
                Creature::Attitude attitude;
                int proximity;
                Categories category;
                MovementModes movement_mode;

                rules_class() : active( false ), whitelist( false ), attitude( Creature::Attitude::HOSTILE ),
                    proximity( 0 ), category( Categories::HOSTILE_SPOTTED ), movement_mode( MovementModes::BOTH ) {}
                rules_class( const std::string &rule_in, bool active_in, bool whitelist_in,
                             Creature::Attitude attitude_in, int proximity_in, Categories cat,
                             MovementModes movement_mode_in ) : rule( rule_in ),
                    active( active_in ), whitelist( whitelist_in ),
                    attitude( attitude_in ), proximity( proximity_in ), category( cat ),
                    movement_mode( movement_mode_in ) {}
        };

        class rule_state_class
        {
            public:
                rule_state state;
                int proximity;

                rule_state_class() : state( rule_state::NONE ), proximity( 0 ) {}
                rule_state_class( rule_state state_in, int proximity_in, Categories ) : state( state_in ),
                    proximity( proximity_in ) {}
        };

        /**
         * The currently-active set of safemode rules, in a form that allows quick
         * lookup. When this is filled (by @ref safemode::create_rules()), every
         * monster existing in the game that matches a rule (either white- or blacklist)
         * is added as the key, with rule_state::WHITELISTED or rule_state::BLACKLISTED as the values.
         * safemode_rules[ 'creature name' ][ 'movement_mode' ][ 'attitude' ].rule_state_class('rule_state', 'proximity')
         */
        // NOLINTNEXTLINE(cata-serialize)
        std::unordered_map < std::string, std::array < std::array < rule_state_class, 3 >, 2 >>
                safemode_rules_hostile; // NOLINT(cata-serialize)
        // NOLINTNEXTLINE(cata-serialize)
        std::array < std::vector < rules_class >, 2 > safemode_rules_sound;

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
        std::string lastmon_whitelist; // NOLINT(cata-serialize)

        bool has_rule( std::string_view rule_in, Creature::Attitude attitude_in );
        void add_rule( const std::string &rule_in, Creature::Attitude attitude_in,
                       int proximity_in, rule_state state_in );
        void remove_rule( std::string_view rule_in, Creature::Attitude attitude_in );
        void clear_character_rules();
        rule_state check_monster( const std::string &creature_name_in, Creature::Attitude attitude_in,
                                  int proximity_in, bool driving = false ) const;

        bool is_sound_safe( const std::string &sound_name_in, int proximity_in,
                            bool driving = false ) const;

        std::string npc_type_name();

        void show();
        void show( const std::string &custom_name_in, bool is_safemode_in );

        bool save_character();
        bool save_global();
        void load_character();
        void load_global();

        bool empty() const;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonArray &ja );
};

safemode &get_safemode();

#endif // CATA_SRC_SAFEMODE_UI_H
