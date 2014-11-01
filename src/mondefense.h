#ifndef MONDEFENSE_H
#define MONDEFENSE_H

class monster;
struct projectile;

class mdefense
{
    public:
        /**
         * @param m The monster the defends itself.
         * @param proj The projectile it was hit by or NULL if it
         * was attacked with a melee attack.
         */
        void zapback            (monster *m, const projectile *proj);
        void none               (monster *, const projectile *) {};
};

#endif
