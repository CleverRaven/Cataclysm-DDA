#ifndef MONDEFENSE_H
#define MONDEFENSE_H

class monster;
struct projectile;

namespace mdefense {
/**
    * @param m The monster the defends itself.
    * @param source The attacker
    * @param proj The projectile it was hit by or NULL if it
    * was attacked with a melee attack.
    */
void zapback (monster *m, Creature *source, const projectile *proj);
    
// static void none(monster *, Creature *, const projectile *) {}; // apparently unused
} //namespace mdefense

#endif
