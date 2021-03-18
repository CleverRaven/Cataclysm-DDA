#pragma once
#ifndef CATA_SRC_MONDEATH_H
#define CATA_SRC_MONDEATH_H

class monster;

namespace mdeath
{
// Drop a body
void normal( monster &z );
// Overkill splatter (also part of normal under conditions)
void splatter( monster &z );
// Acid instead of a body
void acid( monster &z );
// Explodes in vomit :3
void boomer( monster &z );
// Explodes in vomit :3
void boomer_glow( monster &z );
// Kill all nearby vines
void kill_vines( monster &z );
// Kill adjacent vine if it's cut
void vine_cut( monster &z );
// Destroy all roots
void triffid_heart( monster &z );
// Explodes in spores D:
void fungus( monster &z );
// Falls apart
void disintegrate( monster &z );
// Screams loudly
void shriek( monster &z );
// Wolf's howling
void howl( monster &z );
// Rattles like a rattlesnake
void rattle( monster &z );
// Spawns 2 half-worms
void worm( monster &z );
// Hallucination disappears
void disappear( monster &z );
// Morale penalty
void guilt( monster &z );
// Frees blobs, redirects to blobsplit
void brainblob( monster &z );
// Creates more blobs
void blobsplit( monster &z );
// Reverts dancers
void jackson( monster &z );
// Normal death, but melts
void melt( monster &z );
// Removes hypnosis if last one
void amigara( monster &z );
// Turn into a full thing
void thing( monster &z );
// Damaging explosion
void explode( monster &z );
// Blinding ray
void focused_beam( monster &z );
// Broken robot drop
void broken( monster &z );
// Cure verminitis
void ratking( monster &z );
// Sight returns to normal
void darkman( monster &z );
// Explodes in toxic gas
void gas( monster &z );
// All breathers die
void kill_breathers( monster &z );
// Explode like a huge smoke bomb.
void smokeburst( monster &z );
// Explode like a huge tear gas bomb.
void tearburst( monster &z );
// Explode releasing fungal haze.
void fungalburst( monster &z );
// Snicker-snack!
void jabberwock( monster &z );
// Breaks ammo and then itself
void broken_ammo( monster &z );
// Spawns 1-3 roach nymphs
void preg_roach( monster &z );
// Explodes in fire
void fireball( monster &z );
// Similar to above but bigger and guaranteed.
void conflagration( monster &z );
// raises and then upgrades all zombies in a radius
void necro_boomer( monster &z );

// Game over!  Defense mode
void gameover( monster &z );
} //namespace mdeath

void make_mon_corpse( monster &z, int damageLvl );

#endif // CATA_SRC_MONDEATH_H
