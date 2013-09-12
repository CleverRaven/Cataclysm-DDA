#ifndef MONSTER_FACTORY_H
#define MONSTER_FACTORY_H

#include "game.h"
#include "monster.h"
#include "mtype.h"

#include "color.h"
#include "picojson.h"
#include "catajson.h"

#include "mondeath.h"
#include "monattack.h"

typedef void (mdeath::*MonDeathFunction)(game*, monster*);
typedef void (mattack::*MonAttackFunction)(game*, monster*);
typedef std::string Mon_Tag;

// for the death/attack functions
class game;
class monster;

class monster_factory
{
    public:
        // Setup Methods
        /** Default constructor */
        monster_factory();
        void init();
        void init(game* main_game) throw (std::string);

        // Intermediate Methods


        /** Default destructor */
        ~monster_factory();
    protected:
    private:
        // init functions
        void init_death_functions();
        void init_attack_functions();
        void init_sizes();
        void init_flags();
        void init_triggers();
        void init_phases();

        void set_flags(mtype *mon);

        void load_monster_templates() throw (std::string);
        void load_monster_templates_from(const std::string file_name) throw (std::string);

        // utility functions
        m_size size_from_tag(Mon_Tag name);
        phase_id phase_from_tag(Mon_Tag name);

        void tags_from_json(catajson tag_list, std::set<std::string> &tags);
        void set_species_defaults(std::set<std::string> species_tags, mtype *mon);
        void set_monster_flags(std::set<std::string> tags, std::map<Mon_Tag, m_flag> &target);
        void set_monster_triggers(std::set<std::string> tags, std::map<Mon_Tag, monster_trigger> &target);

        void finalize_monsters();
        void set_bitflags();
        void set_flags();
        void set_triggers();

        // missing monster type
        mtype *m_missing_type;
        // storage for monster templates
        std::map<Mon_Tag, mtype*> mon_templates;
        // storage for monster species
        std::map<Mon_Tag, species_type*> mon_species;
        // categories list, vector for now?
        std::vector<Mon_Tag> mon_categories;

        // monster death functions
        std::map<Mon_Tag, MonDeathFunction> mdeath_function_list;
        // monster attack functions
        std::map<Mon_Tag, MonAttackFunction> mattack_function_list;
        // monster sizes
        std::map<Mon_Tag, m_size> mon_size_list;
        // monster flags
        std::map<Mon_Tag, m_flag> mon_flags;
        // monster triggers
        std::map<Mon_Tag, monster_trigger> mon_triggers;
        // phases
        std::map<Mon_Tag, phase_id> phase_map;

};

extern monster_factory *monster_controller;

#endif // MONSTER_FACTORY_H
