#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "bodypart.h"
#include "speech.h"
#include "messages.h"
#include <algorithm>


void mdefense::zapback(monster *m, const projectile* proj)
{
    int j;
    if (rl_dist(m->posx(), m->posy(), g->u.posx, g->u.posy) > 1 ||
        !g->sees_u(m->posx(), m->posy(), j))
        {
        return; // Out of range
        }
    if (proj != NULL)
        {
        return; // Not a melee attack
        }
    if ((!g->u.has_active_bionic("bio_faraday") && !g->u.worn_with_flag("ELECTRIC_IMMUNE") && !g->u.has_artifact_with(AEP_RESIST_ELECTRICITY)) && 
        (g->u.weapon.conductive() || g->u.unarmed_attack()) && (rng(0,100) <= m->def_chance))
        {
        damage_instance shock;
        shock.add_damage(DT_ELECTRIC, rng(1,5));
        g->u.deal_damage(m, bp_arms, 1, shock);
        add_msg(m_bad, ("Striking the %s shocks you!"), m->name().c_str());
        }
    return;
}
