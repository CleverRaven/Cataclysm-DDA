#pragma once
#ifndef MONEVADE_H
#define MONEVADE_H

class monster;
class Creature;
struct dealt_projectile_attack;

namespace mevade
{
/**
* @param m The monster that dodged.
* @param source The attacker.
* @param proj The projectile it dodged or NULL if it dodged a melee attack.
*/
void telestagger( monster &m, Creature *source, const dealt_projectile_attack *proj );

void none( monster &, Creature *, const dealt_projectile_attack * );
} //namespace mdefense

#endif
