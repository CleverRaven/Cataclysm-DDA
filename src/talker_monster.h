#pragma once
#ifndef CATA_SRC_TALKER_MONSTER_H
#define CATA_SRC_TALKER_MONSTER_H

#include <functional>
#include <list>
#include <string>
#include <vector>

#include "coordinates.h"
#include "talker.h"
#include "type_id.h"

class faction;
class item;
class mission;
class npc;
class time_duration;
class vehicle;
struct tripoint;
class monster;

/*
 * Talker wrapper class for monster.
 */
class talker_monster: public talker
{
    public:
        explicit talker_monster( monster *new_me ): me_mon( new_me ) {
        }
        ~talker_monster() override = default;

        // underlying element accessor functions
        monster *get_monster() override {
            return me_mon;
        }
        monster *get_monster() const override {
            return me_mon;
        }
        Creature *get_creature() override {
            return me_mon;
        }
        Creature *get_creature() const override {
            return me_mon;
        }
        // identity and location
        std::string disp_name() const override;

        int posx() const override;
        int posy() const override;
        int posz() const override;
        tripoint pos() const override;
        tripoint_abs_ms global_pos() const override;
        tripoint_abs_omt global_omt_location() const override;

        int pain_cur() const override;

        // effects and values
        bool has_effect( const efftype_id &effect_id, const bodypart_id &bp ) const override;
        effect get_effect( const efftype_id &effect_id, const bodypart_id &bp ) const override;
        void add_effect( const efftype_id &new_effect, const time_duration &dur,
                         std::string bp, bool permanent, bool force, int intensity ) override;
        void remove_effect( const efftype_id &old_effect ) override;
        void mod_pain( int amount ) override;

        std::string get_value( const std::string &var_name ) const override;
        void set_value( const std::string &var_name, const std::string &value ) override;
        void remove_value( const std::string &var_name ) override;

        std::string short_description() const override;
        int get_anger() const override;
        void set_anger( int ) override;
        int morale_cur() const override;
        void set_morale( int ) override;
        int get_friendly() const override;
        void set_friendly( int ) override;
        bool will_talk_to_u( const Character &u, bool force ) override;
        std::vector<std::string> get_topics( bool radio_contact ) override;
    protected:
        talker_monster() = default;
        monster *me_mon;
};
#endif // CATA_SRC_TALKER_MONSTER_H
