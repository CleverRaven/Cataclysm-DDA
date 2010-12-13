#ifndef _IUSE_H_
#define _IUSE_H_

class game;
class item;

class iuse
{
 public:
  void none		(game *g, item *it, bool t) { };
// FOOD AND DRUGS (ADMINISTRATION)
  void royal_jelly	(game *g, item *it, bool t);
  void bandage		(game *g, item *it, bool t);
  void firstaid		(game *g, item *it, bool t);
  void caff		(game *g, item *it, bool t);
  void alcohol		(game *g, item *it, bool t);
  void pkill_1		(game *g, item *it, bool t);
  void pkill_2		(game *g, item *it, bool t);
  void pkill_3		(game *g, item *it, bool t);
  void pkill_4		(game *g, item *it, bool t);
  void pkill_l		(game *g, item *it, bool t);
  void xanax		(game *g, item *it, bool t);
  void cig		(game *g, item *it, bool t);
  void weed		(game *g, item *it, bool t);
  void coke		(game *g, item *it, bool t);
  void meth		(game *g, item *it, bool t);
  void poison		(game *g, item *it, bool t);
  void hallu		(game *g, item *it, bool t);
  void thorazine	(game *g, item *it, bool t);
  void prozac		(game *g, item *it, bool t);
  void sleep		(game *g, item *it, bool t);
  void iodine		(game *g, item *it, bool t);
  void flumed		(game *g, item *it, bool t);
  void flusleep		(game *g, item *it, bool t);
  void inhaler		(game *g, item *it, bool t);
  void blech		(game *g, item *it, bool t);
  void mutagen		(game *g, item *it, bool t);
  void purifier		(game *g, item *it, bool t);
// TOOLS
  void lighter		(game *g, item *it, bool t);
  void sew		(game *g, item *it, bool t);
  void scissors		(game *g, item *it, bool t);
  void extinguisher	(game *g, item *it, bool t);
  void hammer		(game *g, item *it, bool t);
  void light_off	(game *g, item *it, bool t);
  void light_on		(game *g, item *it, bool t);
  void water_purifier	(game *g, item *it, bool t);
  void two_way_radio	(game *g, item *it, bool t);
  void radio_off	(game *g, item *it, bool t);
  void radio_on		(game *g, item *it, bool t);
  void crowbar		(game *g, item *it, bool t);
  void makemound	(game *g, item *it, bool t);
  void dig		(game *g, item *it, bool t);
  void chainsaw_off	(game *g, item *it, bool t);
  void chainsaw_on	(game *g, item *it, bool t);
  void jackhammer	(game *g, item *it, bool t);
  void set_trap		(game *g, item *it, bool t);
  void geiger		(game *g, item *it, bool t);
  void teleport		(game *g, item *it, bool t);
  void can_goo		(game *g, item *it, bool t);
  void pipebomb		(game *g, item *it, bool t);
  void pipebomb_act	(game *g, item *it, bool t);
  void grenade		(game *g, item *it, bool t);
  void grenade_act	(game *g, item *it, bool t);
  void EMPbomb		(game *g, item *it, bool t);
  void EMPbomb_act	(game *g, item *it, bool t);
  void gasbomb		(game *g, item *it, bool t);
  void gasbomb_act	(game *g, item *it, bool t);
  void smokebomb	(game *g, item *it, bool t);
  void smokebomb_act	(game *g, item *it, bool t);
  void molotov		(game *g, item *it, bool t);
  void molotov_lit	(game *g, item *it, bool t);
  void dynamite		(game *g, item *it, bool t);
  void dynamite_act	(game *g, item *it, bool t);
  void mininuke		(game *g, item *it, bool t);
  void mininuke_act	(game *g, item *it, bool t);
  void pheromone	(game *g, item *it, bool t);
  void portal		(game *g, item *it, bool t);
  void manhack		(game *g, item *it, bool t);
  void UPS_off		(game *g, item *it, bool t);
  void UPS_on		(game *g, item *it, bool t);
  void tazer		(game *g, item *it, bool t);
  void mp3		(game *g, item *it, bool t);
  void mp3_on		(game *g, item *it, bool t);
// ARTIFACTS
  void heal		(game *g, item *it, bool t);
  void twist_space	(game *g, item *it, bool t);
  void mass_vampire	(game *g, item *it, bool t);
  void growth		(game *g, item *it, bool t);
  void water		(game *g, item *it, bool t);
  void lava		(game *g, item *it, bool t);
  
};

#endif
