#include "mondefense.h"
#include "monster.h"
#include "creature.h"
#include "damage.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "bodypart.h"
#include "messages.h"

void mdefense::none(monster *, Creature *, const projectile *)
{
}

void mdefense::zapback(monster *const m, Creature *const source, projectile const* const proj)
{
    // Not a melee attack, attacker lucked out or out of range
    if( (proj) || (rng(0, 100) > m->def_chance) || (rl_dist(m->pos(), source->pos()) > 1) ) {
        return;
    }

    bool can_shock = source != nullptr && !source->is_elec_immune();
    if( !can_shock ) {
        return;
    }

    // Players/NPCs can avoid the shock by using non-conductive weapons
    player const *const foe = dynamic_cast<player*>(source);
    if( foe != nullptr && ( !foe->weapon.conductive() && !foe->unarmed_attack() ) ) {
        return;
    }

    damage_instance const shock {DT_ELECTRIC, static_cast<float>(rng(1, 5))};
    source->deal_damage(m, bp_arm_l, shock);
    source->deal_damage(m, bp_arm_r, shock);

    if( g->u.sees(source->pos()) ) {
        auto const msg_type = (source == &g->u) ? m_bad : m_info;
        add_msg(msg_type, _("Striking the %s shocks %s!"),
            m->name().c_str(), source->disp_name().c_str());
    }
    source->check_dead_state();
}
