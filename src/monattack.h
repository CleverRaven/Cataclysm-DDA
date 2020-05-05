#pragma once
#ifndef CATA_SRC_MONATTACK_H
#define CATA_SRC_MONATTACK_H

class monster;
class Creature;

namespace mattack
{
bool none( monster *z );
bool eat_crop( monster *z );
bool eat_food( monster *z );
bool antqueen( monster *z );
bool shriek( monster *z );
bool shriek_alert( monster *z );
bool shriek_stun( monster *z );
bool howl( monster *z );
bool rattle( monster *z );
bool acid( monster *z );
bool acid_accurate( monster *z );
bool acid_barf( monster *z );
bool shockstorm( monster *z );
bool shocking_reveal( monster *z );
bool pull_metal_weapon( monster *z );
bool boomer( monster *z );
bool boomer_glow( monster *z );
bool resurrect( monster *z );
bool smash( monster *z );
void smash_specific( monster *z, Creature *target );
bool science( monster *z );
bool growplants( monster *z );
bool grow_vine( monster *z );
bool vine( monster *z );
bool spit_sap( monster *z );
bool triffid_heartbeat( monster *z );
bool fungus( monster *z );            // Generic fungal spore-launch
bool fungus_corporate( monster *z );   // Used by Crazy Cataclysm; spawns SpOreos(tm).
bool fungus_haze( monster *z );       // Broadly scatter aerobics
bool fungus_big_blossom( monster *z ); // Aerobic & anaerobic, as needed
bool fungus_inject( monster *z );     // Directly inject the spores
bool fungus_bristle( monster *z );    // Fungal hedgerow rake & inject
bool fungus_growth( monster *z );     // Sporeling -> fungal creature
bool fungus_sprout( monster *z );     // Grow fungal walls
bool fungus_fortify( monster *z );    // Grow fungal hedgerows
bool impale( monster *z );
bool dermatik( monster *z );
bool dermatik_growth( monster *z );
bool fungal_trail( monster *z );
bool plant( monster *z );
bool disappear( monster *z );
bool formblob( monster *z );
bool callblobs( monster *z );
bool jackson( monster *z );
bool dance( monster *z );
bool dogthing( monster *z );
bool tentacle( monster *z );
bool vortex( monster *z );
bool gene_sting( monster *z );
bool para_sting( monster *z );
bool triffid_growth( monster *z );
bool stare( monster *z );
bool fear_paralyze( monster *z );
bool nurse_check_up( monster *z );
bool nurse_assist( monster *z );
bool nurse_operate( monster *z );
bool check_money_left( monster *z );
bool photograph( monster *z );
bool tazer( monster *z );
bool flamethrower( monster *z );
bool searchlight( monster *z );
bool copbot( monster *z );
bool chickenbot( monster *z );        // Pick from tazer, M4, MGL
bool multi_robot( monster *z );       // Tazer, flame, M4, MGL, or 120mm!
bool ratking( monster *z );
bool generator( monster *z );
bool upgrade( monster *z );
bool breathe( monster *z );
bool brandish( monster *z );
bool flesh_golem( monster *z );
bool absorb_meat( monster *z );
bool lunge( monster *z );
bool longswipe( monster *z );
bool parrot( monster *z );
bool parrot_at_danger( monster *parrot );
bool darkman( monster *z );
bool slimespring( monster *z );
bool evolve_kill_strike( monster *z );
bool leech_spawner( monster *z );
bool mon_leech_evolution( monster *z );
bool tindalos_teleport( monster *z );
bool flesh_tendril( monster *z );
bool bio_op_random_biojutsu( monster *z );
bool bio_op_takedown( monster *z );
bool bio_op_impale( monster *z );
bool bio_op_disarm( monster *z );
bool ranged_pull( monster *z );
bool grab( monster *z );
bool grab_drag( monster *z );
bool suicide( monster *z );
bool thrown_by_judo( monster *z );    //handles zombie getting thrown when u.is_throw_immune()
bool riotbot( monster *z );
bool stretch_attack( monster *z );
bool stretch_bite( monster *z );
bool kamikaze( monster *z );
bool grenadier( monster *z );
bool grenadier_elite( monster *z );
bool doot( monster *z );
bool zombie_fuse( monster *z );

void taze( monster *z, Creature *target );
void rifle( monster *z, Creature *target );             // Automated M4
void frag( monster *z, Creature *target );              // Automated MGL
void tankgun( monster *z, Creature *target );           // Tankbot primary.
void flame( monster *z, Creature *target );

bool dodge_check( monster *z, Creature *target );
} //namespace mattack

#endif // CATA_SRC_MONATTACK_H
