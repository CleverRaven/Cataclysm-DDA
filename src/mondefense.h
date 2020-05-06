#pragma once
#ifndef CATA_SRC_MONDEFENSE_H
#define CATA_SRC_MONDEFENSE_H

class monster;
class Creature;
struct dealt_projectile_attack;

namespace mdefense
{
/**
    * @param m The monster the defends itself.
    * @param source The attacker
    * @param proj The projectile it was hit by or NULL if it
    * was attacked with a melee attack.
    */
void zapback( monster &m, Creature *source, const dealt_projectile_attack *proj );
void acidsplash( monster &m, Creature *source, const dealt_projectile_attack *proj );
void return_fire( monster &m, Creature *source, const dealt_projectile_attack *proj );

void none( monster &, Creature *, const dealt_projectile_attack * );
} //namespace mdefense

#endif // CATA_SRC_MONDEFENSE_H
