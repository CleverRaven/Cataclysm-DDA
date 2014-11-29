#ifndef  MEDICAL_H
#define  MEDICAL_H

#include <string>
#include <vector>
#include <map>

class Creature;

/*
 * =====================================================================================
 *        Class:  medical
 *  Description:  Controls all medical aspects of a character/creature, and for players
 *  provides an interface to show all the possible options available to them.
 *  TODO: Put more info about the class here as it is fleshed out.
 * =====================================================================================
 */
class Medical
{
    public:
        Medical();
        /* Must be given the Creature's class pointer */
        Medical(Creature *c);
        ~Medical();

    protected:
        void medical_init(Creature *c);

        /* show interface for the player */
        void show_interface();

    private:
        /* The polymorphic Creature class to interface with player, npc, and monster classes.
         * 
         */
        Creature *us;
};

#endif   /* ----- #ifndef MEDICAL_H  ----- */
