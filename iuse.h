#ifndef _IUSE_H_
#define _IUSE_H_

class game;
class item;
class player;



class iuse
{
 public:
  void none		(game *g, player *p, item *it, bool t);

// FOOD AND DRUGS (ADMINISTRATION)
  void sewage		(game *g, player *p, item *it, bool t);
  void honeycomb        (game *g, player *p, item *it, bool t);
  void royal_jelly	(game *g, player *p, item *it, bool t);
  void bandage		(game *g, player *p, item *it, bool t);
  void firstaid		(game *g, player *p, item *it, bool t);
  void disinfectant (game *g, player *p, item *it, bool t);
  void caff		(game *g, player *p, item *it, bool t);
  void alcohol		(game *g, player *p, item *it, bool t);
  void alcohol_weak (game *g, player *p, item *it, bool t);
  void pkill		(game *g, player *p, item *it, bool t);
  void xanax		(game *g, player *p, item *it, bool t);
  void cig		(game *g, player *p, item *it, bool t);
  void antibiotic		(game *g, player *p, item *it, bool t);
  void weed		(game *g, player *p, item *it, bool t);
  void coke		(game *g, player *p, item *it, bool t);
  void crack		(game *g, player *p, item *it, bool t);
  void grack		(game *g, player *p, item *it, bool t);
  void meth		(game *g, player *p, item *it, bool t);
  void vitamins		(game *g, player *p, item *it, bool t);
  void vaccine		(game *g, player *p, item *it, bool t);
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
  void marloss		(game *g, player *p, item *it, bool t);
  void dogfood		(game *g, player *p, item *it, bool t);

// TOOLS
  void lighter		(game *g, player *p, item *it, bool t);
  void primitive_fire(game *g, player *p, item *it, bool t);
  void sew		(game *g, player *p, item *it, bool t);
  void extra_battery(game *g, player *p, item *it, bool t);
  void scissors		(game *g, player *p, item *it, bool t);
  void extinguisher	(game *g, player *p, item *it, bool t);
  void hammer		(game *g, player *p, item *it, bool t);
  void light_off	(game *g, player *p, item *it, bool t);
  void light_on		(game *g, player *p, item *it, bool t);
  void gasoline_lantern_off	(game *g, player *p, item *it, bool t);
  void gasoline_lantern_on	(game *g, player *p, item *it, bool t);
  void lightstrip		(game *g, player *p, item *it, bool t);
  void lightstrip_active(game *g, player *p, item *it, bool t);
  void glowstick		(game *g, player *p, item *it, bool t);
  void glowstick_active(game *g, player *p, item *it, bool t);
  void cauterize_elec	(game *g, player *p, item *it, bool t);
  void solder_weld	    (game *g, player *p, item *it, bool t);
  void water_purifier	(game *g, player *p, item *it, bool t);
  void two_way_radio	(game *g, player *p, item *it, bool t);
  void directional_antenna	(game *g, player *p, item *it, bool t);
  void radio_off	(game *g, player *p, item *it, bool t);
  void radio_on		(game *g, player *p, item *it, bool t);
  void noise_emitter_off(game *g, player *p, item *it, bool t);
  void noise_emitter_on (game *g, player *p, item *it, bool t);
  void roadmap (game *g, player *p, item *it, bool t);
  void roadmap_a_target (game *g, player *p, item *it, bool t, int target);
  void roadmap_targets(game *g, player *p, item *it, bool t, int target, int target_range, int distance, int reveal_distance);
  void picklock         (game *g, player *p, item *it, bool t);
  void crowbar		(game *g, player *p, item *it, bool t);
  void makemound	(game *g, player *p, item *it, bool t);
  void dig		(game *g, player *p, item *it, bool t);
  void siphon	(game *g, player *p, item *it, bool t);
  void chainsaw_off	(game *g, player *p, item *it, bool t);
  void chainsaw_on	(game *g, player *p, item *it, bool t);
  void shishkebab_off	(game *g, player *p, item *it, bool t);
  void shishkebab_on	(game *g, player *p, item *it, bool t);
  void firemachete_off	(game *g, player *p, item *it, bool t);
  void firemachete_on	(game *g, player *p, item *it, bool t);
  void broadfire_off	(game *g, player *p, item *it, bool t);
  void broadfire_on	    (game *g, player *p, item *it, bool t);
  void firekatana_off	(game *g, player *p, item *it, bool t);
  void firekatana_on	(game *g, player *p, item *it, bool t);
  void jackhammer	(game *g, player *p, item *it, bool t);
  void jacqueshammer	(game *g, player *p, item *it, bool t);
  void pickaxe          (game *g, player *p, item *it, bool t);
  void set_trap		(game *g, player *p, item *it, bool t);
  void geiger		(game *g, player *p, item *it, bool t);
  void teleport		(game *g, player *p, item *it, bool t);
  void can_goo		(game *g, player *p, item *it, bool t);
  void pipebomb		(game *g, player *p, item *it, bool t);
  void pipebomb_act	(game *g, player *p, item *it, bool t);
  void grenade		(game *g, player *p, item *it, bool t);
  void grenade_act	(game *g, player *p, item *it, bool t);
  void flashbang	(game *g, player *p, item *it, bool t);
  void flashbang_act	(game *g, player *p, item *it, bool t);
  void c4    		(game *g, player *p, item *it, bool t);
  void c4armed  	(game *g, player *p, item *it, bool t);
  void EMPbomb		(game *g, player *p, item *it, bool t);
  void EMPbomb_act	(game *g, player *p, item *it, bool t);
  void scrambler	(game *g, player *p, item *it, bool t);
  void scrambler_act	(game *g, player *p, item *it, bool t);
  void gasbomb		(game *g, player *p, item *it, bool t);
  void gasbomb_act	(game *g, player *p, item *it, bool t);
  void smokebomb	(game *g, player *p, item *it, bool t);
  void smokebomb_act	(game *g, player *p, item *it, bool t);
  void acidbomb		(game *g, player *p, item *it, bool t);
  void arrow_flamable (game *g, player *p, item *it, bool t);
  void acidbomb_act	(game *g, player *p, item *it, bool t);
  void molotov		(game *g, player *p, item *it, bool t);
  void molotov_lit	(game *g, player *p, item *it, bool t);
  void dynamite		(game *g, player *p, item *it, bool t);
  void dynamite_act	(game *g, player *p, item *it, bool t);
  void firecracker_pack (game *g, player *p, item *it, bool t);
  void firecracker_pack_act (game *g, player *p, item *it, bool t);
  void firecracker (game *g, player *p, item *it, bool t);
  void firecracker_act (game *g, player *p, item *it, bool t);
  void mininuke		(game *g, player *p, item *it, bool t);
  void mininuke_act	(game *g, player *p, item *it, bool t);
  void pheromone	(game *g, player *p, item *it, bool t);
  void portal		(game *g, player *p, item *it, bool t);
  void manhack		(game *g, player *p, item *it, bool t);
  void turret		(game *g, player *p, item *it, bool t);
  void UPS_off		(game *g, player *p, item *it, bool t);
  void UPS_on		(game *g, player *p, item *it, bool t);
  void adv_UPS_off		(game *g, player *p, item *it, bool t);
  void adv_UPS_on		(game *g, player *p, item *it, bool t);
  void tazer		(game *g, player *p, item *it, bool t);
  void mp3		(game *g, player *p, item *it, bool t);
  void mp3_on		(game *g, player *p, item *it, bool t);
  void portable_game    (game *g, player *p, item *it, bool t);
  void vortex		(game *g, player *p, item *it, bool t);
  void dog_whistle	(game *g, player *p, item *it, bool t);
  void vacutainer	(game *g, player *p, item *it, bool t);
  void knife    	(game *g, player *p, item *it, bool t);
  static void cut_log_into_planks(game *g, player *p, item *it);
  void lumber    	(game *g, player *p, item *it, bool t);
  void hacksaw          (game *g, player *p, item *it, bool t);
  void tent             (game *g, player *p, item *it, bool t);
  void shelter          (game *g, player *p, item *it, bool t);
  void torch            (game *g, player *p, item *it, bool t);
  void torch_lit        (game *g, player *p, item *it, bool t);
  void candle           (game *g, player *p, item *it, bool t);
  void candle_lit       (game *g, player *p, item *it, bool t);
  void bullet_puller	(game *g, player *p, item *it, bool t);
  void boltcutters      (game *g, player *p, item *it, bool t);
  void mop              (game *g, player *p, item *it, bool t);
  void spray_can        (game *g, player *p, item *it, bool t);
  void rag              (game *g, player *p, item *it, bool t);
  void pda              (game *g, player *p, item *it, bool t);
  void pda_flashlight   (game *g, player *p, item *it, bool t);
  void LAW              (game *g, player *p, item *it, bool t);
  void heatpack			(game *g, player *p, item *it, bool t);
  void dejar            (game *g, player *p, item *it, bool t);
  void rad_badge        (game *g, player *p, item *it, bool t);
  void boots            (game *g, player *p, item *it, bool t);
  void towel            (game *g, player *p, item *it, bool t);
  void unfold_bicycle        (game *g, player *p, item *it, bool t);
  void adrenaline_injector (game *g, player *p, item *it, bool t);
// MACGUFFINS
  void mcg_note		(game *g, player *p, item *it, bool t);
// ARTIFACTS
// This function is used when an artifact is activated
// It examines the item's artifact-specific properties
// See artifact.h for a list
  void artifact		(game *g, player *p, item *it, bool t);

};

#endif
