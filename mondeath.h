#ifndef _MONDEATH_H_
#define _MONDEATH_H_

class game;
class monster;

class mdeath
{
public:
 void normal		(game *g, monster *z); // Drop a body
 void acid		(game *g, monster *z); // Acid instead of a body
 void boomer		(game *g, monster *z); // Explodes in vomit :3
 void kill_vines	(game *g, monster *z); // Kill all nearby vines
 void vine_cut		(game *g, monster *z); // Kill adjacent vine if it's cut
 void triffid_heart	(game *g, monster *z); // Destroy all roots
 void fungus		(game *g, monster *z); // Explodes in spores D:
 void fungusawake	(game *g, monster *z); // Turn into live fungaloid
 void disintegrate	(game *g, monster *z); // Falls apart
 void shriek		(game *g, monster *z); // Screams loudly
 void worm		(game *g, monster *z); // Spawns 2 half-worms
 void disappear		(game *g, monster *z); // Hallucination disappears
 void guilt		(game *g, monster *z); // Morale penalty
 void blobsplit		(game *g, monster *z); // Creates more blobs
 void melt		(game *g, monster *z); // Normal death, but melts
 void amigara		(game *g, monster *z); // Removes hypnosis if last one
 void thing		(game *g, monster *z); // Turn into a full thing
 void explode		(game *g, monster *z); // Damaging explosion
 void ratking		(game *g, monster *z); // Cure verminitis
 void kill_breathers	(game *g, monster *z); // All breathers die

 void gameover		(game *g, monster *z); // Game over!  Defense mode
};

#endif
