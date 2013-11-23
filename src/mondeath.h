#ifndef _MONDEATH_H_
#define _MONDEATH_H_

class monster;

class mdeath
{
public:
    void normal         (monster *z); // Drop a body
    void acid           (monster *z); // Acid instead of a body
    void boomer         (monster *z); // Explodes in vomit :3
    void kill_vines     (monster *z); // Kill all nearby vines
    void vine_cut       (monster *z); // Kill adjacent vine if it's cut
    void triffid_heart  (monster *z); // Destroy all roots
    void fungus         (monster *z); // Explodes in spores D:
    void disintegrate   (monster *z); // Falls apart
    void shriek         (monster *z); // Screams loudly
    void rattle         (monster *z); // Rattles like a rattlesnake
    void worm           (monster *z); // Spawns 2 half-worms
    void disappear      (monster *z); // Hallucination disappears
    void guilt          (monster *z); // Morale penalty
    void blobsplit      (monster *z); // Creates more blobs
    void melt           (monster *z); // Normal death, but melts
    void amigara        (monster *z); // Removes hypnosis if last one
    void thing          (monster *z); // Turn into a full thing
    void explode        (monster *z); // Damaging explosion
    void broken         (monster *z);  // Broken robot drop
    void ratking        (monster *z); // Cure verminitis
    void darkman        (monster *z); // sight returns to normal
    void kill_breathers (monster *z); // All breathers die
    void smokeburst     (monster *z); // Explode like a huge smoke bomb.
    void zombie         (monster *z); // generate proper clothing for zombies

    void gameover       (monster *z); // Game over!  Defense mode
};

void make_mon_corpse(monster* z, int damageLvl);
void make_gibs(monster* z, int amount);

#endif
