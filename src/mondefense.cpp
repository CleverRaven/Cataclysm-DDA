#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "bodypart.h"
#include "speech.h"
#include "messages.h"
#include <algorithm>


void mdefense::zapback(monster *m, Creature *source, const projectile *proj)
{
    player *foe = dynamic_cast< player* >( source );
    monster *othermon = dynamic_cast< monster* >( source );
    if( rl_dist( m->pos(), source->pos() ) > 1 ) {
        return; // Out of range
    }
    if( proj != nullptr || rng(0, 100) > m->def_chance ) {
        return; // Not a melee attack or attacker lucked out
    }
    if( ( foe != nullptr && // Player/NPC stuff
          ( foe->weapon.conductive() || foe->unarmed_attack() ) && // Eligible for zap
          ( !foe->has_active_bionic("bio_faraday") && !foe->worn_with_flag("ELECTRIC_IMMUNE") &&
          !foe->has_artifact_with(AEP_RESIST_ELECTRICITY) ) ) || // Not immune
        ( othermon != nullptr && // Monster stuff
          othermon->type->sp_defense != &mdefense::zapback && !othermon->has_flag( MF_ELECTRIC ) ) ) {
        damage_instance shock;
        shock.add_damage(DT_ELECTRIC, rng(1, 5));
        source->deal_damage(m, bp_arm_l, shock);
        source->deal_damage(m, bp_arm_r, shock);
        if (g->u.sees(source->pos())) {
        auto msg_type = source == &g->u ? m_bad : m_info;
        add_msg( msg_type, _("Striking the %s shocks %s!"), m->name().c_str(), source->disp_name().c_str() );
        }
    }
    return;
}
