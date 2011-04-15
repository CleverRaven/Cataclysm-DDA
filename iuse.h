#ifndef _IUSE_H_
#define _IUSE_H_

class game;
class item;
class player;

class iuse
{
 public:
  void none		(game *g, player *p, item *it, bool t) { };
// FOOD AND DRUGS (ADMINISTRATION)
  void royal_jelly	(game *g, player *p, item *it, bool t);
  void bandage		(game *g, player *p, item *it, bool t);
  void firstaid		(game *g, player *p, item *it, bool t);
  void caff		(game *g, player *p, item *it, bool t);
  void alcohol		(game *g, player *p, item *it, bool t);
  void pkill_1		(game *g, player *p, item *it, bool t);
  void pkill_2		(game *g, player *p, item *it, bool t);
  void pkill_3		(game *g, player *p, item *it, bool t);
  void pkill_4		(game *g, player *p, item *it, bool t);
  void pkill_l		(game *g, player *p, item *it, bool t);
  void xanax		(game *g, player *p, item *it, bool t);
  void cig		(game *g, player *p, item *it, bool t);
  void weed		(game *g, player *p, item *it, bool t);
  void coke		(game *g, player *p, item *it, bool t);
  void meth		(game *g, player *p, item *it, bool t);
  void poison		(game *g, player *p, item *it, bool t);
  void hallu		(game *g, player *p, item *it, bool t);
  void thorazine	(game *g, player *p, item *it, bool t);
  void prozac		(game *g, player *p, item *it, bool t);
  void sleep		(game *g, player *p, item *it, bool t);
  void iodine		(game *g, player *p, item *it, bool t);
  void flumed		(game *g, player *p, item *it, bool t);
  void flusleep		(game *g, player *p, item *it, bool t);
  void inhaler		(game *g, player *p, item *it, bool t);
  void blech		(game *g, player *p, item *it, bool t);
  void mutagen		(game *g, player *p, item *it, bool t);
  void purifier		(game *g, player *p, item *it, bool t);
// TOOLS
  void lighter		(game *g, player *p, item *it, bool t);
  void sew		(game *g, player *p, item *it, bool t);
  void scissors		(game *g, player *p, item *it, bool t);
  void extinguisher	(game *g, player *p, item *it, bool t);
  void hammer		(game *g, player *p, item *it, bool t);
  void light_off	(game *g, player *p, item *it, bool t);
  void light_on		(game *g, player *p, item *it, bool t);
  void water_purifier	(game *g, player *p, item *it, bool t);
  void two_way_radio	(game *g, player *p, item *it, bool t);
  void radio_off	(game *g, player *p, item *it, bool t);
  void radio_on		(game *g, player *p, item *it, bool t);
  void crowbar		(game *g, player *p, item *it, bool t);
  void makemound	(game *g, player *p, item *it, bool t);
  void dig		(game *g, player *p, item *it, bool t);
  void chainsaw_off	(game *g, player *p, item *it, bool t);
  void chainsaw_on	(game *g, player *p, item *it, bool t);
  void jackhammer	(game *g, player *p, item *it, bool t);
  void set_trap		(game *g, player *p, item *it, bool t);
  void geiger		(game *g, player *p, item *it, bool t);
  void teleport		(game *g, player *p, item *it, bool t);
  void can_goo		(game *g, player *p, item *it, bool t);
  void pipebomb		(game *g, player *p, item *it, bool t);
  void pipebomb_act	(game *g, player *p, item *it, bool t);
  void grenade		(game *g, player *p, item *it, bool t);
  void grenade_act	(game *g, player *p, item *it, bool t);
  void EMPbomb		(game *g, player *p, item *it, bool t);
  void EMPbomb_act	(game *g, player *p, item *it, bool t);
  void gasbomb		(game *g, player *p, item *it, bool t);
  void gasbomb_act	(game *g, player *p, item *it, bool t);
  void smokebomb	(game *g, player *p, item *it, bool t);
  void smokebomb_act	(game *g, player *p, item *it, bool t);
  void molotov		(game *g, player *p, item *it, bool t);
  void molotov_lit	(game *g, player *p, item *it, bool t);
  void dynamite		(game *g, player *p, item *it, bool t);
  void dynamite_act	(game *g, player *p, item *it, bool t);
  void mininuke		(game *g, player *p, item *it, bool t);
  void mininuke_act	(game *g, player *p, item *it, bool t);
  void pheromone	(game *g, player *p, item *it, bool t);
  void portal		(game *g, player *p, item *it, bool t);
  void manhack		(game *g, player *p, item *it, bool t);
  void UPS_off		(game *g, player *p, item *it, bool t);
  void UPS_on		(game *g, player *p, item *it, bool t);
  void tazer		(game *g, player *p, item *it, bool t);
  void mp3		(game *g, player *p, item *it, bool t);
  void mp3_on		(game *g, player *p, item *it, bool t);
// ARTIFACTS
  void heal		(game *g, player *p, item *it, bool t);
  void twist_space	(game *g, player *p, item *it, bool t);
  void mass_vampire	(game *g, player *p, item *it, bool t);
  void growth		(game *g, player *p, item *it, bool t);
  void water		(game *g, player *p, item *it, bool t);
  void lava		(game *g, player *p, item *it, bool t);
  
};

#endif
