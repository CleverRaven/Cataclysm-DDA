#ifndef  MEDICAL_H
#define  MEDICAL_H

#include "bodypart.h"

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
        virtual bool is_player() const
        {
            return false;
        }
        virtual bool is_npc () const
        {
            return false;
        }
        virtual bool is_monster() const
        {
            return false;
        }

    protected:
        /* show interface for the player */
        void show_medical_iface();

    private:
};

#endif   /* ----- #ifndef MEDICAL_H  ----- */
