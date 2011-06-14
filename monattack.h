#ifndef _MONATTACK_H_
#define _MONATTACK_H_

//#include "game.h"

class game;

class mattack
{
 public:
  void none		(game *g, monster *z) { };
  void antqueen		(game *g, monster *z);
  void shriek		(game *g, monster *z);
  void acid		(game *g, monster *z);
  void shockstorm	(game *g, monster *z);
  void boomer		(game *g, monster *z);
  void resurrect        (game *g, monster *z);
  void science		(game *g, monster *z);
  void growplants	(game *g, monster *z);
  void fungus		(game *g, monster *z);
  void leap		(game *g, monster *z);
  void plant		(game *g, monster *z);
  void disappear	(game *g, monster *z);
  void formblob		(game *g, monster *z);
  void gene_sting	(game *g, monster *z);
  void stare		(game *g, monster *z);
  void fear_paralyze	(game *g, monster *z);
  void photograph	(game *g, monster *z) { };	// TODO: needs faction
  void tazer		(game *g, monster *z);
  void smg		(game *g, monster *z);
  void flamethrower	(game *g, monster *z);
  void multi_robot	(game *g, monster *z); // Pick from tazer, smg, flame
};

#endif
