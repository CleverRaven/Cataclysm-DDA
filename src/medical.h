#ifndef  MEDICAL_H
#define  MEDICAL_H

#include "creature.h"
#include "player.h"
#include "npc.h"
#include "monster.h"

#include <string>
#include <vector>
#include <map>


/*
 * =====================================================================================
 *        Class:  medical
 *  Description:  Controls all medical aspects of a character/creature, and for players
 *  provides an interface to show all the possible options available to them.
 *  TODO: Put more info about the class here as it is fleshed out.
 * =====================================================================================
 */
class medical
{
    public:
        medical();
        medical(player *p);

        ~medical();

        /* show interface for the player */
        void show_interface();

    private:
        void init(player *p);
};

#endif   /* ----- #ifndef MEDICAL_H  ----- */
