#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "bodypart.h"
#include "speech.h"
#include <algorithm>


void mdefense::zapback(monster *m)
{
	int j;
		if (rl_dist(m->posx(), m->posy(), g->u.posx, g->u.posy) > 1 || !g->sees_u(m->posx(), m->posy(), j))
	return; // Out of range
	if (g->u.weapon.conductive() || g->u.unarmed_attack() && (rng(0,100) <= m->def_chance));{
        damage_instance shock;
        shock.add_damage(DT_ELECTRIC, rng(1,5));
		g->u.deal_damage(m, bp_arms, 1, shock);
		g->add_msg(("As you strike, the %s shocks you!"), m->name().c_str());}
	return;
};