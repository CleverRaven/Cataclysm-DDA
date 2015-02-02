#ifndef MONDEFENSE_H
#define MONDEFENSE_H

class monster;
class Creature;
struct projectile;

class mdefense
{
    public:
        /**
         * @param m The monster the defends itself.
         * @param source The attacker
         * @param proj The projectile it was hit by or NULL if it
         * was attacked with a melee attack.
         */
        void zapback            (monster *m, Creature *source, const projectile *proj);
        void none               (monster *, Creature *, const projectile *) {};
};

#endif
