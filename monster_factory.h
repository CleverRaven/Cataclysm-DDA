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
        void load_monster_templates() throw (std::string);
        void load_monster_templates_from(const std::string file_name) throw (std::string);


        // missing monster type
        mtype *m_missing_type;
        // storage for monster templates
        std::map<Mon_Tag, mtype*> mon_templates;
        // monster death functions
        std::map<Mon_Tag, MonDeathFunction> mdeath_function_list;
        // monster attack functions
        std::map<Mon_Tag, MonAttackFunction> mattack_function_list;
        // monster sizes
        std::map<std::string, m_size> mon_size_list;

};

extern monster_factory *monster_controller;

#endif // MONSTER_FACTORY_H
