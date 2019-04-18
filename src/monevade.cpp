#include "monevade.h"

#include <algorithm>

#include "creature.h"
#include "game.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "output.h"
#include "player.h"
#include "projectile.h"
#include "rng.h"
#include "translations.h"

const efftype_id effect_stunned( "stunned" );

void mevade::none( monster &, Creature *, const dealt_projectile_attack * )
{
}

void mevade::stagger_dodge( monster &m, Creature *const source,
                            dealt_projectile_attack const *proj )
{
    if( source == nullptr ) {
        return;
    }
    // Can't stagger on projectile attacks.
    if( proj != nullptr ) {
        return;
    }

    // Find a new square to dodge to that's empty and that's adjacent to us and no farther from the attacker.
    std::vector<tripoint> dests;
    const tripoint &m_pos = m.pos();
    const tripoint &source_pos = source->pos();
    const int current_dist = square_dist( m_pos, source_pos );
    for( const tripoint &dest : g->m.points_in_radius( m_pos, 1 ) ) {
        if( square_dist( source_pos, dest ) > current_dist ) {
            continue;
        }
        if( !g->is_empty( dest ) ) {
            continue;
        }
        if( !m.has_flag( MF_SWIMS ) && g->m.has_flag( "LIQUID", dest ) ) {
            continue;
        }
        dests.push_back( dest );
    }
    m.setpos( random_entry( dests ) );

    source->add_msg_if_player( m_bad,
                               _( "The %s shifts away from your attack, sending you reeling." ), m.name() );
    source->add_effect( effect_stunned, rng( 2_turns, 3_turns ) );
}
