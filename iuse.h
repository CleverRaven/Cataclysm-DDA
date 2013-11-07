#ifndef _IUSE_H_
#define _IUSE_H_

class game;
class item;
class player;

// iuse methods returning a bool indicating whether to consume a charge of the item being used.
class iuse
{
 public:
  int none             (game *g, player *p, item *it, bool t);

// FOOD AND DRUGS (ADMINISTRATION)
  int sewage           (game *g, player *p, item *it, bool t);
  int honeycomb        (game *g, player *p, item *it, bool t);
  int royal_jelly      (game *g, player *p, item *it, bool t);
  int bandage          (game *g, player *p, item *it, bool t);
  int firstaid         (game *g, player *p, item *it, bool t);
  int disinfectant     (game *g, player *p, item *it, bool t);
  int caff             (game *g, player *p, item *it, bool t);
  int alcohol          (game *g, player *p, item *it, bool t);
  int alcohol_weak     (game *g, player *p, item *it, bool t);
  int pkill            (game *g, player *p, item *it, bool t);
  int xanax            (game *g, player *p, item *it, bool t);
  int cig              (game *g, player *p, item *it, bool t);
  int antibiotic       (game *g, player *p, item *it, bool t);
  int fungicide        (game *g, player *p, item *it, bool t);
  int weed             (game *g, player *p, item *it, bool t);
  int coke             (game *g, player *p, item *it, bool t);
  int crack            (game *g, player *p, item *it, bool t);
  int grack            (game *g, player *p, item *it, bool t);
  int meth             (game *g, player *p, item *it, bool t);
  int vitamins         (game *g, player *p, item *it, bool t);
  int vaccine          (game *g, player *p, item *it, bool t);
  int poison           (game *g, player *p, item *it, bool t);
  int hallu            (game *g, player *p, item *it, bool t);
  int thorazine        (game *g, player *p, item *it, bool t);
  int prozac           (game *g, player *p, item *it, bool t);
  int sleep            (game *g, player *p, item *it, bool t);
  int iodine           (game *g, player *p, item *it, bool t);
  int flumed           (game *g, player *p, item *it, bool t);
  int flusleep         (game *g, player *p, item *it, bool t);
  int inhaler          (game *g, player *p, item *it, bool t);
  int blech            (game *g, player *p, item *it, bool t);
  int chew            (game *g, player *p, item *it, bool t);
  int mutagen          (game *g, player *p, item *it, bool t);
  int purifier         (game *g, player *p, item *it, bool t);
  int marloss          (game *g, player *p, item *it, bool t);
  int dogfood          (game *g, player *p, item *it, bool t);
  int catfood          (game *g, player *p, item *it, bool t);

// TOOLS
  int lighter          (game *g, player *p, item *it, bool t);
  int primitive_fire   (game *g, player *p, item *it, bool t);
  int sew              (game *g, player *p, item *it, bool t);
  int extra_battery    (game *g, player *p, item *it, bool t);
  int cut_up           (game *g, player *p, item *it, item *cut, bool t);
  int scissors         (game *g, player *p, item *it, bool t);
  int extinguisher     (game *g, player *p, item *it, bool t);
  int hammer           (game *g, player *p, item *it, bool t);
  int light_off        (game *g, player *p, item *it, bool t);
  int light_on         (game *g, player *p, item *it, bool t);
  int gasoline_lantern_off (game *g, player *p, item *it, bool t);
  int gasoline_lantern_on  (game *g, player *p, item *it, bool t);
  int lightstrip       (game *g, player *p, item *it, bool t);
  int lightstrip_active(game *g, player *p, item *it, bool t);
  int glowstick        (game *g, player *p, item *it, bool t);
  int glowstick_active (game *g, player *p, item *it, bool t);
  int solder_weld      (game *g, player *p, item *it, bool t);
  int water_purifier   (game *g, player *p, item *it, bool t);
  int two_way_radio    (game *g, player *p, item *it, bool t);
  int directional_antenna (game *g, player *p, item *it, bool t);
  int radio_off        (game *g, player *p, item *it, bool t);
  int radio_on         (game *g, player *p, item *it, bool t);
  int horn_bicycle     (game *g, player *p, item *it, bool t);
  int noise_emitter_off(game *g, player *p, item *it, bool t);
  int noise_emitter_on (game *g, player *p, item *it, bool t);
  int roadmap          (game *g, player *p, item *it, bool t);
  int picklock         (game *g, player *p, item *it, bool t);
  int crowbar          (game *g, player *p, item *it, bool t);
  int makemound        (game *g, player *p, item *it, bool t);
  int dig              (game *g, player *p, item *it, bool t);
  int siphon           (game *g, player *p, item *it, bool t);
  int chainsaw_off     (game *g, player *p, item *it, bool t);
  int chainsaw_on      (game *g, player *p, item *it, bool t);
  int shishkebab_off   (game *g, player *p, item *it, bool t);
  int shishkebab_on    (game *g, player *p, item *it, bool t);
  int firemachete_off  (game *g, player *p, item *it, bool t);
  int firemachete_on   (game *g, player *p, item *it, bool t);
  int broadfire_off    (game *g, player *p, item *it, bool t);
  int broadfire_on     (game *g, player *p, item *it, bool t);
  int firekatana_off   (game *g, player *p, item *it, bool t);
  int firekatana_on    (game *g, player *p, item *it, bool t);
  int zweifire_off     (game *g, player *p, item *it, bool t);
  int zweifire_on      (game *g, player *p, item *it, bool t);
  int jackhammer       (game *g, player *p, item *it, bool t);
  int jacqueshammer    (game *g, player *p, item *it, bool t);
  int pickaxe          (game *g, player *p, item *it, bool t);
  int set_trap         (game *g, player *p, item *it, bool t);
  int geiger           (game *g, player *p, item *it, bool t);
  int teleport         (game *g, player *p, item *it, bool t);
  int can_goo          (game *g, player *p, item *it, bool t);
  int pipebomb         (game *g, player *p, item *it, bool t);
  int pipebomb_act     (game *g, player *p, item *it, bool t);
  int grenade          (game *g, player *p, item *it, bool t);
  int grenade_act      (game *g, player *p, item *it, bool t);
  int granade          (game *g, player *p, item *it, bool t);
  int granade_act      (game *g, player *p, item *it, bool t);
  int flashbang        (game *g, player *p, item *it, bool t);
  int flashbang_act    (game *g, player *p, item *it, bool t);
  int c4               (game *g, player *p, item *it, bool t);
  int c4armed          (game *g, player *p, item *it, bool t);
  int EMPbomb          (game *g, player *p, item *it, bool t);
  int EMPbomb_act      (game *g, player *p, item *it, bool t);
  int scrambler        (game *g, player *p, item *it, bool t);
  int scrambler_act    (game *g, player *p, item *it, bool t);
  int gasbomb          (game *g, player *p, item *it, bool t);
  int gasbomb_act      (game *g, player *p, item *it, bool t);
  int smokebomb        (game *g, player *p, item *it, bool t);
  int smokebomb_act    (game *g, player *p, item *it, bool t);
  int acidbomb         (game *g, player *p, item *it, bool t);
  int arrow_flamable   (game *g, player *p, item *it, bool t);
  int acidbomb_act     (game *g, player *p, item *it, bool t);
  int molotov          (game *g, player *p, item *it, bool t);
  int molotov_lit      (game *g, player *p, item *it, bool t);
  int matchbomb        (game *g, player *p, item *it, bool t);
  int matchbomb_act    (game *g, player *p, item *it, bool t);
  int dynamite         (game *g, player *p, item *it, bool t);
  int dynamite_act     (game *g, player *p, item *it, bool t);
  int firecracker_pack (game *g, player *p, item *it, bool t);
  int firecracker_pack_act (game *g, player *p, item *it, bool t);
  int firecracker      (game *g, player *p, item *it, bool t);
  int firecracker_act  (game *g, player *p, item *it, bool t);
  int mininuke         (game *g, player *p, item *it, bool t);
  int mininuke_act     (game *g, player *p, item *it, bool t);
  int pheromone        (game *g, player *p, item *it, bool t);
  int portal           (game *g, player *p, item *it, bool t);
  int manhack          (game *g, player *p, item *it, bool t);
  int turret           (game *g, player *p, item *it, bool t);
  int UPS_off          (game *g, player *p, item *it, bool t);
  int UPS_on           (game *g, player *p, item *it, bool t);
  int adv_UPS_off      (game *g, player *p, item *it, bool t);
  int adv_UPS_on       (game *g, player *p, item *it, bool t);
  int tazer            (game *g, player *p, item *it, bool t);
  int tazer2           (game *g, player *p, item *it, bool t);
  int shocktonfa_off   (game *g, player *p, item *it, bool t);
  int shocktonfa_on    (game *g, player *p, item *it, bool t);
  int mp3              (game *g, player *p, item *it, bool t);
  int mp3_on           (game *g, player *p, item *it, bool t);
  int portable_game    (game *g, player *p, item *it, bool t);
  int vortex           (game *g, player *p, item *it, bool t);
  int dog_whistle      (game *g, player *p, item *it, bool t);
  int vacutainer       (game *g, player *p, item *it, bool t);
  int knife            (game *g, player *p, item *it, bool t);
  static int cut_log_into_planks(game *g, player *p, item *it);
  int lumber           (game *g, player *p, item *it, bool t);
  int hacksaw          (game *g, player *p, item *it, bool t);
  int tent             (game *g, player *p, item *it, bool t);
  int shelter          (game *g, player *p, item *it, bool t);
  int torch            (game *g, player *p, item *it, bool t);
  int torch_lit        (game *g, player *p, item *it, bool t);
  int handflare        (game *g, player *p, item *it, bool t);
  int handflare_lit    (game *g, player *p, item *it, bool t);
  int battletorch      (game *g, player *p, item *it, bool t);
  int battletorch_lit  (game *g, player *p, item *it, bool t);
  int candle           (game *g, player *p, item *it, bool t);
  int candle_lit       (game *g, player *p, item *it, bool t);
  int bullet_puller    (game *g, player *p, item *it, bool t);
  int boltcutters      (game *g, player *p, item *it, bool t);
  int mop              (game *g, player *p, item *it, bool t);
  int spray_can        (game *g, player *p, item *it, bool t);
  int rag              (game *g, player *p, item *it, bool t);
  int pda              (game *g, player *p, item *it, bool t);
  int pda_flashlight   (game *g, player *p, item *it, bool t);
  int LAW              (game *g, player *p, item *it, bool t);
  int heatpack         (game *g, player *p, item *it, bool t);
  int hotplate         (game *g, player *p, item *it, bool t);
  int dejar            (game *g, player *p, item *it, bool t);
  int rad_badge        (game *g, player *p, item *it, bool t);
  int boots            (game *g, player *p, item *it, bool t);
  int towel            (game *g, player *p, item *it, bool t);
  int unfold_bicycle   (game *g, player *p, item *it, bool t);
  int airhorn          (game *g, player *p, item *it, bool t);
  int adrenaline_injector (game *g, player *p, item *it, bool t);
// MACGUFFINS
  int mcg_note         (game *g, player *p, item *it, bool t);
// ARTIFACTS
// This function is used when an artifact is activated
// It examines the item's artifact-specific properties
// See artifact.h for a list
  int artifact         (game *g, player *p, item *it, bool t);
};


typedef int (iuse::*use_function_pointer)(game*,player*,item*,bool);

struct use_function {
    use_function_pointer cpp_function;

    use_function() {};

    use_function(use_function_pointer f)
        : cpp_function(f)
    { };

    int call(game*,player*,item*,bool);

    void operator=(use_function_pointer f) {
        cpp_function = f;
    }

    bool operator==(use_function_pointer f) const {
        return f == cpp_function;
    }

    bool operator!=(use_function_pointer f) const {
        return f != cpp_function;
    }
};


#endif
