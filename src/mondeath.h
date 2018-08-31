#pragma once
#ifndef MONDEATH_H
#define MONDEATH_H

class monster;

namespace mdeath
{
void normal( monster &z );        // Drop a body
void acid( monster &z );          // Acid instead of a body
void boomer( monster &z );        // Explodes in vomit :3
void boomer_glow( monster &z );   // Explodes in vomit :3
void kill_vines( monster &z );    // Kill all nearby vines
void vine_cut( monster &z );      // Kill adjacent vine if it's cut
void triffid_heart( monster &z ); // Destroy all roots
void fungus( monster &z );        // Explodes in spores D:
void disintegrate( monster &z );  // Falls apart
void shriek( monster &z );        // Screams loudly
void howl( monster &z );          // Wolf's howling
void rattle( monster &z );        // Rattles like a rattlesnake
void worm( monster &z );          // Spawns 2 half-worms
void disappear( monster &z );     // Hallucination disappears
void guilt( monster &z );         // Morale penalty
void brainblob( monster &z );     // Frees blobs, redirects to blobsplit
void blobsplit( monster &z );     // Creates more blobs
void jackson( monster &z );       // Reverts dancers
void melt( monster &z );          // Normal death, but melts
void amigara( monster &z );       // Removes hypnosis if last one
void thing( monster &z );         // Turn into a full thing
void explode( monster &z );       // Damaging explosion
void focused_beam( monster &z );  // blinding ray
void broken( monster &z );        // Broken robot drop
void ratking( monster &z );       // Cure verminitis
void darkman( monster &z );       // sight returns to normal
void gas( monster &z );           // Explodes in toxic gas
void kill_breathers( monster &z );// All breathers die
void smokeburst( monster &z );    // Explode like a huge smoke bomb.
void jabberwock( monster &z );    // Snicker-snack!
void detonate( monster &z );      // Take the enemy with you
void broken_ammo( monster &z );   // Breaks ammo and then itself
void preg_roach( monster &z );    // Spawns 1-3 roach nymphs
void fireball( monster &z );      // Explodes in fire

void gameover( monster &z );      // Game over!  Defense mode
} //namespace mdeath

void make_mon_corpse( monster &z, int damageLvl );

#endif
