#include "mondodge.h"

#include <algorithm>

#include "ballistics.h"
#include "bodypart.h"
#include "creature.h"
#include "damage.h"
#include "dispersion.h"
#include "game.h"
#include "messages.h"
#include "monster.h"
#include "output.h"
#include "player.h"
#include "projectile.h"
#include "rng.h"
#include "translations.h"

void mdodge::none( monster &, Creature *, const dealt_projectile_attack * )
{
}

void mdodge::telestagger( monster &m, Creature *const source,
    dealt_projectile_attack const *proj )
{
}
