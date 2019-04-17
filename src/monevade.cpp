#include "monevade.h"

#include <algorithm>

#include "creature.h"
#include "game.h"
#include "messages.h"
#include "monster.h"
#include "output.h"
#include "player.h"
#include "projectile.h"
#include "rng.h"
#include "translations.h"

const efftype_id effect_stunned( "stunned" );

void mevade::none( monster &, Creature *, const dealt_projectile_attack * )
{
}

void mevade::telestagger( monster &m, Creature *const source,
    dealt_projectile_attack const *proj )
{
    if( source == nullptr ) {
        return;
    }
    // Can't stagger on ranged attacks.
    if( proj != nullptr ) {
        return;
    }

    // Find another open position adjacent to the attacker.
    const tripoint adjacent = source->adjacent_tile();
    if ( adjacent == source->pos() || adjacent == m.pos() ) {
        // If we have nowhere to teleport, we can't stagger the attacker.
        return;
    }

    m.setpos(adjacent);
    source->add_msg_if_player( m_bad, _( "The %s teleports away from your attack, sending you reeling." ), m.name() );
    source->add_effect( effect_stunned, rng( 2_turns, 3_turns ) );
}
