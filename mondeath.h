#ifndef _MONDEATH_H_
#define _MONDEATH_H_

class game;
class monster;

class mdeath
{
public:
    void normal         (game *g, monster *z); // Drop a body
    void acid           (game *g, monster *z); // Acid instead of a body
    void boomer         (game *g, monster *z); // Explodes in vomit :3
    void kill_vines     (game *g, monster *z); // Kill all nearby vines
    void vine_cut       (game *g, monster *z); // Kill adjacent vine if it's cut
    void triffid_heart  (game *g, monster *z); // Destroy all roots
    void fungus         (game *g, monster *z); // Explodes in spores D:
    void disintegrate   (game *g, monster *z); // Falls apart
    void shriek         (game *g, monster *z); // Screams loudly
    void rattle         (game *g, monster *z); // Rattles like a rattlesnake
    void worm           (game *g, monster *z); // Spawns 2 half-worms
    void disappear      (game *g, monster *z); // Hallucination disappears
    void guilt          (game *g, monster *z); // Morale penalty
    void blobsplit      (game *g, monster *z); // Creates more blobs
    void melt           (game *g, monster *z); // Normal death, but melts
    void amigara        (game *g, monster *z); // Removes hypnosis if last one
    void thing          (game *g, monster *z); // Turn into a full thing
    void explode        (game *g, monster *z); // Damaging explosion
    void ratking        (game *g, monster *z); // Cure verminitis
    void kill_breathers (game *g, monster *z); // All breathers die
    void smokeburst     (game *g, monster *z); // Explode like a huge smoke bomb.
    void zombie         (game *g, monster *z); // generate proper clothing for zombies

    void gameover       (game *g, monster *z); // Game over!  Defense mode
};

void make_mon_corpse(game* g, monster* z, int damageLvl);
void make_gibs(game* g, monster* z, int amount);

#endif
